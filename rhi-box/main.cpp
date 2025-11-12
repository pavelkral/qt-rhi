#include <QApplication>
#include <QRhiWidget>
#include "examplewidget.h"


int main(int argc, char** argv)
{

    QApplication app(argc, argv);
    RhiWidget  widget(QRhi::D3D11);
    widget.setApi(QRhiWidget::Api::Direct3D11);
    widget.resize(1280,720);
    widget.show();
    return app.exec();
}

