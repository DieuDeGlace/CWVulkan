// One bounce from the user's reflective-shadow-map add-on, in CubeVulkan's light space.
layout(set = 0, binding = 4) uniform sampler2D sunReflectance;
layout(set = 0, binding = 5) uniform sampler2D sunDepth;

vec3 bouncedSunlight(vec3 world, vec3 normal) {
    vec3 toEye = world - f.eye.xyz;
    float fade = 1 - smoothstep(28.0 * 28.0, 50.0 * 50.0, dot(toEye, toEye));
    if (fade <= 0) return vec3(0);
    vec3 clip = (f.sunMap * vec4(world, 1)).xyz;
    vec2 extent = vec2(textureSize(sunDepth, 0));
    vec2 texel = (clip.xy * .5 + .5) * extent;
    if (clip.z <= 0 || clip.z >= 1 || any(lessThan(texel, vec2(18))) || any(greaterThan(texel, extent - 18)))
        return vec3(0);

    const ivec2 offsets[12] = ivec2[](ivec2(4,2), ivec2(-2,4), ivec2(-4,-2), ivec2(2,-4),
        ivec2(7,-7), ivec2(7,7), ivec2(-7,7), ivec2(-7,-7), ivec2(17,3), ivec2(-3,17), ivec2(-17,-3), ivec2(3,-17));
    vec3 dx = f.sunMapInverse[0].xyz * (2 / extent.x);
    vec3 dy = f.sunMapInverse[1].xyz * (2 / extent.y);
    vec3 dz = f.sunMapInverse[2].xyz;
    vec2 fraction = fract(texel) - .5;
    vec3 shift = dx * fraction.x + dy * fraction.y;
    vec3 sum = vec3(0);
    for (int i = 0; i < 12; ++i) {
        ivec2 at = ivec2(texel) + offsets[i];
        float depth = texelFetch(sunDepth, at, 0).r;
        if (depth >= 1) continue;
        vec3 offset = dx * offsets[i].x + dy * offsets[i].y + dz * (depth - clip.z);
        vec3 centerDelta = offset - shift;
        if (dot(centerDelta, centerDelta) >= 8.25 * 8.25) continue;
        vec4 material = texelFetch(sunReflectance, at, 0);
        vec3 emitter = faceNormal(uint(clamp(round(material.a * 8) - 1, 0, 5)));
        float facing = dot(emitter, f.bounceSun.xyz);
        vec3 delta = offset - f.bounceSun.xyz * (dot(emitter, shift) / max(facing, .18));
        float d2 = dot(delta, delta);
        float receiving = max(dot(normal, delta) - .09, 0);
        float emitting = max(dot(emitter, -delta) - .09, 0);
        if (receiving <= 0 || emitting <= 0 || d2 >= 49) continue;
        float area = i < 4 ? 1.4 : (i < 8 ? 6.0 : 19.0);
        sum += material.rgb * (area * receiving * emitting / (d2 * max(d2, 1))) *
               ((1 - smoothstep(25, 49, d2)) * smoothstep(0, .18, facing));
    }
    float peak = max(sum.r, max(sum.g, sum.b));
    return sum * f.bounceFront.rgb * (.75 * fade * f.bounceSun.w / (1 + 2.8 * peak));
}

vec3 addSunBounce(vec3 lit, vec3 world, vec3 normal, vec4 albedo, uint flags) {
    if (f.bounceSun.w <= 0 || pc.params.w < .99 || (flags & (GLOW | GUI_WORLD | LAVA_LIGHTING)) != 0u ||
        ((flags & QUEUED_WORLD) != 0u && pc.look.w != 0)) return lit;
    return lit + (1 - .5 * lit) * albedo.rgb * albedo.a * bouncedSunlight(world, normalize(normal));
}
