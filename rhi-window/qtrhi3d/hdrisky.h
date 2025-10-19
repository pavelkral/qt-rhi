#ifndef QT_HDRI_SKY_H
#define QT_HDRI_SKY_H

#include <QObject>
#include <QMatrix4x4>
#include <QString>
#include <QScopedPointer>
#include <QLoggingCategory>

// Forward deklarace RHI typů
class QRhi;
class QRhiBuffer;
class QRhiTexture;
class QRhiSampler;
class QRhiGraphicsPipeline;
class QRhiShaderResourceBindings;
class QRhiRenderPassDescriptor;
class QRhiCommandBuffer;
class QShader;

Q_DECLARE_LOGGING_CATEGORY(logHdri)

class QtHdriSky : public QObject
{
    Q_OBJECT

public:
    explicit QtHdriSky(QRhi *rhi, QObject *parent = nullptr);
    ~QtHdriSky();

    /**
     * @brief Inicializuje skybox, načte HDR, provede konverzi na cubemapu a vytvoří pipelines.
     * @param hdrPath Cesta k .hdr souboru.
     * @param mainPassDesc Popisovač renderovacího průchodu (Render Pass Descriptor) hlavního okna/cíle,
     * do kterého se bude skybox vykreslovat.
     * @return true při úspěchu, jinak false.
     */
    bool init(const QString &hdrPath, QRhiRenderPassDescriptor *mainPassDesc);

    /**
     * @brief Vykreslí skybox do aktivního command bufferu.
     * @param cb Aktivní command buffer.
     * @param view Matice pohledu kamery.
     * @param projection Projekční matice kamery.
     */
    void draw(QRhiCommandBuffer *cb, const QMatrix4x4 &view, const QMatrix4x4 &projection);

    /**
     * @brief Vrátí hotovou cubemapu (např. pro IBL).
     */
    QRhiTexture *environmentCubemap() const { return m_envCubemap.get(); }

private:
    void renderToCubemap(QRhiCommandBuffer *cb);
    bool createPipelines(QRhiRenderPassDescriptor *mainPassDesc);


    QRhi *m_rhi;

    // Geometrie kostky
    QScopedPointer<QRhiBuffer> m_cubeVbo;

    // Zdroje
    QScopedPointer<QRhiTexture> m_hdrTexture;
    QScopedPointer<QRhiTexture> m_envCubemap;
    QScopedPointer<QRhiSampler> m_hdrSampler;     // Pro equirect mapu (GL_LINEAR)
    QScopedPointer<QRhiSampler> m_cubemapSampler; // Pro skybox (GL_LINEAR_MIPMAP_LINEAR)

    // Pipeline pro Equirectangular -> Cubemap
    QScopedPointer<QRhiTexture> m_captureDepthTexture;
    QScopedPointer<QRhiRenderPassDescriptor> m_cubemapPassDesc;
    QScopedPointer<QRhiBuffer> m_equirectUbo;
    QScopedPointer<QRhiShaderResourceBindings> m_equirectBindings;
    QScopedPointer<QRhiGraphicsPipeline> m_equirectPipeline;

    // Pipeline pro vykreslení Skyboxu
    QScopedPointer<QRhiBuffer> m_skyboxUbo;
    QScopedPointer<QRhiShaderResourceBindings> m_skyboxBindings;
    QScopedPointer<QRhiGraphicsPipeline> m_skyboxPipeline;

    // Matice pro konverzi
    QMatrix4x4 m_captureProjection;
    QMatrix4x4 m_captureViews[6];

    bool m_initialized = false;
    static const int CUBEMAP_RESOLUTION = 512;
};

#endif // QT_HDRI_SKY_H
