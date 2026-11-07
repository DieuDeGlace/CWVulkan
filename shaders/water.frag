#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "lighting.glsl"
#include "steam_water.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;
layout(location = 3) flat in uint vFlags;
layout(location = 4) in vec3 vIndirect;
layout(location = 5) in vec4 vClip;
layout(location = 6) in vec3 vView;
layout(location = 7) in vec3 vViewNormal;
layout(location = 8) in vec3 vViewLight;
layout(location = 0) out vec4 outColor;
void main() {
    vec3 normal = vec3(0, 0, 1);
    bool nativeVertex = (vFlags & (STEAM_VERTEX | STEAM_WORLD)) == (STEAM_VERTEX | STEAM_WORLD);
    float blue;
    if (nativeVertex)
        blue = mix(vIndirect.b, vColor.b, sunShare(vWorld, normal)); // Optional engine shadows retain the vertex light result.
    else {
        Shade light = shade(vWorld, normal, true);
        blue = clamp(vColor.b * light.light.b, 0, 1);
    }
    SteamWaterState state = SteamWaterState(f.steamSky[0], f.steamSky[1], f.steamSky[2], f.steamSky[3], f.steamSky[4],
                                           vec4(f.steamSky[5].w, f.steamSky[5].x, pc.params.w, 0));
    outColor = nativeVertex
        ? steamWaterPixel(blue, vClip, vViewNormal, vViewLight, vView, state)
        : steamWaterPixel(blue, vec4(steamScreen(), 0, 1), normal, f.lightDir.xyz, vWorld - f.eye.xyz, state);
}
