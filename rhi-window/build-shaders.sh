#!/bin/bash

QT_PATH="C:/Qt/6.9.0/msvc2022_64"

echo "  -> compile qsb..."
QSB_CMD="$QT_PATH/bin/qsb"
if [ ! -x "$QSB_CMD" ]; then
    echo "err:  'qsb' not found '$QSB_CMD'. check QT_PATH."
    exit 1
fi
"$QSB_CMD" --format hlsl  shaders/color.vert.hlsl -o shaders/prebuild/color.vert.qsb  --hlsl 50
"$QSB_CMD" --format hlsl shaders/color.frag.hlsl -o shaders/prebuild/color.frag.qsb  --hlsl 50
#"$QSB_CMD" --fragment -o frag.qsb shader.frag

if [ ! -f "vert.qsb" ] || [ ! -f "frag.qsb" ]; then
    echo "CHYBA: Kompilace shaderů selhala."
    exit 1
fi
