struct SteamPoint { vec4 position; vec4 colour; };
layout(std430, set = 0, binding = 3) readonly buffer SteamPointTable {
    SteamPoint entries[];
} steamPointsBuffer;

// Ordinary Steam world vertex lighting, independently expressed from
// docs/specs/worldgen-parity-gaps.md §9.3 and steam-voxel-vertex-inputs.md §5.
// RGB has already been decoded from the CPU's packed palette; alpha remains V.
vec3 steamPoints(vec3 world, vec3 normal) {
    vec3 sum = vec3(0);
    for (uint slot = 0u; slot < min(pc.lights.w, 10u); ++slot) {
        SteamPoint entry = steamPointsBuffer.entries[pc.lights.x + slot];
        vec4 light = entry.position;
        if (light.w <= 0)
            continue;
        vec3 delta = world - light.xyz;
        float radial = 1 - min(dot(delta, delta) / (light.w * light.w), 1);
        float facing = clamp(-4 * dot(normal, normalize(delta)), .3, 1);
        sum += facing * radial * radial * entry.colour.rgb;
    }
    return sum;
}

vec3 steamMaterialColour(vec3 palette, vec3 illumination, float ambientAlpha, uint flags) {
    float gain = clamp(1 + .5 * (ambientAlpha - illumination.g), 1, 2);
    vec3 saturated = clamp(gain * palette + (1 - gain) * dot(LUMA, palette), 0, 2);
    vec3 colour = illumination * saturated * f.steamMaterial.rgb;
    if ((flags & GLOW) != 0u)
        colour += palette * f.steamMaterial.rgb - colour;
    // White0 also clamps. Pixel opacity is a separate draw constant.
    return clamp(colour + pc.look.y * (1 - colour), 0, 1);
}

void steamVertexColours(vec3 world, vec3 normal, vec4 palette, uint flags,
                        out vec3 lit, out vec3 indirect) {
    bool lava = (flags & LAVA_LIGHTING) != 0u;
    vec3 direction = lava ? vec3(0, 0, 1) : f.steamSun.xyz;
    vec3 front = lava ? vec3(1, 1, .5) : f.front.rgb;
    vec3 back = lava ? vec3(.5, .30000001192092896, 0) : f.back.rgb;
    vec3 ambient = lava ? vec3(.4000000059604645) : f.ambient.rgb;
    float ambientAlpha = lava ? 1 : f.steamSun.w;
    const vec3 axis = vec3(.597022294998169, .3980148732662201, .696526050567627);
    float orientation = .5 + .5 * abs(dot(normal, axis));
    float facing = max(.7 * dot(normal, direction) + .3, 0);
    float sunlight = palette.a * (pc.look.x * f.steamMaterial.a);
    vec3 local = ambient * orientation + steamPoints(world, normal);
    vec3 full = clamp(clamp(front * facing + back * orientation, 0, 1) * sunlight + local, 0, 1);
    lit = steamMaterialColour(palette.rgb, full, ambientAlpha, flags);
    // A second vertex color supports the engine's optional direct-light shadows.
    // With shadows disabled, only the native full-light result is used.
    vec3 withoutSun = clamp(clamp(back * orientation, 0, 1) * sunlight + local, 0, 1);
    indirect = steamMaterialColour(palette.rgb, withoutSun, ambientAlpha, flags);
}
