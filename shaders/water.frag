#version 450
// From docs/specs/water-rendering.md B.3-B.4; no native shader source is used.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "lighting.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;
layout(location = 3) flat in uint vFlags;
layout(location = 0) out vec4 outColor;
void main() {
    vec2 screen = steamScreen();
    float Y = screen.y + f.steamSky[5].x;
    vec3 toEye = f.eye.xyz - vWorld;
    float distance2 = dot(toEye, toEye);
    vec3 normal = vec3(0, 0, 1);
    Shade light = shade(vWorld, normal, true);
    float blue = clamp(vColor.b * light.light.b, 0, 1);
    vec3 body = mix(f.steamSky[0].rgb, f.steamSky[1].rgb, .8 * steamSmooth(Y + 1));
    vec3 reflected = reflect(-f.lightDir.xyz, normal);
    float sheen = max(dot(normalize(reflected), normalize(toEye)), 0);
    vec3 colour = clamp(blue * body + sheen * .25 * f.steamSky[0].rgb, 0, 1);
    float scale = 1 / max(f.fog.w, .001);
    float exponent = 1 - .75 * exp2(-403.95462 * scale);
    float haze = steamSmooth(min(pow(distance2 * scale * scale, exponent), 1));
    colour = mix(colour, f.steamSky[2].rgb, haze);
    colour = clamp(mix(colour, steamSunGlow(steamSkyAt(screen), screen), haze * haze * haze), 0, 1);
    outColor = vec4(vanillaGrade(colour), min(.75 + min(.000025 * distance2, 1), 1));
}
