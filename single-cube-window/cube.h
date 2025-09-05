#ifndef CUBE_H
#define CUBE_H
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
              QRhiResourceUpdateBatch *u);

    void setModelMatrix(const QMatrix4x4 &mvp, QRhiResourceUpdateBatch *u);
    void draw(QRhiCommandBuffer *cb, QRhiGraphicsPipeline *pipeline);

private:
    // Sdílené mezi všemi krychlemi
    static std::unique_ptr<QRhiBuffer> s_vbuf;
    static std::unique_ptr<QRhiBuffer> s_ibuf;
    static int s_indexCount;
    static bool s_inited;

    // Per-instance
    std::unique_ptr<QRhiBuffer> m_ubuf; // uniform buffer s MVP
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;

    QMatrix4x4 m_modelMatrix;

    static void ensureGeometry(QRhi *rhi, QRhiResourceUpdateBatch *u);

    // Vertex & index data
    static constexpr float vertices[] = {
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

    static constexpr quint16 indices[] = {
        0,1,2,  0,2,3, // front
        5,4,7,  5,7,6, // back
        4,0,3,  4,3,7, // left
        1,5,6,  1,6,2, // right
        3,2,6,  3,6,7, // top
        4,5,1,  4,1,0  // bottom
    };
};

#endif // CUBE_H
