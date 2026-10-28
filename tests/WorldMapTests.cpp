// Checks of the world map's look against docs/specs/worldmap-rendering.md: the occlusion
// window's weights (2.4), the corner colour (2.5), the mesh's faces (2.2, 2.3), the light per
// face (3.4), the pixel steps (3.3) and the camera (4.2, 4.3).
#include "MapLook.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {
int failures = 0;
#define MAP_CHECK(x)                                                                               \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                              \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

bool near(float a, float b, float tolerance) { return std::abs(a - b) <= tolerance; }
bool near(glm::vec3 a, glm::vec3 b, float tolerance) {
    return near(a.r, b.r, tolerance) && near(a.g, b.g, tolerance) && near(a.b, b.b, tolerance);
}

void occlusionWeights() {
    // 2.4: the inner 2 x 2 weigh 5/6, the next ring 1/2, the outer ring 1/6; 12 2/3 in all.
    MAP_CHECK(near(worldmap::occlusionWeight(0, 0), 5.0f / 6, 1e-6f));
    MAP_CHECK(near(worldmap::occlusionWeight(-1, -1), 5.0f / 6, 1e-6f));
    MAP_CHECK(near(worldmap::occlusionWeight(1, 0), 0.5f, 1e-6f));
    MAP_CHECK(near(worldmap::occlusionWeight(-2, -1), 0.5f, 1e-6f));
    MAP_CHECK(near(worldmap::occlusionWeight(2, 0), 1.0f / 6, 1e-6f));
    MAP_CHECK(near(worldmap::occlusionWeight(-3, -3), 1.0f / 6, 1e-6f));
    float total = 0;
    for (int u = -3; u < 3; u++)
        for (int v = -3; v < 3; v++)
            total += worldmap::occlusionWeight(u, v);
    MAP_CHECK(near(total, 12.0f + 2.0f / 3, 1e-4f));

    // A lone voxel: every window in front of a face is empty, so ao = 1. On a floor, the top
    // corners are open and the side corners at the floor half covered, so the floor 0.5.
    std::vector<uint8_t> one(3, 200);
    const worldmap::VoxelModel lone{1, 1, 1, one.data()};
    MAP_CHECK(near(worldmap::cornerOcclusion(lone, {0, 0, 1}, worldmap::TOP), 1, 1e-6f));
    std::vector<uint8_t> slab(8 * 8 * 2 * 3, 0);
    for (int i = 0; i < 8 * 8 * 3; i++)
        slab[i] = 100; // the bottom layer full
    const worldmap::VoxelModel floor{8, 8, 2, slab.data()};
    MAP_CHECK(near(worldmap::cornerOcclusion(floor, {4, 4, 1}, worldmap::TOP), 1, 1e-6f));
    // A west face's corner one layer above the floor: the floor fills one row of its window
    // (weights 1/6, 1/2, 5/6, 5/6, 1/2, 1/6 = 3 of 38/3), so 29/38 is open.
    MAP_CHECK(near(worldmap::cornerOcclusion(floor, {4, 4, 1}, worldmap::WEST), 29.0f / 38, 1e-5f));
}

void cornerColours() {
    // 2.5: an open corner keeps its colour, alpha 2; a hidden one is darker and more saturated.
    const uint8_t green[3] = {60, 180, 40};
    const glm::vec4 open = worldmap::cornerColour(green, 1, false);
    MAP_CHECK(near(glm::vec3(open), glm::vec3(60, 180, 40) / 255.0f, 1e-6f));
    MAP_CHECK(open.a == 2.0f);
    MAP_CHECK(worldmap::colourByte(open.a) == 254); // 510's low byte
    const glm::vec4 hidden = worldmap::cornerColour(green, 0.5f, false);
    const float a = std::pow(0.5f, 1.2f), s = 3 - 2 * a;
    const glm::vec3 dark = glm::vec3(60, 180, 40) / 255.0f * a;
    const float grey = 0.3f * dark.r + 0.59f * dark.g + 0.11f * dark.b;
    MAP_CHECK(near(hidden.g, std::min(1.0f, (1 - s) * grey + s * dark.g), 1e-6f));
    MAP_CHECK(near(hidden.r, std::max(0.0f, (1 - s) * grey + s * dark.r), 1e-6f));
    MAP_CHECK(hidden.g - hidden.r > (open.g - open.r) * a); // more saturated than just darkened
    // .cub models are halved, alpha too.
    MAP_CHECK(near(glm::vec3(worldmap::cornerColour(green, 1, true)), glm::vec3(open) * 0.5f, 1e-6f));
    MAP_CHECK(worldmap::cornerColour(green, 1, true).a == 1.0f);
}

void meshFaces() {
    // 2.2: a lone voxel is a closed cube: 6 faces in the order west, east, south, north,
    // bottom, top; each with 4 corners and the triangles (0, 1, 2), (0, 2, 3).
    std::vector<uint8_t> one(3, 200);
    Mesh mesh;
    worldmap::meshModel({1, 1, 1, one.data()}, 0, 0, 0, 0, false, mesh);
    MAP_CHECK(mesh.vertices.size() == 24 && mesh.indices.size() == 36);
    const int normals[6] = {1, 0, 3, 2, 5, 4};
    for (int f = 0; f < 6 && mesh.vertices.size() == 24; f++)
        MAP_CHECK(mesh.vertices[f * 4].normal == normals[f]);
    if (mesh.indices.size() == 36) {
        MAP_CHECK(mesh.indices[0] == 0 && mesh.indices[1] == 1 && mesh.indices[2] == 2);
        MAP_CHECK(mesh.indices[3] == 0 && mesh.indices[4] == 2 && mesh.indices[5] == 3);
        // The west face's corners: (0,0,0), (0,0,1), (0,1,1), (0,1,0).
        MAP_CHECK(mesh.vertices[1].z == 1 && mesh.vertices[1].y == 0 && mesh.vertices[2].y == 1 && mesh.vertices[3].z == 0);
    }
    MAP_CHECK(mesh.vertices[0].ao == 254);
    // Two voxels side by side drawn as edge columns 0 and 1: no wall between them, but walls
    // along the drawn edges even where the model goes on (a closed block).
    std::vector<uint8_t> row(3 * 3, 200);
    Mesh walls;
    worldmap::meshModel({3, 1, 1, row.data()}, 0, 1, 0, 0, false, walls);
    MAP_CHECK(walls.indices.size() == 10 * 6); // 2 x (south, north, bottom, top) + west + east
}

void faceLights() {
    // 3.4: the light per face (after step 5) and the grey push k (step 6).
    struct Row {
        glm::vec3 normal;
        float direct, back;
        glm::vec3 light;
        float push;
    } rows[] = {
        {{0, 0, 1}, 0.916f, 0.848f, {1, 1, 1}, 1.00f},
        {{0, 1, 0}, 0.608f, 0.699f, {0.98f, 0.98f, 1}, 1.01f},
        {{1, 0, 0}, 0.423f, 0.799f, {0.91f, 0.93f, 1}, 1.03f},
        {{-1, 0, 0}, 0.177f, 0.799f, {0.73f, 0.76f, 1}, 1.12f},
        {{0, 0, -1}, 0, 0.848f, {0.63f, 0.68f, 1}, 1.16f},
        {{0, -1, 0}, 0, 0.699f, {0.52f, 0.56f, 0.84f}, 1.22f},
    };
    for (const Row &r : rows) {
        const worldmap::FaceLight f = worldmap::faceLight(r.normal);
        MAP_CHECK(near(f.direct, r.direct, 0.0006f));
        MAP_CHECK(near(f.back, r.back, 0.0006f));
        MAP_CHECK(near(f.light, r.light, 0.006f));
        MAP_CHECK(near(f.push, r.push, 0.006f));
    }
    // 3.2 steps 6-8: a doubled grey top clips at white; a black instance colour is black, and
    // the white amount lifts it (the outline's pulse).
    MAP_CHECK(near(worldmap::vertexColour({0.5f, 0.5f, 0.5f, 254 / 255.0f}, {0, 0, 1}), glm::vec3(1), 1e-5f));
    MAP_CHECK(near(worldmap::vertexColour({0.3f, 0.6f, 0.2f, 1}, {0, 0, 1}, glm::vec3(0)), glm::vec3(0), 1e-6f));
    MAP_CHECK(near(worldmap::vertexColour({0.3f, 0.6f, 0.2f, 1}, {0, 0, 1}, glm::vec3(0), 0.5f), glm::vec3(0.5f), 1e-6f));
}

void pixelSteps() {
    // 3.3 step 2: grey stays grey, colours move away from it by 1 + 0.25 (1 - grey).
    MAP_CHECK(near(worldmap::saturationLift(glm::vec3(0.4f)), glm::vec3(0.4f), 1e-6f));
    const glm::vec3 c(0.2f, 0.6f, 0.3f);
    const float g = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b, t = 1 + 0.25f * (1 - g);
    MAP_CHECK(near(worldmap::saturationLift(c), t * c + (1 - t) * g, 1e-6f));
    // Step 3: about 3 % fog at the camera's distance of 300.
    MAP_CHECK(near(worldmap::fogAmount(300), 0.0325f, 0.002f));
    MAP_CHECK(worldmap::fogAmount(0) == 0);
    // Step 5: red kept, green and blue a third of the way to grey.
    const glm::vec3 graded = worldmap::greenBlueGrade(glm::vec3(1, 0, 0));
    MAP_CHECK(near(graded, glm::vec3(1, 0.35f * 0.299f, 0.35f * 0.299f), 1e-6f));
    // Step 6: 3x^2 - 2x^3.
    MAP_CHECK(near(worldmap::sCurve(glm::vec3(0, 0.5f, 1)), glm::vec3(0, 0.5f, 1), 1e-6f));
    MAP_CHECK(near(worldmap::sCurve(glm::vec3(0.25f)).r, 0.15625f, 1e-6f));
    // All in order: white stays white without fog.
    MAP_CHECK(near(worldmap::pixelColour(glm::vec3(1), 0, glm::vec3(0)), glm::vec3(1), 1e-6f));
}

void camera() {
    // 4.2: k = 0.8 x viewScale; the pitch never flatter than -120; yaw from the camera.
    MAP_CHECK(near(worldmap::viewScaleFactor(1), 0.8f, 1e-6f));
    MAP_CHECK(near(worldmap::viewScaleFactor(1.8f), 1.44f, 1e-5f));
    MAP_CHECK(near(worldmap::mapPitchDegrees(0.2f), -120, 1e-4f));
    MAP_CHECK(near(worldmap::mapPitchDegrees(glm::radians(60.0f)), -150, 1e-3f));
    MAP_CHECK(near(worldmap::mapYawDegrees(glm::radians(90.0f)), 0, 1e-4f));
    // The centre lands on the screen centre given; one map unit spans 0.00644 viewScale in y.
    const worldmap::MapCamera cam = worldmap::mapCamera(glm::radians(90.0f), glm::radians(90.0f), 1, {0.5f, 0.5f}, {1600, 900});
    glm::vec4 clip = worldmap::clipOf(cam, glm::vec3(cam.view * glm::vec4(0, 0, 0, 1)));
    MAP_CHECK(near(clip.x / clip.w, 0, 1e-5f) && near(clip.y / clip.w, 0, 1e-5f));
    MAP_CHECK(near(clip.w, 300, 1e-3f));
    // Looking straight down with the camera facing south (CubeVulkan's yaw 90 degrees puts the
    // eye north of the player): south is up the screen, so a unit north is 0.00644 down, and
    // east is on the left.
    const glm::vec4 north = worldmap::clipOf(cam, glm::vec3(cam.view * glm::vec4(0, 1, 0, 1)));
    MAP_CHECK(near(north.y / north.w, -2.4142f * 0.8f / 300, 2e-5f));
    const glm::vec4 east = worldmap::clipOf(cam, glm::vec3(cam.view * glm::vec4(1, 0, 0, 1)));
    MAP_CHECK(east.x / east.w < 0);
    // 4.3: a region fades in over 200 ms.
    MAP_CHECK(worldmap::regionFade(-5) == 0);
    MAP_CHECK(near(worldmap::regionFade(100), 0.5f, 1e-6f));
    MAP_CHECK(worldmap::regionFade(199) == 1.0f);
}

} // namespace

int worldMapLookTests() {
    failures = 0;
    occlusionWeights();
    cornerColours();
    meshFaces();
    faceLights();
    pixelSteps();
    camera();
    std::printf("world map look: %s\n", failures ? "FAILED" : "ok");
    return failures;
}
