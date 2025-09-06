#ifndef RHIFBXMODEL_H
#define RHIFBXMODEL_H



#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <QRhiWidget>
#include <rhi/qrhi.h>

#include <QMatrix4x4>
#include <QImage>
#include <QVector3D>
#include <QVector2D>
#include <vector>
#include <memory>

#include <string>
#include <cstdint>
#include <iostream>
#include <filesystem>


struct UBufData {
    QMatrix4x4 uModel;
    QMatrix4x4 uView;
    QMatrix4x4 uProj;
    QVector3D uLightPos;
    float uAmbientStrength;
    QVector3D uLightColor;
    float uMetallicFactor;
    QVector3D uCameraPos;
    float uSmoothnessFactor;
    QVector3D uAlbedoColor;
    float pad0;
    int uHasAlbedo;
    int uHasNormal;
    int uHasMetallic;
    int uHasSmoothness;
};

struct RhiMesh {
    // CPU-side data (filled during loadModel)
    std::vector<float> cpuVertices;      // interleaved like: pos(3), normal(3), uv(2), tangent(3), bitangent(3)
    std::vector<uint32_t> cpuIndices;

    QImage cpuAlbedoImg;
    QImage cpuNormalImg;
    QImage cpuMetallicImg;
    QImage cpuSmoothnessImg;

    // GPU resources (created in createResources, uploaded lazily in draw)
    std::unique_ptr<QRhiBuffer> vbuf;
    std::unique_ptr<QRhiBuffer> ibuf;
    std::unique_ptr<QRhiTexture> texAlbedo;
    std::unique_ptr<QRhiTexture> texNormal;
    std::unique_ptr<QRhiTexture> texMetallic;
    std::unique_ptr<QRhiTexture> texSmoothness;

    std::unique_ptr<QRhiShaderResourceBindings> srb;

    bool hasAlbedo = false;
    bool hasNormal = false;
    bool hasMetallic = false;
    bool hasSmoothness = false;

    bool uploaded = false; // were CPU->GPU uploads performed?
    int indexCount = 0;
};

class RhiFBXModel {
public:
    RhiFBXModel(QRhi* rhi, const std::string& path)
        : m_rhi(rhi), m_path(path)
    {
        loadModel(path);
    }

    ~RhiFBXModel() = default;

    void setFallbackAlbedo(const QVector3D& c) { m_fallbackAlbedo = c; }
    void setFallbackMetallic(float v) { m_fallbackMetallic = v; }
    void setFallbackSmoothness(float v) { m_fallbackSmoothness = v; }

    // Create GPU-side objects (buffers/textures) but DO NOT upload data yet.
    // Call this once you have a valid QRhi (e.g., in widget::initialize()).
    void createResources(QRhiSampler* sampler)
    {
        m_sampler = sampler;

        // create UBO
        m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UBufData)));
        m_ubuf->create();

        // create a tiny 1x1 dummy texture for missing slots
        QImage dummy(1,1, QImage::Format_RGBA8888);
        dummy.fill(Qt::white);
        m_dummyTex.reset(m_rhi->newTexture(QRhiTexture::RGBA8, dummy.size(), 1));
        m_dummyTex->create();
        {
            auto rub = m_rhi->nextResourceUpdateBatch();
            rub->uploadTexture(m_dummyTex.get(), dummy);
            // don't call m_rhi->submitResourceUpdate(...) — we'll upload in draw with cb
            // store the batch so caller can flush it if desired, but to keep API simple,
            // we'll perform this upload lazily in draw as well (below).
            m_pendingInitialUploads.emplace_back(std::move(rub));
        }

        // For each mesh: create empty buffers & textures (textures created if image exists; otherwise will use dummy in SRB)
        for (auto& mesh : m_meshes) {
            // create buffers (no data uploaded)
            mesh.vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                             mesh.cpuVertices.size() * sizeof(float)));
            mesh.ibuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer,
                                             mesh.cpuIndices.size() * sizeof(uint32_t)));
            mesh.vbuf->create();
            mesh.ibuf->create();

            // create textures if we have images, else leave null (SRB will reference dummy)
            if (!mesh.cpuAlbedoImg.isNull()) {
                QImage img = mesh.cpuAlbedoImg.convertToFormat(QImage::Format_RGBA8888);
                mesh.texAlbedo.reset(m_rhi->newTexture(QRhiTexture::RGBA8, img.size(), 1));
                mesh.texAlbedo->create();
                mesh.hasAlbedo = true;
            }
            if (!mesh.cpuNormalImg.isNull()) {
                QImage img = mesh.cpuNormalImg.convertToFormat(QImage::Format_RGBA8888);
                mesh.texNormal.reset(m_rhi->newTexture(QRhiTexture::RGBA8, img.size(), 1));
                mesh.texNormal->create();
                mesh.hasNormal = true;
            }
            if (!mesh.cpuMetallicImg.isNull()) {
                QImage img = mesh.cpuMetallicImg.convertToFormat(QImage::Format_RGBA8888);
                mesh.texMetallic.reset(m_rhi->newTexture(QRhiTexture::RGBA8, img.size(), 1));
                mesh.texMetallic->create();
                mesh.hasMetallic = true;
            }
            if (!mesh.cpuSmoothnessImg.isNull()) {
                QImage img = mesh.cpuSmoothnessImg.convertToFormat(QImage::Format_RGBA8888);
                mesh.texSmoothness.reset(m_rhi->newTexture(QRhiTexture::RGBA8, img.size(), 1));
                mesh.texSmoothness->create();
                mesh.hasSmoothness = true;
            }

            // create SRB: reference actual texture if present, otherwise fallback dummy
            QRhiTexture* albedoRef = mesh.texAlbedo ? mesh.texAlbedo.get() : m_dummyTex.get();
            QRhiTexture* normalRef = mesh.texNormal ? mesh.texNormal.get() : m_dummyTex.get();
            QRhiTexture* metallicRef = mesh.texMetallic ? mesh.texMetallic.get() : m_dummyTex.get();
            QRhiTexture* smoothRef = mesh.texSmoothness ? mesh.texSmoothness.get() : m_dummyTex.get();

            mesh.srb.reset(m_rhi->newShaderResourceBindings());
            mesh.srb->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0,
                                                         QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                         m_ubuf.get()),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, albedoRef, m_sampler),
                QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, normalRef, m_sampler),
                QRhiShaderResourceBinding::sampledTexture(3, QRhiShaderResourceBinding::FragmentStage, metallicRef, m_sampler),
                QRhiShaderResourceBinding::sampledTexture(4, QRhiShaderResourceBinding::FragmentStage, smoothRef, m_sampler)
            });
            mesh.srb->create();
        }
    }

    // Draw — caller must supply QRhiCommandBuffer* (widget::render gives it), and baseUbo (view/proj/light)
    void draw(QRhiCommandBuffer* cb, const UBufData& baseUbo)
    {
        // first: flush any pending initial uploads (e.g. dummy texture), via cb
        for (auto& rubPtr : m_pendingInitialUploads) {
            cb->resourceUpdate(rubPtr.release());
        }
        m_pendingInitialUploads.clear();

        for (auto& mesh : m_meshes) {
            // lazy upload CPU buffers/textures to GPU if not yet done
            if (!mesh.uploaded) {
                auto rub = m_rhi->nextResourceUpdateBatch();
                if (!mesh.cpuVertices.empty()) {
                    rub->uploadStaticBuffer(mesh.vbuf.get(), mesh.cpuVertices.data());
                }
                if (!mesh.cpuIndices.empty()) {
                    rub->uploadStaticBuffer(mesh.ibuf.get(), mesh.cpuIndices.data());
                }

                // upload textures if they exist (use cpu images)
                if (mesh.texAlbedo && !mesh.cpuAlbedoImg.isNull()) {
                    QImage img = mesh.cpuAlbedoImg.convertToFormat(QImage::Format_RGBA8888);
                    rub->uploadTexture(mesh.texAlbedo.get(), img);
                    rub->generateMips(mesh.texAlbedo.get());
                }
                if (mesh.texNormal && !mesh.cpuNormalImg.isNull()) {
                    QImage img = mesh.cpuNormalImg.convertToFormat(QImage::Format_RGBA8888);
                    rub->uploadTexture(mesh.texNormal.get(), img);
                    rub->generateMips(mesh.texNormal.get());
                }
                if (mesh.texMetallic && !mesh.cpuMetallicImg.isNull()) {
                    QImage img = mesh.cpuMetallicImg.convertToFormat(QImage::Format_RGBA8888);
                    rub->uploadTexture(mesh.texMetallic.get(), img);
                    rub->generateMips(mesh.texMetallic.get());
                }
                if (mesh.texSmoothness && !mesh.cpuSmoothnessImg.isNull()) {
                    QImage img = mesh.cpuSmoothnessImg.convertToFormat(QImage::Format_RGBA8888);
                    rub->uploadTexture(mesh.texSmoothness.get(), img);
                    rub->generateMips(mesh.texSmoothness.get());
                }

                // submit via command buffer
                cb->resourceUpdate(rub);
                mesh.uploaded = true;

                // we can free CPU-side image memory to save RAM
                mesh.cpuAlbedoImg = QImage();
                mesh.cpuNormalImg = QImage();
                mesh.cpuMetallicImg = QImage();
                mesh.cpuSmoothnessImg = QImage();
                mesh.cpuVertices.clear();
                mesh.cpuIndices.clear();
            }

            // update UBO per-mesh flags & model matrix (baseUbo contains view/proj/light/camera)
            UBufData u = baseUbo;
            u.uHasAlbedo = mesh.hasAlbedo ? 1 : 0;
            u.uHasNormal = mesh.hasNormal ? 1 : 0;
            u.uHasMetallic = mesh.hasMetallic ? 1 : 0;
            u.uHasSmoothness = mesh.hasSmoothness ? 1 : 0;

            auto rub2 = m_rhi->nextResourceUpdateBatch();
            rub2->updateDynamicBuffer(m_ubuf.get(), 0, sizeof(UBufData), &u);
            cb->resourceUpdate(rub2);

            // bind SRB (contains UBO + textures)
            cb->setShaderResources(mesh.srb.get());

            // set vertex/index buffers and draw
            const QRhiCommandBuffer::VertexInput v(mesh.vbuf.get(), 0);
            cb->setVertexInput(0, 1, &v, mesh.ibuf.get(), 0, QRhiCommandBuffer::IndexUInt32);

           //  cb->beginPass(W->renderTarget(), QColor(30,30,30), {1.0f, 0});
            //  cb->setGraphicsPipeline(m_ps.get());

            cb->drawIndexed(mesh.indexCount);
        }
    }
       // cb->endPass();


private:
    void loadModel(const std::string& path) {
        Assimp::Importer importer;
        unsigned int flags = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices;
        const aiScene* scene = importer.ReadFile(path, flags);
        if (!scene || !scene->mRootNode) {
            std::cerr << "Assimp: failed to load " << path << " : " << importer.GetErrorString() << std::endl;
            return;
        }

        // recursively process nodes
        processNode(scene->mRootNode, scene, path);
    }

    void processNode(aiNode* node, const aiScene* scene, const std::string& path) {
        for (unsigned i = 0; i < node->mNumMeshes; ++i) {
            aiMesh* aMesh = scene->mMeshes[node->mMeshes[i]];
            RhiMesh mesh;
            mesh.cpuVertices.reserve(aMesh->mNumVertices * 14);

            for (unsigned v = 0; v < aMesh->mNumVertices; ++v) {
                aiVector3D p = aMesh->mVertices[v];
                aiVector3D n = aMesh->mNormals ? aMesh->mNormals[v] : aiVector3D(0,1,0);
                aiVector3D t = aMesh->mTangents ? aMesh->mTangents[v] : aiVector3D(1,0,0);
                aiVector3D b = aMesh->mBitangents ? aMesh->mBitangents[v] : aiVector3D(0,0,1);
                aiVector3D uv = aMesh->mTextureCoords[0] ? aMesh->mTextureCoords[0][v] : aiVector3D(0,0,0);

                // pos (3), normal (3), uv (2), tangent (3), bitangent (3)
                mesh.cpuVertices.push_back(p.x); mesh.cpuVertices.push_back(p.y); mesh.cpuVertices.push_back(p.z);
                mesh.cpuVertices.push_back(n.x); mesh.cpuVertices.push_back(n.y); mesh.cpuVertices.push_back(n.z);
                mesh.cpuVertices.push_back(uv.x); mesh.cpuVertices.push_back(uv.y);
                mesh.cpuVertices.push_back(t.x); mesh.cpuVertices.push_back(t.y); mesh.cpuVertices.push_back(t.z);
                mesh.cpuVertices.push_back(b.x); mesh.cpuVertices.push_back(b.y); mesh.cpuVertices.push_back(b.z);
            }

            for (unsigned f = 0; f < aMesh->mNumFaces; ++f) {
                const aiFace& face = aMesh->mFaces[f];
                for (unsigned idx = 0; idx < face.mNumIndices; ++idx) {
                    mesh.cpuIndices.push_back(face.mIndices[idx]);
                }
            }

            mesh.indexCount = static_cast<int>(mesh.cpuIndices.size());

            // material / textures: load QImage into CPU-side holder (no GPU upload here)
            if (aMesh->mMaterialIndex >= 0) {
                aiMaterial* mat = scene->mMaterials[aMesh->mMaterialIndex];

                // Helper lambda to try load texture into QImage
                auto tryLoadTexToQImage = [&](aiTextureType type, QImage& outImg, bool& outHas) {
                    outHas = false;
                    if (mat->GetTextureCount(type) > 0) {
                        aiString texPath;
                        if (mat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
                            std::string tp = texPath.C_Str();
                            // try direct filesystem path
                            QImage img(QString::fromStdString(tp));
                            if (!img.isNull()) {
                                outImg = img.convertToFormat(QImage::Format_RGBA8888);
                                outHas = true;
                                return;
                            }
                            // try relative to model directory
                            // (user can extend resolvePath logic)
                            // for simplicity, try filename only
                            std::string fname = std::filesystem::path(tp).filename().string();
                            QImage img2(QString::fromStdString(fname));
                            if (!img2.isNull()) {
                                outImg = img2.convertToFormat(QImage::Format_RGBA8888);

                                outHas = true;
                                return;
                            }
                            std::cerr << "Warning: texture not found: " << tp << std::endl;
                        }
                    }
                };

                tryLoadTexToQImage(aiTextureType_DIFFUSE, mesh.cpuAlbedoImg, mesh.hasAlbedo);
                tryLoadTexToQImage(aiTextureType_NORMALS, mesh.cpuNormalImg, mesh.hasNormal);
                // some exporters put normals in HEIGHT
                if (!mesh.hasNormal) tryLoadTexToQImage(aiTextureType_HEIGHT, mesh.cpuNormalImg, mesh.hasNormal);
                tryLoadTexToQImage(aiTextureType_SPECULAR, mesh.cpuMetallicImg, mesh.hasMetallic);
                tryLoadTexToQImage(aiTextureType_SHININESS, mesh.cpuSmoothnessImg, mesh.hasSmoothness);
            }

            m_meshes.push_back(std::move(mesh));
        }

        for (unsigned c = 0; c < node->mNumChildren; ++c) {
            processNode(node->mChildren[c], scene, m_path);
        }
    }

private:
    QRhi* m_rhi;
    std::string m_path;
    std::vector<RhiMesh> m_meshes;

    std::unique_ptr<QRhiBuffer> m_ubuf;
    QRhiSampler* m_sampler = nullptr;

    QVector3D m_fallbackAlbedo = {0.8f,0.8f,0.8f};
    float m_fallbackMetallic = 0.0f;
    float m_fallbackSmoothness = 0.2f;

    // dummy 1x1 texture used where mesh has no texture
    std::unique_ptr<QRhiTexture> m_dummyTex;

    // pending initial uploads (e.g. dummy texture) — transferred to GPU on first draw via cb->resourceUpdate()
    std::vector<std::unique_ptr<QRhiResourceUpdateBatch>> m_pendingInitialUploads;
};

#endif // RHIFBXMODEL_H




