#include "Extracted.h"
#include "AlphaLight.h"
#include "AlphaLook.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
// cube::Block::Type values that are not drawn as solid blocks.
constexpr uint8_t AIR_TYPE = 0, WATER_TYPE = 2, WET_TYPE = 3;
constexpr uint8_t LEAVES_TYPE = 8, SNOW_LEAVES_TYPE = 11, LAVA_TYPE = 17, POISON_TYPE = 19;
constexpr uint8_t DRY_AIR = 2; // breakable bit: air that does not become water below sea level.
// A zone model record's flags (0x40-byte records, worldgen-structures.md §4.2).
constexpr size_t MODEL_FLAGS = 0x38;

bool solidType(uint8_t t) { return t != AIR_TYPE && t != WATER_TYPE && t != LAVA_TYPE && t != POISON_TYPE; }

// Game region type -> CubeVulkan Biome.
int biomeOf(int regionType) {
    switch (regionType) {
    case 1: return OCEAN;
    case 2: return HILLS;
    case 3: return WOODLANDS;
    case 5: return DARKWOODS;
    case 7: return MOUNTAINS;
    case 8: return WETLANDS;
    case 9: return DESERT;
    case 11: return SNOWLANDS;
    case 12: return DEADLANDS;
    default: return PLAINS;
    }
}
int64_t floorDiv(int64_t a, int64_t b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }
} // namespace

fs::path ExtractedWorld::find(const fs::path &gameDir) {
    std::error_code ec;
    fs::path best;
    fs::file_time_type newest{};
    for (auto &e : fs::directory_iterator(gameDir / "Extract", ec))
        if (e.is_directory() && e.path().filename().string().rfind("seed_", 0) == 0) {
            auto t = fs::last_write_time(e.path(), ec);
            if (best.empty() || t > newest) {
                best = e.path();
                newest = t;
            }
        }
    return best;
}

void ExtractedWorld::openGenerated(int homeX, int homeY, const fs::path &spriteDir) {
    zones.clear();
    environment.clear();
    buildingParts.clear();
    measuredScale.clear();
    {
        std::lock_guard lock(cacheMutex);
        cache.clear();
        absent.clear();
        estimatedScale.clear();
    }
    folder.clear();
    if (!spriteDir.empty())
        loadSpriteTables(spriteDir);
    generatedMode = true;
    seedValue = 0;
    originX = (int)floorDiv(homeX, 64);
    originY = (int)floorDiv(homeY, 64);
    float ground = std::floor(worldgen::groundHeight(homeX, homeY)) + 1;
    homeLocal = glm::vec3(float(homeX - (int64_t)originX * 64), float(homeY - (int64_t)originY * 64),
                          std::max(ground, 1.f) + Z_OFFSET);
}

// Sprite tables written by the extractor next to the seed folders.
void ExtractedWorld::loadSpriteTables(const fs::path &dir) {
    std::string line;
    std::ifstream groups(dir / "sprite-groups.txt");
    std::vector<std::string> *table = nullptr;
    while (std::getline(groups, line)) {
        if (line.rfind("# environment", 0) == 0)
            table = &environment;
        else if (line.rfind("# building-part", 0) == 0)
            table = &buildingParts;
        else if (line.rfind("#", 0) == 0)
            table = nullptr;
        else if (table) {
            std::istringstream row(line);
            int id; std::string name;
            if (!(row >> id >> name) || id < 0 || id > 65535) continue;
            if (name.rfind("sprites/", 0) == 0)
                name = name.substr(8);
            if (name.size() > 4 && name.substr(name.size() - 4) == ".cub")
                name.resize(name.size() - 4);
            if ((size_t)id >= table->size()) table->resize((size_t)id + 1);
            (*table)[id] = name == "?" ? std::string() : name;
        }
    }
    // Per-sprite sizes measured in game ("slot<TAB>sprites/name.cub<TAB>scale<TAB>samples").
    std::ifstream scales(dir / "sprite-scales.txt");
    while (std::getline(scales, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        size_t a = line.find('\t'), b = a == std::string::npos ? a : line.find('\t', a + 1);
        if (b == std::string::npos)
            continue;
        std::string name = line.substr(a + 1, b - a - 1);
        if (name.rfind("sprites/", 0) == 0)
            name = name.substr(8);
        if (name.size() > 4 && name.substr(name.size() - 4) == ".cub")
            name.resize(name.size() - 4);
        float value = std::strtof(line.c_str() + b + 1, nullptr);
        if (value > 0 && value < 4)
            measuredScale[name] = value;
    }
}

std::shared_ptr<ExtractedWorld::Zone> ExtractedWorld::generateZone(int ax, int ay) {
    auto zone = std::make_shared<Zone>();
    worldgen::Int2 r = worldgen::regionAt(ax * 64 + 32, ay * 64 + 32);
    zone->region = worldgen::regionBiome(r.x, r.y);
    zone->generated = true;
    zone->things = worldgen::zone(ax, ay);
    zone->objects = worldgen::zoneObjects(ax, ay);
    zone->objectsMade = true;
    const std::vector<worldgen::TerrainColumn> columns = worldgen::litZone(ax, ay);
    for (size_t i = 0; i < columns.size(); i++) {
        Column &c = zone->columns[i];
        c.baseZ = columns[i].base;
        c.blocks.reserve(columns[i].blocks.size());
        for (const worldgen::Block &b : columns[i].blocks)
            c.blocks.push_back({b.r, b.g, b.b, b.glow, b.type, b.flags});
    }
    return zone;
}

bool ExtractedWorld::open(const fs::path &dir, std::string &error) {
    error.clear();
    generatedMode = false;
    zones.clear();
    environment.clear();
    buildingParts.clear();
    measuredScale.clear();
    originX = originY = seedValue = 0;
    homeLocal = glm::vec3(0);
    {
        std::lock_guard lock(cacheMutex);
        cache.clear();
        absent.clear();
        estimatedScale.clear();
    }
    folder = dir;
    std::error_code ec;
    for (auto &e : fs::directory_iterator(dir, ec)) {
        int x, y;
        if (e.is_regular_file(ec) && zoneCoordinates(e.path(), x, y))
            zones.insert({x, y});
    }
    if (zones.empty()) {
        error = "no extracted zones in " + dir.string();
        return false;
    }
    long long hx = 0, hy = 0, hz = 0;
    std::ifstream manifest(dir / "manifest.txt");
    std::string line;
    bool haveSeed = false, haveX = false, haveY = false, haveZ = false;
    while (std::getline(manifest, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto number = [&](const char *key, auto &value, bool &found) {
            size_t n = std::strlen(key);
            if (line.compare(0, n, key) != 0) return;
            auto result = std::from_chars(line.data() + n, line.data() + line.size(), value);
            found = result.ec == std::errc() && result.ptr == line.data() + line.size();
        };
        number("seed=", seedValue, haveSeed);
        number("homeX=", hx, haveX);
        number("homeY=", hy, haveY);
        number("homeZ=", hz, haveZ);
    }
    if (!haveSeed || !haveX || !haveY || !haveZ || floorDiv(hx, 64) < INT32_MIN ||
        floorDiv(hx, 64) > INT32_MAX || floorDiv(hy, 64) < INT32_MIN || floorDiv(hy, 64) > INT32_MAX ||
        hz < INT32_MIN || hz > INT32_MAX - Z_OFFSET) {
        zones.clear();
        error = "missing or invalid extraction manifest in " + dir.string();
        return false;
    }
    loadSpriteTables(dir.parent_path());
    originX = (int)floorDiv(hx, 64);
    originY = (int)floorDiv(hy, 64);
    homeLocal = glm::vec3(float(hx - (int64_t)originX * 64), float(hy - (int64_t)originY * 64), float(hz + Z_OFFSET));
    if (!load(originX, originY)) {
        zones.clear();
        error = "the extraction's home zone is missing or damaged; re-extract it in Cube World";
        return false;
    }
    return true;
}

bool ExtractedWorld::zoneCoordinates(const fs::path &file, int &x, int &y) {
    std::string name = file.filename().string();
    if (name.rfind("z_", 0) != 0 || file.extension() != ".cwz") return false;
    const char *end = name.data() + name.size() - 4;
    auto first = std::from_chars(name.data() + 2, end, x);
    if (first.ec != std::errc() || first.ptr == end || *first.ptr != '_') return false;
    auto second = std::from_chars(first.ptr + 1, end, y);
    return second.ec == std::errc() && second.ptr == end &&
           name == "z_" + std::to_string(x) + "_" + std::to_string(y) + ".cwz";
}

bool ExtractedWorld::validateZoneFile(const fs::path &file, std::string &error) {
    return bool(readZone(file, false, error));
}

std::shared_ptr<ExtractedWorld::Zone> ExtractedWorld::load(int ax, int ay) {
    Key key{ax, ay};
    {
        std::lock_guard lock(cacheMutex);
        for (auto it = cache.begin(); it != cache.end(); ++it)
            if (it->first == key) {
                cache.splice(cache.begin(), cache, it); // Most recent first.
                return cache.front().second;
            }
    }
    // A zone the generator's hooks (a mod's) change is made anew, as the game makes every zone
    // when it loads; the others are as extracted.
    const bool generate = generatedMode || worldgen::hooksChange(ax, ay);
    if (!generate && !zones.count(key))
        return nullptr;
    std::string error;
    auto zone = generate ? generateZone(ax, ay)
                              : readZone(folder / ("z_" + std::to_string(ax) + "_" + std::to_string(ay) + ".cwz"), true, error);
    if (!zone) return nullptr;
    applyBlockChanges(ax, ay, *zone);
    std::lock_guard lock(cacheMutex);
    cache.emplace_front(key, zone);
    // The least recently used zone goes, unless it is one kept around the player.
    for (auto it = cache.end(); cache.size() > 96 && it != cache.begin();) {
        --it;
        if (!kept(it->first))
            it = cache.erase(it);
    }
    return zone;
}

bool ExtractedWorld::kept(const Key &key) const {
    for (const auto &[owner, around] : keptBy)
        for (const Key &centre : around.centres)
            if (std::abs(key.first - centre.first) <= around.radius && std::abs(key.second - centre.second) <= around.radius)
                return true;
    return false;
}

ExtractedWorld::~ExtractedWorld() {
    if (keeper.joinable())
        keeper.join();
}

void ExtractedWorld::keepAround(int ax, int ay, int radius) { keepAround(nullptr, {{ax, ay}}, radius); }

void ExtractedWorld::keepAround(const void *owner, const std::vector<std::pair<int, int>> &centres, int radius) {
    {
        std::lock_guard lock(cacheMutex);
        keptBy[owner] = {centres, radius};
    }
    if (keeping)
        return; // asked again next frame
    std::vector<Key> missing;
    std::set<Key> seen;
    for (const Key &centre : centres)
        for (int x = centre.first - radius; x <= centre.first + radius; x++)
            for (int y = centre.second - radius; y <= centre.second + radius; y++)
                if (seen.insert({x, y}).second && !cached(x, y)) {
                    std::lock_guard lock(cacheMutex);
                    if (!absent.count({x, y}))
                        missing.push_back({x, y});
                }
    if (missing.empty())
        return;
    if (keeper.joinable())
        keeper.join();
    keeping = true;
    keeper = std::thread([this, missing = std::move(missing)] {
        for (const Key &key : missing)
            if (!load(key.first, key.second)) {
                std::lock_guard lock(cacheMutex);
                absent.insert(key);
            }
        keeping = false;
    });
}

std::shared_ptr<ExtractedWorld::Zone> ExtractedWorld::cached(int ax, int ay) {
    std::lock_guard lock(cacheMutex);
    for (auto &[key, zone] : cache)
        if (key.first == ax && key.second == ay)
            return zone;
    return nullptr;
}

ExtractedWorld::GameBlock ExtractedWorld::gameBlock(int64_t x, int64_t y, int z, bool *loaded) {
    static const GameBlock stone{0xc8, 0xc8, 0xc8, 0, 1, 0}, air{0xff, 0xff, 0xff, 0, 0, 0}, water{0xff, 0xff, 0xff, 0, 2, 0};
    const int ax = (int)floorDiv(x, 64), ay = (int)floorDiv(y, 64);
    // The last few zones asked for, so a creature's many lookups skip the cache's lock.
    thread_local struct {
        const ExtractedWorld *world = nullptr;
        int ax = 0, ay = 0;
        uint32_t revision = 0;
        std::shared_ptr<Zone> zone;
    } memo[4];
    thread_local int next = 0;
    std::shared_ptr<Zone> zone;
    const uint32_t revision = blocksRevision.load();
    for (auto &m : memo)
        if (m.world == this && m.ax == ax && m.ay == ay && m.revision == revision && m.zone)
            zone = m.zone;
    if (!zone) {
        zone = cached(ax, ay);
        if (zone) {
            memo[next] = {this, ax, ay, revision, zone};
            next = (next + 1) % 4;
        }
    }
    if (loaded)
        *loaded = zone != nullptr;
    if (!zone)
        return stone;
    const Column &c = zone->columns[(x - (int64_t)ax * 64) * 64 + (y - (int64_t)ay * 64)];
    if (z < c.baseZ)
        return stone;
    const size_t i = (size_t)(z - c.baseZ);
    if (i < c.blocks.size()) {
        const Block &b = c.blocks[i];
        if (b.type != 0 || z > 0 || (b.breakable & 2))
            return {b.r, b.g, b.b, b.field3, b.type, b.breakable};
    } else if (z > 0) {
        return air;
    }
    return water;
}

void ExtractedWorld::applyBlockChanges(int ax, int ay, Zone &zone) const {
    std::vector<nativesaves::BlockChange> changes;
    {
        std::lock_guard lock(changesMutex);
        auto it = blockChanges.find({ax, ay});
        if (it == blockChanges.end())
            return;
        changes = it->second;
    }
    static const Block STONE{0xc8, 0xc8, 0xc8, 0, 1, 0}, OPEN{0xff, 0xff, 0xff, 0, 0, 0};
    for (const nativesaves::BlockChange &change : changes) {
        const std::array<int32_t, 3> at = change.block();
        const int64_t lx = at[0] - (int64_t)ax * 64, ly = at[1] - (int64_t)ay * 64;
        if (lx < 0 || lx >= 64 || ly < 0 || ly >= 64)
            continue; // (a change kept under another zone)
        // The column grows as needed: stone below its base and open blocks above its top, as
        // gameBlock reads past either end.
        Column &c = zone.columns[lx * 64 + ly];
        const int z = at[2];
        if (c.blocks.empty())
            c.baseZ = z;
        if (z < c.baseZ) {
            c.blocks.insert(c.blocks.begin(), (size_t)(c.baseZ - z), STONE);
            c.baseZ = z;
        }
        const size_t i = (size_t)(z - c.baseZ);
        if (i >= c.blocks.size())
            c.blocks.resize(i + 1, OPEN);
        const std::array<uint8_t, 6> now = change.now();
        c.blocks[i] = {now[0], now[1], now[2], now[3], now[4], now[5]};
    }
}

void ExtractedWorld::undoBlockChanges(int ax, int ay) {
    std::vector<nativesaves::BlockChange> changes;
    {
        std::lock_guard lock(changesMutex);
        auto it = blockChanges.find({ax, ay});
        if (it == blockChanges.end() || it->second.empty())
            return;
        changes = std::move(it->second);
        blockChanges.erase(it);
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++)
                changedZones.insert({ax + dx, ay + dy});
    }
    std::shared_ptr<Zone> old = cached(ax, ay);
    if (!old)
        return;
    // Each change's "was" block goes back, last change first, into a copy of the zone.
    auto fresh = std::make_shared<Zone>(*old);
    for (auto change = changes.rbegin(); change != changes.rend(); ++change) {
        const std::array<int32_t, 3> at = change->block();
        const int64_t lx = at[0] - (int64_t)ax * 64, ly = at[1] - (int64_t)ay * 64;
        if (lx < 0 || lx >= 64 || ly < 0 || ly >= 64)
            continue;
        Column &c = fresh->columns[lx * 64 + ly];
        const int64_t i = (int64_t)at[2] - c.baseZ;
        if (c.blocks.empty() || i < 0 || i >= (int64_t)c.blocks.size())
            continue; // (outside the column: it holds what the change found there anyway)
        const std::array<uint8_t, 6> was = change->was();
        c.blocks[(size_t)i] = {was[0], was[1], was[2], was[3], was[4], was[5]};
    }
    {
        std::lock_guard lock(cacheMutex);
        for (auto &[key, zone] : cache)
            if (key.first == ax && key.second == ay)
                zone = fresh;
    }
    blocksRevision++;
}

void ExtractedWorld::setBlockChanges(int ax, int ay, const std::vector<nativesaves::BlockChange> &changes) {
    {
        std::lock_guard lock(changesMutex);
        blockChanges[{ax, ay}] = changes;
        // The zone's mesh and its neighbours' (their faces against it) are stale.
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++)
                changedZones.insert({ax + dx, ay + dy});
    }
    // The zone in memory is replaced by a copy with the changes, so a thread reading the old
    // copy is not disturbed. (Putting a change in twice gives the same block, so the copy takes
    // the whole list again.)
    std::shared_ptr<Zone> old = cached(ax, ay);
    if (!old)
        return;
    auto fresh = std::make_shared<Zone>(*old);
    applyBlockChanges(ax, ay, *fresh);
    {
        std::lock_guard lock(cacheMutex);
        for (auto &[key, zone] : cache)
            if (key.first == ax && key.second == ay)
                zone = fresh;
    }
    blocksRevision++;
}

std::vector<std::pair<int, int>> ExtractedWorld::takeChangedZones() {
    std::lock_guard lock(changesMutex);
    std::vector<std::pair<int, int>> out(changedZones.begin(), changedZones.end());
    changedZones.clear();
    return out;
}

bool ExtractedWorld::gameObjects(int ax, int ay, std::vector<worldgen::ZoneObject> &out) {
    const std::shared_ptr<Zone> zone = cached(ax, ay);
    if (!zone)
        return false;
    // An extracted zone keeps no objects of its own: the generator's, on the world's seed
    // (as the game makes them; the generator is set to the extraction's seed with its creatures).
    if (!zone->objectsMade)
        zone->objects = worldgen::zoneObjects(ax, ay), zone->objectsMade = true;
    out = zone->objects;
    return true;
}

std::shared_ptr<ExtractedWorld::Zone> ExtractedWorld::readZone(const fs::path &file, bool decode, std::string &error) {
    error.clear();
    auto fail = [&](const char *why) -> std::shared_ptr<Zone> { error = why; return nullptr; };
    int ax, ay;
    if (!zoneCoordinates(file, ax, ay)) return fail("invalid zone filename");
    std::ifstream in(file, std::ios::binary | std::ios::ate);
    if (!in) return fail("cannot open zone");
    auto bytes = in.tellg();
    if (bytes < 20 || bytes > 128 * 1024 * 1024) return fail("invalid or excessive zone file size");
    std::vector<uint8_t> data((size_t)bytes);
    in.seekg(0);
    if (!in.read((char *)data.data(), data.size()) || memcmp(data.data(), "CWZ1", 4))
        return fail("missing CWZ1 header or truncated zone");
    int32_t coords[2];
    memcpy(coords, data.data() + 4, sizeof coords);
    if (coords[0] != ax || coords[1] != ay) return fail("zone header coordinates do not match filename");
    auto zone = std::make_shared<Zone>();
    auto read = [&](size_t &at, auto &value) {
        if (at + sizeof(value) > data.size())
            return false;
        memcpy(&value, &data[at], sizeof(value));
        at += sizeof(value);
        return true;
    };
    size_t at = 12;
    int32_t region = 0, reserved = 0;
    read(at, region);
    read(at, reserved);
    zone->region = region;
    size_t totalBlocks = 0;
    for (auto &column : zone->columns) {
        int32_t baseZ = 0;
        uint32_t runs = 0;
        if (!read(at, baseZ) || !read(at, runs))
            return fail("truncated column header");
        if (baseZ == INT32_MIN || baseZ > INT32_MAX - Z_OFFSET - 1)
            return fail("column height overflows engine coordinates");
        if (runs > (data.size() - at) / 8) return fail("truncated column runs");
        column.baseZ = baseZ;
        size_t columnBlocks = 0;
        for (uint32_t r = 0; r < runs; r++) {
            uint16_t count = 0;
            Block block{};
            if (!read(at, count) || !read(at, block))
                return fail("truncated block run");
            columnBlocks += count;
            totalBlocks += count;
            // Resource limits for damaged exports, not gameplay height rules.
            if (!count || columnBlocks > 1024 * 1024 || totalBlocks > 16 * 1024 * 1024 ||
                (int64_t)baseZ + (int64_t)columnBlocks > INT32_MAX - Z_OFFSET - 1)
                return fail("invalid or excessive expanded block count");
            if (decode) column.blocks.insert(column.blocks.end(), count, block);
        }
    }
    if (at != data.size()) return fail("unexpected trailing zone data");
    return zone;
}

bool ExtractedWorld::surface(double x, double y, float &height) {
    int64_t bx = (int64_t)std::floor(x), by = (int64_t)std::floor(y);
    int ax = (int)floorDiv(bx, 64) + originX, ay = (int)floorDiv(by, 64) + originY;
    auto zone = load(ax, ay);
    if (!zone)
        return false;
    int lx = (int)(bx - floorDiv(bx, 64) * 64), ly = (int)(by - floorDiv(by, 64) * 64);
    const Column &c = zone->columns[lx * 64 + ly];
    int top = c.baseZ - 1; // Filler below baseZ is solid.
    for (int i = (int)c.blocks.size() - 1; i >= 0; i--)
        if (solidType(c.blocks[i].type)) {
            top = c.baseZ + i;
            break;
        }
    height = float(std::max(top + 1, 1) + Z_OFFSET); // Stand on water at sea level.
    return true;
}

float ExtractedWorld::partScale(const std::string &name) const {
    auto it = measuredScale.find(name);
    if (it != measuredScale.end())
        return it->second;
    // Not measured in game (some interactive objects never draw with their
    // shared sprite buffer). Every measured building part is sized so its
    // widest horizontal side is a whole number of blocks, most often 2: use
    // that, from the sprite's own dimensions.
    std::lock_guard lock(cacheMutex);
    auto cached = estimatedScale.find(name);
    if (cached != estimatedScale.end())
        return cached->second;
    float scale = 1.f / 3;
    std::ifstream cub(folder.parent_path().parent_path() / "sprites" / (name + ".cub"), std::ios::binary);
    int32_t dims[3] = {};
    if (cub.read((char *)dims, sizeof dims) && dims[0] > 0 && dims[1] > 0)
        scale = 2.f / (float)std::max(dims[0], dims[1]);
    estimatedScale[name] = scale;
    return scale;
}

std::vector<Scenery> ExtractedWorld::props(int zx, int zy) {
    int64_t ax = (int64_t)zx + originX, ay = (int64_t)zy + originY;
    if (ax < INT32_MIN || ax > INT32_MAX || ay < INT32_MIN || ay > INT32_MAX) return {};
    return propsAbsolute((int)ax, (int)ay);
}

std::vector<Scenery> ExtractedWorld::propsAbsolute(int ax, int ay) {
    std::vector<Scenery> out;
    if (auto zone = load(ax, ay); generatedMode || (zone && zone->generated)) { // the generator's models and objects, named as the records would be
        if (!zone)
            return out;
        auto local = [&](int64_t x, int64_t y, int64_t z) {
            const int64_t ox = (int64_t)originX * 64 * 65536, oy = (int64_t)originY * 64 * 65536;
            return glm::vec3(float((double(x) - double(ox)) / 65536), float((double(y) - double(oy)) / 65536), float(double(z) / 65536 + Z_OFFSET));
        };
        int listPosition = 0; // (vegetation-wind.md §4.3: every record counts, from 1)
        for (const worldgen::Model &m : zone->things->models) {
            listPosition++;
            if (m.type < 0 || m.type >= (int)environment.size() || environment[m.type].empty() || !(m.size > 0 && m.size < 2) ||
                !std::isfinite(m.yaw))
                continue;
            const std::string &name = environment[m.type];
            const glm::vec3 p = local(m.x, m.y, m.z);
            // Which models sway is the record's flag 4, never the name (vegetation-wind.md §3.1).
            out.push_back({name, p.x, p.y, p.z, glm::radians(m.yaw), m.size, 0, false, m.flags, listPosition});
        }
        for (const worldgen::ZoneObject &o : zone->objects) {
            if (o.type < 0 || o.type >= (int)buildingParts.size() || buildingParts[o.type].empty() || o.turn > 3 || !(o.scale > 0 && o.scale < 8))
                continue;
            const glm::vec3 p = local(o.x, o.y, o.z);
            out.push_back({buildingParts[o.type], p.x, p.y, p.z, o.turn * 1.5707963f, o.scale * partScale(buildingParts[o.type]), 0,
                           measuredScale.count(buildingParts[o.type]) == 0});
        }
        return out;
    }
    std::ifstream in(folder / ("z_" + std::to_string(ax) + "_" + std::to_string(ay) + ".rec"), std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    if (data.size() < 4 || memcmp(data.data(), "CWR1", 4) != 0)
        return out;
    // Game dots (65536 per block) -> local blocks.
    const int64_t originDots[2] = {(int64_t)originX * 64 * 65536, (int64_t)originY * 64 * 65536};
    auto local = [&](const uint8_t *p, float &x, float &y, float &z) {
        int64_t pos[3];
        memcpy(pos, p, sizeof(pos));
        x = float((double(pos[0]) - double(originDots[0])) / 65536);
        y = float((double(pos[1]) - double(originDots[1])) / 65536);
        z = float(double(pos[2]) / 65536 + Z_OFFSET);
        return pos[0] != 0 || pos[1] != 0 || pos[2] != 0;
    };
    size_t at = 4;
    while (at + 12 <= data.size()) {
        uint32_t offset, stride, count;
        memcpy(&offset, &data[at], 4);
        memcpy(&stride, &data[at + 4], 4);
        memcpy(&count, &data[at + 8], 4);
        at += 12;
        if (!stride || count > (data.size() - at) / stride)
            break;
        // Unknown record layouts cannot be decoded using these field offsets.
        bool spriteLayout = stride == 0x40 && (offset == 0x18 || offset == 0x90);
        bool objectLayout = stride == 0xe0 && (offset == 0x30 || offset == 0x48 || offset == 0x60);
        if (!spriteLayout && !objectLayout) {
            at += (size_t)stride * count;
            continue;
        }
        for (uint32_t i = 0; i < count; i++) {
            const uint8_t *r = &data[at + (size_t)i * stride];
            int32_t id;
            memcpy(&id, r, 4);
            float x, y, z;
            // Records at 0x30/0x48 (statues, stone gates) keep their position at +0xa0.
            bool spawnLayout = stride == 0xe0 && (offset == 0x30 || offset == 0x48);
            if (!local(r + (spawnLayout ? 0xa0 : 8), x, y, z))
                continue;
            if (spawnLayout) {
                if (offset == 0x30 && id >= 0 && id < (int)buildingParts.size() && !buildingParts[id].empty())
                    out.push_back({buildingParts[id], x, y, z, 0, partScale(buildingParts[id]), 0,
                                   measuredScale.count(buildingParts[id]) == 0}); // 0x48 repeats 0x30.
                continue;
            }
            if (stride == 0x40 && offset == 0x90) {
                // Second sprite array: building parts (chest lids) in the sprite layout.
                float scale, degrees;
                memcpy(&scale, r + 32, 4);
                memcpy(&degrees, r + 36, 4);
                if (id >= 0 && id < (int)buildingParts.size() && !buildingParts[id].empty() && scale > 0 && scale < 2 && std::isfinite(degrees))
                    out.push_back({buildingParts[id], x, y, z, glm::radians(degrees), scale, 0});
                continue;
            }
            if (stride == 0x40) {
                // Sprite record: environment type, position, blocks per voxel, yaw in degrees.
                float scale, degrees;
                memcpy(&scale, r + 32, 4);
                memcpy(&degrees, r + 36, 4);
                if (id < 0 || id >= (int)environment.size() || environment[id].empty() || !(scale > 0 && scale < 2) || !std::isfinite(degrees))
                    continue;
                // The record's flags (+0x38) say whether it sways or shines; its place in the
                // zone's model list (+0x18) phases the sway (vegetation-wind.md §3.1, §4.3).
                int32_t flags = 0;
                if (offset == 0x18)
                    memcpy(&flags, r + MODEL_FLAGS, 4);
                out.push_back({environment[id], x, y, z, glm::radians(degrees), scale, 0, false, flags, offset == 0x18 ? (int)i + 1 : 0});
            } else if (stride == 0xe0 && offset == 0x60) {
                // Object record: building part, position, quarter turns, scale.
                // The array at 0x78 repeats these objects. Use the measured
                // sprite size; expose the provisional fallback to DataCheck.
                // Rotation is a byte, followed by three bytes of padding. The
                // game leaves that padding uninitialised in many real records.
                uint8_t turns = r[0x20];
                float scale;
                memcpy(&scale, r + 0x24, 4);
                if (id < 0 || id >= (int)buildingParts.size() || buildingParts[id].empty() || turns > 3 ||
                    !(scale > 0 && scale < 8))
                    continue;
                out.push_back({buildingParts[id], x, y, z, turns * 1.5707963f, scale * partScale(buildingParts[id]), 0,
                               measuredScale.count(buildingParts[id]) == 0});
            }
        }
        at += (size_t)stride * count;
    }
    return out;
}

std::vector<soundrules::Emitter> ExtractedWorld::soundEmitters(int ax, int ay) const {
    constexpr uint32_t EMITTER_LIST = 0xa8; // the zone's emitter list
    std::vector<soundrules::Emitter> out;
    std::ifstream in(folder / ("z_" + std::to_string(ax) + "_" + std::to_string(ay) + ".rec"), std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    if (data.size() < 4 || memcmp(data.data(), "CWR1", 4) != 0)
        return out;
    for (size_t at = 4; at + 12 <= data.size();) {
        uint32_t offset, stride, count;
        memcpy(&offset, &data[at], 4);
        memcpy(&stride, &data[at + 4], 4);
        memcpy(&count, &data[at + 8], 4);
        at += 12;
        if (!stride || count > (data.size() - at) / stride)
            break;
        if (offset == EMITTER_LIST && stride == soundrules::emitter::RECORD_SIZE)
            for (uint32_t i = 0; i < count; i++) {
                const uint8_t *r = &data[at + (size_t)i * stride];
                soundrules::Emitter e;
                memcpy(&e.type, r + soundrules::emitter::TYPE, 4);
                memcpy(e.position, r + soundrules::emitter::POSITION, sizeof e.position);
                out.push_back(e);
            }
        at += (size_t)stride * count;
    }
    return out;
}

namespace {
// Solid test inside one column at game z: filler below baseZ, stored blocks, then air.
bool columnSolid(int baseZ, const std::vector<uint8_t> &types, int z) {
    if (z < baseZ)
        return true;
    size_t i = (size_t)(z - baseZ);
    return i < types.size() && solidType(types[i]);
}
} // namespace

bool ExtractedWorld::solidAt(double x, double y, double z) {
    int64_t bx = (int64_t)std::floor(x), by = (int64_t)std::floor(y);
    auto zone = load((int)floorDiv(bx, 64) + originX, (int)floorDiv(by, 64) + originY);
    if (!zone)
        return false;
    const Column &c = zone->columns[(bx - floorDiv(bx, 64) * 64) * 64 + (by - floorDiv(by, 64) * 64)];
    int gz = (int)std::floor(z) - Z_OFFSET;
    if (gz < c.baseZ)
        return true;
    size_t i = (size_t)(gz - c.baseZ);
    return i < c.blocks.size() && solidType(c.blocks[i].type);
}

bool ExtractedWorld::waterAt(double x, double y, double z) {
    int64_t bx = (int64_t)std::floor(x), by = (int64_t)std::floor(y);
    auto zone = load((int)floorDiv(bx, 64) + originX, (int)floorDiv(by, 64) + originY);
    if (!zone)
        return false;
    const Column &c = zone->columns[(bx - floorDiv(bx, 64) * 64) * 64 + (by - floorDiv(by, 64) * 64)];
    int gz = (int)std::floor(z) - Z_OFFSET;
    if (gz < c.baseZ)
        return false;
    size_t i = (size_t)(gz - c.baseZ);
    if (i >= c.blocks.size())
        return gz <= 0;
    const Block &b = c.blocks[i];
    return b.type == WATER_TYPE || (b.type == AIR_TYPE && gz <= 0 && !(b.breakable & DRY_AIR));
}

bool ExtractedWorld::openToSky(double x, double y, double z) {
    int64_t bx = (int64_t)std::floor(x), by = (int64_t)std::floor(y);
    auto zone = load((int)floorDiv(bx, 64) + originX, (int)floorDiv(by, 64) + originY);
    if (!zone)
        return true;
    const Column &c = zone->columns[(bx - floorDiv(bx, 64) * 64) * 64 + (by - floorDiv(by, 64) * 64)];
    int gz = (int)std::floor(z) - Z_OFFSET;
    if (gz < c.baseZ)
        return false;
    for (size_t i = (size_t)(gz - c.baseZ); i < c.blocks.size(); i++)
        if (solidType(c.blocks[i].type) || c.blocks[i].type == LAVA_TYPE)
            return false;
    return true;
}

bool ExtractedWorld::lightColumn(double x, double y, int z0, int count, uint8_t *out) {
    int64_t bx = (int64_t)std::floor(x), by = (int64_t)std::floor(y);
    auto zone = load((int)floorDiv(bx, 64) + originX, (int)floorDiv(by, 64) + originY);
    if (!zone) {
        std::fill(out, out + count, (uint8_t)alphalight::BlockKind::Open);
        return true;
    }
    return columnLight(zone->columns[(bx - floorDiv(bx, 64) * 64) * 64 + (by - floorDiv(by, 64) * 64)], z0, count, out);
}

void ExtractedWorld::lightColumns(int x0, int y0, int width, int height, int z0, int count, uint8_t *out, uint8_t *sky) {
    for (int zy = (int)floorDiv(y0, 64); zy <= (int)floorDiv(y0 + height - 1, 64); zy++)
        for (int zx = (int)floorDiv(x0, 64); zx <= (int)floorDiv(x0 + width - 1, 64); zx++) {
            const std::shared_ptr<Zone> zone = load(zx + originX, zy + originY);
            const int fromX = std::max(x0, zx * 64), toX = std::min(x0 + width, zx * 64 + 64);
            const int fromY = std::max(y0, zy * 64), toY = std::min(y0 + height, zy * 64 + 64);
            for (int y = fromY; y < toY; y++)
                for (int x = fromX; x < toX; x++) {
                    const size_t at = (size_t)(y - y0) * width + (x - x0);
                    uint8_t *column = out + at * count;
                    if (!zone) {
                        std::fill(column, column + count, (uint8_t)alphalight::BlockKind::Open);
                        sky[at] = 1;
                    } else
                        sky[at] = columnLight(zone->columns[(size_t)(x - zx * 64) * 64 + (y - zy * 64)], z0, count, column) ? 1 : 0;
                }
        }
}

bool ExtractedWorld::columnLight(const Column &c, int z0, int count, uint8_t *out) {
    using alphalight::BlockKind;
    auto kindAt = [&](int gz) {
        if (gz < c.baseZ) // filler below the stored blocks (none in an empty column)
            return c.blocks.empty() ? BlockKind::Open : BlockKind::Solid;
        size_t i = (size_t)(gz - c.baseZ);
        if (i >= c.blocks.size())
            return BlockKind::Open;
        uint8_t t = c.blocks[i].type;
        return t == LAVA_TYPE ? BlockKind::Glow : solidType(t) ? BlockKind::Solid : BlockKind::Open;
    };
    const int g0 = z0 - Z_OFFSET;
    for (int k = 0; k < count; k++)
        out[k] = (uint8_t)kindAt(g0 + k);
    for (int gz = std::max(g0 + count, c.baseZ); gz < c.baseZ + (int)c.blocks.size(); gz++)
        if (kindAt(gz) != BlockKind::Open)
            return false;
    return g0 + count >= c.baseZ || c.blocks.empty();
}

bool ExtractedWorld::floorAt(double x, double y, double fromZ, int headroom, float &height) {
    int64_t bx = (int64_t)std::floor(x), by = (int64_t)std::floor(y);
    auto zone = load((int)floorDiv(bx, 64) + originX, (int)floorDiv(by, 64) + originY);
    if (!zone)
        return false;
    const Column &c = zone->columns[(bx - floorDiv(bx, 64) * 64) * 64 + (by - floorDiv(by, 64) * 64)];
    std::vector<uint8_t> types(c.blocks.size());
    for (size_t i = 0; i < types.size(); i++)
        types[i] = c.blocks[i].type;
    int start = (int)std::floor(fromZ) - Z_OFFSET;
    for (int z = start; z >= std::min(start, c.baseZ) - 1; z--) {
        if (!columnSolid(c.baseZ, types, z))
            continue;
        bool open = true;
        for (int k = 1; k <= headroom && open; k++)
            open = !columnSolid(c.baseZ, types, z + k);
        if (!open)
            continue;
        // Stand on the water surface where the floor is under the sea.
        height = float(std::max(z + 1, 1) + Z_OFFSET);
        return true;
    }
    return false;
}

int ExtractedWorld::biomeAt(double x, double y) {
    int ax = (int)floorDiv((int64_t)std::floor(x), 64) + originX;
    int ay = (int)floorDiv((int64_t)std::floor(y), 64) + originY;
    auto zone = load(ax, ay);
    return zone ? biomeOf(zone->region) : PLAINS;
}

bool ExtractedWorld::mesh(int zx, int zy, Mesh &opaque, Mesh &water) {
    int ax = zx + originX, ay = zy + originY;
    std::shared_ptr<Zone> around[3][3];
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++)
            around[j + 1][i + 1] = load(ax + i, ay + j);
    auto self = around[1][1];
    if (!self)
        return false;
    // Column lookup over the zone and a one-column border from its neighbours.
    // Missing neighbours count as this zone's edge column (no cliff walls at
    // the edge of the extracted area).
    auto column = [&](int lx, int ly) -> const Column & {
        int ox = lx < 0 ? -1 : lx >= 64 ? 1 : 0, oy = ly < 0 ? -1 : ly >= 64 ? 1 : 0;
        const auto &z = around[oy + 1][ox + 1];
        if (!z)
            return self->columns[std::clamp(lx, 0, 63) * 64 + std::clamp(ly, 0, 63)];
        return z->columns[(lx - ox * 64) * 64 + (ly - oy * 64)];
    };
    // Block at game z for a column: filler below baseZ (coloured like the
    // column's lowest block), stored blocks, then air.
    auto blockAt = [&](const Column &c, int z, Block &out) {
        if (z < c.baseZ) {
            if (c.blocks.empty())
                return false;
            out = c.blocks.front();
            if (!solidType(out.type))
                out.type = 1; // Solid filler.
            return true;
        }
        size_t i = (size_t)(z - c.baseZ);
        if (i >= c.blocks.size())
            return false;
        out = c.blocks[i];
        return true;
    };
    // Height range worth meshing: from the lowest surface in the 66x66 area
    // (anything lower is hidden) to the highest stored block.
    int lo = INT32_MAX, hi = INT32_MIN;
    for (int ly = -1; ly <= 64; ly++)
        for (int lx = -1; lx <= 64; lx++) {
            const Column &c = column(lx, ly);
            lo = std::min(lo, c.baseZ - 1);
            hi = std::max(hi, c.baseZ + (int)c.blocks.size());
        }
    if (lo > hi)
        return true;
    auto solid = [&](int lx, int ly, int z) {
        if (z <= lo)
            return true; // Below the meshed range: buried.
        Block b;
        return blockAt(column(lx, ly), z, b) && solidType(b.type);
    };
    glm::vec3 offset((float)zx * 64, (float)zy * 64, (float)Z_OFFSET);
    auto colour = [&](int lx, int ly, int z) {
        Block b{};
        blockAt(column(lx, ly), z, b);
        // Vanilla's world shader doubles vertex colours (vc = v1 + v1).
        VoxelColor c{(uint8_t)std::min(255, b.r * 2), (uint8_t)std::min(255, b.g * 2),
                     (uint8_t)std::min(255, b.b * 2), 0};
        // Vanilla never moves a block; the leaf sway is CubeVulkan's WorldMotion option
        // ([Motion] LeafSway, off by default), which the vertex shader applies to these.
        if (b.type == LEAVES_TYPE || b.type == SNOW_LEAVES_TYPE)
            c.flags = SWAY;
        return c;
    };
    // The Alpha look bakes the block light (docs/specs/alpha-rendering.md 9.2-9.4): the light
    // is flooded through the zone and a 16-block border, then every corner samples it.
    const bool alphaLook = renderStyle() == RenderStyle::Alpha;
    std::unique_ptr<alphalight::LightVolume> light;
    if (alphaLook) {
        constexpr int BORDER = 16;
        const int top = std::max(hi + 1, 1); // open air above everything, and above the sea surface
        light = std::make_unique<alphalight::LightVolume>(glm::ivec3(-BORDER, -BORDER, lo),
                                                          glm::ivec3(64 + 2 * BORDER, 64 + 2 * BORDER, top - lo + 1));
        for (int ly = -BORDER; ly < 64 + BORDER; ly++)
            for (int lx = -BORDER; lx < 64 + BORDER; lx++) {
                const Column &c = column(lx, ly);
                for (int z = lo + 1; z <= top; z++) {
                    Block b;
                    alphalight::BlockKind kind = alphalight::BlockKind::Open; // above the stored blocks: air
                    if (blockAt(c, z, b))
                        kind = b.type == LAVA_TYPE    ? alphalight::BlockKind::Glow
                               : solidType(b.type) ? alphalight::BlockKind::Solid
                                                   : alphalight::BlockKind::Open;
                    light->setKind({lx, ly, z}, kind);
                }
            }
        light->flood();
        auto shade = [&](int x, int y, int z, int face, int k, VoxelColor c) {
            const int *corner = FACE_CORNERS[face][k];
            return alphalight::shadeCorner(*light, {x + corner[0], y + corner[1], z + corner[2]}, face, c);
        };
        meshFaces({0, 0, lo + 1}, {64, 64, hi + 1}, solid, colour, shade, opaque, 1.f, offset);
    } else
        meshVoxels({0, 0, lo + 1}, {64, 64, hi + 1}, solid, colour, opaque, 1.f, offset);
    // Water geometry from water-rendering.md A.1/B.1: exposed full block
    // faces, including sides/bottoms, and a coplanar film over exposed wet ground.
    auto typeAt = [&](int x, int y, int z) {
        Block block{};
        blockAt(column(x, y), z, block);
        if (block.type == AIR_TYPE && z <= 0 && !(block.breakable & DRY_AIR)) return WATER_TYPE;
        return block.type;
    };
    auto emitWater = [&](int x, int y, int z, int face) {
        uint32_t base = (uint32_t)water.vertices.size();
        for (int k = 0; k < 4; k++) {
            const int *corner = FACE_CORNERS[face][k];
            glm::vec3 at = offset + glm::vec3(x + corner[0], y + corner[1], z + corner[2]);
            CornerShade shade{128, 255, 255, 255}; // native colour after vertex doubling
            if (alphaLook) {
                glm::vec3 rgb = alphalight::waterColour(light->light({x, y, z}) / 255.f);
                shade = alphalight::shadeCorner(*light, {x + corner[0], y + corner[1], z + corner[2]}, face,
                    VoxelColor{alphalight::toByte(rgb.r), alphalight::toByte(rgb.g), alphalight::toByte(rgb.b), 0});
            }
            water.vertices.push_back({at.x, at.y, at.z, shade.r, shade.g, shade.b, shade.a,
                                      (uint8_t)(alphaLook ? face : 4), WATER_SURFACE, 0, 0});
            water.min = glm::min(water.min, at);
            water.max = glm::max(water.max, at);
        }
        water.indices.insert(water.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    };
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            for (int z = lo + 1; z <= std::max(hi, 0); z++) {
                uint8_t type = typeAt(x, y, z);
                if (type == WATER_TYPE) {
                    for (int face = 0; face < 6; face++) {
                        const int *normal = FACE_NORMALS[face];
                        if (typeAt(x + normal[0], y + normal[1], z + normal[2]) == AIR_TYPE)
                            emitWater(x, y, z, face);
                    }
                } else if (type == WET_TYPE && typeAt(x, y, z + 1) == AIR_TYPE)
                    emitWater(x, y, z, 4);
            }
    return true;
}

int ExtractedWorld::regionTypeAt(double x, double y, int &cellX, int &cellY) {
    int ax = (int)floorDiv((int64_t)std::floor(x), 64) + originX;
    int ay = (int)floorDiv((int64_t)std::floor(y), 64) + originY;
    cellX = (int)floorDiv(ax, 16);
    cellY = (int)floorDiv(ay, 16);
    auto zone = load(ax, ay);
    return zone ? (int)zone->region : -1;
}
