

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

    TextureSet set;
    set.albedo = ":/assets/textures/brick/victorian-brick_albedo.png";
    set.normal = ":/assets/textures/brick/victorian-brick_normal-ogl.png";
    set.metallic = ":/assets/textures/brick/victorian-brick_metallic.png";
    set.rougness = ":/assets/textures/brick/victorian-brick_roughness.png";
    set.height = ":/assets/textures/brick/victorian-brick_height.png";
    set.ao = ":/assets/textures/brick/victorian-brick_ao.png";

    TextureSet set1;
    set1.albedo = ":/assets/textures/panel/sci-fi-panel1-albedo.png";
    set1.normal = ":/assets/textures/panel/sci-fi-panel1-normal-ogl.png";
    set1.metallic = ":/assets/textures/panel/sci-fi-panel1-metallic.png";
    set1.rougness = ":/assets/textures/panel/sci-fi-panel1-roughness.png";
    set1.height = ":/assets/textures/panel/sci-fi-panel1-height.png";
    set1.ao = ":/assets/textures/panel/sci-fi-panel1-ao.png";


    QShader vs = getShader(":/texture.vert.qsb");
    QShader fs = getShader(":/texture.frag.qsb");
    QShader vs1 = getShader(":/light.vert.qsb");
    QShader fs1 = getShader(":/light.frag.qsb");
    QShader vs2 = getShader(":/pbr.vert.qsb");
    QShader fs2 = getShader(":/pbr.frag.qsb");

    QShader vsWire = getShader(":/mcolor.vert.qsb");
    QShader fsWire = getShader(":/mcolor.frag.qsb");


    QVector<float> sphereVertices;
    QVector<quint16> sphereIndices;

    generateSphere(0.5f, 32, 64, sphereVertices, sphereIndices);

    floor.addVertAndInd(indexedPlaneVertices ,indexedPlaneIndices );
    floor.init(m_rhi.get(), m_rp.get(), vs2, fs2, m_initialUpdates,m_shadowMapTexture,m_shadowMapSampler,set);
    //floor.transform.position = QVector3D(0, -0.5f, 0);
    //floor.transform.scale = QVector3D(10, 10, 10);
    //floor.transform.rotation.setX( 270.0f);

    m_cube1.addVertAndInd(cubeVertices1, cubeIndices1);
    m_cube1.init(m_rhi.get(), m_rp.get(), vs2, fs2, m_initialUpdates,m_shadowMapTexture,m_shadowMapSampler,set1);
    m_cube1.transform.position = QVector3D(0, 1, 0);
    m_cube1.transform.scale = QVector3D(2, 2, 2);

    m_cube2.addVertAndInd(sphereVertices, sphereIndices); 
    m_cube2.init(m_rhi.get(),m_rp.get(), vsWire, fsWire, m_initialUpdates,m_shadowMapTexture,m_shadowMapSampler,set);
    m_cube2.transform.position = QVector3D(6.0f,10.4f, 15.4f);
    m_cube2.transform.scale = QVector3D(0.4f,0.4f, 0.4f);

    m_camera.Position = QVector3D(-0.5f,5.5f, 15.5f);
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
    float radius = 10.0f;
    float height = 10.0f;
    QVector3D center(0.0f, 0.0f, 0.0f);

     // lightPos = QVector3D(0.0f,4.4f, 0.0f);
    lightPos.setX(center.x() + radius * cos(lightTime));
    lightPos.setZ(center.z() + radius * sin(lightTime));
    lightPos.setY(height);
   // lightPos.setX(0.0f);
    //lightPos.setX(0.0f);

    QVector3D lightColor(1.0f, 0.98f, 0.95f);
    m_cube2.transform.position = lightPos;

    // QVector3D lightColor(
    //     0.5f + 0.5f * sin(lightTime * 2.0f),
    //     0.5f + 0.5f * sin(lightTime * 0.7f + 2.0f),
    //     0.5f + 0.5f * sin(lightTime * 1.3f + 4.0f)
    //     );

    //  m_cube1.transform.rotation.setY( m_cube1.transform.rotation.y() + 0.5f);
    // m_cube2.transform.rotation.setY(m_cube2.transform.rotation.y() + 0.5f);

    QMatrix4x4 lightView;
    QMatrix4x4 lightSpaceMatrix;
    QMatrix4x4 lightProjection;
    // float nearPlane = 0.1f;
    // float farPlane = 30.0f;
    // float orthoSize = 20.0f;

    // lightProjection.ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
    // lightView.lookAt(lightPos, center, QVector3D(0,1,0));
    // lightSpaceMatrix = lightProjection * lightView;

    QVector<Model*> sceneObjects = { &floor, &m_cube1, &m_cube2 };
    QVector3D sceneExtents;
    QVector3D sceneCenter = computeSceneCenterAndExtents(sceneObjects, sceneExtents);
    lightView.lookAt(lightPos, sceneCenter, QVector3D(0,1,0));
    float orthoX = sceneExtents.x() / 2.0f;
    float orthoZ = sceneExtents.z() / 2.0f;
    float nearPlane = 0.1f;
    float farPlane = sceneExtents.y() + 10.0f; // přidáme buffer
    lightProjection.setToIdentity();
    lightProjection.ortho(-orthoX, orthoX, -orthoZ, orthoZ, nearPlane, farPlane);
    lightSpaceMatrix = lightProjection * lightView;

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

   // qDebug() << "lightprojection = " << lightProjection << "\n";
  //  qDebug() << "lightview = " << lightView << "\n";
   // qDebug() << "lightspace = " << ubo.lightSpace << "\n";

    floor.updateUbo(ubo,resourceUpdates);
    m_cube1.updateUbo(ubo,resourceUpdates);
    m_cube2.updateUbo(ubo,resourceUpdates);
   // frustumModel.updateUbo(ubo, resourceUpdates);

    floor.updateShadowUbo(ubo,shadowBatch);
    m_cube1.updateShadowUbo(ubo,shadowBatch);
    m_cube2.updateShadowUbo(ubo,shadowBatch);

    Q_ASSERT(m_shadowMapRenderTarget);
    Q_ASSERT(m_shadowPipeline);
    Q_ASSERT(m_shadowSRB);
    Q_ASSERT(m_shadowUbo);

    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    const QColor clearColor = QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f);
    const QColor clearColorDepth = QColor::fromRgbF(1.0f, 1,1,1);

//===============================================================================================

    cb->beginPass(m_shadowMapRenderTarget, Qt::black, { 1.0f, 0 }, shadowBatch);
    cb->setGraphicsPipeline(m_shadowPipeline);
    cb->setViewport(QRhiViewport(0, 0, SHADOW_MAP_SIZE.width(), SHADOW_MAP_SIZE.height()));
       // floor.DrawForShadow(cb,m_shadowPipeline,ubo,shadowBatch);
        m_cube1.DrawForShadow(cb,m_shadowPipeline,ubo,shadowBatch);
      //  m_cube1.testShadowPass(m_rhi.get(), cb, m_shadowMapRenderPassDesc);
        m_cube2.DrawForShadow(cb,m_shadowPipeline,ubo,shadowBatch);
    cb->endPass();

    cb->beginPass(m_sc->currentFrameRenderTarget(), clearColor, { 1.0f, 0 }, resourceUpdates);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
        floor.draw(cb);
        m_cube1.draw(cb);
        m_cube2.draw(cb);
       // frustumModel.draw(cb);
    cb->endPass();

}

QVector3D HelloWindow::computeSceneCenterAndExtents(const QVector<Model*> &objects, QVector3D &extents)
{
    if (objects.isEmpty()) return QVector3D(0,0,0);

    QVector3D minPt(FLT_MAX, FLT_MAX, FLT_MAX);
    QVector3D maxPt(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (Model* obj : objects) {
        const QMatrix4x4 &model = obj->transform.getModelMatrix();
        const QVector<float> &verts = obj->m_vert;

        for (int i = 0; i < verts.size(); i += 14) { // stride = 14 floatů
            QVector3D v(verts[i], verts[i+1], verts[i+2]);
            v = model * v;
            minPt.setX(qMin(minPt.x(), v.x()));
            minPt.setY(qMin(minPt.y(), v.y()));
            minPt.setZ(qMin(minPt.z(), v.z()));
            maxPt.setX(qMax(maxPt.x(), v.x()));
            maxPt.setY(qMax(maxPt.y(), v.y()));
            maxPt.setZ(qMax(maxPt.z(), v.z()));
        }
    }

    extents = maxPt - minPt;
    return (minPt + maxPt) * 0.5f; // center
}

//=========================================================================================================
void HelloWindow::generateLightFrustum(float orthoSize,
                                       float nearPlane,
                                       float farPlane,
                                       QVector<float> &vertices,
                                       QVector<quint16> &indices)
{
    vertices.clear();
    indices.clear();


    QVector<QVector3D> corners = {
        {-orthoSize, -orthoSize, -nearPlane}, // 0
        { orthoSize, -orthoSize, -nearPlane}, // 1
        { orthoSize,  orthoSize, -nearPlane}, // 2
        {-orthoSize,  orthoSize, -nearPlane}, // 3
        {-orthoSize, -orthoSize, -farPlane},  // 4
        { orthoSize, -orthoSize, -farPlane},  // 5
        { orthoSize,  orthoSize, -farPlane},  // 6
        {-orthoSize,  orthoSize, -farPlane}   // 7
    };


    QVector<QVector3D> normals = {
        {0, 0, -1}, // near
        {0, 0,  1}, // far
        {-1, 0, 0}, // left
        { 1, 0, 0}, // right
        {0,  1, 0}, // top
        {0, -1, 0}  // bottom
    };

    // UV placeholder (0..1)
    auto addVertex = [&](const QVector3D &pos, const QVector3D &normal, const QVector2D &uv) {
        vertices.append(pos.x());
        vertices.append(pos.y());
        vertices.append(pos.z());
        vertices.append(normal.x());
        vertices.append(normal.y());
        vertices.append(normal.z());
        vertices.append(uv.x());
        vertices.append(uv.y());
    };

    // Near
    addVertex(corners[0], normals[0], {0,0});
    addVertex(corners[1], normals[0], {1,0});
    addVertex(corners[2], normals[0], {1,1});
    addVertex(corners[3], normals[0], {0,1});

    // Far
    addVertex(corners[5], normals[1], {0,0});
    addVertex(corners[4], normals[1], {1,0});
    addVertex(corners[7], normals[1], {1,1});
    addVertex(corners[6], normals[1], {0,1});

    // Left
    addVertex(corners[4], normals[2], {0,0});
    addVertex(corners[0], normals[2], {1,0});
    addVertex(corners[3], normals[2], {1,1});
    addVertex(corners[7], normals[2], {0,1});

    // Right
    addVertex(corners[1], normals[3], {0,0});
    addVertex(corners[5], normals[3], {1,0});
    addVertex(corners[6], normals[3], {1,1});
    addVertex(corners[2], normals[3], {0,1});

    // Top
    addVertex(corners[3], normals[4], {0,0});
    addVertex(corners[2], normals[4], {1,0});
    addVertex(corners[6], normals[4], {1,1});
    addVertex(corners[7], normals[4], {0,1});

    // Bottom
    addVertex(corners[4], normals[5], {0,0});
    addVertex(corners[5], normals[5], {1,0});
    addVertex(corners[1], normals[5], {1,1});
    addVertex(corners[0], normals[5], {0,1});

    // --- Indexy (6 faces * 2 triangles * 3 indices) ---
    for (int f = 0; f < 6; ++f) {
        quint16 base = f * 4;
        indices.append(base + 0);
        indices.append(base + 1);
        indices.append(base + 2);
        indices.append(base + 0);
        indices.append(base + 2);
        indices.append(base + 3);
    }
}


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
    m_shadowPipeline->setDepthBias(2);             // zkus 1..4,
    m_shadowPipeline->setSlopeScaledDepthBias(1.1f);
    m_shadowPipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    m_shadowPipeline->setCullMode(QRhiGraphicsPipeline::Front);
 //   m_shadowPipeline->setCullMode(QRhiGraphicsPipeline::Back);
    m_shadowPipeline->create();
}
