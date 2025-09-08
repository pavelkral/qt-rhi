#ifndef CUBERHIWIDGET_H
#define CUBERHIWIDGET_H

#include <QRhiWidget>
#include <QtGui/QMatrix4x4>
#include <QtGui/QColor>
#include <QtGui/private/qrhi_p.h>
#include <QScopedPointer>
#include <QImage>
#include <rhi/qrhi.h>
#include "cube.h"

class CubeRhiWidget : public QRhiWidget {
    Q_OBJECT

public:
    using QRhiWidget::QRhiWidget;

    Cube m_cube1;
    Cube m_cube2;
    void loadTexture(const QSize &, QRhiResourceUpdateBatch *u);
protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    QRhi *m_rhi = nullptr;
    int m_sampleCount = 1;
    QSize m_pixelSize;

    std::unique_ptr<QRhiRenderPassDescriptor> m_rp;
    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<QRhiSampler> m_sampler;
    QRhiResourceUpdateBatch *m_initialUpdates = nullptr;


    float m_opacity = 1;
    QMatrix4x4 m_viewProjection;
    float m_rotation = 0.0f;
};

#endif // CUBERHIWIDGET_H
