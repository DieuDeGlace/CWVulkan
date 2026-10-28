#pragma once
// A zone's spots, plants, chests, crates, lore objects and items lying about, and the final
// passes (docs/specs/worldgen-structures-contents.md, sections C1-C9). The creature passes that
// share these steps' rand() stream are part C's; they plug in through StructureHooks
// (Structures.h).
#include "worldgen/BuildingRules.h"
#include "worldgen/ZoneBuild.h"
#include "SoundRules.h"

#include <functional>
#include <set>

namespace worldgen {

// C1: the spot map, after the street lamps (0x3162b0); the working copy is set from it.
void findSpots(ZoneBuild &zone);
// C2: plants (0x3191b0).
void plants(ZoneBuild &zone);
// C2.1: at the end of the plant pass, the two lists rebuilt from the zone's models (+0x18), in
// order, with no draws: the light list (+0x90: type 0x4c or flag bit 0) and the emitter list
// (+0xa8: types 0x4d-0x50 and 0x1f). A model can be in both.
struct ModelLists {
    std::vector<Model> lights;   // +0x90
    std::vector<Model> emitters; // +0xa8
};
ModelLists modelLists(const std::vector<Model> &models);
// C3: the cells' chests, crates and lore objects (0x316780).
void cellThings(ZoneBuild &zone, BuildingRules &rules);
// C4: the item a region role of 6-13 lays down (0x11a820's part B).
void roleItem(ZoneBuild &zone, int role);

// C5: the hooks of part C inside the cell pass; unset ones draw nothing.
struct CellPassHooks {
    // 0x317290: the townspeople of a cell of a kind-5 place; returns whether the cell's own things
    // (campfires) are still made.
    std::function<bool(ZoneBuild &, int32_t cx, int32_t cy)> townsfolk;
    // 0x317c54: the first town's three people.
    std::function<void(ZoneBuild &, int32_t cx, int32_t cy)> firstTownPeople;
    // From 0x3181aa: the wild group test (its rand() % 128 draw first) and group.
    std::function<void(ZoneBuild &, int32_t cx, int32_t cy)> wildGroup;
    // 0x2fb850: the creature of a castle cell.
    std::function<void(ZoneBuild &, int32_t cx, int32_t cy, CastleCell k)> castleCreatures;
    // 0x2fa7b0: the creatures of a kind-8 place's cell.
    std::function<void(ZoneBuild &, int32_t cx, int32_t cy)> dungeonCreatures;
    // 0x3189d6: animals.
    std::function<void(ZoneBuild &, int32_t cx, int32_t cy)> animals;
};
// C5: the cells' creatures and items (0x3171a2).
void cellCreaturesAndItems(ZoneBuild &zone, BuildingRules &rules, const CellPassHooks &hooks);

// C7, C8: the generator's copies, the region re-stamp, the lists of 0x31d470 and the marking of
// the blocks inside static objects (0x31d700).
struct ZoneLists {
    std::vector<StaticObject> objectsCopy; // +0x78
    std::vector<GroundItem> itemsCopy;     // +0x48
    std::vector<StaticObject> lights;      // +0xc0: objects of a light type
    struct Position {
        int64_t x, y, z;
    };
    std::vector<Position> treasures;       // +0x4b8: generated items and chests
    std::set<std::tuple<int32_t, int32_t, int32_t>> marked; // +0xe8: blocks given flag 0x40
};
void copyAndRestamp(ZoneBuild &zone, ZoneLists &lists);
void listLightsAndTreasures(ZoneBuild &zone, ZoneLists &lists);
void markObjectBlocks(ZoneBuild &zone, ZoneLists &lists);

// C9: the campfire and things of the daily refresh's travellers' camp, in cell (cx, cy) with spots
// of `kind`; `race` the travellers' race. Part C's refresh calls it with the zone's stream seeded.
void campObjects(ZoneBuild &zone, int32_t cx, int32_t cy, int kind, int race);

// The zone's ambient sound emitters (zone +0xa8, 0x40 bytes each: rivers 0x1f, leaves 0x4d,
// birds 0x4e..0x50; docs/specs/sound.md §4), which Audio's ambient rules play: the emitter list
// of C2.1 as the generator makes it (Generator.cpp, from the zone cache).
std::vector<soundrules::Emitter> soundEmitters(int32_t zx, int32_t zy);

} // namespace worldgen
