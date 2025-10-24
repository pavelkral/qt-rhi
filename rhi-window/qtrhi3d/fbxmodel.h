#ifndef FBXMODEL_H
#define FBXMODEL_H

#include <qdir.h>
#include <rhi/qrhi.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/version.h>
#include <assimp/postprocess.h>
#include <QString>
#include <QFileInfo>
#include <QImage>
#include <QDebug>
#include <vector>
#include <map>
#include <memory>
#include <array>
#include "assimputils.h"

struct MVertex {
    QVector3D position{};
    QVector3D normal{};
    QVector2D coords{};
};

struct Material {
    aiTextureType type{};
    std::shared_ptr<QRhiTexture> texture{};
    std::string path{};
    Material() = default;
    Material(aiTextureType t, std::shared_ptr<QRhiTexture> tex, std::string p)
        : type(t), texture(tex), path(std::move(p)) {}
};

class Mesh {


public:
    std::vector<MVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Material> materials;

private:
    QMatrix4x4 transform_;
    QRhi *rhi_{nullptr};
    std::unique_ptr<QRhiGraphicsPipeline> pipeline_;
    std::unique_ptr<QRhiBuffer> vbuf_;
    std::unique_ptr<QRhiBuffer> ibuf_;
    std::unique_ptr<QRhiBuffer> ubuf_;
    std::unique_ptr<QRhiSampler> sampler_;
    std::unique_ptr<QRhiShaderResourceBindings> srb_;
    bool uploaded_{false};

public:

    Mesh(std::vector<MVertex> verts, std::vector<uint32_t> inds, std::vector<Material> mats, const QMatrix4x4& t)
        : vertices(std::move(verts)), indices(std::move(inds)), materials(std::move(mats)), transform_(t) {


        //qDebug() << "Assimp runtime:"<< aiGetVersionMajor() << "."<< aiGetVersionMinor() << "."<< aiGetVersionRevision();


    }

    void create(QRhi *rhi, QRhiRenderTarget *rt,QRhiRenderPassDescriptor *rp) {
        if (rhi_ != rhi) pipeline_.reset(), rhi_ = rhi;
        if (!pipeline_) {
            vbuf_.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                       static_cast<quint32>(vertices.size() * sizeof(MVertex))));
            vbuf_->create();
            ibuf_.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer,
                                       static_cast<quint32>(indices.size() * sizeof(uint32_t))));
            ibuf_->create();
            ubuf_.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
            ubuf_->create();

            sampler_.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                           QRhiSampler::Repeat, QRhiSampler::Repeat));
            sampler_->create();

            std::vector<QRhiShaderResourceBinding> bindings;
            bindings.emplace_back(QRhiShaderResourceBinding::uniformBuffer(
                0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf_.get()));

            int binding_idx = 1;
            for (auto& mat : materials) {
                if (mat.texture)
                    bindings.emplace_back(QRhiShaderResourceBinding::sampledTexture(
                        binding_idx++, QRhiShaderResourceBinding::FragmentStage, mat.texture.get(), sampler_.get()));
            }

            srb_.reset(rhi->newShaderResourceBindings());
            srb_->setBindings(bindings.begin(), bindings.end());
            srb_->create();

            pipeline_.reset(rhi->newGraphicsPipeline());
            pipeline_->setTopology(QRhiGraphicsPipeline::Triangles);
            pipeline_->setShaderStages({
                { QRhiShaderStage::Vertex, LoadShader(":/shaders/prebuild/model.vert.qsb") },
                { QRhiShaderStage::Fragment, LoadShader(":/shaders/prebuild/model.frag.qsb") }
            });
            QRhiVertexInputLayout layout{};
            layout.setBindings({ 8 * sizeof(float) });
            layout.setAttributes({
                {0, 0, QRhiVertexInputAttribute::Float3, 0},
                {0, 1, QRhiVertexInputAttribute::Float3, 3*sizeof(float)},
                {0, 2, QRhiVertexInputAttribute::Float2, 6*sizeof(float)}
            });
            pipeline_->setVertexInputLayout(layout);
            pipeline_->setShaderResourceBindings(srb_.get());
            pipeline_->setRenderPassDescriptor(rp);
            pipeline_->setDepthTest(true);
            pipeline_->setDepthWrite(true);
            pipeline_->setCullMode(QRhiGraphicsPipeline::Back);
            pipeline_->create();
        }
    }

    void updateUbo(QRhiResourceUpdateBatch *rub, const QMatrix4x4& mvp) {
        if (!uploaded_) {
            rub->uploadStaticBuffer(vbuf_.get(), vertices.data());
            rub->uploadStaticBuffer(ibuf_.get(), indices.data());
            uploaded_ = true;
        }
        rub->updateDynamicBuffer(ubuf_.get(), 0, 64, (mvp * transform_).constData());
    }

    void draw(QRhiCommandBuffer *cb, const QRhiViewport& viewport) {
        cb->setGraphicsPipeline(pipeline_.get());
        cb->setViewport(viewport);
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput input{vbuf_.get(), 0};
        cb->setVertexInput(0, 1, &input, ibuf_.get(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(static_cast<quint32>(indices.size()));
    }


    static inline QShader LoadShader(const QString& name) {
        QFile f(name);
        return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
    }
};

//==========================================================================================================================

class FbxModel {


private:
    std::string dir_;
    bool created_{false};
    bool uploaded_{false};
    std::vector<std::unique_ptr<Mesh>> meshes_;
    std::map<std::string, std::shared_ptr<QRhiTexture>> textures_;
public:
    explicit FbxModel(const QString& path) { load(path); }

    bool load(const QString& resource) {
        dir_ = QFileInfo(resource).dir().absolutePath().toStdString();
        Assimp::Importer importer;
        const auto scene = importer.ReadFile(resource.toStdString(),
                                             aiProcess_Triangulate |
                                                 aiProcess_GenSmoothNormals |
                                                 aiProcess_FlipUVs |
                                                 aiProcess_CalcTangentSpace |
                                                 aiProcess_GenUVCoords);
        if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            qDebug() << "Assimp error:" << importer.GetErrorString();
            return false;
        }
        else {
           // printAnimationsInfo(scene);
            printFullModelInfo(scene);
        }
        meshes_.clear();
        textures_.clear();
        load_node(scene, scene->mRootNode, {});
        created_ = false;
        uploaded_ = false;
        return true;
    }

    void load_node(const aiScene *scene, const aiNode *node, const QMatrix4x4& accumulated) {
        QMatrix4x4 transform = accumulated * utils::to_qmatrix4x4(node->mTransformation);
        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
            load_mesh(scene, scene->mMeshes[node->mMeshes[i]], transform);
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
            load_node(scene, node->mChildren[i], transform);
    }

    void load_mesh(const aiScene *scene, const aiMesh *mesh, const QMatrix4x4& transform) {
        std::vector<MVertex> vertices(mesh->mNumVertices);
        for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
            vertices[i].position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
            if (mesh->HasNormals()) vertices[i].normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
            if (mesh->mTextureCoords[0]) vertices[i].coords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }

        std::vector<uint32_t> indices;
        for (unsigned i = 0; i < mesh->mNumFaces; ++i)
            for (unsigned j = 0; j < mesh->mFaces[i].mNumIndices; ++j)
                indices.push_back(mesh->mFaces[i].mIndices[j]);

        std::vector<Material> materials;
        const auto aimat = scene->mMaterials[mesh->mMaterialIndex];
        for (int t = 0; t < aimat->GetTextureCount(aiTextureType_DIFFUSE); ++t) {
            aiString path;
            aimat->GetTexture(aiTextureType_DIFFUSE, t, &path);
            std::string full = dir_ + "/" + std::string(path.data, path.length);
            materials.emplace_back(aiTextureType_DIFFUSE, nullptr, full);
            textures_[full].reset();
        }
       // printMeshInfo(mesh, scene->mMaterials[mesh->mMaterialIndex]);
        meshes_.emplace_back(std::make_unique<Mesh>(vertices, indices, materials, transform));
    }

    void create(QRhi *rhi, QRhiRenderTarget *rt,QRhiRenderPassDescriptor *rp) {
        if (!created_) {
            for (auto& [path, tex] : textures_) {
                QImage img;
                if (!img.load(QString::fromStdString(path))) continue;
                tex.reset(rhi->newTexture(QRhiTexture::RGBA8, img.size()));
                tex->create();
            }
            for (auto& mesh : meshes_)
                for (auto& mat : mesh->materials)
                    mat.texture = textures_[mat.path];
            created_ = true;
        }
        for (auto& mesh : meshes_) mesh->create(rhi, rt,rp);
    }

    void updateUbo(QRhiResourceUpdateBatch *rub, const QMatrix4x4& mvp) {
        if (!uploaded_) {
            for (auto& [path, tex] : textures_) {
                QImage img;
                if (img.load(QString::fromStdString(path))) rub->uploadTexture(tex.get(), img);
            }
            uploaded_ = true;
        }
        for (auto& mesh : meshes_) mesh->updateUbo(rub, mvp);
    }

    void draw(QRhiCommandBuffer *cb, const QRhiViewport& vp) {
        for (auto& mesh : meshes_) mesh->draw(cb, vp);
    }

     //=============================================================================================================================
    //=================================================================Debug========================================================

    void modelInfo() const {
        qDebug() << "===============================";
        qDebug() << " FBX Model Info";
        qDebug() << "===============================";
        qDebug() << "Meshes:" << meshes_.size();
        qDebug() << "Textures:" << textures_.size();
        qDebug() << "";

        int meshIndex = 0;
        for (const auto& mesh : meshes_) {
            qDebug() << "Mesh" << meshIndex++;
            qDebug() << "  Vertices:" << mesh->vertices.size();
            qDebug() << "  Indices:" << mesh->indices.size();
            qDebug() << "  Materials:" << mesh->materials.size();
            for (const auto& mat : mesh->materials) {
                QString typeName;
                switch (mat.type) {
                case aiTextureType_DIFFUSE: typeName = "DIFFUSE"; break;
                case aiTextureType_SPECULAR: typeName = "SPECULAR"; break;
                case aiTextureType_NORMALS: typeName = "NORMAL"; break;
                default: typeName = "OTHER"; break;
                }
                qDebug() << "    Material type:" << typeName;
                qDebug() << "    Texture path:" << QString::fromStdString(mat.path);
                qDebug() << "    Has QRhiTexture:" << (mat.texture ? "Yes" : "No");
            }
        }

        qDebug() << "";
        qDebug() << "Texture Map:";
        for (const auto& [path, tex] : textures_) {
            qDebug() << " -" << QString::fromStdString(path)
            << (tex ? "(created)" : "(not created)");
        }

        qDebug() << "===============================";
    }
    static void printMeshInfo(const aiMesh* mesh, const aiMaterial* material)
    {
        qDebug() << "--------------------------------------------------";
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
    static void printAnimationsInfo(const aiScene* scene)
    {
        if (!scene->HasAnimations()) {
            qDebug() << "Model has no animations.";
            return;
        }

        qDebug() << "================ Animations Info ================";
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
    static void printFullModelInfo(const aiScene* scene)
    {
        if (!scene) {
            qDebug() << "Scene is null!";
            return;
        }

        qDebug() << "================ FULL MODEL INFO ================";

        // --- Mesh info ---
        qDebug() << "Number of meshes:" << scene->mNumMeshes;
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[i];
            qDebug() << "--------------------------------------------------";
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
        }

        // --- Animation info ---
        if (!scene->HasAnimations()) {
            qDebug() << "No animations in model.";
        } else {
            qDebug() << "Number of animations:" << scene->mNumAnimations;
            for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
                const aiAnimation* anim = scene->mAnimations[a];
                double durationSec = anim->mDuration / (anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0);
                qDebug() << "--------------------------------------------------";
                qDebug() << "Animation" << a << "Name:" << anim->mName.C_Str();
                qDebug() << "  Duration (ticks):" << anim->mDuration;
                qDebug() << "  Ticks per second:" << anim->mTicksPerSecond;
                qDebug() << "  Duration (seconds):" << durationSec;
                qDebug() << "  Number of channels (bones):" << anim->mNumChannels;

                for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
                    const aiNodeAnim* channel = anim->mChannels[c];
                    qDebug() << "    Channel" << c << "Node:" << channel->mNodeName.C_Str();
                    qDebug() << "      Position keys:" << channel->mNumPositionKeys;
                    qDebug() << "      Rotation keys:" << channel->mNumRotationKeys;
                    qDebug() << "      Scaling keys:" << channel->mNumScalingKeys;
                }
            }
        }

        qDebug() << "=================================================";
    }

};

#endif // FBXMODEL_H
