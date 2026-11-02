#version 450
// Steam map regions share the full WORLD pixel contract, including the retained
// raw sky/sun/gradient inputs (steam-map-world-pixel-contract.md).
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"
#include "steam_pixel.glsl"
layout(location = 0) in vec3 vColor;
layout(location = 1) in vec3 vView;
layout(location = 0) out vec4 outColor;

// Must match MapPush in src/RendererMap.cpp.
layout(push_constant) uniform MapPush {
    mat4 modelView;
    vec4 lens;
    vec4 turn;
    vec4 material;
    vec4 fog; // rgb compatibility fog; w selects the full Steam WORLD pixel stages
} map;

const float FOG_SCALE = 1.0 / 20000.0; // the map's fog distance

vec3 smoothCurve(vec3 x) { return x * x * (3.0 - 2.0 * x); }

void main() {
    if (map.fog.w > .5) {
        outColor = vec4(steamPixelColour(vColor, steamScreen(), vView, vec2(0), 0, FOG_SCALE), map.turn.z);
        return;
    }
    // Retain the Alpha appearance option's existing compatibility map coloring.
    vec3 c = vColor;
    float grey = dot(LUMA, c);
    float lift = 1.0 + 0.25 * (1.0 - grey);
    c = lift * c + (1.0 - lift) * grey;
    // The fog (its second step toward the sky, a millionth here, is left out).
    float q = min(1.0, pow(dot(vView, vView) * FOG_SCALE * FOG_SCALE, 1.0 - 0.75 * exp(-403.95462 * FOG_SCALE)));
    c = mix(c, map.fog.rgb, q * q * (3.0 - 2.0 * q));
    c = clamp(c, 0.0, 1.0);
    grey = dot(LUMA, c);
    c.g = 0.65 * c.g + 0.35 * grey;
    c.b = 0.65 * c.b + 0.35 * grey;
    outColor = vec4(smoothCurve(c), map.turn.z);
}
