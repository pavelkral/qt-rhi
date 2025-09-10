#include "cube.h"

void Cube::init(QRhi *rhi,QRhiTexture *texture,QRhiSampler *sampler,QRhiRenderPassDescriptor *rp,
                const QShader &vs,
                const QShader &fs,
                QRhiResourceUpdateBatch *u)
{

    m_vbuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, m_vert.size() * sizeof(float)));
    m_vbuf->create();
    u->uploadStaticBuffer(m_vbuf.get(), m_vert.constData());

    m_ibuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, m_ind.size() * sizeof(quint16)));
    m_ibuf->create();
    u->uploadStaticBuffer(m_ibuf.get(), m_ind.constData());

    m_indexCount = m_ind.size();

    const quint32 UBUF_SIZE = 256;
    m_ubuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,UBUF_SIZE ));
    m_ubuf->create();

    m_srb.reset(rhi->newShaderResourceBindings());
    m_srb->setBindings({
       // QRhiShaderResourceBinding::uniformBuffer(0,QRhiShaderResourceBinding::FragmentStage, m_ubuf.get()),
        QRhiShaderResourceBinding::uniformBuffer(0,QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage , m_ubuf.get()),
        QRhiShaderResourceBinding::sampledTexture(1,QRhiShaderResourceBinding::FragmentStage, texture, sampler)
    });
    m_srb->create();

    m_pipeline.reset(rhi->newGraphicsPipeline());
    m_pipeline->setDepthTest(true);
    m_pipeline->setDepthWrite(true);

    // D3D12
    m_pipeline->setTopology(QRhiGraphicsPipeline::Triangles);

    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout inputLayout;

    inputLayout.setBindings({
        { 8 * sizeof(float) } // stride: pos(3) + normal(3) + uv(2)
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },        // pos
        { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) },  // normal
        { 0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float) }   // uv
    });
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(rp);
    m_pipeline->create();
}
void Cube::updateUniforms(const QMatrix4x4 &viewProjection, QRhiResourceUpdateBatch *u)
{

    QMatrix4x4 modelMatrix = transform.getModelMatrix();
    QMatrix4x4 mvp = viewProjection * modelMatrix;

    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
}
// void Cube::updateUniforms(const QMatrix4x4 &view,
//                           const QMatrix4x4 &projection,
//                           const QMatrix4x4 &lightSpace,
//                           const QVector3D &color,
//                           float opacity,
//                           QRhiResourceUpdateBatch *u)
// {
//     Ubo ubo;
//     ubo.model = transform.getModelMatrix();
//     ubo.view = view;
//     ubo.projection = projection;
//     ubo.lightSpace = lightSpace;
//     ubo.color = QVector4D(color, 1.0f); // Konverze z QVector3D na QVector4D
//     ubo.opacity = opacity;

//     // Nahrajeme celou strukturu do uniformního bufferu
//     u->updateDynamicBuffer(m_ubuf.get(), 0, sizeof(Ubo), &ubo);
// }
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
