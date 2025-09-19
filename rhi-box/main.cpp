#include <QApplication>
#include "examplewidget.h"


int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    qputenv("QT_RHI", "vulkan"); //  d3d11, metal, opengl
    RhiWidget  widget;

    widget.resize(800,600);
    widget.show();

   // widget.loadModel(":/models/your_model.fbx");

    return app.exec();
}

