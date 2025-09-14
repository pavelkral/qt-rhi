#ifndef MODEL_H
#define MODEL_H

#include "transform.h"
#include <rhi/qrhi.h>
#include <memory>
#include <QMatrix4x4>
#include <QMatrix4x4>
#include <QVector4D>


struct Ubo {
    QMatrix4x4 mvp;         // 0–63
    QVector4D opacity;      // 64–79
    QMatrix4x4 model;       // 80–143
    QMatrix4x4 view;        // 144–207
    QMatrix4x4 projection;  // 208–271
    QMatrix4x4 lightSpace;  // 272–335
    QVector4D lightPos;
    QVector4D camPos;    // 336–351
    QVector4D color;        // 352–367
};
struct Vertex {
    QVector3D pos;
    QVector3D normal;
    QVector2D uv;
    QVector3D tangent;
    QVector3D bitangent;
};
class Model {

public:

    void init(QRhi *rhi,
              QRhiRenderPassDescriptor *rp,
              const QShader &vs,
              const QShader &fs,
              QRhiResourceUpdateBatch *u,QString tex_name);

    void addVertAndInd(const QVector<float> &vertices, const QVector<quint16> &indices) {
        m_vert = vertices;
        m_ind = indices;
        m_indexCount = indices.size();
    }
    void setModelMatrix(const QMatrix4x4 &mvp, QRhiResourceUpdateBatch *u);
    void draw(QRhiCommandBuffer *cb);
    Transform transform;
    void updateUniforms(const QMatrix4x4 &viewProjection,float opacity, QRhiResourceUpdateBatch *u);
    QVector<float> computeTangents(const QVector<float>& vertices, const QVector<quint16>& indices);

    void updateUbo(const QMatrix4x4 &view,
                        const QMatrix4x4 &projection,
                        const QMatrix4x4 &lightSpace,
                        const QVector3D &color,
                        float opacity,
                        QRhiResourceUpdateBatch *u);

    Transform &getTransform() { return transform; }
    void loadTexture(QRhi *m_rhi,const QSize &, QRhiResourceUpdateBatch *u,QString tex_name);
private:
    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ibuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    float m_opacity = 1;
    int m_opacityDir = -1;

    QVector<float> m_vert;
    QVector<quint16> m_ind;
    int m_indexCount = 0;
};

#endif // MODEL_H
