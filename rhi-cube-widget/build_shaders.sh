#!/bin/bash

# Cesta ke vstupním shaderům
SHADER_DIR="shaders"
OUT_DIR="$SHADER_DIR/compiled"

# Vytvoř výstupní složku
mkdir -p "$OUT_DIR"

# Kompilace shaderů
for file in "$SHADER_DIR"/*.vert "$SHADER_DIR"/*.frag; do
    name=$(basename "$file")
    outname="$OUT_DIR/${name}.qsb"

    echo "Kompiluji $name → $outname"

    qsb --glsl 450 --hlsl 50 --msl 12 --metal --spirv \
        --output "$outname" "$file"
done

echo "✅ Shader kompilace hotová."
