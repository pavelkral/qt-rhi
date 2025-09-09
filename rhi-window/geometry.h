#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QVector>
#include <qtypes.h>

 QVector<float> cubeVertices = {
     // pozice (x, y, z)      // texturovací souřadnice (u, v)
     // Přední stěna
     -0.5f, -0.5f,  0.5f,      0.0f, 0.0f, // 0: levý dolní
     0.5f, -0.5f,  0.5f,      1.0f, 0.0f, // 1: pravý dolní
     0.5f,  0.5f,  0.5f,      1.0f, 1.0f, // 2: pravý horní
     -0.5f,  0.5f,  0.5f,      0.0f, 1.0f, // 3: levý horní

     // Zadní stěna
     -0.5f, -0.5f, -0.5f,      1.0f, 0.0f, // 4: levý dolní
     0.5f, -0.5f, -0.5f,      0.0f, 0.0f, // 5: pravý dolní
     0.5f,  0.5f, -0.5f,      0.0f, 1.0f, // 6: pravý horní
     -0.5f,  0.5f, -0.5f,      1.0f, 1.0f  // 7: levý horní
};

QVector<quint16> cubeIndices = {
    // Přední stěna
    0, 1, 2,  0, 2, 3,

    // Zadní stěna
    5, 4, 7,  5, 7, 6,

    // Levá stěna
    4, 0, 3,  4, 3, 7,

    // Pravá stěna
    1, 5, 6,  1, 6, 2,

    // Horní stěna
    3, 2, 6,  3, 6, 7,

    // Spodní stěna
    4, 5, 1,  4, 1, 0
};

 QVector<float> planeVertices = {
     -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // 0
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // 1
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f, // 2
     -0.5f,  0.5f, 0.0f,  0.0f, 1.0f  // 3
 };
 QVector<quint16> planeIndices = {
     0, 1, 2,
     0, 2, 3
 };

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
 QVector<float> testVert = {
     -0.5f,-0.5f,0, 1,0,0, 0,0,
     0.5f,-0.5f,0, 1,0,0, 1,0,
     0.5f, 0.5f,0, 1,0,0, 1,1,
     -0.5f, 0.5f,0, 1,0,0, 0,1
 };
 QVector<quint16> testInd = {0,1,2,0,2,3};
#endif // GEOMETRY_H
