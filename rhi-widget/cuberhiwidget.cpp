#include "cuberhiwidget.h"
#include <QFile>

static QShader getShader(const QString &name)
{
    QFile f(name);
    return f.open(QIODevice::ReadOnly)
               ? QShader::fromSerialized(f.readAll())
               : QShader();
}


void CubeRhiWidget::loadTexture(const QSize &, QRhiResourceUpdateBatch *u)
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
    // .flipped(Qt::Horizontal | Qt::Vertical);
    m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, image.size()));
    m_texture->create();

    u->uploadTexture(m_texture.get(), image);
}
void CubeRhiWidget::initialize(QRhiCommandBuffer *cb)
{
    m_rhi = rhi();

    m_rp.reset(renderTarget()->renderPassDescriptor());
   // m_sc->setRenderPassDescriptor(m_rp.get());
    m_initialUpdates = m_rhi->nextResourceUpdateBatch();

    // Vertex buffer s krychlí
    //m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cubeVertexData)));
    //m_vbuf->create();
    //m_initialUpdates->uploadStaticBuffer(m_vbuf.get(), cubeVertexData);
    // Uniform buffer: 64 B pro mat4 mvp
    // static const quint32 UBUF_SIZE = 64;
    //  m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE));
    //   m_ubuf->create();

QSize out = renderTarget()->pixelSize();

    m_viewProjection = m_rhi->clipSpaceCorrMatrix();
    m_viewProjection.perspective(45.0f, out.width() / (float) out.height(), 0.01f, 1000.0f);
    m_viewProjection.translate(0, 0, -4);
    // Načti texturu (používáme stávající API)
    loadTexture(QSize(), m_initialUpdates);

    // Sampler
    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_sampler->create();



    QShader vs = getShader(":/shader_assets/cube.vert.qsb");
    QShader fs = getShader(":/shader_assets/cube.frag.qsb");

    m_cube1.init(m_rhi, m_texture.get(), m_sampler.get(), m_rp.get(), vs, fs, m_initialUpdates);
    m_cube2.init(m_rhi, m_texture.get(), m_sampler.get(), m_rp.get(), vs, fs, m_initialUpdates);


}

void CubeRhiWidget::render(QRhiCommandBuffer *cb)
{

    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();

    if (m_initialUpdates) {
        resourceUpdates->merge(m_initialUpdates);
        m_initialUpdates->release();
        m_initialUpdates = nullptr;
    }

    m_rotation += 1.0f;
    QMatrix4x4 modelViewProjection = m_viewProjection;
    modelViewProjection.rotate(m_rotation, 0, 1, 0);

 //   QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
  //  const QSize outputSizeInPixels = currentPixelSize();
    //  cb->beginPass(m_sc->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, resourceUpdates);
    //  cb->setGraphicsPipeline(m_colorPipeline.get());
    //  cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    // cb->setShaderResources();

    QMatrix4x4 mvp1 = m_viewProjection;
    mvp1.translate(-1.5f, 0, 0);
    mvp1.rotate(m_rotation, 0, 1, 0);
    m_cube1.setModelMatrix(mvp1, resourceUpdates);

    QMatrix4x4 mvp2 = m_viewProjection;
    mvp2.translate(1.5f, 0, 0);
    mvp2.rotate(-m_rotation, 0, 1, 0);

    m_cube2.setModelMatrix(mvp2, resourceUpdates);
    const QColor clearColor = QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f);

    cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, resourceUpdates);
  //  cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
     QSize out = renderTarget()->pixelSize();
     cb->setViewport(QRhiViewport(0, 0, out.width(), out.height()));
    m_cube1.draw(cb);
    m_cube2.draw(cb);

    cb->endPass();



    update();
}
