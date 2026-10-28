#pragma once
// Damage, as the supported Steam build computes it. Read from the running game's
// code (read-only) and checked hit by hit with ZoneExtractor's hit probe
// (Extract\hits.txt; build/check_hits.py): see docs/combat.md.
//
// The attacker (attack routine, RVA 0x2c5250):
//   damage = multiplier(ability) * power * (1 + 5 * charge^2) * (1.25 - rand() * 0.5 / 32767)
//   doubled when critical. Power is the attack power (0x4fa70), or GetSpellPower for
//   the abilities 31-34 and 96-97. A hit is critical when the attacker's critical
//   chance (0x50d30), less the target's defense / 16 * 0.05, exceeds rand() / 32767.
// The target (hit routine, 0x29eb60):
//   taken = max(0, damage - defense * (1 - min(1, combo / maxCombo)))
//   with defense the armour value, or GetResistance for the magic hits of abilities 31-34.
//   The attacker's combo counts its landed hits; the game resets it 4 s after the
//   last one (and at some ability transitions, not modelled yet).
// Effects, charge, mana, stun: further below.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace combat {
// The game's rand(): 0..32767.
inline int nativeRand(std::mt19937 &random) { return std::uniform_int_distribution<int>(0, 32767)(random); }

// Per-ability damage multiplier (RVA 0x50f20); every other ability is 1.
inline float abilityMultiplier(int ability) {
    switch (ability) {
    case 6: case 27: return .5f;
    case 22: return .1f;
    case 31: case 33: return .4f;
    case 32: case 34: return .6f;
    case 38: case 44: case 58: case 59: case 69: case 160: return 2;
    case 47: case 105: case 106: return .75f;
    case 53: return 3;
    case 56: return .25f;
    case 73: return .2f;
    default: return 1;
    }
}

// Abilities whose power is spell power (the rest use attack power).
inline bool usesSpellPower(int ability) { return (ability >= 31 && ability <= 34) || ability == 96 || ability == 97; }
// Abilities whose hits the target meets with resistance instead of armor.
inline bool magic(int ability) { return ability >= 31 && ability <= 34; }

// Hits before armor stops counting (RVA 0x5f9e0), by the right-hand weapon's id
// (category 3; -1 when there is none). Without one, appearance flag 0x10 gives 15.
inline int maxCombo(int weaponId, unsigned flags2 = 0) {
    switch (weaponId) {
    case -1: return flags2 & 0x10 ? 15 : 30;
    case 3: case 4: case 10: return 50; // dagger, fist, staff
    case 6: return 60;                  // bow
    case 8: return 80;                  // boomerang
    case 11: case 12: return 20;        // wand, bracelet
    case 15: case 16: case 17: return 15; // greatsword, greataxe, greatmace
    default: return 30;
    }
}
constexpr double COMBO_RESET = 4.0; // seconds without a landed hit

inline bool critical(float attackerCritical, float targetDefense, std::mt19937 &random) {
    float roll = (float)nativeRand(random) / 32767;
    return attackerCritical - targetDefense / 16 * .05f > roll;
}

// The damage leaving the attacker, before the target's defense. Abilities 12, 61
// and 63 add nothing besides their charge.
inline float outgoing(float power, int ability, float charge, bool isCritical, std::mt19937 &random) {
    float base = (ability == 12 || ability == 61 || ability == 63) ? 0.f : 1.f;
    float hit = abilityMultiplier(ability) * power * (5 * charge * charge + base);
    hit *= 1.25f - (float)nativeRand(random) * .5f / 32767;
    return isCritical ? hit * 2 : hit;
}

// What the target loses.
inline float taken(float damage, float defense, int combo, int maxCombo) {
    float ignored = std::min(1.f, (float)std::max(combo, 0) / (float)std::max(maxCombo, 1));
    return std::max(0.f, damage - defense * (1 - ignored));
}

// --- Effects (the game's buff list, Creature +0x980) -------------------------------
// Types as the game numbers them (CWSDK's Creature::Buff::buff type).
enum Effect { BULWARK = 1, WAR_FRENZY = 2, CAMOUFLAGE = 3, POISON = 4, MANA_SHIELD = 6, INSTANT_FIRE = 9,
              SWIFTNESS = 12, NINJUTSU = 13, FLAME_RUSH = 16 };
struct Active {
    int type = 0;
    float value = 0;
    double left = 0; // seconds
};
struct Effects {
    std::vector<Active> list;
    // Adding a type already present refreshes it (the game keeps one per type).
    void add(int type, float value, double seconds) {
        for (auto &e : list)
            if (e.type == type) {
                e.value = value, e.left = seconds;
                return;
            }
        list.push_back({type, value, seconds});
    }
    const Active *find(int type) const {
        for (auto &e : list)
            if (e.type == type) return &e;
        return nullptr;
    }
    void update(double dt) {
        for (auto &e : list) e.left -= dt;
        list.erase(std::remove_if(list.begin(), list.end(), [](const Active &e) { return e.left <= 0; }), list.end());
    }
};

// Poison (creature update, 0x2df8ab): each frame its time left (ms) crosses a multiple
// of 400, the target is hit again for the poison's value, through the hit routine
// (hit type 6, not blockable), so armor still counts. 7 hits over its 3 s; none as it
// runs out. True when the step from `before` to `after` seconds left is such a tick.
inline bool poisonTick(double before, double after) {
    return after > 0 && (int)(float)(before * 1000) / 400 != (int)(float)(after * 1000) / 400;
}

// The effect an ability gives its user as it starts (RVA 0x29d3c0, switch on ability - 109).
// Mana Shield's strength is four times the user's spell power.
inline bool skillEffect(int ability, float spellPower, Active &out) {
    switch (ability) {
    case 109: out = {CAMOUFLAGE, 0, 20}; return true;
    case 112: out = {SWIFTNESS, 2, 10}; return true;
    case 113: out = {BULWARK, .5f, 10}; return true;
    case 114: out = {WAR_FRENZY, 0, 10}; return true;
    case 115: out = {MANA_SHIELD, spellPower * 4, 30}; return true;
    case 141: out = {NINJUTSU, 0, 20}; return true;
    case 146: out = {FLAME_RUSH, 2, 10}; return true;
    default: return false;
    }
}

// Charge (+0xba8): the mana held in a charged attack when it is let go. Abilities 6 and
// 12 keep half of it. It multiplies damage by 1 + 5·charge² and is the hit's stun chance.
inline float releasedCharge(int ability, float mana) { return ability == 6 || ability == 12 ? mana * .5f : mana; }

// Mana from a landed hit, once per attack (attack routine): base 0.1 ± 0.05, times the
// ability's length / 500 ms, doubled for ability 59, x1.5 one time in eight, times the
// attacker's mana generation. The caller caps MP at 1.
inline float manaFromHit(int ability, double abilitySeconds, float manaGeneration, std::mt19937 &random) {
    float spread = 1 - 2 * (float)nativeRand(random) / 32767;
    float gain = spread * .05f + .1f;
    if (ability == 59) gain *= 2;
    gain *= (float)(abilitySeconds * 1000 / 500);
    if (nativeRand(random) % 8 == 0) gain *= 1.5f;
    return gain * manaGeneration;
}

// A warrior's landed hit may start War Frenzy for 8 s: chance 0.1 x min(1, length in ms x 0.01).
inline bool warriorFrenzy(double abilitySeconds, std::mt19937 &random) {
    float chance = .1f * std::min(1.f, (float)(abilitySeconds * 1000 * .01));
    return (float)nativeRand(random) / 32767 < chance;
}

// Stun (hit routine): the hit's charge is its chance, or 1 for abilities 13, 59, 69, 70,
// 75, 100 and 103. x0.15 against elite targets (appearance flag 0x40000), x0.1 when a
// creature that is neither a player nor elite strikes, x0.9 always; never through
// Bulwark, nor within 3 s of the last stun. Lasts 1.5 s on a player, 3 s on others.
constexpr unsigned ELITE = 0x40000;
inline float stunChance(int ability, float charge, bool attackerIsPlayer, unsigned attackerFlags2, unsigned targetFlags2) {
    float chance = charge;
    if (ability == 13 || ability == 59 || ability == 69 || ability == 70 || ability == 103) chance = 1;
    if (targetFlags2 & ELITE) chance *= .15f;
    if (!(attackerFlags2 & ELITE) && !attackerIsPlayer) chance *= .1f;
    if (ability == 100 || ability == 75) chance = 1;
    return chance * .9f;
}
inline double stunSeconds(bool targetIsPlayer) { return targetIsPlayer ? 1.5 : 3.0; }

struct Attacker {
    float attack = 0, spell = 0, critical = 0;
    int ability = 0, combo = 0, maxCombo = 30;
    float charge = 0;
    const Effects *effects = nullptr;
};
// Blocking (hit routine 0x29eb60, read 2026-10-22). It is checked first, on the
// incoming damage, before mana shield and armor.
//
// An item's power (0x10ad90 over 0x109c50), for a weapon (category 3):
//   2^(effective rarity + (xorshift32(modifier) % 1000)/2000 + upgrades·0.5/(32 two-handed, else 16))
// times 2 for daggers and fists (ids 3, 4), 8 for the two-handed ids below (a wand,
// 11, is 6 on a non-player) and 4 for the rest, shields (13) included. An item
// bound to another region has effective rarity -1 (0x109720), which CubeVulkan
// doesn't track yet: items count as in their region.
inline bool twoHanded(int id) { return id >= 0 && id <= 23 && ((0x878de0u >> id) & 1); }
inline float itemPower(int category, int id, int rarity, unsigned modifier, int upgrades = 0, bool player = true) {
    if (category != 3)
        return 0;
    uint32_t x = modifier;
    x ^= x << 13, x ^= x >> 17, x ^= x << 5;
    float base = std::pow(2.f, rarity + (float)(x % 1000) / 2000 + upgrades * .5f / (twoHanded(id) ? 32 : 16));
    if (id == 3 || id == 4) return base * 2;
    if (id == 5 || id == 13 || !twoHanded(id)) return base * 4;
    return base * (id == 11 && !player ? 6 : 8);
}
struct HeldItem {
    int category = 0, id = 0, rarity = 0;
    unsigned modifier = 0;
};
// How much a creature's block holds (0x50370): 5, plus a shield's power x4 (x6 for a
// guardian warrior), or else each hand's weapon power; then x2 for appearance flag
// 0x200, x1.2 for 0x20000, x4 for 0x40000 (elite), x1.1 for 0x4000000.
inline float blockFactor(const HeldItem &left, const HeldItem &right, int cls, int spec, unsigned flags2, bool player = true) {
    float f = 5;
    if (left.category == 3 && left.id == 13)
        f += itemPower(3, 13, left.rarity, left.modifier, 0, player) * (cls == 1 && spec == 1 ? 6 : 4);
    else
        f += itemPower(left.category, left.id, left.rarity, left.modifier, 0, player) +
             itemPower(right.category, right.id, right.rarity, right.modifier, 0, player);
    if (flags2 & 0x200) f *= 2;
    if (flags2 & 0x20000) f *= 1.2f;
    if (flags2 & 0x40000) f *= 4;
    if (flags2 & 0x4000000) f *= 1.1f;
    return f;
}
// Guarding (0x5ed30): never while stunned; ability 9 or 110, the warrior's whirl
// (95) while it plays, and a guardian warrior's 60 and 64.
inline bool guarding(int ability, int cls, int spec, bool stunned) {
    if (stunned) return false;
    return ability == 9 || ability == 110 || ability == 95 || ((ability == 60 || ability == 64) && cls == 1 && spec == 1);
}
// Block power (+0x184, -1..1) refills by 0.5 a second while not guarding, or while
// guarding with ability 95 or 159 (creature update, 0x2dfe2b: ms / 2000).
inline void refillBlock(float &block, double dt, int ability, bool isGuarding) {
    if (!isGuarding || ability == 95 || ability == 159)
        block = std::min(1.f, block + (float)dt * .5f);
}

struct Defender {
    float armor = 0, resistance = 0;
    const Effects *effects = nullptr;
    // Blocking: the defender's block power (null: cannot block), whether it guards,
    // its ability, block factor and mana (gained from blocks) with its generation.
    float *block = nullptr;
    bool guarding = false;
    int ability = 0;
    float blockFactor = 5;
    float *mana = nullptr;
    float manaGeneration = 0;
};
struct Hit {
    float damage = 0;  // what the target loses
    bool critical = false;
    bool absorbed = false; // the target's mana shield took it all
    float healed = 0;  // the attacker's lifesteal (War Frenzy: half the damage)
    bool poisons = false; // ability 18: Poison for 3 s worth 0.2 of the damage before defense
    float poison = 0;
    bool blocked = false; // the target guarded: no damage, its block power paid instead
};
// The target's side of a hit already worth `out` (blocking, mana shield, armor, Bulwark).
inline Hit takeHit(float out, const Attacker &a, const Defender &d);
inline Hit hit(const Attacker &a, const Defender &d, std::mt19937 &random) {
    float defense = magic(a.ability) ? d.resistance : d.armor;
    const bool isCritical = critical(a.critical, defense, random);
    float power = usesSpellPower(a.ability) ? a.spell : a.attack;
    float out = outgoing(power, a.ability, a.charge, isCritical, random);
    Hit h = takeHit(out, a, d);
    h.critical = isCritical;
    return h;
}
inline Hit takeHit(float out, const Attacker &a, const Defender &d) {
    float defense = magic(a.ability) ? d.resistance : d.armor;
    Hit h;
    // A guarding target with block power left takes nothing. Its block power drops by
    // the damage over its block factor (half during the whirl, 95), down to -1, and
    // it gains max(0, (1 - that) x 0.5) x mana generation in mana, up to 1.
    if (d.block && d.guarding && *d.block > 0) {
        float drain = out / d.blockFactor * (d.ability == 95 ? .5f : 1);
        *d.block = std::max(-1.f, *d.block - drain);
        if (d.mana)
            *d.mana = std::min(1.f, *d.mana + std::max(0.f, (1 - drain) * .5f) * d.manaGeneration);
        h.blocked = true;
        return h;
    }
    // The target's mana shield is subtracted next; a hit it covers does nothing.
    float damage = out;
    if (d.effects)
        for (auto &e : d.effects->list)
            if (e.type == MANA_SHIELD) damage -= e.value;
    if (damage <= 0) {
        h.absorbed = true;
        return h;
    }
    h.damage = taken(damage, defense, a.combo, a.maxCombo);
    if (a.effects && a.effects->find(WAR_FRENZY)) // half of it, before the target's Bulwark
        h.healed = h.damage * .5f;
    if (d.effects) // Bulwark scales what lands
        for (auto &e : d.effects->list)
            if (e.type == BULWARK) h.damage *= e.value;
    if (a.ability == 18)
        h.poisons = true, h.poison = out * .2f;
    return h;
}
} // namespace combat
