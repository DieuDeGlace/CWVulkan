// Which model the game draws for an item (0x173b10), from docs/specs/item-models.md.
#include "NativeItems.h"

#include <cstring>
#include <fstream>
#include <iterator>

namespace nativeitems {

namespace {

int32_t readI32(const uint8_t *p) {
    int32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

uint32_t readU32(const uint8_t *p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

// Category 0x16's stand-in ids (spec §1).
constexpr uint32_t STAND_IN_WEAPON = 0x3f;
constexpr uint32_t STAND_IN_CHEST = 0x40;
constexpr uint32_t STAND_IN_NECK = 0x41;
constexpr uint32_t STAND_IN_RING = 0x42;

// Ids and categories the rules give no model (spec §2.4).
constexpr uint8_t CATEGORY_NONE = 0x00;
constexpr uint8_t CATEGORY_NEVER = 0x1e;
constexpr uint32_t WEAPON_WITHOUT_MODEL_A = 0x16;
constexpr uint32_t WEAPON_WITHOUT_MODEL_B = 0x17;
constexpr uint32_t CHEST_WITHOUT_MODEL = 1;

// Category 1's one id whose rarity is added unclamped (0x1738f0).
constexpr uint8_t CATEGORY_RAW_RARITY = 0x01;
constexpr uint32_t RAW_RARITY_ID = 1;

// The rules of spec §2.4, one row per category up to 0x21. The modifier periods are the
// least common multiples of the remainders each category takes: weapons 11 and 6 (66),
// gear 5, categories 0x11 and 0x12 4, 0x13 3, 0x16 2, 3, 4, 5, 6 and 10 (60).
CategoryRule makeRule(char kind, bool id, bool material, bool rarity, uint32_t period) {
    CategoryRule r;
    r.never = kind == 'x';
    r.onDemand = kind == 'd';
    r.readsId = id;
    r.readsMaterial = material;
    r.readsRarity = rarity;
    r.modifierPeriod = period;
    return r;
}

const std::array<CategoryRule, LAST_CATEGORY_WITH_MODELS + 1> &categoryRules() {
    //                                     kind  id     mat    rarity period
    static const std::array<CategoryRule, LAST_CATEGORY_WITH_MODELS + 1> rules = {
        makeRule('x', false, false, false, 1),  // 0x00 none
        makeRule('-', true, false, true, 1),    // 0x01 by id; id 1 adds the raw rarity
        makeRule('-', false, false, false, 1),  // 0x02 one
        makeRule('-', true, true, true, 66),    // 0x03 weapons: id, material, rarity, mod 11 / mod 6
        makeRule('-', true, true, true, 5),     // 0x04 chest: material (and id), rarity, mod 5
        makeRule('-', false, true, true, 5),    // 0x05
        makeRule('-', false, true, true, 5),    // 0x06
        makeRule('-', false, true, true, 5),    // 0x07
        makeRule('-', false, true, true, 5),    // 0x08 whether the material is 12, rarity, mod 5
        makeRule('-', false, true, true, 5),    // 0x09
        makeRule('-', false, false, false, 1),  // 0x0a one
        makeRule('-', true, true, false, 1),    // 0x0b id, then material
        makeRule('-', false, true, false, 1),   // 0x0c material
        makeRule('-', false, false, false, 1),  // 0x0d one
        makeRule('-', false, false, false, 1),  // 0x0e one
        makeRule('-', false, false, true, 1),   // 0x0f rarity (shifted)
        makeRule('-', false, false, false, 1),  // 0x10 one
        makeRule('-', false, false, false, 4),  // 0x11 mod 4
        makeRule('-', false, false, false, 4),  // 0x12 mod 4
        makeRule('-', true, false, false, 3),   // 0x13 whether the id is 1, mod 3
        makeRule('-', true, false, false, 1),   // 0x14 id 0x18, 0x19 or another
        makeRule('-', true, false, false, 1),   // 0x15 id (up to 0x127)
        makeRule('-', true, false, false, 60),  // 0x16 id, mod 2, 3, 4, 5, 6 or 10
        makeRule('d', false, false, false, 1),  // 0x17 made on demand
        makeRule('-', true, false, false, 1),   // 0x18 id
        makeRule('-', false, false, false, 1),  // 0x19 one
        makeRule('-', true, false, false, 1),   // 0x1a id
        makeRule('-', false, false, false, 1),  // 0x1b one
        makeRule('-', true, false, false, 1),   // 0x1c id
        makeRule('-', false, false, false, 1),  // 0x1d one
        makeRule('x', false, false, false, 1),  // 0x1e none
        makeRule('-', false, false, false, 1),  // 0x1f one
        makeRule('-', false, false, false, 1),  // 0x20 one
        makeRule('-', false, false, false, 1),  // 0x21 one
    };
    return rules;
}

// A small cursor over the extracted file.
struct Reader {
    const std::vector<uint8_t> &bytes;
    size_t at = 0;
    bool fits(size_t n) const { return at + n <= bytes.size(); }
    uint8_t u8() { return bytes[at++]; }
    uint16_t u16() {
        const uint16_t v = (uint16_t)(bytes[at] | bytes[at + 1] << 8);
        at += 2;
        return v;
    }
    uint32_t u32() {
        const uint32_t v = readU32(bytes.data() + at);
        at += 4;
        return v;
    }
};

bool fail(std::string *why, const std::string &text) {
    if (why)
        *why = text;
    return false;
}

} // namespace

// --- the item's fields ---------------------------------------------------------------

ModelFields normalised(ModelFields f, int32_t standsFor) {
    if (f.category != CATEGORY_STAND_IN)
        return f;
    switch (f.id) {
    case STAND_IN_WEAPON:
        f.category = CATEGORY_WEAPON;
        f.id = (uint32_t)standsFor;
        break;
    case STAND_IN_CHEST:
        f.category = CATEGORY_CHEST, f.id = 0;
        break;
    case STAND_IN_NECK:
        f.category = CATEGORY_NECK, f.id = 0;
        break;
    case STAND_IN_RING:
        f.category = CATEGORY_RING, f.id = 0;
        break;
    default:
        break;
    }
    return f;
}

ModelFields fieldsOf(const uint8_t *item) {
    ModelFields f;
    f.category = item[ITEM_CATEGORY];
    f.id = (uint32_t)readI32(item + ITEM_ID);
    f.modifier = readU32(item + ITEM_MODIFIER);
    f.rarity = item[ITEM_RARITY];
    f.material = item[ITEM_MATERIAL];
    return normalised(f, readI32(item + ITEM_STANDS_FOR));
}

// --- the rules -----------------------------------------------------------------------

const CategoryRule &categoryRule(uint8_t category) {
    static const CategoryRule none = makeRule('x', false, false, false, 1);
    return category <= LAST_CATEGORY_WITH_MODELS ? categoryRules()[category] : none;
}

bool neverHasModel(const ModelFields &f) {
    if (f.category == CATEGORY_NONE || f.category == CATEGORY_NEVER || f.category > LAST_CATEGORY_WITH_MODELS)
        return true;
    if (f.category == CATEGORY_WEAPON && (f.id == WEAPON_WITHOUT_MODEL_A || f.id == WEAPON_WITHOUT_MODEL_B))
        return true;
    return f.category == CATEGORY_CHEST && f.id == CHEST_WITHOUT_MODEL;
}

bool usesRawRarity(const ModelFields &f) {
    return f.category == CATEGORY_RAW_RARITY && f.id == RAW_RARITY_ID;
}

int rarityTerm(const ModelFields &f) {
    if (usesRawRarity(f))
        return f.rarity;
    if (f.category == CATEGORY_RARITY_SHIFTED) {
        const int shifted = f.rarity > 0 ? f.rarity - 1 : 0;
        return shifted < 3 ? shifted : 3;
    }
    return f.rarity < HIGHEST_DISTINCT_RARITY ? f.rarity : HIGHEST_DISTINCT_RARITY;
}

uint32_t modifierTerm(uint32_t modifier) {
    return modifier % MODIFIER_PERIOD;
}

uint32_t modifierTerm(const ModelFields &f) {
    return modifierTerm(f.modifier) % categoryRule(f.category).modifierPeriod;
}

// --- the extracted table -------------------------------------------------------------

uint64_t ModelTable::keyOf(uint8_t category, uint32_t id, uint8_t material) {
    return (uint64_t)category << 40 | (uint64_t)material << 32 | id;
}

void ModelTable::setPeriodAndIdBound(uint32_t period, uint32_t bound, bool coversAbove) {
    modifierPeriod = period;
    idBound = bound;
    idBoundCoversAbove = coversAbove;
}

void ModelTable::add(uint8_t category, uint32_t id, uint8_t material, uint32_t rarities, uint32_t modifiers,
                     std::vector<int32_t> models) {
    Grid g;
    g.rarities = rarities;
    g.modifiers = modifiers;
    g.models = std::move(models);
    keys[keyOf(category, id, material)] = (uint32_t)grids.size();
    grids.push_back(std::move(g));
}

// The row an item reads: rarities that the rules say look the same share one row, which is
// the lowest rarity extracted for them. Category 0x0f's rarity 1 reads rarity 0's row.
static uint32_t rarityRow(const ModelFields &f) {
    if (!categoryRule(f.category).readsRarity)
        return 0;
    if (usesRawRarity(f))
        return f.rarity;
    const int term = rarityTerm(f);
    if (f.category == CATEGORY_RARITY_SHIFTED)
        return term == 0 ? 0 : (uint32_t)term + 1; // rarity term t is first reached at rarity t + 1
    return (uint32_t)term;
}

int ModelTable::fromGrid(const Grid &g, const ModelFields &f) const {
    uint32_t row = rarityRow(f);
    if (row >= g.rarities) {
        // Category 1 id 1 in a table that stops at rarity 4 (CWI2): the tree adds the raw
        // rarity to a base index, which the extracted rows show as a run base, base + 1, ...
        const int32_t base = g.models[0];
        bool run = usesRawRarity(f) && base >= 0 && g.modifiers == 1;
        for (uint32_t r = 1; run && r < g.rarities; r++)
            run = g.models[r] == base + (int32_t)r;
        if (run)
            return base + (int)row;
        row = g.rarities - 1;
    }
    const uint32_t column = g.modifiers == 1 ? 0 : modifierTerm(f) % g.modifiers;
    return g.models[(size_t)row * g.modifiers + column];
}

int ModelTable::modelNumber(const ModelFields &f) const {
    const CategoryRule &rule = categoryRule(f.category);
    if (rule.never || rule.onDemand || neverHasModel(f))
        return -1;
    // Fields the category never reads are looked up as 0, so any value finds its key.
    uint32_t id = rule.readsId ? f.id : 0;
    const uint8_t material = rule.readsMaterial ? f.material : 0;
    if (id >= idBound) {
        if (!idBoundCoversAbove)
            return -1; // beyond what the table holds
        id = idBound;
    }
    const auto it = keys.find(keyOf(f.category, id, material));
    return it == keys.end() ? -1 : fromGrid(grids[it->second], f);
}

// CWI2 (the first extraction): ids 0..127, materials 0..63, the modifier modulo 330.
//   u32 grid count; each: i32 kind (0 one, 1 five rarities, 2 330 modifiers, 3 both,
//   rarity-major), its numbers as i32; u32 key count; each: u8 category, u8 id,
//   u8 material, u8 0, u16 grid.
// CWI3 (spec §4 widened): ids up to the bound, whose own key answers for every id above it;
// materials 0..63 and 0x80..0x8f; the modifier modulo 660; category 1 id 1 by raw rarity.
//   u32 modifier period, u32 id bound, u32 grid count; each: u32 rarities, u32 modifiers
//   (1 or the period), the numbers as i32, rarity-major; u32 key count; each: u8 category,
//   u8 material, u16 id, u32 grid.
bool ModelTable::parse(const std::vector<uint8_t> &bytes, std::string *why) {
    grids.clear();
    keys.clear();
    Reader in{bytes};
    if (!in.fits(8))
        return fail(why, "item model table: too short");
    const bool v3 = std::memcmp(bytes.data(), "CWI3", 4) == 0;
    if (!v3 && std::memcmp(bytes.data(), "CWI2", 4) != 0)
        return fail(why, "item model table: not CWI2 or CWI3");
    in.at = 4;
    if (v3) {
        if (!in.fits(8))
            return fail(why, "item model table: truncated header");
        const uint32_t period = in.u32();
        const uint32_t bound = in.u32();
        if (period == 0 || MODIFIER_PERIOD % period != 0)
            return fail(why, "item model table: odd modifier period");
        setPeriodAndIdBound(period, bound, true);
    } else {
        setPeriodAndIdBound(330, 128, false);
    }
    if (!in.fits(4))
        return fail(why, "item model table: truncated");
    const uint32_t gridCount = in.u32();
    grids.reserve(gridCount);
    for (uint32_t i = 0; i < gridCount; i++) {
        Grid g;
        if (v3) {
            if (!in.fits(8))
                return fail(why, "item model table: truncated grid");
            g.rarities = in.u32();
            g.modifiers = in.u32();
            if (g.rarities == 0 || g.rarities > 256 || (g.modifiers != 1 && g.modifiers != modifierPeriod))
                return fail(why, "item model table: bad grid shape");
        } else {
            if (!in.fits(4))
                return fail(why, "item model table: truncated grid");
            const uint32_t kind = in.u32();
            if (kind > 3)
                return fail(why, "item model table: bad grid kind");
            g.rarities = kind & 1 ? 5 : 1;
            g.modifiers = kind & 2 ? 330 : 1;
        }
        const size_t n = (size_t)g.rarities * g.modifiers;
        if (!in.fits(n * 4))
            return fail(why, "item model table: truncated grid");
        g.models.resize(n);
        for (auto &m : g.models)
            m = (int32_t)in.u32();
        grids.push_back(std::move(g));
    }
    if (!in.fits(4))
        return fail(why, "item model table: no keys");
    const uint32_t keyCount = in.u32();
    for (uint32_t i = 0; i < keyCount; i++) {
        uint8_t category, material;
        uint32_t id, grid;
        if (v3) {
            if (!in.fits(8))
                return fail(why, "item model table: truncated keys");
            category = in.u8();
            material = in.u8();
            id = in.u16();
            grid = in.u32();
        } else {
            if (!in.fits(6))
                return fail(why, "item model table: truncated keys");
            category = in.u8();
            id = in.u8();
            material = in.u8();
            in.u8(); // padding
            grid = in.u16();
        }
        if (grid >= grids.size())
            return fail(why, "item model table: key names a missing grid");
        keys[keyOf(category, id, material)] = grid;
    }
    return true;
}

bool ModelTable::load(const std::filesystem::path &file, std::string *why) {
    std::ifstream in(file, std::ios::binary);
    if (!in)
        return fail(why, "no " + file.string());
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return parse(bytes, why);
}

// --- category 0x17: models made on demand (spec §3) -----------------------------------

namespace {

constexpr int ON_DEMAND_SIZE = 24;
constexpr int ON_DEMAND_PLAIN_SHAPES = 7; // ids 0..6 have their own generator; others share one

// The seed of the draw counter, from the id and the modifier.
uint32_t onDemandCounter(int32_t id, uint32_t modifier) {
    uint32_t v = modifier * 57u + (uint32_t)id;
    v ^= v << 13;
    const uint32_t h = ((v * v * 60493u + 19990303u) * v + 0xd208dd0du) & 0x7fffffffu;
    return 100000u - h / 21474u;
}

// One channel drawn from a counter value: an xorshift of it, 10..259.
float colourChannel(uint32_t c) {
    uint32_t x = c ^ (c << 13);
    x ^= x >> 17;
    x ^= x << 5;
    return (float)(x % 250u + 10u);
}

} // namespace

OnDemandModel makeOnDemandModel(int32_t id, uint32_t modifier) {
    OnDemandModel m;
    m.id = id;
    m.modifier = modifier;
    m.size = ON_DEMAND_SIZE;
    uint32_t counter = onDemandCounter(id, modifier);
    for (auto &colour : m.colours) {
        colour[0] = colourChannel(counter + 2);
        colour[1] = colourChannel(counter + 1);
        colour[2] = colourChannel(counter);
        counter += 3;
    }
    m.counter = counter;
    m.shape = id >= 0 && id < ON_DEMAND_PLAIN_SHAPES ? id : ON_DEMAND_PLAIN_SHAPES;
    return m;
}

const OnDemandModel &onDemandModel(int32_t id, uint32_t modifier) {
    // Keyed by (id, modifier) as two signed integers, id first; entries are never removed.
    static std::map<std::pair<int32_t, int32_t>, OnDemandModel> cache;
    const auto key = std::make_pair(id, (int32_t)modifier);
    auto it = cache.find(key);
    if (it == cache.end())
        it = cache.emplace(key, makeOnDemandModel(id, modifier)).first;
    return it->second;
}

// --- the world's table ---------------------------------------------------------------

namespace {
ModelTable worldTable;
bool worldLoaded = false;
} // namespace

bool load(const std::filesystem::path &oracleDir, std::string *why) {
    worldLoaded = worldTable.load(oracleDir / "item-models.bin", why);
    return worldLoaded;
}

bool loaded() {
    return worldLoaded;
}

int modelId(const uint8_t *item) {
    return worldLoaded ? worldTable.modelNumber(fieldsOf(item)) : -1;
}

} // namespace nativeitems
