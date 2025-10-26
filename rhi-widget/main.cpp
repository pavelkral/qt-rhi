#include <QApplication>
#include <QVBoxLayout>
#include "rhiwidget.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);

   // RhiWidget w;
   // w.resize(1280, 600);
  //  w.show();

    RhiWidget *rhiWidget = new RhiWidget;

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
