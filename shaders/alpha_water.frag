#version 450
// The Alpha look's water (docs/specs/alpha-rendering.md §6.4 and §12): only the lit blue
// channel is kept, turned into a blue that deepens towards the bottom of the screen, with a
// sky-coloured glint, haze and sky by distance, and an opacity that grows with distance.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "lighting.glsl"
#include "alpha.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vLit;
layout(location = 3) in vec3 vShaded;
layout(location = 4) in vec4 vFog;
layout(location = 5) flat in uint vFlags;
layout(location = 7) in vec3 vToCamera;
layout(location = 0) out vec4 outColor;

void main() {
    float blue = mix(vShaded.b, vLit.b, sunShare(vWorld, normalize(vNormal)));
    float Y = vFog.y;
    vec3 sky = alphaSkyAt(Y);
    // The glint: a half vector between the camera and the sun lifted upwards.
    vec3 halfway = normalize(normalize(f.alphaSun.xyz + vec3(0, 0, 2)) + normalize(vToCamera));
    float glint = pow(saturate(halfway.z), 10) * vLit.a;
    vec3 body = vec3(0, .6 - .5 * saturate(Y + 1), 1);
    vec3 c = blue * body + glint * f.alphaSky1.rgb;
    c = mix(c, f.alphaFog.rgb, saturate(vFog.z));
    c = mix(c, sky, saturate(2 * vFog.z - 1));
    float opacity = min(.8 + .0001 * dot(vToCamera, vToCamera), 1);
    outColor = vec4(c, opacity);
}
