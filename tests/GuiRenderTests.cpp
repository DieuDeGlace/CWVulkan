// Actual Vulkan integration of the immediate UI/WORLD boundary. Synthetic data only.
#include "Renderer.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>

int guiRenderTests() {
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto *window = glfwCreateWindow(128, 128, "GUI WORLD check", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    Renderer renderer;
    RenderSettings settings;
    settings.style = RenderStyle::Steam;
    settings.vsync = settings.shadows = settings.taa = settings.smaa = false;
    settings.resolution = settings.nearResolution = 64;
    std::string error;
    if (!renderer.init(window, settings, error)) {
        std::cerr << "GUI WORLD: " << error << '\n';
        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    const std::array<uint8_t, 4> white{255, 255, 255, 255};
    if (!renderer.setOverlayAtlas(1, 1, white.data())) {
        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    auto plane = [&](uint8_t grey) {
        Mesh mesh;
        // The CUB -Z face's own winding; no duplicate reversed triangles.
        for (auto p : {glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 1, 0), glm::vec3(0, 1, 0)}) {
            mesh.vertices.push_back({p.x, p.y, p.z, grey, grey, grey, 255, 5, STEAM_VERTEX, 0, 0});
            mesh.min = glm::min(mesh.min, p);
            mesh.max = glm::max(mesh.max, p);
        }
        mesh.indices = {0, 3, 2, 0, 2, 1};
        return renderer.upload(mesh);
    };
    const auto ordinary = plane(64), brighter = plane(96);
    Mesh occluder;
    for (auto p : {glm::vec3(-1, -1, .2f), glm::vec3(1, -1, .2f), glm::vec3(1, 1, .2f), glm::vec3(-1, 1, .2f)})
        occluder.vertices.push_back({p.x, p.y, p.z, 255, 0, 0, 255, 4, 0, 0, 0});
    occluder.min = {-1, -1, .2f};
    occluder.max = {1, 1, .2f};
    occluder.indices = {0, 1, 2, 0, 2, 3};
    DrawItem preceding;
    preceding.mesh = renderer.upload(occluder);
    preceding.flags = NATIVE_MENU;
    std::vector<OverlayVertex> ui;
    auto quad = [&](float left, float top, float right, float bottom, std::array<uint8_t, 3> c) {
        for (auto p : {glm::vec2(left, top), glm::vec2(right, top), glm::vec2(right, bottom), glm::vec2(left, top),
                       glm::vec2(right, bottom), glm::vec2(left, bottom)})
            ui.push_back({p.x, p.y, 0, 0, c[0], c[1], c[2], 255});
    };
    quad(0, 0, 128, 128, {0, 0, 255});
    const auto background = ui;
    quad(60, 60, 68, 68, {0, 255, 0});
    const auto foreground = ui;
    guiitem::Request icon;
    icon.beforeVertex = 6;
    icon.mesh = ordinary;
    icon.allocation = {1, 1, 1};
    icon.at.screen = {64, 64};
    icon.at.size = .1f;
    Camera camera;
    camera.eye = {0, -2, 1};
    camera.target = {0, 0, 1};
    Lighting lighting;
    const auto root = std::filesystem::temp_directory_path() /
                      ("CubeVulkan-gui-world-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(root);
    int failures = 0, checks = 0;
    auto check = [&](const char *name, const std::vector<OverlayVertex> &text, const std::vector<guiitem::Request> &items,
                     const std::vector<DrawItem> &scene, glm::ivec2 at, std::array<int, 3> expected) {
        const auto capture = root / (std::string(name) + ".png");
        renderer.overlay(text, items);
        renderer.capture(capture.string());
        renderer.frame(camera, lighting, {0, 0, 0}, 0, scene, {});
        int width = 0, height = 0, channels = 0;
        auto *pixels = stbi_load(capture.string().c_str(), &width, &height, &channels, 4);
        bool pass = pixels && width == 128 && height == 128;
        std::array<int, 3> actual{};
        if (pass)
            for (int i = 0; i < 3; i++) {
                actual[i] = pixels[4 * (at.y * width + at.x) + i];
                pass &= std::abs(actual[i] - expected[i]) <= 2;
            }
        stbi_image_free(pixels);
        std::cout << "GUI WORLD " << name << ": " << actual[0] << ',' << actual[1] << ',' << actual[2] << (pass ? " PASS\n" : " FAIL\n");
        failures += !pass;
        checks++;
    };
    // -Z, V255 saturates every native illumination channel to1. The doubled
    // 64/255 grey remains grey through saturation; final cubic grading gives128.
    check("model-over-background", background, {icon}, {}, {64, 64}, {128, 128, 128});
    check("outside-model", background, {icon}, {}, {20, 64}, {0, 0, 255});
    check("manual-text-after-model", foreground, {icon}, {}, {64, 64}, {0, 255, 0});
    check("model-beside-manual-text", foreground, {icon}, {}, {72, 64}, {128, 128, 128});
    check("retained-scene-depth", background, {icon}, {preceding}, {64, 64}, {0, 0, 255});
    auto near = icon, behind = icon;
    near.at.screenDepth = -.2f;
    behind.mesh = brighter;
    check("retained-sibling-depth", background, {near, behind}, {}, {64, 64}, {128, 128, 128});
    auto dark = icon;
    dark.at.darkPass = true;
    check("main-over-dark", background, {dark}, {}, {64, 64}, {128, 128, 128});
    check("offset-dark-edge", background, {dark}, {}, {78, 78}, {0, 0, 0});
    auto spirit = icon;
    spirit.item[nativesaves::item::TYPE] = 3;
    spirit.item[nativesaves::item::ATTRIBUTE_COUNT] = 1;
    spirit.item[0x1d] = 1;
    spirit.cubeMesh = brighter;
    // Material0 uses the documented fallback grey0.5: cubic(2*(96/255)*0.5) gives81.
    check("native-spirit-cube", background, {spirit}, {}, {37, 64}, {81, 81, 81});
    // A later frame without a request must restore the ordinary overlay fast path.
    check("model-request-cleared", background, {}, {}, {64, 64}, {0, 0, 255});
    settings.taa = settings.smaa = settings.shadows = true;
    renderer.settings = settings;
    check("taa-smaa-shadows-first", background, {icon}, {}, {64, 64}, {128, 128, 128});
    check("taa-smaa-shadows-next", background, {icon}, {}, {64, 64}, {128, 128, 128});
    settings.style = RenderStyle::Alpha;
    renderer.settings = settings;
    check("alpha-world-native-ui", background, {icon}, {}, {64, 64}, {128, 128, 128});
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "GUI WORLD: " << checks << " image checks, " << failures << " failures\n";
    if (!failures && std::filesystem::equivalent(root.parent_path(), std::filesystem::temp_directory_path()) &&
        root.filename().string().rfind("CubeVulkan-gui-world-", 0) == 0)
        std::filesystem::remove_all(root);
    else
        std::cerr << "GUI WORLD captures retained: " << root << '\n';
    return failures ? 1 : 0;
}
