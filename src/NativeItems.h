#pragma once
// Which model the game draws for an item (0x173b10, docs/specs/item-models.md).
//
// The game decides it by a fixed tree on five fields of the item: its category, id,
// material, rarity and modifier. Which model index each leaf names is the game's data, so
// it is not written here: tools/ZoneExtractor asks the game for every combination that
// matters and writes the answers to Extract/oracle/item-models.bin (itemmodels.request).
// This module holds the rules that say which combinations are the same (rarity from 4 on,
// the modifier only through its remainders, the fields a category never reads), reads that
// table, and looks items up in it.
//
// Category 0x17 is the exception: the game makes those models on demand from the id and
// the modifier (spec §3). onDemandModel() gives what the spec describes of them.
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nativeitems {

// The item record (0xa0 bytes, drops-and-items.md §0.1): the fields the model depends on.
constexpr size_t ITEM_CATEGORY = 0x00;   // u8
constexpr size_t ITEM_ID = 0x04;         // i32, compared unsigned where ranges are tested
constexpr size_t ITEM_MODIFIER = 0x08;   // u32
constexpr size_t ITEM_RARITY = 0x14;     // u8
constexpr size_t ITEM_STANDS_FOR = 0x18; // i32: for category 0x16 id 0x3f, the weapon's kind
constexpr size_t ITEM_MATERIAL = 0x1c;   // u8

// Categories the rules single out.
constexpr uint8_t CATEGORY_WEAPON = 0x03;
constexpr uint8_t CATEGORY_CHEST = 0x04;
constexpr uint8_t CATEGORY_NECK = 0x08;
constexpr uint8_t CATEGORY_RING = 0x09;
constexpr uint8_t CATEGORY_RARITY_SHIFTED = 0x0f; // its rarity term is min(max(rarity - 1, 0), 3)
constexpr uint8_t CATEGORY_STAND_IN = 0x16;       // ids 0x3f..0x42 stand for gear pieces
constexpr uint8_t CATEGORY_ON_DEMAND = 0x17;      // models made on demand (spec §3)
constexpr uint8_t LAST_CATEGORY_WITH_MODELS = 0x21;

// Every modifier remainder the tree takes (2, 3, 4, 5, 6, 10, 11) repeats with this period.
constexpr uint32_t MODIFIER_PERIOD = 660;
// Rarities from this one on look the same (except category 1 id 1, which adds the raw byte).
constexpr int HIGHEST_DISTINCT_RARITY = 4;

// The fields that decide an item's model, after the stand-in items are normalised (§1).
struct ModelFields {
    uint8_t category = 0;
    uint32_t id = 0;
    uint32_t modifier = 0;
    uint8_t rarity = 0;
    uint8_t material = 0;
};

// Reads the fields from an item record and normalises category 0x16's stand-ins (0x1092b0):
// id 0x3f becomes a weapon of the kind at +0x18, ids 0x40, 0x41 and 0x42 a chest, neck and
// ring piece of id 0. Material, rarity and modifier are kept.
ModelFields fieldsOf(const uint8_t *item);
ModelFields normalised(ModelFields fields, int32_t standsFor);

// What a category's models depend on (spec §2.4). Fields a category never reads can be
// left out of its lookup.
struct CategoryRule {
    bool never = false;          // no item of the category has a model
    bool onDemand = false;       // category 0x17: made on demand, not in the model list
    bool readsId = false;
    bool readsMaterial = false;
    bool readsRarity = false;
    uint32_t modifierPeriod = 1; // the least common multiple of the remainders it takes (1: none)
};
const CategoryRule &categoryRule(uint8_t category);

// True when the rules alone say the item has no model (category 0, 0x1e and above 0x21,
// weapons 0x16 and 0x17, chest piece id 1).
bool neverHasModel(const ModelFields &fields);

// The rarity term the tree adds (§2.2): min(rarity, 4); category 0x0f min(max(rarity-1, 0), 3);
// category 1 id 1 the raw byte.
int rarityTerm(const ModelFields &fields);
// Whether the rarity is used unclamped (category 1 id 1).
bool usesRawRarity(const ModelFields &fields);
// The modifier as the tree sees it (§2.3): its remainder by 660, every remainder the tree
// takes being a divisor of that period.
uint32_t modifierTerm(uint32_t modifier);
// The same, reduced to the category's own period (always a divisor of 660).
uint32_t modifierTerm(const ModelFields &fields);

// The extracted table: for each (category, id, material) a grid of model numbers by
// rarity and modifier. Model numbers index the game's model list (Extract/sprites.txt).
class ModelTable {
public:
    // Reads the CWI3 format, or the older CWI2 (ids 0..127, materials 0..63, the modifier
    // modulo 330), from a file or from memory.
    bool load(const std::filesystem::path &file, std::string *why = nullptr);
    bool parse(const std::vector<uint8_t> &bytes, std::string *why = nullptr);
    bool empty() const { return keys.empty(); }

    // The model number for the item's fields (normalised already); -1 for none or unknown.
    int modelNumber(const ModelFields &fields) const;

    // For building tables by hand (the tests): one grid of rarities x modifiers model
    // numbers, rarity-major, under a key.
    void setPeriodAndIdBound(uint32_t modifierPeriod, uint32_t idBound, bool idBoundCoversAbove);
    void add(uint8_t category, uint32_t id, uint8_t material, uint32_t rarities, uint32_t modifiers, std::vector<int32_t> models);

private:
    struct Grid {
        uint32_t rarities = 1;  // rows: rarity 0 .. rarities - 1, higher rarities as the last
        uint32_t modifiers = 1; // columns: 1, or the table's modifier period
        std::vector<int32_t> models;
    };
    static uint64_t keyOf(uint8_t category, uint32_t id, uint8_t material);
    int fromGrid(const Grid &grid, const ModelFields &fields) const;

    std::vector<Grid> grids;
    std::unordered_map<uint64_t, uint32_t> keys; // (category, id, material) -> grid
    uint32_t modifierPeriod = MODIFIER_PERIOD;   // CWI2: 330
    uint32_t idBound = 0;                        // keys cover ids below it (and, see below, at it)
    bool idBoundCoversAbove = false;             // CWI3: the key at idBound answers for every id above
};

// Initial parameters for a category 0x17 procedural item (0x1737b0, 0x2b880).
// ProceduralItems owns the resulting voxel resource and its independent shape generators.
struct OnDemandModel {
    int32_t id = 0;
    uint32_t modifier = 0;
    int size = 24;                              // voxels along each axis
    std::array<std::array<float, 3>, 4> colours{}; // four colours, channels 10..259
    uint32_t counter = 0;                       // the draw counter after the colours
    int shape = 0;                              // the id's shape generator: 0..6, or 7 for any other id
};
// The model for (id, modifier): made the first time, then kept (the store's cache never
// drops one). The pointer stays valid for the program's life.
const OnDemandModel &onDemandModel(int32_t id, uint32_t modifier);
// Made afresh, without the cache.
OnDemandModel makeOnDemandModel(int32_t id, uint32_t modifier);

// The world's table, read once from Extract/oracle.
bool load(const std::filesystem::path &oracleDir, std::string *why = nullptr);
bool loaded();
// The model number for an item record (0xa0 bytes, game layout); -1 for none or unknown,
// and for category 0x17, whose models are not in the list (see onDemandModel).
int modelId(const uint8_t *item);

} // namespace nativeitems
