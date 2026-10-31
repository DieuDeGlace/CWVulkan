// Actual Vulkan projection check for gameplay-camera §7.2. No owner assets required.
#include "Renderer.h"
#include "CameraMotion.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>

int cameraRenderTests() {
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(128, 128, "Camera projection check", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    const auto root = std::filesystem::temp_directory_path() /
                      ("cubevulkan-camera-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    int failures = 0;
    for (const auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
        const char *name = style == RenderStyle::Steam ? "steam" : "alpha";
        Renderer renderer;
        RenderSettings settings;
        settings.style = style;
        settings.fov = 50;
        settings.water.style = waterlook::Style::Native;
        settings.vsync = settings.shadows = settings.taa = settings.smaa = false;
        settings.resolution = settings.nearResolution = 64;
        settings.fogDistance = settings.sceneryDistance = 1000000;
        std::string error;
        if (!renderer.init(window, settings, error)) {
            std::cerr << "Camera renderer " << name << " initialization: " << error << '\n';
            ++failures;
            renderer.shutdown();
            continue;
        }
        Mesh mesh;
        for (glm::vec3 p : {glm::vec3(-.2f, 0, -.2f), glm::vec3(.2f, 0, -.2f), glm::vec3(.2f, 0, .2f), glm::vec3(-.2f, 0, .2f)}) {
            mesh.vertices.push_back({p.x, p.y, p.z, 255, 0, 255, 255, 4, STEAM_VERTEX, 0, 0});
            mesh.min = glm::min(mesh.min, p);
            mesh.max = glm::max(mesh.max, p);
        }
        mesh.indices = {0, 1, 2, 0, 2, 3};
        DrawItem item;
        item.mesh = renderer.upload(mesh);
        item.kind = DrawKind::Scenery;
        item.standingLight = .5f;
        Camera camera;
        camera.eye = {0, -4, 0};
        camera.target = {0, 0, 0};
        Lighting lighting;
        lighting.timeMs = 43200000;
        std::array<int, 3> ink{};
        int width = 0, height = 0;
        auto centroid = [&](const char *suffix, bool baseline) {
            const auto path = root / (std::string(name) + suffix + ".png");
            renderer.capture(path.string());
            renderer.frame(camera, lighting, {0, 0, 0}, 0, {item}, {});
            int channels = 0;
            auto *pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
            glm::dvec2 sum(0);
            size_t count = 0;
            if (pixels && width > 0 && height > 0) {
                if (baseline) {
                    const auto *centre = pixels + 4 * ((height / 2) * width + width / 2);
                    ink = {centre[0], centre[1], centre[2]};
                }
                for (int y = 0; y < height; ++y)
                    for (int x = 0; x < width; ++x) {
                        const auto *pixel = pixels + 4 * (y * width + x);
                        if (pixel[0] == ink[0] && pixel[1] == ink[1] && pixel[2] == ink[2]) {
                            sum += glm::dvec2(x, y);
                            ++count;
                        }
                    }
            }
            stbi_image_free(pixels);
            return count > 4 && count < size_t(width * height / 3) ? sum / double(count) : glm::dvec2(-10000);
        };
        const steampoints::Point ahead{0, 0, 0}, behind{0, -8 * 65536, 0};
        bool culling = !renderer.steamSphereVisible(ahead, .5f, 160);
        const auto before = centroid("-baseline", true);
        culling = culling && renderer.steamSphereVisible(ahead, .5f, 160) && !renderer.steamSphereVisible(behind, .5f, 160) &&
                  !renderer.steamSphereVisible(ahead, .5f, 2);
        camera.nativeViewRotation = cameramotion::viewRotation(90, 0, 180);
        camera.target = camera.eye; // Native obstruction can collapse eye/anchor.
        const auto coincident = centroid("-native-coincident", false);
        bool orientation = coincident.x > 0 && std::abs(coincident.x - before.x) <= 1 && std::abs(coincident.y - before.y) <= 1;
        camera.nativeViewRotation.reset();
        camera.target = {0, 0, 0};
        camera.viewShake = {.5f, .25f}; // Native left/up view-space units.
        const auto after = centroid("-shake", false);
        // Pinhole projection, independently of the renderer's view-matrix implementation.
        const double cot = 1.0 / std::tan(50.0 * 3.141592653589793 / 360.0);
        const glm::dvec2 expected(-.5 * cot * height / 8.0, -.25 * cot * height / 8.0);
        const bool pass = before.x > 0 && after.x > 0 && std::abs((after.x - before.x) - expected.x) <= 1.0 &&
                          std::abs((after.y - before.y) - expected.y) <= 1.0;
        std::cout << "Camera renderer " << name << (pass ? " PASS" : " FAIL") << " delta " << after.x - before.x << ','
                  << after.y - before.y << " expected " << expected.x << ',' << expected.y << '\n';
        failures += !pass;
        // Mutating the next camera cannot change the completed renderer epoch.
        camera.target = {0, -8, 0};
        camera.viewShake = {};
        culling = culling && renderer.steamSphereVisible(ahead, .5f, 160) && !renderer.steamSphereVisible(behind, .5f, 160);
        renderer.frame(camera, lighting, {0, 0, 0}, 0, {}, {});
        culling = culling && !renderer.steamSphereVisible(ahead, .5f, 160) && renderer.steamSphereVisible(behind, .5f, 160);
        camera.eye = camera.target = {0, 0, 0};
        camera.nativeViewRotation = cameramotion::viewRotation(0, 0, 0);
        renderer.frame(camera, lighting, {0, 0, 0}, 0, {}, {});
        orientation = orientation && renderer.steamSphereVisible({0, 0, 4 * 65536}, .5f, 160) &&
                      !renderer.steamSphereVisible({0, 0, -4 * 65536}, .5f, 160);
        renderer.shutdown();
        culling = culling && !renderer.steamSphereVisible(behind, .5f, 160);
        std::cout << "Camera orientation " << name << (orientation ? " PASS" : " FAIL") << " coincident target/pole\n";
        failures += !orientation;
        std::cout << "Camera culling " << name << (culling ? " PASS" : " FAIL") << " retained renderer epoch/range/lifetime\n";
        failures += !culling;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "Camera captures: " << root << '\n';
    return failures;
}
