#ifndef PROCEDURALSKYRHI_H
#define PROCEDURALSKYRHI_H


#include <rhi/qrhi.h>
#include <QVector4D>
#include <memory> // Pro std::unique_ptr
#include <QMatrix4x4>
#include <QVector3D>
#include <QFile>

struct SkyUniforms {
    QMatrix4x4 invProjection;
    QMatrix4x4 invView;
    alignas(16) QVector3D sunDirection;
    float time;
};
struct alignas(16) GpuUniforms {
    float invProjection[16];       // offset 0   (64 B)
    float invView[16];        // offset 64  (64 B)
    float sunDirection[16];  // offset 128 (64 B)
    float params[16];  // offset 192 (64 B)
};

class ProceduralSkyRHI {
public:

    ProceduralSkyRHI(QRhi *rhi,QRhiRenderPassDescriptor *rp)
        : m_rhi(rhi)
    {
        m_ubo.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic,
                                     QRhiBuffer::UniformBuffer,
                                     sizeof(GpuUniforms)));
        if (!m_ubo->create()) {
            qWarning("Failed to create sky UBO");
            return;
        }
        m_srb.reset(m_rhi->newShaderResourceBindings());
        m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, // binding = 0
                                                     QRhiShaderResourceBinding::FragmentStage,
                                                     m_ubo.get())
        });
        if (!m_srb->create()) {
            qWarning("Failed to create sky SRB");
            return;
        }

        m_pipeline.reset(m_rhi->newGraphicsPipeline());

        m_pipeline->setShaderStages({
            { QRhiShaderStage::Vertex, getShader(QLatin1String(":/shaders/prebuild/pcgsky.vert.qsb")) },
            { QRhiShaderStage::Fragment, getShader(QLatin1String(":/shaders/prebuild/pcgsky.frag.qsb")) }
        });

        QRhiVertexInputLayout inputLayout;
        m_pipeline->setVertexInputLayout(inputLayout);

        m_pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
        m_pipeline->setShaderResourceBindings(m_srb.get());
        m_pipeline->setRenderPassDescriptor(rp);
        m_pipeline->setDepthTest(true);
        m_pipeline->setDepthWrite(true);
        m_pipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);

        if (!m_pipeline->create()) {
            qWarning("Failed to create sky pipeline");
            return;
        }
    }
    static QShader getShader(const QString &name)
    {
        QFile f(name);
        if (f.open(QIODevice::ReadOnly))
            return QShader::fromSerialized(f.readAll());
        return QShader();
    }

    void update(QRhiResourceUpdateBatch *batch,
                const QMatrix4x4 &invView,
                const QMatrix4x4 &invProjection,
                const QVector3D &sunDirection,
                float time)
    {
        m_uboData.invProjection = invProjection;
        m_uboData.invView = invView;
        m_uboData.sunDirection = sunDirection;
        m_uboData.time = time;

        GpuUniforms gpuUbo{};
        memcpy(gpuUbo.invProjection,invProjection.constData(), 64);
        memcpy(gpuUbo.invView,invView.constData(),       64);
        gpuUbo.sunDirection[0] = sunDirection.x();
        gpuUbo.sunDirection[1] = sunDirection.y();
        gpuUbo.sunDirection[2] = sunDirection.z();
        gpuUbo.sunDirection[3] = 1.0f;
        gpuUbo.params[0] = time;
        gpuUbo.params[1] = 1.0f;
        gpuUbo.params[2] = 1.0;
        gpuUbo.params[3] = 1.0f;

        batch->updateDynamicBuffer(m_ubo.get(), 0, sizeof(SkyUniforms), &gpuUbo);
    }
    void draw(QRhiCommandBuffer *cb) {
        if (!m_pipeline) return;

        cb->setGraphicsPipeline(m_pipeline.get());
        cb->setShaderResources(m_srb.get());
        cb->draw(3);
    }


private:
    QRhi *m_rhi = nullptr;
    SkyUniforms m_uboData; // cpu dat
    std::unique_ptr<QRhiBuffer> m_ubo;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
};

#endif // PROCEDURALSKYRHI_H
