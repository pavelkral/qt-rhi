// transform.h
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <QVector3D>
#include <QMatrix4x4>

class Transform
{
public:
    QVector3D position;
    QVector3D rotation; // Eulerovy úhly ve stupních (degrees)
    QVector3D scale;

    // Konstruktor s výchozími hodnotami
    Transform(const QVector3D& pos = QVector3D(0.0f, 0.0f, 0.0f),
              const QVector3D& rot = QVector3D(0.0f, 0.0f, 0.0f),
              const QVector3D& s = QVector3D(1.0f, 1.0f, 1.0f))
        : position(pos), rotation(rot), scale(s)
    {
    }

    // Vypočítá a vrátí modelovou matici
    QMatrix4x4 getModelMatrix() const
    {
        QMatrix4x4 modelMatrix; // Vytvoří se jako jednotková matice (identity matrix)

        // Aplikuje transformace v pořadí: posun -> rotace -> změna měřítka
        modelMatrix.translate(position);
        // Rotace kolem jednotlivých os. QMatrix4x4::rotate() očekává úhly ve stupních.
        modelMatrix.rotate(rotation.x(), 1.0f, 0.0f, 0.0f); // Kolem osy X
        modelMatrix.rotate(rotation.y(), 0.0f, 1.0f, 0.0f); // Kolem osy Y
        modelMatrix.rotate(rotation.z(), 0.0f, 0.0f, 1.0f); // Kolem osy Z
        modelMatrix.scale(scale);

        return modelMatrix;
    }
};

#endif // TRANSFORM_H
