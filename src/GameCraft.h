#pragma once
// Crafting and weapon upgrades (docs/specs/loot-crafting.md):
//
//   * the recipe of an item (0x2aec30): what it takes and at which station it is made;
//   * the default recipes every character knows (frontend-characters.md §5.6, 0x9afb0);
//   * the crafting lists (0xb2180): the known recipes by tab, then for every unlocked rank the
//     region's own weapons, armour and jewellery for the player's class;
//   * how many times a recipe can be made from what is carried (0xb2ee0);
//   * the craft button (0xb03d0): held for 1000 ms for the first item, each further one 1.2
//     times faster, down to 200 ms;
//   * the extra yield of resources (0x8fbc0);
//   * upgrade cubes placed on a weapon (0x108b90, 0x10a0e0), which the weapon damage reads
//     as its upgrade count (nativerules::weaponDamage).
#include <array>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace gamecraft {

// An item as the recipes use it: type (+0x00, "category"), subtype (+0x04, "id"), material
// (+0x1c), rarity (+0x14), modifier (+0x08) and region (+0x0c, +0x10).
struct Thing {
    int category = 0, id = 0, material = 0, rarity = 0;
    uint32_t modifier = 0;
    int region[2] = {0, 0};
    bool operator==(const Thing &o) const {
        return category == o.category && id == o.id && material == o.material && rarity == o.rarity && modifier == o.modifier &&
               region[0] == o.region[0] && region[1] == o.region[1];
    }
};
struct Ingredient {
    Thing item;
    int quantity = 0;
};

// The station number a recipe asks for (0x2aec30 writes it at record +0xa0). The game only
// compares these numbers (loot-crafting.md §1.7, open question 3); the names are CubeVulkan's
// reading of what each recipe makes (water flasks at 1, ingots at 3, iron gear at 4, yarn at 5,
// cloth gear at 6, planks at 7, wooden weapons at 8, food at 9).
enum Workstation { None = 0, Water = 1, Furnace = 3, Anvil = 4, SpinningWheel = 5, Loom = 6, Saw = 7, Workbench = 8, Campfire = 9 };

struct Formula {
    Thing result;
    int workstation = None;
    std::vector<Ingredient> ingredients;
};

// The recipe of an item (0x2aec30, §1). A type-0x1d item has an empty recipe (result type 0).
Formula recipeOf(const Thing &item);

// The default recipes (0x9afb0 with argument 0, frontend-characters.md §5.6), in the order the
// game adds them.
std::vector<Thing> startingFormulas(int playerClass);

// The crafting window's tabs (§2.1, §2.2).
enum Tab { Weapons, Armor, Amulets, Cooking, Alchemy, Ingredients, TABS };

// The crafting lists (0xb2180, §2): the known recipes sorted into tabs, then for each rank
// 0..4 that `hasRank` unlocks the region's rank gear for the class. Draws from the game's
// rand(), seeded per rank from the region, so the lists are the same every time.
std::array<std::vector<Formula>, TABS> formulas(const std::vector<Thing> &known, int playerClass, int regionX, int regionY,
                                                const std::function<bool(int rank)> &hasRank);

// Whether a rank's gear is listed (0x2b4260, §2.2 step 1): rank 0 always; rank k from 1 to 4
// once a place of role k + 1 in region (x, y) (the crafting-book places) is in `solved`, the
// player's solved place cells (player data +0xc8).
bool rankUnlocked(int rank, int32_t regionX, int32_t regionY, const std::set<std::array<int32_t, 2>> &solved);

// How many times a recipe can be made (0xb2ee0, §2.3): the fewest any ingredient allows, from
// `held` (the total quantity of equal stacks carried); 0 for a recipe without ingredients.
int times(const Formula &formula, const std::function<int(const Thing &)> &held);

// The extra yield of a crafted resource (0x8fbc0 step 5, §3.2): type 11 but not subtype 0x1a,
// one chance in five, up to three times. Draws only while it keeps succeeding.
int extraPieces(const Thing &result, const std::function<int()> &rand);

// The craft button held down (0xb03d0, §3.1). `progress` is −1 while nothing is being made.
struct CraftButton {
    static constexpr float START_SPEED = 0.001f; // progress per ms: 1000 ms for the first item
    static constexpr float SPEED_UP = 1.2f;      // after each item made while held
    static constexpr float TOP_SPEED = 0.005f;   // 200 ms an item at most
    float progress = -1.0f;
    float speed = START_SPEED;
    // One frame of `dtMs`. `shown`: the crafting window is open; `pressed`: the button is ready
    // and held. Returns true when an item is to be made this frame; `started` is set on the
    // frame a new item starts (the caller then takes the selected recipe).
    bool update(float dtMs, bool shown, bool pressed, bool *started = nullptr);
};

// What the crafting refusals and the recipe details say about a station: "Requires Anvil" and
// so on, "" for none.
std::string requirement(int workstation);
// The zone object type of a station (use-prompt.md §3.7: campfire 0x53, furnace 0x62, anvil
// 0x63, spinning wheel 0x64, loom 0x65, saw 0x66, workbench 0x67), −1 for water and none.
int stationEntity(int workstation);

// --- Upgrade cubes (§4) ----------------------------------------------------------------------
// The item record's attribute entries: four bytes each from +0x1d (x, y, z, material), their
// number at +0x9d.

// How many cubes a weapon holds: 32 for a two-handed weapon (type 3, a 2H subtype), else 16.
int cubeCap(const uint8_t *item);
// 0x108b90: a cube at (x, y, z). An existing one there only changes its material; a new one is
// appended while there is room. False when it did not fit.
bool placeCube(uint8_t *item, int8_t x, int8_t y, int8_t z, uint8_t material);
// 0x10a0e0: removes the first cube at (x, y, z); later ones move down. False when none is there.
bool removeCube(uint8_t *item, int8_t x, int8_t y, int8_t z);

} // namespace gamecraft
