#pragma once
// Creatures in the world (docs/specs/worldgen-creatures.md §7): the creature record as its
// constructor leaves it (0x4ce80), how the world turns a zone's spawn points into creatures near
// the players (0x2e17d0), how an arena master's stage moves on (in 0x2e3510), and how a creature
// becomes a pet (0x2b67b0). They run in the world update, not in generation.
//
// A creature is the game's record (CREATURE_SIZE bytes, its layout), so the game's own rules,
// physics and pose (NativeRules, NativeUpdate, NativePose) work on it unchanged; the containers
// it keeps outside those bytes come beside it (CreatureExtras).
#include "worldgen/CreatureTables.h"
#include "worldgen/Spawns.h"

#include <array>
#include <cstdint>
#include <functional>
#include <set>
#include <vector>

namespace worldgen {

constexpr size_t CREATURE_SIZE = 0x15f8;
using CreatureRecord = std::vector<uint8_t>;

// Record offsets (§7.1, §7.3) that generation and the engine read.
namespace creature {
constexpr size_t ID = 0x8, POSITION = 0x10, YAW = 0x30, HOSTILITY = 0x60, RACE = 0x64,
                 ABILITY = 0x68, // the current ability; a spawn point's mode goes here
                 APPEARANCE = 0x78, FLAGS = 0x80, SIZE = 0x84, TOGGLES = 0x128, STUN = 0x130, LEVEL = 0x144, CLASS = 0x14c,
                 SPEC = 0x14d, REGION = 0x150, HEALTH = 0x180, BLOCK_POWER = 0x184, GRIP = 0x194, LIGHT_RADIUS = 0x198,
                 STAGE = 0x19c, OWNER = 0x1a0, HOME_PLACE = 0x1b0, HOME = 0x1b8, HOME_CELL = 0x1d0, ITEM_IN_USE = 0x1d8,
                 EQUIPMENT = 0x278, NAME = 0x958, PEOPLE = 0x970, TAG = 0x974, STYLE = 0x978, STAMINA = 0x9a0,
                 WETNESS = 0x9a4, PET_ID = 0x9d8, UNSET_A08 = 0xa08, UNSET_ITEM = 0xa0c, UNSET_AAC = 0xaac, UNSET_AB0 = 0xab0,
                 ROAM = 0xbcc, ABILITY_PICK = 0xbe8, ANIMATION = 0xc38, FIELD_1530 = 0x1530;
// The animation state's size factor (creature-pose.md: A+0x5b8, multiplied into the body's
// scale). Every pose call in the game's recordings (Extract/oracle/anim*.bin) holds 1.0 there,
// so the constructor leaves it at 1; at 0 the creature is drawn with no size at all.
constexpr size_t POSE_SIZE_FACTOR = ANIMATION + 0x5b8;
constexpr size_t ITEM_SIZE = 0xa0;
constexpr size_t STACK_SIZE = 0xa4;
constexpr uint32_t PET_ROAM = 0x80; // the AI flags of a pet
} // namespace creature

// What a creature keeps beside its record: its inventory (+0x9f0), the two ability lists
// (+0xbd0, +0xbf0) and its group (+0xc08).
struct CreatureExtras {
    Inventory inventory;
    std::vector<int32_t> abilities, abilities2;
    std::set<int64_t> group;
};
struct MadeCreature {
    CreatureRecord record;
    CreatureExtras extras;
};

// A creature as the constructor (0x4ce80) leaves it, with its id (§7.3), its appearance as the
// appearance constructor (0x4d1f0) leaves it.
CreatureRecord newCreature(int64_t id);

// What the world answers for the resident pass.
struct ResidentWorld {
    std::function<bool(int64_t id)> alive;                           // a creature with that id lives (world+0x450)
    std::function<bool(int64_t id)> dead;                            // the id is in the dead set (world+0x20)
    std::function<int32_t(int32_t zx, int32_t zy)> deadTracked;      // a zone's count of dead tracked creatures
    std::function<bool(int stage)> arenaAtStage;                     // a registered arena master has this stage, without an HP test
    std::function<bool(int64_t x, int64_t y, int64_t z)> nearPlayer; // optional live-record range resolver
    struct Player {
        int64_t x, y, z; // 1/65536 blocks
    };
    std::vector<Player> players;
};

// Whether a spawn point becomes a creature now (§7.1 step 2). Its +0x928 cells (never filled by
// generation) are not tested: their rule was not traced (open question 3).
bool residentReady(const SpawnPoint &p, int32_t zx, int32_t zy, const ResidentWorld &world);

// The zones the pass looks at (§7.1): every zone within 256 blocks of a player, once each, in
// first-encounter key order. Consumers still filter loaded zones and process them
// in unsigned Zone-pointer order (steam-authoritative-residents.md §2).
std::vector<Int2> residentZones(const std::vector<ResidentWorld::Player> &players);

// A creature made from a spawn point (§7.1 steps 3-5), from the constructor's defaults with the
// point's id. `worldRand` is the world's rand() (spiders that drop from above start 80-129 blocks
// higher); `spared`: the creature's region is one where a player lifted the demon portal;
// `maxHealth` gives 0x5fac0 for the record (NativeRules).
Int2 residentRegion(const SpawnPoint &p); // copied position, truncation toward zero before region lookup
MadeCreature makeResident(const SpawnPoint &p, Draw worldRand, bool spared, const std::function<float(const uint8_t *record)> &maxHealth);

// Makes creature `record` the pet of `owner` (0x2b67b0, §7.4): its race's looks and gear drawn
// from srand(race), the owner and the pet's AI flags. The caller has set its race, hostility,
// level and flags. `ownerRecord` finds the owner's record (null when it is not in the world),
// whose pet id is then set. Returns what the gear gave beside the record.
CreatureExtras makePet(uint8_t *record, int64_t owner, const CreatureTables &tables,
                       const std::function<uint8_t *(int64_t id)> &ownerRecord = {});

// A creature made in play rather than from a spawn point: the constructor with `id`, its race,
// hostility, level, position (1/65536 blocks) and region, then made the pet of `owner` (0 for
// none) as above, at full health.
MadeCreature makeInPlay(int64_t id, int32_t race, int32_t hostility, int32_t level, const int64_t position[3], Int2 region,
                        int64_t owner, const CreatureTables &tables, const std::function<float(const uint8_t *record)> &maxHealth);

// Writes an item as a creature or spawn point keeps it (0xa0 bytes; the rest zero).
void writeItem(uint8_t *at, const Item &item);
// Puts gear on a creature record: its class and specialisation, its worn items and its ability
// pick (the lists and the inventory live outside the record's bytes).
void wearGear(uint8_t *record, const Gear &gear);
// An inventory as the engine's creatures keep it: tabs of 0xa4-byte stacks (+0 quantity, +4 the item).
std::vector<std::vector<std::array<uint8_t, creature::STACK_SIZE>>> inventoryRecords(const Inventory &inventory);

// An arena master's stage byte after the zone's teams (§7.2): `points` the spawn points of a
// loaded zone around its home. Returns the new stage byte (0xff once the arena is finished).
uint8_t arenaStage(uint8_t stage, const std::vector<const SpawnPoint *> &points, const std::function<bool(int64_t)> &dead);
constexpr uint8_t ARENA_MASTER_CLASS = 0xa1;
constexpr uint8_t ARENA_FINISHED = 0xff;

} // namespace worldgen
