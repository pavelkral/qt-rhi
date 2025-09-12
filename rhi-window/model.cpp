#include "model.h"

void Model::init(QRhi *rhi,QRhiTexture *texture,QRhiSampler *sampler,QRhiRenderPassDescriptor *rp,
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

    const quint32 UBUF_SIZE = 512;
    m_ubuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,UBUF_SIZE ));
    m_ubuf->create();

    m_srb.reset(rhi->newShaderResourceBindings());
    m_srb->setBindings({
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
   // m_pipeline->setCullMode(QRhiGraphicsPipeline::Back);
    m_pipeline->setDepthTest(true);
    m_pipeline->setDepthWrite(true);
 //   m_pipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(rp);
    m_pipeline->create();
}
void Model::updateUniforms(const QMatrix4x4 &viewProjection,float opacity, QRhiResourceUpdateBatch *u)
{


    u->updateDynamicBuffer(m_ubuf.get(), 64, 4, &opacity);

    QMatrix4x4 modelMatrix = transform.getModelMatrix();
    QMatrix4x4 mvp = viewProjection * modelMatrix;

    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
}
void Model::updateUbo(const QMatrix4x4 &view,
                          const QMatrix4x4 &projection,
                          const QMatrix4x4 &lightSpace,
                          const QVector3D &color,
                          float opacity,
                          QRhiResourceUpdateBatch *u )
{


    Ubo ubo;
    ubo.mvp         = projection * view * transform.getModelMatrix();
    ubo.opacity     = QVector4D(opacity, 0.0f, 0.0f, 0.0f);  // vec4
    ubo.model       = transform.getModelMatrix();
    ubo.view        = view;
    ubo.projection  = projection;
    ubo.lightSpace  = lightSpace;
    ubo.lightPos    = QVector4D(0.0f, 5.0f, 0.0f, 1.0f);
    ubo.color       = QVector4D(color, 1.0f);

   // u->updateDynamicBuffer(m_ubuf.get(), 0, sizeof(Ubo), &ubo);

   //  qDebug() << "--- UBO Debug Information ---";
   //  qDebug() << "Size of UBO struct:" << sizeof(Ubo) << "bytes";
   //  qDebug() << "\n## Data Member: mvp (Model-View-Projection Matrix)";
   //  qDebug() << "Type: QMatrix4x4, Size:" << sizeof(QMatrix4x4) << "bytes";
   //  qDebug() << "Type const: QMatrix4x4, Size:" << sizeof(ubo.mvp.constData()) << "bytes";
   //  qDebug() << "\n## Data Member: opacity";
   //  qDebug() << "Type: float, Size:" << sizeof(float) << "bytes";
   //  qDebug() << "Type const: float, Size:" << sizeof(ubo.opacity) << "bytes";
   //  qDebug() << "\n## Data Member: lightPos (Light Position)";
   //  qDebug() << "Type: QVector4D, Size:" << sizeof(QVector4D) << "bytes";
   //  qDebug() << "Type const: QVector4D, Size:" << sizeof(ubo.lightPos) << "bytes";
   //  qDebug() << "--- End of UBO Debug ---";

     u->updateDynamicBuffer(m_ubuf.get(), 0,   64, ubo.mvp.constData());
     u->updateDynamicBuffer(m_ubuf.get(), 64,  16, reinterpret_cast<const float*>(&ubo.opacity));
     u->updateDynamicBuffer(m_ubuf.get(), 80,  64, ubo.model.constData());
     u->updateDynamicBuffer(m_ubuf.get(), 144, 64, ubo.view.constData());
     u->updateDynamicBuffer(m_ubuf.get(), 208, 64, ubo.projection.constData());
     u->updateDynamicBuffer(m_ubuf.get(), 272, 64, ubo.lightSpace.constData());
     u->updateDynamicBuffer(m_ubuf.get(), 336, 16, reinterpret_cast<const float*>(&ubo.lightPos));
     u->updateDynamicBuffer(m_ubuf.get(), 352, 16, reinterpret_cast<const float*>(&ubo.color));

}
void Model::setModelMatrix(const QMatrix4x4 &mvp, QRhiResourceUpdateBatch *u)
{
    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
}
void Model::draw(QRhiCommandBuffer *cb)
{
    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setShaderResources(m_srb.get());
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding, m_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(m_indexCount);
}
