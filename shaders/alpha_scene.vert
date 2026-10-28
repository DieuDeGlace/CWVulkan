#version 450
// The Alpha look's world vertex shader (docs/specs/alpha-rendering.md §5): every terrain,
// water, model and creature vertex is lit here, by the sun and sky terms and the point lights.
// Clouds (pc.look.w = 1) take the cloud pass's constants (§4, docs/specs/sky-and-clouds.md
// §A.5): the sun colour on both sides, the sky's average colour in their unlit creases, and
// fog that reaches half again as far.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "alpha.glsl"
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor; // rgb, a = baked light (terrain) or 1 (models)
layout(location = 2) in uvec4 inNormalFlags;
layout(location = 0) out vec3 vWorld;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec4 vLit;    // colour with the sun term, a = the vertex's light value
layout(location = 3) out vec3 vShaded; // colour without the sun term (in shadow)
layout(location = 4) out vec4 vFog;    // x, y: screen position (y shifted), z, w: fog distances
layout(location = 5) flat out uint vFlags;
layout(location = 6) out vec2 vViewNormal; // twice the view-space normal's x and y (the shimmer)
layout(location = 7) out vec3 vToCamera;

// §5.7: the point lights this draw was given, falling off with distance and facing.
vec3 pointLightSum(vec3 world, vec3 normal) {
    vec3 sum = vec3(0);
    uint count = min(pc.lights.w, 10u);
    for (uint i = 0u; i < count; i++) {
        uint index = (pc.lights[i / 4u] >> (8u * (i % 4u))) & 0xffu;
        vec4 light = f.pointLights[index];
        if (light.w <= 0)
            continue;
        vec3 d = world - light.xyz;
        float q = max(dot(d, d), 1e-8);
        float facing = clamp(-4 * dot(normal, d * inversesqrt(q)), .3, 1);
        float fall = 1 - min(q / (light.w * light.w), 1);
        sum += facing * fall * fall * f.pointColours[index].rgb;
    }
    return sum;
}

// §5.9: the lit colour K (the light T times the vertex colour), then the hit flash towards
// white.
vec3 withFlash(vec3 K) { return K + pc.look.y * (1 - K); }

// How far moonlight takes colours towards grey (CubeVulkan's moon, src/AlphaMoon.h).
const float MOON_GREY = .55;
// The cloud pass's fog distance is 1.5 times the world's.
const float CLOUD_FOG_REACH = 1.5;

void main() {
    uint flags = inNormalFlags.y | uint(pc.params.x);
    if ((flags & 8u) != 0u) { // captured menu geometry: colours already final
        gl_Position = pc.model * vec4(inPosition, 1);
        vWorld = vec3(0);
        vNormal = vec3(0, 0, 1);
        vLit = inColor;
        vShaded = inColor.rgb;
        vFog = vec4(0);
        vFlags = flags;
        vViewNormal = vec2(0);
        vToCamera = vec3(0);
        return;
    }
    vec3 world = sway((pc.model * vec4(inPosition, 1)).xyz, inPosition, flags);
    vec3 face = (flags & 16u) != 0u
        ? normalize(vec3(inNormalFlags.x, inNormalFlags.z, inNormalFlags.w) / 127.5 - 1)
        : faceNormal(inNormalFlags.x);
    vec3 N = normalize(mat3(pc.model) * face);
    gl_Position = f.viewProj * vec4(world, 1);

    // §5.2: the screen position for the sky gradient, divided per vertex. Vulkan's clip y
    // points down; the gradient's Y points up.
    vec2 screen = vec2(gl_Position.x, -gl_Position.y) / gl_Position.w;
    bool cloud = pc.look.w > .5;
    vec3 toCamera = f.eye.xyz - world;
    float fogScale = cloud ? f.alphaSun.w / CLOUD_FOG_REACH : f.alphaSun.w;
    float fogScale2 = fogScale * fogScale;
    vFog = vec4(screen.x, screen.y + f.alphaView.x, dot(toCamera, toCamera) * fogScale2,
                dot(toCamera.xy, toCamera.xy) * fogScale2);

    // §5.6: sun, back and ambient light, all scaled by S, by the light the vertex was baked
    // with and by the light where a model stands; then the point lights, unscaled.
    vec3 D = f.alphaSun.xyz;
    vec3 sun = max(dot(N, D), 0) * f.alphaFront.rgb;
    vec3 backColour = cloud ? f.alphaFront.rgb : f.alphaBack.rgb;
    vec3 sky = f.alphaAmbient.rgb + max(dot(N, -D), 0) * backColour;
    float strength = f.alphaMaterial.a * pc.look.x * inColor.a;
    vec3 points = pointLightSum(world, N);

    // CubeVulkan's moon (src/AlphaMoon.h; not part of the spec): opposite the sun, lighting
    // faces the way the sun does, plus a cool ambient floor. Its colours already carry the
    // moon's strength M, which plays the part S plays for the sun; like the sun it is scaled
    // by the baked light and the light where a model stands, so caves stay dark.
    float baked = pc.look.x * inColor.a;
    vec3 moonDirect = max(dot(N, -D), 0) * f.alphaMoonFront.rgb * baked;
    vec3 moonFill = (max(dot(N, D), 0) * f.alphaMoonBack.rgb + f.alphaNight.rgb) * baked;

    // Shadows take away the direct term of whichever light casts them: the sun by day, the
    // moon by night.
    bool moonCasts = f.alphaMoonBack.w > .5;
    vec3 sunDirect = sun * strength;
    vec3 alphaLit = clamp(sky * strength + sunDirect + points, 0, 1.2);
    vec3 alphaShaded = moonCasts ? alphaLit : clamp(sky * strength + points, 0, 1.2);
    vec3 moonLit = moonFill + moonDirect;
    vec3 moonShaded = moonCasts ? moonFill : moonLit;

    vec3 colour = inColor.rgb * f.alphaMaterial.rgb;
    // By moonlight colours fade towards grey, as they do to the eye at night, so the
    // moonlit world reads blue rather than as a dim day.
    vec3 moonColour = mix(colour, vec3(dot(colour, LUMA)), MOON_GREY);
    vec3 lit = alphaLit * colour + moonLit * moonColour;
    vec3 shaded = alphaShaded * colour + moonShaded * moonColour;
    if (cloud) { // §5.9: the darkness colour, the sky's average, where the light falls short
        vec3 darkness = (f.alphaSky1.rgb + f.alphaSky2.rgb) * .5;
        lit += darkness * (1 - alphaLit);
        shaded = lit;
    }
    if ((flags & GLOW) != 0u) // lamps and runes: never darker than their own colour
        lit = shaded = colour;

    vWorld = world;
    vNormal = N;
    vLit = vec4(withFlash(lit), inColor.a);
    vShaded = withFlash(shaded);
    vFlags = flags;
    vViewNormal = 2 * normalize(mat3(f.view) * N).xy;
    vToCamera = toCamera;
}
