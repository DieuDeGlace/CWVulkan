#version 450
// The Alpha look's cloud pixel shader (docs/specs/alpha-rendering.md §6.3): the lit cloud
// colour blends into the sky gradient behind it by 3D distance, late and then quickly
// (g = 1 - (1 - f^5)^2), so near clouds keep their shading and far ones melt into the sky.
// The vertex shader (alpha_scene.vert, pc.look.w = 1) already used the cloud pass's longer
// fog distance. Clouds take no shadows.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "alpha.glsl"
layout(location = 2) in vec4 vLit;
layout(location = 4) in vec4 vFog;
layout(location = 0) out vec4 outColor;

void main() {
    float f5 = pow(saturate(vFog.z), 5);
    float g = 1 - (1 - f5) * (1 - f5);
    // params.w: 0 opaque; -1 the depth-only draw of a faded cloud; else its near-fade opacity
    // ([Render] AlphaCloudFade, drawn blended after the world).
    float opacity = pc.params.w == 0 ? 1 : clamp(pc.params.w, 0, 1);
    outColor = vec4(mix(vLit.rgb, alphaSkyAt(vFog.y), g), opacity);
}
