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

    QRhiDriverInfo info = rhi->driverInfo();
    qDebug() << "GPU / Driver name:" << info.deviceName;
    qDebug() << "Driver version:" << info.CpuDevice;
    qDebug() << "Vendor ID:" << QString("0x%1").arg(info.vendorId, 4, 16, QChar('0'));
    qDebug() << "Device ID:" << QString("0x%1").arg(info.deviceId, 4, 16, QChar('0'));

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
    //CUSTOM_WARNING(QString("API: %1").arg(message));
    Logger::instance().log("===============================", Qt::magenta);
    int supportedCount = 0;
    for (const auto& featureInfo : allFeatures) {
        if (rhi->isFeatureSupported(featureInfo.first)) {
            qDebug() << "ok" << featureInfo.second;
            supportedCount++;
        }
        else {
            qCritical() << "no" << featureInfo.second;
        }
    }

    qDebug() << "--- Supported Futures:" << supportedCount << "/" << allFeatures.count() << "---";
    qDebug() << "========================";
}

#endif // APIFUTURESINFO_H
