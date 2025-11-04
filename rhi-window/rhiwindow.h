// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include "qtrhi3d/camera.h"
#include "qtrhi3d/fbxmodel.h"
#include "qtrhi3d/model.h"
#include "qtrhi3d/proceduralsky.h"
#include "qtrhi3d/hdrisky.h"
#include <QWindow>
#include <QOffscreenSurface>
#include <QElapsedTimer>
#include <rhi/qrhi.h>

class RhiWindow : public QWindow
{
public:
    RhiWindow(QRhi::Implementation graphicsApi);
    QString graphicsApiName() const;
    void releaseSwapChain();
    QMatrix4x4 m_projection;
    int m_currentFps = 555;
    QElapsedTimer m_timer;
    int m_frameCount = 0;
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
   // QMatrix4x4 m_viewProjection;

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
    void updateFullscreenTexture(const QSize &pixelSize, QRhiResourceUpdateBatch *u);

    QSet<int> pressedKeys;
    QPointF lastMousePosition;
    QElapsedTimer mainTimer;
    bool cameraMovementEnabled = true;
    float deltaTime = 0;
    const QSize SHADOW_MAP_SIZE = QSize(2048, 2048);
    QRhiResourceUpdateBatch *initialUpdateBatch = nullptr;

    QRhiTexture *shadowMapTexture = nullptr;
    QRhiSampler *shadowMapSampler = nullptr;
    QRhiTextureRenderTarget *shadowMapRenderTarget = nullptr;
    QRhiRenderPassDescriptor * shadowMapRenderPassDesc = nullptr;

    QRhiBuffer* shadowUbo = nullptr;
    QRhiShaderResourceBindings *shadowSRB = nullptr;
    QRhiGraphicsPipeline *shadowPipeline = nullptr;

    std::unique_ptr<QRhiShaderResourceBindings> uiSRB= nullptr;
    std::unique_ptr<QRhiGraphicsPipeline> uiPipeline = nullptr;
    std::unique_ptr<QRhiTexture> uiTexture = nullptr;
    std::unique_ptr<QRhiSampler> uiSampler = nullptr;

public:
    float lightTime = 0.0f;
    QVector3D lightPosition;
    float modelRotation = 0;
    float objectOpacity = 1;
    int objectOpacityDir = -1;
    float shaderapi = 0;
    Camera mainCamera;
    Model cubeModel1;
    Model cubeModel;
    Model sphereModel;
    Model sphereModel1;
    Model lightSphere;
    Model floor;
    std::unique_ptr<FbxModel> model = nullptr;
    std::unique_ptr<ProceduralSkyRHI> sky = nullptr;
    std::unique_ptr<HdriSky> hsky;
    QVector<Model*> models;



};

#endif
