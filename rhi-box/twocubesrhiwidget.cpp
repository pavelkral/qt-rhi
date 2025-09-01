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

void TwoCubesRhiWidget::initialize(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
    m_rhi = this->rhi();

    // renderPassDescriptor spravuje QRhiWidget -> jen získáme pointer (nevlastníme)
    m_rpDesc.reset(this->renderTarget()->renderPassDescriptor());

    createGeometry();
    createTexture();
    buildPipelines();
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
    m_rpDesc.reset();
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
    m_uboCubeA.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, int(sizeof(UBOData))));
    m_uboCubeA->create();
    m_uboCubeB.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, int(sizeof(UBOData))));
    m_uboCubeB->create();

    // Načtení shaderů (.qsb z resource)
    const QShader vsh = loadQSB(":/shaders/cube.vert.qsb");
    const QShader fshLambert = loadQSB(":/shaders/lambert.frag.qsb");
    const QShader fshToon = loadQSB(":/shaders/toon.frag.qsb");

    // Layout vertexů
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ QRhiVertexInputBinding(8 * sizeof(float)) });
    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),                 // pos
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float)), // normal
        QRhiVertexInputAttribute(0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float))  // uv
    });

    auto createPipeline = [&](const QShader &frag, std::unique_ptr<QRhiGraphicsPipeline> &outPipe,
                              std::unique_ptr<QRhiShaderResourceBindings> &outSrb) {
        outSrb.reset(m_rhi->newShaderResourceBindings());
        outSrb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_uboCubeA.get()), // placeholder, před draw přepneme na A/B
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_tex.get(), m_sampler.get())
        });
        outSrb->create();

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
        outPipe->setRenderPassDescriptor(m_rpDesc.get());
        outPipe->setVertexInputLayout(inputLayout);
        outPipe->create();
    };

    createPipeline(fshLambert, m_pipeLambert, m_srbLambert);
    createPipeline(fshToon, m_pipeToon, m_srbToon);
}

void TwoCubesRhiWidget::render(QRhiCommandBuffer *cb)
{
    const QSize pixelSize = this->size() * this->devicePixelRatioF();
    const float aspect = float(pixelSize.width()) / float(pixelSize.height());

    // Kamera
    QMatrix4x4 proj; proj.perspective(60.f, aspect, 0.1f, 100.0f);
    QMatrix4x4 view; view.lookAt(QVector3D(0, 2.5f, 6.0f), QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    QVector3D lightPos = QVector3D(3.0f * cosf(m_angle * 0.02f), 2.5f, 3.0f * sinf(m_angle * 0.02f));

    // Model matice pro A a B
    QMatrix4x4 modelA; modelA.rotate(m_angle, 0.f, 1.f, 0.f); modelA.translate(-1.7f, 0.f, 0.f);
    QMatrix4x4 modelB; modelB.rotate(-m_angle*1.2f, 1.f, 0.f, 0.f); modelB.translate(+1.7f, 0.f, 0.f);

    auto makeUBO = [&](const QMatrix4x4 &model){
        UBOData u{};
        u.model = model;
        u.mvp = proj * view * model;
        // Normal matrix = inverse transpose of upper-left 3x3
        QMatrix3x3 nm = model.normalMatrix();
        u.normalMat = nm;
        u.lightPos = lightPos;
        return u;
    };

    const UBOData uboA = makeUBO(modelA);
    const UBOData uboB = makeUBO(modelB);

    // Vytvoříme batch (upload + ubo update). Batch předáme do beginPass.
    auto *u = m_rhi->nextResourceUpdateBatch();

    if (!m_uploaded) {
        // upload surových bufferů/textury jen jednou
        u->uploadStaticBuffer(m_vbuf.get(), m_geomVertsData.constData());
        u->uploadStaticBuffer(m_ibuf.get(), m_geomIdxData.constData());
        u->uploadTexture(m_tex.get(), m_texImg);
        m_uploaded = true;
    }

    u->updateDynamicBuffer(m_uboCubeA.get(), 0, sizeof(UBOData), &uboA);
    u->updateDynamicBuffer(m_uboCubeB.get(), 0, sizeof(UBOData), &uboB);

    const QColor clearCol = QColor(18, 18, 22);
    cb->beginPass(this->renderTarget(), clearCol, {1.0f, 0}, u);

    // Nastavení vertex/index bufferů
    QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding, m_ibuf.get(), 0, QRhiCommandBuffer::IndexUInt16);

    // DRAW: Krychle A (Lambert/Blinn)
    cb->setGraphicsPipeline(m_pipeLambert.get());
    // přepnout SRB tak, aby binding 0 ukazoval na UBO A
    m_srbLambert->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uboCubeA.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_tex.get(), m_sampler.get())
    });
    m_srbLambert->create();
    cb->setShaderResources(m_srbLambert.get());
    cb->drawIndexed(m_indexCount);

    // DRAW: Krychle B (Toon)
    cb->setGraphicsPipeline(m_pipeToon.get());
    m_srbToon->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uboCubeB.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_tex.get(), m_sampler.get())
    });
    m_srbToon->create();
    cb->setShaderResources(m_srbToon.get());
    cb->drawIndexed(m_indexCount);

    cb->endPass();
}
