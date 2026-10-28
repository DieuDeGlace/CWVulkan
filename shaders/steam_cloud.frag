#version 450
// The Steam look's clouds (docs/specs/sky-and-clouds.md §B.7, the cloud pixel shader): the lit
// model colour, square-rooted and a little more saturated, fades into the sky behind it (the
// gradient and the sun's glow) by 3D distance, then takes the game's grading. pc.look.x is the
// cloud's opacity (it fades near the camera), pc.look.y the pass's fog scale. Used with
// scene.vert.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "steam_sky.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;
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
    vec3 toCamera = f.eye.xyz - vWorld;
    float fog = clamp(dot(toCamera, toCamera) * pc.look.y * pc.look.y, 0, 1);
    float f3 = fog * fog * fog;
    float g = 1 - (1 - f3) * (1 - f3);
    vec2 screen = steamScreen();
    vec3 sky = steamSunGlow(steamSkyAt(screen), screen);
    outColor = vec4(vanillaGrade(clamp(mix(lit, sky, g), 0, 1)), pc.look.x);
}
