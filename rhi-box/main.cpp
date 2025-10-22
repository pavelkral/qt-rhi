#include <QApplication>
#include "examplewidget.h"


int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    qputenv("QT_RHI", "vulkan"); //  d3d11, metal, opengl
    RhiWidget  widget;

    widget.resize(1920,1080);
    widget.show();



    return app.exec();
}

