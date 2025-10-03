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
    QVector<float> m_vert;
    QVector<quint16> m_ind;
    int m_indexCount = 0;
    float m_opacity = 1;
    int m_opacityDir = -1;

    void addVertAndInd(const QVector<float> &vertices, const QVector<quint16> &indices);
    void init(QRhi *rhi,QRhiRenderPassDescriptor *rp,const QShader &vs,const QShader &fs,
              QRhiResourceUpdateBatch *u,QRhiTexture *shadowmap,QRhiSampler *shadowsampler,const TextureSet &set);

    void updateUbo(Ubo ubo,QRhiResourceUpdateBatch *u);
    void updateShadowUbo(Ubo ubo, QRhiResourceUpdateBatch *u);
    void draw(QRhiCommandBuffer *cb);
    void DrawForShadow(QRhiCommandBuffer *cb,QRhiGraphicsPipeline *shadowPipeline,
                       Ubo ubo,QRhiResourceUpdateBatch *u)  ;

    QVector<float> computeTangents(const QVector<float>& vertices, const QVector<quint16>& indices);
    void loadTexture(QRhi *m_rhi,const QSize &, QRhiResourceUpdateBatch *u,QString tex_name,
                     std::unique_ptr<QRhiTexture> &texture,
                     std::unique_ptr<QRhiSampler> &sampler);
    Transform &getTransform() {
        return transform;
    }
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

    std::unique_ptr<QRhiBuffer> m_shadowUbo;
    std::unique_ptr<QRhiShaderResourceBindings> m_shadowSrb;

};

#endif // MODEL_H
