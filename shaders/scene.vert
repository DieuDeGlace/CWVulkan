#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "steam_vertex.glsl"
#include "steam_transform.glsl"
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor; // Native encoded RGB/V, or legacy RGB/AO.
layout(location = 2) in uvec4 inNormalFlags;
layout(location = 0) out vec3 vWorld;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec4 vColor;
layout(location = 3) flat out uint vFlags;
layout(location = 4) out vec3 vIndirect;
layout(location = 5) out vec4 vClip;
layout(location = 6) out vec3 vView;
layout(location = 7) out vec3 vViewNormal;
layout(location = 8) out vec3 vViewLight;
void main() {
    uint flags = inNormalFlags.y | uint(pc.params.x);
    vec4 palette = inColor;
    if ((flags & STEAM_VERTEX) != 0u)
        palette.rgb *= 2;
    if ((flags & 8u) != 0u) {
        gl_Position = pc.model * vec4(inPosition, 1);
        vWorld = vec3(0);
        vNormal = vec3(0, 0, 1);
        vColor = palette;
        vFlags = flags;
        vIndirect = palette.rgb;
        vClip = gl_Position;
        vView = vec3(0);
        vViewNormal = vec3(0);
        vViewLight = vec3(0);
        return;
    }
    vec3 world = (pc.model * vec4(inPosition, 1)).xyz;
    if ((flags & GUI_WORLD) == 0u)
        world = sway(world, inPosition, flags);
    bool originWorld = (flags & ORIGIN_WORLD) != 0u;
    vWorld = originWorld ? world - f.homeToOrigin.xyz : world;
    vec3 normal = (flags & 16u) != 0u
        ? normalize(vec3(inNormalFlags.x, inNormalFlags.z, inNormalFlags.w) / 127.5 - 1)
        : faceNormal(inNormalFlags.x);
    vNormal = normalize(mat3(pc.model) * normal);
    vColor = palette;
    vIndirect = palette.rgb;
    if ((flags & (STEAM_VERTEX | STEAM_WORLD)) == (STEAM_VERTEX | STEAM_WORLD)) {
        vec3 lightPosition = originWorld ? world : world + f.homeToOrigin.xyz;
        steamVertexColours(lightPosition, vNormal, palette, flags, vColor.rgb, vIndirect);
    }
    vFlags = flags;
    // WORLD composes the independent GPU matrices before its local-position dots.
    // Lighting still consumes the separately transformed WORLD point above.
    mat4 activeView = originWorld ? f.originView : f.view;
    bool nativeWorld = (flags & STEAM_WORLD) != 0u;
    mat4 worldView = nativeWorld ? steamCompose(activeView, pc.model) : activeView * pc.model;
    bool depthOffset = (flags & (GUI_WORLD | QUEUED_WORLD | WORLD_DEPTH_OFFSET)) != 0u;
    bool leafSway = (flags & GUI_WORLD) == 0u && (flags & SWAY) != 0u && !(f.wind.w < .5);
    vec4 viewPoint;
    if (nativeWorld && !leafSway) {
        mat4 projection = originWorld ? f.originProjection : f.projection;
        mat4 clipTransform = steamCompose(projection, worldView);
        if (depthOffset)
            clipTransform[3][2] -= pc.params.z;
        viewPoint = worldView * vec4(inPosition, 1);
        gl_Position = clipTransform * vec4(inPosition, 1);
    } else {
        // Optional engine leaf deformation occurs after WORLD; retain that path.
        // Native plant motion is already part of its submitted WORLD matrix.
        viewPoint = activeView * vec4(world, 1);
        if (originWorld)
            gl_Position = f.originProjection * viewPoint;
        else if (nativeWorld)
            gl_Position = f.projection * viewPoint;
        else
            gl_Position = f.viewProj * vec4(world, 1);
        if (depthOffset)
            gl_Position.z -= pc.params.z;
    }
    // Preserve homogeneous coordinates for division after raster interpolation.
    vClip = vec4(gl_Position.x, -gl_Position.y, gl_Position.zw);
    // The native camera basis reverses the generic right-handed X/Z axes.
    // Keep all shared water reflection/distance varyings in that same basis.
    vView = originWorld ? viewPoint.xyz : vec3(-viewPoint.x, viewPoint.y, -viewPoint.z);
    vec3 viewNormal = normalize(mat3(worldView) * normal);
    vec3 viewLight = mat3(worldView) * f.steamSun.xyz;
    vViewNormal = originWorld ? viewNormal : vec3(-viewNormal.x, viewNormal.y, -viewNormal.z);
    vViewLight = originWorld ? viewLight : vec3(-viewLight.x, viewLight.y, -viewLight.z);
}
