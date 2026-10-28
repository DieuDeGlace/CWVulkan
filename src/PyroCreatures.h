#pragma once
// PyroProgression v1.4 ("relative" variants, Pyro's mod as installed in the owner's game) on
// the game's own creatures: its numeric creature level (GetCreatureLevel, which the variants
// make RelativeCreatureLevel) and its stat hooks on them (OnCreatureHPCalculated,
// OnCreatureArmorCalculated, OnCreatureResistanceCalculated, OnCreatureAttackPowerCalculated,
// OnCreatureSpellPowerCalculated: ApplyCreatureBuff), through the mod loader's callbacks
// (nativerules::addCallbacks). The rules are the mod's (Progression.h); the region and its
// biome are the game's (0x2ada20, 0x2b9080). Only in [Progression] Mode=pyro.
#include "worldgen/Regions.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <string>

namespace pyrocreatures {

// The mod's levels kept per character and world (its keys mix the world's seed and name and
// the character's name), in Mods/PyroRelativeLevels.state as appended lines:
//  - "R <key> <level>": the level each region's enemies follow. PlayerRelative: the player's,
//    always. DiscoveryLocked: the player's level on first entering it, for good (RelativeTick).
//  - "G <key> <level>": a piece of gear's level, given once (RelativeItemLevel): gear the
//    character owned when the mod came gets its old level (`own`), a drop gets the player's
//    level less 0-2 or, from a stronger enemy, sometimes more (`dropped`, its 0x2a5b70 hook).
//    Gear with the same category, id, modifier and material shares a level.
// CubeVulkan reads the game's file (never writing it) and `ownFile`, and appends its own
// entries to `ownFile` when `keepNew`.
class RelativeLevels {
  public:
    void open(int32_t worldSeed, const std::string &worldName, const std::string &characterName, bool locked,
              const std::filesystem::path &gameFile, const std::filesystem::path &ownFile, bool keepNew = true);
    bool locked() const { return lockedRegions; }
    // The level region `r`'s enemies follow, the player being at `playerLevel`.
    int basis(worldgen::Int2 r, int playerLevel) const;
    // The player stands in region `r` (each tick): locked, a region entered for the first
    // time keeps `playerLevel`.
    void visit(worldgen::Int2 r, int playerLevel);
    size_t size() const; // regions
    // Gear (categories 3-9): its kept level, given the player's (and the enemy's when dropped)
    // the first time.
    int itemLevel(int category, int id, uint32_t modifier, int material, int playerLevel);
    int dropped(int category, int id, uint32_t modifier, int material, int playerLevel, int enemyLevel);
    void own(int category, int id, uint32_t modifier, int material, int level);
    int keptLevel(int category, int id, uint32_t modifier, int material) const; // 0: none yet
    // The level the mod gave gear before keeping levels (`legacy`): by the distance from the
    // player's region to the item's, and its modifier.
    static int legacyLevel(worldgen::Int2 itemRegion, worldgen::Int2 playerRegion, uint32_t modifier);
    // Whether the game's file has region entries (only the locked variant writes them).
    static bool lockedInGame(const std::filesystem::path &gameFile);

  private:
    uint64_t key(worldgen::Int2 r) const;
    uint64_t itemKey(int category, int id, uint32_t modifier, int material) const;
    int give(char kind, std::map<uint64_t, int> &to, uint64_t key, int level); // (lock held)
    mutable std::mutex lock;
    std::map<uint64_t, int> regions, gear;
    uint64_t identity = 0;
    bool lockedRegions = false, keep = false;
    std::filesystem::path ownPath;
};

// A creature's numeric level: a player or a pet keeps its own (+0x144); any other follows
// its region's basis by that region's biome and roll, and its id.
int level(const uint8_t *creature, int32_t worldSeed, const std::function<int(worldgen::Int2 region)> &basis);
int level(const uint8_t *creature, int32_t worldSeed, int playerLevel);

// The mod's level text over a creature or an item: "LV 12 ", "LV 1.50K ", "LV 2.00M ".
std::string levelText(int level);

// While one lives, the creatures' stats follow their levels (`basis` is asked at each call,
// so they follow the player's level as it rises).
class StatHooks {
  public:
    StatHooks(int32_t worldSeed, std::function<int(worldgen::Int2 region)> basis);
    ~StatHooks();
    StatHooks(const StatHooks &) = delete;
    StatHooks &operator=(const StatHooks &) = delete;

  private:
    int handle = 0;
};

} // namespace pyrocreatures
