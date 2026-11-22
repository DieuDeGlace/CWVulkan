#version 450
// Temporal anti-aliasing resolve, as the user's later Infinite Cubes Vulkan work refined it
// (its TemporalResolveShader, itself adapted from this pass): reprojection from this pixel's own
// depth, history kept on the unjittered grid, a 2x2 previous-depth check around the projected
// ray, Catmull-Rom history clipped toward the current colour, edge accumulation, a stillness
// term that lets history build up to 0.94 as on-screen motion falls from 1 px to 0.05 px (so a
// still picture stops shimmering with the jitter), Karis weighting and a small resolve blur.
// CubeVulkan's own addition: neither game has a temporal pass, only multisampling
// (docs/specs/shadows.md §3).
layout(set = 0, binding = 0) uniform sampler2D scene;
layout(set = 0, binding = 1) uniform sampler2D history;
layout(set = 0, binding = 2) uniform sampler2D depthTex;
layout(set = 0, binding = 3) uniform sampler2D previousDepth;
layout(push_constant) uniform Resolve {
    vec4 a;              // weight, valid, jitterX, jitterY (pixels)
    vec4 b;              // blurStrength, edgeWeight, reproject, (unused)
    mat4 clipToPrevious; // current jittered clip -> previous jittered clip
    vec4 previousJitter; // xy: last frame's jitter in pixels
} pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

float maxChannel(vec3 v) { return max(v.r, max(v.g, v.b)); }

// The most history a still pixel keeps.
const float STILL_HISTORY = .94;

vec3 historyAt(vec2 uv, vec2 size) {
    vec2 position = uv * size, center = floor(position - .5) + .5, f = position - center;
    vec2 w0 = f * (-.5 + f * (1 - .5 * f)), w1 = 1 + f * f * (-2.5 + 1.5 * f), w2 = f * (.5 + f * (2 - 1.5 * f)),
         w3 = f * f * (-.5 + .5 * f);
    vec2 w12 = w1 + w2, t0 = (center - 1) / size, t3 = (center + 2) / size, t12 = (center + w2 / w12) / size;
    vec4 outer = vec4(w12.x * w0.y, w0.x * w12.y, w12.x * w3.y, w3.x * w12.y);
    float centerWeight = w12.x * w12.y;
    vec3 sum = textureLod(history, vec2(t12.x, t0.y), 0).rgb * outer.x + textureLod(history, vec2(t0.x, t12.y), 0).rgb * outer.y +
               textureLod(history, vec2(t12.x, t3.y), 0).rgb * outer.z + textureLod(history, vec2(t3.x, t12.y), 0).rgb * outer.w +
               textureLod(history, t12, 0).rgb * centerWeight;
    return max(sum / (dot(outer, vec4(1)) + centerWeight), vec3(0));
}

void main() {
    float weight = pc.a.x;
    bool valid = pc.a.y > .5;
    vec2 jitter = pc.a.zw;
    float blurStrength = pc.b.x, edgeWeight = pc.b.y;
    bool reproject = pc.b.z > .5;
    ivec2 isize = textureSize(scene, 0);
    vec2 size = vec2(isize);
    vec2 outputUV = gl_FragCoord.xy / size;
    vec2 uv = outputUV + jitter / size; // the unjittered sample of this frame
    vec4 current = textureLod(scene, uv, 0);
    outColor = current;
    if (!valid && blurStrength == 0)
        return;

    vec3 lo = current.rgb, hi = current.rgb, axisSum = vec3(0);
    for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++) {
            vec3 c = textureLod(scene, uv + vec2(x, y) / size, 0).rgb;
            lo = min(lo, c);
            hi = max(hi, c);
            if (abs(x) + abs(y) == 1)
                axisSum += c;
        }
    vec3 filtered = mix(current.rgb, (current.rgb * 4 + axisSum) / 8, blurStrength);
    outColor = vec4(filtered, current.a);
    if (!valid)
        return;

    ivec2 last = isize - 1;
    float depth = texelFetch(depthTex, clamp(ivec2(gl_FragCoord.xy), ivec2(0), last), 0).r;
    vec2 sourceUV = outputUV - jitter / size;
    vec2 motion = vec2(0), depthUV = sourceUV;
    float tolerance = .0002;
    float projectedDepth = depth;
    bool inside = true;
    if (reproject) {
        // Vulkan clip space: y points down, so uv = ndc * .5 + .5.
        vec4 previous = pc.clipToPrevious * vec4(sourceUV * 2 - 1, depth, 1);
        vec3 previousNdc = previous.xyz / max(previous.w, 1e-8);
        vec2 previousUV = previousNdc.xy * .5 + .5;
        motion = previousUV - sourceUV;
        depthUV = previousUV + pc.previousJitter.xy / size;
        projectedDepth = previousNdc.z;
        tolerance += min(.005, 2 * (abs(dFdx(previousNdc.z)) + abs(dFdy(previousNdc.z))));
        inside = previous.w > 0;
    }
    vec2 historyUV = outputUV + motion;
    inside = inside && all(greaterThanEqual(historyUV, vec2(0))) && all(lessThanEqual(historyUV, vec2(1))) &&
             all(greaterThanEqual(depthUV, vec2(0))) && all(lessThanEqual(depthUV, vec2(1)));
    bool sky = depth >= .99999;
    // Depths around the projected source ray, where the previous scene was jittered.
    ivec2 depthBase = ivec2(floor(clamp(depthUV, 0, 1) * size - .5));
    bool sameSurface = false;
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < 2; x++) {
            float d = texelFetch(previousDepth, clamp(depthBase + ivec2(x, y), ivec2(0), last), 0).r;
            sameSurface = sameSurface || (sky ? d >= .99999 : d < .99999 && abs(d - projectedDepth) <= tolerance);
        }
    vec3 raw = historyAt(clamp(historyUV, 0, 1), size);

    // Clip history toward this pixel's colour, within its neighbourhood's range.
    vec3 delta = raw - current.rgb;
    vec3 room = mix(current.rgb - lo, hi - current.rgb, greaterThan(delta, vec3(0)));
    vec3 limits = clamp((room + 1e-5) / max(abs(delta), vec3(1e-5)), 0, 1);
    vec3 clipped = current.rgb + delta * min(limits.x, min(limits.y, limits.z));
    float confidence = 1 - smoothstep(.02, .12, maxChannel(abs(raw - clipped)));
    float edge = smoothstep(.04, .20, maxChannel(hi - lo));
    float still = 1 - smoothstep(.05, 1., length(motion * size));
    float accumulation = inside && sameSurface ? mix(mix(weight, edgeWeight, edge), STILL_HISTORY, still) * confidence : 0.;
    float currentWeight = (1 - accumulation) / (1 + maxChannel(filtered));
    float historyWeight = accumulation / (1 + maxChannel(clipped));
    outColor = vec4((filtered * currentWeight + clipped * historyWeight) / (currentWeight + historyWeight), current.a);
}
