#version 450
#extension GL_GOOGLE_include_directive : require
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaa_common.glsl"
layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D blendTex;
layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vOffset;
layout(location = 0) out vec4 outColor;
void main() { outColor = SMAANeighborhoodBlendingPS(vUv, vOffset, colorTex, blendTex); }
