#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace encounters {
// Steam's native owner function uses 8192-block cells with warped boundaries.
// The native owner check remains authoritative at a candidate's actual site.
constexpr int RegionBlocks = 8192;
// Landmark kinds are anchored at a native location instead of a random site.
// They follow Ambush so that stored kind numbers of older records stay valid.
enum class Kind : unsigned {
    Battle,
    Rescue,
    Bounty,
    ChestSack,
    ChestBoss,
    Ambush,
    Hill,   // Hill, Canyon, Crater, Rocks: a named boss and its pack.
    Camp,   // Camp, Bandit Camp: a leader and a larger group.
    Shrine, // Altar, Sanctuary, Menhir, Circle of Power, Graveyard: a guardian.
    Count
};
constexpr unsigned NaturalKindCount = unsigned(Kind::Ambush);
inline bool groupEvent(Kind kind) {
    return kind == Kind::Battle || kind == Kind::Ambush;
}
inline bool guardianEvent(Kind kind) {
    return kind == Kind::Rescue || kind == Kind::Bounty || kind == Kind::ChestBoss;
}
inline bool landmarkEvent(Kind kind) {
    return kind == Kind::Hill || kind == Kind::Camp || kind == Kind::Shrine;
}
// The named boss of an encounter: a guardian, or a landmark group's leader
// (always enemy slot 0).
inline bool leader(Kind kind, unsigned enemy) {
    return guardianEvent(kind) || (landmarkEvent(kind) && enemy == 0);
}
// Native location types (dictionary names verified in the executable) that
// host a landmark encounter when the native planner put no utility-item quest
// there. Settlements, dungeons, lakes (CubeMegaMod's lake encounters) and mana
// pumps (their own native Iron Legion population) are never landmarks; every
// other natural location is.
inline bool landmarkKind(int locationType, Kind &kind) {
    switch (locationType) {
    case 10: // Mountain
    case 18: // Crater
    case 19: // Volcano
    case 20: // Tree (mana tree)
    case 21: // Rock
    case 23: // Island
    case 24: // Arena
    case 25: // Forest
    case 26: // DarkForest
    case 27: // EnchantedForest
    case 28: // Canyon
    case 29: // Rocks
    case 30: // Plain
    case 31: // Hill
    case 32: // Grove
    case 33: // Glade
    case 36: // Beach
        kind = Kind::Hill;
        return true;
    case 22: // OldHut
    case 34: // Camp
    case 35: // BanditCamp
        kind = Kind::Camp;
        return true;
    case 9:  // Graveyard
    case 16: // CircleOfPower
    case 37: // Altar
    case 38: // Sanctuary
    case 39: // Menhir
        kind = Kind::Shrine;
        return true;
    default:
        return false;
    }
}
// Group size including the leader.
inline unsigned landmarkEnemies(Kind kind, uint64_t roll) {
    switch (kind) {
    case Kind::Hill:
        return 4 + unsigned((roll >> 16) % 3); // Boss + 3-5.
    case Kind::Camp:
        return 5 + unsigned((roll >> 16) % 2); // Leader + 4-5.
    case Kind::Shrine:
        return 3 + unsigned((roll >> 16) % 2); // Guardian + 2-3.
    default:
        return 0;
    }
}
// Landmark records use these per-region slots; random encounters use the
// lower ones (EventsPerRegion is at most 8).
constexpr unsigned FirstLandmarkSlot = 8, LandmarkSlots = 24;
struct Region {
    int x = 0, y = 0;
};
struct Site {
    int64_t x = 0, y = 0;
};
inline uint64_t mix(uint64_t n) {
    n = (n ^ (n >> 30)) * 0xbf58476d1ce4e5b9ull;
    n = (n ^ (n >> 27)) * 0x94d049bb133111ebull;
    return n ^ (n >> 31);
}
inline uint64_t identity(uint64_t world, Region region, unsigned slot) {
    return mix(world ^ mix(uint32_t(region.x)) ^ (mix(uint32_t(region.y)) << 1) ^
               (uint64_t(slot) + 1) * 0x9e3779b97f4a7c15ull);
}
inline int64_t floorDivide(int64_t value, int64_t divisor) {
    return value / divisor - (value < 0 && value % divisor != 0);
}
inline bool within(Site a, Site b, int radius) {
    long double dx = (long double)a.x - b.x, dy = (long double)a.y - b.y;
    return dx * dx + dy * dy <= (long double)radius * radius;
}
struct Plan {
    Site site;
    Kind kind;
    unsigned enemies;
    uint64_t roll;
};
// A cycle is persisted. Chunk reloads within one session preserve the instance;
// completion or a new session advances it so future encounters move around.
inline Plan plan(uint64_t world, Region region, unsigned slot, unsigned cycle,
                 unsigned attempt = 0) {
    auto roll = mix(identity(world, region, slot) ^ mix(uint64_t(cycle) + 1));
    auto position = mix(roll ^ mix(uint64_t(attempt) + 0x4f564552ull));
    auto kind = static_cast<Kind>(roll % NaturalKindCount);
    constexpr unsigned span = RegionBlocks - 128;
    return {{int64_t(region.x) * RegionBlocks + 64 + int64_t(position % span),
             int64_t(region.y) * RegionBlocks + 64 + int64_t((position >> 32) % span)},
            kind,
            kind == Kind::Battle      ? 4u + unsigned((roll >> 16) % 3)
            : kind == Kind::ChestSack ? 0u
                                      : 1u,
            roll};
}
inline bool relocating(Site next, Site previous, unsigned cycle) {
    return cycle == 0 || !within(next, previous, 128);
}
inline unsigned allEnemies(unsigned count) {
    return count >= 1 && count <= 6 ? (1u << count) - 1u : 0;
}
inline bool objectiveComplete(unsigned count, unsigned defeatedMask) {
    auto all = allEnemies(count);
    return all && (defeatedMask & all) == all;
}
inline bool chestEvent(Kind kind) {
    return kind == Kind::ChestSack || kind == Kind::ChestBoss;
}
inline unsigned enemyCount(Kind kind, uint64_t roll) {
    return groupEvent(kind) ? 4u + unsigned((roll >> 16) % 3) : kind == Kind::ChestSack ? 0u : 1u;
}
// Fixed, bounded candidates for a user-requested test. Native terrain/POI
// validation still decides whether any candidate can become an encounter.
inline Site testSite(Site player, unsigned attempt) {
    constexpr Site directions[] = {{1, 0},  {1, 1},   {0, 1},  {-1, 1},
                                   {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
    auto offset = directions[attempt % 8];
    auto distance = 64 + 16 * (attempt / 8);
    return {player.x + offset.x * distance, player.y + offset.y * distance};
}
inline uint64_t cooldownDeadline(uint64_t now, unsigned minutes) {
    auto seconds = uint64_t(std::clamp(minutes, 30u, 1440u)) * 60;
    return now > std::numeric_limits<uint64_t>::max() - seconds
               ? std::numeric_limits<uint64_t>::max()
               : now + seconds;
}
inline bool canRestart(uint64_t now, uint64_t deadline, uint64_t emptySince) {
    return deadline && now >= deadline && emptySince && now >= emptySince && now - emptySince >= 60;
}
// Native stars remain independent of numeric PyroProgression levels.
inline int stars(int nativeStars, bool boss) {
    return std::clamp(nativeStars + (boss ? 1 : 0), 0, 4);
}
} // namespace encounters
