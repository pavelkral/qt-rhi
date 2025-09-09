

#include "rhiwindow.h"
#include <QPlatformSurfaceEvent>
#include <QPainter>
#include <QFile>
#include <QImage>
#include <QFont>
#include <rhi/qshader.h>
#include "geometry.h"

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
    m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,QSize(),1,
                                      QRhiRenderBuffer::UsedWithSwapChainOnly));
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
  //  m_viewProjection = m_rhi->clipSpaceCorrMatrix();
  //  m_viewProjection.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.01f, 1000.0f);
  //  m_viewProjection.translate(0, 0, -4);
    //const QSize outputSize = m_sc->currentPixelSize();
    // Přejmenujeme m_viewProjection na m_projection
    m_projection = m_rhi->clipSpaceCorrMatrix();
    m_projection.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.1f, 100.0f);
    // Odstraníme .translate(0, 0, -4); protože pozici řeší kamera
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
    //! [render-precheck]
    //! [render-resize]
    if (m_sc->currentPixelSize() != m_sc->surfacePixelSize() || m_newlyExposed) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        m_newlyExposed = false;
    }
    //! [render-resize]
    //! [beginframe]
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

HelloWindow::HelloWindow(QRhi::Implementation graphicsApi)
    : RhiWindow(graphicsApi),
    m_camera(QVector3D(0.0f, 0.0f, 5.0f))
{
    //setFocusPolicy(Qt::StrongFocus);
    // Skryje kurzor a zachytí ho v okně
   // setCursor(Qt::BlankCursor);
}

void HelloWindow::loadTexture(const QSize &, QRhiResourceUpdateBatch *u)
{
    if (m_texture)
        return;

    QImage image(":/assets/textures/floor.png");
    if (image.isNull()) {
        qWarning("Failed to load :/crate.png texture. Using 64x64 checker fallback.");
        image = QImage(64, 64, QImage::Format_RGBA8888);
        image.fill(Qt::white);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                if (((x / 8) + (y / 8)) & 1)
                    image.setPixelColor(x, y, QColor(50, 50, 50));
    } else {
        image = image.convertToFormat(QImage::Format_RGBA8888);
    }

    if (m_rhi->isYUpInNDC())
        image = image.mirrored(); // aby UV nebylo vzhůru nohama na některých backendech
   // .flipped(Qt::Horizontal | Qt::Vertical);
    m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, image.size()));
    m_texture->create();

    u->uploadTexture(m_texture.get(), image);
}

void HelloWindow::customInit()
{
    m_initialUpdates = m_rhi->nextResourceUpdateBatch();

    loadTexture(QSize(), m_initialUpdates);

    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_sampler->create();

    QShader vs = getShader(":/texture.vert.qsb");
    QShader fs = getShader(":/texture.frag.qsb");

    QShader vs1 = getShader(":/light.vert.qsb");
    QShader fs1 = getShader(":/light.frag.qsb");


    m_cube1.addVertAndInd(cubeVertices, cubeIndices);
     m_cube2.addVertAndInd(planeVertices, planeIndices);


    m_cube1.init(m_rhi.get(), m_texture.get(), m_sampler.get(), m_rp.get(), vs, fs, m_initialUpdates);
    m_cube2.init(m_rhi.get(), m_texture.get(), m_sampler.get(), m_rp.get(), vs, fs, m_initialUpdates);

    m_cube1.transform.position = QVector3D(-1.5f, 0, 0);
    m_cube2.transform.position = QVector3D(1.5f, 0, 0);


    m_timer.start();
}


void HelloWindow::customRender()
{
    m_dt = m_timer.restart() / 1000.0f;
    updateCamera(m_dt);

    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();

    if (m_initialUpdates) {
        resourceUpdates->merge(m_initialUpdates);
        m_initialUpdates->release();
        m_initialUpdates = nullptr;
    }

    m_rotation += 30.0f * m_dt;

    QMatrix4x4 view = m_camera.GetViewMatrix();
    QMatrix4x4 viewProjection = m_projection * view;
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    const QSize outputSizeInPixels = m_sc->currentPixelSize();

    m_cube1.transform.rotation.setY( m_cube1.transform.rotation.y() + 0.5f);
    m_cube1.updateUniforms(viewProjection, resourceUpdates);

    m_cube2.transform.rotation.setY(m_cube2.transform.rotation.y() + 0.5f); m_cube2.updateUniforms(viewProjection, resourceUpdates);


  //  QMatrix4x4 model2;
  //  model2.translate(1.5f, 0, 0);
  //  model2.rotate(-m_rotation, 0, 1, 0);
  //  QMatrix4x4 mvp2 = viewProjection * model2;
  //  m_cube2.setModelMatrix(mvp2, resourceUpdates);


    QMatrix4x4 lightSpaceMatrix; // Získaná ze světla (pro stíny)
    // ... výpočet lightSpaceMatrix ...
    QVector3D objectColor(1.0f, 0.8f, 0.5f); // Oranžový nádech
    float objectOpacity = 1.0f;

    // 3. Zavoláte novou metodu se všemi parametry
    // m_cube2.updateUniforms(view,
    //                       viewProjection,
    //                       lightSpaceMatrix,
    //                       objectColor,
    //                       objectOpacity,
    //                       resourceUpdates);


    // m_cube1.updateUniforms(view,
    //                        viewProjection,
    //                        lightSpaceMatrix,
    //                        objectColor,
    //                        objectOpacity,
    //                        resourceUpdates);

    const QColor clearColor = QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f);
    cb->beginPass(m_sc->currentFrameRenderTarget(), clearColor, { 1.0f, 0 }, resourceUpdates);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });

    m_cube1.draw(cb);
    m_cube2.draw(cb);

    cb->endPass();

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

    // Volitelně: Vracení kurzoru do středu okna pro neomezený pohyb
    // QPoint center = mapToGlobal(rect().center());
    // QCursor::setPos(center);
    // m_lastMousePos = mapFromGlobal(center);
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
}
