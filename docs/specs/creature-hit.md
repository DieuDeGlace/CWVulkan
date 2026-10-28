# A hit on a creature (0x29eb60 and 0x29dcf0)

Behavioural specification of Steam Cube World, written from the decompiled executable, its
assembly and the binary's constants. Routines are cited by RVA (image base 0x140000000);
constants are cited by the RVA of the value in the binary. Offsets are byte offsets into the
record named. World positions are signed 64-bit integers in 1/65536 of a block. Times are
milliseconds. "rand" is the C runtime's `rand()` (0..32767), shared with the rest of the game.

Shared vocabulary (buff records, the sound request, the buff-change notice, **give buff**
0x65390 and the **hostility test** 0x50550) is defined in `skills-and-interactions.md` §0 and
is used here unchanged.

Two routines are covered:

* **The hit routine, 0x29eb60.** Called when something strikes a creature. It decides whether
  the strike is ignored, deflected as immune, blocked, soaked by a mana shield, or lands; it
  applies armour, combo-dependent extras (weapon spirit effects), lifesteal, the stun roll and
  the hit's sounds and particles; and it writes one **damage number** record describing the
  outcome. It returns a byte: 1 when the hit landed (including a hit fully soaked by a mana
  shield), 0 otherwise.
* **The apply routine, 0x29dcf0.** Given a damage number record, it applies it to the target:
  aggro, knock-back, stun, health, mana-shield wear, and death (drops, rewards, kill credit,
  aggro clean-up). 0x29eb60 calls it for the records it writes (unless this world is a remote
  client); 0x29d980 and the projectile routine 0x2dba40 also call it.

**The damage formula and the critical roll are not in these routines.** The caller passes the
final pre-defence damage and a "critical" flag (see §1.2 for what each caller does). 0x29eb60
applies only defensive and situational modifiers.

---

## 0. Records and fields

### 0.1 Creature fields read or written

| Offset | Type | Meaning (as used here) |
|---|---|---|
| +0x08 | i64 | creature id |
| +0x10, +0x18, +0x20 | i64 ×3 | position |
| +0x34, +0x38, +0x3c | f32 ×3 | velocity (zeroed by a stunning hit) |
| +0x4c, +0x50, +0x54 | f32 ×3 | knock-back velocity (replaced by a stronger push) |
| +0x5c | byte | movement flags: bit 0 on the ground; bit 5 (0x20) makes the creature immune (§2.3) |
| +0x60 | byte | hostility kind (0 player; 1 hostile; 6, 7, 8 used below) |
| +0x64 | i32 | race |
| +0x68 | byte | current ability id |
| +0x6c | f32 | time spent in the current ability (ms) |
| +0x70 | i32 | combo (count of landed hits) |
| +0x80 | u32 | appearance/state flags: bits 4, 9, 13 (0x2000), 17, 18, 26 used |
| +0x84 | f32 | first size factor |
| +0x128 | u16 | state flags: bit 7 (0x80) immune; bit 8 (0x100) gives a reward on death |
| +0x12c | f32 | dodge-roll timer (read by 0x4ee50) |
| +0x130 | f32 | stun timer (> 0 stunned; counts below zero afterwards) |
| +0x134 | f32 | a second timer; while > 0 the creature cannot guard |
| +0x138 | f32 | a timer raised by "ice" spirits (cap 5000) |
| +0x13c | f32 | a timer raised by "wind" spirits on the attacker (cap 5000) |
| +0x144 | i32 | level |
| +0x14c, +0x14d | byte | class (1..4), specialisation |
| +0x150 | i32 ×2 | for players: the region whose item levels apply (§5.2) |
| +0x158 | f32 | a secondary mana value, kept ≤ mana |
| +0x180 | f32 | health |
| +0x184 | f32 | block power (−1..1) |
| +0x188 | f32 | mana (0..1) |
| +0x18c | f32 | a value reduced by 0.25 per damaging hit, floor 0 |
| +0x318, +0x3b8, +0x458, +0x4f8, +0x598, +0x638, +0x6d8, +0x778, +0x818 | item ×9 (0xa0 each) | equipment: +0x638 weapon slot A, +0x6d8 weapon slot B, +0x3b8 chest (item type 4), +0x458 (type 6), +0x4f8 (type 5), +0x598 (type 7), +0x318 (type 8), +0x778 and +0x818 (type 9) |
| +0x8b8 | item | an extra carried item (drops only) |
| +0x968 | i64 | for players: the owner id stamped on reward drops |
| +0x980 | list | buffs (type byte +0, strength f32 +4, duration f32 +8, source id i64 +0x10) |
| +0x994 | f32 | hit-reaction value (set to 0.5, 0.3 or 1.0) |
| +0x9d8 | i64 | id of a linked creature (killed together with this one) |
| +0x9e0 | i64 | id of the last creature this one hit or was hit by |
| +0x9f0 | vector | inventory tabs (each tab a vector of 0xa4-byte slots: i32 count, then an item) |
| +0xb88 | map i64 → f32 | aggro: creature id → anger |
| +0xb98 | map i64 → f32 | hits dealt: creature id → number of hits |

Item record fields used (0xa0 bytes): +0 type byte (3 weapon, 4 chest, 5, 6, 7 armour pieces,
8, 9 accessories, 11, 12 coins, 15, 24); +4 i32 subtype; +8 u32 seed; +0xc, +0x10 i32 region;
+0x14 byte rarity; +0x1c byte material; +0x20 an array of 4-byte spirit entries whose first
byte is a material (0x80..0x83 are the four spirit materials); +0x9d byte count of entries.

**Two-handed weapon subtypes**: the set {5, 6, 7, 8, 10, 11, 15, 16, 17, 18, 23} (bit mask
0x878de0 over subtypes 0..23). It is used in several places below.

### 0.2 World fields

| Offset | Meaning |
|---|---|
| +0x20 | a map searched only by dead code in §3.6 step 7 |
| +0x40 | list of all creatures, in world order |
| +0x90 / +0x98 | list of **damage numbers** (0x48-byte records) / its count |
| +0xa0 / +0xa8 | list of **sound requests** (0x30 bytes) / count |
| +0xb0 / +0xb8 | list of **particle bursts** (0x48 bytes) / count |
| +0x120 / +0x128 | list of **kill credits** (0x10 bytes) / count |
| +0x140 / +0x148 | list of **aggro notices** (0x18 bytes) / count |
| +0x150 / +0x158 | list of **buff-change notices** (0x30 bytes) / count |
| +0x358 | byte: remote-client flag |
| +0x448 | the local player (may be null) |
| +0x450 | map id → creature (lookup 0xe720 returns null when absent) |

### 0.3 The damage number record (0x48 bytes, appended to world+0x90)

| Offset | Type | Meaning |
|---|---|---|
| +0x00 | i64 | attacker id (0 when there is no attacker) |
| +0x08 | i64 | target id |
| +0x10 | f32 | damage (negative = healing) |
| +0x14 | byte | critical |
| +0x15..+0x17 | — | padding, not written |
| +0x18 | i32 | stun duration (ms); 0 = no stun |
| +0x1c | i32 | second timer for target +0x134; 0x29eb60 always writes 0 |
| +0x20 | i64 ×3 | position the number appears at |
| +0x38 | f32 ×3 | push (knock-back) vector |
| +0x44 | byte | the hit's spirit flag (argument 6) on the main record, else 0 |
| +0x45 | byte | **kind**: 0 normal, 1 blocked, 3 immune, 4 dodge reward (not produced here), 5 mana shield |
| +0x46 | byte | 1 when the attacker's ability is 0x1d, else 0 |
| +0x47 | — | padding, not written |

### 0.4 Other produced records

* **Sound request** (world+0xa0, 0x30 bytes): position (3 × i64), sound id i32 +0x18, pitch
  f32 +0x1c, volume f32 +0x20 (always 1.0 here), +0x24 padding (not written), 8 zero bytes
  +0x28.
* **Particle burst** (world+0xb0, 0x48 bytes): position (3 × i64) +0, velocity f32 ×3 +0x18,
  colour f32 ×4 +0x24, size f32 +0x34, count i32 +0x38, style i32 +0x3c, spread f32 +0x40,
  +0x44..+0x47 not written by either producer here.
* **Buff-change notice** (world+0x150, 0x30 bytes): source id +0, target id +8, the 0x18-byte
  buff +0x10, flag byte +0x28 (always 0 here).
* **Aggro notice** (world+0x140, 0x18 bytes): the angering creature's id +0, the angered
  creature's id +8, the anger value f32 +0x10, +0x14 padding.
* **Kill credit** (world+0x120, 0x10 bytes): credited creature id +0 (0 = nobody), dead
  creature's id +8.

### 0.5 Numeric conventions

All arithmetic is single-precision float (SSE scalar), evaluated in exactly the order written
in this document; integers are converted to float exactly (rand values, combo, counts). The
only double-precision arithmetic is the distance test 0xf3e20 (§4.2). Where a step says
"rand × k / 32767 + c", the order is: convert, multiply by k, divide by 32767.0 (0x42e5f0),
then add c. "max(a, b)"/"min(a, b)" below are exact comparisons; where it matters which
operand wins on equality it does not change the value.

---

## 1. Inputs of the hit routine

### 1.1 Arguments (in call order)

| # | Name used here | Type | Meaning |
|---|---|---|---|
| 1 | world | pointer | the world |
| 2 | target | creature | the creature struck |
| 3 | attacker | creature or null | who struck |
| 4 | damage | f32 | final pre-defence damage, critical doubling already applied |
| 5 | critical | byte | non-zero for a critical hit |
| 6 | spirit flag | byte | non-zero lets the attacker's weapon spirits (§2.8) act; copied to record +0x44 |
| 7 | stun chance | f32 | the base chance of stunning (the hit's "charge") |
| 8 | (pointer) | — | not read |
| 9 | push | pointer to f32 ×3 | push vector |
| 10 | hit type | i32 | 0 physical (armour applies); any other value: resistance applies. 2 and 7 have extra effects |
| 11 | ability | i32 | the ability that produced the hit (0x12, 0x1d, 0x8c, 0x94 have effects) |
| 12 | (pointer) | — | not read |
| 13 | blockable | byte | non-zero when the target may block; also affects +0x994 |

### 1.2 What the callers pass (context only)

* Projectile hit routine 0x2dba40: the projectile's damage, doubled when
  rand / 32767 < 0x50d30(attacker, target, …) (the critical chance); spirit flag from the
  projectile; hit type from the projectile; ability 0; blockable 1.
* 0x2a45e0: hit type 2, blockable 1.
* Creature upkeep 0x2de4c0 (poison ticks): attacker = the poison buff's source creature,
  damage = the buff's strength, critical 0, spirit flag 0, hit type 6, blockable 0.
* 0x2e3510: no attacker, hit type 1 when its ability is 0x11 else 0, blockable 1.

---

## 2. The hit routine, 0x29eb60

### 2.1 Early rejection (return 0, no effects)

Return 0 at once when any of the following holds:

1. target health (+0x180) ≤ 0;
2. target hostility kind is 8;
3. target race is 0x114;
4. the world is a remote client (+0x358 non-zero) **and** (the target is the local player,
   or the attacker is not the local player).

### 2.2 Race bonus

If there is an attacker, the attacker is the local player, and the race category of the
target's race (0x65750) is 6 — races 0x49, 0x52, 0x55, 0x6e, 0xe1, 0xe2, 0xe5 — then:

* count = 0x510f0(attacker): if the attacker has at least 2 inventory tabs, the number of
  slots in the **first** tab whose count is > 0 and whose item has type 24 and subtype 12
  (each such slot adds 1.0, in float, regardless of the stack size); otherwise 0.
* damage = damage × (count + 1.0).

Call the damage after this step **D0**. It is remembered for the poison of §2.11.

### 2.3 No attacker

If the attacker is null: draw one rand and discard it; set target +0x994 = 0.5; go to the
common tail (§2.11) with the damage unchanged.

### 2.4 Attacker present: ground-only abilities

If the attacker's ability (+0x68) is 0x64 or 0x4b and the target is not on the ground
(+0x5c bit 0 clear): return 0, no effects.

### 2.5 Ability 0x1d particles

If the attacker's ability is 0x1d, append a particle burst: position = target position;
velocity (0, 0, 10.0); colour (0.6, 0.6, 1.0, 1.0); size = rand × 0.1 / 32767 + 0.1 (one
rand; 0.1 is 0x42e528); count 8; style 1; spread 3.0 (0x42e580).

### 2.6 Immunity

The target is **hittable** when the ability argument is 0x8c, or all of: target +0x128 bit 7
clear, target +0x5c bit 5 clear, and not (target ability is 0x7d with target ability time
< 5000.0 (0x42e5e4)).

If not hittable:

1. append a sound: target position, id 0x18, pitch = rand × 0.3 / 32767 + 1.0 (0.3 is
   0x42e704);
2. append a damage number: attacker id, target id, damage = the current damage, critical 0,
   stun 0, +0x1c 0, target position, push = the push vector unscaled, +0x44 0, kind 3, +0x46 0;
3. if not a remote client, apply it with 0x29dcf0 (§3);
4. return 0.

### 2.7 Blocking

Only when **blockable** is non-zero. The target blocks when either:

* it is guarding (0x5ed30, §5.6) and its block power (+0x184) > 0; or
* its ability is 0x48, 0x49 or 0x82 and wind-up ≤ ability time < wind-up + active, where
  wind-up = 0x4ead0(target, current ability) and active = 0x660d0(target, current ability),
  both integers converted to float. For 0x48 and 0x49 these are 200 and 3000; for 0x82
  wind-up = truncate(400.0 / attack speed) (400.0 is 0x42e7e8; attack speed §5.8) and active
  = 3000.

On a block:

1. cost = damage / blockValue(target) (0x50370, §5.5). If the target's ability is 0x48, 0x49
   or 0x82, cost = 0. If the target's ability is 0x5f, cost = cost × 0.5.
   (The game prints "mana cost" and the value to the console; no state effect.)
2. If not a remote client:
   * gain = (1.0 − cost) × 0.5, then gain = max(gain, 0);
   * m = 0x5f8d0(target): 1.0 − strength × 0.1 of the first buff of type 0x22, or 1.0;
   * if the target is guarding (0x5ed30 again): mana += m × gain; and, unless its ability is
     0x5f, +0x158 += m × gain;
   * if mana > 1.0, mana = 1.0; then if +0x158 > mana, +0x158 = mana.
3. Damage number: attacker id, target id, damage = the current damage, critical 0, stun 0,
   target position, push = push × 0.5 (0x4ac964) per component, +0x44 0, kind 1, +0x46 0.
4. Block power −= cost; if it is now < −1.0, set it to −1.0. (Done even on a remote client.)
5. Append the damage number to world+0x90.
6. Append a sound: target position, id 0x18, pitch = rand × 0.3 / 32767 + 1.0.
7. If not a remote client, apply the record with 0x29dcf0 (it returns at once for kind 1).
8. Return 0.

### 2.8 Mana shield, armour and the combo ratio

1. S = sum, in list order, of the strengths of the target's buffs of type 6.
2. combo ratio **r** = float(attacker combo +0x70) / float(maxCombo(attacker)) (0x5f9e0,
   §5.7); **rc** = min(r, 1.0).
3. If S > 0 (mana shield):
   1. damage number: attacker id, target id, damage = the current damage, critical 0, stun 0,
      target position, push × 0.5, +0x44 0, kind 5, +0x46 0; append it;
   2. append a sound: target position, id 0x5e, pitch = rand × 0.3 / 32767 + 1.0;
   3. if not a remote client, apply the record with 0x29dcf0 (which wears the shield down,
      §3.5);
   4. damage = damage − S; if damage ≤ 0, **return 1** (nothing else happens).
4. Defence: if hit type is 0, defence = armour(target) (0x4f080, §5.3); otherwise defence =
   resistance(target) (0x64d90, §5.3).
   damage = max(0, damage − defence × (1.0 − rc)). (Multiply first, then subtract.)
5. Hit type 7 with damage > 0: heal = damage × 0.1 (0x42e528). Append a damage number with
   attacker id at +0 **and** +8, damage = −heal, critical 0, stun 0, +0x1c 0, attacker
   position, push (0, 0, 0), +0x44..+0x46 all 0. Then attacker health += heal (no cap).

### 2.9 Combo extras

Recompute r as in §2.8 step 2 (same value).

**Spirit particles**, when r > 0.25 (0x4ac95c):

* v = (push x, push y). If the push vector's squared length (all three components) is > 0,
  multiply both by 1.0 / sqrt(that). Then multiply both by 4.0 (0x4aca20). The burst velocity
  is (v.x, v.y, 5.0) (0x42e584).
* For each weapon W in weaponsInUse(attacker) (0x4fd40, §5.4), call 0x2a4230(world, target
  position, that velocity, r, spirit flag, W): for each spirit entry of W (first byte ≥ 0x80),
  in order, append a particle burst: target position; velocity as given, doubled when the
  spirit flag is set; colour = materialColour(entry material, s = r × 0.5) (0x109960, below);
  size 0.1; count = truncate(r × 3.0 + 1.0), plus 10 when the spirit flag is set; style 2;
  spread 3.0.
* materialColour for 0x80 / 0x81 / 0x82 / 0x83: base RGB (1.0, 0.5, 0.1) / (0.3, 1.0, 0.5) /
  (0.3, 0.5, 1.0) / (0.8, 0.8, 1.0), alpha base s + 1.0; every channel is then multiplied by
  (0.5 × s + 1.0). Any other material ≥ 0x84 gives (0.5, 0.5, 0.5, 1.0).

**Spirit effects**, when the spirit flag is non-zero and r > 0:

* L = weaponsInUse(attacker); n = its length. If n = 0 nothing happens. Otherwise
  q = 0.1 / float(n), heal = 0, and for each weapon in L, for each spirit entry in order:
  | entry material | effect |
  |---|---|
  | 0x80 | damage += q × r |
  | 0x81 | t = q × r; heal += t + t; damage += t × 0.5 |
  | 0x82 | damage += (q × 0.25) × r; target +0x138 += r × 500.0 (0x42e5c8) |
  | 0x83 | damage += (q × 0.25) × r; attacker +0x13c += r × 500.0 |
  | other | nothing |
* Then (still only when n > 0): if target +0x138 > 5000.0 set it to 5000.0; if attacker
  +0x13c > 5000.0 set it to 5000.0.
* If heal > 0: append a damage number (attacker id at +0 and +8, damage −heal, all else 0,
  attacker position, zero push) and attacker health += heal (no cap).

**Lifesteal**: if the attacker has a buff of type 2 (first match, 0x5e3a0): heal = damage ×
0.5; append the same kind of self-heal damage number with −heal; attacker health += heal; then
attacker health = min(health, maxHealth(attacker)) (0x5fac0, §5.2).

### 2.10 Hit reaction and the impact sound

1. If the attacker's ability is 0x1d: if target +0x994 < 0.3, set it to 0.3. Otherwise, if
   blockable is non-zero, set target +0x994 = 1.0.
2. L = weaponsInUse(attacker) (computed again).
3. If hit type is 0:
   * if the ability argument is 0x1d, no sound;
   * otherwise: weapon subtype k = subtype (+4) of the first weapon in L, or −1 if L is empty;
     if the attacker's ability is 0x45, k = 0x11. pitch = rand × 0.2 / 32767 + 0.9 (0.2 is
     0x42e700, 0.9 is 0x42e534). Sound id:
     | k | not critical | critical |
     |---|---|---|
     | 0, 1 | 1 (pitch += 0.1) | 2 (pitch += 0.1) |
     | 15, 16 | 1 | 2 |
     | 3, 4 | 7 | 8 |
     | 5 | 3 | 4 |
     | 6, 7, 8 | 9 | 10 |
     | anything else, −1 | 5 | 6 |
     Then: if the attacker's ability is 0x37, id = 0xb; else if it is 0x0b, id = 5. (0x5e9b0 is
     called on the attacker here; its result is unused.) Append the sound at the target
     position.
4. If hit type is not 0: append a sound at the target position with id 0x2d, pitch 1.0 when
   hit type is 2, else id 0x2a, pitch 1.5 (0x42e548). No rand.

### 2.11 Common tail (both with and without attacker)

1. (No effect.) The routine compares the target's velocity length with the push length and
   would rescale the velocity, but the comparison can never succeed; nothing is written.
2. Build the main damage number: attacker id (0 if none), target id, damage × the strengths
   of every target buff of type 1 (in list order), critical, stun 0, +0x1c 0, target
   position, push copied unscaled, +0x44 = spirit flag, kind 0, +0x46 0.
3. **Poison** (ability argument 0x12): append to the target's buff list (directly, without
   the give-buff matching) a buff of type 4, strength D0 × 0.2, duration 3000.0 (0x42e5dc),
   source = attacker id; append a buff-change notice (attacker id, target id, that buff, 0).
4. **Critical sound**: if critical is non-zero and the target's hostility kind is not 7: draw
   one rand; pitch = rand × 0.1 / 32767 + 1.0; the sound id comes from the target's race:
   | race | id |
   |---|---|
   | 0 | 0x3f |
   | 1 | 0x40 |
   | 2, 0x2b, 0x53, 0xa7, 0xca, 0xe3, 0xe6, 0xec | 0x41 |
   | 3, 0x2d, 0x54, 0xcb, 0xe4, 0xed | 0x42 |
   | 4 | 0x43 |
   | 5 | 0x44 |
   | 7 | 0x45 |
   | 8 | 0x46 |
   | 9 | 0x47 |
   | 10 | 0x48 |
   | 0x0b, 0x104 | 0x49 |
   | 0x0c, 0x105 | 0x4a |
   | 0x0d | 0x4d |
   | 0x0e | 0x4e |
   | 0x0f | 0x4b |
   | 0x10 | 0x4c |
   | 0x25..0x28 | 0x52 |
   | 0x2e, 0x6c, 0x6d, 0x72, 0x73, 0xd0 | 0x50 |
   | 0x30, 0x33, 0x57 | 0x51 |
   | 0x60 | 0x53 |
   | 0x77 | 0x4f |
   | any other | no sound (the rand is still drawn) |
   Append the sound at the target position.
5. **Stun roll** — only with an attacker, when target +0x130 < −3000.0 (0x43efe8) and the
   target's hostility kind is not 7:
   1. chance = the stun chance argument;
   2. if the attacker's ability is one of 0x0d, 0x3b, 0x45, 0x46, 0x67: chance = 1.0;
   3. if the target has +0x80 bit 18: chance × 0.15 (0x42e6fc);
   4. if the attacker lacks +0x80 bit 18 and its hostility kind is not 0: chance × 0.1;
   5. if the attacker's ability is 0x64 or 0x4b: chance = 1.0;
   6. if the target's ability is 0x58: chance = 1.0;
   7. chance × 0.9 (0x42e534);
   8. if the target has any buff of type 1: chance = 0;
   9. draw one rand; if rand / 32767 < chance: record stun = 3000 when the target's
      hostility kind is not 0, else 1500 (0x5fc20); record push z = 10.0; append a sound at
      the target position, id 0x17, pitch 1.0.
6. Ability argument 0x94: record push z += 18.0 (0x437fe0) / target +0x84.
7. If the attacker's ability is 0x1d: record +0x46 = 1.
8. Append the record to world+0x90. If not a remote client, apply it with 0x29dcf0 (§3).
9. Return 1.

### 2.12 rand() draws, in order

| Path | Draws |
|---|---|
| no attacker | 1 discarded; then 1 if critical and target kind ≠ 7 |
| attacker ability 0x1d | 1 (particle size) first |
| immune / blocked | 1 (sound), end |
| mana shield | 1 (sound); end if soaked |
| hit type 0, ability argument ≠ 0x1d | 1 (impact sound pitch) |
| critical, target kind ≠ 7 | 1 |
| stun gate passed (attacker, timer < −3000, target kind ≠ 7) | 1, even when the chance is 0 |
| death in 0x29dcf0 | the draws of 0x2a6860 and 0x2a7ca0 (§4) |

### 2.13 Order of appends for a landed hit with an attacker

ability-0x1d particles → [mana-shield number, sound, shield wear] → hit-type-7 heal number →
spirit particles → spirit heal number → lifesteal number → impact sound → poison buff and
notice → critical sound → stun sound → main number → everything 0x29dcf0 appends.

On a remote client every append above still happens, and so do the attacker's self-heals
(§2.8 step 5, §2.9), the spirit timers and the block-power change; only the calls to
0x29dcf0 and the block's mana changes are skipped.

---

## 3. The apply routine, 0x29dcf0 (world, record)

### 3.1 Gate

1. T = the creature with the record's target id (+8). Return if absent, or if its health is
   ≤ 0.
2. Return if the world has no local player and T's hostility kind is 0.
3. Return if the record kind is 1 (blocked).
4. A = the creature with the record's attacker id (+0), possibly null.

### 3.2 Hit side effects on T

If the record damage > 0: T +0x18c −= 0.25, floor 0; and if T's ability is 0x58, set T's
ability to 0 and T's stun timer to 3000.0.

### 3.3 Aggro (record damage ≥ 0 and A exists)

1. A +0x9e0 = T's id; T +0x9e0 = A's id.
2. w = 1.0, or 0.001 (0x42e514) when A's hostility kind is neither 0 nor 6.
3. A's hits-dealt map: entry for T's id (created as 0 if absent) += 1.0.
4. T's aggro entry for A's id (created as 0 if absent) is increased by:
   * when A is a "ranged" creature (0x5eca0, §5.9): w × damage + 0.01 (0x42e51c);
   * else when A's race is 0x19: (damage × 20.0) × w + 0.01;
   * else when A's weapon slot B holds a two-handed weapon and A's specialisation is 1:
     (damage × 80.0) × w + 0.1;
   * else: (damage + damage) × w + 0.01.
   (The increment is computed first and then added to the stored value.)
5. Aggro notice (0x2c3200) for (A, T), §4.1.

### 3.4 Flag 0x2000 gate

If T has +0x80 bit 13 (0x2000): walk T's aggro map in ascending id order and find the first
entry whose anger > 0 and whose creature exists and has hostility kind 0. If none: return.
If one exists: return when A is null or A's hostility kind is neither 0 nor 6; otherwise
continue.

### 3.5 Effects (skipped entirely when the kind is 3)

1. **Knock-back** (record damage ≥ 0): p = record push. If T's stun timer > 0 and the record
   stun < 1, p × 0.1. If T's hostility kind is not 7 and ((not a remote client and T's kind
   is not 0) or T is the local player) and |T knock-back|² ≤ |p|², set T's knock-back to p.
   If record stun > 0, zero T's velocity.
2. If record +0x1c > 0: T +0x134 = max(T +0x134, float(+0x1c)).
3. Kind 4: call 0x4ee50(T, world) and do nothing else: if T's +0x12c ≠ 0 and T is class 4
   specialisation 1, mana += 0.25 (cap 1.0), give T (give buff) a buff of type 0xb, strength
   0, duration 30000.0 (0x432954), source 0, and append a buff-change notice (T's id, T's
   id, that buff, 0).
4. Otherwise:
   1. If record stun > 0: T stun timer = max(timer, float(stun)).
   2. Stop here unless (not a remote client) or T is the local player.
   3. Damage < 0 (healing): health −= damage; if health > maxHealth(T), health =
      maxHealth(T). If not a remote client, for every creature C in world order: if A and C
      are hostile (0x50550(A, C)) and C's aggro map contains T's id, C's aggro entry for A
      (created if absent) −= damage × 0.1, then aggro notice (A, C).
   4. Damage = 0: nothing.
   5. Damage > 0, kind 5 (mana shield): for each T buff of type 6 in list order: strength −=
      damage; if the result is ≤ 0, zero both strength and duration; append a buff-change
      notice (A's id, T's id, the buff after the change, 0).
   6. Damage > 0, any other kind: if canBeDamaged(T) and (A is null or canBeDamaged(A)),
      health −= damage. canBeDamaged (0x50520): hostility kind 0, 1, 3 or 6; or kind 7 with
      race not in 0x8c..0x8e.
   7. If health ≤ 0: death (§3.6).

### 3.6 Death

1. T health = 0.
2. Append a sound at T's position: id 0x2d when T's race is 0x11d, else id 0x1b (the game
   also prints "DIE SOUND"); pitch 1.0.
3. T ability time (+0x6c) = 0.
4. Drops: 0x2a6860(world, T) (§4.3).
5. Rewards: if T +0x128 has bit 8 (0x100): 0x2a7ca0(world, T position, min(T level + 1, 4))
   (§4.4).
6. If T +0x9d8 ≠ 0 and that creature exists, set its health to 0 (no death processing).
7. (No effect.) The routine finds the first aggro entry with anger > 0 belonging to an
   existing player and puts it in a temporary list, and walks a container at T +0xc08 when
   it holds more than one element; neither result is used.
8. **Kill credit**: if T's aggro map is empty, append (0, T id). Otherwise append (id, T id)
   for every aggro entry in ascending id order, whatever its anger.
9. For every creature C in world order: if C +0x9e0 equals T's id or T's +0x9d8, set it to
   0; then erase from C's aggro map the entries whose id equals T's id or T's +0x9d8.

No experience is granted here; the kill-credit list is the hand-off.

---

## 4. Callees of the apply routine

### 4.1 Aggro notice, 0x2c3200 (world, A, X)

Only when X's health > 0:

1. Append an aggro notice (A id, X id, X's aggro entry for A — created as 0 if absent).
2. For every creature C in world order with C not null, C ≠ A, C ≠ X, A and C hostile
   (0x50550(C, A)), and C within 8.0 blocks of X (§4.2): look up C's aggro entry for A
   (created as 0 if absent); if it is exactly 0.0, set it to 0.5 and append an aggro notice
   (A id, C id, 0.5).

### 4.2 Distance test, 0xf3e20 (p, q, d)

Per axis difference in blocks = double(p − q) × (1/65536 as a double). "Farther than d" when
|dx| > d, or |dy| > d, or dx² + dy² + dz² > double(d × d computed in float). §4.1 uses the
negation ("within").

### 4.3 Drops, 0x2a6860 (summary)

Behaviour verified separately by the drops recording (CDR1, §6.3); summarised here for order
of effects and rand use. Every laid item goes through 0x2a5b70(world, item, position, angle,
scale, 1, dead creature, 0) where position = (x + truncate((1 − 2·r2/32767) × 65536), y +
truncate((1 − 2·r1/32767) × 65536), z), r1 and r2 being two consecutive rands (first for y),
and angle = r3 × 360.0 / 32767 (a third rand).

1. Nothing for hostility kind 0 or 3, or +0x80 bit 11 (0x800).
2. Candidate equipment (in this order): slot A if not empty and subtype not 0x16/0x17; slot B
   likewise; chest if subtype ≠ 1; +0x598, +0x4f8, +0x458, +0x318 if not empty. If any:
   rand mod 16 == 0 or +0x80 bit 9 → lay a copy of candidate[rand mod count], scale 1.0.
3. +0x8b8 not empty and rand mod 50 == 0 → lay it, scale 1.0.
4. Hostile (kind 1): rand mod 20 == 0 or flags 0x40200 → a generated item (rand parity:
   0 → 0x2d15d0, 1 → 0x2d0680, both with the creature's level), rarity +1 (max 4) when flags
   0x40200, scale 1.0. Kind 7 instead uses its own rand mod 20 for this. Then (kinds 1 and 7
   and kind 1 with no equipment drop) rand mod 8 == 0 → one of five items by rand mod 5.
   Then a type-15 item (seeded by rand) on a 1-in-20 roll for kind 1 with +0x80 bit 3, or
   +0x80 bit 4. Kind 1 then lays coins (type 12, material 11, scale 0.75): f =
   ((level − 1) × 0.1 + 1) × (rand / 32767 + 1), ×10 for +0x80 bit 9, ×20 for bit 18; coins
   are laid while their count so far (as a float) is < f.
5. rand mod 3 + 1 race-specific material drops (each with an extra rand mod 8 roll).
6. Kinds 1 and 7: every inventory item is laid (each stack unit), scale 0.75, except type 1
   subtype 1.

### 4.4 Rewards, 0x2a7ca0 (world, position, tier)

1. zone = 0x2ada20(world, block x, block y): the region identity at that place.
2. For every creature P in world order with hostility kind 0 and squared distance (float,
   per axis float(int64 difference) × 1/65536 (0x42e510)) < 10000.0 (0x42e5e8):
   one rand; even → item = 0x2d0680(world, tier, P's class); odd → item = 0x2d15d0(world);
   the item's region is replaced by zone; then the three position/angle rands; lay it
   with scale 1.0, count 1, no dropping creature, owner = P +0x968.
3. Coins: c = ((tier − 1) × 0.1 + 1.0) × 10.0 (0x61000, 0x42e598); count =
   truncate((rand / 32767 + 1.0) × c); each coin (type 12, material 11, all else 0) takes
   the three rands and is laid with scale 0.75, owner 0.
4. For i = 1..tier: rand mod 8 == 0 → an item of type 11, subtype i + 0x1f, rarity i, all
   else 0, three rands, scale 0.75, owner 0.

---

## 5. Stat callees

### 5.1 Item level and power (0x109720, 0x109c50)

* Item level 0x109720(item, region): the region is the creature's +0x150 for creatures of
  hostility kind 0, 4 or 6, else the item's own region. If the item's type is 3..9 or 0x1a,
  its seed is divisible by (rarity + 1) × 10, and the Chebyshev distance between the two
  regions is < 2, the level is the rarity. Otherwise the level is the rarity if the regions
  are equal, else −1.
* Item power 0x109c50(item, creature, upgrades): h = seed; h ^= h << 13; h ^= h >> 17;
  h ^= h << 5 (32-bit unsigned). exponent = float(upgrades) × 0.5 / float(32 if a two-handed
  weapon else 16) + (float(level) + float(h mod 1000) / 2000.0). Power = 2.0 raised to the
  exponent (`powf`).

### 5.2 Max health, 0x5fac0

base 100.0, or 125.0 (0x432938) for class 1 specialisation 1. Add the item health of each
equipped item whose type matches its slot (+0x638 and +0x6d8 type 3; +0x3b8 type 4; +0x458
type 6; +0x4f8 type 5; +0x598 type 7), in that order. Then ×10.0 for +0x80 bit 9, ×1.5 for
bit 17, ×15.0 for bit 18; × the strength of the first buff of type 0x16; × the strengths of
every buff of type 0x20; ×1.1 for bit 26.

Item health 0x1097b0: types 3..7 only, else 0. h = seed + 0x2673, then the xorshift above
(<< 13, >> 17, << 5), f = (float(h mod 100000) × 0.25 / 100000.0 + 1.0) × 10.0; ×2 for type 4
(×2 again for subtype 1); ×2 for a two-handed weapon; result = power(item, 0) × f × m with m
= 2.0 for material 1, 1.5 for 0x1b, 1.75 for 0x1c, else 1.0.

### 5.3 Armour 0x4f080 and resistance 0x64d90

Sum of the per-item value over +0x3b8 (type 4), +0x458 (type 6), +0x4f8 (type 5), +0x598
(type 7), counting an item only when the creature is not a player or its class may use the
item (0x1094d0, §5.10). Then ×1.2 when +0x80 has bit 9 or 17; ×2 for bit 18; ×0.25 if the
creature has a buff of type 0x0e (first match); × the strength of the first buff of type 0x18
(armour) or 0x19 (resistance).

Per-item value (armour 0x108d50 / resistance 0x10a1e0): types 4..7 only. h = seed + 0x18008
(armour) or + 0x1a6d (resistance), xorshift as above; f = float(h mod 100000) × 0.25 /
100000.0 + 1.0; ×2 for type 4 (×2 again for subtype 1). Material factor (+0x1c):

| material | armour factor | resistance factor |
|---|---|---|
| 1 | 1.2 | 0.85 |
| 4 | 1.25 | 0.85 |
| 6 | 0.25 | 0.25 |
| 0x0b | 1.125 | 1 |
| 0x13 | 0.8 | 1 |
| 0x14 | 0.8 | 0.85 |
| 0x18, 0x1a, 0x20 | 0.5 | 1 |
| 0x1b, 0x1c, 0x1e, 0x1f | 0.8 | 0.75 |
| 0x1d | 1.15 | 0.85 |
| 0x21 | 0.9 | 0.75 |
| 0x22 | 1 | 0.75 |
| 0x23 | 0.6 | 1 |
| any other | 1 | 1 |

f is multiplied by the factor; result = power(item, 0) × f. (Factors: 1.2 0x42e718, 1.25
0x42e71c, 1.125 0x441c74, 0.8 0x42e530, 1.15 0x4328b4, 0.9 0x42e534, 0.6 0x42e708, 0.85
0x42e714, 0.75 0x42e710.)

### 5.4 Weapons in use, 0x4fd40

A weapon is "usable" when it is type 3 and (the creature is not a player or its class may use
it). Result, in order:

1. slot B if it is a usable weapon with subtype 6 or 7 → [B]; else
2. slot A if it is a usable weapon with subtype 6 or 7 → [A]; else by current ability:
   * 3, 5, 8, 9, 0x0b, 0x13, 0x28, 0x2a → [A] if usable, else [];
   * 0x0c, 0x0d, 0x11, 0x12, 0x15, 0x37, 0x5f, 0x64, 0x6c, 0x8c, 0x94, 0x9e → A if usable,
     then B if usable;
   * 0x29, 0x2b → [B] if usable, else [];
   * any other → B if usable, then A if A is type 3 with subtype 12 and (not a player or its
     class may use it).

### 5.5 Block value, 0x50370

Start 5.0. If slot A holds a weapon of subtype 13 that the creature may use: value =
blockItem(slot B) × 6.0 + 5.0 for class 1 specialisation 1, else blockItem(slot B) × 4.0 +
5.0. Otherwise add blockItem(slot A) if it is a usable weapon, then blockItem(slot B) likewise.
Then ×2 for +0x80 bit 9, ×1.2 bit 17, ×4.0 bit 18, ×1.1 bit 26.

blockItem 0x10ad90 (weapons only, else 0): p = power(item, its +0x9d count). Subtypes 3, 4:
p × 2; 5 and 13: p × 4; two-handed: p × 6 when subtype 11 and the creature is not a player,
else p × 8; anything else: p × 4.

### 5.6 Guarding, 0x5ed30

False if +0x134 > 0. True for ability 9; for ability 0x5f while its time < 20000 (its total
length from 0x4f220); for ability 0x6e; for abilities 0x3c and 0x40 when class 1
specialisation 1. Otherwise false.

### 5.7 Max combo, 0x5f9e0

When slot B holds a weapon: subtypes 3, 4, 10 → 50; 6 → 60; 8 → 80; 11, 12 → 20; 15, 16, 17 →
15; other → 30. Without a weapon in slot B: 15 if +0x80 bit 4, else 30.

### 5.8 Attack speed, 0x66460 (only for the 0x82 block window)

s = 1.0; class 1 spec 0: s = rc × 0.5 + 1.0; class 3 spec 1: s += rc; class 2 spec 0: s += rc
× 0.5 (rc = min(combo / max combo, 1)). Add itemSpeed for each item whose type matches its
slot (A, B type 3; +0x3b8 4; +0x458 6; +0x4f8 5; +0x598 7; +0x318 8; +0x778, +0x818 9). Double
if any buff of type 0x0c. ×0.75 bit 9, ×0.6 bit 18, ×1.1 bit 26.

itemSpeed 0x10a490 (types 3..9): a = 0.02 for a two-handed weapon or types 4, 8, 9, else
0.01; ×2 for type 4 subtype 1. b = float(seed mod 21) / 20.0. Material 4, 0x21, 0x22, 0x23:
b × 0.9; 6: a × 0.5; 12: b + 1.0; 0x1d: b + 0.5. v = power(item, 0) × 0.0625 × a × b × 3.0
(left to right); 0 when v < 0.001.

### 5.9 Ranged attacker, 0x5eca0

True when class is 3, 0x95, 0x96, 0x97 or 0x98; race 0x75 or 0x56; slot B weapon subtype 10,
11 or 12; or either weapon slot holds a weapon of subtype 6, 7, 8, 10 or 11.

### 5.10 Class may use item, 0x1094d0

Weapons by subtype: 3, 4, 5 → class 4; 6, 7, 8, 14 → class 2; 9 → any; 10, 11, 12 → class 3;
other subtypes below 18 → class 1; 18 and above → any. Armour (types 4..7) by material:
1, 4, 5, 11, 19, 23, 29 → class 1; 20, 27, 31, 33 → class 2; 24, 26, 32, 35 → class 3; 28,
30, 34 → class 4; others → any. Type 2 items are judged by the item type they wrap. Other
types → any.

---

## 6. Verification

### 6.1 The recording

ZoneExtractor's hit probe records each call of 0x29eb60 into `hit.bin` (magic `CHT1`), one
length-prefixed entry per call (about 600 hits are available). Each entry holds:

* flags: remote client, target is the local player, attacker is the local player, a local
  player exists, and the call's result;
* the call's arguments: target id, attacker id, damage, critical, spirit flag, stun chance,
  argument 8's 24 bytes, the push vector, hit type, ability, argument 12, blockable;
* before the call: the target's (and attacker's, when present) whole record, buff list,
  aggro map, hits-dealt map and two id sets; for every other creature its address, first
  0x200 bytes, +0x9e0 and aggro map; and the counts of seven world lists (aggro notices
  0x18, buff notices 0x30, an 8-byte list, damage numbers 0x48, sounds 0x30, particles 0x48,
  kill credits 0x10);
* the same after the call, with the entries each list gained;
* every rand() value drawn during the call.

### 6.2 How to check an implementation

Replay each entry from its "before" state, feeding the recorded rand values in order, and
compare:

1. the result byte;
2. the target's and attacker's whole records (4-byte words), buff lists, both maps and both
   sets; other creatures' first 0x200 bytes, +0x9e0 and aggro maps;
3. each list's gained entries byte for byte, ignoring padding: aggro notices +0x14..+0x17;
   damage numbers +0x15..+0x17 and +0x47; sounds +0x24..+0x27. The 8-byte list must gain
   nothing;
4. the number of rand draws: equal when the target survives; when it dies, the routine's own
   draws must be a prefix of the recording (drops and rewards draw more).

Group failures by case (client or not; on, by or between non-local creatures; ability;
result) to localise errors. Useful probes: block paths (kind 1 numbers and mana changes),
mana-shield soaks (return 1 with no main number), the stun roll (draw count), spirit
effects (fractional damage additions), and kills (kill credits and aggro clean-up).

### 6.3 Drops and rewards

`drops.bin` (CDR1) records each death's 0x2a6860 call with every 0x2a5b70 call; use it for
§4.3. Rewards (§4.4) are not covered by a separate recording: kills of reward creatures in
`hit.bin` are their only check.

---

## 7. Open questions

1. Meanings of several fields: +0x994, +0x138, +0x13c, +0x158, +0x18c, +0x134, +0x5c bit 5,
   +0x128 bit 7, +0x9d8, and buff types 1, 2, 4, 6, 0x0c, 0x0e, 0x16, 0x18, 0x19, 0x20, 0x22.
   The behaviour above does not depend on the names.
2. Particle bursts leave +0x44..+0x47 unwritten (and the block, shield and immune numbers
   leave the same padding as the main one). If the probe compares particle bursts whole, the
   game's values there are stack leftovers.
3. What the probe's 8-byte list is.
4. Item generators 0x2d0680, 0x2d15d0, 0x2d0fc0, the zone lookup 0x2ada20 and the drop
   layer 0x2a5b70 are not specified here (§4.3 and §4.4 give only call order, arguments and
   rands).
5. Where experience is awarded from the kill-credit list (not in these routines).
6. With ability argument 0x12 and no attacker, the routine reads the attacker's id from a
   null pointer; callers are assumed never to do that.
7. Kind 4 numbers and a non-zero +0x1c come from producers other than 0x29eb60; their
   sources were not examined.
