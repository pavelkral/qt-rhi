#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <qtypes.h>

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
#endif // GEOMETRY_H
