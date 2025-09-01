#ifndef RHIWIDGET_H
#define RHIWIDGET_H

#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QFile>
#include <QTimer>
#include "RhiFBXModel.h"


class RhiWidget : public QRhiWidget {
    Q_OBJECT

private:
    QRhi* m_rhi = nullptr;
    std::unique_ptr<RhiFBXModel> m_model;
    std::unique_ptr<QRhiGraphicsPipeline> m_ps;
    std::unique_ptr<QRhiSampler> m_sampler;
    QShader* m_vs = nullptr;
    QShader* m_fs = nullptr;
    using QRhiWidget::QRhiWidget;
    QTimer* m_timer = nullptr;
    float m_angle = 0.0f;

public:


    RhiWidget(QWidget *parent = nullptr)
        : QRhiWidget(parent)
    {
        // jednoduchá animace modelu
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this]{
            m_angle += 1.0f;
            if(m_angle > 360.f) m_angle -= 360.f;
            update(); // vyvolá render
        });
        m_timer->start(16);
    }

protected:
    void initialize(QRhiCommandBuffer *cb) override {
        m_rhi = this->rhi();

        // sampler
        m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear,
                                          QRhiSampler::Linear,
                                          QRhiSampler::None,
                                          QRhiSampler::Repeat,
                                          QRhiSampler::Repeat));
        m_sampler->create();

        // načtení modelu
        m_model = std::make_unique<RhiFBXModel>(m_rhi, "assets/models/Player/untitled.fbx");
        m_model->createResources(m_sampler.get());

        // pipeline
        m_ps.reset(m_rhi->newGraphicsPipeline());
        m_ps->setTopology(QRhiGraphicsPipeline::Triangles);
        m_ps->setCullMode(QRhiGraphicsPipeline::Back);
        m_ps->setFrontFace(QRhiGraphicsPipeline::CCW);
        m_ps->setDepthTest(true);
        m_ps->setDepthWrite(true);

        // vertex input
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ {14 * sizeof(float)} });
        inputLayout.setAttributes({
            {0, 0, QRhiVertexInputAttribute::Float3, 0},          // pos
            {0, 1, QRhiVertexInputAttribute::Float3, 3*sizeof(float)}, // norm
            {0, 2, QRhiVertexInputAttribute::Float2, 6*sizeof(float)}, // uv
            {0, 3, QRhiVertexInputAttribute::Float3, 8*sizeof(float)}, // tangent
            {0, 4, QRhiVertexInputAttribute::Float3, 11*sizeof(float)} // bitangent
        });
        m_ps->setVertexInputLayout(inputLayout);

        // shader stages
        QRhiShaderStage vsStage(QRhiShaderStage::Vertex, loadShader(":/shaders/prebuild/model.vert.qsb"));
        QRhiShaderStage fsStage(QRhiShaderStage::Fragment, loadShader(":/shaders/prebuild/model.frag.qsb"));
        m_ps->setShaderStages({ vsStage, fsStage });
        m_ps->setShaderResourceBindings(nullptr); // SRB je per-mesh

        m_ps->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
        m_ps->create();
    }

    void render(QRhiCommandBuffer *cb) override {



       // cb->beginPass(this->renderTarget(), QColor(30,30,30), {1.0f, 0});
      //  cb->setGraphicsPipeline(m_ps.get());

        // UBO data
        UBufData u;
        u.uModel.setToIdentity();
        u.uModel.rotate(m_angle, 0,1,0);
        u.uView.lookAt({0,2,5},{0,0,0},{0,1,0});
        u.uProj.perspective(45.f,float(width())/float(height()),0.1f,100.f);
        u.uLightPos = QVector3D(2,4,3);
        u.uLightColor = QVector3D(1,1,1);
        u.uCameraPos = QVector3D(0,2,5);
        u.uAlbedoColor = QVector3D(1,1,1);
        u.uAmbientStrength = 0.2f;
        u.uMetallicFactor = 0.0f;
        u.uSmoothnessFactor = 0.5f;
        u.uHasAlbedo = u.uHasNormal = u.uHasMetallic = u.uHasSmoothness = 0;

        m_model->draw(cb,u);

       // cb->endPass();
    }

private:
    QShader* loadShaderFile(const QString& path) {
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        QByteArray data = f.readAll();
        QShader* s = new QShader(QShader::fromSerialized(data));
        return s;
    }

    QShader loadShader(const QString& path) {
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        QByteArray data = f.readAll();
        return QShader::fromSerialized(data);
    }

};

#endif // RHIWIDGET_H






