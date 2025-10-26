#include "rhiwidget.h"
#include <QFile>

static QShader getShader(const QString &name)
{
    QFile f(name);
    return f.open(QIODevice::ReadOnly)
               ? QShader::fromSerialized(f.readAll())
               : QShader();
}


void RhiWidget::loadTexture(const QSize &, QRhiResourceUpdateBatch *u)
{
    if (m_texture)
        return;

    QImage image(":/text.jpg");
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
        image = image.mirrored();
    // .flipped(Qt::Horizontal | Qt::Vertical);
    m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, image.size()));
    m_texture->create();

    u->uploadTexture(m_texture.get(), image);
}
void RhiWidget::initialize(QRhiCommandBuffer *cb)
{
    m_rhi = rhi();

    m_rp.reset(renderTarget()->renderPassDescriptor());
   // m_sc->setRenderPassDescriptor(m_rp.get());
    m_initialUpdates = m_rhi->nextResourceUpdateBatch();
    if (m_pixelSize != renderTarget()->pixelSize()) {
        m_pixelSize = renderTarget()->pixelSize();
       // emit resized();
    }
    if (m_sampleCount != renderTarget()->sampleCount()) {
        m_sampleCount = renderTarget()->sampleCount();
       // scene = {};
    }

    m_viewProjection = m_rhi->clipSpaceCorrMatrix();
    m_viewProjection.perspective(45.0f, m_pixelSize.width() / (float) m_pixelSize.height(), 0.01f, 1000.0f);

    m_viewProjection.translate(0, 0, -4);

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

void RhiWidget::render(QRhiCommandBuffer *cb)
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
   //  QSize out = renderTarget()->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, m_pixelSize.width(), m_pixelSize.height()));
  //   cb->setViewport(QRhiViewport(0, 0, out.width(), out.height()));
    m_cube1.draw(cb);
    m_cube2.draw(cb);

    cb->endPass();

    update();
}
