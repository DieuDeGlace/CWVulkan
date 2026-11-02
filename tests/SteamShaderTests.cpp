#include "SteamCreatureLights.h"
#include "NativePose.h"
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
#include <algorithm>
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
    settings.water.style = waterlook::Style::Native;
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
    {
        const DrawItem saved = item;
        const uint32_t originalMesh = item.mesh;
        Mesh back = mesh;
        std::swap(back.indices[1], back.indices[2]);
        const uint32_t backMesh = renderer.upload(back);
        renderer.settings.fogDistance = 1000000;
        item.kind = DrawKind::Particle;
        item.nativeParticle = scenevisibility::Sphere{{0, 0, 65536}, 1};
        item.flags = GLOW;
        item.flash = 0;
        item.castShadow = false;
        for (int test = 0; test < 4; ++test) {
            item.mesh = test == 1 ? backMesh : originalMesh;
            item.material = test == 2 ? glm::vec4(0) : test == 3 ? glm::vec4(1) : glm::vec4(1, 1, 1, 0);
            item.flags = test == 3 ? 0 : GLOW;
            const auto capture = root / ("particle-" + std::to_string(test) + ".png");
            renderer.capture(capture.string());
            renderer.frame(camera, lighting, {0, 0, 0}, 0, {item}, {});
            const auto pixel = centre(capture);
            const std::array<int, 3> expected = test == 2   ? std::array<int, 3>{0, 0, 0}
                                                : test == 3 ? std::array<int, 3>{7, 52, 5}
                                                            : std::array<int, 3>{82, 214, 60};
            bool matches = pixel.has_value();
            if (pixel)
                for (int c = 0; c < 3; ++c)
                    matches &= std::abs((*pixel)[c] - expected[c]) <= 2;
            std::cout << "Steam shader particle " << test << (matches ? " PASS\n" : " FAIL\n");
            failures += !matches;
        }
        item.mesh = originalMesh;
        item.flags = GLOW;
        item.material = glm::vec4(1);
        item.nativeShininess = true;
        const auto shinyCapture = root / "particle-note-shininess.png";
        renderer.capture(shinyCapture.string());
        renderer.frame(camera, lighting, {0, 0, 0}, 0, {item}, {});
        const auto shiny = centre(shinyCapture);
        // Independent shader contract: X=1/128, screenY=-1/128, gradient=-1/3,
        // viewNormal=(0,1). Apply its ripple, saturation and final grade to Glow RGB.
        const std::array<int, 3> shinyExpected{91, 226, 67};
        bool shinyMatches = shiny.has_value();
        if (shiny)
            for (int channel = 0; channel < 3; ++channel)
                shinyMatches &= std::abs((*shiny)[channel] - shinyExpected[channel]) <= 2;
        std::cout << "Steam shader particle note shininess" << (shinyMatches ? " PASS\n" : " FAIL\n");
        failures += !shinyMatches;
        DrawItem plain = item;
        plain.nativeShininess = false;
        const auto resetCapture = root / "particle-shininess-reset.png";
        renderer.capture(resetCapture.string());
        renderer.frame(camera, lighting, {0, 0, 0}, 0, {item, plain}, {});
        const auto reset = centre(resetCapture);
        bool resetMatches = reset.has_value();
        if (reset)
            for (int channel = 0; channel < 3; ++channel)
                resetMatches &= std::abs((*reset)[channel] - std::array<int, 3>{82, 214, 60}[channel]) <= 2;
        std::cout << "Steam shader particle shininess reset" << (resetMatches ? " PASS\n" : " FAIL\n");
        failures += !resetMatches;
        item = saved;
        renderer.settings.fogDistance = 2;
    }
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
    checkPoints("registered", {item}, {82, 214, 60});
    {
        DrawItem posePart = item;
        posePart.nativeLightZone = steampoints::ZoneKey{-100, -100};
        checkPoints("pose-binding-absent", {posePart}, {7, 52, 5});
        posePart.nativeLightZone = sources->key;
        checkPoints("pose-binding-present", {posePart}, {82, 214, 60});
    }

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
    // Native adaptive ranges must reach the actual light/actor/terrain rendering inputs.
    renderer.resetSceneRange();
    renderer.settings.nativeZoneRadius = 3;
    bool rangeMeshesReady = true;
    probe.sceneZone = [&](int64_t, int64_t) -> std::optional<scenevisibility::Bounds> {
        scenevisibility::Bounds zone;
        zone.minimum = {-4*65536,-4*65536,-4*65536};
        zone.maximum = {4*65536,4*65536,4*65536};
        zone.radius = 8;
        zone.meshed = rangeMeshesReady;
        zone.triangleCost = 0;
        return zone;
    };
    renderer.setWorldProbe(probe);
    renderer.frame(camera,lighting,{0,0,0},1,{},{}); // one capped second of native range growth
    const auto liveRanges = renderer.nativeRanges();
    if (!liveRanges || liveRanges->models != 38.4f || liveRanges->abilities != liveRanges->scene ||
        liveRanges->abilities <= 42 || liveRanges->abilities >= 96) {
        ++failures;
        std::cerr << "Adaptive scene range did not reach the live renderer\n";
    }
    emote.nativeActor->centre = {0,40*65536,65536};
    checkPoints("range-actor-distinct-from-model", {emote}, {82,214,60});
    const auto rangeEmptyPath = root / "range-empty.png";
    renderer.capture(rangeEmptyPath.string());
    renderer.frame(camera, lighting, {0, 0, 0}, 0, {}, {});
    const auto rangeEmpty = centre(rangeEmptyPath);
    if (rangeEmpty) {
        DrawItem particle = emote;
        particle.nativeActor.reset();
        particle.kind = DrawKind::Particle;
        particle.castShadow = false;
        particle.nativeParticle = scenevisibility::Sphere{{0, 30 * 65536, 65536}, 2};
        checkPoints("particle-native-range-admitted", {particle}, {82, 214, 60});
        particle.nativeParticle->centre[1] = 40 * 65536;
        checkPoints("particle-native-range-cutoff", {particle}, *rangeEmpty);
    }
    if (!rangeEmpty)
        ++failures;
    else {
        emote.nativeActor->centre = {0, 70 * 65536, 65536};
        checkPoints("range-actor-budget-cutoff", {emote}, *rangeEmpty);
        rangeMeshesReady = false;
        emote.nativeActor->centre = {0,40*65536,65536};
        checkPoints("range-readiness-lost", {emote}, *rangeEmpty);
        if (!renderer.nativeRanges() || renderer.nativeRanges()->scene != .001f) {
            ++failures;
            std::cerr << "Missing mesh readiness did not snap the live range down\n";
        }
        rangeMeshesReady = true;
        renderer.frame(camera,lighting,{0,0,0},1,{},{});
        checkPoints("range-readiness-restored", {emote}, {82,214,60});
    }
    auto oldZone = std::make_shared<scenevisibility::Bounds>();
    oldZone->minimum = {0,0,-4*65536}; oldZone->maximum = {65*65536,65*65536,4*65536};
    oldZone->center = {32*65536,32*65536,0}; oldZone->meshed = true; oldZone->triangleCost = 0;
    auto pendingZone = *oldZone;
    probe.sceneZone = [&](int64_t, int64_t) { return std::optional<scenevisibility::Bounds>(pendingZone); };
    renderer.setWorldProbe(probe);
    DrawItem pendingDraw = emote;
    pendingDraw.nativeActor.reset(); pendingDraw.nativeZone = oldZone;
    checkPoints("pending-cache-original-visible", {pendingDraw}, {82,214,60});
    if (rangeEmpty) {
        pendingZone.minimum[2] += 1000*65536; pendingZone.maximum[2] += 1000*65536; pendingZone.center[2] += 1000*65536;
        checkPoints("pending-cache-new-bounds-old-geometry", {pendingDraw}, *rangeEmpty);
        pendingZone = *oldZone;
        checkPoints("pending-cache-committed-bounds", {pendingDraw}, {82, 214, 60});
    }
    int sceneQueries = 0;
    probe.sceneZone = [&](int64_t, int64_t) {
        ++sceneQueries;
        return std::optional<scenevisibility::Bounds>(*oldZone);
    };
    renderer.setWorldProbe(probe);
    const scenevisibility::Sphere nearOldCamera{{0, 0, 65536}, 1};
    const bool retained = renderer.steamSphereVisible(nearOldCamera.centre, nearOldCamera.radius, 100);
    Camera movedCamera = camera;
    movedCamera.eye = {500, 0, 1};
    movedCamera.target = {500, 2, 1};
    movedCamera.nativeFixedEye = steampoints::Point{500 * 65536, 0, 65536};
    renderer.prepareScene(movedCamera, lighting, 3);
    const bool current = retained && !renderer.steamSphereVisible(nearOldCamera.centre, nearOldCamera.radius, 100) &&
                         !renderer.steamActorVisible(nearOldCamera);
    std::cout << "Steam scene current pose state" << (current ? " PASS\n" : " FAIL\n");
    failures += !current;
    const int preparedQueries = sceneQueries;
    renderer.frame(movedCamera, lighting, {500, 0, 0}, 3, {}, {});
    const bool once = preparedQueries > 0 && sceneQueries == preparedQueries;
    std::cout << "Steam scene prepared range reuse" << (once ? " PASS\n" : " FAIL\n");
    failures += !once;
    {
        WorldProbe largeProbe;
        largeProbe.origin = {1073741824.0, -536870912.0, -50.0};
        renderer.settings.fogDistance = 400;
        renderer.settings.sceneryDistance = 100;
        renderer.setWorldProbe(largeProbe);
        camera.viewShake = {0, 0};
        camera.nativeFixedEye.emplace();
        camera.nativeFixedLookAt.emplace();
        for (int axis = 0; axis < 3; ++axis) {
            const int64_t origin = int64_t(largeProbe.origin[axis] * 65536);
            (*camera.nativeFixedEye)[axis] = origin + int64_t(double(camera.eye[axis]) * 65536);
            (*camera.nativeFixedLookAt)[axis] = origin + int64_t(double(camera.target[axis]) * 65536);
        }
        const auto retainedOffset = renderer.speechProjection()->offset;
        renderer.prepareScene(camera, lighting, 0);
        const bool retainedSpeech = renderer.speechProjection()->offset == retainedOffset;
        std::cout << "Steam scene retained speech schedule" << (retainedSpeech ? " PASS\n" : " FAIL\n");
        failures += !retainedSpeech;
        const auto emptyCapture = root / "large-origin-empty.png";
        renderer.capture(emptyCapture.string());
        renderer.frame(camera, lighting, {}, 0, {}, {});
        const auto empty = centre(emptyCapture);
        failures += !empty;
        const auto &speech = *renderer.speechProjection();
        glm::vec3 target;
        for (int axis = 0; axis < 3; ++axis)
            target[axis] = float((*camera.nativeFixedLookAt)[axis] + speech.offset[axis]) * (1.f / 65536.f);
        const auto clip = speech.perspective * speech.view * glm::vec4(target, 1);
        const bool centered = speech.offset[0] == -(*camera.nativeFixedLookAt)[0] && speech.offset[1] == -(*camera.nativeFixedLookAt)[1] &&
                              clip.w > 0 && std::abs(clip.x / clip.w) < .00001f && std::abs(clip.y / clip.w) < .00001f;
        std::cout << "Steam scene large-origin speech center" << (centered ? " PASS\n" : " FAIL\n");
        failures += !centered;
        auto cached = std::make_shared<scenevisibility::Bounds>();
        cached->minimum = {0, 0, -54 * 65536};
        cached->maximum = {65 * 65536, 65 * 65536, -46 * 65536};
        cached->center = {32 * 65536, 32 * 65536, -50 * 65536};
        cached->meshed = true;
        cached->triangleCost = 0;
        DrawItem shifted = item;
        shifted.kind = DrawKind::Scenery;
        shifted.nativeLights.reset();
        shifted.nativeZone = cached;
        shifted.nativeScenery = steamscenery::Bounds{{0, 0, -50 * 65536}, 1, false};
        checkPoints("large-origin-cached-zone", {shifted}, {7, 52, 5});
        auto latest = *cached;
        for (int axis = 0; axis < 2; ++axis) {
            const int64_t origin = int64_t(largeProbe.origin[axis] * 65536);
            latest.minimum[axis] += origin;
            latest.maximum[axis] += origin;
            latest.center[axis] += origin;
        }
        latest.center[2] += 1000 * 65536;
        largeProbe.sceneZone = [&](int64_t x, int64_t y) -> std::optional<scenevisibility::Bounds> {
            if (x != (latest.minimum[0] >> 22) || y != (latest.minimum[1] >> 22))
                return std::nullopt;
            return latest;
        };
        renderer.setWorldProbe(largeProbe);
        if (empty)
            checkPoints("large-origin-latest-center", {shifted}, *empty);
    }
    {
        DrawItem object = item;
        object.model = glm::mat4(1);
        object.kind = DrawKind::Scenery;
        object.flags = 0;
        object.flash = 0;
        object.nativeShininess = false;
        object.material = glm::vec4(1);
        object.nativeActor.reset();
        object.nativeScenery.reset();
        object.nativeLights.reset();
        object.nativeLightZone.reset();
        object.nativeModel.reset();
        object.nativePosition.reset();
        object.nativeObject = scenevisibility::Sphere{{0, 0, 65536}, 4};
        object.nativeZone = std::make_shared<scenevisibility::Bounds>(); // unmeshed zone must not reject this route
        renderer.setWorldProbe({});
        renderer.settings.fogDistance = renderer.settings.sceneryDistance = 1000000;
        Camera nearby;
        nearby.eye = {0, -2, 1};
        nearby.target = {0, 0, 1};
        Lighting steady;
        steady.timeMs = 0;
        const auto capture = root / "static-object-unmeshed-zone.png";
        renderer.capture(capture.string());
        renderer.frame(nearby, steady, {}, 0, {object}, {});
        const auto pixel = centre(capture);
        const bool admitted = pixel && std::abs((*pixel)[0] - 7) <= 2 && std::abs((*pixel)[1] - 52) <= 2 && std::abs((*pixel)[2] - 5) <= 2;
        std::cout << "Steam shader static-object independent zone admission" << (admitted ? " PASS\n" : " FAIL\n");
        failures += !admitted;
    }
    {
        // Exercise the ordinary pose adapter through the actual queued GPU consumer.
        renderer.setWorldProbe({});
        renderer.settings.fogDistance = renderer.settings.sceneryDistance = 1000000;
        Camera nearby;
        nearby.eye = {0, -2, 1};
        nearby.target = {0, 0, 1};
        Lighting steady;
        steady.timeMs = 0;
        DrawItem posed;
        posed.mesh = item.mesh;
        posed.kind = DrawKind::Scenery;
        posed.standingLight = .5f;
        nativepose::Draw descriptor{};
        std::fill(std::begin(descriptor.tint), std::end(descriptor.tint), 1.f);
        descriptor.opacity = 1;
        descriptor.glow = 1;
        const auto captureDraws = [&](const std::string &name, const std::vector<DrawItem> &draws) {
            const auto capture = root / (name + ".png");
            renderer.capture(capture.string());
            renderer.frame(nearby, steady, {}, 0, draws, {});
            return centre(capture);
        };
        const auto expect = [&](const char *name, const auto &pixel, std::array<int, 3> expected) {
            bool ok = pixel.has_value();
            if (pixel)
                for (int c = 0; c < 3; ++c)
                    ok &= std::abs((*pixel)[c] - expected[c]) <= 2;
            std::cout << "Steam shader " << name << (ok ? " PASS\n" : " FAIL\n");
            failures += !ok;
        };
        worldmodel::applyPose(posed, descriptor);
        expect("queued-pose-glow", captureDraws("queued-pose-glow", {posed}), {82, 214, 60});
        descriptor.tint[3] = 0;
        worldmodel::applyPose(posed, descriptor);
        expect("queued-pose-material-alpha", captureDraws("queued-pose-material-alpha", {posed}), {82, 214, 60});
        descriptor.tint[0] = descriptor.tint[1] = descriptor.tint[2] = 0;
        worldmodel::applyPose(posed, descriptor);
        expect("queued-pose-material-rgb", captureDraws("queued-pose-material-rgb", {posed}), {0, 0, 0});
        descriptor.tint[0] = descriptor.tint[1] = descriptor.tint[2] = 1;
        descriptor.shininess = .25f;
        worldmodel::applyPose(posed, descriptor);
        DrawItem shinyReference = posed;
        shinyReference.nativeControls.reset();
        shinyReference.flags = GLOW;
        shinyReference.nativeShininess = true;
        const auto shiny = captureDraws("queued-pose-shininess-reference", {shinyReference});
        if (shiny)
            expect("queued-pose-shininess", captureDraws("queued-pose-shininess", {posed}), *shiny);
        else
            ++failures;
        descriptor.shininess = 0;
        descriptor.glow = -1;
        worldmodel::applyPose(posed, descriptor);
        const auto negative = captureDraws("queued-pose-negative-glow", {posed});
        Mesh equalLuminance = mesh;
        for (auto &v : equalLuminance.vertices) {
            v.r = 55;
            v.g = 92;
            v.b = 66;
        }
        posed.mesh = renderer.upload(equalLuminance);
        descriptor.glow = -2;
        worldmodel::applyPose(posed, descriptor);
        if (negative)
            expect("queued-pose-negative-glow", captureDraws("queued-pose-equal-luminance", {posed}), *negative);
        else
            ++failures;
        posed.mesh = item.mesh;
        descriptor.glow = 1;
        descriptor.white = 1;
        worldmodel::applyPose(posed, descriptor);
        expect("queued-pose-white", captureDraws("queued-pose-white", {posed}), {255, 255, 255});
        descriptor.white = 0;
        descriptor.tint[3] = 1;
        descriptor.opacity = .5f;
        worldmodel::applyPose(posed, descriptor);
        const auto sky = captureDraws("queued-pose-sky", {});
        if (sky) {
            std::array<int, 3> half;
            for (int c = 0; c < 3; ++c)
                half[c] = (std::array<int, 3>{82, 214, 60}[c] + (*sky)[c] + 1) / 2;
            expect("queued-pose-faded", captureDraws("queued-pose-faded", {posed}), half);
            Mesh opposite = mesh;
            std::swap(opposite.indices[1], opposite.indices[2]);
            posed.mesh = renderer.upload(opposite);
            descriptor.kind = nativepose::MIRRORED_DRAW;
            worldmodel::applyPose(posed, descriptor);
            expect("queued-pose-explicit-opposite", captureDraws("queued-pose-explicit-opposite", {posed}), half);
        } else {
            ++failures;
        }
        posed.mesh = item.mesh;
        descriptor.kind = 0;
        descriptor.opacity = 1;
        worldmodel::applyPose(posed, descriptor);
        DrawItem blocker = posed;
        blocker.model = glm::translate(glm::mat4(1), glm::vec3(0, -.1f, 0));
        blocker.nativeControls->white = 1;
        expect("queued-pose-depth-baseline", captureDraws("queued-pose-depth-baseline", {blocker, posed}), {255, 255, 255});
        descriptor.depthOffset = .1f;
        worldmodel::applyPose(posed, descriptor);
        expect("queued-pose-depth-offset", captureDraws("queued-pose-depth-offset", {blocker, posed}), {82, 214, 60});
        WorldProbe submergedProbe;
        submergedProbe.water = [](glm::vec3) { return true; };
        renderer.setWorldProbe(submergedProbe);
        DrawItem frameReference = posed;
        frameReference.nativeControls.reset();
        frameReference.flags = GLOW;
        frameReference.material = glm::vec4(1);
        const auto framePixel = captureDraws("queued-frame-material-reference", {frameReference});
        const auto poseFrame = renderer.steamPoseFrame();
        std::memcpy(descriptor.tint, &poseFrame.material[0], sizeof descriptor.tint);
        descriptor.depthOffset = 0;
        worldmodel::applyPose(posed, descriptor);
        if (framePixel)
            expect("queued-frame-material-once", captureDraws("queued-frame-material-once", {posed}), *framePixel);
        else
            ++failures;
        renderer.setWorldProbe({});
    }
    {
        // Raw ground admission precedes hopping; tagged faded submissions interleave
        // ground before static objects even when the adapter appended them afterward.
        renderer.setWorldProbe({});
        renderer.settings.fogDistance = renderer.settings.sceneryDistance = 1000000;
        Camera nearby;
        nearby.eye = {0, -2, 1};
        nearby.target = {0, 0, 1};
        Lighting steady;
        steady.timeMs = 0;
        const auto captureGround = [&](const char *name, const std::vector<DrawItem> &draws) {
            const auto path = root / (std::string(name) + ".png");
            renderer.capture(path.string());
            renderer.frame(nearby, steady, {}, 0, draws, {});
            return centre(path);
        };
        const auto sky = captureGround("ground-order-sky", {});
        DrawItem ground;
        ground.mesh = item.mesh;
        ground.kind = DrawKind::Scenery;
        ground.material = {0, 0, 0, 1};
        ground.nativeControls = worldmodel::Controls{1, 0, 0, 0, false};
        ground.nativeGround = scenevisibility::Sphere{{0, -4 * 65536, 65536}, 0};
        ground.nativeOrder = worldmodel::Order{0, 0, 0, true};
        const auto rejected = captureGround("ground-raw-sphere", {ground});
        const bool rawGate = sky && rejected && *sky == *rejected;
        std::cout << "Steam shader ground-raw-sphere" << (rawGate ? " PASS\n" : " FAIL\n");
        failures += !rawGate;
        ground.nativeGround->centre = {0, 0, 65536};
        ground.opacity = .5f;
        DrawItem object = ground;
        object.nativeGround.reset();
        object.nativeControls->white = 1;
        object.nativeOrder->ground = false;
        const auto blended = captureGround("ground-static-order", {object, ground});
        bool ordered = sky && blended;
        if (ordered)
            for (int channel = 0; channel < 3; ++channel)
                ordered &= std::abs((*blended)[channel] - int(std::lround(127.5f + .25f * (*sky)[channel]))) <= 2;
        std::cout << "Steam shader ground-static-faded-order" << (ordered ? " PASS\n" : " FAIL\n");
        failures += !ordered;
        const auto checkCube = [&](const char *name, const auto &actual, const auto &expected) {
            bool equal = actual && expected && sky && *expected != *sky;
            if (equal)
                for (int channel = 0; channel < 3; ++channel)
                    equal &= std::abs((*actual)[channel] - (*expected)[channel]) <= 1;
            std::cout << "Steam shader " << name << (equal ? " PASS\n" : " FAIL\n");
            failures += !equal;
        };
        DrawItem cube;
        cube.mesh = item.mesh;
        cube.kind = DrawKind::OwnedGroundCube;
        cube.castShadow = false;
        cube.nativeModel = glm::mat4(1);
        cube.model = glm::translate(glm::mat4(1), {0, -10, 0}); // a generic home-space sphere would be rejected
        cube.material = {.5f, .25f, 1, .8f};
        DrawItem reference = cube;
        reference.kind = DrawKind::Scenery;
        reference.model = glm::mat4(1);
        reference.nativeControls = worldmodel::Controls{1, 0, 0, 0, false};
        const auto explicitCube = captureGround("owned-cube-explicit-controls", {reference});
        checkCube("owned-cube-no-extra-sphere", captureGround("owned-cube-no-extra-sphere", {cube}), explicitCube);
        DrawItem carrier = reference;
        carrier.nativeModel = glm::translate(glm::mat4(1), {2, 0, 0}) * glm::scale(glm::mat4(1), glm::vec3(.1f));
        carrier.model = *carrier.nativeModel;
        carrier.nativeGround = scenevisibility::Sphere{{0, 0, 65536}, 10};
        carrier.opacity = 2;
        carrier.nativeControls = worldmodel::Controls{0, 1, 1, .002f, false};
        reference.opacity = 2;
        reference.nativeControls = worldmodel::Controls{1, 1, 0, .002f, false};
        const auto retainedReference = captureGround("owned-cube-retained-reference", {reference});
        checkCube("owned-cube-retained-controls", captureGround("owned-cube-retained-controls", {carrier, cube}), retainedReference);
        DrawItem particle = cube;
        particle.kind = DrawKind::Particle;
        particle.model = glm::mat4(1);
        particle.flags = GLOW;
        particle.material = {1, 0, 0, 1};
        cube.material = {0, 1, 0, 1};
        const auto particleOnly = captureGround("owned-cube-particle-reference", {particle});
        checkCube("owned-cube-before-burst", captureGround("owned-cube-before-burst", {particle, cube}), particleOnly);
    }
    {
        // Native arithmetic reference: a quarter block survives a fixed origin at block2^24.
        // Compare visible coverage with the same small scene near zero, not with rounded home floats.
        renderer.setWorldProbe({});
        renderer.settings.fogDistance = renderer.settings.sceneryDistance = 1000000;
        Mesh marker;
        for (glm::vec3 p : {glm::vec3(-.04f, 0, .7f), glm::vec3(.04f, 0, .7f), glm::vec3(0, 0, 1.3f)}) {
            marker.vertices.push_back({p.x, p.y, p.z, 51, 102, 25, 128, 4, STEAM_VERTEX, 0, 0});
            marker.min = glm::min(marker.min, p);
            marker.max = glm::max(marker.max, p);
        }
        marker.indices = {0, 1, 2};
        DrawItem actor;
        actor.mesh = renderer.upload(marker);
        actor.flags = GLOW;
        actor.flash = 1;
        actor.castShadow = false;
        Lighting steady;
        steady.timeMs = 0;
        const auto renderMarker = [&](const char *name, int64_t blockOrigin, float fraction, bool anchored = false) {
            Camera fixedCamera;
            fixedCamera.eye = {float(blockOrigin), -2, 1};
            fixedCamera.target = {float(blockOrigin), 0, 1};
            fixedCamera.nativeFixedEye = steampoints::Point{blockOrigin * 65536, -2 * 65536, 65536};
            fixedCamera.nativeFixedLookAt = steampoints::Point{blockOrigin * 65536, 0, 65536};
            actor.model = glm::translate(glm::mat4(1), glm::vec3(float(blockOrigin) + fraction, 0, 0));
            actor.nativeModel = glm::translate(glm::mat4(1), glm::vec3(fraction, 0, 0));
            actor.nativePosition.reset();
            if (anchored) {
                actor.nativeModel = glm::mat4(1);
                actor.nativePosition = steampoints::Point{blockOrigin * 65536 + int64_t(fraction * 65536), 0, 0};
            }
            actor.nativeActor = scenevisibility::Sphere{{blockOrigin * 65536 + int64_t(fraction * 65536), 0, 65536}, 1};
            const auto path = root / (std::string(name) + ".png");
            renderer.capture(path.string());
            renderer.frame(fixedCamera, steady, fixedCamera.target, 0, {actor}, {});
            return path;
        };
        const auto mask = [](const std::filesystem::path &path) {
            int width = 0, height = 0, channels = 0;
            auto *pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
            std::vector<uint8_t> result;
            if (pixels && width == 128 && height == 128)
                for (int y = 40; y < 88; ++y)
                    for (int x = 40; x < 88; ++x) {
                        const auto *p = pixels + 4 * (y * width + x);
                        result.push_back(p[0] > 240 && p[1] > 240 && p[2] > 240);
                    }
            stbi_image_free(pixels);
            return result;
        };
        const auto integral = mask(renderMarker("origin-integral", 0, 0));
        const auto reference = mask(renderMarker("origin-quarter-reference", 0, .25f));
        const auto distant = mask(renderMarker("origin-quarter-distant", int64_t(1) << 24, .25f));
        const bool visibleFraction =
            !reference.empty() && std::count(reference.begin(), reference.end(), uint8_t(1)) > 0 && reference != integral;
        const bool sameCoverage = visibleFraction && reference == distant;
        std::cout << "Steam shader quarter-block fixture" << (visibleFraction ? " PASS\n" : " FAIL\n");
        std::cout << "Steam shader fixed-origin quarter-block coverage" << (sameCoverage ? " PASS\n" : " FAIL\n");
        failures += !visibleFraction + !sameCoverage;
        const auto anchored = mask(renderMarker("origin-quarter-anchor", int64_t(1) << 24, .25f, true));
        const bool anchoredCoverage = visibleFraction && anchored == reference;
        std::cout << "Steam shader fixed-anchor quarter-block coverage" << (anchoredCoverage ? " PASS\n" : " FAIL\n");
        failures += !anchoredCoverage;
        const int64_t farBlock = int64_t(1) << 24;
        auto lamp = sourceModel(0, 0, 10);
        lamp.record.x = farBlock * 65536 + 16384;
        auto nearby = std::make_shared<steampoints::ZoneSources>();
        nearby->key = {int32_t(farBlock / 64), 0};
        nearby->models.push_back(lamp);
        actor.nativeLights = nearby;
        actor.nativeLightZone = nearby->key;
        actor.flags = 0;
        actor.flash = 0;
        const auto litMarker = renderMarker("origin-quarter-point-light", farBlock, .25f);
        int width = 0, height = 0, channels = 0;
        auto *pixels = stbi_load(litMarker.string().c_str(), &width, &height, &channels, 4);
        bool sameLight = visibleFraction && pixels && width == 128 && height == 128;
        for (int y = 40; sameLight && y < 88; ++y)
            for (int x = 40; sameLight && x < 88; ++x)
                if (reference[size_t(y - 40) * 48 + x - 40]) {
                    const auto *p = pixels + 4 * (y * width + x);
                    sameLight = std::abs(int(p[0]) - 82) <= 2 && std::abs(int(p[1]) - 214) <= 2 && std::abs(int(p[2]) - 60) <= 2;
                }
        stbi_image_free(pixels);
        std::cout << "Steam shader fixed-origin point-light binding" << (sameLight ? " PASS\n" : " FAIL\n");
        failures += !sameLight;
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
