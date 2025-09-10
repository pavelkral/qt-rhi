#ifndef CUBE_H
#define CUBE_H

#include "transform.h"
#include <rhi/qrhi.h>
#include <memory>
#include <QMatrix4x4>
#include <QMatrix4x4>
#include <QVector4D>


struct Ubo {
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    QMatrix4x4 lightSpace;
    QVector4D color;
    float opacity;
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

    Transform &getTransform() { return transform; }

private:

    // Per-instance resources
    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ibuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    int m_indexCount = 0;
    QVector<float> m_vert;
    QVector<quint16> m_ind;
};

#endif // CUBE_H
