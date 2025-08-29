// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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



   RhiWidget w;
   // w.setLayout(layout);
    w.resize(1280, 720);
    w.show();

    return app.exec();
}
