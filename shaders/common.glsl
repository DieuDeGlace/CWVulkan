#include "frame.glsl"

layout(push_constant) uniform Push {
    mat4 model;
    vec4 params; // x instance flags, y shadow cascade (0 far, 1 near), z model height (GUI: clip depth offset), w pixel opacity (separate from material light)
    vec4 look;   // Alpha look: x light where the object stands (0..1), y hit flash, z shimmer, w 1 for clouds;
                 // Steam look's star and cloud passes: x opacity, y fog scale. Immediate GUI: w is float Glow.
    uvec4 lights; // Alpha look: up to 10 point-light indices, 8 bits each in x..z; w count
    vec4 material; // instance material RGBA; identity for ordinary draws
} pc;

const uint SWAY = 1u, GLOW = 2u, WATER = 4u, STEAM_VERTEX = 128u, LAVA_LIGHTING = 256u, STEAM_WORLD = 512u, GUI_WORLD = 1024u;
const uint ORIGIN_WORLD = 2048u, QUEUED_WORLD = 4096u, WORLD_DEPTH_OFFSET = 8192u;

#include "steam_colour.glsl"

vec3 faceNormal(uint id) {
    return id == 0u ? vec3(1, 0, 0) : id == 1u ? vec3(-1, 0, 0) : id == 2u ? vec3(0, 1, 0)
         : id == 3u ? vec3(0, -1, 0) : id == 4u ? vec3(0, 0, 1) : vec3(0, 0, -1);
}

// The WorldMotion mod's leaf sway, a CubeVulkan option ([Motion] LeafSway, off by default):
// vanilla never moves a block (docs/specs/vegetation-wind.md §1). Only leaf blocks carry SWAY;
// the game's own plant sway is in the model matrix (src/VegetationWind.h). Applied identically
// in the shadow pass, so moving leaves and their shadows agree.
vec3 sway(vec3 world, vec3 local, uint flags) {
    if ((flags & SWAY) == 0u || f.wind.w < .5)
        return world;
    float d = length(world.xy - f.eye.xy);
    float reach = 1 - smoothstep(f.wind.z * .75, f.wind.z, d);
    if (reach <= 0)
        return world;
    float t = f.eye.w * f.wind.y;
    float gust = .65 + .35 * sin(t * .23 + world.x * .011 + world.y * .007);
    float phase = t + world.x * .37 + world.y * .29;
    vec2 offset = vec2(sin(phase), cos(phase * .83 + 1.3)) * f.wind.x * gust;
    // Instanced plants bend from their base; terrain leaves move as a whole.
    float bend = pc.params.z > 0 ? clamp(local.z / pc.params.z, 0, 1) : 1;
    return world + vec3(offset * bend * bend * reach, 0);
}

// Shared underwater art direction, applied to world fragments before the HUD.
// A blue contribution from brightness keeps green banks and plants teal too;
// multiplying their near-zero blue channel alone cannot produce that cast.
vec3 submergedSceneColour(vec3 colour) {
    if (f.alphaView.w < .5) return colour;
    float brightness = dot(max(colour, vec3(0)), vec3(.299, .587, .114));
    return vec3(colour.r * .55, colour.g * .88, max(colour.b, brightness * .95));
}
