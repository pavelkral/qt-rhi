#ifndef HDRI_SKY_QRHI_H
#define HDRI_SKY_QRHI_H

#include <QString>
#include <QImage>
#include <QDebug>
#include <QMatrix4x4>
#include <memory>
#include <vector>
#include <array>
#include <QFile>
#include <rhi/qrhi.h>
#include "stb/stb_image.h"

struct HdriVertex {
    QVector3D pos;
};
struct alignas(16) SkyUbo {
    float projection[16];
    float view[16];
};
class HdriSky {
public:
    explicit HdriSky(const QString &equirectPath = {}) : eqPath_(equirectPath) {}

    // Load just stores path; actual QRhi resources are created in create().
    void setEquirectangular(const QString &path) { eqPath_ = path; created_ = false; uploaded_ = false; }

    // Create QRhi resources (pipelines, textures, buffers). Must be called when
    // QRhi* is available (e.g. in the renderer init or when the device changes).
    void create(QRhi *rhi, QRhiRenderTarget * rt, QRhiRenderPassDescriptor * rp,QRhiResourceUpdateBatch *rub) {
        if (!rhi) return;
        if (rhi_ != rhi) {
            // drop device-local resources when device changed
            pipelineSky_.reset();
            pipelineE2C_.reset();
            vbuf_.reset();
            ubuf_.reset();
            srbSky_.reset();
            srbE2C_.reset();
            sampler_.reset();
            envCubemap_.reset();
            equirectTex_.reset();
            rhi_ = rhi;
            created_ = false;
            uploaded_ = false;
        }

        if (created_) return;

        // --- vertex buffer for a unit cube ---
        std::vector<HdriVertex> vertices = cubeVertices();
        vbuf_.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                   static_cast<quint32>(vertices.size() * sizeof(HdriVertex))));
        vbuf_->create();
        rub->uploadStaticBuffer(vbuf_.get(), 0,
                                static_cast<quint32>(vertices.size() * sizeof(HdriVertex)),
                                vertices.data());
        // uniform buffer for view/projection
        ubuf_.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,  sizeof(SkyUbo)));
        ubuf_->create();

        sampler_.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                       QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        sampler_->create();

        // Create empty cube map texture. We allocate levels for mipmaps later.
        // NOTE: for HDR you may want QRhiTexture::RGBA16F instead of RGBA8.
        // Create empty cube map texture
        envCubemap_.reset(rhi->newTexture(
            QRhiTexture::RGBA8,
            QSize(CUBEMAP_RESOLUTION, CUBEMAP_RESOLUTION),
            1,
            QRhiTexture::CubeMap | QRhiTexture::RenderTarget));
        envCubemap_->create();
        // Equirectangular texture (source). We attempt to load via QImage later
        equirectTex_.reset(rhi->newTexture(
            QRhiTexture::RGBA8,
            QSize(1, 1),
            1,
            QRhiTexture::Flag()));
        equirectTex_->create();
        // Shader resource bindings will be created together with pipelines

        // --- Equirectangular -> Cubemap pipeline (vertex + fragment) ---
        pipelineE2C_.reset(rhi->newGraphicsPipeline());
        pipelineE2C_->setTopology(QRhiGraphicsPipeline::Triangles);
        pipelineE2C_->setShaderStages({
            { QRhiShaderStage::Vertex, LoadShader(":/shaders/prebuild/equirect2cube.vert.qsb") },
            { QRhiShaderStage::Fragment, LoadShader(":/shaders/prebuild/equirect2cube.frag.qsb") }
        });
        QRhiVertexInputLayout layout{};
        layout.setBindings({ sizeof(HdriVertex) });
        layout.setAttributes({ {0, 0, QRhiVertexInputAttribute::Float3, 0} });
        pipelineE2C_->setVertexInputLayout(layout);

        // SRB for equirect->cube: binding 0 = ubuf, binding 1 = sampler2D
        std::vector<QRhiShaderResourceBinding> bindingsE2C;
        bindingsE2C.emplace_back(QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf_.get()));
        bindingsE2C.emplace_back(QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage, equirectTex_.get(), sampler_.get()));
        srbE2C_.reset(rhi->newShaderResourceBindings());
        srbE2C_->setBindings(bindingsE2C.begin(), bindingsE2C.end());
        srbE2C_->create();//
        pipelineE2C_->setShaderResourceBindings(srbE2C_.get());
        pipelineE2C_->setRenderPassDescriptor(rp);
        pipelineE2C_->setDepthTest(false);
        pipelineE2C_->setDepthWrite(false);
        pipelineE2C_->setCullMode(QRhiGraphicsPipeline::Back);
        pipelineE2C_->create();

        // --- Skybox pipeline (vertex + fragment) ---
        pipelineSky_.reset(rhi->newGraphicsPipeline());
        pipelineSky_->setTopology(QRhiGraphicsPipeline::Triangles);
        pipelineSky_->setShaderStages({
            { QRhiShaderStage::Vertex, LoadShader(":/shaders/prebuild/skybox.vert.qsb") },
            { QRhiShaderStage::Fragment, LoadShader(":/shaders/prebuild/skybox.frag.qsb") }
        });
        pipelineSky_->setVertexInputLayout(layout);

        std::vector<QRhiShaderResourceBinding> bindingsSky;
        bindingsSky.emplace_back(QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf_.get()));
        bindingsSky.emplace_back(QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage, envCubemap_.get(), sampler_.get()));
        srbSky_.reset(rhi->newShaderResourceBindings());
        srbSky_->setBindings(bindingsSky.begin(), bindingsSky.end());
        srbSky_->create();

        pipelineSky_->setShaderResourceBindings(srbSky_.get());
        pipelineSky_->setRenderPassDescriptor(rp);
        pipelineSky_->setDepthTest(true);
        pipelineSky_->setDepthWrite(false);

        pipelineSky_->setCullMode(QRhiGraphicsPipeline::Back);
        pipelineSky_->create();

        created_ = true;
    }

    // Uploads the equirectangular image and performs CPU-side conversion to fill
    // the cube faces. The uploads are recorded into the provided resource update batch.
    void updateResources(QRhiResourceUpdateBatch *rub, const QMatrix4x4 &view, const QMatrix4x4 &proj) {
        if (!created_ || !rub) return;

        if (!uploaded_) {
            // load equirectangular image
            QImage img;
            if (!img.load(eqPath_)) {
                qWarning() << "HdriSky: failed to load equirectangular image:" << eqPath_;
                return;
            }
            img = img.convertToFormat(QImage::Format_RGBA8888);
            rub->uploadTexture(equirectTex_.get(), img);

            // CPU-side convert to cubemap
            std::array<QImage,6> faces;
            for (int f=0; f<6; ++f) faces[f] = QImage(CUBEMAP_RESOLUTION, CUBEMAP_RESOLUTION, QImage::Format_RGBA8888);

            auto sampleEquirect = [&](const QVector3D &dir)->QColor{
                const float invAtanX = 0.1591f;
                const float invAtanY = 0.3183f;
                float u = atan2f(dir.z(), dir.x()) * invAtanX + 0.5f;
                float v = asinf(qBound(-1.0f, dir.y(), 1.0f)) * invAtanY + 0.5f;
                int sx = qBound(0, int(u * img.width()), img.width()-1);
                int sy = qBound(0, int(v * img.height()), img.height()-1);
                return img.pixelColor(sx, sy);
            };

            for (int face = 0; face < 6; ++face) {
                for (int y = 0; y < CUBEMAP_RESOLUTION; ++y) {
                    for (int x = 0; x < CUBEMAP_RESOLUTION; ++x) {
                        float nx = (2.0f * (x + 0.5f) / float(CUBEMAP_RESOLUTION)) - 1.0f;
                        float ny = (2.0f * (y + 0.5f) / float(CUBEMAP_RESOLUTION)) - 1.0f;
                        QVector3D dir;
                        switch(face) {
                        case 0: dir = QVector3D( 1.0f,    -ny,   -nx); break; // +X
                        case 1: dir = QVector3D(-1.0f,    -ny,    nx); break; // -X
                        case 2: dir = QVector3D( nx,      1.0f,   ny); break; // +Y
                        case 3: dir = QVector3D( nx,     -1.0f,  -ny); break; // -Y
                        case 4: dir = QVector3D( nx,     -ny,    1.0f); break; // +Z
                        default:dir = QVector3D(-nx,    -ny,   -1.0f); break; // -Z
                        }
                        dir.normalize();
                        faces[face].setPixelColor(x, y, sampleEquirect(dir));
                    }
                }
            }

            for (int face = 0; face < 6; ++face) {
                rub->uploadTexture(envCubemap_.get(), faces[face]);
            }

            uploaded_ = true;
        }

        // --- aktualizace uniform bufferu ---
        QMatrix4x4 viewNoTrans = view;
        viewNoTrans.setColumn(3, QVector4D(0, 0, 0, 1));
        QMatrix4x4 mvp = proj * viewNoTrans;

        SkyUbo gpuUbo{};
        memcpy(gpuUbo.projection,proj.constData(), 64);
        memcpy(gpuUbo.view, viewNoTrans.constData(),       64);
      \

        rub->updateDynamicBuffer(ubuf_.get(), 0, sizeof(SkyUbo), &gpuUbo);
       // rub->updateDynamicBuffer(ubuf_.get(), 0, 64, mvp.constData());
    }
    // Record draw commands for the skybox. `cb` must be in a render pass that has
    // a compatible render target (depth test enabled as needed). The view matrix
    // passed should be full camera view; the function will remove camera
    // translation so the skybox stays centered.
    void draw(QRhiCommandBuffer *cb) {
        if (!created_ || !uploaded_) return;

        cb->setGraphicsPipeline(pipelineSky_.get());
      //  cb->setViewport(viewport);
        cb->setShaderResources(srbSky_.get());
        const QRhiCommandBuffer::VertexInput in{vbuf_.get(), 0};
        cb->setVertexInput(0, 1, &in, nullptr, 0, QRhiCommandBuffer::IndexUInt32);
        cb->draw(36);
    }

    static inline QShader LoadShader(const QString &name) {
        QFile f(name);
        return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
    }

private:
    QString eqPath_;
    QRhi *rhi_{nullptr};
    bool created_{false};
    bool uploaded_{false};

    std::unique_ptr<QRhiGraphicsPipeline> pipelineE2C_;
    std::unique_ptr<QRhiGraphicsPipeline> pipelineSky_;
    std::unique_ptr<QRhiBuffer> vbuf_;
    std::unique_ptr<QRhiBuffer> ubuf_;
    std::unique_ptr<QRhiTexture> equirectTex_;
    std::unique_ptr<QRhiTexture> envCubemap_;
    std::unique_ptr<QRhiSampler> sampler_;
    std::unique_ptr<QRhiShaderResourceBindings> srbE2C_;
    std::unique_ptr<QRhiShaderResourceBindings> srbSky_;

    static constexpr int CUBEMAP_RESOLUTION = 512;

    static std::vector<HdriVertex> cubeVertices() {
        // 36 vertices (3 floats each) for a cube (same order as original)
        static const float verts[] = {
            // back
            -1,-1,-1,   1,-1,-1,   1, 1,-1,
             1, 1,-1,  -1, 1,-1,  -1,-1,-1,
            // front
            -1,-1, 1,   1,-1, 1,   1, 1, 1,
             1, 1, 1,  -1, 1, 1,  -1,-1, 1,
            // top
            -1, 1,-1,  -1, 1, 1,   1, 1, 1,
             1, 1, 1,   1, 1,-1,  -1, 1,-1,
            // bottom
            -1,-1,-1,  -1,-1, 1,   1,-1, 1,
             1,-1, 1,   1,-1,-1,  -1,-1,-1,
            // left
            -1, 1, 1,  -1, 1,-1,  -1,-1,-1,
            -1,-1,-1,  -1,-1, 1,  -1, 1, 1,
            // right
             1,-1,-1,   1,-1, 1,   1, 1, 1,
             1, 1, 1,   1, 1,-1,   1,-1,-1
        };
        std::vector<HdriVertex> out; out.reserve(36);
        for (int i=0;i<36;i++) out.push_back({ QVector3D(verts[i*3+0], verts[i*3+1], verts[i*3+2]) });
        return out;
    }
};

#endif // HDRI_SKY_QRHI_H

