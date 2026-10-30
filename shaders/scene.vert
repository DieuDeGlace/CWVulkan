#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor; // Native encoded RGB/V, or legacy RGB/AO.
layout(location = 2) in uvec4 inNormalFlags;
layout(location = 0) out vec3 vWorld;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec4 vColor;
layout(location = 3) flat out uint vFlags;
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
        return;
    }
    vec3 world = (pc.model * vec4(inPosition, 1)).xyz;
    world = sway(world, inPosition, flags);
    vWorld = world;
    vec3 normal = (flags & 16u) != 0u
        ? normalize(vec3(inNormalFlags.x, inNormalFlags.z, inNormalFlags.w) / 127.5 - 1)
        : faceNormal(inNormalFlags.x);
    vNormal = normalize(mat3(pc.model) * normal);
    vColor = palette;
    vFlags = flags;
    gl_Position = f.viewProj * vec4(world, 1);
}
