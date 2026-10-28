// The new world generator's terrain (src/worldgen, docs/specs/worldgen-terrain*.md) against the
// game's own outputs (worldgen-terrain.md §4):
//   - Extract/gen-oracle.txt: every value of the height pipeline at ~3800 points;
//   - Extract/gen-blocks.txt: the world values, colours and blocks of columns;
//   - Extract/gen_seed_123: zones the game generated afresh (unlit), their terrain columns, and
//     their .rtrace rand() calls;
//   - Extract/seed_123: zones saved while playing (lit), for the light pass.
// `EngineTests wgterrain [zones]` runs everything with details and timings; the default test run
// takes a fast subset.
#include "Assets.h"
#include "Extracted.h"
#include "worldgen/Generator.h"
#include "worldgen/CellObjects.h"
#include "worldgen/BlockColours.h"
#include "worldgen/Height.h"
#include "worldgen/Places.h"
#include "worldgen/Regions.h"
#include "worldgen/WorldValues.h"
#include "worldgen/ZoneLight.h"
#include "worldgen/ZoneTerrain.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;
#define WG_CHECK(x)                                                                                \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                              \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

namespace wg = worldgen;
namespace fs = std::filesystem;

fs::path gameDir() {
    Assets a;
    return a.locate("") ? a.gameDir() : fs::path();
}

// Lines of an oracle file with '|' as spaces; comments skipped.
template <class F> void eachLine(std::ifstream &in, F f) {
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        for (auto &ch : line)
            if (ch == '|')
                ch = ' ';
        std::istringstream s(line);
        f(s);
    }
}

// --- gen-oracle.txt (§4.1) ------------------------------------------------------------------

int heightOracle(const fs::path &dir, bool verbose) {
    std::ifstream in(dir / "Extract" / "gen-oracle.txt");
    if (!in)
        return 0;
    const char *names[] = {"regionX", "regionY", "kind", "big", "variant", "radius", "cx", "cy", "dist2", "weight",
                           "lake", "base", "roads", "w13", "w125", "w8", "river", "lava", "swamp", "high", "a44",
                           "b59", "veg", "height0", "height4"};
    constexpr int N = 25;
    int wrong[N] = {}, points = 0, shown = 0;
    eachLine(in, [&](std::istringstream &s) {
        int x, y;
        double game[N];
        s >> x >> y;
        for (double &v : game)
            s >> v;
        if (!s)
            return;
        points++;
        const wg::Int2 r = wg::regionAt(x, y);
        const wg::ColumnValues c = wg::columnValues(x, y, 0);
        const wg::Place &p = c.place;
        const double mine[N] = {(double)r.x, (double)r.y, (double)p.kind, (double)p.big, (double)p.variant,
                                (double)p.radius, (double)p.point.x, (double)p.point.y, p.dist2, p.weight, c.lake, c.base,
                                c.roads, c.lavaRegion, c.forestRegion, c.wet, c.river, c.lava, c.swamp, c.highlands,
                                c.dryRegion, c.dryness, c.vegetation, c.height, wg::groundHeight(x, y, 4)};
        for (int i = 0; i < N; i++)
            if ((float)game[i] != (float)mine[i]) {
                wrong[i]++;
                if (verbose && shown++ < 30)
                    std::printf("  %d %d %s: game %.9g, new %.9g\n", x, y, names[i], game[i], mine[i]);
            }
    });
    int total = 0;
    for (int i = 0; i < N; i++) {
        total += wrong[i];
        if (verbose || wrong[i])
            std::printf("wgterrain gen %-8s %d of %d differ\n", names[i], wrong[i], points);
    }
    std::printf("wgterrain gen-oracle: %d points, %d values of %d differ\n", points, total, points * N);
    WG_CHECK(points > 0 && total == 0);
    return total;
}

// --- gen-blocks.txt (§4.2) ------------------------------------------------------------------

int blocksOracle(const fs::path &dir, bool verbose) {
    std::ifstream in(dir / "Extract" / "gen-blocks.txt");
    if (!in)
        return 0;
    std::string header;
    std::getline(in, header);
    int seed = 123;
    if (header.find(" seed ") != std::string::npos)
        seed = std::atoi(header.c_str() + header.find(" seed ") + 6);
    const wg::WorldValues world = wg::worldValues(seed);
    const bool headerOk = header.find("world+0x330 " + std::to_string(world.sandX()) + " world+0x334 " +
                                      std::to_string(world.sandY())) != std::string::npos;
    std::printf("wgterrain world values: seed %d, zone seed %d, sand %d %d (header %s)\n", seed, world.zoneSeed(),
                world.sandX(), world.sandY(), headerOk ? "matches" : "DIFFERS");
    WG_CHECK(headerOk);
    if (seed == 123)
        WG_CHECK(world.zoneSeed() == 15376 && world.sandX() == 10211 && world.sandY() == 5983);
    const wg::SandOffsets sand = wg::sandOffsets(world);
    std::vector<std::string> names = {"warmth", "kind27", "snow", "snowR", "snowG", "snowB", "bedR", "bedG", "bedB", "top"};
    for (int d : {0, 1, 3, 10, 40})
        for (const char *p : {"r", "g", "b", "glow", "type", "flags"})
            names.push_back(std::string(p) + std::to_string(d));
    const size_t N = names.size();
    std::vector<int> wrong(N);
    int points = 0, shown = 0;
    eachLine(in, [&](std::istringstream &s) {
        int x, y;
        double h[5];
        std::vector<double> game(N);
        s >> x >> y;
        for (double &v : h)
            s >> v;
        for (double &v : game)
            s >> v;
        if (!s)
            return;
        points++;
        const wg::ColumnValues c = wg::columnValues(x, y, 0);
        const wg::ColourInputs in = wg::colourInputs(c);
        const wg::NeighbourHeights nb{(float)h[1], (float)h[2], (float)h[3], (float)h[4]};
        const int32_t fz = wg::floorToInt(c.height);
        const wg::BlockProps bed = wg::riverBedColour(x, y, fz - 1, sand);
        std::vector<double> mine = {in.warmth, in.kind27, wg::snowAt(x, y, in.warmth, nb) ? 1.0 : 0.0,
                                    wg::SNOW_BLOCK.r, wg::SNOW_BLOCK.g, wg::SNOW_BLOCK.b, bed.r, bed.g, bed.b,
                                    wg::waterLevel(c.base, c.place)};
        for (int d : {0, 1, 3, 10, 40}) {
            const wg::BlockProps p = wg::blockAt(x, y, fz - d, c.height, nb, in, sand);
            for (double v : {(double)p.r, (double)p.g, (double)p.b, (double)p.glow, (double)p.type, (double)p.flags})
                mine.push_back(v);
        }
        for (size_t i = 0; i < N; i++)
            if ((float)game[i] != (float)mine[i]) {
                wrong[i]++;
                if (verbose && shown++ < 30)
                    std::printf("  %d %d %s: game %.9g, new %.9g\n", x, y, names[i].c_str(), game[i], mine[i]);
            }
    });
    int total = 0;
    for (size_t i = 0; i < N; i++) {
        total += wrong[i];
        if (verbose && wrong[i])
            std::printf("wgterrain blocks %-7s %d of %d differ\n", names[i].c_str(), wrong[i], points);
    }
    std::printf("wgterrain gen-blocks: %d points, %d values of %d differ\n", points, total, points * (int)N);
    WG_CHECK(points > 0 && total == 0);
    return total;
}

// --- Zones (§4.3) ---------------------------------------------------------------------------

struct GameZone {
    int32_t zx = 0, zy = 0, region = 0;
    std::vector<wg::TerrainColumn> columns;
};

bool readZone(const fs::path &file, GameZone &z) {
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    if (data.size() < 20 || std::memcmp(data.data(), "CWZ1", 4) != 0)
        return false;
    std::memcpy(&z.zx, &data[4], 4), std::memcpy(&z.zy, &data[8], 4), std::memcpy(&z.region, &data[12], 4);
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
    return true;
}

std::vector<fs::path> zoneFiles(const fs::path &dir, int maxZones) {
    std::vector<fs::path> files;
    if (!fs::exists(dir))
        return files;
    for (auto &e : fs::directory_iterator(dir)) {
        int ax, ay;
        if (e.path().extension() != ".cwz" || !ExtractedWorld::zoneCoordinates(e.path(), ax, ay))
            continue;
        if (std::abs((long long)ax) > 100000 || std::abs((long long)ay) > 100000)
            continue; // not vanilla (see genZonesMatchExtracted)
        files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());
    if ((int)files.size() > maxZones)
        files.resize((size_t)maxZones);
    return files;
}

// Block types the terrain makes; any other is a structure's.
bool terrainType(int t) {
    return t == 0 || t == 2 || t == 3 || t == 4 || t == 5 || t == 6 || t == 10 || t == 13 || t == 14 || t == 17 ||
           t == 18 || t == 19;
}
// A block the terrain could have made: one of its types, with the glow and flags it gives them
// (lava and poison glow; snow has flag 1; an overhang light's spot is glowing air). Anything
// else is a structure's.
bool terrainBlock(const wg::Block &b) {
    if (!terrainType(b.type) || b.flags > 1)
        return false;
    return b.glow == 0 || (b.glow == 255 && (b.type == 17 || b.type == 19 || b.type == 0));
}

wg::Block blockOf(const wg::TerrainColumn &c, int32_t z) {
    return z >= c.base && z < c.top() ? c.blocks[(size_t)(z - c.base)] : wg::EMPTY_BLOCK;
}

struct ColumnTally {
    int columns = 0, exact = 0, sameBase = 0;
    long long blocks = 0, blocksSame = 0;
    void add(const wg::TerrainColumn &game, const wg::TerrainColumn &ours) {
        columns++;
        sameBase += game.base == ours.base;
        const int32_t lo = std::min(game.base, ours.base), hi = std::max(game.top(), ours.top());
        bool same = true;
        for (int32_t z = lo; z < hi; z++) {
            const bool eq = blockOf(game, z) == blockOf(ours, z);
            blocks++, blocksSame += eq;
            same = same && eq;
        }
        exact += same;
    }
};

// The game's rand() calls of a zone (.rtrace): caller RVA and value.
std::vector<std::pair<uint32_t, uint16_t>> readTrace(const fs::path &file) {
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    std::vector<std::pair<uint32_t, uint16_t>> out(data.size() / 6);
    for (size_t i = 0; i < out.size(); i++)
        std::memcpy(&out[i].first, &data[i * 6], 4), std::memcpy(&out[i].second, &data[i * 6 + 4], 2);
    return out;
}
// The terrain's own draws: the cliff pass of 0x2ff6f0 (zone spec §5.6).
bool terrainCaller(uint32_t rva) { return rva >= 0x301a0c && rva < 0x302254; }

int zonesOracle(const fs::path &dir, int maxZones, bool verbose, bool timing) {
    const std::vector<fs::path> files = zoneFiles(dir, maxZones);
    if (files.empty())
        return 0;
    const std::string name = dir.filename().string();
    const int seed = std::atoi(name.substr(name.find_last_of('_') + 1).c_str());
    const wg::WorldValues world = wg::worldValues(seed);
    wg::TerrainOptions options;
    options.anchor = wg::terrainAnchor; // part B's structure anchor (worldgen-structures.md §6.2)
    options.recordDraws = true;
    ColumnTally plain, inPlace, all;
    int zones = 0, regionsWrong = 0, built = 0, traces = 0, tracesSame = 0, tracesPrefix = 0;
    long long gameDraws = 0, ourDraws = 0;
    std::map<int, std::array<int, 2>> byKind; // natural columns in places by kind: exact, all
    std::vector<std::pair<double, std::string>> zoneRates;
    double newSeconds = 0;
    std::vector<GameZone> games;
    for (const fs::path &file : files) {
        GameZone g;
        if (!readZone(file, g))
            continue;
        zones++;
        const wg::Int2 r = wg::regionAt(g.zx * 64 + 32, g.zy * 64 + 32);
        regionsWrong += wg::regionBiome(r.x, r.y) != g.region;
        const auto t0 = std::chrono::steady_clock::now();
        const wg::ZoneTerrain ours = wg::zoneTerrain(world, g.zx, g.zy, options);
        newSeconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        int zoneNatural = 0, zoneExact = 0;
        for (int i = 0; i < wg::ZONE_COLUMNS; i++) {
            const wg::TerrainColumn &gc = g.columns[(size_t)i], &oc = ours.columns[(size_t)i];
            bool structure = false;
            for (const wg::Block &b : gc.blocks)
                structure = structure || !terrainBlock(b);
            if (structure) {
                built++;
                continue;
            }
            const wg::Place &p = ours.field[(size_t)i].place;
            const bool place = p.weight > 0.0f;
            ColumnTally &t = place ? inPlace : plain;
            const int before = t.exact;
            t.add(gc, oc);
            all.add(gc, oc);
            const bool same = t.exact > before;
            zoneNatural++, zoneExact += same;
            if (place) {
                byKind[p.kind][0] += same;
                byKind[p.kind][1]++;
            }
            static int shownColumns = 0;
            static const int showLimit = std::getenv("WG_SHOW") ? std::atoi(std::getenv("WG_SHOW")) : 10;
            if (verbose && !same && (!place || std::getenv("WG_SHOW")) && shownColumns++ < showLimit) {
                const int32_t x = g.zx * 64 + i / 64, y = g.zy * 64 + i % 64;
                std::printf("  %s column %d %d differs (game base %d, %zu blocks; new base %d, %zu blocks)\n",
                            place ? "place" : "plain", x, y,
                            gc.base, gc.blocks.size(), oc.base, oc.blocks.size());
                const int32_t lo = std::min(gc.base, oc.base), hi = std::max(gc.top(), oc.top());
                for (int32_t z = lo; z < hi; z++) {
                    const wg::Block a = blockOf(gc, z), b = blockOf(oc, z);
                    if (a != b)
                        std::printf("    z %d: game %d %d %d %d %d %d, new %d %d %d %d %d %d\n", z, a.r, a.g, a.b, a.glow,
                                    a.type, a.flags, b.r, b.g, b.b, b.glow, b.type, b.flags);
                }
            }
        }
        char label[64];
        std::snprintf(label, sizeof label, "%d %d (place kind %d)", g.zx, g.zy, ours.place.kind);
        zoneRates.push_back({zoneNatural ? 100.0 * zoneExact / zoneNatural : 100.0, label});
        // Its rand() calls.
        fs::path trace = file;
        trace.replace_extension(".rtrace");
        if (fs::exists(trace)) {
            const auto calls = readTrace(trace);
            std::vector<uint16_t> game;
            size_t lastTerrain = 0, firstOther = calls.size();
            for (size_t k = 0; k < calls.size(); k++) {
                if (terrainCaller(calls[k].first))
                    game.push_back(calls[k].second), lastTerrain = k + 1;
                else if (firstOther == calls.size())
                    firstOther = k;
            }
            traces++;
            gameDraws += (long long)game.size(), ourDraws += (long long)ours.draws.size();
            tracesSame += game == ours.draws;
            tracesPrefix += lastTerrain <= firstOther; // the terrain's draws all come first
        }
    }
    std::printf("wgterrain zones %d (%s): region types %d wrong; %d columns with structures\n", zones, name.c_str(),
                regionsWrong, built);
    std::printf("  natural columns outside places: %d of %d exact (%.3f%%), base the same %d; blocks %lld of %lld\n",
                plain.exact, plain.columns, plain.columns ? 100.0 * plain.exact / plain.columns : 0.0, plain.sameBase,
                plain.blocksSame, plain.blocks);
    std::printf("  natural columns in places: %d of %d exact (%.3f%%), base the same %d; blocks %lld of %lld\n",
                inPlace.exact, inPlace.columns, inPlace.columns ? 100.0 * inPlace.exact / inPlace.columns : 0.0,
                inPlace.sameBase, inPlace.blocksSame, inPlace.blocks);
    std::printf("  all natural columns: %d of %d exact (%.3f%%)\n", all.exact, all.columns,
                all.columns ? 100.0 * all.exact / all.columns : 0.0);
    if (traces)
        std::printf("  rand(): %d traces, %d with the terrain's draws the same (game %lld, new %lld draws), %d with "
                    "them all before part B's\n",
                    traces, tracesSame, gameDraws, ourDraws, tracesPrefix);
    if (verbose) {
        for (auto [k, n] : byKind)
            if (n[0] != n[1])
                std::printf("  place kind %d: %d of %d natural columns exact\n", k, n[0], n[1]);
        std::sort(zoneRates.begin(), zoneRates.end());
        for (size_t k = 0; k < zoneRates.size() && zoneRates[k].first < 100.0 && k < 40; k++)
            std::printf("  zone %s: %.2f%% of natural columns exact\n", zoneRates[k].second.c_str(), zoneRates[k].first);
    }
    if (timing) {
        std::printf("  time, new terrain: %.3f s for %d zones (%.1f ms a zone)\n", newSeconds, zones,
                    1000.0 * newSeconds / std::max(zones, 1));
        // Lit zones, each made with its 3 x 3 unlit neighbours from scratch (no cache between zones):
        // the terrain alone, and as the engine loads a zone (worldgen::litZone: terrain and
        // structures of all nine).
        worldgen::setWorldSeed(seed);
        const size_t litCount = std::min<size_t>(files.size(), 10);
        double newLit = 0, engineLit = 0;
        for (size_t k = 0; k < litCount; k++) {
            int ax, ay;
            ExtractedWorld::zoneCoordinates(files[k], ax, ay);
            auto t1 = std::chrono::steady_clock::now();
            {
                std::map<std::pair<int32_t, int32_t>, std::vector<wg::TerrainColumn>> unlit;
                (void)wg::lightZone(ax, ay, [&](int32_t zx, int32_t zy) -> const std::vector<wg::TerrainColumn> & {
                    auto &v = unlit[{zx, zy}];
                    if (v.empty())
                        v = wg::zoneTerrain(world, zx, zy).columns;
                    return v;
                });
            }
            auto t2 = std::chrono::steady_clock::now();
            worldgen::forgetZones();
            (void)worldgen::litZone(ax, ay);
            auto t3 = std::chrono::steady_clock::now();
            newLit += std::chrono::duration<double>(t2 - t1).count();
            engineLit += std::chrono::duration<double>(t3 - t2).count();
        }
        worldgen::forgetZones();
        std::printf("  time, a lit zone with its 9 unlit ones: terrain %.1f ms, with structures (worldgen::litZone) %.1f ms (over %zu zones)\n",
                    1000.0 * newLit / std::max<size_t>(litCount, 1), 1000.0 * engineLit / std::max<size_t>(litCount, 1), litCount);
    }
    WG_CHECK(regionsWrong == 0);
    WG_CHECK(all.columns > 0 && plain.exact == plain.columns);
    if (traces)
        WG_CHECK(tracesSame == traces);
    return plain.columns - plain.exact;
}

// The light pass against zones saved while playing (lit), on the columns where neither the
// column nor any within the light's reach holds a structure.
int lightOracle(const fs::path &dir, int maxZones) {
    const std::vector<fs::path> files = zoneFiles(dir, maxZones);
    if (files.empty())
        return 0;
    const std::string name = dir.filename().string();
    const wg::WorldValues world = wg::worldValues(std::atoi(name.substr(name.find_last_of('_') + 1).c_str()));
    std::map<std::pair<int32_t, int32_t>, std::vector<wg::TerrainColumn>> unlit;
    auto source = [&](int32_t zx, int32_t zy) -> const std::vector<wg::TerrainColumn> & {
        auto &z = unlit[{zx, zy}];
        if (z.empty())
            z = wg::zoneTerrain(world, zx, zy).columns;
        return z;
    };
    ColumnTally tally;
    int unlitDiffer = 0;
    long long water = 0;
    for (const fs::path &file : files) {
        GameZone g;
        if (!readZone(file, g))
            continue;
        if (unlit.size() > 200)
            unlit.clear();
        const std::vector<wg::TerrainColumn> lit = wg::lightZone(g.zx, g.zy, source);
        const std::vector<wg::TerrainColumn> &plainOurs = source(g.zx, g.zy);
        const std::vector<wg::TerrainColumn> &game = g.columns;
        for (auto &c : game)
            for (auto &k : c.blocks)
                water += k.type == wg::blocktype::WATER;
        // Columns the same as ours but for the light: the terrain alone, no structure.
        auto unlitSame = [](wg::TerrainColumn a, wg::TerrainColumn b) {
            for (auto *c : {&a, &b})
                for (auto &k : c->blocks)
                    if (wg::isOpen(k))
                        k.r = 0;
            ColumnTally t;
            t.add(a, b);
            return t.exact == 1;
        };
        std::vector<char> natural(wg::ZONE_COLUMNS);
        for (int i = 0; i < wg::ZONE_COLUMNS; i++)
            natural[(size_t)i] = unlitSame(game[(size_t)i], plainOurs[(size_t)i]);
        // Every column whose light can reach (8 columns each way) is such a column.
        constexpr int REACH = 8;
        for (int lx = REACH; lx < 64 - REACH; lx++)
            for (int ly = REACH; ly < 64 - REACH; ly++) {
                bool ok = true;
                for (int dx = -REACH; dx <= REACH && ok; dx++)
                    for (int dy = -REACH; dy <= REACH && ok; dy++)
                        ok = natural[(size_t)wg::ZoneTerrain::index(lx + dx, ly + dy)];
                if (!ok)
                    continue;
                const int i = wg::ZoneTerrain::index(lx, ly);
                const wg::TerrainColumn &gc = game[(size_t)i];
                const int before = tally.exact;
                tally.add(gc, lit[(size_t)i]);
                static int shown = 0;
                if (tally.exact == before && std::getenv("WG_SHOW") && shown++ < std::atoi(std::getenv("WG_SHOW"))) {
                    std::printf("  column %d %d differs\n", g.zx * 64 + lx, g.zy * 64 + ly);
                    for (int32_t z = gc.base; z < gc.top(); z++) {
                        const wg::Block p = blockOf(gc, z), q = blockOf(lit[(size_t)i], z);
                        if (p != q)
                            std::printf("    z %d: game %d %d %d %d %d %d, new %d %d %d %d %d %d\n", z, p.r, p.g, p.b, p.glow,
                                        p.type, p.flags, q.r, q.g, q.b, q.glow, q.type, q.flags);
                    }
                }
            }
        for (int i = 0; i < wg::ZONE_COLUMNS; i++)
            unlitDiffer += !natural[(size_t)i];
    }
    std::printf("wglight %s: %zu zones; %d of %d columns whose light comes from the terrain alone are exact lit "
                "(%.3f%%), blocks %lld of %lld; (%d columns differ unlit; %lld water blocks)\n",
                name.c_str(), files.size(), tally.exact, tally.columns, tally.columns ? 100.0 * tally.exact / tally.columns : 0.0,
                tally.blocksSame, tally.blocks, unlitDiffer, water);
    return tally.columns - tally.exact;
}

fs::path generatedZonesDir(const fs::path &game) {
    if (const char *d = std::getenv("CW_ZONES_DIR"); d && *d)
        return d;
    return game / "Extract" / "gen_seed_123";
}

} // namespace

// The fast subset in the default test run.
int worldgenTerrainTests() {
    failures = 0;
    const fs::path game = gameDir();
    if (game.empty())
        return 0;
    heightOracle(game, false);
    blocksOracle(game, false);
    zonesOracle(generatedZonesDir(game), 25, false, false);
    return failures;
}

// `EngineTests wgterrain [zones]`: everything, with details and timings.
int worldgenTerrainCommand(int maxZones) {
    failures = 0;
    const fs::path game = gameDir();
    if (game.empty())
        return std::printf("no game folder\n"), 1;
    heightOracle(game, true);
    blocksOracle(game, true);
    zonesOracle(generatedZonesDir(game), maxZones, true, true);
    std::printf(failures ? "%d failure(s)\n" : "wgterrain: all passed\n", failures);
    return failures ? 1 : 0;
}

// `EngineTests wglight [zones]`: the light pass against Extract/seed_123.
int worldgenLightCommand(int maxZones) {
    const fs::path game = gameDir();
    if (game.empty())
        return 1;
    lightOracle(game / "Extract" / "seed_123", maxZones);
    return 0;
}

// Debug: the lit and unlit blocks around (x, y) at height z, ours and the game's (seed_123).
int worldgenLightProbe(int32_t x, int32_t y, int32_t z) {
    const fs::path game = gameDir();
    const wg::WorldValues world = wg::worldValues(123);
    std::map<std::pair<int32_t, int32_t>, std::vector<wg::TerrainColumn>> unlit;
    auto source = [&](int32_t zx, int32_t zy) -> const std::vector<wg::TerrainColumn> & {
        auto &v = unlit[{zx, zy}];
        if (v.empty())
            v = wg::zoneTerrain(world, zx, zy).columns;
        return v;
    };
    const int32_t zx = wg::floorDiv(x, 6), zy = wg::floorDiv(y, 6);
    const std::vector<wg::TerrainColumn> lit = wg::lightZone(zx, zy, source);
    GameZone g;
    readZone(game / "Extract" / "seed_123" / ("z_" + std::to_string(zx) + "_" + std::to_string(zy) + ".cwz"), g);
    for (int dy = 5; dy >= -5; dy--) {
        for (int dx = -5; dx <= 5; dx++) {
            const int32_t lx = x + dx - zx * 64, ly = y + dy - zy * 64;
            if (lx < 0 || ly < 0 || lx > 63 || ly > 63) {
                std::printf("  ----------- ");
                continue;
            }
            const int i = wg::ZoneTerrain::index(lx, ly);
            const wg::Block o = blockOf(lit[(size_t)i], z), u = blockOf(source(zx, zy)[(size_t)i], z);
            const wg::Block gb = g.columns.empty() ? wg::Block{} : blockOf(g.columns[(size_t)i], z);
            std::printf(" %2d:%3d/%3d%s", u.type, o.r, gb.r, z >= lit[(size_t)i].top() ? "^" : " ");
        }
        std::printf("\n");
    }
    return 0;
}
