// Crafting and weapon upgrades, written from docs/specs/loot-crafting.md (and the default
// recipes of frontend-characters.md §5.6). See GameCraft.h.
#include "GameCraft.h"

#include "NativeRules.h"
#include "NativeInteract.h"
#include "worldgen/Noise.h"
#include "worldgen/Planner.h"

#include <algorithm>
#include <cstring>

namespace gamecraft {
namespace {

// Item types the recipes name.
constexpr int CONSUMABLE = 1, WEAPON = 3, CHEST = 4, HANDS = 5, FEET = 6, SHOULDER = 7, NECK = 8, RING = 9, RESOURCE = 11;
constexpr int DISH = 0x1c;   // the 0x1c recipes (dishes and elixirs; the dictionary names them)
constexpr int NO_RECIPE = 0x1d;

// Resource subtypes the recipe table takes its ingredients from (type 11).
constexpr int YARN = 9, CUBE = 10;
constexpr int TIER_STONE_BASE = 0x1f; // the tier ingredient is subtype 0x1f + rarity (0x20..0x23)
constexpr int WATER_FLASK = 0x1a;     // the one resource without an extra yield

// Materials the recipe table tests.
constexpr int IRON = 1, WOOD = 2;

// Item record fields (drops-and-items.md §0.1) for the upgrade cubes.
constexpr size_t ITEM_TYPE = 0x00, ITEM_SUBTYPE = 0x04, ATTRIBUTES = 0x1d, ATTRIBUTE_SIZE = 4, ATTRIBUTE_COUNT = 0x9d;
constexpr int CUBES_ONE_HANDED = 16, CUBES_TWO_HANDED = 32;

// The rank gear's seed offset (§2.2 step 2) and the extra-yield odds (§3.2 step 5).
constexpr uint32_t RANK_SEED_OFFSET = 0xf5f9;
constexpr int EXTRA_ODDS = 5, EXTRA_MOST = 3;
constexpr int RANKS = 5;

// The 2H set of stat-rules.md §0.3, by subtype.
bool twoHanded(int subtype) {
    uint8_t record[0xa0]{};
    record[ITEM_TYPE] = WEAPON;
    std::memcpy(record + ITEM_SUBTYPE, &subtype, 4);
    return nativerules::twoHandedWeapon(record);
}

// One recipe being written: the result and its ingredients.
class RecipeWriter {
  public:
    explicit RecipeWriter(Formula &f) : f(f) {}
    // An ingredient: always type 11, modifier and region 0.
    void add(int subtype, int quantity, int material = 0, int rarity = 0) {
        Ingredient in;
        in.item.category = RESOURCE, in.item.id = subtype, in.item.material = material, in.item.rarity = rarity;
        in.quantity = quantity;
        f.ingredients.push_back(in);
    }
    void station(int s) { f.workstation = s; }

  private:
    Formula &f;
};

// §1.1, type 1.
void consumable(RecipeWriter &w, int subtype, int r) {
    switch (subtype) {
    case 1:
        w.add(0x16, 1);
        switch (r) { // a stronger potion takes its rarity's flower
        case 1: w.add(0x1c, 1, 0, 1); break;
        case 2: w.add(0x1d, 1, 0, 2); break;
        case 3: w.add(0x1e, 1, 0, 3); break;
        case 4: w.add(0x19, 1, 0, 4); break;
        default: break;
        }
        w.add(0x1a, 1);
        break;
    case 2: w.add(0x17, 1), w.add(0x0c, 1, 0x19); break;
    case 3: w.add(0x1d, 1, 0, 1), w.add(0x1a, 1); break;
    case 4: w.add(0x14, 4), w.station(Campfire); break;
    case 7: w.add(0x1b, 1); break;
    case 8: w.add(0x0f, 1), w.add(0x15, 1), w.station(Campfire); break;
    case 10: w.add(0x11, 1); break;
    case 11: w.add(0x10, 1), w.station(Campfire); break;
    case 12: w.add(0x16, 1), w.add(0x19, 1, 0, 1), w.add(0x1a, 1); break;
    default: break;
    }
}

// §1.2, type 3. n = rarity + 1.
void weapon(RecipeWriter &w, int subtype, int n, int m) {
    switch (subtype) {
    case 0: case 1: case 2: case 3: case 13:
        w.add(CUBE, 5 * n, IRON), w.station(Anvil);
        break;
    case 4:
        w.add(CUBE, 4 * n, IRON), w.add(YARN, 2 * n, 0x1c), w.station(Anvil);
        break;
    case 5: case 15: case 16: case 17:
        w.add(CUBE, 10 * n, IRON), w.station(Anvil);
        break;
    case 6:
        w.add(CUBE, 10 * n, WOOD), w.add(YARN, n, 0x1b), w.station(Workbench);
        break;
    case 7:
        w.add(CUBE, 2 * n, IRON), w.add(CUBE, 8 * n, WOOD), w.add(YARN, n, 0x1b), w.station(Workbench);
        break;
    case 8: case 10: case 11:
        w.add(CUBE, 10 * n, WOOD), w.station(Workbench);
        break;
    case 12:
        w.add(CUBE, 5 * n, m), w.station(Anvil);
        break;
    default: break; // 9, 14, 18 and above
    }
}

// §1.3, armour: iron at the anvil, anything else as yarn at the loom.
void armour(RecipeWriter &w, int type, int n, int m) {
    const int k = type == CHEST ? 10 * n : type == SHOULDER ? 6 * n : 5 * n;
    if (m == IRON)
        w.add(CUBE, k, IRON), w.station(Anvil);
    else
        w.add(YARN, k, m), w.station(Loom);
}

// §1.5, type 11.
void resource(RecipeWriter &w, int subtype, int m) {
    switch (subtype) {
    case YARN: // spun from the material's fibre; other materials have no recipe
        if (m == 0x1a)
            w.add(6, 1), w.station(SpinningWheel);
        else if (m == 0x1b)
            w.add(5, 1, 0x16), w.station(SpinningWheel);
        else if (m == 0x1c)
            w.add(0x0b, 1, 0x1c), w.station(SpinningWheel);
        break;
    case CUBE: // wood is sawn from a log, anything else smelted from its ore
        if (m == WOOD)
            w.add(1, 1, WOOD), w.station(Saw);
        else
            w.add(0, 1, m), w.station(Furnace);
        break;
    case 0x16: w.add(0x18, 1), w.station(Campfire); break;
    case WATER_FLASK: w.add(0x0c, 1, 0x19), w.station(Water); break;
    default: break;
    }
}

// §1.6, type 0x1c; every one ends with (0x1a) x1. n = rarity + 1.
void dish(RecipeWriter &w, int subtype, int n) {
    switch (subtype) {
    case 0: w.add(0x16, 5 * n), w.add(0x14, n); break;
    case 1: w.add(0x0f, 5 * n), w.add(0x24, 2 * n); break;
    case 2: w.add(0x0f, 5 * n), w.add(0x25, 2 * n); break;
    case 3: w.add(0x13, 5 * n), w.add(0x1d, 2 * n); break;
    case 4: w.add(0x26, 2), w.add(0x27, 2), w.add(0x28, 3); break;
    case 5: w.add(0x26, 1), w.add(0x2b, 3), w.station(Campfire); break;
    case 6: w.add(0x29, 2), w.add(0x2a, 2); break;
    default: return; // no ingredients at all
    }
    w.add(0x1a, 1);
}

// §1.7: the tier stone of rarity 1..4 for everything but consumables and dishes.
void tierIngredient(RecipeWriter &w, const Thing &q) {
    if (q.category == CONSUMABLE || q.category == DISH || q.rarity < 1 || q.rarity > 4)
        return;
    const bool big = q.category == CHEST || (q.category == WEAPON && twoHanded(q.id));
    w.add(TIER_STONE_BASE + q.rarity, big ? 4 : 2, 0, q.rarity);
}

// The crafting tab of a known recipe (§2.1); −1 when it is not listed.
int tabOfKnown(const Thing &k) {
    switch (k.category) {
    case WEAPON: return Weapons;
    case CONSUMABLE: return (k.id == 1 || k.id == 2 || k.id == 3 || k.id == 12) ? Alchemy : Cooking;
    case DISH: return (k.id == 4 || k.id == 5 || k.id == 6) ? Cooking : Alchemy;
    case RESOURCE: return Ingredients;
    default: return -1;
    }
}

// The armour material of each class (§2.2 step 5); 0 keeps the weapon's.
int armourMaterial(int playerClass) {
    switch (playerClass) {
    case 1: return IRON;
    case 2: return 0x1b;
    case 3: return 0x1a;
    case 4: return 0x1c;
    default: return 0;
    }
}

} // namespace

Thing fromRecord(const ItemRecord &record) {
    Thing item;
    item.category = record[0];
    std::memcpy(&item.id, record.data() + 4, 4);
    std::memcpy(&item.modifier, record.data() + 8, 4);
    std::memcpy(item.region, record.data() + 0xc, 8);
    item.rarity = record[0x14];
    std::memcpy(&item.formula, record.data() + 0x18, 4);
    item.material = record[0x1c];
    std::memcpy(item.attributes.data(), record.data() + ATTRIBUTES, item.attributes.size());
    item.attributeCount = record[ATTRIBUTE_COUNT];
    return item;
}
ItemRecord toRecord(const Thing &item) {
    ItemRecord record{};
    record[0] = (uint8_t)item.category;
    std::memcpy(record.data() + 4, &item.id, 4);
    std::memcpy(record.data() + 8, &item.modifier, 4);
    std::memcpy(record.data() + 0xc, item.region, 8);
    record[0x14] = (uint8_t)item.rarity;
    std::memcpy(record.data() + 0x18, &item.formula, 4);
    record[0x1c] = (uint8_t)item.material;
    std::memcpy(record.data() + ATTRIBUTES, item.attributes.data(), item.attributes.size());
    record[ATTRIBUTE_COUNT] = item.attributeCount;
    return record;
}
bool Thing::operator==(const Thing &other) const {
    const auto left = toRecord(*this), right = toRecord(other);
    return nativeinteract::sameItem(left.data(), right.data());
}

Formula recipeOf(const Thing &q) {
    Formula f;
    if (q.category == NO_RECIPE)
        return f; // an empty record: its result is type 0
    f.result = q;
    RecipeWriter w(f);
    const int n = q.rarity + 1, m = q.material;
    switch (q.category) {
    case CONSUMABLE: consumable(w, q.id, q.rarity); break;
    case WEAPON: weapon(w, q.id, n, m); break;
    case CHEST: case HANDS: case FEET: case SHOULDER: armour(w, q.category, n, m); break;
    case NECK: case RING: w.add(CUBE, 5 * n, m); break; // station 0
    case RESOURCE: resource(w, q.id, m); break;
    case DISH: dish(w, q.id, n); break;
    default: break;
    }
    tierIngredient(w, q);
    return f;
}

std::vector<Thing> startingFormulas(int playerClass) {
    std::vector<Thing> known;
    auto add = [&](int type, int subtype, int material = 0) {
        Thing t;
        t.category = type, t.id = subtype, t.material = material; // rarity: the routine's argument, 0
        if (std::find(known.begin(), known.end(), t) == known.end())
            known.push_back(t);
    };
    for (int subtype : {1, 2, 4, 10, 8, 11, 7})
        add(CONSUMABLE, subtype);
    for (int subtype = 0; subtype <= 6; subtype++)
        add(DISH, subtype);
    add(RESOURCE, WATER_FLASK);
    add(RESOURCE, 0x16);
    if (playerClass == 4)
        add(RESOURCE, YARN, 0x1c);
    if (playerClass == 2)
        add(RESOURCE, YARN, 0x1b);
    if (playerClass == 3)
        add(RESOURCE, YARN, 0x1a);
    for (int material : {1, 2, 11, 12})
        add(RESOURCE, CUBE, material);
    return known;
}

std::array<std::vector<Formula>, TABS> formulas(const std::vector<Thing> &known, int playerClass, int regionX, int regionY,
                                                const std::function<bool(int rank)> &hasRank) {
    std::array<std::vector<Formula>, TABS> tabs;
    for (const Thing &k : known)
        if (const int tab = tabOfKnown(k); tab >= 0)
            tabs[(size_t)tab].push_back(recipeOf(k));

    worldgen::CrtRandom rng;
    const uint32_t regionSeed = worldgen::random(worldgen::cellKey(regionX, regionY) + RANK_SEED_OFFSET);
    for (int rank = 0; rank < RANKS; rank++) {
        if (!hasRank || !hasRank(rank))
            continue;
        rng.seed(regionSeed + (uint32_t)rank);
        Thing item;
        item.category = WEAPON, item.rarity = rank;
        item.region[0] = regionX, item.region[1] = regionY;
        // "make": a fresh modifier, then the recipe goes to the tab.
        auto make = [&](int tab) {
            item.modifier = (uint32_t)rng.next();
            tabs[(size_t)tab].push_back(recipeOf(item));
        };
        auto weaponOf = [&](int subtype) { item.id = subtype, make(Weapons); };
        switch (playerClass) {
        case 1:
            item.material = IRON;
            weaponOf(rng.next() % 3);
            weaponOf(13);
            weaponOf(rng.next() % 3 + 15);
            break;
        case 2:
            item.material = WOOD;
            weaponOf(6), weaponOf(7), weaponOf(8);
            break;
        case 3:
            item.material = WOOD;
            weaponOf(11), weaponOf(10);
            item.material = rng.next() % 2 == 0 ? 11 : 12;
            weaponOf(12);
            break;
        case 4:
            item.material = IRON;
            weaponOf(3), weaponOf(4), weaponOf(5);
            break;
        default: break;
        }
        if (const int m = armourMaterial(playerClass))
            item.material = m;
        item.id = 0;
        for (int type : {CHEST, SHOULDER, HANDS, FEET})
            item.category = type, make(Armor);
        for (int type : {NECK, RING}) {
            item.category = type;
            item.material = rng.next() % 2 == 0 ? 11 : 12;
            make(Amulets);
        }
    }
    return tabs;
}

bool rankUnlocked(int rank, int32_t regionX, int32_t regionY, const std::set<std::array<int32_t, 2>> &solved) {
    if (rank == 0)
        return true;
    const auto plan = worldgen::regionPlan(regionX, regionY);
    for (const auto &[place, role] : plan->roles)
        if (role == rank + 1 && solved.count({place.first, place.second}))
            return true;
    return false;
}

int times(const Formula &formula,const std::function<int(const Thing &)> &held) {
    if (formula.ingredients.empty())
        return 0;
    int fewest = -1;
    for (const Ingredient &in : formula.ingredients) {
        const int n = in.quantity > 0 ? held(in.item) / in.quantity : 0;
        fewest = fewest < 0 ? n : std::min(fewest, n);
    }
    return std::max(fewest, 0);
}

int extraPieces(const Thing &result, const std::function<int()> &rand) {
    if (result.category != RESOURCE || result.id == WATER_FLASK)
        return 0;
    int extra = 0;
    while (extra < EXTRA_MOST && rand() % EXTRA_ODDS == 0)
        extra++;
    return extra;
}

bool CraftButton::update(float dtMs, bool shown, bool pressed, bool *started) {
    if (started)
        *started = false;
    if (!shown) {
        progress = -1.0f;
        return false;
    }
    if (!pressed)
        speed = START_SPEED; // (an item already started still completes, open question 4)
    else if (progress < 0.0f) {
        progress = 0.0f;
        if (started)
            *started = true;
    }
    if (progress < 0.0f)
        return false;
    progress += dtMs * speed;
    if (progress < 1.0f)
        return false;
    speed = std::min(speed * SPEED_UP, TOP_SPEED);
    progress = -1.0f;
    return true;
}

std::string requirement(int workstation) {
    switch (workstation) {
    case Water: return "Requires Water";
    case Furnace: return "Requires Furnace";
    case Anvil: return "Requires Anvil";
    case SpinningWheel: return "Requires Spinning Wheel";
    case Loom: return "Requires Loom";
    case Saw: return "Requires Saw";
    case Workbench: return "Requires Workbench";
    case Campfire: return "Requires Campfire";
    default: return "";
    }
}

int stationEntity(int workstation) {
    switch (workstation) {
    case Campfire: return 0x53;
    case Furnace: return 0x62;
    case Anvil: return 0x63;
    case SpinningWheel: return 0x64;
    case Loom: return 0x65;
    case Saw: return 0x66;
    case Workbench: return 0x67;
    default: return -1;
    }
}

int cubeCap(const uint8_t *item) {
    return nativerules::twoHandedWeapon(item) ? CUBES_TWO_HANDED : CUBES_ONE_HANDED;
}

bool placeCube(uint8_t *item, int32_t x, int32_t y, int32_t z, uint8_t material) {
    const int count = item[ATTRIBUTE_COUNT];
    for (int i = 0; i < count; i++) {
        uint8_t *cube = item + ATTRIBUTES + (size_t)i * ATTRIBUTE_SIZE;
        if (cube[0] == (uint8_t)x && cube[1] == (uint8_t)y && cube[2] == (uint8_t)z) {
            cube[3] = material;
            return true;
        }
    }
    if (count >= cubeCap(item))
        return false;
    uint8_t *cube = item + ATTRIBUTES + (size_t)count * ATTRIBUTE_SIZE;
    cube[0] = (uint8_t)x, cube[1] = (uint8_t)y, cube[2] = (uint8_t)z, cube[3] = material;
    item[ATTRIBUTE_COUNT] = (uint8_t)(count + 1);
    return true;
}

bool removeCube(uint8_t *item, int32_t x, int32_t y, int32_t z) {
    const int count = item[ATTRIBUTE_COUNT];
    for (int i = 0; i < count; i++) {
        uint8_t *cube = item + ATTRIBUTES + (size_t)i * ATTRIBUTE_SIZE;
        if ((int8_t)cube[0] != x || (int8_t)cube[1] != y || (int8_t)cube[2] != z)
            continue;
        std::memmove(cube, cube + ATTRIBUTE_SIZE, (size_t)(count - 1 - i) * ATTRIBUTE_SIZE);
        std::memset(item + ATTRIBUTES + (size_t)(count - 1) * ATTRIBUTE_SIZE, 0, ATTRIBUTE_SIZE);
        item[ATTRIBUTE_COUNT] = (uint8_t)(count - 1);
        return true;
    }
    return false;
}

} // namespace gamecraft
