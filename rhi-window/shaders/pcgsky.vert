#version 450
// Nepotřebujeme "in" proměnné, kreslíme pomocí gl_VertexID

// Výstup do fragment shaderu
layout(location = 0) out vec2 v_clipSpace;

void main() {
    // Vytváří trojúhelník pokrývající celou obrazovku pomocí gl_VertexID
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = -1.0 + float((gl_VertexIndex & 2) << 1);
    
    v_clipSpace = vec2(x, y);
    gl_Position = vec4(v_clipSpace, 1.0, 1.0);
}
