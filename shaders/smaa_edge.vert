#version 450
#extension GL_GOOGLE_include_directive : require
#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0
#include "smaa_common.glsl"
layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vOffset[3];
void main() {
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vUv = p;
    gl_Position = vec4(p * 2 - 1, 0, 1);
    SMAAEdgeDetectionVS(vUv, vOffset);
}
