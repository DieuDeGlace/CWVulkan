#ifndef CUBEVULKAN_STEAM_COLOUR_GLSL
#define CUBEVULKAN_STEAM_COLOUR_GLSL
// Existing engine color helpers, verified against steam-world-shader-output.md §§3/6.
const vec3 LUMA = vec3(.299, .587, .114);
vec3 vanillaBoost(vec3 c) {
    float l = dot(LUMA, c);
    float boost = 1 + .25 * (1 - l);
    return boost * c + l * (1 - boost);
}
vec3 vanillaGrade(vec3 c) {
    c = clamp(c, 0, 1);
    float grey = dot(LUMA, c) * .35;
    c.yz = c.yz * .65 + grey;
    return c * c * (3 - 2 * c);
}

#endif
