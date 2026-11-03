// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
#pragma once
// A generated zone: 256 x 256 block columns (docs/specs/alpha-worldgen-terrain.md §1), and the
// things the generator places on it (plants, tree spots, ground items).
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace alphagen {

// Terrain §1.3. Bytes R, G, B, then the type byte: type in bits 0-4 and flags above.
struct Block {
    uint8_t r = 0, g = 0, b = 0, typeByte = 0;
    uint8_t type() const { return typeByte & 0x1f; }
};

// Block types (terrain §1.3; names from their use).
enum BlockType : uint8_t {
    AIR = 0,
    SOLID = 1, // the implicit fill under a column
    WATER = 2,
    LAKE_BED = 3,
    GRASS = 4,
    FIELD = 5, // a garden's ploughed ground (structures spec §5.5.3)
    ROCK = 6,
    WOOD_TYPE = 7,   // tree trunks and branches (trees spec §2.1)
    LEAVES_TYPE = 8, // tree leaves
    SAND = 9,
    SNOW = 10,
    TOWN_GROUND = 11,
    FIRE_GROUND = 12,
    GLOWING = 13,
};
constexpr uint8_t SURFACE_COLOURED = 0x20; // coloured by the surface-colour routine
constexpr uint8_t DRY_AIR = 0x40;          // stays air even below sea level
constexpr uint8_t KEEP = 0x80;

// An empty or water cell lets light through (rendering §9.1, §9.2).
inline bool clear(uint8_t type) {
    return type == AIR || type == WATER;
}

// Terrain §1.2: the climate of a column and its stored blocks, from `bottom` upwards.
struct Column {
    float moisture = 0, warmth = 0, fire = 0;
    int32_t bottom = 0;
    int32_t belowSurface = 0; // surface z - 8 (+0x14; its use is not traced)
    std::vector<Block> blocks;
    int32_t top() const { return bottom + (int32_t)blocks.size(); } // first z above the stored blocks
};

// Vegetation spec §1.3: a plant sprite.
struct Plant {
    int32_t x = 0, y = 0, z = 0; // stands at the corner (x, y) on top of the block below z
    float scale = 0;
    float rotation = 0; // degrees about +Z
    int32_t kind = 0;   // plant kind table A (vegetation §1.1)
    uint32_t flags = 0; // 4: sways (vegetation-wind.md §3)
};

// Vegetation spec §5: where the tree grid decided a tree stands. The tree builder is not
// specified yet, so these are kept but not built.
struct TreeSpot {
    int32_t x = 0, y = 0, z = 0;
    int32_t size = 0, height = 0, kind = 0;
};

// Vegetation spec §4: a collectable lying on the ground.
struct GroundItem {
    int32_t x = 0, y = 0, z = 0;
    uint8_t category = 0, subtype = 0;
    float rotation = 0, scale = 1;
};

// The fields of an item record the generator writes (behaviour spec §10.1).
struct ItemRecord {
    uint8_t type = 0, subtype = 0; // +0, +1 (type 0: an empty slot)
    uint32_t seed = 0;             // +0x04
    uint32_t product = 0;          // +0x08: what a recipe (type 2) makes
    uint8_t rarity = 0;            // +0x0c
    uint8_t material = 0;          // +0x0d
    uint8_t byte0e = 0;            // +0x0e (1 on shop stock; meaning open)
    int16_t level = 0;             // +0x10 (the count, for coins)
};

// Vegetation spec §1.3: a static object (a campfire, a tent, a street light, a runestone...).
// Positions are in 1/65536 blocks; the object stands centred on (x, y) with its foot at z and
// fills `scale` blocks, turned by quarter turns.
struct StaticObject {
    int32_t kind = 0; // static kind (table B), e.g. 65 campfire
    int64_t x = 0, y = 0, z = 0;
    int32_t rotation = 0; // quarter turns (the prefab placer stores some as -r)
    float scale[3] = {1, 1, 1};
    uint8_t state = 0;                // +0x30: 1 for objects made from a prefab blob, 2 for a filled chest
    int32_t timer = 0;                // +0x34: a spike trap's phase, ms
    std::vector<ItemRecord> contents; // +0x48: a chest's items
};

// Vegetation spec §1.3, structures spec §1.10.2 item 5: a sprite placed by a structure (a
// torch, a wall skull, a liana, a cobweb, a sign...). The original keeps these in the plant
// list; they are apart here because they sit at fractional positions.
struct Sprite {
    int32_t kind = 0;            // plant kind table A
    int64_t x = 0, y = 0, z = 0; // 1/65536 blocks
    float size = 0.0625f;        // +0x20: blocks per model voxel
    float angle = 0;             // +0x24: degrees about +Z
    int32_t flags = 2;           // +0x38: 4 sways like a plant; 1 is set on lit sprites with a `tint`
                                 // (what 1 and 2 do is open)
    float tint[3] = {0, 0, 0};
};

// Structures spec §1.10.6: an item lying in the world (table loot).
struct ItemDrop {
    int64_t x = 0, y = 0, z = 0; // 1/65536 blocks
    float rotation = 0;          // degrees
    float scale = 0.06f;         // +0x134
    ItemRecord item;
};

// Structures spec §1.10.6: a marker the zone keeps (type 5 a dungeon exit, type 6 a boss).
struct ZoneMarker {
    int32_t type = 0;
    int64_t x = 0, y = 0, z = 0;
    int32_t kind = 0;      // the boss's creature type
    int32_t creature = -1; // index into Zone::spawns
};

// Structures spec §5.5.9: one stop of a townsperson's day, in 1/65536 blocks and ms of the day.
struct ScheduleStop {
    int64_t x = 0, y = 0, z = 0;
    int32_t timeMs = 0;
};

// Behaviour spec §2.1, §9.1: a spawn object, which the world step turns into a creature near
// the player. Positions in 1/65536 blocks.
struct Spawn {
    float radius = 200; // activation radius, blocks
    int64_t x = 0, y = 0, z = 0;
    int32_t controller = 1; // controller kind of the creature (6: an inert object such as a deposit)
    int32_t type = 4;       // creature type
    int32_t level = 1;      // (unset by the spawn defaults; 1 here)
    uint8_t powerTier = 0;
    int32_t windowStart = 0, windowEnd = 0;   // time-of-day window in ms; equal: always
    float multipliers[5] = {100, 1, 1, 1, 1}; // stat multipliers
    uint8_t characterClass = 0;               // +0x30
    float yaw = 0;                            // +0x54, degrees
    uint16_t appearanceFlags = 0;             // +0x7a: 0x1000 takes the map cell's level, 0x200 boss, 0x40 keeps its gear
    bool bossLoot = false;                    // +0x10e8
    ItemRecord equipment[13];                 // +0x120
    std::vector<ItemRecord> inventory;        // +0xf6c
    int32_t field42 = 0;                      // +0x5c, copied to record field 42 (4: visits `field41`, 3: …; open)
    int32_t field41[3] = {};                  // +0x60, copied to record field 41 (a place, blocks)
    std::vector<ScheduleStop> schedule;       // +0x10a0
    // Wildlife §4.9: the behaviour sequence (0 Combat, LookAtPlayer, RandomWalk; 1 a cell
    // monster's Combat, RandomWalk; 2 a group leader's Combat, LookAtPlayer, WalkPath,
    // RandomWalk; 3 a member's Combat, LookAtPlayer, Companion of `leader`, WalkPath, RandomWalk;
    // location creatures: 5 a ring creature's Combat, WalkPath, RandomInteraction, RandomWalk;
    // 6 a group leader's Combat, WalkPath, RandomWalk),
    // the route walked (blocks) and the leader's index in the same spawn list.
    uint8_t behaviour = 0;
    std::vector<std::array<int32_t, 3>> route;
    int32_t leader = -1;
    // AlphaVulkan's quests (docs/design/quests.md): the quest a creature belongs to (0: none)
    // and what it is to it (quests::Role).
    uint32_t quest = 0;
    uint8_t questRole = 0;
    // What the zone generator's finishing pass (spawn objects spec §3.2) made of it: the
    // appearance record (172 bytes, +0x74; its flags are `appearanceFlags`), the
    // specialization (+0x31), the special actions (+0x10ac) and a boss's +0x10b8 choice.
    // Until then `finished` is false and the creature is made up when it comes alive.
    bool finished = false;
    std::array<uint8_t, 172> appearance{};
    uint8_t specialization = 0;
    std::vector<int32_t> specials;
    int32_t bossChoice = 0;
};

// Trees §5: a kind-60 record, a point in an autumn or blossom crown that leaves fall from
// (particles §7.1). In 1/65536 blocks; the leaf colour as 0..1 per channel.
struct LeafSource {
    int64_t x = 0, y = 0, z = 0;
    float r = 0, g = 0, b = 0;
};

// Structures §5.5.4 step 16: a furnished house, for its residents' seats (trails spec §4.4):
// its origin (blocks), its storey cells (a, b, level; each covers the blocks origin + (13a ...
// 13a + 13, 13b ... 13b + 13, 7 level ... 7 level + 6)), and its stools and benches (seats) and
// beds, as indices into the zone's statics.
struct House {
    int32_t ox = 0, oy = 0, oz = 0;
    std::vector<std::array<int32_t, 3>> storeys;
    std::vector<int32_t> seats, beds;
};

// A position later passes keep their distance from (caves spec §3.1), in 1/65536 blocks.
struct ObjectMark {
    int64_t x = 0, y = 0, z = 0;
};

struct Zone {
    static constexpr int32_t SIZE = 256;
    int32_t zx = 0, zy = 0;
    std::vector<Column> columns = std::vector<Column>(SIZE * SIZE);
    std::vector<Plant> plants;
    std::vector<TreeSpot> trees;
    std::vector<GroundItem> items;
    std::vector<StaticObject> statics;
    std::vector<ObjectMark> objectList;
    std::vector<Spawn> spawns;
    std::vector<Sprite> sprites;
    std::vector<ItemDrop> drops;
    std::vector<ZoneMarker> markers;
    std::vector<LeafSource> leafSources;
    std::vector<std::array<int64_t, 3>> songPosts; // trees §4.9: kind-61 records (1/65536 blocks)
    std::vector<House> houses;
    // Structures spec §5.2: a town zone's plots (g x g, index i + g j) with their roles.
    struct TownPlot {
        int32_t ox = 0, oy = 0, size = 0, kind = 0, sub = 0, face = 0;
    };
    std::vector<TownPlot> townPlots;
    // Rendering §9.2: the light of empty and water cells under the sky line, by cell. Cells
    // not listed are lit fully (255) if clear, or are solid.
    std::vector<std::pair<uint64_t, uint8_t>> shadedLight; // key: see lightKey

    int32_t originX() const { return zx * SIZE; }
    int32_t originY() const { return zy * SIZE; }
    bool contains(int32_t x, int32_t y) const { return x >= originX() && y >= originY() && x < originX() + SIZE && y < originY() + SIZE; }
    Column &column(int32_t x, int32_t y) { return columns[(x - originX()) + SIZE * (y - originY())]; }
    const Column &column(int32_t x, int32_t y) const { return columns[(x - originX()) + SIZE * (y - originY())]; }

    // Terrain §1.4: the block at a cell of this zone, stored or implicit.
    Block read(int32_t x, int32_t y, int32_t z) const;
    // Terrain §1.5: writes a block. Below the stored range, the cells between it and the old
    // bottom are filled by `gapFill(z)` (the terrain passes rock-coloured blocks).
    void place(int32_t x, int32_t y, int32_t z, Block b, const std::function<Block(int32_t)> &gapFill = {});
    // The propagated light of a clear cell (rendering §9.2).
    uint8_t light(int32_t x, int32_t y, int32_t z) const;
};

// The block of an implicit cell: under a column (z < bottom) and outside the world.
constexpr Block IMPLICIT_SOLID{200, 200, 200, SOLID};

uint64_t lightKey(int32_t x, int32_t y, int32_t z);

} // namespace alphagen
