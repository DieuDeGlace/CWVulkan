#version 450
// Resolved image to the screen when SMAA is off.
layout(set = 0, binding = 0) uniform sampler2D source;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;
void main() { outColor = textureLod(source, vUv, 0); }
