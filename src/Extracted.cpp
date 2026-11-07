#include "Extracted.h"
#include "MeshProfile.h"
#include "ObjectOccupancyCapture.h"
#include "SteamSceneRange.h"
#include "GameMath.h"
#include "AlphaLight.h"
#include "AlphaLook.h"
#include "worldgen/ZoneLight.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
uint64_t nextZoneCacheRevision() {
    static std::atomic<uint64_t> revision{0};
    return revision.fetch_add(1, std::memory_order_relaxed) + 1;
}

// cube::Block::Type values that are not drawn as solid blocks.
constexpr uint8_t AIR_TYPE = 0, WATER_TYPE = 2, WET_TYPE = 3;
constexpr uint8_t LEAVES_TYPE = 8, SNOW_LEAVES_TYPE = 11, LAVA_TYPE = 17, POISON_TYPE = 19;
constexpr uint8_t DRY_AIR = 2; // breakable bit: air that does not become water below sea level.
// A zone model record's flags (0x40-byte records, worldgen-structures.md §4.2).
constexpr size_t MODEL_FLAGS = 0x38;
template <class T> T modelField(const ExtractedWorld::ModelRecord &record, size_t offset) {
    T value;
    std::memcpy(&value, record.data() + offset, sizeof value);
    return value;
}

bool solidType(uint8_t t) {
    return t != AIR_TYPE && t != WATER_TYPE && t != LAVA_TYPE && t != POISON_TYPE;
}

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
    clearCache();
    zones.clear();
    environment.clear();
    buildingParts.clear();
    measuredScale.clear();
    folder.clear();
    if (!spriteDir.empty())
        loadSpriteTables(spriteDir);
    generatedMode = true;
    seedValue = worldgen::currentWorld().seed;
    originX = (int)floorDiv(homeX, 64);
    originY = (int)floorDiv(homeY, 64);
    float ground = std::floor(worldgen::groundHeight(homeX, homeY)) + 1;
    homeLocal = glm::vec3(float(homeX - (int64_t)originX * 64), float(homeY - (int64_t)originY * 64),
                          std::max(ground, 1.f) + Z_OFFSET);
    if (worldgen::usingAlphaTerrain()) {
        float actual;
        if (surface(homeLocal.x, homeLocal.y, actual))
            homeLocal.z = actual;
    }
}

// Sprite tables written by the extractor next to the seed folders.
void ExtractedWorld::loadSpriteTables(const fs::path &dir) {
    spriteFolder = dir.parent_path() / "sprites";
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
    zone->markedObjectBlocks = zone->things->lists.marked;
    auto models = std::make_shared<std::vector<ModelRecord>>();
    models->reserve(zone->things->models.size());
    for (const auto &model : zone->things->models) {
        ModelRecord record{};
        const auto put = [&](size_t offset, const auto &value) { std::memcpy(record.data() + offset, &value, sizeof value); };
        put(0, model.type);
        put(8, model.x);
        put(16, model.y);
        put(24, model.z);
        put(32, model.size);
        put(36, model.yaw);
        put(40, -1.0f);
        put(44, model.r);
        put(48, model.g);
        put(52, model.b);
        put(56, model.flags);
        models->push_back(record);
    }
    zone->models = std::move(models);
    zone->objects = worldgen::zoneObjects(ax, ay);
    zone->objectsMade = true;
    const std::vector<worldgen::TerrainColumn> columns = worldgen::litZone(ax, ay);
    for (size_t i = 0; i < columns.size(); i++) {
        Column &c = zone->columns[i];
        c.base = columns[i].base;
        c.fullColour = columns[i].fullColour;
        c.sourceShift = columns[i].sourceShift;
        c.blocks.reserve(columns[i].blocks.size());
        for (const worldgen::Block &b : columns[i].blocks)
            c.blocks.push_back({b.r, b.g, b.b, b.glow, b.type, b.flags});
    }
    return zone;
}

std::unique_ptr<ExtractedWorld> ExtractedWorld::forkSource() const {
    auto result = std::make_unique<ExtractedWorld>();
    result->folder = folder;
    result->environment = environment;
    result->buildingParts = buildingParts;
    result->measuredScale = measuredScale;
    result->spriteFolder = spriteFolder;
    result->zones = zones;
    result->originX = originX;
    result->originY = originY;
    result->seedValue = seedValue;
    result->generatedMode = generatedMode;
    result->homeLocal = homeLocal;
    return result;
}

bool ExtractedWorld::open(const fs::path &dir, std::string &error) {
    clearCache();
    error.clear();
    generatedMode = false;
    zones.clear();
    environment.clear();
    buildingParts.clear();
    measuredScale.clear();
    originX = originY = seedValue = 0;
    homeLocal = glm::vec3(0);
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

std::vector<worldgen::TerrainColumn> ExtractedWorld::readStoredZoneColumns(const fs::path &file, std::string &error) {
    auto zone = readZone(file, true, error, true);
    if (!zone)
        return {};
    return {std::make_move_iterator(std::begin(zone->columns)), std::make_move_iterator(std::end(zone->columns))};
}

std::shared_ptr<ExtractedWorld::Zone> ExtractedWorld::load(int ax, int ay) {
    if (generatedMode && !worldgen::terrainZoneAvailable(ax, ay))
        return nullptr;
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
    auto zone =
        generate ? generateZone(ax, ay) : readZone(folder / ("z_" + std::to_string(ax) + "_" + std::to_string(ay) + ".cwz"), true, error);
    if (!zone)
        return nullptr;
    if (!generate) {
        auto records = readCapturedLists(ax, ay, true);
        if (records.objectRewards)
            zone->objectRewards = std::make_shared<const objectrewards::Map>(std::move(*records.objectRewards));
        if (records.markedObjectBlocks)
            zone->markedObjectBlocks = std::move(*records.markedObjectBlocks);
    }
    applyBlockChanges(ax, ay, *zone);
    std::lock_guard lock(cacheMutex);
    // A foreground query and prefetch can finish the same read concurrently. Preserve
    // the resident zone's identity instead of inserting another copy of the same key.
    for (auto it = cache.begin(); it != cache.end(); ++it)
        if (it->first == key) {
            cache.splice(cache.begin(), cache, it);
            return cache.front().second;
        }
    cache.emplace_front(key, zone);
    // Explicit keeps can exceed the normal cache budget (the native player
    // radius alone pins121 zones). Keep a separate bounded LRU for unpinned
    // readers, or every farther mesh request would evict its own source at once.
    constexpr size_t UNPINNED_LIMIT = 96;
    size_t unpinned = size_t(std::count_if(cache.begin(), cache.end(), [&](const auto &entry) { return !kept(entry.first); }));
    for (auto it = cache.end(); unpinned > UNPINNED_LIMIT && it != cache.begin();) {
        --it;
        if (!kept(it->first)) {
            it = cache.erase(it);
            --unpinned;
        }
    }
    cacheRevision = nextZoneCacheRevision();
    return zone;
}

bool ExtractedWorld::kept(const Key &key) const {
    for (const auto &[owner, around] : keptBy)
        for (const Key &centre : around.centres)
            if (std::abs(key.first - centre.first) <= around.radius && std::abs(key.second - centre.second) <= around.radius)
                return true;
    return false;
}

ExtractedWorld::ExtractedWorld() : cacheRevision(nextZoneCacheRevision()) {
}

void ExtractedWorld::clearCache() {
    if (keeper.joinable())
        keeper.join();
    std::lock_guard lock(cacheMutex);
    cache.clear();
    keptBy.clear();
    absent.clear();
    estimatedScale.clear();
    voxelWidths.clear();
    {
        std::lock_guard changesLock(changesMutex);
        changedModels.clear();
    }
    cacheRevision = nextZoneCacheRevision();
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

thread_local ExtractedWorld::BlockEdits *ExtractedWorld::activeEdits = nullptr;

ExtractedWorld::BlockEdits::BlockEdits(ExtractedWorld &owner) : world(owner), previous(activeEdits), root(owner.blockEdits()) {
    if (!root) {
        root = this;
        writerLock = std::unique_lock(world.terrainWriterMutex);
    }
    activeEdits = this;
}
ExtractedWorld::BlockEdits::~BlockEdits() {
    if (root == this && !replacements.empty()) {
        std::lock_guard lock(world.cacheMutex);
        bool replaced = false;
        for (auto &[key, zone] : world.cache) {
            const auto draft = replacements.find(key);
            // A draft belongs to one loaded-zone lifetime, never a replacement load.
            if (draft != replacements.end() && zone && zone->identity == draft->second->identity) {
                zone = draft->second;
                replaced = true;
            }
        }
        if (replaced)
            world.cacheRevision = nextZoneCacheRevision();
    }
    activeEdits = previous;
}
ExtractedWorld::BlockEdits *ExtractedWorld::blockEdits() const {
    for (BlockEdits *scope = activeEdits; scope; scope = scope->previous)
        if (&scope->world == this)
            return scope->root;
    return nullptr;
}
std::shared_ptr<ExtractedWorld::Zone> ExtractedWorld::editedZone(int ax, int ay) const {
    if (const BlockEdits *scope = blockEdits()) {
        const auto found = scope->replacements.find({ax, ay});
        if (found != scope->replacements.end())
            return found->second;
    }
    return nullptr;
}
std::shared_ptr<ExtractedWorld::Zone> ExtractedWorld::writableZone(int ax, int ay) {
    BlockEdits *scope = blockEdits();
    if (!scope)
        return nullptr;
    if (auto draft = editedZone(ax, ay))
        return draft;
    const auto current = cached(ax, ay);
    if (!current)
        return nullptr;
    auto draft = std::make_shared<Zone>(*current);
    scope->replacements.emplace(Key{ax, ay}, draft);
    return draft;
}
int ExtractedWorld::gameBlockLight(int32_t x, int32_t y, int32_t z) {
    bool loaded = false;
    auto block = gameBlock(x, y, z, &loaded);
    if (!loaded)
        return 255;
    while (block[4] == 10 || block[4] == 12 || block[4] == 19) {
        const uint32_t next = uint32_t(z) + 1;
        std::memcpy(&z, &next, sizeof z);
        block = gameBlock(x, y, z);
    }
    if (block[4] == 0 || block[4] == 2)
        return std::max<int>(block[0] < 5 ? 5 : block[0], block[3]);
    return block[3];
}
bool ExtractedWorld::removeGameBlock(int32_t x, int32_t y, int32_t z) {
    if (!blockEdits()) {
        auto batch = editBlocks();
        return removeGameBlock(x, y, z);
    }
    const int ax = int(floorDiv(x, 64)), ay = int(floorDiv(y, 64));
    auto current = editedZone(ax, ay);
    if (!current)
        current = cached(ax, ay);
    if (!current)
        return false;
    const size_t column = size_t(int64_t(x) - int64_t(ax) * 64) * 64 + size_t(int64_t(y) - int64_t(ay) * 64);
    const Column &before = current->columns[column];
    const int64_t index = int64_t(z) - before.base;
    if (index < 0 || index >= int64_t(before.blocks.size()))
        return false;
    const auto draft = writableZone(ax, ay);
    draft->columns[column].blocks[size_t(index)] = {0xff, 0xff, 0xff, 0, uint8_t(z < 1 ? 2 : 0), 0};
    return true;
}
bool ExtractedWorld::writeGameBlock(Zone &zone, int32_t ax, int32_t ay, int32_t x, int32_t y, int32_t z, const GameBlock &bytes) {
    const auto index = worldgen::setterColumn(ax, ay, x, y);
    if (!index)
        return false;
    const size_t i = *index;
    Column &column = zone.columns[i];
    const worldgen::ColourInputs *colours = nullptr;
    worldgen::SandOffsets sand;
    if (zone.things && i < zone.things->terrain.retainedColours.size()) {
        colours = &zone.things->terrain.retainedColours[i];
        sand = zone.things->terrain.sand;
    } else if (zone.recordedColours && i < zone.recordedColours->size()) {
        colours = &(*zone.recordedColours)[i];
        sand = zone.recordedSand;
    }
    const bool gap = !column.empty() && int64_t(z) + 1 < column.base;
    worldgen::ColumnFiller filler;
    if (gap && column.fullColour) {
        filler = worldgen::alphaColumnFiller(x, y, column.sourceShift);
        if (!filler)
            return false;
    }
    // Legacy block-only exports need native cached inputs; Alpha gaps use the
    // retained source palette, including the column's vertical translation.
    if (gap && !colours && !filler)
        return false;
    const worldgen::ColourInputs unused;
    return worldgen::storeColumn(column, x, y, z, {bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]}, colours ? *colours : unused,
                                 sand, filler ? &filler : nullptr);
}
bool ExtractedWorld::setGameBlock(int32_t x, int32_t y, int32_t z, const GameBlock &block) {
    if (!blockEdits()) {
        auto batch = editBlocks();
        return setGameBlock(x, y, z, block);
    }
    const int ax = int(floorDiv(x, 64)), ay = int(floorDiv(y, 64));
    const auto draft = writableZone(ax, ay);
    return draft && writeGameBlock(*draft, ax, ay, x, y, z, block);
}
void ExtractedWorld::rememberBlockChange(const nativesaves::BlockChange &change) {
    const auto at = change.block();
    std::lock_guard lock(changesMutex);
    blockChanges[{int(floorDiv(at[0], 64)), int(floorDiv(at[1], 64))}].push_back(change);
}
void ExtractedWorld::invalidateGameBlock(int32_t x, int32_t y) {
    BlockNotice notice{};
    std::memcpy(notice.data(), &x, 4);
    std::memcpy(notice.data() + 4, &y, 4);
    invalidateGameBlocks(std::span(&notice, 1));
}
void ExtractedWorld::invalidateGameBlocks(std::span<const BlockNotice> changes) {
    std::set<Key> affected;
    const auto shifted = [](int32_t value, int offset) {
        const uint32_t bits = uint32_t(value) + uint32_t(offset);
        int32_t result;
        std::memcpy(&result, &bits, sizeof result);
        return result;
    };
    for (const auto &notice : changes) {
        int32_t x, y;
        std::memcpy(&x, notice.data(), 4);
        std::memcpy(&y, notice.data() + 4, 4);
        for (int dx : {-13, 0, 13})
            for (int dy : {-13, 0, 13})
                affected.emplace(int(floorDiv(shifted(x, dx), 64)), int(floorDiv(shifted(y, dy), 64)));
    }
    std::vector<Key> loaded;
    {
        std::lock_guard lock(cacheMutex);
        for (const Key &key : affected)
            if (std::any_of(cache.begin(), cache.end(), [&](const auto &entry) { return entry.first == key && entry.second; }))
                loaded.push_back(key);
    }
    std::lock_guard lock(changesMutex);
    changedZones.insert(loaded.begin(), loaded.end());
}

std::shared_ptr<const std::vector<ExtractedWorld::ModelRecord>> ExtractedWorld::gameModels(int32_t x, int32_t y) {
    auto zone = editedZone(x, y);
    if (!zone)
        zone = cached(x, y);
    return zone ? zone->models : nullptr;
}
size_t ExtractedWorld::removeUnsupportedModels(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    auto edits = editBlocks();
    const int ax0 = std::max(0, x0) / 64, ay0 = std::max(0, y0) / 64;
    const int ax1 = std::max(0, x1) / 64, ay1 = std::max(0, y1) / 64;
    std::vector<Key> selected;
    {
        std::lock_guard lock(cacheMutex);
        for (const auto &[key, zone] : cache)
            if (zone && key.first >= ax0 && key.first <= ax1 && key.second >= ay0 && key.second <= ay1)
                selected.push_back(key);
    }
    std::sort(selected.begin(), selected.end());
    size_t removed = 0;
    for (const auto &key : selected) {
        const auto records = gameModels(key.first, key.second);
        if (!records || records->empty())
            continue;
        std::vector<size_t> erase;
        for (size_t i = records->size(); i-- > 0;) {
            const auto &model = (*records)[i];
            if (!(model[0x38] & 2))
                continue;
            std::array<int64_t, 3> position;
            std::memcpy(position.data(), model.data() + 8, sizeof position);
            position[2] = gamemath::subtractFixed(position[2], 65536);
            if (gameBlock(gamemath::floorFixedBlock(position[0]), gamemath::floorFixedBlock(position[1]),
                          gamemath::floorFixedBlock(position[2]))[4] == 0)
                erase.push_back(i);
        }
        if (erase.empty())
            continue;
        auto draft = writableZone(key.first, key.second);
        auto replacement = std::make_shared<std::vector<ModelRecord>>(*records);
        auto state = std::make_shared<Zone::ModelLightState>();
        {
            std::lock_guard lock(draft->modelLights->mutex);
            state->waterAuxiliary = draft->modelLights->waterAuxiliary;
            for (size_t i = 0; i < replacement->size(); ++i) {
                if (draft->modelLights->waterModels.count(int(i) + 1))
                    (*replacement)[i][0x38] |= 8;
                const auto light = draft->modelLights->lightSamples.find(int(i) + 1);
                if (light != draft->modelLights->lightSamples.end())
                    std::memcpy((*replacement)[i].data() + 0x28, &light->second, 4);
            }
        }
        for (size_t index : erase) {
            for (size_t next = index + 1; next < replacement->size(); ++next) {
                // Native semantic fields move; destination padding is not assigned.
                std::memcpy((*replacement)[next - 1].data(), (*replacement)[next].data(), 4);
                std::memcpy((*replacement)[next - 1].data() + 8, (*replacement)[next].data() + 8, 52);
            }
            replacement->pop_back();
        }
        draft->models = std::move(replacement);
        draft->modelLights = std::move(state);
        removed += erase.size();
        std::lock_guard lock(changesMutex);
        changedModels.insert(key);
    }
    return removed;
}
std::vector<std::pair<int, int>> ExtractedWorld::takeChangedModels() {
    std::lock_guard lock(changesMutex);
    std::vector<Key> result(changedModels.begin(), changedModels.end());
    changedModels.clear();
    return result;
}

std::optional<ExtractedWorld::ZoneToken> ExtractedWorld::gameZoneToken(int32_t x, int32_t y) const {
    std::lock_guard lock(cacheMutex);
    for (const auto &[key, zone] : cache)
        if (key == Key{x, y} && zone)
            return ZoneToken{uint64_t(reinterpret_cast<uintptr_t>(zone->identity.get())), zone, generatedMode || zone->generated};
    return std::nullopt;
}

bool ExtractedWorld::gameZoneLoaded(int32_t x, int32_t y) const {
    std::lock_guard lock(cacheMutex);
    return std::any_of(cache.begin(), cache.end(), [&](const auto &entry) { return entry.first == Key{x, y} && entry.second; });
}

ExtractedWorld::GameBlock ExtractedWorld::gameBlock(int64_t x, int64_t y, int z, bool *loaded) {
    static const GameBlock stone{0xc8, 0xc8, 0xc8, 0, 1, 0};
    const int ax = (int)floorDiv(x, 64), ay = (int)floorDiv(y, 64);
    // The last few zones asked for, so a creature's many lookups skip the cache's lock.
    thread_local struct {
        const ExtractedWorld *world = nullptr;
        int ax = 0, ay = 0;
        uint64_t revision = 0;
        std::weak_ptr<Zone> zone;
    } memo[4];
    thread_local int next = 0;
    std::shared_ptr<Zone> zone = editedZone(ax, ay);
    const uint64_t revision = cacheRevision.load();
    if (!zone) {
        for (auto &m : memo)
            if (m.world == this && m.ax == ax && m.ay == ay && m.revision == revision)
                zone = m.zone.lock();
        if (!zone) {
            zone = cached(ax, ay);
            if (zone) {
                memo[next] = {this, ax, ay, revision, zone};
                next = (next + 1) % 4;
            }
        }
    }
    if (loaded)
        *loaded = zone != nullptr;
    if (!zone)
        return stone;
    return readGameBlock(*zone, ax, ay, x, y, z);
}

ExtractedWorld::GameBlock ExtractedWorld::readGameBlock(const Zone &zone, int32_t ax, int32_t ay, int64_t x, int64_t y, int32_t z) {
    static const GameBlock stone{0xc8, 0xc8, 0xc8, 0, 1, 0}, air{0xff, 0xff, 0xff, 0, 0, 0}, water{0xff, 0xff, 0xff, 0, 2, 0};
    const int64_t lx = x - int64_t(ax) * 64, ly = y - int64_t(ay) * 64;
    if (lx < 0 || lx >= 64 || ly < 0 || ly >= 64)
        return stone;
    const Column &c = zone.columns[size_t(lx * 64 + ly)];
    // Native effective lookup tests base before the live span, including empty columns.
    if (z < c.base)
        return stone;
    const size_t i = size_t(int64_t(z) - c.base);
    if (i < c.blocks.size()) {
        const Block &b = c.blocks[i];
        if (b.type != 0 || z > 0 || (b.flags & 2))
            return {b.r, b.g, b.b, b.glow, b.type, b.flags};
    } else if (z > 0) {
        return air;
    }
    return water;
}

bool ExtractedWorld::markObjectBlocks(int ax, int ay, std::span<const nativesaves::Record> records) {
    auto edits = editBlocks();
    const auto zone = writableZone(ax, ay);
    if (!zone)
        return false;
    std::vector<worldgen::StaticObject> objects;
    objects.reserve(records.size());
    for (const auto &record : records) {
        auto &object = objects.emplace_back();
        const auto read = [&](size_t offset, auto &value) { std::memcpy(&value, record.data() + offset, sizeof value); };
        read(0, object.type);
        read(8, object.x);
        read(16, object.y);
        read(24, object.z);
        read(32, object.turn);
        read(36, object.scale);
        object.active = record[40] != 0;
    }
    worldgen::markObjectBlocks(
        objects, zone->markedObjectBlocks,
        [&](int32_t x, int32_t y, int32_t z) {
            const auto b = readGameBlock(*zone, ax, ay, x, y, z);
            return Block{b[0], b[1], b[2], b[3], b[4], b[5]};
        },
        [&](int32_t x, int32_t y, int32_t z, Block b) {
            writeGameBlock(*zone, ax, ay, x, y, z, {b.r, b.g, b.b, b.glow, b.type, b.flags});
        });
    return true;
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
    for (const auto &change : changes) {
        const auto at = change.block();
        writeGameBlock(zone, ax, ay, at[0], at[1], at[2], change.now());
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
    auto edits = editBlocks();
    auto fresh = writableZone(ax, ay);
    if (!fresh)
        return;
    for (auto change = changes.rbegin(); change != changes.rend(); ++change) {
        const std::array<int32_t, 3> at = change->block();
        const int64_t lx = at[0] - (int64_t)ax * 64, ly = at[1] - (int64_t)ay * 64;
        if (lx < 0 || lx >= 64 || ly < 0 || ly >= 64)
            continue;
        Column &c = fresh->columns[lx * 64 + ly];
        const int64_t i = (int64_t)at[2] - c.base;
        if (c.blocks.empty() || i < 0 || i >= (int64_t)c.blocks.size())
            continue; // (outside the column: it holds what the change found there anyway)
        const std::array<uint8_t, 6> was = change->was();
        c.blocks[(size_t)i] = {was[0], was[1], was[2], was[3], was[4], was[5]};
    }
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
    auto edits = editBlocks();
    if (auto fresh = writableZone(ax, ay))
        applyBlockChanges(ax, ay, *fresh);
}

std::vector<std::pair<int, int>> ExtractedWorld::takeChangedZones() {
    std::lock_guard lock(changesMutex);
    std::vector<std::pair<int, int>> out(changedZones.begin(), changedZones.end());
    changedZones.clear();
    return out;
}

bool ExtractedWorld::gameObjects(int ax, int ay, std::vector<worldgen::ZoneObject> &out) {
    auto zone = editedZone(ax, ay);
    if (!zone)
        zone = cached(ax, ay);
    if (!zone)
        return false;
    if (zone->objectsMade) {
        out = zone->objects;
        return true;
    }
    // Lazy metadata must be immutable too: a simultaneous block/light writer
    // copies the same Zone. Publish initialization through the shared write scope.
    auto edits = editBlocks();
    zone = editedZone(ax, ay);
    if (!zone)
        zone = cached(ax, ay);
    if (!zone)
        return false;
    if (!zone->objectsMade) {
        zone = writableZone(ax, ay);
        zone->objects = worldgen::zoneObjects(ax, ay);
        zone->objectsMade = true;
    }
    out = zone->objects;
    return true;
}

std::shared_ptr<const std::vector<ExtractedWorld::ModelRecord>> ExtractedWorld::readModels(const fs::path &zoneFile) {
    fs::path file = zoneFile;
    file.replace_extension(".rec");
    std::ifstream input(file, std::ios::binary | std::ios::ate);
    auto result = std::make_shared<std::vector<ModelRecord>>();
    if (!input || input.tellg() < 4 || input.tellg() > 128 * 1024 * 1024)
        return result;
    std::vector<uint8_t> bytes(size_t(input.tellg()));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(bytes.data()), bytes.size()) || std::memcmp(bytes.data(), "CWR1", 4))
        return result;
    for (size_t at = 4; at + 12 <= bytes.size();) {
        uint32_t tag, stride, count;
        std::memcpy(&tag, bytes.data() + at, 4);
        std::memcpy(&stride, bytes.data() + at + 4, 4);
        std::memcpy(&count, bytes.data() + at + 8, 4);
        at += 12;
        if (!stride || count > (bytes.size() - at) / stride)
            break;
        if (tag == 0x18 && stride == sizeof(ModelRecord)) {
            const size_t first = result->size();
            result->resize(first + count);
            std::memcpy(result->data() + first, bytes.data() + at, size_t(count) * stride);
        }
        at += size_t(count) * stride;
    }
    return result;
}

std::shared_ptr<ExtractedWorld::Zone> ExtractedWorld::readZone(const fs::path &file, bool decode, std::string &error, bool storedOnly) {
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
        if (!storedOnly && (baseZ == INT32_MIN || baseZ > INT32_MAX - Z_OFFSET - 1))
            return fail("column height overflows engine coordinates");
        if (runs > (data.size() - at) / 8)
            return fail("truncated column runs");
        column.base = baseZ;
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
                (!storedOnly && int64_t(baseZ) + int64_t(columnBlocks) > INT32_MAX - Z_OFFSET - 1))
                return fail("invalid or excessive expanded block count");
            if (decode) column.blocks.insert(column.blocks.end(), count, block);
        }
    }
    if (at != data.size())
        return fail("unexpected trailing zone data");
    if (storedOnly)
        return zone;
    if (decode)
        zone->models = readModels(file);
    fs::path materialFile = file;
    materialFile.replace_extension(".cwm");
    std::error_code materialError;
    if (fs::exists(materialFile, materialError)) {
        constexpr size_t expected = 20 + 4096 * 6 * sizeof(float);
        if (fs::file_size(materialFile, materialError) != expected || materialError)
            return fail("invalid column material file size");
        std::ifstream material(materialFile, std::ios::binary);
        std::array<char, 4> magic{};
        std::array<int32_t, 4> header{};
        material.read(magic.data(), magic.size());
        material.read(reinterpret_cast<char *>(header.data()), sizeof header);
        if (!material || magic != std::array<char, 4>{'C', 'W', 'M', '1'} || header[0] != ax || header[1] != ay)
            return fail("invalid column material header");
        auto colours = std::make_shared<std::vector<worldgen::ColourInputs>>(4096);
        for (auto &colour : *colours) {
            std::array<float, 6> values;
            material.read(reinterpret_cast<char *>(values.data()), sizeof values);
            if (!material || !std::all_of(values.begin(), values.end(), [](float v) { return std::isfinite(v); }))
                return fail("invalid nonfinite column material input");
            colour.highlands = values[0];
            colour.warmth = values[1];
            colour.dryness = values[2];
            colour.lavaRegion = values[3];
            colour.forestRegion = values[4];
            colour.kind27 = values[5];
        }
        if (decode) {
            zone->recordedColours = std::move(colours);
            zone->recordedSand = {double(header[2]), double(header[3])};
        }
    } else if (materialError)
        return fail("column material file could not be inspected");
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
    int top = c.base - 1; // Filler below baseZ is solid.
    for (int i = (int)c.blocks.size() - 1; i >= 0; i--)
        if (solidType(c.blocks[i].type)) {
            top = c.base + i;
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

std::pair<float, bool> ExtractedWorld::objectScale(int type, const std::string &name, float scale) const {
    int width = 0;
    {
        std::lock_guard lock(cacheMutex);
        auto found = voxelWidths.find(name);
        if (found == voxelWidths.end()) {
            int32_t dimensions[3]{};
            std::ifstream model(spriteFolder / (name + ".cub"), std::ios::binary);
            if (model.read((char *)dimensions, sizeof dimensions) &&
                dimensions[0] > 0 && dimensions[1] > 0 && dimensions[2] > 0)
                width = dimensions[0];
            voxelWidths[name] = width;
        } else
            width = found->second;
    }
    if (width > 0)
        return {objectvisual::voxelScale(type, scale, width), false};
    // A missing owner model cannot render. Retain the legacy metadata fallback
    // for incomplete exports, with its existing provenance warning.
    return {scale * partScale(name), measuredScale.count(name) == 0};
}

std::optional<worldgen::MeshLightVolume> ExtractedWorld::lightsFor(int ax, int ay,
                                                                    const std::shared_ptr<Zone> (&around)[3][3]) {
    for (const auto &row : around)
        for (const auto &zone : row)
            if (!zone)
                return std::nullopt;
    std::vector<worldgen::TerrainColumn> columns[3][3];
    for (auto &row : columns)
        for (auto &zone : row)
            zone.resize(64 * 64);
    // The solver reads only64+8+8 columns. Copy that window, not all nine zones.
    for (int x = -8; x < 72; ++x)
        for (int y = -8; y < 72; ++y) {
            const int dx = x < 0 ? -1 : x >= 64 ? 1 : 0, dy = y < 0 ? -1 : y >= 64 ? 1 : 0;
            const int index = (x - dx * 64) * 64 + y - dy * 64;
            const auto &source = around[dy + 1][dx + 1]->columns[index];
            auto &target = columns[dy + 1][dx + 1][index];
            target.base = source.base;
            target.blocks.reserve(source.blocks.size());
            for (const auto &block : source.blocks)
                target.blocks.push_back({block.r, block.g, block.b, block.glow, block.type, block.flags});
        }
    return worldgen::meshZoneLight(ax, ay, [&](int32_t x, int32_t y) -> const auto & { return columns[y - ay + 1][x - ax + 1]; });
}

void ExtractedWorld::publishBlockLight(int ax, int ay, const std::shared_ptr<Zone> &source, const worldgen::MeshLightVolume &light) {
    auto edits = editBlocks();
    auto current = editedZone(ax, ay);
    if (!current)
        current = cached(ax, ay);
    if (!current || current->identity != source->identity)
        return;
    std::shared_ptr<Zone> draft;
    for (int x = 0; x < 64; ++x)
        for (int y = 0; y < 64; ++y) {
            const size_t column = size_t(x) * 64 + y;
            const Column &stored = current->columns[column];
            for (size_t z = 0; z < stored.blocks.size(); ++z) {
                const Block &block = stored.blocks[z];
                if (block.type != 0 && block.type != 2)
                    continue;
                const uint8_t red = light.at(int64_t(ax) * 64 + x, int64_t(ay) * 64 + y, int64_t(stored.base) + int64_t(z));
                if (red == block.r)
                    continue;
                if (!draft)
                    draft = writableZone(ax, ay);
                draft->columns[column].blocks[z].r = red;
            }
        }
    // No notice/history/dirty request: this is the completed solver's stored data.
    // A single safe engine snapshot replaces native per-record concurrent stores.
}

void ExtractedWorld::lightModels(int zx, int zy, std::vector<Scenery> &scenery, const worldgen::MeshLightVolume *solved) {
    if (std::none_of(scenery.begin(), scenery.end(), [](const Scenery &s) { return s.modelType >= 0; }))
        return;
    const int ax = zx + originX, ay = zy + originY;
    std::shared_ptr<Zone> neighbors[3][3];
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x) {
            neighbors[y + 1][x + 1] = load(ax + x, ay + y);
            if (!neighbors[y + 1][x + 1])
                return;
        }
    std::optional<worldgen::MeshLightVolume> computed;
    if (!solved || solved->columns.empty()) {
        computed = lightsFor(ax, ay, neighbors);
        if (!computed)
            return;
        solved = &*computed;
    }
    Zone &center = *neighbors[1][1];
    std::lock_guard modelLock(center.modelLights->mutex);
    for (Scenery &s : scenery)
        if (s.modelType >= 0) {
            const float sample = float(solved->fixed(s.modelPosition));
            s.light = sample / 255.0f;
            // A model may have moved to another index since an older mesh job read
            // its scenery. Such a job must not annotate the current vector's entry.
            const size_t index = size_t(s.listPosition - 1);
            if (!center.models || index >= center.models->size() || modelField<int32_t>((*center.models)[index], 0) != s.modelType ||
                std::memcmp((*center.models)[index].data() + 8, s.modelPosition.data(), sizeof(int64_t) * 3) != 0)
                continue;
            center.modelLights->lightSamples[s.listPosition] = sample;
            const auto &p = s.modelPosition;
            if (gameBlock(p[0] >> 16, p[1] >> 16, (int32_t)(p[2] >> 16))[4] == WATER_TYPE)
                center.modelLights->waterModels.insert(s.listPosition);
            if ((s.modelFlags & 8) || center.modelLights->waterModels.count(s.listPosition)) {
                s.modelFlags |= 8;
                center.modelLights->waterModels.insert(s.listPosition);
            }
        }
}

std::optional<scenecapture::MeshCache> ExtractedWorld::meshCapture(int zx, int zy) const {
    const int64_t ax = (int64_t)zx + originX, ay = (int64_t)zy + originY;
    if (generatedMode || ax < INT32_MIN || ax > INT32_MAX || ay < INT32_MIN || ay > INT32_MAX)
        return std::nullopt;
    const auto path = folder / ("z_" + std::to_string(ax) + "_" + std::to_string(ay) + ".rec");
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in || in.tellg() < 4 || in.tellg() > 128 * 1024 * 1024)
        return std::nullopt;
    std::vector<uint8_t> bytes((size_t)in.tellg());
    in.seekg(0);
    if (!in.read((char *)bytes.data(), bytes.size()) || std::memcmp(bytes.data(), "CWR1", 4))
        return std::nullopt;
    std::optional<scenecapture::MeshCache> captured;
    for (size_t at = 4; at < bytes.size();) {
        if (bytes.size() - at < 12)
            return std::nullopt;
        uint32_t header[3];
        std::memcpy(header, bytes.data() + at, sizeof header);
        at += sizeof header;
        const size_t stride = header[1], count = header[2];
        if (!stride || count > (bytes.size() - at) / stride)
            return std::nullopt;
        if (header[0] == scenecapture::MESH_CACHE_RECORD) {
            if (captured || count != 1)
                return std::nullopt;
            captured = scenecapture::decode(bytes.data() + at, stride);
            if (!captured)
                return std::nullopt;
        }
        at += stride * count;
    }
    if (captured) {
        // Native horizontal cache bounds are fixed by the zone, not by its models.
        if (captured->minimum[0] != ax * 64 * 65536 || captured->maximum[0] != (ax * 64 + 65) * 65536 ||
            captured->minimum[1] != ay * 64 * 65536 || captured->maximum[1] != (ay * 64 + 65) * 65536)
            return std::nullopt;
    }
    return captured;
}

std::vector<Scenery> ExtractedWorld::props(int zx, int zy, std::shared_ptr<const void> *modelSnapshot) {
    int64_t ax = (int64_t)zx + originX, ay = (int64_t)zy + originY;
    if (ax < INT32_MIN || ax > INT32_MAX || ay < INT32_MIN || ay > INT32_MAX)
        return {};
    return propsAbsolute((int)ax, (int)ay, modelSnapshot);
}

std::vector<Scenery> ExtractedWorld::propsAbsolute(int ax, int ay, std::shared_ptr<const void> *modelSnapshot) {
    auto zone = editedZone(ax, ay);
    if (!zone)
        zone = load(ax, ay);
    if (!zone)
        return {};
    return propsForZone(zone, ax, ay, modelSnapshot, true);
}

std::optional<ExtractedWorld::LoadedObjectProps> ExtractedWorld::loadedObjectProps(int32_t ax, int32_t ay,
                                                                                   const std::vector<nativesaves::Record> *records) {
    std::shared_ptr<const Zone> zone;
    {
        std::lock_guard lock(cacheMutex);
        for (const auto &[key, cachedZone] : cache)
            if (key == Key{ax, ay} && cachedZone) {
                zone = cachedZone;
                break;
            }
    }
    if (!zone)
        return {};
    return LoadedObjectProps{zone->identity, records ? objectProps(*records) : propsForZone(zone, ax, ay, nullptr, false)};
}

std::optional<ExtractedWorld::CapturedLists> ExtractedWorld::capturedLists(int32_t ax, int32_t ay) const {
    {
        std::lock_guard lock(cacheMutex);
        const auto found = std::find_if(cache.begin(), cache.end(), [&](const auto &entry) { return entry.first == Key{ax, ay}; });
        if (found == cache.end() || !found->second || generatedMode || found->second->generated)
            return {};
    }
    return readCapturedLists(ax, ay);
}

std::optional<nativesaves::Item> ExtractedWorld::capturedObjectReward(int32_t ax, int32_t ay, int32_t index) const {
    std::lock_guard lock(cacheMutex);
    for (const auto &[key, zone] : cache)
        if (key == Key{ax, ay} && zone && !generatedMode && !zone->generated && zone->objectRewards) {
            const auto found = zone->objectRewards->find(index);
            return found == zone->objectRewards->end() ? nativesaves::Item{} : found->second;
        }
    return {};
}

ExtractedWorld::CapturedLists ExtractedWorld::readCapturedLists(int32_t ax, int32_t ay, bool includeMarks) const {
    CapturedLists result;
    std::ifstream input(folder / ("z_" + std::to_string(ax) + "_" + std::to_string(ay) + ".rec"), std::ios::binary | std::ios::ate);
    if (!input || input.tellg() < 4 || input.tellg() > 128 * 1024 * 1024)
        return result;
    std::vector<uint8_t> bytes(size_t(input.tellg()));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(bytes.data()), bytes.size()) || std::memcmp(bytes.data(), "CWR1", 4))
        return result;
    bool rewardsSeen = false, occupancySeen = false;
    for (size_t at = 4; at + 12 <= bytes.size();) {
        uint32_t tag, stride, count;
        std::memcpy(&tag, bytes.data() + at, 4);
        std::memcpy(&stride, bytes.data() + at + 4, 4);
        std::memcpy(&count, bytes.data() + at + 8, 4);
        at += 12;
        if (!stride || count > (bytes.size() - at) / stride) {
            if (tag == objectoccupancy::Chunk)
                result.markedObjectBlocks.reset();
            break;
        }
        if (tag == objectrewards::CHUNK) {
            if (!rewardsSeen && stride == objectrewards::RECORD_BYTES)
                result.objectRewards = objectrewards::decode(bytes.data() + at, size_t(count) * stride);
            else
                result.objectRewards.reset();
            rewardsSeen = true;
        }
        if (includeMarks && tag == objectoccupancy::Chunk) {
            if (!occupancySeen && stride == objectoccupancy::RecordBytes)
                result.markedObjectBlocks = objectoccupancy::decode(bytes.data() + at, size_t(count) * stride);
            else
                result.markedObjectBlocks.reset();
            occupancySeen = true;
        }
        auto *destination = tag == 0x30 ? &result.groundItems : tag == 0x60 ? &result.staticObjects : nullptr;
        if (destination && stride == sizeof(nativesaves::Record)) {
            const size_t first = destination->size();
            destination->resize(first + count);
            std::memcpy(destination->data() + first, bytes.data() + at, size_t(count) * stride);
        }
        at += size_t(count) * stride;
    }
    return result;
}

std::vector<Scenery> ExtractedWorld::objectProps(std::span<const nativesaves::Record> records) {
    std::vector<Scenery> out;
    for (size_t index = 0; index < records.size(); ++index) {
        const auto &record = records[index];
        const auto state = objectvisual::read(record.data());
        if (state.type < 0 || state.type >= int(buildingParts.size()) || buildingParts[state.type].empty() || !(state.scale > 0) ||
            !std::isfinite(state.scale))
            continue;
        std::array<int64_t, 3> position;
        std::memcpy(position.data(), record.data() + 8, sizeof position);
        const glm::vec3 local = toLocal(position.data());
        const auto [size, estimated] = objectScale(state.type, buildingParts[state.type], state.scale);
        Scenery placed{buildingParts[state.type], local.x, local.y, local.z, state.turn * 1.5707963f, size, 0, estimated};
        placed.object = state;
        placed.objectKey = {state.type, int(index), position};
        out.push_back(std::move(placed));
    }
    return out;
}

std::vector<Scenery> ExtractedWorld::propsForZone(const std::shared_ptr<const Zone> &zone, int ax, int ay,
                                                  std::shared_ptr<const void> *modelSnapshot, bool includeModels) {
    std::vector<Scenery> out;
    if (modelSnapshot)
        *modelSnapshot = zone->models;
    if (includeModels && zone->models) {
        std::lock_guard lock(zone->modelLights->mutex);
        for (size_t index = 0; index < zone->models->size(); ++index) {
            const auto &record = (*zone->models)[index];
            const int32_t type = modelField<int32_t>(record, 0);
            std::array<int64_t, 3> position;
            std::memcpy(position.data(), record.data() + 8, sizeof position);
            const float size = modelField<float>(record, 32), yaw = modelField<float>(record, 36);
            const glm::vec3 point{float(double(gamemath::subtractFixed(position[0], originDotsX())) / 65536.0),
                                  float(double(gamemath::subtractFixed(position[1], originDotsY())) / 65536.0),
                                  float(double(position[2]) / 65536.0 + Z_OFFSET)};
            if (type == 0x59 && std::isfinite(size) && std::isfinite(yaw)) {
                Scenery shaft{};
                shaft.x = point.x;
                shaft.y = point.y;
                shaft.z = point.z;
                shaft.beam = lightbeam::Placement{{gamemath::subtractFixed(position[0], originDotsX()),
                                                   gamemath::subtractFixed(position[1], originDotsY()),
                                                   gamemath::addFixed(position[2], int64_t(Z_OFFSET) * 65536)},
                                                  size,
                                                  yaw};
                out.push_back(std::move(shaft));
                continue;
            }
            if (type < 0 || type >= int(environment.size()) || environment[type].empty() || !(size > 0) || !std::isfinite(size) ||
                !std::isfinite(yaw))
                continue;
            Scenery model{
                environment[type], point.x, point.y, point.z, glm::radians(yaw), size, 0, false, modelField<int32_t>(record, 0x38),
                int(index) + 1};
            model.modelType = type;
            model.modelPosition = position;
            if (zone->modelLights->waterModels.count(model.listPosition))
                model.modelFlags |= 8;
            const auto cached = zone->modelLights->lightSamples.find(model.listPosition);
            const float sample = cached == zone->modelLights->lightSamples.end() ? modelField<float>(record, 0x28) : cached->second;
            if (std::isfinite(sample) && sample >= 0 && sample <= 255)
                model.light = sample / 255.0f;
            out.push_back(std::move(model));
        }
    }
    if (generatedMode || zone->generated) {
        const auto local = [&](int64_t x, int64_t y, int64_t z) {
            const int64_t position[3]{x, y, z};
            return toLocal(position);
        };
        int objectIndex = 0;
        for (const worldgen::ZoneObject &o : zone->objects) {
            const int index = objectIndex++;
            if (o.type < 0 || o.type >= (int)buildingParts.size() || buildingParts[o.type].empty() || !(o.scale > 0) || !std::isfinite(o.scale))
                continue;
            const glm::vec3 p = local(o.x, o.y, o.z);
            const auto [scale, estimated] = objectScale(o.type, buildingParts[o.type], o.scale);
            Scenery placed{buildingParts[o.type], p.x, p.y, p.z, o.turn * 1.5707963f, scale, 0, estimated};
            placed.object = {o.type, o.turn, (uint8_t)o.active, o.scale, o.swing};
            placed.objectKey = {o.type, index, {o.x, o.y, o.z}};
            out.push_back(std::move(placed));
        }
        return out;
    }
    const auto objects = objectProps(readCapturedLists(ax, ay).staticObjects);
    out.insert(out.end(), objects.begin(), objects.end());
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
    if (gz < c.base)
        return true;
    size_t i = (size_t)(gz - c.base);
    return i < c.blocks.size() && solidType(c.blocks[i].type);
}

bool ExtractedWorld::waterAt(double x, double y, double z) {
    int64_t bx = (int64_t)std::floor(x), by = (int64_t)std::floor(y);
    auto zone = load((int)floorDiv(bx, 64) + originX, (int)floorDiv(by, 64) + originY);
    if (!zone)
        return false;
    const Column &c = zone->columns[(bx - floorDiv(bx, 64) * 64) * 64 + (by - floorDiv(by, 64) * 64)];
    int gz = (int)std::floor(z) - Z_OFFSET;
    if (gz < c.base)
        return false;
    size_t i = (size_t)(gz - c.base);
    if (i >= c.blocks.size())
        return gz <= 0;
    const Block &b = c.blocks[i];
    return b.type == WATER_TYPE || (b.type == AIR_TYPE && gz <= 0 && !(b.flags & DRY_AIR));
}

bool ExtractedWorld::openToSky(double x, double y, double z) {
    int64_t bx = (int64_t)std::floor(x), by = (int64_t)std::floor(y);
    auto zone = load((int)floorDiv(bx, 64) + originX, (int)floorDiv(by, 64) + originY);
    if (!zone)
        return true;
    const Column &c = zone->columns[(bx - floorDiv(bx, 64) * 64) * 64 + (by - floorDiv(by, 64) * 64)];
    int gz = (int)std::floor(z) - Z_OFFSET;
    if (gz < c.base)
        return false;
    for (size_t i = (size_t)(gz - c.base); i < c.blocks.size(); i++)
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
        if (gz < c.base) // filler below the stored blocks (none in an empty column)
            return c.blocks.empty() ? BlockKind::Open : BlockKind::Solid;
        size_t i = (size_t)(gz - c.base);
        if (i >= c.blocks.size())
            return BlockKind::Open;
        uint8_t t = c.blocks[i].type;
        return t == LAVA_TYPE ? BlockKind::Glow : solidType(t) ? BlockKind::Solid : BlockKind::Open;
    };
    const int g0 = z0 - Z_OFFSET;
    for (int k = 0; k < count; k++)
        out[k] = (uint8_t)kindAt(g0 + k);
    for (int gz = std::max(g0 + count, c.base); gz < c.base + (int)c.blocks.size(); gz++)
        if (kindAt(gz) != BlockKind::Open)
            return false;
    return g0 + count >= c.base || c.blocks.empty();
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
    for (int z = start; z >= std::min(start, c.base) - 1; z--) {
        if (!columnSolid(c.base, types, z))
            continue;
        bool open = true;
        for (int k = 1; k <= headroom && open; k++)
            open = !columnSolid(c.base, types, z + k);
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

std::optional<scenevisibility::Bounds> ExtractedWorld::boundsFor(int ax, int ay,
                                                                         const std::shared_ptr<Zone> (&around)[3][3]) {
    for (const auto &row : around)
        for (const auto &zone : row)
            if (!zone)
                return std::nullopt;
    auto column = [&](int64_t x, int64_t y) -> const Column & {
        const int lx = (int)(x - (int64_t)ax * 64), ly = (int)(y - (int64_t)ay * 64);
        const int dx = lx < 0 ? -1 : lx >= 64 ? 1 : 0, dy = ly < 0 ? -1 : ly >= 64 ? 1 : 0;
        return around[dy + 1][dx + 1]->columns[(lx - dx * 64) * 64 + ly - dy * 64];
    };
    return scenevisibility::build(
        ax, ay,
        [&](int64_t x, int64_t y) -> std::optional<scenevisibility::Column> {
            const auto &c = column(x, y);
            return scenevisibility::Column{c.base, c.base + (int)c.blocks.size()};
        },
        [&](int64_t x, int64_t y, int64_t z) -> uint8_t {
            const auto &c = column(x, y);
            if (z < c.base)
                return 1;
            const uint64_t index = (uint64_t)(z - c.base);
            if (index < c.blocks.size()) {
                const auto &b = c.blocks[(size_t)index];
                if (b.type || z > 0 || (b.flags & DRY_AIR))
                    return b.type;
            } else if (z > 0)
                return 0;
            return 2;
        });
}

std::optional<scenevisibility::Bounds> ExtractedWorld::meshBounds(int zx, int zy) {
    const int64_t ax = (int64_t)zx + originX, ay = (int64_t)zy + originY;
    if (ax <= INT32_MIN || ax >= INT32_MAX || ay <= INT32_MIN || ay >= INT32_MAX)
        return std::nullopt;
    std::shared_ptr<Zone> around[3][3];
    for (int j = -1; j <= 1; ++j)
        for (int i = -1; i <= 1; ++i)
            around[j + 1][i + 1] = load((int)ax + i, (int)ay + j);
    return boundsFor((int)ax, (int)ay, around);
}

bool ExtractedWorld::mesh(int zx, int zy, Mesh &opaque, Mesh &water, std::optional<scenevisibility::Bounds> *nativeBounds,
                          worldgen::MeshLightVolume *nativeLight, Mesh *lava, std::vector<TerrainMeshBand> *terrainBands) {
    meshprofile::Sample profile(zx, zy, renderStyle() == RenderStyle::Alpha);
    const size_t opaqueStart = opaque.indices.size(), waterStart = water.indices.size(), lavaStart = lava ? lava->indices.size() : 0;
    if (terrainBands)
        terrainBands->clear();
    if (nativeLight)
        *nativeLight = {};
    if (nativeBounds)
        nativeBounds->reset();
    const int64_t absoluteX = (int64_t)zx + originX, absoluteY = (int64_t)zy + originY;
    if (absoluteX <= INT32_MIN || absoluteX >= INT32_MAX || absoluteY <= INT32_MIN || absoluteY >= INT32_MAX)
        return false;
    const int ax = (int)absoluteX, ay = (int)absoluteY;
    std::shared_ptr<Zone> around[3][3];
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++)
            around[j + 1][i + 1] = load(ax + i, ay + j);
    profile.mark(meshprofile::Load);
    auto self = around[1][1];
    if (!self)
        return false;
    const bool alphaLook = renderStyle() == RenderStyle::Alpha;
    const auto nativeBox = (!alphaLook || nativeBounds) ? boundsFor(ax, ay, around) : std::nullopt;
    if (nativeBounds)
        *nativeBounds = nativeBox;
    profile.mark(meshprofile::Bounds);
    worldgen::MeshLightVolume localLight;
    auto &solvedLight = nativeLight ? *nativeLight : localLight;
    // Gameplay light is independent of the selected rendering palette. Publish
    // target stored-air/water red before model samples, geometry and GPU install.
    if (auto solved = lightsFor(ax, ay, around)) {
        solvedLight = std::move(*solved);
        profile.mark(meshprofile::Light);
        publishBlockLight(ax, ay, self, solvedLight);
        profile.mark(meshprofile::PublishLight);
    }
    const bool nativeEncoding = !alphaLook && !solvedLight.columns.empty();
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
        if (z < c.base) {
            if (c.blocks.empty())
                return false;
            out = c.blocks.front();
            if (!solidType(out.type))
                out.type = 1; // Solid filler.
            return true;
        }
        size_t i = (size_t)(z - c.base);
        if (i >= c.blocks.size())
            return false;
        out = c.blocks[i];
        return true;
    };
    auto effectiveBlock = [&](int x, int y, int64_t z) -> Block {
        const auto &c = column(x, y);
        if (z < c.base)
            return {200, 200, 200, 0, 1, 0};
        const uint64_t index = (uint64_t)(z - c.base);
        if (index < c.blocks.size()) {
            auto block = c.blocks[(size_t)index];
            // Native2a3260 writes solved red onto target-zone stored air/water
            // before mesh queries. Overlay it on this immutable job snapshot.
            if (nativeEncoding && x >= 0 && y >= 0 && x < 64 && y < 64 && (block.type == 0 || block.type == 2))
                block.r = solvedLight.at((int64_t)ax * 64 + x, (int64_t)ay * 64 + y, z);
            if (block.type || z > 0 || (block.flags & DRY_AIR))
                return block;
        } else if (z > 0)
            return {255, 255, 255, 0, 0, 0};
        return {255, 255, 255, 0, 2, 0};
    };
    const steamvoxel::Probe nativeProbe = [&](steamvoxel::Point p) {
        const auto block = effectiveBlock((int)p.x, (int)p.y, p.z);
        return steamvoxel::Sample{block.type == 0 || block.type == 2,
            solvedLight.at((int64_t)ax * 64 + p.x, (int64_t)ay * 64 + p.y, p.z)};
    };
    // Height range worth meshing: from the lowest surface in the 66x66 area
    // (anything lower is hidden) to the highest stored block.
    int lo = INT32_MAX, hi = INT32_MIN;
    for (int ly = -1; ly <= 64; ly++)
        for (int lx = -1; lx <= 64; lx++) {
            const Column &c = column(lx, ly);
            lo = std::min(lo, c.base - 1);
            hi = std::max(hi, c.base + (int)c.blocks.size());
        }
    if (lo > hi) {
        profile.complete(lo, hi, 0, 0, 0);
        return true;
    }
    auto solid = [&](int lx, int ly, int z) {
        if (z <= lo)
            return true; // Below the meshed range: buried.
        if (nativeEncoding) {
            const uint8_t type = effectiveBlock(lx, ly, z).type;
            return type != 0 && type != 2; // rendering is distinct from collision solidity
        }
        Block b;
        return blockAt(column(lx, ly), z, b) && solidType(b.type);
    };
    glm::vec3 offset((float)zx * 64, (float)zy * 64, (float)Z_OFFSET);
    // meshFaces samples each voxel's colour before shading that voxel's corners.
    // Preserve implicit Alpha RGB in float throughout those corner calls.
    std::optional<glm::vec3> implicitColour;
    auto colour = [&](int lx, int ly, int z) {
        implicitColour.reset();
        const auto &stored = column(lx, ly);
        Block b{};
        blockAt(stored, z, b);
        if (stored.fullColour && z < stored.base) {
            implicitColour = worldgen::alphaImplicitColour(stored, int32_t(int64_t(ax) * 64 + lx), int32_t(int64_t(ay) * 64 + ly), z);
            if (implicitColour && !alphaLook) {
                b.r = alphalight::toByte(implicitColour->r);
                b.g = alphalight::toByte(implicitColour->g);
                b.b = alphalight::toByte(implicitColour->b);
            }
        }
        // Steam bytes are half intensity; Alpha columns retain their original
        // full RGB. Retained Steam construction columns keep the native encoding.
        if (column(lx, ly).fullColour) {
            if (nativeEncoding) {
                b.r /= 2;
                b.g /= 2;
                b.b /= 2;
            } else {
                VoxelColor c{b.r, b.g, b.b, 0};
                if (b.type == LEAVES_TYPE || b.type == SNOW_LEAVES_TYPE)
                    c.flags |= SWAY;
                return c;
            }
        }
        VoxelColor c = nativeEncoding ? VoxelColor{b.r, b.g, b.b, STEAM_VERTEX}
                                      : VoxelColor{(uint8_t)std::min(255, b.r * 2), (uint8_t)std::min(255, b.g * 2),
                                                   (uint8_t)std::min(255, b.b * 2), 0};
        // Vanilla never moves a block; the leaf sway is CubeVulkan's WorldMotion option
        // ([Motion] LeafSway, off by default), which the vertex shader applies to these.
        if (b.type == LEAVES_TYPE || b.type == SNOW_LEAVES_TYPE)
            c.flags |= SWAY;
        return c;
    };
    std::map<int32_t, TerrainMeshBand> bands;
    const bool localWater = nativeEncoding && terrainBands && nativeBox;
    auto waterGroup = [&](int32_t z) -> Mesh & {
        if (!localWater)
            return water;
        const int32_t base = terrainBandBase(z, nativeBox->lowest);
        auto &band = bands[base];
        band.base = base;
        band.water.localOrigin = glm::ivec3(0, 0, base);
        return band.water;
    };
    // The Alpha look bakes the block light (docs/specs/alpha-rendering.md 9.2-9.4): the light
    // is flooded through the zone and a 16-block border, then every corner samples it.
    profile.mark(meshprofile::Setup);
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
        profile.mark(meshprofile::AlphaLight);
        alphalight::CornerCache corners(*light, {0, 0}, {65, 65});
        auto shade = [&](int x, int y, int z, int face, int k, VoxelColor c) {
            const int *corner = FACE_CORNERS[face][k];
            const auto sample = corners.sample({x + corner[0], y + corner[1], z + corner[2]}, face);
            const auto rgb = implicitColour.value_or(glm::vec3(c.r, c.g, c.b) / 255.f);
            return alphalight::shadeCorner(sample, rgb);
        };
        meshFaces({0, 0, lo + 1}, {64, 64, hi + 1}, solid, colour, shade, opaque, 1.f, offset);
    } else if (nativeEncoding) {
        auto shade = [&](int x, int y, int z, int face, int k, VoxelColor c) {
            const int *corner = steamvoxel::CORNERS[face][k];
            const auto encoded = steamvoxel::terrainCorner(glm::vec4(glm::vec3(c.r, c.g, c.b) / 255.0f, 1),
                {x + corner[0], y + corner[1], z + corner[2]}, face, nativeProbe, effectiveBlock(x, y, z).type == 0x14);
            return CornerShade{encoded.r, encoded.g, encoded.b, encoded.a};
        };
        const MeshSelector group = [&](int x, int y, int z) -> Mesh & {
            if (terrainBands && nativeBox) {
                const int32_t base = terrainBandBase(z, nativeBox->lowest);
                auto &band = bands[base];
                band.base = base;
                Mesh &mesh = effectiveBlock(x, y, z).type == LAVA_TYPE ? band.lava : band.opaque;
                mesh.localOrigin = glm::ivec3(0, 0, base);
                return mesh;
            }
            return lava && effectiveBlock(x, y, z).type == LAVA_TYPE ? *lava : opaque;
        };
        meshFaces({0, 0, lo + 1}, {64, 64, hi + 1}, solid, colour, shade, opaque, 1.f, offset,
                  VoxelLayout::SteamTerrain, group);
    } else
        meshVoxels({0, 0, lo + 1}, {64, 64, hi + 1}, solid, colour, opaque, 1.f, offset);
    profile.mark(meshprofile::Solid);
    // Water geometry from water-rendering.md A.1/B.1: exposed full block
    // faces, including sides/bottoms, and a coplanar film over exposed wet ground.
    auto typeAt = [&](int x, int y, int z) {
        if (nativeEncoding)
            return effectiveBlock(x, y, z).type;
        Block block{};
        blockAt(column(x, y), z, block);
        if (block.type == AIR_TYPE && z <= 0 && !(block.flags & DRY_AIR))
            return WATER_TYPE;
        return block.type;
    };
    bool wholeWater = nativeEncoding && nativeBox && nativeBox->lowest < 1 && nativeBox->upper >= 0;
    for (int x = 0; x < 64 && wholeWater; ++x)
        for (int y = 0; y < 64; ++y)
            if (effectiveBlock(x, y, 0).type != WATER_TYPE || effectiveBlock(x, y, 1).r != 255) {
                wholeWater = false;
                break;
            }
    if (wholeWater) {
        Mesh &target = waterGroup(0);
        const uint32_t base = (uint32_t)target.vertices.size();
        const glm::ivec2 corners[]{{0,0}, {64,0}, {64,64}, {0,64}};
        for (auto corner : corners) {
            const glm::vec3 at = localWater
                ? glm::vec3(corner, float(int64_t(1) - target.localOrigin->z))
                : offset + glm::vec3(corner, 1);
            target.vertices.push_back({at.x, at.y, at.z, 63, 127, 127, 255, 4, WATER_SURFACE | STEAM_VERTEX, 0, 0});
            target.min = glm::min(target.min, at);
            target.max = glm::max(target.max, at);
        }
        for (uint32_t index : steamvoxel::CUB_TRIANGLES)
            target.indices.push_back(base + index);
    }
    auto emitWater = [&](int x, int y, int z, int face) {
        Mesh &target = waterGroup(z);
        const uint32_t base = (uint32_t)target.vertices.size();
        CornerShade corners[4];
        for (int k = 0; k < 4; k++) {
            const int *corner = nativeEncoding ? steamvoxel::CORNERS[face][k] : FACE_CORNERS[face][k];
            const glm::vec3 at = localWater
                ? glm::vec3(float(x + corner[0]), float(y + corner[1]), float(int64_t(z) - target.localOrigin->z + corner[2]))
                : offset + glm::vec3(x + corner[0], y + corner[1], z + corner[2]);
            CornerShade shade{128, 255, 255, 255};
            if (alphaLook) {
                glm::vec3 rgb = alphalight::waterColour(light->light({x, y, z}) / 255.f);
                shade = alphalight::shadeCorner(*light, {x + corner[0], y + corner[1], z + corner[2]}, face,
                    VoxelColor{alphalight::toByte(rgb.r), alphalight::toByte(rgb.g), alphalight::toByte(rgb.b), 0});
            } else if (nativeEncoding) {
                const auto encoded = steamvoxel::terrainCorner({.25f, .5f, .5f, 1},
                    {x + corner[0], y + corner[1], z + corner[2]}, 4, nativeProbe);
                shade = {encoded.r, encoded.g, encoded.b, encoded.a};
            }
            corners[k] = shade;
            const uint8_t flags = WATER_SURFACE | (nativeEncoding ? STEAM_VERTEX : 0);
            target.vertices.push_back({at.x, at.y, at.z, shade.r, shade.g, shade.b, shade.a,
                                       (uint8_t)(alphaLook ? face : 4), flags, 0, 0});
            target.min = glm::min(target.min, at);
            target.max = glm::max(target.max, at);
        }
        if (nativeEncoding)
            appendFaceIndices(target, base, corners, VoxelLayout::SteamTerrain);
        else
            for (uint32_t index : steamvoxel::CUB_TRIANGLES)
                target.indices.push_back(base + index);
    };
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            for (int z = lo + 1; z <= std::max(hi, 0); z++) {
                uint8_t type = typeAt(x, y, z);
                if (type == WATER_TYPE) {
                    if (wholeWater && z == 0)
                        continue;
                    for (int index = 0; index < 6; index++) {
                        const int face = nativeEncoding ? steamvoxel::FACE_ORDER[index] : index;
                        const int *normal = FACE_NORMALS[face];
                        if (typeAt(x + normal[0], y + normal[1], z + normal[2]) == AIR_TYPE)
                            emitWater(x, y, z, face);
                    }
                } else if (type == WET_TYPE && typeAt(x, y, z + 1) == AIR_TYPE)
                    emitWater(x, y, z, 4);
            }
    profile.mark(meshprofile::Water);
    size_t solidIndices = opaque.indices.size() - opaqueStart;
    size_t lavaIndices = lava ? lava->indices.size() - lavaStart : 0;
    size_t waterIndices = water.indices.size() - waterStart;
    if (terrainBands)
        for (auto &[base, band] : bands) {
            if (band.opaque.empty() && band.lava.empty() && band.water.empty())
                continue;
            solidIndices += band.opaque.indices.size();
            lavaIndices += band.lava.indices.size();
            waterIndices += band.water.indices.size();
            terrainBands->push_back(std::move(band));
        }
    if (nativeEncoding && nativeBounds && *nativeBounds)
        (*nativeBounds)->triangleCost = scenerange::triangleCost(solidIndices, waterIndices, lavaIndices);
    profile.complete(lo, hi, solidIndices, lavaIndices, waterIndices);
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
