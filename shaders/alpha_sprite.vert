#version 450
// The Alpha look's stars and sun sprite (docs/specs/alpha-rendering.md §8.3, §8.4), placed
// with the sky camera (the view's rotation and a 50 degree lens) at the far plane, so the
// world covers them.
//   pc.params.x = 0: one star per instance, a fan of four triangles, opaque white at the
//                    centre and clear at the corners: (+-1, +-1) x size added to the star's
//                    point in view space, before the lens (docs/specs/sky-and-clouds.md §A.3),
//                    so stars are a few pixels at most and have no minimum size.
//   pc.params.x = 1: the sun, a quad 80 x 80 in view space around 100 x the sun direction.
//   pc.params.x = 2: CubeVulkan's moon (src/AlphaMoon.h, not in the spec), a smaller quad
//                    opposite the sun; pc.params.y is how visible it is.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(location = 0) in vec4 inStar; // xyz on the sky sphere, w size
layout(location = 0) out vec4 vColour;
layout(location = 1) out vec2 vUv;

const float MOON_HALF_SIZE = 9; // the halo's edge; the disc is 0.6 of it
const vec2 CORNERS[4]= vec2[](vec2(-1, -1), vec2(1, -1), vec2(1, 1), vec2(-1, 1));

void main() {
    if (pc.params.x < .5) {
        int triangle = gl_VertexIndex / 3, k = gl_VertexIndex % 3;
        vec3 centre = mat3(f.view) * inStar.xyz;
        if (centre.z >= 0) { // behind the camera
            gl_Position = vec4(0, 0, 2, 1);
            vColour = vec4(0);
            vUv = vec2(0);
            return;
        }
        vec2 offset = k == 0 ? vec2(0) : CORNERS[(triangle + k - 1) % 4] * inStar.w;
        vec4 clip = f.skyProj * vec4(centre + vec3(offset, 0), 1);
        gl_Position = vec4(clip.xy, clip.w, clip.w);
        vColour = vec4(1, 1, 1, k == 0 ? 1 : 0);
        vUv = vec2(0);
        return;
    }
    const vec2 QUAD[6] = vec2[](vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 0), vec2(1, 1), vec2(0, 1));
    vec2 uv = QUAD[gl_VertexIndex % 6];
    // The moon (CubeVulkan's own) sits opposite the sun, on a smaller quad.
    bool moon = pc.params.x > 1.5;
    vec3 centre = mat3(f.view) * (100 * (moon ? -f.alphaSun.xyz : f.alphaSun.xyz));
    vec4 clip = f.skyProj * vec4(centre + vec3((uv * 2 - 1) * (moon ? MOON_HALF_SIZE : 40), 0), 1);
    gl_Position = vec4(clip.xy, clip.w, clip.w);
    vColour = f.sunColour;
    vUv = uv;
}
