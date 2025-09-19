

#include "rhiwindow.h"
#include <QPlatformSurfaceEvent>
#include <QPainter>
#include <QFile>
#include <QImage>
#include <QFont>
#include <rhi/qshader.h>
#include "qtrhi3d/geometry.h"

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
    if (m_graphicsApi == QRhi::Null) {
        QRhiNullInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Null, &params));
    }

#if QT_CONFIG(opengl)
    if (m_graphicsApi == QRhi::OpenGLES2) {
        m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
        QRhiGles2InitParams params;
        params.fallbackSurface = m_fallbackSurface.get();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
    }
#endif

#if QT_CONFIG(vulkan)
    if (m_graphicsApi == QRhi::Vulkan) {
        QRhiVulkanInitParams params;
        params.inst = vulkanInstance();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));
    }
#endif

#ifdef Q_OS_WIN
    if (m_graphicsApi == QRhi::D3D11) {
     QRhiD3D11InitParams params;
       params.enableDebugLayer = true;
       m_rhi.reset(QRhi::create(QRhi::D3D11, &params));
     //   QRhiVulkanInitParams params;
    //    params.inst = vulkanInstance();
     //   params.window = this;
     //   m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));

    } else if (m_graphicsApi == QRhi::D3D12) {
        QRhiD3D12InitParams params;
        params.enableDebugLayer = true;
        m_rhi.reset(QRhi::create(QRhi::D3D12, &params));
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
    m_camera(QVector3D(0.0f, 0.0f, 5.0f))
{
  //  setFocusPolicy(Qt::StrongFocus);
    // Skryje kurzor a zachytí ho v okně
    //setCursor(Qt::BlankCursor);
}
HelloWindow::~HelloWindow()
{
    if (m_shadowPipeline) {
       // m_shadowPipeline->release();
        delete m_shadowPipeline;
        m_shadowPipeline = nullptr;
    }

    if (m_shadowSRB) {
       // m_shadowSRB->release();
        delete m_shadowSRB;
        m_shadowSRB = nullptr;
    }

    if (m_shadowUbo) {
    //    m_shadowUbo->release();
        delete m_shadowUbo;
        m_shadowUbo = nullptr;
    }

    delete m_shadowMapTexture;
    delete m_shadowMapSampler;
    delete m_shadowMapRenderTarget;
    delete m_shadowMapRenderPassDesc;
   // delete m_shadowUbo;
  //  delete m_shadowSRB;
  //  delete m_shadowPipeline;
    // QRhi samotné uvolníš až nakonec

}
void HelloWindow::customInit()
{
    m_initialUpdates = m_rhi->nextResourceUpdateBatch();

    initShadowMapResources(m_rhi.get());

    QShader vs = getShader(":/texture.vert.qsb");
    QShader fs = getShader(":/texture.frag.qsb");
    QShader vs1 = getShader(":/light.vert.qsb");
    QShader fs1 = getShader(":/light.frag.qsb");
    QShader vs2 = getShader(":/pbr.vert.qsb");
    QShader fs2 = getShader(":/pbr.frag.qsb");

    QVector<float> sphereVertices;
    QVector<quint16> sphereIndices;

    generateSphere(0.5f, 32, 64, sphereVertices, sphereIndices);

    floor.addVertAndInd(indexedPlaneVertices ,indexedPlaneIndices );
    floor.init(m_rhi.get(), m_rp.get(), vs2, fs2, m_initialUpdates,m_shadowMapTexture,m_shadowMapSampler);
    floor.transform.position = QVector3D(0, -0.5f, 0);
    floor.transform.scale = QVector3D(10, 10, 10);
    floor.transform.rotation.setX( 270.0f);

    m_cube1.addVertAndInd(cubeVertices1, cubeIndices1);
    m_cube1.init(m_rhi.get(), m_rp.get(), vs2, fs2, m_initialUpdates,m_shadowMapTexture,m_shadowMapSampler);
    m_cube1.transform.position = QVector3D(-1.5f, 0, 0);

    m_cube2.addVertAndInd(sphereVertices, sphereIndices); 
    m_cube2.init(m_rhi.get(),m_rp.get(), vs2, fs2, m_initialUpdates,m_shadowMapTexture,m_shadowMapSampler); 
    m_cube2.transform.position = QVector3D(2.5f, 1, 0);
    m_cube2.transform.scale = QVector3D(1.5f,1.5f, 1.5f);

    m_camera.Position = QVector3D(-0.5f,1.5f, 5.5f);
    m_timer.start();
}


void HelloWindow::customRender()
{
    m_dt = m_timer.restart() / 1000.0f;

    lightTime += m_dt;

    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();
    QRhiResourceUpdateBatch *shadowBatch = m_rhi->nextResourceUpdateBatch();

    if (m_initialUpdates) {
        resourceUpdates->merge(m_initialUpdates);
        shadowBatch->merge(m_initialUpdates);
        m_initialUpdates->release();
        m_initialUpdates = nullptr;
    }


    m_opacity += m_opacityDir * 0.005f;
    if (m_opacity < 0.0f || m_opacity > 1.0f) {
        m_opacityDir *= -1;
        m_opacity = qBound(0.0f, m_opacity, 1.0f);
    }
    m_rotation += 30.0f * m_dt;

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    updateCamera(m_dt);
    QMatrix4x4 view = m_camera.GetViewMatrix();
    QMatrix4x4 projection = m_projection;

    QVector3D camPos = m_camera.Position;
    float objectOpacity = 1.0f;
    float radius = 15.0f;
    float height = 20.0f;
    QVector3D center(0.0f, 0.0f, 0.0f);
    QVector3D lightPos(-5.0f, 20.0f, -15.0f);

 //   QVector3D lightColor(1.0f, 1.0f, 1.0f);
    lightPos.setX(center.x() + radius * cos(lightTime));
    lightPos.setZ(center.z() + radius * sin(lightTime));
    lightPos.setY(height);

     QVector3D lightColor(
         0.5f + 0.5f * sin(lightTime * 2.0f),
         0.5f + 0.5f * sin(lightTime * 0.7f + 2.0f),
         0.5f + 0.5f * sin(lightTime * 1.3f + 4.0f)
         );
    m_cube1.transform.rotation.setY( m_cube1.transform.rotation.y() + 0.5f);
    m_cube2.transform.rotation.setY(m_cube2.transform.rotation.y() + 0.5f);

    QMatrix4x4 lightProjection;
    float nearPlane = 1.0f;
    float farPlane = 50.0f;
    float orthoSize = 30.0f;
    lightProjection.ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
    QMatrix4x4 lightView;
    lightView.lookAt(lightPos, center, QVector3D(0,1,0));
    QMatrix4x4 lightSpaceMatrix = lightProjection * lightView;
    float debug = 0.0F;
    float lightIntensity = 2.0f;

    Ubo ubo;
    //ubo.model       = transform.getModelMatrix();
    ubo.view        = view;
    ubo.projection  = projection;
    ubo.lightSpace  = lightSpaceMatrix;
    ubo.lightPos    = QVector4D(lightPos, 1.0f);
    ubo.lightColor  = QVector4D(lightColor, 1.0f);
    ubo.camPos      = QVector4D(camPos, 1.0f);
    ubo.opacity     = QVector4D(0.0f,0.0f,0.0f, m_opacity);
    ubo.misc       = QVector4D(debug,lightIntensity,0.0f, 1.0f);



    floor.updateUbo(ubo,resourceUpdates);
    m_cube1.updateUbo(ubo,resourceUpdates);
    m_cube2.updateUbo(ubo,resourceUpdates);


    floor.updateShadowUbo(ubo,shadowBatch);
    m_cube1.updateShadowUbo(ubo,shadowBatch);
    m_cube2.updateShadowUbo(ubo,shadowBatch);

    Q_ASSERT(m_shadowMapRenderTarget);
    Q_ASSERT(m_shadowPipeline);
    Q_ASSERT(m_shadowSRB);
    Q_ASSERT(m_shadowUbo);

    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    const QColor clearColor = QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f);
    const QColor clearColorDepth = QColor::fromRgbF(1.0f, 1,1,1);

    cb->beginPass(m_shadowMapRenderTarget, clearColorDepth, { 1.0f, 0 }, shadowBatch);

    cb->setGraphicsPipeline(m_shadowPipeline);
    cb->setViewport(QRhiViewport(0, 0, SHADOW_MAP_SIZE.width(), SHADOW_MAP_SIZE.height()));
        floor.DrawForShadow(cb,m_shadowPipeline,ubo,shadowBatch);
        m_cube1.DrawForShadow(cb,m_shadowPipeline,ubo,shadowBatch);
        m_cube2.DrawForShadow(cb,m_shadowPipeline,ubo,shadowBatch);

    cb->endPass();

    cb->beginPass(m_sc->currentFrameRenderTarget(), clearColor, { 1.0f, 0 }, resourceUpdates);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
        floor.draw(cb);
        m_cube1.draw(cb);
        m_cube2.draw(cb);
    cb->endPass();

}

//=========================================================================================================

void HelloWindow::keyPressEvent(QKeyEvent *e)
{
    m_pressedKeys.insert(e->key());
}

void HelloWindow::keyReleaseEvent(QKeyEvent *e)
{
    m_pressedKeys.remove(e->key());
}

void HelloWindow::mouseMoveEvent(QMouseEvent *e)
{
    // Pokud je to první pohyb myši, jen nastavíme počáteční pozici
    if (m_lastMousePos.isNull()) {
        m_lastMousePos = e->position();
        return;
    }

    float xoffset = e->position().x() - m_lastMousePos.x();
    float yoffset = m_lastMousePos.y() - e->position().y(); // Obráceně, protože Y souřadnice rostou dolů

    m_lastMousePos = e->position();
    m_camera.ProcessMouseMovement(xoffset, yoffset);

    // center cursor
     //QPoint center = mapToGlobal(geometry().center());
     //QCursor::setPos(center);
     //m_lastMousePos = mapFromGlobal(center);
}

void HelloWindow::updateCamera(float dt)
{
    if (m_pressedKeys.contains(Qt::Key_W))
        m_camera.ProcessKeyboard(FORWARD, dt);
    if (m_pressedKeys.contains(Qt::Key_S))
        m_camera.ProcessKeyboard(BACKWARD, dt);
    if (m_pressedKeys.contains(Qt::Key_A))
        m_camera.ProcessKeyboard(LEFT, dt);
    if (m_pressedKeys.contains(Qt::Key_D))
        m_camera.ProcessKeyboard(RIGHT, dt);
    if (m_pressedKeys.contains(Qt::Key_Escape))
        qApp->quit();
}

void HelloWindow::initShadowMapResources(QRhi *rhi) {

    m_shadowMapTexture = rhi->newTexture(
        QRhiTexture::D32F,                  // pouze depth
        SHADOW_MAP_SIZE,
        1,
        QRhiTexture::RenderTarget    // není potřeba UsedAsSampled v Qt 6.9.2
        );
    m_shadowMapTexture->create();

    m_shadowMapSampler = rhi->newSampler(
        QRhiSampler::Nearest,
        QRhiSampler::Nearest,
        QRhiSampler::None,
        QRhiSampler::ClampToEdge,
        QRhiSampler::ClampToEdge,
        QRhiSampler::ClampToEdge
        );

    m_shadowMapSampler->create();
    const quint32 UBUF_SIZE = 512;

    m_shadowUbo = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE);
    m_shadowUbo->create();

    m_shadowSRB = rhi->newShaderResourceBindings();
    m_shadowSRB->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, m_shadowUbo)
    });
    m_shadowSRB->create();
    // 2. Render target description s depth attachmentem
    QRhiTextureRenderTargetDescription shadowRtDesc;
    shadowRtDesc.setDepthTexture(m_shadowMapTexture);
    // 3. Render target + render pass descriptor
    m_shadowMapRenderTarget = rhi->newTextureRenderTarget(shadowRtDesc);
    m_shadowMapRenderPassDesc = m_shadowMapRenderTarget->newCompatibleRenderPassDescriptor();
    m_shadowMapRenderTarget->setRenderPassDescriptor(m_shadowMapRenderPassDesc);
    m_shadowMapRenderTarget->create();

    m_shadowPipeline = rhi->newGraphicsPipeline();

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 3 * sizeof(float) } }); // jen pozice
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 } });

    m_shadowPipeline->setVertexInputLayout(inputLayout);


    QShader vs = getShader(":/shaders/prebuild/depth.vert.qsb");
    QShader fs = getShader(":/shaders/prebuild/depth.frag.qsb"); // depth-only shader

    m_shadowPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });


    m_shadowPipeline->setShaderResourceBindings(m_shadowSRB);

    Q_ASSERT(m_shadowMapRenderPassDesc); // sanity
    m_shadowPipeline->setRenderPassDescriptor(m_shadowMapRenderPassDesc);
    m_shadowPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
    m_shadowPipeline->setDepthTest(true);
    m_shadowPipeline->setDepthWrite(true);
    m_shadowPipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    m_shadowPipeline->setCullMode(QRhiGraphicsPipeline::Back);
    m_shadowPipeline->create();
}
