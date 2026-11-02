#include "Assets.h"
#include "Extracted.h"
#include "World.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <thread>

namespace {
template <class T> void field(std::ostream &out, const T &value) {
    out.write(reinterpret_cast<const char *>(&value), sizeof value);
}
template <class T> void put(std::array<uint8_t, 0xe0> &record, size_t offset, const T &value) {
    std::memcpy(record.data() + offset, &value, sizeof value);
}
void fixtureZone(const fs::path &folder, int32_t x, int32_t y, int32_t objectX, bool originObject = false) {
    const auto stem = "z_" + std::to_string(x) + "_" + std::to_string(y);
    std::ofstream terrain(folder / (stem + ".cwz"), std::ios::binary);
    terrain.write("CWZ1", 4);
    field(terrain, x);
    field(terrain, y);
    field(terrain, int32_t(0));
    field(terrain, int32_t(0));
    for (int column = 0; column < 4096; ++column) {
        field(terrain, int32_t(0));
        field(terrain, uint32_t(0));
    }
    std::ofstream records(folder / (stem + ".rec"), std::ios::binary);
    records.write("CWR1", 4);
    field(records, uint32_t(0x60));
    field(records, uint32_t(0xe0));
    field(records, uint32_t(originObject ? 2 : 1));
    for (int i = 0; i < (originObject ? 2 : 1); ++i) {
        std::array<uint8_t, 0xe0> object{};
        put(object, 0, int32_t(3));
        const std::array<int64_t, 3> point = i ? std::array<int64_t, 3>{} : std::array<int64_t, 3>{int64_t(objectX) * 65536, 65536, 65536};
        put(object, 8, point);
        put(object, 0x24, 1.f);
        object[0x28] = 1;
        records.write(reinterpret_cast<const char *>(object.data()), object.size());
    }
}
std::vector<DrawItem> objects(World &world) {
    std::vector<DrawItem> opaque, water;
    world.collect(opaque, water);
    std::erase_if(opaque, [](const DrawItem &draw) { return !draw.nativeObject; });
    return opaque;
}
} // namespace

int staticObjectStreamingTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL static-object streaming: %s\n", message);
        }
    };
    const auto root = fs::temp_directory_path() /
                      ("CubeVulkan-object-streaming-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto folder = root / "Extract" / "fixture";
    fs::create_directories(folder);
    fs::create_directories(root / "sprites");
    std::ofstream(root / "cubeworld.exe") << "asset resolver fixture; never executable";
    std::ofstream(root / "Extract" / "sprite-groups.txt") << "# building-part\n3 fixture-door.cub\n";
    std::ofstream(folder / "manifest.txt") << "seed=0\nhomeX=1\nhomeY=1\nhomeZ=1\n";
    {
        std::ofstream cube(root / "sprites" / "fixture-door.cub", std::ios::binary);
        field(cube, int32_t(2));
        field(cube, int32_t(1));
        field(cube, int32_t(3));
        for (int i = 0; i < 6; ++i) {
            const uint8_t rgb[]{80, 70, 60};
            cube.write(reinterpret_cast<const char *>(rgb), 3);
        }
    }
    fixtureZone(folder, 0, 0, 1, true);
    fixtureZone(folder, -1, 0, -63);
    fixtureZone(folder, 1, 0, 65);
    const auto oldStyle = renderStyle();
    {
        Renderer renderer;
        RenderSettings settings;
        settings.style = RenderStyle::Steam;
        renderer.initHeadless(settings);
        Assets assets;
        ExtractedWorld blocks;
        std::string error;
        const bool ready = assets.locate(utf8(root)) && blocks.open(folder, error);
        check(ready, "synthetic assets and captured zones open");
        if (ready) {
            blocks.propsAbsolute(-1, 0); // Load data only; no World terrain mesh exists.
            check(!blocks.loadedObjectProps(1, 0) && !blocks.gameZoneLoaded(1, 0), "object lookup does not load a present disk zone");
            Terrain terrain;
            World world(terrain, assets, renderer, 0, &blocks);
            scenerange::Player player;
            player.physical = {65536, 65536, 65536};
            world.setScenePlayerProvider([&]() -> std::optional<scenerange::Player> { return player; });
            const glm::vec3 remote{64000, 64000, 129};
            world.update(remote);
            auto draws = objects(world);
            int liveQueries = 0;
            std::vector<DrawItem> initialOpaque, initialWater;
            world.collect(initialOpaque, initialWater);
            const bool terrainDraw =
                std::any_of(initialOpaque.begin(), initialOpaque.end(), [](const auto &draw) { return draw.kind == DrawKind::Terrain; });
            check(!terrainDraw && initialWater.empty() && draws.size() == 3,
                  "loaded objects render before any terrain mesh publication, including a valid all-zero position");
            if (draws.size() == 3) {
                check((*draws[0].nativePosition)[0] == -63 * 65536 && (*draws[1].nativePosition)[0] == 65536 &&
                          *draws[2].nativePosition == steampoints::Point{},
                      "absolute zone XY order and source record order are retained");
                const auto closed = draws[1].model;
                world.setObjectProvider([&](int, int, const objectvisual::Key &) -> std::optional<objectvisual::State> {
                    ++liveQueries;
                    return objectvisual::State{3, 0, 0, 1, 0};
                });
                draws = objects(world);
                check(liveQueries == 3 && std::memcmp(&closed, &draws[1].model, sizeof closed) != 0,
                      "independent records retain live door clocks without duplicate provider calls");
            }
            player.physical[0] = 4 * 64 * 65536;
            world.update(remote);
            check(objects(world).empty() && !blocks.gameZoneLoaded(1, 0),
                  "physical player square retires old records without loading new zones");
            player.physical[0] = 65536;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            do {
                world.update(remote);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } while (world.pendingCount() && std::chrono::steady_clock::now() < deadline);
            check(world.pendingCount() == 0, "remote empty mesh job completes before source reset");
            if (!world.pendingCount()) {
                const auto before = blocks.gameZoneToken(0, 0);
                fixtureZone(folder, 0, 0, 8);
                check(blocks.open(folder, error), "source reset reloads fixture");
                const auto after = blocks.gameZoneToken(0, 0);
                world.update(remote);
                draws = objects(world);
                check(before && after && before->order != after->order && draws.size() == 1 && (*draws[0].nativePosition)[0] == 8 * 65536,
                      "zone lifetime changes replace old object records and discard unloaded neighbors");
                const auto meshDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                do {
                    world.update({1, 1, 129});
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } while (world.pendingCount() && std::chrono::steady_clock::now() < meshDeadline);
                liveQueries = 0;
                draws = objects(world);
                check(world.pendingCount() == 0 && draws.size() == 3 && liveQueries == 3,
                      "terrain publication does not duplicate static records or their live callbacks");
                fixtureZone(folder, 0, 0, 9);
                world.refreshModels(0, 0);
                world.update({1, 1, 129});
                draws = objects(world);
                const auto current = blocks.gameZoneToken(0, 0);
                check(
                    current && after && current->order == after->order &&
                        std::any_of(draws.begin(), draws.end(), [](const auto &draw) { return (*draw.nativePosition)[0] == 9 * 65536; }) &&
                        std::none_of(draws.begin(), draws.end(), [](const auto &draw) { return (*draw.nativePosition)[0] == 8 * 65536; }),
                    "explicit model invalidation refreshes object records within the same zone lifetime");
            }
        }
    }
    setRenderStyle(oldStyle);
    fs::remove_all(root);
    std::printf("Native static-object streaming: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
