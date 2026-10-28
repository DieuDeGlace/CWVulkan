#include "PyroCreatures.h"

#include "worldgen/Residents.h"
#include "NativeRules.h"
#include "Progression.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace pyrocreatures {
namespace {
template <class T> T get(const uint8_t *r, size_t at) {
    T v;
    std::memcpy(&v, r + at, sizeof v);
    return v;
}
// The game's hostility kinds the mod leaves alone (EntityBehaviour Player, Pet).
bool ownLevel(const uint8_t *c) {
    const uint8_t hostility = c[worldgen::creature::HOSTILITY];
    return hostility == 0 || hostility == 6;
}
// The mod's creature scaling by stat (m_CreatureScaling).
constexpr float HEALTH = 2, ARMOR = 0.01f, RESISTANCE = 0.01f, ATTACK = 3, SPELL = 3;
} // namespace

void RelativeLevels::open(int32_t worldSeed, const std::string &worldName, const std::string &characterName, bool locked,
                     const std::filesystem::path &gameFile, const std::filesystem::path &ownFile, bool keepNew) {
    std::lock_guard hold(lock);
    // The mod's session(): its identity, and the entries for every character and world.
    identity = progression::mix((uint32_t)worldSeed);
    for (unsigned char c : worldName)
        identity = progression::mix(identity ^ c);
    for (size_t n = 0; n < 16 && n < characterName.size() && characterName[n]; n++)
        identity = progression::mix(identity ^ (uint8_t)characterName[n]);
    lockedRegions = locked;
    ownPath = ownFile;
    keep = keepNew;
    regions.clear();
    for (const auto &file : {gameFile, ownFile}) {
        std::ifstream in(file);
        char kind;
        uint64_t key;
        int level;
        while (in >> kind >> key >> level)
            if (level > 0 && level <= 10000000 && (kind == 'R' || kind == 'G'))
                (kind == 'R' ? regions : gear)[key] = level;
    }
}

int RelativeLevels::give(char kind, std::map<uint64_t, int> &to, uint64_t k, int level) {
    auto [it, added] = to.emplace(k, std::max(1, level));
    if (added && keep && !ownPath.empty()) {
        std::ofstream out(ownPath, std::ios::app);
        out << kind << ' ' << k << ' ' << it->second << '\n';
    }
    return it->second;
}

uint64_t RelativeLevels::itemKey(int category, int id, uint32_t modifier, int material) const {
    uint64_t h = identity;
    for (uint64_t v : {(uint64_t)(uint8_t)category, (uint64_t)(uint32_t)id, (uint64_t)modifier, (uint64_t)(uint8_t)material})
        h = progression::mix(h ^ v);
    return h;
}

int RelativeLevels::itemLevel(int category, int id, uint32_t modifier, int material, int playerLevel) {
    std::lock_guard hold(lock);
    const uint64_t k = itemKey(category, id, modifier, material);
    if (auto it = gear.find(k); it != gear.end())
        return it->second;
    return give('G', gear, k, progression::lootLevel(playerLevel, playerLevel, k));
}

int RelativeLevels::dropped(int category, int id, uint32_t modifier, int material, int playerLevel, int enemyLevel) {
    std::lock_guard hold(lock);
    const uint64_t k = itemKey(category, id, modifier, material);
    return give('G', gear, k, progression::lootLevel(playerLevel, enemyLevel, k));
}

void RelativeLevels::own(int category, int id, uint32_t modifier, int material, int level) {
    std::lock_guard hold(lock);
    give('G', gear, itemKey(category, id, modifier, material), level);
}

int RelativeLevels::keptLevel(int category, int id, uint32_t modifier, int material) const {
    std::lock_guard hold(lock);
    auto it = gear.find(itemKey(category, id, modifier, material));
    return it != gear.end() ? it->second : 0;
}

int RelativeLevels::legacyLevel(worldgen::Int2 itemRegion, worldgen::Int2 playerRegion, uint32_t modifier) {
    const int distance = std::max(std::abs(playerRegion.x - itemRegion.x), std::abs(playerRegion.y - itemRegion.y));
    return progression::ownedItemLevel(distance, modifier);
}

uint64_t RelativeLevels::key(worldgen::Int2 r) const {
    return progression::mix(identity ^ progression::regionRoll(0, r.x, r.y));
}

int RelativeLevels::basis(worldgen::Int2 r, int playerLevel) const {
    std::lock_guard hold(lock);
    if (!lockedRegions)
        return playerLevel;
    auto it = regions.find(key(r));
    return it != regions.end() ? it->second : playerLevel;
}

void RelativeLevels::visit(worldgen::Int2 r, int playerLevel) {
    std::lock_guard hold(lock);
    if (!lockedRegions)
        return;
    give('R', regions, key(r), playerLevel);
}

size_t RelativeLevels::size() const {
    std::lock_guard hold(lock);
    return regions.size();
}

bool RelativeLevels::lockedInGame(const std::filesystem::path &gameFile) {
    std::ifstream in(gameFile);
    char kind;
    uint64_t key;
    int level;
    while (in >> kind >> key >> level)
        if (kind == 'R')
            return true;
    return false;
}

int level(const uint8_t *c, int32_t worldSeed, int playerLevel) {
    return level(c, worldSeed, [playerLevel](worldgen::Int2) { return playerLevel; });
}

int level(const uint8_t *c, int32_t worldSeed, const std::function<int(worldgen::Int2)> &basis) {
    if (ownLevel(c))
        return get<int32_t>(c, worldgen::creature::LEVEL);
    worldgen::Int2 r{get<int32_t>(c, worldgen::creature::REGION), get<int32_t>(c, worldgen::creature::REGION + 4)};
    if (r.x == 0 && r.y == 0) { // (none kept: the region of its block, 0x2ada20)
        auto block = [](int64_t n) { return (int32_t)(n / 65536 - (n < 0 && n % 65536 != 0)); };
        r = worldgen::regionAt(block(get<int64_t>(c, worldgen::creature::POSITION)), block(get<int64_t>(c, worldgen::creature::POSITION + 8)));
    }
    return progression::enemyLevel(basis(r), worldgen::regionBiome(r.x, r.y), progression::regionRoll(worldSeed, r.x, r.y),
                                   (uint64_t)get<int64_t>(c, worldgen::creature::ID));
}

std::string levelText(int level) {
    char text[32];
    const double v = level;
    if (v > 1e6)
        std::snprintf(text, sizeof text, "LV %.2fM ", v / 1e6);
    else if (v > 1e3)
        std::snprintf(text, sizeof text, "LV %.2fK ", v / 1e3);
    else
        std::snprintf(text, sizeof text, "LV %.0f ", v);
    return text;
}

StatHooks::StatHooks(int32_t worldSeed, std::function<int(worldgen::Int2)> basis) {
    // ApplyCreatureBuff: stat * 0.05 * (1 + scaling) * (1 + log2((level + 1001) / 1000) * 1000).
    auto buff = [worldSeed, basis](float scaling) {
        return [=](const uint8_t *c, float &stat) {
            if (!ownLevel(c))
                stat *= 0.05f * (1 + scaling) * progression::creatureScale(level(c, worldSeed, basis));
        };
    };
    handle = nativerules::addCallbacks({buff(HEALTH), buff(ATTACK), buff(SPELL), buff(ARMOR), buff(RESISTANCE)});
}

StatHooks::~StatHooks() {
    nativerules::removeCallbacks(handle);
}

} // namespace pyrocreatures
