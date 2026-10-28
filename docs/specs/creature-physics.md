# Creature physics (0x2bb8e0)

Behavioural specification of Steam Cube World's per-creature movement step, written from the
decompiled executable, its assembly and the binary's constants. Routines are cited by RVA
(image base 0x140000000); constants by the RVA of the value. Offsets are byte offsets into the
record named. World positions are signed 64-bit integers in 1/65536 of a block ("dots"). Times
are milliseconds. Vocabulary shared with other specs: the sound request, particle burst,
damage number and buff-change notice (`creature-hit.md` §0.3–§0.4), give buff 0x65390 and the
hostility test 0x50550 (`skills-and-interactions.md` §0), the ability timing parts 0x4f220 /
0x4ead0 / 0x660d0 (`skills-and-interactions.md` §2.2), the attack-speed factor 0x66460 and
smoothing factor 0xa7a0 (`creature-upkeep.md` §4.3, §4.9), the block lookups 0xeab0 / 0xe9a0
(`creature-upkeep.md` §4.7, `effect-objects.md` §2.7), the skill values 0x663f0 / 0x65330 /
0x5f6e0 (`lore-and-artifacts.md` §5.3), the effect record (`effect-objects.md` §0.1), maximum
health 0x5fac0 and maximum combo 0x5f9e0 (`creature-hit.md` §5).

---

## 0. Conventions

* All arithmetic is IEEE single precision (SSE scalar, round to nearest, no fused
  multiply-add) unless a step says **double**. Every operation is rounded before the next and
  the order written here is the game's order. "a × b × c" means (a × b) × c.
* **trunc64(f)** converts a float to a signed 64-bit integer toward zero; **trunc32** the same
  to 32 bits. **dots(f)** means trunc64(f × 65536.0) (65536.0 at 0x42e5f8). **blocks(i)** means
  float(i) × 1/65536 (0x42e510), the float conversion of the 64-bit integer being exact-rounded.
* **floor-block(p)** is the block index of a dot coordinate rounded toward −∞ (0xfd80 for three
  axes); "the cell of" a position is floor-block per axis.
* A **squared length in blocks** of an integer difference (dx, dy, dz) is
  ((blocks(dx))² + (blocks(dy))²) + (blocks(dz))², each component converted first and squared
  after (not multiplied by 1/65536 twice).
* **s(dt, c)** is the smoothing factor 0xa7a0 with dt widened to double and c the float
  constant named (`creature-upkeep.md` §4.9). **ease(x → target, c)** is x = s(dt, c) × (target
  − x) + x (0x16ee0); for a 3-vector (0x16f20) the three differences are taken first, then the
  factor once, then each component = difference × factor + component.
* **turn(a → b)** is 0x17840 with factor 1.0 (§7.3): it re-expresses an angle b in degrees as
  the angle nearest a.
* Comparisons are as the game makes them; a NaN compares false unless stated.
* rand() is the CRT's (0..32767), import slot 0x42cd28, shared with the rest of the game.
  "rand × k / 32767 + c" means: float of the draw, × k, ÷ 32767.0 (0x42e5f0), + c.
* Library thunks: sqrtf 0x395462, sqrt (double) 0x39545c, sinf 0x395456, cosf 0x395450, asinf
  0x395468, asin (double) 0x3954da, acosf 0x3954a4, expf 0x3954aa.
* "**Local**" means the creature is the local player (world+0x448). "**Authoritative**" means
  world+0x358 (remote-client flag) is 0. "**Owned**" means local, or authoritative and the
  creature's hostility kind (+0x60) is not 0.
* "**t**" is the ability timer +0x6c; "**dur**" = float(0x4f220(C)) (total duration),
  "**wind**" = float(0x4ead0(C, −1)), "**mid**" the integer 0x660d0(C, −1). "t < dur" etc. are
  float comparisons of +0x6c against those floats.
* **dts** = dt × 0.001 (0x42e514), the step in seconds.

## 1. Where it runs and how time is split

The frame routine 0x1400f5520 takes the frame's elapsed whole milliseconds m, caps it at 1000,
and runs the world tick 0x2da6c0 several times: when m > 16 it runs k = ((m − 17) >> 4) + 1
ticks (unsigned shift) with dt = 16.0 each, then one more tick with dt = float(m − 16k); when
m ≤ 16 a single tick with dt = float(m). The world tick first clears bit 0x10 of +0x128 for
every creature in world+0x40, then walks that list in order and, for each creature whose health
+0x180 > 0, calls 0x2bb8e0(world, C, dt), then the ability update 0x2c5250 and the interaction
handler 0x2c3710; every creature (alive or not) then gets the upkeep 0x2de4c0.

Inside 0x2bb8e0 the move itself is split further into n equal sub-steps (§5.3).

## 2. Inputs and outputs

### 2.1 Creature record fields

| Offset | Type | Use |
|---|---|---|
| +0x08 | i64 | id |
| +0x10, +0x18, +0x20 | i64 ×3 | position (x, y, z); z is the body centre |
| +0x28 | f32 | pitch, degrees (written) |
| +0x2c | f32 | lean (bank), degrees (written) |
| +0x30 | f32 | yaw, degrees; forward is (−sin, cos, 0) (written) |
| +0x34, +0x38, +0x3c | f32 ×3 | velocity, blocks/s (written) |
| +0x40, +0x44, +0x48 | f32 ×3 | wanted acceleration (set by the AI / input); zeroed in one case |
| +0x4c, +0x50, +0x54 | f32 ×3 | knock-back velocity (written) |
| +0x58 | f32 | body tilt, degrees (written) |
| +0x5c | u32 | contact flags (§6.1), rewritten each call; read **before** that from the previous call |
| +0x60 | u8 | hostility kind (0 player) |
| +0x64 | i32 | race |
| +0x68 | u8 | ability (may be set to 0 and a few other values) |
| +0x6c | f32 | ability timer t (written in one case) |
| +0x70 | i32 | combo |
| +0x80 | u32 | appearance/physics flags: 0x1, 0x2 (slow fall / floating), 0x200, 0x10000 (no unstick, no buoyancy), 0x100 / 0x10100, 0x40000, 0x100000, 0x400000, 0x800000, 0x2000000 |
| +0x84, +0x88, +0x8c | f32 ×3 | body size x, y, z (blocks) |
| +0x128 | u16 | state flags: 0x2, 0x4 (faces its aim; written), 0x10 (no horizontal collision), 0x40 (sprint; cleared), 0x1000 (held: no collision, no gravity), 0x2000 |
| +0x12c | f32 | dodge timer |
| +0x130 | f32 | stun timer (written: raised by falls and glide crashes) |
| +0x134, +0x138, +0x13c | f32 | timers (read) |
| +0x14c, +0x14d | u8 | class, specialisation |
| +0x158 | f32 | charge (written) |
| +0x174, +0x178 | f32 | aim x, y |
| +0x180 | f32 | health (written by fall and lava damage) |
| +0x188 | f32 | charge limit (written) |
| +0x194 | f32 | **ground grip** (speed factor), written for owned creatures (§3.2) |
| +0x198 | f32 | **lamp radius**, written for owned creatures |
| +0x1a0 | i64 | owner id (pets) |
| +0x1a8 | i64 | id of the creature it rides / is ridden by |
| +0x1d8 | item | item in use (type byte +0, subtype i32 +4, rarity byte +0x14) |
| +0x1e0 | i32 | level (other specs give the level at +0x144; +0x1e0 is read only by 0x504e0 for boat drift — check) |
| +0x6d8, +0x6dc | u8, i32 | weapon item type, subtype |
| +0x990 | f32 | step-smoothing offset, blocks (written) |
| +0x998, +0x99c | f32 | stride strength, stride phase (written) |
| +0x9a0 | f32 | stamina (written by ability 0x93) |
| +0x9a4 | f32 | (on a pet's owner) increased while the pet is wet |
| +0x9ac, +0x9b0, +0x9b4 | f32 ×3 | wall normal (written) |
| +0x9d8 | i64 | mount id |
| +0xb4c, +0xb50, +0xb54 | f32 ×3 | aim direction used for facing |
| +0xb60, +0xb70, +0xb74 | i32 | skill counters (swimming, sailing, light) |
| +0xba8 | f32 | released charge (written) |
| +0xbb0 | f32 | height where the current fall began, blocks (written) |
| +0xbb4 | f32 | glide time, ms (written) |
| +0xbb8 | u8 | written in the charge end (§4.3) |
| +0xbbc | f32 | air time, ms (written) |

### 2.2 World fields

| Offset | Use |
|---|---|
| +0x40 | creature list (world order) |
| +0x50 | object list (records laid out like creatures: position +0x10, size +0x84, type +0x64, extra height +0x17c) |
| +0x60, +0xd0 | effect list and its outgoing copy list |
| +0x90 | damage numbers (append 0xa16f0) |
| +0xa0 | sound requests (append 0xa18b0 or inline) |
| +0xb0 | particle bursts (append 0x2d0450) |
| +0x150 | buff-change notices (append 0x7380 / 0xa1530) |
| +0x358 | remote-client flag |
| +0x440 | world clock, ms |
| +0x448 | local player |
| +0x450 | id → creature map (0xe720) |
| +0x460 | zone map (64 × 64-block zones) |

### 2.3 Blocks

A block is six bytes; byte 4 is its **type**, byte 5 its **flags**. Lookups use 0xe9a0 (block
coordinates) or 0xeab0 (dot position → floor-block → 0xe9a0). A missing zone gives the default
solid block. Types that matter here: 0 air, 2 water, 3 wet (shallow), 8/12, 10/11, 13, 16
(0x10), 17 (0x11, lava), 19 (0x13, swamp); every other non-zero type is plain solid.

* **Blocked** (the general rule): type AND 0xfd is not 0 (anything but air and water); or
  flag 0x40 is set and the passage rule applies and (the creature has no buff 0x13, or flag
  0x80 is clear). Where "the passage rule applies" is said to depend on +0x5c, it applies only
  when bit 0x40 of the previous +0x5c is clear; elsewhere it always applies.
* **Box of a creature**: half extents hx = dots(sx × 0.5), hy = dots(sy × 0.5), hz = dots(sz ×
  0.5); its cells run from floor-block(p − h) to floor-block(p + h) per axis, inclusive.
* **Shell scan**: of a box's cells only those with at least one coordinate equal to the box's
  minimum or maximum on that axis are looked at.

## 3. Speed set-up

Let **a** = (+0x40, +0x44, +0x48) × dts (per component) and **m** the top speed.

### 3.1 Ability stance (owned creatures only)

1. Clear bit 0x4 of +0x128.
2. If the ability A is not 0, and A is not one of 0x5f, 0x78, 0x8a, 0x8e, 0x98, 0x9f, 0x0c,
   0x50–0x52, 0x54–0x5a, 0x76, 0x79, 0x7c, 0x7d, 0x80, 0x81, 0x92, 0x9c, 0x9d, 0xa2, 0xa3, and
   not (A = 0xa1 with hostility 0), and (t < float(0x4f220 + 200), or A is 0x5f, 0x93 or one of
   0x09, 0x19, 0x1a, 0x1c, 0x1d, 0x25, 0x3c, 0x40, 0x41):
   1. if A is none of 0x23, 0x5f–0x62, 0x65, 0x75, 0x85, 0x87–0x89, 0x8c, 0x8f, 0x90, 0x95,
      0x9b and (+0x128 AND 0x2012) = 0, set bit 0x4 of +0x128;
   2. clear bit 0x40 of +0x128.
3. If A is 0x85 and |+0x40..+0x48|² > 0 (((x² + y²) + z²)), set A = 0.

### 3.2 Base speed, grip and lamp

1. If A is 0x79 and bit 0x1 of +0x5c is set: zero +0x40..+0x48.
2. a = (+0x40, +0x44, +0x48) × dts. If the stun timer +0x130 > 0: a = 0.
3. If +0x128 bit 0x1000 is clear, A is neither 0x78 nor 0x7a, +0x5c bit 0x10 is set and a.z > 0:
   a.z = 0.
4. m = 6.0 (0x4aca24). If A = 0x51 and stamina +0x9a0 > 0: 7.8 (0x4724b0). If A = 0x9f and t <
   dur: 9.0 (0x42e790). If +0x80 has bit 0x40000 or bit 0x200: 8.0 (0x4aca30). If A = 0x9a and
   t < dur: 0.1. (Each later rule overrides the earlier.)
5. **Grip** (owned creatures only). +0x194 = 1.0, then, using the previous +0x5c:
   * A = 0x79 and in water (bit 0x2): +0x194 = sailing value 0x65330;
   * otherwise, in water and +0x80 has neither 0x200 nor 0x40000: swimming value 0x663f0;
   * otherwise the first that applies, each only when +0x80 has neither 0x200 nor 0x40000
     (the last needs no such check): bit 0x80 → 0.35 (0x3eb33333); bit 0x200 → 0.25; bit
     0x400 → 0.98 (0x3f7ae148); bit 0x800 → 0.85 (0x3f59999a); else it stays 1.0.

   Then: if +0x990 > 0.1, +0x194 × 0.8; if +0x5c bit 0x2000, +0x194 × 1.1. Finally
   **+0x198** = light value 0x5f6e0.
6. m = m × +0x194 (for every creature; a non-owned creature keeps the stored value).
7. **Riding**: only for the local player with A = 0x76. If in water, A = 0. Otherwise look up
   the mount (+0x9d8, 0xe720); if found, its health > 0 and its race passes 0x5ef80 (§7.6), m ×
   1.5.
8. **Gliding pace** (A = 0x78): call 0x504e0(level) and ignore the result. If not on ground
   (+0x5c bit 0x1 clear):
   1. g = min(1.0, max(0, +0xbb4 × 0.001 − 0.5)); m = m × (((1 − g) + (1 − g)) + g × 3.0).
   2. Pitch: in double, L = sqrt((vx² + vy²) + vz²) of the velocity, q = 1.0 / L × vz; target
      = float(asin(q) × 57.29577951308232 (0x42e768)); +0x28 = 0x17840(+0x28, target, s(dt,
      0.1)) (§7.3 with that factor).
9. If A = 0x50 and in water: A = 0.
10. **Sprint**: if +0x128 bit 0x40 is set and A is not 0x50, not (0x7a with +0x5c AND 0x5 ≠ 0),
    not (0x78 off the ground): in double, m × 1.5 (0x441c78) if A = 0x53, else m + m; back to
    float.
11. Buff 0x0c (0x5e3a0): m + m.
12. **Fire trail**: if the creature has buff 0x10, or A = 0x92 with +0x188 > 0: m × 1.3
    (0x4328b8), then §4.1.

### 3.3 Charges, dashes and fixed speeds (in this order)

1. A = 0x32 → m = 40 (0x42e5ac). A = 0x37 with t < wind + mid (integer sum, then float) → 40.
   A = 0x34 with t < 500 → 40.
2. **Charge flag** K: A = 0x32, or (A in 0x02–0x08, 0x0a–0x15, 0x3b, 0x3f, 0x42–0x44 and t <
   200).
3. A = 0x52: if t ≥ 500, A = 0; else m = 40.
4. If (local or authoritative) and K: §4.2 (contact or forward push).
5. A = 0x55: if t ≥ float(0x108fa0(item +0x1d8)) (§7.5), A = 0; else m = 0.
6. **Dodge cancels**: if the dodge timer +0x12c ≠ 0 (NaN counts) and (A is not 0x34, 0x51, 0x6c
   or t > dur): if class 4 specialisation 1, A = 0 unless A is 0x06, 0x12 or 0x15; otherwise A
   = 0 and charge +0x158 = 0.
7. Let **no-turn** = false (it stops §7.4's turning).
8. A in 0x45, 0x46, 0x67: if t ≤ wind, m = 0.
9. A in 0x45, 0x46, 0x94, 0x67, with wind < t < wind + (3 × mid) / 4 (C integer division): build
   the yaw basis (§7.2); a.x = ((R.x × 0) + F.x) + U.x × 0, a.y likewise with the y components,
   each then × 20.0 (a.z unchanged); no-turn = true; m = 50.
10. A = 0x5f, or A = 0x9f with t < dur: no-turn = true.
11. A = 0x4e, wind < t < wind + mid: a = (((F × 0) − R × 1.0) + U × 0) × 20 per component (all
    three); no-turn; m = 60. A = 0x4f: a = ((F × 0 + R) + U × 0) × 20; A = 0x4d: a = ((R × 0 +
    F) + U × 0) × 20; both with no-turn and m = 60 (0x42e7b8).
12. A = 0x48 with t < dur: m = 0, no-turn.
13. A = 0x49 with t < dur: yaw +0x30 = dts × 720 (0x472594) + yaw; no-turn; m = 12.
14. A in 0x45, 0x46, 0x94, 0x67 with t > wind: no-turn = no-turn or (t < dur).
15. Hostility 5: m = 30.
16. A = 0x3a or 0x4b, with t < dur and t ≥ wind: no-turn; m = 0.1.
17. A = 0x25: +0x158 = 0x66460(C) × (dt × 0.0005) + +0x158.
18. **Charging**: A in 0x09, 0x19, 0x1a, 0x1c, 0x3c, 0x40, 0x41, 0x93 with t ≥ wind:
    1. c = 0x66460(C) × (dt × 0.00075 (0x472180)).
    2. Class 2 specialisation 0: r = float(combo +0x70) / float(0x5f9e0(C)); c = c × (min(r,
       4.0) + 1.0).
    3. Buff 0x02: c × 10. Buff 0x0a: c = +0x188.
    4. A = 0x93: take = min(stamina, c); stamina = stamina − take; +0x158 = take + +0x158; if
       > 1.0 → 1.0; if +0x158 > +0x188, +0x188 = +0x158. Other A: +0x158 = c + +0x158.
19. If +0x158 > +0x188: +0x158 = +0x188 (always).
20. Multipliers: +0x138 ≠ 0 (NaN counts) → m × 0.5; buff 0x1d → × 0.5; buff 0x0e → × 0.1;
    +0x13c ≠ 0 → × 1.5.
21. **Back-pedal**: if +0x128 bit 0x4, dodge +0x12c = 0 and (A ≠ 0x34 or t > 1000): with v =
    (+0x34, +0x38) and the aim w = (+0x174, +0x178), if w.x × v.x + w.y × v.y < 0: w = w × (1 /
    sqrt(w.x² + w.y²)), v = v × (1 / sqrt(v.x² + v.y²)); m = m × ((v.x × w.x + v.y × w.y) × 0.5
    + 1.0).
22. +0x128 bit 0x1000: m = 10.
23. A = 0x7a (climbing): m × 0.5 and knock-back +0x4c..+0x54 = 0.
24. Glide time: A = 0x78 off the ground → +0xbb4 = dt + +0xbb4; otherwise +0xbb4 = 0.

## 4. Side effects of the speed set-up

### 4.1 Fire trail effect (§3.2 step 12)

Walk the effect list world+0x60. If any effect with hit type (+0x6c) 5 and owner = this id lies
within 3 blocks of the feet — the difference effect position − creature position, with dots(sz
× 0.5) added to z, has squared length in blocks < 9.0 — do nothing. Otherwise build an effect
record: owner = id; position = creature position with z + dots(0.01 − sz × 0.5) (x and y +
trunc64(0.0)); target 0; +0x38, +0x40 0; velocity 0; damage = 0x50f20(C, A) × 0x4fa70(C, 0);
radius +0x54 = 5.0; +0x58 = 1.0; +0x5c = 0; +0x60 = 0; flag F 0; kind 6; hit type 5; age 0;
lifetime 5000; freeze 0. Append it to world+0x60 and a copy to world+0xd0.

### 4.2 Contact or forward push (§3.3 step 4)

1. For each other creature O in world+0x40 with health > 0 and 0x50550(C, O) true: r = (sx +
   1.0) + O.sx × 0.7 (0x4328ac). With 64-bit wrapping products, d = ((dx × dx) ÷ 65536) + ((dy ×
   dy) ÷ 65536) (each a signed division rounding toward zero; dx, dy = C − O). If d <
   trunc64(r × r × 65536): the game then compares a vertical gap that it computes from a zero
   (|blocks(0)| < O.sz × 0.5 + sz + 1.0), which holds for any positive sizes; so **contact** is
   set. Every O is visited.
2. If contact, or t > 500:
   1. If A = 0x32: +0xba8 = +0x188; +0x188 = 0; A = 0; t = 0; +0xbb8 = 0. If the weapon type is
      3: A = 6 for subtype 5, 0x12 for subtype 3, else 0x15; t = float(0x4ead0(C, −1) + 1); if
      the creature has buff 0x0b: give it (refresh flag 0) the buff {type 0x0b, strength 0,
      duration 0, source 0} (bytes 1–3 and +0xc not written), append a notice {source id, target
      id, that buff, flag 0} to world+0x150, and set +0xbb8 = 1.
   2. If A = 0x31: +0xba8 = 0, A = 0x37 (unreachable: 0x31 never sets K).
3. If there was no contact: with the yaw basis (§7.2) and k = m + m: a.x = ((a.x + F.x × k) +
   R.x × 0) + U.x × 0, the same for y and z with their components.

## 5. Velocity, gravity and the move

### 5.1 Integrate and cap

1. v = v + a per component (written to +0x34..+0x3c).
2. If A = 0x7a and the previous +0x5c has bit 0x4 (climbing on a wall): if ((vx² + vy²) + vz²)
   > m × m: q = 1 / sqrtf(that), v = (v × q) × m per component. Otherwise: if vx² + vy² > m × m:
   q = 1 / sqrtf(vx² + vy²), vx = vx × q × m, vy = vy × q × m.

### 5.2 Gravity

* **Climbing** (A = 0x7a with previous bit 0x4): f = s(dt, 0.0025 (0x42e6d8)); v = v + f × (0 −
  v) per component (x: vx + f × (0 − vx); y and z: f × (0 − v) + v). If vz > −10: +0xbb0 =
  blocks(z).
* Otherwise, only when (+0x128 AND 0x2012) = 0, A ≠ 0x98, +0x128 bit 0x1000 clear and A is
  neither 0x9c nor 0x9d. Let M be the creature +0x1a8 names (0xe720), and k the fall factor:
  1. A = 0x80, M exists with +0x80 bit 0x2, and not stunned (+0x130 ≤ 0): k = dts × 0.5;
  2. else if (+0x80 bit 0x2 and +0x130 ≤ 0), or A = 0x53, or (A = 0x86 or 0x87 with t < dur):
     k = dts × 0.1;
  3. else if A = 0x9f with vz < 0 and t < dur: k = dts × 0.5;
  4. else if A = 0x8a and +0x130 ≤ 0: k = dts × 0.035 (0x43ef3c);
  5. else if A = 0x78: k = (1.0 − min(1.0, max(0, +0xbb4 × 0.003 − 0.5)) × 0.75) × dts;
  6. else (**normal gravity**): if vz ≥ 0, +0xbb0 = blocks(z); vz = vz − dts × 30.0; done.

  In cases 1–5: vz = vz − k × 30.0, then +0xbb0 = blocks(z).

### 5.3 Glider checks (local player, A = 0x78)

* In water: A = 0.
* Not on ground: if +0xbb4 > 1000 and previous bit 0x4 and |v|² > 64 and (n.y × vy + n.x ×
  vx) + n.z × vz < −4 (n = wall normal +0x9ac): **crash** (below). Then, crash or not, add the
  wind: w = 0x2e65e0(world, position) (§7.7); v = w × dts + v per component.
* On ground: if |+0x2c| > 40 (absolute value taken in double): **crash**.
* **Crash**: stun +0x130 = float(0x5fc20(C)) (1500 for the player, 3000 otherwise); A = 0;
  sound 0x17 at the position, pitch 1.0, volume 1.0, +0x28 0.

Then: if the stun timer > 0, charge +0x158 = 0.

### 5.4 Lean (+0x2c)

1. target = 0. If (+0x40² + +0x44²) as double > 0.01 and vx² + vy² > 0.01: d = (+0x40, +0x44)
   × (1 / sqrtf of its square), u = (vx, vy) × (1 / sqrtf); s = u.y × d.x − u.x × d.y, clamped
   (max with −1 first, then min with 1); target = float((double(asinf(s)) / π) × 180 × 0.5).
2. If sx > 2.0: target × (2.0 / sx). If +0x128 bit 0x4: target = 0.
3. If gliding (0x5ee70): target = max(−90, min(90, target × 40)) and c = 0.002; else c = 0.005.
4. +0x2c = s(dt, c) × (target − +0x2c) + +0x2c.

### 5.5 Displacement and pushing apart

1. D = (dts × (vx + kx), dts × (vy + ky), dts × (vz + kz)) with k the knock-back.
2. Climbing on a wall: D = D − n × (dts × 0.01) per component.
3. **Push apart**, unless +0x80 has 0x2000000, A is 0x57–0x59, 0x77, 0x82 or 0x91, +0x128 bit
   0x1000, or (A is 0x8c or 0x94 with t < dur). For each O in world+0x40, skipping when:
   A is 0x80/0x81 and +0x1a8 = O's id; O's ability is 0x83 and O+0x1a8 = this id; O is C; O+0x80
   bit 0x800000; O's health ≤ 0; C is hostility 8, or 7 with race ≠ 0x11d, and O's hostility is
   not 7 or 8; C is hostility 0 and O's is 0, 3 or 6; O's ability is 0x77 or 0x8c; O+0x128
   bit 0x1000; O+0x80 bit 0x2000000; O's race is 0x11d and C's is not.
   1. O's sizes (sx', sy', sz'); if O's hostility is not 7 or 8: sx' + 1.0, sy' + 1.0, sz' + 0.
   2. Overlap on each axis in turn (x, y, z): O.p − dots(s' × 0.5) ≤ dots(s × 0.5) + C.p and C.p
      − dots(s × 0.5) < dots(s' × 0.5) + O.p. Skip O unless all hold.
   3. e = (blocks(C.x − O.x), blocks(C.y − O.y)). If its squared length (with dz) < 0.0001:
      one rand: α = float(double(r) × 2π / 32767.0) (doubles 0x43ef98, 0x446198); e = (cosf α,
      sinf α).
   4. If e.x² + e.y² + 0 > 0: normalise (0x127f0, z = 0); if stunned, e × 0.1. f = O.sx / sx;
      target = D + f × e; D = ease toward target with c = 0.001 (0x16f20).
4. **Unstick** 0x2da1e0 (§7.1), when (authoritative or local), +0x80 bit 0x10000 clear, +0x128
   bits 0x10 and 0x1000 clear, and A ≠ 0x98.

### 5.6 Sub-steps

1. n = trunc32(sqrtf((Dx² + Dy²) + Dz²) × 1.5 + 1.0); d = D × (1.0 / float(n)).
2. **Step-up allowed** unless A is 0x57, 0x58 or 0x7a or (+0x128 AND 0x2002) ≠ 0.
3. **Ignore set**: every object of world+0x50 whose box (half extents dots(size × 0.5)) overlaps
   the creature's box at the starting position (same test as §5.5 3.2) is remembered; those
   objects never block this call.
4. Remember the start position P0. Let **pass** = creature has buff 0x13.
5. For each of the n sub-steps, for axis x, then y, then z:
   1. Remember the pre-move position; position[axis] += dots(d[axis]).
   2. If +0x128 bit 0x1000: no collision on this axis.
   3. **Blocks**: footprint sizes sx, sy (both 0.8 when A = 0x57 and +0x128 bit 0x2000), sz;
      shell-scan the box at the new position, passage rule depending on +0x5c. The scan goes x,
      y, z nested and stops at the end of the z column where the first blocked cell was found.
      For x and y with a hit, go straight to step 5.
   4. **Objects** (also for z with or without a block hit): for each object O of world+0x50: if
      O's type is 0xfb and (pass or +0x80 bit 0x100000): skip. Sizes (O.sx, O.sy, O.sz) and
      O.z; for type 0x12e, O.sz + O+0x17c and O.z + dots(O+0x17c × 0.5). Only when
      trunc64(float(pre[axis] − O.p[axis]) × d[axis]) < 0 (moving toward it): with lo/hi the
      per-axis minimum/maximum of pre-move and new position, the overlap is O.p − dots(o × 0.5)
      ≤ dots(s × 0.5) + hi and lo − dots(s × 0.5) < dots(o × 0.5) + O.p on all three axes. An
      overlapping object not in the ignore set is a hit; mark **on object**, clear step-up
      allowed, stop the object scan.
   5. No hit: next axis. On a hit:
      * **z**: if (authoritative or local) and vz < −2.0: sound 0x23, pitch rand × 0.2 / 32767
        + 1.0, volume 0.5, +0x28 = id when hostility 0 else 0. If the previous +0x5c has no
        0x2 and vz < −5 and the local player and dots(+0xbb0) − z > 1310720 (20 blocks):
        **fall damage** (below). Then z −= dots(d.z); vz = 0; knock-back z = 0; +0xbb0 =
        blocks(z).
      * **x or y**: if +0x128 bit 0x10: keep the move. Else if step-up is not allowed: undo
        (position[axis] −= dots(d[axis])). Else test a raised box: all cells (not a shell) of
        the box centred at (x, y, z + 66191) (trunc64 of 66191.36 at 0x4725b0), blocked with
        the passage rule always applying. Any blocked → undo. Otherwise **step up**: z_old =
        z; f = floor-block(z − hz); z = hz + f × 65536 + 66191; unless (A = 0x94 and t ≤ dur),
        +0x990 = blocks(z − (z_old − dots(+0x990))); then if +0x990 > 1.0 it is 1.0.
   6. **Fall damage**: F = dots(+0xbb0) − z; q = trunc64(float(F) / 20.0) − 65536; H =
      0x5fac0(C); dmg = blocks(trunc64(float(trunc64(float(q) × H)) × 0.5)); health −= dmg;
      stun = max(stun, float(0x5fc20(C))); sound 0x17 pitch 1.0 volume 1.0; if health < 0, 0;
      damage number {attacker 0, target id, dmg, not critical, stun 0, second timer 0,
      position, push 0, flags 0} to world+0x90.

## 6. After the move

### 6.1 Contact flags (+0x5c)

Save the previous bits 0x1, 0x2, 0x4, 0x80, 0x200 and the wall normal, then +0x5c = 0.

1. **Stuck (0x20)**: if stun ≤ 0, vx² + vy² > 16 and |(+0x40, +0x44, +0x48)|² > 16, and the
   squared length in blocks of P0 − position < ((d.x² + d.y²) + d.z²) × 0.01, set 0x20.
2. Box B at the new position (half extents of the true sizes).
3. **Water**: for every cell of B: type 2 → set 0x2, and the water top W = max(W, cell z + 1)
   (W starts at B's minimum z); flag 0x40 → set 0x40.
4. **In water**:
   1. Buoyancy, when +0x80 bit 0x10000 is clear and race is not 0x83–0x8b (0x53e70): f =
      blocks(W × 65536 − z) / (sz × 0.5). g = 0 if f ≤ 0; else g = f + 1.0 when f < 1.0, else
      1.0; but if A = 0x79, or hostility 7/8 with +0x80 AND 0x10100 = 0, g = f × 1.5 + 1.0. vz =
      (dts × 30) × g + vz.
   2. Still in water: scan B (x, y, z); the first cell not of type 2 sets 0x10 and ends it.
5. **Not in water**:
   1. Ground: G = floor-block(z − hz − 13107) (0.2 below the feet). For x, then y over B: the
      block (x, y, G), blocked (passage rule always applying) → set 0x1; type 3 → **wet**. The
      x loop stops after the row in which 0x1 was set.
6. **Surface under feet**: S = floor-block(z − hz − 65536). For x, then y over B, block (x, y,
   S): type 0x11 → 0x80; 0x13 → 0x200; 10 or 11 → 0x400; 8 or 12 → 0x800; 13 → 0x2000; type 0
   → nothing; any other → clear 0x2e80 and stop scanning.
7. A = 0x53: clear 0x80 and 0x200.
8. If **wet** (or in water) and hostility 6: the owner (+0x1a0) gets +0x9a4 = dts × 0.05 +
   +0x9a4, capped at 1.0, only while its value was < 1.0.
9. **Step down**: when not on object, the previous bit 0x1 was set, not (A = 0x94 with t ≤
   dur), step-up allowed, d.x² + d.y² > 0, +0x5c AND 0x3 = 0 and not d.z > 0: level L =
   floor-block(z − hz − 78643) (1.2 below the feet); for x then y over B, the first block
   (x, y, L) whose type AND 0xfd ≠ 0 sets 0x1, z_old = z, z = hz + 66191 + L × 65536, +0x990 =
   blocks(z − (z_old − dots(+0x990))) (no cap), and ends the scan.
10. **Boat drift** (A = 0x79, in water, item +0x1d8 type 0x18 subtype 2): c = 0.0025 if
    |+0x40..|² > 0 else 0.005 (this choice also serves §6.2); vx, vy eased to 0 with factor
    s(dt, c × 0.1), vz with s(dt, c); w = +0x40..; if |w|² > 144, w = w × (1 / sqrtf) × 12; w =
    w × 0x504e0(level) (§7.6); knock-back eased toward w with c = 0.005.
11. **Friction** (c as in step 10): if +0x134 = 0 (NaN fails):
    * on ground: unless A is 0x32 or 0x52, or A = 0x37 with t ≤ wind + mid, or A in 0x07, 0x08,
      0x0b, 0x12–0x15 with t < wind: v eased to 0 with c (3D). Then knock-back eased to 0 with
      0.005.
    * off ground, A = 0x78 or 0x80: knock-back eased to 0 with 0.005.
    Then (regardless of +0x134): in water and A ≠ 0x79 → knock-back eased to 0 with 0.005.
    Always: knock-back eased to 0 with 0.00001 (0x43ef28).
12. **Wall probes**: h = (+0x40, +0x44) normalised (q = 1 / sqrtf; z part q × 0) or 0; normal =
    0; best = −2. For each side in order −y, +y, −x, +x the probe line is: player: p ± dots(s ×
    0.5) ± 16384; others: p ± trunc32(s × 0.5 + 0.5) × 65536 ± 32768 (floor-block of that).
    Over the other horizontal axis and z of B, a blocked cell (passage rule always applying)
    sets 0x4, 0x1000 if its z > B's minimum z, 0x100 if type 0x10, 0x4000 if flag 0x04, and
    with score −y: (h.x × −0 − h.y) + h.z × −0, +y: (h.x × −0 + h.y) + h.z × −0, −x: (h.y × −0 −
    h.x) + h.z × −0, +x: (h.y × −0 + h.x) + h.z × −0: if score > best, best = score and the
    normal becomes (0, 1, 0), (0, −1, 0), (1, 0, 0) or (−1, 0, 0) respectively.
13. **Ladders**: for each object of type 0x12e with +0x17c > 1.0: r = (O.sx × 0.5 + sx × 0.5)
    + 0.1; e = (blocks(C.x − O.x), blocks(C.y − O.y)); if max(|e.y|, |e.x|) < r and O.z ≤ z <
    O.z + dots(O+0x17c): set 0x1104 and normal (x, y) = e × (1 / sqrtf(e.y² + e.x²)).

### 6.2 Surfaces and pitch

* **In water**:
  * A = 0x79: score = [water 3 ahead] − [air 3 ahead, 1 below] − [water 3 behind] + [air 3
    behind, 1 below], with ahead = position + (dots(cos β × 3), dots(sin β × 3)), β = float(yaw
    × π / 180 + π/2) in double, blocks by 0xeab0 at z and z − 65536. +0x28 eased with s(dt,
    0.003): toward −20 if score < 0 (as +0x28 − f × (+0x28 + 20)), 0 if 0, 20 if > 0.
  * otherwise: v eased to 0 with 0.005; unless hostility 7/8 or +0x80 bit 0x1: +0x28 = +0x28 −
    s(dt, 0.01) × (+0x28 + 60).
  * If the previous flags lacked 0x2 and vz < −3: particles (pos, velocity (0, 0, 10), colour
    (0.2, 0.7, 1.0, 1.0), size 0.4, count 15, style 6, spread 3.0); and if owned, sound 0x22,
    pitch rand / 32767 + 0.9, volume 1.0, +0x28 = id.
  * Local player without buff 0x1b, and not (buff 0x1d with strength > 9800): with column
    coordinates x / 65536 and y / 65536 rounded toward zero, the warmth (0x35ce0, 0x358e0,
    0x2d9b60) < 0.2 → give buff (refresh 1) {0x1d, strength 0.75, duration 10000, source 0}
    and append a notice {id, id, buff, flag 0} via 0xa1530.
* **Lava** (0x80): unless hostility 7/8 or +0x80 bit 0x1, +0x28 = +0x28 − s(dt, 0.01) × (+0x28
  + 20). If the previous lacked 0x80 and vz < −3: particles (velocity (0, 0, 10), colour (1.0,
  0.3, 0, 1.0), size 0.4, count 15, style 1, spread 3.0) and sound 0x2a, pitch rand / 32767 +
  0.9. Local, alive, without buff 0x1a, and trunc32(clock − dt) / 200 ≠ trunc32(clock) / 200:
  dmg = float(double(0x5fac0(C)) × 0.1); health −= dmg; sound 0x2a pitch 1.0; health < 0 → 0;
  damage number as for falls.
* **Swamp** (0x200, without 0x80): pitch as lava with c = 0.01; if newly entered and vz < −3:
  particles (colour (0.5, 0.4, 0.3, 1.0), size 0.4, count 15, style 0) and sound 0x22, pitch
  rand / 32767 + 0.6. Local, no buff 0x1c, no buff 0x04: give (0x29d2f0, refresh 1) {0x04,
  strength 0x5fac0(C) × 0.01, duration 3000, source 0} with its notice.
* **Otherwise**: if not gliding and the dodge timer is 0, +0x28 eased to 0 with 0.01.

### 6.3 Climbing over, dashes' dust

* Local, A = 0x7a, previous 0x4 set, now clear, +0x40² + +0x44² > 0 and (old n.y × h.y + old
  n.x × h.x) + old n.z × h.z ≤ 0: v = old normal × −2.0.
* A = 0x94 or 0x46 with wind ≤ t < wind + mid: for 0x94 +0x998 = +0x99c = 0. If the block at
  (x, y, z − hz − 32768) is not air and trunc32(t) / 50 ≠ trunc32(dt + t) / 50: dust — two
  rands r1, r2; direction u = normalised velocity (0x179b0), k = sz × 0.5 + 2.0, position =
  creature + (dots(u × k), with z + dots(0.5 − sz × 0.5)); velocity ((r2 × 4 / 32767 − 2) + vx ×
  0.5, (r1 × 4 / 32767 − 2) + vy × 0.5, vz × 0.5 + 2.0); colour water (0.2, 0.7, 1.0) style 6 or
  the block's bytes 0–2 × 1/128 style 5, alpha 1.0, size 0.3, count 8, spread 3.0; then if
  owned, sound 1 with pitch rand × 0.2 / 32767 + 0.8, volume 1.0, +0x28 = id. **The call then
  ends** (no stride, air time, offset or turning this step).

### 6.4 Stride (+0x998 strength, +0x99c phase) and footsteps

Let k0 = sqrtf(0.8 / sx).

* A = 0x79, flag 0x10 and |v|² > 16: when trunc32(t − dt) / 100 ≠ trunc32(t) / 100: **wake**
  (particles at position − (dots(vx × 0.1), dots(vy × 0.1), 0), velocity (vx × −0.25, vy ×
  −0.25, rand × 3 / 32767 + 2), colour (0.2, 0.7, 1.0, 1.0), size 0.25, count rand AND 3 + 5,
  style 6, spread 3; sound 0x24 + rand % 3, pitch rand × 1.5 / 32767 + 0.6, volume 0.75).
* A is 0x53 or 0x52, 0x7d with t < 5000, or 0x61/0x86/0x87/0x90 with t < dur: nothing.
* A = 0x98 with t > 2000: +0x99c = dts × 10 + +0x99c; +0x998 = dts + +0x998.
* On ground, +0x80 bit 0x2, climbing on a wall, or in water:
  * |v|² ≤ 0.5, or A is 0x79, 0x9c or 0x9d → idle (below).
  * Climbing: +0x99c = (|v| × dts × 6.0) × k0 + +0x99c; +0x998 = dts × 0.01 + +0x998.
  * Not on ground: +0x99c = k0 × dt × 0.002 × |v_xy| × 1.5 + old; +0x998 += dts. If
    authoritative, flag 0x10 and trunc32(double(+0x99c) / π) > trunc32(double(old) / π): a rand
    AND 3; if not 0, a **swim splash** (as the wake, at the same offset, colour (0.2, 0.7, 1.0,
    1.0), size 0.25; rands for velocity z, count, sound id, pitch in that order).
  * On ground: inc = k0 × dt × 0.0001 × |v_xy| × 1.5 when A = 0x32, or 0x37 with t < dur, or
    one of 0x07, 0x08, 0x0b, 0x12–0x15 with t < wind; else k0 × dts × 1.5 × |v_xy| (in that order: the recordings fix it to the ulp), doubled for
    A = 0x50. +0x99c += inc; +0x998 += dts. If owned, not dodging, out of water, |v_xy| > 5 and
    the phase crossed a multiple of π: footstep sound at the position; the block at (x, y, z −
    hz − 32768) of type 3 gives id 0x24 + rand % 3 with pitch 1.0, else id 0x23 with pitch rand
    × 0.2 / 32767 + 1.0; volume 1.0, +0x28 = id for the player.
* **Idle / airborne**: if not on ground, not in water and not climbing on a wall: +0x99c = 0;
  +0x998 = k0 × dts + +0x998.

Then: +0xbbc = dt + +0xbbc when off ground, out of water and not climbing on a wall, else 0. If
+0x998 > 1, 1. +0x998 eased to 0 with 0.005. If +0x80 bit 0x400000: +0x99c = float(x % 10000)
(unsigned) with x = 0x177e0(low 32 bits of id), +0x998 = 0.1.

**Step offset**: target T = 0.75 when +0x5c has 0x80 or 0x200, else 0. A = 0x94 with t < dur:
+0x990 = T. Otherwise ease toward T with c = 0.01 when +0x990 < 0, else 0.02.

### 6.5 Turning

* **Climbing on a wall**: +0x58 eased toward 45 with 0.005. w = −normal, normalised (3D); c =
  w.y clamped to [−1, 1]; target = −acosf(c) if w.x > 0, else acosf(c), in degrees
  (float(double(x) / π × 180)); +0x30 eased toward turn(+0x30 → target) with 0.005.
* Otherwise +0x58 = expf(dt × −0.01) × +0x58, then:
  * If +0x128 bit 0x4 is set and not stunned: with w = (+0xb4c, +0xb50), if |w|² > 0 and not
    no-turn: normalise (2D), target as above, +0x30 eased toward turn(+0x30 → target) with 0.1.
    Then w3 = (+0xb4c, +0xb50, +0xb54); if |w3|² > 0: normalise; +0x58 eased toward
    turn(+0x58 → asinf(w3.z) in degrees) with 0.005.
  * Otherwise, if the dodge timer is 0 (NaN fails), vx² + vy² > 0.02, (a.y·v.y + a.x·v.x) +
    a.z·v.z > 0 (a = +0x40..) and not no-turn: w = (vx, vy) normalised, target as above,
    +0x30 eased toward turn(+0x30 → target) with 0.1.

## 7. Callees

### 7.1 Unstick 0x2da1e0 and overlap 0x2b77c0
If health ≥ 0 and (+0x128 AND 0x2002) = 0: sizes (0.8, 0.8, sz) for A 0x57/0x58, else the
real ones. If any column of the box has no loaded block column (0x935e0 null), stop. pass =
buff 0x13; honour = +0x5c bit 0x40 clear and (+0x128 AND 0x2002) = 0. If 0x2b77c0 says free,
stop. Otherwise try x + 1, x − 1, y + 1, y − 1, x + 2, x − 2, y + 2, y − 2 blocks: the first
free one is taken. If none: z + 1 block and +0x990 = 0.
0x2b77c0 (world, position, sizes, pass, honour): shell scan of the box; true at the first
blocked cell (passage rule applying when honour).

### 7.2 Yaw basis 0x13050
The 4 × 4 identity (0x42e630) rotated by yaw × 0.0174532924 (cosf then sinf): R = (cos, sin,
0), F = (−sin, cos, 0), U = (0, 0, 1).

### 7.3 Turn 0x17840 (a, b, f)
α = float(double(a / 180) × π), β likewise; cosα, sinα, cosβ, sinβ as floats widened to double.
If cosβ cosα + sinβ sinα < −0.9 (double 0x42e820), return b. Otherwise d = clamp(float(sinβ
cosα − cosβ sinα), −1, 1) (min then max); return float(double(asinf(d)) × (f × 180 / π)) + a.

### 7.4 Small helpers
0x5f0d0 = +0x128 bit 0x1000. 0x5eee0 = +0x80 bit 0x40000. 0x5eda0 = +0x80 bit 0x200. 0x5ee70
(gliding) = A = 0x78, ground clear, stun ≤ 0, dodge ≤ 0. 0x53e70 = race − 0x83 > 8 unsigned.
0x5fc20 = 1500 for hostility 0, else 3000. 0x127f0, 0x179b0, 0x2c2fd0 normalise (one reciprocal,
then multiply). 0x2b630 = x² + y²; 0x2b650 = (x² + y²) + z²; 0x9bad0 / 0x9baf0 their sqrtf.
0x17320 (p, q) = (q.y p.y + q.x p.x) + q.z p.z. 0x177e0(x) = x ^= x << 13; x ^= x >> 17; x ^= x
<< 5 (32-bit unsigned).

### 7.5 Item use time 0x108fa0
Type 0x1c → 500. Type 1: g = 1.5 − float(rarity) × 0.5 × 0.25; subtype 1 or 0xc → trunc32(g ×
5000), 2 → trunc32(g × 6000), 3 → 500, other → trunc32(g × 10000). Other types → 0.

### 7.6 0x504e0 and 0x5ef80
0x504e0(L) = (1 − 1 / ((float(L) − 1) / 50 + 1)) × 1.8 + 1.2. 0x5ef80(race) is true for races
0x13, 0x14, 0x16, 0x17, 0x19–0x28, 0x3f–0x43, 0x4a, 0x4b, 0x62–0x64, 0x66–0x69, 0x97, 0x9e,
0xa3, 0xa4, 0xa6, 0xad, 0xc8, 0xc9, 0xd1–0xd3, 0xd5, 0xd7, 0x106, 0x107, 0x10e, 0x125–0x127.

### 7.7 Wind 0x2e65e0 and noise 0x2e0c0
X = trunc64(double(x) × −0.02), Y the same for y. N(u, v) is 0x2e0c0 on doubles. wind z =
2 × (N((0x108a0000 − X) × 2⁻¹⁶, (0x14e10000 − Y) × 2⁻¹⁶) + 0.5); wind y = N((0xd7f0000 − X) ×
2⁻¹⁶, (0x20f60000 − Y) × 2⁻¹⁶) × 8; wind x = N(trunc64(double(x) × 0.02) × 2⁻¹⁶, trunc64(double(y)
× 0.02) × 2⁻¹⁶) × 8. 0x2e0c0 is two-dimensional simplex noise on doubles: inputs × 0.32; skew
F = (√3 − 1) × 0.5, unskew G = (3 − √3) / 6; floors by truncation with a −1 correction; the
permutation is Perlin's standard 256-entry table (0x470ed0, doubled to 512 by 0x2b7690) and a
copy modulo 12 selects the 12 edge gradients (0x530a40, three doubles each); each corner gives
(y·g.y + x·g.x) × t × t × t × t with t = (0.5 − x²) − y² when t ≥ 0; result float((n1 + n0) + n2)
× 70.

### 7.8 Producers
0x2d0450 particle burst, 0xa18b0 / 0x370f0 sound request, 0xa16f0 damage number, 0x7380 /
0xa1530 notices, 0x36ce0 effect node, 0x29d2f0 give buff with notice, 0x80270 sound defaults
(pitch 1, volume 1, +0x28 0), 0x86de0 / 0x2981a0 / 0x814e0 the ignore set.

## 8. rand() order

In call order: one per pushed-apart creature exactly on top (§5.5); landing sound (§5.6); splash,
lava or swamp particles' sound (§6.2); dust: two then one for the sound (§6.3); wake 4, swim
splash 1 + 4, footstep 1 (§6.4).

## 9. Verification

ZoneExtractor's `update.request N` writes CWU2 records (`EngineTests updateoracle`): the
creature before and after, dt, the world clock, its buffs, creatures within 40 blocks and
nearby objects, the creatures its record names (+0x1a0, +0x1a8, +0x9d8), the blocks within 5 of
its box with zone-loaded bits, and every rand() drawn. Replay each record through the
implementation and compare every 4-byte word of the creature record and the number of draws.
The 8822 recorded updates (2935 idle, 5887 in a fight) exercise §3, §5.1–§5.2 normal gravity,
§5.4–§5.6, §6.1, friction, stride and turning; they do not reach water, lava, swamp, climbing,
gliding, falls, sailing, most dashes, or the local player's paths (skipped by the test). A
world-record check (sounds, particles, effects, notices) needs the recording to be extended.

## 10. Open questions

1. Meanings of +0x80 bits, +0x128 bits 0x2 / 0x2000, block flags 0x40 / 0x80 / 0x04, types
   8/10/11/12/13/16, objects 0xfb and 0x12e, and +0x9a4 are inferred from use only.
2. The game's vertical test in §4.2 uses a zero; whether any caller relies on it is unknown.
3. The yaw ease factor 0.1 was read from register loads on the paths seen; a path that reaches
   §6.5 without them was not found but not ruled out.
4. Warmth 0x35ce0 / 0x358e0 / 0x2d9b60 is not specified here.
5. sinf, cosf, asinf, acosf and expf results must match the UCRT bit for bit.
