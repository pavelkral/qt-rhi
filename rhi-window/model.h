#ifndef MODEL_H
#define MODEL_H

#include "transform.h"
#include <rhi/qrhi.h>
#include <memory>
#include <QMatrix4x4>
#include <QMatrix4x4>
#include <QVector4D>


struct Ubo {
    QMatrix4x4 mvp;
    QVector4D opacity;
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    QMatrix4x4 lightSpace;
    QVector4D lightPos;
    QVector4D camPos;
    QVector4D color;
};

struct Vertex {
    QVector3D pos;
    QVector3D normal;
    QVector2D uv;
    QVector3D tangent;
    QVector3D bitangent;
};

struct TempVert {
    QVector3D pos;
    QVector3D normal;
    QVector2D uv;
    QVector3D tangent;
    QVector3D bitangent;
};

class Model {

public:
    Transform transform;

    void init(QRhi *rhi,QRhiRenderPassDescriptor *rp,const QShader &vs,const QShader &fs,QRhiResourceUpdateBatch *u,QString tex_name);
    void addVertAndInd(const QVector<float> &vertices, const QVector<quint16> &indices);
    void setModelMatrix(const QMatrix4x4 &mvp, QRhiResourceUpdateBatch *u);
    void draw(QRhiCommandBuffer *cb);   
    void updateUniforms(const QMatrix4x4 &viewProjection,float opacity, QRhiResourceUpdateBatch *u);
    QVector<float> computeTangents(const QVector<float>& vertices, const QVector<quint16>& indices);
    void updateUbo(const QMatrix4x4 &view,const QMatrix4x4 &projection,const QMatrix4x4 &lightSpace,const QVector3D &color,
                        const QVector3D &lightPos,
                        const QVector3D &camPos,
                        const float opacity,
                        QRhiResourceUpdateBatch *u);

    Transform &getTransform() {
        return transform;
    }
    void loadTexture(QRhi *m_rhi,const QSize &, QRhiResourceUpdateBatch *u,QString tex_name,std::unique_ptr<QRhiTexture> &texture,
                     std::unique_ptr<QRhiSampler> &sampler);
private:

    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiTexture> m_tex_norm;
    std::unique_ptr<QRhiSampler> m_sampler_norm;
    std::unique_ptr<QRhiTexture> m_texture_met;
    std::unique_ptr<QRhiSampler> m_sampler_met;
    std::unique_ptr<QRhiTexture> m_texture_rough;
    std::unique_ptr<QRhiSampler> m_sampler_rough;
    std::unique_ptr<QRhiTexture> m_texture_ao;
    std::unique_ptr<QRhiSampler> m_sampler_ao;
    std::unique_ptr<QRhiTexture> m_texture_height;
    std::unique_ptr<QRhiSampler> m_sampler_height;

    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ibuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;

    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    QVector<float> m_vert;
    QVector<quint16> m_ind;
    int m_indexCount = 0;
    float m_opacity = 1;
    int m_opacityDir = -1;

};

#endif // MODEL_H
