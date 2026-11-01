#include "Extracted.h"
#include "ZoneStates.h"
#include "Spawned.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <future>
#include <limits>
#include <thread>

namespace {
namespace fs = std::filesystem;
template <class T> void field(std::ostream &out, const T &value) {
    out.write(reinterpret_cast<const char *>(&value), sizeof value);
}
void materials(const fs::path &directory, int32_t x, int32_t y, float highlands = 1.0f) {
    std::ofstream out(directory / ("z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwm"), std::ios::binary);
    out.write("CWM1", 4);
    field(out, x);
    field(out, y);
    field(out, int32_t(0));
    field(out, int32_t(0));
    for (int i = 0; i < 4096; ++i)
        for (float value : {highlands, 0.5f, 0.25f, 0.0f, 0.0f, 0.0f})
            field(out, value);
}
void fixture(const fs::path &directory, int32_t x, int32_t y) {
    materials(directory, x, y);
    std::ofstream out(directory / ("z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwz"), std::ios::binary);
    out.write("CWZ1", 4);
    field(out, x);
    field(out, y);
    field(out, int32_t(0));
    field(out, int32_t(0));
    for (int i = 0; i < 4096; ++i) {
        field(out, int32_t(-1));
        field(out, uint32_t(1));
        field(out, uint16_t(4));
        const ExtractedWorld::GameBlock block{20, 30, 40, 0, 1, 0x11};
        out.write(reinterpret_cast<const char *>(block.data()), block.size());
    }
}
} // namespace

int blockMutationTests() {
    int failures = 0;
    auto check = [&](bool condition, const char *message) {
        if (!condition) {
            std::printf("FAIL block mutation: %s\n", message);
            ++failures;
        }
    };
    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path directory = fs::temp_directory_path() / ("CubeVulkan-block-edits-" + std::to_string(suffix));
    fs::create_directories(directory);
    {
        std::ofstream manifest(directory / "manifest.txt");
        manifest << "seed=0\nhomeX=0\nhomeY=0\nhomeZ=1\n";
    }
    for (const auto &[x, y] :
         std::vector<std::pair<int32_t, int32_t>>{{0, 0}, {1, 0}, {-1, 0}, {0, -1}, {-1, -1}, {33554431, 0}, {-33554432, 0}})
        fixture(directory, x, y);
    {
        ExtractedWorld world;
        std::string error;
        check(world.open(directory, error), "synthetic block store opens");
        const auto identity = world.gameZoneToken(0, 0);
        check(identity.has_value(), "loaded home zone has stable identity");
        std::promise<void> ready, during, checked, after;
        auto goDuring = during.get_future();
        auto goAfter = after.get_future();
        int beforeType = -1, duringType = -1, afterType = -1;
        std::thread reader([&] {
            beforeType = world.gameBlock(0, 0, 1)[4];
            ready.set_value();
            goDuring.wait();
            duringType = world.gameBlock(0, 0, 1)[4];
            checked.set_value();
            goAfter.wait();
            afterType = world.gameBlock(0, 0, 1)[4];
        });
        ready.get_future().wait();
        {
            auto edits = world.editBlocks();
            check(world.removeGameBlock(0, 0, 1), "in-column removal accepted");
            check(world.gameBlock(0, 0, 1)[4] == 0, "writer sees its physical edit immediately");
            {
                auto nested = world.editBlocks();
                check(world.removeGameBlock(0, 0, 0), "nested removal below Z1 accepted");
            }
            check(world.gameBlock(0, 0, 0) == ExtractedWorld::GameBlock{255, 255, 255, 0, 2, 0}, "below-Z1 removal writes the water value");
            during.set_value();
            checked.get_future().wait();
            check(beforeType == 1 && duringType == 1, "other thread retains old snapshot through nested edits");
            check(!world.removeGameBlock(0, 0, -2) && !world.removeGameBlock(0, 0, 3),
                  "removal cannot grow below or above stored column extent");
            check(!world.removeGameBlock(64, 0, 1) && !world.gameZoneLoaded(1, 0), "missing zone is neither loaded nor changed");
        }
        after.set_value();
        reader.join();
        check(afterType == 0, "reader memo invalidates after publication");
        check(identity && world.gameZoneToken(0, 0)->order == identity->order, "physical edits preserve loaded-zone identity");
        check(world.takeChangedZones().empty(), "physical mutation alone does not request rendering");
        const auto original = ExtractedWorld::GameBlock{20, 30, 40, 0, 1, 0x11};
        const auto empty = ExtractedWorld::GameBlock{255, 255, 255, 0, 0, 0};
        world.rememberBlockChange(zonestates::blockChange(1, 0, 1, original, empty));
        check(world.gameBlock(1, 0, 1) == original && world.takeChangedZones().empty(),
              "history append neither applies advertised after-bytes nor dirties meshes");
        world.rememberBlockChange(zonestates::blockChange(0, 0, 1, original, empty));
        {
            auto edits = world.editBlocks();
            world.removeGameBlock(0, 0, 2);
            world.undoBlockChanges(0, 0);
            check(world.gameBlock(0, 0, 1) == original, "reset inside edit scope sees and reverses current physical state");
        }
        check(world.gameBlock(0, 0, 1) == original, "outer publication cannot overwrite nested reset");
        world.takeChangedZones();

        float height = 0;
        for (const auto &[x, y] :
             std::vector<std::pair<int32_t, int32_t>>{{1, 0}, {-1, 0}, {0, -1}, {-1, -1}, {33554431, 0}, {-33554432, 0}})
            check(world.surface(double(x) * 64, double(y) * 64, height), "neighbor fixture loaded explicitly");
        world.invalidateGameBlock(32, 32);
        check(world.takeChangedZones() == std::vector<std::pair<int, int>>{{0, 0}},
              "interior notice is nine block samples, not nine zones");
        world.invalidateGameBlock(0, 0);
        world.invalidateGameBlock(0, 0);
        check(world.takeChangedZones() == std::vector<std::pair<int, int>>{{-1, -1}, {-1, 0}, {0, -1}, {0, 0}},
              "boundary samples retain negative zones and deduplicate requests");
        world.invalidateGameBlock(std::numeric_limits<int32_t>::max(), 32);
        check(world.takeChangedZones() == std::vector<std::pair<int, int>>{{-33554432, 0}, {33554431, 0}},
              "invalidation offsets wrap signed32 before floor division");
    }
    {
        nativesaves::WorldState stored;
        int replays = 0;
        zonestates::ZoneStates states(&stored, false, [&](auto, const auto &) { ++replays; });
        states.load({0, 0}, {}, {});
        const auto change = zonestates::blockChange(0, 0, 0, {}, {});
        check(states.appendBlockHistory(change) && states.appendBlockHistory(change), "loaded history retains repeated notices");
        check(states.find({0, 0})->blockChanges.size() == 2 && replays == 0, "live append never calls stored-history replay");
        check(!states.appendBlockHistory(zonestates::blockChange(64, 0, 0, {}, {})) && stored.zones.empty(),
              "missing-zone notice cannot create archived state");
    }
    {
        using namespace worldgen;
        ColourInputs colours;
        colours.highlands = 1;
        const SandOffsets sand;
        const worldgen::Block old{11, 22, 33, 44, 5, 6}, requested{7, 8, 9, 10, 12, 13};
        TerrainColumn column{10, {old, old, old}};
        check(storeColumn(column, 0, 0, 15, requested, colours, sand) && column.base == 10 && column.blocks.size() == 6,
              "full setter extends upward without changing base");
        check(column.blocks[3] == EMPTY_BLOCK && column.blocks[4] == EMPTY_BLOCK && column.blocks[5] == requested,
              "upward gaps contain raw zero bytes and requested cell keeps all six bytes");
        check(storeColumn(column, 0, 0, 7, requested, colours, sand) && column.base == 7 && column.blocks[0] == requested &&
                  column.blocks[1] == toBlock(buriedColour(0, 0, 8, colours, sand)) && column.blocks[3] == old,
              "downward fill colors only interior gap, preserving requested and old-base cells");
        TerrainColumn empty;
        check(!storeColumn(empty, 0, 0, 5, AIR_BLOCK, colours, sand) && empty.empty() && empty.base == 0,
              "empty-above-top rejection precedes base change and sea substitution");
        check(storeColumn(empty, 0, 0, -2, AIR_BLOCK, colours, sand) && empty.base == -2 && empty.blocks.size() == 1 &&
                  empty.blocks[0] == SEA_WATER,
              "empty column accepts negative request without a gap to its former base");
        check(storeColumn(empty, 0, 0, -2, {1, 2, 3, 4, 0, 0x40}, colours, sand) && empty.blocks[0] == SEA_WATER,
              "sea substitution replaces entire block even with nonzero flags other than bit2");
        const worldgen::Block dry{1, 2, 3, 4, 0, 2};
        check(storeColumn(empty, 0, 0, -2, dry, colours, sand) && empty.blocks[0] == dry, "dry-air bit2 prevents sea substitution");
        check(!setterColumn(33554431, 0, std::numeric_limits<int32_t>::max(), 0) &&
                  setterColumn(-33554432, 0, std::numeric_limits<int32_t>::min(), 0) == size_t(0),
              "direct setter bounds retain signed low32 upper-bound wrap");
        const worldgen::Block packed = toBlock({255.0f, 510.0f, -10.0f, 3, 6, 7});
        check(packed == worldgen::Block{127, 255, 0, 3, 6, 7}, "buried color packing halves, clamps and truncates");
    }
    {
        ExtractedWorld world;
        std::string error;
        check(world.open(directory, error), "captured material fixture opens");
        const ExtractedWorld::GameBlock requested{7, 8, 9, 10, 12, 13};
        check(world.setGameBlock(0, 0, -3, requested) && world.gameBlock(0, 0, -3) == requested && world.gameBlock(0, 0, -2)[4] == 6,
              "recorded column materials drive downward live setter fill");
    }
    {
        ExtractedWorld terrain;
        std::string error;
        check(terrain.open(directory, error), "adapter fixture opens");
        nativesaves::WorldState stored;
        SpawnedCreatures actors;
        actors.setStoredZones(terrain, &stored, false);
        const ExtractedWorld::GameBlock original{20, 30, 40, 0, 1, 0x11}, empty{255, 255, 255, 0, 0, 0}, red{200, 0, 0, 0, 1, 0};
        const auto record = [&](int x, int y, int z, const auto &after) {
            const auto kept = zonestates::blockChange(x, y, z, original, after);
            cwnet::BlockChangeRecord wire;
            std::copy(kept.bytes.begin(), kept.bytes.end(), wire.begin());
            return wire;
        };
        actors.applyBlockChange(record(0, 0, 0, empty));
        check(terrain.gameBlock(0, 0, 0)[4] == 2, "received empty notice uses water-preserving removal");
        actors.applyBlockChange(record(0, 0, 3, empty));
        check(!terrain.removeGameBlock(0, 0, 3), "failed received removal cannot extend column");
        actors.applyBlockChange(record(64, 0, 1, red));
        check(!terrain.gameZoneLoaded(1, 0), "receiving a missing-zone notice does not load terrain");
        actors.applyBlockChange(record(0, 0, 6, red));
        check(terrain.gameBlock(0, 0, 6) == red && terrain.gameBlock(0, 0, 4) == ExtractedWorld::GameBlock{},
              "received nonempty notice uses the full setter with zero upward gaps");
        cwnet::ZoneAnswer answer;
        check(actors.zoneAnswer(terrain, {0, 0}, answer) && answer.blockChanges.size() == 3,
              "received history includes failed loaded removal but excludes missing zone");
        actors.events().blockChanges.push_back(record(0, 0, 0, red));
        actors.simulate(terrain, 0);
        actors.simulate(terrain, 0);
        answer = {};
        check(actors.zoneAnswer(terrain, {0, 0}, answer) && answer.blockChanges.size() == 5 && actors.events().blockChanges.size() == 1,
              "real World tail repeats outstanding history across zero substeps without consuming notices");
        check(terrain.gameBlock(0, 0, 0)[4] == 2 && terrain.takeChangedZones().empty(),
              "World history tail neither re-applies advertised data nor requests meshes");
        actors.recordAllZones();
        check(stored.zones.size() == 1 && stored.zones.begin()->second.blockChanges.size() == 5,
              "unload persists exact repeated native history");
    }
    materials(directory, 0, 0, std::numeric_limits<float>::quiet_NaN());
    {
        std::string error;
        check(!ExtractedWorld::validateZoneFile(directory / "z_0_0.cwz", error) && error.find("material") != std::string::npos,
              "nonfinite material sidecar rejected by structural validation");
    }
    fs::remove_all(directory);
    std::printf("Physical block edits, history and render requests: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
