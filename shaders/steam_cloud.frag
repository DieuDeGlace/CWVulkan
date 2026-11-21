#version 450
// Steam mode 10. The vertex stage lights the baked model colors. Fog uses the
// frame's reciprocal and interpolated view position. Draw opacity is independent.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "steam_sky.glsl"
layout(location = 2) in vec4 vColor;
layout(location = 5) in vec4 vClip;
layout(location = 6) in vec3 vView;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 lit = vanillaBoost(sqrt(vColor.rgb));
    float fogScale = pc.look.y > 0 ? pc.look.y : f.steamSky[5].w;
    float fog = clamp(dot(vView, vView) * (fogScale * fogScale), 0, 1);
    float unfogged = 1 - fog * fog * fog;
    float weight = 1 - unfogged * unfogged;
    vec2 screen = vClip.xy / vClip.w;
    vec3 sky = steamSunGlow(steamSkyAt(screen), screen);
    outColor = vec4(vanillaGrade(clamp(mix(lit, sky, weight), 0, 1)), pc.look.z);
}
