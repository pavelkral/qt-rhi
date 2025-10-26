#ifndef APIFUTURESINFO_H
#define APIFUTURESINFO_H

#include <QDebug>
#include <rhi/qrhi.h>
#include <QDebug>
#include <QList>
#include <QPair>
#include <logger.h>

void dumpApiFeatures(QRhi *rhi)
{
    if (!rhi) {
        qWarning() << "printSupportedRhiFeatures:, QRhi is null.";
        return;
    }

    Logger::instance().log("===============================", Qt::magenta);
    Logger::instance().log("QRhi Configuration", Qt::magenta);
    //CUSTOM_WARNING(QString("API: %1").arg(message));
    Logger::instance().log("===============================", Qt::magenta);
    // 1. Backend
    QString backendName;
    switch (rhi->backend()) {
    case QRhi::Null:       backendName = "Null"; break;
    case QRhi::Vulkan:     backendName = "Vulkan"; break;
    case QRhi::D3D11:      backendName = "Direct3D 11"; break;
    case QRhi::D3D12:      backendName = "Direct3D 12"; break;
    case QRhi::OpenGLES2:   backendName = "OpenGL ES / OpenGL"; break;
    case QRhi::Metal:      backendName = "Metal"; break;
    default:               backendName = "Neznamy"; break;
    }
    qDebug() << " Backend API:" << backendName;

    Logger::instance().log("===============================", Qt::magenta);
    Logger::instance().log("Gpu info", Qt::magenta);
    Logger::instance().log("===============================", Qt::magenta);
    QRhiDriverInfo info = rhi->driverInfo();
    qDebug() << "GPU / Driver name:" << info.deviceName;
    qDebug() << "Driver version:" << info.CpuDevice;
    qDebug() << "Vendor ID:" << QString("0x%1").arg(info.vendorId, 4, 16, QChar('0'));
    qDebug() << "Device ID:" << QString("0x%1").arg(info.deviceId, 4, 16, QChar('0'));

    QList<QPair<QRhi::Feature, const char*>> unsuportedFeatures ;

    static const QList<QPair<QRhi::Feature, const char*>> allFeatures = {
        { QRhi::MultisampleTexture, "MultisampleTexture" },
        { QRhi::MultisampleRenderBuffer, "MultisampleRenderBuffer" },
        { QRhi::DebugMarkers, "DebugMarkers" },
        { QRhi::Timestamps, "Timestamps" },
        { QRhi::Instancing, "Instancing" },
        { QRhi::CustomInstanceStepRate, "CustomInstanceStepRate" },
        { QRhi::PrimitiveRestart, "PrimitiveRestart" },
        { QRhi::NonDynamicUniformBuffers, "NonDynamicUniformBuffers" },
        { QRhi::NonFourAlignedEffectiveIndexBufferOffset, "NonFourAlignedEffectiveIndexBufferOffset" },
        { QRhi::NPOTTextureRepeat, "NPOTTextureRepeat" },
        { QRhi::RedOrAlpha8IsRed, "RedOrAlpha8IsRed" },
        { QRhi::ElementIndexUint, "ElementIndexUint" },
        { QRhi::Compute, "Compute" },
        { QRhi::WideLines, "WideLines" },
        { QRhi::VertexShaderPointSize, "VertexShaderPointSize" },
        { QRhi::BaseVertex, "BaseVertex" },
        { QRhi::BaseInstance, "BaseInstance" },
        { QRhi::TriangleFanTopology, "TriangleFanTopology" },
        { QRhi::ReadBackNonUniformBuffer, "ReadBackNonUniformBuffer" },
        { QRhi::ReadBackNonBaseMipLevel, "ReadBackNonBaseMipLevel" },
        { QRhi::TexelFetch, "TexelFetch" },
        { QRhi::RenderToNonBaseMipLevel, "RenderToNonBaseMipLevel" },
        { QRhi::IntAttributes, "IntAttributes" },
        { QRhi::ScreenSpaceDerivatives, "ScreenSpaceDerivatives" },
        { QRhi::ReadBackAnyTextureFormat, "ReadBackAnyTextureFormat" },
        { QRhi::PipelineCacheDataLoadSave, "PipelineCacheDataLoadSave" },
        { QRhi::ImageDataStride, "ImageDataStride" },
        { QRhi::RenderBufferImport, "RenderBufferImport" },
        { QRhi::ThreeDimensionalTextures, "ThreeDimensionalTextures" },
        { QRhi::RenderTo3DTextureSlice, "RenderTo3DTextureSlice" },
        { QRhi::TextureArrays, "TextureArrays" },
        { QRhi::Tessellation, "Tessellation" },
        { QRhi::GeometryShader, "GeometryShader" },
        { QRhi::TextureArrayRange, "TextureArrayRange" },
        { QRhi::NonFillPolygonMode, "NonFillPolygonMode" },
        { QRhi::OneDimensionalTextures, "OneDimensionalTextures" },
        { QRhi::OneDimensionalTextureMipmaps, "OneDimensionalTextureMipmaps" },
        { QRhi::HalfAttributes, "HalfAttributes" },
        { QRhi::RenderToOneDimensionalTexture, "RenderToOneDimensionalTexture" },
        { QRhi::ThreeDimensionalTextureMipmaps, "ThreeDimensionalTextureMipmaps" },
        { QRhi::MultiView, "MultiView" },
        { QRhi::TextureViewFormat, "TextureViewFormat" },
        { QRhi::ResolveDepthStencil, "ResolveDepthStencil" },
        { QRhi::VariableRateShading, "VariableRateShading" },
        { QRhi::VariableRateShadingMap, "VariableRateShadingMap" },
        { QRhi::VariableRateShadingMapWithTexture, "VariableRateShadingMapWithTexture" },
            { QRhi::PerRenderTargetBlending, "PerRenderTargetBlending" }
    };

    Logger::instance().log("===============================", Qt::magenta);
    Logger::instance().log("Supported Futures QRhi backend", Qt::magenta);
    Logger::instance().log("===============================", Qt::magenta);

    int supportedCount = 0;
    for (const auto& featureInfo : allFeatures) {
        if (rhi->isFeatureSupported(featureInfo.first)) {
            qDebug() << "ok" << featureInfo.second;
            supportedCount++;
        }
        else {
            unsuportedFeatures.append({featureInfo.first,featureInfo.second});
           // qCritical() << "no" << featureInfo.second;
        }


    }

    qDebug() << "--- Supported Futures:" << supportedCount << "/" << allFeatures.count() << "---";
    qDebug() << "========================";

    Logger::instance().log("===============================", Qt::magenta);
    Logger::instance().log("UnSupported Futures QRhi", Qt::magenta);
    Logger::instance().log("===============================", Qt::magenta);

    for (const auto& featureInfo1 : unsuportedFeatures) {

            qCritical() << "no" << featureInfo1.second;
    }
    qCritical() << "--- UnSupported Futures:"  << unsuportedFeatures.count() << "/"<< allFeatures.count() << "---";
    qCritical() << "========================";
}

#endif // APIFUTURESINFO_H
