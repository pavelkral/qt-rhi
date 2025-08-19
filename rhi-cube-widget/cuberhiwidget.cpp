#include "cuberhiwidget.h"
#include <QFile>

static QShader getShader(const QString &name)
{
    QFile f(name);
    return f.open(QIODevice::ReadOnly)
               ? QShader::fromSerialized(f.readAll())
               : QShader();
}

// Krychlové vrcholy s UV souřadnicemi
static const struct verticies { float x, y, z, u, v; } vertices[] = {
    {-1,-1, 1, 0,0}, {1,-1, 1, 1,0}, {1,1, 1, 1,1}, {-1,1, 1, 0,1},
    {-1,-1,-1, 1,0}, {-1,1,-1, 1,1}, {1,1,-1, 0,1}, {1,-1,-1, 0,0},
    };

static const quint32 indices[] = {
    0,1,2, 2,3,0, 4,5,6, 6,7,4,
    3,2,6, 6,5,3, 0,4,7, 7,1,0,
    1,7,6, 6,2,1, 0,3,5, 5,4,0
};

void CubeRhiWidget::initialize(QRhiCommandBuffer *cb)
{
    m_rhi = rhi();

    if (!m_vbuf) {

        m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)));
        m_vbuf->create();
        m_ibuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices)));
        m_ibuf->create();
        m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(QMatrix4x4)));
        m_ubuf->create();

        QImage img(":/text.jpg");

        if (img.isNull()) {
            qWarning("Failed to load texture image!");
            return;
        }

        img = img.convertToFormat(QImage::Format_RGBA8888);

        if (m_rhi->isYUpInNDC())
            img = img.mirrored();

        m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, img.size()));


        if (!m_texture->create()) {
            qWarning("Failed to create texture");
            return;
        }
        else {
            qWarning("create texture");
        }

        QRhiResourceUpdateBatch *batch = m_rhi->nextResourceUpdateBatch();
        batch->uploadTexture(m_texture.get(), img);

        m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear,
                                      QRhiSampler::None, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        m_sampler->create();



        m_srb.reset(m_rhi->newShaderResourceBindings());
        m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, m_ubuf.data()),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                      m_texture.data(), m_sampler.data())
        });
        m_srb->create();

        m_pipeline.reset(m_rhi->newGraphicsPipeline());
        m_pipeline->setShaderStages({
            {QRhiShaderStage::Vertex, getShader(":/shader_assets/cube.vert.qsb")},
            {QRhiShaderStage::Fragment, getShader(":/shader_assets/cube.frag.qsb")}
        });
        QRhiVertexInputLayout il;
        il.setBindings({{5 * sizeof(float)}});
        il.setAttributes({
            {0, 0, QRhiVertexInputAttribute::Float3, 0},
            {0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float)}
        });
        m_pipeline->setVertexInputLayout(il);
        m_pipeline->setShaderResourceBindings(m_srb.data());
        m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
        m_pipeline->setSampleCount(renderTarget()->sampleCount());
        m_pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
        m_pipeline->create();

        QRhiResourceUpdateBatch *u = m_rhi->nextResourceUpdateBatch();
        u->uploadStaticBuffer(m_vbuf.data(), vertices);
        u->uploadStaticBuffer(m_ibuf.data(), indices);
        u->uploadTexture(m_texture.data(), img.convertToFormat(QImage::Format_RGBA8888));
        cb->resourceUpdate(u);

        QSize out = renderTarget()->pixelSize();
        m_viewProjection = m_rhi->clipSpaceCorrMatrix();
        m_viewProjection.perspective(45, out.width() / float(out.height()), 0.1f, 100.0f);
        m_viewProjection.translate(0, 0, -4);
    }
}

void CubeRhiWidget::render(QRhiCommandBuffer *cb)
{
    QRhiResourceUpdateBatch *u = rhi()->nextResourceUpdateBatch();

    m_rotation += 1.0f;
    QMatrix4x4 m = m_viewProjection;
    m.rotate(m_rotation, 1, 1, 0);
    u->updateDynamicBuffer(m_ubuf.data(), 0, sizeof(QMatrix4x4), m.constData());

   // QImage img(":/tex.jpg");
  //  img = img.convertToFormat(QImage::Format_RGBA8888);
  //  m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, img.size()));

  //  u->uploadTexture(m_texture.get(),img);
    cb->beginPass(renderTarget(), Qt::black, {1.0f, 0}, u);
    cb->setGraphicsPipeline(m_pipeline.data());
    QSize out = renderTarget()->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, out.width(), out.height()));
    cb->setShaderResources(m_srb.data());
    QRhiCommandBuffer::VertexInput vin{m_vbuf.data(), 0};
    cb->setVertexInput(0, 1, &vin, m_ibuf.data(), QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(36);
    cb->endPass();

    update();
}
