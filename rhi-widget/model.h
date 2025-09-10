#ifndef MODEL_H
#define MODEL_H
#pragma once
#include <rhi/qrhi.h>
#include <memory>
#include <QMatrix4x4>

class Cube {
public:
    void init(QRhi *rhi,
              QRhiTexture *texture,
              QRhiSampler *sampler,
              QRhiRenderPassDescriptor *rp,
              const QShader &vs,
              const QShader &fs,
              QRhiResourceUpdateBatch *u);

    void setModelMatrix(const QMatrix4x4 &mvp, QRhiResourceUpdateBatch *u);
    void draw(QRhiCommandBuffer *cb);

private:
    // Per-instance resources
    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ibuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    int m_indexCount = 0;

    // Per-instance vertex/index data
    float m_vertices[8 * 5] = {
        // pos                  // uv
        -0.5f,-0.5f, 0.5f,      0.0f,0.0f, // 0
        0.5f,-0.5f, 0.5f,      1.0f,0.0f, // 1
        0.5f, 0.5f, 0.5f,      1.0f,1.0f, // 2
        -0.5f, 0.5f, 0.5f,      0.0f,1.0f, // 3
        -0.5f,-0.5f,-0.5f,      1.0f,0.0f, // 4
        0.5f,-0.5f,-0.5f,      0.0f,0.0f, // 5
        0.5f, 0.5f,-0.5f,      0.0f,1.0f, // 6
        -0.5f, 0.5f,-0.5f,      1.0f,1.0f  // 7
    };

    quint16 m_indices[36] = {
        0,1,2,  0,2,3, // front
        5,4,7,  5,7,6, // back
        4,0,3,  4,3,7, // left
        1,5,6,  1,6,2, // right
        3,2,6,  3,6,7, // top
        4,5,1,  4,1,0  // bottom
    };
};

#endif // CUBE_H
