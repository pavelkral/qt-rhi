#ifndef CUBE_H
#define CUBE_H
#pragma once
#include <rhi/qrhi.h>
#include <memory>

class Cube {
public:
    void init(QRhi *rhi, QRhiResourceUpdateBatch *u);
    void draw(QRhiCommandBuffer *cb);

private:
    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ibuf;
    int m_indexCount = 0;

    // 8 unikátních vrcholů
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

    // 36 indexů (6 stran × 2 trojúhelníky × 3 vrcholy)
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
