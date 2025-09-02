#include <QApplication>
#include "examplewidget.h"
#include "TwoCubesRhiWidget.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    qputenv("QT_RHI", "vulkan"); // nebo: d3d11, metal, opengl
   // RhiWidget  widget;
     TwoCubesRhiWidget widget;
    widget.resize(800,600);
    widget.show();

   // widget.loadModel(":/models/your_model.fbx");

    return app.exec();
}

