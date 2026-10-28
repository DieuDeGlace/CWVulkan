# Effect objects: projectiles, arrows, areas (0x2dba40 and the creating branches of 0x2c5250)

Behavioural specification of Steam Cube World, written from the decompiled executable, its
assembly and the binary's constants. Routines are cited by RVA (image base 0x140000000);
constants are cited by the RVA of the value in the binary. Offsets are byte offsets into the
record named. World positions are signed 64-bit integers in 1/65536 of a block; distances and
velocities are in blocks and blocks per second; times are milliseconds. "rand" is the C
runtime's `rand()` (0..32767), shared with the rest of the game, and "r/32767" always means
the draw converted to float and divided by 32767.0 (0x42e5f0).

Shared vocabulary is used unchanged from the other specs:

* `creature-hit.md`: the hit routine 0x29eb60 and its 13 arguments (§1.1), the apply routine
  0x29dcf0, the damage number (§0.3), sound request, particle burst, buff-change notice and
  aggro notice records (§0.4), the distance test 0xf3e20 (§4.2), item power 0x109c50 (§5.1),
  armour 0x4f080 and resistance 0x64d90 (§5.3), max combo 0x5f9e0 (§5.7).
* `skills-and-interactions.md` §0: give buff 0x65390, hostility test 0x50550; §2.5 the power
  value 0x65c70.
* `creature-upkeep.md`: attack-speed factor 0x66460 (§4.3), mana factor 0x5f8d0 (§4.5), block
  read 0xeab0/0xe9a0/0xeb90 (§4.7), smoothing factor 0xa7a0 (§4.9).

Two routines are covered:

* **The effect step, 0x2dba40** (world, dt). Called once per world step by the world tick
  0x2da6c0, after every creature has had its physics, ability update, interaction handler and
  upkeep for that step. It moves every effect object, lets it touch creatures and blocks, and
  ends or transforms it.
* **The effect-creating branches of the ability routine 0x2c5250** (world, creature, dt):
  shots and arrows, repeating shots, charged spells, cast areas, the ranger area, the healing
  areas left by the water beam, and the landing marker of a flight (§3).

---

## 0. Records and conventions

### 0.1 The effect record (0x80 bytes; list at world+0x60)

The world keeps its effects in a doubly linked list at world+0x60 (element count at +0x68);
each list node holds one 0x80-byte record. Every created effect is also appended to a second
list at world+0xd0 (outgoing copies for the network); 0x2dba40 never touches that one.

| Offset | Type | Default (0x29a0b0) | Meaning |
|---|---|---|---|
| +0x00 | i64 | −1 | owner creature id |
| +0x08, +0x10, +0x18 | i64 ×3 | not written | position |
| +0x20, +0x28, +0x30 | i64 ×3 | 0 | target point (kinds 4 and 10) |
| +0x38 | 8 bytes | 0 | not read here |
| +0x40 | 4 bytes | 0 | not read here |
| +0x44, +0x48, +0x4c | f32 ×3 | not written | velocity (blocks/s) |
| +0x50 | f32 | not written | damage (heal amount for hit type 2; poison strength for 6) |
| +0x54 | f32 | 0.5 | radius of the contact box (blocks) |
| +0x58 | f32 | 1.0 | not read by the step (a size used elsewhere) |
| +0x5c | f32 | 1.0 | charge: the hit's stun chance and push strength |
| +0x60 | f32 | 0 | not read by the step |
| +0x64 | byte | 0 | **flag F** (see below) |
| +0x65..+0x67 | — | not written | padding |
| +0x68 | i32 | 0 | **kind** (§0.2) |
| +0x6c | byte | 0 | **hit type** (passed to 0x29eb60; 1, 2, 4, 6 have meaning here) |
| +0x6d..+0x6f | — | not written | padding |
| +0x70 | f32 | 0 | age (ms) |
| +0x74 | i32 | 5000 | lifetime (ms) |
| +0x78 | i32 | 0 | freeze time: raises a struck creature's +0x138 timer to this value |
| +0x7c..+0x7f | — | not written | padding |

The constructor 0x29a0b0 writes exactly the defaults shown; creating code then fills the rest.
Some creators build the record field by field without the constructor (§3); they write the
same defaults unless stated.

**Flag F (+0x64).** For kinds 0, 1, 2, 3, 4 and 7 it means "do not damage on contact": such an
effect still stops at the first creature it touches but deals no direct damage there, and for
kinds 0, 1, 2 and 7 it is what makes the effect burst at the end of its flight (§1.9). It is
passed to 0x29eb60 as that routine's sixth argument (the "spirit flag"). A burst of kind 3 with
hit type 2 decides the lifetime of the area it turns into by F.

**Hit types used here.** 0 physical; 1 and 4 apply the short status buff 0x0e in areas; 2
heals friendly creatures instead of striking (§1.7.11); 6 poisons in areas (buff 4). Every
non-zero hit type also makes the critical chance use resistance instead of armour (§2.1).

### 0.2 Kinds

| Kind | Movement before the sub-steps (§1.3) | Touches creatures | Touches blocks | Ends |
|---|---|---|---|---|
| 0, 1, 2 | gravity | yes, stops at the first | yes | contact, wall, lifetime; bursts only when F |
| 3 | none (straight) | yes, stops at the first | yes | contact or wall → burst; lifetime |
| 4 | homes on the target point (speed 50) | no | no | within 2 blocks of the target and age ≥ 500 → burst; lifetime |
| 5 | returns to its owner | yes, keeps going | yes (stops) | removed at age > 1200 or without owner |
| 6 | gravity on its velocity, but never moves | yes, every 500 ms | no | lifetime only |
| 7 | gravity | yes, stops at the first | yes | contact, wall, lifetime; bursts only when F |
| 8 | none | no | no | lifetime |
| 9 | none; follows the terrain (§1.5) | no | no | lifetime |
| 10 | homes on the target point (speed 20) | no | no | like 4, then becomes an area |
| other | gravity | never struck (§1.7.6), never stops | yes | wall (bursts only when F), lifetime |

Kinds 0, 1 and 2 behave identically in the step; they differ only in who creates them (§3).

### 0.3 Creature fields used beyond those listed in `creature-hit.md` §0.1

| Offset | Type | Use |
|---|---|---|
| +0x74 | f32 | time since the last landed hit; set to 0 when the combo is raised |
| +0x84, +0x88, +0x8c | f32 ×3 | size factors; half-extents of the contact box are each × 0.5 |
| +0xb4 | f32 | copied to a boomerang's +0x58 |
| +0x12c | f32 | dodge-roll timer: a dodging creature is never struck by effects |
| +0x138 | f32 | freeze timer (raised by effect +0x78) |
| +0x158 | f32 | mana being charged |
| +0x174, +0x178, +0x17c | f32 ×3 | aim vector (blocks; from the creature towards where it aims) |
| +0x188 | f32 | mana |
| +0x18c | f32 | a value that adds half of itself to the critical chance |
| +0x1a8 | i64 | id of the creature an ability is aimed at |
| +0x9a0 | f32 | stamina |
| +0x9c8 | ordered id set | "set B": ids of creatures that dodged this creature's effects |
| +0xba8 | f32 | charge released by the last charged attack |

### 0.4 World fields

| Offset | Meaning |
|---|---|
| +0x08 | i32 copied into block-change records |
| +0x40 | creature list, world order |
| +0x60 / +0x68 | effect list / count |
| +0x90 | damage numbers (append 0xa16f0) |
| +0xa0 | sound requests (append 0xa18b0 or the inline equivalent) |
| +0xb0 | particle bursts (append 0x2d0450 or the inline equivalent) |
| +0xc0 | block-change records (0x20 bytes each) |
| +0xd0 | outgoing effect copies |
| +0x150 | buff-change notices (append 0xa1530) |
| +0x358 | remote-client flag |
| +0x448 | local player (may be null) |
| +0x450 | id → creature map (0xe720 looks it up; null when absent) |
| +0x460 | zone map keyed by zone coordinates (each zone 64 × 64 blocks) |

### 0.5 Records produced here

Sound requests, particle bursts, damage numbers, buff-change notices: as in `creature-hit.md`
§0.3–§0.4. In a sound request, the 8 bytes at +0x28 are 0 unless a section says they hold the
emitting creature's id; +0x24 is never written.

**Block-change record** (0x20 bytes, appended to world+0xc0):

| Offset | Meaning |
|---|---|
| +0x00, +0x04, +0x08 | i32 ×3: block coordinates (floor of position / 65536, per axis) |
| +0x0c..+0x11 | the six bytes of the block before the change |
| +0x12..+0x17 | the six bytes of the global block at 0x5534a8 (set at start-up to ff ff ff 00 00 00, an empty block) |
| +0x18 | the world's i32 at +0x08 |
| +0x1c..+0x1f | not written |

A block is six bytes: red, green, blue, an unused byte, the type (byte 4: 0 empty, 2 water,
anything else solid) and a flag byte (byte 5). Byte 5 bit 0 marks a block that effects destroy;
byte 5 bit 6 marks a block that line-of-sight rays treat as solid.

### 0.6 Numeric conventions

* All arithmetic is single-precision float in the order written. Double precision appears only
  in the smoothing factor 0xa7a0, the distance test 0xf3e20 and one sound pitch (§3.1).
* **trunc64(x)** converts a float to a 64-bit integer, rounding toward zero; **trunc32** the
  same to 32 bits. "to fixed" means trunc64(x × 65536.0) (0x42e5f8).
* **Fixed to blocks** means float(the 64-bit integer) × 1/65536 (0x42e510).
* Two kinds of integer division by 65536 appear, and they differ for negative positions:
  **floor-block** (round toward −∞; used by every block and zone lookup: 0xeab0, 0xfd80,
  0x2a59f0, 0x2d1d20) and **trunc-block** (round toward zero, i.e. C division; used where
  stated: kind 9's terrain column, the rectangle passed to 0x2e0710, kind 5's pull).
* "The n-ms boundary is crossed" means trunc32(age − dt) / n ≠ trunc32(age) / n, both
  divisions truncating toward zero and the subtraction done in float on the already advanced
  age, unless a section gives other operands.
* Integer → float conversions are exact for the values involved.

---

## 1. The effect step, 0x2dba40 (world, dt)

### 1.1 Iteration and removal

1. Create an empty temporary list of effects to remove.
2. Visit every effect in the list at world+0x60, front to back. Each visit is §1.2–§1.9. A
   visit either keeps the effect, transforms it in place (§1.9 step 6), or adds it to the
   removal list.
3. After the last effect, unlink and free every effect on the removal list. The remaining
   effects keep their order.

### 1.2 The owner

Look the owner id (+0x00) up in the id map at world+0x450. Call the result **O**; it is null
when the id is absent (including the default −1).

### 1.3 Movement before the sub-steps, by kind

Let **k** = dt × 0.001 (0x42e514) where needed. The smoothing factor s(dt, c) is 0xa7a0 with
dt widened to double and c the stated float constant (`creature-upkeep.md` §4.9).

**Kinds 3, 8, 9:** nothing here.

**Kind 4** (and **kind 10** with the differences noted):

1. d = (target − position) per axis, fixed to blocks.
2. If dx² + dy² + dz² > 1.0: scale d by (1.0 / sqrt(that sum)) (one reciprocal, then three
   multiplications).
3. d = d × 50.0 (0x42e5b0); for kind 10, × 20.0 (0x42e5a0).
4. f = s(dt, 0.002 (0x42e6d0)). Velocity v = v + (d − v) × f per axis.
5. If the age (+0x70, not yet advanced) > 500.0 (0x42e5c8): g = s(dt, 0.005 (0x42e518)).
   Let e = (target − position) per axis, fixed to blocks, all three taken before any change.
   Per axis: take e − 0.0, multiply by g, add 0.0, multiply by 65536.0, truncate to 64 bits,
   and add the result to the position. (The ±0.0 terms are in the code and change nothing
   but the sign of a zero.)

**Kind 5** (boomerang):

1. If O is null: put the effect on the removal list; the visit ends.
2. If age > 1200.0 (0x432944): removal; the visit ends.
3. If age > 1100.0 (0x472598): m = trunc64(((age − 1100.0) / 100.0) × 65536.0). For each axis,
   with D = O's position − effect position (64-bit): t = D × m (64-bit, wrapping); t = t / 65536
   truncated toward zero; t = t × m; t = t / 65536 truncated toward zero; position += t. All
   three D are taken before any change. (The pull grows as the square of (age − 1100) / 100.)
4. Else if age > 800.0 (0x432940): v = (O's position − effect position) per axis fixed to
   blocks, × 10.0 (0x42e598).
5. Else: aim = (O's position + to-fixed(O's +0x174..+0x17c offset)) − effect position, per axis
   fixed to blocks, × 5.0 (0x42e584). Then (0x16f20) with f = s(dt, 0.05 (0x42e524)):
   v = (aim − v) × f + v per axis (the three differences taken before f is computed).

**Every other kind** (0, 1, 2, 6, 7 and unknown kinds): gravity. v.z = v.z − (((dt × 0.001) ×
30.0 (0x42e5a8)) × 0.25 (0x4ac95c)).

### 1.4 Common part: displacement, age, expiry

1. The step displacement **d** = v × k per axis (taken now, from the velocity after §1.3).
2. age = dt + age.
3. If the kind is not 5 and age > float(lifetime):
   a. If the kind is not 6 and the owner id names a creature (0xe720), reset that creature's
      combo: a = 0x66460(owner); owner +0x70 (combo) = 0; b = 0x66460(owner); owner +0x6c
      (ability time) = (a / b) × owner +0x6c.
   b. Put the effect on the removal list; the visit ends.

### 1.5 Kinds 8 and 9 (no contact at all)

**Kind 8:** position += trunc64(d × 65536) per axis. The visit ends.

**Kind 9:**

1. position += trunc64(d × 65536) per axis.
2. f = 1.0 − (age + age) / float(lifetime); f4 = (f × f) × (f × f).
3. h = the terrain height (0x35ea0, §2.8) of the column (trunc-block x, trunc-block y) of the
   new position, fourth argument 1. If h ≤ 0 (or not a number), h = 0.
4. goal = trunc64(h × 65536) + (trunc64((f4 × 1000.0 (0x42e5d4)) × 65536) + 0x960000).
   (0x960000 is 150 blocks.)
5. g = s(dt, 0.01 (0x42e51c)). z += trunc64(float(goal − z) × g).
6. The visit ends.

### 1.6 Push vector and sub-steps

1. **Sub-step count** n = trunc32(sqrt(dx² + dy² + dz²) × 2.0 + 1.0) (dx, dy, dz the
   displacement of §1.4). If n < 1, the visit ends (the effect is kept).
2. **Sub-step move** m = (dx / float(n), dy / float(n), dz / float(n)), computed once.
3. **Push vector** P (passed to every hit): L² = vx² + vy² + vz² (current velocity). If L² > 0:
   q = 1.0 / sqrt(L²), hx = vx × q, hy = vy × q; otherwise hx = vx, hy = vy. c = +0x5c × 5.0;
   P = (hx × c, hy × c, +0x5c × 3.0 (0x42e580)).
4. Run sub-steps i = 0 … n − 1 (§1.7, §1.8). A sub-step can end the visit early by bursting
   (§1.9). If all n run, the effect is kept.

The age does not change between sub-steps, so every time test below gives the same answer in
each sub-step of one visit.

### 1.7 Sub-step, part A: the creature pass

At the start of each sub-step clear three things: **touched** (stops the pass), **landed** (a
hit returned true) and **struck** (the creature last passed to 0x29eb60 by this pass).

Walk the creature list (world+0x40) in order. For each creature C:

#### 1.7.1 Eligibility (skip C when any holds)

1. C's health (+0x180) ≤ 0.
2. The kind is 4 or 10.
3. Unless (kind 6 with hit type 2): O is not null and (C is O, or 0x50550(O, C) is false —
   C is not hostile to the owner). A healing area (kind 6, hit type 2) and an ownerless effect
   consider every living creature.

#### 1.7.2 Contact box

R = trunc64(radius (+0x54) × 65536); Hx, Hy, Hz = trunc64((C's +0x84, +0x88, +0x8c × 0.5) ×
65536). C is **missed** when any of these holds, tested in this order:

1. effect x + R < C x − Hx;  2. effect y + R < C y − Hy;  3. effect z + R < C z − Hz;
4. C x + Hx ≤ effect x − R;  5. C y + Hy ≤ effect y − R;  6. C z + Hz ≤ effect z − R.

(C's box is centred on its position on all three axes.)

#### 1.7.3 Line of sight

Missed unless 0x2b5d90(world, effect position, C position, flag 1, limit 200.0 (0x42e5c0), C)
is true (§2.5).

A missed C: if **touched** is already set, stop the pass; otherwise go to the next creature.
(**touched** is only set when the pass is about to stop anyway, so in practice: next creature.)

#### 1.7.4 Dodging

If C's dodge timer (+0x12c) is not exactly 0.0 (a not-a-number counts as not 0): if O is not
null and 0x50550(O, C) is true, insert C's id into O's set B (+0x9c8; 0x37a40, no duplicates).
Go to the next creature. Nothing else happens to C.

#### 1.7.5 Status areas (kind 6 with hit type 1, 4 or 6)

Only when the world is not a remote client:

1. **Hit type 6:** if C has a buff of type 4 (first match, 0x5e3a0), nothing in this step.
   Otherwise give C (0x65390, refresh flag 1) the buff {type 4, strength = effect damage
   (+0x50), duration 4000.0, source 0} and append a buff-change notice {source = O's id, target
   = C's id, that buff, flag 0}.
   **Hit types 1 and 4:** give C (refresh flag 1) the buff {type 0x0e, strength 0, duration
   200.0, source 0} and append the notice the same way.
   In both cases the buff's bytes 1–3 and +0x0c..+0x0f are not written, and when O is null
   the notice's source id is not written either (it holds whatever an earlier notice in this
   call left).
2. If the 500-ms boundary is crossed:
   a. Particle burst at C's position: velocity (0, 0, 5.0), alpha 1.0, size 0.4, count 15,
      spread 3.0, and by hit type: 1 → style 1, colour (1.0, 0.5, 0.3); 6 → style 2, colour
      (0.2, 1.0, 0.1); 4 → style 0, colour (0.6, 0.5, 0.3).
   b. Sound at C's position, volume 1.0, +0x28 = 0, pitch = r × 0.1 (0x42e528) / 32767 + 0.8
      (0x42e530) (one rand), id 0x30 for hit type 4, 0x2d for hit type 6. **For hit type 1 the id
      is not written**: it is whatever the same stack slot held (the id of the previous sound
      request built by this call of 0x2dba40, or older stack contents). See Open questions.

#### 1.7.6 The strike gate

If the hit type is 4, go to the next creature (no damage, no stop).

C is **struck** when any of these holds:

* the kind is 0, 1, 2, 3, 4 or 7 and F is 0;
* the kind is 6 and the 500-ms boundary is crossed;
* age > 300.0 (0x42e7e0), the kind is 5 and the 100-ms boundary is crossed.

If C is not struck, skip to §1.7.13. If struck, and the kind is neither 6 nor 5, set
**touched**. Then §1.7.7–§1.7.12 in order.

#### 1.7.7 Owner's mana gain

Only when F is 0, the kind is not 6, 7 or 3, and O is not null:

1. r1 = rand. g = (1.0 − (float(r1) + float(r1)) / 32767.0) × 0.05 (0x42e524) + 0.1.
2. If the kind is 5: g = g × 0.2 (0x42e700).
3. g = g × (float(0x4f220(O)) / 300.0 (0x42e7e0)) — the owner's current ability duration
   (§2.2).
4. m = 0x5f8d0(O) × g (mana factor first, then the product).
5. r2 = rand. If r2 mod 8 = 0: m = m + m.
6. If the world is not a remote client, or O is the local player: O's mana += m; if the result
   > 1.0 it becomes 1.0.

#### 1.7.8 Critical roll and damage

1. crit = false. If O is not null: r3 = rand; c = 0x50d30(O, C, hit type ≠ 0) (§2.1); crit =
   c > r3/32767. (The rand is drawn before 0x50d30 is called.)
2. D = effect damage (+0x50); if crit, D = D + D.

#### 1.7.9 Owner procs (0x5e820)

Only when O is not null and (O is the local player, or (the world is not a remote client and
O's hostility kind (+0x60) is not 0)):

1. Roll 0x5e820(O, 0.15 (0x42e6fc)) (§2.3; draws one rand). If false, or F is not 0, skip
   the rest of this section.
2. If O is class 2 specialisation 1 (+0x14c = 2, +0x14d = 1): give O (refresh flag 0) the buff
   {type 0x0a, strength 0, duration 30000.0, source 0}; append the notice {source = O's id,
   target = O's id, the buff, flag 0}; append a sound at O's position: id 0x32, pitch 1.0,
   volume 1.0.
3. If the kind is 3 and the hit type is 1: the same with buff type 9.

#### 1.7.10 Who applies

1. D' = max(0, D).
2. If O is not null, O's hostility kind is 0 (a player) and O is not the local player: go to
   the next creature (no hit, no freeze). That player's own game applies its effects.

#### 1.7.11 Heal or hit

**Hit type 2 (heal).** Only if O is not null, 0x50550(O, C) is false (C is friendly to the
owner; C may be O itself) and C's hostility kind is neither 7 nor 8:

1. Build a damage number: attacker = the effect's owner id (+0x00), target = C's id, amount =
   −D', critical = crit, +0x18..+0x1f = 0, position = C's position, push = (0, 0, 0), +0x44,
   +0x45, +0x46 = 0. (+0x15..+0x17 and +0x47 are not written.)
2. Append it to world+0x90.
3. If the world is not a remote client, or (O is the local player and C is the local player):
   append it to world+0x90 **a second time**, then apply it with 0x29dcf0(world, number).

Otherwise (hostile C, kinds 7/8, or no owner) nothing happens for hit type 2. **struck** is not
changed by a heal.

**Any other hit type.** Set **struck** = C. Call 0x29eb60 with:

| # | Value |
|---|---|
| 1 | world |
| 2 | C |
| 3 | O (may be null) |
| 4 | D' |
| 5 | crit |
| 6 | F |
| 7 | charge (+0x5c) |
| 8 | pointer to the effect's position |
| 9 | pointer to P (§1.6) |
| 10 | hit type (byte widened to 32 bits) |
| 11 | 0 (ability) |
| 12 | pointer to the effect record (its +0x00) |
| 13 | 1 (blockable) |

If it returns non-zero, set **landed**.

#### 1.7.12 Freeze timer

If float(+0x78) > C's +0x138, set C's +0x138 = float(+0x78). (Reached after a heal, after a
skipped heal and after a hit; not after §1.7.10's skip.)

#### 1.7.13 Stop rule

If the kind is 0, 1, 2, 3, 4, 7 or 10 (whether or not C was struck): set **touched** and stop
the pass. Otherwise (kinds 5, 6, unknown): next creature.

So kinds 0–3 and 7 stop at the first creature that passes eligibility, the box, line of sight,
is not dodging and the hit type is not 4.

### 1.8 Sub-step, part B: moving

1. **Kind 6:** the sub-step ends here (an area never moves and never bursts).
2. **Kind 5:** if the 100-ms boundary is crossed: sound at the effect's position, id 0x0f,
   pitch r × 0.1 / 32767 + 1.5 (0x42e548), volume 0.7 (0x3f333333), +0x28 = 0.
   **Kind 10:** if the 200-ms boundary is crossed: the same with pitch r × 0.1 / 32767 + 1.0.
   (Repeated in every sub-step, with a fresh rand each time.)
3. position += (trunc64(mx × 65536), trunc64(my × 65536), trunc64(mz × 65536)).
4. **blocked** = false. Kinds 4 and 10 skip the block tests (go to step 7).
5. Read the block at the new position (0xeab0, floor-block; an unloaded zone reads as solid
   type 1).
   * **Solid** (type not 0 and not 2): undo step 3 (subtract the same three amounts); blocked
     = true. If the kind is 0, 1 or 2, append a particle burst at the restored position:
     velocity (0, 0, 10.0), colour (0.5, 0.5, 0.5, 1.0), size 0.1, count 4, style 0, spread
     3.0.
   * **Empty or water:** sp = sqrt(vx² + vy² + vz²) × (dt × 0.001); u = the velocity
     normalised by 0x179b0 (one reciprocal of the length, three multiplications); back = u ×
     −1.0 (0x4aca4c). t = 0x2d1d20(world, position, back, sp, stop-at-first-blocking, honour
     byte-5 bit 6) (§2.5). If sp > t: blocked = true (the position is **not** undone).
     Note the ray length is the whole step's distance, not the sub-step's.
6. **Block destruction**, only when blocked and the world is not a remote client:
   1. any = false. For a = −4 … 4 (x offset, outermost), b = −4 … 4 (y), c = −4 … 4 (z,
      innermost): q = position + (a, b, c) × 65536. Let o = (q − position) per axis fixed to
      blocks. If ox² + oy² + oz² < 16.0 (0x4aca34), read the block at q (0xeab0). If its byte 5
      has bit 0 set:
      * particle burst at q: velocity (0, 0, 10.0), colour (byte0, byte1, byte2) × 1/128
        (0x4386e0) and alpha 1.0, size 0.5, count 3, style 0, spread 3.0;
      * 0x2a59f0(world, q): replaces that block with an empty one (water when its floor-block
        z < 1) (§2.7);
      * append a block-change record for q (old block = the bytes read);
      * any = true.
   2. If any: sound at the effect's position, id 0x1a, pitch r × 0.4 (0x42e52c) / 32767 + 0.5,
      volume 1.0, +0x28 = 0. Then 0x2e0710(world, trunc-block(x − 4·65536),
      trunc-block(y − 4·65536), trunc-block(x + 4·65536), trunc-block(y + 4·65536)) (§2.7).
7. **Wall resets the combo:** if **touched** is clear, blocked is true, the kind is not 6 and O
   is not null: a = 0x66460(O); O +0x70 = 0; b = 0x66460(O); O +0x6c = (a / b) × O +0x6c.
8. **Kind 5:** if **landed**: O +0x70 += 1; 0x603f0(O) (no effect); O +0x74 = 0. If blocked:
   velocity = (0, 0, 0). The sub-step ends (a boomerang never bursts).
9. **Other kinds** burst (§1.9) when **touched**, or blocked, or (kind 4 or 10 and the
   distance test 0xf3e20(position, target, 2.0) says "not farther" and age ≥ 500.0). Otherwise
   the sub-step ends.

### 1.9 The burst

A burst ends the visit; the remaining sub-steps do not run.

1. **Visible burst** — kinds 3, 4 and 10 always; kinds 0, 1, 2, 7 and others only when F ≠ 0
   (then without the particle of 1b):
   a. Kind 4: sound at the effect's position, id 0x54, pitch r × 0.4 / 32767 + 1.0, volume
      1.0. Kind 10: id 0x22, pitch r × 0.4 / 32767 + 1.5.
   b. Kinds 3, 4, 10: particle burst at the effect's position, velocity (0, 0, 10.0), spread
      3.0, count and size 20 and 0.3 (kinds 4, 10) or 8 and 0.2 (kind 3); if the kind is 4 or
      the hit type is 1: style 1, colour (1.0, 1.0, 0.0, 1.0); otherwise style 2, alpha 1.0,
      colour (0.3, 0.8, 0.2) for kind 10, (0.25, 0.5, 1.0) otherwise.
   c. If **touched** is clear: sound at the effect's position, volume 1.0, pitch r × 0.4 /
      32767 + 1.0, id: kinds 3, 4, 10 → 0x2d for hit types 2 and 6, else 0x2a; other kinds →
      0x14.
   d. **Area pass** with radius ra = 10.0 (kinds 4, 10) or 5.0 (others) — next step.
   Kinds 0, 1, 2, 7 (and others) with F = 0 skip all of 1 and go to step 4.
2. **Area pass.** Walk the creature list in order. For each C:
   1. Skip C if it is **struck** (the creature last passed to 0x29eb60 in this sub-step).
   2. If O is not null:
      * if the world is not a remote client, O's hostility kind is 0 and O is not the local
        player: **end the whole area pass** now;
      * skip C if it is O, or 0x50550(O, C) is false, or C's health ≤ 0.
      If O is null, no hostility or health test is made.
   3. Contact box as §1.7.2 with ra in place of the radius; line of sight as §1.7.3.
   4. Dodging as §1.7.4 (insert into O's set B when O exists — here without the hostility
      re-test — and skip C).
   5. crit = false; if O is not null: r = rand, crit = 0x50d30(O, C, hit type ≠ 0) > r/32767.
      D = +0x50, doubled when crit (**not** clamped at 0).
   6. Call 0x29eb60 with the arguments of §1.7.11 except argument 4 = D. If it returns
      non-zero, set **landed**.
   No mana gain, procs, heals or freeze happen in the area pass.
3. (End of the area pass.)
4. **Combo:** if **landed**, the kind is not 6 and O is not null: O +0x70 += 1; 0x603f0(O);
   O +0x74 = 0.
5. **Becoming an area:** if (kind 3 and hit type 2) or kind 10:
   * radius = 16.0 for kind 10; otherwise +0x5c × 5.0 + 5.0;
   * kind = 6;
   * age = 0.0 if F ≠ 0, otherwise 3000.0 (so with the default 5000-ms lifetime the area lasts
     5000 ms after a charged spell, 2000 ms otherwise);
   * damage = damage × 0.05; charge (+0x5c) = 0; F = 0.
   Position, velocity, hit type, owner, lifetime and +0x78 are kept. The effect stays in the
   list and the visit ends.
6. Otherwise put the effect on the removal list.

### 1.10 Consequences worth stating

* **Areas take more than one tick per boundary.** Gravity keeps accelerating an area's
  velocity (§1.3) even though it never moves, so its sub-step count n grows over its life,
  and the creature pass of §1.7 runs n times per visit with the same age. Each run strikes
  again when the 500-ms boundary is crossed, with fresh rands. An area made from a burst keeps
  the projectile's velocity too.
* An effect that touches a creature still makes the rest of its sub-step (move, block tests,
  block destruction) before bursting at the moved position.
* Only one damage application per creature and sub-step: the area pass skips the creature the
  contact hit just struck (but not a creature that was healed or only touched with F set).

### 1.11 rand() order within one visit

In order of occurrence, each only when its condition holds:

1. Per sub-step, per creature in world order: status-area sound (§1.7.5); mana gain r1, r2
   (§1.7.7); critical r3 (§1.7.8); the proc roll inside 0x5e820 (§1.7.9); then whatever
   0x29eb60 or 0x29dcf0 draw.
2. Per sub-step: kind 5 / kind 10 whoosh sound (§1.8 step 2); block-destruction sound (§1.8
   step 6).
3. At the burst: kind 4 / 10 sound, burst sound (§1.9 step 1), then per creature of the area
   pass the critical rand followed by 0x29eb60's draws.

---

## 2. Callees of the step

### 2.1 Critical chance, 0x50d30 (attacker A, target T or null, flag "magic")

1. c = A's +0x18c × 0.5 + 0.0.
2. If A's ability (+0x68) is 0x8c and float(0x4f220(A)) > A's +0x6c: c = c + 0.5.
3. For each slot whose item type matches (+0x638 type 3, +0x6d8 type 3, +0x3b8 type 4, +0x458
   type 6, +0x4f8 type 5, +0x598 type 7, +0x318 type 8, +0x778 type 9, +0x818 type 9), in this
   order: c = c + itemCrit(item, A).
4. If T is not null: c = c − (magic ? 0x5f740(T) : 0x50ea0(T)).
5. Return c.

**itemCrit, 0x1090f0** (types 3..9 only, else 0):

1. a = 0.01 (0x42e51c). a = 0.02 (0x42e6e8) when the item is a weapon (type 3) whose subtype
   is ≤ 23 and in the two-handed set, or its type is 4, 8 or 9.
2. If type 4 and subtype 1: a = a + a.
3. b = 1.0 − float(seed (+0x08, unsigned) mod 21) / 20.0.
4. Material (+0x1c) 6: a = a × 0.5. Material 0x0b: b = b + 1.0.
5. v = (((power(item, A, 0) × 0.0625 (0x4328a4)) × a) × b) × 3.0, power being 0x109c50.
6. Return v, or 0 when 0.001 > v.

**Defence against crits.** 0x50ea0(T) = (armour(T) (0x4f080) / powf(2.0, 4.0)) × 0.05, then
× 0.25 if T has a buff of type 0x0e (first match). 0x5f740(T) is the same with resistance
(0x64d90). (powf(2, 4) is 16 exactly.)

### 2.2 Ability duration 0x4f220 = effect time 0x4ead0 + middle 0x660d0 + recovery

Each part is either a fixed integer or trunc32(K / S) with S = 0x66460(creature) (each part
calls it separately). 0x4ead0 and 0x660d0 called with −1 use the creature's current ability;
their values are exactly parts A and B below. The sum is returned as a 32-bit integer.

**Part A (effect time, 0x4ead0):**

| Abilities | Value |
|---|---|
| 0, 1, 9, 0x0c, 0x34, 0x38, 0x3d–0x3f, 0x50–0x52, 0x5f, 0x6e, 0x74, 0x76, 0x78–0x7a, 0x92, 0x95, 0x9b, 0x9e, 0x9f | 0 |
| 2, 0x0a, 0x1b, 0x45–0x47, 0x4a–0x4f | 300/S |
| 3–8, 0x0b, 0x0f, 0x12–0x16 | 100/S |
| 0x18–0x1a, 0x1c, 0x25, 0x3c, 0x40, 0x41, 0x93 | 50/S |
| 0x1d, 0x32, 0x71, 0x8c, 0x8e, 0x8f | 100 |
| 0x1f, 0x21, 0x3a, 0x3b, 0x67 | 800/S |
| 0x20, 0x22, 0x23 | 1600/S |
| 0x26, 0x2c, 0x62 | 600/S if the +0x6d8 item's subtype (+0x6dc) is 12, else 1200/S |
| 0x27, 0x2d, 0x42, 0x43, 0x68, 0x6b | 500/S |
| 0x2e, 0x30 | 1200/S |
| 0x37 | 400 |
| 0x48, 0x49 | 200 |
| 0x60 | 5000/S |
| 0x64, 0x69, 0x6a | 1000/S |
| 0x6c | 100 if the creature has a buff of type 13, else 200/S |
| 0x0d, 0x0e, 0x10, 0x11, 0x28–0x2b, 0x44, 0x94 | 200/S |
| 0x75, 0xa1 | 3000 |
| 0x85 | 60000 |
| 0x86–0x88 | 2000/S |
| 0x89, 0xa2, 0xa3 | 5000 |
| 0x8b | 10 |
| 0x90 | 1500/S |
| 0x98 | 120000 |
| 0x9a | 2000 |
| any other (and ids > 0xa3) | 400/S |

**Part B (middle, 0x660d0):**

| Abilities | Value |
|---|---|
| 0, 1, 9, 0x19, 0x1a, 0x1c, 0x33, 0x3c, 0x40, 0x50–0x52, 0x76, 0x78–0x7a, 0x86–0x89, 0x90, 0x92, 0x9a, 0xa1–0xa3 | 0 |
| 2, 3, 0x0a, 0x13, 0x14, 0x44 | 200/S |
| 4, 5, 0x26, 0x2c, 0x3f | 100/S |
| 6, 0x0d, 0x11, 0x12, 0x15, 0x16, 0x42, 0x43 | 400/S |
| 7, 8 | 150/S |
| 0x0b | 200 |
| 0x0e–0x10 | 250/S |
| 0x17, 0x18 | 50/S |
| 0x1b, 0x23 | 1200/S |
| 0x1d, 0x69, 0x6a | 2000 |
| 0x1f, 0x21, 0x4a, 0x67 | 600 |
| 0x20, 0x22 | 1200 |
| 0x2e, 0x30, 0x38, 0x68, 0x6b | 500/S |
| 0x2f | 1000/S |
| 0x34, 0x6c | 250 if the creature has a buff of type 13, else 500 |
| 0x4d–0x4f | 500 |
| 0x37 | 100 |
| 0x45, 0x46, 0x9b, 0x9f | 1000 |
| 0x48, 0x49, 0x82 | 3000 |
| 0x5f | 20000 |
| 0x64 | 6000 |
| 0x8b | 5000 |
| 0x8c | 5000/S |
| 0x94 | trunc32(+0xba8 × 1000.0) |
| any other | 300/S |

**Part C (recovery, only inside 0x4f220):**

| Abilities | Value |
|---|---|
| 1, 9, 0x19, 0x1a, 0x1c, 0x1d, 0x32, 0x3c, 0x40, 0x50–0x52, 0x5f, 0x76, 0x78–0x7a, 0x89, 0x92, 0x9a, 0x9b, 0x9f, 0xa1–0xa3 | 0 |
| 0, 0x34 | 100 |
| 4–6, 0x3a, 0x3b, 0x3f | 300/S |
| 8, 0x0f, 0x13, 0x42, 0x43 | 200/S |
| 0x0b | 600 |
| 0x0c, 0x17, 0x1b, 0x1f–0x23, 0x26–0x2e, 0x30, 0x3d, 0x3e, 0x44, 0x68–0x6b, 0x74, 0x86–0x88, 0x90, 0x9e | 100/S |
| 0x10 | 400/S |
| 0x18 | 10/S |
| 0x37 | 400 |
| 0x45, 0x46, 0x67 | 800/S |
| 0x48, 0x49 | 200 |
| 0x6c | 50 if the creature has a buff of type 13, else 100 |
| 0x8b | 10 |
| any other | 500/S |

### 2.3 Proc roll, 0x5e820 (creature, p)

If the creature's +0x6d8 item is a weapon (type 3): subtypes 0, 1, 2, 12 → p × 0.5; subtypes
8, 10 → p × 0.3 (0x42e704). Then one rand; true when p > r/32767.

### 2.4 Small callees

* 0x5f8d0 mana factor, 0x66460 attack speed: `creature-upkeep.md`.
* 0x5e3a0 (creature, type): the first buff of that type, or none.
* 0x37a40: insert an id into an ordered id set (nothing when present).
* 0x603f0: walks the creature's cooldown map and changes nothing.
* 0x179b0: q = 1.0 / sqrt(x² + y² + z²); returns (q·x, q·y, q·z).
* 0x16f20 (v, target, dt, c): computes the three differences target − v, then f = 0xa7a0(dt, c),
  then v = difference × f + v per axis.

### 2.5 Line of sight 0x2b5d90 and the block ray 0x2d1d20

**0x2b5d90 (world, A, B, flag, limit, creature):**

1. If 0xf3e20(A, B, limit) says "farther": false.
2. d = (B − A) per axis fixed to blocks; s = (dx² + dy²) + dz². If 0.0001 (0x4ac954) > s: true.
3. L = sqrt(s); u = d / L (three divisions). t = 0x2d1d20(world, A, u, L,
   stop-at-first-blocking, honour = flag). True when t ≥ L − 0.01 (0x42e51c).

**0x2d1d20 (world, start, direction u, maximum M, mode, honour) → distance:**

1. cell = floor-block of start (0xfd80). Look up the zone at (floor(cell.x / 64),
   floor(cell.y / 64)) in world+0x460; a missing zone or a zone without data is "none".
2. t = 0. If not (M > 0): return min(M, 0).
3. Loop:
   1. If the zone is none, stop.
   2. Read the cell's block from the zone column (0xeb90). It is **passable** when its type is
      0 or 2 and (honour is 0 or byte 5 bit 6 is clear). Continue only while "not passable"
      equals the mode (mode 0 used here: continue while passable); otherwise stop.
   3. If more than 500 steps have been taken, stop; count this step.
   4. Current point p = start + trunc64((u × t) × 65536) per axis.
   5. step = 10.0, axis = 0. For x, then y, then z, when not (0.000001 (0x42e50c) > u²):
      boundary = (cell + 1) × 65536 if u > 0, else cell × 65536; s = float(boundary − p) / u;
      s = float(trunc64(s)) × 1/65536. If s < step (strictly): step = s, axis = this axis.
   6. cell[axis] += 1 if u[axis] > 0, else −= 1. If the cell's zone coordinates changed,
      look the zone up again.
   7. t = t + step. Continue while M > t.
4. Return M if M < t, else t.

### 2.6 Smoothing 0xa7a0

As in `creature-upkeep.md` §4.9: n = trunc32(dt × 100.0) (doubles, 0x4aca18), q = c × 0.02
(double, 0x42e540), acc = 0; for i = 0 … n − 1: acc = acc + ((i + 1.0) / n − acc) × q (all
double); result = acc rounded to float.

### 2.7 Blocks

* **0xeab0 (world, position):** floor-block coordinates; zone (floor(x/64), floor(y/64)); a
  missing zone or zone without data gives the default block at 0x54dcb0 (c8 c8 c8 00 01 00,
  solid); otherwise the zone column's block (0xeb90).
* **0x2a59f0 (world, position):** floor-block coordinates; 0x91160 writes, if the zone exists
  and z lies inside the column's stored range, the block at 0x5514d0 (ff ff ff 00 02 00, water)
  when z < 1, else the block at 0x5514c8 (ff ff ff 00 00 00, empty). Both are set at start-up.
* **0x2e0710 (world, x0, y0, x1, y1)** (block coordinates): each value below 0 becomes 0; zone
  range = floor(x0/64)…floor(x1/64) × floor(y0/64)…floor(y1/64). For each existing zone, its
  list of 0x40-byte static entries is walked from last to first; an entry whose byte +0x38 has
  bit 1 set, and whose block one unit below its position (floor-block of x, y and z − 65536,
  read with 0xe9a0) has type 0, is erased (later entries shift down). This drops objects left
  floating by destroyed blocks.

### 2.8 Terrain height 0x35ea0 (world, block x, block y, flag)

The world generator's surface height for a column, in blocks (the same routine the map uses,
`map-markers.md`). Only kind 9 uses it here. Not specified further.

---

## 3. Creation in the ability routine 0x2c5250 (world, creature C, dt)

### 3.0 Common rules

* **Who creates.** Every branch below runs only when (the world is not a remote client and C's
  hostility kind is not 0) or C is the local player, unless stated. (On a server or single
  player: non-player creatures and the local player; on a client: the local player only.)
* **Time.** t = C's ability time (+0x6c) as it was before this step (0x2de4c0 advances it
  later). "**Crosses X**" means t ≤ X < t + dt with X converted to float (t + dt computed as
  dt + t). T = 0x4ead0(C, −1), M = 0x660d0(C, −1) (§2.2).
* **Aim** is C's +0x174..+0x17c; "normalised aim" is 0x179b0 of it.
* **Launch position** = C's position + to-fixed(the shot's own velocity × (dt × 0.001)) per axis (the scalar
  dt × 0.001 multiplies each component, then to-fixed).
* **Both lists.** Each record is appended to world+0x60 and to world+0xd0 (§0.1).
* **Callees.** m = 0x50f20(C, ability), w = 0x4fa70(C, 0) (attack power, see `combat.md`),
  p = 0x65c70(C, 0) (power value, `skills-and-interactions.md` §2.5).

  0x50f20 multiplier: 6, 0x1b → 0.5; 0x16 → 0.1; 0x1f, 0x21 → 0.4; 0x20, 0x22 → 0.6; 0x26,
  0x2c, 0x3a, 0x3b, 0x45, 0xa0 → 2.0; 0x2f, 0x69, 0x6a → 0.75; 0x35 → 3.0; 0x38 → 0.25; 0x49
  → 0.2; every other ability 1.0.

  Mana cost 0x5f7c0(C, ability): 0x20, 0x22, 0x26, 0x2c, 0x2e, 0x30, 0x69, 0x6a → 0.3, or 0
  when C has a buff of type 9; 0x23 → 0.0625; 0x33 → 1.0; 0x61 → 0.5; 0x86–0x88 → 0.03125;
  others 0.

  Stamina cost 0x65ea0(C, ability): 0 when C has a buff of type 13; else 0x16 → 0.5; 0x32,
  0x52, 0x6c → 0.25; 0x34, 0x37, 0xa0 → 1.0; others 0.
* **Variation** V = 1.25 (0x42e71c) − (r × 0.5) / 32767, one rand per created record.
* **Consuming a buff.** "Consume buff X" means: if C has a buff of type X, give C (refresh flag
  0) the buff {type X, strength 0, duration 0, source 0} — which overwrites the existing one —
  and append a notice {source = C's id, target = C's id, that buff, flag 0}.

### 3.1 Shots and arrows: abilities 0x17, 0x18, 0x1b, 0x6c, 0xa0

1. If t = 0.0 exactly: stamina (+0x9a0) −= 0x65ea0(C, ability); if negative, 0.
2. If t crosses T: if 0x5f7c0(C, ability) > 0, mana −= 0x5f7c0(C, ability) (called again);
   otherwise mana −= +0x158. If mana < 0, 0. Then +0xba8 (charge) = +0x158; +0x158 = 0.
3. A record is created when t crosses T, or (ability 0x1b, charge > 0 and t crosses T + 300)
   — the boomerang's second throw. Otherwise nothing more.
4. Record (constructor defaults, then):
   * velocity = normalised aim × 100.0 (0x42e5bc); age = 0; radius = 0.5;
   * ability 0x1b: q = charge × 0.2; radius = (charge + charge) + 2.0; otherwise q = charge;
   * +0x5c = q; if 0x5f7c0(C, ability) > 0: q = 0x5f7c0(C, ability) and +0x5c = q;
   * +0x60 = q;
   * F = 1 when charge > 0 or the ability is 0x18, else 0;
   * position = the launch position; owner = C's id;
   * if F: consume buff 0x0a;
   * kind: 0x6c → 2; 0xa0 → 10 with hit type 6, +0x58 = 1.0, +0x5c = 0.0, radius 1.0, F = 1,
     velocity (0, 0, 50.0), target = C's position + to-fixed(aim); 0x1b → 5 with +0x58 = C's
     +0xb4; 0x17, 0x18 → 1 if C's class (+0x14c) is 0x95, else 0;
   * damage: kk = 5.0 (1.0 for 0x1b); base = 1.0, or 0.1 if +0x5c > 0, or 0.0 for ability
     0x18 (tested last); mw = m × w; r = rand; damage = (V + 0.0) × (((charge × charge) × kk +
     base) × mw).
5. Append to both lists.
6. Sound at C's position, volume 1.0, +0x28 = C's id, one rand r:
   * 0x17, 0x18: x = r × 0.5 / 32767; F ≠ 0 → id 0x16, pitch x + 1.0; F = 0 → id 0x0f, pitch
     x + 2.0;
   * 0x1b, 0xa0: id 0x0f, pitch = float((double(r) × 0.51) / 32767.0 + 0.7) in double
     (0x4721f0, 0x446198, 0x4721f8 — the last is the double of float 0.7);
   * 0x6c: id 0x16, pitch r × 0.5 / 32767 + 1.0.

### 3.2 Repeating shots: ability 0x38

1. If t crosses T: +0xba8 = +0x158; mana −= +0x158 (no floor); +0x158 = 0; consume buff 0x0a.
2. step = M / 4 (integer division toward zero). Shoot when T ≤ t, t < float(T + M), and
   trunc32(t) / step ≠ trunc32(t + dt) / step, and the aim's squared length (x² + y² + z²) > 0.
3. Sound at C's position (same who-creates gate): id 0x16, pitch r × 0.5 / 32767 + 1.0, volume
   1.0, +0x28 = C's id.
4. Record: constructor; velocity = normalised aim × 150.0 (0x42e938); age 0; radius 0.5;
   +0x5c = charge × 0.25; +0x60 = 0; F = 1; position = launch position; owner C; kind = 1 if
   C's class is 0x95, else 0; damage = (V + 0.0) × (((charge × charge) × 5.0 + 0.1) × (m ×
   w)), the rand drawn after the sound's.
5. Append to both lists.

### 3.3 Spells: abilities 0x26–0x30 and 0x7b

1. **Charging sound:** if float(T − 200) ≥ t and trunc32(t) / 200 ≠ trunc32(t + dt) / 200:
   sound at C's position, id 0x27, pitch r × 0.25 / 32767 + 1.0, volume 1.0, +0x28 = C's id.
2. **Shot moment** S = T. For 0x2e, 0x2f, 0x30: third = M / 3 (toward zero); e =
   trunc32((dt + t) − float(T)); S = T + (e − e mod third) (C remainder); if S < T, S = T.
3. If t crosses T and the aim's squared length > 0: mana −= 0x5f7c0(C, ability) (no floor).
4. A record is created when t < float(T + M), t crosses S, and the aim's squared length > 0.
5. Record: constructor; velocity = normalised aim × 100.0 if C's +0x6dc (slot B subtype) is
   12, else × 50.0; age 0; radius = +0x158 × 4.0 + 0.5; position = C's position; +0x5c =
   +0x158; +0x58 = +0x158 + 0.5; if C's race (+0x64) is 0x65: y += to-fixed(C's +0x88) and z
   += to-fixed(C's +0x8c × 0.5); owner C. Then by ability:
   * **0x7b:** kind 7; damage = (V × (m × w)) × 5.0; +0x58 = 4.0; +0x5c = 1.0; velocity =
     normalised aim × 100.0.
   * **0x2f:** kind 4; damage = (V × (m × w)) × 5.0; +0x58 = 3.0; +0x5c = 0.5; radius 5.0;
     velocity (0, 0, 50.0); target = C's position + to-fixed(aim).
   * **every other ability of the branch:** kind 3 and damage = V × (m × p). Then 0x27–0x2b
     and 0x2d keep +0x58 and +0x5c; 0x2e and 0x30 set +0x58 = +0x58 + 0.5 and +0x5c = 0.25;
     the rest (0x26, 0x2c) set +0x58 = +0x58 + 1.0 and +0x5c = 1.0.
   * hit type: if C's class is 3 or 0x98: 2 when the specialisation (+0x14d) is 1, else 1;
     other classes leave 0.
   * F = 1 for abilities 0x26, 0x2c, 0x2e, 0x30 (the bit set 0x1504000000000 over ids ≤ 0x30),
     else 0 — this replaces any F set above.
6. Append to world+0x60. If +0x158 > 0: mana −= +0x158 (floor 0) and +0x158 = 0. If F: consume
   buff 9. Append the record to world+0xd0.
7. Sound at C's position, volume 0.9 (0x42e534), +0x28 = 0: hit type 2 → id 0x2c, pitch 1.25
   for abilities 0x2a, 0x2b, 0x2d, else 1.0; otherwise id 0x29, pitch 2.0 for 0x27–0x29,
   else 1.0.

### 3.4 Cast areas: abilities 0x62, 0x63

1. For every C (no who-creates gate): if t < float(T) and trunc32(t)/200 ≠ trunc32(t + dt)/200:
   sound at C's position, id 0x27, pitch r × 0.25 / 32767 + 1.0, volume 1.0, +0x28 = 0.
2. With the gate, when t crosses T:
   1. sound at C's position, id 0x29, pitch r × 0.25 / 32767 + 1.0, volume 1.0;
   2. record built without the constructor: owner C, position C's position, target 0, +0x38..
      +0x43 = 0, velocity (0, 0, 0), damage = (m × p) + (m × p), radius 8.0 (0x4aca30), +0x58 =
      1.0, +0x5c = 0, +0x60 = 0, F = 0, kind 6, hit type 1 (0x62) or 2 (0x63 — the **healing
      area**), age 0, lifetime 5000, +0x78 = 0;
   3. append to both lists.

### 3.5 Ranger area: ability 0x8e

When t = 0.0 exactly: a record built without the constructor: owner C, position C's position,
target 0, velocity 0, damage = m × w, radius = 0x61110(1000) (≈ 9.7617 for the fixed skill
level 1000 of 0x65690), +0x58 = 1.0, +0x5c = 0, +0x60 = 0, F = 0, kind 6, hit type 4, age 0,
lifetime = 0x610d0(1000) (= 19523), +0x78 = 0. Append to both lists.

* 0x61110(L) = (1.0 − 1.0 / ((float(L) − 1.0) / 50.0 + 1.0)) × 5.0 + 5.0.
* 0x610d0(L) = trunc32((1.0 − 1.0 / ((float(L) − 1.0) / 50.0 + 1.0)) × 10000.0 + 10000.0)
  (0x42e5e8).

Such an area only applies buff 0x0e to the owner's enemies (hit type 4 never damages).

### 3.6 Healing areas left by the beam (abilities 0x68–0x6b, 0x74, 0x1d; class 3 spec 1)

*Note (creature-abilities-ranged.md): the 0x74 and 0x1d paths in this branch are unreachable, because the ability switch sends those ids to other branches (melee and ally heal). Only 0x68–0x6b reach it.*

The beam itself strikes creatures directly with 0x29eb60 and is not an effect object. Only for
a creature of class 3 and specialisation 1, it leaves healing areas (kind 6, hit type 2):

The beam fires when t crosses T and, for abilities 0x69, 0x6a and 0x1d, again whenever
trunc32(t − T) / 200 ≠ trunc32((t − T) + dt) / 200 while T < t + dt and t < T + M.

* **At the beam's end**, every time the beam fires (no further time test): constructor; kind 6; F = 0;
  +0x5c = 0; +0x60 = 0; damage = p × 0.1; radius 4.0; hit type 2; owner C; position = the beam
  origin (0x654a0) + trunc64((normalised aim × length) × 65536) per axis, length being the
  beam's reach (the block ray 0x2d1d20 from the origin, at most 200, or 10 for 0x74); +0x58 =
  1.0; age 0; velocity 0; lifetime 5000.
* **At a struck enemy**, for each enemy the beam strikes while t crosses T: the same record
  at the beam's sample point where the enemy was found.

Both are appended to both lists.

### 3.7 Flight landing marker: ability 0x9c (local player only)

When the flight's elapsed time reaches its duration, after C is placed at the destination:
constructor; kind 8; +0x5c = 0; velocity = ((destination − start) fixed to blocks with z = 0)
normalised (0x127f0) × 20.0; age 0; radius 0; position = C's position; +0x58 = C's +0x84 × 1.5.
Append to both lists; sound at C's position, id 0x79, pitch 1.0, volume 1.0. (Its expiry, like
any kind other than 6, resets C's combo, §1.4.)

### 3.8 Other creators (not specified here)

* 0x2bb8e0 (creature physics) leaves kind-6 areas of hit type 5 behind a creature with buff
  0x10 or ability 0x92.
* 0x2e17d0 (called by the world tick) appends effects.
* 0x29d980 and 0xa5a50 copy effects received from the network into world+0x60.
* No creator of kind 9 was found in 0x2c5250.

---

## 4. Constants

| RVA | Value | Use |
|---|---|---|
| 0x42e510 | 1/65536 | fixed to blocks |
| 0x42e5f8 | 65536.0 | to fixed |
| 0x42e5f0 | 32767.0 | rand divisor |
| 0x4ac980 | 1.0 | |
| 0x4ac964 | 0.5 | half-extents, crit |
| 0x42e584 | 5.0 | push, aims, AoE radius, areas |
| 0x42e580 | 3.0 | push z |
| 0x42e514 | 0.001 | ms → s |
| 0x42e5a8 | 30.0 | gravity |
| 0x4ac95c | 0.25 | gravity, crit buff |
| 0x42e5b0 | 50.0 | kind 4 speed, spell speed |
| 0x42e5a0 | 20.0 | kind 10 speed, item crit, flight |
| 0x42e6d0 | 0.002 | homing velocity smoothing |
| 0x42e518 | 0.005 | homing position smoothing |
| 0x42e51c | 0.01 | kind 9 smoothing, item crit, sight margin |
| 0x42e524 | 0.05 | boomerang smoothing, mana gain, area damage |
| 0x42e5c8 | 500.0 | homing start, burst age |
| 0x432944 / 0x472598 / 0x432940 | 1200 / 1100 / 800 | boomerang phases |
| 0x42e5bc | 100.0 | boomerang pull, arrow speed |
| 0x42e598 | 10.0 | boomerang return, AoE radius |
| 0x42e5d4 | 1000.0 | kind 9 height |
| 0x4ac9d0 | 2.0 | sub-steps, reach test |
| 0x4aca4c | −1.0 | back ray |
| 0x4aca34 | 16.0 | destruction radius² |
| 0x4386e0 | 1/128 | debris colour |
| 0x42e5c0 | 200.0 | sight limit |
| 0x42e528 | 0.1 | pitches, base damage, mana gain |
| 0x42e530 | 0.8 | status pitch |
| 0x42e52c | 0.4 | pitches |
| 0x42e548 | 1.5 | pitches, landing marker |
| 0x42e700 | 0.2 | boomerang mana, 0x1b charge |
| 0x42e7e0 | 300.0 | mana gain, boomerang strike age |
| 0x42e6fc | 0.15 | proc chance |
| 0x42e704 | 0.3 | proc weapon factor |
| 0x4acb30 | sign mask | heal negation |
| 0x4ac954 | 0.0001 | sight minimum |
| 0x42e50c | 0.000001 | ray axis threshold |
| 0x4aca20 | 4.0 | powf exponent, spell radius |
| 0x42e6e8 | 0.02 | item crit |
| 0x4328a4 | 0.0625 | item crit, mana cost |
| 0x42e71c | 1.25 | variation |
| 0x42e534 | 0.9 | spell sound volume |
| 0x4721f0 / 0x446198 / 0x4721f8 | 0.51 / 32767.0 / 0.7 (doubles) | boomerang sound |
| 0x42e938 | 150.0 | repeating shot speed |
| 0x4aca30 | 8.0 | cast area radius |
| 0x42e5e8 | 10000.0 | ranger area lifetime |
| 0x437f60 | 0.0 | added to V in §3.2 |
| immediates | 4000.0, 200.0, 30000.0, 3000.0, 16.0, 0.7, 0x960000 | buff durations, conversion age and radius, whoosh volume, kind 9 |

---

## 5. Verification

ZoneExtractor's `effects.request` records calls of 0x2dba40 into `effects.bin` (magic `CFX1`),
one length-prefixed entry per call. Each entry holds: the remote-client and local-player flags,
dt; every effect record (0x80 bytes) before the call; every creature (owners in full with
their buffs, aggro and hits-dealt maps and the two id sets; others' first 0x200 bytes and
buffs); cubes of blocks around each effect with per-column "zone loaded" flags; the six world
lists' lengths before (sounds 0x30, particles 0x48, damage numbers 0x48, buff notices 0x30,
aggro notices 0x18, block changes 0x20); the effects and creatures after; the entries each
list gained; every call of 0x29eb60 requested (79 bytes: target, attacker, damage, critical,
flag, charge, position, push, hit type, ability, record pointer, blockable) with its result;
and every rand() value drawn during the call.

An implementation is checked by rebuilding the world from the "before" data, feeding the
recorded rand values in order, running one step, and comparing:

* the effect list after (ignoring +0x65..+0x67, +0x6d..+0x6f, +0x7c..+0x7f);
* every creature record and buff list;
* the gained sounds (ignoring +0x24..+0x27), particles, damage numbers (ignoring +0x15..+0x17
  and +0x47), buff notices, aggro notices and block changes, in order;
* the hit requests (ignoring argument 12's pointer) and that the same results come back;
* the number of rand values consumed (exactly all of them).

**The 400 arrow steps** (entries whose effects are kinds 0–2) exercise: gravity and the
displacement taken before the age advance; the sub-step count from the displacement; the
contact box and the line-of-sight ray; stopping at the first creature; the order mana gain r1,
r2 → critical r3 → proc rand → the hit routine's draws; the 13 hit arguments (push from the
normalised horizontal velocity, charge, flag F, hit type); the solid-block undo with its grey
particle versus the back-ray block without undo; the combo reset on a wall and on expiry; and
removal (arrows have F = 0 unless charged, so most end without a burst sound).

**The 300 healing-area steps** (kind 6, hit type 2) exercise: no movement; the 500-ms boundary
and its repetition across sub-steps as the area's falling velocity grows (§1.10); every living
creature considered (no owner filter); friendly creatures healed with a negative number equal
to −max(0, damage), doubled on a critical; the number appended twice and applied once by
0x29dcf0 on a non-remote world; hostile creatures and races 7/8 skipped; the freeze timer
raised for every struck creature; the rand order mana gain → critical → proc; the "other
player's area" skip; and expiry at the lifetime without a combo reset. Areas born from a
burst (age 3000 or 0, damage × 0.05, radius 5 + 5·charge) can be told apart by their radius.

Any mismatch in the rand count usually points at §1.11's order or a missing repetition per
sub-step; a mismatch only in a status-area sound id points at the stale id of §1.7.5.

---

## Open questions

1. **Stale sound id for hit type 1 areas.** In §1.7.5 the sound id is not written for hit type
   1 (areas of ability 0x62). It is whatever the stack slot holds: the id of an earlier sound
   request built in the same call (0x30, 0x2d, 0x14, 0x2a or 0x2d of a burst), otherwise
   unknown. The recordings should show what the game emits; an implementation needs a rule
   (e.g. "the last id this call wrote to that slot, else 0").
2. **Other unwritten bytes.** The status buffs' bytes 1–3 and +0x0c..+0x0f, and a notice's
   source id when the area has no owner, come from the stack. Recordings will compare them
   exactly unless masked.
3. **Kind 9.** Its creator was not found (not in 0x2c5250 or the interaction handler); 0x35ea0
   is only summarised, and the meaning of the +150-block offset is unknown.
4. **+0x58, +0x60, +0x38..+0x43** are never read by the step. Who reads them (drawing,
   network) is not established.
5. **0x4fa70 (attack power)** has no complete written spec; `combat.md` summarises it. The
   damage of §3.1–§3.5 depends on it (and on 0x65c70).
6. **Hit type 5** (fire trail of 0x2bb8e0) and the creators of §3.8 are not specified; in the
   step, hit type 5 behaves like any non-zero hit type (§1.7).
7. **Block flags.** Byte 5 bit 0 ("destroyed by effects") and bit 6 ("blocks sight rays") are
   named from their use only.
8. **0x2e0710's entries** (0x40-byte static entries with flag bit 1 at +0x38) are not identified
   (probably loose objects or ground items); the routine is summarised.
9. **Remote asymmetry.** The contact hit skips other players' effects on every world, but the
   area pass only stops for them on a non-remote world; on a remote client another player's
   burst damages creatures locally. This follows the code; whether the recordings contain such
   a case is unknown.
10. **0x2c5250's beam branch** (§3.6) was read only far enough to place its healing areas; the
    beam's direct hits, block destruction and particles belong to a separate spec.
11. **Division by zero.** §3.3's M / 3 and §3.2's M / 4 trap when M < 3 or M < 4; the ability
    tables make that impossible for the abilities concerned, but an implementation should not
    crash if it happens.
