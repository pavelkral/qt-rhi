#ifndef TYPES_H
#define TYPES_H
#include <QMatrix4x4>
#include <QMatrix4x4>
#include <QVector4D>

struct Ubo {
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    QMatrix4x4 lightSpace;
    QVector4D lightPos;
    QVector4D lightColor;
    QVector4D camPos;
    QVector4D opacity;
    QVector4D misc;
};

struct Vertex {
    QVector3D pos;
    QVector3D normal;
    QVector2D uv;
    QVector3D tangent;
    QVector3D bitangent;
};


struct TextureSet {
    QString albedo;
    QString normal;
    QString metallic;
    QString rougness;
    QString height;
    QString ao;
};

struct alignas(16) GpuUbo {
    float model[16];       // offset 0   (64 B)
    float view[16];        // offset 64  (64 B)
    float projection[16];  // offset 128 (64 B)
    float lightSpace[16];  // offset 192 (64 B)

    float lightPos[4];     // offset 256 (16 B)
    float color[4];        // offset 272 (16 B)
    float camPos[4];       // offset 288 (16 B)
    float opacity[4];      // offset 304 (16 B)
    float misc[4];
};
struct alignas(16) GpuShadowUbo {
    float model[16];       // offset 0   (64 B)
    float view[16];        // offset 64  (64 B)
    float projection[16];  // offset 128 (64 B)
    float lightSpace[16];  // offset 192 (64 B)
};

#endif // TYPES_H
