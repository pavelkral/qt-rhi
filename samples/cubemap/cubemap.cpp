#include "../shared/examplefw.h"
#include "../shared/cube.h"
#include <QMatrix4x4>
#include <QFile>

struct {
    QList<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTexture *cubeTex = nullptr;
    QRhiTexture *equirectTex = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srbSkybox = nullptr;
    QRhiGraphicsPipeline *psSkybox = nullptr;
    QRhiShaderResourceBindings *srbConvert = nullptr;
    QRhiGraphicsPipeline *psConvert = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
} d;

// Načtení HDR equirectangular mapy
QByteArray loadHdr(const QString &fn, QSize *size)
{
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open %s", qPrintable(fn));
        return QByteArray();
    }

    char sig[256];
    f.read(sig, 11);
    if (strncmp(sig, "#?RADIANCE\n", 11))
        return QByteArray();

    QByteArray buf = f.readAll();
    const char *p = buf.constData();
    const char *pEnd = p + buf.size();

           // Process lines until the empty one.
    QByteArray line;
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n') {
            if (line.isEmpty())
                break;
            if (line.startsWith(QByteArrayLiteral("FORMAT="))) {
                const QByteArray format = line.mid(7).trimmed();
                if (format != QByteArrayLiteral("32-bit_rle_rgbe")) {
                    qWarning("HDR format '%s' is not supported", format.constData());
                    return QByteArray();
                }
            }
            line.clear();
        } else {
            line.append(c);
        }
    }
    if (p == pEnd) {
        qWarning("Malformed HDR image data at property strings");
        return QByteArray();
    }

           // Get the resolution string.
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n')
            break;
        line.append(c);
    }
    if (p == pEnd) {
        qWarning("Malformed HDR image data at resolution string");
        return QByteArray();
    }

    int w = 0, h = 0;
    // We only care about the standard orientation.
    if (!sscanf(line.constData(), "-Y %d +X %d", &h, &w)) {
        qWarning("Unsupported HDR resolution string '%s'", line.constData());
        return QByteArray();
    }
    if (w <= 0 || h <= 0) {
        qWarning("Invalid HDR resolution");
        return QByteArray();
    }

           // output is RGBA32F
    const int blockSize = 4 * sizeof(float);
    QByteArray data;
    data.resize(w * h * blockSize);

    typedef unsigned char RGBE[4];
    RGBE *scanline = new RGBE[w];

    for (int y = 0; y < h; ++y) {
        if (pEnd - p < 4) {
            qWarning("Unexpected end of HDR data");
            delete[] scanline;
            return QByteArray();
        }

        scanline[0][0] = *p++;
        scanline[0][1] = *p++;
        scanline[0][2] = *p++;
        scanline[0][3] = *p++;

        if (scanline[0][0] == 2 && scanline[0][1] == 2 && scanline[0][2] < 128) {
            // new rle, the first pixel was a dummy
            for (int channel = 0; channel < 4; ++channel) {
                for (int x = 0; x < w && p < pEnd; ) {
                    unsigned char c = *p++;
                    if (c > 128) { // run
                        if (p < pEnd) {
                            int repCount = c & 127;
                            c = *p++;
                            while (repCount--)
                                scanline[x++][channel] = c;
                        }
                    } else { // not a run
                        while (c-- && p < pEnd)
                            scanline[x++][channel] = *p++;
                    }
                }
            }
        } else {
            // old rle
            scanline[0][0] = 2;
            int bitshift = 0;
            int x = 1;
            while (x < w && pEnd - p >= 4) {
                scanline[x][0] = *p++;
                scanline[x][1] = *p++;
                scanline[x][2] = *p++;
                scanline[x][3] = *p++;

                if (scanline[x][0] == 1 && scanline[x][1] == 1 && scanline[x][2] == 1) { // run
                    int repCount = scanline[x][3] << bitshift;
                    while (repCount--) {
                        memcpy(scanline[x], scanline[x - 1], 4);
                        ++x;
                    }
                    bitshift += 8;
                } else { // not a run
                    ++x;
                    bitshift = 0;
                }
            }
        }

               // adjust for -Y orientation
        float *fp = reinterpret_cast<float *>(data.data() + (h - 1 - y) * blockSize * w);
        for (int x = 0; x < w; ++x) {
            float d = qPow(2.0f, float(scanline[x][3]) - 128.0f);
            // r, g, b, a
            *fp++ = scanline[x][0] / 256.0f * d;
            *fp++ = scanline[x][1] / 256.0f * d;
            *fp++ = scanline[x][2] / 256.0f * d;
            *fp++ = 1.0f;
        }
    }

    delete[] scanline;

    if (size)
        *size = QSize(w, h);

    return data;
}
void Window::customInit()
{
    // Vertex buffer pro cube
    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;

           // Uniform buffer
    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(QMatrix4x4));
    d.ubuf->create();
    d.releasePool << d.ubuf;

           // HDR equirect texture
    QSize hdrSize;
    QByteArray hdrData = loadHdr(QLatin1String(":/OpenfootageNET_fieldairport-512.hdr"), &hdrSize);
    Q_ASSERT(!hdrData.isEmpty());
    d.equirectTex = m_r->newTexture(QRhiTexture::RGBA32F, hdrSize);
    d.releasePool << d.equirectTex;
    d.equirectTex->create();

    QRhiTextureUploadDescription desc({0, 0, {hdrData.constData(), quint32(hdrData.size())}});
    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadTexture(d.equirectTex, desc);

           // Cubemap texture
    const QSize cubeMapSize(512, 512);
    d.cubeTex = m_r->newTexture(QRhiTexture::RGBA32F, cubeMapSize, 1,
                                QRhiTexture::CubeMap | QRhiTexture::UsedWithGenerateMips);
    d.releasePool << d.cubeTex;
    d.cubeTex->create();

           // Sampler
    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->create();

           // --- Convert equirect → cubemap pipeline ---
    d.srbConvert = m_r->newShaderResourceBindings();
    d.releasePool << d.srbConvert;
    d.srbConvert->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.equirectTex, d.sampler)
    });
    d.srbConvert->create();

    d.psConvert = m_r->newGraphicsPipeline();
    d.releasePool << d.psConvert;
    d.psConvert->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/cubemap.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/equirect_to_cubemap.frag.qsb")) }
    });
    d.psConvert->setCullMode(QRhiGraphicsPipeline::Back);
    d.psConvert->setDepthTest(false);    // HDR→cubemap nevyužívá depth
    d.psConvert->setDepthWrite(false);

    QRhiVertexInputLayout layout;
    layout.setBindings({ {3 * sizeof(float)} });
    layout.setAttributes({ {0, 0, QRhiVertexInputAttribute::Float3, 0} });
    d.psConvert->setVertexInputLayout(layout);
    d.psConvert->setShaderResourceBindings(d.srbConvert);
    // ⚠️ create() zatím nevoláme — až po získání render-passu níže

           // --- Skybox pipeline (zatím bez render passu) ---
    d.srbSkybox = m_r->newShaderResourceBindings();
    d.releasePool << d.srbSkybox;
    d.srbSkybox->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.cubeTex, d.sampler)
    });
    d.srbSkybox->create();

    d.psSkybox = m_r->newGraphicsPipeline();
    d.releasePool << d.psSkybox;
    d.psSkybox->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/cubemap.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/cubemap.frag.qsb")) }
    });
    d.psSkybox->setCullMode(QRhiGraphicsPipeline::Front);
    d.psSkybox->setDepthTest(true);
    d.psSkybox->setDepthWrite(false);
    d.psSkybox->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    d.psSkybox->setVertexInputLayout(layout);
    d.psSkybox->setShaderResourceBindings(d.srbSkybox);
    // ⚠️ create() až v customRender() — teď ještě nevíme render-pass

           // Upload cube vertex buffer
    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

           // --- Render HDR equirect to cubemap faces ---
    QMatrix4x4 captureViews[6];
    captureViews[0].lookAt(QVector3D(0,0,0), QVector3D(1,0,0), QVector3D(0,-1,0)); // +X
    captureViews[1].lookAt(QVector3D(0,0,0), QVector3D(-1,0,0), QVector3D(0,-1,0)); // -X
    captureViews[2].lookAt(QVector3D(0,0,0), QVector3D(0,1,0), QVector3D(0,0,1));   // +Y
    captureViews[3].lookAt(QVector3D(0,0,0), QVector3D(0,-1,0), QVector3D(0,0,-1)); // -Y
    captureViews[4].lookAt(QVector3D(0,0,0), QVector3D(0,0,1), QVector3D(0,-1,0));  // +Z
    captureViews[5].lookAt(QVector3D(0,0,0), QVector3D(0,0,-1), QVector3D(0,-1,0)); // -Z

    const float fov = 90.0f;
    const float aspect = 1.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 10.0f;
    QMatrix4x4 proj;
    proj.perspective(fov, aspect, nearPlane, farPlane);

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch(); // Tento 'u' se zdá být nepoužitý, ale nevadí

           // --- Jednorázově vytvoříme kompatibilní render pass descriptor pro pipeline ---
    QRhiTextureRenderTargetDescription tmpDesc;
    QRhiColorAttachment tmpAtt;
    tmpAtt.setTexture(d.cubeTex);
    tmpDesc.setColorAttachments({ tmpAtt });
    QRhiTextureRenderTarget *tmpRT = m_r->newTextureRenderTarget(tmpDesc);
    QRhiRenderPassDescriptor *compatibleRP = tmpRT->newCompatibleRenderPassDescriptor();
    tmpRT->create();

    d.psConvert->setRenderPassDescriptor(compatibleRP);
    d.psConvert->create();

    for (int face = 0; face < 6; ++face) {
        QMatrix4x4 mvp = proj * captureViews[face];
        QRhiResourceUpdateBatch *u1 = m_r->nextResourceUpdateBatch();
        u1->updateDynamicBuffer(d.ubuf, 0, sizeof(QMatrix4x4), mvp.constData());

        QRhiTextureRenderTargetDescription cubeRTDesc;
        QRhiColorAttachment colorAtt;
        colorAtt.setTexture(d.cubeTex);
        colorAtt.setLayer(face);
        cubeRTDesc.setColorAttachments({ colorAtt });
        QRhiTextureRenderTarget *cubeRT = m_r->newTextureRenderTarget(cubeRTDesc);
        cubeRT->setRenderPassDescriptor(compatibleRP);
        cubeRT->create();

        cb->beginPass(cubeRT, {0,0,0,1}, {1.0f,0}, u1);

        cb->setViewport(QRhiViewport(0, 0, cubeMapSize.width(), cubeMapSize.height())); // <-- OPRAVA: Chybějící viewport

        cb->setGraphicsPipeline(d.psConvert);
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(36);
        cb->endPass();

        delete cubeRT;
    }

    delete tmpRT;
    delete compatibleRP; // <-- OPRAVA: Uvolnění render pass descriptoru, aby neunikl

           // Generate mipmaps for cubemap
    d.initialUpdates->generateMips(d.cubeTex);
}

void Window::customRender()
{
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();

           // MVP pro skybox
    QMatrix4x4 mvp = m_r->clipSpaceCorrMatrix();
    mvp.perspective(90.0f, outputSizeInPixels.width()/(float)outputSizeInPixels.height(), 0.01f, 100.0f);
    mvp.scale(10);
    static float rx = 0;
    mvp.rotate(rx, 1,0,0);
    rx += 0.5f;
    u->updateDynamicBuffer(d.ubuf, 0, sizeof(QMatrix4x4), mvp.constData());

           // --- Lazy create skybox pipeline when first frame begins ---
    static bool skyboxPipelineCreated = false;
    if (!skyboxPipelineCreated) {
        QRhiRenderPassDescriptor *rp = m_sc->newCompatibleRenderPassDescriptor();
        d.psSkybox->setRenderPassDescriptor(rp);
        d.psSkybox->create();
        delete rp;
        skyboxPipelineCreated = true;
    }

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, {1.0f,0}, u);

    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height())); // <-- OPRAVA: Chybějící viewport

    cb->setGraphicsPipeline(d.psSkybox);
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(36);
    cb->endPass();
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}
