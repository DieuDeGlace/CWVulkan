// The HUD's turning gold coin (docs/specs/hud-money.md). See HudCoin.h.
#include "HudCoin.h"

#include "Assets.h"
#include "CubModel.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace hudcoin {

namespace {

// §2.1: one turn every TURN_PERIOD_MS of world time; the coin tipped toward the viewer.
constexpr int32_t TURN_PERIOD_MS = 10000;
constexpr float TILT_DEGREES = -30.0f;
// §2.1: the scale is GUI_SCALE_FACTOR x the GUI scale, kept the same size in pixels whatever
// the window's height by REFERENCE_HEIGHT / height.
constexpr float GUI_SCALE_FACTOR = 0.004f, REFERENCE_HEIGHT = 1024.0f;
// §2.2: the coin's middle and its shadow's, in GUI units; the shadow sits behind the coin.
constexpr glm::vec2 COIN_AT{45, 52}, SHADOW_AT{50, 57};
constexpr float SHADOW_DEPTH = -0.5f;
constexpr float COIN_GLOW = 1.0f;

// §2.3: an overlay model sits one unit in front of the overlay camera.
constexpr glm::vec3 IN_FRONT{0, 0, 1};

// §3: the overlay lens. Its far plane is 64 times a renderer distance the spec leaves unnamed;
// at the coin's depth only its being far matters.
constexpr float FIELD_OF_VIEW_DEGREES = 50.0f, NEAR_PLANE = 0.05f, FAR_PLANE = 64.0f * 128.0f;
// §3: every entry's projection is moved half a unit toward the viewer in clip depth.
constexpr float ENTRY_DEPTH_SHIFT = -0.5f;

// How many baked meshes are kept before the oldest goes.
constexpr size_t BAKED_KEPT = 8;

} // namespace

const worldmap::LightRig OVERLAY_LIGHTS{
    {0.7f, 0.7f, 0.2f},                      // front colour
    {0.5f, 0.5f, 0.5f},                      // back colour
    glm::normalize(glm::vec3(1, 1, -1)),     // toward the light
    {0.3f, 0.3f, 0.8f, 1.0f},                // ambient
};

float turnDegrees(double worldClockMs) {
    const int32_t clock = (int32_t)(int64_t)worldClockMs;
    return (float)(clock % TURN_PERIOD_MS) * 0.0001f * 360.0f;
}

std::vector<OverlayDraw> coinDraws(float guiScale, float height, double worldClockMs) {
    OverlayDraw coin;
    coin.angles = {TILT_DEGREES, turnDegrees(worldClockMs), 0.0f};
    coin.scale = (guiScale * GUI_SCALE_FACTOR) * (REFERENCE_HEIGHT / std::max(1.0f, height));
    coin.glow = COIN_GLOW;
    OverlayDraw shadow = coin;
    shadow.screen = SHADOW_AT * guiScale;
    shadow.colour = {0, 0, 0, 1};
    shadow.depth = SHADOW_DEPTH;
    coin.screen = COIN_AT * guiScale;
    coin.colour = {1, 1, 1, 1};
    return {shadow, coin};
}

glm::mat4 modelMatrix(const OverlayDraw &d, glm::vec3 size) {
    // Each step acts in the model's own frame (on the right), as the game's matrix helpers do.
    glm::mat4 m = glm::translate(glm::mat4(1), IN_FRONT);
    m = glm::rotate(m, glm::radians(d.angles.x), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(d.angles.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(d.angles.z), glm::vec3(0, 0, 1));
    m = glm::scale(m, glm::vec3(d.scale));
    return glm::translate(m, size * -0.5f); // centred
}

glm::mat4 overlayProjection(float width, float height) {
    const float f = 1.0f / std::tan(glm::radians(FIELD_OF_VIEW_DEGREES) * 0.5f);
    const float aspect = -(width / std::max(1.0f, height)); // negative: the overlay mirrors x
    glm::mat4 p(0.0f);
    p[0][0] = f / aspect;
    p[1][1] = f;
    p[2][2] = FAR_PLANE / (FAR_PLANE - NEAR_PLANE);
    p[2][3] = 1.0f;
    p[3][2] = -(NEAR_PLANE * FAR_PLANE) / (FAR_PLANE - NEAR_PLANE);
    return p;
}

glm::mat4 entryClip(const OverlayDraw &d, glm::vec3 size, float width, float height) {
    width = std::max(1.0f, width), height = std::max(1.0f, height);
    // The shift that lands the model's middle on its screen point (after the divide by w).
    glm::mat4 shift(1);
    shift[3] = glm::vec4(2.0f * (d.screen.x - width * 0.5f) / width, ((d.screen.y - height * 0.5f) / height) * -2.0f,
                         ENTRY_DEPTH_SHIFT, 1.0f);
    glm::mat4 clip = shift * overlayProjection(width, height) * modelMatrix(d, size);
    clip[3][2] -= d.depth; // the depth number is taken off the clip depth
    // Direct3D's clip y runs up, the renderer's (Vulkan's) down.
    const glm::mat4 flipY = glm::scale(glm::mat4(1), glm::vec3(1, -1, 1));
    return flipY * clip;
}

glm::vec3 faceColour(glm::vec4 vertex, glm::vec3 modelNormal, const glm::mat4 &model, const OverlayDraw &d) {
    // The view is the identity: the normal through the model matrix is already in the overlay
    // camera's frame, where the lights are given.
    const glm::vec3 normal = glm::normalize(glm::mat3(model) * modelNormal);
    const glm::vec3 lit = worldmap::vertexColour(vertex, normal, OVERLAY_LIGHTS, glm::vec3(d.colour), 0.0f, d.glow);
    // The pixel shader's steps. Its fog (distance 10000 in this pass) is a ten-thousandth at
    // the coin's depth and is left out.
    const glm::vec3 lifted = worldmap::saturationLift(lit);
    return worldmap::sCurve(worldmap::greenBlueGrade(glm::clamp(lifted, glm::vec3(0), glm::vec3(1))));
}

uint32_t Coin::meshFor(Renderer &renderer, const glm::mat4 &model, const OverlayDraw &d) {
    Mesh mesh = base;
    uint64_t key = 1469598103934665603ull; // FNV-1a over the baked colours
    for (Vertex &v : mesh.vertices) {
        const int *n = FACE_NORMALS[v.normal];
        const glm::vec4 read(v.r / 255.0f, v.g / 255.0f, v.b / 255.0f, v.ao / 255.0f);
        const glm::vec3 c = faceColour(read, glm::vec3(n[0], n[1], n[2]), model, d);
        v.r = (uint8_t)std::lround(c.r * 255), v.g = (uint8_t)std::lround(c.g * 255), v.b = (uint8_t)std::lround(c.b * 255);
        v.flags = NATIVE_MENU;
        for (uint8_t byte : {v.r, v.g, v.b})
            key = (key ^ byte) * 1099511628211ull;
    }
    if (auto it = baked.find(key); it != baked.end())
        return it->second;
    const uint32_t id = renderer.upload(mesh);
    baked[key] = id;
    bakedOrder.push_back(key);
    if (bakedOrder.size() > BAKED_KEPT) {
        renderer.release(baked[bakedOrder.front()]);
        baked.erase(bakedOrder.front());
        bakedOrder.erase(bakedOrder.begin());
    }
    return id;
}

void Coin::draw(Renderer &renderer, const Assets &assets, std::vector<DrawItem> &opaque, float width, float height,
                float guiScale, double worldClockMs) {
    if (!tried) {
        tried = true;
        CubModel cub;
        if (auto path = assets.resolve(std::filesystem::path("sprites") / (std::string(COIN_SPRITE) + ".cub")); path && cub.load(*path)) {
            // Meshed as the game meshes every voxel model (worldmap-rendering.md §2), its corner
            // at the origin.
            worldmap::meshModel(worldmap::VoxelModel{cub.sx, cub.sy, cub.sz, cub.rgb.data()}, 0, cub.sx - 1, 0, cub.sy - 1, true,
                                base);
            size = glm::vec3(cub.sx, cub.sy, cub.sz);
            loaded = !base.empty();
        }
    }
    if (!loaded)
        return;
    for (const OverlayDraw &d : coinDraws(guiScale, height, worldClockMs)) {
        DrawItem item;
        item.mesh = meshFor(renderer, modelMatrix(d, size), d);
        item.model = entryClip(d, size, width, height);
        item.flags = NATIVE_MENU;
        item.castShadow = false;
        opaque.push_back(item);
    }
}

} // namespace hudcoin
