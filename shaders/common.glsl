// Shared by every CubeVulkan shader. Must match FrameUniforms in Renderer.cpp.
layout(set = 0, binding = 0) uniform Frame {
    mat4 viewProj;
    mat4 invViewProj;
    mat4 lightFar;
    mat4 lightNear;
    vec4 eye;        // xyz camera position, w time in seconds
    vec4 lightDir;   // xyz towards the shadow light, w shadow strength
    vec4 front;      // rgb native front light, w sun warmth
    vec4 back;       // rgb native back light, w fade start (fraction of radius)
    vec4 ambient;    // rgb ambient, w camera fade (0/1)
    vec4 fog;        // rgb fog colour, w fog end distance
    vec4 sky;        // w daylight (rgb unused)
    vec4 shadowFar;  // texel size, depth bias, radius, 1 / depth span
    vec4 shadowNear; // texel size, depth bias, radius, enabled
    vec4 wind;       // the leaf-sway option: amplitude (blocks), speed, range, enabled
    vec4 focus;      // xyz player position, w lantern radius (0 = off)
    vec4 sunDisk;    // xyz direction to the sun, w visibility
    vec4 moonDisk;   // xyz direction to CubeVulkan's moon (Steam look), w visibility
    // The Steam look's sky (steam_sky.glsl): sky1, sky2, fog, sun colour, sun position, and
    // [5] = (gradient shift, 1 on the title screen, 0, 1).
    vec4 steamSky[6];
    // The Alpha look (docs/specs/alpha-rendering.md; src/AlphaLook.h).
    vec4 alphaSky1;     // rgb horizon side of the sky gradient
    vec4 alphaSky2;     // rgb zenith side
    vec4 alphaFog;      // rgb distance haze
    vec4 alphaFront;    // rgb sun colour
    vec4 alphaBack;     // rgb light on faces turned away from the sun
    vec4 alphaAmbient;  // rgb light on every face
    vec4 alphaMaterial; // rgb tint every draw is multiplied by, a sunlight strength S
    vec4 alphaSun;      // xyz direction towards the sun, w 1 / fog distance
    vec4 alphaView;     // x sky gradient shift with the camera's tilt, yz one pixel in screen units
    vec4 sunColour;     // the sun sprite's colour
    // CubeVulkan's moon in the Alpha look (src/AlphaMoon.h); towards it is -alphaSun.xyz.
    vec4 alphaMoonFront; // rgb moonlight on faces turned to the moon, w its strength M
    vec4 alphaMoonBack;  // rgb moonlight on faces turned away, w 1 when the moon casts the shadows
    vec4 alphaNight;     // rgb night ambient, w the moon's phase (0 new, .5 full)
    mat4 skyProj;       // the sky camera's lens, for the stars and the sun sprite
    mat4 view;          // the camera's view matrix
    vec4 pointLights[64];  // xyz position, w radius (blocks)
    vec4 pointColours[64]; // rgb colour
    vec4 steamSun;      // xyz submitted native sunlight, w native ambient alpha
    vec4 steamMaterial; // frame material RGB and sunlight alpha (not pixel opacity)
} f;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 params; // x instance flags, y shadow cascade (0 far, 1 near), z model height (voxels), w pixel opacity (separate from material light)
    vec4 look;   // Alpha look: x light where the object stands (0..1), y hit flash, z shimmer, w 1 for clouds;
                 // Steam look's star and cloud passes: x opacity, y fog scale
    uvec4 lights; // Alpha look: up to 10 point-light indices, 8 bits each in x..z; w count
} pc;

const uint SWAY = 1u, GLOW = 2u, WATER = 4u, STEAM_VERTEX = 128u, LAVA_LIGHTING = 256u, STEAM_WORLD = 512u;

// Vanilla's colour stages (Steam world pixel shader, as re-expressed in
// SteamShadows' InlineTerrain.h). Lit surface colours get a saturation boost
// before fog; every final colour then gets a grey mix into green and blue and
// an S-curve. src/Palette.h inverts these to derive block colours.
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
