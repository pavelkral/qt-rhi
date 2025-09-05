#include "cube.h"

std::unique_ptr<QRhiBuffer> Cube::s_vbuf;
std::unique_ptr<QRhiBuffer> Cube::s_ibuf;
int Cube::s_indexCount = 0;
bool Cube::s_inited = false;

void Cube::ensureGeometry(QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    if (s_inited) return;

    // Vertex buffer
    s_vbuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)));
    s_vbuf->create();
    u->uploadStaticBuffer(s_vbuf.get(), vertices);

    // Index buffer
    s_ibuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices)));
    s_ibuf->create();
    u->uploadStaticBuffer(s_ibuf.get(), indices);

    s_indexCount = sizeof(indices) / sizeof(indices[0]);
    s_inited = true;
}

void Cube::init(QRhi *rhi,
                QRhiTexture *texture,
                QRhiSampler *sampler,
                QRhiRenderPassDescriptor *rp,
                QRhiResourceUpdateBatch *u)
{
    ensureGeometry(rhi, u);

    // Uniform buffer pro MVP
    m_ubuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    m_ubuf->create();

    // Shader resource bindings: binding 0 = UBO, binding 1 = texture+sampler
    m_srb.reset(rhi->newShaderResourceBindings());
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0,
                                                 QRhiShaderResourceBinding::VertexStage, m_ubuf.get()),
        QRhiShaderResourceBinding::sampledTexture(1,
                                                  QRhiShaderResourceBinding::FragmentStage, texture, sampler)
    });
    m_srb->create();
}

void Cube::setModelMatrix(const QMatrix4x4 &mvp, QRhiResourceUpdateBatch *u)
{
    m_modelMatrix = mvp;
    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
}

void Cube::draw(QRhiCommandBuffer *cb, QRhiGraphicsPipeline *pipeline)
{
    cb->setGraphicsPipeline(pipeline);
    cb->setShaderResources(m_srb.get());
    const QRhiCommandBuffer::VertexInput vbufBinding(s_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding, s_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(s_indexCount);
}
