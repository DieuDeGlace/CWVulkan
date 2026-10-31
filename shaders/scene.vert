#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "steam_vertex.glsl"
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
layout(location = 7) out vec2 vViewNormal;
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
        vViewNormal = vec2(0);
        return;
    }
    vec3 world = (pc.model * vec4(inPosition, 1)).xyz;
    if ((flags & GUI_WORLD) == 0u)
        world = sway(world, inPosition, flags);
    vWorld = world;
    vec3 normal = (flags & 16u) != 0u
        ? normalize(vec3(inNormalFlags.x, inNormalFlags.z, inNormalFlags.w) / 127.5 - 1)
        : faceNormal(inNormalFlags.x);
    vNormal = normalize(mat3(pc.model) * normal);
    vColor = palette;
    vIndirect = palette.rgb;
    if ((flags & (STEAM_VERTEX | STEAM_WORLD)) == (STEAM_VERTEX | STEAM_WORLD)) {
        steamVertexColours(world, vNormal, palette, flags, vColor.rgb, vIndirect);
    }
    vFlags = flags;
    gl_Position = f.viewProj * vec4(world, 1);
    if ((flags & GUI_WORLD) != 0u)
        gl_Position.z -= pc.params.z; // native shader DepthOffset, not NDC translation
    // Preserve homogeneous coordinates for division after raster interpolation.
    vClip = vec4(gl_Position.x, -gl_Position.y, gl_Position.zw);
    vView = (f.view * vec4(world, 1)).xyz;
    vec3 viewNormal = normalize(mat3(f.view * pc.model) * normal);
    vViewNormal = vec2(-viewNormal.x, viewNormal.y); // native LH view, negative lens aspect
}
