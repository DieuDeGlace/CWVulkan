#version 450
// Interface quads in screen pixels (origin top left).
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec4 inColor;
layout(push_constant) uniform Screen { vec2 size; } screen;
layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;
void main() {
    vUv = inUv;
    vColor = inColor;
    gl_Position = vec4(inPos / screen.size * 2 - 1, 0, 1);
}
