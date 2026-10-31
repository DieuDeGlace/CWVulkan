#pragma once
// How the world map looks: the mesh of a map model, the map's own light rig and colour steps,
// and its camera (docs/specs/worldmap-rendering.md, sections cited as §n).
//
// The map does not share the world's look. Whatever the world's style (Alpha or Steam), the
// map's models are meshed with soft corner occlusion that darkens and saturates (§2), then lit
// by a fixed rig of the map's own and graded per pixel (§3) by shaders/map.vert and map.frag.
// The functions here are the CPU side of that: the mesher, the camera, and the same light and
// colour steps the shaders run, which the tests check against the spec's tables.
#include "Mesher.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <array>

namespace worldmap {

// --- §2 The mesh ------------------------------------------------------------------------------

// A voxel model as the mesher reads it: sizeX x sizeY x sizeZ voxels of three colour bytes,
// x fastest, then y, then z. A voxel whose three bytes are 0 is empty, and so is every voxel
// outside the model.
struct VoxelModel {
    int sizeX = 0, sizeY = 0, sizeZ = 0;
    const uint8_t *rgb = nullptr;
    bool inside(int i, int j, int k) const {
        return i >= 0 && j >= 0 && k >= 0 && i < sizeX && j < sizeY && k < sizeZ;
    }
    const uint8_t *at(int i, int j, int k) const {
        return rgb + (((size_t)k * sizeY + j) * sizeX + i) * 3;
    }
    bool empty(int i, int j, int k) const {
        if (!inside(i, j, k))
            return true;
        const uint8_t *v = at(i, j, k);
        return (v[0] | v[1] | v[2]) == 0;
    }
};

// The faces in the order the mesher emits them (§2.2), and the normal index each one carries
// (the index the shader turns back into a normal: Mesher.h's FACE_NORMALS).
enum MapFace : int { WEST, EAST, SOUTH, NORTH, BOTTOM, TOP };
constexpr int FACE_NORMAL_INDEX[6] = {1, 0, 3, 2, 5, 4};

// How far the corner occlusion looks (model +0x9c): a 6 x 6 window of voxels.
constexpr int OCCLUSION_REACH = 3;
// No corner is darker than this before the power (0x4ac964).
constexpr float OCCLUSION_FLOOR = 0.5f;

// The weight of one voxel of the occlusion window, at offsets (du, dv) from the corner along
// the face's two axes (each -3 .. 2): 5/6 for the inner 2 x 2, 1/2 for the next ring, 1/6 for
// the outer ring.
float occlusionWeight(int du, int dv);

// How open a face corner is (§2.4): the weighted share of empty voxels in the 6 x 6 window in
// front of the face, at least OCCLUSION_FLOOR. `corner` is the lattice point, `face` a MapFace.
float cornerOcclusion(const VoxelModel &model, glm::ivec3 corner, int face);

// A corner's colour from its voxel's bytes and its openness (§2.5): darkened by ao^1.2 and
// pushed away from its grey by 3 - 2 ao^1.2. Alpha is 2. `halve` halves all four, as for
// models loaded from .cub files; the map's regions are not halved (§2.1).
glm::vec4 cornerColour(const uint8_t rgb[3], float ao, bool halve);

// A colour value as the vertex keeps it (§2.3): the low 8 bits of trunc(value x 255).
uint8_t colourByte(float value);

// Meshes a model as the map does (§2.2, §2.3): every non-empty voxel in columns
// firstI .. lastI and firstJ .. lastJ, all heights. A face shows when the voxel beyond it is
// empty, and the side walls along the edge columns always show, so the block is closed. Each
// face has its own four corners (in the spec's order) and the triangles (0, 1, 2), (0, 2, 3).
// Vertices are in voxel units; their fourth colour byte is the corner's alpha.
void meshModel(const VoxelModel &model, int firstI, int lastI, int firstJ, int lastJ, bool halve, Mesh &mesh);

// --- §3 The light and colour steps ------------------------------------------------------------

// The map pass's light rig (§3.1).
inline const glm::vec3 LIGHT_FRONT{0.75f, 0.7f, 0.3f};
inline const glm::vec3 LIGHT_BACK{0.5f, 0.5f, 0.5f};
inline const glm::vec3 LIGHT_TOWARDS{0.2f, 0.5f, 1.0f}; // normalised where it is used
inline const glm::vec4 LIGHT_AMBIENT{0.25f, 0.3f, 0.7f, 1.0f};
// The shader's own fixed direction for the second, unsigned light term (§3.2 step 4).
inline const glm::vec3 BACK_TERM_DIRECTION{0.597022f, 0.398015f, 0.696526f};
// The map's fog reaches this far (map units), so it barely tints the map (§3.3 step 3).
constexpr float FOG_DISTANCE = 20000.0f;

// The light one face receives, by its normal in map space (§3.2 steps 3-6): the direct term d,
// the back term h, the light after step 5 and the grey push k of step 6. `alpha` is the vertex
// colour's alpha (254/255 for a map region) and multiplies the sun terms.
struct FaceLight {
    float direct = 0, back = 0;
    glm::vec3 light{0};
    float push = 1;
};
FaceLight faceLight(glm::vec3 normal, float alpha = 254.0f / 255.0f);

// The vertex shader's output colour (§3.2 steps 6-8) for vertex colour `vertex` (as the shader
// reads it, 0..1), a face normal in map space, the instance's colour and white amount.
glm::vec3 vertexColour(glm::vec4 vertex, glm::vec3 normal, glm::vec3 material = glm::vec3(1), float white = 0);

// The same model shader under another pass's lights (renderer slot +0xc0: front colour, back
// colour, the direction toward the light, ambient with its w), as the HUD's overlay pass sets
// them (hud-money.md §3). `glow` is the instance's glow number (§3.2 steps 6 and 7): above 0
// the colour moves toward the unlit doubled colour by it, below 0 it is the doubled colour's
// grey.
struct LightRig {
    glm::vec3 front, back, towards;
    glm::vec4 ambient;
};
inline const LightRig MAP_LIGHTS{LIGHT_FRONT, LIGHT_BACK, LIGHT_TOWARDS, LIGHT_AMBIENT};
FaceLight faceLight(glm::vec3 normal, float alpha, const LightRig &lights);
glm::vec3 vertexColour(glm::vec4 vertex, glm::vec3 normal, const LightRig &lights, glm::vec3 material, float white, float glow);

// The pixel shader's steps (§3.3).
glm::vec3 saturationLift(glm::vec3 c);           // step 2
float fogAmount(float viewDistance);              // step 3: how far toward the fog colour
glm::vec3 greenBlueGrade(glm::vec3 c);           // step 5
glm::vec3 sCurve(glm::vec3 c);                    // step 6
// All of them in order, from the interpolated vertex colour to the pixel's colour.
glm::vec3 pixelColour(glm::vec3 vertex, float viewDistance, glm::vec3 fogColour);

// --- §4 The camera ----------------------------------------------------------------------------

constexpr float HALF_FIELD_OF_VIEW = 3.14159265f / 8; // 45 degrees in all
constexpr float CAMERA_DISTANCE = 300.0f;               // map units in front of the camera
constexpr float FLATTEST_PITCH = -120.0f;               // degrees: 30 degrees below the horizon
// The clip depth terms (near 0.1, far 1000) and the device depth shift of the whole map.
constexpr float DEPTH_SCALE = 1.0001f, DEPTH_BIAS = -0.10001f, DEPTH_SHIFT = -0.1f;
// The per-instance depth offsets (§3.1, map-markers.md §2.5).
constexpr float REGION_DEPTH_OFFSET = 0.5f;
constexpr float FIRST_MARKER_LAYER = 2.0f;
// A new region fades in at this rate per ms, and is whole above FADE_SNAP (§4.3).
constexpr float FADE_RATE = 0.005f, FADE_SNAP = 0.99f;

// The map's scale for a view scale (§4.2): 0.6 x 80 / 120 x 16 / 8 = 0.8 of it.
float viewScaleFactor(float viewScale);

// The map's camera for one frame. `view` takes map units (relative to the map's centre) to the
// camera's space, which looks along +z with +y up. The lens then takes a camera-space point
// (x, y, z) to the device as (lens.x x + lens.z z, lens.y y + lens.w z, ...) over z: lens.x is
// -f / aspect (the x axis is mirrored), lens.y is f, lens.zw the shift that puts the map's
// centre at its place on the screen.
struct MapCamera {
    float yawDegrees = 0, pitchDegrees = 0, scale = 1;
    glm::mat4 view{1};
    glm::vec4 lens{0};
    glm::vec2 screen{1}; // window pixels
};
// From the world camera's turn as CubeVulkan keeps it (yaw: the direction from the player to
// the eye, radians from +x; pitch: how far the eye is above the player, radians), the view
// scale, the screen centre (fractions of the window, from the bottom left) and the window.
MapCamera mapCamera(float cameraYaw, float cameraPitch, float viewScale, glm::vec2 centre, glm::vec2 screen);
// The map's turns (§4.2) from the world camera's: yaw = -(game yaw), pitch = min(-(game
// pitch), -120), in degrees.
float mapYawDegrees(float cameraYaw);
float mapPitchDegrees(float cameraPitch);

// A camera-space point through the lens, as D3D's clip space (y up, depth 0..1 over w), with
// the instance's depth offset taken off the clip depth.
glm::vec4 clipOf(const MapCamera &camera, glm::vec3 viewPoint, float depthOffset = 0);

// How far a region built `msSinceBuilt` ago has faded in (§4.3).
float regionFade(double msSinceBuilt);
// Native hover border: float world clock, full header center and staged fixed-delta conversion.
float borderOpacity(float worldMilliseconds);
glm::mat4 borderPlacement(const std::array<int32_t, 3> &point, const std::array<int64_t, 3> &center, int unit, float viewScale,
                          glm::vec3 allocation);

} // namespace worldmap
