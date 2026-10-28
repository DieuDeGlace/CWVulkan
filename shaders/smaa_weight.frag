#version 450
#extension GL_GOOGLE_include_directive : require
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaa_common.glsl"
layout(set = 0, binding = 0) uniform sampler2D edgesTex;
layout(set = 0, binding = 1) uniform sampler2D areaTex;
layout(set = 0, binding = 2) uniform sampler2D searchTex;
layout(location = 0) in vec2 vUv;
layout(location = 1) in vec2 vPixel;
layout(location = 2) in vec4 vOffset[3];
layout(location = 0) out vec4 outWeights;
void main() { outWeights = SMAABlendingWeightCalculationPS(vUv, vPixel, vOffset, edgesTex, areaTex, searchTex, vec4(0)); }
