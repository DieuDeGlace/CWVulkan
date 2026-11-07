#ifndef CUBEVULKAN_STEAM_WATER_GLSL
#define CUBEVULKAN_STEAM_WATER_GLSL
#include "steam_colour.glsl"
// Pixel contract: steam-native-water-shader-20261107.md. Inputs are interpolated
// clip/view values; lit blue already includes the shared vertex light solve.
struct SteamWaterState {
    vec4 sky1, sky2, fog, sun, sunPosition;
    vec4 controls; // fog scale, vertical translation, pixel alpha, unused
};
float waterCurve(float x) { return x * x * (3 - 2 * x); }
vec4 steamWaterPixel(float blue, vec4 clip, vec3 normal, vec3 direction, vec3 position, SteamWaterState state) {
    vec2 screen = clip.xy / clip.w;
    float y = screen.y + state.controls.y;
    vec3 top = mix(state.sky1.rgb, state.sky2.rgb, waterCurve(clamp(.4 * y + .2, 0, 1)));
    vec3 bottom = .4 * state.sky1.rgb + .6 * state.fog.rgb;
    vec3 sky = mix(bottom, top, waterCurve(clamp(2 * y + 1.5, 0, 1)));
    vec2 delta = vec2((state.sunPosition.x - screen.x) * state.sunPosition.z, state.sunPosition.y - y);
    float sun = (1 - min(4 * dot(delta, delta), 1)) * state.sunPosition.w;
    vec3 skySun = mix(sky, state.sun.rgb, sun * sun * sun * sun);
    vec3 body = mix(state.sky1.rgb, state.sky2.rgb, .8 * waterCurve(clamp(y + 1, 0, 1)));
    vec3 reflected = normalize(2 * dot(normal, direction) * normal - direction);
    float sheen = max(dot(reflected, -normalize(position)), 0);
    vec3 colour = clamp(blue * body + .25 * sheen * state.sky1.rgb, 0, 1);
    float distance2 = dot(position, position);
    float scale = state.controls.x;
    float exponent = 1 - .75 * exp2(-403.954620361328125 * scale);
    float fog = waterCurve(min(pow(distance2 * (scale * scale), exponent), 1));
    colour = mix(colour, state.fog.rgb, fog);
    colour = clamp(mix(colour, skySun, fog * fog * fog), 0, 1);
    float opacity = min(.75 + min(.000025 * distance2, 1), 1) * state.controls.z;
    return vec4(vanillaGrade(colour), opacity);
}
#endif
