// Actual Vulkan map-pass checks with synthetic geometry; no owner assets are needed.
#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>

int mapRenderTests() {
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto *window = glfwCreateWindow(128, 128, "Map renderer check", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    Renderer renderer;
    RenderSettings settings;
    settings.style = RenderStyle::Steam;
    settings.water.style = waterlook::Style::Native;
    settings.vsync = settings.shadows = settings.taa = settings.smaa = false;
    settings.resolution = settings.nearResolution = 64;
    std::string error;
    if (!renderer.init(window, settings, error)) {
        std::cerr << "Map renderer: " << error << '\n';
        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    Mesh plane;
    for (auto p : {glm::vec3(-2, -2, 0), glm::vec3(2, -2, 0), glm::vec3(2, 2, 0), glm::vec3(-2, 2, 0)})
        plane.vertices.push_back({p.x, p.y, p.z, 96, 96, 96, 255, 5, 0, 0, 0});
    plane.min = {-2, -2, 0};
    plane.max = {2, 2, 0};
    plane.indices = {0, 3, 2, 0, 2, 1};
    DrawItem item;
    item.mesh = renderer.upload(plane);
    item.flags = WORLD_MAP;
    item.castShadow = false;
    item.model = glm::translate(glm::mat4(1), glm::vec3(0, 0, 2));
    item.map.lens = {-1, 1, 0, 0};
    Camera camera;
    camera.eye = {0, -2, 1};
    camera.target = {0, 0, 1};
    Lighting light;
    const auto root = std::filesystem::current_path() / "build" /
                      ("map-render-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    int failures = 0;
    auto capture = [&](const char *name, std::vector<DrawItem> items) {
        const auto path = root / (std::string(name) + ".png");
        renderer.capture(path.string());
        renderer.frame(camera, light, {0, 0, 0}, 0, items, {});
        int width = 0, height = 0, channels = 0;
        auto *pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
        std::array<int, 3> result{-1000, -1000, -1000};
        if (pixels && width && height)
            for (int channel = 0; channel < 3; ++channel)
                result[channel] = pixels[4 * ((height / 2) * width + width / 2) + channel];
        else
            ++failures;
        stbi_image_free(pixels);
        return result;
    };
    auto check = [&](bool pass, const char *name) {
        std::cout << "Map renderer " << name << (pass ? " PASS\n" : " FAIL\n");
        failures += !pass;
    };
    const auto background = capture("background", {});
    const auto opaque = capture("opaque", {item});
    check(opaque != background, "native-winding-visible");
    Mesh reversed = plane;
    reversed.indices = {0, 1, 2, 0, 2, 3};
    auto backFacing = item;
    backFacing.mesh = renderer.upload(reversed);
    check(capture("reversed-winding", {backFacing}) == background, "native-reversed-winding-culled");
    item.map.opacity = .8f;
    const auto faded = capture("faded", {item});
    bool blended = true;
    for (int channel = 0; channel < 3; ++channel)
        blended &= std::abs(faded[channel] - (.8f * opaque[channel] + .2f * background[channel])) <= 2;
    check(blended, "faded-depth-equality-and-color");
    item.map.opacity = .4f;
    auto behind = item;
    behind.model = glm::translate(glm::mat4(1), glm::vec3(0, 0, 3));
    behind.map.colour = {0, 0, 1};
    check(capture("front-first", {item, behind}) == capture("back-first", {behind, item}), "depth-prepass-before-faded-color");
    auto invisible = item;
    invisible.model = glm::translate(glm::mat4(1), glm::vec3(0, 0, 1));
    invisible.map.opacity = 0;
    check(capture("zero-front", {invisible, behind}) == capture("behind-only", {behind}), "zero-opacity-submits-no-depth");
    Mesh foregroundPlane = plane;
    for (auto &vertex : foregroundPlane.vertices) {
        vertex.z = vertex.y + 1;
        vertex.y = -1.8f;
        vertex.r = 128;
        vertex.g = vertex.b = 0;
        vertex.normal = 3;
    }
    foregroundPlane.min = {-2, -1.8f, -1};
    foregroundPlane.max = {2, -1.8f, 3};
    // Both windings make this depth fixture independent of ordinary world face selection.
    foregroundPlane.indices.insert(foregroundPlane.indices.end(), {0, 1, 2, 0, 2, 3});
    DrawItem foregroundWorld;
    foregroundWorld.mesh = renderer.upload(foregroundPlane);
    foregroundWorld.castShadow = false;
    item.map.opacity = 1;
    const auto foregroundColor = capture("foreground-world", {foregroundWorld});
    check(foregroundColor != background && capture("foreground-with-map", {foregroundWorld, item}) == foregroundColor,
          "inherits-world-depth-without-private-clear");
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return failures;
}
