#ifndef ASSIMPUTILS_H
#define ASSIMPUTILS_H
#include <assimp/matrix4x4.h>
#include <QMatrix4x4>
#include <logger.h>
#include <assimp/scene.h>



namespace utils
{
class Mesh;
class QRhiTexture;

inline QMatrix4x4 to_qmatrix4x4(aiMatrix4x4 aim)
{
    return {
        aim.a1, aim.a2, aim.a3, aim.a4, aim.b1, aim.b2, aim.b3, aim.b4,
        aim.c1, aim.c2, aim.c3, aim.c4, aim.d1, aim.d2, aim.d3, aim.d4,
    };
}

 static inline void printFullModelInfo(const aiScene* scene)
{
    if (!scene) {
        qDebug() << "Scene is null!";
        return;
    }

    Logger::instance().log("===============================", Qt::magenta);
    Logger::instance().log("Asset Info", Qt::magenta);
    Logger::instance().log("===============================", Qt::magenta);

    // --- Mesh info ---
    qInfo() << "Number of meshes:" << scene->mNumMeshes;
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        qInfo() << "--------------------------------------------------";
        qDebug() << "Mesh" << i << "Name:" << mesh->mName.C_Str();
        qDebug() << "  Vertices:" << mesh->mNumVertices;
        qDebug() << "  Faces:" << mesh->mNumFaces;
        qDebug() << "  Has normals:" << (mesh->HasNormals() ? "Yes" : "No");
        qDebug() << "  Has tangents/bitangents:" << (mesh->HasTangentsAndBitangents() ? "Yes" : "No");
        qDebug() << "  Has texture coords:" << (mesh->HasTextureCoords(0) ? "Yes" : "No");
        qDebug() << "  Has vertex colors:" << (mesh->HasVertexColors(0) ? "Yes" : "No");

        // --- Material info ---
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            aiString matName;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_NAME, matName))
                qDebug() << "  Material Name:" << matName.C_Str();

            aiColor3D color;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, color))
                qDebug() << "    Diffuse:" << color.r << color.g << color.b;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_SPECULAR, color))
                qDebug() << "    Specular:" << color.r << color.g << color.b;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_AMBIENT, color))
                qDebug() << "    Ambient:" << color.r << color.g << color.b;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_EMISSIVE, color))
                qDebug() << "    Emissive:" << color.r << color.g << color.b;

            float fval;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_SHININESS, fval)) qDebug() << "    Shininess:" << fval;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_OPACITY, fval)) qDebug() << "    Opacity:" << fval;

            // --- Textures ---
            auto printTex = [&](aiTextureType type, const char* label) {
                int count = mat->GetTextureCount(type);
                for (int t = 0; t < count; ++t) {
                    aiString path;
                    mat->GetTexture(type, t, &path);
                    qDebug() << "    Texture type" << label << ":" << path.C_Str();
                }
            };

            qDebug() << "Textures:" ;
            printTex(aiTextureType_DIFFUSE, "DIFFUSE");
            printTex(aiTextureType_SPECULAR, "SPECULAR");
            printTex(aiTextureType_NORMALS, "NORMALS");
            printTex(aiTextureType_HEIGHT, "HEIGHT");
            printTex(aiTextureType_EMISSIVE, "EMISSIVE");
            printTex(aiTextureType_OPACITY, "OPACITY");

            // Assimp 6 PBR textury – fallback přes UNKNOWN
            int unknownCount = mat->GetTextureCount(aiTextureType_UNKNOWN);
            for (int t = 0; t < unknownCount; ++t) {
                aiString path;
                mat->GetTexture(aiTextureType_UNKNOWN, t, &path);
                qDebug() << "    PBR / UNKNOWN texture:" << path.C_Str();
            }

            // Pokud je glTF PBR, lze načíst speciální klíče
            // aiString baseColorTex, metallicRoughnessTex;
            // if (AI_SUCCESS == mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, &baseColorTex))
            //     qDebug() << "    BaseColor texture:" << baseColorTex.C_Str();
            // if (AI_SUCCESS == mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_ROUGHNESS_TEXTURE, &metallicRoughnessTex))
            //     qDebug() << "    Metallic-Roughness texture:" << metallicRoughnessTex.C_Str();
        }
      //  qInfo() << "--------------------------------------------------";
    }

    // --- Animation info ---
    if (!scene->HasAnimations()) {
        qInfo() << "No animations in model.";
    } else {
        qInfo()  << "Number of animations:" << scene->mNumAnimations;
        for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
            const aiAnimation* anim = scene->mAnimations[a];
            double durationSec = anim->mDuration / (anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0);
            qDebug() << "--------------------------------------------------";
            qDebug() << "Animation" << a << "Name:" << anim->mName.C_Str();
            qDebug() << "  Duration (ticks):" << anim->mDuration;
            qDebug() << "  Ticks per second:" << anim->mTicksPerSecond;
            qDebug() << "  Duration (seconds):" << durationSec;
            qDebug() << "  Number of channels (bones):" << anim->mNumChannels;

            // for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
            //     const aiNodeAnim* channel = anim->mChannels[c];
            //     qDebug() << "    Channel" << c << "Node:" << channel->mNodeName.C_Str();
            //     qDebug() << "      Position keys:" << channel->mNumPositionKeys;
            //     qDebug() << "      Rotation keys:" << channel->mNumRotationKeys;
            //     qDebug() << "      Scaling keys:" << channel->mNumScalingKeys;
            // }
        }
    }

    Logger::instance().log("===============================", Qt::magenta);
    Logger::instance().log("End Info", Qt::magenta);
    Logger::instance().log("===============================", Qt::magenta);
}


 static inline void printMeshInfo(const aiMesh* mesh, const aiMaterial* material)
{
    Logger::instance().log("===============================", Qt::magenta);
    Logger::instance().log("FBX Mesh Info", Qt::magenta);
    Logger::instance().log("===============================", Qt::magenta);
    qDebug() << "Mesh name:" << mesh->mName.C_Str();
    qDebug() << "Vertices:" << mesh->mNumVertices;
    qDebug() << "Faces:" << mesh->mNumFaces;

    // --- Vertex info ---
    qDebug() << "Has normals:" << (mesh->HasNormals() ? "Yes" : "No");
    qDebug() << "Has tangents/bitangents:" << (mesh->HasTangentsAndBitangents() ? "Yes" : "No");
    qDebug() << "Has texture coords:" << (mesh->HasTextureCoords(0) ? "Yes" : "No");
    qDebug() << "Has vertex colors:" << (mesh->HasVertexColors(0) ? "Yes" : "No");

    // --- Material info ---
    if (material) {
        aiString name;
        if (AI_SUCCESS == material->Get(AI_MATKEY_NAME, name))
            qDebug() << "Material name:" << name.C_Str();

        aiColor3D color;
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
            qDebug() << "Diffuse color:" << color.r << color.g << color.b;
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, color))
            qDebug() << "Specular color:" << color.r << color.g << color.b;
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, color))
            qDebug() << "Ambient color:" << color.r << color.g << color.b;
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, color))
            qDebug() << "Emissive color:" << color.r << color.g << color.b;

        float shininess = 0.0f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, shininess))
            qDebug() << "Shininess:" << shininess;

        float opacity = 1.0f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_OPACITY, opacity))
            qDebug() << "Opacity:" << opacity;

        float metal = 0.0f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, metal))
            qDebug() << "Metallic factor:" << metal;

        float rough = 0.0f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, rough))
            qDebug() << "Roughness factor:" << rough;

        // --- Textures ---
        auto printTex = [&](aiTextureType type, const char* label) {
            int count = material->GetTextureCount(type);
            if (count > 0) {
                qDebug() << "Texture type" << label << ":" << count;
                for (int i = 0; i < count; ++i) {
                    aiString path;
                    material->GetTexture(type, i, &path);
                    qDebug() << "   -" << path.C_Str();
                }
            }
        };
        printTex(aiTextureType_DIFFUSE, "DIFFUSE");
        printTex(aiTextureType_SPECULAR, "SPECULAR");
        printTex(aiTextureType_NORMALS, "NORMALS");
        printTex(aiTextureType_HEIGHT, "HEIGHT");
        printTex(aiTextureType_EMISSIVE, "EMISSIVE");
        printTex(aiTextureType_OPACITY, "OPACITY");
        printTex(aiTextureType_METALNESS, "METALNESS");
        //  printTex(aiTextureType_ROUGHNESS, "ROUGHNESS");
        printTex(aiTextureType_AMBIENT_OCCLUSION, "AO");
    }
    qDebug() << "--------------------------------------------------";
}
 static inline void printAnimationsInfo(const aiScene* scene)
{
    if (!scene->HasAnimations()) {
        qDebug() << "Model has no animations.";
        return;
    }

    Logger::instance().log("===============================", Qt::magenta);
    Logger::instance().log("Animation Info", Qt::magenta);
    Logger::instance().log("===============================", Qt::magenta);
    qDebug() << "Number of animations:" << scene->mNumAnimations;

    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        const aiAnimation* anim = scene->mAnimations[i];
        double durationSec = anim->mDuration / (anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0);
        qDebug() << "Animation" << i;
        qDebug() << "  Name:" << anim->mName.C_Str();
        qDebug() << "  Duration (ticks):" << anim->mDuration;
        qDebug() << "  Ticks per second:" << anim->mTicksPerSecond;
        qDebug() << "  Duration (seconds):" << durationSec;
        qDebug() << "  Number of channels (bones):" << anim->mNumChannels;

        for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
            const aiNodeAnim* channel = anim->mChannels[c];
            qDebug() << "    Channel:" << c << "Node:" << channel->mNodeName.C_Str();
            qDebug() << "      Position keys:" << channel->mNumPositionKeys;
            qDebug() << "      Rotation keys:" << channel->mNumRotationKeys;
            qDebug() << "      Scaling keys:" << channel->mNumScalingKeys;
        }
    }
    qDebug() << "=================================================";
}
} // namespace utils
#endif // ASSIMPUTILS_H
