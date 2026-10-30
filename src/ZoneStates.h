#pragma once
// What changed in each zone, kept the way the game keeps it (docs/specs/native-saves.md §5.2,
// §5.3): a zone in memory has its ground items (zone +0x30), its static objects (+0x60), its
// block changes (+0xd8), its place counters (+0x4d8) and its count of dead tracked creatures
// (+0x490). When a zone is made, the world's stored state for it is put on top of what the
// generator made (0x2ae0c0); when it leaves memory, its state is recorded into the world's map
// (0x2d3e20), which world.db saves.
//
//   zonestates::ZoneStates states(&saves.world(), false, [&](Cell zone, const auto &changes) {
//       extractedWorld.setBlockChanges(zone[0], zone[1], changes);
//   });
//   auto &lists = states.load(zone, generatedItems, generatedObjects);
//   ... lists.staticObjects[i][OBJECT_STATE] = 0; states.objectsChanged(zone);
//   states.unload(zone, deadCount);
#include "NativeSaves.h"
#include "worldgen/Generator.h"

#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace zonestates {

using nativesaves::BlockChange;
using nativesaves::Cell;
using nativesaves::Record;

// A ground item record (drops-and-items.md §0.4) for an item the generator lays down: the item,
// where it lies, its angle and flags, a count of 1 and no owner. `day` is the world's day.
Record groundItemRecord(const worldgen::GroundItem &g, int32_t day);
// A static object record (use-prompt.md §0.3) for a generator's object: kind, place, quarter
// turns, scale, the item it takes, active, with its swing finished.
Record staticObjectRecord(const worldgen::ZoneObject &o);
// Whether a ground item slot holds an item (an empty slot's item type is 0).
bool holdsItem(const Record &groundItem);
// Where a ground item lies (1/65536 blocks).
std::array<int64_t, 3> groundItemPosition(const Record &groundItem);
// The zone of a block, of 64 blocks a side.
Cell zoneOfBlock(int64_t x, int64_t y);
// A block change entry (28 bytes, §5.3): the block, as it was and as it is now.
BlockChange blockChange(int32_t x, int32_t y, int32_t z, const std::array<uint8_t, 6> &was, const std::array<uint8_t, 6> &now);

class ZoneStates {
  public:
    using BlocksChanged = std::function<void(Cell zone, const std::vector<BlockChange> &changes)>;
    // `world` holds the stored states (null: nothing is applied or recorded). In server mode
    // nothing is applied (0x2ae0c0). `blocks` is told a zone's block changes whenever they are
    // set: after the zone is made and after each change in play.
    ZoneStates(nativesaves::WorldState *world = nullptr, bool serverMode = false, BlocksChanged blocks = {});

    bool loaded(Cell zone) const { return zones.count(zone) != 0; }
    nativesaves::ZoneLists *find(Cell zone);
    const nativesaves::ZoneLists *find(Cell zone) const;
    // The zones in memory.
    std::vector<Cell> zonesInMemory() const;
    // Early world-step pass (worldgen-parity-gaps.md §4.4). Only existing zones
    // within three zones of a player advance; overlapping rectangles count once.
    // A non-null usedCells means a local player exists, including an empty set.
    void advanceObjects(float milliseconds, const std::vector<Cell> &playerZones,
                        const std::set<Cell> *usedCells = nullptr);

    // A zone is made with what the generator gave it; the stored state goes on top (0x2ae0c0):
    // its saved ground items and static objects replace the generated ones, its block changes
    // are put back in order, and its dead count and place counters are set.
    nativesaves::ZoneLists &load(Cell zone, std::vector<Record> groundItems, std::vector<Record> staticObjects);
    // A zone leaves memory: its state is recorded (0x2d3e20) and its lists let go. `deadTracked`
    // is the zone's dead tracked creatures when the creature side knows it (otherwise the count
    // it was made with is kept).
    void unload(Cell zone, std::optional<int32_t> deadTracked = std::nullopt);
    // Every zone in memory is recorded, and let go (the world's shutdown).
    void unloadAll();

    // What play changed.
    void objectsChanged(Cell zone);     // zone +0xfa
    void groundItemsChanged(Cell zone); // zone +0xf9
    // A block changed: appended to its zone's list (the zone is made empty if not in memory yet).
    void addBlockChange(const BlockChange &change);

  private:
    nativesaves::WorldState *world;
    bool serverMode;
    BlocksChanged blocks;
    std::map<Cell, nativesaves::ZoneLists> zones;
};

} // namespace zonestates
