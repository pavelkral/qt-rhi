#ifndef CUBE_H
#define CUBE_H
#pragma once
#include "transform.h"

#include <rhi/qrhi.h>
#include <memory>
#include <QMatrix4x4>
#include <QMatrix4x4>
#include <QVector4D>

// Uniform Buffer Object struktura
// Musí přesně odpovídat struktuře definované v shaderu
struct Ubo {
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    QMatrix4x4 lightSpace;
    QVector4D color;    // Používáme QVector4D pro správné zarovnání paměti (16 bytů)
    float opacity;
    // Padding, aby celková velikost byla násobkem 16, což je často vyžadováno
    float pad[3];
};


class Cube {
public:
    void init(QRhi *rhi,
              QRhiTexture *texture,
              QRhiSampler *sampler,
              QRhiRenderPassDescriptor *rp,
              const QShader &vs,
              const QShader &fs,
              QRhiResourceUpdateBatch *u);
    void addVertAndInd(const QVector<float> &vertices, const QVector<quint16> &indices) {
        m_vert = vertices;
        m_ind = indices;
        m_indexCount = indices.size();
    }
    void setModelMatrix(const QMatrix4x4 &mvp, QRhiResourceUpdateBatch *u);
    void draw(QRhiCommandBuffer *cb);
    Transform transform;
    void updateUniforms(const QMatrix4x4 &viewProjection, QRhiResourceUpdateBatch *u);
    // void updateUniforms(const QMatrix4x4 &view,
    //                     const QMatrix4x4 &projection,
    //                     const QMatrix4x4 &lightSpace,
    //                     const QVector3D &color,
    //                     float opacity,
    //                     QRhiResourceUpdateBatch *u);
    // Přidáme getter pro snadný přístup k transformaci objektu
    Transform &getTransform() { return transform; }
private:
    // Per-instance resources
    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ibuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    int m_indexCount = 0;

    // Per-instance vertex/index data

    QVector<float> m_vert;
    QVector<quint16> m_ind;
};

#endif // CUBE_H
