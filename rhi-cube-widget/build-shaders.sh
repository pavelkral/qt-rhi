#!/bin/bash

QT_PATH="C:/Qt/6.9.0/msvc2022_64"

# Kontrola, zda byla cesta k Qt nastavena
#if [ "$QT_PATH" == "/path/to/your/Qt/6.9.0/your_kit" ]; then
    #echo "CHYBA: Prosím, upravte proměnnou 'QT_PATH' ve skriptu tak, aby ukazovala na vaši Qt 6.9+ instalaci."
    #exit 1
#fi

echo "  -> Kompiluji shadery pomocí qsb..."
QSB_CMD="$QT_PATH/bin/qsb"
if [ ! -x "$QSB_CMD" ]; then
    echo "CHYBA: Nástroj 'qsb' nebyl nalezen v '$QSB_CMD'. Zkontrolujte cestu QT_PATH."
    exit 1
fi
"$QSB_CMD" shaders/cube.vert -o cube.vert.qsb --glsl 450 --hlsl 50
"$QSB_CMD" shaders/cube.frag -o cube.frag.qsb --glsl 450 --hlsl 50
#"$QSB_CMD" --fragment -o frag.qsb shader.frag

if [ ! -f "vert.qsb" ] || [ ! -f "frag.qsb" ]; then
    echo "CHYBA: Kompilace shaderů selhala."
    exit 1
fi
