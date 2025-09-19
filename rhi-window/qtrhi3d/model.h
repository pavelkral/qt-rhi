#ifndef MODEL_H
#define MODEL_H


#include "types.h"
#include "transform.h"

#include <rhi/qrhi.h>
#include <memory>
#include <QMatrix4x4>
#include <QMatrix4x4>
#include <QVector4D>


class Model {

public:
    Transform transform;

    void addVertAndInd(const QVector<float> &vertices, const QVector<quint16> &indices);
    void init(QRhi *rhi,QRhiRenderPassDescriptor *rp,const QShader &vs,const QShader &fs,
              QRhiResourceUpdateBatch *u,QRhiTexture *shadowmap,QRhiSampler *shadowsampler);
    void draw(QRhiCommandBuffer *cb);   
    void updateUniforms(const QMatrix4x4 &viewProjection,float opacity, QRhiResourceUpdateBatch *u);
    QVector<float> computeTangents(const QVector<float>& vertices, const QVector<quint16>& indices);
    void updateUbo(Ubo ubo,QRhiResourceUpdateBatch *u,QRhiBuffer *shadowUbo, QRhiResourceUpdateBatch *shadowBatch);
    void loadTexture(QRhi *m_rhi,const QSize &, QRhiResourceUpdateBatch *u,QString tex_name,std::unique_ptr<QRhiTexture> &texture,
                     std::unique_ptr<QRhiSampler> &sampler);
    Transform &getTransform() {
        return transform;
    }


    void DrawForShadow(QRhiCommandBuffer *cb,
                     QRhiGraphicsPipeline *shadowPipeline,
                     QRhiShaderResourceBindings *shadowSRB,
                     QRhiBuffer *shadowUbo,
                    Ubo ubo,QRhiResourceUpdateBatch *u) const ;
private:

   // Ubo ubo;
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
