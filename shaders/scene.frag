#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "lighting.glsl"
#include "steam_fragment.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;
layout(location = 3) flat in uint vFlags;
layout(location = 4) in vec3 vIndirect;
layout(location = 5) in vec4 vClip;
layout(location = 6) in vec3 vView;
layout(location = 7) in vec2 vViewNormal;
layout(location = 0) out vec4 outColor;
void main() {
    if ((vFlags & 8u) != 0u) {
        outColor = vec4(vColor.rgb, (vFlags & 64u) != 0u ? vColor.a : 1);
        return;
    }
    if ((vFlags & (STEAM_VERTEX | STEAM_WORLD)) == (STEAM_VERTEX | STEAM_WORLD)) {
        vec3 colour = mix(vIndirect, vColor.rgb, sunShare(vWorld, normalize(vNormal)));
        outColor = vec4(submergedSceneColour(steamWorldPixel(colour, vClip.xy / vClip.w, vView, vViewNormal)), pc.params.w);
        return;
    }
    vec3 albedo = vColor.rgb * (f.steamSky[5].z > .5 ? f.steamMaterial.rgb : vec3(1));
    if ((vFlags & GLOW) != 0u) {
        outColor = vec4(submergedSceneColour(vanillaGrade(applyFog(albedo, vWorld))), pc.params.w);
        return;
    }
    // Vanilla terrain has only light corner darkening.
    float ao = mix(.78, 1, vColor.a);
    Shade s = shade(vWorld, normalize(vNormal), true, pc.look.x);
    vec3 color = vanillaBoost(clamp(albedo * s.light * ao + s.add, 0, 1));
    outColor = vec4(submergedSceneColour(vanillaGrade(applyFog(color, vWorld))), pc.params.w);
}
