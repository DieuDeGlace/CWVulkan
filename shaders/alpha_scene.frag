#version 450
// The Alpha look's world pixel shader (docs/specs/alpha-rendering.md §6.2): haze towards the
// fog colour by distance, then the sky over the far ground, and the shimmer of shiny items.
// CubeVulkan's optional shadows take away only the direct term of the light casting them
// (the sun by day, CubeVulkan's moon by night) that the vertex shader added.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "lighting.glsl"
#include "alpha.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vLit;
layout(location = 3) in vec3 vShaded;
layout(location = 4) in vec4 vFog;
layout(location = 5) flat in uint vFlags;
layout(location = 6) in vec2 vViewNormal;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265;

void main() {
    if ((vFlags & 8u) != 0u) {
        outColor = vec4(vLit.rgb, 1);
        return;
    }
    float lit = (vFlags & GLOW) != 0u ? 1 : sunShare(vWorld, normalize(vNormal));
    // Scaled by the light the surface or model already has (vertex light x where it stands),
    // so a place the baked sky light dims is not darkened twice (shadows.md §4).
    lit = 1 - (1 - lit) * saturate(vLit.a * pc.look.x);
    vec3 c = mix(vShaded, vLit.rgb, lit);

    float Y = vFog.y;
    vec3 sky = alphaSkyAt(Y);
    // Haze by 3D distance (up to 75 %), then the sky by horizontal distance.
    float haze = saturate(1.5 * vFog.z - .025);
    haze = .75 * (1 - (1 - haze) * (1 - haze));
    float far = smoothCurve(saturate((vFog.w - .6) * 2.5));
    c = mix(c, f.alphaFog.rgb, haze);
    c = mix(c, sky, far * far);

    // Shiny items shimmer with the screen position and the view normal.
    if (pc.look.z > 0) {
        float angle = 10 * Y + 5 * vFog.x + vViewNormal.y + vViewNormal.x;
        angle -= 2 * PI * ceil((angle - PI) / (2 * PI)); // into (-pi, pi]
        c *= 1 + .3 * sin(angle);
    }
    outColor = vec4(submergedSceneColour(c), pc.params.w);
}
