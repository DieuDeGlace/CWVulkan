# Creature abilities, part 1: the step, its tail and the melee moves (0x2c5250)

Behavioural specification of Steam Cube World's per-creature ability step, written from the
decompiled executable, its assembly and the binary's constants. Routines are cited by RVA
(image base 0x140000000); constants by the RVA of the value. Offsets are byte offsets into the
record named. Times are milliseconds, distances blocks, world positions signed 64-bit integers
in 1/65536 of a block ("dots").

The step is split over two documents:

* **this one**: where the step runs, its inputs and outputs, the switch on the ability and
  the full list of cases, the common tail (combo timeout and cooldowns), and the melee
  branch (0x2c5cf0), which serves 53 abilities;
* **`creature-abilities-ranged.md`**: every other branch (shots, arrows, area casts, charging
  moves, healing, the ray spells, the beam, using items, movement skills, summons, taming,
  special items).

Shared vocabulary, used unchanged: the hit routine 0x29eb60 and its 13 arguments, the apply
routine 0x29dcf0, the damage-number, sound, particle-burst and buff-change-notice records
(`creature-hit.md` §0.3, §0.4, §1.1, §3); give buff 0x65390 and the hostility test 0x50550
(`skills-and-interactions.md` §0); the cooldown table 0x50bd0 (same, §1.7); the effect record
and the effect-creating branches (`effect-objects.md` §0.1, §3); the ability timing parts,
attack-speed factor and stat callees (`stat-rules.md` §4, §6, §7, `creature-upkeep.md` §4); the
yaw basis 0x13050, the turn 0x17840, the overlap test 0x2b77c0 and the small vector helpers
(`creature-physics.md` §7); the end-of-attack pass and the evade reward 0x4ee50
(`creature-upkeep.md` step 12 and §4.10); sight 0x2b5d90 and the block ray 0x2d1d20
(`effect-objects.md` §2.5); block reads 0xeab0 and the block writes 0x2a59f0 / 0x2e0710
(`effect-objects.md` §2.7).

---

## 0. Conventions

* Arithmetic is IEEE single precision (SSE scalar, round to nearest, no fused multiply-add)
  unless a step says **double**. Each operation is rounded before the next and the order
  written is the game's order; "a × b × c" means (a × b) × c. The decompiler's pseudocode
  regroups several of these products; the order given here was taken from the assembly.
* **trunc32 / trunc64**: float to 32- or 64-bit integer, toward zero. **dots(f)** =
  trunc64(f × 65536.0) (0x42e5f8). **blocks(i)** = float(i) × 1/65536 (0x42e510).
* **trunc-block(p)**: a dot coordinate divided by 65536 toward zero (the game adds 0xffff to a
  negative value before an arithmetic shift by 16). **floor-block(p)**: toward −∞ (0xfd80).
* **Squared distance in blocks** of an integer difference (dx, dy, dz): ((blocks(dx))² +
  (blocks(dy))²) + (blocks(dz))², each component converted, then squared.
* **Flat distance test** (used by the melee and ray strikes): with dx, dy the 64-bit dot
  differences, sq(d) = (d × d, a 64-bit product) divided by 65536 toward zero; the test is
  sq(dx) + sq(dy) < trunc64((r × r) × 65536.0) as 64-bit integers.
* **|v|** of a float v where marked "via double": v widened to double, sign bit cleared,
  rounded back to float (the same value as the float absolute value).
* **rand** is the CRT's (0..32767), import slot 0x42cd28, shared with the whole game. "rand
  × k / 32767 + c" means: float of the draw, × k, ÷ 32767.0 (0x42e5f0), + c. The **variation**
  V = 1.25 (0x42e71c) − (rand × 0.5 / 32767), one draw.
* Comparisons are as the game makes them; a comparison with NaN is false unless stated.
* Library thunks: sqrtf 0x395462, sqrt (double) 0x39545c, sinf 0x395456, cosf 0x395450.

### 0.1 Names used in every branch

* **C** the creature, **W** the world, **dt** the step's elapsed ms (the third argument, a
  float).
* **t** = C's ability time (+0x6c), read from the record each time it is used (a branch that
  writes +0x6c sees its own write). The step never advances it; the upkeep 0x2de4c0 does,
  later in the same world tick.
* **a** = C's ability id (+0x68), read from the record each time it is used.
* **T** = 0x4ead0(C, −1), the effect part; **M** = 0x660d0(C, −1), the middle part; **D** =
  0x4f220(C), the whole duration (`stat-rules.md` §7.1). All three are integers and are
  recomputed at every use (they depend on buffs and the combo, which can change in a branch).
* **authoritative**: W+0x358 is 0. **local**: C is the record at W+0x448.
* **deciding world**: (authoritative and C's hostility kind +0x60 is not 0) or C is local.
  On a server or in single player this is every non-player creature plus the local player;
  on a client only the local player.
* **passes X** (X an integer or float, converted to float): t ≤ X and X < t + dt.
  **reaches X**: t < X and X ≤ t + dt. ("t + dt" is the float sum of dt and t.)
* **n-ms boundary**: trunc32(t) / n differs from trunc32(t + dt) / n (C integer division,
  toward zero). Where the operands differ (t − T instead of t, etc.) the section says so.
* **m** = 0x50f20(C, ability) the ability's damage multiplier; **w** = 0x4fa70(C, 0) the
  attack power; **p** = 0x65c70(C, 0) the spell power; **crit(X, magic)** = 0x50d30(C, X,
  magic) (`stat-rules.md` §4.1–§4.4).
* **mana cost** 0x5f7c0(C, ability, −1), **stamina cost** 0x65ea0(C, ability), **attack-speed
  factor** 0x66460(C), **maximum health** 0x5fac0(X), **mana factor** 0x5f8d0(C), **proc roll**
  0x5e820(C, chance) (`stat-rules.md` §4.5, §4.7, §5.1, §6).
* **Set A** = C's ordered id set at +0x9b8 (count +0x9c0): creatures this swing has dealt
  with. **Set B** = +0x9c8 (count +0x9d0): creatures that dodged this swing. Both are std::set
  of i64, walked in ascending id order; 0x37a40 inserts (no-op if present), 0x3d6c0 empties.

### 0.2 Producers

All appends go to the end of the world list named.

* **Sound request** (W+0xa0, 0x30 bytes, 0xa18b0): position (3 × i64), id i32 +0x18, pitch
  f32 +0x1c, volume f32 +0x20, +0x24..+0x27 never written, owner id i64 +0x28. Some branches
  fill a record that 0x80270 has defaulted (pitch 1.0, volume 1.0, owner 0); the others write
  every field. Unless a line says otherwise, **volume is 1.0**. "Owner C" means C's id at
  +0x28; "owner 0" means 0.
* **Particle burst** (W+0xb0, 0x48 bytes, 0x2d0450): position (3 × i64) +0, velocity f32 ×3
  +0x18, colour f32 ×4 (r, g, b, a) +0x24, size f32 +0x34, count i32 +0x38, style i32 +0x3c,
  spread f32 +0x40; +0x44..+0x47 never written. A colour or alpha marked "unset" is not written
  by the game: the record carries whatever the stack slot held (it cannot be reproduced; see
  §6).
* **Damage number** (W+0x90, 0x48 bytes, 0xa16f0), layout as `creature-hit.md` §0.3. Every
  number made here writes 0 into +0x14 (unless a critical flag is given), +0x18..+0x1f, the
  push +0x38..+0x43, and +0x44..+0x46 (unless a kind byte is given); +0x15..+0x17 and +0x47
  are never written.
* **Buff-change notice** (W+0x150, 0x30 bytes, 0xa1530): source id, target id, the 0x18-byte
  buff, flag byte +0x28 (0 unless stated). The buff's padding bytes (+1..+3, +0xc..+0xf) are
  copied from wherever the buff was assembled and are not initialised by the game.
* **Consume buff X**: if C has a buff of type X (0x5e3a0, first match), give C (0x65390,
  refresh flag 0) the buff {type X, strength 0, duration 0, source 0}, then append a notice
  {C's id, C's id, that buff, flag 0}. Types 9, 10 and 11 are refresh types, so this overwrites
  C's entry of that type whose source is 0 (or appends one if the existing entry has another
  source); the upkeep then removes it.
* **Give-and-tell** 0x29d2f0(W, X, buff, f): give X the buff with refresh flag f, then append a
  notice {X's id, X's id, the buff, flag f}.
* **Block change** (W+0xc0, 0x20 bytes, 0xa1680): `effect-objects.md` §0.5.
* **Hit request**: a call of 0x29eb60 (`creature-hit.md` §1.1). Its arguments are listed as
  (target, damage, critical, spirit flag, stun chance, position pointer, push, hit type,
  ability, pointer 12, blockable); the world and the attacker C are always the first and third
  arguments. Its result (landed or not) is used only where a section says so; it may draw rand.
* **Effect object** (W+0x60, and the same record appended to W+0xd0): `effect-objects.md`.
* **Interaction** (C+0xab8, appended): a record defaulted by 0x9660 with its kind set
  (`skills-and-interactions.md` §1.8, §2.1).

---

## 1. Where it runs, inputs and outputs

The world tick 0x2da6c0 walks the creature list (W+0x40) in order; for each creature with
health (+0x180) > 0 it calls the physics 0x2bb8e0, then this step 0x2c5250 (W, C, dt), then
the interaction handler 0x2c3710; for every creature, alive or not, it then calls the upkeep
0x2de4c0. The step returns nothing.

### 1.1 Creature fields

R read, W written; "X" marks fields read or written on other creatures.

| Offset | Type | Use |
|---|---|---|
| +0x08 | i64 | id (written by taming, 0x7e) |
| +0x10, +0x18, +0x20 | i64 ×3 | position (written by several movement branches; X: written by 0x75, 0x9a's summon, 0x7e, 0x81) |
| +0x30 | f32 | yaw, degrees (written by the spinning moves) |
| +0x34..+0x3c | f32 ×3 | velocity (written by 0x98, 0x9c, 0x9d; +0x3c by 0x9e) |
| +0x40..+0x48 | f32 ×3 | movement intent (written by 0x98, 0x9c, 0x9d) |
| +0x4c..+0x54 | f32 ×3 | knock-back velocity (written by 0x98, 0x9c, 0x9d; X by 0x81) |
| +0x60 | u8 | hostility kind (0 player; 1, 4, 6, 7, 8 used) |
| +0x64 | i32 | race |
| +0x68 | u8 | ability (written: set to 0 or another id in some branches; X by 0x80, 0x9a) |
| +0x6c | f32 | ability time (written by 0x86–0x88, 0x89, 0x9c, the combo drop) |
| +0x70 | i32 | combo (written) |
| +0x74 | f32 | time since the last landed hit (written) |
| +0x80 | u32 | flags: 0x800 (a summon or clone), 0x200, 0x8000 read |
| +0x84, +0x88, +0x8c | f32 ×3 | size factors (width, depth, height) |
| +0x128 | u16 | state flags: 0x4 (strikes from its aim) read |
| +0x12c | f32 | dodge timer (written for rogues; X read) |
| +0x130 | f32 | stun timer |
| +0x144 | i32 | level |
| +0x14c, +0x14d | u8 | class, specialisation |
| +0x158 | f32 | charge being built (written) |
| +0x174, +0x178, +0x17c | f32 ×3 | aim vector (blocks, from C toward where it aims) |
| +0x180 | f32 | health (written by 0x82/food healing and 0x85) |
| +0x188 | f32 | mana (written) |
| +0x18c | f32 | stealth value (written by rogues' hits and 0x34) |
| +0x19c | u8 | race 0x12e's rising flag (X written by 0x9a) |
| +0x1a0 | i64 | owner id |
| +0x1a8 | i64 | target / ally id |
| +0x1b0, +0x1b4 | i32 ×2 | flight point (block x, y) |
| +0x1b8..+0x1c8 | i64 ×3 | where a flight or path started (written) |
| +0x1d0, +0x1d4 | i32 ×2 | a home cell (ability 0x75) |
| +0x1d8 | item (0xa0) | item in use: type byte +0x1d8, subtype i32 +0x1dc |
| +0x638, +0x63c | u8, i32 | weapon slot A type, subtype |
| +0x6d8, +0x6dc | u8, i32 | weapon slot B type, subtype |
| +0x970 | u8 | a look/element kind (colours of 0x1d) |
| +0x980 | list | buffs |
| +0x9a0 | f32 | stamina (written) |
| +0x9b8, +0x9c8 | sets | set A and set B (§0.1) |
| +0x9d8 | i64 | mount / pet id |
| +0x9e0 | i64 | last creature hit or hit by |
| +0x9f0 | vector | inventory tabs (0x18 bytes each; slots of 0xa4 bytes: i32 count, then an item) |
| +0xab8 | list | pending interactions (written) |
| +0xad8 | i32 | the combo as the current swing began (written) |
| +0xae0..+0xaf0 | i64 ×3 | a path's destination (written by 0x89, read by 0x98) |
| +0xb10..+0xb20 | i64 ×3 | drawn position (written by 0x98, 0x9c) |
| +0xb4 | f32 | copied to a boomerang |
| +0xb78 | map i32 → f32 | cooldowns (written by the tail and 0x70/0x71/0x73) |
| +0xba8 | f32 | charge released into the current attack (written) |
| +0xbac | u8 | "this swing dealt with someone" (written) |
| +0xbb0 | f32 | fall start height, blocks (written by movement branches) |
| +0xbb8 | u8 | "sure critical" (written) |
| +0xbcc | u32 | AI flags (written on summons) |
| +0xbe8, +0xbf0..+0xbf8 | i32, vector of i32 | summon mode and the races a summoner can call |
| +0x1528 | pointer | (local player) a structure whose +0xa8 is a set of visited cells |

### 1.2 World fields

| Offset | Use |
|---|---|
| +0x08 | i32 change counter stamped on block changes |
| +0x20 | ordered set of dead ids (0x7e inserts) |
| +0x40 | creature list, world order |
| +0x60 / +0xd0 | effect list / outgoing effect copies |
| +0x90 | damage numbers |
| +0xa0 | sound requests |
| +0xb0 | particle bursts |
| +0xc0 | block changes |
| +0x150 | buff-change notices |
| +0x1a0 | set of zones to rebuild (0x89) |
| +0x358 | remote-client flag |
| +0x448 | local player |
| +0x450 | id → creature map (0xe720 looks up; none when absent) |
| +0x460 | zone map (0x16e00 looks up by zone x, y) |

---

## 2. The switch

0x2c5250 reads a = C+0x68 once at entry. If a − 2 (as an unsigned 32-bit value) is above 0x9e
(so a is 0, 1, or above 0xa0) the step goes straight to the tail. Otherwise the byte table at
0x2d00dc, indexed by a − 2, selects one of 41 entries of the offset table at 0x2d0044 (offsets
from the image base). The branches and the abilities they serve:

| Branch | Abilities | What it does | Where |
|---|---|---|---|
| 0x2c5cf0 | 2–8, 0x0a–0x16, 0x1f–0x22, 0x35, 0x37, 0x3a, 0x3b, 0x3d–0x3f, 0x42–0x47, 0x49, 0x4b–0x4f, 0x5f–0x61, 0x64, 0x67, 0x74, 0x8b, 0x94, 0x9e, 0x9f | melee moves and close spells | §4 |
| 0x2cf5fa | 0x26–0x30, 0x7b | staff and wand shots, volleys, the meteor | part 2 §1 |
| 0x2ceb05 | 0x17, 0x18, 0x1b, 0x6c, 0xa0 | bow and crossbow arrows, boomerang, homing bolt | part 2 §2 |
| 0x2cf1ea | 0x38 | arrow barrage | part 2 §3 |
| 0x2c9ffc | 0x62, 0x63 | area casts (status area, healing area) | part 2 §4 |
| 0x2cad1d | 0x19, 0x1a, 0x1c, 0x25, 0x3c, 0x40, 0x41, 0x93 | charging moves (a hum) | part 2 §5 |
| 0x2cae0d | 0x1d | healing an ally (stream) | part 2 §6 |
| 0x2cb299 | 0x68–0x6b | ray spells | part 2 §7 |
| 0x2c97c1 | 0x88 | channelled beam | part 2 §8 |
| 0x2c5337 | 0x54, 0x55, 0x82 | using an item; the healing skill | part 2 §9 |
| 0x2c8ed0 | 0x23 | heal the target from mana | part 2 §10 |
| 0x2c9290 | 0x86, 0x87 | healing pulses around C or its aim | part 2 §10 |
| 0x2c8b16 | 0x8c | repeated strikes on a focus target | part 2 §11 |
| 0x2c9ea6, 0x2c9dfb, 0x2c9f51 | 0x70, 0x71, 0x73 | start sounds of three self-skills | part 2 §12 |
| 0x2c8d3a | 0x8e | ranger's area | part 2 §12 |
| 0x2cab73, 0x2cac73, 0x2cabe3 | 0x8f, 0x90, 0x95 | shadow clone, water bubbles, heroic shout (interactions) | part 2 §12 |
| 0x2c5a1b | 0x31, 0x32, 0x52 | dashes and rolls (stamina, a sound) | part 2 §13 |
| 0x2ca321 | 0x33 | blink along the aim | part 2 §13 |
| 0x2cf174 | 0x34 | full stealth | part 2 §13 |
| 0x2c5886 | 0x75 | return to the home cell (local player) | part 2 §13 |
| 0x2cd9bb | 0x98 | travel along a curve to a destination (local player) | part 2 §13 |
| 0x2cdfd9 | 0x9c | eagle flight (local player) | part 2 §13 |
| 0x2cde09 | 0x9d | rising on the eagle (local player) | part 2 §13 |
| 0x2ca803 | 0x65, 0x66 | summon a creature | part 2 §14 |
| 0x2ce833 | 0x7e | taming | part 2 §14 |
| 0x2cca01 | 0x80 | a pet's interaction with its owner | part 2 §14 |
| 0x2ce6a7 | 0x81 | throwing another creature | part 2 §14 |
| 0x2ccb1a | 0x9a | calling up a race-0x12e creature | part 2 §14 |
| 0x2c59b9 | 0x85 | self-kill (local player) | part 2 §14 |
| 0x2c5b29 | 9 | a sound at the end of the effect part, then a hum | part 2 §15 |
| 0x2cc920 | 0x4a | a growl | part 2 §15 |
| 0x2cb17b | 0x53 | a random hum | part 2 §15 |
| 0x2cce4c | 0x89 | using a special item (local player) | part 2 §16 |
| 0x2c5ce7 | 0x1e, 0x24, 0x36, 0x39, 0x48, 0x50, 0x51, 0x56–0x5e, 0x6d–0x6f, 0x72, 0x76–0x7a, 0x7c, 0x7d, 0x7f, 0x83, 0x84, 0x8a, 0x8d, 0x91, 0x92, 0x96, 0x97, 0x99, 0x9b; and 0, 1, > 0xa0 | nothing: straight to the tail | — |

The no-op abilities do their work elsewhere: 0x77 (riding) and 0x91 (being carried) in the
upkeep (`creature-upkeep.md` steps 2–3, as is 0x8c's circling, step 4); 0x92's mana drain in
the upkeep; the
glider 0x78 and the sailing and swimming states in the physics; the R skills 0x6d and 0x8d
on their press (`skills-and-interactions.md` §1.4–§1.5).

Every branch ends in the common tail (§3), whether it acted or not. No branch returns early
past the tail.

---

## 3. The common tail (0x2cfe95)

Runs for every call, after the branch, with the step's dt.

1. **Combo timeout.** Only if authoritative or C is local:
   1. +0x74 = dt + +0x74.
   2. If +0x74 > 4000.0 (0x4725a4): **drop the combo** (§3.1). Then walk the cooldown map
      (+0xb78) in ascending key order; for each entry whose key 0x368230(C, key) accepts, set
      its value to float(0x50bd0(C, key)). 0x368230 returns false for every input (its body
      is "return 0"), so this walk changes nothing.
2. **Cooldowns run down.** For every entry of the cooldown map, in ascending key order (key
   i32 at node+0x1c, value f32 at node+0x20), unless 0x368230 accepts the key (it never
   does): value = value − dt; if 0 > value, value = 0.0. Entries are never removed.

### 3.1 Dropping the combo

f1 = attack-speed factor (0x66460) of C; +0x70 = 0; f2 = 0x66460 of C again (it depends on the
combo); +0x6c = (f1 / f2) × +0x6c. (The ability keeps the same share of its duration while
its speed changes.)

This same three-step procedure is used by the melee branch (§4.14) and the ray spells.

---

## 4. The melee branch (0x2c5cf0)

For a ∈ {2–8, 0x0a–0x16, 0x1f–0x22, 0x35, 0x37, 0x3a, 0x3b, 0x3d–0x3f, 0x42–0x47, 0x49,
0x4b–0x4f, 0x5f–0x61, 0x64, 0x67, 0x74, 0x8b, 0x94, 0x9e, 0x9f}. The steps run in this order.
"a" is re-read where the game re-reads it; it does not change inside this branch.

### 4.1 Reach and the strike centre

1. Y = the yaw basis 0x13050 of C's yaw (+0x30): the identity rotated by yaw × 0.0174532924
   (0x42e520); its second row is F = (−sin, cos, 0) (`creature-physics.md` §7.2). Only F's
   three entries and the first and third rows' entries are read below.
2. width = C+0x84. Start with **ahead** = 1.5 and **reach** = width × 1.5 (0x42e548).
3. Apply in this order (each line tests a as it is at entry):
   * a ≤ 0x14 and bit a of 0x18c5bc (a ∈ {2, 3, 4, 5, 7, 8, 0x0a, 0x0e, 0x0f, 0x13, 0x14}):
     ahead = 1.0, reach = width + width.
   * a ∈ {0x15, 0x16}: reach = reach × 2.25 (0x472210).
   * a ∈ {0x5f, 0x9f}: ahead = 0, reach = reach × 1.75 (0x4328cc).
   * a ∈ {0x45, 0x47}: ahead = 0.
   * a = 0x94: ahead = 0, reach = reach + reach.
   * a = 0x74: ahead = 0, reach = reach + reach.
   * a ∈ {0x60, 0x61}: ahead = 0, reach = 8.0 (0x4aca30).
   * a ∈ {0x1f, 0x21}: ahead = 0, reach = 3.0 (0x42e580).
   * a ∈ {0x20, 0x22}: ahead = 0, reach = 5.0 (0x42e584).
   * a ∈ {0x64, 0x4b}: ahead = 0, reach = 20.0 (0x42e5a0).
   * a ∈ {0x8b, 0x49, 0x4d, 0x4e, 0x4f}: ahead = 0, reach = reach × 1.2 (0x42e718).
4. ahead = ahead × width. The offset is (with Y's entries Y[0..15], rows of four):
   ox = (Y[4] × ahead + Y[0] × 0.0) + Y[8] × 0.0; oy = (ahead × Y[5] + Y[1] × 0.0) + Y[9] ×
   0.0; oz = (Y[6] × ahead + Y[2] × 0.0) + Y[10] × 0.0.
5. **Centre** H = C's position + (dots(ox), dots(oy), dots(oz)) per axis (64-bit adds).
6. If a ∈ 0x1f..0x22 (the close spells): H = C's position + dots(aim) per axis (dots of
   +0x174, +0x178, +0x17c).
7. If a ∈ {0x4c, 0x46}: H.z = H.z − dots((C+0x8c − C+0x84) × 0.5).
8. If a ∈ {0x0c, 0x37, 0x3e} (bits of 0x4080000000001000 for a ≤ 0x3e) or a = 0x9e: H = C's
   position, reach = reach × 3.0.

### 4.2 The strike window

L = T, L2 = M (both recomputed at each use, which gives the same values). Integer arithmetic,
then conversion to float:

1. start = float((L2 / 2 − 100) + L); end = float(L2 / 2 + L).
2. If trunc32(start) < L: start = float(L).
3. If L + L2 ≤ trunc32(end): end = float(L − 1 + L2).
4. a ∈ {0x45, 0x46}: start = float(L2 / 3 + L).
5. a = 0x67: start = float(L2 / 5 + L); end = float(L + L2).
6. a = 0x94: start = float(L); end = float(L + L2).

Below, "start" and "end" are used as float(trunc32(start)) and float(trunc32(end)) (they hold
whole numbers already).

### 4.3 Repeated strikes

**newSwing** starts false. The moves that strike again and again are R = {6, 0x0c, 0x1f,
0x20, 0x21, 0x22, 0x49, 0x5f, 0x64, 0x8b, 0x9e, 0x9f} (0x780001040 over a ≤ 0x22; 0x49; 0x5f;
0xc00008000000001 over a − 0x64). For a in R:

1. If not (t ≥ float(L)) or not (t < float(L + L2)): start = end = float(L). Go to §4.4.
2. Period P = 1000 if a = 0x64, else 250. a ∈ {0x0c, 0x9e, 6}: P = L2 / 3. a ∈ {0x1f, 0x21}:
   P = L2 / 6. a ∈ {0x20, 0x22}: P = L2 / 12. (A zero P would divide by zero; see open
   questions.)
3. i0 = trunc32(t − float(L)); start = float(L + (i0 − i0 mod P)); end = float(trunc32(start)
   + 100). (C remainder; i0 ≥ 0 here.)
4. i1 = trunc32((dt + t) − float(L)); next = float(L + (i1 − i1 mod P)). If next ≠ start and
   trunc32(next) < L + L2: +0xad8 = +0x70; newSwing = true; empty set A, then set B.
5. If end < t + dt: newSwing = true.
6. **Swing sound**: if the deciding world holds and a is neither 0x8b nor 6: unless (float(L) ≤
   t − dt and trunc32(t − float(L)) / P = trunc32((t − float(L)) + dt) / P), append:
   * a sound at C's position, plus dots(aim) per axis if C+0x128 bit 0x4 is set: id 0x0f,
     pitch rand × 0.25 / 32767 + 1.0, owner C;
   * if a = 0x64, a second sound at C's position: id 0x55, pitch rand × 0.1 / 32767 + 0.5,
     owner C.

### 4.4 Breaking blocks

Only if start < t ≤ end, W is authoritative, a ∉ {0x4b, 0x60, 0x64} (bits 0, 21, 25 of
0x2200001 over a − 0x4b) and a ≠ 0x8b:

1. r = trunc32(reach); if r ≥ 9, r = 8; else if r < 4, r = 4. broke = false.
2. For dx = −r … r (outer), dy = −r … r, dz = −r … r (inner), with P = H + (dx, dy, dz) ×
   65536: if the squared distance in blocks of (P − H) < float(r × r):
   1. B = the block at P (0xeab0, floor-block coordinates). Continue only if B's byte 5 has
      bit 0 set (a block strikes break).
   2. Particle burst at P: velocity (0, 0, 10.0), colour (B.r × 1/128, B.g × 1/128, B.b ×
      1/128, 1.0) (0x4386e0), size 0.5, count 3, style 0, spread 3.0.
   3. Remove the block (0x2a59f0 at P).
   4. Block change: floor-block coordinates of P, B's six bytes, the empty block (0x5534a8),
      W+0x08.
   5. broke = true.
3. If broke: a sound at C's position, id 0x1a, pitch rand × 0.4 / 32767 + 0.5, owner 0. Then
   with q = trunc64(reach × 65536.0): 0x2e0710(W, trunc-block(H.x − q), trunc-block(H.y − q),
   trunc-block(H.x + q), trunc-block(H.y + q)).

### 4.5 Who goes on

If W is authoritative, C's hostility kind is 0 and C is not local (another player on a
server), the branch ends here and the tail runs. Otherwise everything below runs. (So a
client runs it for every creature it sees, the server for its own creatures and its local
player.) **own** below means the deciding world (§0.1).

### 4.6 Rogue dodge and spinning

1. If C is class 4 specialisation 1, a ∈ {6, 0x12, 0x15} (bits of 0x240040) and t = 0.0:
   +0x12c = (mana × 600.0) × mana (0x42e7ec).
2. If a ∈ {0x5f, 0x9f} and t < float(D): +0x30 = 0x17840(+0x30, ((dt × 0.00025) × 360.0) ×
   8.0 + +0x30, 1.0) (constants 0x472178, 0x42e7e4, 0x4aca30): the yaw turns 720° a second.

### 4.7 Charging, casting and two-handed sounds

Exactly one of these, by a:

* **a ∈ {0x60, 0x61}** (charging up):
  1. If t < float(T) and a 200-ms boundary: sound at C, id 0x27, pitch rand × 0.25 / 32767 +
     1.0, owner 0.
  2. If t < float(T) and float(T) ≤ t + dt: a sound at C, id 0x54, pitch 1.0, owner 0; then a
     particle burst at C's position: velocity (0, 0, 2.0), colour unset, size 0.25, count 0x50,
     style 1, spread 20.0.
* **a ∈ 0x1f..0x22** (close spells):
  1. As above: 200-ms hum 0x27.
  2. If t < float(T) and float(T) ≤ t + dt: a sound at C, id 0x2c if C's specialisation is 1,
     else 0x29, pitch 1.0, owner 0; then a particle burst at C's position + dots(aim) per
     axis: velocity (0, 0, 2.0), size 0.25, count 0x14, spread 8.0, and if the specialisation
     is 1 colour (0, 0.2, 1.0, 1.0) and style 2, else colour unset and style 1.
* **otherwise**, if t = 0.0 (exactly; the game tests t ≠ 0.0 against the constant at
  0x437f60) and C holds a two-handed weapon (0x5e9b0: slot B, else slot A, of type 3 with a
  subtype in the two-handed set, `creature-hit.md` §0.1) and a ≠ 0x64: a sound at C, id 0x11,
  pitch rand × 0.1 / 32767 + 1.0, owner 0.

### 4.8 Ability 0x9e's leap

If a = 0x9e and t ≤ 100.0 and 100.0 < t + dt: +0x3c = +0x3c + 8.0.

### 4.9 The whoosh

If own, and t ≤ float(T) and float(T) < t + dt, and a ≠ 0x64: pitch = rand × 0.2 / 32767 +
0.9 (0x42e700, 0x42e534); then by a:

| a | id | pitch |
|---|---|---|
| 0x0b | 0x10 | as drawn |
| 0x0c | 0x33 | as drawn |
| 0x9e, 0x9f | 0x33 | × 2.0 |
| 0x37 | 0x0e | as drawn |
| other | 0x0f | × 1.5, then × 0.5 if C holds a two-handed weapon or a ∈ {0x45, 0x46, 0x94, 0x67} |

The sound is at C's position, owner C.

### 4.10 The mid-swing grunt

If own and a ∈ {0x3a, 0x3b, 0x3d, 0x4b} (bits of 0x2000b over a − 0x3a), with mid =
float(M / 2 + T): if t < mid and mid ≤ t + dt: a sound at C, id 0x0c, pitch rand × 0.25 /
32767 + 1.0, owner C.

### 4.11 Winding up

If t ≤ float(T):
1. +0xad8 = +0x70.
2. **Credit dodgers**: for each id in set B (ascending) that is not in set A and whose creature
   V exists (W+0x450): the evade reward 0x4ee50 on V (`creature-upkeep.md` §4.10), then a
   damage number {from C, to V, 0.0, position V's, kind byte +0x45 = 4}.
3. Empty set A, then set B. +0xbac = 0.

### 4.12 Ability 0x4b's roar

If a = 0x4b, with mid = float(M / 2 + T): if t < mid and mid ≤ t + dt: a sound at C, id 0x55,
pitch rand × 0.1 / 32767 + 0.5, owner 0.

### 4.13 End of the window

If end ≤ t: credit dodgers (as §4.11 step 2), then empty set B.

### 4.14 A missed swing loses the combo

If +0xbac = 0, float(T + M) ≤ t + dt, t < float(D) and +0x12c = 0.0: drop the combo (§3.1).

### 4.15 Stamina

If t = 0.0: stamina (+0x9a0) = stamina − stamina cost(a); if 0 > stamina, stamina = 0.

### 4.16 Mana and the charge

1. If a ∈ {0x20, 0x22}: if t ≤ float(T) and float(T) < t + dt: +0xba8 = 0.1; mana = mana −
   mana cost(a); if 0 > mana, mana = 0.
2. Else, if t = 0.0:
   1. +0xbb8 = 0; +0xba8 = 0.
   2. If mana cost(a) ≤ 0:
      * a ∈ {6, 0x12, 0x15}: +0xba8 = mana; mana = 0; if C has a buff of type 0x0b: consume
        buff 0x0b (§0.2) and +0xbb8 = 1 (the hit will be critical).
      * else, a ∈ {0x37, 0x94, 0x16, 0x60, 0x61}: +0xba8 = 1.0.
   3. Else: +0xba8 = mana cost(a) (asked again); mana = mana − +0xba8 (no floor here).
3. If +0x158 > 0: +0xba8 = +0x158; mana = mana − +0x158; +0xbac = 0; if a ∈ {0x0c, 6}:
   +0xba8 = +0xba8 × 0.5.
4. +0x158 = 0.
5. If mana ≤ 0 and mana ≠ 0 (negative): mana = 0.
6. **charged** = +0xba8 > 0, or a ∈ {0x0c, 0x5f, 0x9f}.

### 4.17 The hits

Only if start < t ≤ end; otherwise the branch ends.

1. **bonus** = 0.0 if a ∈ {0x0c, 0x3d, 0x3f} (bits of 0xa000000000001000), else 1.0.
   **landedAny** = false.
2. If a = 0x20, float(T) ≤ t, and C has a buff of type 9: consume buff 9.
3. For each creature X in W's list, in order (C itself included):
   1. Skip X unless 0.0 < X's health and X's id is not in set A.
   2. rX = X+0x84 × 0.7 (0x4328ac) + reach.
   3. **Eye** = C's position; for a ∈ 0x1f..0x22, eye = (H.x, H.y, H.z + dots(reach)).
   4. Skip unless the flat distance test of (H − X's position) passes with r = rX, and
      |blocks(H.z − X.z)| (via double) < (X+0x8c × 0.5 + C+0x8c) + reach.
   5. Skip unless sight 0x2b5d90(W, eye, X's position, flag 1, limit 200.0, X) is true.
   6. If C and X are hostile (0x50550):
      1. If X's dodge timer (+0x12c) ≠ 0.0: insert X's id into set B; go to the next X (no
         further step for X).
      2. dir = (blocks(X.x − C.x), blocks(X.y − C.y), 0.0); if (dir.x² + dir.y²) + 0.0 > 0.01
         (0x42e51c), normalise it (0x127f0: q = 1 / sqrt(x² + y² + z²), components × q).
      3. hit type = 1 for a ∈ {0x1f, 0x20}, 2 for a ∈ {0x21, 0x22}, else 0.
      4. critical: if +0xbb8 ≠ 0, true; else draw rand and critical = (rand / 32767 <
         crit(X, hit type ≠ 0)) — the chance is asked after the draw.
      5. push factor k = 0x5f180(C, a) (§4.20).
      6. damage:
         * a ∈ {0x60, 0x61} or a ∈ 0x1f..0x22: damage = V × (m × p) (the draw for V after
           m and p are asked);
         * else: damage = ((1.25 − rand × 0.5 / 32767) + 0.0) × ((((+0xba8 × +0xba8) × 5.0)
           + bonus) × (m × w)), m and w asked before the draw, +0xba8 read before it.
      7. a = 0x16: k = 0x61190(1000) and damage = damage × 0x61150(0x16) (§4.20). (0x65690
         returns the skill level 1000 for any skill.)
      8. If critical: damage = damage + damage.
      9. s = min(sqrtf(C+0x84 / X+0x84), 1.75) (minss with 0x4328cc: s if s < 1.75, else
         1.75). push = (dir.x × s × k, dir.y × s × k, (s × 0.25) × k).
      10. comboOK = set A is empty (count +0x9c0 = 0) and 0 ≥ C's stun timer (+0x130).
      11. Hit request (target X, damage, critical, spirit flag = charged, stun chance =
          +0xba8, position = C's position, push, hit type, ability a, 0, blockable 1).
      12. If it landed:
          1. a = 0x1f, the proc roll 0x5e820(C, 0.25) succeeds (one rand), and C's
             specialisation is 0: give C {type 9, strength 0, duration 30000.0, source 0}
             (refresh 0) and append its notice (flag 0); a sound at C, id 0x32, pitch 1.0,
             owner 0.
          2. C is class 1 (warrior): q = min(float(D) × 0.01, 1.0) (1.0 when not (1.0 >
             float(D) × 0.01)); if the proc roll 0x5e820(C, q × 0.1) succeeds: give C {type 2,
             strength 0, duration 8000.0, source 0} (refresh 0) with its notice; a sound at
             C, id 0x60, pitch 1.0, owner 0.
          3. If comboOK: +0x70 = +0x70 + 1; 0x603f0(C) (walks the cooldown map, changes
             nothing); +0x74 = 0.
          4. If charged and C is class 4 specialisation 0: +0x18c = +0x18c + +0xba8; if 1.0 <
             +0x18c, +0x18c = 1.0.
          5. **Mana from the hit**: if (authoritative or C local) and not landedAny and not
             charged: g = (1.0 − (rand + rand) / 32767) × 0.05 + 0.1 (the draw converted
             once and added to itself); if a = 0x3b, g = g + g; g = g × (float(D) / 500.0);
             if a second rand is a multiple of 8, g = g × 1.5; mana = mana factor × g + mana;
             if 1.0 < mana, mana = 1.0.
          6. landedAny = true.
      13. Whether it landed or not: insert X's id into set A; +0xbac = 1.
   7. **A healer's heal**: if a ∈ {0x21, 0x22}, newSwing, C and X are not hostile, X's
      hostility kind is not 8 and C's specialisation is 1:
      1. amount = (p × (rand × 0.5 / 32767 − 1.25)) × 0.1 (the draw before p is asked;
         negative = healing).
      2. critical = rand / 32767 < crit(X, true) (a second draw; it only sets the flag).
      3. Damage number {from C, to X, amount, critical, position X's}.
      4. A sound at C, id 0x2d, pitch 1.0, owner 0.
      5. If authoritative, or both C and X are local: apply it (0x29dcf0, `creature-hit.md`
         §3).
4. After the loop: if landedAny, slot A's subtype (+0x63c) is not 0x0d, and (slot B's type
   +0x6d8 is 0 or slot A's type +0x638 is 3): 0x4f220(C) is called and its result discarded
   (no effect).

### 4.18 rand() order in the melee branch

Swing sound(s) (§4.3: one, then one more for 0x64) → block-break sound (§4.4) → hum or
two-handed sound (§4.7) → whoosh (§4.9) → grunt (§4.10) → roar (§4.12) → then per creature X
in list order: critical draw (unless +0xbb8), damage draw, draws inside the hit routine, the
0x1f proc, the warrior's proc, the two mana draws; or for a healed friend its two draws.

### 4.19 What the melee branch creates

Sounds 0x0c, 0x0e, 0x0f, 0x10, 0x11, 0x1a, 0x27, 0x29, 0x2c, 0x2d, 0x32, 0x33, 0x54, 0x55,
0x60; particle bursts (debris, charge-up, cast); block changes; damage numbers (dodge marks,
heals); buff-change notices (buffs 0x0b consumed, 9 consumed or given, 2 given); hit
requests. No effect objects.

### 4.20 Callees special to the melee branch

* **Push factor 0x5f180(C, a)**: 0 if C's hostility kind is 6 or 3. Otherwise by a, with
  S = attack-speed factor: {2, 3, 0x0a, 0x0e, 0x0f, 0x10, 0x3d, 0x3e} → 6.0 / S; {4, 5, 7, 8,
  0x13, 0x14, 0x3f} → 3.0 / S; 6 → 4.0; {0x0c, 0x60} → 10.0; {0x12, 0x15} → 6.0; {0x16, 0x61}
  → 30.0; 0x1f–0x22 → 0.0; 0x37 → 5.0; {0x42, 0x43} → 5.0 / S; any other → 2.0 / S.
* **0x61190(L)** = (1.0 − 1.0 / ((float(L) − 1.0) / 50.0 + 1.0)) × 15.0 + 15.0 (0x42e7a4).
* **0x61150(n)** = (1.0 − 1.0 / ((float(n) − 1.0) / 50.0 + 1.0)) × 0.5 + 0.5.
* **0x17840(a, b, f)**: `creature-physics.md` §7.3.
* **0x5e9b0** two-handed: slot B (+0x6d8) of type 3 with subtype (+0x6dc) < 24 and bit set in
  0x878de0, else the same test on slot A (+0x638, +0x63c).

---

## 5. Constants of part 1

| RVA | Value | Use |
|---|---|---|
| 0x2d00dc / 0x2d0044 | tables | case index (byte per a − 2) / branch offsets |
| 0x4725a4 | 4000.0 | combo timeout |
| 0x42e520 | 0.0174532924 | degrees to radians (0x13050) |
| 0x42e548 | 1.5 | base reach and ahead; whoosh pitch |
| 0x4ac980 | 1.0 | |
| 0x472210 | 2.25 | reach of 0x15, 0x16 |
| 0x4328cc | 1.75 | reach of spins; push size cap |
| 0x4aca30 | 8.0 | reach of 0x60/0x61; spin speed factor; 0x9e leap |
| 0x42e580 | 3.0 | reach of 0x1f/0x21; × 3 reach; debris spread |
| 0x42e584 | 5.0 | reach of 0x20/0x22; charge² factor |
| 0x42e5a0 | 20.0 | reach of 0x64/0x4b; charge-up spread |
| 0x42e718 | 1.2 | reach of 0x8b, 0x49, 0x4d–0x4f |
| 0x4ac964 | 0.5 | halves |
| 0x42e5f8 / 0x42e510 | 65536.0 / 1/65536 | dots, blocks |
| 0x42e5f0 | 32767.0 | rand divisor |
| 0x4386e0 | 1/128 | debris colour |
| 0x4ac95c | 0.25 | pitches, push z |
| 0x42e528 | 0.1 | pitches, warrior proc, mana gain base |
| 0x42e52c | 0.4 | debris sound pitch |
| 0x42e700 / 0x42e534 | 0.2 / 0.9 | whoosh pitch |
| 0x4ac9d0 | 2.0 | whoosh pitch for 0x9e/0x9f; burst velocity z |
| 0x42e7ec | 600.0 | rogue dodge length |
| 0x472178 / 0x42e7e4 | 0.00025 / 360.0 | spin rate |
| 0x4328ac | 0.7 | target width share |
| 0x42e51c | 0.01 | push normalisation threshold; warrior proc |
| 0x42e5c0 | 200.0 | sight limit |
| 0x42e71c | 1.25 | variation |
| 0x437f60 | 0.0 | added to the variation; t = 0 tests |
| 0x42e524 | 0.05 | mana gain spread |
| 0x42e5c8 | 500.0 | mana gain duration scale |
| immediates | 100, 250, 1000, 0x0f, 0x55, 30000.0, 8000.0 | window, periods, ids, buff lengths |

---

## 6. Verification

**The oracle.** `EngineTests abilityoracle <files>` (tests/EngineTests.cpp, `stepOracle` with
RVA 0x2c5250) replays real calls of this step recorded by ZoneExtractor's `update.request`
("<count> 2c5250", optionally followed by an ability filter such as "82,88"). The melee
recordings `Extract/oracle/ability-fight.bin` and `ability-swing.bin` hold 13,786 calls; more
recordings cover the other branches: `ability-bow.bin`, `ability-mage.bin` (CWU4),
`ability-heal.bin`, `ability-beam.bin`, `ability-spells.bin`.

**File:** magic "CWU3" or "CWU4", a 4-byte seed, then records, each preceded by its u32 size.
A record is laid out exactly as the upkeep's (`creature-upkeep.md` §7): creature address, "is
local", W+0x358, dt; the creature's 0x15f8 bytes before and after; the world clock; the
buffs; the world creature list with near creatures' first 0x200 bytes; nearby objects; the
creature's effect objects; the records named at +0x1a8, +0x1a0, +0x9d8 (0x9b0 bytes each); the
block box with zone-loaded bytes; every rand() value drawn; then u32 RVA (0x2c5250 here) and
the size of the rest: buffs and "sides" (aggro map, damage map, set A, set B) before; the
world lists before (aggro notes 0x18, buff notices 0x30, calls 8, damage numbers 0x48, sounds
0x30, particles 0x48, and in CWU4 effect objects 0x80); the buffs, sides and per-list gains
after; the hits applied during the call (79 bytes each: target i64, attacker i64, damage f32,
critical u8, spirit u8, stun chance f32, position 24 bytes, push 12 bytes, hit type i32,
ability i32, 8 bytes, blockable u8); newer files then hold the dead set, the zone links, and
per hit its rand span (draws before, draws after) and result byte.

**How to check an implementation:**
1. Skip calls of the local player.
2. Build C from the "before" bytes, buffs and sides; the others from their recorded heads and
   named records; the world's creature list in the recorded order (creatures not near are
   placed far away); the world flag and dt as recorded.
3. Feed rand() the recorded values in order. For each hit request the implementation makes,
   skip that hit's recorded draws and use its recorded result instead of running the hit.
4. Run the step once (branch and tail).
5. When the call made hits, compare the hit requests (79-byte form) with the recorded hits
   and nothing else. Otherwise expect exactly: the record's 0x15f8 bytes equal "after" (with
   the container counts +0x988, +0x9c0, +0x9d0 taken from the implementation's containers);
   the buffs, maps and sets equal; the buff notices, damage numbers, sounds, particles and (CWU4)
   effect objects appended equal to the recorded gains; the same number of rand() draws.
6. Masks: damage-number bytes +0x15..+0x17 and +0x47; sound bytes +0x24..+0x27; particle
   bytes +0x44..+0x47; effect bytes +0x65..+0x67, +0x6d..+0x6f, +0x7c..+0x7f; for ability 0x1d
   the damage number's amount (it reads the ally's maximum health, not recorded) and the
   burst alpha; for 0x68–0x6b the burst alpha (both unset stack slots).
7. A call that reads a block outside the recorded box is "not checkable", not a failure.

**What the recordings cannot check:** the unset colours of §4.7 (masked only for 0x1d and
0x68–0x6b; the charge-up and cast bursts carry stack garbage); block changes and zone rebuilds
(not recorded); writes to other creatures; the local player's branches (0x75, 0x85, 0x89, 0x98,
0x9c, 0x9d) since local calls are skipped; the effect of 0x368230 (constant).

---

## 7. Open questions (part 1)

1. **Zero periods.** In §4.3 a period of L2 / 3, L2 / 6 or L2 / 12 is 0 when M is small (for
   example a very fast attack speed); the game's integer division would then fault. Whether M
   can get that small for those abilities was not checked.
2. **Unset burst colours.** The charge-up burst (0x60, 0x61) never writes its colour, and the
   cast burst of 0x1f–0x22 writes it only for specialisation 1. The values are whatever the
   stack held; an implementation must choose something (the renderer's view of it is
   unknown).
3. **0x368230** returns false for every skill, which makes the cooldown reset in the tail and
   the "skip" test in the countdown dead. Whether another build patches it is unknown.
4. **The friendly heal includes C itself** (C is in the world list and is not hostile to
   itself), so a healer of specialisation 1 heals itself on each new swing of 0x21/0x22. This
   follows from the code; it was not confirmed in a recording.
5. **sinf / cosf** of the UCRT are used by 0x13050 and 0x17840; their exact rounding (as in
   `creature-upkeep.md`) is assumed to be the UCRT's correctly-rounded-in-practice results.
6. **The "eye" for close spells** is lifted by the reach (not by a height); the purpose is
   unclear but the value is as given.
