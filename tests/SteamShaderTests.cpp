#include "SteamCreatureLights.h"
#include "worldgen/Residents.h"
#include "SteamTransientLights.h"
// Explicit GPU integration check. No game assets or Steam process are needed.
// Reference colors are independently derived in steam-world-shader-output.md.
#include <set>
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
    settings.fogDistance = settings.sceneryDistance = 1000000;
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
    renderer.settings.fogDistance = 1000000;
    mesh.vertices[0].y = 0;
    mesh.max.y = 0;
    item.mesh = renderer.upload(mesh);
    item.flags = 0;
    item.flash = 0;
    auto sourceModel = [](float x, float y, float intensity) {
        steampoints::ModelSource source;
        source.record.type = 20;
        source.record.flags = 1;
        source.record.x = (int64_t)(x * 65536);
        source.record.y = (int64_t)(y * 65536);
        source.record.z = 65536;
        source.record.r = source.record.g = source.record.b = intensity;
        return source;
    };
    auto checkPoints = [&](const char *name, const std::vector<DrawItem> &draws, std::array<int,3> expected) {
        const auto capture = root / (std::string(name) + ".png");
        renderer.capture(capture.string());
        renderer.frame(camera, lighting, {0,0,0}, 0, draws, {});
        const auto actual = centre(capture);
        bool matches = actual.has_value();
        if (matches)
            for (int c = 0; c < 3; ++c)
                matches &= std::abs((*actual)[c] - expected[c]) <= 2;
        std::cout << "Steam shader points " << name << (matches ? " PASS\n" : " FAIL\n");
        failures += !matches;
    };
    auto sources = std::make_shared<steampoints::ZoneSources>();
    sources->key = {0,0};
    for (int i = 0; i < 10; ++i)
        sources->models.push_back(sourceModel(i / 16.f, 0, 0));
    sources->models.push_back(sourceModel(1, 0, 10));
    item.nativeLights = sources;
    checkPoints("first-ten", {item}, {7,52,5}); // eleventh light is retained CPU-side, not uploaded
    sources->models.erase(sources->models.begin(), sources->models.begin() + 10);
    // Intensity10 saturates illumination to1 at every vertex; the resulting
    // non-glow palette equals the independent Glow1 RGB reference above.
    checkPoints("registered", {item}, {82,214,60});

    auto distant = std::make_shared<steampoints::ZoneSources>();
    distant->key = {-1,1};
    std::vector<steampoints::ZoneKey> loaded{{0,0}};
    for (int y = 1; y <= 7; ++y) {
        loaded.push_back({-1,y});
        for (int i = 0; i < 10; ++i)
            distant->models.push_back(sourceModel(-8, y * 64.f + 32 + i / 16.f, 0));
    }
    WorldProbe probe;
    probe.loadedLightZones = [loaded] { return loaded; };
    renderer.setWorldProbe(probe);
    DrawItem sourceOnly;
    sourceOnly.nativeLights = distant;
    checkPoints("storage-offset70", {sourceOnly, item}, {82,214,60});

    // Seventy nearer lights in another zone must not consume a global64 budget.
    auto near = std::make_shared<steampoints::ZoneSources>();
    near->key = {0,0};
    for (int i = 0; i < 70; ++i)
        near->models.push_back(sourceModel(0, i / 64.f, 0));
    sourceOnly.nativeLights = near;
    sources->key = {0,1};
    sources->models = {sourceModel(0, 100, 10)};
    item.model[3].y = 100;
    probe.loadedLightZones = [] { return std::vector<steampoints::ZoneKey>{{0,0}, {0,1}}; };
    renderer.setWorldProbe(probe);
    checkPoints("no-global64-cap", {sourceOnly, item}, {82,214,60});
    // Change personal state without rebuilding geometry or rebinding the probe.
    // Ten nearby warm sources saturate all illumination components, matching
    // the independent fully lit RGB reference above.
    sources->key = {0,0};
    sources->models.clear();
    worldgen::StaticObject personal;
    personal.type = 0x7e;
    personal.scale = 0; // emitted position remains (0,0,1)
    sources->objects.assign(10, personal);
    item.model = glm::mat4(1);
    probe.loadedLightZones = [] { return std::vector<steampoints::ZoneKey>{{0,0}}; };
    std::set<std::array<int32_t, 2>> usedCells;
    probe.usedCell = [&](int32_t x, int32_t y) { return usedCells.count({x,y}) != 0; };
    renderer.setWorldProbe(probe);
    checkPoints("personal-before-use", {item}, {7,52,5});
    usedCells.insert({0,0});
    checkPoints("personal-after-use", {item}, {82,214,60});
    usedCells.clear();
    checkPoints("personal-other-character", {item}, {7,52,5});
    sources->objects.clear();
    std::vector<steampoints::EffectSource> liveEffects{{3, {0,0,65536}, 40}};
    probe.effectLights = [&] { return liveEffects; };
    renderer.setWorldProbe(probe);
    checkPoints("projectile-kind3", {item}, {82,214,60});
    liveEffects[0].kind = 4;
    checkPoints("projectile-kind4", {item}, {82,214,60});
    liveEffects[0].kind = 7;
    checkPoints("projectile-unlit-kind", {item}, {7,52,5});
    liveEffects.clear();
    checkPoints("projectile-removed", {item}, {7,52,5});
    steampoints::TransientLights flashes;
    for (int i = 0; i < 10; ++i)
        flashes.append({{0,0,65536}, {1.5f,.75f,.15f}, 20, 0, 400});
    probe.transientLights = [&] { return flashes.values(); };
    renderer.setWorldProbe(probe);
    checkPoints("transient-born", {item}, {82,214,60});
    const steampoints::Point previousPoint{0,0,65536};
    if (!renderer.steamSphereVisible(previousPoint, 1, 100)) {
        ++failures;
        std::cerr << "Previous scene sphere unexpectedly rejected\n";
    }
    probe.origin = {6400,-12800,-32};
    renderer.setWorldProbe(probe);
    if (!renderer.steamSphereVisible(previousPoint, 1, 100) ||
        renderer.steamSphereVisible({6400LL*65536,-12800LL*65536,-31LL*65536}, 1, 100)) {
        ++failures;
        std::cerr << "Pending world origin changed previous scene visibility\n";
    }
    probe.origin = {};
    renderer.setWorldProbe(probe);
    flashes.advance(400);
    checkPoints("transient-zero-fade", {item}, {7,52,5});
    flashes.advance(1);
    checkPoints("transient-expired", {item}, {7,52,5});
    std::vector<worldgen::CreatureRecord> lightActors(10, worldgen::newCreature(1));
    auto actorField = [&](size_t field, const auto &value) {
        for (auto &record : lightActors) std::memcpy(record.data() + field, &value, sizeof value);
    };
    actorField(0x180, 1.f); actorField(0x128, uint16_t(1)); actorField(0x198, 30.f);
    actorField(0x80, uint32_t(0)); actorField(0x8c, 4.f);
    probe.creatureLights = [&](const steampoints::Frame &frame, std::vector<steampoints::Light> &out) {
        for (const auto &record : lightActors)
            steampoints::collectCreature(record.data(), [](int) { return false; }, frame, out);
    };
    renderer.setWorldProbe(probe);
    checkPoints("creature-live-without-geometry", {item}, {82,214,60});
    actorField(0x180, 0.f);
    checkPoints("creature-dead", {item}, {7,52,5});
    actorField(0x180, 1.f);
    checkPoints("creature-restored", {item}, {82,214,60});
    lightActors.clear();
    auto bounds = std::make_shared<scenevisibility::Bounds>();
    bounds->minimum = {-4*65536,-4*65536,-4*65536};
    bounds->maximum = {4*65536,4*65536,4*65536};
    bounds->radius = 8;
    bounds->meshed = true;
    item.nativeZone = bounds;
    steampoints::GroundSource book;
    book.category = 0x13;
    book.quantity = 1;
    book.owner = 42;
    book.position[2] = -2 * 65536; // emitted at Z1, independently of hop animation
    steampoints::GroundSources ground;
    ground.player = 7;
    ground.items.assign(10, book);
    probe.groundLights = [&](steampoints::ZoneKey key) {
        if (key != sources->key) ++failures;
        return ground;
    };
    renderer.setWorldProbe(probe);
    checkPoints("ground-other-owner", {item}, {7,52,5});
    ground.player = 42;
    checkPoints("ground-own-item", {item}, {82,214,60});
    for (auto &entry : ground.items) entry.timerMs = 1;
    checkPoints("ground-delayed", {item}, {7,52,5});
    ground.items.clear();
    checkPoints("ground-picked-up", {item}, {7,52,5});
    // World bounds rebase XY while retaining native Z; item records stay absolute.
    probe.origin = {6400, -12800, -32};
    sources->key = {100,-200};
    probe.loadedLightZones = [&] { return std::vector<steampoints::ZoneKey>{sources->key}; };
    bounds->center[2] = -32 * 65536;
    bounds->minimum[2] -= 32 * 65536;
    bounds->maximum[2] -= 32 * 65536;
    book.position = {6400LL * 65536, -12800LL * 65536, -34 * 65536};
    ground.items.assign(10, book);
    renderer.setWorldProbe(probe);
    checkPoints("ground-rebased-origin", {item}, {82,214,60});
    // Emotes share the ordinary opaque depth pass and their actor's sphere gate.
    probe.origin = {};
    ground.items.clear();
    probe.loadedLightZones = [] { return std::vector<steampoints::ZoneKey>{{0,0}}; };
    renderer.setWorldProbe(probe);
    DrawItem emote = item;
    emote.model = glm::mat4(1);
    emote.nativeZone.reset(); emote.nativeLights.reset();
    emote.kind = DrawKind::Emote;
    emote.flags = GLOW;
    emote.nativeActor = scenevisibility::Sphere{{0,0,65536},2};
    checkPoints("emote-native-glow", {emote}, {82,214,60});
    const auto emptyPath = root / "empty-emote-scene.png";
    renderer.capture(emptyPath.string());
    renderer.frame(camera,lighting,{0,0,0},0,{},{});
    const auto emptyColour = centre(emptyPath);
    if (!emptyColour) ++failures;
    else {
        emote.nativeActor->centre = {10000LL*65536,0,65536};
        checkPoints("emote-actor-range", {emote}, *emptyColour);
        emote.nativeActor->centre = {0,-100*65536,65536};
        checkPoints("emote-actor-frustum", {emote}, *emptyColour);
    }
    emote.nativeActor->centre = {0,0,65536};
    DrawItem blocker = emote;
    blocker.nativeActor.reset();
    blocker.flash = 1;
    blocker.kind = DrawKind::Model;
    blocker.model = glm::translate(glm::mat4(1), glm::normalize(camera.eye-camera.target)*.5f) *
                    glm::scale(glm::mat4(1),glm::vec3(3));
    const auto blockerPath = root / "emote-blocker.png";
    renderer.capture(blockerPath.string());
    renderer.frame(camera,lighting,{0,0,0},0,{blocker},{});
    const auto blockerColour = centre(blockerPath);
    if (!blockerColour) ++failures;
    else checkPoints("emote-world-depth", {blocker,emote}, *blockerColour);
    blocker.model = emote.model;
    checkPoints("emote-equal-depth", {blocker,emote}, {82,214,60});
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
