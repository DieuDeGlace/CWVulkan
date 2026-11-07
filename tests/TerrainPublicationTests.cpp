#include "AlphaLook.h"
#include "Assets.h"
#include "Extracted.h"
#include "World.h"
#include "stb_image.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>

namespace {
template <class T> void field(std::ostream &out, const T &value) {
    out.write(reinterpret_cast<const char *>(&value), sizeof value);
}
void fixture(const std::filesystem::path &directory, int32_t x, int32_t y, uint8_t type) {
    std::ofstream out(directory / ("z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwz"), std::ios::binary);
    out.write("CWZ1", 4);
    for (int32_t value : {x, y, int32_t(0), int32_t(0)}) field(out, value);
    for (int lx = 0; lx < 64; ++lx)
        for (int ly = 0; ly < 64; ++ly) {
            field(out, int32_t(37));
            field(out, uint32_t(1));
            field(out, uint16_t(1));
            const bool target = x == 1 && y == 0 && lx == 32 && ly == 32;
            field(out, worldgen::Block{80, 100, 120, 0, uint8_t(target ? type : 0), uint8_t(target && type ? 0 : 2)});
        }
}
bool finish(World &world) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    do {
        world.update({96, 32, 166});
        if (world.zoneCount() == 1 && !world.pendingCount()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}
struct Draws {
    std::vector<DrawItem> opaque, water;
    explicit Draws(const World &world) { world.collect(opaque, water); }
};
} // namespace

int terrainPublicationTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL terrain publication: %s\n", message);
        }
    };
    const auto directory = std::filesystem::temp_directory_path() /
        ("CubeVulkan-terrain-publication-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    std::ofstream(directory / "manifest.txt") << "seed=0\nhomeX=0\nhomeY=0\nhomeZ=37\n";
    const auto previousStyle = renderStyle();
    for (auto style : {RenderStyle::Steam, RenderStyle::Alpha}) {
        setRenderStyle(style);
        const bool steam = style == RenderStyle::Steam;
        for (int x = 0; x <= 2; ++x)
            for (int y = -1; y <= 1; ++y)
                fixture(directory, x, y, 2);
        ExtractedWorld blocks;
        std::string error;
        check(blocks.open(directory, error), "synthetic non-home water zone opens");
        Renderer renderer;
        RenderSettings settings;
        settings.style = style;
        renderer.initHeadless(settings);
        Assets assets;
        TerrainSettings options;
        options.lilies = false;
        Terrain terrain(options);
        {
            World world(terrain, assets, renderer, 0, &blocks);
            check(finish(world), "initial asynchronous mesh finishes");
            const Draws initial(world);
            check(initial.opaque.empty() && initial.water.size() == 1, "initial active group contains only water");
            const auto initialMesh = initial.water.empty() ? 0 : initial.water.front().mesh;
            check(blocks.setGameBlock(96, 32, 37, {255, 255, 255, 0, 0, 2}), "remove the last water cell");
            Mesh solid, water, lava;
            std::optional<scenevisibility::Bounds> bounds;
            std::vector<TerrainMeshBand> bands;
            check(blocks.mesh(1, 0, solid, water, &bounds, nullptr, &lava, &bands) && solid.empty() && water.empty() &&
                      lava.empty() && bands.empty(), "replacement fixture really has no geometry in any stream");
            for (int repeat = 0; repeat < 2; ++repeat) {
                world.refreshZone(1, 0);
                check(finish(world), "empty asynchronous replacement finishes");
                const Draws empty(world);
                if (steam) {
                    check(empty.opaque.empty() && empty.water.size() == 1 && empty.water.front().mesh == initialMesh,
                          "wholly empty Steam staging preserves the same active water group repeatedly");
                    check(empty.water.size() == 1 && empty.water.front().nativeZone && empty.water.front().nativeZone->meshed &&
                              empty.water.front().nativeZone->triangleCost == 0,
                          "fresh zero-cost zone metadata is independent of retained active geometry");
                } else {
                    check(empty.opaque.empty() && empty.water.empty(), "Alpha keeps its empty-replacement behavior");
                }
            }
            check(blocks.setGameBlock(96, 32, 37, {80, 100, 120, 0, 6, 0}), "replace empty terrain with a solid cell");
            world.refreshZone(1, 0);
            check(finish(world), "solid-only replacement finishes");
            const Draws solidOnly(world);
            check(solidOnly.opaque.size() == 1 && solidOnly.water.empty(),
                  "nonempty solid-only replacement retires old water rather than preserving an empty water stream");
            const auto solidMesh = solidOnly.opaque.empty() ? 0 : solidOnly.opaque.front().mesh;
            check(blocks.setGameBlock(96, 32, 37, {255, 255, 255, 0, 0, 2}), "remove the last solid cell");
            world.refreshZone(1, 0);
            check(finish(world), "second wholly empty replacement finishes");
            const Draws afterSolid(world);
            check(afterSolid.water.empty() && (steam ? afterSolid.opaque.size() == 1 && afterSolid.opaque.front().mesh == solidMesh
                                                   : afterSolid.opaque.empty()),
                  "whole-empty policy applies to active solid groups and is scoped to Steam");
            check(blocks.setGameBlock(96, 32, 37, {80, 100, 120, 0, 2, 0}), "water returns after empty staging");
            world.refreshZone(1, 0);
            check(finish(world), "water replacement finishes");
            const Draws restored(world);
            check(restored.opaque.empty() && restored.water.size() == 1 && restored.water.front().mesh != initialMesh,
                  "later nonempty staging replaces retained groups with newly uploaded water");

            // Invalidate this disposable source cache and damage its non-home file.
            // The indexed source still admits a job, whose actual mesher returns false.
            std::ofstream(directory / "z_1_0.cwz", std::ios::binary) << "BROKEN";
            check(blocks.open(directory, error), "home stays valid while the selected source file is unavailable");
            world.refreshZone(1, 0);
            check(finish(world), "failed source mesh finishes without a stalled job");
            const Draws failed(world);
            check(failed.water.size() == restored.water.size() && failed.water.size() == 1 &&
                      failed.water.front().mesh == restored.water.front().mesh &&
                      failed.water.front().nativeZone == restored.water.front().nativeZone,
                  "failed mesh preserves active handles and metadata rather than publishing an empty success");
            fixture(directory, 1, 0, 2);
            check(blocks.open(directory, error), "disposable source is repaired");
            world.refreshZone(1, 0);
            check(finish(world), "explicit retry after source repair finishes");
            const Draws retry(world);
            check(retry.water.size() == 1 && retry.water.front().mesh != restored.water.front().mesh,
                  "a later successful result can replace geometry retained across source failure");
        }
        check(blocks.setGameBlock(96, 32, 37, {255, 255, 255, 0, 0, 2}), "prepare initially empty zone");
        {
            World empty(terrain, assets, renderer, 0, &blocks);
            check(finish(empty), "initially empty zone completes without prior active groups");
            const Draws draws(empty);
            check(draws.opaque.empty() && draws.water.empty(), "empty initial staging invents no active geometry");
        }
        renderer.shutdown();
    }
    setRenderStyle(previousStyle);
    if (std::filesystem::equivalent(directory.parent_path(), std::filesystem::temp_directory_path()) &&
        directory.filename().string().rfind("CubeVulkan-terrain-publication-", 0) == 0)
        std::filesystem::remove_all(directory);
    std::printf("Native terrain publication: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}

// The CPU publication fixture also exercises real uploads and deferred releases.
// A fixed camera/palette isolates resource lifetime from adaptive-range changes.
int terrainPublicationRenderTests(Renderer &renderer, const std::filesystem::path &root) {
    const auto directory = root / "publication";
    if (!std::filesystem::create_directory(directory)) return 1;
    std::ofstream(directory / "manifest.txt") << "seed=0\nhomeX=0\nhomeY=0\nhomeZ=37\n";
    for (int x = 0; x <= 2; ++x)
        for (int y = -1; y <= 1; ++y)
            fixture(directory, x, y, 2);
    const auto previousStyle = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    const auto previousSettings = renderer.settings;
    renderer.settings.fogDistance = renderer.settings.sceneryDistance = 128;
    ExtractedWorld blocks;
    std::string error;
    if (!blocks.open(directory, error)) {
        renderer.settings = previousSettings;
        setRenderStyle(previousStyle);
        return 1;
    }
    Assets assets;
    TerrainSettings options;
    options.lilies = false;
    Terrain terrain(options);
    int failures = 0;
    {
        World world(terrain, assets, renderer, 0, &blocks);
        const bool installed = finish(world);
        WorldProbe fixed;
        fixed.origin.z = -ExtractedWorld::Z_OFFSET;
        renderer.setWorldProbe(std::move(fixed));
        Camera camera;
        camera.eye = {96.5f, 30, 168};
        camera.target = {96.5f, 32.5f, 165.5f};
        Lighting lighting;
        lighting.timeMs = 0;
        // Advance the initial blue palette once, then freeze scene time. Fully
        // lit water could otherwise saturate to the same blue as the empty sky.
        renderer.frame(camera, lighting, {}, 1, {}, {});
        const auto capture = [&](const char *label, bool draw = true) {
            const Draws draws(world);
            const auto path = directory / (std::string(label) + ".png");
            renderer.capture(path.string());
            renderer.frame(camera, lighting, {}, 1, draw ? draws.opaque : std::vector<DrawItem>{},
                           draw ? draws.water : std::vector<DrawItem>{});
            int width = 0, height = 0, channels = 0;
            auto *bytes = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
            std::vector<uint8_t> pixels;
            if (bytes && width == 128 && height == 128)
                pixels.assign(bytes, bytes + size_t(width) * height * 4);
            stbi_image_free(bytes);
            return pixels;
        };
        const auto empty = capture("empty", false);
        const auto water = capture("active-water");
        const bool visible = installed && !empty.empty() && water.size() == empty.size() && water != empty;
        std::printf("Native water active runtime publication %s\n", visible ? "PASS" : "FAIL");
        failures += !visible;
        bool retained = visible;
        for (int frame = 0; frame < 4; ++frame) {
            retained &= blocks.setGameBlock(96, 32, 37, {255, 255, 255, 0, 0, 2});
            world.refreshZone(1, 0);
            retained &= finish(world);
            retained &= capture("retained-water") == water;
        }
        std::printf("Native water empty-staging GPU retention %s\n", retained ? "PASS" : "FAIL");
        failures += !retained;
        bool replacement = blocks.setGameBlock(96, 32, 37, {80, 100, 120, 0, 6, 0});
        world.refreshZone(1, 0);
        replacement &= finish(world);
        const Draws solidDraws(world);
        const auto solid = capture("solid-replacement");
        replacement &= solidDraws.water.empty() && solidDraws.opaque.size() == 1 &&
                       solid.size() == empty.size() && solid != empty && solid != water;
        replacement &= blocks.setGameBlock(96, 32, 37, {80, 100, 120, 0, 2, 0});
        world.refreshZone(1, 0);
        replacement &= finish(world);
        replacement &= capture("water-replacement") == water;
        std::printf("Native water nonempty replacement GPU resources %s\n", replacement ? "PASS" : "FAIL");
        failures += !replacement;
        world.update({2048, 2048, 166});
        const Draws retiredDraws(world);
        bool retired = retiredDraws.opaque.empty() && retiredDraws.water.empty();
        for (int frame = 0; frame < 4; ++frame)
            retired &= capture("retired-water") == empty;
        std::printf("Native water retirement fence reuse %s\n", retired ? "PASS" : "FAIL");
        failures += !retired;
    }
    renderer.settings = previousSettings;
    setRenderStyle(previousStyle);
    return failures;
}
