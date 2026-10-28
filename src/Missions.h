#pragma once
// The missions' progress (the mission update, 0x131620, which the world's mission update 0x2e1750
// runs with the planner of the player's region): what makes a place's mission finished
// (the character's data +0xd8) or also solved (+0xc8), and what the completed ones give the player.
// The places are 1024-block cells, a big place's rounded down to even.
#include "worldgen/Generator.h"

#include <array>
#include <cstdint>
#include <functional>
#include <set>
#include <vector>

namespace missions {

using Cell = std::array<int32_t, 2>;

struct World {
    const uint8_t *player = nullptr;      // the local player's record (world+0x448)
    std::set<Cell> *finished = nullptr;   // the character's data +0xd8
    std::set<Cell> *solved = nullptr;     // the character's data +0xc8
    // Every creature's record (world+0x40).
    std::function<void(const std::function<void(const uint8_t *)> &)> creatures;
    // A loaded zone's objects (zone+0x60), and whether one is still active (its +0x28, 1 at
    // first): a chest no longer is once opened.
    std::function<std::vector<worldgen::ZoneObject>(int zx, int zy)> objects;
    std::function<bool(const worldgen::ZoneObject &)> active;
    // A near zone's mission creatures (zone+0x498), and whether a creature is dead for good
    // (world+0x20).
    std::function<std::vector<int64_t>(int zx, int zy)> missionCreatures;
    std::function<bool(int64_t id)> dead;
    // Whether the player carries special item `id` of its region (0x5e890; 4 always), and
    // an item (0x5e780: an artifact).
    std::function<bool(int id)> hasSpecial;
    std::function<bool(const worldgen::Item &)> carries;
};

// What the completed missions give (set on the player by the caller each update):
// witch curses lift petrification (+0x128 bit 0x8000), demon portals 0x800, all magic
// barriers 0x4000; flames of power a buff 0x21 of that strength, the mana pumps left a buff
// 0x22 (each 60 s, renewed below 1 s left or when the count changes, ended at none).
struct Effects {
    bool witch = false, demon = false, barriers = false;
    int flames = 0, pumpsLeft = 0;
};

// One update; false (nothing done) while the player flies on the eagle.
bool update(World &world, Effects &effects);

// Whether the player has completed the place's mission (0x5e430: finished or solved).
bool completed(const std::set<Cell> &finished, const std::set<Cell> &solved, Cell place);

} // namespace missions
