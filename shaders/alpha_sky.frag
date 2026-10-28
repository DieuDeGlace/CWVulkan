#version 450
// The Alpha look's sky (docs/specs/alpha-rendering.md §6.1, §8.1): the screen-space gradient
// alone. The sun and the stars are sprites drawn over it (alpha_sprite.*).
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "alpha.glsl"
layout(location = 0) in vec2 vNdc;
layout(location = 0) out vec4 outColor;
void main() {
    // Vulkan's screen y points down; the gradient's Y points up.
    outColor = vec4(alphaSkyAt(-vNdc.y + f.alphaView.x), 1);
}
