// The new world generator's structures and zone contents (src/worldgen, docs/specs/
// worldgen-structures*.md) against the game's own outputs (worldgen-structures.md §8):
//   - Extract/gen-cells.txt: the cell objects of ~60,000 cells;
//   - Extract/gen_seed_123: 125 zones the game generated afresh: their blocks (.cwz), objects,
//     models and items (.rec), every rand() call (.rtrace), and the world's day (day.txt).
// `EngineTests wgstructures [zones]` runs everything with details and timings; the default test
// run takes a fast subset.
//
// The planner's facts come from part C's region planner; every creature hook is left unset, so
// the rand() streams are compared up to each zone's first creature draw (part C's own test,
// WorldgenCreatureTests.cpp, compares the whole streams).
#include "Assets.h"
#include "Extracted.h"
#include "WorldgenObjectOracle.h"
#include "../tools/ZoneExtractor/DungeonCapture.h"
#include "../tools/ZoneExtractor/DungeonCeilingCapture.h"
#include "../tools/ZoneExtractor/DungeonGateCapture.h"
#include "../tools/ZoneExtractor/DungeonSpawnCapture.h"
#include "worldgen/CellObjects.h"
#include "worldgen/BuildingBuilder.h"
#include "worldgen/Mason.h"
#include "worldgen/Structures.h"
#include "worldgen/WorldValues.h"
#include "worldgen/ZoneCreatures.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace {

int failures = 0;
#define WS_CHECK(x)                                                                                \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                              \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

namespace wg = worldgen;
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

fs::path gameDir() {
    Assets a;
    return a.locate("") ? a.gameDir() : fs::path();
}
double seconds(Clock::time_point a) { return std::chrono::duration<double>(Clock::now() - a).count(); }

// --- gen-cells.txt ----------------------------------------------------------------------------

int cellsOracle(const fs::path &dir, bool verbose, int maxCells) {
    std::ifstream in(dir / "Extract" / "gen-cells.txt");
    if (!in)
        return 0;
    const wg::WorldValues world = wg::worldValues(123);
    wg::CellObjects cells(world);
    std::map<std::pair<int, int>, int> wrongBy;
    int count = 0, wrong = 0, shown = 0;
    std::string line;
    while (std::getline(in, line) && count < maxCells) {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream s(line);
        int cx, cy, game;
        if (!(s >> cx >> cy >> game))
            continue;
        count++;
        const int mine = cells.type(cx, cy);
        if (mine != game) {
            wrong++;
            wrongBy[{game, mine}]++;
            if (verbose && shown++ < 20)
                std::printf("  cell %d %d: game %d, new %d\n", cx, cy, game, mine);
        }
    }
    if (verbose)
        for (auto &[k, n] : wrongBy)
            std::printf("  game %d / new %d: %d cells\n", k.first, k.second, n);
    std::printf("wgstructures gen-cells: %d of %d cells differ\n", wrong, count);
    WS_CHECK(count > 0 && wrong == 0);
    return wrong;
}

// --- The generated zones ------------------------------------------------------------------------

struct GameZone {
    int32_t zx = 0, zy = 0;
    std::vector<wg::TerrainColumn> columns;
    std::map<uint32_t, std::vector<std::vector<uint8_t>>> lists; // .rec lists by zone offset
    std::vector<std::pair<uint32_t, uint16_t>> trace;
};

bool readZone(const fs::path &cwz, GameZone &z) {
    if (!ExtractedWorld::zoneCoordinates(cwz, z.zx, z.zy))
        return false;
    std::string error;
    z.columns = ExtractedWorld::readStoredZoneColumns(cwz, error);
    if (z.columns.size() != wg::ZONE_COLUMNS)
        return false;
    for (const auto &column : z.columns) {
        // The legacy effective comparison below uses an int32 top and loop.
        if (column.base == INT32_MIN || int64_t(column.base) + int64_t(column.blocks.size()) >= INT32_MAX)
            return false;
    }
    fs::path rec = cwz;
    rec.replace_extension(".rec");
    std::ifstream in(rec, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    for (size_t at = 4; data.size() >= 4 && at + 12 <= data.size();) {
        uint32_t offset, stride, count;
        std::memcpy(&offset, &data[at], 4), std::memcpy(&stride, &data[at + 4], 4), std::memcpy(&count, &data[at + 8], 4);
        at += 12;
        if (!stride || count > (data.size() - at) / stride)
            break;
        for (uint32_t i = 0; i < count; i++)
            z.lists[offset].emplace_back(data.begin() + (std::ptrdiff_t)(at + (size_t)i * stride),
                                         data.begin() + (std::ptrdiff_t)(at + (size_t)(i + 1) * stride));
        at += (size_t)stride * count;
    }
    fs::path rt = cwz;
    rt.replace_extension(".rtrace");
    std::ifstream tin(rt, std::ios::binary);
    std::vector<uint8_t> tr((std::istreambuf_iterator<char>(tin)), {});
    z.trace.resize(tr.size() / 6);
    for (size_t i = 0; i < z.trace.size(); i++)
        std::memcpy(&z.trace[i].first, &tr[i * 6], 4), std::memcpy(&z.trace[i].second, &tr[i * 6 + 4], 2);
    return true;
}

template <class T> T field(const std::vector<uint8_t> &r, size_t off) {
    T v;
    std::memcpy(&v, &r[off], sizeof v);
    return v;
}

// The draws of part C's routines (by the caller of rand() in the trace): the creatures' looks
// and gear, the region planner and its roles, the castle, dungeon and wild group creatures, the
// townspeople, the animals and the marked-region pass. Anything else is part B's (or A's).
bool creatureCaller(uint32_t rva) {
    return (rva >= 0x40000 && rva < 0x60000) || (rva >= 0x110000 && rva < 0x150000) || (rva >= 0x2fa7b0 && rva < 0x2fc270) ||
           (rva >= 0x2fdf90 && rva < 0x2fe300) || (rva >= 0x317290 && rva < 0x317e7b) || (rva >= 0x3181aa && rva < 0x318250) ||
           (rva >= 0x3189d6 && rva < 0x318f6c) || (rva >= 0x31a800 && rva < 0x31aac0);
}

// Block types the terrain makes; any other is a structure's.
bool terrainType(int t) {
    return t == 0 || t == 2 || t == 3 || t == 4 || t == 5 || t == 6 || t == 10 || t == 13 || t == 14 || t == 17 || t == 18 || t == 19;
}

wg::Block blockOf(const wg::TerrainColumn &c, int32_t z) {
    return z >= c.base && z < c.top() ? c.blocks[(size_t)(z - c.base)] : wg::EMPTY_BLOCK;
}

bool sameStoredColumn(const wg::TerrainColumn &a, const wg::TerrainColumn &b) {
    return (a.base == b.base || (a.empty() && b.empty())) && a.blocks == b.blocks;
}

// Block tallies of a zone against the game's.
struct BlockTally {
    long long columns = 0, exact = 0, blocks = 0, same = 0, structGame = 0, structMine = 0, structSame = 0;
    void add(const wg::TerrainColumn &game, const wg::TerrainColumn &mine) {
        columns++;
        const int32_t lo = std::min(game.base, mine.base), hi = std::max(game.top(), mine.top());
        bool all = true;
        for (int32_t z = lo; z < hi; z++) {
            const wg::Block a = blockOf(game, z), b = blockOf(mine, z);
            const bool eq = a == b;
            blocks++, same += eq, all = all && eq;
            const bool gs = !terrainType(a.type), ms = !terrainType(b.type);
            structGame += gs, structMine += ms, structSame += gs && eq;
        }
        exact += all;
    }
};

// Multisets of record keys, game against ours.
template <class Key> struct Matched {
    std::multiset<Key> game, mine;
    int matched() const {
        std::multiset<Key> left = mine;
        int n = 0;
        for (const Key &k : game) {
            auto it = left.find(k);
            if (it != left.end())
                left.erase(it), n++;
        }
        return n;
    }
};

uint32_t bitsOf(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

struct Totals {
    int zones = 0;
    BlockTally blocks;
    int objectsGame = 0, objectsMine = 0, objectsSame = 0, unspecifiedTurns = 0, rawTurnDifferences = 0;
    int modelsGame = 0, modelsMine = 0, modelsSame = 0, modelsFloats = 0;
    int itemsGame = 0, itemsMine = 0, itemsSame = 0, itemsAngle = 0;
    std::map<int, std::array<int, 3>> objectsByType; // game, ours, same
    std::map<int, std::array<int, 4>> modelsByType;  // game, ours, same place, same floats
    std::map<std::pair<int, int>, std::array<int, 3>> itemsByKind;
};

// Compares one generated zone (objects, models, items) against the game's records.
void compareRecords(const GameZone &g, const std::vector<wg::StaticObject> &objects, const std::vector<wg::Model> &models,
                    const std::vector<wg::GroundItem> &items, Totals &t, bool verbose, int &shown) {
    // Objects.
    std::vector<objectoracle::Placement> capturedObjects;
    if (auto it = g.lists.find(wg::zonelist::OBJECTS); it != g.lists.end())
        for (const auto &record : it->second) {
            const auto placement = objectvisual::placement(record.data());
            ++t.objectsByType[placement.type][0];
            capturedObjects.push_back(placement);
        }
    for (const auto &object : objects)
        ++t.objectsByType[object.type][1];
    t.objectsGame += (int)capturedObjects.size();
    t.objectsMine += (int)objects.size();
    const auto compared = objectoracle::compare(std::move(capturedObjects), objects);
    t.objectsSame += (int)compared.matches.size();
    for (const auto &match : compared.matches) {
        ++t.objectsByType[match.captured.type][2];
        t.unspecifiedTurns += !match.turnInitialized;
        t.rawTurnDifferences += match.captured.turn != match.generated.turn;
    }
    if (verbose) {
        const auto print = [&](const char *label, const objectoracle::Placement &key) {
            if (shown++ < 25)
                std::printf("  zone %d %d %s: object %d at %.3f %.3f %.3f turn %u\n", g.zx, g.zy, label, key.type,
                            key.position[0] / 65536.0, key.position[1] / 65536.0, key.position[2] / 65536.0, (unsigned)key.turn);
        };
        for (const auto &key : compared.unmatchedCaptured)
            print("game only", key);
        for (const auto &key : compared.unmatchedGenerated)
            print("engine only", key);
    }

    // Models: type and position, then the two floats.
    using MKey = std::tuple<int, int64_t, int64_t, int64_t>;
    std::multimap<MKey, std::pair<float, float>> gm, mm;
    if (auto it = g.lists.find(wg::zonelist::MODELS); it != g.lists.end())
        for (const auto &r : it->second)
            gm.insert({{field<int32_t>(r, 0), field<int64_t>(r, 8), field<int64_t>(r, 0x10), field<int64_t>(r, 0x18)},
                       {field<float>(r, 0x20), field<float>(r, 0x24)}});
    for (const auto &m : models)
        mm.insert({{m.type, m.x, m.y, m.z}, {m.size, m.yaw}});
    for (auto &[k, v] : gm) {
        t.modelsByType[std::get<0>(k)][0]++;
        auto range = mm.equal_range(k);
        if (range.first == range.second) {
            if (verbose && shown++ < 25)
                std::printf("  zone %d %d game only: model %d at %.3f %.3f %.3f\n", g.zx, g.zy, std::get<0>(k), std::get<1>(k) / 65536.0,
                            std::get<2>(k) / 65536.0, std::get<3>(k) / 65536.0);
            continue;
        }
        auto best = range.first;
        for (auto i = range.first; i != range.second; ++i)
            if (i->second == v)
                best = i;
        t.modelsSame++, t.modelsByType[std::get<0>(k)][2]++;
        if (best->second == v)
            t.modelsFloats++, t.modelsByType[std::get<0>(k)][3]++;
        mm.erase(best);
    }
    for (const auto &m : models)
        t.modelsByType[m.type][1]++;
    if (verbose)
        for (auto &[k, v] : mm)
            if (shown++ < 25)
                std::printf("  zone %d %d ours only: model %d at %.3f %.3f %.3f\n", g.zx, g.zy, std::get<0>(k), std::get<1>(k) / 65536.0,
                            std::get<2>(k) / 65536.0, std::get<3>(k) / 65536.0);
    t.modelsGame += (int)gm.size(), t.modelsMine += (int)models.size();

    // Items: category, id and position, then the angle.
    using IKey = std::tuple<int, int, int64_t, int64_t, int64_t>;
    std::multimap<IKey, float> gi, mi;
    if (auto it = g.lists.find(wg::zonelist::ITEMS); it != g.lists.end())
        for (const auto &r : it->second)
            gi.insert({{r[0], field<int32_t>(r, 4), field<int64_t>(r, 0xa0), field<int64_t>(r, 0xa8), field<int64_t>(r, 0xb0)},
                       field<float>(r, 0xc0)});
    for (const auto &i : items)
        mi.insert({{i.item.category, i.item.id, i.x, i.y, i.z}, i.angle});
    for (auto &[k, v] : gi) {
        auto &kind = t.itemsByKind[{std::get<0>(k), std::get<1>(k)}];
        kind[0]++;
        auto f = mi.find(k);
        if (f == mi.end()) {
            if (verbose && shown++ < 25)
                std::printf("  zone %d %d game only: item %d/%d at %.3f %.3f %.3f\n", g.zx, g.zy, std::get<0>(k), std::get<1>(k),
                            std::get<2>(k) / 65536.0, std::get<3>(k) / 65536.0, std::get<4>(k) / 65536.0);
            continue;
        }
        t.itemsSame++, kind[2]++;
        t.itemsAngle += bitsOf(f->second) == bitsOf(v);
        mi.erase(f);
    }
    for (const auto &i : items)
        t.itemsByKind[{i.item.category, i.item.id}][1]++;
    if (verbose)
        for (auto &[k, v] : mi)
            if (shown++ < 25)
                std::printf("  zone %d %d ours only: item %d/%d at %.3f %.3f %.3f\n", g.zx, g.zy, std::get<0>(k), std::get<1>(k),
                            std::get<2>(k) / 65536.0, std::get<3>(k) / 65536.0, std::get<4>(k) / 65536.0);
    t.itemsGame += (int)gi.size(), t.itemsMine += (int)items.size();
}

// The light and emitter lists (contents file §C2.1): the game's +0x90 against the rule applied to
// its own +0x18, and against ours; our emitters against the rule on the game's +0x18 (the game's
// +0xa8 is not in these .rec files). Compared: type, position and +0x20..+0x38 but +0x28 (not
// modelled); the uncopied +0x04 and +0x3c are left out.
struct ModelListTally {
    int zones = 0, rulesWhole = 0, oursWhole = 0, emittersWhole = 0;
    long long lights = 0, emitters = 0;
};
bool sameModel(const std::vector<uint8_t> &game, const wg::Model &m) {
    return field<int32_t>(game, 0) == m.type && field<int64_t>(game, 8) == m.x && field<int64_t>(game, 0x10) == m.y &&
           field<int64_t>(game, 0x18) == m.z && bitsOf(field<float>(game, 0x20)) == bitsOf(m.size) &&
           bitsOf(field<float>(game, 0x24)) == bitsOf(m.yaw) && bitsOf(field<float>(game, 0x2c)) == bitsOf(m.r) &&
           bitsOf(field<float>(game, 0x30)) == bitsOf(m.g) && bitsOf(field<float>(game, 0x34)) == bitsOf(m.b) &&
           field<int32_t>(game, 0x38) == m.flags;
}
void checkModelLists(const GameZone &g, const wg::ZoneContents *ours, ModelListTally &t) {
    static const std::vector<std::vector<uint8_t>> none;
    auto list = [&](uint32_t offset) -> const std::vector<std::vector<uint8_t>> & {
        auto it = g.lists.find(offset);
        return it == g.lists.end() ? none : it->second;
    };
    constexpr uint32_t LIGHTS = 0x90;
    const auto &models = list(wg::zonelist::MODELS), &lights = list(LIGHTS);
    // The rule on the game's own models, as raw records.
    std::vector<wg::Model> gameModels;
    for (const auto &r : models) {
        wg::Model m;
        m.type = field<int32_t>(r, 0), m.x = field<int64_t>(r, 8), m.y = field<int64_t>(r, 0x10), m.z = field<int64_t>(r, 0x18);
        m.size = field<float>(r, 0x20), m.yaw = field<float>(r, 0x24);
        m.r = field<float>(r, 0x2c), m.g = field<float>(r, 0x30), m.b = field<float>(r, 0x34), m.flags = field<int32_t>(r, 0x38);
        gameModels.push_back(m);
    }
    const wg::ModelLists rule = wg::modelLists(gameModels);
    auto whole = [&](const std::vector<wg::Model> &mine) {
        if (mine.size() != lights.size())
            return false;
        for (size_t i = 0; i < mine.size(); i++)
            if (!sameModel(lights[i], mine[i]))
                return false;
        return true;
    };
    t.zones++;
    t.lights += (long long)lights.size();
    t.rulesWhole += whole(rule.lights);
    if (ours) {
        // All initialized side-torch metadata is now independently specified.
        // Compare the full list in order; there is no torch exception.
        bool same = whole(ours->modelLights);
        if (!same && std::getenv("WS_LIGHTS")) { // debugging: a zone whose light list is not ours
            std::printf("  zone %d %d lights: game %zu, ours %zu\n", g.zx, g.zy, lights.size(), ours->modelLights.size());
            for (const auto &r : lights)
                std::printf("    game type %d flags %d at %lld %lld\n", field<int32_t>(r, 0), field<int32_t>(r, 0x38),
                            (long long)field<int64_t>(r, 8), (long long)field<int64_t>(r, 0x10));
            for (const wg::Model &m : ours->modelLights)
                std::printf("    ours type %d flags %d at %lld %lld\n", m.type, m.flags, (long long)m.x, (long long)m.y);
        }
        t.oursWhole += same;
        bool emittersSame = ours->emitters.size() == rule.emitters.size();
        for (size_t i = 0; emittersSame && i < rule.emitters.size(); i++) {
            const wg::Model &a = rule.emitters[i], &b = ours->emitters[i];
            emittersSame = a.type == b.type && a.x == b.x && a.y == b.y && a.z == b.z;
        }
        t.emittersWhole += emittersSame;
        t.emitters += (long long)ours->emitters.size();
    }
}

std::vector<fs::path> zoneFiles(const fs::path &dir, int maxZones) {
    std::vector<fs::path> files;
    if (!fs::exists(dir))
        return files;
    for (auto &e : fs::directory_iterator(dir)) {
        int ax, ay;
        if (e.path().extension() == ".cwz" && ExtractedWorld::zoneCoordinates(e.path(), ax, ay))
            files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());
    if ((int)files.size() > maxZones)
        files.resize((size_t)maxZones);
    return files;
}

void printTotals(const char *who, const Totals &t) {
    std::printf("  %s: blocks %lld of %lld the same, columns %lld of %lld exact; structure blocks game %lld, %s %lld, the same %lld\n",
                who, t.blocks.same, t.blocks.blocks, t.blocks.exact, t.blocks.columns, t.blocks.structGame, who, t.blocks.structMine,
                t.blocks.structSame);
    std::printf("  %s: objects game %d, %s %d, the same %d; models game %d, %s %d, same place %d, same floats %d; items game %d, %s %d, "
                "the same %d, same angle %d\n",
                who, t.objectsGame, who, t.objectsMine, t.objectsSame, t.modelsGame, who, t.modelsMine, t.modelsSame, t.modelsFloats,
                t.itemsGame, who, t.itemsMine, t.itemsSame, t.itemsAngle);
    std::printf("  %s: objects with unwritten native turns %d; raw turn differences %d\n", who, t.unspecifiedTurns,
                t.rawTurnDifferences);
}

int zonesOracle(const fs::path &dir, int maxZones, bool verbose, const wg::CreatureTables *creatures = nullptr) {
    const bool strictStorage = creatures && std::getenv("WS_STORED_EXACT");
    const std::vector<fs::path> files = zoneFiles(dir, maxZones);
    if (files.empty()) {
        WS_CHECK(!strictStorage);
        return 0;
    }
    const std::string name = dir.filename().string();
    const int seed = std::atoi(name.substr(name.find_last_of('_') + 1).c_str());
    int day = -1;
    if (std::ifstream d(dir / "day.txt"); d)
        d >> day;
    const wg::WorldValues world = wg::worldValues(seed);
    wg::StructureOptions options;
    options.recordDraws = true;
    options.hooks.planner = wg::plannerFacts;
    Totals mine;
    ModelListTally modelLists;
    int traces = 0, tracesToCreature = 0, shown = 0, markOnlyColumns = 0, otherColumns = 0;
    size_t storedDifferences = 0;
    long long prefixTotal = 0, creatureTotal = 0;
    double newSeconds = 0;
    std::vector<std::string> notes;
    std::map<int, std::array<int, 2>> columnsByType; // by the cell object: columns, differing (not just marks)
    for (const fs::path &file : files) {
        GameZone g;
        if (!readZone(file, g)) {
            if (strictStorage) {
                std::printf("Could not read stored columns from %s\n", file.filename().string().c_str());
                WS_CHECK(false);
            }
            continue;
        }
        if (const char *find = std::getenv("WS_FINDTYPE")) { // debugging: the zones holding a cell object
            for (int32_t cx = 4 * g.zx; cx < 4 * g.zx + 4; cx++)
                for (int32_t cy = 4 * g.zy; cy < 4 * g.zy + 4; cy++)
                    if (wg::cellObjectsFor(world).type(cx, cy) == std::atoi(find))
                        std::printf("  type %s at cell %d %d (zone %d %d)\n", find, cx, cy, g.zx, g.zy);
            continue;
        }
        if (const char *only = std::getenv("WS_ONLY")) { // debugging: one zone
            int ox = 0, oy = 0;
            std::sscanf(only, "%d %d", &ox, &oy);
            if (ox != g.zx || oy != g.zy)
                continue;
        }
        mine.zones++;
        auto t0 = Clock::now();
        wg::ZoneContents z = creatures ? wg::generateZoneWithCreatures(world, *creatures, day, g.zx, g.zy, true)
                                       : wg::generateZone(world, g.zx, g.zy, options);
        newSeconds += seconds(t0);
        // Fresh generation and daily refresh are distinct fixture phases. Only genr contains
        // the refresh pass; the other sets retain the objects of fresh generation.
        if (day >= 0) {
            if (creatures && name == "genr_seed_123")
                wg::refreshZoneCreatures(z, world, *creatures, day);
            else if (!creatures)
                wg::refreshZone(z, world, day);
        }
        for (int i = 0; i < wg::ZONE_COLUMNS; i++) {
            const wg::TerrainColumn &gc = g.columns[(size_t)i], &oc = z.terrain.columns[(size_t)i];
            if (strictStorage && !sameStoredColumn(gc, oc) && storedDifferences++ < 8)
                std::printf("  zone %d %d stored column %d %d: base %d/%d, count %zu/%zu\n",
                            g.zx, g.zy, i / wg::ZONE_SIZE, i % wg::ZONE_SIZE, gc.base, oc.base,
                            gc.blocks.size(), oc.blocks.size());
            const long long before = mine.blocks.exact;
            mine.blocks.add(gc, oc);
            static int shownColumns = 0;
            static const int showLimit = std::getenv("WS_SHOW") ? std::atoi(std::getenv("WS_SHOW")) : 0;
            if (const char *col = std::getenv("WS_COLUMN")) { // debugging: one column, game and new, block by block
                int px = 0, py = 0;
                std::sscanf(col, "%d %d", &px, &py);
                if (px == g.zx * 64 + i / 64 && py == g.zy * 64 + i % 64)
                    std::printf("    game base %d top %d, new base %d top %d\n", gc.base, gc.top(), oc.base, oc.top());
                if (px == g.zx * 64 + i / 64 && py == g.zy * 64 + i % 64)
                    for (int32_t zz = std::min(gc.base, oc.base); zz < std::max(gc.top(), oc.top()); zz++) {
                        const wg::Block a = blockOf(gc, zz), b = blockOf(oc, zz);
                        std::printf("    z %d: game %d %d %d %d %d %d, new %d %d %d %d %d %d%s\n", zz, a.r, a.g, a.b, a.glow, a.type, a.flags,
                                    b.r, b.g, b.b, b.glow, b.type, b.flags, a == b ? "" : "  *");
                    }
            }
            bool onlyMarks = true; // differing only in flag 0x40 (the blocks inside static objects)
            for (int32_t zz = std::min(gc.base, oc.base); zz < std::max(gc.top(), oc.top()) && onlyMarks; zz++) {
                wg::Block a = blockOf(gc, zz), b = blockOf(oc, zz);
                if (a == b)
                    continue;
                if ((a.flags ^ b.flags) & 0x40) {
                    // A marked block may also be a stored air block that is otherwise not kept.
                    a.flags &= (uint8_t)~0x40, b.flags &= (uint8_t)~0x40;
                    if (a.type == 0 && a.flags == 0)
                        a = wg::EMPTY_BLOCK;
                    if (b.type == 0 && b.flags == 0)
                        b = wg::EMPTY_BLOCK;
                }
                onlyMarks = a == b;
            }
            if (mine.blocks.exact == before)
                (onlyMarks ? markOnlyColumns : otherColumns)++;
            {
                const int32_t x = g.zx * 64 + i / 64, y = g.zy * 64 + i % 64;
                auto &tally = columnsByType[wg::cellObjectsFor(world).type(x >> 4, y >> 4)];
                tally[0]++;
                tally[1] += mine.blocks.exact == before && !onlyMarks;
            }
            if (mine.blocks.exact == before && !onlyMarks && shownColumns++ < showLimit) {
                const int32_t x = g.zx * 64 + i / 64, y = g.zy * 64 + i % 64;
                std::printf("  column %d %d (cell %d %d, object %d) differs\n", x, y, x >> 4, y >> 4,
                            wg::cellObjectsFor(world).type(x >> 4, y >> 4));
                for (int32_t zz = std::min(gc.base, oc.base); zz < std::max(gc.top(), oc.top()); zz++) {
                    const wg::Block a = blockOf(gc, zz), b = blockOf(oc, zz);
                    if (a != b)
                        std::printf("    z %d: game %d %d %d %d %d %d, new %d %d %d %d %d %d\n", zz, a.r, a.g, a.b, a.glow, a.type, a.flags,
                                    b.r, b.g, b.b, b.glow, b.type, b.flags);
                }
            }
        }
        if (const char *sl = std::getenv("WS_SLICE")) { // debugging: a slice along x at y, z1 down to z0 (game | new)
            int sx = 0, sy = 0, z0 = 0, z1 = 0;
            std::sscanf(sl, "%d %d %d %d", &sx, &sy, &z0, &z1);
            if (sx >> 6 == g.zx && sy >> 6 == g.zy) {
                auto code = [](const wg::Block &b) {
                    if (b.type == 0)
                        return (b.flags & 2) ? '.' : ' ';
                    const char c = (char)(b.type < 10 ? '0' + b.type : 'a' + (b.type - 10) % 26);
                    return (b.flags & 2) ? c : (char)(c == '6' ? '#' : '+');
                };
                for (int32_t zz = z1; zz >= z0; zz--) {
                    std::string a, b;
                    for (int32_t x = sx; x < sx + 16; x++) {
                        const int k = wg::ZoneTerrain::index(x - g.zx * 64, sy - g.zy * 64);
                        a += code(blockOf(g.columns[(size_t)k], zz));
                        b += code(blockOf(z.terrain.columns[(size_t)k], zz));
                    }
                    std::printf("  %4d %s | %s\n", zz, a.c_str(), b.c_str());
                }
            }
        }
        compareRecords(g, z.objects, z.models, z.items, mine, verbose, shown);
        checkModelLists(g, &z, modelLists);
        // This prefix belongs to fresh generation. genr captures only a partial
        // daily refresh: its trace cannot be compared to the fresh A/B prefix.
        // Full model/block comparisons above still apply to that snapshot set.
        if (!g.trace.empty() && name != "genr_seed_123") {
            traces++;
            size_t firstCreature = g.trace.size();
            for (size_t k = 0; k < g.trace.size(); k++)
                if (creatureCaller(g.trace[k].first)) {
                    firstCreature = k;
                    break;
                }
            // A creature's spot is taken just before its own draws: that draw is part C's too.
            while (firstCreature > 0 && firstCreature < g.trace.size() &&
                   (g.trace[firstCreature - 1].first == 0x31bf98 || g.trace[firstCreature - 1].first == 0x31bd43))
                firstCreature--;
            // Step 8, the region role's things, runs between the plants and the cell pass. Unless
            // the role is 6-13 (part B's role item), its draws are part C's even where they come
            // from the spot routine.
            if (!(z.planner.role >= 6 && z.planner.role <= 13)) {
                size_t lastPlant = 0;
                for (size_t k = 0; k < firstCreature; k++)
                    if (g.trace[k].first >= 0x319000 && g.trace[k].first < 0x31a800)
                        lastPlant = k + 1;
                if (lastPlant > 0 && lastPlant < firstCreature && !(g.trace[lastPlant].first >= 0x3171a2 && g.trace[lastPlant].first < 0x318f6c))
                    firstCreature = lastPlant;
            }
            size_t same = 0;
            while (same < g.trace.size() && same < z.terrain.draws.size() && g.trace[same].second == z.terrain.draws[same])
                same++;
            const bool ok = same >= firstCreature;
            if (std::getenv("WS_TRACE") && !ok) // debugging: the draws around the first difference
                for (size_t k = same > 12 ? same - 12 : 0; k < same + 6 && k < g.trace.size(); k++)
                    std::printf("    draw %zu: game 0x%x %d, new %d%s\n", k, g.trace[k].first, g.trace[k].second,
                                k < z.terrain.draws.size() ? z.terrain.draws[k] : -1, k == same ? "  <-" : "");
            tracesToCreature += ok;
            prefixTotal += (long long)std::min(same, firstCreature), creatureTotal += (long long)firstCreature;
            char line[200];
            std::snprintf(line, sizeof line, "zone %d %d (place %d): %zu of %zu draws before the first creature draw match%s", g.zx, g.zy,
                          z.place.place.kind, std::min(same, firstCreature), firstCreature,
                          ok ? "" : (same < g.trace.size() ? (" (first difference at a 0x" + [&] {
                              char b[16];
                              std::snprintf(b, sizeof b, "%x", g.trace[same].first);
                              return std::string(b);
                          }() + " draw)").c_str()
                                                             : ""));
            notes.push_back(line);
        }
    }
    std::printf("wgstructures zones %d (%s, day %d%s):\n", mine.zones, name.c_str(), day,
                creatures ? ", full creature/object generation" : ", structure-only");
    printTotals("new", mine);
    if (strictStorage) {
        std::printf("  exact stored spans: %lld checked, %zu differ\n", mine.blocks.columns, storedDifferences);
        WS_CHECK(mine.zones > 0 && storedDifferences == 0);
    }
    if (name == "genr_seed_123")
        std::printf("  refresh RNG: partial capture; fresh-generation prefix check not applicable\n");
    std::printf("  new: of the differing columns, %d differ only in the flag 0x40 of blocks inside static objects, %d otherwise\n",
                markOnlyColumns, otherColumns);
    std::printf("  rand(): %d traces; %d zones match up to their first creature draw (%lld of %lld draws)\n", traces, tracesToCreature,
                prefixTotal, creatureTotal);
    std::printf("  light lists (+0x90, %lld records): the rule on the game's models whole in %d of %d zones, ours in %d (complete lists); emitters "
                "(+0xa8, %lld) as the rule on the game's models in %d\n",
                modelLists.lights, modelLists.rulesWhole, modelLists.zones, modelLists.oursWhole, modelLists.emitters,
                modelLists.emittersWhole);
    if (verbose) {
        for (auto &[k, n] : columnsByType)
            if (n[1] != 0 || std::getenv("WS_TYPES"))
                std::printf("  columns of cell object %d: %d, %d differ beyond the marks\n", k, n[0], n[1]);
        for (auto &[k, n] : mine.objectsByType)
            if (n[0] != n[2] || n[1] != n[2])
                std::printf("  object type %d: game %d, new %d, the same %d\n", k, n[0], n[1], n[2]);
        for (auto &[k, n] : mine.modelsByType)
            if (n[0] != n[3] || n[1] != n[2])
                std::printf("  model type %d: game %d, new %d, same place %d, same floats %d\n", k, n[0], n[1], n[2], n[3]);
        for (auto &[k, n] : mine.itemsByKind)
            if (n[0] != n[2] || n[1] != n[2])
                std::printf("  item %d/%d: game %d, new %d, the same %d\n", k.first, k.second, n[0], n[1], n[2]);
        for (const std::string &s : notes)
            if (s.find("(first difference") != std::string::npos)
                std::printf("  %s\n", s.c_str());
        std::printf("  time: new %.3f s for %d zones (%.1f ms a zone)\n", newSeconds, mine.zones, 1000.0 * newSeconds / std::max(mine.zones, 1));
    }
    WS_CHECK(traces == 0 || tracesToCreature == traces);
    WS_CHECK(mine.blocks.structGame == mine.blocks.structSame && mine.blocks.structMine == mine.blocks.structGame);
    WS_CHECK(mine.modelsSame == mine.modelsGame && mine.modelsMine == mine.modelsGame);
    if (creatures)
        WS_CHECK(mine.blocks.same == mine.blocks.blocks && mine.blocks.exact == mine.blocks.columns);
    WS_CHECK(modelLists.rulesWhole == modelLists.zones && modelLists.oursWhole == modelLists.zones &&
             modelLists.emittersWhole == modelLists.zones);
    return mine.zones;
}

// The blocks of the zones saved while playing (Extract/seed_123, lit, made on day 7): every block
// but the light of open ones (the red byte of air and water, which the light pass sets), by the
// cell object of the column.
void worldOracle(const fs::path &dir, int maxZones, bool verbose) {
    std::vector<fs::path> files;
    if (!fs::exists(dir))
        return;
    for (auto &e : fs::directory_iterator(dir)) {
        int ax, ay;
        if (e.path().extension() != ".cwz" || !ExtractedWorld::zoneCoordinates(e.path(), ax, ay))
            continue;
        if (std::abs((long long)ax) > 100000 || std::abs((long long)ay) > 100000)
            continue; // not vanilla
        files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());
    // Spread the sample over the world when it is smaller than the set.
    if ((int)files.size() > maxZones) {
        std::vector<fs::path> sample;
        for (int i = 0; i < maxZones; i++)
            sample.push_back(files[(size_t)i * files.size() / (size_t)maxZones]);
        files = sample;
    }
    const std::string name = dir.filename().string();
    const wg::WorldValues world = wg::worldValues(std::atoi(name.substr(name.find_last_of('_') + 1).c_str()));
    wg::StructureOptions options;
    options.hooks.planner = wg::plannerFacts;
    std::map<int, std::array<long long, 3>> byType; // columns, columns differing, of them only in marks
    long long columns = 0, exact = 0, structGame = 0, structSame = 0;
    int zones = 0, shown = 0;
    ModelListTally modelLists;
    std::map<std::pair<int, int>, int> firstDiff; // a differing column's first block: game, new (type * 1000 + flags)
    for (const fs::path &file : files) {
        GameZone g;
        if (!readZone(file, g))
            continue;
        zones++;
        const wg::ZoneContents z = wg::generateZone(world, g.zx, g.zy, options);
        checkModelLists(g, &z, modelLists);
        if (const char *mp = std::getenv("WS_MAP")) { // debugging: a layer of a zone: rock in both B, game only G, new only O
            int mx = 0, my = 0, mz = 0;
            std::sscanf(mp, "%d %d %d", &mx, &my, &mz);
            if (mx >> 6 == g.zx && my >> 6 == g.zy)
                for (int ly = 63; ly >= 0; ly--) {
                    std::string line;
                    for (int lx = 0; lx < 64; lx++) {
                        const int k = wg::ZoneTerrain::index(lx, ly);
                        const bool a = blockOf(g.columns[(size_t)k], mz).type == 6, b = blockOf(z.terrain.columns[(size_t)k], mz).type == 6;
                        line += a && b ? 'B' : a ? 'G' : b ? 'O' : '.';
                    }
                    std::printf("  %5d %s\n", g.zy * 64 + ly, line.c_str());
                }
        }
        for (int i = 0; i < wg::ZONE_COLUMNS; i++) {
            const wg::TerrainColumn &gc = g.columns[(size_t)i], &oc = z.terrain.columns[(size_t)i];
            const int32_t x = g.zx * 64 + i / 64, y = g.zy * 64 + i % 64;
            auto &t = byType[wg::cellObjectsFor(world).type(x >> 4, y >> 4)];
            t[0]++, columns++;
            bool same = true, onlyMarks = true;
            for (int32_t zz = std::min(gc.base, oc.base); zz < std::max(gc.top(), oc.top()); zz++) {
                wg::Block a = blockOf(gc, zz), b = blockOf(oc, zz);
                if (!terrainType(a.type))
                    structGame++, structSame += a == b;
                // The light (the red of open blocks), and the green and blue of saved water (open
                // point of the terrain stage), are not the generator's.
                if (wg::isOpen(a) && wg::isOpen(b))
                    a.r = b.r = 0;
                if (a.type == wg::blocktype::WATER && b.type == wg::blocktype::WATER)
                    a.g = b.g, a.b = b.b;
                if (a == b)
                    continue;
                same = false;
                if ((a.flags ^ b.flags) & 0x40) {
                    a.flags &= (uint8_t)~0x40, b.flags &= (uint8_t)~0x40;
                    if (a.type == 0 && a.flags == 0)
                        a = wg::EMPTY_BLOCK;
                    if (b.type == 0 && b.flags == 0)
                        b = wg::EMPTY_BLOCK;
                }
                if (wg::isOpen(a) && wg::isOpen(b))
                    a.r = b.r = 0;
                if (!(a == b) && onlyMarks)
                    firstDiff[{(int)a.type * 1000 + a.flags, (int)b.type * 1000 + b.flags}]++;
                onlyMarks = onlyMarks && a == b;
            }
            exact += same;
            if (!same) {
                t[1]++, t[2] += onlyMarks;
                if (verbose && !onlyMarks && shown++ < (std::getenv("WS_SHOW") ? std::atoi(std::getenv("WS_SHOW")) : 0)) {
                    std::printf("  world column %d %d (cell object %d) differs\n", x, y, wg::cellObjectsFor(world).type(x >> 4, y >> 4));
                    for (int32_t zz = std::min(gc.base, oc.base); zz < std::max(gc.top(), oc.top()); zz++) {
                        const wg::Block a = blockOf(gc, zz), b = blockOf(oc, zz);
                        if (a != b && !(wg::isOpen(a) && wg::isOpen(b) && a.g == b.g && a.b == b.b && a.type == b.type && a.flags == b.flags))
                            std::printf("    z %d: game %d %d %d %d %d %d, new %d %d %d %d %d %d\n", zz, a.r, a.g, a.b, a.glow, a.type,
                                        a.flags, b.r, b.g, b.b, b.glow, b.type, b.flags);
                    }
                }
            }
        }
    }
    std::printf("wgstructures world %s: %d zones, %lld of %lld columns exact but for the light; structure blocks %lld of %lld the same\n",
                name.c_str(), zones, exact, columns, structSame, structGame);
    std::printf("  light lists (+0x90, %lld records): the rule on the game's models whole in %d of %d zones, ours in %d (complete lists); emitters %lld, "
                "as the rule on the game's models in %d\n",
                modelLists.lights, modelLists.rulesWhole, modelLists.zones, modelLists.oursWhole, modelLists.emitters,
                modelLists.emittersWhole);
    WS_CHECK(modelLists.rulesWhole == modelLists.zones);
    if (verbose)
        for (auto &[k, n] : firstDiff)
            if (n >= 100)
                std::printf("  first differing block game %d/%d, new %d/%d (type/flags): %d columns\n", k.first / 1000, k.first % 1000,
                            k.second / 1000, k.second % 1000, n);
    if (verbose)
        for (auto &[k, n] : byType)
            if (n[1] != n[2])
                std::printf("  cell object %d: %lld columns, %lld differ (%lld only in the marks of objects)\n", k, n[0], n[1], n[2]);
}

fs::path generatedZonesDir(const fs::path &game) {
    if (const char *d = std::getenv("CW_ZONES_DIR"); d && *d)
        return d;
    return game / "Extract" / "gen_seed_123";
}

} // namespace

// The fast subset in the default test run.
void hangingConeChecks() {
    const auto world = wg::worldValues(123);
    wg::CellObjects cells(world);
    wg::ZoneTerrain terrain;
    terrain.columns.resize(wg::ZONE_COLUMNS);
    terrain.retainedColours.resize(wg::ZONE_COLUMNS);
    wg::ZoneBuild zone(terrain, world, cells, true);
    auto mason = wg::Mason::atCell(zone, 1, 1);
    mason.hangingCone(8, 8, 16, 4, 9);

    // From the independent cone formula. At local Z=12 the squared radius
    // is about 4.94. The second phase puts these two points on opposite sides.
    const auto inside = zone.read(24, 22, 12);
    WS_CHECK(inside.type == wg::blocktype::ROCK && inside.flags == 0x22);
    WS_CHECK(zone.read(24, 26, 12).flags != 0x22); // No cone write, including below the stored span.
    WS_CHECK(terrain.draws.empty());
}

void plannerDoorChecks() {
    // Independent arithmetic examples from worldgen-parity-gaps.md §10.
    WS_CHECK(wg::cellRandomUnsigned(26966, 0, 0x4e6289) == 0xffffffffu);
    WS_CHECK(wg::cellRandomUnsigned(26966, 0, 0x4e6289) % 3u == 0);
    WS_CHECK(wg::cellRandomUnsigned(26541, 0, 0x71504) % 5u == 0);
    WS_CHECK(wg::cellRandomUnsigned(70386, 0, 0x55260) % 3u == 0);
    WS_CHECK(wg::cellRandomUnsigned(63861 / 3, 0, 0x7b7e8) % 6u == 2);
    const wg::WorldValues world = wg::worldValues(123);
    wg::CellObjects cells(world);
    wg::BuildingRules rules(cells);
    wg::ZoneTerrain terrain;
    wg::ZoneBuild zone(terrain, world, cells, false);
    wg::BuildingBuilder building(zone, rules, 0, 0, 0, 0, 1, {});
    building.roof = 5;
    zone.planner.items = {16};
    zone.place.cell = {0, 0};
    WS_CHECK(building.doorObjectType() == 16);
    zone.place.cell = {1, 0};
    WS_CHECK(building.doorObjectType() == 8); // Native selector low two bits are zero.
    zone.place.cell = {-1, 0};
    WS_CHECK(building.doorObjectType() == 8); // Native roof-five predicate is false.
    zone.place.cell = {157894, 0};
    zone.planner.items = {100, 101, 102, 103, 104};
    WS_CHECK(building.doorObjectType() == 103); // R=-3 indexes as uint32, not positive signed modulo.
    zone.planner.items = {16};
    zone.place.cell = {0, 0};
    building.t = 76;
    WS_CHECK(building.doorObjectType() == 16);
    zone.place.cell = {-1, 0};
    WS_CHECK(building.doorObjectType() == -1);
    building.t = 100;
    WS_CHECK(building.doorObjectType() == 16); // Cell type100 overrides the parity test.
    zone.planner.items.clear();
    building.t = 1;
    WS_CHECK(building.doorObjectType() == 8);
}

void dungeonGuardFloorChecks() {
    const auto world = wg::worldValues(123);
    wg::CellObjects cells(world);
    wg::ZoneTerrain terrain;
    wg::ZoneBuild first(terrain, world, cells, false);
    first.place.cell = {-10, -12};
    first.place.place = wg::cellPlace(first.place.cell.x, first.place.cell.y);
    wg::ZoneBuild second(terrain, world, cells, false);
    second.place.cell = {-12, -18};
    second.place.place = wg::cellPlace(second.place.cell.x, second.place.cell.y);

    // Identical zone coordinates must not make independent builds share the selected floor.
    WS_CHECK(first.dungeonGuardFloor().code == 2);
    WS_CHECK((first.dungeonGuardFloor().cell == wg::Int2{-569, -720}));
    WS_CHECK(second.dungeonGuardFloor().code == 4);
    WS_CHECK((second.dungeonGuardFloor().cell == wg::Int2{-714, -1065}));
    WS_CHECK(first.dungeonGuardFloor().code == 2);

    // The captured seed-123 entrance has one spawn guarding its cell.
    const auto entrance = wg::generateZone(world, -143, -180);
    const auto threshold = entrance.gateThresholds.find({-569, -718});
    WS_CHECK(threshold != entrance.gateThresholds.end() && threshold->second == 1);
}

int worldgenStructureTests() {
    failures = 0;
    hangingConeChecks();
    plannerDoorChecks();
    dungeonGuardFloorChecks();
    const fs::path game = gameDir();
    if (game.empty())
        return 0;
    cellsOracle(game, false, 5000);
    const auto directory = generatedZonesDir(game);
    zonesOracle(directory, 6, false);
    wg::CreatureTables creatures;
    std::string why;
    if (creatures.load(directory.parent_path() / "oracle", &why))
        zonesOracle(directory, 6, false, &creatures);
    else
        std::printf("SKIP full generated terrain comparison: %s\n", why.c_str());
    return failures;
}

// Classify only cells whose zone coordinates are present. This is an engine-side
// target selection report for a native capture, not a comparison of captured bytes.
int worldgenDungeonCoverage(const fs::path &directory, int seed) {
    if (!fs::is_directory(directory)) {
        std::puts("No zone directory for dungeon coverage");
        return 1;
    }
    const auto world = wg::worldValues(seed);
    wg::CellObjects cells(world);
    wg::BuildingRules rules(cells);
    using Label = std::tuple<int, int, int, int, int>; // place, structure, castle kind, detail, style
    struct Coverage {
        int cells = 0;
        std::tuple<int, int, int, int> representative; // zone x/y and cell x/y
    };
    std::map<Label, Coverage> coverage;
    int zoneFiles = 0, dungeonZones = 0;
    for (const auto &entry : fs::directory_iterator(directory)) {
        int zx, zy;
        if (!entry.is_regular_file() || !ExtractedWorld::zoneCoordinates(entry.path(), zx, zy) ||
            zx < -100000 || zx > 100000 || zy < -100000 || zy > 100000)
            continue;
        ++zoneFiles;
        const int px = wg::floorDiv(zx, wg::PLACE_SHIFT - wg::ZONE_SHIFT);
        const int py = wg::floorDiv(zy, wg::PLACE_SHIFT - wg::ZONE_SHIFT);
        const auto place = wg::cellPlace(px, py);
        const int structure = wg::bigStructure(px, py, place.kind, place.big);
        if (!structure)
            continue;
        bool hasCastleCell = false;
        for (int dy = 0; dy < 4; ++dy) {
            for (int dx = 0; dx < 4; ++dx) {
                const int cx = zx * 4 + dx, cy = zy * 4 + dy;
                const auto castle = rules.castleCell(cx, cy);
                if (!castle.kind)
                    continue;
                hasCastleCell = true;
                auto &group = coverage[{place.kind, structure, castle.kind, castle.detail, castle.style}];
                const auto coordinate = std::tuple(zx, zy, cx, cy);
                if (group.cells == 0 || coordinate < group.representative)
                    group.representative = coordinate;
                ++group.cells;
            }
        }
        dungeonZones += hasCastleCell;
    }

    std::printf("Dungeon capture candidates for seed %d: %d zone files, %d with predicted castle cells\n", seed, zoneFiles, dungeonZones);
    bool detail11 = false, detail12 = false, style1 = false;
    for (const auto &[label, group] : coverage) {
        const auto [place, structure, kind, detail, style] = label;
        const auto [zx, zy, cx, cy] = group.representative;
        std::printf("  place %d structure %d castle %d detail %d style %d: %d cells; zone %d %d, cell %d %d, top %d, floor %d\n",
                    place, structure, kind, detail, style, group.cells, zx, zy, cx, cy,
                    rules.castleBlockTop(cx, cy), rules.castleFloor(cx, cy));
        detail11 |= structure == 42 && detail == 11;
        detail12 |= structure == 44 && detail == 12;
        style1 |= style == 1;
    }
    std::printf("  requested labels present in engine predictions: 42/detail11 %s, 44/detail12 %s, style1 %s\n",
                detail11 ? "yes" : "no", detail12 ? "yes" : "no", style1 ? "yes" : "no");
    return zoneFiles == 0;
}

namespace {
using dungeoncapture::Point;
struct FloorEdges {
    std::array<Point, 2> neighbors{};
    std::array<const char *, 2> names{};
    size_t count = 0;
};
FloorEdges floorEdges(Point cell) {
    const auto [x, y] = cell;
    const auto lx = wg::posMod(x, 8), ly = wg::posMod(y, 8);
    FloorEdges edges;
    const auto add = [&](Point neighbor, const char *name) {
        edges.neighbors[edges.count] = neighbor;
        edges.names[edges.count++] = name;
    };
    if (lx == 0) add({x - 1, y}, "west");
    if (ly == 0) add({x, y - 1}, "south");
    if (lx == 7) add({x + 1, y}, "east");
    if (ly == 7) add({x, y + 1}, "north");
    return edges;
}

int32_t smoothFloor(int32_t top, int32_t other) {
    return wg::wrapAdd(top, std::max(top, other)) / 2;
}

void reportDungeonFloors(const capturepolicy::GenerationCoordinates &targets,
                         const std::map<Point, dungeoncapture::Decision> &rows) {
    std::puts("Target floors (edge and smoothing derived from queried tops):");
    for (const auto &target : targets) {
        const auto &current = rows.at(target);
        const auto edges = floorEdges(target);
        const auto smoothed = edges.count ? smoothFloor(current.top, rows.at(edges.neighbors[0]).top) : current.top;
        std::printf("  cell %d %d: structure %d, kind/detail/style %u/%u/%u, %s, top %d, smoothed %d, floor %d, delta %lld",
                    target.first, target.second, current.structure, current.castle & 255u, (current.castle >> 8) & 255u,
                    (current.castle >> 16) & 255u, edges.count ? edges.names[0] : "interior", current.top, smoothed,
                    current.floor, int64_t(current.floor) - smoothed);
        if (edges.count == 2)
            std::printf(", alternate %s smoothed %d", edges.names[1], smoothFloor(current.top, rows.at(edges.neighbors[1]).top));
        std::putchar('\n');
    }
}

size_t compareDungeonSpawnCells(const std::string &zone, const dungeoncapture::SpawnCapture &capture,
                                const wg::ZoneSpawns &spawns) {
    const auto orderedCells = [](const std::vector<wg::Int2> &cells) {
        std::vector<captureabi::SpawnCell> ordered;
        for (const auto &cell : cells)
            ordered.push_back({cell.x, cell.y});
        std::sort(ordered.begin(), ordered.end());
        ordered.erase(std::unique(ordered.begin(), ordered.end()), ordered.end());
        return ordered;
    };
    size_t differences = 0;
    auto generated = spawns.points.begin();
    for (const auto &native : capture.records) {
        while (generated != spawns.points.end() && generated->first < native.id) {
            std::printf("%s: extra engine spawn %lld\n", zone.c_str(), static_cast<long long>(generated->first));
            ++generated;
            ++differences;
        }
        if (generated == spawns.points.end() || generated->first != native.id) {
            std::printf("%s: missing engine spawn %lld\n", zone.c_str(), static_cast<long long>(native.id));
            ++differences;
            continue;
        }
        const auto &spawn = generated++->second;
        const bool guardMatch = native.cells.guards == orderedCells(spawn.area);
        const bool activationMatch = native.cells.activation == orderedCells(spawn.clearedCells);
        bool keysMatch = true;
        if (capture.keysRecorded) {
            std::vector<dungeoncapture::SpawnKey> engineKeys;
            for (const auto &tab : spawn.gear.inventory.tabs)
                for (const auto &stack : tab) {
                    const auto &item = stack.item;
                    if (item.category == dungeoncapture::DungeonKeyCategory)
                        engineKeys.push_back({stack.quantity, item.id, {item.region.x, item.region.y}, item.modifier,
                                              item.realm, item.rarity, item.material});
                }
            const auto keys = capture.keys.find(native.id);
            keysMatch = keys == capture.keys.end() ? engineKeys.empty() : engineKeys == keys->second;
            if (keys != capture.keys.end())
                for (const auto &key : keys->second)
                    std::printf("%s: native key %d, quantity %d, region %d %d, spawn %lld\n", zone.c_str(),
                                key.id, key.quantity, key.region[0], key.region[1], static_cast<long long>(native.id));
        }
        if (!guardMatch || !activationMatch || !keysMatch) {
            std::printf("%s: spawn %lld differs (guard %s, activation %s, key %s)\n", zone.c_str(),
                        static_cast<long long>(native.id), guardMatch ? "match" : "differs",
                        activationMatch ? "match" : "differs",
                        capture.keysRecorded ? (keysMatch ? "match" : "differs") : "unrecorded");
            ++differences;
        }
    }
    for (; generated != spawns.points.end(); ++generated) {
        std::printf("%s: extra engine spawn %lld\n", zone.c_str(), static_cast<long long>(generated->first));
        ++differences;
    }
    return differences;
}
} // namespace

int worldgenDungeonOracle(const fs::path &directory, const fs::path &tablesDirectory) {
    using namespace dungeoncapture;
    const auto read = [](const fs::path &file, size_t limit) {
        std::ifstream input(file, std::ios::binary | std::ios::ate);
        if (!input || input.tellg() < 0 || uint64_t(input.tellg()) > limit)
            return std::string{};
        std::string bytes(size_t(input.tellg()), '\0');
        input.seekg(0);
        if (!input.read(bytes.data(), bytes.size()))
            return std::string{};
        return bytes;
    };
    Plan expected;
    capturepolicy::GenerationCoordinates targets;
    const auto request = read(directory / "targets.txt", capturepolicy::MaximumGenerationRequestBytes);
    const auto marker = read(directory / "complete.txt", CompletedWithCeilings.size());
    const bool withCeilings = marker == CompletedWithCeilings;
    const bool withGateThresholds = withCeilings || marker == CompletedWithGateThresholds;
    const bool withSpawnKeys = withGateThresholds || marker == CompletedWithSpawnKeys;
    const bool withSpawnCells = withSpawnKeys || marker == CompletedWithSpawnCells;
    if ((!withSpawnCells && marker != Completed) || withCeilings != wantsCeilings(request) ||
        !plan(request, capturepolicy::RetainedGenerationBudget::MaximumAttempts, expected) ||
        !capturepolicy::parseGenerationRequest(targetText(request), MaxTargets, targets)) {
        std::puts("Dungeon oracle requires a completed capture with a valid target list");
        return 1;
    }
    wg::CreatureTables tables;
    std::string error;
    if (!tables.load(tablesDirectory, &error)) {
        std::printf("Dungeon oracle requires owner creature tables: %s\n", error.c_str());
        return 1;
    }

    std::optional<Context> batch;
    std::map<Point, Decision> nativeRows;
    size_t decisionDifferences = 0, columnDifferences = 0;
    size_t guardZones = 0, spawnRecords = 0, spawnDifferences = 0;
    size_t gateThresholds = 0, gateDifferences = 0;
    size_t ceilingRows = 0, domeRows = 0, ceilingDifferences = 0;
    const std::set<Point> targetCells(targets.begin(), targets.end());
    const bool showAllDifferences = std::getenv("CW_DUNGEON_DIFF") != nullptr;
    for (const auto &[coordinate, cells] : expected) {
        const auto [zx, zy] = coordinate;
        const auto stem = "z_" + std::to_string(zx) + "_" + std::to_string(zy);
        const auto bytes = read(directory / (stem + ".cwd"), PrefixSize + 16 * RowSize);
        Zone labels;
        if (!decode(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size(), labels) ||
            labels.zone != coordinate || (batch && *batch != labels.context) || labels.decisions.size() != cells.size()) {
            std::printf("%s: invalid or mismatched dungeon labels\n", stem.c_str());
            return 1;
        }
        if (!batch)
            std::printf("Dungeon capture source: %s\n", labels.context.vanillaCandidate ? "vanilla candidate" : "modified or unclassified");
        batch = labels.context;
        const auto world = wg::worldValues(std::bit_cast<int32_t>(batch->seed));
        if (world.zoneSeed() != batch->derivedSeed || world.sandX() != batch->sandX || world.sandY() != batch->sandY) {
            std::printf("%s: engine world seed or sand context differs from capture\n", stem.c_str());
            return 1;
        }
        wg::CellObjects cellObjects(world);
        wg::BuildingRules rules(cellObjects);
        for (const auto &native : labels.decisions) {
            const auto [cx, cy] = native.cell;
            if (!cells.contains(native.cell)) {
                std::printf("%s: unexpected queried cell %d %d\n", stem.c_str(), cx, cy);
                return 1;
            }
            nativeRows.emplace(native.cell, native);
            const Point place{wg::floorDiv(cx, 6), wg::floorDiv(cy, 6)};
            const auto [px, py] = place;
            const auto context = wg::cellPlace(px, py);
            const auto castle = rules.castleCell(cx, cy);
            const uint32_t packed = uint32_t(castle.kind) | (uint32_t(castle.detail) << 8) |
                                    (uint32_t(castle.style) << 16) | (uint32_t(castle.steps) << 24);
            const Decision engine{native.cell, place, context.big ? Point{px & ~1, py & ~1} : place,
                                  context.big, context.kind, wg::bigStructure(px, py, context.kind, context.big),
                                  rules.type(cx, cy), packed, rules.castleBlockTop(cx, cy), rules.castleFloor(cx, cy)};
            if (native != engine) {
                ++decisionDifferences;
                std::printf("%s: cell %d %d differs (native/engine kind %d/%d structure %d/%d building %d/%d "
                            "castle %08x/%08x top %d/%d floor %d/%d)\n",
                            stem.c_str(), cx, cy, native.kind, engine.kind, native.structure, engine.structure,
                            native.building, engine.building, native.castle, engine.castle, native.top, engine.top,
                            native.floor, engine.floor);
            }
        }
        const auto captured = ExtractedWorld::readStoredZoneColumns(directory / (stem + ".cwz"), error);
        if (captured.size() != wg::ZONE_COLUMNS) {
            std::printf("%s: %s\n", stem.c_str(), error.c_str());
            return 1;
        }
        CeilingRows engineCeilings;
        wg::CastleCeilingObserver observer;
        if (withCeilings) {
            observer = [&](int32_t x, int32_t y, int32_t floor, int lx, int ly, bool dome, int32_t ceiling) {
                if (targetCells.count({x, y}) &&
                    !engineCeilings.emplace(CeilingKey{x, y, floor, lx, ly}, CeilingValue{dome, ceiling}).second)
                    ++ceilingDifferences;
            };
        }
        const auto generated = wg::generateZoneWithCreatures(world, tables, batch->day, zx, zy, false, std::move(observer));
        if (withCeilings) {
            CeilingCapture capture;
            if (!decodeCeilings(read(directory / (stem + ".cdo"), MaxCeilingBytes), capture) ||
                capture.zone != coordinate || capture.seed != batch->seed || capture.day != batch->day) {
                std::printf("%s: missing, invalid or mismatched ceiling capture\n", stem.c_str());
                return 1;
            }
            ceilingRows += capture.rows.size();
            for (const auto &[key, value] : capture.rows) {
                if (!targetCells.count({key[0], key[1]})) {
                    std::printf("%s: ceiling row is outside the explicit targets\n", stem.c_str());
                    return 1;
                }
                domeRows += value.first;
                const auto engine = engineCeilings.find(key);
                if (engine != engineCeilings.end() && engine->second == value)
                    continue;
                if (ceilingDifferences++ < 16 || showAllDifferences) {
                    std::printf("%s: ceiling differs at cell %d %d floor %d column %d %d (Steam %d %d",
                                stem.c_str(), key[0], key[1], key[2], key[3], key[4], int(value.first), value.second);
                    if (engine != engineCeilings.end())
                        std::printf(", engine %d %d)\n", int(engine->second.first), engine->second.second);
                    else
                        std::puts(", missing engine observation)");
                }
            }
            for (const auto &[key, value] : engineCeilings)
                if (!capture.rows.count(key) && (ceilingDifferences++ < 16 || showAllDifferences))
                    std::printf("%s: extra engine ceiling at cell %d %d floor %d column %d %d (%d %d)\n",
                                stem.c_str(), key[0], key[1], key[2], key[3], key[4], int(value.first), value.second);
        }
        if (withGateThresholds) {
            GateCapture capture;
            if (!decodeGateThresholds(read(directory / (stem + ".cgt"), MaxGateCaptureBytes), capture) ||
                capture.zone != captureabi::SpawnCell{zx, zy} || capture.seed != batch->seed || capture.day != batch->day) {
                std::printf("%s: missing, invalid or mismatched gate-threshold capture\n", stem.c_str());
                return 1;
            }
            gateThresholds += capture.thresholds.size();
            for (const auto &[cell, native] : capture.thresholds) {
                const auto engine = generated.gateThresholds.find(cell);
                if (engine == generated.gateThresholds.end()) {
                    std::printf("%s: missing threshold at %d %d (Steam %d)\n", stem.c_str(), cell[0], cell[1], native);
                    ++gateDifferences;
                } else if (engine->second != native) {
                    std::printf("%s: threshold at %d %d differs (Steam %d, engine %d)\n",
                                stem.c_str(), cell[0], cell[1], native, engine->second);
                    ++gateDifferences;
                }
            }
            for (const auto &[cell, engine] : generated.gateThresholds)
                if (!capture.thresholds.count(cell)) {
                    std::printf("%s: extra threshold at %d %d (engine %d)\n", stem.c_str(), cell[0], cell[1], engine);
                    ++gateDifferences;
                }
        }
        if (withSpawnCells && labels.decisions.front().kind == 8) {
            SpawnCapture capture;
            if (!decodeSpawnCells(read(directory / (stem + ".cwg"), MaxSpawnCaptureBytes), capture) ||
                capture.zone != captureabi::SpawnCell{zx, zy} || capture.seed != batch->seed || capture.day != batch->day ||
                capture.keysRecorded != withSpawnKeys) {
                std::printf("%s: missing, invalid or mismatched spawn-cell capture\n", stem.c_str());
                return 1;
            }
            ++guardZones;
            spawnRecords += capture.records.size();
            spawnDifferences += compareDungeonSpawnCells(stem, capture, generated.spawns);
        }
        size_t changed = 0;
        for (size_t i = 0; i < captured.size(); ++i) {
            const auto &native = captured[i];
            const auto &engine = generated.terrain.columns[i];
            if (sameStoredColumn(native, engine))
                continue;
            if (changed++ < 4 || showAllDifferences) {
                std::printf("%s: stored column %zu %zu differs (base %d/%d, count %zu/%zu)\n",
                            stem.c_str(), i / wg::ZONE_SIZE, i % wg::ZONE_SIZE, native.base, engine.base,
                            native.blocks.size(), engine.blocks.size());
                if (native.base != engine.base)
                    continue;
                const size_t common = std::min(native.blocks.size(), engine.blocks.size());
                for (size_t j = 0; j < common; ++j) {
                    const auto &a = native.blocks[j], &b = engine.blocks[j];
                    if (a != b) {
                        std::printf("  z %lld: %u %u %u %u %u %u | %u %u %u %u %u %u\n",
                                    static_cast<long long>(native.base) + static_cast<long long>(j),
                                    unsigned(a.r), unsigned(a.g), unsigned(a.b), unsigned(a.glow), unsigned(a.type), unsigned(a.flags),
                                    unsigned(b.r), unsigned(b.g), unsigned(b.b), unsigned(b.glow), unsigned(b.type), unsigned(b.flags));
                        if (!showAllDifferences)
                            break;
                    }
                }
            }
        }
        columnDifferences += changed;
        std::printf("%s: %zu helper rows, %zu of %zu stored columns match\n",
                    stem.c_str(), labels.decisions.size(), captured.size() - changed, captured.size());
    }
    reportDungeonFloors(targets, nativeRows);
    if (withSpawnCells)
        std::printf("Dungeon spawn cells: %zu kind-8 zones, %zu native spawns, %zu differing spawn records\n",
                    guardZones, spawnRecords, spawnDifferences);
    if (withGateThresholds)
        std::printf("Dungeon gate thresholds: %zu native entries, %zu differences\n", gateThresholds, gateDifferences);
    if (withCeilings)
        std::printf("Dungeon ceilings: %zu native observations, %zu dome columns, %zu differences\n",
                    ceilingRows, domeRows, ceilingDifferences);
    std::printf("Dungeon oracle: %zu zones, %zu differing helper rows, %zu differing stored columns\n",
                expected.size(), decisionDifferences, columnDifferences);
    return decisionDifferences || columnDifferences || spawnDifferences || gateDifferences || ceilingDifferences ||
           (withCeilings && !ceilingRows) || !batch->vanillaCandidate ? 1 : 0;
}

int worldgenDungeonFloorTargets(const fs::path &directory, int seed) {
    const auto world = wg::worldValues(seed);
    wg::CellObjects cells(world);
    wg::BuildingRules rules(cells);
    struct Candidate {
        Point cell;
        int score = 0;
    };
    std::map<std::string, Candidate> picks;
    const auto consider = [&](const std::string &label, Point cell, int score) {
        auto it = picks.find(label);
        if (it == picks.end() || score > it->second.score || (score == it->second.score && cell < it->second.cell))
            picks[label] = {cell, score};
    };
    const auto classify = [&](int cx, int cy) {
        const Point point{cx, cy};
        const auto castle = rules.castleCell(cx, cy);
        const auto edges = floorEdges(point);
        const int32_t top = rules.castleBlockTop(cx, cy);
        const auto neighborTop = [&](size_t i) {
            const auto [x, y] = edges.neighbors[i];
            return rules.castleBlockTop(x, y);
        };
        const auto smoothed = edges.count ? smoothFloor(top, neighborTop(0)) : top;
        const auto floor = rules.castleFloor(cx, cy);
        const int proximity = castle.kind ? 20 : 0;
        if (edges.count == 1 && smoothed != top)
            consider(std::string("edge-") + edges.names[0], point, proximity + std::min(20, std::abs(smoothed - top)));
        if (edges.count == 2) {
            const int separation = std::abs(smoothed - smoothFloor(top, neighborTop(1)));
            if (separation > 5) // Alternate branch plus any room increment cannot explain this result.
                consider(std::string("corner-") + edges.names[0] + "-" + edges.names[1], point, proximity + std::min(20, separation));
        }
        if (!edges.count && castle.kind) {
            if (floor > top)
                consider(floor < 0 ? "increment-negative-floor" : "increment-nonnegative-floor", point, floor - top);
            if (floor == top)
                consider("interior-zero-delta", point, 1);
            if (castle.style == 1)
                consider("style-one-interior", point, castle.kind >= 9 ? 2 : 1);
        }
    };

    std::set<Point> zoneCoordinates;
    std::error_code error;
    if (!fs::is_directory(directory, error))
        return std::printf("No zone directory for floor targets\n"), 1;
    for (const auto &entry : fs::directory_iterator(directory)) {
        int zx, zy;
        if (entry.is_regular_file() && ExtractedWorld::zoneCoordinates(entry.path(), zx, zy) &&
            zx >= -100000 && zx <= 100000 && zy >= -100000 && zy <= 100000)
            zoneCoordinates.emplace(zx, zy);
    }
    for (const auto &[zx, zy] : zoneCoordinates) {
        const int px = wg::floorDiv(zx, wg::PLACE_SHIFT - wg::ZONE_SHIFT);
        const int py = wg::floorDiv(zy, wg::PLACE_SHIFT - wg::ZONE_SHIFT);
        const auto place = wg::cellPlace(px, py);
        if (!wg::bigStructure(px, py, place.kind, place.big))
            continue;
        bool active = false;
        for (int y = 0; y < 4 && !active; ++y)
            for (int x = 0; x < 4 && !active; ++x)
                active = rules.castleCell(zx * 4 + x, zy * 4 + y).kind != 0;
        if (!active)
            continue;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                classify(zx * 4 + x, zy * 4 + y);
    }

    std::printf("Engine-predicted floor targets for seed %d, limited to %zu captured zone coordinates\n", seed, zoneCoordinates.size());
    std::set<Point> selected;
    std::string request;
    dungeoncapture::Plan plan;
    for (const auto &[label, candidate] : picks) {
        const auto [x, y] = candidate.cell;
        if (selected.count(candidate.cell)) {
            std::printf("  %s: cell %d %d\n", label.c_str(), x, y);
            continue;
        }
        auto next = request + std::to_string(x) + ' ' + std::to_string(y) + '\n';
        const bool fits = dungeoncapture::plan(next, capturepolicy::RetainedGenerationBudget::MaximumAttempts, plan);
        std::printf("  %s: cell %d %d%s\n", label.c_str(), x, y, fits ? "" : " (deferred to another process)");
        if (fits) {
            selected.insert(candidate.cell);
            request = std::move(next);
        }
    }
    if (selected.empty()) {
        std::puts("No floor targets found in this coordinate set");
        return 1;
    }
    std::printf("Capture request (%zu cells, %zu zones):\n%s", selected.size(), plan.size(), request.c_str());
    return 0;
}

// Search for the two ceiling detail targets or a room in one requested structure.
// Only the original game can confirm the labels and supply their stored blocks.
int worldgenDungeonTargets(int seed, int requestedStructure) {
    const auto world = wg::worldValues(seed);
    wg::CellObjects cells(world);
    wg::BuildingRules rules(cells);
    constexpr int radius = 64;
    constexpr int cellsPerPlace = (1 << wg::PLACE_SHIFT) / 16;
    constexpr int cellToZoneShift = wg::ZONE_SHIFT - 4;
    const auto findRoom = [&](int px, int py, int detail) -> std::optional<wg::Int2> {
        const int startX = px * cellsPerPlace, startY = py * cellsPerPlace;
        for (int cy = startY; cy < startY + 2 * cellsPerPlace; ++cy)
            for (int cx = startX; cx < startX + 2 * cellsPerPlace; ++cx)
                if (const auto castle = rules.castleCell(cx, cy);
                    castle.kind && (detail < 0 ? castle.kind >= 9 : castle.detail == detail))
                    return wg::Int2{cx, cy};
        return std::nullopt;
    };

    std::printf("Engine-predicted dungeon targets for seed %d in place cells [%d, %d] on both axes\n", seed, -radius, radius);
    bool found11 = false, found12 = false;
    for (int distance = 0; distance <= radius && !(found11 && found12); distance += 2) {
        for (int py = -distance; py <= distance; py += 2) {
            for (int px = -distance; px <= distance; px += 2) {
                if (std::max(std::abs(px), std::abs(py)) != distance)
                    continue;
                const auto place = wg::cellPlace(px, py);
                const int structure = wg::bigStructure(px, py, place.kind, place.big);
                if (requestedStructure ? structure != requestedStructure
                                       : (structure != 42 || found11) && (structure != 44 || found12))
                    continue;
                const int detail = requestedStructure ? -1 : (structure == 42 ? 11 : 12);
                if (const auto cell = findRoom(px, py, detail)) {
                    const auto castle = rules.castleCell(cell->x, cell->y);
                    std::printf("  structure %d kind %d detail %d style %d: cell %d %d, zone %d %d, top %d, floor %d\n",
                                structure, castle.kind, castle.detail, castle.style, cell->x, cell->y,
                                wg::floorDiv(cell->x, cellToZoneShift), wg::floorDiv(cell->y, cellToZoneShift),
                                rules.castleBlockTop(cell->x, cell->y), rules.castleFloor(cell->x, cell->y));
                    if (requestedStructure)
                        return 0;
                    if (structure == 42)
                        found11 = true;
                    else
                        found12 = true;
                }
            }
        }
    }
    if (requestedStructure)
        return std::printf("  no room predicted for structure %d in this search window\n", requestedStructure), 1;
    if (!found11 || !found12)
        std::printf("  missing predicted labels: 42/detail11 %s, 44/detail12 %s\n", found11 ? "no" : "yes", found12 ? "no" : "yes");
    return found11 && found12 ? 0 : 1;
}

int worldgenDungeonGuardTargets(int seed) {
    const auto world = wg::worldValues(seed);
    wg::CellObjects cells(world);
    wg::BuildingRules rules(cells);
    wg::ZoneTerrain terrain;
    constexpr int radius = 128;
    constexpr int cellToPlaceShift = wg::PLACE_SHIFT - 4;
    constexpr std::array<int, 4> floorCodes{0, 2, 3, 4};
    std::set<int> remaining(floorCodes.begin(), floorCodes.end());
    bool needGate = true;
    std::printf("Engine-predicted kind-8 guard targets for seed %d\n", seed);
    for (int distance = 1; distance <= radius && (needGate || !remaining.empty()); ++distance) {
        for (int py = -distance; py <= distance; ++py) {
            for (int px = -distance; px <= distance; ++px) {
                if (std::max(std::abs(px), std::abs(py)) != distance)
                    continue;
                const auto place = wg::cellPlace(px, py);
                if (place.kind != 8 || (place.big && (px % 2 != 0 || py % 2 != 0)))
                    continue;
                const wg::Int2 placeCell{px, py};
                wg::ZoneBuild zone(terrain, world, cells, false);
                zone.place.place = place;
                zone.place.cell = placeCell;
                const auto &floor = zone.dungeonGuardFloor();
                const auto square = wg::fortressSquare(placeCell, place.point);
                const auto [ax, ay] = square.middle;
                const int reach = square.size / 2 + 2; // Include the cell just outside the gate.
                const bool gateEnabled = needGate && (wg::random(wg::cellKey(px, py) + 0x10581) & 1);
                const int placeMask = place.big ? ~1 : ~0;
                for (int x = ax - reach; x <= ax + reach; ++x) {
                    for (int y = ay - reach; y <= ay + reach; ++y) {
                        if ((wg::floorDiv(x, cellToPlaceShift) & placeMask) != px ||
                            (wg::floorDiv(y, cellToPlaceShift) & placeMask) != py)
                            continue;
                        const int type = rules.type(x, y);
                        if (type >= 45 && type <= 47) {
                            if ((x == ax && y == ay) || !remaining.contains(floor.code))
                                continue;
                            if (rules.fortressFloorCode(x, y, 0) == 13 || rules.fortressFloorCode(x, y, 1) == 13) {
                                std::printf("  floor %d: cell %d %d, place %d %d", floor.code, x, y, px, py);
                                if (floor.code)
                                    std::printf(", selected cell %d %d", floor.cell.x, floor.cell.y);
                                std::putchar('\n');
                                remaining.erase(floor.code);
                            }
                        } else if (gateEnabled && needGate &&
                                   (rules.type(x - 1, y) == 46 || rules.type(x + 1, y) == 46 ||
                                    rules.type(x, y - 1) == 46 || rules.type(x, y + 1) == 46)) {
                            std::printf("  gate: cell %d %d, place %d %d\n", x, y, px, py);
                            needGate = false;
                        }
                    }
                }
            }
        }
    }
    for (const int code : remaining)
        std::printf("  no floor-%d candidate in the search window\n", code);
    if (needGate)
        std::puts("  no gate candidate in the search window");
    remaining.erase(0); // The native no-selection case may be absent from this search.
    return needGate || !remaining.empty() ? 1 : 0;
}

// `EngineTests wgstructures [zones]`; WS_COMPLETE=1 checks full generation in all four owner sets.
int worldgenStructureCommand(int maxZones) {
    failures = 0;
    hangingConeChecks();
    plannerDoorChecks();
    dungeonGuardFloorChecks();
    const fs::path game = gameDir();
    if (game.empty())
        return std::printf("no game folder\n"), 1;
    if (const char *c = std::getenv("WS_CELL")) { // debugging: what the rules say about a cell and its neighbours
        int cx = 0, cy = 0;
        std::sscanf(c, "%d %d", &cx, &cy);
        const wg::WorldValues world = wg::worldValues(123);
        wg::BuildingRules rules(wg::cellObjectsFor(world));
        for (int dy = 1; dy >= -1; dy--)
            for (int dx = -1; dx <= 1; dx++) {
                const wg::CastleCell k = rules.castleCell(cx + dx, cy + dy);
                const wg::Int2 p = wg::cellPoint(cx + dx, cy + dy);
                std::printf("cell %d %d: type %d base %d offset %d floor0 %d castle %d/%d/%d/%d point %d %d th %d tw %d\n", cx + dx,
                            cy + dy, rules.type(cx + dx, cy + dy), rules.baseHeight(cx + dx, cy + dy), rules.floorOffset(cx + dx, cy + dy),
                            (int)rules.floorExists(cx + dx, cy + dy, 0), k.kind, k.detail, k.style, k.steps, p.x, p.y,
                            wg::cellRandom(cx + dx, cy + dy, 0x2fbcc94) % 10 + 20, wg::cellRandom(cx + dx, cy + dy, 0x12ce32c) % 10 + 12);
            }
    }
    cellsOracle(game, true, 1 << 30);
    const auto directory = generatedZonesDir(game);
    wg::CreatureTables creatures;
    const wg::CreatureTables *complete = nullptr;
    if (std::getenv("WS_STORED_EXACT") && !std::getenv("WS_COMPLETE")) {
        std::puts("WS_STORED_EXACT requires WS_COMPLETE for full zone generation");
        return 1;
    }
    if (std::getenv("WS_COMPLETE")) {
        std::string why;
        if (!creatures.load(directory.parent_path() / "oracle", &why))
            return std::printf("cannot compare full generation: %s\n", why.c_str()), 1;
        complete = &creatures;
    }
    if (complete) {
        for (const char *set : {"gen_seed_123", "gen2_seed_123", "genb_seed_123", "genr_seed_123"})
            zonesOracle(directory.parent_path() / set, maxZones, true, complete);
    } else {
        zonesOracle(directory, maxZones, true);
    }
    if (const char *w = std::getenv("WS_WORLD")) // the whole extracted world, or a spread sample of it
        worldOracle(game / "Extract" / "seed_123", std::atoi(w), true);
    std::printf(failures ? "%d failure(s)\n" : "wgstructures: all passed\n", failures);
    return failures ? 1 : 0;
}
