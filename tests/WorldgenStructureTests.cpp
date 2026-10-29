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
#include "worldgen/CellObjects.h"
#include "worldgen/BuildingBuilder.h"
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
    {
        std::ifstream in(cwz, std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
        if (data.size() < 20 || std::memcmp(data.data(), "CWZ1", 4) != 0)
            return false;
        std::memcpy(&z.zx, &data[4], 4), std::memcpy(&z.zy, &data[8], 4);
        size_t at = 20;
        z.columns.resize(wg::ZONE_COLUMNS);
        for (auto &c : z.columns) {
            uint32_t runs;
            std::memcpy(&c.base, &data[at], 4), std::memcpy(&runs, &data[at + 4], 4);
            at += 8;
            for (uint32_t r = 0; r < runs; r++) {
                uint16_t n;
                wg::Block b;
                std::memcpy(&n, &data[at], 2), std::memcpy(&b, &data[at + 2], 6);
                at += 8;
                c.blocks.insert(c.blocks.end(), n, b);
            }
        }
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

struct ObjectKey {
    int type;
    int64_t x, y, z;
    int turn;
    uint32_t scale;
    bool operator<(const ObjectKey &o) const {
        return std::tie(type, x, y, z, turn, scale) < std::tie(o.type, o.x, o.y, o.z, o.turn, o.scale);
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
    int objectsGame = 0, objectsMine = 0, objectsSame = 0;
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
    std::multiset<ObjectKey> game, mine;
    if (auto it = g.lists.find(wg::zonelist::OBJECTS); it != g.lists.end())
        for (const auto &r : it->second)
            game.insert({field<int32_t>(r, 0), field<int64_t>(r, 8), field<int64_t>(r, 0x10), field<int64_t>(r, 0x18), r[0x20],
                         bitsOf(field<float>(r, 0x24))});
    for (const auto &o : objects)
        mine.insert({o.type, o.x, o.y, o.z, o.turn, bitsOf(o.scale)});
    std::multiset<ObjectKey> left = mine;
    for (const ObjectKey &k : game) {
        t.objectsByType[k.type][0]++;
        auto f = left.find(k);
        if (f != left.end()) {
            left.erase(f), t.objectsSame++, t.objectsByType[k.type][2]++;
        } else if (verbose && shown++ < 25) {
            std::printf("  zone %d %d game only: object %d at %.3f %.3f %.3f turn %d\n", g.zx, g.zy, k.type, k.x / 65536.0, k.y / 65536.0,
                        k.z / 65536.0, k.turn);
        }
    }
    for (const ObjectKey &k : mine)
        t.objectsByType[k.type][1]++;
    if (verbose)
        for (const ObjectKey &k : left)
            if (shown++ < 25)
                std::printf("  zone %d %d ours only: object %d at %.3f %.3f %.3f turn %d\n", g.zx, g.zy, k.type, k.x / 65536.0,
                            k.y / 65536.0, k.z / 65536.0, k.turn);
    t.objectsGame += (int)game.size(), t.objectsMine += (int)mine.size();

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
    int zones = 0, rulesWhole = 0, oursWhole = 0, emittersWhole = 0, sideTorches = 0;
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
        // The rooms' side model of kind 1 (type 0x43, buildings file §6, 0x6a9a0) is a light in the
        // game (tint (0.6, 0.5, 0.2), flags 1); the spec gives it no tint or flags, so ours is not.
        // Those records are left out of the comparison until the spec says.
        constexpr int32_t SIDE_TORCH = 0x43;
        std::vector<std::vector<uint8_t>> withoutTorches;
        for (const auto &r : lights)
            if (field<int32_t>(r, 0) != SIDE_TORCH)
                withoutTorches.push_back(r);
        t.sideTorches += (int)(lights.size() - withoutTorches.size());
        bool same = ours->modelLights.size() == withoutTorches.size();
        for (size_t i = 0; same && i < withoutTorches.size(); i++)
            same = sameModel(withoutTorches[i], ours->modelLights[i]);
        if (!same && std::getenv("WS_LIGHTS")) { // debugging: a zone whose light list is not ours
            std::printf("  zone %d %d lights: game %zu (%zu but the side torches), ours %zu\n", g.zx, g.zy, lights.size(),
                        withoutTorches.size(), ours->modelLights.size());
            for (const auto &r : withoutTorches)
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
}

int zonesOracle(const fs::path &dir, int maxZones, bool verbose) {
    const std::vector<fs::path> files = zoneFiles(dir, maxZones);
    if (files.empty())
        return 0;
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
    long long prefixTotal = 0, creatureTotal = 0;
    double newSeconds = 0;
    std::vector<std::string> notes;
    std::map<int, std::array<int, 2>> columnsByType; // by the cell object: columns, differing (not just marks)
    for (const fs::path &file : files) {
        GameZone g;
        if (!readZone(file, g))
            continue;
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
        wg::ZoneContents z = wg::generateZone(world, g.zx, g.zy, options);
        newSeconds += seconds(t0);
        // The extracted records were taken on day.txt's day; without part C the refresh only
        // restores the objects from the generator's copy.
        if (day >= 0)
            wg::refreshZone(z, world, day);
        for (int i = 0; i < wg::ZONE_COLUMNS; i++) {
            const wg::TerrainColumn &gc = g.columns[(size_t)i], &oc = z.terrain.columns[(size_t)i];
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
        // The draws, to the first creature draw.
        if (!g.trace.empty()) {
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
    std::printf("wgstructures zones %d (%s, day %d):\n", mine.zones, name.c_str(), day);
    printTotals("new", mine);
    std::printf("  new: of the differing columns, %d differ only in the flag 0x40 of blocks inside static objects, %d otherwise\n",
                markOnlyColumns, otherColumns);
    std::printf("  rand(): %d traces; %d zones match up to their first creature draw (%lld of %lld draws)\n", traces, tracesToCreature,
                prefixTotal, creatureTotal);
    std::printf("  light lists (+0x90, %lld records): the rule on the game's models whole in %d of %d zones, ours in %d (%d side torches left out); emitters "
                "(+0xa8, %lld) as the rule on the game's models in %d\n",
                modelLists.lights, modelLists.rulesWhole, modelLists.zones, modelLists.oursWhole, modelLists.sideTorches, modelLists.emitters,
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
    std::printf("  light lists (+0x90, %lld records): the rule on the game's models whole in %d of %d zones, ours in %d (%d side torches left out); emitters %lld, "
                "as the rule on the game's models in %d\n",
                modelLists.lights, modelLists.rulesWhole, modelLists.zones, modelLists.oursWhole, modelLists.sideTorches, modelLists.emitters,
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
void plannerDoorChecks() {
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

int worldgenStructureTests() {
    failures = 0;
    plannerDoorChecks();
    const fs::path game = gameDir();
    if (game.empty())
        return 0;
    cellsOracle(game, false, 5000);
    zonesOracle(generatedZonesDir(game), 6, false);
    return failures;
}

// `EngineTests wgstructures [zones]`.
int worldgenStructureCommand(int maxZones) {
    failures = 0;
    plannerDoorChecks();
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
    zonesOracle(generatedZonesDir(game), maxZones, true);
    if (const char *w = std::getenv("WS_WORLD")) // the whole extracted world, or a spread sample of it
        worldOracle(game / "Extract" / "seed_123", std::atoi(w), true);
    std::printf(failures ? "%d failure(s)\n" : "wgstructures: all passed\n", failures);
    return failures ? 1 : 0;
}
