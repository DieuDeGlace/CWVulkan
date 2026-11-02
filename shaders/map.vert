#version 450
// Map WORLD and VIEW remain independent inputs (steam-map-world-pixel-contract §6).
#extension GL_GOOGLE_include_directive : require
#include "frame.glsl"
#include "steam_colour.glsl"
#include "steam_transform.glsl"
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in uvec4 inNormalFlags;
layout(location = 0) out vec3 vColor;
layout(location = 1) out vec3 vView;
layout(location = 2) out vec2 vViewNormal;

// Must match MapPush in src/RendererMap.cpp.
layout(push_constant) uniform MapPush {
    mat4 world;
    vec4 lens;
    vec4 parameters; // opacity, depth offset, Glow, Shininess
    vec4 material;
    vec4 selection;  // White, full Steam pixels, reserved
} map;

const vec3 LIGHT_FRONT = vec3(.75, .7, .3), LIGHT_BACK = vec3(.5);
// Instruction-derived float32 outputs of the two CPU normalization stages.
const vec3 LIGHT_TOWARDS = vec3(.17609018087387085, .4402254521846771, .8804509043693542);
const vec4 LIGHT_AMBIENT = vec4(.25, .3, .7, 1);
const vec3 BACK_TERM_DIRECTION = vec3(.597022294998169, .3980148732662201, .696526050567627);
const float DEPTH_SCALE = 1.0001 - .1, DEPTH_BIAS = -.10001;

vec3 faceNormal(uint id) {
    return id == 0u ? vec3(1, 0, 0) : id == 1u ? vec3(-1, 0, 0) : id == 2u ? vec3(0, 1, 0)
         : id == 3u ? vec3(0, -1, 0) : id == 4u ? vec3(0, 0, 1) : vec3(0, 0, -1);
}

void main() {
    vec3 localNormal = faceNormal(inNormalFlags.x);
    vec3 normal = normalize(mat3(map.world) * localNormal);
    mat4 worldView = steamCompose(f.mapView, map.world);
    vView = (worldView * vec4(inPosition, 1)).xyz;
    vViewNormal = normalize(mat3(worldView) * localNormal).xy;
    mat4 projection = mat4(0);
    projection[0][0] = map.lens.x;
    projection[1][1] = map.lens.y;
    projection[2] = vec4(map.lens.zw, DEPTH_SCALE, 1);
    projection[3][2] = DEPTH_BIAS;
    mat4 clipTransform = steamCompose(projection, worldView);
    clipTransform[3][2] -= map.parameters.y;
    vec4 clip = clipTransform * vec4(inPosition, 1);
    gl_Position = vec4(clip.x, -clip.y, clip.zw);

    float direct = max(.7 * dot(normal, LIGHT_TOWARDS) + .3, 0);
    float back = .5 * abs(dot(normal, BACK_TERM_DIRECTION)) + .5;
    vec3 sun = clamp(LIGHT_FRONT * direct + LIGHT_BACK * back, 0, 1);
    float sunlight = inColor.a * map.material.a;
    vec3 light = clamp(sun * sunlight + LIGHT_AMBIENT.rgb * back, 0, 1);
    vec3 doubled = inColor.rgb + inColor.rgb;
    float gain = clamp((LIGHT_AMBIENT.w - light.g) * .5 + 1, 1, 2);
    vec3 pushed = map.parameters.z < 0 ? vec3(dot(LUMA, doubled))
                   : clamp(gain * doubled + (1 - gain) * dot(LUMA, doubled), 0, 2);
    vec3 colour = (light * map.material.rgb) * pushed;
    if (map.parameters.z > 0)
        colour += map.parameters.z * (doubled * map.material.rgb - colour);
    vColor = clamp(colour + map.selection.x * (1 - colour), 0, 1);
}
