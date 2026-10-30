// MoonandShadow's lighting, re-expressed for CubeVulkan's own pipeline.
#include "steam_sky.glsl"
layout(set = 0, binding = 1) uniform sampler2DShadow farShadow;
layout(set = 0, binding = 2) uniform sampler2DShadow nearShadow;

// Sun visibility (1 = lit) from one map. region = (texel, bias, radius, 1/span).
// CubeVulkan's own shadows (neither game has any: docs/specs/shadows.md §1), tuned by the
// audit of shadows.md §4:
// * the lookup moves along the face normal by most of a texel (a normal offset), which suits
//   axis-aligned voxel faces better than a large slope bias: no lit gap at the foot of walls;
// * the depth bias never drops below half a texel of the map in use (no acne on far, sunlit
//   tops), and its slope part is capped at two texels (no peter-panning at grazing light);
// * a 3 x 3 grid of bilinear compare taps softens the edge over about a quarter of a block,
//   nearer the soft baked shade of the vanilla look (§2) than a one-texel stair step.
float shadowVisibility(sampler2DShadow map, mat4 transform, vec4 region, vec3 w, vec3 n,
                       float facing) {
    // Faces turned from the light get none of its direct term anyway (alpha-rendering.md §5
    // step 6); fade the shadow test out only in the last few degrees so faces near sunrise do
    // not flip between lit and shaded.
    float exposure = smoothstep(0., .1, facing);
    float worldTexel = region.x * 2 * region.z; // a texel's width in blocks
    vec3 p = (transform * vec4(w + n * worldTexel * .75, 1)).xyz;
    vec2 uv = p.xy * .5 + .5;
    if (exposure <= 0)
        return 0;
    if (any(lessThan(uv, vec2(2 * region.x))) || any(greaterThan(uv, vec2(1 - 2 * region.x))) ||
        p.z < 0 || p.z > 1)
        return exposure; // Outside the map: lit (the distance fade ends shadows first).
    float depthTexel = worldTexel * region.w; // a texel's width in depth units
    float slope = sqrt(clamp(1 - facing * facing, 0, 1)) / max(abs(facing), .1);
    float bias = max(region.y, .5 * depthTexel) + depthTexel * min(slope * .5, 2.);
    float sum = 0;
    for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++)
            sum += textureLod(map, vec3(uv + vec2(x, y) * region.x * 1.5, p.z - bias), 0);
    return sum / 9 * exposure;
}

// Sun visibility from the two cascades: the near map close to the player, blended into the
// far map towards the near map's edge. `ground` is the horizontal distance to the player.
float cascadeVisibility(vec3 w, vec3 n, float sunFacing, float ground) {
    float nearRadius = f.shadowNear.z, blendStart = nearRadius * .8;
    vec4 nearRegion = vec4(f.shadowNear.x, f.shadowNear.y, nearRadius, f.shadowFar.w);
    if (f.shadowNear.w > .5 && ground < blendStart)
        return shadowVisibility(nearShadow, f.lightNear, nearRegion, w, n, sunFacing);
    float lit = shadowVisibility(farShadow, f.lightFar, f.shadowFar, w, n, sunFacing);
    if (f.shadowNear.w > .5 && ground < nearRadius) {
        float nearLit = shadowVisibility(nearShadow, f.lightNear, nearRegion, w, n, sunFacing);
        lit = mix(nearLit, lit, smoothstep(blendStart, nearRadius, ground));
    }
    return lit;
}

// The share of direct sunlight that reaches w (1 = all of it) once the shadow strength and
// the fade towards the shadow radius are applied. Used by the Alpha look, whose shadows only
// take away its sun term.
float sunShare(vec3 w, vec3 n) {
    float strength = f.lightDir.w;
    if (strength <= 0)
        return 1;
    vec3 offset = w - f.focus.xyz;
    float radius = f.shadowFar.z;
    float fade = 1 - smoothstep(radius * f.back.w, radius, length(vec3(offset.xy, offset.z * f.ambient.w)));
    if (fade <= 0)
        return 1;
    float lit = cascadeVisibility(w, n, dot(n, f.lightDir.xyz), length(w.xy - f.focus.xy));
    return 1 - (1 - lit) * strength * fade;
}

struct Shade {
    vec3 light; // Multiplies albedo.
    vec3 add;   // Added (warm sunlight).
};

Shade shade(vec3 w, vec3 n, bool receiveShadow, float materialLight) {
    bool lava = (uint(pc.params.x) & LAVA_LIGHTING) != 0u;
    vec3 L = lava ? vec3(0, 0, 1) : f.lightDir.xyz;
    vec3 frontColour = lava ? vec3(1, 1, .5) : f.front.rgb;
    vec3 backColour = lava ? vec3(.5, .30000001192092896, 0) : f.back.rgb;
    vec3 ambientColour = lava ? vec3(.4000000059604645) : f.ambient.rgb;
    // Native world shader terms (front/back/ambient, orientation).
    float orientation = .5 + .5 * abs(dot(n, vec3(.597022294998169, .3980148732662201, .696526050567627)));
    float facing = max(0, .3 + .7 * dot(n, L));
    vec3 back = backColour * orientation;
    // Player lantern at night (native-style point light around the player).
    vec3 local = vec3(0);
    float protection = 0;
    if (f.focus.w > 0) {
        vec3 delta = w - (f.focus.xyz + vec3(0, 0, 1.5));
        float d2 = dot(delta, delta);
        float falloff = clamp(1 - d2 / (f.focus.w * f.focus.w), 0, 1);
        float toward = clamp(-4 * dot(n, delta * inversesqrt(max(d2, 1e-4))), .3, 1);
        local = vec3(1, .72, .42) * falloff * falloff * toward * (1 - f.sky.w) * .9;
        protection = clamp(4 * falloff * falloff * toward, 0, 1) * (1 - f.sky.w);
    }
    vec3 ambient = ambientColour * orientation + local;
    vec3 full = clamp(ambient + clamp(back + frontColour * facing, 0, 1) * materialLight, 0, 1);
    vec3 indirect = clamp(ambient + clamp(back, 0, 1) * materialLight, 0, 1);
    Shade s;
    s.light = full;
    s.add = vec3(0);
    float strength = f.lightDir.w;
    if (!receiveShadow || strength <= 0)
        return s;
    vec3 offset = w - f.focus.xyz;
    float ground = length(w.xy - f.focus.xy);
    float radius = f.shadowFar.z;
    float fade = 1 - smoothstep(radius * f.back.w, radius,
                                length(vec3(offset.xy, offset.z * f.ambient.w)));
    if (fade <= 0)
        return s;
    float sunFacing = dot(n, L);
    float lit = cascadeVisibility(w, n, sunFacing, ground);
    // Shadow removes only the direct-sunlight share; ambient, back and lantern stay.
    float opacity = (1 - lit) * strength * fade * (1 - protection);
    s.light = full - (full - indirect) * opacity;
    s.add = vec3(1, .65, .2) * f.front.w * lit * fade * clamp(sunFacing, 0, 1) * f.sky.w;
    return s;
}

// The Steam look's distance fog. It fades into the sky gradient behind the pixel
// (steam_sky.glsl), so far ground meets the sky without a seam. The curve is CubeVulkan's own:
// the Steam world shader's fog is not specified yet (alpha-rendering.md §14 outlines it).
vec3 applyFog(vec3 color, vec3 w) {
    float d = length(w - f.eye.xyz);
    float t = smoothstep(f.fog.w * .55, f.fog.w, d);
    return mix(color, steamSkyAt(steamScreen()), t);
}

Shade shade(vec3 w, vec3 n, bool receiveShadow) {
    return shade(w, n, receiveShadow, 1.0);
}
