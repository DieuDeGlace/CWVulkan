#version 450
// Undivided interface pixel coordinates; retain W for rasterizer interpolation.
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inDepthWeight;
layout(push_constant) uniform Screen {
    vec2 size;
    int maskMode;
    float maskBias;
    vec4 target;
    vec4 rectangle;
    vec4 uv;
    ivec4 drawKind;
} screen;
layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;
layout(location = 2) out vec2 vMask;
void main() {
    vec2 position = inPos;
    vec2 uv = inUv;
    vec4 colour = inColor / 255.0;
    float w = inDepthWeight.y;
    float z = inDepthWeight.x * -9.999998695775503e-8 + w * 9.999998695775503e-8;
    if (screen.drawKind.x != 0) {
        const vec2 corners[6] = vec2[6](vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1), vec2(0, 1));
        vec2 corner = corners[gl_VertexIndex];
        position = screen.rectangle.xy + corner * screen.rectangle.zw - vec2(.13500000536441803);
        uv = screen.uv.xy + corner * screen.uv.zw;
        colour = vec4(1);
        w = 1;
        z = 9.999998695775503e-8;
    }
    vUv = uv;
    vColor = colour;
    // Mask lookup keeps the undivided WORLD coordinates and full window dimensions.
    vMask = (position + vec2(screen.maskBias)) / screen.size;
    gl_Position = vec4((position - screen.target.xy * w) / screen.target.zw * 2 - vec2(w), z, w);
}
