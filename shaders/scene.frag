#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "lighting.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;
layout(location = 3) flat in uint vFlags;
layout(location = 0) out vec4 outColor;
void main() {
    if ((vFlags & 8u) != 0u) {
        outColor = vec4(vColor.rgb, (vFlags & 64u) != 0u ? vColor.a : 1);
        return;
    }
    vec3 albedo = vColor.rgb * (f.steamSky[5].z > .5 ? vec3(.5, .5, 1) : vec3(1));
    if ((vFlags & GLOW) != 0u) {
        outColor = vec4(vanillaGrade(applyFog(albedo, vWorld)), 1);
        return;
    }
    // Vanilla terrain has only light corner darkening.
    float ao = mix(.78, 1, vColor.a);
    Shade s = shade(vWorld, normalize(vNormal), true);
    vec3 color = vanillaBoost(clamp(albedo * s.light * ao + s.add, 0, 1));
    outColor = vec4(vanillaGrade(applyFog(color, vWorld)), 1);
}
