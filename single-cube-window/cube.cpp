#include "cube.h"

void Cube::init(QRhi *rhi,
                QRhiTexture *texture,
                QRhiSampler *sampler,
                QRhiRenderPassDescriptor *rp,
                const QShader &vs,
                const QShader &fs,
                QRhiResourceUpdateBatch *u)
{
    // Vertex buffer
    m_vbuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(m_vertices)));
    m_vbuf->create();
    u->uploadStaticBuffer(m_vbuf.get(), m_vertices);

    // Index buffer
    m_ibuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(m_indices)));
    m_ibuf->create();
    u->uploadStaticBuffer(m_ibuf.get(), m_indices);

    m_indexCount = sizeof(m_indices) / sizeof(m_indices[0]);

    // Uniform buffer
    m_ubuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    m_ubuf->create();

    // Shader resource bindings
    m_srb.reset(rhi->newShaderResourceBindings());
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, m_ubuf.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture, sampler)
    });
    m_srb->create();

    // Pipeline
    m_pipeline.reset(rhi->newGraphicsPipeline());
    m_pipeline->setDepthTest(true);
    m_pipeline->setDepthWrite(true);
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 5 * sizeof(float) } // stride
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) }
    });

    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(rp);
    m_pipeline->create();
}

void Cube::setModelMatrix(const QMatrix4x4 &mvp, QRhiResourceUpdateBatch *u)
{
    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
}

void Cube::draw(QRhiCommandBuffer *cb)
{
    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setShaderResources(m_srb.get());
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding, m_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(m_indexCount);
}
