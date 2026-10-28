#pragma once
// The game's stat rules (docs/specs/stat-rules.md): the numbers it derives from a creature
// record (level, class, flags, worn items, buffs) or from an item record, and the small tests
// built on them. Everything here works on game-layout records: a creature
// (nativepose::CREATURE_SIZE bytes) and its items (0xa0 bytes each; the eleven equipment
// slots from +0x278). The game routine each function follows is cited by its RVA.
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>

namespace nativerules {

// Whether the creature has a buff of a type (a walk of its list at creature+0x980).
using HasBuff = std::function<bool(int type)>;

// A buff as the stat rules read it: its type and strength, in the creature's list order.
struct Buff {
    int type = 0;
    float value = 0;
};

// --- The item's own level and power (spec §1) --------------------------------------------
// An item record: type +0, subtype +4, modifier (its seed) +8, region +0xc (two ints),
// rarity +0x14, the wrapped type of a formula +0x18, material +0x1c, attribute count +0x9d.

// The item's level against a region: its rarity at home (or near home for some), else -1.
int itemLevel(const uint8_t *item, const int32_t region[2]);                     // 0x109720
// Whether a player of `cls` (1..4) may use the item.
bool classMayUse(const uint8_t *item, int cls);                                  // 0x1094d0
// Whether the creature may use the item: anyone but a player, or a player of a fitting class.
bool mayUse(const uint8_t *creature, const uint8_t *item);                       // 0x50640
// 2 to the item's level plus a small part of its seed (and `upgrades` / 32 or / 64).
float itemPower(const uint8_t *item, const uint8_t *creature, int upgrades = 0); // 0x109c50

// --- Per-item stats (§2); each takes the creature only to judge the item's region -------
float weaponDamage(const uint8_t *item, const uint8_t *creature);      // 0x10ad90, with upgrades
float baseWeaponDamage(const uint8_t *item, const uint8_t *creature);  // 0x108f20, without
float itemHealth(const uint8_t *item, const uint8_t *creature);        // 0x1097b0
float itemArmor(const uint8_t *item, const uint8_t *creature);         // 0x108d50
float itemResistance(const uint8_t *item, const uint8_t *creature);    // 0x10a1e0
float itemCritical(const uint8_t *item, const uint8_t *creature);      // 0x1090f0
float itemHaste(const uint8_t *item, const uint8_t *creature);         // 0x10a490
float itemStaminaRegen(const uint8_t *item, const uint8_t *creature);  // 0x109f30

// Consumables and prices (§2.6, §2.7).
float healAmount(const uint8_t *item);   // 0x109460
int useTime(const uint8_t *item);        // 0x108fa0, in ms
float demonDamage(const uint8_t *item);  // 0x109290
int itemPrice(const uint8_t *item);      // 0x109d30
int sellPrice(int price);                // 0x10a390
// A weapon of the two-handed set (§0.3, subtypes in mask 0x878de0).
bool twoHandedWeapon(const uint8_t *item);
// Gear whose modifier is a multiple of (rarity + 1) × 10: it counts in the regions around home
// too and its price doubles (§1.2, §2.7).
bool renowned(const uint8_t *item);

// --- Mod hooks ---------------------------------------------------------------------------
// The mod loader's calls at the end of the creature stat rules (CubeModLoader's
// OnCreatureHPCalculated, OnCreatureAttackPowerCalculated, OnCreatureSpellPowerCalculated,
// OnCreatureArmorCalculated and OnCreatureResistanceCalculated), for gameplay modules such as
// OverworldEvents and PyroProgression: none by default. As the loader calls every mod's
// hook, each set added is called in turn, in the order added.
struct Callbacks {
    std::function<void(const uint8_t *creature, float &hp)> maxHealth;       // 0x5fac0
    std::function<void(const uint8_t *creature, float &power)> attackPower;  // 0x4fa70
    std::function<void(const uint8_t *creature, float &power)> spellPower;   // 0x65c70
    std::function<void(const uint8_t *creature, float &armor)> armor;        // 0x4f080
    std::function<void(const uint8_t *creature, float &value)> resistance;   // 0x64d90
};
// Returns a handle for removeCallbacks.
int addCallbacks(Callbacks callbacks);
void removeCallbacks(int handle);

// --- Offence (§3, §4) --------------------------------------------------------------------
// Which weapons attack and spell power add up: those the current ability strikes with
// (0x4fd40), or both hands whatever the ability (0x52ce0, the character sheet's).
enum class Weapons { InUse, Both };

float attackPower(const uint8_t *creature, const Buff *buffs, size_t buffCount,
                  Weapons weapons = Weapons::InUse);                                 // 0x4fa70
float spellPower(const uint8_t *creature, const Buff *buffs, size_t buffCount,
                 Weapons weapons = Weapons::InUse);                                  // 0x65c70
// How much an ability's projectiles and effects hurt, as a factor.
float abilityDamageFactor(int ability);                                              // 0x50f20
// The chance that a hit on `target` (none: no defence) is critical; `magic` picks the
// target's resistance over its armour. Not clamped.
float critChance(const uint8_t *creature, const HasBuff &hasBuff, const uint8_t *target, const Buff *targetBuffs,
                 size_t targetBuffCount, bool magic);                                // 0x50d30
// The haste factor: 1, plus combo and item haste, doubled by a type-0x0c buff.
float attackSpeed(const uint8_t *creature, const HasBuff &hasBuff);                  // 0x66460
// The combo at which the combo bonus is full.
int comboThreshold(const uint8_t *creature);                                         // 0x5f9e0
// A proc's chance after the second weapon's adjustment; the proc happens when
// float(rand) / 32767 is below it (procHappens, with the caller's draw).
float procChance(const uint8_t *creature, float chance);                             // 0x5e820
bool procHappens(const uint8_t *creature, float chance, int rand);                   // 0x5e820

// --- Defence (§5) ------------------------------------------------------------------------
float maxHealth(const uint8_t *creature, const Buff *buffs = nullptr, size_t buffCount = 0); // 0x5fac0
float armor(const uint8_t *creature, const Buff *buffs, size_t buffCount);                  // 0x4f080
float resistance(const uint8_t *creature, const Buff *buffs, size_t buffCount);             // 0x64d90
float blockValue(const uint8_t *creature);                                                  // 0x50370
// What the target's armour (or resistance) takes off a critical chance.
float critDefence(const uint8_t *target, const Buff *buffs, size_t buffCount);              // 0x50ea0
float magicCritDefence(const uint8_t *target, const Buff *buffs, size_t buffCount);         // 0x5f740

// --- Mana and stamina (§6) ---------------------------------------------------------------
float manaFactor(const Buff *buffs, size_t buffCount);                  // 0x5f8d0
float staminaFactor(const uint8_t *creature);                           // 0x647b0
float manaCost(int ability, const HasBuff &hasBuff);                    // 0x5f7c0
float staminaCost(int ability, const HasBuff &hasBuff);                 // 0x65ea0

// --- Ability durations (§7) --------------------------------------------------------------
// An ability's three parts in ms (a negative ability: the creature's current one, +0x68),
// and their sum for the current ability.
int abilityEffectTime(const uint8_t *creature, int ability, const HasBuff &hasBuff);   // 0x4ead0
int abilityMiddleTime(const uint8_t *creature, int ability, const HasBuff &hasBuff);   // 0x660d0
int abilityRecoveryTime(const uint8_t *creature, int ability, const HasBuff &hasBuff); // 0x5e9f0
int abilityDuration(const uint8_t *creature, const HasBuff &hasBuff);                  // 0x4f220
// How long a hit's stun lasts (ms), and the fixed skill level the upkeep passes on.
int stunDuration(const uint8_t *creature);                                             // 0x5fc20
int skillLevel();                                                                      // 0x65690

// --- Tests (§8) --------------------------------------------------------------------------
// Whether the creature may start `ability` now; `cooldowns` is its map at +0xb78 (none: no
// cooldowns).
bool canStartAbility(const uint8_t *creature, int ability, const HasBuff &hasBuff,
                     const std::map<int32_t, float> *cooldowns);                       // 0x507b0
// Whether two creatures may fight each other.
bool hostile(const uint8_t *a, const uint8_t *b);                                      // 0x50550
bool canBeDamaged(const uint8_t *creature);                                            // 0x50520
bool usesMana(const uint8_t *creature);                                                // 0x5edd0
bool isRanged(const uint8_t *creature);                                                // 0x5eca0
bool holdsTwoHanded(const uint8_t *creature);                                          // 0x5e9b0
bool isGuarding(const uint8_t *creature, const HasBuff &hasBuff);                      // 0x5ed30
// The ability on the class's "Ultimate" button (0 for none).
int classSkill(const uint8_t *creature);                                               // 0x656e0
// The race's group (0..7, §8.4).
int raceGroup(int race);                                                               // 0x65750

// --- Skills and level curves (§9, §10) ---------------------------------------------------
enum class Skill { Climbing, Swimming, Diving, Riding, HangGliding, Sailing, LightRadius };
float skillValue(const uint8_t *creature, Skill skill); // 0x50b60, 0x663f0, 0x51190, 0x652d0, 0x5e3d0, 0x65330, 0x5f6e0

float cloneAuraStrength(int n);     // 0x61150
int cloneAuraDuration(int n);       // 0x65460
float stealthGain(int n);           // 0x656a0
float glideFactor(int n);           // 0x504e0
int experienceForLevel(int level);  // 0x5fa80: the experience the next level takes
float levelFactor(float x);         // 0x61000

} // namespace nativerules
