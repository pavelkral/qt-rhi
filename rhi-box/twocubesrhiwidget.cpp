#include "TwoCubesRhiWidget.h"

#include <QFile>
#include "TwoCubesRhiWidget.h"

#include <QFile>
#include <array>
#include <rhi/qshader.h>

using namespace std;

static QShader loadQSB(const char *resourcePath)
{
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qFatal("Nelze otevřít shader: %s", resourcePath);
    }
    const QByteArray data = f.readAll();
    QShader s = QShader::fromSerialized(data);
    if (!s.isValid()) {
        qFatal("Neplatný QShader: %s", resourcePath);
    }

    qDebug() << "Loaded shader" << resourcePath << "valid?" << s.isValid();
    return s;
}

TwoCubesRhiWidget::TwoCubesRhiWidget(QWidget *parent)
    : QRhiWidget(parent)
{
    // Plynulá animace
    connect(&m_anim, &QTimer::timeout, this, [this]{
        m_angle += 0.5f;
        if (m_angle > 360.f) m_angle -= 360.f;
        update();
    });
    m_anim.start(16); // ~60 FPS
}

TwoCubesRhiWidget::~TwoCubesRhiWidget()
{
}

void TwoCubesRhiWidget::initialize(QRhiCommandBuffer * /*cb*/)
{
    m_rhi = this->rhi();

    // Render pass descriptor přímo z render targetu
    m_rpDesc = renderTarget()->renderPassDescriptor();
    renderTarget()->setRenderPassDescriptor(m_rpDesc);

    // --- GEOMETRIE ---
    createGeometry();   // vytvoření m_vbuf, m_ibuf + upload dat
    createTexture();    // vytvoření m_tex/m_sampler + upload

    // --- UBO ---
    m_uboCubeA.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UBOData)));
    m_uboCubeA->create();
    m_uboCubeB.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UBOData)));
    m_uboCubeB->create();

    // --- SHADER RESOURCE BINDINGS ---
    m_srbLambert.reset(m_rhi->newShaderResourceBindings());
    m_srbLambert->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                 m_uboCubeA.get()), // placeholder, render loop přepne
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_tex.get(), m_sampler.get())
    });
    m_srbLambert->create();

    m_srbToon.reset(m_rhi->newShaderResourceBindings());
    m_srbToon->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                 m_uboCubeB.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_tex.get(), m_sampler.get())
    });
    m_srbToon->create();

    // --- PIPELINES ---
    const QShader vsh = loadQSB(":/shaders/prebuild/cube.vert.qsb");
    const QShader fshLambert = loadQSB(":/shaders/prebuild/lambert.frag.qsb");
    const QShader fshToon = loadQSB(":/shaders/prebuild/toon.frag.qsb");

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ QRhiVertexInputBinding(8 * sizeof(float)) });
    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float)),
        QRhiVertexInputAttribute(0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float))
    });

    auto createPipeline = [&](const QShader &frag, std::unique_ptr<QRhiGraphicsPipeline> &pipe) {
        pipe.reset(m_rhi->newGraphicsPipeline());
        pipe->setShaderStages({
            { QRhiShaderStage::Vertex, vsh },
            { QRhiShaderStage::Fragment, frag }
        });
        pipe->setCullMode(QRhiGraphicsPipeline::Back);
        pipe->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
        pipe->setDepthTest(true);
        pipe->setDepthWrite(true);
        pipe->setSampleCount(renderTarget()->sampleCount());
        pipe->setRenderPassDescriptor(m_rpDesc);
        pipe->setVertexInputLayout(inputLayout);
        pipe->create();
    };

    createPipeline(fshLambert, m_pipeLambert);
    createPipeline(fshToon, m_pipeToon);

    m_uploaded = true; // vše statické vytvořeno a upload hotový
}


void TwoCubesRhiWidget::releaseResources()
{
    m_pipeLambert.reset();
    m_pipeToon.reset();
    m_srbLambert.reset();
    m_srbToon.reset();
    m_uboCubeA.reset();
    m_uboCubeB.reset();
    m_sampler.reset();
    m_tex.reset();
    m_vbuf.reset();
    m_ibuf.reset();
    m_rpDesc = nullptr; // jen vynulovat, NE mazat
}

void TwoCubesRhiWidget::createGeometry()
{
    // Jednoduchá krychle (pozice, normála, uv)
    struct V { float px,py,pz; float nx,ny,nz; float u,v; };

    const array<V, 24> verts = { // 6 stran * 4 vrcholy
                                // +X
                                V{+1, -1, -1,  +1, 0, 0,  0, 1},
                                V{+1, +1, -1,  +1, 0, 0,  0, 0},
                                V{+1, +1, +1,  +1, 0, 0,  1, 0},
                                V{+1, -1, +1,  +1, 0, 0,  1, 1},
                                // -X
                                V{-1, -1, +1,  -1, 0, 0,  0, 1},
                                V{-1, +1, +1,  -1, 0, 0,  0, 0},
                                V{-1, +1, -1,  -1, 0, 0,  1, 0},
                                V{-1, -1, -1,  -1, 0, 0,  1, 1},
                                // +Y
                                V{-1, +1, -1,   0, +1, 0, 0, 1},
                                V{-1, +1, +1,   0, +1, 0, 0, 0},
                                V{+1, +1, +1,   0, +1, 0, 1, 0},
                                V{+1, +1, -1,   0, +1, 0, 1, 1},
                                // -Y
                                V{-1, -1, +1,   0, -1, 0, 0, 1},
                                V{-1, -1, -1,   0, -1, 0, 0, 0},
                                V{+1, -1, -1,   0, -1, 0, 1, 0},
                                V{+1, -1, +1,   0, -1, 0, 1, 1},
                                // +Z
                                V{+1, -1, +1,   0, 0, +1, 0, 1},
                                V{+1, +1, +1,   0, 0, +1, 0, 0},
                                V{-1, +1, +1,   0, 0, +1, 1, 0},
                                V{-1, -1, +1,   0, 0, +1, 1, 1},
                                // -Z
                                V{-1, -1, -1,   0, 0, -1, 0, 1},
                                V{-1, +1, -1,   0, 0, -1, 0, 0},
                                V{+1, +1, -1,   0, 0, -1, 1, 0},
                                V{+1, -1, -1,   0, 0, -1, 1, 1},
                                };

    const array<quint16, 36> idx = {
        0,1,2, 0,2,3,
        4,5,6, 4,6,7,
        8,9,10, 8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23
    };

    m_indexCount = (int)idx.size();

    // Vytvoříme GPU buffery (bez uploadu) — upload uděláme při prvním renderu
    m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, int(sizeof(verts))));
    m_vbuf->create();

    m_ibuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, int(sizeof(idx))));
    m_ibuf->create();

    // uložíme si surová data, aby upload mohl proběhnout z render() (k dispozici je cb)
    m_geomVertsData.resize(int(sizeof(verts)));
    memcpy(m_geomVertsData.data(), verts.data(), sizeof(verts));

    m_geomIdxData.resize(int(sizeof(idx)));
    memcpy(m_geomIdxData.data(), idx.data(), sizeof(idx));
}

void TwoCubesRhiWidget::createTexture()
{
    // Vytvoříme šachovnicovou texturu 256x256
    const int W = 256, H = 256; QImage img(W, H, QImage::Format_RGBA8888);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            bool c = ((x/32) % 2) ^ ((y/32) % 2);
            QColor col = c ? QColor(220, 220, 220) : QColor(90, 90, 90);
            img.setPixelColor(x, y, col);
        }
    }

    m_texImg = img;

    m_tex.reset(m_rhi->newTexture(QRhiTexture::RGBA8, QSize(W, H)));
    m_tex->create();

    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                      QRhiSampler::Repeat, QRhiSampler::Repeat));
    m_sampler->create();
}

void TwoCubesRhiWidget::buildPipelines()
{
    // UBO pro každou krychli
    m_uboCubeA.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UBOData)));
    m_uboCubeA->create();
    m_uboCubeB.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UBOData)));
    m_uboCubeB->create();

    // Načtení shaderů (.qsb z resource)
    const QShader vsh = loadQSB(":/shaders/prebuild/cube.vert.qsb");
    const QShader fshLambert = loadQSB(":/shaders/prebuild/lambert.frag.qsb");
    const QShader fshToon = loadQSB(":/shaders/prebuild/toon.frag.qsb");

    // Layout vertexů
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ QRhiVertexInputBinding(8 * sizeof(float)) });
    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float)),
        QRhiVertexInputAttribute(0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float))
    });

    auto createPipeline = [&](const QShader &frag, std::unique_ptr<QRhiGraphicsPipeline> &outPipe,
                              std::unique_ptr<QRhiShaderResourceBindings> &outSrb,
                              QRhiBuffer *ubo)
    {
        outSrb.reset(m_rhi->newShaderResourceBindings());
        outSrb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubo),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_tex.get(), m_sampler.get())
        });
        outSrb->create(); // jen jednou při inicializaci

        outPipe.reset(m_rhi->newGraphicsPipeline());
        outPipe->setShaderStages({
            { QRhiShaderStage::Vertex, vsh },
            { QRhiShaderStage::Fragment, frag }
        });
        outPipe->setCullMode(QRhiGraphicsPipeline::Back);
        outPipe->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
        outPipe->setDepthTest(true);
        outPipe->setDepthWrite(true);
        outPipe->setSampleCount(this->renderTarget()->sampleCount());
        outPipe->setRenderPassDescriptor(m_rpDesc);
        outPipe->setVertexInputLayout(inputLayout);
        outPipe->create();
    };

    // Krychle A → Lambert
    createPipeline(fshLambert, m_pipeLambert, m_srbLambert, m_uboCubeA.get());
    // Krychle B → Toon
    createPipeline(fshToon, m_pipeToon, m_srbToon, m_uboCubeB.get());
}

void TwoCubesRhiWidget::render(QRhiCommandBuffer *cb)
{
    using namespace std;
    qDebug() << "---- render() start ----";

    // základní sanity checks
    if (!cb) {
        qCritical() << "render: cb == nullptr!";
        return;
    }
    if (!m_rhi) {
        qCritical() << "render: m_rhi == nullptr!";
        return;
    }
    if (!renderTarget()) {
        qCritical() << "render: renderTarget() == nullptr!";
        return;
    }
    if (!m_rpDesc) {
        qCritical() << "render: m_rpDesc == nullptr!";
        return;
    }

    qDebug() << "pipelines:" << m_pipeLambert.get() << m_pipeToon.get();
    qDebug() << "SRBs:" << m_srbLambert.get() << m_srbToon.get();
    qDebug() << "buffers vbuf/ibuf/uboA/uboB:" << m_vbuf.get() << m_ibuf.get() << m_uboCubeA.get() << m_uboCubeB.get();
    qDebug() << "texture/sampler:" << m_tex.get() << m_sampler.get();
    qDebug() << "m_uploaded:" << m_uploaded;
    qDebug() << "sampleCount:" << renderTarget()->sampleCount();

    // Pokud něco klíčového chybí — vypiš a safe return (prevence pádu)
    if (!m_pipeLambert || !m_pipeToon) {
        qCritical() << "render: one or more pipelines missing; abort render.";
        return;
    }
    if (!m_srbLambert || !m_srbToon) {
        qCritical() << "render: one or more SRBs missing; abort render.";
        return;
    }
    if (!m_vbuf || !m_ibuf) {
        qCritical() << "render: vertex/index buffer missing; abort render.";
        return;
    }
    // textura/sampler může chybět do prvního uploadu, to není fatální

    // výpočet kamer a transformací
    const QSize pixelSize = this->size() * this->devicePixelRatioF();
    if (pixelSize.width() <= 0 || pixelSize.height() <= 0) {
        qWarning() << "render: invalid pixelSize" << pixelSize;
        return;
    }
    const float aspect = float(pixelSize.width()) / float(pixelSize.height());

    QMatrix4x4 proj; proj.perspective(60.f, aspect, 0.1f, 100.0f);
    QMatrix4x4 view; view.lookAt(QVector3D(0, 2.5f, 6.0f), QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    QVector4D lightPos = QVector4D(3.0f * cosf(m_angle * 0.02f), 2.5f, 3.0f * sinf(m_angle * 0.02f),1.0);

   // QVector4D(lightPos, 1.0f);
    QMatrix4x4 modelA; modelA.rotate(m_angle, 0.f, 1.f, 0.f); modelA.translate(-1.7f, 0.f, 0.f);
    QMatrix4x4 modelB; modelB.rotate(-m_angle*1.2f, 1.f, 0.f, 0.f); modelB.translate(+1.7f, 0.f, 0.f);

    auto makeUBO = [&](const QMatrix4x4 &model) {
        UBOData u{};

        // mat4 -> float[16]
        const float *pMvp = model.constData();
        const float *pProj = proj.constData();
        const float *pView = view.constData();

        // mvp = proj * view * model
        QMatrix4x4 mvpMat = proj * view * model;
        memcpy(u.mvp, mvpMat.constData(), sizeof(float)*16);

        // model
        memcpy(u.model, model.constData(), sizeof(float)*16);

        // normalMat (4x4)
        QMatrix3x3 nm3 = model.normalMatrix();
        QMatrix4x4 nm4;
        nm4.setRow(0, QVector4D(nm3(0,0), nm3(0,1), nm3(0,2), 0.0f));
        nm4.setRow(1, QVector4D(nm3(1,0), nm3(1,1), nm3(1,2), 0.0f));
        nm4.setRow(2, QVector4D(nm3(2,0), nm3(2,1), nm3(2,2), 0.0f));
        nm4.setRow(3, QVector4D(0.0f, 0.0f, 0.0f, 1.0f));
        memcpy(u.normalMat, nm4.constData(), sizeof(float)*16);

        // lightPos
        u.lightPos[0] = lightPos.x();
        u.lightPos[1] = lightPos.y();
        u.lightPos[2] = lightPos.z();
        u.lightPos[3] = 1.0f;

        return u;
    };

    const UBOData uboA = makeUBO(modelA);
    const UBOData uboB = makeUBO(modelB);

    // resource update batch: upload statických dat první frame, UBO vždy
    QRhiResourceUpdateBatch *u = m_rhi->nextResourceUpdateBatch();
    if (!u) {
        qCritical() << "render: nextResourceUpdateBatch() returned nullptr!";
        return;
    }

    if (!m_uploaded) {
        // safety: ensure raw data exist
        if (m_geomVertsData.isEmpty() || m_geomIdxData.isEmpty()) {
            qCritical() << "render: geometry raw data empty but upload not done; abort.";
            return;
        }
        // statický upload
        u->uploadStaticBuffer(m_vbuf.get(), m_geomVertsData.constData());
        u->uploadStaticBuffer(m_ibuf.get(), m_geomIdxData.constData());

        if (!m_texImg.isNull()) {
            u->uploadTexture(m_tex.get(), m_texImg);
        } else {
            qWarning() << "render: texture image is null; continuing without texture upload";
        }

        m_uploaded = true;
        qDebug() << "render: performed initial static uploads.";
    }
 //   static_assert(sizeof(UBOData) == 208, "UBOData size must be 208 bytes to match std140 layout (mat4+mat4+mat4+vec3+pad)");
    qDebug() << "sizeof(UBOData) =" << int(sizeof(UBOData));
    // aktualizace UBO
    u->updateDynamicBuffer(m_uboCubeA.get(), 0, int(sizeof(UBOData)), &uboA);
    u->updateDynamicBuffer(m_uboCubeB.get(), 0, int(sizeof(UBOData)), &uboB);
 //   auto *u = m_rhi->nextResourceUpdateBatch();
  //  u->updateDynamicBuffer(m_uboCubeA.get(), 0, sizeof(UBOData), &uboA);
  //  u->updateDynamicBuffer(m_uboCubeB.get(), 0, sizeof(UBOData), &uboB);
  //  m_rhi->resourceUpdate(u);

    const QColor clearCol = QColor(18, 18, 22);
    // začněte pass s update batchem
    cb->beginPass(this->renderTarget(), clearCol, {1.0f, 0}, u);

    // vertex/index nastavení
    QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding, m_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt16);

    // DRAW A (Lambert)
     if (m_pipeLambert && m_srbLambert) {
        cb->setGraphicsPipeline(m_pipeLambert.get());
        cb->setShaderResources(m_srbLambert.get());
        cb->drawIndexed(m_indexCount);
    } else {
        qWarning() << "render: skipping Lambert draw because pipeline/SRB missing";
    }

    // DRAW B (Toon)
    if (m_pipeToon && m_srbToon) {
        cb->setGraphicsPipeline(m_pipeToon.get());
        cb->setShaderResources(m_srbToon.get());
        cb->drawIndexed(m_indexCount);
    } else {
        qWarning() << "render: skipping Toon draw because pipeline/SRB missing";
    }

    cb->endPass();

    qDebug() << "---- render() end ----";
}
