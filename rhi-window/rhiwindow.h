// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include "camera.h"
#include "cube.h"

#include <QWindow>
#include <QOffscreenSurface>
#include <rhi/qrhi.h>

class RhiWindow : public QWindow
{
public:
    RhiWindow(QRhi::Implementation graphicsApi);
    QString graphicsApiName() const;
    void releaseSwapChain();

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

    bool m_hasSwapChain = false;
    QMatrix4x4 m_viewProjection;
    QMatrix4x4 m_projection;
    Cube m_cube1;
    Cube m_cube2;
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

    void customInit() override;
    void customRender() override;
protected:
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
private:
    void loadTexture(const QSize &pixelSize, QRhiResourceUpdateBatch *u);
    void updateCamera(float dt); // Funkce pro aktualizaci kamery každý frame

    Camera m_camera;
    QSet<int> m_pressedKeys;
    QPointF m_lastMousePos;
    QElapsedTimer m_timer;
    float m_dt = 0;

    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<QRhiSampler> m_sampler;

    QRhiResourceUpdateBatch *m_initialUpdates = nullptr;

    float m_rotation = 0;
    float m_opacity = 1;
    int m_opacityDir = -1;
};

#endif
