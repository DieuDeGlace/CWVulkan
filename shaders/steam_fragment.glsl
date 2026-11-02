// Ordinary scene/GUI draw inputs for the shared WORLD pixel stages.
#include "steam_pixel.glsl"
vec3 steamWorldPixel(vec3 colour, vec2 screen, vec3 viewPosition, vec2 viewNormal) {
    return steamPixelColour(colour, screen, viewPosition, viewNormal, pc.look.z, f.steamSky[5].w);
}
