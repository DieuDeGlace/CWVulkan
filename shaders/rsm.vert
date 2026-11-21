#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 colour;
layout(location = 2) in uvec4 face;
layout(location = 0) out vec4 reflectance;
void main() {
    uint flags = face.y | uint(pc.params.x);
    vec3 world = sway((pc.model * vec4(position, 1)).xyz, position, flags);
    gl_Position = f.sunMap * vec4(world, 1);
    // The Steam mesh packs half-range RGB. Alpha terrain stores its display colour directly.
    float scale = (flags & STEAM_VERTEX) != 0u ? 2.0 : 1.0;
    reflectance = vec4(clamp(colour.rgb * scale, 0, 1), float(face.x + 1u) / 8.0);
}
