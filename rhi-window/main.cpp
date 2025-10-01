
#include <QGuiApplication>
#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/qslider.h>
#include <QtWidgets/qwidget.h>
#include "rhiwindow.h"

//#define EDITOR_MODE

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QRhi::Implementation graphicsApi;
    int ret = 0;

    // Use platform-specific defaults when no command-line arguments given.
#if defined(Q_OS_WIN)
  // graphicsApi = QRhi::D3D12;
    graphicsApi = QRhi::Vulkan;
  // graphicsApi = QRhi::OpenGLES2;
#elif QT_CONFIG(metal)
    graphicsApi = QRhi::Metal;
#elif QT_CONFIG(vulkan)
    graphicsApi = QRhi::Vulkan;
#else
    graphicsApi = QRhi::OpenGLES2;
#endif

    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    QCommandLineOption nullOption({ "n", "null" }, QLatin1String("Null"));
    cmdLineParser.addOption(nullOption);
    QCommandLineOption glOption({ "g", "opengl" }, QLatin1String("OpenGL"));
    cmdLineParser.addOption(glOption);
    QCommandLineOption vkOption({ "v", "vulkan" }, QLatin1String("Vulkan"));
    cmdLineParser.addOption(vkOption);
    QCommandLineOption d3d11Option({ "d", "d3d11" }, QLatin1String("Direct3D 11"));
    cmdLineParser.addOption(d3d11Option);
    QCommandLineOption d3d12Option({ "D", "d3d12" }, QLatin1String("Direct3D 12"));
    cmdLineParser.addOption(d3d12Option);
    QCommandLineOption mtlOption({ "m", "metal" }, QLatin1String("Metal"));
    cmdLineParser.addOption(mtlOption);

    cmdLineParser.process(app);
    if (cmdLineParser.isSet(nullOption))
        graphicsApi = QRhi::Null;
    if (cmdLineParser.isSet(glOption))
        graphicsApi = QRhi::OpenGLES2;
    if (cmdLineParser.isSet(vkOption))
        graphicsApi = QRhi::Vulkan;
    if (cmdLineParser.isSet(d3d11Option))
        graphicsApi = QRhi::D3D11;
    if (cmdLineParser.isSet(d3d12Option))
        graphicsApi = QRhi::D3D12;
    if (cmdLineParser.isSet(mtlOption))
        graphicsApi = QRhi::Metal;

    // For OpenGL, to ensure there is a depth/stencil buffer for the window.
    // With other APIs this is under the application's control (QRhiRenderBuffer etc.)
    // and so no special setup is needed for those.
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    // Special case macOS to allow using OpenGL there.
    // (the default Metal is the recommended approach, though)
    // gl_VertexID is a GLSL 130 feature, and so the default OpenGL 2.1 context
    // we get on macOS is not sufficient.
#ifdef Q_OS_MACOS
    fmt.setVersion(4, 1);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
#endif
    QSurfaceFormat::setDefaultFormat(fmt);

#if QT_CONFIG(vulkan)
    QVulkanInstance inst;
    if (graphicsApi == QRhi::Vulkan) {
        // Request validation, if available. This is completely optional
        // and has a performance impact, and should be avoided in production use.
        inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
        // Play nice with QRhi.
        inst.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
        if (!inst.create()) {
            qWarning("Failed to create Vulkan instance, switching to OpenGL");
            graphicsApi = QRhi::OpenGLES2;
        }
    }
#endif

#ifdef EDITOR_MODE
    {
        QMainWindow mainWindow;
        HelloWindow *rhiWindow = new HelloWindow(graphicsApi);

#if QT_CONFIG(vulkan)
        if (graphicsApi == QRhi::Vulkan)
            rhiWindow->setVulkanInstance(&inst);
#endif

        rhiWindow->resize(1280, 720);
        rhiWindow->setTitle("QRhi - " + rhiWindow->graphicsApiName() + " (Editor Mode)");

        QWidget *container = QWidget::createWindowContainer(rhiWindow);
        QSlider *slider = new QSlider(Qt::Vertical);
        slider->setRange(0, 100);
        slider->setValue(50);

        QObject::connect(slider, &QSlider::valueChanged, [rhiWindow](int value){
            // rhiWindow->setSomeParameter(value);
        });

        QWidget *centralWidget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(centralWidget);
        layout->addWidget(container);
        layout->addWidget(slider);

        mainWindow.setCentralWidget(centralWidget);
        mainWindow.resize(1320, 720);
        mainWindow.show();

        ret = app.exec();

        if (rhiWindow->handle())
            rhiWindow->releaseSwapChain();

         delete rhiWindow;
    }
#else // !EDITOR_MODE
    {
        HelloWindow window(graphicsApi);

#if QT_CONFIG(vulkan)
        if (graphicsApi == QRhi::Vulkan)
            window.setVulkanInstance(&inst);
#endif

        window.resize(1280, 720);
        window.setTitle("QRhi" + QLatin1String(" - ") + window.graphicsApiName());
        window.show();

        ret = app.exec();

        if (window.handle())
            window.releaseSwapChain();
    }
#endif

    return ret;
}
