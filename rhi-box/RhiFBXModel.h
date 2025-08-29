#ifndef RHIFBXMODEL_H
#define RHIFBXMODEL_H

#include <rhi/qrhi.h>


#include <QMatrix4x4>
#include <QVector3D>
#include <vector>
#include <memory>

struct UBufData {
    QMatrix4x4 uModel;
    QMatrix4x4 uView;
    QMatrix4x4 uProj;
    QVector3D uLightPos;       float uAmbientStrength;
    QVector3D uLightColor;     float uMetallicFactor;
    QVector3D uCameraPos;      float uSmoothnessFactor;
    QVector3D uAlbedoColor;    float pad0;
    int uHasAlbedo;
    int uHasNormal;
    int uHasMetallic;
    int uHasSmoothness;
};

struct RhiMesh {
    std::unique_ptr<QRhiBuffer> vbuf, ibuf;
    int indexCount = 0;

    std::unique_ptr<QRhiTexture> texAlbedo, texNormal, texMetallic, texSmoothness;
    std::unique_ptr<QRhiShaderResourceBindings> srb;

    bool hasAlbedo=false, hasNormal=false, hasMetallic=false, hasSmoothness=false;
};

class RhiFBXModel {
public:
    RhiFBXModel(QRhi* rhi)
        : m_rhi(rhi)
    {}

    void setFallbackTexture(QRhiTexture* tex) { m_dummyTex = tex; }

    void addMesh(RhiMesh&& mesh) { m_meshes.push_back(std::move(mesh)); }

    void createResources(QRhiSampler* sampler) {
        m_sampler = sampler;

        // UBO
        m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UBufData)));
        m_ubuf->create();

        for(auto& mesh : m_meshes) {
            // fallback textures
            QRhiTexture* albedoRef = mesh.texAlbedo ? mesh.texAlbedo.get() : m_dummyTex;
            QRhiTexture* normalRef = mesh.texNormal ? mesh.texNormal.get() : m_dummyTex;
            QRhiTexture* metallicRef = mesh.texMetallic ? mesh.texMetallic.get() : m_dummyTex;
            QRhiTexture* smoothRef = mesh.texSmoothness ? mesh.texSmoothness.get() : m_dummyTex;

            mesh.srb.reset(m_rhi->newShaderResourceBindings());
            mesh.srb->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage|QRhiShaderResourceBinding::FragmentStage, m_ubuf.get()),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, albedoRef, m_sampler),
                QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, normalRef, m_sampler),
                QRhiShaderResourceBinding::sampledTexture(3, QRhiShaderResourceBinding::FragmentStage, metallicRef, m_sampler),
                QRhiShaderResourceBinding::sampledTexture(4, QRhiShaderResourceBinding::FragmentStage, smoothRef, m_sampler)
            });
            mesh.srb->create();
        }
    }

    void draw(QRhiCommandBuffer* cb, const UBufData& u) {
        for(auto& mesh : m_meshes) {
            // aktualizace UBO přímo přes command buffer
            auto rub = m_rhi->nextResourceUpdateBatch();
            rub->updateDynamicBuffer(m_ubuf.get(), 0, sizeof(UBufData), &u);
            cb->resourceUpdate(rub);

            cb->setShaderResources(mesh.srb.get());
            const QRhiCommandBuffer::VertexInput v(mesh.vbuf.get(), 0);
            cb->setVertexInput(0,1,&v, mesh.ibuf.get(), 0, QRhiCommandBuffer::IndexUInt32);
            cb->drawIndexed(mesh.indexCount);
        }
    }

private:
    QRhi* m_rhi;
    QRhiSampler* m_sampler = nullptr;
    QRhiTexture* m_dummyTex = nullptr;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::vector<RhiMesh> m_meshes;
};

#endif // RHIFBXMODEL_H


