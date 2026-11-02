#version 450
// The interface atlas (the game's HUD art and font glyphs), tinted per vertex.
layout(set = 0, binding = 0) uniform sampler2D atlas;
layout(set = 0, binding = 1) uniform sampler2D layerMask;
layout(push_constant) uniform Screen { vec2 size; int maskMode; float maskBias; } screen;
layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec2 vMask;
layout(location = 0) out vec4 outColor;
void main() {
    vec4 colour = texture(atlas, vUv) * vColor;
    if (screen.maskMode != 0) {
        vec4 mask = texture(layerMask, vMask);
        if (mask.a > 0)
            mask.rgb /= mask.a;
        if (screen.maskMode == 1)
            colour.rgb = mix(colour.rgb, mask.rgb, mask.a);
        else if (screen.maskMode == 2)
            colour *= mask;
        else
            colour.a *= mask.a;
    }
    outColor = colour;
}
