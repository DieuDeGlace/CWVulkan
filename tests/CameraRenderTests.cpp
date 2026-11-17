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
#include <optional>

namespace {
// A 3-by-3 neighborhood contains two diagonal patches. All five pixels in
// the centre's depth cross are clear. Only the previous frame has a surface there.
bool revealedSkyHistory(Renderer &renderer, GLFWwindow *window, const std::filesystem::path &root) {
    renderer.settings.fov = 50;
    renderer.settings.taa = true;
    renderer.settings.jitterScale = renderer.settings.resolveBlur = 0;
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    if (width < 4 || height < 4)
        return false;
    const int x = width / 2, y = height / 2;
    const float worldPerPixel = 8 * std::tan(glm::radians(25.f)) / height;
    const auto patch = [&](int dx, int dy, float halfWidth, uint8_t shade) {
        Mesh mesh;
        for (const auto corner : {glm::vec2(-1, -1), glm::vec2(1, -1), glm::vec2(1, 1), glm::vec2(-1, 1)}) {
            const glm::vec3 p((x + dx + .5f - width * .5f + corner.x * halfWidth) * worldPerPixel, 0,
                              (height * .5f - y - dy - .5f + corner.y * halfWidth) * worldPerPixel);
            mesh.vertices.push_back({p.x, p.y, p.z, shade, shade, shade, 255, 4, STEAM_VERTEX, 0, 0});
            mesh.min = glm::min(mesh.min, p);
            mesh.max = glm::max(mesh.max, p);
        }
        mesh.indices = {0, 1, 2, 0, 2, 3};
        DrawItem item;
        item.mesh = renderer.upload(mesh);
        item.kind = DrawKind::Scenery;
        item.standingLight = 1.0f;
        return item;
    };
    const auto foreground = patch(0, 0, 3.25f, 180);
    const std::vector<DrawItem> revealed{patch(-1, -1, .45f, 3), patch(1, 1, .45f, 255)};
    Camera camera;
    camera.eye = {0, -4, 0};
    camera.target = {0, 0, 0};
    Lighting light;
    light.timeMs = 43200000;
    using Pixels = std::array<std::array<uint8_t, 3>, 9>;
    const auto draw = [&](const char *name, const std::vector<DrawItem> &items) -> std::optional<Pixels> {
        const auto path = root / (std::string(name) + ".png");
        renderer.capture(path.string());
        renderer.frame(camera, light, {0, 0, 0}, 0, items, {});
        int w = 0, h = 0, channels = 0;
        auto *image = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
        std::optional<Pixels> result;
        if (image && w == width && h == height) {
            auto &sample = result.emplace();
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    std::copy_n(image + 4 * ((y + dy) * w + x + dx), 3, sample[(dy + 1) * 3 + dx + 1].begin());
        }
        stbi_image_free(image);
        return result;
    };

    renderer.cutHistory();
    const auto sky = draw("temporal-sky", {});
    renderer.cutHistory();
    const auto old = draw("temporal-foreground", {foreground});
    const auto accumulated = draw("temporal-history", revealed);
    renderer.cutHistory();
    const auto reset = draw("temporal-reset", revealed);
    if (!sky || !old || !accumulated || !reset)
        return false;
    for (int i : {1, 3, 4, 5, 7})
        if ((*sky)[i] != (*reset)[i])
            return false;
    return (*old)[4] != (*reset)[4] && (*reset)[0] != (*sky)[0] && (*reset)[8] != (*sky)[8] &&
           (*accumulated)[4] == (*reset)[4];
}
// The roof is small enough to use scenery culling. Its projected shadow
// falls ten blocks inward from the near cascade's receiver radius.
bool shadowAcrossCascade(Renderer &renderer, RenderStyle style, const std::filesystem::path &root) {
    const char *name = style == RenderStyle::Alpha ? "alpha" : "steam";
    renderer.settings.smaa = renderer.settings.taa = renderer.settings.shadows = false;
    const Lighting light = lightingAt(10.f, {});
    const auto &settings = renderer.settings;
    const glm::vec3 direction = style == RenderStyle::Alpha
        ? alpha::shadowLight(light.timeMs, settings.sunShadowStrength, settings.moon).direction : light.lightDir;
    const glm::vec2 towardsLight = glm::normalize(glm::vec2(direction));
    constexpr float shadowInset = 10.f;
    const glm::vec3 spot(towardsLight * (settings.nearRadius - shadowInset), 0.f);
    const glm::vec3 roof(towardsLight * settings.nearRadius,
                         shadowInset * direction.z / glm::length(glm::vec2(direction)));
    const auto plane = [&](glm::vec3 center, float radius, bool castsShadow) {
        Mesh mesh;
        for (const auto corner : {glm::vec2(-1, -1), glm::vec2(1, -1), glm::vec2(1, 1), glm::vec2(-1, 1)}) {
            const auto point = center + glm::vec3(corner * radius, 0.f);
            mesh.vertices.push_back({point.x, point.y, point.z, 87, 93, 102, 255, 4, STEAM_VERTEX, 0, 0});
        }
        mesh.min = center - glm::vec3(radius, radius, 0.f);
        mesh.max = center + glm::vec3(radius, radius, 0.f);
        mesh.indices = {0, 1, 2, 0, 2, 3};
        DrawItem draw;
        draw.mesh = renderer.upload(mesh);
        draw.kind = DrawKind::Terrain;
        draw.castShadow = castsShadow;
        return draw;
    };
    const std::vector<DrawItem> geometry{plane(spot, 4.f, false), plane(roof, .65f, true)};
    Camera camera;
    camera.eye = spot + glm::vec3(towardsLight.y * 2.f, -towardsLight.x * 2.f, 7.f);
    camera.target = spot;
    const auto sample = [&](const char *label, float focusAlong) -> std::optional<int> {
        const auto path = root / (std::string("cascade-") + name + "-" + label + ".png");
        renderer.capture(path.string());
        renderer.frame(camera, light, glm::vec3(towardsLight * focusAlong, 0.f), 0, geometry, {});
        int w = 0, h = 0, channels = 0;
        auto *pixels = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
        std::optional<int> result;
        if (pixels && w > 0 && h > 0) {
            const auto *pixel = pixels + 4 * ((h / 2) * w + w / 2);
            result = pixel[0] + pixel[1] + pixel[2];
        }
        stbi_image_free(pixels);
        return result;
    };
    const auto lit = sample("lit", .08f);
    renderer.settings.shadows = true;
    const auto fromInside = sample("inside", .08f);
    const auto fromOutside = sample("outside", -.08f);
    renderer.settings.cascades = false;
    const auto farMap = sample("far", -.08f);
    if (!lit || !fromInside || !fromOutside || !farMap)
        return false;
    const int darkness = *lit - *fromInside;
    return darkness > 24 && *lit - *farMap > 24 && (*lit - *fromOutside) * 5 > darkness * 4;
}
} // namespace

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
        settings.resolution = 1024;
        settings.nearResolution = 512;
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
        // Canonical radians must override a deliberately different compatibility FOV,
        // in both the rasterized geometry and the CPU visibility projection.
        renderer.settings.fov = 110;
        camera.nativeFovRadians = .8726646900177002f;
        const auto nativeWide = centroid("-native-fov50", false);
        const steampoints::Point side{2 * 65536, 0, 0};
        const bool wideVisible = renderer.steamSphereVisible(side, .02f, 160);
        camera.nativeFovRadians = .5235987901687622f;
        const auto nativeNarrow = centroid("-native-fov30", false);
        const double narrowCot = 1.0 / std::tan(30.0 * 3.141592653589793 / 360.0);
        const glm::dvec2 narrowExpected(-.5 * narrowCot * height / 8.0, -.25 * narrowCot * height / 8.0);
        const bool lensPass = nativeWide.x > 0 && nativeNarrow.x > 0 && std::abs(nativeWide.x - after.x) <= 1 &&
                              std::abs(nativeWide.y - after.y) <= 1 && wideVisible && !renderer.steamSphereVisible(side, .02f, 160) &&
                              std::abs((nativeNarrow.x - before.x) - narrowExpected.x) <= 1 &&
                              std::abs((nativeNarrow.y - before.y) - narrowExpected.y) <= 1;
        std::cout << "Camera native lens " << name << (lensPass ? " PASS" : " FAIL") << " raw radians/CPU visibility/GPU pinhole\n";
        failures += !lensPass;
        camera.nativeFovRadians.reset();
        renderer.settings.fov = 50;
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
        if (style == RenderStyle::Alpha) {
            const bool history = revealedSkyHistory(renderer, window, root);
            std::cout << "Temporal resolve revealed sky " << (history ? "PASS" : "FAIL") << '\n';
            failures += !history;
        }
        const bool shadow = shadowAcrossCascade(renderer, style, root);
        std::cout << "Shadow cascade " << name << (shadow ? " PASS" : " FAIL") << '\n';
        failures += !shadow;
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
