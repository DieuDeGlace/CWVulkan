#version 450
// The world map's vertex shader (docs/specs/worldmap-rendering.md §3.2; the CPU twin is
// src/MapLook.cpp). The map keeps its own light rig whatever the world's look: a front light
// from a fixed direction that wraps, a fixed unsigned back term, and a blue ambient. Vertex
// colours arrive at half strength and are doubled here.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;          // the corner colour; a scales the sun terms
layout(location = 2) in uvec4 inNormalFlags;   // x: the face's normal index
layout(location = 0) out vec3 vColor;
layout(location = 1) out vec3 vView;           // the camera-space position, for the fog

// Must match MapPush in src/RendererMap.cpp.
layout(push_constant) uniform MapPush {
    mat4 modelView;  // the model's units to the map camera's space
    vec4 lens;       // -f / aspect, f, the screen shift x, y
    vec4 turn;       // the view's yaw and pitch (radians), the instance's opacity, its depth offset
    vec4 material;   // rgb the instance's colour, a its white amount
    vec4 fog;        // rgb the world's fog colour
} map;

const vec3 LIGHT_FRONT = vec3(0.75, 0.7, 0.3);
const vec3 LIGHT_BACK = vec3(0.5);
const vec3 LIGHT_TOWARDS = normalize(vec3(0.2, 0.5, 1.0));
const vec4 LIGHT_AMBIENT = vec4(0.25, 0.3, 0.7, 1.0);
const vec3 BACK_TERM_DIRECTION = vec3(0.597022, 0.398015, 0.696526);
const vec3 LUMA = vec3(0.299, 0.587, 0.114);
const float DEPTH_SCALE = 1.0001 - 0.1, DEPTH_BIAS = -0.10001;

vec3 faceNormal(uint id) {
    return id == 0u ? vec3(1, 0, 0) : id == 1u ? vec3(-1, 0, 0) : id == 2u ? vec3(0, 1, 0)
         : id == 3u ? vec3(0, -1, 0) : id == 4u ? vec3(0, 0, 1) : vec3(0, 0, -1);
}

// The view's turn (yaw about z, then pitch about x): the light's directions are fixed in map
// space, so they are turned into the camera's space with the normals.
mat3 viewTurn() {
    float cy = cos(map.turn.x), sy = sin(map.turn.x), cp = cos(map.turn.y), sp = sin(map.turn.y);
    mat3 yaw = mat3(cy, sy, 0, -sy, cy, 0, 0, 0, 1);
    mat3 pitch = mat3(1, 0, 0, 0, cp, sp, 0, -sp, cp);
    return pitch * yaw;
}

void main() {
    vec4 view = map.modelView * vec4(inPosition, 1);
    vView = view.xyz;
    // Through the lens, then the instance's depth offset; Vulkan's y points down.
    vec4 clip = vec4(map.lens.x * view.x + map.lens.z * view.z, map.lens.y * view.y + map.lens.w * view.z,
                     DEPTH_SCALE * view.z + DEPTH_BIAS - map.turn.w, view.z);
    gl_Position = vec4(clip.x, -clip.y, clip.z, clip.w);

    mat3 turn = viewTurn();
    vec3 normal = normalize(mat3(map.modelView) * faceNormal(inNormalFlags.x));
    float direct = max(0.0, 0.7 * dot(normal, turn * LIGHT_TOWARDS) + 0.3);
    float back = 0.5 * abs(dot(normal, turn * BACK_TERM_DIRECTION)) + 0.5;
    vec3 sun = clamp(back * LIGHT_BACK + direct * LIGHT_FRONT, 0.0, 1.0);
    vec3 light = clamp(back * LIGHT_AMBIENT.rgb + sun * inColor.a, 0.0, 1.0);

    // The colour doubled, then pushed from its grey the more as the face gets less green light.
    vec3 doubled = 2.0 * inColor.rgb;
    float push = clamp((LIGHT_AMBIENT.w - light.g) * 0.5 + 1.0, 1.0, 2.0);
    vec3 pushed = clamp(push * doubled + (1.0 - push) * dot(LUMA, doubled), 0.0, 2.0);
    vec3 lit = light * map.material.rgb * pushed;
    vColor = clamp(lit + map.material.a * (1.0 - lit), 0.0, 1.0);
}
