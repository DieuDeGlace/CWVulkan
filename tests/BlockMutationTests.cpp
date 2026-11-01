#include "Extracted.h"
#include "ZoneStates.h"
#include "Spawned.h"
#include <chrono>
#include <cstdio>
#include <cstring>
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
    {
        const auto modelDirectory = directory / "models";
        fs::create_directory(modelDirectory);
        fs::copy_file(directory / "manifest.txt", modelDirectory / "manifest.txt");
        fixture(modelDirectory, 0, 0);
        std::vector<ExtractedWorld::ModelRecord> models;
        const auto addModel = [&](int32_t type, int64_t x, int64_t z, int32_t flags) {
            ExtractedWorld::ModelRecord record{};
            const auto put = [&](size_t offset, const auto &value) { std::memcpy(record.data() + offset, &value, sizeof value); };
            put(0, type);
            put(8, x * 65536);
            put(24, z * 65536);
            put(32, 99.0f);
            put(36, 180.0f);
            put(40, 123.5f);
            put(56, flags);
            record[4] = uint8_t(0xa0 + models.size());
            record[60] = uint8_t(0xb0 + models.size());
            models.push_back(record);
        };
        addModel(100, 0, 3, 2);
        addModel(101, 1, 4, 2);
        addModel(102, 2, 4, 0);
        addModel(103, 3, 1, 2 | 8);
        addModel(0x59, 4, 4, 2);
        {
            std::ofstream out(modelDirectory / "z_0_0.rec", std::ios::binary);
            out.write("CWR1", 4);
            field(out, uint32_t(0x18));
            field(out, uint32_t(64));
            field(out, uint32_t(models.size()));
            out.write(reinterpret_cast<const char *>(models.data()), models.size() * 64);
            field(out, uint32_t(0x90));
            field(out, uint32_t(64));
            field(out, uint32_t(1));
            out.write(reinterpret_cast<const char *>(models[1].data()), 64);
        }
        ExtractedWorld terrain;
        std::string error;
        check(terrain.open(modelDirectory, error), "ordinary model fixture opens");
        const auto before = terrain.gameModels(0, 0);
        check(before && before->size() == 5, "all ordinary models retained before resource-name filtering");
        terrain.removeGameBlock(3, 0, 0);
        check(terrain.removeUnsupportedModels(0, 0, 0, 0) == 2,
              "cleanup visits whole selected vector, removes unsupported flags2 models including shafts, retains water support");
        const auto current = terrain.gameModels(0, 0);
        const auto type = [](const auto &record) {
            int32_t value;
            std::memcpy(&value, record.data(), 4);
            return value;
        };
        check(current && current->size() == 3 && type((*current)[0]) == 100 && type((*current)[1]) == 102 && type((*current)[2]) == 103,
              "semantic model survivors keep relative order independent of missing assets, size and yaw");
        check(before && before->size() == 5 && current && (*current)[1][4] == models[1][4] && (*current)[1][60] == models[1][60],
              "old snapshot remains immutable and shifted records preserve destination padding");
        check(current && ((*current)[2][56] & 8), "retained semantic flags survive vector erasure");
        check(terrain.takeChangedZones().empty() && terrain.takeChangedModels() == std::vector<std::pair<int, int>>{{0, 0}},
              "support cleanup requests internal scenery refresh without terrain notice or dirty request");
        terrain.removeGameBlock(0, 0, 2);
        check(terrain.removeUnsupportedModels(-1000, -1000, -100, -100) == 1 && terrain.gameModels(0, 0)->size() == 2,
              "four negative bounds independently clamp to zone0");
        check(!terrain.gameModels(1, 0), "missing model zone is not loaded");
        ExtractedWorld reversed;
        check(reversed.open(modelDirectory, error) && reversed.removeUnsupportedModels(63, 63, 1, 1) == 2,
              "reversed block bounds still select the same zone when converted zone bounds agree");
    }
    {
        ExtractedWorld terrain;
        std::string error;
        check(terrain.open(directory, error), "destructive-source adapter fixture opens");
        nativesaves::WorldState stored;
        SpawnedCreatures actors;
        actors.setStoredZones(terrain, &stored, false);
        actors.setClock(7, 5000);
        bool made = false;
        auto *source = actors.networkCreature(901, made);
        source[0x60] = 7;
        const auto put = [&](size_t offset, const auto &value) { std::memcpy(source + offset, &value, sizeof value); };
        put(0x64, int32_t(0x90));
        put(0x180, 0.0f);
        put(0x6c, 0.0f);
        actors.simulate(terrain, 0);
        const size_t notices = actors.events().blockChanges.size();
        check(notices > 0 && terrain.gameBlock(0, 0, 0)[4] == 2 && terrain.gameBlock(0, 0, 1)[4] == 0,
              "real destructive source mutates loaded columns to water/empty before its notices");
        if (notices) {
            int32_t day;
            std::memcpy(&day, actors.events().blockChanges[0].data() + 0x18, 4);
            check(day == 7, "actual source notices carry World day");
        }
        actors.simulate(terrain, 0);
        cwnet::ZoneAnswer answer;
        check(actors.zoneAnswer(terrain, {0, 0}, answer) && answer.blockChanges.size() == notices * 2 &&
                  actors.events().blockChanges.size() == notices,
              "destructive-source zero repeat appends retained history without breaking blocks twice");
    }
    {
        ExtractedWorld terrain;
        std::string error;
        check(terrain.open(directory, error), "block-light fixture opens");
        check(terrain.gameBlockLight(64, 0, 0) == 255, "missing block-light zone is fully lit without being loaded");
        terrain.setGameBlock(0, 0, 2, {0, 0, 0, 0, 10, 0});
        terrain.setGameBlock(0, 0, 3, {0, 0, 0, 0, 12, 0});
        terrain.setGameBlock(0, 0, 4, {0, 0, 0, 0, 19, 0});
        terrain.setGameBlock(0, 0, 5, {3, 0, 0, 0, 0, 40});
        check(terrain.gameBlockLight(0, 0, 2) == 40, "channel0 climbs types10/12/19 and combines open-block channels");
        terrain.setGameBlock(0, 0, 5, {2, 0, 0, 0, 2, 0});
        check(terrain.gameBlockLight(0, 0, 5) == 5, "dark open water retains the native floor5");
        terrain.setGameBlock(0, 0, 5, {250, 0, 0, 0, 1, 17});
        check(terrain.gameBlockLight(0, 0, 5) == 17, "solid light reads byte5 rather than palette red");
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
