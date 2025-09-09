#!/bin/bash

SHADER_DIR="shaders"
OUT_DIR="shaders/prebuild"
QT_PATH="C:/Qt/6.9.0/msvc2022_64"

#mkdir -p "$OUT_DIR"
QSB_CMD="$QT_PATH/bin/qsb"
if [ ! -x "$QSB_CMD" ]; then
    echo "err:  'qsb' not found '$QSB_CMD'. check QT_PATH."
    exit 1
fi

for file in "$SHADER_DIR"/*.vert "$SHADER_DIR"/*.frag; do
    name=$(basename "$file")
    outname="$OUT_DIR/${name}.qsb"

    echo "compiling $name → $outname"

    "$QSB_CMD" --glsl 450 --hlsl 50   \
        --output "$outname" "$file"
done

echo "✅ Shaders compiled."
