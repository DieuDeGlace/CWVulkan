# Combat

How the supported Steam build (timestamp 5d93ab0d) turns an attack into lost health.

**Sources**
- The rules were read from the running game's code with no mods loaded. Only `CubeModLoader` was present, plus ZoneExtractor, which reads without changing anything.
- They were then checked hit by hit against the game. The SteamStub-protected executable is never decrypted or changed on disk.
- Implemented in `src/Combat.h` and tested by `damageMatchesNativeHits` in `EngineTests`.

## The rules

**The attacker** (attack routine, RVA 0x2c5250):

    damage = multiplier(ability) × power × (1 + 5·charge²) × (1.25 − rand()·0.5/32767)

- **Power:** the attack power (0x4fa70). Abilities 31–34 and 96–97 use GetSpellPower (0x65c70) instead.
- **Abilities 12, 61 and 63** use `5·charge²` with no `+1`.
- **Critical hits** double the damage. A hit is critical when the attacker's critical chance (0x50d30), minus the target's `defense / 16 × 0.05`, is more than `rand()/32767`.
- **Multipliers by ability** (0x50f20; every other ability is 1):

| Ability | Multiplier |
|---|---|
| 6, 27 | 0.5 |
| 22 | 0.1 |
| 31, 33 | 0.4 |
| 32, 34 | 0.6 |
| 38, 44, 58, 59, 69, 160 | 2 |
| 47, 105, 106 | 0.75 |
| 53 | 3 |
| 56 | 0.25 |
| 73 | 0.2 |

**The target** (hit routine, 0x29eb60):

    taken = max(0, damage − defense × (1 − min(1, combo / maxCombo)))

- **Defense** is the armour value, or GetResistance against the magic hits of abilities 31–34. Armor is **subtracted**: a weak attacker can do nothing to a well-armored target.
- **Combo** is the attacker's count of landed hits. Each one adds 1. The game resets it 4 s after the last landed hit, and at some ability transitions.
- **maxCombo** (0x5f9e0) comes from the right-hand weapon:

| Right-hand weapon | maxCombo |
|---|---|
| Dagger, fist, staff | 50 |
| Bow | 60 |
| Boomerang | 80 |
| Wand, bracelet | 20 |
| Greatsword, greataxe, greatmace | 15 |
| Any other weapon | 30 |
| None | 15 with appearance flag 0x10, else 30 |

**Inputs to the attack power**, for the record:

    (5 + Σ item attack) × (1 + 0.2·[+0x18c])

- **Abilities:** ×50 for 0x60, ×2 for 0x64, ×5 for 0x67, ×3 for 0x6c.
- **Appearance flags (flags2):** ×1.5 for 0x200, ×1.2 for 0x20000, ×2 for 0x40000, ×1.1 for 0x4000000.
- **Effects:** ×value for effect 0x17; ×(1 + 0.1·value) for Ancient Power (0x21).

CubeVulkan uses the game's own stat values from the snapshots and creature records, not this formula.

## Validation (2026-10-22)

ZoneExtractor's hit probe hooks the hit routine and logs each call to `Extract\hits.txt`:
- the damage, critical flag, hit type and ability;
- the attacker's combo, max combo, attack and spell power;
- the target's armor and resistance;
- the health it lost.

`tools/CheckHits.py <game folder> [since-ms]` compares the log with the rules.

The test warrior fought several monsters:
- **Attacker side:** all 276 monster hits were within the 0.75–1.25 roll of the game's own the attack power, and the 3 criticals were doubled.
- **Defense side, monster hits:** where one hit could be matched to one health change, the prediction matched to 0.005, e.g. 52.477 predicted, 52.480 lost. The combo term is visible: 39.094 predicted with combo 1, 39.043 without it, 39.090 lost.
- **Defense side, player hits:** each health change matched exactly while the combo climbed (2.5313, 1.1473, 0.9547, 4.5157).
- **Too much armor:** the warrior's 7-damage hits against level-2 monsters with 24 armor took no health, as the rule predicts.

The roughly 34 unmatched hits came from several monsters striking within the same 0.4 s, so the log could not tell which hit caused which health change.

## Effects, blocking, charge, mana and stun

Also read from the hit and attack routines; implemented in `Combat.h`, tested by `combatEffects`.

**Effects** are the game's buff list at +0x980. Class skills start these (RVA 0x29d3c0, by ability):

| Ability | Effect | Value | Duration |
|---|---|---|---|
| 109 | Camouflage (3) | | 20 s |
| 112 | Swiftness (12) | 2 | 10 s |
| 113 | Bulwark (1) | 0.5 | 10 s |
| 114 | War Frenzy (2) | | 10 s |
| 115 | Mana Shield (6) | 4 × spell power | 30 s |
| 141 | Ninjutsu (13) | | 20 s |
| 146 | Flame Rush (16) | 2 | 10 s |

A fire cast (31) may also give Instant Fire (9) for 30 s.

**In a hit:**
- **Mana Shield:** subtracted from the damage first; a hit it covers does nothing. The hit routine does not use the shield up.
- **Bulwark:** multiplies what lands by its value, and prevents stuns.
- **War Frenzy:** heals the attacker half the damage before the target's Bulwark. A warrior's landed hit can start it for 8 s, with chance `0.1 × min(1, ability length in ms × 0.01)`.
- **Ability 18:** poisons the target (effect 4) for 3 s, worth 0.2 × the damage before defense.
- **Poison ticks** (creature update, 0x2df8ab): every frame in which the effect's time left (ms) crosses a multiple of 400, the target is hit again for the poison's value. The hit goes through the hit routine as hit type 6 and can't be blocked, so armor still counts. That makes 7 hits over 3 s, none as it runs out.
- **Effect timers** run in milliseconds. Effect 17 heals max HP × its value each time its time left crosses a whole second.
- **Hit type 7:** heals the attacker 10%.

**Blocking** is checked first, on the incoming damage, before mana shield and armor (and only when the caller marks the hit as blockable). A target blocks when it is **guarding** with **block power** (+0x184, −1 to 1) above 0, or plays abilities 72, 73 or 130 inside their window. It then takes no health damage. Instead:
- block power drops by `damage / block factor` (half that during the whirl, 95; nothing for 72/73/130), down to −1;
- the target gains `max(0, (1 − that)·0.5) × mana generation` in mana, up to 1.

**Guarding** (0x5ed30): never while stunned. Ability 9 or 110, the warrior's whirl (95) while it plays, and a Guardian warrior's 60 and 64.

**Block power refills** by 0.5 a second (frame ms / 2000, creature update 0x2dfe2b) while not guarding, or while guarding with 95 or 159. From −1 it takes 2 s to block again, 4 s to be full.

**Block factor** (0x50370):

    5 + shield power × 4 (× 6 for a Guardian)      with a shield (3, 13) in the left hand
    5 + left weapon power + right weapon power      otherwise

then ×2 for appearance flag 0x200, ×1.2 for 0x20000, ×4 for elites (0x40000), ×1.1 for 0x4000000. A player's item must pass the class check (0x1094d0); other creatures' items always count.

**Item power** (0x10ad90 over 0x109c50), for weapons (category 3):

    2^(effective rarity + (xorshift32(modifier) % 1000) / 2000 + upgrades × 0.5 / (32 two-handed, else 16))

- times 2 for daggers and fists (ids 3, 4); 8 for the two-handed ids 6, 7, 8, 10, 11, 15, 16, 17, 18, 23 (a wand, 11, is 6 on a non-player); 4 for the rest, shields included.
- The xorshift is 13, 17, 5. Upgrades are the item's spirit count (+0x9d), 0 on Steam items.
- **Effective rarity** (0x109720): an item bound to a region other than the wearer's is −1, unless it is within one region and its modifier divides by `(rarity+1) × 10`. Non-player creatures always count as in region.

**Charge** (+0xba8) is the mana held in a charged attack when it is let go:
- abilities 6 and 12 keep half;
- it multiplies damage by `1 + 5·charge²`;
- it is the hit's stun chance.

**Mana from a landed hit**, once per attack:

    (0.1 ± 0.05) × ability length / 500 ms

- doubled for ability 59;
- ×1.5 one time in eight;
- times the attacker's mana generation (0x5f8d0), capped at 1.

**Stun:**
- **Chance:** the hit's charge. It is 1 for abilities 13, 59, 69, 70 and 103 (and 75 and 100 regardless).
- **Modifiers:**
  - ×0.15 against elite targets (flags2 0x40000);
  - ×0.1 when a creature that is neither a player nor elite strikes;
  - ×0.9 always.
- **Exceptions:** never through Bulwark, and not within 3 s of the target's last stun.
- **Length (0x5fc20):** 1.5 s on a player, 3 s on others.

**In CubeVulkan:**
- Effects are kept for the player and each monster.
- Skills with the abilities above start their effects. None of the recorded class moves uses them yet.
- War Frenzy procs and heals.
- Bulwark and Mana Shield defend.
- The ranger's salvo release carries the mana it spent as charge.
- Stuns stop monsters and the player.
- Mana per hit uses the rule when a capture records mana generation. The extractor now writes `managen=`; older captures keep the logged +0.2.

## Not modelled yet

- **The player's class bonus:** the local player's damage times `1 + 0x510f0(attacker)` when 0x65750 returns 6.
- **Ability 22's extra factor.**
- **Projectiles:** ability-0 hits of type 2 are computed elsewhere.
- **Combo resets at ability transitions.**
- **Blocking, partly:** the player blocks as above. Monsters never guard yet (their abilities don't include 9, 95 or 110 in recordings), items count as in their region, the "blockable" flag is taken as always set, and abilities 72/73/130 aren't handled.
- **Knockback** (0x5f180, by ability; partly read): none for hostility 3 or 6, and none for abilities 31–34.
  - Fixed amounts: 10 for abilities 12 and 96; 6 for 18 and 21; 30 for 22 and 97; 5 for 55; 4 for 6.
  - The rest are a number divided by 0x66460: 6 for 2, 3, 10, 14–16, 61, 62; 3 for 4, 5, 7, 8, 19, 20, 63; 5 for 66, 67; 2 for everything else.
  - 0x66460 starts at 1 and grows with the combo for berserkers, ×0.5 (and for two other class specs), plus a term from the left-hand item (0x10a490).
  - How the push moves the target is not read yet.
- **Item spirit effects** (cube materials 0x80–0x83). Steam items don't carry them.
- **Which abilities a monster uses:** CubeVulkan uses the ability recorded most often for that monster.

The fight logged in the user's modded game on 2026-10-21 does not follow these rules. CubeModLoader lets mods hook the armour value, GetCritical and the attack power, and PyroProgression was installed then. Pyro mode may need the mod's own stat changes measured separately.
