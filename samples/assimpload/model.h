#ifndef RHI_MODEL_H
#define RHI_MODEL_H

#include <qdir.h>
#include <rhi/qrhi.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <QString>
#include <QFileInfo>
#include <QImage>
#include <QDebug>
#include <vector>
#include <map>
#include <memory>
#include <array>
#include "utils.h"

struct Vertex {
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

    Mesh(std::vector<Vertex> verts, std::vector<uint32_t> inds, std::vector<Material> mats, const QMatrix4x4& t)
        : vertices(std::move(verts)), indices(std::move(inds)), materials(std::move(mats)), transform_(t) {}

    void create(QRhi *rhi, QRhiRenderTarget *rt) {
        if (rhi_ != rhi) pipeline_.reset(), rhi_ = rhi;
        if (!pipeline_) {
            vbuf_.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                       static_cast<quint32>(vertices.size() * sizeof(Vertex))));
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
                { QRhiShaderStage::Vertex, LoadShader(":/vertex.vert.qsb") },
                { QRhiShaderStage::Fragment, LoadShader(":/fragment.frag.qsb") }
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
            pipeline_->setRenderPassDescriptor(rt->renderPassDescriptor());
            pipeline_->setDepthTest(true);
            pipeline_->setDepthWrite(true);
            pipeline_->setCullMode(QRhiGraphicsPipeline::Back);
            pipeline_->create();
        }
    }

    void upload(QRhiResourceUpdateBatch *rub, const QMatrix4x4& mvp) {
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

public:
    std::vector<Vertex> vertices;
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

    static inline QShader LoadShader(const QString& name) {
        QFile f(name);
        return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
    }
};

//==========================================================================================================================

class Model {
public:
    explicit Model(const QString& path) { load(path); }

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
        std::vector<Vertex> vertices(mesh->mNumVertices);
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

        meshes_.emplace_back(std::make_unique<Mesh>(vertices, indices, materials, transform));
    }

    void create(QRhi *rhi, QRhiRenderTarget *rt) {
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
        for (auto& mesh : meshes_) mesh->create(rhi, rt);
    }

    void upload(QRhiResourceUpdateBatch *rub, const QMatrix4x4& mvp) {
        if (!uploaded_) {
            for (auto& [path, tex] : textures_) {
                QImage img;
                if (img.load(QString::fromStdString(path))) rub->uploadTexture(tex.get(), img);
            }
            uploaded_ = true;
        }
        for (auto& mesh : meshes_) mesh->upload(rub, mvp);
    }

    void draw(QRhiCommandBuffer *cb, const QRhiViewport& vp) {
        for (auto& mesh : meshes_) mesh->draw(cb, vp);
    }

private:
    std::string dir_;
    bool created_{false};
    bool uploaded_{false};
    std::vector<std::unique_ptr<Mesh>> meshes_;
    std::map<std::string, std::shared_ptr<QRhiTexture>> textures_;
};

#endif // RHI_MODEL_H

