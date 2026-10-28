#include "NativePlayer.h"

#include "NativeRules.h"
#include "TalkGen.h"

#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace nativeplayer {

Item item(int category, int id, int material, int rarity, uint32_t modifier, int regionX, int regionY, int formula) {
    Item it{};
    it[0] = (uint8_t)category;
    const int32_t i = id, region[2] = {regionX, regionY}, f = formula;
    std::memcpy(&it[4], &i, 4);
    std::memcpy(&it[8], &modifier, 4);
    std::memcpy(&it[0xc], region, 8);
    it[0x14] = (uint8_t)rarity;
    std::memcpy(&it[0x18], &f, 4);
    it[0x1c] = (uint8_t)material;
    return it;
}

worldgen::CreatureRecord record(const Who &who, const std::array<Item, 11> &worn) {
    worldgen::CreatureRecord r = worldgen::newCreature(1);
    r[worldgen::creature::HOSTILITY] = 0;
    std::memcpy(&r[worldgen::creature::RACE], &who.race, 4);
    std::memcpy(&r[worldgen::creature::FLAGS], &who.flags2, 4);
    std::memcpy(&r[worldgen::creature::LEVEL], &who.level, 4);
    r[worldgen::creature::CLASS] = (uint8_t)who.cls;
    r[worldgen::creature::SPEC] = (uint8_t)who.spec;
    std::memcpy(&r[worldgen::creature::REGION], who.region, 8);
    for (size_t s = 0; s < worn.size(); s++)
        std::memcpy(&r[worldgen::creature::EQUIPMENT + s * 0xa0], worn[s].data(), 0xa0);
    return r;
}

Stats stats(const worldgen::CreatureRecord &r) {
    auto none = [](int) { return false; };
    Stats s;
    s.maxHp = nativerules::maxHealth(r.data());
    s.attack = nativerules::attackPower(r.data(), nullptr, 0);
    s.spell = nativerules::spellPower(r.data(), nullptr, 0);
    s.armor = nativerules::armor(r.data(), nullptr, 0);
    s.resistance = nativerules::resistance(r.data(), nullptr, 0);
    s.critical = nativerules::critChance(r.data(), none, nullptr, nullptr, 0, true);
    s.haste = nativerules::attackSpeed(r.data(), none);
    return s;
}

// --- The class-adapted treasure item (0x10a720, docs/specs/player-item-adaptation.md) -----

namespace {

namespace field {
constexpr size_t TYPE = 0x00, SUBTYPE = 0x04, MODIFIER = 0x08, REGION = 0x0c, RARITY = 0x14,
                 MATERIAL = 0x1c;
} // namespace field

constexpr uint8_t TREASURE = 30, WEAPON = 3, FIRST_ARMOUR = 4, AMULET = 8, RING = 9, COINS = 31;
constexpr uint8_t IRON = 1, WOOD = 2, GOLD = 11, SILVER = 12;

// Gold when the draw is odd, silver when it is even (rings, amulets, a mage's third weapon).
uint8_t preciousMetal(uint32_t draw) { return (draw & 1) == 0 ? SILVER : GOLD; }

// The armour materials by class (1 warrior … 4 rogue) and the draw m = 0, 1, 2.
constexpr uint8_t ARMOUR_MATERIAL[4][3] = {
    {1, 29, 4},   // warrior
    {27, 31, 33}, // ranger
    {26, 32, 35}, // mage
    {28, 30, 34}, // rogue
};

// A new item: all zero but its type, and the treasure's region, rarity and modifier.
Item adapted(const Item &treasure, uint8_t type) {
    Item out{};
    out[field::TYPE] = type;
    std::memcpy(&out[field::REGION], &treasure[field::REGION], 8);
    out[field::RARITY] = treasure[field::RARITY];
    std::memcpy(&out[field::MODIFIER], &treasure[field::MODIFIER], 4);
    return out;
}

void setSubtype(Item &it, uint32_t subtype) { std::memcpy(&it[field::SUBTYPE], &subtype, 4); }

} // namespace

Item transformed(const Item &in, int cls) {
    if (in[field::TYPE] != TREASURE)
        return in;
    uint32_t seed, subtype;
    std::memcpy(&seed, &in[field::MODIFIER], 4);
    std::memcpy(&subtype, &in[field::SUBTYPE], 4);
    using talkgen::xorshift;
    const uint32_t kind = xorshift(seed) % 7;

    // A bag of gold keeps only its rarity and modifier (no region).
    if (subtype == 1) {
        Item coins{};
        coins[field::TYPE] = COINS;
        coins[field::RARITY] = in[field::RARITY];
        std::memcpy(&coins[field::MODIFIER], &seed, 4);
        return coins;
    }
    // Kinds 5 and 6: a ring or an amulet, in gold or silver.
    if (kind == 5 || kind == 6) {
        Item jewel = adapted(in, kind == 5 ? RING : AMULET);
        jewel[field::MATERIAL] = preciousMetal(xorshift(seed + 1));
        return jewel;
    }
    // The rest depends on the class; a creature without one leaves the item as it was.
    if (cls < 1 || cls > 4)
        return in;

    if (kind == 0) { // a weapon of the class's three kinds
        Item weapon = adapted(in, WEAPON);
        const uint32_t which = xorshift(seed + 1) % 3, extra = xorshift(seed + 2);
        static constexpr uint32_t SUBTYPES[4][3] = {
            {0, 15, 13}, // warrior: a one-handed weapon, a two-handed one, a shield
            {6, 7, 8},   // ranger: bow, crossbow, boomerang
            {10, 11, 12},// mage: staff, wand, bracelet
            {3, 4, 5},   // rogue: dagger, fist weapon, longsword
        };
        uint32_t chosen = SUBTYPES[cls - 1][which];
        if (cls == 1 && which < 2)
            chosen += extra % 3; // the warrior's first two kinds come in three subtypes each
        setSubtype(weapon, chosen);
        uint8_t material = (cls == 1 || cls == 4) ? IRON : WOOD;
        if (cls == 3 && which == 2)
            material = preciousMetal(extra);
        weapon[field::MATERIAL] = material;
        return weapon;
    }
    // Kinds 1 … 4: one of the four armour pieces (chest, hands, feet, shoulders).
    Item armour = adapted(in, (uint8_t)(FIRST_ARMOUR + xorshift(seed + 1) % 4));
    armour[field::MATERIAL] = ARMOUR_MATERIAL[cls - 1][xorshift(seed + 2) % 3];
    return armour;
}

bool appearance(worldgen::CreatureRecord &r, const std::filesystem::path &snapshot) {
    std::ifstream in(snapshot);
    if (!in)
        return false;
    const size_t A = worldgen::creature::APPEARANCE;
    auto f = [&](size_t o, float v) { std::memcpy(&r[A + o], &v, 4); };
    auto s16 = [&](size_t o, int v) {
        const int16_t x = (int16_t)v;
        std::memcpy(&r[A + o], &x, 2);
    };
    // "key=value" pairs of a line after its first word.
    auto pairs = [](std::istringstream &l) {
        std::map<std::string, std::string> m;
        std::string w;
        while (l >> w)
            if (const size_t eq = w.find('='); eq != std::string::npos)
                m[w.substr(0, eq)] = w.substr(eq + 1);
        return m;
    };
    auto num = [](const std::map<std::string, std::string> &m, const char *k) {
        const auto it = m.find(k);
        return it == m.end() ? 0.0 : std::stod(it->second);
    };
    // The places (blocks of the model) and their offsets in the appearance.
    static const std::map<std::string, size_t> places = {{"chest_position", 0x68}, {"head_position", 0x74},
                                                         {"hands_position", 0x80}, {"feet_position", 0x8c},
                                                         {"unknown_position", 0x98}, {"wings_position", 0xa4},
                                                         {"hands_rotation", 0x50}};
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream l(line);
        std::string first;
        l >> first;
        const size_t eq = first.find('=');
        const std::string key = eq == std::string::npos ? first : first.substr(0, eq);
        if (key == "hair_color") {
            std::istringstream v(line.substr(eq + 1));
            int c[4] = {0, 0, 0, 0};
            v >> c[0] >> c[1] >> c[2] >> c[3];
            for (int k = 0; k < 4; k++)
                r[A + 2 + k] = (uint8_t)c[k];
        } else if (key == "flags2") {
            const uint32_t v = (uint32_t)std::stoul(first.substr(eq + 1), nullptr, 16);
            std::memcpy(&r[A + 8], &v, 4);
        } else if (key == "scales") {
            const auto m = pairs(l);
            f(0xc, (float)num(m, "graphics")), f(0x10, (float)num(m, "hitbox")), f(0x14, (float)num(m, "physics"));
        } else if (key == "models") {
            const auto m = pairs(l);
            const char *names[] = {"head", "hair", "hands", "feet", "chest", "tail", "shoulder", "wings"};
            for (int k = 0; k < 8; k++)
                s16(0x18 + 2 * k, (int)num(m, names[k]));
        } else if (key == "part_scales") {
            const auto m = pairs(l);
            const char *names[] = {"head", "chest", "hands", "feet", "unknown", "weapon", "tail", "shoulder", "wings"};
            for (int k = 0; k < 9; k++)
                f(0x28 + 4 * k, (float)num(m, names[k]));
        } else if (key == "rotations") {
            const auto m = pairs(l);
            f(0x4c, (float)num(m, "chest")), f(0x5c, (float)num(m, "feet")), f(0x60, (float)num(m, "wings"));
            f(0x64, (float)num(m, "unknown"));
        } else if (const auto it = places.find(key); it != places.end()) {
            std::istringstream v(line.substr(eq + 1));
            float x = 0, y = 0, z = 0;
            v >> x >> y >> z;
            f(it->second, x), f(it->second + 4, y), f(it->second + 8, z);
        }
    }
    return true;
}

} // namespace nativeplayer
