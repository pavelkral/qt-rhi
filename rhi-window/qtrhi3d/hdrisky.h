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

    void initCubemapOnGPU(QRhiResourceUpdateBatch *rub, QRhiCommandBuffer *cb) {
        if (!rhi_ || !created_ || uploaded_) return;
        if (eqPath_.isEmpty()) { qWarning() << "HdriSky: no eq path"; return; }

        int width = 0, height = 0, comps = 0;
        stbi_set_flip_vertically_on_load(true);
        float *data = stbi_loadf(eqPath_.toUtf8().constData(), &width, &height, &comps, 0);
        if (!data) { qWarning() << "HdriSky: failed to load HDR:" << eqPath_; return; }

        hdrTex_ = std::unique_ptr<QRhiTexture>(rhi_->newTexture(QRhiTexture::RGBA32F, QSize(width,height), 1));
        hdrTex_->create();

        QByteArray byteData(reinterpret_cast<const char*>(data), width * height * comps * sizeof(float));
        stbi_image_free(data);

        QRhiTextureSubresourceUploadDescription subresDesc(byteData);
        subresDesc.setSourceSize(QSize(width, height));
        QRhiTextureUploadDescription uploadDesc({ { 0, 0, subresDesc } });
        rub->uploadTexture(hdrTex_.get(), uploadDesc);

        envCubemap_ = std::unique_ptr<QRhiTexture>(
            rhi_->newTexture(QRhiTexture::RGBA16F, QSize(CUBEMAP_RESOLUTION, CUBEMAP_RESOLUTION), 1,
                             QRhiTexture::CubeMap | QRhiTexture::RenderTarget)
            );
        envCubemap_->create();

        initSampler_ = std::unique_ptr<QRhiSampler>(
            rhi_->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge)
            );
        initSampler_->create();

        // shader bindings
        QRhiTextureRenderTargetDescription rtDesc;
        QList<QRhiColorAttachment> colorAttachments;
        for (int face = 0; face < 6; ++face) {
            QRhiColorAttachment ca(envCubemap_.get());
            ca.setLayer(face);
            ca.setLevel(0);
            colorAttachments.append(ca);
        }
        // správně: nastavení přes metodu
        rtDesc.setColorAttachments(colorAttachments.begin(), colorAttachments.end());

        initRt_.reset(rhi_->newTextureRenderTarget(rtDesc));
        initSrb_->create();

        // pipeline
        initPipeline_ = std::unique_ptr<QRhiGraphicsPipeline>(rhi_->newGraphicsPipeline());
        QRhiVertexInputLayout layout;
        layout.setBindings({ sizeof(HdriVertex) });
        layout.setAttributes({ {0, 0, QRhiVertexInputAttribute::Float3, 0} });
        initPipeline_->setVertexInputLayout(layout);
        initPipeline_->setShaderStages({
            {QRhiShaderStage::Vertex, LoadShader(":/shaders/prebuild/equirect2cube.vert.qsb")},
            {QRhiShaderStage::Fragment, LoadShader(":/shaders/prebuild/equirect2cube.frag.qsb")}
        });
        initPipeline_->setShaderResourceBindings(initSrb_.get());
        initPipeline_->setTopology(QRhiGraphicsPipeline::Triangles);
        initPipeline_->setCullMode(QRhiGraphicsPipeline::None);
        initPipeline_->setDepthTest(false);
        initPipeline_->setDepthWrite(false);
        initPipeline_->create();

        // draw faces
        for (int face = 0; face < 6; ++face) {
            QRhiColorAttachment ca(envCubemap_.get());
            ca.setLayer(face);
            ca.setLevel(0);
            QRhiTextureRenderTargetDescription rtDesc(ca);
            auto rt = std::unique_ptr<QRhiTextureRenderTarget>(rhi_->newTextureRenderTarget(rtDesc));
            auto rp = std::unique_ptr<QRhiRenderPassDescriptor>(rt->newCompatibleRenderPassDescriptor());
            rt->setRenderPassDescriptor(rp.get());
            rt->create();

            cb->beginPass(rt.get(), QColor(0,0,0,255), {1.0f, 0});
            cb->setGraphicsPipeline(initPipeline_.get());
            cb->setShaderResources(initSrb_.get());
            const QRhiCommandBuffer::VertexInput in{vbuf_.get(), 0};
            cb->setVertexInput(0, 1, &in);
            cb->draw(36);
            cb->endPass();
        }

        uploaded_ = true;
    }

    void initCubemapOnGPU2(QRhiResourceUpdateBatch *rub, QRhiCommandBuffer *cb) {
        if (!rhi_ || !created_ || uploaded_ || !rub || !cb) return; // Kontrola cb
        if (eqPath_.isEmpty()) { qWarning() << "HdriSky: no eq path"; return; }

        // --- 1. Načtení HDR textury ---
        int width=0, height=0, comps=0;
        stbi_set_flip_vertically_on_load(true);
        float *data = stbi_loadf(eqPath_.toUtf8().constData(), &width, &height, &comps, 0);
        if (!data) { qWarning() << "HdriSky: failed to load HDR:" << eqPath_; return; }
        if (comps < 3) { stbi_image_free(data); qWarning() << "HdriSky: HDR needs 3+ components"; return; }

        // Zvolíme správný formát podle počtu komponent
        QRhiTexture::Format hdrFormat = QRhiTexture::RGBA32F;
        int bytesPerPixel = 4 * sizeof(float);
        if (comps == 3) {
            hdrFormat = QRhiTexture::RGBA32F;
            bytesPerPixel = 3 * sizeof(float);
        }

        hdrTex_ = std::unique_ptr<QRhiTexture>(rhi_->newTexture(hdrFormat, QSize(width, height), 1));
        hdrTex_->create();

        QByteArray byteData(reinterpret_cast<const char*>(data), width * height * bytesPerPixel);
        QRhiTextureSubresourceUploadDescription subresDesc;
        subresDesc.setData(byteData);
        subresDesc.setSourceSize(QSize(width, height));
        QRhiTextureUploadDescription uploadDesc({ { 0, 0, subresDesc } });
        rub->uploadTexture(hdrTex_.get(), uploadDesc);
        stbi_image_free(data);

        // --- 2. Cílová Cubemap textura ---
        // Vytvoříme novou cubemapu s vyšší přesností (RGBA16F) a jako RenderTarget
        envCubemap_.reset(rhi_->newTexture(QRhiTexture::RGBA16F, QSize(CUBEMAP_RESOLUTION, CUBEMAP_RESOLUTION), 1,
                                           QRhiTexture::CubeMap | QRhiTexture::RenderTarget));
        envCubemap_->create();

        // --- 3. Sampler pro HDR texturu ---
        initSampler_ = std::unique_ptr<QRhiSampler>(rhi_->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        initSampler_->create();

        // --- 4. Lokální UBO pro matice ---
        // Vytvoříme UBO pro 6 sad matic (jedna pro každou stranu)
        // Musíme respektovat zarovnání pro dynamické offsety
         qint32 uboStride = sizeof(SkyUbo);
        // const qint32 minAlign = rhi_->ubo;
        // if (uboStride % minAlign != 0) {
        //     uboStride = (uboStride + minAlign - 1) / minAlign * minAlign;
        // }
        int blockSize = sizeof(SkyUbo);
        // zarovnaná velikost bloku
        int alignedBlockSize = rhi_->ubufAligned(blockSize);

        // když vypočítáš offset pro instanci i:
       // int offset = i * alignedBlockSize;
        auto initUbuf = std::unique_ptr<QRhiBuffer>(rhi_->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, uboStride * 6));
        initUbuf->create();

        // Připravíme data matic
        QMatrix4x4 captureProjection;
        captureProjection.perspective(90.0f, 1.0f, 0.1f, 10.0f); // 90° FOV, poměr stran 1:1
        std::array<QMatrix4x4, 6> captureViews;
        captureViews[0].lookAt(QVector3D(0,0,0), QVector3D( 1, 0, 0), QVector3D(0,-1, 0)); // +X
        captureViews[1].lookAt(QVector3D(0,0,0), QVector3D(-1, 0, 0), QVector3D(0,-1, 0)); // -X
        captureViews[2].lookAt(QVector3D(0,0,0), QVector3D( 0, 1, 0), QVector3D(0, 0, 1)); // +Y
        captureViews[3].lookAt(QVector3D(0,0,0), QVector3D( 0,-1, 0), QVector3D(0, 0,-1)); // -Y
        captureViews[4].lookAt(QVector3D(0,0,0), QVector3D( 0, 0, 1), QVector3D(0,-1, 0)); // +Z
        captureViews[5].lookAt(QVector3D(0,0,0), QVector3D( 0, 0,-1), QVector3D(0,-1, 0)); // -Z

        // Nahrajeme všech 6 matic do bufferu najednou
        QByteArray uboData(uboStride * 6, 0); // Vynulujeme paměť
        for (int face = 0; face < 6; ++face) {
            SkyUbo gpuUbo{};
            memcpy(gpuUbo.projection, captureProjection.constData(), 16*sizeof(float));
            memcpy(gpuUbo.view, captureViews[face].constData(), 16*sizeof(float));
            // Zkopírujeme data na správný offset
            memcpy(uboData.data() + face * uboStride, &gpuUbo, sizeof(SkyUbo));
        }
        rub->updateDynamicBuffer(initUbuf.get(), 0, uboData.size(), uboData.constData());


        // --- 5. Lokální SRB pro inicializaci ---
        std::vector<QRhiShaderResourceBinding> bindings;
        // Binding 0: Náš nový UBO s dynamickým offsetem
        bindings.emplace_back(QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage,
            initUbuf.get(), 0, uboStride)); // uboStride udává velikost jedné sekce
        // Binding 1: Vstupní HDR textura
        bindings.emplace_back(QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage, hdrTex_.get(), initSampler_.get()));

        auto initSrb = std::unique_ptr<QRhiShaderResourceBindings>(rhi_->newShaderResourceBindings());
        initSrb->setBindings(bindings.begin(), bindings.end());
        initSrb->create();

        // --- 6. Pipeline a Render Targety (6x) ---
        // Potřebujeme 6 render targetů a 6 render passů, jeden pro každou stranu
        std::array<std::unique_ptr<QRhiTextureRenderTarget>, 6> faceRenderTargets;
        std::array<std::unique_ptr<QRhiRenderPassDescriptor>, 6> faceRenderPasses;
        std::unique_ptr<QRhiGraphicsPipeline> initPipeline;

        QRhiViewport viewport(0, 0, CUBEMAP_RESOLUTION, CUBEMAP_RESOLUTION);

        for (int face = 0; face < 6; ++face) {
            // Nastavíme render target pro danou stranu (layer)
            QRhiColorAttachment ca;
            ca.setLayer(face);
            ca.setTexture(envCubemap_.get());
           // ca.setLoadOp(QRhiRenderPassDescriptor::DontCare); // Nepotřebujeme čistit, přepisujeme vše
           // ca.setStoreOp(QRhiRenderPassDescriptor::Store);  // Chceme uložit výsledek

            QRhiTextureRenderTargetDescription rtDesc({ca});
            faceRenderTargets[face].reset(rhi_->newTextureRenderTarget(rtDesc));
            faceRenderPasses[face].reset(faceRenderTargets[face]->newCompatibleRenderPassDescriptor());
            faceRenderTargets[face]->setRenderPassDescriptor(faceRenderPasses[face].get());
            faceRenderTargets[face]->create();

            // Pipeline vytvoříme jen jednou (všechny passy jsou kompatibilní)
            if (face == 0) {
                initPipeline.reset(rhi_->newGraphicsPipeline());
                QRhiVertexInputLayout layout{};
                layout.setBindings({ sizeof(HdriVertex) });
                layout.setAttributes({ {0, 0, QRhiVertexInputAttribute::Float3, 0} });
                initPipeline->setVertexInputLayout(layout);
                initPipeline->setShaderStages({
                    {QRhiShaderStage::Vertex, LoadShader(":/shaders/prebuild/equirect2cube.vert.qsb")},
                    {QRhiShaderStage::Fragment, LoadShader(":/shaders/prebuild/equirect2cube.frag.qsb")}
                });
                initPipeline->setShaderResourceBindings(initSrb.get());
                initPipeline->setRenderPassDescriptor(faceRenderPasses[0].get()); // Váže se na první RP
                initPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
                initPipeline->setCullMode(QRhiGraphicsPipeline::None);
                initPipeline->setDepthTest(false);

                initPipeline->setDepthWrite(false);
                initPipeline->create();
            }
        }

        // --- 7. Vykreslení 6 stran ---
        // Nyní máme 6 platných render targetů a pipeline
        for (int face = 0; face < 6; face++) {
            // Začneme pass pro konkrétní stranu
            cb->beginPass(faceRenderTargets[face].get(), QColor(), {1.0f, 0});
            cb->setGraphicsPipeline(initPipeline.get());
            cb->setViewport(viewport); // Nastavíme viewport
            quint32 dynamicOffset = face * alignedBlockSize;
            // Nastavíme správný offset do UBO pro tuto stranu
            QRhiCommandBuffer::DynamicOffset dyn;
            dyn.first = face;
            dyn.second = alignedBlockSize;

            cb->setShaderResources(initSrb.get(), 1, &dyn); // 1 = počet offsetů

            const QRhiCommandBuffer::VertexInput in{vbuf_.get(), 0};
            cb->setVertexInput(0, 1, &in, nullptr, 0, QRhiCommandBuffer::IndexUInt32);
            cb->draw(36);
            cb->endPass();
        }

        // --- 8. Oprava hlavního SRB (ŘEŠÍ PÁD PŘI KRESLENÍ) ---
        // Protože jsme resetovali envCubemap_, musíme aktualizovat srbSky_,
        // aby ukazoval na novou texturu.
        std::vector<QRhiShaderResourceBinding> bindingsSky;
        bindingsSky.emplace_back(QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf_.get()));
        bindingsSky.emplace_back(QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage, envCubemap_.get(), sampler_.get())); // Zde použijeme novou mapu

        srbSky_.reset(rhi_->newShaderResourceBindings());
        srbSky_->setBindings(bindingsSky.begin(), bindingsSky.end());
        srbSky_->create();

        // Všechny lokální unique_ptr (initUbuf, initSrb, hdrTex, atd.) se zde automaticky uvolní
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


    std::unique_ptr<QRhiTextureRenderTarget> initRt_;
    std::unique_ptr<QRhiRenderPassDescriptor> initRp_;
    std::unique_ptr<QRhiGraphicsPipeline> initPipeline_;
    std::unique_ptr<QRhiShaderResourceBindings> initSrb_;
    std::unique_ptr<QRhiSampler> initSampler_;
    std::unique_ptr<QRhiTexture> hdrTex_;

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



