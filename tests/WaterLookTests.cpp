#include "WaterLook.h"
#include "Config.h"
#include "Renderer.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <algorithm>
#include <array>
#include <string>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <vector>
namespace {
int failures = 0;
#define WATER_CHECK(...)                                                                                                                   \
    do {                                                                                                                                   \
        if (!(__VA_ARGS__)) {                                                                                                              \
            std::printf("FAIL shared water line%d: %s\n", __LINE__, #__VA_ARGS__);                                                         \
            ++failures;                                                                                                                    \
        }                                                                                                                                  \
    } while (0)
struct Picture {
    int width = 0, height = 0;
    std::vector<uint8_t> pixels;
    bool load(const std::filesystem::path &path) {
        int components;
        auto *data = stbi_load(path.string().c_str(), &width, &height, &components, 4);
        if (!data)
            return false;
        pixels.assign(data, data + (size_t)width * height * 4);
        stbi_image_free(data);
        return true;
    }
    int channel(int x, int y, int channel) const { return pixels[((size_t)y * width + x) * 4 + channel]; }
};
void quad(Mesh &mesh, std::array<glm::vec3, 4> points, glm::u8vec3 colour, uint8_t normal, uint8_t flags = 0) {
    const auto start = (uint32_t)mesh.vertices.size();
    for (auto point : points) {
        mesh.vertices.push_back({point.x, point.y, point.z, colour.r, colour.g, colour.b, 255, normal, flags, 0, 0});
        mesh.min = glm::min(mesh.min, point);
        mesh.max = glm::max(mesh.max, point);
    }
    for (uint32_t index : {0u, 1u, 2u, 2u, 1u, 3u})
        mesh.indices.push_back(start + index);
}
} // namespace
int waterLookTests() {
    Config config;
    waterlook::Settings settings;
    settings.read(config);
    WATER_CHECK(settings.style == waterlook::Style::Omega && settings.reflections);
    config.set("Render", "Style", "Steam");
    settings.read(config);
    WATER_CHECK(settings.style == waterlook::Style::Omega);
    config.set("Water", "Style", "NATIVE");
    settings.read(config);
    WATER_CHECK(settings.style == waterlook::Style::Native);
    for (float light : {0.f, .25f, 1.f}) {
        const auto palette = waterlook::submerged(light);
        WATER_CHECK(palette.fog.b > palette.fog.g && palette.material.g > palette.material.r);
        for (auto colour : {palette.horizon, palette.zenith, palette.fog}) {
            const auto roundtrip = waterlook::steamDisplayColour(waterlook::steamShaderColour(colour));
            WATER_CHECK(glm::length(roundtrip - colour) < .00001f);
        }
    }
    std::printf("Shared water settings/palette: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}

int waterShaderTests() {
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto *window = glfwCreateWindow(256, 192, "Shared water GPU fixture", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    const auto root = std::filesystem::current_path() / "build" / "shared-water-images";
    std::filesystem::create_directories(root);
    struct Look {
        RenderStyle style;
        bool packedVertices;
        const char *name;
    };
    for (const auto look : {Look{RenderStyle::Alpha, false, "alpha"}, Look{RenderStyle::Steam, false, "steam"},
                            Look{RenderStyle::Steam, true, "steam-packed"}}) {
        glfwSetWindowSize(window, 256, 192);
        glfwPollEvents();
        Renderer renderer;
        RenderSettings settings;
        settings.style = look.style;
        settings.water.style = waterlook::Style::Omega;
        settings.vsync = settings.shadows = settings.taa = settings.smaa = false;
        settings.resolution = settings.nearResolution = 64;
        settings.fogDistance = 128;
        settings.sceneryDistance = 128;
        std::string error;
        if (!renderer.init(window, settings, error)) {
            std::printf("Shared water init: %s\n", error.c_str());
            renderer.shutdown();
            glfwDestroyWindow(window);
            glfwTerminate();
            return 1;
        }
        WorldProbe probe;
        probe.water = [](glm::vec3 point) { return point.z < 0; };
        renderer.setWorldProbe(probe);
        Mesh floor, alternateFloor, bank, water;
        quad(floor, {{{-8, -4, -2}, {8, -4, -2}, {-8, 14, -2}, {8, 14, -2}}}, {120, 180, 40}, 4);
        quad(alternateFloor, {{{-8, -4, -2}, {8, -4, -2}, {-8, 14, -2}, {8, 14, -2}}}, {40, 70, 160}, 4);
        quad(bank, {{{-3, 8, 0}, {3, 8, 0}, {-3, 8, 5}, {3, 8, 5}}}, {220, 35, 20}, 3);
        quad(water, {{{-8, -4, 0}, {8, -4, 0}, {-8, 12, 0}, {8, 12, 0}}}, {40, 100, 180}, 4, WATER_SURFACE);
        if (look.packedVertices)
            for (auto *mesh : {&floor, &alternateFloor, &bank, &water})
                for (auto &vertex : mesh->vertices) {
                    vertex.r /= 2;
                    vertex.g /= 2;
                    vertex.b /= 2;
                    vertex.flags |= STEAM_VERTEX;
                }
        DrawItem floorDraw, alternateFloorDraw, bankDraw, waterDraw;
        floorDraw.mesh = renderer.upload(floor);
        alternateFloorDraw.mesh = renderer.upload(alternateFloor);
        bankDraw.mesh = renderer.upload(bank);
        waterDraw.mesh = renderer.upload(water);
        floorDraw.kind = alternateFloorDraw.kind = bankDraw.kind = waterDraw.kind = DrawKind::Terrain;
        const uint8_t white[] = {255, 255, 255, 255};
        WATER_CHECK(renderer.setOverlayAtlas(1, 1, white));
        bool showOverlay = false;
        Camera camera;
        camera.eye = {0, -8, 6};
        camera.target = {0, 3, 0};
        Lighting lighting;
        lighting.hour = 12;
        lighting.timeMs = 43200000;
        lighting.lightDir = glm::normalize(glm::vec3(1, -1, 2));
        lighting.front = glm::vec3(1);
        lighting.back = glm::vec3(.4f);
        lighting.ambient = glm::vec3(.2f);
        const std::string prefix = look.name;
        for (int frame = 0; frame < 32; ++frame) {
            glfwPollEvents();
            renderer.frame(camera, lighting, {}, frame * .1, {floorDraw}, {waterDraw});
        }
        auto capture = [&](const std::string &name, const std::vector<DrawItem> &opaque, bool reflections, double clock = 4.0) {
            renderer.settings.water.reflections = reflections;
            const auto path = root / (prefix + "-" + name + ".png");
            for (int frame = 0; frame < 3; ++frame) {
                glfwPollEvents();
                if (showOverlay)
                    renderer.overlay({{10, 10, .5f, .5f, 255, 255, 255, 255},
                                      {40, 10, .5f, .5f, 255, 255, 255, 255},
                                      {10, 35, .5f, .5f, 255, 255, 255, 255},
                                      {10, 35, .5f, .5f, 255, 255, 255, 255},
                                      {40, 10, .5f, .5f, 255, 255, 255, 255},
                                      {40, 35, .5f, .5f, 255, 255, 255, 255}});
                if (frame == 2)
                    renderer.capture(path.string());
                renderer.frame(camera, lighting, {}, clock, opaque, {waterDraw});
            }
            Picture picture;
            WATER_CHECK(picture.load(path));
            return picture;
        };
        auto noBank = capture("no-bank", {floorDraw}, true);
        auto reflected = capture("reflected-bank", {floorDraw, bankDraw}, true);
        auto noReflection = capture("no-reflection", {floorDraw, bankDraw}, false);
        int changed = 0, redGain = 0, switchPixels = 0;
        if (!noBank.pixels.empty() && !reflected.pixels.empty() && !noReflection.pixels.empty() && reflected.width == noBank.width) {
            for (int y = 100; y < 175; ++y)
                for (int x = 80; x < 176; ++x) {
                    const int difference = reflected.channel(x, y, 0) - noBank.channel(x, y, 0);
                    changed += std::abs(difference) > 2;
                    redGain += std::max(difference, 0);
                    switchPixels += std::abs(reflected.channel(x, y, 0) - noReflection.channel(x, y, 0)) > 2;
                }
        }
        std::printf("Shared water %s reflection changed%d redgain%d switch%d\n", prefix.c_str(), changed, redGain, switchPixels);
        WATER_CHECK(changed > 5 && redGain > 30 && switchPixels > 5);
        auto changedFloor = capture("alternate-floor", {alternateFloorDraw, bankDraw}, false);
        int transmittedPixels = 0;
        if (!changedFloor.pixels.empty() && changedFloor.width == noReflection.width)
            for (int y = 100; y < 175; ++y)
                for (int x = 80; x < 176; ++x)
                    transmittedPixels += std::abs(changedFloor.channel(x, y, 1) - noReflection.channel(x, y, 1)) > 10;
        WATER_CHECK(transmittedPixels > 100);
        camera.eye = {0, -6, -1};
        camera.target = {0, 5, -1};
        for (int frame = 0; frame < 20; ++frame)
            renderer.frame(camera, lighting, {}, 4.1 + frame * .1, {floorDraw}, {waterDraw});
        showOverlay = true;
        auto submerged = capture("submerged", {floorDraw}, false, 6.1);
        if (!submerged.pixels.empty()) {
            int red = 0, green = 0, blue = 0;
            for (int y = 25; y < 65; ++y)
                for (int x = 80; x < 176; ++x) {
                    red += submerged.channel(x, y, 0);
                    green += submerged.channel(x, y, 1);
                    blue += submerged.channel(x, y, 2);
                }
            std::printf("Shared water %s distant submerged RGB%d,%d,%d\n", prefix.c_str(), red, green, blue);
            WATER_CHECK(blue > 3 * green && blue > red);
            const int foregroundRed = submerged.channel(128, 150, 0);
            const int foregroundGreen = submerged.channel(128, 150, 1);
            const int foregroundBlue = submerged.channel(128, 150, 2);
            WATER_CHECK(foregroundGreen > foregroundRed + 10);
            WATER_CHECK(foregroundBlue > foregroundGreen * .65f);
            for (int channel = 0; channel < 3; ++channel)
                WATER_CHECK(submerged.channel(20, 20, channel) == 255);
        }
        renderer.settings.taa = renderer.settings.smaa = true;
        glfwSetWindowSize(window, 320, 240);
        for (int frame = 0; frame < 8; ++frame) {
            glfwPollEvents();
            renderer.frame(camera, lighting, {}, 6.2 + frame * .1, {floorDraw}, {waterDraw});
        }
        auto resized = capture("resized", {floorDraw}, true, 7.1);
        WATER_CHECK(resized.width == 320 && resized.height == 240);
        renderer.shutdown();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("Shared water GPU images: %s (%s)\n", failures ? "FAILED" : "PASS", root.string().c_str());
    return failures;
}

// Native Alpha keeps the water depth when deferred scenery is drawn. The same
// far geometry must be visible without water; a missing/cull-only pass cannot pass.
int alphaSceneryShaderTests() {
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto *window = glfwCreateWindow(256, 192, "Alpha scenery staging", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    Renderer renderer;
    RenderSettings settings;
    settings.style = RenderStyle::Alpha;
    settings.water.style = waterlook::Style::Native;
    settings.vsync = settings.shadows = settings.taa = settings.smaa = false;
    settings.resolution = settings.nearResolution = 64;
    settings.fogDistance = settings.sceneryDistance = 256;
    std::string error;
    if (!renderer.init(window, settings, error)) {
        std::printf("Alpha staging init: %s\n", error.c_str());
        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    Mesh scenery, surface;
    quad(scenery, {{{-8, -2, 0}, {8, -2, 0}, {-8, 2, 0}, {8, 2, 0}}}, {230, 10, 10}, 4, GLOW);
    quad(surface, {{{-16, 0, 0}, {16, 0, 0}, {-16, 60, 0}, {16, 60, 0}}}, {40, 100, 180}, 4, WATER_SURFACE);
    DrawItem model, water;
    model.mesh = renderer.upload(scenery);
    model.kind = DrawKind::Scenery;
    water.mesh = renderer.upload(surface);
    water.kind = DrawKind::Terrain;
    Camera camera;
    camera.eye = {0, -8, 8};
    camera.target = {0, 28, 0};
    Lighting lighting;
    lighting.hour = 12;
    lighting.timeMs = 43200000;
    const auto root = std::filesystem::current_path() / "build" / "alpha-scenery-images";
    std::filesystem::create_directories(root);
    for (int frame = 0; frame < 32; ++frame)
        renderer.frame(camera, lighting, {}, frame * .1, {}, {});
    auto capture = [&](const char *name, float y, bool drawModel, bool drawWater) {
        model.model = glm::translate(glm::mat4(1), glm::vec3(0, y, -2));
        model.alphaScenery = alphascenery::Placement{alphascenery::Kind::Model, {0, (int64_t)(y * 65536), -2 * 65536}};
        const auto path = root / (std::string(name) + ".png");
        for (int frame = 0; frame < 3; ++frame) {
            glfwPollEvents();
            if (frame == 2)
                renderer.capture(path.string());
            renderer.frame(camera, lighting, {}, 4, drawModel ? std::vector<DrawItem>{model} : std::vector<DrawItem>{},
                           drawWater ? std::vector<DrawItem>{water} : std::vector<DrawItem>{});
        }
        Picture picture;
        WATER_CHECK(picture.load(path));
        return picture;
    };
    auto empty = capture("water-only", 36, false, true);
    auto near = capture("near-before-water", 36, true, true);
    auto far = capture("far-after-water", 40, true, true);
    auto dry = capture("far-without-water", 40, true, false);
    auto sky = capture("sky-only", 40, false, false);
    int nearRed = 0, farRed = 0, dryRed = 0;
    if (!empty.pixels.empty() && !near.pixels.empty() && !far.pixels.empty() && !dry.pixels.empty() && !sky.pixels.empty())
        for (int y = 0; y < 192; ++y)
            for (int x = 0; x < 256; ++x) {
                nearRed += near.channel(x, y, 0) > empty.channel(x, y, 0) + 3;
                farRed += far.channel(x, y, 0) > empty.channel(x, y, 0) + 3;
                dryRed += dry.channel(x, y, 0) > sky.channel(x, y, 0) + 10;
            }
    std::printf("Alpha staging GPU: near red%d far red%d dry red%d\n", nearRed, farRed, dryRed);
    WATER_CHECK(nearRed > 30 && farRed == 0 && dryRed > 30);
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return failures;
}
