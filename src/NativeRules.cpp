// The game's stat rules, written from docs/specs/stat-rules.md (section numbers below are
// that document's). Arithmetic is single precision in the order the spec gives, since the
// recordings compare these values bit for bit.
#include "NativeRules.h"
#include "NativeCrt.h"

#include <cmath>
#include <cstring>
#include <initializer_list>
#include <utility>
#include <vector>

namespace nativerules {
namespace {

// --- Record layouts (§0.2, §0.3) ---------------------------------------------------------

// The creature record.
constexpr size_t HOSTILITY = 0x60;      // u8, 0 = player
constexpr size_t RACE = 0x64;           // i32
constexpr size_t ABILITY = 0x68;        // u8, the current ability
constexpr size_t ABILITY_TIME = 0x6c;   // f32, ms spent in it
constexpr size_t COMBO = 0x70;          // i32
constexpr size_t FLAGS = 0x80;          // u32
constexpr size_t MORE_FLAGS = 0x128;    // u8
constexpr size_t STUN_TIMER = 0x130;    // f32, stunned while above 0
constexpr size_t STAMINA_LOCK = 0x134;  // f32
constexpr size_t LEVEL = 0x144;         // i32
constexpr size_t CLASS = 0x14c;         // u8
constexpr size_t SPECIALISATION = 0x14d; // u8
constexpr size_t REGION = 0x150;        // two i32
constexpr size_t MANA = 0x188;          // f32, 0..1
constexpr size_t STEALTH = 0x18c;       // f32, 0..1
constexpr size_t STAMINA = 0x9a0;       // f32
constexpr size_t SKILL_POINTS = 0xb5c;  // seven i32, one per skill (§9)
constexpr size_t LONG_ABILITY_FACTOR = 0xba8; // f32, the length of ability 0x94 in seconds

// Equipment slots (§0.4), each counting only when it holds its own item type.
constexpr size_t NECK = 0x318, CHEST = 0x3b8, FEET = 0x458, HANDS = 0x4f8, SHOULDER = 0x598;
constexpr size_t WEAPON_1 = 0x638, WEAPON_2 = 0x6d8, RING_1 = 0x778, RING_2 = 0x818;

// The item record.
constexpr size_t ITEM_TYPE = 0x0;
constexpr size_t ITEM_SUBTYPE = 0x4;   // i32
constexpr size_t ITEM_MODIFIER = 0x8;  // u32, the item's seed
constexpr size_t ITEM_REGION = 0xc;    // two i32
constexpr size_t ITEM_RARITY = 0x14;   // u8
constexpr size_t WRAPPED_TYPE = 0x18;  // i32; a formula's item type in its low byte
constexpr size_t ITEM_MATERIAL = 0x1c; // u8
constexpr size_t ITEM_UPGRADES = 0x9d; // u8, the attribute count

// Item types.
constexpr uint8_t CONSUMABLE = 1, FORMULA = 2, WEAPON = 3, CHEST_ARMOR = 4, GLOVES = 5, BOOTS = 6,
                  SHOULDER_ARMOR = 7, AMULET = 8, RING = 9, TYPE_0B = 0x0b, TYPE_16 = 0x16,
                  SPECIAL_ITEM = 0x18, TYPE_1A = 0x1a, TYPE_1C = 0x1c;

struct Slot {
    size_t offset;
    uint8_t type;
};
// The nine-slot order (critical chance, haste, stamina factor).
constexpr Slot NINE_SLOTS[] = {{WEAPON_1, WEAPON},     {WEAPON_2, WEAPON},  {CHEST, CHEST_ARMOR},
                               {FEET, BOOTS},          {HANDS, GLOVES},     {SHOULDER, SHOULDER_ARMOR},
                               {NECK, AMULET},         {RING_1, RING},      {RING_2, RING}};
// The six-slot order of maximum health.
constexpr Slot HEALTH_SLOTS[] = {{WEAPON_1, WEAPON}, {WEAPON_2, WEAPON},  {CHEST, CHEST_ARMOR},
                                 {FEET, BOOTS},      {HANDS, GLOVES},     {SHOULDER, SHOULDER_ARMOR}};
// The armour order.
constexpr Slot ARMOR_SLOTS[] = {{CHEST, CHEST_ARMOR}, {FEET, BOOTS}, {HANDS, GLOVES}, {SHOULDER, SHOULDER_ARMOR}};

// Creature flags (+0x80) by bit; what they stand for isn't established (spec open point 2).
constexpr uint32_t FLAG_4 = 1u << 4;   // shortens the combo threshold
constexpr uint32_t FLAG_9 = 1u << 9;   // a strong multiplier (health x10)
constexpr uint32_t FLAG_15 = 1u << 15; // a side among hostility-1 creatures
constexpr uint32_t FLAG_17 = 1u << 17; // a mild multiplier (health x1.5)
constexpr uint32_t FLAG_18 = 1u << 18; // the strongest multiplier (health x15)
constexpr uint32_t FLAG_22 = 1u << 22; // never fights
constexpr uint32_t FLAG_26 = 1u << 26; // a small bonus on everything (x1.1)
constexpr uint8_t MORE_FLAG_5 = 1u << 5; // +0x128: fights anyone

// Buff types, named by what they do here.
constexpr int BUFF_FREE_SPELLS = 9;       // spells cost no mana
constexpr int BUFF_DOUBLE_HASTE = 0x0c;
constexpr int BUFF_QUICK_MOVES = 0x0d;    // no stamina costs, shorter moves
constexpr int BUFF_WEAKENED = 0x0e;       // armour and resistance a quarter
constexpr int BUFF_HEALTH = 0x16;
constexpr int BUFF_POWER = 0x17;
constexpr int BUFF_ARMOR = 0x18;
constexpr int BUFF_RESISTANCE = 0x19;
constexpr int BUFF_HEALTH_STACK = 0x20;   // every one counts
constexpr int BUFF_POWER_BONUS = 0x21;
constexpr int BUFF_MANA_FACTOR = 0x22;

// Abilities the rules test for by id.
constexpr int GUARD = 9;
constexpr int MANA_SHIELD = 0x1d;
constexpr int ABILITY_32 = 0x32;          // blocks starting anything else
constexpr int GUARD_STANCE = 0x3c, GUARD_STANCE_2 = 0x40;
constexpr int ABILITY_50 = 0x50;
constexpr int LONG_GUARD = 0x5f;
constexpr int ABILITY_60 = 0x60, ABILITY_64 = 0x64, ABILITY_67 = 0x67, ABILITY_6C = 0x6c;
constexpr int GUARD_6E = 0x6e;
constexpr int STUN_BREAKER = 0x71;        // may start while stunned
constexpr int STEALTH_STRIKE = 0x8c;      // critical bonus while it runs
constexpr int TIMED_ABILITY = 0x94;       // its middle part comes from the record

constexpr int PLAYER = 0;
constexpr float RAND_MAX_F = 32767.0f;

// --- Small helpers -----------------------------------------------------------------------

template <class T> T read(const uint8_t *p, size_t offset) {
    T v;
    std::memcpy(&v, p + offset, sizeof v);
    return v;
}

uint32_t flags(const uint8_t *c) { return read<uint32_t>(c, FLAGS); }
bool hasFlag(const uint8_t *c, uint32_t flag) { return (flags(c) & flag) != 0; }
bool isPlayer(const uint8_t *c) { return c[HOSTILITY] == PLAYER; }
bool isClass(const uint8_t *c, int cls, int specialisation) { return c[CLASS] == cls && c[SPECIALISATION] == specialisation; }
bool has(const HasBuff &hasBuff, int type) { return hasBuff && hasBuff(type); }

uint8_t itemType(const uint8_t *item) { return item[ITEM_TYPE]; }
int32_t subtype(const uint8_t *item) { return read<int32_t>(item, ITEM_SUBTYPE); }
uint32_t modifier(const uint8_t *item) { return read<uint32_t>(item, ITEM_MODIFIER); }
uint8_t material(const uint8_t *item) { return item[ITEM_MATERIAL]; }
uint8_t rarity(const uint8_t *item) { return item[ITEM_RARITY]; }

// Two-handed weapons (§0.3): subtypes 5-8, 10, 11, 15-18 and 23.
bool isTwoHanded(const uint8_t *item) {
    constexpr uint32_t TWO_HANDED_SUBTYPES = 0x878de0;
    const uint32_t s = (uint32_t)subtype(item);
    return itemType(item) == WEAPON && s <= 23 && ((TWO_HANDED_SUBTYPES >> s) & 1);
}
bool isEquipment(uint8_t type) { return (type >= WEAPON && type <= RING) || type == TYPE_1A; }

// A slot counts when it holds its own item type; "usable" also asks that the creature may use it.
bool counts(const uint8_t *c, const Slot &s) { return c[s.offset] == s.type; }
bool usableIn(const uint8_t *c, const Slot &s) { return counts(c, s) && mayUse(c, c + s.offset); }

// A float to int toward zero as the processor does it: NaN or out of range gives INT32_MIN.
int truncToInt(float x) {
    if (!(x >= -2147483648.0f && x < 2147483648.0f))
        return INT32_MIN;
    return (int)x;
}

// The item seed's scramble (§0.1).
uint32_t xorshift(uint32_t v) {
    v ^= v << 13;
    v ^= v >> 17;
    v ^= v << 5;
    return v;
}

// The roll in [1, 1.25) behind health, armour, resistance and stamina regeneration (§2).
float statRoll(const uint8_t *item, uint32_t salt) {
    const uint32_t v = xorshift(modifier(item) + salt);
    return (float)(v % 100000u) * 0.25f / 100000.0f + 1.0f;
}

// Doubles the factor for a chest piece, and again for chest subtype 1.
float chestFactor(const uint8_t *item, float f) {
    if (itemType(item) == CHEST_ARMOR) {
        f = f + f;
        if (subtype(item) == 1)
            f = f + f;
    }
    return f;
}

// "Renowned" items count fully in the regions around home too (§1.2, §2.7).
bool isRenowned(const uint8_t *item) {
    const uint32_t every = ((uint32_t)rarity(item) + 1u) * 10u;
    return isEquipment(itemType(item)) && modifier(item) % every == 0;
}

int32_t wrappedDifference(int32_t a, int32_t b) { return (int32_t)((uint32_t)a - (uint32_t)b); }
int32_t wrappedAbs(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }

const Buff *firstBuff(const Buff *buffs, size_t count, int type) {
    for (size_t i = 0; i < count; i++)
        if (buffs[i].type == type)
            return &buffs[i];
    return nullptr;
}
bool hasBuffIn(const Buff *buffs, size_t count, int type) { return firstBuff(buffs, count, type) != nullptr; }

// --- Mod hooks ---------------------------------------------------------------------------

std::vector<std::pair<int, Callbacks>> &hooks() {
    static std::vector<std::pair<int, Callbacks>> list;
    return list;
}
int lastHandle = 0;

template <class Hook> void runHooks(Hook Callbacks::*hook, const uint8_t *creature, float &value) {
    for (auto &[handle, callbacks] : hooks())
        if (callbacks.*hook)
            (callbacks.*hook)(creature, value);
}

// --- Weapon lists (§3) -------------------------------------------------------------------

struct WeaponList {
    const uint8_t *items[2] = {};
    size_t count = 0;
    void add(const uint8_t *item) { items[count++] = item; }
};

const Slot W1{WEAPON_1, WEAPON}, W2{WEAPON_2, WEAPON};

// The weapons the current ability strikes with (0x4fd40).
WeaponList weaponsInUse(const uint8_t *c) {
    WeaponList list;
    auto isBow = [&](const Slot &s) { // a usable weapon of subtype 6 or 7
        const int32_t t = subtype(c + s.offset);
        return usableIn(c, s) && (t == 6 || t == 7);
    };
    if (isBow(W2)) {
        list.add(c + WEAPON_2);
        return list;
    }
    if (isBow(W1)) {
        list.add(c + WEAPON_1);
        return list;
    }
    switch (c[ABILITY]) {
    case 3: case 5: case 8: case 9: case 0x0b: case 0x13: case 0x28: case 0x2a: // the first hand's moves
        if (usableIn(c, W1))
            list.add(c + WEAPON_1);
        break;
    case 0x0c: case 0x0d: case 0x11: case 0x12: case 0x15: case 0x37: case 0x5f: case 0x64: case 0x6c:
    case 0x8c: case 0x94: case 0x9e: // both hands
        if (usableIn(c, W1))
            list.add(c + WEAPON_1);
        if (usableIn(c, W2))
            list.add(c + WEAPON_2);
        break;
    case 0x29: case 0x2b: // the second hand's moves
        if (usableIn(c, W2))
            list.add(c + WEAPON_2);
        break;
    default: { // the second hand, and a first-hand weapon of subtype 12
        if (usableIn(c, W2))
            list.add(c + WEAPON_2);
        const uint8_t *first = c + WEAPON_1;
        if (itemType(first) == WEAPON && subtype(first) == 12 && mayUse(c, first))
            list.add(first);
        break;
    }
    }
    return list;
}

// Both hands, whatever the ability (0x52ce0).
WeaponList bothWeapons(const uint8_t *c) {
    WeaponList list;
    if (counts(c, W2))
        list.add(c + WEAPON_2);
    if (counts(c, W1))
        list.add(c + WEAPON_1);
    return list;
}

WeaponList weaponList(const uint8_t *c, Weapons which) { return which == Weapons::Both ? bothWeapons(c) : weaponsInUse(c); }

// 5 plus the damage of each listed weapon, in list order (§4.1 step 1, §4.2 step 1).
float weaponSum(const uint8_t *c, Weapons which) {
    const WeaponList list = weaponList(c, which);
    float s = 5.0f;
    for (size_t i = 0; i < list.count; i++)
        s = s + weaponDamage(list.items[i], c);
    return s;
}

// Weapon damage for both 0x10ad90 (with upgrades) and 0x108f20 (without) (§2.1).
float damageOf(const uint8_t *item, const uint8_t *creature, bool withUpgrades) {
    if (itemType(item) != WEAPON)
        return 0.0f;
    const float power = itemPower(item, creature, withUpgrades ? item[ITEM_UPGRADES] : 0);
    switch (subtype(item)) {
    case 3: case 4:
        return power + power;
    case 5: case 13:
        return power * 4.0f;
    case 11:
        if (withUpgrades && !isPlayer(creature))
            return power * 6.0f;
        return power * 8.0f;
    default:
        return isTwoHanded(item) ? power * 8.0f : power * 4.0f;
    }
}

// Item critical chance and haste share one shape (§2.4).
float critOrHaste(const uint8_t *item, const uint8_t *creature, bool critical) {
    const uint8_t type = itemType(item);
    if (type < WEAPON || type > RING)
        return 0.0f;
    float a = (isTwoHanded(item) || type == CHEST_ARMOR || type == AMULET || type == RING) ? 0.02f : 0.01f;
    if (type == CHEST_ARMOR && subtype(item) == 1)
        a = a + a;
    const float q = (float)(modifier(item) % 21u) / 20.0f;
    float b = critical ? 1.0f - q : q;
    switch (material(item)) {
    case 4: case 0x21: case 0x22: case 0x23:
        if (!critical)
            b = b * 0.9f;
        break;
    case 6:
        a = a * 0.5f;
        break;
    case 0x0b:
        if (critical)
            b = b + 1.0f;
        break;
    case 0x0c:
        if (!critical)
            b = b + 1.0f;
        break;
    case 0x1d:
        if (!critical)
            b = b + 0.5f;
        break;
    default:
        break;
    }
    const float power = itemPower(item, creature);
    const float v = power * 0.0625f * a * b * 3.0f;
    return v >= 0.001f ? v : 0.0f;
}

// The armour sum shared by armour and resistance (§5.2).
float defenceOf(const uint8_t *c, const Buff *buffs, size_t count, float (*itemValue)(const uint8_t *, const uint8_t *),
                int ownBuff) {
    float a = 0.0f;
    for (const Slot &s : ARMOR_SLOTS)
        if (usableIn(c, s))
            a = a + itemValue(c + s.offset, c);
    if (hasFlag(c, FLAG_9) || hasFlag(c, FLAG_17))
        a = a * 1.2f;
    if (hasFlag(c, FLAG_18))
        a = a + a;
    if (hasBuffIn(buffs, count, BUFF_WEAKENED))
        a = a * 0.25f;
    if (const Buff *b = firstBuff(buffs, count, ownBuff))
        a = a * b->value;
    return a;
}

// --- Ability durations (§7.1) ------------------------------------------------------------

// One part of a duration: a fixed time, or a time divided by the haste factor.
struct Part {
    float ms = 0.0f;
    bool hasted = false;
};
Part fixed(float ms) { return {ms, false}; }
Part hasted(float ms) { return {ms, true}; }

int partValue(const uint8_t *c, Part p, const HasBuff &hasBuff) {
    if (!p.hasted)
        return (int)p.ms;
    return truncToInt(p.ms / attackSpeed(c, hasBuff));
}

int abilityOf(const uint8_t *c, int ability) { return ability < 0 ? c[ABILITY] : ability; }

Part effectPart(const uint8_t *c, int ability, bool quick) {
    switch (ability) {
    case 0: case 1: case 9: case 0x0c: case 0x34: case 0x38: case 0x3d: case 0x3e: case 0x3f: case 0x50: case 0x51:
    case 0x52: case 0x5f: case 0x6e: case 0x74: case 0x76: case 0x78: case 0x79: case 0x7a: case 0x92: case 0x95:
    case 0x9b: case 0x9e: case 0x9f:
        return fixed(0);
    case 2: case 0x0a: case 0x1b: case 0x45: case 0x46: case 0x47: case 0x4a: case 0x4b: case 0x4c: case 0x4d:
    case 0x4e: case 0x4f:
        return hasted(300);
    case 3: case 4: case 5: case 6: case 7: case 8: case 0x0b: case 0x0f: case 0x12: case 0x13: case 0x14: case 0x15:
    case 0x16:
        return hasted(100);
    case 0x18: case 0x19: case 0x1a: case 0x1c: case 0x25: case 0x3c: case 0x40: case 0x41: case 0x93:
        return hasted(50);
    case 0x1d: case 0x32: case 0x71: case 0x8c: case 0x8e: case 0x8f:
        return fixed(100);
    case 0x1f: case 0x21: case 0x3a: case 0x3b: case 0x67:
        return hasted(800);
    case 0x20: case 0x22: case 0x23:
        return hasted(1600);
    case 0x26: case 0x2c: case 0x62: // quicker with a subtype-12 weapon in the second hand (no type test)
        return hasted(read<int32_t>(c, WEAPON_2 + ITEM_SUBTYPE) == 12 ? 600.0f : 1200.0f);
    case 0x27: case 0x2d: case 0x42: case 0x43: case 0x68: case 0x6b:
        return hasted(500);
    case 0x2e: case 0x30:
        return hasted(1200);
    case 0x37:
        return fixed(400);
    case 0x48: case 0x49:
        return fixed(200);
    case 0x60:
        return hasted(5000);
    case 0x64: case 0x69: case 0x6a:
        return hasted(1000);
    case 0x6c:
        return quick ? fixed(100) : hasted(200);
    case 0x0d: case 0x0e: case 0x10: case 0x11: case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x44: case 0x94:
        return hasted(200);
    case 0x75: case 0xa1:
        return fixed(3000);
    case 0x85:
        return fixed(60000);
    case 0x86: case 0x87: case 0x88:
        return hasted(2000);
    case 0x89: case 0xa2: case 0xa3:
        return fixed(5000);
    case 0x8b:
        return fixed(10);
    case 0x90:
        return hasted(1500);
    case 0x98:
        return fixed(120000);
    case 0x9a:
        return fixed(2000);
    default:
        return hasted(400);
    }
}

Part middlePart(int ability, bool quick) {
    switch (ability) {
    case 0: case 1: case 9: case 0x19: case 0x1a: case 0x1c: case 0x33: case 0x3c: case 0x40: case 0x50: case 0x51:
    case 0x52: case 0x76: case 0x78: case 0x79: case 0x7a: case 0x86: case 0x87: case 0x88: case 0x89: case 0x90:
    case 0x92: case 0x9a: case 0xa1: case 0xa2: case 0xa3:
        return fixed(0);
    case 2: case 3: case 0x0a: case 0x13: case 0x14: case 0x44:
        return hasted(200);
    case 4: case 5: case 0x26: case 0x2c: case 0x3f:
        return hasted(100);
    case 6: case 0x0d: case 0x11: case 0x12: case 0x15: case 0x16: case 0x42: case 0x43:
        return hasted(400);
    case 7: case 8:
        return hasted(150);
    case 0x0b:
        return fixed(200);
    case 0x0e: case 0x0f: case 0x10:
        return hasted(250);
    case 0x17: case 0x18:
        return hasted(50);
    case 0x1b: case 0x23:
        return hasted(1200);
    case 0x1d: case 0x69: case 0x6a:
        return fixed(2000);
    case 0x1f: case 0x21: case 0x4a: case 0x67:
        return fixed(600);
    case 0x20: case 0x22:
        return fixed(1200);
    case 0x2e: case 0x30: case 0x38: case 0x68: case 0x6b:
        return hasted(500);
    case 0x2f:
        return hasted(1000);
    case 0x34: case 0x6c:
        return fixed(quick ? 250.0f : 500.0f);
    case 0x4d: case 0x4e: case 0x4f:
        return fixed(500);
    case 0x37:
        return fixed(100);
    case 0x45: case 0x46: case 0x9b: case 0x9f:
        return fixed(1000);
    case 0x48: case 0x49: case 0x82:
        return fixed(3000);
    case 0x5f:
        return fixed(20000);
    case 0x64:
        return fixed(6000);
    case 0x8b:
        return fixed(5000);
    case 0x8c:
        return hasted(5000);
    default:
        return hasted(300);
    }
}

Part recoveryPart(int ability, bool quick) {
    switch (ability) {
    case 1: case 9: case 0x19: case 0x1a: case 0x1c: case 0x1d: case 0x32: case 0x3c: case 0x40: case 0x50: case 0x51:
    case 0x52: case 0x5f: case 0x76: case 0x78: case 0x79: case 0x7a: case 0x89: case 0x92: case 0x9a: case 0x9b:
    case 0x9f: case 0xa1: case 0xa2: case 0xa3:
        return fixed(0);
    case 0: case 0x34:
        return fixed(100);
    case 4: case 5: case 6: case 0x3a: case 0x3b: case 0x3f:
        return hasted(300);
    case 8: case 0x0f: case 0x13: case 0x42: case 0x43:
        return hasted(200);
    case 0x0b:
        return fixed(600);
    case 0x0c: case 0x17: case 0x1b: case 0x1f: case 0x20: case 0x21: case 0x22: case 0x23: case 0x26: case 0x27:
    case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x30: case 0x3d: case 0x3e:
    case 0x44: case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x74: case 0x86: case 0x87: case 0x88: case 0x90:
    case 0x9e:
        return hasted(100);
    case 0x10:
        return hasted(400);
    case 0x18:
        return hasted(10);
    case 0x37:
        return fixed(400);
    case 0x45: case 0x46: case 0x67:
        return hasted(800);
    case 0x48: case 0x49:
        return fixed(200);
    case 0x6c:
        return fixed(quick ? 50.0f : 100.0f);
    case 0x8b:
        return fixed(10);
    default:
        return hasted(500);
    }
}

// Whether the current ability is still running (its duration beyond the time spent in it).
bool abilityRunning(const uint8_t *c, const HasBuff &hasBuff) {
    return (float)abilityDuration(c, hasBuff) > read<float>(c, ABILITY_TIME);
}

} // namespace

// --- §1 The item's own level and power ---------------------------------------------------

int itemLevel(const uint8_t *item, const int32_t region[2]) {
    const int32_t x = read<int32_t>(item, ITEM_REGION), y = read<int32_t>(item, ITEM_REGION + 4);
    if (isRenowned(item)) {
        const int32_t dx = wrappedAbs(wrappedDifference(region[0], x)), dy = wrappedAbs(wrappedDifference(region[1], y));
        if ((dx > dy ? dx : dy) < 2)
            return rarity(item);
    }
    return region[0] == x && region[1] == y ? rarity(item) : -1;
}

namespace {
// Who may use a type and material/subtype; the formula case unwraps to its item type.
bool classMayUseAs(uint8_t type, const uint8_t *item, int cls) {
    if (type == WEAPON) {
        const uint32_t s = (uint32_t)subtype(item);
        switch (s) {
        case 3: case 4: case 5: return cls == 4;              // rogue weapons
        case 6: case 7: case 8: case 14: return cls == 2;     // ranger weapons
        case 9: return true;
        case 10: case 11: case 12: return cls == 3;           // mage weapons
        case 0: case 1: case 2: case 13: case 15: case 16: case 17: return cls == 1; // warrior weapons
        default: return true;
        }
    }
    if (type >= CHEST_ARMOR && type <= SHOULDER_ARMOR) {
        switch (material(item)) {
        case 1: case 4: case 5: case 11: case 19: case 23: case 29: return cls == 1;
        case 20: case 27: case 31: case 33: return cls == 2;
        case 24: case 26: case 32: case 35: return cls == 3;
        case 28: case 30: case 34: return cls == 4;
        default: return true;
        }
    }
    return true;
}
} // namespace

bool classMayUse(const uint8_t *item, int cls) {
    if (itemType(item) == FORMULA) {
        const uint8_t wrapped = item[WRAPPED_TYPE]; // the low byte of +0x18
        return wrapped == FORMULA || classMayUseAs(wrapped, item, cls);
    }
    return classMayUseAs(itemType(item), item, cls);
}

bool mayUse(const uint8_t *creature, const uint8_t *item) {
    return !isPlayer(creature) || classMayUse(item, creature[CLASS]);
}

float itemPower(const uint8_t *item, const uint8_t *creature, int upgrades) {
    // Players and hostility kinds 4 and 6 judge items by their own region; the rest by the
    // item's, so it is always at home for them (§1.1).
    int32_t region[2] = {read<int32_t>(item, ITEM_REGION), read<int32_t>(item, ITEM_REGION + 4)};
    if (creature) {
        const uint8_t kind = creature[HOSTILITY];
        if (kind == 0 || kind == 4 || kind == 6)
            region[0] = read<int32_t>(creature, REGION), region[1] = read<int32_t>(creature, REGION + 4);
    }
    const int level = itemLevel(item, region);
    const uint32_t seedPart = xorshift(modifier(item)) % 1000u;
    const float divisor = isTwoHanded(item) ? 32.0f : 16.0f;
    const float upgradePart = (float)upgrades * 0.5f / divisor;
    const float levelPart = (float)level + (float)seedPart / 2000.0f;
    return nativecrt::powf(2.0f, upgradePart + levelPart);
}

// --- §2 Per-item stats -------------------------------------------------------------------

float weaponDamage(const uint8_t *item, const uint8_t *creature) { return damageOf(item, creature, true); }
float baseWeaponDamage(const uint8_t *item, const uint8_t *creature) { return damageOf(item, creature, false); }

float itemHealth(const uint8_t *item, const uint8_t *creature) {
    const uint8_t type = itemType(item);
    if (type < WEAPON || type > SHOULDER_ARMOR)
        return 0.0f;
    float f = chestFactor(item, statRoll(item, 0x2673) * 10.0f);
    if (isTwoHanded(item))
        f = f + f;
    float m = 1.0f;
    switch (material(item)) {
    case 1: m = 2.0f; break;
    case 0x1b: m = 1.5f; break;
    case 0x1c: m = 1.75f; break;
    default: break;
    }
    return itemPower(item, creature) * f * m;
}

float itemArmor(const uint8_t *item, const uint8_t *creature) {
    const uint8_t type = itemType(item);
    if (type < CHEST_ARMOR || type > SHOULDER_ARMOR)
        return 0.0f;
    float f = chestFactor(item, statRoll(item, 0x18008));
    switch (material(item)) { // materials not listed leave it alone
    case 1: f = f * 1.2f; break;
    case 4: f = f * 1.25f; break;
    case 6: f = f * 0.25f; break;
    case 0x0b: f = f * 1.125f; break;
    case 0x13: case 0x14: f = f * 0.8f; break;
    case 0x18: case 0x1a: case 0x20: f = f * 0.5f; break;
    case 0x1b: case 0x1c: case 0x1e: case 0x1f: f = f * 0.8f; break;
    case 0x1d: f = f * 1.15f; break;
    case 0x21: f = f * 0.9f; break;
    case 0x23: f = f * 0.6f; break;
    default: break;
    }
    return itemPower(item, creature) * f;
}

float itemResistance(const uint8_t *item, const uint8_t *creature) {
    const uint8_t type = itemType(item);
    if (type < CHEST_ARMOR || type > SHOULDER_ARMOR)
        return 0.0f;
    float f = chestFactor(item, statRoll(item, 0x1a6d));
    switch (material(item)) {
    case 1: case 4: case 0x14: case 0x1d: f = f * 0.85f; break;
    case 6: f = f * 0.25f; break;
    case 0x1b: case 0x1c: case 0x1e: case 0x1f: case 0x21: case 0x22: f = f * 0.75f; break;
    default: break;
    }
    return itemPower(item, creature) * f;
}

float itemCritical(const uint8_t *item, const uint8_t *creature) { return critOrHaste(item, creature, true); }
float itemHaste(const uint8_t *item, const uint8_t *creature) { return critOrHaste(item, creature, false); }

float itemStaminaRegen(const uint8_t *item, const uint8_t *creature) {
    const uint8_t type = itemType(item);
    if (!((type >= CHEST_ARMOR && type <= RING) || type == TYPE_1A))
        return 0.0f;
    float f = statRoll(item, 0x157) * 0.01f;
    if (type == CHEST_ARMOR)
        f = f + f;
    float g = (float)((modifier(item) * 8u) % 21u) / 20.0f;
    switch (material(item)) {
    case 0x1b: case 0x1f: g = g + 0.5f; break;
    case 0x1c: case 0x1e: g = g + 1.0f; break;
    case 0x1d: g = g + 0.1f; break;
    case 0x21: g = g + 0.4f; break;
    case 0x22: g = g + 0.75f; break;
    default: break;
    }
    const float t = itemPower(item, creature) * f * g;
    const float v = (float)((double)t * 1.5); // formed in double, rounded once
    return v >= 0.001f ? v : 0.0f;
}

float healAmount(const uint8_t *item) {
    if (itemType(item) != CONSUMABLE)
        return 0.0f;
    switch (subtype(item)) {
    case 1: case 2: case 4: case 5: case 6: case 7: case 8: case 12: return 200.0f;
    case 10: case 11: return 100.0f;
    default: return 0.0f;
    }
}

int useTime(const uint8_t *item) {
    if (itemType(item) == TYPE_1C)
        return 500;
    if (itemType(item) != CONSUMABLE)
        return 0;
    float base;
    switch (subtype(item)) {
    case 3: return 500;
    case 1: case 12: base = 5000.0f; break;
    case 2: base = 6000.0f; break;
    default: base = 10000.0f; break;
    }
    // Rarer consumables work faster: an eighth less per rarity step.
    const float k = (float)rarity(item) * 0.5f * 0.25f;
    return truncToInt((1.5f - k) * base);
}

float demonDamage(const uint8_t *item) {
    return itemType(item) == SPECIAL_ITEM && subtype(item) == 12 ? 1.0f : 0.0f;
}

int itemPrice(const uint8_t *item) {
    const uint8_t type = itemType(item);
    float base;
    switch (type) {
    case WEAPON: base = 5.0f; break;
    case CHEST_ARMOR: base = 4.0f; break;
    case GLOVES: case BOOTS: case RING: base = 2.0f; break;
    case SHOULDER_ARMOR: case AMULET: base = 3.0f; break;
    case TYPE_0B: base = subtype(item) >= 0x20 && subtype(item) <= 0x23 ? 10.0f : 0.2f; break;
    case TYPE_16: base = 50.0f; break;
    default: base = 0.2f; break;
    }
    if (isTwoHanded(item))
        base = base + base;
    float f = base * 5.0f;
    if (isRenowned(item))
        f = f + f;
    const int price = truncToInt(nativecrt::powf(2.0f, (float)rarity(item)) * f);
    return price > 1 ? price : 1;
}

int sellPrice(int price) {
    const float half = std::sqrt((float)price) * 0.5f;
    return truncToInt(half > 1.0f ? half : 1.0f);
}

// --- Mod hooks ---------------------------------------------------------------------------

int addCallbacks(Callbacks callbacks) {
    hooks().emplace_back(++lastHandle, std::move(callbacks));
    return lastHandle;
}

void removeCallbacks(int handle) {
    auto &list = hooks();
    for (auto it = list.begin(); it != list.end(); ++it)
        if (it->first == handle) {
            list.erase(it);
            return;
        }
}

// --- §4 Offence --------------------------------------------------------------------------

float attackPower(const uint8_t *c, const Buff *buffs, size_t count, Weapons weapons) {
    float s = weaponSum(c, weapons);
    s = (read<float>(c, STEALTH) * 0.2f + 1.0f) * s;
    switch (c[ABILITY]) {
    case ABILITY_60: s = s * 50.0f; break;
    case ABILITY_64: s = s + s; break;
    case ABILITY_67: s = s * 5.0f; break;
    case ABILITY_6C: s = s * 3.0f; break;
    default: break;
    }
    if (hasFlag(c, FLAG_9))
        s = s * 1.5f;
    if (hasFlag(c, FLAG_17))
        s = s * 1.2f;
    if (hasFlag(c, FLAG_18))
        s = s + s;
    if (const Buff *b = firstBuff(buffs, count, BUFF_POWER))
        s = s * b->value;
    if (const Buff *b = firstBuff(buffs, count, BUFF_POWER_BONUS))
        s = s * (b->value * 0.1f + 1.0f);
    if (hasFlag(c, FLAG_26))
        s = s * 1.1f;
    runHooks(&Callbacks::attackPower, c, s);
    return s;
}

float spellPower(const uint8_t *c, const Buff *buffs, size_t count, Weapons weapons) {
    float s = weaponSum(c, weapons);
    if (hasFlag(c, FLAG_9))
        s = s * 1.5f;
    if (hasFlag(c, FLAG_17))
        s = s * 1.2f;
    if (hasFlag(c, FLAG_18))
        s = s * 4.0f;
    if (const Buff *b = firstBuff(buffs, count, BUFF_POWER))
        s = s * b->value;
    if (hasFlag(c, FLAG_26))
        s = s * 1.1f;
    runHooks(&Callbacks::spellPower, c, s);
    return s;
}

float abilityDamageFactor(int ability) {
    switch (ability) {
    case 6: case 0x1b: return 0.5f;
    case 0x16: return 0.1f;
    case 0x1f: case 0x21: return 0.4f;
    case 0x20: case 0x22: return 0.6f;
    case 0x26: case 0x2c: case 0x3a: case 0x3b: case 0x45: case 0xa0: return 2.0f;
    case 0x2f: case 0x69: case 0x6a: return 0.75f;
    case 0x35: return 3.0f;
    case 0x38: return 0.25f;
    case 0x49: return 0.2f;
    default: return 1.0f;
    }
}

float critChance(const uint8_t *c, const HasBuff &hasBuff, const uint8_t *target, const Buff *targetBuffs,
                 size_t targetBuffCount, bool magic) {
    float chance = read<float>(c, STEALTH) * 0.5f + 0.0f;
    if (c[ABILITY] == STEALTH_STRIKE && abilityRunning(c, hasBuff))
        chance = chance + 0.5f;
    for (const Slot &s : NINE_SLOTS)
        if (counts(c, s))
            chance = chance + itemCritical(c + s.offset, c);
    if (target)
        chance = chance - (magic ? magicCritDefence(target, targetBuffs, targetBuffCount)
                                 : critDefence(target, targetBuffs, targetBuffCount));
    return chance;
}

float attackSpeed(const uint8_t *c, const HasBuff &hasBuff) {
    // The combo's share of the threshold, at most 1 (a NaN counts as full).
    auto comboShare = [&] {
        const float q = (float)read<int32_t>(c, COMBO) / (float)comboThreshold(c);
        return q < 1.0f ? q : 1.0f;
    };
    float s = 1.0f;
    if (isClass(c, 1, 0))
        s = comboShare() * 0.5f + 1.0f;
    else if (isClass(c, 3, 1))
        s = s + comboShare();
    else if (isClass(c, 2, 0))
        s = s + comboShare() * 0.5f;
    for (const Slot &slot : NINE_SLOTS)
        if (counts(c, slot))
            s = s + itemHaste(c + slot.offset, c);
    if (has(hasBuff, BUFF_DOUBLE_HASTE))
        s = s + s;
    if (hasFlag(c, FLAG_9))
        s = s * 0.75f;
    if (hasFlag(c, FLAG_18))
        s = s * 0.6f;
    if (hasFlag(c, FLAG_26))
        s = s * 1.1f;
    return s;
}

int comboThreshold(const uint8_t *c) {
    const uint8_t *second = c + WEAPON_2;
    if (itemType(second) == WEAPON) {
        switch (subtype(second)) {
        case 3: case 4: case 10: return 50;
        case 6: return 60;
        case 8: return 80;
        case 11: case 12: return 20;
        case 15: case 16: case 17: return 15;
        default: return 30;
        }
    }
    return hasFlag(c, FLAG_4) ? 15 : 30;
}

float procChance(const uint8_t *c, float chance) {
    const uint8_t *second = c + WEAPON_2;
    if (itemType(second) == WEAPON) {
        switch (subtype(second)) {
        case 0: case 1: case 2: case 12: return chance * 0.5f;
        case 8: case 10: return chance * 0.3f;
        default: break;
        }
    }
    return chance;
}

bool procHappens(const uint8_t *c, float chance, int rand) { return (float)rand / RAND_MAX_F < procChance(c, chance); }

// --- §5 Defence --------------------------------------------------------------------------

float maxHealth(const uint8_t *c, const Buff *buffs, size_t count) {
    float h = isClass(c, 1, 1) ? 125.0f : 100.0f;
    for (const Slot &s : HEALTH_SLOTS)
        if (counts(c, s))
            h = h + itemHealth(c + s.offset, c);
    if (hasFlag(c, FLAG_9))
        h = h * 10.0f;
    if (hasFlag(c, FLAG_17))
        h = h * 1.5f;
    if (hasFlag(c, FLAG_18))
        h = h * 15.0f;
    if (const Buff *b = firstBuff(buffs, count, BUFF_HEALTH))
        h = h * b->value;
    for (size_t i = 0; i < count; i++)
        if (buffs[i].type == BUFF_HEALTH_STACK)
            h = h * buffs[i].value;
    if (hasFlag(c, FLAG_26))
        h = h * 1.1f;
    runHooks(&Callbacks::maxHealth, c, h);
    return h;
}

float armor(const uint8_t *c, const Buff *buffs, size_t count) {
    float a = defenceOf(c, buffs, count, itemArmor, BUFF_ARMOR);
    runHooks(&Callbacks::armor, c, a);
    return a;
}

float resistance(const uint8_t *c, const Buff *buffs, size_t count) {
    float a = defenceOf(c, buffs, count, itemResistance, BUFF_RESISTANCE);
    runHooks(&Callbacks::resistance, c, a);
    return a;
}

float blockValue(const uint8_t *c) {
    const uint8_t *first = c + WEAPON_1, *second = c + WEAPON_2;
    float b = 5.0f;
    if (itemType(first) == WEAPON && subtype(first) == 13 && mayUse(c, first)) { // a shield
        const float factor = isClass(c, 1, 1) ? 6.0f : 4.0f;
        b = weaponDamage(second, c) * factor + 5.0f;
    } else {
        if (usableIn(c, W1))
            b = weaponDamage(first, c) + b;
        if (usableIn(c, W2))
            b = b + weaponDamage(second, c);
    }
    if (hasFlag(c, FLAG_9))
        b = b + b;
    if (hasFlag(c, FLAG_17))
        b = b * 1.2f;
    if (hasFlag(c, FLAG_18))
        b = b * 4.0f;
    if (hasFlag(c, FLAG_26))
        b = b * 1.1f;
    return b;
}

namespace {
// A sixteenth of a defence value, times 0.05, quartered again by the weakening buff (§5.4).
float critDefenceFrom(float defence, const Buff *buffs, size_t count) {
    float d = defence / nativecrt::powf(2.0f, 4.0f) * 0.05f;
    if (hasBuffIn(buffs, count, BUFF_WEAKENED))
        d = d * 0.25f;
    return d;
}
} // namespace

float critDefence(const uint8_t *target, const Buff *buffs, size_t count) {
    return critDefenceFrom(armor(target, buffs, count), buffs, count);
}

float magicCritDefence(const uint8_t *target, const Buff *buffs, size_t count) {
    return critDefenceFrom(resistance(target, buffs, count), buffs, count);
}

// --- §6 Mana and stamina -----------------------------------------------------------------

float manaFactor(const Buff *buffs, size_t count) {
    const Buff *b = firstBuff(buffs, count, BUFF_MANA_FACTOR);
    return b ? 1.0f - b->value * 0.1f : 1.0f;
}

float staminaFactor(const uint8_t *c) {
    float f = 1.0f;
    for (const Slot &s : NINE_SLOTS)
        if (counts(c, s))
            f = f + itemStaminaRegen(c + s.offset, c);
    return f;
}

float manaCost(int ability, const HasBuff &hasBuff) {
    switch (ability) {
    case 0x20: case 0x22: case 0x26: case 0x2c: case 0x2e: case 0x30: case 0x69: case 0x6a:
        return has(hasBuff, BUFF_FREE_SPELLS) ? 0.0f : 0.3f;
    case 0x23: return 0.0625f;
    case 0x33: return 1.0f;
    case 0x61: return 0.5f;
    case 0x86: case 0x87: case 0x88: return 0.03125f;
    default: return 0.0f;
    }
}

float staminaCost(int ability, const HasBuff &hasBuff) {
    if (has(hasBuff, BUFF_QUICK_MOVES))
        return 0.0f;
    switch (ability) {
    case 0x16: return 0.5f;
    case 0x32: case 0x52: case 0x6c: return 0.25f;
    case 0x34: case 0x37: case 0xa0: return 1.0f;
    default: return 0.0f;
    }
}

// --- §7 Durations ------------------------------------------------------------------------

int abilityEffectTime(const uint8_t *c, int ability, const HasBuff &hasBuff) {
    return partValue(c, effectPart(c, abilityOf(c, ability), has(hasBuff, BUFF_QUICK_MOVES)), hasBuff);
}

int abilityMiddleTime(const uint8_t *c, int ability, const HasBuff &hasBuff) {
    ability = abilityOf(c, ability);
    if (ability == TIMED_ABILITY)
        return truncToInt(read<float>(c, LONG_ABILITY_FACTOR) * 1000.0f);
    return partValue(c, middlePart(ability, has(hasBuff, BUFF_QUICK_MOVES)), hasBuff);
}

int abilityRecoveryTime(const uint8_t *c, int ability, const HasBuff &hasBuff) {
    return partValue(c, recoveryPart(abilityOf(c, ability), has(hasBuff, BUFF_QUICK_MOVES)), hasBuff);
}

int abilityDuration(const uint8_t *c, const HasBuff &hasBuff) {
    const int ability = c[ABILITY];
    const uint32_t sum = (uint32_t)abilityEffectTime(c, ability, hasBuff) + (uint32_t)abilityMiddleTime(c, ability, hasBuff) +
                         (uint32_t)abilityRecoveryTime(c, ability, hasBuff); // wraps as the game's 32-bit sum does
    return (int32_t)sum;
}

int stunDuration(const uint8_t *c) { return isPlayer(c) ? 1500 : 3000; }
int skillLevel() { return 1000; }

// --- §8 Tests ----------------------------------------------------------------------------

bool canStartAbility(const uint8_t *c, int ability, const HasBuff &hasBuff, const std::map<int32_t, float> *cooldowns) {
    const int current = c[ABILITY];
    if (ability == current && abilityRunning(c, hasBuff))
        return false;
    if (read<float>(c, STUN_TIMER) > 0.0f && ability != STUN_BREAKER)
        return false;
    if (ability == ABILITY_50 && (current == ABILITY_50 || abilityRunning(c, hasBuff)))
        return false;
    if (current == ABILITY_32)
        return false;
    if (manaCost(ability, hasBuff) > read<float>(c, MANA))
        return false;
    if (staminaCost(ability, hasBuff) > read<float>(c, STAMINA))
        return false;
    if (cooldowns) {
        const auto it = cooldowns->find(ability);
        if (it != cooldowns->end() && !(it->second == 0.0f)) // a NaN cooldown also blocks
            return false;
    }
    if (ability == MANA_SHIELD)
        return read<float>(c, MANA) > 0.0f;
    return true;
}

bool hostile(const uint8_t *a, const uint8_t *b) {
    constexpr uint8_t ALWAYS_HOSTILE = 7, NEVER_HOSTILE = 8, HOSTILITY_1 = 1;
    constexpr uint8_t PEACEFUL_CLASS = 0xa1;
    constexpr int32_t PEACEFUL_RACE = 0x12f;
    if (a == b || hasFlag(a, FLAG_22) || hasFlag(b, FLAG_22))
        return false;
    const uint8_t ka = a[HOSTILITY], kb = b[HOSTILITY];
    if (ka == ALWAYS_HOSTILE || kb == ALWAYS_HOSTILE)
        return true;
    if (a[CLASS] == PEACEFUL_CLASS || b[CLASS] == PEACEFUL_CLASS || read<int32_t>(a, RACE) == PEACEFUL_RACE ||
        read<int32_t>(b, RACE) == PEACEFUL_RACE || ka == NEVER_HOSTILE || kb == NEVER_HOSTILE)
        return false;
    const bool sidesDiffer = ((flags(a) ^ flags(b)) & FLAG_15) != 0;
    if (ka == HOSTILITY_1 && (kb != HOSTILITY_1 || sidesDiffer))
        return true;
    if (kb == HOSTILITY_1 && (ka != HOSTILITY_1 || sidesDiffer))
        return true;
    return (a[MORE_FLAGS] & MORE_FLAG_5) || (b[MORE_FLAGS] & MORE_FLAG_5);
}

bool canBeDamaged(const uint8_t *c) {
    switch (c[HOSTILITY]) {
    case 0: case 1: case 3: case 6:
        return true;
    case 7: {
        const int32_t race = read<int32_t>(c, RACE);
        return race < 0x8c || race > 0x8e;
    }
    default:
        return false;
    }
}

namespace {
bool isMageClass(const uint8_t *c) { return c[CLASS] == 3 || c[CLASS] == 0x97 || c[CLASS] == 0x98; }
bool isCasterRace(const uint8_t *c) {
    const int32_t race = read<int32_t>(c, RACE);
    return race == 0x75 || race == 0x56;
}
bool holdsWeaponOf(const uint8_t *item, std::initializer_list<int32_t> subtypes) {
    if (itemType(item) != WEAPON)
        return false;
    for (int32_t s : subtypes)
        if (subtype(item) == s)
            return true;
    return false;
}
} // namespace

bool usesMana(const uint8_t *c) {
    return isMageClass(c) || isCasterRace(c) || holdsWeaponOf(c + WEAPON_2, {10, 11, 12});
}

bool isRanged(const uint8_t *c) {
    if (isMageClass(c) || c[CLASS] == 0x95 || c[CLASS] == 0x96 || isCasterRace(c))
        return true;
    if (holdsWeaponOf(c + WEAPON_2, {10, 11, 12}))
        return true;
    return holdsWeaponOf(c + WEAPON_2, {6, 7, 8, 10, 11}) || holdsWeaponOf(c + WEAPON_1, {6, 7, 8, 10, 11});
}

bool holdsTwoHanded(const uint8_t *c) { return isTwoHanded(c + WEAPON_2) || isTwoHanded(c + WEAPON_1); }

bool isGuarding(const uint8_t *c, const HasBuff &hasBuff) {
    if (read<float>(c, STAMINA_LOCK) > 0.0f)
        return false;
    switch (c[ABILITY]) {
    case GUARD: case GUARD_6E:
        return true;
    case LONG_GUARD:
        return abilityRunning(c, hasBuff);
    case GUARD_STANCE: case GUARD_STANCE_2:
        return isClass(c, 1, 1);
    default:
        return false;
    }
}

int classSkill(const uint8_t *c) {
    const bool first = c[SPECIALISATION] == 0;
    switch (c[CLASS]) {
    case 1: return first ? 0x94 : 0x95;
    case 2: return first ? 0x8f : 0x8e;
    case 3: return first ? 0x2f : 0x90;
    case 4: return first ? 0x6d : 0x8d;
    default: return 0;
    }
}

int raceGroup(int race) {
    // Each group as inclusive ranges of race ids (§8.4).
    struct Range {
        int first, last, group;
    };
    static constexpr Range RANGES[] = {
        {0x78, 0x82, 1}, {0x12e, 0x12e, 1},
        {0x83, 0x90, 2}, {0xaf, 0xbe, 2}, {0xc2, 0xc6, 2}, {0xcc, 0xcf, 2}, {0xd4, 0xd4, 2}, {0xd6, 0xd6, 2},
        {0xdd, 0xdf, 2}, {0xe7, 0xeb, 2}, {0xee, 0xfb, 2}, {0x106, 0x106, 2}, {0x108, 0x10c, 2}, {0x10f, 0x113, 2},
        {0x116, 0x11e, 2}, {0x121, 0x121, 2}, {0x124, 0x124, 2}, {0x129, 0x12d, 2},
        {0x00, 0x10, 3}, {0x12, 0x12, 3}, {0x2b, 0x2e, 3}, {0x30, 0x31, 3}, {0x33, 0x34, 3}, {0x36, 0x36, 3},
        {0x4c, 0x51, 3}, {0x53, 0x54, 3}, {0x5f, 0x5f, 3}, {0x94, 0x95, 3}, {0xa7, 0xa7, 3}, {0xbf, 0xbf, 3},
        {0xc7, 0xc7, 3}, {0xca, 0xcb, 3}, {0xd0, 0xd0, 3}, {0xd8, 0xd8, 3}, {0xe3, 0xe4, 3}, {0xe6, 0xe6, 3},
        {0xec, 0xed, 3}, {0xfc, 0xfe, 3}, {0x100, 0x100, 3}, {0x103, 0x105, 3}, {0x128, 0x128, 3}, {0x130, 0x131, 3},
        {0x13, 0x24, 4}, {0x32, 0x32, 4}, {0x35, 0x35, 4}, {0x37, 0x44, 4}, {0x4a, 0x4b, 4}, {0x57, 0x5d, 4},
        {0x62, 0x64, 4}, {0x66, 0x6b, 4}, {0x74, 0x74, 4}, {0x91, 0x93, 4}, {0x96, 0x9f, 4}, {0xa1, 0xa4, 4},
        {0xa8, 0xac, 4}, {0xae, 0xae, 4}, {0xc8, 0xc9, 4}, {0xd1, 0xd3, 4}, {0xd5, 0xd5, 4}, {0xd7, 0xd7, 4},
        {0xd9, 0xdc, 4}, {0xe0, 0xe0, 4}, {0x10e, 0x10e, 4}, {0x115, 0x115, 4}, {0x125, 0x127, 4}, {0x12f, 0x12f, 4},
        {0x11, 0x11, 5}, {0x29, 0x2a, 5}, {0x60, 0x61, 5}, {0x75, 0x75, 5}, {0xa5, 0xa6, 5}, {0xad, 0xad, 5},
        {0xff, 0xff, 5}, {0x10d, 0x10d, 5}, {0x114, 0x114, 5},
        {0x49, 0x49, 6}, {0x52, 0x52, 6}, {0x55, 0x55, 6}, {0x6e, 0x6e, 6}, {0xe1, 0xe2, 6}, {0xe5, 0xe5, 6},
        {0x45, 0x48, 7}, {0xc0, 0xc1, 7}, {0x107, 0x107, 7}, {0x11f, 0x120, 7}, {0x122, 0x123, 7},
    };
    for (const Range &r : RANGES)
        if (race >= r.first && race <= r.last)
            return r.group;
    return 0;
}

// --- §9 Skills, §10 Level curves ---------------------------------------------------------

float skillValue(const uint8_t *c, Skill skill) {
    const int32_t points = isPlayer(c) ? read<int32_t>(c, SKILL_POINTS + 4 * (size_t)skill)
                                       : (int32_t)((uint32_t)read<int32_t>(c, LEVEL) * 10u);
    const float k = 1.0f - 1.0f / ((float)points / 20.0f + 1.0f); // 0 with no points, towards 1
    const bool strong = hasFlag(c, FLAG_9) || hasFlag(c, FLAG_18);
    switch (skill) {
    case Skill::Climbing:
    case Skill::Swimming:
        return strong ? 1.0f : (k + 1.0f) * 0.5f;
    case Skill::Diving:
        return isPlayer(c) ? (1.0f - k) * 0.1f : 0.0f;
    case Skill::Riding:
        return k * 1.4f + 1.1f;
    case Skill::HangGliding:
    case Skill::Sailing:
        return k * 1.9f + 1.1f;
    case Skill::LightRadius:
        return k * 12.0f + 8.0f;
    }
    return 0.0f;
}

namespace {
// The saturating curve of a level-like number: 0 at 1, towards 1 (§10.1).
float levelCurve(int n) { return 1.0f - 1.0f / (((float)n - 1.0f) / 50.0f + 1.0f); }
} // namespace

float cloneAuraStrength(int n) { return levelCurve(n) * 0.5f + 0.5f; }
int cloneAuraDuration(int n) { return truncToInt(levelCurve(n) * 15000.0f + 15000.0f); }
float stealthGain(int n) { return levelCurve(n) * 0.4f + 0.8f; }
float glideFactor(int n) { return levelCurve(n) * 1.8f + 1.2f; }

int experienceForLevel(int level) {
    const float exponent = (float)(int32_t)((uint32_t)level - 1u) * 0.5f;
    const double xp = std::pow(2.0, (double)exponent) * 10.0;
    if (!(xp >= -2147483648.0 && xp < 2147483648.0))
        return INT32_MIN;
    return (int)xp;
}

float levelFactor(float x) { return (x - 1.0f) * 0.1f + 1.0f; }

bool twoHandedWeapon(const uint8_t *item) { return isTwoHanded(item); }
bool renowned(const uint8_t *item) { return isRenowned(item); }

} // namespace nativerules
