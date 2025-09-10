// transform.h
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <QVector3D>
#include <QMatrix4x4>

class Transform
{
public:
    QVector3D position;
    QVector3D rotation; // Euler
    QVector3D scale;

    Transform(const QVector3D& pos = QVector3D(0.0f, 0.0f, 0.0f),
              const QVector3D& rot = QVector3D(0.0f, 0.0f, 0.0f),
              const QVector3D& s = QVector3D(1.0f, 1.0f, 1.0f))
        : position(pos), rotation(rot), scale(s)
    {
    }

    QMatrix4x4 getModelMatrix() const
    {
        QMatrix4x4 modelMatrix;
        modelMatrix.translate(position);
        modelMatrix.rotate(rotation.x(), 1.0f, 0.0f, 0.0f); //  X
        modelMatrix.rotate(rotation.y(), 0.0f, 1.0f, 0.0f); //  Y
        modelMatrix.rotate(rotation.z(), 0.0f, 0.0f, 1.0f); //  Z
        modelMatrix.scale(scale);

        return modelMatrix;
    }
};

#endif // TRANSFORM_H
