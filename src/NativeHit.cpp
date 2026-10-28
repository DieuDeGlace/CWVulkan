// A hit on a creature, written from docs/specs/creature-hit.md.
//
// hit() is the game's hit routine (0x29eb60): it decides whether a strike is ignored,
// deflected, blocked, soaked by a mana shield or lands, and describes the outcome as damage
// numbers, sounds and particles. apply() (0x29dcf0) carries a damage number out on its
// target: aggro, knock-back, stun, health, the shield's wear and death.
#include "NativeHit.h"

#include "NativeBuffs.h"
#include "NativeRules.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace nativehit {
namespace {

using nativeai::Creature;

// ---------------------------------------------------------------------------------------
// Record layouts (creature-hit.md §0)

// Creature record.
constexpr size_t CREATURE_ID = 0x08;
constexpr size_t POSITION = 0x10;      // 3 x i64
constexpr size_t VELOCITY = 0x34;      // 3 x f32
constexpr size_t KNOCKBACK = 0x4c;     // 3 x f32
constexpr size_t MOVE_FLAGS = 0x5c;    // bit 0 on the ground, bit 5 untouchable
constexpr size_t HOSTILITY = 0x60;
constexpr size_t RACE = 0x64;
constexpr size_t ABILITY = 0x68;
constexpr size_t ABILITY_TIME = 0x6c;
constexpr size_t COMBO = 0x70;
constexpr size_t LOOK_FLAGS = 0x80;
constexpr size_t SIZE_FACTOR = 0x84;
constexpr size_t STATE_FLAGS = 0x128;  // u16
constexpr size_t DODGE_TIMER = 0x12c;
constexpr size_t STUN_TIMER = 0x130;
constexpr size_t GUARD_LOCK = 0x134;   // while > 0 the creature can't guard
constexpr size_t ICE_TIMER = 0x138;
constexpr size_t WIND_TIMER = 0x13c;
constexpr size_t CLASS = 0x14c;
constexpr size_t SPECIALISATION = 0x14d;
constexpr size_t MANA_RESERVE = 0x158; // kept at or below mana
constexpr size_t HEALTH = 0x180;
constexpr size_t BLOCK_POWER = 0x184;
constexpr size_t MANA = 0x188;
constexpr size_t STEALTH = 0x18c;
constexpr size_t WEAPON_A = 0x638;
constexpr size_t WEAPON_B = 0x6d8;
constexpr size_t HIT_REACTION = 0x994;
constexpr size_t LINKED_ID = 0x9d8;    // dies together with this creature
constexpr size_t LAST_FOE = 0x9e0;

// Item record.
constexpr size_t ITEM_TYPE = 0x00;
constexpr size_t ITEM_SUBTYPE = 0x04;
constexpr size_t ITEM_ENTRIES = 0x20;  // 4-byte entries; this byte is the entry's material
constexpr size_t ITEM_ENTRY_SIZE = 4;
constexpr size_t ITEM_ENTRY_COUNT = 0x9d;

// An inventory slot: a count, then the item.
constexpr size_t SLOT_ITEM = 4;

// Damage number record (world+0x90).
constexpr size_t NUMBER_ATTACKER = 0x00;
constexpr size_t NUMBER_TARGET = 0x08;
constexpr size_t NUMBER_DAMAGE = 0x10;
constexpr size_t NUMBER_CRITICAL = 0x14;
constexpr size_t NUMBER_STUN = 0x18;
constexpr size_t NUMBER_GUARD_LOCK = 0x1c;
constexpr size_t NUMBER_POSITION = 0x20;
constexpr size_t NUMBER_PUSH = 0x38;
constexpr size_t NUMBER_SPIRIT = 0x44;
constexpr size_t NUMBER_KIND = 0x45;
constexpr size_t NUMBER_CHARGED = 0x46;

// Damage number kinds.
constexpr uint8_t KIND_NORMAL = 0;
constexpr uint8_t KIND_BLOCKED = 1;
constexpr uint8_t KIND_IMMUNE = 3;
constexpr uint8_t KIND_DODGE_REWARD = 4;
constexpr uint8_t KIND_MANA_SHIELD = 5;

// Hostility kinds. Only 0 and 1 have known meanings; the others are named by number.
constexpr uint8_t PLAYER = 0;
constexpr uint8_t HOSTILE = 1;
constexpr uint8_t HOSTILITY_6 = 6;
constexpr uint8_t HOSTILITY_7 = 7;  // no critical cry, no stun, not pushed
constexpr uint8_t HOSTILITY_8 = 8;  // never hit

// Buff types.
constexpr uint8_t BUFF_DAMAGE_TAKEN = 1; // multiplies the damage taken; prevents stuns
constexpr uint8_t BUFF_LIFESTEAL = 2;
constexpr uint8_t BUFF_POISON = 4;
constexpr uint8_t BUFF_MANA_SHIELD = 6;
constexpr uint8_t BUFF_DODGE_REWARD = 0x0b;
constexpr uint8_t BUFF_MANA_FACTOR = 0x22;

// Look flags (+0x80).
constexpr uint32_t LOOK_REWARD_GATE = 1u << 13; // only players' foes may hurt it
constexpr uint32_t LOOK_BOSS = 1u << 18;        // resists stuns; stuns readily

// State flags (+0x128).
constexpr uint16_t STATE_IMMUNE = 1u << 7;

// Movement flags (+0x5c).
constexpr uint8_t MOVE_ON_GROUND = 1u << 0;
constexpr uint8_t MOVE_IMMUNE = 1u << 5;

// Ability ids.
constexpr int ABILITY_GUARD = 9;
constexpr int ABILITY_BASH_SOUND = 0x0b;
constexpr int ABILITY_POISON = 0x12;
constexpr int ABILITY_MANA_CHARGE = 0x1d;
constexpr int ABILITY_SMASH_SOUND = 0x37;
constexpr int ABILITY_UNARMED_SOUND = 0x45;
constexpr int ABILITY_SHIELD_BLOCK_1 = 0x48;
constexpr int ABILITY_SHIELD_BLOCK_2 = 0x49;
constexpr int ABILITY_GROUND_ONLY_1 = 0x4b;
constexpr int ABILITY_GROUND_ONLY_2 = 0x64;
constexpr int ABILITY_STAGGERED = 0x58;
constexpr int ABILITY_GUARD_STANCE = 0x5f;
constexpr int ABILITY_GUARD_6E = 0x6e;
constexpr int ABILITY_SHELTERED = 0x7d;
constexpr int ABILITY_COUNTER_BLOCK = 0x82;
constexpr int ABILITY_PIERCE = 0x8c;
constexpr int ABILITY_UPLIFT = 0x94;

// Sounds.
constexpr int SOUND_DEFLECT = 0x18;
constexpr int SOUND_STUN = 0x17;
constexpr int SOUND_DEATH = 0x1b;
constexpr int SOUND_DEATH_SPECIAL = 0x2d; // also hit type 2's impact
constexpr int SOUND_SPELL_IMPACT = 0x2a;
constexpr int SOUND_MANA_SHIELD = 0x5e;
constexpr int RACE_SPECIAL_DEATH = 0x11d;
constexpr int RACE_UNHITTABLE = 0x114;

// Times.
constexpr float SHELTER_TIME = 5000.0f;
constexpr float POISON_DURATION = 3000.0f;
constexpr float STUN_READY_BELOW = -3000.0f;
constexpr float SPIRIT_TIMER_CAP = 5000.0f;
constexpr float DODGE_REWARD_DURATION = 30000.0f;
constexpr float RAND_MAX_F = 32767.0f;

// ---------------------------------------------------------------------------------------
// Record access

template <class T> T read(const uint8_t *p, size_t offset) {
    T v;
    std::memcpy(&v, p + offset, sizeof v);
    return v;
}
template <class T> void write(uint8_t *p, size_t offset, T v) { std::memcpy(p + offset, &v, sizeof v); }

int64_t idOf(const Creature *c) { return read<int64_t>(c->record, CREATURE_ID); }
uint8_t hostility(const Creature *c) { return c->record[HOSTILITY]; }
int ability(const Creature *c) { return c->record[ABILITY]; }
float health(const Creature *c) { return read<float>(c->record, HEALTH); }
void setHealth(Creature *c, float v) { write(c->record, HEALTH, v); }
bool hasLook(const Creature *c, uint32_t bit) { return (read<uint32_t>(c->record, LOOK_FLAGS) & bit) != 0; }
bool isClass(const Creature *c, int cls, int spec) {
    return c->record[CLASS] == cls && c->record[SPECIALISATION] == spec;
}

// A uniform rand() fraction scaled and offset: rand x scale / 32767 + offset.
float randomized(World &world, float scale, float offset) {
    return (float)world.rand() * scale / RAND_MAX_F + offset;
}

nativerules::HasBuff hasBuffOf(const Creature *c) {
    return [c](int type) { return nativebuffs::find(*c, (uint8_t)type) != nullptr; };
}

float maxHealthOf(const Creature *c) {
    const std::vector<nativerules::Buff> buffs = nativebuffs::forStats(*c);
    return nativerules::maxHealth(c->record, buffs.data(), buffs.size());
}

// ---------------------------------------------------------------------------------------
// Items and the stat helpers this routine needs (creature-hit.md §5, stat-rules.md)

const uint8_t *weaponA(const Creature *c) { return c->record + WEAPON_A; }
const uint8_t *weaponB(const Creature *c) { return c->record + WEAPON_B; }
int itemType(const uint8_t *item) { return item[ITEM_TYPE]; }
int subtype(const uint8_t *item) { return read<int32_t>(item, ITEM_SUBTYPE); }
bool isWeapon(const uint8_t *item) { return itemType(item) == 3; }

bool isTwoHanded(const uint8_t *item) {
    const uint32_t s = (uint32_t)subtype(item);
    return isWeapon(item) && s <= 23 && ((0x878de0u >> s) & 1);
}

// A weapon the creature may use (stat-rules.md §0.5).
bool usableWeapon(const Creature *c, const uint8_t *item) {
    return isWeapon(item) && nativerules::mayUse(c->record, item);
}

// 0x4fd40: the weapons the creature's current ability strikes with.
std::vector<const uint8_t *> weaponsInUse(const Creature *c) {
    const uint8_t *a = weaponA(c), *b = weaponB(c);
    auto isBow = [&](const uint8_t *w) { return usableWeapon(c, w) && (subtype(w) == 6 || subtype(w) == 7); };
    if (isBow(b))
        return {b};
    if (isBow(a))
        return {a};
    std::vector<const uint8_t *> out;
    switch (ability(c)) {
    case 3: case 5: case 8: case 9: case 0x0b: case 0x13: case 0x28: case 0x2a:
        if (usableWeapon(c, a))
            out.push_back(a);
        break;
    case 0x0c: case 0x0d: case 0x11: case 0x12: case 0x15: case 0x37:
    case 0x5f: case 0x64: case 0x6c: case 0x8c: case 0x94: case 0x9e:
        if (usableWeapon(c, a))
            out.push_back(a);
        if (usableWeapon(c, b))
            out.push_back(b);
        break;
    case 0x29: case 0x2b:
        if (usableWeapon(c, b))
            out.push_back(b);
        break;
    default:
        if (usableWeapon(c, b))
            out.push_back(b);
        if (usableWeapon(c, a) && subtype(a) == 12)
            out.push_back(a);
        break;
    }
    return out;
}

// 0x10ad90 for weapons only; anything else holds nothing.
float blockItem(const Creature *c, const uint8_t *item) {
    return isWeapon(item) ? nativerules::weaponDamage(item, c->record) : 0.0f;
}

// 0x50370: how much the creature's block holds.
float blockValue(const Creature *c) {
    const uint8_t *a = weaponA(c), *b = weaponB(c);
    float value = 5.0f;
    if (usableWeapon(c, a) && subtype(a) == 13) { // a shield
        const float factor = isClass(c, 1, 1) ? 6.0f : 4.0f;
        value = blockItem(c, b) * factor + 5.0f;
    } else {
        if (usableWeapon(c, a))
            value = blockItem(c, a) + value;
        if (usableWeapon(c, b))
            value = value + blockItem(c, b);
    }
    if (hasLook(c, 1u << 9))
        value = value + value;
    if (hasLook(c, 1u << 17))
        value *= 1.2f;
    if (hasLook(c, LOOK_BOSS))
        value *= 4.0f;
    if (hasLook(c, 1u << 26))
        value *= 1.1f;
    return value;
}

// 0x5ed30: whether the creature is holding a guard.
bool guarding(const Creature *c) {
    if (read<float>(c->record, GUARD_LOCK) > 0.0f)
        return false;
    switch (ability(c)) {
    case ABILITY_GUARD:
    case ABILITY_GUARD_6E:
        return true;
    case ABILITY_GUARD_STANCE:
        return (float)nativerules::abilityDuration(c->record, hasBuffOf(c)) > read<float>(c->record, ABILITY_TIME);
    case 0x3c:
    case 0x40:
        return isClass(c, 1, 1);
    default:
        return false;
    }
}

// 0x5f8d0: what mana gains are multiplied by.
float manaFactor(const Creature *c) {
    const nativebuffs::Buff *b = nativebuffs::find(*c, BUFF_MANA_FACTOR);
    return b ? 1.0f - nativebuffs::strength(*b) * 0.1f : 1.0f;
}

// 0x5eca0: a creature that fights from range.
bool ranged(const Creature *c) {
    const int cls = c->record[CLASS];
    const int race = read<int32_t>(c->record, RACE);
    if (cls == 3 || (cls >= 0x95 && cls <= 0x98) || race == 0x75 || race == 0x56)
        return true;
    const uint8_t *b = weaponB(c);
    if (isWeapon(b) && subtype(b) >= 10 && subtype(b) <= 12)
        return true;
    for (const uint8_t *w : {b, weaponA(c)})
        if (isWeapon(w))
            switch (subtype(w)) {
            case 6: case 7: case 8: case 10: case 11:
                return true;
            }
    return false;
}

// 0x50520: whether the creature's health can be lowered.
bool canBeDamaged(const Creature *c) {
    switch (hostility(c)) {
    case 0: case 1: case 3: case 6:
        return true;
    case HOSTILITY_7: {
        const int race = read<int32_t>(c->record, RACE);
        return race < 0x8c || race > 0x8e;
    }
    default:
        return false;
    }
}

// Race group 6 of 0x65750: the races the local player's charms hurt more.
bool charmedAgainst(int race) {
    switch (race) {
    case 0x49: case 0x52: case 0x55: case 0x6e: case 0xe1: case 0xe2: case 0xe5:
        return true;
    default:
        return false;
    }
}

// 0x510f0: the charms (type 24, subtype 12) the creature carries in its first inventory tab,
// one per slot whatever its stack size; none unless it has at least two tabs.
float charmCount(const Creature *c) {
    float count = 0.0f;
    if (c->inventory.size() < 2)
        return count;
    for (const auto &slot : c->inventory.front()) {
        const uint8_t *item = slot.data() + SLOT_ITEM;
        if (read<int32_t>(slot.data(), 0) > 0 && itemType(item) == 24 && subtype(item) == 12)
            count += 1.0f;
    }
    return count;
}

// The combo as a fraction of the attacker's combo threshold (0x5f9e0).
float comboRatio(const Creature *c) {
    return (float)read<int32_t>(c->record, COMBO) / (float)nativerules::comboThreshold(c->record);
}

// The critical cry of a target's race (0: none).
int criticalCry(int race) {
    switch (race) {
    case 0: return 0x3f;
    case 1: return 0x40;
    case 2: case 0x2b: case 0x53: case 0xa7: case 0xca: case 0xe3: case 0xe6: case 0xec: return 0x41;
    case 3: case 0x2d: case 0x54: case 0xcb: case 0xe4: case 0xed: return 0x42;
    case 4: return 0x43;
    case 5: return 0x44;
    case 7: return 0x45;
    case 8: return 0x46;
    case 9: return 0x47;
    case 10: return 0x48;
    case 0x0b: case 0x104: return 0x49;
    case 0x0c: case 0x105: return 0x4a;
    case 0x0d: return 0x4d;
    case 0x0e: return 0x4e;
    case 0x0f: return 0x4b;
    case 0x10: return 0x4c;
    case 0x25: case 0x26: case 0x27: case 0x28: return 0x52;
    case 0x2e: case 0x6c: case 0x6d: case 0x72: case 0x73: case 0xd0: return 0x50;
    case 0x30: case 0x33: case 0x57: return 0x51;
    case 0x60: return 0x53;
    case 0x77: return 0x4f;
    default: return 0;
    }
}

// ---------------------------------------------------------------------------------------
// What the hit tells the world

void playSound(World &world, const uint8_t *position, int id, float pitch) {
    std::array<uint8_t, 0x30> s{};
    std::memcpy(s.data(), position, 24);
    write<int32_t>(s.data(), 0x18, id);
    write<float>(s.data(), 0x1c, pitch);
    write<float>(s.data(), 0x20, 1.0f);
    world.sounds.push_back(s);
}

struct Burst {
    float velocity[3] = {0, 0, 0};
    float colour[4] = {0, 0, 0, 0};
    float size = 0;
    int32_t count = 0, style = 0;
    float spread = 0;
};

void addParticles(World &world, const uint8_t *position, const Burst &b) {
    std::array<uint8_t, 0x48> p{};
    std::memcpy(p.data(), position, 24);
    std::memcpy(p.data() + 0x18, b.velocity, 12);
    std::memcpy(p.data() + 0x24, b.colour, 16);
    write(p.data(), 0x34, b.size);
    write(p.data(), 0x38, b.count);
    write(p.data(), 0x3c, b.style);
    write(p.data(), 0x40, b.spread);
    world.particles.push_back(p);
}

// A damage number (world+0x90) in readable form.
struct DamageNumber {
    int64_t attacker = 0, target = 0;
    float damage = 0;
    uint8_t critical = 0;
    int32_t stun = 0, guardLock = 0;
    int64_t position[3] = {0, 0, 0};
    float push[3] = {0, 0, 0};
    uint8_t spirit = 0, kind = KIND_NORMAL, charged = 0;

    Number bytes() const {
        Number n{};
        write(n.data(), NUMBER_ATTACKER, attacker);
        write(n.data(), NUMBER_TARGET, target);
        write(n.data(), NUMBER_DAMAGE, damage);
        n[NUMBER_CRITICAL] = critical;
        write(n.data(), NUMBER_STUN, stun);
        write(n.data(), NUMBER_GUARD_LOCK, guardLock);
        std::memcpy(n.data() + NUMBER_POSITION, position, 24);
        std::memcpy(n.data() + NUMBER_PUSH, push, 12);
        n[NUMBER_SPIRIT] = spirit;
        n[NUMBER_KIND] = kind;
        n[NUMBER_CHARGED] = charged;
        return n;
    }
    static DamageNumber from(const Number &n) {
        DamageNumber d;
        d.attacker = read<int64_t>(n.data(), NUMBER_ATTACKER);
        d.target = read<int64_t>(n.data(), NUMBER_TARGET);
        d.damage = read<float>(n.data(), NUMBER_DAMAGE);
        d.critical = n[NUMBER_CRITICAL];
        d.stun = read<int32_t>(n.data(), NUMBER_STUN);
        d.guardLock = read<int32_t>(n.data(), NUMBER_GUARD_LOCK);
        std::memcpy(d.position, n.data() + NUMBER_POSITION, 24);
        std::memcpy(d.push, n.data() + NUMBER_PUSH, 12);
        d.spirit = n[NUMBER_SPIRIT];
        d.kind = n[NUMBER_KIND];
        d.charged = n[NUMBER_CHARGED];
        return d;
    }
};

// A number on the target at its position: the deflected, blocked and shield numbers.
DamageNumber numberOn(const Creature *target, const Creature *attacker, float damage, const float push[3],
                      float pushScale, uint8_t kind) {
    DamageNumber n;
    n.attacker = attacker ? idOf(attacker) : 0;
    n.target = idOf(target);
    n.damage = damage;
    std::memcpy(n.position, target->record + POSITION, 24);
    for (int i = 0; i < 3; i++)
        n.push[i] = pushScale == 1.0f ? push[i] : push[i] * pushScale;
    n.kind = kind;
    return n;
}

// The attacker heals itself by `amount`: a negative number on itself, then its health.
void selfHeal(World &world, Creature *attacker, float amount) {
    DamageNumber n;
    n.attacker = n.target = idOf(attacker);
    n.damage = -amount;
    std::memcpy(n.position, attacker->record + POSITION, 24);
    world.numbers.push_back(n.bytes());
    setHealth(attacker, health(attacker) + amount);
}

// Appends a number and, on a server, applies it.
void publish(World &world, const DamageNumber &n) {
    const Number bytes = n.bytes();
    world.numbers.push_back(bytes);
    if (!world.client)
        nativehit::apply(world, bytes);
}

// ---------------------------------------------------------------------------------------
// Aggro (0x2c3200 and the distance test 0xf3e20)

// Whether two positions are within `range` blocks of each other.
bool within(const uint8_t *p, const uint8_t *q, float range) {
    constexpr double DOTS = 1.0 / 65536.0;
    double d[3];
    for (int i = 0; i < 3; i++)
        d[i] = (double)(read<int64_t>(p, POSITION + 8 * i) - read<int64_t>(q, POSITION + 8 * i)) * DOTS;
    if (std::fabs(d[0]) > range || std::fabs(d[1]) > range)
        return false;
    return !(d[0] * d[0] + d[1] * d[1] + d[2] * d[2] > (double)(range * range));
}

void aggroNote(World &world, int64_t from, int64_t to, float anger) {
    std::array<uint8_t, 0x18> note{};
    write(note.data(), 0, from);
    write(note.data(), 8, to);
    write(note.data(), 0x10, anger);
    world.aggroNotes.push_back(note);
}

// `holder` tells the world how angry it is at `foe`; its allies within 8 blocks who weren't
// angry at `foe` yet become a little angry.
void spreadAggro(World &world, Creature *foe, Creature *holder) {
    if (!(health(holder) > 0.0f))
        return;
    const int64_t foeId = idOf(foe);
    aggroNote(world, foeId, idOf(holder), holder->aggro[foeId]);
    for (Creature *c : world.creatures) {
        if (!c || c == foe || c == holder || !nativerules::hostile(c->record, foe->record) ||
            !within(c->record, holder->record, 8.0f))
            continue;
        float &anger = c->aggro[foeId];
        if (anger == 0.0f) {
            anger = 0.5f;
            aggroNote(world, foeId, idOf(c), 0.5f);
        }
    }
}

// ---------------------------------------------------------------------------------------
// The hit's steps (creature-hit.md §2)

Creature *lookup(World &world, int64_t id) { return world.byId ? world.byId(id) : nullptr; }
bool isLocal(const World &world, const Creature *c) { return c && world.local == c->record; }

// §2.1: strikes that are ignored outright.
bool ignored(const World &world, const Creature *target, const Creature *attacker) {
    if (health(target) <= 0.0f || hostility(target) == HOSTILITY_8 ||
        read<int32_t>(target->record, RACE) == RACE_UNHITTABLE)
        return true;
    return world.client && (isLocal(world, target) || !isLocal(world, attacker));
}

// §2.6: whether the target can be touched at all.
bool hittable(const Creature *target, int hitAbility) {
    if (hitAbility == ABILITY_PIERCE)
        return true;
    if (read<uint16_t>(target->record, STATE_FLAGS) & STATE_IMMUNE)
        return false;
    if (target->record[MOVE_FLAGS] & MOVE_IMMUNE)
        return false;
    return !(ability(target) == ABILITY_SHELTERED && read<float>(target->record, ABILITY_TIME) < SHELTER_TIME);
}

// §2.7: whether the target blocks this strike.
bool blocks(const Creature *target) {
    if (guarding(target) && read<float>(target->record, BLOCK_POWER) > 0.0f)
        return true;
    const int a = ability(target);
    if (a != ABILITY_SHIELD_BLOCK_1 && a != ABILITY_SHIELD_BLOCK_2 && a != ABILITY_COUNTER_BLOCK)
        return false;
    const nativerules::HasBuff has = hasBuffOf(target);
    const float windUp = (float)nativerules::abilityEffectTime(target->record, a, has);
    const float active = (float)nativerules::abilityMiddleTime(target->record, a, has);
    const float time = read<float>(target->record, ABILITY_TIME);
    return windUp <= time && time < windUp + active;
}

// §2.7 on a block: the target's mana and block power pay for it. Returns nothing; the
// caller then reports it.
void payForBlock(World &world, Creature *target, float damage) {
    float cost = damage / blockValue(target);
    const int a = ability(target);
    if (a == ABILITY_SHIELD_BLOCK_1 || a == ABILITY_SHIELD_BLOCK_2 || a == ABILITY_COUNTER_BLOCK)
        cost = 0.0f;
    if (a == ABILITY_GUARD_STANCE)
        cost *= 0.5f;

    uint8_t *t = target->record;
    if (!world.client) {
        const float gain = std::max((1.0f - cost) * 0.5f, 0.0f);
        const float factor = manaFactor(target);
        if (guarding(target)) {
            write(t, MANA, read<float>(t, MANA) + factor * gain);
            if (a != ABILITY_GUARD_STANCE)
                write(t, MANA_RESERVE, read<float>(t, MANA_RESERVE) + factor * gain);
        }
        if (read<float>(t, MANA) > 1.0f)
            write(t, MANA, 1.0f);
        if (read<float>(t, MANA_RESERVE) > read<float>(t, MANA))
            write(t, MANA_RESERVE, read<float>(t, MANA));
    }
    float power = read<float>(t, BLOCK_POWER) - cost;
    if (power < -1.0f)
        power = -1.0f;
    write(t, BLOCK_POWER, power);
}

// §2.9: the colour of a spirit material's particles at strength s.
void spiritColour(int material, float s, float out[4]) {
    static const float base[4][3] = {{1.0f, 0.5f, 0.1f}, {0.3f, 1.0f, 0.5f}, {0.3f, 0.5f, 1.0f}, {0.8f, 0.8f, 1.0f}};
    if (material < 0x80 || material > 0x83) {
        out[0] = out[1] = out[2] = 0.5f;
        out[3] = 1.0f;
        return;
    }
    const float glow = 0.5f * s + 1.0f;
    for (int i = 0; i < 3; i++)
        out[i] = base[material - 0x80][i] * glow;
    out[3] = (s + 1.0f) * glow;
}

// The spirit materials (0x80 and up) of a weapon's entries, in order.
std::vector<int> spiritsOf(const uint8_t *weapon) {
    std::vector<int> out;
    for (int i = 0; i < weapon[ITEM_ENTRY_COUNT]; i++) {
        const int material = weapon[ITEM_ENTRIES + ITEM_ENTRY_SIZE * i];
        if (material >= 0x80)
            out.push_back(material);
    }
    return out;
}

// §2.9: the attacker's weapon spirits glow on the target.
void spiritParticles(World &world, const Creature *target, const Creature *attacker, const float push[3], float r,
                     bool spirit) {
    float vx = push[0], vy = push[1];
    const float lengthSq = push[0] * push[0] + push[1] * push[1] + push[2] * push[2];
    if (lengthSq > 0.0f) {
        const float inverse = 1.0f / std::sqrt(lengthSq);
        vx *= inverse, vy *= inverse;
    }
    vx *= 4.0f, vy *= 4.0f;
    for (const uint8_t *weapon : weaponsInUse(attacker))
        for (int material : spiritsOf(weapon)) {
            Burst b;
            b.velocity[0] = vx, b.velocity[1] = vy, b.velocity[2] = 5.0f;
            if (spirit)
                for (float &v : b.velocity)
                    v = v + v;
            spiritColour(material, r * 0.5f, b.colour);
            b.size = 0.1f;
            b.count = (int32_t)(r * 3.0f + 1.0f) + (spirit ? 10 : 0);
            b.style = 2;
            b.spread = 3.0f;
            addParticles(world, target->record + POSITION, b);
        }
}

// §2.9: the spirits' extra damage, the target's ice and the attacker's wind timers, and
// the attacker's spirit heal. Returns the damage.
float spiritEffects(World &world, Creature *target, Creature *attacker, float damage, float r) {
    const std::vector<const uint8_t *> weapons = weaponsInUse(attacker);
    if (weapons.empty())
        return damage;
    const float share = 0.1f / (float)weapons.size();
    float heal = 0.0f;
    for (const uint8_t *weapon : weapons)
        for (int material : spiritsOf(weapon)) {
            switch (material) {
            case 0x80: // fire
                damage += share * r;
                break;
            case 0x81: { // life
                const float t = share * r;
                heal += t + t;
                damage += t * 0.5f;
                break;
            }
            case 0x82: // ice
                damage += share * 0.25f * r;
                write(target->record, ICE_TIMER, read<float>(target->record, ICE_TIMER) + r * 500.0f);
                break;
            case 0x83: // wind
                damage += share * 0.25f * r;
                write(attacker->record, WIND_TIMER, read<float>(attacker->record, WIND_TIMER) + r * 500.0f);
                break;
            }
        }
    if (read<float>(target->record, ICE_TIMER) > SPIRIT_TIMER_CAP)
        write(target->record, ICE_TIMER, SPIRIT_TIMER_CAP);
    if (read<float>(attacker->record, WIND_TIMER) > SPIRIT_TIMER_CAP)
        write(attacker->record, WIND_TIMER, SPIRIT_TIMER_CAP);
    if (heal > 0.0f)
        selfHeal(world, attacker, heal);
    return damage;
}

// §2.10: the sound of a physical strike landing.
void impactSound(World &world, const Creature *target, const Creature *attacker, bool critical) {
    const std::vector<const uint8_t *> weapons = weaponsInUse(attacker);
    int kind = weapons.empty() ? -1 : subtype(weapons.front());
    if (ability(attacker) == ABILITY_UNARMED_SOUND)
        kind = 0x11;
    float pitch = randomized(world, 0.2f, 0.9f);
    int id;
    switch (kind) {
    case 0: case 1:
        id = critical ? 2 : 1;
        pitch += 0.1f;
        break;
    case 15: case 16:
        id = critical ? 2 : 1;
        break;
    case 3: case 4:
        id = critical ? 8 : 7;
        break;
    case 5:
        id = critical ? 4 : 3;
        break;
    case 6: case 7: case 8:
        id = critical ? 10 : 9;
        break;
    default:
        id = critical ? 6 : 5;
        break;
    }
    if (ability(attacker) == ABILITY_SMASH_SOUND)
        id = 0xb;
    else if (ability(attacker) == ABILITY_BASH_SOUND)
        id = 5;
    playSound(world, target->record + POSITION, id, pitch);
}

// §2.11 step 5: whether the hit stuns the target (one rand drawn once the gate passes).
bool rollStun(World &world, const Creature *target, const Creature *attacker, float chance) {
    switch (ability(attacker)) {
    case 0x0d: case 0x3b: case 0x45: case 0x46: case 0x67:
        chance = 1.0f;
    }
    if (hasLook(target, LOOK_BOSS))
        chance *= 0.15f;
    if (!hasLook(attacker, LOOK_BOSS) && hostility(attacker) != PLAYER)
        chance *= 0.1f;
    if (ability(attacker) == ABILITY_GROUND_ONLY_2 || ability(attacker) == ABILITY_GROUND_ONLY_1)
        chance = 1.0f;
    if (ability(target) == ABILITY_STAGGERED)
        chance = 1.0f;
    chance *= 0.9f;
    if (nativebuffs::find(*target, BUFF_DAMAGE_TAKEN))
        chance = 0.0f;
    return (float)world.rand() / RAND_MAX_F < chance;
}

// §2.11: the main number and what goes with it, for every strike that lands.
bool land(World &world, Creature *target, Creature *attacker, const nativeupkeep::Hit &h, float damage, float d0) {
    DamageNumber n;
    n.attacker = attacker ? idOf(attacker) : 0;
    n.target = idOf(target);
    for (const nativebuffs::Buff &b : target->buffs)
        if (b[0] == BUFF_DAMAGE_TAKEN)
            damage *= nativebuffs::strength(b);
    n.damage = damage;
    n.critical = h.critical;
    std::memcpy(n.position, target->record + POSITION, 24);
    std::memcpy(n.push, h.push, 12);
    n.spirit = h.flag;

    if (h.ability == ABILITY_POISON) {
        const nativebuffs::Buff poison = nativebuffs::make(BUFF_POISON, d0 * 0.2f, POISON_DURATION, n.attacker);
        target->buffs.push_back(poison);
        world.buffNotes.push_back(nativebuffs::notice(n.attacker, n.target, poison, 0));
    }

    if (h.critical && hostility(target) != HOSTILITY_7) {
        const float pitch = randomized(world, 0.1f, 1.0f);
        if (const int cry = criticalCry(read<int32_t>(target->record, RACE)))
            playSound(world, target->record + POSITION, cry, pitch);
    }

    if (attacker && read<float>(target->record, STUN_TIMER) < STUN_READY_BELOW && hostility(target) != HOSTILITY_7 &&
        rollStun(world, target, attacker, h.charge)) {
        n.stun = hostility(target) != PLAYER ? 3000 : 1500;
        n.push[2] = 10.0f;
        playSound(world, target->record + POSITION, SOUND_STUN, 1.0f);
    }

    if (h.ability == ABILITY_UPLIFT)
        n.push[2] += 18.0f / read<float>(target->record, SIZE_FACTOR);
    if (attacker && ability(attacker) == ABILITY_MANA_CHARGE)
        n.charged = 1;
    publish(world, n);
    return true;
}

// ---------------------------------------------------------------------------------------
// The apply routine's steps (creature-hit.md §3)

// §3.3: the attacker and target remember each other and the target grows angry.
void angerAt(World &world, Creature *target, Creature *attacker, float damage) {
    const int64_t targetId = idOf(target), attackerId = idOf(attacker);
    write(attacker->record, LAST_FOE, targetId);
    write(target->record, LAST_FOE, attackerId);
    const uint8_t kind = hostility(attacker);
    const float weight = kind == PLAYER || kind == HOSTILITY_6 ? 1.0f : 0.001f;
    attacker->damage[targetId] += 1.0f;

    float increase;
    if (ranged(attacker))
        increase = weight * damage + 0.01f;
    else if (read<int32_t>(attacker->record, RACE) == 0x19)
        increase = damage * 20.0f * weight + 0.01f;
    else if (isTwoHanded(weaponB(attacker)) && attacker->record[SPECIALISATION] == 1)
        increase = damage * 80.0f * weight + 0.1f;
    else
        increase = (damage + damage) * weight + 0.01f;
    target->aggro[attackerId] += increase;
    spreadAggro(world, attacker, target);
}

// §3.4: a creature with look flag 13 that a player is fighting only takes hits from
// players and kind-6 creatures.
bool shieldedFromOthers(World &world, const Creature *target, const Creature *attacker) {
    if (!hasLook(target, LOOK_REWARD_GATE))
        return false;
    for (const auto &[id, anger] : target->aggro) {
        if (!(anger > 0.0f))
            continue;
        const Creature *c = lookup(world, id);
        if (c && hostility(c) == PLAYER)
            return !attacker || (hostility(attacker) != PLAYER && hostility(attacker) != HOSTILITY_6);
    }
    return true;
}

// §3.5 step 1.
void knockBack(World &world, Creature *target, const DamageNumber &n) {
    uint8_t *t = target->record;
    float p[3] = {n.push[0], n.push[1], n.push[2]};
    if (read<float>(t, STUN_TIMER) > 0.0f && n.stun < 1)
        for (float &v : p)
            v *= 0.1f;
    const uint8_t kind = hostility(target);
    if (kind != HOSTILITY_7 && ((!world.client && kind != PLAYER) || isLocal(world, target))) {
        float k[3];
        std::memcpy(k, t + KNOCKBACK, 12);
        if (k[0] * k[0] + k[1] * k[1] + k[2] * k[2] <= p[0] * p[0] + p[1] * p[1] + p[2] * p[2])
            std::memcpy(t + KNOCKBACK, p, 12);
    }
    if (n.stun > 0)
        std::memset(t + VELOCITY, 0, 12);
}

// §3.5 step 3 (0x4ee50): a dodge rewarded.
void rewardDodge(World &world, Creature *target) {
    uint8_t *t = target->record;
    if (read<float>(t, DODGE_TIMER) == 0.0f || !isClass(target, 4, 1))
        return;
    write(t, MANA, std::min(read<float>(t, MANA) + 0.25f, 1.0f));
    const nativebuffs::Buff b = nativebuffs::make(BUFF_DODGE_REWARD, 0.0f, DODGE_REWARD_DURATION, 0);
    nativebuffs::give(*target, b, false);
    world.buffNotes.push_back(nativebuffs::notice(idOf(target), idOf(target), b, 0));
}

// §3.5 step 4.3: a heal; on a server the healer angers those fighting the healed.
void heal(World &world, Creature *target, Creature *healer, float damage) {
    float h = health(target) - damage;
    const float most = maxHealthOf(target);
    if (h > most)
        h = most;
    setHealth(target, h);
    if (world.client || !healer)
        return;
    const int64_t targetId = idOf(target), healerId = idOf(healer);
    for (Creature *c : world.creatures)
        if (nativerules::hostile(healer->record, c->record) && c->aggro.count(targetId)) {
            c->aggro[healerId] -= damage * 0.1f;
            spreadAggro(world, healer, c);
        }
}

// §3.5 step 4.5: the mana shield takes the damage.
void wearShield(World &world, Creature *target, int64_t attackerId, float damage) {
    for (nativebuffs::Buff &b : target->buffs) {
        if (b[0] != BUFF_MANA_SHIELD)
            continue;
        const float left = nativebuffs::strength(b) - damage;
        write(b.data(), 4, left);
        if (left <= 0.0f) {
            write(b.data(), 4, 0.0f);
            write(b.data(), 8, 0.0f);
        }
        world.buffNotes.push_back(nativebuffs::notice(attackerId, idOf(target), b, 0));
    }
}

// §3.6: the target dies.
void die(World &world, Creature *dead) {
    uint8_t *t = dead->record;
    setHealth(dead, 0.0f);
    const int sound = read<int32_t>(t, RACE) == RACE_SPECIAL_DEATH ? SOUND_DEATH_SPECIAL : SOUND_DEATH;
    playSound(world, t + POSITION, sound, 1.0f);
    write(t, ABILITY_TIME, 0.0f);
    if (world.died) // its drops (0x2a6860) and, for state flag 8, the players' rewards (0x2a7ca0)
        world.died(dead);

    const int64_t deadId = idOf(dead), linkedId = read<int64_t>(t, LINKED_ID);
    if (linkedId != 0)
        if (Creature *linked = lookup(world, linkedId))
            setHealth(linked, 0.0f);

    if (dead->aggro.empty())
        world.kills.emplace_back(0, deadId);
    for (const auto &entry : dead->aggro)
        world.kills.emplace_back(entry.first, deadId);

    for (Creature *c : world.creatures) {
        const int64_t last = read<int64_t>(c->record, LAST_FOE);
        if (last == deadId || last == linkedId)
            write<int64_t>(c->record, LAST_FOE, 0);
        c->aggro.erase(deadId);
        c->aggro.erase(linkedId);
    }
}

} // namespace

// ---------------------------------------------------------------------------------------

bool hit(World &world, Creature *target, Creature *attacker, const nativeupkeep::Hit &h) {
    if (ignored(world, target, attacker))
        return false;

    // §2.2: the local player's charms multiply its damage on some races.
    float damage = h.damage;
    if (attacker && isLocal(world, attacker) && charmedAgainst(read<int32_t>(target->record, RACE)))
        damage = damage * (charmCount(attacker) + 1.0f);
    const float d0 = damage;

    // §2.3: nothing struck it (a trap, the world).
    if (!attacker) {
        world.rand();
        write(target->record, HIT_REACTION, 0.5f);
        return land(world, target, nullptr, h, damage, d0);
    }

    // §2.4.
    if ((ability(attacker) == ABILITY_GROUND_ONLY_2 || ability(attacker) == ABILITY_GROUND_ONLY_1) &&
        !(target->record[MOVE_FLAGS] & MOVE_ON_GROUND))
        return false;

    // §2.5.
    if (ability(attacker) == ABILITY_MANA_CHARGE) {
        Burst b;
        b.velocity[2] = 10.0f;
        b.colour[0] = 0.6f, b.colour[1] = 0.6f, b.colour[2] = 1.0f, b.colour[3] = 1.0f;
        b.size = randomized(world, 0.1f, 0.1f);
        b.count = 8;
        b.style = 1;
        b.spread = 3.0f;
        addParticles(world, target->record + POSITION, b);
    }

    // §2.6.
    if (!hittable(target, h.ability)) {
        playSound(world, target->record + POSITION, SOUND_DEFLECT, randomized(world, 0.3f, 1.0f));
        publish(world, numberOn(target, attacker, damage, h.push, 1.0f, KIND_IMMUNE));
        return false;
    }

    // §2.7.
    if (h.last && blocks(target)) {
        const DamageNumber n = numberOn(target, attacker, damage, h.push, 0.5f, KIND_BLOCKED);
        payForBlock(world, target, damage);
        world.numbers.push_back(n.bytes());
        playSound(world, target->record + POSITION, SOUND_DEFLECT, randomized(world, 0.3f, 1.0f));
        if (!world.client)
            nativehit::apply(world, n.bytes());
        return false;
    }

    // §2.8: a mana shield soaks what it can, then armour or resistance, which a high combo
    // pierces.
    float shield = 0.0f;
    for (const nativebuffs::Buff &b : target->buffs)
        if (b[0] == BUFF_MANA_SHIELD)
            shield += nativebuffs::strength(b);
    const float r = comboRatio(attacker);
    const float rc = std::min(r, 1.0f);
    if (shield > 0.0f) {
        const DamageNumber n = numberOn(target, attacker, damage, h.push, 0.5f, KIND_MANA_SHIELD);
        world.numbers.push_back(n.bytes());
        playSound(world, target->record + POSITION, SOUND_MANA_SHIELD, randomized(world, 0.3f, 1.0f));
        if (!world.client)
            nativehit::apply(world, n.bytes());
        damage = damage - shield;
        if (damage <= 0.0f)
            return true;
    }
    const std::vector<nativerules::Buff> stats = nativebuffs::forStats(*target);
    const float defence = h.type == 0 ? nativerules::armor(target->record, stats.data(), stats.size())
                                      : nativerules::resistance(target->record, stats.data(), stats.size());
    damage = std::max(0.0f, damage - defence * (1.0f - rc));
    if (h.type == 7 && damage > 0.0f)
        selfHeal(world, attacker, damage * 0.1f);

    // §2.9: what the combo adds.
    if (r > 0.25f)
        spiritParticles(world, target, attacker, h.push, r, h.flag != 0);
    if (h.flag && r > 0.0f)
        damage = spiritEffects(world, target, attacker, damage, r);
    if (nativebuffs::find(*attacker, BUFF_LIFESTEAL)) {
        selfHeal(world, attacker, damage * 0.5f);
        const float most = maxHealthOf(attacker);
        if (health(attacker) > most)
            setHealth(attacker, most);
    }

    // §2.10.
    if (ability(attacker) == ABILITY_MANA_CHARGE) {
        if (read<float>(target->record, HIT_REACTION) < 0.3f)
            write(target->record, HIT_REACTION, 0.3f);
    } else if (h.last) {
        write(target->record, HIT_REACTION, 1.0f);
    }
    if (h.type == 0) {
        if (h.ability != ABILITY_MANA_CHARGE)
            impactSound(world, target, attacker, h.critical != 0);
    } else if (h.type == 2) {
        playSound(world, target->record + POSITION, SOUND_DEATH_SPECIAL, 1.0f);
    } else {
        playSound(world, target->record + POSITION, SOUND_SPELL_IMPACT, 1.5f);
    }

    return land(world, target, attacker, h, damage, d0);
}

void apply(World &world, const Number &number) {
    const DamageNumber n = DamageNumber::from(number);

    // §3.1.
    Creature *target = lookup(world, n.target);
    if (!target || health(target) <= 0.0f)
        return;
    if (!world.local && hostility(target) == PLAYER)
        return;
    if (n.kind == KIND_BLOCKED)
        return;
    Creature *attacker = lookup(world, n.attacker);
    uint8_t *t = target->record;

    // §3.2: a hurt creature loses its stealth and is knocked out of a stagger.
    if (n.damage > 0.0f) {
        write(t, STEALTH, std::max(read<float>(t, STEALTH) - 0.25f, 0.0f));
        if (ability(target) == ABILITY_STAGGERED) {
            t[ABILITY] = 0;
            write(t, STUN_TIMER, 3000.0f);
        }
    }

    // §3.3, §3.4.
    if (n.damage >= 0.0f && attacker)
        angerAt(world, target, attacker, n.damage);
    if (shieldedFromOthers(world, target, attacker))
        return;
    if (n.kind == KIND_IMMUNE)
        return;

    // §3.5.
    if (n.damage >= 0.0f)
        knockBack(world, target, n);
    if (n.guardLock > 0)
        write(t, GUARD_LOCK, std::max(read<float>(t, GUARD_LOCK), (float)n.guardLock));
    if (n.kind == KIND_DODGE_REWARD) {
        rewardDodge(world, target);
        return;
    }
    if (n.stun > 0)
        write(t, STUN_TIMER, std::max(read<float>(t, STUN_TIMER), (float)n.stun));
    if (world.client && !isLocal(world, target))
        return;
    if (n.damage < 0.0f) {
        heal(world, target, attacker, n.damage);
    } else if (n.damage > 0.0f) {
        if (n.kind == KIND_MANA_SHIELD)
            wearShield(world, target, n.attacker, n.damage);
        else if (canBeDamaged(target) && (!attacker || canBeDamaged(attacker)))
            setHealth(target, health(target) - n.damage);
    }
    if (health(target) <= 0.0f)
        die(world, target);
}

} // namespace nativehit
