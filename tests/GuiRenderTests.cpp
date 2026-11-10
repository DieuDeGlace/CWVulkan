// Actual Vulkan integration: synthetic geometry and optional owner-atlas HUD emission.
#include "Renderer.h"
#include "HudCoin.h"
#include "HudCompass.h"
#include "Frontend.h"
#include "ChatLine.h"
#include "Assets.h"
#include "Hud.h"
#include "VoxelEdit.h"
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
                     const std::vector<DrawItem> &scene, glm::ivec2 at, std::array<int, 3> expected, int tolerance = 2,
                     std::optional<int> expectedAlpha = {}, const std::vector<plasma::LayerCommand> &layers = {}) {
        const auto capture = root / (std::string(name) + ".png");
        renderer.overlay(text, items, layers);
        renderer.capture(capture.string(), expectedAlpha.has_value());
        renderer.frame(camera, lighting, {0, 0, 0}, 0, scene, {});
        int width = 0, height = 0, channels = 0;
        auto *pixels = stbi_load(capture.string().c_str(), &width, &height, &channels, 4);
        bool pass = pixels && width == 128 && height == 128;
        std::array<int, 3> actual{};
        if (pass)
            for (int i = 0; i < 3; i++) {
                actual[i] = pixels[4 * (at.y * width + at.x) + i];
                pass &= std::abs(actual[i] - expected[i]) <= tolerance;
            }
        if (pass && expectedAlpha)
            pass &= pixels[4 * (at.y * width + at.x) + 3] == *expectedAlpha;
        stbi_image_free(pixels);
        std::cout << "GUI WORLD " << name << ": " << actual[0] << ',' << actual[1] << ',' << actual[2] << (pass ? " PASS\n" : " FAIL\n");
        failures += !pass;
        checks++;
    };
    auto retainedSurface = std::make_shared<plasma::RetainedSurface>();
    auto cachedText = background;
    for (auto &vertex : cachedText) {
        vertex.x = 20 + vertex.x * .3125f;
        vertex.y = 20 + vertex.y * .3125f;
        vertex.r = 255;
        vertex.g = vertex.b = 0;
        vertex.a = 128;
    }
    auto cacheInput = background;
    cacheInput.insert(cacheInput.end(), cachedText.begin(), cachedText.end());
    const std::vector<plasma::LayerCommand> cacheFill{
        {plasma::LayerCommand::Kind::CacheBegin, {6, 0}, 0, retainedSurface, {20, 20, 40, 40}},
        {plasma::LayerCommand::Kind::CacheEnd, {12, 0}, 0, retainedSurface},
        {plasma::LayerCommand::Kind::CacheCopy, {12, 0}, 0, retainedSurface, {20, 20, 40, 40}}};
    check("cached-surface-rgba-composition", cacheInput, {}, {}, {40, 40}, {64, 0, 127}, 2, 255, cacheFill);
    const std::vector<plasma::LayerCommand> cacheHit{{plasma::LayerCommand::Kind::CacheCopy, {6, 0}, 0, retainedSurface, {20, 20, 40, 40}}};
    check("cached-surface-next-frame-reuse", background, {}, {}, {40, 40}, {64, 0, 127}, 2, 255, cacheHit);
    auto movedCopy = cacheHit;
    movedCopy[0].rectangle.x = 60;
    check("cached-surface-live-destination", background, {}, {}, {80, 40}, {64, 0, 127}, 2, 255, movedCopy);
    auto awayFromCache = icon;
    awayFromCache.at.screen = {105, 105};
    check("cached-surface-keeps-world-depth-attachment", background, {awayFromCache}, {}, {40, 40}, {64, 0, 127}, 2, 255, cacheHit);
    auto fractional = background;
    for (auto &vertex : fractional) {
        vertex.r = .75f;
        vertex.g = vertex.b = 0;
    }
    check("float-vertex-colour", fractional, {}, {}, {64, 64}, {1, 0, 0}, 0);
    auto rounded = fractional;
    for (auto &vertex : rounded)
        vertex.r = 0;
    check("rounded-colour-control", rounded, {}, {}, {64, 64}, {0, 0, 0}, 0);
    auto translucent = background;
    auto red = background;
    for (auto &vertex : red) {
        vertex.r = 255;
        vertex.g = vertex.b = 0;
        vertex.a = 128;
    }
    translucent.insert(translucent.end(), red.begin(), red.end());
    check("source-over-alpha", translucent, {}, {}, {64, 64}, {128, 0, 127}, 2, 255);
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
    auto ring = icon;
    ring.kind = guiitem::Request::Kind::Ring;
    ring.at.darkPass = true; // This independent queued path ignores a direct widget's shadow policy.
    ring.cubeMesh = brighter;
    ring.item = spirit.item;
    check("ring-before-plasma", foreground, {ring}, {}, {64, 64}, {0, 255, 0});
    check("ring-main-no-dark-copy", background, {ring}, {}, {64, 64}, {128, 128, 128});
    check("ring-retained-scene-depth", background, {ring}, {preceding}, {64, 64}, {0, 0, 255});
    // Exercise the real CUB-to-GUI mesh producer, including its packed-color shader admission.
    // The +Z face survives the logo's half-turn and the queued WORLD cull state.
    CubModel logoCub;
    logoCub.sx = logoCub.sy = logoCub.sz = 1;
    logoCub.rgb = {128, 128, 128};
    auto logo = frontend::logoRequest({{64, 64}, 0, .5f}, {1, 1, 1}, renderer.upload(guiitem::voxelMesh(logoCub)), 960);
    logo.at.darkPass = true;
    logo.cubeMesh = brighter;
    logo.item = spirit.item; // queued logos must not inherit widget-only shadow or spirit passes
    for (auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
        settings.style = style;
        renderer.settings = settings;
        const std::string prefix = style == RenderStyle::Steam ? "steam-logo-" : "alpha-logo-";
        check((prefix + "world-colour").c_str(), {}, {logo}, {}, {64, 64}, {128, 128, 128});
        check((prefix + "before-plasma").c_str(), background, {logo}, {}, {64, 64}, {0, 0, 255});
        check((prefix + "retained-scene-depth").c_str(), {}, {logo}, {preceding}, {64, 64}, {255, 0, 0});
    }
    auto brightLogoCub = logoCub;
    brightLogoCub.rgb = {192, 192, 192};
    auto highResource = logo;
    highResource.mesh = renderer.upload(guiitem::voxelMesh(brightLogoCub));
    for (auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
        settings.style = style;
        renderer.settings = settings;
        const std::string prefix = style == RenderStyle::Steam ? "steam-queue-" : "alpha-queue-";
        check((prefix + "resource-order").c_str(), {}, {highResource, logo}, {}, {64, 64}, {216, 216, 216});
        auto black = logo;
        black.material = {0, 0, 0, 1};
        check((prefix + "same-resource-fifo").c_str(), {}, {logo, black}, {}, {64, 64}, {0, 0, 0});
    }
    // An asset-free solid CUB exercises the same producer as the owner's coin.
    CubModel coinCub;
    coinCub.sx = coinCub.sy = 8;
    coinCub.sz = 2;
    coinCub.rgb.assign(8 * 8 * 2 * 3, 128);
    const auto coinMesh = renderer.upload(guiitem::voxelMesh(coinCub));
    const auto coins = hudcoin::coinRequests(coinMesh, {8, 8, 2}, 960, 1, 128, 0);
    auto overBackground = coins;
    for (auto &request : overBackground)
        request.beforeVertex = background.size();
    for (auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
        settings.style = style;
        renderer.settings = settings;
        const std::string prefix = style == RenderStyle::Steam ? "steam-coin-" : "alpha-coin-";
        // WORLD output §3: screen(-.2890625,.1796875), normalXY(0,-.5), frame
        // gradient−1/3 gives ripple phase−3.9817708; grey128 grades to90.3.
        check((prefix + "world-colour").c_str(), background, overBackground, {}, {45, 52}, {90, 90, 90});
        check((prefix + "offset-shadow").c_str(), background, overBackground, {}, {64, 68}, {0, 0, 0});
        check((prefix + "shadow-only").c_str(), background, {overBackground[0]}, {}, {50, 57}, {0, 0, 0});
        check((prefix + "before-plasma").c_str(), background, coins, {}, {45, 52}, {0, 0, 255});
        check((prefix + "retained-scene-depth").c_str(), {}, coins, {preceding}, {45, 52}, {255, 0, 0});
    }
    CubModel compassCub;
    compassCub.sx = compassCub.sy = compassCub.sz = 1;
    compassCub.rgb = {128, 128, 128};
    const auto compass = hudcompass::request(renderer.upload(guiitem::voxelMesh(compassCub)), {1, 1, 1}, {128, 128}, 960, 1, 0);
    if (compass) {
        auto visible = *compass;
        visible.beforeVertex = background.size();
        for (auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
            settings.style = style;
            renderer.settings = settings;
            const std::string prefix = style == RenderStyle::Steam ? "steam-compass-" : "alpha-compass-";
            check((prefix + "world-colour").c_str(), background, {visible}, {}, {68, 20}, {128, 128, 128});
            check((prefix + "before-plasma").c_str(), background, {*compass}, {}, {68, 20}, {0, 0, 255});
            check((prefix + "retained-scene-depth").c_str(), {}, {*compass}, {preceding}, {68, 20}, {255, 0, 0});
        }
    } else
        ++failures;
    const auto row = guiitem::hudItemRequests(coinMesh, {8, 8, 2}, nativesaves::makeItem(24, 1), 64, {128, 128}, 1, 0, 960);
    auto rowOverBackground = row;
    for (auto &request : rowOverBackground)
        request.beforeVertex = background.size();
    for (auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
        settings.style = style;
        renderer.settings = settings;
        const std::string prefix = style == RenderStyle::Steam ? "steam-hud-row-" : "alpha-hud-row-";
        // Fixed overlay illumination of the +Z face tilted−120 degrees yields
        // approximately RGB128/127/102 after WORLD saturation and cubic grading.
        check((prefix + "world-colour").c_str(), background, rowOverBackground, {}, {64, 52}, {128, 127, 102});
        check((prefix + "offset-shadow").c_str(), background, rowOverBackground, {}, {75, 60}, {0, 0, 0});
        check((prefix + "before-plasma").c_str(), background, row, {}, {64, 52}, {0, 0, 255});
        check((prefix + "retained-scene-depth").c_str(), {}, row, {preceding}, {64, 52}, {255, 0, 0});
    }
    guiitem::Request raw;
    raw.kind = guiitem::Request::Kind::RawEdit;
    raw.beforeVertex = 6;
    raw.mesh = ordinary;
    const auto edit = voxeledit::frame({1, 1, 1}, {128, 128}, glm::mat4(1), {}, {128, 128.0f / 1.2f}, {});
    if (edit) {
        raw.model = edit->model;
        raw.projection = edit->projection;
        check("edit-raw-native-palette", background, {raw}, {}, {64, 64}, {128, 128, 128});
        raw.material = {1, 0, 0, 1};
        // Shader-output §§3/6: red K=128/255 gains saturation before the clamped
        // luminance mix into G/B and cubic grade, yielding157/3/3 rather than128/0/0.
        check("edit-raw-material", background, {raw}, {}, {64, 64}, {157, 3, 3});
        check("edit-raw-before-text", foreground, {raw}, {}, {64, 64}, {0, 255, 0});
        check("edit-raw-retained-depth", background, {raw}, {preceding}, {64, 64}, {0, 0, 255});
    } else
        ++failures;
    if (edit) {
        auto character = raw;
        character.kind = guiitem::Request::Kind::Character;
        character.material = {1, 1, 1, .25f};
        character.state = guiitem::characterState();
        character.state->glow = 1;
        check("character-opaque-material-alpha", background, {character}, {}, {64, 64}, {128, 128, 128});
        character.mirrored = true;
        for (int column = 0; column < 4; ++column)
            character.model[column][0] = -character.model[column][0];
        check("character-opposite-winding", background, {character}, {}, {64, 64}, {128, 128, 128});
        check("character-before-text", foreground, {character}, {}, {64, 64}, {0, 255, 0});
        check("character-retained-depth", background, {character}, {preceding}, {64, 64}, {0, 0, 255});
    }
    // Raw WORLD uses independently supplied matrices. Large equal/opposite
    // translations must cancel before local vertices are evaluated.
    renderer.settings = settings;
    raw.material = glm::vec4(1);
    raw.model = raw.view = glm::mat4(1);
    raw.projection = glm::mat4(1);
    raw.projection[0][0] = -1; // native negative-aspect winding
    raw.projection[3] = {.5f, -.5f, .5f, 1};
    check("raw-matrix-baseline", background, {raw}, {}, {64, 64}, {128, 128, 128});
    raw.model = glm::translate(glm::mat4(1), {16777216.f, 0, 0});
    raw.view = glm::translate(glm::mat4(1), {-16777216.f, 0, 0});
    check("raw-matrix-composition", background, {raw}, {}, {64, 64}, {128, 128, 128});
    // Offset the composite clip-Z coefficient before its local dot. Subtracting
    // after the dot erases this small depth and incorrectly covers the occluder.
    raw.model = raw.view = glm::mat4(1);
    raw.model[0][2] = .5f;
    raw.projection[3][2] = raw.depthOffset = 16777216.f;
    check("raw-depth-visible", background, {raw}, {}, {64, 64}, {128, 128, 128});
    check("raw-depth-coefficient", background, {raw}, {preceding}, {64, 64}, {0, 0, 255});
    // Identical screen-space triangles need different interpolated colors when
    // their retained clip W differs. The flattened control keeps their coverage.
    auto projected = background;
    projected.insert(projected.end(),
                     {{16, 16, 0, 0, 255, 0, 0, 255, 1}, {224, 32, 0, 0, 0, 0, 0, 255, 2}, {32, 224, 0, 0, 0, 0, 0, 255, 2}});
    check("projective-colour", projected, {}, {}, {40, 40}, {168, 0, 0});
    auto flattened = projected;
    for (size_t i = background.size(); i < flattened.size(); ++i) {
        flattened[i].x /= flattened[i].w;
        flattened[i].y /= flattened[i].w;
        flattened[i].w = 1;
    }
    check("projective-flattened-control", flattened, {}, {}, {40, 40}, {125, 0, 0});
    auto clippedDepth = background;
    auto depthQuad = red;
    for (auto &vertex : depthQuad) {
        vertex.a = 255;
        vertex.z = 2;
    }
    clippedDepth.insert(clippedDepth.end(), depthQuad.begin(), depthQuad.end());
    check("gui-positive-z-near-clip", clippedDepth, {}, {}, {64, 64}, {0, 0, 255});
    for (size_t i = background.size(); i < clippedDepth.size(); ++i)
        clippedDepth[i].z = -20000000;
    check("gui-negative-z-far-clip", clippedDepth, {}, {}, {64, 64}, {0, 0, 255});
    using Layer = plasma::LayerCommand;
    auto projectiveMask = background;
    auto leftMask = background;
    for (auto &vertex : leftMask) {
        vertex.x = std::min(vertex.x, 48.f);
        vertex.r = vertex.g = vertex.b = 255;
    }
    projectiveMask.insert(projectiveMask.end(), leftMask.begin(), leftMask.end());
    for (size_t i = background.size(); i < projected.size(); ++i) {
        auto vertex = projected[i];
        vertex.r = 255;
        projectiveMask.push_back(vertex);
    }
    const std::vector<Layer> projectiveCommands{
        {Layer::Kind::Push, {6, 0}}, {Layer::Kind::Pop, {12, 0}}, {Layer::Kind::Mask, {12, 0}, 3}, {Layer::Kind::Mask, {15, 0}}};
    check("projective-raw-mask-uv", projectiveMask, {}, {}, {40, 40}, {0, 0, 255}, 2, {}, projectiveCommands);
    for (size_t i = 12; i < projectiveMask.size(); ++i) {
        projectiveMask[i].x /= projectiveMask[i].w;
        projectiveMask[i].y /= projectiveMask[i].w;
        projectiveMask[i].w = 1;
    }
    check("projective-divided-mask-control", projectiveMask, {}, {}, {40, 40}, {255, 0, 0}, 2, {}, projectiveCommands);
    auto whiteMask = background;
    for (auto &vertex : whiteMask) {
        vertex.x -= .135f;
        vertex.y -= .135f;
        vertex.r = vertex.g = vertex.b = vertex.a = 255;
    }
    auto clipped = background;
    clipped.insert(clipped.end(), red.begin(), red.end());
    clipped.insert(clipped.end(), whiteMask.begin(), whiteMask.end());
    const std::array<std::array<int, 3>, 3> masked{{{255, 127, 127}, {128, 0, 127}, {128, 128, 255}}};
    for (uint32_t mode = 1; mode <= 3; ++mode) {
        const std::vector<Layer> commands{
            {Layer::Kind::Push, {6, 0}}, {Layer::Kind::Pop, {12, 0}}, {Layer::Kind::Mask, {12, 0}, mode}, {Layer::Kind::Mask, {18, 0}}};
        const std::string name = "clip-mode-" + std::to_string(mode);
        check(name.c_str(), clipped, {}, {}, {64, 64}, masked[mode - 1], 2, 255, commands);
    }
    auto triangle = background;
    triangle.insert(triangle.end(), red.begin(), red.end());
    for (auto point : {glm::vec2(20, 20), glm::vec2(108, 20), glm::vec2(20, 108)})
        triangle.push_back({point.x - .135f, point.y - .135f, 0, 0, 255, 255, 255, 255});
    const std::vector<Layer> triangleCommands{
        {Layer::Kind::Push, {6, 0}}, {Layer::Kind::Pop, {12, 0}}, {Layer::Kind::Mask, {12, 0}, 2}, {Layer::Kind::Mask, {15, 0}}};
    check("clip-triangle-inside", triangle, {}, {}, {40, 40}, {128, 0, 127}, 2, {}, triangleCommands);
    check("clip-triangle-outside", triangle, {}, {}, {100, 100}, {0, 0, 255}, 2, {}, triangleCommands);
    check("clip-unlayered-control", triangle, {}, {}, {100, 100}, {128, 0, 127});
    auto empty = background;
    empty.insert(empty.end(), whiteMask.begin(), whiteMask.end());
    const std::vector<Layer> emptyCommands{
        {Layer::Kind::Push, {6, 0}}, {Layer::Kind::Pop, {6, 0}}, {Layer::Kind::Mask, {6, 0}, 2}, {Layer::Kind::Mask, {12, 0}}};
    check("clip-reused-full-clear", empty, {}, {}, {40, 40}, {0, 0, 255}, 2, {}, emptyCommands);
    auto nested = clipped;
    nested.insert(nested.end(), whiteMask.begin(), whiteMask.end());
    const std::vector<Layer> nestedCommands{{Layer::Kind::Push, {6, 0}},     {Layer::Kind::Push, {6, 0}},  {Layer::Kind::Pop, {12, 0}},
                                            {Layer::Kind::Mask, {12, 0}, 2}, {Layer::Kind::Mask, {18, 0}}, {Layer::Kind::Pop, {18, 0}},
                                            {Layer::Kind::Mask, {18, 0}, 2}, {Layer::Kind::Mask, {24, 0}}};
    check("clip-nested-unpremultiplication", nested, {}, {}, {64, 64}, {128, 0, 127}, 2, {}, nestedCommands);
    auto inside = icon, outside = icon;
    inside.beforeVertex = 6;
    outside.beforeVertex = 12;
    outside.mesh = brighter;
    const std::vector<Layer> callbackCommands{
        {Layer::Kind::Push, {6, 0}}, {Layer::Kind::Pop, {6, 1}}, {Layer::Kind::Mask, {6, 1}, 2}, {Layer::Kind::Mask, {12, 1}}};
    check("clip-null-depth-and-restoration", empty, {inside, outside}, {preceding}, {64, 64}, {128, 128, 128}, 2, {}, callbackCommands);
    if (!renderer.setOverlayAtlas(1, 1, white.data()))
        ++failures;
    const std::vector<Layer> replacedAtlas{
        {Layer::Kind::Push, {6, 0}}, {Layer::Kind::Pop, {12, 0}}, {Layer::Kind::Mask, {12, 0}, 2}, {Layer::Kind::Mask, {18, 0}}};
    check("clip-atlas-replacement", clipped, {}, {}, {64, 64}, {128, 0, 127}, 2, {}, replacedAtlas);
    // Leave consecutive frames in flight: the retained pool is shared on the
    // graphics queue, rather than copied per frame or synchronized by readback.
    for (int frame = 0; frame < 4; ++frame) {
        auto pending = clipped;
        for (size_t i = 6; i < 12; ++i) {
            pending[i].r = frame & 1 ? 0.f : 255.f;
            pending[i].g = frame & 1 ? 255.f : 0.f;
        }
        renderer.overlay(pending, {}, replacedAtlas);
        renderer.frame(camera, lighting, {}, 0, {}, {});
    }
    check("clip-queued-pool-reuse", clipped, {}, {}, {64, 64}, {128, 0, 127}, 2, {}, replacedAtlas);
    // Exercise the actual HUD traversal. Only its local white atlas cell is
    // sampled; all test geometry/colors are synthetic and no owner data is saved.
    Assets assets;
    if (assets.locate("")) {
        Hud hud;
        std::string error;
        if (!hud.load(assets.gameDir(), error)) {
            std::cerr << "Owner HUD could not load: " << error << '\n';
            ++failures;
        } else {
            hud.setGuiScale(1);
            hud.layout(renderer, {128, 128});
            Hud::ScreenItem backdrop;
            backdrop.kind = Hud::ScreenItem::Square;
            backdrop.size = {128, 128};
            backdrop.color = {0, 0, 1, 1};
            std::vector<OverlayVertex> ownerBackground;
            hud.drawScreen({backdrop}, ownerBackground);
            hud.addItemIcon("context-integration-fixture", coinCub);
            hud.prepareFrame(renderer, {128, 128});
            Hud::State rowState;
            const auto original = nativesaves::makeItem(24, 1);
            rowState.modelRow.items.push_back({"context-integration-fixture", &original, 64});
            rowState.contextCount = 2;
            std::vector<OverlayVertex> amountTwo, amountOne, amountZero;
            hud.draw(rowState, amountTwo);
            const auto rowRequests = hud.itemDraws();
            if (rowRequests.size() != 2 || rowRequests[1].kind != guiitem::Request::Kind::QueuedModel) {
                std::cerr << "FAIL actual HUD context row callback/resource admission\n";
                ++failures;
            }
            check("owner-context-model-row", {}, rowRequests, {}, {64, 52}, {128, 127, 102});
            rowState.contextCount = 1;
            hud.draw(rowState, amountOne);
            rowState.contextCount = 0;
            hud.draw(rowState, amountZero);
            if (amountTwo.size() <= amountOne.size() || amountOne.size() != amountZero.size()) {
                std::cerr << "FAIL actual HUD context count threshold/font emission\n";
                ++failures;
            }
            rowState.modelRow.items.clear();
            rowState.gold.reset();
            std::vector<OverlayVertex> absentGold, positiveGold, negativeGold;
            hud.draw(rowState, absentGold);
            rowState.gold = 1;
            hud.draw(rowState, positiveGold);
            rowState.gold = -1;
            hud.draw(rowState, negativeGold);
            if (positiveGold.size() <= absentGold.size() || negativeGold.size() <= positiveGold.size()) {
                std::cerr << "FAIL actual HUD signed gold/absence separation\n";
                ++failures;
            }
            ChatLine cachedChat;
            hud.appendChat(cachedChat, u"Cache", {255, 255, 255});
            const auto emitChat = [&] {
                hud.clearItemDraws();
                std::vector<OverlayVertex> text;
                hud.drawChat(cachedChat, 0, text);
                return text;
            };
            const auto chatFrame = [&](const char *name, bool repaint) {
                auto text = emitChat();
                auto commands = hud.clipDraws();
                const bool begins = !commands.empty() && commands.front().kind == plasma::LayerCommand::Kind::CacheBegin;
                if (begins != repaint || (!repaint && (!text.empty() || commands.size() != 1))) {
                    std::cerr << "FAIL actual HUD retained-chat callback admission\n";
                    ++failures;
                }
                auto frame = ownerBackground;
                frame.insert(frame.end(), text.begin(), text.end());
                for (auto &command : commands)
                    command.before.vertices += ownerBackground.size();
                check(name, frame, {}, {}, {5, 5}, {0, 0, 255}, 2, 255, commands);
            };
            chatFrame("owner-chat-first-surface", true);
            chatFrame("owner-chat-post-creation-dirty", true);
            chatFrame("owner-chat-retained-callback-suppression", false);
            const auto assertRepaint = [&] {
                const auto text = emitChat();
                if (hud.clipDraws().empty() || hud.clipDraws().front().kind != plasma::LayerCommand::Kind::CacheBegin) {
                    std::cerr << "FAIL actual HUD chat invalidation\n";
                    ++failures;
                }
            };
            hud.appendChat(cachedChat, u"", {255, 255, 255});
            assertRepaint();
            cachedChat.enter();
            hud.updateChat(cachedChat);
            assertRepaint(); // An equal text/blink editing update still marks the Widget.
            hud.updateChat(cachedChat);
            assertRepaint();
            maphistory::History history({{0, 0}});
            history.show(true);
            hud.bindMapHistory(history.view(), "", true);
            auto *panel = const_cast<plasma::SceneNode *>(hud.mapHistoryRoot());
            if (!panel)
                ++failures;
            else {
                auto children = std::move(panel->children);
                auto widget = std::move(panel->widget);
                panel->local = glm::mat4(1);
                panel->widget.emplace();
                panel->widget->flags = plasma::WIDGET_DRAW_FIRST;
                panel->displayFlags = plasma::DISPLAY_BLUR;
                panel->blurRadius = 4;
                auto probe = std::make_unique<plasma::SceneNode>();
                probe->parent = panel;
                probe->shape = 0;
                probe->animatedShape.emplace();
                probe->animatedDrawing.emplace();
                auto &shape = *probe->animatedDrawing;
                shape.filled = true;
                shape.fill.vertices = {
                    {{20, 20}, {1, 0, 0, 1}}, {{45, 20}, {1, 0, 0, 1}}, {{45, 45}, {1, 0, 0, 1}}, {{20, 45}, {1, 0, 0, 1}}};
                shape.fill.indices = {0, 1, 2, 0, 2, 3};
                panel->children.push_back(std::move(probe));
                std::vector<OverlayVertex> emitted;
                panel->onDraw = [&](plasma::SceneNode &) {
                    if (emitted.size() < 6) {
                        ++failures;
                        return;
                    }
                    const std::vector<OverlayVertex> own(emitted.end() - 6, emitted.end());
                    for (auto vertex : own) {
                        vertex.x += 60;
                        vertex.r = vertex.b = 0;
                        vertex.g = vertex.a = 255;
                        emitted.push_back(vertex);
                    }
                };
                hud.prepareFrame(renderer, {128, 128});
                for (float radius : {4.f, 40.f}) {
                    panel->blurRadius = radius;
                    for (bool clip : {false, true}) {
                        panel->clips = clip;
                        Hud::State state;
                        state.crosshair = false;
                        hud.draw(state, emitted);
                        if (emitted.size() < 6) {
                            ++failures;
                            continue;
                        }
                        auto composed = background;
                        for (auto &vertex : composed) {
                            vertex.u = emitted.back().u;
                            vertex.v = emitted.back().v;
                        }
                        composed.insert(composed.end(), emitted.begin(), emitted.end());
                        auto commands = hud.clipDraws();
                        for (auto &command : commands)
                            command.before.vertices += background.size();
                        auto requests = hud.itemDraws();
                        for (auto &request : requests)
                            request.beforeVertex += background.size();
                        const std::string prefix = std::string(radius > 8 ? "blur-large-" : "blur-") + (clip ? "with-clip-" : "plain-");
                        check((prefix + "discarded-child").c_str(), composed, requests, {}, {32, 32}, {0, 0, 255}, 2, {}, commands);
                        check((prefix + "own-hook").c_str(), composed, requests, {}, {90, 32}, {0, 255, 0}, 2, {}, commands);
                        // Sample the retained final source explicitly. Omitting the
                        // rebind clear would reveal the red child in the plain case.
                        commands.push_back({Layer::Kind::Mask, {composed.size(), requests.size()}, 3});
                        const std::vector<OverlayVertex> mask(emitted.end() - 6, emitted.end());
                        for (auto vertex : mask) {
                            vertex.x -= 60;
                            vertex.r = vertex.g = vertex.b = vertex.a = 255;
                            composed.push_back(vertex);
                        }
                        commands.push_back({Layer::Kind::Mask, {composed.size(), requests.size()}});
                        check((prefix + "empty-final-mask").c_str(), composed, requests, {}, {32, 32}, {0, 0, 255}, 2, {}, commands);
                    }
                }
                panel->onDraw = {};
                panel->children = std::move(children);
                panel->widget = std::move(widget);
            }
        }
    } else
        std::cout << "SKIP owner-atlas HUD blur integration: no local installation\n";
    if (assets.locate("")) {
        settings.taa=settings.smaa=settings.shadows=false;
        renderer.settings=settings;
        Hud editHud;
        if (!editHud.load(assets.gameDir(),error)) ++failures;
        else for (const float scale : {1.f,1.5f}) {
            editHud.setGuiScale(scale);
            editHud.layout(renderer,{128,128});
            editHud.setInterfaceState({{false,false,true,false,false},false,false});
            Hud::ScreenItem backdrop;
            backdrop.kind=Hud::ScreenItem::Square;backdrop.size={128/scale,128/scale};backdrop.color={0,0,1,1};
            Hud::ScreenItem field;
            field.kind=Hud::ScreenItem::Template;field.name="edit";field.instance="render-name";
            field.at={20,40};field.size={80,30};field.text=L"AB";field.retainText=true;
            field.branch=uiroot::Branch::Creation;
            const Hud::Screen screen{backdrop,field};
            editHud.publishFrontendScreen(screen);
            // The normal clip target is linearly resampled during composition.
            // Isolate the submitted caret material for this exact RGB probe;
            // the ordinary clipped images remain retained separately.
            auto *page=const_cast<plasma::SceneNode *>(editHud.interfaceBranchRoot(uiroot::Branch::Creation));
            auto *clip=page ? page->find("clip") : nullptr;
            if (!clip) {std::cerr<<"FAIL ordinary Edit clip fixture\n";++failures;continue;}
            clip->clips=false;
            const auto emit=[&] {
                editHud.clearItemDraws();std::vector<OverlayVertex> vertices;
                editHud.drawScreen(screen,vertices);return vertices;
            };
            const auto unfocused=emit();
            editHud.moveInterfacePointer({60*scale,55*scale});
            editHud.interfaceLeftDown();editHud.interfaceLeftUp();
            const auto off=emit();
            const auto same=[](const std::vector<OverlayVertex> &a,const std::vector<OverlayVertex> &b) {
                if (a.size()!=b.size()) return false;
                for (size_t i=0;i<a.size();++i) {
                    const auto &x=a[i],&y=b[i];
                    if (x.x!=y.x || x.y!=y.y || x.r!=y.r || x.g!=y.g || x.b!=y.b || x.a!=y.a ||
                        x.u!=y.u || x.v!=y.v || x.w!=y.w || x.z!=y.z) return false;
                }
                return true;
            };
            if (!editHud.interfaceHasFocus() || !same(unfocused,off)) {
                std::cerr<<"FAIL ordinary Edit phase0/selection return-only draw\n";++failures;
            }
            for (int phase=1;phase<=3;++phase) {
                editHud.advanceGui(500);
                const auto vertices=emit();
                if (!(phase&1)) {
                    if (!same(off,vertices)) {std::cerr<<"FAIL ordinary Edit phase1000\n";++failures;}
                    continue;
                }
                std::vector<OverlayVertex> red;
                for (const auto &v : vertices) if (v.r==255 && v.g==0 && v.b==0 && v.a==255) red.push_back(v);
                if (red.size()!=6 || vertices.size()!=off.size()+6) {
                    std::cerr<<"FAIL ordinary Edit red caret geometry\n";++failures;continue;
                }
                const glm::ivec2 pixel(int(std::floor((red[0].x+red[2].x)*.5f)),int(std::floor((red[0].y+red[2].y)*.5f)));
                const std::string name="owner-edit-unclipped-caret-"+std::to_string(int(scale*10))+"-"+std::to_string(phase*500);
                check(name.c_str(),vertices,editHud.itemDraws(),{},pixel,{255,0,0},2,{},editHud.clipDraws());
            }
            editHud.interfaceKeyboard(plasma::Input::Key::Down,0x0D);
            if (editHud.interfaceHasFocus() || !same(off,emit())) {
                std::cerr<<"FAIL ordinary Edit cleared-focus draw\n";++failures;
            }
            // The next scale starts at an even 500ms quotient.
            editHud.advanceGui(500);
        }
    }
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
