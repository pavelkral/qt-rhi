#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QVector>
#include <qtypes.h>
#include <QVector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void generateSphere(float radius, int rings, int sectors,QVector<float>& vertices, QVector<quint16>& indices)
{

    vertices.clear();
    indices.clear();

    const float R = 1.0f / (float)(rings - 1);
    const float S = 1.0f / (float)(sectors - 1);

    // Generování vrcholů (pozice, normály, UV)
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            const float y = sin(-M_PI / 2 + M_PI * r * R);
            const float x = cos(2 * M_PI * s * S) * sin(M_PI * r * R);
            const float z = sin(2 * M_PI * s * S) * sin(M_PI * r * R);

            // Pozice vrcholu
            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);

            // Normála (pro kouli centrovanou v počátku je to stejné jako normalizovaná pozice)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // UV souřadnice
            vertices.push_back(s * S);
            vertices.push_back(r * R);
        }
    }

    // Generování indexů pro trojúhelníky
    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < sectors - 1; ++s) {
            // Indexy čtyř vrcholů tvořících čtyřúhelník
            quint16 v1 = r * sectors + s;
            quint16 v2 = r * sectors + (s + 1);
            quint16 v3 = (r + 1) * sectors + (s + 1);
            quint16 v4 = (r + 1) * sectors + s;

            // První trojúhelník
            indices.push_back(v1);
            indices.push_back(v3);
            indices.push_back(v4);

            // Druhý trojúhelník
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v3);
        }
    }
}
 QVector<float> cubeVertices1 = {
     // pos                 // normal       // uv
     // Front (+Z)
     -0.5f,-0.5f, 0.5f,     0,0,1,          0.0f,0.0f,
     0.5f,-0.5f, 0.5f,     0,0,1,          1.0f,0.0f,
     0.5f, 0.5f, 0.5f,     0,0,1,          1.0f,1.0f,
     -0.5f, 0.5f, 0.5f,     0,0,1,          0.0f,1.0f,

     // Back (-Z)
     -0.5f,-0.5f,-0.5f,     0,0,-1,         1.0f,0.0f,
     0.5f,-0.5f,-0.5f,     0,0,-1,         0.0f,0.0f,
     0.5f, 0.5f,-0.5f,     0,0,-1,         0.0f,1.0f,
     -0.5f, 0.5f,-0.5f,     0,0,-1,         1.0f,1.0f,

     // Left (-X)
     -0.5f,-0.5f,-0.5f,    -1,0,0,          0.0f,0.0f,
     -0.5f,-0.5f, 0.5f,    -1,0,0,          1.0f,0.0f,
     -0.5f, 0.5f, 0.5f,    -1,0,0,          1.0f,1.0f,
     -0.5f, 0.5f,-0.5f,    -1,0,0,          0.0f,1.0f,

     // Right (+X)
     0.5f,-0.5f,-0.5f,     1,0,0,          1.0f,0.0f,
     0.5f,-0.5f, 0.5f,     1,0,0,          0.0f,0.0f,
     0.5f, 0.5f, 0.5f,     1,0,0,          0.0f,1.0f,
     0.5f, 0.5f,-0.5f,     1,0,0,          1.0f,1.0f,

     // Top (+Y)
     -0.5f, 0.5f, 0.5f,     0,1,0,          0.0f,0.0f,
     0.5f, 0.5f, 0.5f,     0,1,0,          1.0f,0.0f,
     0.5f, 0.5f,-0.5f,     0,1,0,          1.0f,1.0f,
     -0.5f, 0.5f,-0.5f,     0,1,0,          0.0f,1.0f,

     // Bottom (-Y)
     -0.5f,-0.5f, 0.5f,     0,-1,0,         0.0f,1.0f,
     0.5f,-0.5f, 0.5f,     0,-1,0,         1.0f,1.0f,
     0.5f,-0.5f,-0.5f,     0,-1,0,         1.0f,0.0f,
     -0.5f,-0.5f,-0.5f,     0,-1,0,         0.0f,0.0f,
 };

 QVector<quint16> cubeIndices1 = {
     0,1,2,  0,2,3,       // front
     4,5,6,  4,6,7,       // back
     8,9,10, 8,10,11,     // left
     12,13,14, 12,14,15,  // right
     16,17,18, 16,18,19,  // top
     20,21,22, 20,22,23   // bottom
 };
 QVector<float> planeVertices1 = {
     // pos               // normal         // uv
     -0.5f,-0.5f, 0.0f,   0,0,1,            0.0f,0.0f, // 0
     0.5f,-0.5f, 0.0f,   0,0,1,            1.0f,0.0f, // 1
     0.5f, 0.5f, 0.0f,   0,0,1,            1.0f,1.0f, // 2
     -0.5f, 0.5f, 0.0f,   0,0,1,            0.0f,1.0f  // 3
 };

 QVector<quint16> planeIndices1 = {
     0,1,2,
     0,2,3
 };

 QVector<float> indexedPlaneVertices = {
     // Pos                  // Normal            // UV
     15.0f, -0.5f, 15.0f,    0.0f, 1.0f, 0.0f,   10.0f, 0.0f,
     15.0f, -0.5f, -15.0f,   0.0f, 1.0f, 0.0f,   10.0f, 10.0f,
     -15.0f, -0.5f, -15.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
     -15.0f, -0.5f, 15.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f
 };
 QVector<quint16> indexedPlaneIndices = {0, 1, 2, 0, 2, 3};


#endif // GEOMETRY_H
