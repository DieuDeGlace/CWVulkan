// The new world generator's creatures (src/worldgen, docs/specs/worldgen-creatures*.md) against
// the game's own outputs (worldgen-creatures.md §8):
//   - Extract/oracle: gear.bin (30,000 scripted gear calls), looks.bin (every race's looks),
//     pickers.txt (the race pickers), regions.txt (region plans);
//   - the generated zone sets (Extract/gen_seed_123, gen2_, genb_, and genr_ refreshed on day.txt's
//     day): every rand() of each zone's generation (.rtrace), its objects, models and items
//     (.rec) and its spawn points (.spawns).
// `EngineTests wgcreatures [zones]` runs everything with details; the default run takes a subset.
#include "Assets.h"
#include "Extracted.h"
#include "worldgen/CreatureBuilder.h"
#include "worldgen/Residents.h"
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
#define WC_CHECK(x)                                                                                \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                              \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

namespace wg = worldgen;
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
double seconds(Clock::time_point a) { return std::chrono::duration<double>(Clock::now() - a).count(); }

fs::path gameDir() {
    Assets a;
    return a.locate("") ? a.gameDir() : fs::path();
}

std::vector<uint8_t> readFile(const fs::path &p) {
    std::ifstream in(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), {});
}
template <class T> T field(const uint8_t *r, size_t off) {
    T v;
    std::memcpy(&v, r + off, sizeof v);
    return v;
}

// The extraction tool's scripted rand() (§8.4).
struct ScriptedRand {
    uint32_t state;
    int calls = 0;
    explicit ScriptedRand(uint32_t seed) : state(seed ? seed : 0x9e3779b9u) {}
    int operator()() {
        state ^= state << 13, state ^= state >> 17, state ^= state << 5;
        calls++;
        return (int)(state & 0x7fff);
    }
};

// --- gear.bin --------------------------------------------------------------------------------

int gearOracle(const fs::path &file, int maxRecords, bool verbose) {
    const std::vector<uint8_t> b = readFile(file);
    if (b.size() < 4 || std::memcmp(b.data(), "CWG2", 4) != 0)
        return 0;
    size_t at = 4;
    auto i32 = [&](size_t o) { return field<int32_t>(b.data(), o); };
    int records = 0, wrong = 0;
    std::map<std::string, int> fields;
    while (at < b.size() && records < maxRecords) {
        wg::GearInput in{i32(at), i32(at + 4), i32(at + 8), (uint32_t)i32(at + 16), i32(at + 12)};
        const uint8_t clsIn = b[at + 20];
        const uint32_t seed = (uint32_t)i32(at + 21), calls = (uint32_t)i32(at + 25);
        const uint8_t cls = b[at + 29], spec = b[at + 30];
        const uint8_t *eq = &b[at + 31];
        at += 31 + 0x6e0;
        auto ints = [&] {
            std::vector<int32_t> v((size_t)i32(at));
            for (size_t k = 0; k < v.size(); k++)
                v[k] = i32(at + 4 + 4 * k);
            at += 4 + 4 * v.size();
            return v;
        };
        const std::vector<int32_t> v1 = ints();
        const int32_t pick = i32(at);
        at += 4;
        const std::vector<int32_t> v2 = ints();
        std::vector<std::array<int, 4>> inv;
        const int tabs = i32(at);
        at += 4;
        for (int t = 0; t < tabs; t++) {
            const int bytes = i32(at);
            at += 4;
            for (int s = 0; s + 0xa4 <= bytes; s += 0xa4)
                inv.push_back({t, i32(at + (size_t)s), b[at + (size_t)s + 4], i32(at + (size_t)s + 8)});
            at += (size_t)bytes;
        }
        records++;
        ScriptedRand r(seed);
        wg::Gear g;
        g.cls = clsIn;
        wg::makeGear(in, wg::Draw(r), g);
        std::vector<std::string> bad;
        if ((uint32_t)r.calls != calls)
            bad.push_back("draws");
        if (g.cls != cls || g.spec != spec)
            bad.push_back("class");
        for (int s = 0; s < 11; s++) {
            const uint8_t *it = eq + s * 0xa0;
            const wg::Item &m = g.equipment[(size_t)s];
            if (m.category != it[0] || m.id != field<int32_t>(it, 4) || m.modifier != field<uint32_t>(it, 8) || m.rarity != it[0x14] ||
                m.material != it[0x1c]) {
                bad.push_back("slot" + std::to_string(s));
            }
        }
        if (g.abilities != v1 || g.abilities2 != v2 || g.abilityPick != pick)
            bad.push_back("abilities");
        std::vector<std::array<int, 4>> ours;
        for (size_t t = 0; t < g.inventory.tabs.size(); t++)
            for (const wg::Stack &s : g.inventory.tabs[t])
                ours.push_back({(int)t, s.quantity, s.item.category, s.item.id});
        if (ours != inv)
            bad.push_back("inventory");
        if (!bad.empty()) {
            for (auto &f : bad)
                fields[f]++;
            if (verbose && wrong < 8) {
                std::printf("  gear: hostility %d race 0x%x level %d people %d flags %x class 0x%x:", in.hostility, in.race, in.level,
                            in.people, in.flags, clsIn);
                for (auto &f : bad)
                    std::printf(" %s", f.c_str());
                std::printf("\n");
            }
            wrong++;
        }
    }
    std::printf("wgcreatures gear.bin: %d of %d gear records match\n", records - wrong, records);
    if (verbose)
        for (auto &[f, n] : fields)
            std::printf("  wrong %s: %d\n", f.c_str(), n);
    WC_CHECK(records > 0 && wrong == 0);
    return wrong;
}

// --- looks.bin -------------------------------------------------------------------------------

int looksOracle(const fs::path &dir, const wg::CreatureTables &tables, bool verbose) {
    const std::vector<uint8_t> b = readFile(dir / "looks.bin");
    if (b.size() < 4)
        return 0;
    constexpr size_t RECORD = 16 + wg::Appearance::SIZE;
    int records = 0, same = 0;
    std::map<int, int> wrongRaces;
    for (size_t at = 4; at + RECORD <= b.size(); at += RECORD) {
        const int32_t race = field<int32_t>(&b[at], 0);
        ScriptedRand r(field<uint32_t>(&b[at], 4));
        wg::Appearance a;
        wg::makeLooks(tables, race, false, wg::Draw(r), a);
        records++;
        const bool ok = (uint32_t)r.calls == field<uint32_t>(&b[at], 8) && std::memcmp(a.bytes.data(), &b[at + 16], wg::Appearance::SIZE) == 0;
        same += ok;
        if (!ok && wrongRaces[race]++ == 0 && verbose && wrongRaces.size() <= 12) {
            std::printf("  looks race 0x%x: draws %d/%u;", race, r.calls, field<uint32_t>(&b[at], 8));
            for (size_t k = 0; k < wg::Appearance::SIZE; k++)
                if (a.bytes[k] != b[at + 16 + k])
                    std::printf(" +%zx %02x/%02x", k, a.bytes[k], b[at + 16 + k]);
            std::printf("\n");
        }
    }
    std::printf("wgcreatures looks.bin: %d of %d appearances match (%zu races differ)\n", same, records, wrongRaces.size());
    WC_CHECK(records > 0 && same == records);
    return records - same;
}

// --- pickers.txt -----------------------------------------------------------------------------

int pickersOracle(const fs::path &dir, const wg::CreatureTables &tables) {
    std::ifstream in(dir / "pickers.txt");
    static const std::map<uint32_t, wg::Picker> BY_RVA = {
        {0x141480, wg::Picker::People}, {0x1408e0, wg::Picker::PeopleOrBeasts}, {0x142190, wg::Picker::Undead},
        {0x142760, wg::Picker::UndeadLeader}, {0x13ffc0, wg::Picker::Beasts}, {0x13feb0, wg::Picker::BeastLeader},
        {0x141390, wg::Picker::Lizards}, {0x142000, wg::Picker::Golems}, {0x13fa50, wg::Picker::TowerFolk},
        {0x13fda0, wg::Picker::Imps}, {0x140490, wg::Picker::KindEightPeople}};
    int runs = 0, same = 0;
    std::set<std::string> seen;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream l(line);
        std::string rva;
        uint32_t seed, draws;
        int32_t race, culture = -1;
        if (!(l >> rva >> seed >> draws >> race))
            continue;
        l >> culture;
        const auto it = BY_RVA.find((uint32_t)std::stoul(rva, nullptr, 16));
        if (it == BY_RVA.end())
            continue;
        seen.insert(rva);
        ScriptedRand r(seed);
        const int32_t ours = tables.pick(it->second, wg::Draw(r), culture);
        runs++;
        same += ours == race && (uint32_t)r.calls == draws;
    }
    std::printf("wgcreatures pickers.txt: %d of %d picker runs match (%zu pickers recorded; imps, kind-8 people and the culture lists %s)\n",
                same, runs, seen.size(), seen.count("13fda0") && seen.count("140490") ? "recorded" : "not recorded");
    WC_CHECK(runs > 0 && same == runs);
    return runs - same;
}

// --- regions.txt -----------------------------------------------------------------------------

int regionsOracle(const fs::path &file, bool verbose) {
    std::ifstream in(file);
    struct Game {
        std::map<wg::PlaceCell, int> role, level;
        wg::Int2 special;
        int marked = 0;
        std::vector<int32_t> items;
    };
    std::map<std::pair<int, int>, Game> regions;
    Game *g = nullptr;
    for (std::string line; std::getline(in, line);) {
        std::istringstream l(line);
        std::string what;
        l >> what;
        if (what == "region") {
            int x, y;
            l >> x >> y;
            // Regions far out (|r| >= 1000) were dumped from another world and are left out.
            g = std::abs(x) < 1000 && std::abs(y) < 1000 ? &regions[{x, y}] : nullptr;
        } else if (!g) {
            continue;
        } else if (what == "map") {
            std::string off;
            int x, y, v;
            l >> off >> x >> y >> v;
            if (off == "10")
                g->role[{x, y}] = v;
            else if (off == "60")
                g->level[{x, y}] = v;
        } else if (what == "special") {
            l >> g->special.x >> g->special.y;
        } else if (what == "marked") {
            l >> g->marked;
        } else if (what == "items") {
            for (int v; l >> v;)
                g->items.push_back(v);
        }
    }
    int same = 0;
    for (auto &[r, game] : regions) {
        const wg::RegionPlan ours = wg::makeRegionPlan(r.first, r.second);
        const bool ok = ours.roles == game.role && ours.levels == game.level && ours.specialTown == game.special &&
                        ours.marked == (game.marked != 0) && ours.items == game.items;
        same += ok;
        if (!ok && verbose)
            std::printf("  region %d %d differs: roles %zu/%zu levels %zu/%zu\n", r.first, r.second, ours.roles.size(), game.role.size(),
                        ours.levels.size(), game.level.size());
    }
    std::printf("wgcreatures regions.txt: %d of %zu region plans match\n", same, regions.size());
    WC_CHECK(!regions.empty() && same == (int)regions.size());
    return (int)regions.size() - same;
}

// --- The generated zones ---------------------------------------------------------------------

struct GamePoint {
    int64_t x, y, z, id;
    float yaw;
    int32_t host, race, level, people, style, roam, mode;
    wg::Int2 home;
    uint32_t flags;
    uint8_t cls, spec;
    const uint8_t *equip;
    const uint8_t *appearance;
};

std::vector<GamePoint> readSpawns(const std::vector<uint8_t> &b) {
    std::vector<GamePoint> out;
    if (b.size() < 8)
        return out;
    const uint32_t n = field<uint32_t>(b.data(), 4);
    size_t at = 8;
    for (uint32_t i = 0; i < n && at + 0x950 <= b.size(); i++) {
        const uint8_t *r = &b[at];
        GamePoint g;
        g.x = field<int64_t>(r, 0), g.y = field<int64_t>(r, 8), g.z = field<int64_t>(r, 16), g.yaw = field<float>(r, 0x18);
        g.host = field<int32_t>(r, 0x1c), g.race = field<int32_t>(r, 0x20), g.level = field<int32_t>(r, 0x24);
        g.people = field<int32_t>(r, 0x28), g.style = field<int32_t>(r, 0x30), g.id = field<int64_t>(r, 0x38);
        g.flags = field<uint32_t>(r, 0x58), g.roam = field<int32_t>(r, 0x8b8), g.mode = field<int32_t>(r, 0x8bc) & 0xff;
        g.home = {field<int32_t>(r, 0x8f8), field<int32_t>(r, 0x8fc)};
        g.cls = r[0x34], g.spec = r[0x35];
        g.equip = r + 0x110, g.appearance = r + 0x50;
        at += 0x950;
        for (int v = 0; v < 2; v++)
            at += 4 + 4 * (size_t)field<uint32_t>(b.data(), at);
        out.push_back(g);
    }
    return out;
}

struct ObjectKey {
    int type;
    int64_t x, y, z;
    int turn;
    uint32_t scale;
    bool operator<(const ObjectKey &o) const { return std::tie(type, x, y, z, turn, scale) < std::tie(o.type, o.x, o.y, o.z, o.turn, o.scale); }
};
uint32_t bitsOf(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
template <class K> int matched(std::multiset<K> game, const std::multiset<K> &mine) {
    int n = 0;
    for (const K &k : mine) {
        auto it = game.find(k);
        if (it != game.end())
            game.erase(it), n++;
    }
    return n;
}

struct Totals {
    int zones = 0, traces = 0, wholeTraces = 0;
    long long draws = 0, drawsSame = 0;
    int objectsGame = 0, objectsMine = 0, objectsSame = 0;
    int modelsGame = 0, modelsSame = 0;
    int itemsGame = 0, itemsMine = 0, itemsSame = 0, itemsAngle = 0;
    int pointsGame = 0, pointsMine = 0, pointsPlaced = 0, pointsSame = 0, idsSame = 0;
    int pointsExcused = 0; // pairs left out as the documented exception below
    std::string set;       // the oracle set's folder name
    std::map<std::string, int> wrongFields;
    std::vector<std::string> notes;
    double newSeconds = 0;
};

// The one documented exception (worldgen-creatures-zone.md §4.2 errata, "Settled"): in genr_seed_123
// zone (-18, -18) the second shopper's spot lies past the end of the counter list and the game read
// heap memory there, so its spawn point (a townsperson, hostility 3, race 1) has a garbage
// position. Ours stands on the counter spot at block (-1119.5, -1119.5, 88). Only that pair is
// excused; any other unpaired point still fails.
bool excusedPair(const std::string &set, int32_t zx, int32_t zy, const GamePoint &game, const wg::SpawnPoint &ours) {
    constexpr int64_t SPOT_X = (int64_t)(-1119.5 * 65536), SPOT_Y = (int64_t)(-1119.5 * 65536), SPOT_Z = 88 * 65536;
    constexpr int TOWNSPERSON = 3, RACE = 1;
    return set == "genr_seed_123" && zx == -18 && zy == -18 && game.host == TOWNSPERSON && game.race == RACE &&
           ours.hostility == TOWNSPERSON && ours.race == RACE && ours.x == SPOT_X && ours.y == SPOT_Y && ours.z == SPOT_Z;
}

void compareZone(const fs::path &cwz, const wg::ZoneContents &z, const std::vector<uint16_t> &draws, Totals &t, bool verbose,
                 int32_t zx, int32_t zy, bool compareTrace) {
    // The draws.
    fs::path p = cwz;
    const std::vector<uint8_t> tr = readFile(p.replace_extension(".rtrace"));
    const size_t n = tr.size() / 6;
    if (compareTrace && n > 0) {
        t.traces++;
        size_t same = 0;
        while (same < n && same < draws.size() && field<uint16_t>(&tr[same * 6], 4) == draws[same])
            same++;
        const bool whole = same == n && draws.size() == n;
        t.wholeTraces += whole;
        t.draws += (long long)n, t.drawsSame += (long long)same;
        if (!whole) {
            char line[200];
            std::snprintf(line, sizeof line, "zone %d %d: draws the same %zu of %zu (ours %zu), first difference at a 0x%x draw", zx, zy, same, n,
                          draws.size(), same < n ? field<uint32_t>(&tr[same * 6], 0) : 0u);
            t.notes.push_back(line);
            if (std::getenv("WC_TRACE"))
                for (size_t k = same > 10 ? same - 10 : 0; k < same + 4 && k < n; k++)
                    std::printf("    draw %zu: game 0x%x %d, new %d%s\n", k, field<uint32_t>(&tr[k * 6], 0), field<uint16_t>(&tr[k * 6], 4),
                                k < draws.size() ? draws[k] : -1, k == same ? "  <-" : "");
        }
    }
    // The records.
    const std::vector<uint8_t> rec = readFile(p.replace_extension(".rec"));
    std::multiset<ObjectKey> go, mo;
    std::multiset<std::tuple<int, int64_t, int64_t, int64_t>> gm, mm;
    std::multiset<std::tuple<int, int, int64_t, int64_t, int64_t>> gi, mi;
    std::multiset<std::tuple<int, int, int64_t, int64_t, int64_t, uint32_t>> gia, mia;
    for (size_t at = 4; rec.size() >= 4 && at + 12 <= rec.size();) {
        const uint32_t offset = field<uint32_t>(&rec[at], 0), stride = field<uint32_t>(&rec[at], 4), count = field<uint32_t>(&rec[at], 8);
        at += 12;
        if (!stride || count > (rec.size() - at) / stride)
            break;
        for (uint32_t i = 0; i < count; i++) {
            const uint8_t *r = &rec[at + (size_t)i * stride];
            if (offset == wg::zonelist::OBJECTS)
                go.insert({field<int32_t>(r, 0), field<int64_t>(r, 8), field<int64_t>(r, 0x10), field<int64_t>(r, 0x18), r[0x20],
                           bitsOf(field<float>(r, 0x24))});
            else if (offset == wg::zonelist::MODELS)
                gm.insert({field<int32_t>(r, 0), field<int64_t>(r, 8), field<int64_t>(r, 0x10), field<int64_t>(r, 0x18)});
            else if (offset == wg::zonelist::ITEMS) {
                gi.insert({r[0], field<int32_t>(r, 4), field<int64_t>(r, 0xa0), field<int64_t>(r, 0xa8), field<int64_t>(r, 0xb0)});
                gia.insert({r[0], field<int32_t>(r, 4), field<int64_t>(r, 0xa0), field<int64_t>(r, 0xa8), field<int64_t>(r, 0xb0),
                            bitsOf(field<float>(r, 0xc0))});
            }
        }
        at += (size_t)stride * count;
    }
    for (const auto &o : z.objects)
        mo.insert({o.type, o.x, o.y, o.z, o.turn, bitsOf(o.scale)});
    for (const auto &m : z.models)
        mm.insert({m.type, m.x, m.y, m.z});
    for (const auto &i : z.items) {
        mi.insert({i.item.category, i.item.id, i.x, i.y, i.z});
        mia.insert({i.item.category, i.item.id, i.x, i.y, i.z, bitsOf(i.angle)});
    }
    t.objectsGame += (int)go.size(), t.objectsMine += (int)mo.size(), t.objectsSame += matched(go, mo);
    t.modelsGame += (int)gm.size(), t.modelsSame += matched(gm, mm);
    t.itemsGame += (int)gi.size(), t.itemsMine += (int)mi.size(), t.itemsSame += matched(gi, mi), t.itemsAngle += matched(gia, mia);
    if (verbose && std::getenv("WC_RECORDS")) {
        for (const ObjectKey &k : go)
            if (!mo.count(k))
                std::printf("  zone %d %d game only: object %d at %.2f %.2f %.2f turn %d\n", zx, zy, k.type, k.x / 65536.0, k.y / 65536.0, k.z / 65536.0, k.turn);
        for (const ObjectKey &k : mo)
            if (!go.count(k))
                std::printf("  zone %d %d ours only: object %d at %.2f %.2f %.2f turn %d\n", zx, zy, k.type, k.x / 65536.0, k.y / 65536.0, k.z / 65536.0, k.turn);
    }
    // The spawn points: each game point is paired with ours by position and race (§8.2).
    const std::vector<uint8_t> sb = readFile(p.replace_extension(".spawns"));
    const std::vector<GamePoint> game = readSpawns(sb);
    std::vector<const wg::SpawnPoint *> ours;
    for (const auto &[id, sp] : z.spawns.points)
        ours.push_back(&sp);
    std::vector<bool> used(ours.size());
    t.pointsGame += (int)game.size(), t.pointsMine += (int)ours.size();
    int shown = 0;
    std::vector<const GamePoint *> unpaired;
    for (const GamePoint &g : game) {
        size_t best = ours.size();
        for (size_t i = 0; i < ours.size(); i++)
            if (!used[i] && ours[i]->x == g.x && ours[i]->y == g.y && ours[i]->z == g.z && ours[i]->race == g.race) {
                best = i;
                break;
            }
        if (best == ours.size()) {
            unpaired.push_back(&g);
            continue;
        }
            if (verbose && std::getenv("WC_POINTS") && shown++ < 6)
                std::printf("  zone %d %d game only: hostility %d race 0x%x at %.2f %.2f %.2f\n", zx, zy, g.host, g.race, g.x / 65536.0,
                            g.y / 65536.0, g.z / 65536.0);
        used[best] = true;
        const wg::SpawnPoint &o = *ours[best];
        t.pointsPlaced++;
        t.idsSame += o.id == g.id;
        std::vector<std::string> bad;
        if (o.hostility != g.host)
            bad.push_back("hostility");
        if (o.level != g.level)
            bad.push_back("level");
        if (o.flags() != g.flags)
            bad.push_back("flags");
        if (o.people != g.people || (int32_t)o.style != g.style)
            bad.push_back("people");
        if ((int32_t)o.roam != g.roam)
            bad.push_back("roam");
        if (o.mode != g.mode)
            bad.push_back("mode");
        if (o.home != g.home)
            bad.push_back("home");
        if (bitsOf(o.yaw) != bitsOf(g.yaw))
            bad.push_back("yaw");
        if (o.gear.cls != g.cls || o.gear.spec != g.spec)
            bad.push_back("class");
        std::array<uint8_t, wg::Appearance::SIZE> mine = o.appearance.bytes;
        std::memcpy(&mine[5], g.appearance + 5, 3); // padding: whatever the game's memory held
        if (std::memcmp(mine.data(), g.appearance, mine.size()) != 0)
            bad.push_back("appearance");
        for (int s = 0; s < 11; s++) {
            const uint8_t *it = g.equip + s * 0xa0;
            const wg::Item &m = o.gear.equipment[(size_t)s];
            if (m.category != it[0] || m.id != field<int32_t>(it, 4) || m.modifier != field<uint32_t>(it, 8) || m.material != it[0x1c] ||
                m.rarity != it[0x14]) {
                bad.push_back("equipment");
                break;
            }
        }
        if (bad.empty()) {
            t.pointsSame++;
            continue;
        }
        for (auto &f : bad)
            t.wrongFields[f]++;
        if (verbose && std::getenv("WC_POINTS") && shown++ < 6) {
            std::printf("  zone %d %d: hostility %d race 0x%x level %d/%d flags %x/%x roam %x/%x home %d %d/%d %d:", zx, zy, g.host, g.race,
                        o.level, g.level, o.flags(), g.flags, o.roam, g.roam, o.home.x, o.home.y, g.home.x, g.home.y);
            for (auto &f : bad)
                std::printf(" %s", f.c_str());
            std::printf("\n");
        }
    }
    for (const GamePoint *g : unpaired) {
        bool excused = false;
        for (size_t i = 0; i < ours.size() && !excused; i++)
            if (!used[i] && excusedPair(t.set, zx, zy, *g, *ours[i]))
                used[i] = excused = true;
        if (excused) {
            t.pointsExcused++;
            continue;
        }
        t.wrongFields["unpaired"]++;
        if (verbose && std::getenv("WC_POINTS") && shown++ < 6)
            std::printf("  zone %d %d game only: hostility %d race 0x%x at %.2f %.2f %.2f\n", zx, zy, g->host, g->race, g->x / 65536.0,
                        g->y / 65536.0, g->z / 65536.0);
    }
    if (verbose && std::getenv("WC_POINTS"))
        for (size_t i = 0; i < ours.size(); i++)
            if (!used[i])
                std::printf("  zone %d %d ours only: hostility %d race 0x%x at %.2f %.2f %.2f\n", zx, zy, ours[i]->hostility, ours[i]->race,
                            ours[i]->x / 65536.0, ours[i]->y / 65536.0, ours[i]->z / 65536.0);
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

// One generated set. `refreshDay` >= 0: the set was saved after the daily refresh of that day.
Totals zonesOracle(const fs::path &dir, const wg::CreatureTables &tables, int maxZones, bool verbose, int refreshDay) {
    Totals t;
    t.set = dir.filename().string();
    const std::vector<fs::path> files = zoneFiles(dir, maxZones);
    if (files.empty())
        return t;
    const std::string name = dir.filename().string();
    const int seed = std::atoi(name.substr(name.find_last_of('_') + 1).c_str());
    int day = 0;
    if (std::ifstream d(dir / "day.txt"); d)
        d >> day;
    const wg::WorldValues world = wg::worldValues(seed);
    for (const fs::path &file : files) {
        int32_t zx = 0, zy = 0;
        ExtractedWorld::zoneCoordinates(file, zx, zy);
        if (const char *only = std::getenv("WC_ONLY")) {
            int ox = 0, oy = 0;
            std::sscanf(only, "%d %d", &ox, &oy);
            if (ox != zx || oy != zy)
                continue;
        }
        t.zones++;
        auto t0 = Clock::now();
        wg::ZoneContents z = wg::generateZoneWithCreatures(world, tables, day, zx, zy, true);
        const std::vector<uint16_t> draws = z.terrain.draws;
        if (refreshDay >= 0)
            wg::refreshZoneCreatures(z, world, tables, refreshDay);
        t.newSeconds += seconds(t0);
        compareZone(file, z, draws, t, verbose, zx, zy, refreshDay < 0);
    }
    std::printf("wgcreatures %s (%d zones, day %d%s):\n", name.c_str(), t.zones, day, refreshDay >= 0 ? ", refreshed" : "");
    if (t.traces)
        std::printf("  rand(): %d of %d traces whole (%lld of %lld draws)\n", t.wholeTraces, t.traces, t.drawsSame, t.draws);
    std::printf("  objects game %d, new %d, the same %d; models game %d, the same %d; items game %d, new %d, the same %d, same angle %d\n",
                t.objectsGame, t.objectsMine, t.objectsSame, t.modelsGame, t.modelsSame, t.itemsGame, t.itemsMine, t.itemsSame, t.itemsAngle);
    std::printf("  spawn points game %d, new %d, paired %d, every field the same %d, same id %d, excused %d\n", t.pointsGame, t.pointsMine,
                t.pointsPlaced, t.pointsSame, t.idsSame, t.pointsExcused);
    if (verbose) {
        for (auto &[f, n] : t.wrongFields)
            std::printf("  wrong %s: %d\n", f.c_str(), n);
        for (const std::string &s : t.notes)
            std::printf("  %s\n", s.c_str());
        std::printf("  time: %.3f s (%.1f ms a zone, part A to C)\n", t.newSeconds, 1000.0 * t.newSeconds / std::max(t.zones, 1));
    }
    return t;
}

fs::path extractDir(const fs::path &game) { return game / "Extract"; }

bool loadTables(const fs::path &game, wg::CreatureTables &tables) {
    std::string why;
    if (!tables.load(extractDir(game) / "oracle", &why)) {
        std::printf("wgcreatures: %s\n", why.c_str());
        return false;
    }
    return true;
}

// The resident pass and arena stages on made-up points (§7).
void residentChecks() {
    wg::SpawnPoint a, b2, c;
    a.id = 1, a.requiredStage = 1;
    b2.id = 2, b2.requiredStage = 1;
    c.id = 3, c.requiredStage = 3;
    std::set<int64_t> dead{1, 2};
    auto isDead = [&](int64_t id) { return dead.count(id) != 0; };
    WC_CHECK(wg::arenaStage(0, {&a, &b2, &c}, isDead) == 2);
    dead.insert(3);
    WC_CHECK(wg::arenaStage(2, {&a, &b2, &c}, isDead) == 0xff);
    wg::ResidentWorld w;
    w.players.push_back({0, 0, 0});
    w.dead = isDead;
    wg::SpawnPoint p;
    p.id = 9, p.x = 100 * wg::DOTS;
    p.prerequisites = {1, 2};
    WC_CHECK(wg::residentReady(p, 0, 0, w));
    p.prerequisites.insert(4);
    WC_CHECK(!wg::residentReady(p, 0, 0, w));
    p.prerequisites.clear(), p.x = 300 * wg::DOTS;
    WC_CHECK(!wg::residentReady(p, 0, 0, w));
    WC_CHECK(wg::residentZones(w.players).size() == 81);

    // The creature constructor's defaults (§7.3).
    const wg::CreatureRecord r = wg::newCreature(77);
    auto f32 = [&](size_t at) { return field<float>(r.data(), at); };
    WC_CHECK(r.size() == wg::CREATURE_SIZE && field<int64_t>(r.data(), wg::creature::ID) == 77 && r[wg::creature::HOSTILITY] == 3);
    WC_CHECK(f32(wg::creature::STUN) == -3000.0f && field<int32_t>(r.data(), wg::creature::LEVEL) == 1 && f32(wg::creature::HEALTH) == 500.0f);
    WC_CHECK(f32(wg::creature::BLOCK_POWER) == 1.0f && f32(wg::creature::GRIP) == 1.0f && f32(wg::creature::LIGHT_RADIUS) == 10.0f);
    WC_CHECK(field<int32_t>(r.data(), wg::creature::HOME_PLACE) == 0x7fffffff && field<int32_t>(r.data(), wg::creature::HOME_CELL + 4) == -1);
    WC_CHECK(f32(wg::creature::STAMINA) == 1.0f && field<int32_t>(r.data(), wg::creature::ABILITY_PICK) == 1 && f32(0x1530) == 200.0f);
    // The pose's size factor is 1 in every recorded pose call; at 0 a creature has no size.
    WC_CHECK(f32(wg::creature::POSE_SIZE_FACTOR) == 1.0f);
    // The resident pass starts from them (§7.1): home cell = the 16-block cell of its blocks.
    wg::SpawnPoint q;
    q.id = 5, q.x = 40 * wg::DOTS, q.y = -3 * wg::DOTS;
    auto zero = [] { return 0; };
    const wg::MadeCreature m = wg::makeResident(q, wg::Draw(zero), false, {});
    WC_CHECK(field<int32_t>(m.record.data(), wg::creature::HOME_CELL) == 2 && field<int32_t>(m.record.data(), wg::creature::HOME_CELL + 4) == -1);
    WC_CHECK(field<float>(m.record.data(), wg::creature::STUN) == -3000.0f && field<int64_t>(m.record.data(), wg::creature::ID) == 5);

    // The inventory (§4.5): tabs by category, stacking, the counted items.
    wg::Inventory inv;
    wg::Item potion, sword, coin;
    potion.category = 1, potion.id = 1;
    sword.category = 3;
    coin.category = 12, coin.material = 11;
    inv.add(potion, 1), inv.add(potion, 2), inv.add(sword, 1), inv.add(sword, 1), inv.add(coin, 5);
    WC_CHECK(inv.tabs.size() == 3 && inv.tabs[0].size() == 2 && inv.tabs[2].size() == 1 && inv.tabs[2][0].quantity == 3);
    WC_CHECK(inv.countedMaterial == 5 && wg::Inventory::tabOf(23) == 5 && wg::Inventory::tabOf(2) == 3 && wg::Inventory::tabOf(29) == 1);
    inv.tabs[0][0].quantity = 0; // an emptied stack takes the next item that does not stack
    inv.add(sword, 1);
    WC_CHECK(inv.tabs[0].size() == 2 && inv.tabs[0][0].quantity == 1);
}

// Pets (§7.4): every pet of a race draws srand(race)'s numbers, so it always has the same looks
// and gear; the owner's pet id is set.
void petChecks(const wg::CreatureTables &tables) {
    auto pet = [&](int64_t id) {
        wg::CreatureRecord c = wg::newCreature(id);
        const int32_t race = 0x31;
        std::memcpy(&c[wg::creature::RACE], &race, 4);
        c[wg::creature::HOSTILITY] = 6;
        return c;
    };
    wg::CreatureRecord owner = wg::newCreature(1), a = pet(-5), b = pet(-6);
    const wg::CreatureExtras ea = wg::makePet(a.data(), 1, tables, [&](int64_t id) { return id == 1 ? owner.data() : nullptr; });
    const wg::CreatureExtras eb = wg::makePet(b.data(), 1, tables);
    WC_CHECK(std::memcmp(&a[wg::creature::APPEARANCE], &b[wg::creature::APPEARANCE], wg::Appearance::SIZE) == 0);
    WC_CHECK(std::memcmp(&a[wg::creature::EQUIPMENT], &b[wg::creature::EQUIPMENT], 11 * wg::creature::ITEM_SIZE) == 0);
    WC_CHECK(ea.abilities == eb.abilities && field<int64_t>(a.data(), wg::creature::OWNER) == 1);
    WC_CHECK(field<uint32_t>(a.data(), wg::creature::ROAM) == wg::creature::PET_ROAM && field<int64_t>(owner.data(), wg::creature::PET_ID) == -5);
}

} // namespace

// The fast subset in the default test run.
int worldgenCreatureTests() {
    failures = 0;
    residentChecks();
    const fs::path game = gameDir();
    if (game.empty())
        return failures;
    wg::CreatureTables tables;
    if (!loadTables(game, tables))
        return failures;
    const fs::path oracle = extractDir(game) / "oracle";
    gearOracle(oracle / "gear.bin", 3000, false);
    looksOracle(oracle, tables, false);
    petChecks(tables);
    regionsOracle(oracle / "regions.txt", false);
    const Totals t = zonesOracle(extractDir(game) / "gen_seed_123", tables, 6, false, -1);
    WC_CHECK(t.wholeTraces == t.traces);
    WC_CHECK(t.pointsSame == t.pointsGame && t.pointsMine == t.pointsGame);
    return failures;
}

// `EngineTests wgcreatures [zones]`.
int worldgenCreatureCommand(int maxZones) {
    failures = 0;
    residentChecks();
    const fs::path game = gameDir();
    if (game.empty())
        return std::printf("no game folder\n"), 1;
    wg::CreatureTables tables;
    if (!loadTables(game, tables))
        return 1;
    const fs::path oracle = extractDir(game) / "oracle";
    gearOracle(oracle / "gear.bin", 1 << 30, true);
    looksOracle(oracle, tables, true);
    petChecks(tables);
    pickersOracle(oracle, tables);
    regionsOracle(oracle / "regions.txt", true);
    Totals all;
    for (const char *set : {"gen_seed_123", "gen2_seed_123", "genb_seed_123", "genr_seed_123"}) {
        const bool refreshed = std::string(set) == "genr_seed_123";
        int day = 0;
        if (std::ifstream d(extractDir(game) / set / "day.txt"); d)
            d >> day;
        const Totals t = zonesOracle(extractDir(game) / set, tables, maxZones, true, refreshed ? day : -1);
        if (std::string(set) == "gen_seed_123") {
            WC_CHECK(t.wholeTraces == t.traces);
            WC_CHECK(t.itemsSame == t.itemsGame && t.itemsMine == t.itemsGame);
        }
        WC_CHECK(t.pointsSame + t.pointsExcused == t.pointsGame && t.pointsMine == t.pointsGame);
    }
    std::printf(failures ? "%d failure(s)\n" : "wgcreatures: all passed\n", failures);
    return failures ? 1 : 0;
}
