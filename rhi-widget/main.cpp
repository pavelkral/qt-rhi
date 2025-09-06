#include <QApplication>
#include "cuberhiwidget.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    CubeRhiWidget w;
    w.resize(1280, 600);
    w.show();
    return app.exec();
}
