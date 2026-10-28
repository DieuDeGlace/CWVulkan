#pragma once
// Character progression, switchable ([Progression] Mode= in CubeVulkan.ini or Options):
//
//  vanilla  Steam Cube World as released: no XP or levels. Gear strength comes
//           from rarity (and the region lock); artifacts raise travel skills.
//  pyro     The rules of the PyroProgression v1.4 mod ("relative" variant, as
//           installed in the owner's game): XP from kills, levels, enemy levels
//           that follow the player's by biome, and levelled gear. The numbers
//           below are that mod's formulas, re-implemented here.
//
// Everything is a pure function of its inputs, so the engine and tests agree.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace progression {

enum class Mode { Vanilla, Pyro };
inline const char *modeName(Mode m) { return m == Mode::Pyro ? "pyro" : "vanilla"; }

// --- XP and levels (pyro) ------------------------------------------------------
// XP needed to go from `level` to the next (PyroProgression replaces the game's
// own function at RVA 0x5fa80 with this; the HUD shows 0/54 at level 1).
inline int xpToNext(int level) { return 50 + 4 * std::max(1, level); }

inline uint64_t mix(uint64_t v) {
    v ^= v >> 30;
    v *= 0xbf58476d1ce4e5b9ULL;
    v ^= v >> 27;
    v *= 0x94d049bb133111ebULL;
    return v ^ (v >> 31);
}
// A region's fixed roll, so its enemy levels stay the same across restarts.
inline uint64_t regionRoll(int seed, int x, int y) {
    return mix(uint32_t(seed) ^ mix(uint32_t(x)) ^ mix(uint64_t(uint32_t(y)) + 0x9e3779b97f4a7c15ULL));
}
// Biome difficulty (the game's biome ids): 0 easy, 1 moderate, 2 hard.
inline int tier(int biome) {
    switch (biome) {
    case 4: case 9: case 12: case 13: return 2; // jungle, desert, deadlands, lava
    case 5: case 6: case 10: case 11: return 1; // dark woods, snow forest, savannah, snowlands
    default: return 0;
    }
}
// An enemy's numeric level from the player's: 90-98% in easy biomes, 108-116% in
// moderate ones, 125-133% in hard ones, give or take one level per creature.
inline int enemyLevel(int player, int biome, uint64_t region, uint64_t entity) {
    player = std::clamp(player, 1, 1000000);
    int t = tier(biome);
    int percent = (t == 0 ? 90 : t == 1 ? 108 : 125) + int(region % 9);
    int result = (player * percent + 50) / 100 + int(mix(entity) % 3) - 1;
    return std::max(t ? player + t : 1, result);
}
// XP for a kill. stars: the creature's star rating (the game's entity level field).
inline int killXp(int stars, int enemy, int player, bool boss = false, bool named = false, bool mini = false) {
    player = std::max(1, player);
    float xp = float(stars + 1);
    if (player > enemy)
        xp *= std::pow(float(enemy + 1) / float(player), 3.f);
    else {
        int difference = std::clamp(enemy - player, 0, 20);
        xp *= (1 + difference / 6.f) * std::pow(1.05f, (float)difference);
    }
    if (boss) xp *= 10;
    if (named) xp *= 5;
    if (mini) xp *= 2;
    return int(xp);
}
// What each level adds to the player's own stats.
struct LevelBonus {
    float hp = 0, armor = 0, attack = 0, critical = 0;
};
inline LevelBonus levelBonus(int levels) {
    return {1.f * levels, .2f * levels, .2f * levels, .0001f * levels};
}
// Enemy stats grow with their level by this factor.
inline float creatureScale(int level) { return 1 + 1000 * std::log2((level + 1001.f) / 1000.f); }

// --- Gear ----------------------------------------------------------------------
inline uint32_t pyroRand(uint64_t seed) { return uint32_t((seed * 1103515245ULL + 12345ULL) / 65536ULL % 32768ULL); }
// The game's modifier scramble, and the small rarity shift it gives an item.
inline int32_t modifierHash(uint32_t modifier) {
    int32_t m1 = int32_t(modifier ^ (modifier << 13));
    int32_t m2 = m1 ^ (m1 >> 17);
    return int32_t(uint32_t(m2) ^ (uint32_t(m2) << 5));
}
inline float modifierShift(uint32_t modifier) { return float(modifierHash(modifier) / 0x10624DD3) / 8.f; }
// A dropped item's level: the player's, minus 0-2; above-level enemies sometimes
// drop items above the player's level.
inline int lootLevel(int player, int enemy, uint64_t roll) {
    player = std::clamp(player, 1, 1000000);
    int ordinary = std::max(1, player - int(roll % 3));
    if (enemy > player && mix(roll) % 3 == 0)
        return player + 1 + int(mix(roll + 1) % uint64_t(enemy - player));
    return ordinary;
}
// The level of gear the character already owned (its region and modifier).
inline int ownedItemLevel(int regionDistance, uint32_t modifier) { return 1 + 5 * regionDistance + int(pyroRand(modifier) % 5); }
// How strong an item's main stats are (health, armor, attack): by level and rarity
// with pyro, by rarity alone in vanilla. Only ratios between items are used.
inline float gearStrength(Mode mode, int level, int rarity, uint32_t modifier) {
    float r = rarity + modifierShift(modifier);
    if (mode == Mode::Pyro)
        return 1000 * std::log2((std::max(1, level) + 3 * r + 1001.f) / 1000.f);
    return 3 * (r + 1);
}

} // namespace progression
