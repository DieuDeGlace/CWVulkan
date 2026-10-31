// How the world map looks (docs/specs/worldmap-rendering.md). See MapLook.h.
#include "MapLook.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace worldmap {

namespace {

// The grey the mesher measures a colour by (0x42e704, 0x43ef64, 0x43ef48) and the one the
// shaders use; they differ.
float mesherGrey(glm::vec3 c) { return 0.3f * c.r + 0.59f * c.g + 0.11f * c.b; }
float shaderGrey(glm::vec3 c) { return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b; }

float saturate(float x) { return std::clamp(x, 0.0f, 1.0f); }
glm::vec3 saturate(glm::vec3 c) { return glm::clamp(c, glm::vec3(0), glm::vec3(1)); }
float smooth(float x) { return x * x * (3 - 2 * x); }

constexpr float OCCLUSION_POWER = 1.2f; // 0x42e718
constexpr float WRAP_SLOPE = 0.7f, WRAP_FLOOR = 0.3f; // the direct term's wrap (shader literals)
constexpr float BACK_TERM_WEIGHT = 0.5f;               // (shader literal)

// Each face: its normal, and its corners relative to the voxel, in the order of §2.2.
struct FaceShape {
    glm::ivec3 normal;
    glm::ivec3 corners[4];
};
const FaceShape FACES[6] = {
    {{-1, 0, 0}, {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}}, // west
    {{1, 0, 0}, {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}},  // east
    {{0, -1, 0}, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}}, // south
    {{0, 1, 0}, {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}},  // north
    {{0, 0, -1}, {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}}, // bottom
    {{0, 0, 1}, {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}},  // top
};

} // namespace

// --- §2 The mesh ------------------------------------------------------------------------------

float occlusionWeight(int du, int dv) {
    // The voxel's distance from the corner, in the largest of its axes (the normal's axis is
    // always half a voxel, never the largest).
    const float reach = std::max({std::abs(du + 0.5f), std::abs(dv + 0.5f), 0.5f});
    return (OCCLUSION_REACH - reach) / OCCLUSION_REACH;
}

float cornerOcclusion(const VoxelModel &model, glm::ivec3 corner, int face) {
    const glm::ivec3 normal = FACES[face].normal;
    // The layer just in front of the face: offset 0 along a positive normal, -1 along a negative.
    const int axis = normal.x != 0 ? 0 : normal.y != 0 ? 1 : 2;
    const int front = normal[axis] > 0 ? 0 : -1;
    const int uAxis = (axis + 1) % 3, vAxis = (axis + 2) % 3;
    float open = 0, total = 0;
    for (int du = -OCCLUSION_REACH; du < OCCLUSION_REACH; du++)
        for (int dv = -OCCLUSION_REACH; dv < OCCLUSION_REACH; dv++) {
            glm::ivec3 p = corner;
            p[axis] += front;
            p[uAxis] += du;
            p[vAxis] += dv;
            const float w = occlusionWeight(du, dv);
            total += w;
            if (model.empty(p.x, p.y, p.z))
                open += w;
        }
    if (total <= 0)
        return 0;
    return std::max(OCCLUSION_FLOOR, open / total);
}

glm::vec4 cornerColour(const uint8_t rgb[3], float ao, bool halve) {
    const float darken = std::pow(ao, OCCLUSION_POWER);
    glm::vec3 c = glm::vec3(rgb[0], rgb[1], rgb[2]) / 255.0f * darken; // (no colour offset on the map)
    float grey = mesherGrey(c);
    if (grey > 1) {
        c /= grey;
        grey = 1;
    }
    // Hidden corners are pushed away from their grey: 1 for an open corner, up to 2.13.
    const float push = 3 - 2 * darken;
    for (int i = 0; i < 3; i++)
        c[i] = saturate((1 - push) * grey + push * c[i]);
    glm::vec4 corner(c, 2.0f);
    return halve ? corner * 0.5f : corner;
}

uint8_t colourByte(float value) { return (uint8_t)((int)(value * 255.0f) & 0xff); }

void meshModel(const VoxelModel &model, int firstI, int lastI, int firstJ, int lastJ, bool halve, Mesh &mesh) {
    for (int i = firstI; i <= lastI; i++)
        for (int j = firstJ; j <= lastJ; j++)
            for (int k = 0; k < model.sizeZ; k++) {
                if (model.empty(i, j, k))
                    continue;
                const bool shows[6] = {
                    i == firstI || model.empty(i - 1, j, k), i == lastI || model.empty(i + 1, j, k),
                    j == firstJ || model.empty(i, j - 1, k), j == lastJ || model.empty(i, j + 1, k),
                    model.empty(i, j, k - 1),                model.empty(i, j, k + 1),
                };
                const uint8_t *bytes = model.at(i, j, k);
                for (int face = 0; face < 6; face++) {
                    if (!shows[face])
                        continue;
                    const uint32_t first = (uint32_t)mesh.vertices.size();
                    for (const glm::ivec3 &offset : FACES[face].corners) {
                        const glm::ivec3 corner = glm::ivec3(i, j, k) + offset;
                        const glm::vec4 c = cornerColour(bytes, cornerOcclusion(model, corner, face), halve);
                        Vertex v{};
                        v.x = (float)corner.x, v.y = (float)corner.y, v.z = (float)corner.z;
                        v.r = colourByte(c.r), v.g = colourByte(c.g), v.b = colourByte(c.b), v.ao = colourByte(c.a);
                        v.normal = (uint8_t)FACE_NORMAL_INDEX[face];
                        mesh.vertices.push_back(v);
                        mesh.min = glm::min(mesh.min, glm::vec3(corner));
                        mesh.max = glm::max(mesh.max, glm::vec3(corner));
                    }
                    mesh.indices.insert(mesh.indices.end(), {first, first + 1, first + 2, first, first + 2, first + 3});
                }
            }
}

// --- §3 The light and colour steps ------------------------------------------------------------

FaceLight faceLight(glm::vec3 normal, float alpha) { return faceLight(normal, alpha, MAP_LIGHTS); }

FaceLight faceLight(glm::vec3 normal, float alpha, const LightRig &lights) {
    FaceLight f;
    f.direct = std::max(0.0f, WRAP_SLOPE * glm::dot(normal, glm::normalize(lights.towards)) + WRAP_FLOOR);
    f.back = BACK_TERM_WEIGHT * std::abs(glm::dot(normal, BACK_TERM_DIRECTION)) + BACK_TERM_WEIGHT;
    const glm::vec3 sun = saturate(f.back * lights.back + f.direct * lights.front);
    f.light = saturate(f.back * glm::vec3(lights.ambient) + sun * alpha);
    // The less green light a face gets, the more its colour is pushed from grey.
    f.push = std::clamp((lights.ambient.w - f.light.g) * 0.5f + 1.0f, 1.0f, 2.0f);
    return f;
}

glm::vec3 vertexColour(glm::vec4 vertex, glm::vec3 normal, glm::vec3 material, float white) {
    return vertexColour(vertex, normal, MAP_LIGHTS, material, white, 0.0f);
}

glm::vec3 vertexColour(glm::vec4 vertex, glm::vec3 normal, const LightRig &lights, glm::vec3 material, float white, float glow) {
    const FaceLight f = faceLight(normal, vertex.a, lights);
    const glm::vec3 doubled = 2.0f * glm::vec3(vertex);
    const float grey = shaderGrey(doubled);
    const glm::vec3 pushed = glm::clamp(f.push * doubled + (1 - f.push) * grey, glm::vec3(0), glm::vec3(2));
    glm::vec3 lit = f.light * material * (glow < 0 ? glm::vec3(grey) : pushed);
    if (glow > 0) // a glowing instance shows its own colour, unlit, by its glow
        lit = glm::mix(lit, doubled * material, glow);
    return saturate(lit + white * (1.0f - lit));
}

glm::vec3 saturationLift(glm::vec3 c) {
    const float grey = shaderGrey(c);
    const float lift = 1 + 0.25f * (1 - grey);
    return lift * c + (1 - lift) * grey;
}

float fogAmount(float viewDistance) {
    const float scale = 1.0f / FOG_DISTANCE;
    const float power = 1 - 0.75f * std::exp(-403.95462f * scale);
    const float q = std::min(1.0f, std::pow(viewDistance * viewDistance * scale * scale, power));
    return smooth(q);
}

glm::vec3 greenBlueGrade(glm::vec3 c) {
    const float grey = shaderGrey(c);
    return glm::vec3(c.r, 0.65f * c.g + 0.35f * grey, 0.65f * c.b + 0.35f * grey);
}

glm::vec3 sCurve(glm::vec3 c) { return glm::vec3(smooth(c.r), smooth(c.g), smooth(c.b)); }

glm::vec3 pixelColour(glm::vec3 vertex, float viewDistance, glm::vec3 fogColour) {
    glm::vec3 c = saturationLift(vertex);
    // The fog's second step, toward the sky's gradient by the fog amount to the fourth, is
    // about a millionth on the map and left out.
    c = glm::mix(c, fogColour, fogAmount(viewDistance));
    return sCurve(greenBlueGrade(saturate(c)));
}

// --- §4 The camera ----------------------------------------------------------------------------

float viewScaleFactor(float viewScale) { return viewScale * 0.6f * 80.0f / 120.0f * 16.0f / 8.0f; }

float mapYawDegrees(float cameraYaw) {
    // The game's camera yaw is CubeVulkan's less a quarter turn (main.cpp's control input); the
    // map turns the other way.
    const float gameYaw = glm::degrees(cameraYaw) - 90.0f;
    return -gameYaw;
}

float mapPitchDegrees(float cameraPitch) {
    // The game's pitch is 90 degrees looking level and 180 looking straight down; CubeVulkan's
    // is how far the camera looks down, in radians.
    const float gamePitch = 90.0f + glm::degrees(cameraPitch);
    return std::min(-gamePitch, FLATTEST_PITCH);
}

MapCamera mapCamera(float cameraYaw, float cameraPitch, float viewScale, glm::vec2 centre, glm::vec2 screen) {
    MapCamera camera;
    camera.yawDegrees = mapYawDegrees(cameraYaw);
    camera.pitchDegrees = mapPitchDegrees(cameraPitch);
    camera.scale = viewScaleFactor(viewScale);
    camera.screen = screen;
    // Turn by the yaw, tilt by the pitch, scale, then push the map in front of the camera.
    glm::mat4 view = glm::translate(glm::mat4(1), glm::vec3(0, 0, CAMERA_DISTANCE));
    view = glm::scale(view, glm::vec3(camera.scale));
    view = glm::rotate(view, glm::radians(camera.pitchDegrees), glm::vec3(1, 0, 0));
    view = glm::rotate(view, glm::radians(camera.yawDegrees), glm::vec3(0, 0, 1));
    camera.view = view;
    const float focal = 1.0f / std::tan(HALF_FIELD_OF_VIEW);
    const float aspect = screen.x / std::max(screen.y, 1e-4f);
    // The wrapper converts each eased screen centre to integer pixels first.
    const glm::vec2 pixel(float(int32_t(screen.x * centre.x)), float(int32_t(screen.y * centre.y)));
    const glm::vec2 shift = 2.f * (pixel - screen * .5f) / screen;
    camera.lens = glm::vec4(-focal / aspect, focal, shift.x, shift.y);
    return camera;
}

glm::vec4 clipOf(const MapCamera &camera, glm::vec3 p, float depthOffset) {
    const glm::vec4 &lens = camera.lens;
    return glm::vec4(lens.x * p.x + lens.z * p.z, lens.y * p.y + lens.w * p.z,
                     (DEPTH_SCALE + DEPTH_SHIFT) * p.z + DEPTH_BIAS - depthOffset, p.z);
}

float regionFade(double msSinceBuilt) {
    const float fade = (float)msSinceBuilt * FADE_RATE;
    if (fade < 0)
        return 0;
    return fade > FADE_SNAP ? 1.0f : fade;
}

} // namespace worldmap
