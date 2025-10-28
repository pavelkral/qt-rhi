#ifndef HDRI_SKY_QRHI_H
#define HDRI_SKY_QRHI_H

#include <QString>
#include <QImage>
#include <QDebug>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>
#include <memory>
#include <vector>
#include <array>
#include <QFile>

#include <rhi/qrhi.h>

#include "stb/stb_image.h"

struct HdriVertex { QVector3D pos; };
struct alignas(16) SkyUbo { float projection[16]; float view[16]; };

class HdriSky {
public:
    explicit HdriSky(const QString &equirectPath = {}) : eqPath_(equirectPath) {}

    void setEquirectangular(const QString &path) { eqPath_ = path; uploaded_ = false; }

    void create(QRhi *rhi, QRhiRenderPassDescriptor *rp, QRhiResourceUpdateBatch *rub) {
        if (!rhi || !rub) return;
        if (rhi_ != rhi) {
            pipelineSky_.reset();
            vbuf_.reset();
            ubuf_.reset();
            srbSky_.reset();
            sampler_.reset();
            envCubemap_.reset();
            equirectTex_.reset();
            rhi_ = rhi;
            created_ = false;
            uploaded_ = false;
        }
        if (created_) return;

        // vertex data (unit cube)
        std::vector<HdriVertex> vertices = cubeVertices();
        quint32 vbufSize = static_cast<quint32>(vertices.size() * sizeof(HdriVertex));
        vbuf_.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbufSize));
        vbuf_->create();
        // --- Nahrání dat do vertex bufferu (dříve chybělo) ---
        rub->uploadStaticBuffer(vbuf_.get(), 0, vbufSize, vertices.data());

        // uniform buffer
        ubuf_.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(SkyUbo)));
        ubuf_->create();

        sampler_.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                       QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        sampler_->create();

        // empty cubemap texture (RGBA8 used for simplicity; change to RGBA16F if you want FP)
        envCubemap_.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(CUBEMAP_RESOLUTION, CUBEMAP_RESOLUTION),
                                          1, QRhiTexture::CubeMap | QRhiTexture::RenderTarget));
        envCubemap_->create();

        // placeholder equirect texture (we upload actual pixels later)
        equirectTex_.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(1,1), 1, QRhiTexture::Flag()));
        equirectTex_->create();

        // pipeline (skybox). Requires prebuilt QSB shaders in resources or adapt to create from source.
        pipelineSky_.reset(rhi->newGraphicsPipeline());
        pipelineSky_->setTopology(QRhiGraphicsPipeline::Triangles);
        pipelineSky_->setShaderStages({
            { QRhiShaderStage::Vertex,   LoadShader(":/shaders/prebuild/skybox.vert.qsb") },
            { QRhiShaderStage::Fragment, LoadShader(":/shaders/prebuild/skybox.frag.qsb") }
        });

        QRhiVertexInputLayout layout{};
        layout.setBindings({ sizeof(HdriVertex) });
        layout.setAttributes({ {0, 0, QRhiVertexInputAttribute::Float3, 0} });
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
        pipelineSky_->setCullMode(QRhiGraphicsPipeline::None);
        pipelineSky_->create();

        created_ = true;
    }

    // initCubemap: jednorázové načtení HDR (stb), CPU-konverze -> 6 QImage + upload do cubemap pomocí rub.
    void initCubemap(QRhiResourceUpdateBatch *rub) {
        if (!rhi_ || !rub || !created_) return;
        if (uploaded_) return;
        if (eqPath_.isEmpty()) { qWarning() << "HdriSky: no eq path"; return; }

        int width=0, height=0, comps=0;
        stbi_set_flip_vertically_on_load(true);
        float *data = stbi_loadf(eqPath_.toUtf8().constData(), &width, &height, &comps, 0);
        if (!data) { qWarning() << "HdriSky: failed to load HDR:" << eqPath_; return; }
        if (comps < 3) { stbi_image_free(data); qWarning() << "HdriSky: unexpected components"; return; }

        std::array<QImage,6> faces;
        for (int f=0; f<6; ++f){
            faces[f] = QImage(CUBEMAP_RESOLUTION, CUBEMAP_RESOLUTION, QImage::Format_ARGB32);
            //faces[f].fill(Qt::transparent );
        }
        auto sample = [&](const QVector3D &dir)->QVector3D {
            const float invAtanX = 0.1591f, invAtanY = 0.3183f;
            float u = atan2f(dir.z(), dir.x()) * invAtanX + 0.5f;
            float v = asinf(qBound(-1.0f, dir.y(), 1.0f)) * invAtanY + 0.5f;
            int sx = qBound(0, int(u * width), width-1);
            int sy = qBound(0, int(v * height), height-1);
            int idx = (sy * width + sx) * comps;
            return QVector3D(data[idx+0], data[idx+1], data[idx+2]);
        };

        for (int face=0; face<6; ++face) {
            for (int y=0; y<CUBEMAP_RESOLUTION; ++y) {
                QRgb *scanline = reinterpret_cast<QRgb*>(faces[face].scanLine(y));
                for (int x=0; x<CUBEMAP_RESOLUTION; ++x) {
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
                    QVector3D col = sample(dir);
                    // tonemap + gamma to fit into RGBA8 (simple solution)
                    QVector3D mapped = col / (col + QVector3D(1.0f,1.0f,1.0f));
                    mapped.setX(powf(mapped.x(), 1.0f/2.2f));
                    mapped.setY(powf(mapped.y(), 1.0f/2.2f));
                    mapped.setZ(powf(mapped.z(), 1.0f/2.2f));
                    int ir = qBound(0, int(mapped.x()*255.0f), 255);
                    int ig = qBound(0, int(mapped.y()*255.0f), 255);
                    int ib = qBound(0, int(mapped.z()*255.0f), 255);
                   // scanline[x] = qRgba(ir, ig, ib, 255);
                    scanline[x] = qRgba(ib, ig, ir, 255);
                }
            }
        }

        stbi_image_free(data);

        // --- SPRÁVNÉ VOLÁNÍ: uploadTexture(texture, image, arrayLayer, level) ---
        for (int face = 0; face < 6; ++face) {
            // Připravíme popis subresource uploadu
            QRhiTextureSubresourceUploadDescription subresDesc(faces[face]);
            // Face index se určuje přes sliceDesc (arrayLayer = face)
            QRhiTextureUploadDescription uploadDesc({ { face, 0, subresDesc } });
            rub->uploadTexture(envCubemap_.get(), uploadDesc);
        }
        uploaded_ = true;
    }
    // initCubemap: jednorázové načtení HDR (stb), CPU-konverze -> 6 QImage + upload do cubemap pomocí rub.
    void initCubemapOnGPU(QRhiResourceUpdateBatch *rub,QRhiCommandBuffer *cb) {
        if (!rhi_ || !created_ || uploaded_) return;
        if (eqPath_.isEmpty()) { qWarning() << "HdriSky: no eq path"; return; }

        // --- 1. Load HDR texture ---
        int width=0, height=0, comps=0;
        stbi_set_flip_vertically_on_load(true);
        float *data = stbi_loadf(eqPath_.toUtf8().constData(), &width, &height, &comps, 0);
        if (!data) { qWarning() << "HdriSky: failed to load HDR:" << eqPath_; return; }

        std::unique_ptr<QRhiTexture> hdrTex(rhi_->newTexture(QRhiTexture::RGBA32F, QSize(width,height), 1));
        hdrTex->create();
        QByteArray byteData(reinterpret_cast<const char*>(data), width * height * comps * sizeof(float));
        QRhiTextureSubresourceUploadDescription subresDesc;
        subresDesc.setData(byteData);

        subresDesc.setSourceSize(QSize(width, height));

        // Zápis obdobný cubemap face uploadu
        QRhiTextureUploadDescription uploadDesc({ { 0, 0, subresDesc } }); // arrayLayer = 0, mipLevel = 0

        rub->uploadTexture(hdrTex.get(), uploadDesc);
        stbi_image_free(data);

        // --- 2. Cubemap render target ---
        envCubemap_.reset(rhi_->newTexture(QRhiTexture::RGBA16F, QSize(CUBEMAP_RESOLUTION,CUBEMAP_RESOLUTION), 1,
                                           QRhiTexture::CubeMap | QRhiTexture::RenderTarget));
        envCubemap_->create();

        QRhiTextureRenderTargetDescription rtDesc;
        QList<QRhiColorAttachment> colorAttachments;
        for (int face = 0; face < 6; ++face) {
            QRhiColorAttachment ca(envCubemap_.get());
            ca.setLayer(face);
            ca.setLevel(0);
           // ca.setLoadOp(QRhiRenderPassDescriptor::Clear);
           // ca.setStoreOp(QRhiRenderPassDescriptor::Store);
           // ca.setClearColor(QColor(0, 0, 0, 255));
            colorAttachments.append(ca);
        }
       // rtDesc.setColorAttachments(colorAttachments);

        auto initRt = std::unique_ptr<QRhiTextureRenderTarget>(rhi_->newTextureRenderTarget(rtDesc));
        initRt->create();
        auto initRp = std::unique_ptr<QRhiRenderPassDescriptor>(initRt->newCompatibleRenderPassDescriptor());
        initRt->setRenderPassDescriptor(initRp.get());

        // --- 3. Sampler ---
        auto initSampler = std::unique_ptr<QRhiSampler>(rhi_->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        initSampler->create();

        // --- 4. SRB ---
        std::vector<QRhiShaderResourceBinding> bindings;
        bindings.emplace_back(QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, ubuf_.get()));
        bindings.emplace_back(QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, hdrTex.get(), initSampler.get()));
        auto initSrb = std::unique_ptr<QRhiShaderResourceBindings>(rhi_->newShaderResourceBindings());
        initSrb->setBindings(bindings.begin(), bindings.end());
        initSrb->create();

        // --- 5. Pipeline (init-only) ---
        auto initPipeline = std::unique_ptr<QRhiGraphicsPipeline>(rhi_->newGraphicsPipeline());
        QRhiVertexInputLayout layout{};
        layout.setBindings({ sizeof(HdriVertex) });
        layout.setAttributes({ {0, 0, QRhiVertexInputAttribute::Float3, 0} });
        initPipeline->setVertexInputLayout(layout);
        initPipeline->setShaderStages({
            {QRhiShaderStage::Vertex, LoadShader(":/shaders/prebuild/cubemap.vert.qsb")},
            {QRhiShaderStage::Fragment, LoadShader(":/shaders/prebuild/equirect2cube.frag.qsb")}
        });
        initPipeline->setShaderResourceBindings(initSrb.get());
        initPipeline->setRenderPassDescriptor(initRp.get());
        initPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
        initPipeline->setCullMode(QRhiGraphicsPipeline::None);
        initPipeline->setDepthTest(false);
        initPipeline->setDepthWrite(false);
        initPipeline->create();

        // --- 6. Vertex buffer cube ---
        std::vector<HdriVertex> verts = cubeVertices();
        auto initVbuf = std::unique_ptr<QRhiBuffer>(rhi_->newBuffer(QRhiBuffer::Immutable,QRhiBuffer::VertexBuffer,sizeof(HdriVertex)*verts.size()));
        initVbuf->create();
        rub->uploadStaticBuffer(initVbuf.get(), 0, sizeof(HdriVertex)*verts.size(), verts.data());

        // --- 7. Draw cubemap faces ---
        //  QRhiCommandBuffer *cb = rhi_->nextFrameCommandBuffer();
        for (int face=0; face<6; face++) {
            cb->beginPass(initRt.get(), QColor(0,0,0,255), {1.0f,0});
            cb->setGraphicsPipeline(initPipeline.get());
            cb->setShaderResources(initSrb.get());
            const QRhiCommandBuffer::VertexInput in{initVbuf.get(), 0};
            cb->setVertexInput(0, 1, &in, nullptr, 0, QRhiCommandBuffer::IndexUInt32);
            cb->draw(36);
            cb->endPass();
        }

        uploaded_ = true;
    }

    void updateResources(QRhiResourceUpdateBatch *rub, const QMatrix4x4 &view, const QMatrix4x4 &proj) {
        if (!created_ || !rub) return;
        QMatrix4x4 viewNoTrans = view;
        viewNoTrans(0,3)=viewNoTrans(1,3)=viewNoTrans(2,3)=0.0f; viewNoTrans(3,3)=1.0f;
        SkyUbo gpuUbo{};
        memcpy(gpuUbo.projection, proj.constData(), 16*sizeof(float));
        memcpy(gpuUbo.view, viewNoTrans.constData(), 16*sizeof(float));
        rub->updateDynamicBuffer(ubuf_.get(), 0, sizeof(SkyUbo), &gpuUbo);
    }

    void draw(QRhiCommandBuffer *cb, QRhiViewport viewport = QRhiViewport()) {
        if (!created_ || !uploaded_) return;
        cb->setGraphicsPipeline(pipelineSky_.get());
       // cb->setViewport(viewport);
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

    std::unique_ptr<QRhiGraphicsPipeline> pipelineSky_;
    std::unique_ptr<QRhiBuffer> vbuf_;
    std::unique_ptr<QRhiBuffer> ubuf_;
    std::unique_ptr<QRhiTexture> equirectTex_;
    std::unique_ptr<QRhiTexture> envCubemap_;
    std::unique_ptr<QRhiSampler> sampler_;
    std::unique_ptr<QRhiShaderResourceBindings> srbSky_;

    static constexpr int CUBEMAP_RESOLUTION = 512;

    static std::vector<HdriVertex> cubeVertices() {
        static const float verts[] = {
            -1,-1,-1,  1,-1,-1,  1, 1,-1,  1, 1,-1, -1, 1,-1, -1,-1,-1,
            -1,-1, 1,  1,-1, 1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1,-1, 1,
            -1, 1,-1, -1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1, -1, 1,-1,
            -1,-1,-1, -1,-1, 1,  1,-1, 1,  1,-1, 1,  1,-1,-1, -1,-1,-1,
            -1, 1, 1, -1, 1,-1, -1,-1,-1, -1,-1,-1, -1,-1, 1, -1, 1, 1,
            1,-1,-1,  1,-1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1,  1,-1,-1
        };
        std::vector<HdriVertex> out; out.reserve(36);
        for (int i=0;i<36;i++) out.push_back({ QVector3D(verts[i*3+0], verts[i*3+1], verts[i*3+2]) });
        return out;
    }
};

#endif // HDRI_SKY_QRHI_H



