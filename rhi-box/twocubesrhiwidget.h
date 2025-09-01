#ifndef TWOCUBESRHIWIDGET_H
#define TWOCUBESRHIWIDGET_H

#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QTimer>

class TwoCubesRhiWidget : public QRhiWidget
{
    Q_OBJECT
public:
    explicit TwoCubesRhiWidget(QWidget *parent = nullptr);
    ~TwoCubesRhiWidget() override;

protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    void releaseResources() override;
 //   void resizeSwapChain() override {}

private:
    void buildPipelines();
    void createGeometry();
    void createTexture();

    struct UBOData {
        QMatrix4x4 mvp;
        QMatrix4x4 model;
        QMatrix3x3 normalMat;
        QVector3D lightPos;
        float pad = 0.0f;
    };

    // QRhi
    QRhi *m_rhi = nullptr;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rpDesc;

    // Geometry
    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ibuf;
    int m_indexCount = 0;

    // Raw backing data to upload on first render
    QByteArray m_geomVertsData;
    QByteArray m_geomIdxData;

    // Texture + sampler
    std::unique_ptr<QRhiTexture> m_tex;
    std::unique_ptr<QRhiSampler> m_sampler;
    QImage m_texImg;

    // Pipelines
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeLambert;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeToon;

    std::unique_ptr<QRhiShaderResourceBindings> m_srbLambert;
    std::unique_ptr<QRhiShaderResourceBindings> m_srbToon;

    // Uniform buffers
    std::unique_ptr<QRhiBuffer> m_uboCubeA;
    std::unique_ptr<QRhiBuffer> m_uboCubeB;

    QTimer m_anim;
    float m_angle = 0.0f;

    // Upload state
    bool m_uploaded = false;
};
#endif // TWOCUBESRHIWIDGET_H
