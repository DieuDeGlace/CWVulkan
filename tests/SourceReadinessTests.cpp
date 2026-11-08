#include "AlphaLook.h"
#include "Assets.h"
#include "Extracted.h"
#include "World.h"
#include <chrono>
#include <cstdio>
#include <thread>

int sourceReadinessTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL source readiness: %s\n", message);
        }
    };
    if (worldgen::modHooksSet() || worldgen::usingAlphaTerrain()) {
        check(false, "fixture requires the independent Steam generator without another owner's hooks");
        return failures;
    }
    worldgen::forgetZones();
    const auto previous = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    Renderer renderer;
    RenderSettings settings;
    settings.style = RenderStyle::Steam;
    renderer.initHeadless(settings);
    Assets assets;
    TerrainSettings options;
    options.lilies = false;
    Terrain terrain(options);
    {
        World world(terrain, assets, renderer, 3, &blocks);
        world.setSourceRenderDistance(10);
        scenerange::Player player;
        player.physical = {-32768, -32768, int64_t(blocks.home().z - ExtractedWorld::Z_OFFSET) * 65536};
        world.setScenePlayerProvider([&] { return std::optional(player); });
        const glm::vec3 focus{-0.5f, -0.5f, blocks.home().z};
        world.update(focus);
        check(world.preparingTerrain() && world.pendingSourceCount() == 4,
              "cold retained source inputs block readiness while their real source batch runs");
        // Independent V10 footprint intersection: 45 demanded inputs, 36 inside
        // the physical radius-four circle centred at(-1,-1), nine fringe inputs.
        struct Row { int x, minimumY, maximumY; };
        constexpr Row persistent[]{{-3,-2,2},{-2,-3,2},{-1,-3,3},{0,-3,2},{1,-3,2},{2,-3,1},{3,-1,-1}};
        unsigned stable = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (stable < 16 && std::chrono::steady_clock::now() < deadline) {
            world.update(focus);
            bool inputs = true;
            for (const auto row : persistent)
                for (int y = row.minimumY; y <= row.maximumY; ++y)
                    inputs &= blocks.gameZoneLoaded(row.x, y);
            if (inputs && world.pendingCount() == 0 && world.zoneCount() > 0 && world.pendingSourceCount() > 0)
                ++stable;
            else
                stable = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        check(stable == 16, "real world reaches repeated fringe-only source work with prepared geometry and complete retained inputs");
        std::printf("Source readiness observation: %zu prepared, %zu mesh jobs, %zu loaded sources, %zu source jobs\n",
                    world.zoneCount(), world.pendingCount(), blocks.loadedSourceCount(), world.pendingSourceCount());
        check(!world.preparingTerrain(), "background fringe churn does not block a prepared scene indefinitely");
        check(world.pendingSourceCount() > 0 && world.zoneCount() > 0,
              "ready scenes retain active source scheduling and prepared geometry");
        world.setSourceRenderDistance(15);
        check(world.preparingTerrain(), "source-distance changes invalidate an earlier readiness decision");
        world.setSourceRenderDistance(10);
        world.setViewZones(4);
        check(world.preparingTerrain(), "range changes invalidate an earlier background-only readiness decision");
        world.setViewZones(3);
        player.physical[0] = 64LL * 65536;
        world.update({64.0f, -0.5f, blocks.home().z});
        check(world.preparingTerrain(), "changed physical demand blocks readiness during an older source batch");
        player.physical[0] = -32768;
        player.ability = 0x9c;
        player.eagleDestination = {128,128};
        world.update(focus);
        check(world.preparingTerrain(), "new destination demand remains required even during an older background batch");
        world.setScenePlayerProvider({});
    }
    renderer.shutdown();
    setRenderStyle(previous);
    worldgen::forgetZones();
    std::printf("Native source readiness: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
