#version 450
// Full-screen triangle for post passes; uv (0,0) is the top left (Vulkan).
layout(location = 0) out vec2 vUv;
void main() {
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vUv = p;
    gl_Position = vec4(p * 2 - 1, 0, 1);
}
