#ifndef CUBERHIWIDGET_H
#define CUBERHIWIDGET_H

#include <QRhiWidget>
#include <QtGui/QMatrix4x4>
#include <QtGui/QColor>
#include <QtGui/private/qrhi_p.h>
#include <QScopedPointer>
#include <QImage>
#include <rhi/qrhi.h>
// Widget pro vykreslení texturované krychle pomocí Qt RHI
class CubeRhiWidget : public QRhiWidget {
    Q_OBJECT

public:
    using QRhiWidget::QRhiWidget;

protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    QRhi *m_rhi = nullptr;
    QScopedPointer<QRhiBuffer> m_vbuf;   // Vertex buffer
    QScopedPointer<QRhiBuffer> m_ibuf;   // Index buffer
    QScopedPointer<QRhiBuffer> m_ubuf;   // Uniform buffer (pro MVP matici)
    QScopedPointer<QRhiTexture> m_texture;   // Textura krychle
    QScopedPointer<QRhiSampler> m_sampler;   // Sampler pro texturu
    QScopedPointer<QRhiShaderResourceBindings> m_srb; // Bindings
    QScopedPointer<QRhiGraphicsPipeline> m_pipeline; // Pipeline

    QMatrix4x4 m_viewProjection;  // ViewProjection matice
    float m_rotation = 0.0f;      // Rotace krychle
};

#endif // CUBERHIWIDGET_H
