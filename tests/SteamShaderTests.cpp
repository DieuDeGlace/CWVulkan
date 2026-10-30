// Explicit GPU integration check. No game assets or Steam process are needed.
// Reference colors are independently derived in steam-world-shader-output.md.
#include "Renderer.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>

int steamShaderTests() {
    if (!glfwInit()) {
        std::cerr << "Steam shader: GLFW initialization failed\n";
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(128, 128, "Steam shader check", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Steam shader: hidden window creation failed\n";
        return 1;
    }
    Renderer renderer;
    RenderSettings settings;
    settings.style = RenderStyle::Steam;
    settings.vsync = settings.shadows = settings.taa = settings.smaa = false;
    settings.resolution = settings.nearResolution = 64;
    settings.fogDistance = 1000000;
    std::string error;
    if (!renderer.init(window, settings, error)) {
        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cerr << "Steam shader: " << error << '\n';
        return 1;
    }
    Mesh mesh;
    for (glm::vec3 p : {glm::vec3(-2, 0, -1), glm::vec3(2, 0, -1), glm::vec3(0, 0, 3)}) {
        mesh.vertices.push_back({p.x, p.y, p.z, 51, 102, 25, 128, 4, STEAM_VERTEX, 0, 0});
        mesh.min = glm::min(mesh.min, p);
        mesh.max = glm::max(mesh.max, p);
    }
    mesh.indices = {0, 1, 2};
    DrawItem item;
    item.mesh = renderer.upload(mesh);
    item.kind = DrawKind::Scenery;
    item.standingLight = .5f;
    Camera camera;
    camera.eye = {0, -2, 1};
    camera.target = {0, 0, 1};
    Lighting lighting;
    lighting.timeMs = 0;
    struct Case {
        const char *name;
        uint32_t flags;
        float white;
        std::array<int, 3> expected;
    };
    const Case cases[]{
        {"lit", 0, 0, {7, 52, 5}},
        {"glow", GLOW, 0, {82, 214, 60}},
        {"white", 0, 1, {255, 255, 255}},
    };
    const auto root = std::filesystem::temp_directory_path() /
        ("CubeVulkan-steam-shader-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(root)) {
        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cerr << "Steam shader: temporary fixture already exists\n";
        return 1;
    }
    int failures = 0;
    for (const auto &test : cases) {
        item.flags = test.flags;
        item.flash = test.white;
        const auto capture = root / (std::string(test.name) + ".png");
        renderer.capture(capture.string());
        renderer.frame(camera, lighting, {0, 0, 0}, 0, {item}, {});
        int width = 0, height = 0, channels = 0;
        auto *pixels = stbi_load(capture.string().c_str(), &width, &height, &channels, 4);
        bool matches = pixels && width > 0 && height > 0;
        std::array<int, 3> actual{};
        if (matches) {
            const auto *pixel = pixels + 4 * ((height / 2) * width + width / 2);
            for (int c = 0; c < 3; ++c) {
                actual[c] = pixel[c];
                matches &= std::abs(actual[c] - test.expected[c]) <= 2;
            }
        }
        stbi_image_free(pixels);
        std::cout << "Steam shader " << test.name << ": " << actual[0] << ',' << actual[1] << ',' << actual[2]
                  << (matches ? " PASS\n" : " FAIL\n");
        failures += !matches;
    }
    // A varying-W triangle at/behind F must become the same graded sky as an
    // empty scene, including White1 and Glow1. This also checks that homogeneous
    // clip coordinates are divided after interpolation, rather than before it.
    renderer.settings.fogDistance = 2;
    const auto skyCapture = root / "sky.png";
    renderer.capture(skyCapture.string());
    renderer.frame(camera, lighting, {0, 0, 0}, 0, {}, {});
    auto centre = [](const std::filesystem::path &path) -> std::optional<std::array<int, 3>> {
        int width = 0, height = 0, channels = 0;
        auto *pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
        std::optional<std::array<int, 3>> value;
        if (pixels && width > 0 && height > 0) {
            const auto *pixel = pixels + 4 * ((height / 2) * width + width / 2);
            value = std::array<int, 3>{pixel[0], pixel[1], pixel[2]};
        }
        stbi_image_free(pixels);
        return value;
    };
    const auto sky = centre(skyCapture);
    mesh.vertices[0].y = 3;
    mesh.max.y = 3;
    item.mesh = renderer.upload(mesh);
    for (const auto &test : cases) {
        item.flags = test.flags;
        item.flash = test.white;
        const auto capture = root / (std::string(test.name) + "-fog.png");
        renderer.capture(capture.string());
        renderer.frame(camera, lighting, {0, 0, 0}, 0, {item}, {});
        const auto actual = centre(capture);
        bool matches = sky && actual;
        if (matches)
            for (int c = 0; c < 3; ++c)
                matches &= std::abs((*sky)[c] - (*actual)[c]) <= 2;
        std::cout << "Steam shader full fog " << test.name << (matches ? " PASS\n" : " FAIL\n");
        failures += !matches;
    }
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    const bool ownFixture = std::filesystem::equivalent(root.parent_path(), std::filesystem::temp_directory_path()) &&
        root.filename().string().rfind("CubeVulkan-steam-shader-", 0) == 0;
    if (!failures && ownFixture)
        std::filesystem::remove_all(root); // only this test's newly created directory
    else
        std::cerr << "Shader captures retained: " << root << '\n';
    return failures ? 1 : 0;
}
