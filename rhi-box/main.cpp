
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QFontInfo>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include "examplewidget.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

qputenv("QT_GRAPHICSSYSTEM", "vulkan");

   RhiWidget w;
   // w.setLayout(layout);
    w.resize(1280, 720);
   //w.setPreferredBackend(QRhi::Vulkan);
    w.setApi(QRhiWidget::Api::Vulkan);
    w.show();

    return app.exec();
}
