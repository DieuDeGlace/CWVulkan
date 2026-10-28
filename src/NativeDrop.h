#pragma once
// What a dying creature leaves behind, rewards, and the item generators, on game-layout
// records (docs/specs/drops-and-items.md).
//
//   * drop (0x2a6860): a creature that is not a player or a townsperson may drop one of its
//     worn items, its pet, a generated weapon or armour piece, pet food or a set item, a
//     type-15 item, coins, its race's resources and everything it carries.
//   * place (0x2a5b70): lays one item on the ground. It gives gear the region the creature
//     came from, may reserve the item for a player who can use it (raising its rarity) and
//     puts it on the ground of a loaded zone.
//   * reward (0x2a7ca0): gear for every player within 100 blocks, coins and tier resources.
//   * weapon, armour, petFood (0x2d15d0, 0x2d0680, 0x2d0fc0): the item generators.
#include "NativeAI.h"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace nativedrop {

// An item record (0xa0 bytes, game layout).
using Item = std::array<uint8_t, 0xa0>;

// The arguments of one call to the drop layer (0x2a5b70), apart from the dropping creature.
struct Place {
    Item item{};
    int64_t pos[3]{};
    float angle = 0;
    float scale = 1; // passed by every caller but never read by the game
    int32_t count = 1;
    int64_t owner = 0; // the player it is for; 0 lets the drop layer choose
};

// An item on the ground as a zone keeps it (zone +0x30, 0xe0 bytes).
using Drop = std::array<uint8_t, 0xe0>;

// The parts of the world these routines read and write.
struct World {
    std::function<int()> rand;                            // the C runtime's rand(), 0..32767
    std::vector<nativeai::Creature *> creatures;          // world +0x40, in world order
    std::function<nativeai::Creature *(int64_t id)> byId; // world +0x450
    int32_t day = 0;                                      // world +0x08
    std::vector<int32_t> petList;                         // world +0x360
    std::map<int32_t, std::vector<int32_t>> petListsByKey; // world +0x378
    std::function<nativeai::Block(int x, int y, int z)> block; // the block at a block position
    std::function<bool(int zx, int zy)> zone;             // whether a zone (64 blocks) is loaded
    // Told of every call to the drop layer, for the oracle.
    std::function<void(const Place &)> placing;
    // The ground items laid, in order (what the game appends to each zone's list).
    std::vector<Drop> drops;
};

// 0x2a6860: everything `dead` drops.
void drop(World &world, nativeai::Creature *dead);
// 0x2a5b70: lay one item. `from` is the dropping creature, or null.
void place(World &world, const Place &p, nativeai::Creature *from);
// 0x2a7ca0: a reward of tier `tier` (1..4) at `pos`.
void reward(World &world, const int64_t pos[3], int tier);
// The generators write a whole item into `item` (0xa0 bytes). `cls` is a class 1..4, or
// anything else (callers pass -1) for every class. `plain` skips the armour material draws.
void weapon(World &world, uint8_t *item, uint8_t rarity, int cls); // 0x2d15d0
void armour(World &world, uint8_t *item, uint8_t rarity, int cls, bool plain); // 0x2d0680
void petFood(World &world, uint8_t *item, int key);                 // 0x2d0fc0
// The region set (§0.7): types 2..9, 15, 22, 24, 26 and 30, whose items are bound to a region.
bool inRegionSet(uint8_t type);

} // namespace nativedrop
