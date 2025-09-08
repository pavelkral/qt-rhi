#include <QApplication>
#include <QVBoxLayout>
#include "cuberhiwidget.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);

   // CubeRhiWidget w;
   // w.resize(1280, 600);
  //  w.show();

    CubeRhiWidget *rhiWidget = new CubeRhiWidget;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(rhiWidget);

    QWidget w;
    w.setLayout(layout);
    w.resize(1280, 720);
    w.setFixedHeight(720);
    w.setFixedWidth(1280);
    w.show();

    return app.exec();
}
