#version 450
// Temporal anti-aliasing resolve, ported from the user's Steam Cube World
// TemporalAA mod (camera reprojection from scene depth, Catmull-Rom history,
// clip toward the current colour, edge accumulation, Karis weighting, and the
// small resolve blur that gives the softer Omega-style voxel edges).
// CubeVulkan's own addition: neither game has a temporal pass, only multisampling
// (docs/specs/shadows.md §3). Tuned by the audit of shadows.md §5: the sky keeps less history
// (the sun, moon and stars move across it), and the history is dropped on a cut.
layout(set = 0, binding = 0) uniform sampler2D scene;
layout(set = 0, binding = 1) uniform sampler2D history;
layout(set = 0, binding = 2) uniform sampler2D depthTex;
layout(set = 0, binding = 3) uniform sampler2D previousDepth;
layout(push_constant) uniform Resolve {
    vec4 a;              // weight, valid, jitterX, jitterY (pixels)
    vec4 b;              // blurStrength, edgeWeight, reproject, depthTolerance
    mat4 clipToPrevious; // current jittered clip -> previous jittered clip
    vec4 previousJitter; // xy: last frame's jitter in pixels
} pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

float maxChannel(vec3 v) { return max(v.r, max(v.g, v.b)); }

// The share of the usual history the sky keeps.
const float SKY_HISTORY = .5;

vec3 historyAt(vec2 uv, vec2 size) {
    vec2 position = uv * size, center = floor(position - .5) + .5, f = position - center;
    vec2 w0 = f * (-.5 + f * (1 - .5 * f)), w1 = 1 + f * f * (-2.5 + 1.5 * f), w2 = f * (.5 + f * (2 - 1.5 * f)),
         w3 = f * f * (-.5 + .5 * f);
    vec2 w12 = w1 + w2, t0 = (center - 1) / size, t3 = (center + 2) / size, t12 = (center + w2 / w12) / size;
    vec3 sum = textureLod(history, vec2(t12.x, t0.y), 0).rgb * (w12.x * w0.y) +
               textureLod(history, vec2(t0.x, t12.y), 0).rgb * (w0.x * w12.y) +
               textureLod(history, t12, 0).rgb * (w12.x * w12.y) +
               textureLod(history, vec2(t3.x, t12.y), 0).rgb * (w3.x * w12.y) +
               textureLod(history, vec2(t12.x, t3.y), 0).rgb * (w12.x * w3.y);
    float total = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
    return max(sum / total, vec3(0));
}

void main() {
    float weight = pc.a.x, valid = pc.a.y;
    vec2 jitter = pc.a.zw;
    float blurStrength = pc.b.x, edgeWeight = pc.b.y, reproject = pc.b.z, depthTolerance = pc.b.w;
    ivec2 isize = textureSize(scene, 0);
    vec2 size = vec2(isize);
    vec2 uv = (gl_FragCoord.xy + jitter) / size; // unjittered sample of this frame
    vec4 current = textureLod(scene, uv, 0);
    vec3 lo = current.rgb, hi = current.rgb, left = current.rgb, right = current.rgb, up = current.rgb,
         down = current.rgb;
    for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0)
                continue;
            vec3 c = textureLod(scene, uv + vec2(x, y) / size, 0).rgb;
            lo = min(lo, c);
            hi = max(hi, c);
            if (y == 0 && x == -1) left = c;
            if (y == 0 && x == 1) right = c;
            if (x == 0 && y == -1) up = c;
            if (x == 0 && y == 1) down = c;
        }
    float accepted = valid;
    bool sky = false;
    vec3 raw;
    if (reproject > .5) {
        // Nearest depth of a small cross: silhouette pixels follow the foreground.
        ivec2 pixel = clamp(ivec2(uv * size), ivec2(0), isize - 1), nearest = pixel;
        float d = texelFetch(depthTex, pixel, 0).r;
        const ivec2 cross[4] = ivec2[](ivec2(-1, 0), ivec2(1, 0), ivec2(0, -1), ivec2(0, 1));
        for (int i = 0; i < 4; i++) {
            ivec2 q = clamp(pixel + cross[i], ivec2(0), isize - 1);
            float dq = texelFetch(depthTex, q, 0).r;
            if (dq < d) {
                d = dq;
                nearest = q;
            }
        }
        vec2 nearestUV = (vec2(nearest) + .5) / size;
        // Vulkan clip space: y points down, so uv = ndc * .5 + .5.
        vec4 previous = pc.clipToPrevious * vec4(nearestUV * 2 - 1, d, 1);
        vec3 previousNdc = previous.xyz / max(previous.w, 1e-8);
        vec2 motion = previousNdc.xy * .5 + .5 - nearestUV;
        vec2 previousUV = uv + motion;
        vec2 historyUV = previousUV - pc.previousJitter.xy / size;
        bool inside = previous.w > 0 && all(greaterThanEqual(historyUV, vec2(0))) && all(lessThanEqual(historyUV, vec2(1)));
        ivec2 previousPixel = clamp(ivec2(previousUV * size), ivec2(0), isize - 1);
        float previousSurface = texelFetch(previousDepth, previousPixel, 0).r;
        float tolerance = depthTolerance + 4 * (abs(dFdx(previousNdc.z)) + abs(dFdy(previousNdc.z)));
        sky = d >= .99999;
        bool sameSurface = sky || abs(previousSurface - previousNdc.z) <= tolerance;
        accepted *= inside && sameSurface ? 1 : 0;
        raw = historyAt(clamp(historyUV, vec2(0), vec2(1)), size);
    } else
        raw = texelFetch(history, ivec2(gl_FragCoord.xy), 0).rgb;
    // Clip history toward this pixel's colour until it enters the neighbourhood.
    vec3 offset = raw - current.rgb;
    vec3 room = mix(lo - current.rgb, hi - current.rgb, greaterThan(offset, vec3(0)));
    vec3 limits = mix(vec3(1), clamp(room / offset, 0, 1), greaterThan(abs(offset), vec3(1e-5)));
    vec3 old = current.rgb + offset * min(limits.x, min(limits.y, limits.z));
    float outside = maxChannel(abs(raw - old));
    float confidence = 1 - smoothstep(.02, .12, outside);
    float edge = smoothstep(.04, .20, maxChannel(hi - lo));
    float accumulation = mix(weight, max(weight, edgeWeight), edge) * accepted * confidence;
    if (sky)
        accumulation *= SKY_HISTORY; // (stars are a few pixels: a long history smears them)
    vec3 filtered = mix(current.rgb, (current.rgb * 4 + left + right + up + down) / 8, blurStrength);
    float currentWeight = (1 - accumulation) / (1 + maxChannel(filtered));
    float historyWeight = accumulation / (1 + maxChannel(old));
    outColor = vec4((filtered * currentWeight + old * historyWeight) / (currentWeight + historyWeight), current.a);
}
