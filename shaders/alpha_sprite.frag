#version 450
// Stars take their vertex colour; the sun multiplies its colour by a round glow, white at the
// centre and yellow towards its clear edge (docs/specs/alpha-rendering.md §8.4). The moon is
// CubeVulkan's own (src/AlphaMoon.h): a pale pixel-art disc with a few darker craters, a
// faint halo, and its phase.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(location = 0) in vec4 vColour;
layout(location = 1) in vec2 vUv;
layout(location = 0) out vec4 outColor;

// The moon is drawn on a coarse grid so it reads as pixel art, like the blocks around it.
const float CELLS = 24;      // grid cells across the moon's quad
const float DISC = .6;       // the disc's radius (the quad is -1..1)
const vec3 MOON = vec3(.90, .93, 1.0);
const vec3 CRATER = vec3(.66, .70, .84);
// Crater spots on the disc: xy centre, z radius (disc units, -1..1 across the disc).
const vec3 CRATERS[5] = vec3[](vec3(-.35, .30, .26), vec3(.30, .42, .16), vec3(.20, -.30, .30),
                               vec3(-.42, -.35, .14), vec3(.52, -.02, .12));

vec4 moon() {
    // The centre of this pixel's grid cell, in quad units (-1..1).
    vec2 cell = (floor(vUv * CELLS) + .5) / CELLS * 2 - 1;
    float r = length(cell);
    if (r > DISC) {
        // A faint cool halo round the disc.
        float halo = 1 - clamp((r - DISC) / (1 - DISC), 0, 1);
        return vec4(.55, .65, .9, .18 * halo * halo);
    }
    vec2 p = cell / DISC; // on the disc, -1..1
    vec3 c = MOON;
    for (int i = 0; i < 5; i++)
        if (length(p - CRATERS[i].xy) < CRATERS[i].z)
            c = CRATER;
    c *= 1 - .12 * smoothstep(.75, 1, length(p)); // a slightly darker rim

    // The phase: the lit part grows from the right (waxing) to full, then shrinks to the
    // left (waning). The dark part keeps a dim earthshine so the disc never vanishes.
    float phase = f.alphaNight.w;
    float edge = cos(phase * 6.2831853) * sqrt(max(1 - p.y * p.y, 0));
    bool lit = phase < .5 ? p.x > edge : p.x < -edge;
    if (!lit)
        c = c * .18 + vec3(.02, .03, .06);
    return vec4(c, 1);
}

void main() {
    if (pc.params.x < .5) {
        outColor = vColour;
        return;
    }
    if (pc.params.x > 1.5) {
        vec4 m = moon();
        outColor = vec4(m.rgb, m.a * pc.params.y);
        return;
    }
    // The glow as a 256 x 256 texture would hold it, texel centres at half steps.
    float r = length(vUv * 256 - 128);
    float q = pow(clamp(1.05 - 1.05 * .0078125 * r, 0, 1), 4);
    vec4 glow = vec4(1, 1, (100 + 155 * q) / 255, q);
    outColor = vColour * glow;
}
