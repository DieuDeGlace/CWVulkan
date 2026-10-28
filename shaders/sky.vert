#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(location = 0) out vec2 vNdc;
void main() {
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2 - 1;
    vNdc = p;
    gl_Position = vec4(p, 1, 1); // Far plane; drawn with depth test LESS_OR_EQUAL.
}
