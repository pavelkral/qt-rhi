// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include "qtrhi3d/camera.h"
#include "qtrhi3d/model.h"

#include <QWindow>
#include <QOffscreenSurface>
#include <rhi/qrhi.h>

class RhiWindow : public QWindow
{
public:
    RhiWindow(QRhi::Implementation graphicsApi);
    QString graphicsApiName() const;
    void releaseSwapChain();
    QMatrix4x4 m_projection;
protected:
    virtual void customInit() = 0;
    virtual void customRender() = 0;

#if QT_CONFIG(opengl)
    std::unique_ptr<QOffscreenSurface> m_fallbackSurface;
#endif
    std::unique_ptr<QRhi> m_rhi;
    std::unique_ptr<QRhiSwapChain> m_sc;
    std::unique_ptr<QRhiRenderBuffer> m_ds;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rp;

    QMatrix4x4 createProjection(QRhi *rhi, float fovDeg, float aspect, float nearPlane, float farPlane);
    bool m_hasSwapChain = false;
    QMatrix4x4 m_viewProjection;

private:
    void init();
    void resizeSwapChain();
    void render();
    void exposeEvent(QExposeEvent *) override;
    bool event(QEvent *) override;

    QRhi::Implementation m_graphicsApi;
    bool m_initialized = false;
    bool m_notExposed = false;
    bool m_newlyExposed = false;
};

//================================================================================================

class HelloWindow : public RhiWindow
{
public:

    float lightTime = 0.0f;

    HelloWindow(QRhi::Implementation graphicsApi);
    ~HelloWindow();
    void customInit() override;
    void customRender() override;
    void initShadowMapResources(QRhi *rhi);
    void releaseShadowMapResources();
protected:
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
private:
    void updateCamera(float dt);
    void generateLightFrustum(float orthoSize, float nearPlane, float farPlane,
                                           QVector<float> &vertices, QVector<quint16> &indices);
    QVector3D computeSceneCenterAndExtents(const QVector<Model*> &objects, QVector3D &extents);
    void updateFullscreenTexture(const QSize &pixelSize, QRhiResourceUpdateBatch *u);

    QSet<int> m_pressedKeys;
    QPointF m_lastMousePos;
    QElapsedTimer m_timer;
    float m_dt = 0;
    QRhiResourceUpdateBatch *m_initialUpdates = nullptr;

    const QSize SHADOW_MAP_SIZE = QSize(2048, 2048);
    QRhiTexture *m_shadowMapTexture = nullptr;
    QRhiSampler *m_shadowMapSampler = nullptr;
    QRhiTextureRenderTarget *m_shadowMapRenderTarget = nullptr;
    QRhiRenderPassDescriptor * m_shadowMapRenderPassDesc = nullptr;

    std::unique_ptr<QRhiShaderResourceBindings> m_fullscreenQuadSrb;
    std::unique_ptr<QRhiGraphicsPipeline> m_fullscreenQuadPipeline;
    std::unique_ptr<QRhiTexture> m_fullscreenTexture;
    std::unique_ptr<QRhiSampler> m_fullScreenSampler;

    QRhiBuffer* m_shadowUbo = nullptr;
    QRhiShaderResourceBindings *m_shadowSRB = nullptr;
    QRhiGraphicsPipeline *m_shadowPipeline = nullptr;

    QVector3D lightPosition;
    float m_rotation = 0;
    float m_opacity = 1;
    int m_opacityDir = -1;

    Model cubeModel1;
    Model lightSphere;
    Model floor;
    Camera mainCamera;
    Model frustumModel;
};

#endif
