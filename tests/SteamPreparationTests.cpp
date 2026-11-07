#include "AlphaLook.h"
#include "Assets.h"
#include "Extracted.h"
#include "World.h"
#include "worldgen/Options.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <future>
#include <thread>

namespace {
template <class T> void field(std::ostream &out, const T &value) {
    out.write(reinterpret_cast<const char *>(&value), sizeof value);
}
void fixture(const std::filesystem::path &directory, int32_t x, int32_t y) {
    std::ofstream out(directory / ("z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwz"), std::ios::binary);
    out.write("CWZ1", 4);
    for (int32_t value : {x, y, int32_t(0), int32_t(0)}) field(out, value);
    for (int i = 0; i < 4096; ++i) {
        field(out, int32_t(37));
        field(out, uint32_t(1));
        field(out, uint16_t(1));
        field(out, worldgen::Block{80, 100, 120, 0, 6, 0});
    }
}
bool finish(World &world, glm::vec3 focus) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    do {
        world.update(focus);
        if (!world.pendingCount()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}
void selection(const auto &check) {
    scenerange::PreparationBatch batch(3);
    int visits = 0;
    const auto complete = [&] { ++visits; return true; };
    check(!batch.consider(566231, false, complete) && visits == 0,
          "clean nonzero cost is accounted without visiting neighbors");
    check(batch.consider(0, false, complete) && visits == 1,
          "clean zero cost is admitted at the exact independent budget boundary");
    check(!batch.consider(1, false, complete) && !batch.consider(0, true, complete) && visits == 1,
          "clean cost pushes the prefix over budget before a later dirty zero result");
    check(batch.consider(5, true, complete) && visits == 2,
          "dirty nonzero cost remains admitted beyond the prefix budget");
    check(!batch.consider(-7, false, complete) && visits == 2,
          "unselected negative signed cost still restores prefix capacity");
    check(!batch.consider(0, false, [&] { ++visits; return false; }) &&
              batch.consider(0, false, complete) && visits == 4,
          "incomplete neighbors reject before a later zero-cost candidate");
    scenerange::PreparationBatch capped(3);
    visits = 0;
    for (int i = 0; i < 12; ++i)
        check(capped.consider(0, false, complete) == (i < 8), "selection admits only the first eight complete candidates");
    check(visits == 12, "neighborhood checks still precede the full-batch gate");
}
} // namespace

int steamPreparationTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL Steam preparation: %s\n", message);
        }
    };
    selection(check);
    if (worldgen::modHooksSet()) {
        check(false, "fixture must not replace another owner's generator hooks");
        return failures;
    }
    const auto directory = std::filesystem::temp_directory_path() /
        ("CubeVulkan-steam-preparation-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    std::ofstream(directory / "manifest.txt") << "seed=0\nhomeX=0\nhomeY=0\nhomeZ=37\n";
    for (int x = -2; x <= 2; ++x)
        for (int y = -2; y <= 2; ++y)
            fixture(directory, x, y);
    const auto previousStyle = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    ExtractedWorld blocks;
    std::string error;
    check(blocks.open(directory, error), "disposable complete-neighborhood source opens");
    Renderer renderer;
    RenderSettings settings;
    settings.style = RenderStyle::Steam;
    renderer.initHeadless(settings);
    Assets assets;
    TerrainSettings options;
    options.lilies = false;
    Terrain terrain(options);
    std::promise<void> entered, release;
    const auto enteredFuture = entered.get_future();
    const auto releaseFuture = release.get_future().share();
    std::atomic<bool> claimed = false;
    const auto caller = std::this_thread::get_id();
    {
        World world(terrain, assets, renderer, 0, &blocks);
        check(finish(world, {32, 32, 166}) && world.zoneCount() == 1, "initial active terrain finishes before installing the gate");
        std::vector<DrawItem> opaque, water;
        world.collect(opaque, water);
        check(opaque.size() == 1, "initial active terrain is drawable");
        worldgen::ModHooks hooks;
        hooks.placeKind = [&](int32_t, int32_t, bool, int kind) {
            // Only the first worker to load an uncached disposable neighbor waits.
            // Other workers use the unchanged answer and can finish their meshes.
            if (std::this_thread::get_id() != caller && !claimed.exchange(true)) {
                entered.set_value();
                releaseFuture.wait();
            }
            return kind;
        };
        worldgen::setModHooks(hooks);
        world.setViewZones(1);
        world.update({32, 32, 166});
        check(world.pendingCount() == 8, "selected native batch spans eight coordinates with bounded worker lanes");
        const bool held = enteredFuture.wait_for(std::chrono::seconds(10)) == std::future_status::ready;
        check(held, "controlled worker reaches the source gate");
        if (held) {
            world.setCpuPerformance(0); // a preference change waits for this batch boundary
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            while (std::chrono::steady_clock::now() < deadline) {
                world.update({32, 32, 166});
                if (world.zoneCount() != 1) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            check(world.zoneCount() == 1, "no partial batch publishes while a sibling mesh is still running");
            world.update({8192, 8192, 166});
            opaque.clear(); water.clear();
            world.collect(opaque, water);
            check(!opaque.empty(), "terrain retirement waits for the outstanding native batch");
        }
        release.set_value();
        world.setCpuPerformance(-1); // all nonzero values choose the reached FULL branch
        world.setViewZones(0);
        check(finish(world, {8192, 8192, 166}), "released workers finish and permit the next preparation phase");
        opaque.clear(); water.clear();
        world.collect(opaque, water);
        check(opaque.empty() && water.empty(), "completed batch permits later terrain retirement");
    }
    worldgen::setModHooks({});
    renderer.shutdown();
    setRenderStyle(previousStyle);
    if (std::filesystem::equivalent(directory.parent_path(), std::filesystem::temp_directory_path()) &&
        directory.filename().string().rfind("CubeVulkan-steam-preparation-", 0) == 0)
        std::filesystem::remove_all(directory);
    std::printf("Steam mesh preparation: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
