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

void TwoCubesRhiWidget::initialize(QRhiCommandBuffer * cb)
{
    m_rhi = this->rhi();

    // Render pass descriptor přímo z render targetu
   // m_rpDesc = renderTarget()->renderPassDescriptor();
   // renderTarget()->setRenderPassDescriptor(m_rpDesc);
    m_rpDesc = renderTarget()->renderPassDescriptor();
    // --- GEOMETRIE ---
    createGeometry();   // vytvoření m_vbuf, m_ibuf + upload dat
    createTexture();    // vytvoření m_tex/m_sampler + upload
    buildPipelines();

    // === DŮLEŽITÉ: jednorázový upload statických dat ===
    QRhiResourceUpdateBatch *u = m_rhi->nextResourceUpdateBatch();
    u->uploadStaticBuffer(m_vbuf.get(), m_geomVertsData.constData());
    u->uploadStaticBuffer(m_ibuf.get(), m_geomIdxData.constData());
    if (!m_texImg.isNull())
        u->uploadTexture(m_tex.get(), m_texImg);

    // v initialize() máme platné CB → pošli batch hned teď
    cb->resourceUpdate(u);

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
    // --- UBO pro každou krychli ---
    m_uboCubeA.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UBOData)));
    m_uboCubeA->create();
    m_uboCubeB.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UBOData)));
    m_uboCubeB->create();

    // --- Shadery ---
    const QShader vsh = loadQSB(":/shaders/prebuild/cube.vert.qsb");
    const QShader fshLambert = loadQSB(":/shaders/prebuild/lambert.frag.qsb");
    const QShader fshToon = loadQSB(":/shaders/prebuild/toon.frag.qsb");

    // --- Vertex input layout ---
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ QRhiVertexInputBinding(8 * sizeof(float)) });
    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),               // pozice
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float)), // normála
        QRhiVertexInputAttribute(0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float))  // UV
    });

    // --- Pomocná lambda pro vytvoření pipeline ---
    auto createPipeline = [&](const QShader &frag,
                              std::unique_ptr<QRhiGraphicsPipeline> &outPipe,
                              std::unique_ptr<QRhiShaderResourceBindings> &outSrb,
                              QRhiBuffer *ubo)
    {
        // --- Shader Resource Bindings ---
        outSrb.reset(m_rhi->newShaderResourceBindings());
        outSrb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(
                0,
                QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                ubo),
            QRhiShaderResourceBinding::sampledTexture(
                1,
                QRhiShaderResourceBinding::FragmentStage,
                m_tex.get(),
                m_sampler.get())
        });
        outSrb->create();

        // --- Graphics pipeline ---
        outPipe.reset(m_rhi->newGraphicsPipeline());
        outPipe->setShaderStages({
            { QRhiShaderStage::Vertex, vsh },
            { QRhiShaderStage::Fragment, frag }
        });
        outPipe->setCullMode(QRhiGraphicsPipeline::Back);
        outPipe->setDepthTest(true);
        outPipe->setDepthWrite(true);
        outPipe->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
        outPipe->setSampleCount(renderTarget()->sampleCount());
        outPipe->setShaderResourceBindings(outSrb.get());
        outPipe->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
        outPipe->setVertexInputLayout(inputLayout);
       // outPipe->create();
        if (!outPipe->create())
            qCritical() << "Nepodařilo se vytvořit pipeline!";
    };

    // --- Vytvoření pipeline ---
    createPipeline(fshLambert, m_pipeLambert, m_srbLambert, m_uboCubeA.get());
    createPipeline(fshToon, m_pipeToon, m_srbToon, m_uboCubeB.get());
}


void TwoCubesRhiWidget::render(QRhiCommandBuffer *cb)
{
    qDebug() << "---- render() start ----";

    if (!cb || !m_rhi || !renderTarget()) {
        qCritical() << "render: invalid state, abort";
        return;
    }

    // Kontrola bufferů
    if (!m_vbuf || !m_ibuf || !m_uboCubeA || !m_uboCubeB) {
        qWarning() << "render: missing GPU buffers, skipping frame";
        return;
    }

    // Debug-safe výpis
    qDebug() << "drawIndexed: count=" << m_indexCount
             << "geomVertsData=" << (m_geomVertsData.isEmpty() ? -1 : m_geomVertsData.size())
             << "geomIdxData=" << (m_geomIdxData.isEmpty() ? -1 : m_geomIdxData.size())
             << "m_uploaded=" << m_uploaded;

    // --- výpočet transformací ---
    const QSize pixelSize = size() * devicePixelRatioF();
    if (pixelSize.isEmpty()) {
        qWarning() << "render: pixelSize empty, skipping";
        return;
    }

    const float aspect = float(pixelSize.width()) / float(pixelSize.height());
    QMatrix4x4 proj; proj.perspective(60.f, aspect, 0.1f, 100.f);
    QMatrix4x4 view; view.lookAt(QVector3D(0, 2.5f, 6.0f),
                QVector3D(0, 0, 0),
                QVector3D(0, 1, 0));

    QVector4D lightPos(3.0f * cosf(m_angle * 0.02f), 2.5f,
                       3.0f * sinf(m_angle * 0.02f), 1.0f);

    QMatrix4x4 modelA; modelA.rotate(m_angle, 0.f, 1.f, 0.f);
    modelA.translate(-1.7f, 0.f, 0.f);
    QMatrix4x4 modelB; modelB.rotate(-m_angle * 1.2f, 1.f, 0.f, 0.f);
    modelB.translate(+1.7f, 0.f, 0.f);

    auto makeUBO = [&](const QMatrix4x4 &model) {
        UBOData u{};
        QMatrix4x4 mvp = proj * view * model;
        memcpy(u.mvp, mvp.constData(), sizeof(float)*16);
        memcpy(u.model, model.constData(), sizeof(float)*16);

        QMatrix3x3 nm3 = model.normalMatrix();
        QMatrix4x4 nm4;
        nm4.setRow(0, QVector4D(nm3(0,0), nm3(0,1), nm3(0,2), 0));
        nm4.setRow(1, QVector4D(nm3(1,0), nm3(1,1), nm3(1,2), 0));
        nm4.setRow(2, QVector4D(nm3(2,0), nm3(2,1), nm3(2,2), 0));
        nm4.setRow(3, QVector4D(0,0,0,1));

        memcpy(u.normalMat, nm4.constData(), sizeof(float)*16);
        u.lightPos[0] = lightPos.x();
        u.lightPos[1] = lightPos.y();
        u.lightPos[2] = lightPos.z();
        u.lightPos[3] = lightPos.w();
        return u;
    };

    const UBOData uboA = makeUBO(modelA);
    const UBOData uboB = makeUBO(modelB);

    QRhiResourceUpdateBatch *u = m_rhi->nextResourceUpdateBatch();
    if (!u) {
        qWarning() << "render: failed to get resource update batch";
        return;
    }

    u->updateDynamicBuffer(m_uboCubeA.get(), 0, sizeof(UBOData), &uboA);
    u->updateDynamicBuffer(m_uboCubeB.get(), 0, sizeof(UBOData), &uboB);

    cb->beginPass(renderTarget(), QColor(18, 18, 22), {1.f, 0}, u);

    QRhiCommandBuffer::VertexInput vi(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vi, m_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt16);

    // DRAW A (Lambert)
    if (m_pipeLambert && m_srbLambert) {
        qDebug() << "[Lambert] pipeline =" << m_pipeLambert.get()
        << "SRB =" << m_srbLambert.get();
        cb->setGraphicsPipeline(m_pipeLambert.get());
        cb->setShaderResources(m_srbLambert.get());  // ❌ dříve prázdné volání
        cb->drawIndexed(m_indexCount);
    } else {
        qWarning() << "Lambert pipeline nebo SRB není připravena, draw přeskočeno!";
    }

    // DRAW B (Toon)
    if (m_pipeToon && m_srbToon) {
        qDebug() << "[Toon] pipeline =" << m_pipeToon.get()
        << "SRB =" << m_srbToon.get();
        cb->setGraphicsPipeline(m_pipeToon.get());
        cb->setShaderResources(m_srbToon.get()); // ❌ dříve prázdné volání
        cb->drawIndexed(m_indexCount);
    } else {
        qWarning() << "Toon pipeline nebo SRB není připravena, draw přeskočeno!";
    }

    cb->endPass();
    qDebug() << "---- render() end ----";
}
