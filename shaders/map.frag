#version 450
// The world map's pixel shader (docs/specs/worldmap-rendering.md §3.3; the CPU twin is
// src/MapLook.cpp): a saturation lift, the faint far fog, a partial grey in green and blue,
// and an S-curve. Alpha is the instance's opacity (a new region's fade-in).
layout(location = 0) in vec3 vColor;
layout(location = 1) in vec3 vView;
layout(location = 0) out vec4 outColor;

// Must match MapPush in src/RendererMap.cpp.
layout(push_constant) uniform MapPush {
    mat4 modelView;
    vec4 lens;
    vec4 turn;
    vec4 material;
    vec4 fog;
} map;

const vec3 LUMA = vec3(0.299, 0.587, 0.114);
const float FOG_SCALE = 1.0 / 20000.0; // the map's fog distance

vec3 smoothCurve(vec3 x) { return x * x * (3.0 - 2.0 * x); }

void main() {
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
