#version 450
// The Alpha look's cloud pixel shader (docs/specs/alpha-rendering.md §6.3): the lit cloud
// colour blends into the sky gradient behind it by 3D distance, late and then quickly
// (g = 1 - (1 - f^5)^2), so near clouds keep their shading and far ones melt into the sky.
// The vertex shader (alpha_scene.vert, pc.look.w = 1) already used the cloud pass's longer
// fog distance. Clouds take no shadows.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "alpha.glsl"
layout(location = 2) in vec4 vLit;
layout(location = 4) in vec4 vFog;
layout(location = 0) out vec4 outColor;

void main() {
    float f5 = pow(saturate(vFog.z), 5);
    float g = 1 - (1 - f5) * (1 - f5);
    outColor = vec4(mix(vLit.rgb, alphaSkyAt(vFog.y), g), 1);
}
