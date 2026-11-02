#version 450
// Undivided interface pixel coordinates; retain W for rasterizer interpolation.
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inDepthWeight;
layout(push_constant) uniform Screen { vec2 size; int maskMode; float maskBias; } screen;
layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;
layout(location = 2) out vec2 vMask;
void main() {
    vUv = inUv;
    vColor = inColor / 255.0;
    vMask = (inPos + vec2(screen.maskBias)) / screen.size;
    // The pixel bias is already in XY. It is not scaled by W; neither are mask UVs.
    float w = inDepthWeight.y;
    float z = inDepthWeight.x * -9.999998695775503e-8 + w * 9.999998695775503e-8;
    gl_Position = vec4(inPos / screen.size * 2 - vec2(w), z, w);
}
