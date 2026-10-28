#version 450
// The Steam look's sky (docs/specs/sky-and-clouds.md §B.5): the screen-space gradient of the
// palette's colours, the sun as a glow inside it, and the game's grading. The title screen fills
// the same constants from the game's own menu draw. The stars are cube models drawn over it
// (§B.6, Renderer.cpp) and the clouds come after the world (§B.7).
//
// The moon disc is CubeVulkan's own addition (the Steam release has no moon): MoonLight's moon,
// following the same clock as its moonlight (DayNight.h).
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "steam_sky.glsl"
layout(location = 0) in vec2 vNdc;
layout(location = 0) out vec4 outColor;
void main() {
    // Vulkan's screen y points down; the sky's points up.
    vec2 screen = vec2(vNdc.x, -vNdc.y);
    vec3 color = steamSunGlow(steamSkyAt(screen), screen);

    // CubeVulkan's moon (not in the game), only in the game world, not on the title screen.
    if (f.moonDisk.w > 0 && f.steamSky[5].y < .5) {
        vec4 a = f.invViewProj * vec4(vNdc, 0, 1), b = f.invViewProj * vec4(vNdc, 1, 1);
        vec3 dir = normalize(b.xyz / b.w - a.xyz / a.w);
        float moon = dot(dir, f.moonDisk.xyz);
        color = mix(color, vec3(.86, .9, 1), smoothstep(.9993, .9996, moon) * f.moonDisk.w);
        color += vec3(.5, .6, .8) * pow(max(moon, 0), 400) * .25 * f.moonDisk.w;
    }
    outColor = vec4(vanillaGrade(clamp(color, 0, 1)), 1);
}
