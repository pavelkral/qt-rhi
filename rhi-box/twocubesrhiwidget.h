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

    struct UBOData
    {
        float mvp[16];       // mat4
        float model[16];     // mat4
        float normalMat[16]; // mat4
        float lightPos[4];   // vec4 (x,y,z,w)
    };
    // celkem 208 B
  //  static_assert(sizeof(UBOData) == 208,"UBOData size must be exactly 208 bytes (std140 layout).");
    // QRhi
    QRhi *m_rhi = nullptr;
    QRhiRenderPassDescriptor* m_rpDesc = nullptr;
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
