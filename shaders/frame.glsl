#ifndef CUBEVULKAN_FRAME_GLSL
#define CUBEVULKAN_FRAME_GLSL
// Shared by every CubeVulkan shader. Must match rendergpu::FrameUniforms in RendererUniforms.h.
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
    // [5] = (gradient shift, title flag, underwater flag, bound WORLD fog scale).
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
    mat4 projection;    // separate WORLD lens; viewProj remains available to other passes
    mat4 originView;
    mat4 originProjection;
    vec4 homeToOrigin;
} f;
#endif
