

#include "rhiwindow.h"
#include <QPlatformSurfaceEvent>
#include <QPainter>
#include <QFile>
#include <QImage>
#include <QFont>
#include <rhi/qshader.h>

//================================== RhiWindow ==================================

//! [rhiwindow-ctor]
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
//! [rhiwindow-ctor]

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

//! [expose]
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
//! [expose]

//! [event]
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
//! [event]

//! [rhi-init]
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
    //! [rhi-init]

    //! [swapchain-init]
    m_sc.reset(m_rhi->newSwapChain());
    m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                      QSize(),
                                      1,
                                      QRhiRenderBuffer::UsedWithSwapChainOnly));
    m_sc->setWindow(this);
    m_sc->setDepthStencil(m_ds.get());
    m_rp.reset(m_sc->newCompatibleRenderPassDescriptor());
    m_sc->setRenderPassDescriptor(m_rp.get());
    //! [swapchain-init]

    customInit();
}

//! [swapchain-resize]
void RhiWindow::resizeSwapChain()
{
    m_hasSwapChain = m_sc->createOrResize(); // also handles m_ds

    const QSize outputSize = m_sc->currentPixelSize();
    m_viewProjection = m_rhi->clipSpaceCorrMatrix();
    m_viewProjection.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.01f, 1000.0f);
    m_viewProjection.translate(0, 0, -4);
}
//! [swapchain-resize]

void RhiWindow::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_sc->destroy();
    }
}

//! [render-precheck]
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
    //! [beginframe]

    //! [request-update]
    m_rhi->endFrame(m_sc.get());
    requestUpdate();
}
//! [request-update]

//================================== Helpery =====================================

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());
    return QShader();
}

//================================== HelloWindow =================================

// 36 vrcholů = 6 stěn × 2 trojúhelníky × 3 vrcholy; každý vrchol: pos(3) + uv(2)
static float cubeVertexData[] = {
    // front
    -0.5f,-0.5f, 0.5f, 0.0f,0.0f,
    0.5f,-0.5f, 0.5f, 1.0f,0.0f,
    0.5f, 0.5f, 0.5f, 1.0f,1.0f,
    -0.5f,-0.5f, 0.5f, 0.0f,0.0f,
    0.5f, 0.5f, 0.5f, 1.0f,1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f,1.0f,
    // back
    -0.5f,-0.5f,-0.5f, 1.0f,0.0f,
    0.5f, 0.5f,-0.5f, 0.0f,1.0f,
    0.5f,-0.5f,-0.5f, 0.0f,0.0f,
    -0.5f,-0.5f,-0.5f, 1.0f,0.0f,
    -0.5f, 0.5f,-0.5f, 1.0f,1.0f,
    0.5f, 0.5f,-0.5f, 0.0f,1.0f,
    // left
    -0.5f,-0.5f,-0.5f, 0.0f,0.0f,
    -0.5f,-0.5f, 0.5f, 1.0f,0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f,1.0f,
    -0.5f,-0.5f,-0.5f, 0.0f,0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f,1.0f,
    -0.5f, 0.5f,-0.5f, 0.0f,1.0f,
    // right
    0.5f,-0.5f,-0.5f, 1.0f,0.0f,
    0.5f, 0.5f, 0.5f, 0.0f,1.0f,
    0.5f,-0.5f, 0.5f, 0.0f,0.0f,
    0.5f,-0.5f,-0.5f, 1.0f,0.0f,
    0.5f, 0.5f,-0.5f, 1.0f,1.0f,
    0.5f, 0.5f, 0.5f, 0.0f,1.0f,
    // top
    -0.5f, 0.5f, 0.5f, 0.0f,0.0f,
    0.5f, 0.5f, 0.5f, 1.0f,0.0f,
    0.5f, 0.5f,-0.5f, 1.0f,1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f,0.0f,
    0.5f, 0.5f,-0.5f, 1.0f,1.0f,
    -0.5f, 0.5f,-0.5f, 0.0f,1.0f,
    // bottom
    -0.5f,-0.5f, 0.5f, 0.0f,1.0f,
    0.5f,-0.5f,-0.5f, 1.0f,0.0f,
    0.5f,-0.5f, 0.5f, 1.0f,1.0f,
    -0.5f,-0.5f, 0.5f, 0.0f,1.0f,
    -0.5f,-0.5f,-0.5f, 0.0f,0.0f,
    0.5f,-0.5f,-0.5f, 1.0f,0.0f
};

//------------------------------------------------------------------------------

HelloWindow::HelloWindow(QRhi::Implementation graphicsApi)
    : RhiWindow(graphicsApi)
{
}

// Nahradíme původní loadTexture za loader textury z resource.
void HelloWindow::loadTexture(const QSize &, QRhiResourceUpdateBatch *u)
{
    if (m_texture)
        return;

    QImage image(":/text.jpg");          // <- vlož svou texturu do resource pod tímto aliasem
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

    m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, image.size()));
    m_texture->create();

    u->uploadTexture(m_texture.get(), image);
}

//! [render-init-1]
void HelloWindow::customInit()
{
    m_initialUpdates = m_rhi->nextResourceUpdateBatch();

    // Vertex buffer s krychlí
    m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cubeVertexData)));
    m_vbuf->create();
    m_initialUpdates->uploadStaticBuffer(m_vbuf.get(), cubeVertexData);

    // Uniform buffer: 64 B pro mat4 mvp
    static const quint32 UBUF_SIZE = 64;
    m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE));
    m_ubuf->create();
    //! [render-init-1]

    // Načti texturu (používáme stávající API)
    loadTexture(QSize(), m_initialUpdates);

    // Sampler
    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_sampler->create();

    // Shader resource bindings: binding 0 = UBO, binding 1 = texture+sampler
    m_colorTriSrb.reset(m_rhi->newShaderResourceBindings());
    m_colorTriSrb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0,
                                                 QRhiShaderResourceBinding::VertexStage, m_ubuf.get()),
        QRhiShaderResourceBinding::sampledTexture(1,
                                                  QRhiShaderResourceBinding::FragmentStage, m_texture.get(), m_sampler.get())
    });
    m_colorTriSrb->create();

    // Grafická pipeline pro texturovanou krychli
    m_colorPipeline.reset(m_rhi->newGraphicsPipeline());
    m_colorPipeline->setDepthTest(true);
    m_colorPipeline->setDepthWrite(true);

    m_colorPipeline->setShaderStages({
        { QRhiShaderStage::Vertex,   getShader(QLatin1String(":/texture.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture.frag.qsb")) }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 5 * sizeof(float) } // pos(3) + uv(2)
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) }
    });
    m_colorPipeline->setVertexInputLayout(inputLayout);
    m_colorPipeline->setShaderResourceBindings(m_colorTriSrb.get());
    m_colorPipeline->setRenderPassDescriptor(m_rp.get());
    m_colorPipeline->create();

    cube.init(m_rhi.get(), m_initialUpdates);
    // (Nepotřebujeme fullscreen quad pipeline — tu prostě nevytváříme.)
}

//! [render-1]
void HelloWindow::customRender()
{
    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();

    if (m_initialUpdates) {
        resourceUpdates->merge(m_initialUpdates);
        m_initialUpdates->release();
        m_initialUpdates = nullptr;
    }
    //! [render-1]

    //! [render-rotation]
    m_rotation += 1.0f;
    QMatrix4x4 modelViewProjection = m_viewProjection;
    modelViewProjection.rotate(m_rotation, 0, 1, 0);
    // rotuj po více osách
    resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 0, 64, modelViewProjection.constData());
    //! [render-rotation]

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    const QSize outputSizeInPixels = m_sc->currentPixelSize();

    cb->beginPass(m_sc->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, resourceUpdates);


    cb->setGraphicsPipeline(m_colorPipeline.get());
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    cube.draw(cb);
    cb->endPass();
}
