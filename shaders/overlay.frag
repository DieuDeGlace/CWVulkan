#version 450
// The interface atlas (the game's HUD art and font glyphs), tinted per vertex.
layout(set = 0, binding = 0) uniform sampler2D atlas;
layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = texture(atlas, vUv) * vColor;
}
