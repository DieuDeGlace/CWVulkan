#version 450
// The Steam look's clouds (docs/specs/sky-and-clouds.md §B.7, the cloud pixel shader): the lit
// model colour, square-rooted and a little more saturated, fades into the sky behind it (the
// gradient and the sun's glow) by 3D distance, then takes the game's grading. pc.look.x is the
// cloud's opacity (it fades near the camera). Fog uses the frame's bound reciprocal
// and the view-space position from scene.vert.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "steam_sky.glsl"
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;
layout(location = 6) in vec3 vView;
layout(location = 0) out vec4 outColor;

// The Steam world's light on one face (alpha-rendering.md §14): a wrapped front term, and back
// and ambient light weighted by the face's turn against a fixed axis. The light colours are
// the Steam look's (DayNight.h); clouds carry no baked light.
const vec3 FACE_AXIS = vec3(.597, .398, .697);
vec3 faceLight(vec3 n) {
    float facing = max(.7 * dot(n, f.lightDir.xyz) + .3, 0);
    float h = .5 + .5 * abs(dot(n, FACE_AXIS));
    return clamp(facing * f.front.rgb + h * f.back.rgb + h * f.ambient.rgb, 0, 1);
}

void main() {
    vec3 K = vColor.rgb * faceLight(normalize(vNormal));
    vec3 lit = vanillaBoost(sqrt(K));
    float fogScale = f.steamSky[5].w;
    float fog = clamp(dot(vView, vView) * (fogScale * fogScale), 0, 1);
    float f3 = fog * fog * fog;
    float g = 1 - (1 - f3) * (1 - f3);
    vec2 screen = steamScreen();
    vec3 sky = steamSunGlow(steamSkyAt(screen), screen);
    outColor = vec4(vanillaGrade(clamp(mix(lit, sky, g), 0, 1)), pc.look.x);
}
