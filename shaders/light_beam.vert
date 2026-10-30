#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColour;
layout(location = 0) out vec4 colour;
void main() {
    // One column per sampled corner. Vertex x is its immutable identifier.
    gl_Position = f.viewProj * pc.model[int(inPosition.x)];
    colour = vec4(1, 1, 1, inColour.a * pc.params.z);
}
