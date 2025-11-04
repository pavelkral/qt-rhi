

#include "rhiwindow.h"
#include <QPlatformSurfaceEvent>
#include <QPainter>
#include <QFile>
#include <QImage>
#include <QFont>
#include <QRandomGenerator>
#include <rhi/qshader.h>
#include "qtrhi3d/geometry.h"
#include "qtrhi3d/apifuturesinfo.h"
#include <rhi/qrhi_platform.h>


//================================== RhiWindow ==================================

RhiWindow::RhiWindow(QRhi::Implementation graphicsApi)
    : m_graphicsApi(graphicsApi)
{
    switch (graphicsApi) {
    case QRhi::OpenGLES2:
        setSurfaceType(OpenGLSurface);
        break;
    case QRhi::Vulkan:
        setSurfaceType(VulkanSurface);
        break;
    case QRhi::D3D11:
    case QRhi::D3D12:
        setSurfaceType(Direct3DSurface);
        break;
    case QRhi::Metal:
        setSurfaceType(MetalSurface);
        break;
    case QRhi::Null:
        break; // RasterSurface
    }
}

QString RhiWindow::graphicsApiName() const
{
    switch (m_graphicsApi) {
    case QRhi::Null:
        return QLatin1String("Null (no output)");
    case QRhi::OpenGLES2:
        return QLatin1String("OpenGL");
    case QRhi::Vulkan:
        return QLatin1String("Vulkan");
    case QRhi::D3D11:
        return QLatin1String("Direct3D 11");
    case QRhi::D3D12:
        return QLatin1String("Direct3D 12");
    case QRhi::Metal:
        return QLatin1String("Metal");
    }
    return QString();
}

void RhiWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed() && !m_initialized) {
        init();
        resizeSwapChain();
        m_initialized = true;
    }

    const QSize surfaceSize = m_hasSwapChain ? m_sc->surfacePixelSize() : QSize();

    if ((!isExposed() || (m_hasSwapChain && surfaceSize.isEmpty())) && m_initialized && !m_notExposed)
        m_notExposed = true;

    if (isExposed() && m_initialized && m_notExposed && !surfaceSize.isEmpty()) {
        m_notExposed = false;
        m_newlyExposed = true;
    }

    if (isExposed() && !surfaceSize.isEmpty())
        render();
}

bool RhiWindow::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        render();
        break;
    case QEvent::PlatformSurface:
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            releaseSwapChain();
        break;
    default:
        break;
    }
    return QWindow::event(e);
}

void RhiWindow::init()
{
    QRhi::Flags rhiFlags = QRhi::EnableDebugMarkers;
    if (m_graphicsApi == QRhi::Null) {
        QRhiNullInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Null, &params,rhiFlags));
    }

#if QT_CONFIG(opengl)
    if (m_graphicsApi == QRhi::OpenGLES2) {
        m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
        QRhiGles2InitParams params;
        params.fallbackSurface = m_fallbackSurface.get();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params,rhiFlags));
    }
#endif

#if QT_CONFIG(vulkan)
    if (m_graphicsApi == QRhi::Vulkan) {
        QRhiVulkanInitParams params;
        params.inst = vulkanInstance();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::Vulkan, &params,rhiFlags));
    }
#endif

#ifdef Q_OS_WIN
    if (m_graphicsApi == QRhi::D3D11) {
     QRhiD3D11InitParams params;
       params.enableDebugLayer = true;
       m_rhi.reset(QRhi::create(QRhi::D3D11, &params,rhiFlags));
     //   QRhiVulkanInitParams params;
    //    params.inst = vulkanInstance();
     //   params.window = this;
     //   m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));

    } else if (m_graphicsApi == QRhi::D3D12) {
        QRhiD3D12InitParams params;
        params.enableDebugLayer = true;
        QRhi::Flags rhiFlags = QRhi::EnableDebugMarkers;
        m_rhi.reset(QRhi::create(QRhi::D3D12, &params,rhiFlags));
    }
#endif

#if QT_CONFIG(metal)
    if (m_graphicsApi == QRhi::Metal) {
        QRhiMetalInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Metal, &params));
    }
#endif

    if (!m_rhi)
        qFatal("Failed to create RHI backend");

    m_sc.reset(m_rhi->newSwapChain());
    m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,QSize(),1,QRhiRenderBuffer::UsedWithSwapChainOnly));
    m_sc->setWindow(this);
    m_sc->setDepthStencil(m_ds.get());
    m_rp.reset(m_sc->newCompatibleRenderPassDescriptor());
    m_sc->setRenderPassDescriptor(m_rp.get());

    customInit();
}

void RhiWindow::resizeSwapChain()
{
    m_hasSwapChain = m_sc->createOrResize(); // also handles m_ds
    const QSize outputSize = m_sc->currentPixelSize();
    m_frameCount = 0;
    m_timer.restart();
    m_projection = createProjection(m_rhi.get(), 45.0f, outputSize.width() / (float)outputSize.height(), 0.1f, 1000.0f);

}
QMatrix4x4 RhiWindow::createProjection(QRhi *rhi, float fovDeg, float aspect, float nearPlane, float farPlane)
{
    QMatrix4x4 proj;
    proj.perspective(fovDeg, aspect, nearPlane, farPlane);
    // Flip Y pro Vulkan
    if (rhi->backend() == QRhi::Vulkan) {
        proj(1,1) *= -1.0f;
    }
    return proj;
}

void RhiWindow::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_sc->destroy();
    }
}

void RhiWindow::render()
{
    if (!m_hasSwapChain || m_notExposed)
        return;

    if (m_sc->currentPixelSize() != m_sc->surfacePixelSize() || m_newlyExposed) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        m_newlyExposed = false;
    }

    QRhi::FrameOpResult result = m_rhi->beginFrame(m_sc.get());
    if (result == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        result = m_rhi->beginFrame(m_sc.get());
    }
    if (result != QRhi::FrameOpSuccess) {
        qWarning("beginFrame failed with %d, will retry", result);
        requestUpdate();
        return;
    }
    m_frameCount += 1;

    if (m_timer.elapsed() > 1000) {
        m_currentFps = m_frameCount;
        qWarning("ca. %d fps", m_currentFps);

        m_timer.restart();
        m_frameCount = 0;
    }


    customRender();
    m_rhi->endFrame(m_sc.get());
    requestUpdate();
}

//================================== Helpery =====================================

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());
    return QShader();
}

//================================== HelloWindow =================================
//==================================              =================================


HelloWindow::HelloWindow(QRhi::Implementation graphicsApi)
    : RhiWindow(graphicsApi),
    mainCamera(QVector3D(0.0f, 0.0f, 5.0f))
{
  //setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::BlankCursor);

}
HelloWindow::~HelloWindow()
{
    if (shadowPipeline) {
       // m_shadowPipeline->release();
        delete shadowPipeline;
        shadowPipeline = nullptr;
    }

    if (shadowSRB) {
       // m_shadowSRB->release();
        delete shadowSRB;
        shadowSRB = nullptr;
    }

    if (shadowUbo) {
    //    m_shadowUbo->release();
        delete shadowUbo;
        shadowUbo = nullptr;
    }

    delete shadowMapTexture;
    delete shadowMapSampler;
    delete shadowMapRenderTarget;
    delete shadowMapRenderPassDesc;


}
void HelloWindow::customInit()
{
    //dumpApiFeatures(m_rhi.get());

    initialUpdateBatch = m_rhi->nextResourceUpdateBatch();

    initShadowMapResources(m_rhi.get());


    sky = std::make_unique<ProceduralSkyRHI>(m_rhi.get(), m_rp.get());

    hsky = std::make_unique<HdriSky>("assets/textures/sky.hdr");
    hsky->create(m_rhi.get(),m_rp.get(),initialUpdateBatch);
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    hsky->initCubemap(initialUpdateBatch);
  //  hsky->initCubemapOnGPU(initialUpdateBatch,cb);
    const QSize outputSize = m_sc->currentPixelSize();
    m_projection = createProjection(m_rhi.get(), 45.0f, outputSize.width() / (float)outputSize.height(), 0.1f, 1000.0f);

    TextureSet set;
    set.albedo = ":/assets/textures/brick/victorian-brick_albedo.png";
    set.normal = ":/assets/textures/brick/victorian-brick_normal-ogl.png";
    set.metallic = ":/assets/textures/brick/victorian-brick_metallic.png";
    set.rougness = ":/assets/textures/brick/victorian-brick_roughness.png";
    set.height = ":/assets/textures/brick/victorian-brick_height.png";
    set.ao = ":/assets/textures/brick/victorian-brick_ao.png";

    TextureSet set1;
    set1.albedo = ":/assets/textures/floor.png";
    set1.normal = ":/assets/textures/floorN.png";
    set1.metallic = ":/assets/textures/floorM.png";
    set1.rougness = ":/assets/textures/panel/sci-fi-panel1-roughness.png";
    set1.height = ":/assets/textures/panel/sci-fi-panel1-height.png";
    set1.ao = ":/assets/textures/panel/sci-fi-panel1-ao.png";

    QShader vsWire = getShader(":/shaders/prebuild/mcolor.vert.qsb");
    QShader fsWire = getShader(":/shaders/prebuild/mcolor.frag.qsb");
    QShader vs = getShader(":/shaders/prebuild/texture.vert.qsb");
    QShader fs = getShader(":/shaders/prebuild/texture.frag.qsb");
    QShader vs1 = getShader(":/shaders/prebuild/light.vert.qsb");
    QShader fs1 = getShader(":/shaders/prebuild/light.frag.qsb");
    QShader vs2 ;
    QShader fs2 ;

    switch (m_rhi->backend()) {
    case QRhi::Vulkan:
     //   qDebug() << "Vulkan";
        vs2 = getShader(":/shaders/prebuild/pbrvk.vert.qsb");
        fs2 = getShader(":/shaders/prebuild/pbrvk.frag.qsb");
        shaderapi = 3;
        break;
    case QRhi::OpenGLES2:
     //   qDebug() << "OpenGL / OpenGLES";

         vs2 = getShader(":/shaders/prebuild/pbr.vert.qsb");
         fs2 = getShader("://shaders/prebuild/pbr.frag.qsb");
         shaderapi = 1;
        break;
    case QRhi::D3D11:
     //   qDebug() << "Direct3D11";
        vs2 = getShader(":/shaders/prebuild/pbrd3d.vert.qsb");
        fs2 = getShader(":/shaders/prebuild/pbrd3d.frag.qsb");
        break;
        shaderapi = 2;
    case QRhi::D3D12:
       // qDebug() << "Direct3D12";
        vs2 = getShader(":/shaders/prebuild/pbrd3d.vert.qsb");
        fs2 = getShader(":/shaders/prebuild/pbrd3d.frag.qsb");
        shaderapi = 2;
        break;
    case QRhi::Metal:      qDebug() << "Metal";
        break;
    default:               qDebug() << "Null / Unknown"; break;
    }

    QVector<float> sphereVertices;
    QVector<quint16> sphereIndices;
    generateSphere(0.5f, 32, 64, sphereVertices, sphereIndices);

    QVector<float> cVertices;
    QVector<quint16> cIndices;
    generateCube(1.0f, cVertices, cIndices);

    QVector<float> planeVertices;
    QVector<quint16> planeIndices;
    generatePlane(150.0f, 150.0f, 10, 10, 20.0f, 20.0f, planeVertices, planeIndices);

    floor.addVertAndInd(planeVertices ,planeIndices );
    floor.init(m_rhi.get(), m_rp.get(), vs2, fs2, initialUpdateBatch,shadowMapTexture,shadowMapSampler,set);
    floor.transform.position = QVector3D(0, -0.5f, 0);
    //floor.transform.scale = QVector3D(10, 10, 10);
    //floor.transform.rotation.setX( 270.0f);

    cubeModel1.addVertAndInd(cVertices, cIndices);
    cubeModel1.init(m_rhi.get(), m_rp.get(), vs2, fs2, initialUpdateBatch,shadowMapTexture,shadowMapSampler,set1);
    cubeModel1.transform.position = QVector3D(-6, 1.5, 0);
    cubeModel1.transform.scale = QVector3D(2, 2, 2);

    cubeModel.addVertAndInd(planeVertices, planeIndices);
    cubeModel.init(m_rhi.get(), m_rp.get(), vs2, fs2, initialUpdateBatch,shadowMapTexture,shadowMapSampler,set1);
    cubeModel.transform.position = QVector3D(6, 1.0, 0);
    cubeModel.transform.scale = QVector3D(2, 2, 2);

    lightSphere.addVertAndInd(sphereVertices, sphereIndices);
    lightSphere.init(m_rhi.get(),m_rp.get(), vsWire, fsWire, initialUpdateBatch,shadowMapTexture,shadowMapSampler,set);
    lightSphere.transform.position = QVector3D(6.0f,10.4f, 15.4f);
    lightSphere.transform.scale = QVector3D(0.2f,0.2f, 0.2f);

    sphereModel.addVertAndInd(sphereVertices, sphereIndices);
    sphereModel.init(m_rhi.get(),m_rp.get(), vs2, fs2, initialUpdateBatch,shadowMapTexture,shadowMapSampler,set);
    sphereModel.transform.position = QVector3D(2.0f,2.0f, -6.0f);
    sphereModel.transform.scale = QVector3D(3.0f,3.0f, 3.0f);

    sphereModel1.addVertAndInd(sphereVertices, sphereIndices);
    sphereModel1.init(m_rhi.get(),m_rp.get(), vs2, fs2, initialUpdateBatch,shadowMapTexture,shadowMapSampler,set1);
    sphereModel1.transform.position = QVector3D(-2.0f,1.0f, -6.0f);
    sphereModel1.transform.scale = QVector3D(3.0f,3.0f, 3.0f);

    QString url = QCoreApplication::applicationDirPath() + "/assets/models/jet/jet.fbx";
    if (!QFile::exists(url)) {qWarning() << "url not exist:" << url; return;} 
    model = std::make_unique<FbxModel>(url);
    model->create(m_rhi.get(),m_sc->currentFrameRenderTarget(),m_rp.get());
   // model->modelInfo();

    models.append(&floor);
    models.append(&cubeModel1);
    models.append(&cubeModel);
    models.append(&lightSphere);
    models.append(&sphereModel);
    models.append(&sphereModel1);

    mainCamera.Position = QVector3D(-0.5f,5.5f, 15.5f);
    mainTimer.start();
}


void HelloWindow::customRender()
{

    deltaTime = mainTimer.restart() / 5000.0f;
    lightTime += deltaTime;

    QRhiResourceUpdateBatch *resourceUpdateBatch = m_rhi->nextResourceUpdateBatch();
    QRhiResourceUpdateBatch *shadowUpdateBatch = m_rhi->nextResourceUpdateBatch();
    QRhiResourceUpdateBatch *resourceUpdateBatch1 = m_rhi->nextResourceUpdateBatch();

    if (initialUpdateBatch) {
        resourceUpdateBatch->merge(initialUpdateBatch);
        shadowUpdateBatch->merge(initialUpdateBatch);
        initialUpdateBatch->release();
        initialUpdateBatch = nullptr;
    }

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();


    updateCamera(deltaTime);


    objectOpacity += objectOpacityDir * 0.005f;
    if (objectOpacity < 0.0f || objectOpacity > 1.0f) {
        objectOpacityDir *= -1;
        objectOpacity = qBound(0.0f, objectOpacity, 1.0f);
    }

    modelRotation += 30.0f * deltaTime;
   // QVector3D camPos = mainCamera.Position;
    float objectOpacity = 1.0f;
    float radius = 30.0f;
    float height = 10.0f;
    QVector3D center(0.0f, 0.0f, 0.0f);
    // lightPos = QVector3D(0.0f,4.4f, 0.0f);
    lightPosition.setX(center.x() + radius * cos(lightTime));
    lightPosition.setZ(center.z() + radius * sin(lightTime));
    lightPosition.setY(height);
    // lightPos.setX(0.0f);
    //lightPos.setZ(0.0f);
    lightSphere.transform.position = lightPosition;
     QVector3D lightColor(1.0f, 0.98f, 0.95f);
    // QVector3D lightColor(
    //     0.5f + 0.5f * sin(lightTime * 2.0f),
    //     0.5f + 0.5f * sin(lightTime * 0.7f + 2.0f),
    //     0.5f + 0.5f * sin(lightTime * 1.3f + 4.0f)
    //     );

    sphereModel1.transform.rotation.setY( sphereModel1.transform.rotation.y() + 0.5f);
    cubeModel1.transform.rotation.setY(cubeModel1.transform.rotation.y() + 0.5f);

    QMatrix4x4 lightView;
    QMatrix4x4 lightSpaceMatrix;
    QMatrix4x4 lightProjection;
    float nearPlane = 0.1f;
    float farPlane = 60.0f;
    float orthoSize = 40.0f;

    lightView.lookAt(lightPosition, center, QVector3D(0,1,0));

    QRhi::Implementation backend = m_rhi->backend();
    if (backend == QRhi::D3D11 || backend == QRhi::D3D12  || backend == QRhi::Vulkan ) {
        lightProjection.ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
        lightSpaceMatrix= lightProjection * lightView;
        lightSpaceMatrix = m_rhi->clipSpaceCorrMatrix() * lightSpaceMatrix;
    }
    else{
         lightProjection.ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
         lightSpaceMatrix= lightProjection * lightView;
    }

    QMatrix4x4 view = mainCamera.GetViewMatrix();
    float debug = 0.0F;
    float lightIntensity = 1.0f;
    float ambientStrange = 1.0f;
    float backendId = 1.0f;

    Ubo ubo;
    ubo.view        = view;
    ubo.projection  = m_projection;
    ubo.lightSpace  = lightSpaceMatrix;
    ubo.lightPos    = QVector4D(lightPosition, 1.0f);
    ubo.lightColor  = QVector4D(lightColor, 1.0f);
    ubo.camPos      = QVector4D(mainCamera.Position, 1.0f);
    ubo.opacity     = QVector4D(0.0f,0.0f,0.0f, objectOpacity);
    ubo.misc       = QVector4D(debug,lightIntensity,ambientStrange,shaderapi);

    QVector<float> cVertices;
    QVector<quint16> cIndices;
    QMatrix4x4 invView = view.inverted();
    QMatrix4x4 invProj =m_projection.inverted();
    QVector3D sunDir = lightPosition.normalized();

    //========================================update uniform====================================================
    for (auto m : std::as_const(models)) {
        m->updateUbo(ubo,resourceUpdateBatch);
    }

    for (auto m : std::as_const(models)) {
        m->updateShadowUbo(ubo,shadowUpdateBatch);
    }

    //float time = m_casovac->elapsedSeconds();
    sky->update(resourceUpdateBatch, invView, invProj, sunDir, lightTime);
    hsky->updateResources(resourceUpdateBatch,view, m_projection);
   // QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    generateCube(1.0f, cVertices, cIndices);
    cubeModel.updateGeometry(m_rhi.get(), resourceUpdateBatch1, cVertices, cIndices);
   // m_rhi->submitResourceUpdate();
    cb->resourceUpdate(resourceUpdateBatch1);

    Q_ASSERT(shadowMapRenderTarget);
    Q_ASSERT(shadowPipeline);
    Q_ASSERT(shadowSRB);
    Q_ASSERT(shadowUbo);

    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    const QColor clearColor = QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f);
    const QColor clearColorDepth = QColor::fromRgbF(1.0f, 1,1,1);

    updateFullscreenTexture(outputSizeInPixels, resourceUpdateBatch);
    QMatrix4x4 mvp_;
    mvp_.setToIdentity();
    QMatrix4x4 model1;

    model1.setToIdentity();
   // mvp_.perspective(45.0f, rtsz.width() / (float)rtsz.height(), 0.01f, 3000.0f);
    model1.translate({ -5.0f, 0.0f, -28.0f });
   // mvp_.rotate(rotation_);
    mvp_ = m_projection * view * model1;
    model->updateUbo(resourceUpdateBatch, mvp_);


    //========================================draw====================================================

    cb->beginPass(shadowMapRenderTarget, Qt::black, { 1.0f, 0 }, shadowUpdateBatch);
    cb->debugMarkBegin(QByteArrayLiteral("Shadows"));
    cb->setGraphicsPipeline(shadowPipeline);
    cb->setViewport(QRhiViewport(0, 0, SHADOW_MAP_SIZE.width(), SHADOW_MAP_SIZE.height()));

        for (auto m : std::as_const(models)) {
              m->DrawForShadow(cb,shadowPipeline,ubo,shadowUpdateBatch);
        }

    // floor.DrawForShadow(cb,m_shadowPipeline,ubo,shadowUpdateBatch);
    // cubeModel.DrawForShadow(cb,m_shadowPipeline,ubo,shadowUpdateBatch);
    // cubeModel1.DrawForShadow(cb,m_shadowPipeline,ubo,shadowUpdateBatch);
    // sphereModel1.DrawForShadow(cb,m_shadowPipeline,ubo,shadowUpdateBatch);
    // sphereModel.DrawForShadow(cb,m_shadowPipeline,ubo,shadowUpdateBatch);
    cb->debugMarkEnd();
    cb->endPass();

    cb->beginPass(m_sc->currentFrameRenderTarget(), clearColor, { 1.0f, 0 }, resourceUpdateBatch);

    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->debugMarkBegin(QByteArrayLiteral("Sky"));
      // sky->draw(cb);
        hsky->draw(cb);

    cb->debugMarkEnd();
        for (auto m : std::as_const(models)) {
            m->draw(cb);
        }

        model->draw(cb, { 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });

    cb->setGraphicsPipeline(uiPipeline.get());
    cb->setShaderResources(uiSRB.get());
   // cb->resourceUpdate(resourceUpdateBatch);
    cb->draw(3);

    cb->endPass();

}
//======================================================iNPUT======================================================================

void HelloWindow::keyPressEvent(QKeyEvent *e)
{
    // switch fps
    if (e->key() == Qt::Key_Q) {
        cameraMovementEnabled = !cameraMovementEnabled; // true/false

        if (cameraMovementEnabled) {
            setCursor(Qt::BlankCursor);
            lastMousePosition = mapFromGlobal(QCursor::pos());
        } else {
            setCursor(Qt::ArrowCursor);
        }
        return;
    }
    pressedKeys.insert(e->key());
}

void HelloWindow::keyReleaseEvent(QKeyEvent *e)
{
    pressedKeys.remove(e->key());
}

void HelloWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (!cameraMovementEnabled) {
        return;
    }

    QPoint localCenter(width() / 2, height() / 2);


    if (lastMousePosition.isNull()) {
        lastMousePosition = localCenter;

        return;
    }

    float xoffset = e->position().x() - localCenter.x();
    float yoffset = localCenter.y() - e->position().y();

    mainCamera.ProcessMouseMovement(xoffset, yoffset);

    QPoint globalCenter = mapToGlobal(localCenter);
    QCursor::setPos(globalCenter);


    lastMousePosition = localCenter;

    e->accept();
}

void HelloWindow::updateCamera(float dt)
{
    if (!cameraMovementEnabled) {
        return;
    }
    if (pressedKeys.contains(Qt::Key_W))
        mainCamera.ProcessKeyboard(FORWARD, dt);
    if (pressedKeys.contains(Qt::Key_S))
        mainCamera.ProcessKeyboard(BACKWARD, dt);
    if (pressedKeys.contains(Qt::Key_A))
        mainCamera.ProcessKeyboard(LEFT, dt);
    if (pressedKeys.contains(Qt::Key_D))
        mainCamera.ProcessKeyboard(RIGHT, dt);
    if (pressedKeys.contains(Qt::Key_Escape))
        qApp->quit();
}

//======================================================iNIT RESOURCES AND PIPELINES

void HelloWindow::initShadowMapResources(QRhi *rhi) {

    //=======================================shadowpipeline=======================================

    shadowMapTexture = rhi->newTexture(
        QRhiTexture::D32F,                  //only depth
        SHADOW_MAP_SIZE,
        1,
        QRhiTexture::RenderTarget
        );
    shadowMapTexture->create();

    shadowMapSampler = rhi->newSampler(
        QRhiSampler::Nearest,
        QRhiSampler::Nearest,
        QRhiSampler::None,
        QRhiSampler::ClampToEdge,
        QRhiSampler::ClampToEdge,
        QRhiSampler::ClampToEdge
        );

    shadowMapSampler->create();
    const quint32 UBUF_SIZE = 512;

    shadowUbo = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE);
    shadowUbo->create();

    shadowSRB = rhi->newShaderResourceBindings();
    shadowSRB->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, shadowUbo)
    });
    shadowSRB->create();

    //  Render target description with depth attachment
    QRhiTextureRenderTargetDescription shadowRtDesc;
    shadowRtDesc.setDepthTexture(shadowMapTexture);
    //  Render target + render pass descriptor
    shadowMapRenderTarget = rhi->newTextureRenderTarget(shadowRtDesc);
    shadowMapRenderPassDesc = shadowMapRenderTarget->newCompatibleRenderPassDescriptor();
    shadowMapRenderTarget->setRenderPassDescriptor(shadowMapRenderPassDesc);
    shadowMapRenderTarget->create();
    shadowPipeline = rhi->newGraphicsPipeline();

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 14 * sizeof(float) }
    });

    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },                    // pos
        { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) },    // normal
        { 0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float) },    // uv
        { 0, 3, QRhiVertexInputAttribute::Float3, 8 * sizeof(float) },    // tangent
        { 0, 4, QRhiVertexInputAttribute::Float3, 11 * sizeof(float) }    // bitangent
    });

    shadowPipeline->setVertexInputLayout(inputLayout);

    QShader vs = getShader(":/shaders/prebuild/depth.vert.qsb");
    QShader fs = getShader(":/shaders/prebuild/depth.frag.qsb"); // depth-only shader

    shadowPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    shadowPipeline->setShaderResourceBindings(shadowSRB);

    switch (rhi->backend()) {
    case QRhi::Vulkan:
      //  qDebug() << "Vulkan";
        shadowPipeline->setCullMode(QRhiGraphicsPipeline::None);
        break;
    case QRhi::OpenGLES2:
      //  qDebug() << "OpenGL / OpenGLES";
        shadowPipeline->setCullMode(QRhiGraphicsPipeline::None);
        break;
    case QRhi::D3D11:
     //   qDebug() << "Direct3D11";
         shadowPipeline->setCullMode(QRhiGraphicsPipeline::None);
        break;
    case QRhi::D3D12:
      //  qDebug() << "Direct3D12";
         shadowPipeline->setCullMode(QRhiGraphicsPipeline::None);
        break;
    case QRhi::Metal:      qDebug() << "Metal";
        break;
    default:               qDebug() << "Null / Unknown";
        break;
    }

    //Q_ASSERT(m_shadowMapRenderPassDesc); // sanity

    shadowPipeline->setRenderPassDescriptor(shadowMapRenderPassDesc);
    shadowPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
    shadowPipeline->setDepthTest(true);
    shadowPipeline->setDepthWrite(true);

    switch (rhi->backend()) {
    case QRhi::Vulkan:
    case QRhi::OpenGLES2:
        shadowPipeline->setDepthBias(5);
        shadowPipeline->setSlopeScaledDepthBias(1.1f);
        break;

    case QRhi::D3D11:
    case QRhi::D3D12:
        shadowPipeline->setDepthBias(5);   // D3D constant bias
        shadowPipeline->setSlopeScaledDepthBias(1.0f);
        break;

    case QRhi::Metal:
        shadowPipeline->setDepthBias(5);
        shadowPipeline->setSlopeScaledDepthBias(1.0f);
        break;

    default:
        break;
    }
    shadowPipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    shadowPipeline->create();

    //=======================================full screen pipeline=======================================

    updateFullscreenTexture(m_sc->surfacePixelSize(), initialUpdateBatch);

    uiSampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    uiSampler->create();

    uiSRB.reset(m_rhi->newShaderResourceBindings());
    uiSRB->setBindings({
        QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage,
                                                  uiTexture.get(), uiSampler.get())
    });
    uiSRB->create();

    uiPipeline.reset(m_rhi->newGraphicsPipeline());
    uiPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/shaders/prebuild/quad.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/shaders/prebuild/quad.frag.qsb")) }
    });

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.opColor = QRhiGraphicsPipeline::Add;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.opAlpha = QRhiGraphicsPipeline::Add;

    uiPipeline->setTargetBlends({ blend });
    uiPipeline->setDepthTest(false);
    uiPipeline->setDepthWrite(false);
    uiPipeline->setVertexInputLayout({});
    uiPipeline->setShaderResourceBindings(uiSRB.get());
    uiPipeline->setRenderPassDescriptor(m_rp.get());
    uiPipeline->create();
}

void HelloWindow::updateFullscreenTexture(const QSize &pixelSize, QRhiResourceUpdateBatch *u)
{
    if (uiTexture && uiTexture->pixelSize() == pixelSize)
        return;

    if (!uiTexture)
        uiTexture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, pixelSize));
    else
        uiTexture->setPixelSize(pixelSize);

    uiTexture->create();

     QImage image(pixelSize, QImage::Format_RGBA8888_Premultiplied);
     image.setDevicePixelRatio(devicePixelRatio());
     image.fill(Qt::transparent );
     QPainter painter(&image);
    // painter.setRenderHint(QPainter::Antialiasing, true);

    // // --- sky ---
    // QLinearGradient skyGradient(QPointF(0, 0), QPointF(0, pixelSize.height()));
    // skyGradient.setColorAt(0.0, QColor::fromRgbF(0.53f, 0.81f, 0.98f, 1.0f)); // light blue
    // skyGradient.setColorAt(1.0, QColor::fromRgbF(0.0f, 0.4f, 0.8f, 1.0f));   // dark blue
    // painter.fillRect(QRectF(QPointF(0, 0), pixelSize), skyGradient);

    // --- clouds ---
    // painter.setPen(Qt::NoPen);
    // painter.setBrush(QColor(255, 255, 255, 100));

    // QRandomGenerator *rng = QRandomGenerator::global();
    // int cloudCount = 6;
    // for (int i = 0; i < cloudCount; ++i) {
    //     int x = (pixelSize.width() / cloudCount) * i + rng->bounded(50);
    //     int y = pixelSize.height() / 5 + (rng->bounded(60) - 30);
    //     int w = 120 + rng->bounded(80);
    //     int h = 60 + rng->bounded(30);
    //     painter.drawEllipse(QRect(x, y, w, h));
    // }
      qDebug() << "cal";
     m_frameCount += 1;
     if (m_timer.elapsed() > 1000) {
         m_currentFps = m_frameCount;

     }
    painter.setPen(Qt::white);
    QFont font;
    font.setPixelSize(0.04 * qMin(width(), height()));
    painter.setFont(font);
    //painter.drawText(QRectF(QPointF(10, 10), size() - QSize(20, 20)), 0,QLatin1String("QRhi %1 API").arg(graphicsApiName()));
  //  painter.drawText(QRectF(QPointF(10, 10), size() - QSize(20, 20)), 0,QLatin1String("QRhi - %1 - %2 -").arg(graphicsApiName()).arg((int)m_currentFps));
    // 1. Získejte data pro tisk
    QString apiName = graphicsApiName();
    int fpsValue = m_currentFps; // ⭐ Používáme proměnnou, která drží výsledek, ne m_frameCount!

    // 2. Vytvořte finální řetězec pomocí moderního QRegularExpression (pro %1, %2)
    // a uložte jej do lokální proměnné.
    QString textToDraw = QStringLiteral("QRhi - %1 - %2 FPS")
                             .arg(apiName)
                             .arg(fpsValue);

    painter.drawText(QRectF(QPointF(10, 10), size() - QSize(20, 20)), 0, textToDraw);
    painter.end();

    if (m_rhi->isYUpInNDC())
        image = image.mirrored();

    u->uploadTexture(uiTexture.get(), image);
}

// qDebug() << "lightprojection = " << lightProjection << "\n";
//  qDebug() << "lightview = " << lightView << "\n";
// qDebug() << "lightspace = " << ubo.lightSpace << "\n";
//=========================================================================================================
