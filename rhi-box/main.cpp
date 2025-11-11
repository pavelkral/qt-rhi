#include <QApplication>
#include <QRhiWidget>
#include "examplewidget.h"


int main(int argc, char** argv)
{

    QApplication app(argc, argv);
    RhiWidget  widget(QRhi::D3D12);
    widget.setApi(QRhiWidget::Api::Direct3D12);
    widget.resize(1920,1080);
    widget.show();
    return app.exec();
}

