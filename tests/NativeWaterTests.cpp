// Fixed-origin equivalence for native water's shared WORLD view-space inputs.
// Uses only synthetic geometry and existing independent coordinate contracts.
#include "Renderer.h"
#include "ShaderCompute.h"
#include "water_reference_comp.h"
#include <cmath>
#include <iomanip>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>

int terrainPublicationRenderTests(Renderer &, const std::filesystem::path &);

namespace {
int waterArithmeticReferences() {
    using V = std::array<float, 4>;
    using Case = std::array<V, 11>;
    const V sky1{.2f,.35f,.55f,1}, sky2{.7f,.8f,.95f,1}, fog{.45f,.5f,.6f,1}, sun{1,.8f,.5f,1};
    const V sunPosition{.15f,.25f,1.5f,1};
    const std::array<Case, 3> cases{{
        Case{sky1,sky2,fog,sun,sunPosition,V{.008f,.1f,.8f,0},V{.45f,0,0,0},V{.4f,-.2f,.8f,2},V{0,0,1,0},V{.6f,0,.8f,0},V{-20,10,-70,0}},
        Case{sky1,sky2,fog,sun,sunPosition,V{.008f,.1f,.65f,0},V{.3f,0,0,0},V{-1.2f,1.6f,2,4},V{0,.6f,.8f,0},V{.6f,0,.8f,0},V{40,-40,-60,0}},
        Case{sky1,sky2,fog,sun,sunPosition,V{.012f,.1f,.9f,0},V{.8f,0,0,0},V{.3f,-1.2f,1.5f,3},V{0,.6f,.8f,0},V{-.6f,0,-.8f,0},V{25,15,-50,0}}
    }};
    static_assert(sizeof(Case) == 176);
    // Independent source-side arithmetic table, before blend/UNORM conversion.
    const V expected[]{{.28357509f,.37083031f,.46690795f,.708f},
                       {.25270894f,.33789275f,.43488681f,.598f},
                       {.35895824f,.47449442f,.59784957f,.750375f}};
    try {
        const auto actual = shadercheck::compute(spv_water_reference_comp, sizeof(spv_water_reference_comp),
                                                  cases.data(), sizeof(cases), uint32_t(cases.size()));
        int failures = 0;
        for (size_t i = 0; i < cases.size(); ++i) {
            bool match = true;
            for (int c = 0; c < 4; ++c)
                match &= std::isfinite(actual[i][c]) && std::abs(actual[i][c] - expected[i][c]) <= .00001f;
            std::cout << "Native water arithmetic " << char('A' + i) << ' ' << std::setprecision(8)
                      << actual[i][0] << ',' << actual[i][1] << ',' << actual[i][2] << ',' << actual[i][3]
                      << (match ? " PASS\n" : " FAIL\n");
            failures += !match;
        }
        return failures;
    } catch (const std::exception &error) {
        std::cerr << "Native water arithmetic: " << error.what() << '\n';
        return 1;
    }
}
} // namespace

int nativeWaterTests() {
    const int arithmeticFailures = waterArithmeticReferences();
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(128, 128, "Native water check", nullptr, nullptr);
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
    settings.fogDistance = 4;
    settings.sceneryDistance = 128;
    std::string error;
    if (!renderer.init(window, settings, error)) {
        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cerr << "Native water: " << error << '\n';
        return 1;
    }
    const auto root = std::filesystem::temp_directory_path() /
        ("CubeVulkan-native-water-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(root)) {
        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    Mesh mesh;
    for (glm::vec3 p : {glm::vec3(-3, 0, -2), glm::vec3(3, 0, -2), glm::vec3(0, 0, 4)}) {
        mesh.vertices.push_back({p.x, p.y, p.z, 51, 102, 25, 128, 4, WATER_SURFACE | STEAM_VERTEX, 0, 0});
        mesh.min = glm::min(mesh.min, p);
        mesh.max = glm::max(mesh.max, p);
    }
    mesh.indices = {0, 1, 2};
    DrawItem surface;
    surface.mesh = renderer.upload(mesh);
    surface.kind = DrawKind::Terrain;
    surface.material = {1, 1, 1, .5f};
    surface.nativeModel = glm::mat4(1);
    surface.castShadow = false;
    Lighting lighting;
    lighting.timeMs = 0;
    auto capture = [&](const char *label, int64_t origin, bool draw, bool waterSurface = true) {
        Camera camera;
        camera.eye = {float(double(origin) + .25), -2, 1};
        camera.target = {float(double(origin) + .25), 0, 1};
        camera.nativeFixedEye = steampoints::Point{origin * 65536 + 16384, -2 * 65536, 65536};
        camera.nativeFixedLookAt = steampoints::Point{origin * 65536 + 16384, 0, 65536};
        surface.nativePosition = steampoints::Point{origin * 65536, 0, 0};
        surface.model = glm::translate(glm::mat4(1), glm::vec3(float(origin), 0, 0));
        const auto path = root / (std::string(label) + ".png");
        renderer.capture(path.string());
        const std::vector<DrawItem> draws = draw ? std::vector<DrawItem>{surface} : std::vector<DrawItem>{};
        renderer.frame(camera, lighting, {}, 0, waterSurface ? std::vector<DrawItem>{} : draws,
                       waterSurface ? draws : std::vector<DrawItem>{});
        int width = 0, height = 0, channels = 0;
        auto *data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
        std::vector<uint8_t> pixels;
        if (data && width == 128 && height == 128)
            pixels.assign(data, data + size_t(width) * height * 4);
        stbi_image_free(data);
        return pixels;
    };
    const auto empty = capture("empty", 0, false);
    const auto near = capture("near", 0, true);
    const auto opaqueNear = capture("opaque-near", 0, true, false);
    const bool visible = !near.empty() && near.size() == empty.size() && near != empty;
    int failures = arithmeticFailures + !visible;
    std::cout << "Native water visible synthetic surface" << (visible ? " PASS\n" : " FAIL\n");
    for (int64_t origin : {int64_t(1) << 24, -(int64_t(1) << 24)}) {
        const auto opaqueFar = capture(origin > 0 ? "opaque-positive" : "opaque-negative", origin, true, false);
        const bool geometry = !opaqueNear.empty() && opaqueNear == opaqueFar;
        std::cout << "Native water shared geometry control " << origin << (geometry ? " PASS\n" : " FAIL\n");
        failures += !geometry;
        const auto far = capture(origin > 0 ? "positive-origin" : "negative-origin", origin, true);
        const bool matches = visible && near == far;
        std::cout << "Native water fixed-origin " << origin << (matches ? " PASS\n" : " FAIL\n");
        failures += !matches;
    }
    auto sources = std::make_shared<steampoints::ZoneSources>();
    sources->key = {0, 0};
    auto lamp = [](float x, float intensity) {
        steampoints::ModelSource source;
        source.record.type = 20;
        source.record.flags = 1;
        source.record.x = int64_t(x * 65536);
        source.record.z = 65536;
        source.record.r = source.record.g = source.record.b = intensity;
        return source;
    };
    for (int i = 0; i < 10; ++i)
        sources->models.push_back(lamp(i / 16.f, 0));
    sources->models.push_back(lamp(1, 10));
    surface.nativeLights = sources;
    const auto limited = capture("point-slot-limit", 0, true);
    const bool tenSlots = visible && limited == near;
    std::cout << "Native water first ten point slots" << (tenSlots ? " PASS\n" : " FAIL\n");
    failures += !tenSlots;
    sources->models.erase(sources->models.begin(), sources->models.begin() + 10);
    const auto lit = capture("registered-point", 0, true);
    const size_t centre = 4 * (64 * 128 + 64);
    const bool point = lit.size() == near.size() && !lit.empty() && lit[centre + 2] > near[centre + 2] + 8;
    std::cout << "Native water registered vertex point light" << (point ? " PASS\n" : " FAIL\n");
    failures += !point;
    failures += terrainPublicationRenderTests(renderer, root);
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    const bool ownFixture = std::filesystem::equivalent(root.parent_path(), std::filesystem::temp_directory_path()) &&
        root.filename().string().rfind("CubeVulkan-native-water-", 0) == 0;
    if (!failures && ownFixture)
        std::filesystem::remove_all(root);
    else
        std::cerr << "Native water captures retained: " << root << '\n';
    return failures ? 1 : 0;
}
