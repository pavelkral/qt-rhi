#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QVector>
#include <qtypes.h>
#include <QVector>
#include <cmath>
#include <qvector3d.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
void generatePlane(float width, float depth, int segX, int segZ,
                   float tileU, float tileV,
                   QVector<float>& vertices, QVector<quint16>& indices)
{
    vertices.clear();
    indices.clear();

    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;

    float stepX = width / segX;
    float stepZ = depth / segZ;

    float uvStepX = 1.0f / segX;
    float uvStepZ = 1.0f / segZ;

    // Generování vrcholů
    for (int z = 0; z <= segZ; z++) {
        for (int x = 0; x <= segX; x++) {
            float posX = -halfW + x * stepX;
            float posZ = -halfD + z * stepZ;

            // Pozice
            vertices.push_back(posX);
            vertices.push_back(0.0f);
            vertices.push_back(posZ);

            // Normála (směrem nahoru)
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);

            // UV s tilingem
            vertices.push_back(x * uvStepX * tileU);
            vertices.push_back(z * uvStepZ * tileV);
        }
    }

    // Generování indexů
    for (int z = 0; z < segZ; z++) {
        for (int x = 0; x < segX; x++) {
            quint16 v1 = z * (segX + 1) + x;
            quint16 v2 = v1 + 1;
            quint16 v3 = v1 + (segX + 1);
            quint16 v4 = v3 + 1;

            // První trojúhelník
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v3);

            // Druhý trojúhelník
            indices.push_back(v2);
            indices.push_back(v4);
            indices.push_back(v3);
        }
    }
}


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

void generateCube(float size, QVector<float>& vertices, QVector<quint16>& indices)
{
    vertices.clear();
    indices.clear();

    float half = size * 0.5f;

    // Struktura: {px,py,pz, nx,ny,nz, u,v}
    struct Face {
        QVector3D normal;
        QVector3D v[4];
        QVector2D uv[4];
    };

    // 6 stěn krychle
    std::vector<Face> faces = {
        // Front (+Z)
        { QVector3D(0,0,1),
         { {-half,-half, half}, { half,-half, half}, { half, half, half}, {-half, half, half} },
         { {0,0}, {1,0}, {1,1}, {0,1} } },

        // Back (-Z)
        { QVector3D(0,0,-1),
         { { half,-half,-half}, {-half,-half,-half}, {-half, half,-half}, { half, half,-half} },
         { {0,0}, {1,0}, {1,1}, {0,1} } },

        // Left (-X)
        { QVector3D(-1,0,0),
         { {-half,-half,-half}, {-half,-half, half}, {-half, half, half}, {-half, half,-half} },
         { {0,0}, {1,0}, {1,1}, {0,1} } },

        // Right (+X)
        { QVector3D(1,0,0),
         { { half,-half, half}, { half,-half,-half}, { half, half,-half}, { half, half, half} },
         { {0,0}, {1,0}, {1,1}, {0,1} } },

        // Top (+Y)
        { QVector3D(0,1,0),
         { {-half, half, half}, { half, half, half}, { half, half,-half}, {-half, half,-half} },
         { {0,0}, {1,0}, {1,1}, {0,1} } },

        // Bottom (-Y)
        { QVector3D(0,-1,0),
         { {-half,-half,-half}, { half,-half,-half}, { half,-half, half}, {-half,-half, half} },
         { {0,0}, {1,0}, {1,1}, {0,1} } },
        };

    quint16 indexOffset = 0;
    for (auto& face : faces) {
        // Přidání vrcholů (pozice, normála, UV)
        for (int i = 0; i < 4; i++) {
            vertices.push_back(face.v[i].x());
            vertices.push_back(face.v[i].y());
            vertices.push_back(face.v[i].z());

            vertices.push_back(face.normal.x());
            vertices.push_back(face.normal.y());
            vertices.push_back(face.normal.z());

            vertices.push_back(face.uv[i].x());
            vertices.push_back(face.uv[i].y());
        }

        // Přidání indexů (2 trojúhelníky na stěnu)
        indices.push_back(indexOffset + 0);
        indices.push_back(indexOffset + 1);
        indices.push_back(indexOffset + 2);

        indices.push_back(indexOffset + 0);
        indices.push_back(indexOffset + 2);
        indices.push_back(indexOffset + 3);

        indexOffset += 4;
    }
}

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
