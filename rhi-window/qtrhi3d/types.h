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
    float debugMode;
    float lightIntensity;

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
#endif // TYPES_H
