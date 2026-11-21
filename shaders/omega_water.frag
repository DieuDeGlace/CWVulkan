#version 450
// Original shared surface shader: scene refraction and crisp voxel-bank reflections.
// No Omega shader, texture or source code is used.
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
#include "lighting.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNormal;
layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 0) uniform sampler2D opaqueColour;
layout(set = 1, binding = 1) uniform sampler2D opaqueDepth;
layout(std140, set = 1, binding = 2) uniform Water {
    vec4 optics;      // near, far, ray steps, reflection strength
    vec4 body;        // scattered surface RGB, light strength
    vec4 absorption;  // red/green/blue extinction per block
    vec4 screen;      // inverse width/height, submerged camera, time
    vec4 fog;         // scene fog RGB and range
    vec4 sky;         // reflection fallback RGB
    vec4 waves;       // wrapped phase offsets, revised treatment, Alpha lighting
} water;

float linearDepth(float depth) {
    return water.optics.x * water.optics.y /
           (water.optics.y - depth * (water.optics.y - water.optics.x));
}
bool onScreen(vec2 uv) {
    return all(greaterThan(uv, vec2(0))) && all(lessThan(uv, vec2(1)));
}
bool tracePoint(vec3 origin, vec3 direction, float distance, out vec2 uv, out float separation) {
    vec4 clip = f.viewProj * vec4(origin + direction * distance, 1);
    if (clip.w <= 0) return false;
    uv = clip.xy / clip.w * .5 + .5;
    if (!onScreen(uv)) return false;
    float depth = texture(opaqueDepth, uv).r;
    separation = depth < .999999 ? clip.w - linearDepth(depth) : -water.optics.y;
    return true;
}
vec3 reflection(vec3 origin, vec3 direction) {
    vec3 fallback = water.sky.rgb;
    if (water.optics.w <= 0 || water.screen.z > .5) return fallback;
    float previousDistance = .08;
    float previousSeparation = -1;
    int steps = int(water.optics.z);
    for (int i = 1; i <= steps; ++i) {
        float fraction = float(i) / float(steps);
        float distance = .08 + 64 * fraction * fraction;
        vec2 uv;
        float separation;
        if (!tracePoint(origin, direction, distance, uv, separation)) break;
        if (separation >= 0 && previousSeparation < 0) {
            float low = previousDistance, high = distance;
            for (int refine = 0; refine < 4; ++refine) {
                float middle = (low + high) * .5;
                vec2 middleUv;
                float middleSeparation;
                if (!tracePoint(origin, direction, middle, middleUv, middleSeparation)) break;
                if (middleSeparation < 0) low = middle;
                else { high = middle; uv = middleUv; separation = middleSeparation; }
            }
            if (separation < 2) {
                vec2 edge = min(uv, 1 - uv);
                float fade = smoothstep(0, .04, min(edge.x, edge.y));
                return mix(fallback, texture(opaqueColour, uv).rgb, fade);
            }
        }
        previousDistance = distance;
        previousSeparation = separation;
    }
    return fallback;
}
vec3 revisedNormal(vec3 view) {
    float strength = 1. - smoothstep(40., 120., length(view));
    if (vNormal.z <= .5 || strength <= 0.) return normalize(vNormal);
    const vec2 first = vec2(.65, .31), second = vec2(-.27, .72);
    float a = dot(vWorld.xy, first) + water.waves.x;
    float b = dot(vWorld.xy, second) + water.waves.y;
    vec2 slope = .12 * cos(a) * first + .11 * cos(b) * second
               + .05 * cos(2.*a+b) * (2.*first+second)
               + .05 * cos(a-2.*b) * (first-2.*second)
               + .025 * cos(3.*a+2.*b) * (3.*first+2.*second);
    return normalize(vec3(-strength * slope, 1));
}
vec3 omegaNormal() {
    vec3 normal = normalize(cross(dFdx(vWorld), dFdy(vWorld)));
    if (normal.z < 0) normal = -normal;
    if (normal.z > .5) {
        float time = water.screen.w;
        vec2 ripple = vec2(sin(vWorld.x * .65 + vWorld.y * .31 + time * .8),
                           sin(vWorld.y * .72 - vWorld.x * .27 - time * .65));
        normal = normalize(normal + vec3(ripple * .018, 0));
    }
    return normal;
}
void main() {
    vec2 uv = gl_FragCoord.xy * water.screen.xy;
    vec3 view = f.eye.xyz - vWorld;
    vec3 toEye = normalize(view);
    bool revised = water.waves.z > .5;
    vec3 normal = revised ? revisedNormal(view) : omegaNormal();
    if (dot(normal, toEye) < 0) normal = -normal;
    vec2 margin = water.screen.xy * (revised ? .5 : 1.);
    vec2 refractedUv = clamp(uv + normal.xy * (revised ? .006 : .002), margin, 1 - margin);
    float sceneDepth = linearDepth(texture(opaqueDepth, refractedUv).r);
    float surfaceDepth = (f.viewProj * vec4(vWorld, 1)).w;
    // A thin surface film still needs a visible blue body over bright grass.
    float thickness = clamp(sceneDepth - surfaceDepth, revised ? .3 : .8, 100);
    vec3 transmission = exp(-water.absorption.rgb * thickness);
    vec3 colour = texture(opaqueColour, refractedUv).rgb * transmission +
                  water.body.rgb * water.body.w * (1 - transmission);
    if (revised) {
        if (vNormal.z > .5 && water.screen.z < .5) {
            vec3 sun = water.waves.w > .5 ? f.alphaSun.xyz : f.sunDisk.xyz;
            float alignment = clamp(dot(reflect(-toEye, normal), sun), 0., 1.);
            float glint = min(3. * pow(alignment, 300.), 1.) * water.body.w * clamp(5. * sun.z, 0., 1.);
            if (glint > .001)
                colour += vec3(glint * sunShare(vWorld, vec3(0, 0, 1)));
        }
    } else {
        vec3 reflected = reflection(vWorld + normal * .04, reflect(-toEye, normal));
        float facing = clamp(dot(normal, toEye), 0, 1);
        float fresnel = .18 + .82 * pow(1 - facing, 5);
        if (water.screen.z > .5) fresnel = 0;
        colour = mix(colour, reflected, fresnel * water.optics.w);
        float sheen = pow(max(dot(reflect(-normalize(f.lightDir.xyz), normal), toEye), 0), 96);
        if (water.screen.z < .5) colour += vec3(sheen * .7 * water.body.w);
    }
    float haze = smoothstep(water.fog.w * .6, max(water.fog.w, .001), length(view));
    colour = mix(colour, water.fog.rgb, haze);
    // Refraction already includes the opaque scene; a second alpha blend would
    // count that background twice. The nearest water surface still writes depth.
    outColor = vec4(clamp(colour, 0, 1), 1);
}
