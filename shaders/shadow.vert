#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(location = 0) in vec3 inPosition;
layout(location = 2) in uvec2 inNormalFlags;
void main() {
    uint flags = inNormalFlags.y | uint(pc.params.x);
    vec3 world = sway((pc.model * vec4(inPosition, 1)).xyz, inPosition, flags);
    gl_Position = (pc.params.y > .5 ? f.lightNear : f.lightFar) * vec4(world, 1);
}
