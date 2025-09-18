#include "model.h"
#include <cmath>


void Model::addVertAndInd(const QVector<float> &vertices, const QVector<quint16> &indices) {
    m_vert = vertices;
    m_ind = indices;
    m_indexCount = indices.size();
}

void Model::init(QRhi *rhi,QRhiRenderPassDescriptor *rp,
                const QShader &vs,
                const QShader &fs,
                QRhiResourceUpdateBatch *u,QRhiTexture *shadowmap,QRhiSampler *shadowsampler)
{

    QVector<float>  m_vert1= computeTangents( m_vert,m_ind);

    m_vbuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, m_vert1.size() * sizeof(float)));
    m_vbuf->create();
    u->uploadStaticBuffer(m_vbuf.get(), m_vert1.constData());

    m_ibuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, m_ind.size() * sizeof(quint16)));
    m_ibuf->create();
    u->uploadStaticBuffer(m_ibuf.get(), m_ind.constData());

    m_indexCount = m_ind.size();

    const quint32 UBUF_SIZE = 512;
    m_ubuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,UBUF_SIZE ));
    m_ubuf->create();

    TextureSet set;
    set.albedo = ":/assets/textures/brick/victorian-brick_albedo.png";
    set.normal = ":/assets/textures/brick/victorian-brick_normal-ogl.png";
    set.metallic = ":/assets/textures/brick/victorian-brick_metallic.png";
    set.rougness = ":/assets/textures/brick/victorian-brick_roughness.png";
    set.height = ":/assets/textures/brick/victorian-brick_height.png";
    set.ao = ":/assets/textures/brick/victorian-brick_ao.png";

    loadTexture(rhi,QSize(), u,set.albedo,m_texture,m_sampler);
    loadTexture(rhi,QSize(), u,set.normal,m_tex_norm,m_sampler_norm);
    loadTexture(rhi,QSize(), u,set.metallic,m_texture_met,m_sampler_met);
    loadTexture(rhi,QSize(), u,set.rougness,m_texture_rough,m_sampler_rough);
    loadTexture(rhi,QSize(), u,set.height,m_texture_height,m_sampler_height);
    loadTexture(rhi,QSize(), u,set.ao,m_texture_ao,m_sampler_ao);
    Q_ASSERT(shadowmap);
    Q_ASSERT(shadowsampler);
    m_srb.reset(rhi->newShaderResourceBindings());
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0,QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage , m_ubuf.get()),
        QRhiShaderResourceBinding::sampledTexture(1,QRhiShaderResourceBinding::FragmentStage, m_texture.get(), m_sampler.get()),
        QRhiShaderResourceBinding::sampledTexture(2,QRhiShaderResourceBinding::FragmentStage, m_tex_norm.get(), m_sampler_norm.get()),
        QRhiShaderResourceBinding::sampledTexture(3,QRhiShaderResourceBinding::FragmentStage, m_texture_met.get(), m_sampler_met.get()),
        QRhiShaderResourceBinding::sampledTexture(4,QRhiShaderResourceBinding::FragmentStage, m_texture_rough.get(), m_sampler_rough.get()),
        QRhiShaderResourceBinding::sampledTexture(5,QRhiShaderResourceBinding::FragmentStage, m_texture_ao.get(), m_sampler_ao.get()),
        QRhiShaderResourceBinding::sampledTexture(6,QRhiShaderResourceBinding::FragmentStage, m_texture_height.get(), m_sampler_height.get()),
        QRhiShaderResourceBinding::sampledTexture(7,QRhiShaderResourceBinding::FragmentStage, shadowmap, shadowsampler)
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
        { 14 * sizeof(float) }
    });

    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },                    // pos
        { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) },    // normal
        { 0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float) },    // uv
        { 0, 3, QRhiVertexInputAttribute::Float3, 8 * sizeof(float) },    // tangent
        { 0, 4, QRhiVertexInputAttribute::Float3, 11 * sizeof(float) }    // bitangent
    });
    //m_pipeline->setCullMode(QRhiGraphicsPipeline::Back);
    m_pipeline->setDepthTest(true);
    m_pipeline->setDepthWrite(true);
    //m_pipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(rp);
    m_pipeline->create();
}


void Model::updateUniforms(const QMatrix4x4 &viewProjection,float opacity, QRhiResourceUpdateBatch *u)
{
    QMatrix4x4 modelMatrix = transform.getModelMatrix();
    QMatrix4x4 mvp = viewProjection * modelMatrix;
    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
    u->updateDynamicBuffer(m_ubuf.get(), 64, 4, &opacity);
}
void Model::updateUbo(const QMatrix4x4 &view,
                        const QMatrix4x4 &projection,
                        const QMatrix4x4 &lightSpace,
                        const QVector3D &color,
                        const QVector3D &lightPos,
                        const QVector3D &camPos,
                        const float opacity,
                        Ubo ubo,
                        QRhiResourceUpdateBatch *u )
{

    struct alignas(16) GpuUbo {
        float model[16];       // offset 0   (64 B)
        float view[16];        // offset 64  (64 B)
        float projection[16];  // offset 128 (64 B)
        float lightSpace[16];  // offset 192 (64 B)

        float lightPos[4];     // offset 256 (16 B)
        float color[4];        // offset 272 (16 B)
        float camPos[4];       // offset 288 (16 B)
        float opacity[4];      // offset 304 (16 B)
        float debugMode;       // offset 320 (4 B)
        float lightIntensity[1];  // offset 324 (4 B)
        float _padding[2];     // offset 328 (8 B) → aby to zarovnalo na 336
    };

    static_assert(sizeof(GpuUbo) == 336, "GpuUbo must be 336 bytes");

    GpuUbo gpuUbo{};

    memcpy(gpuUbo.model,      transform.getModelMatrix().constData(), 64);
    memcpy(gpuUbo.view,       view.constData(),       64);
    memcpy(gpuUbo.projection, projection.constData(), 64);
    memcpy(gpuUbo.lightSpace, lightSpace.constData(), 64);

    gpuUbo.lightPos[0] = lightPos.x();
    gpuUbo.lightPos[1] = lightPos.y();
    gpuUbo.lightPos[2] = lightPos.z();
    gpuUbo.lightPos[3] = 1.0f;

    gpuUbo.color[0] = color.x();
    gpuUbo.color[1] = color.y();
    gpuUbo.color[2] = color.z();
    gpuUbo.color[3] = 1.0f;

    gpuUbo.camPos[0] = camPos.x();
    gpuUbo.camPos[1] = camPos.y();
    gpuUbo.camPos[2] = camPos.z();
    gpuUbo.camPos[3] = 1.0f;

    gpuUbo.opacity[0] = 0.0f;
    gpuUbo.opacity[1] = 0.0f;
    gpuUbo.opacity[2] = 0.0f;
    gpuUbo.opacity[3] = opacity;

    gpuUbo.debugMode      = float(ubo.debugMode);
    gpuUbo.lightIntensity[0] = float(ubo.lightIntensity);;

    //u->updateDynamicBuffer(m_ubuf.get(), 0, sizeof(GpuUbo), &gpuUbo);
   // u->updateDynamicBuffer(m_ubuf.get(), 0, sizeof(Ubo), &ubo);

     u->updateDynamicBuffer(m_ubuf.get(), 0,   64, transform.getModelMatrix().constData());
     u->updateDynamicBuffer(m_ubuf.get(), 64, 64, ubo.view.constData());
     u->updateDynamicBuffer(m_ubuf.get(), 128, 64, ubo.projection.constData());
    u->updateDynamicBuffer(m_ubuf.get(), 192, 64, ubo.lightSpace.constData());
    u->updateDynamicBuffer(m_ubuf.get(), 256, 16, reinterpret_cast<const float*>(&ubo.lightPos));
    u->updateDynamicBuffer(m_ubuf.get(), 272, 16, reinterpret_cast<const float*>(&ubo.lightColor));
    u->updateDynamicBuffer(m_ubuf.get(), 288, 16, reinterpret_cast<const float*>(&ubo.camPos));
    u->updateDynamicBuffer(m_ubuf.get(), 304,  16, reinterpret_cast<const float*>(&ubo.opacity));
    u->updateDynamicBuffer(m_ubuf.get(), 304,  4, reinterpret_cast<const float*>(&ubo.debugMode));
    u->updateDynamicBuffer(m_ubuf.get(), 308,  4, reinterpret_cast<const float*>(&ubo.lightIntensity));

}

void Model::draw(QRhiCommandBuffer *cb)
{
    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setShaderResources(m_srb.get());
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding, m_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(m_indexCount);
}

void Model::DrawForShadow(QRhiCommandBuffer *cb,
                             QRhiGraphicsPipeline *shadowPipeline,
                             QRhiShaderResourceBindings *shadowSRB,
                             QRhiBuffer *shadowUbo,
                             const QMatrix4x4& lightSpaceMatrix,
                            Ubo ubo,
                            QRhiResourceUpdateBatch *u) const
{
    u->updateDynamicBuffer(shadowUbo, 0,   64, transform.getModelMatrix().constData());
    u->updateDynamicBuffer(shadowUbo, 64, 64, ubo.view.constData());
    u->updateDynamicBuffer(shadowUbo, 128, 64, ubo.projection.constData());
    u->updateDynamicBuffer(shadowUbo, 192, 64,lightSpaceMatrix.constData());
    u->updateDynamicBuffer(shadowUbo, 256, 16, reinterpret_cast<const float*>(&ubo.lightPos));
    u->updateDynamicBuffer(shadowUbo, 272, 16, reinterpret_cast<const float*>(&ubo.lightColor));
    u->updateDynamicBuffer(shadowUbo, 288, 16, reinterpret_cast<const float*>(&ubo.camPos));
    u->updateDynamicBuffer(shadowUbo, 304,  16, reinterpret_cast<const float*>(&ubo.opacity));
    u->updateDynamicBuffer(shadowUbo, 304,  4, reinterpret_cast<const float*>(&ubo.debugMode));
    u->updateDynamicBuffer(shadowUbo, 308,  4, reinterpret_cast<const float*>(&ubo.lightIntensity));

    cb->setGraphicsPipeline(shadowPipeline);
    cb->setShaderResources(shadowSRB);
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding, m_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(m_indexCount);
}

void Model::loadTexture(QRhi *m_rhi,const QSize &, QRhiResourceUpdateBatch *u,QString tex_name,std::unique_ptr<QRhiTexture> &texture,
                        std::unique_ptr<QRhiSampler> &sampler)
{
    if (texture)
        return;

    QImage image(tex_name);
    if (image.isNull()) {
        qWarning("Failed to texture. Using 64x64 checker fallback.");
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
        image = image.flipped(Qt::Horizontal | Qt::Vertical); // UV invert
    // .mirrored();

    int mipLevels = static_cast<int>(floor(log2(qMax(image.width(), image.height())))) + 1;

    texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8,
                                      image.size(),
                                      1,
                                      QRhiTexture::Flags(QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips)));
    texture->create();
    u->uploadTexture(texture.get(), image);
    u->generateMips(texture.get());

    sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                    QRhiSampler::Repeat, QRhiSampler::Repeat));
    sampler->create();
}

QVector<float> Model::computeTangents(const QVector<float>& vertices, const QVector<quint16>& indices)
{
    const int strideIn = 8;   // pos(3) + normal(3) + uv(2)
    const int strideOut = 14; // pos(3) + normal(3) + uv(2) + tangent(3) + bitangent(3)
    const int vertexCount = vertices.size() / strideIn;

    QVector<Vertex> temp(vertexCount);
    // fill data
    for (int i = 0; i < vertexCount; i++) {
        temp[i].pos = QVector3D(vertices[i*strideIn+0],
                                vertices[i*strideIn+1],
                                vertices[i*strideIn+2]);
        temp[i].normal = QVector3D(vertices[i*strideIn+3],
                                   vertices[i*strideIn+4],
                                   vertices[i*strideIn+5]);
        temp[i].uv = QVector2D(vertices[i*strideIn+6],
                               vertices[i*strideIn+7]);
        temp[i].tangent = QVector3D(0,0,0);
        temp[i].bitangent = QVector3D(0,0,0);
    }

    // tangentu/bitangentu for triangles
    for (int i = 0; i < indices.size(); i += 3) {
        Vertex& v0 = temp[indices[i]];
        Vertex& v1 = temp[indices[i+1]];
        Vertex& v2 = temp[indices[i+2]];

        QVector3D edge1 = v1.pos - v0.pos;
        QVector3D edge2 = v2.pos - v0.pos;
        QVector2D deltaUV1 = v1.uv - v0.uv;
        QVector2D deltaUV2 = v2.uv - v0.uv;

        float f = 1.0f;
        float det = deltaUV1.x() * deltaUV2.y() - deltaUV2.x() * deltaUV1.y();
        if (fabs(det) > 1e-6f) {
            f = 1.0f / det;
        }

        QVector3D tangent(
            f * (deltaUV2.y() * edge1.x() - deltaUV1.y() * edge2.x()),
            f * (deltaUV2.y() * edge1.y() - deltaUV1.y() * edge2.y()),
            f * (deltaUV2.y() * edge1.z() - deltaUV1.y() * edge2.z())
            );

        QVector3D bitangent(
            f * (-deltaUV2.x() * edge1.x() + deltaUV1.x() * edge2.x()),
            f * (-deltaUV2.x() * edge1.y() + deltaUV1.x() * edge2.y()),
            f * (-deltaUV2.x() * edge1.z() + deltaUV1.x() * edge2.z())
            );

        v0.tangent += tangent; v1.tangent += tangent; v2.tangent += tangent;
        v0.bitangent += bitangent; v1.bitangent += bitangent; v2.bitangent += bitangent;
    }

    // normalize
    for (int i = 0; i < vertexCount; i++) {
        temp[i].tangent.normalize();
        temp[i].bitangent.normalize();
    }

    QVector<float> out;
    out.resize(vertexCount * strideOut);

    for (int i = 0; i < vertexCount; i++) {
        out[i*strideOut + 0] = temp[i].pos.x();
        out[i*strideOut + 1] = temp[i].pos.y();
        out[i*strideOut + 2] = temp[i].pos.z();

        out[i*strideOut + 3] = temp[i].normal.x();
        out[i*strideOut + 4] = temp[i].normal.y();
        out[i*strideOut + 5] = temp[i].normal.z();

        out[i*strideOut + 6] = temp[i].uv.x();
        out[i*strideOut + 7] = temp[i].uv.y();

        out[i*strideOut + 8]  = temp[i].tangent.x();
        out[i*strideOut + 9]  = temp[i].tangent.y();
        out[i*strideOut + 10] = temp[i].tangent.z();

        out[i*strideOut + 11] = temp[i].bitangent.x();
        out[i*strideOut + 12] = temp[i].bitangent.y();
        out[i*strideOut + 13] = temp[i].bitangent.z();
    }

    return out;
}
