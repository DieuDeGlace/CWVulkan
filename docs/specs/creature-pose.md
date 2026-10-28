# Creature pose (0x17ab0)

Behavioural specification of Steam Cube World's creature pose routine, written from the
decompiled executable (the longer-budget export of 0x17ab0), its assembly and the binary's
constants. Routines are cited by RVA (image base 0x140000000). Offsets are byte offsets
into the record named: **C** is the creature record (0x15f8 bytes), **A** is its animation
state, which lives inside the creature at C+0xc38. Angles are degrees unless stated, times
are milliseconds, lengths are model units (one unit is one voxel of a body-part model) and
world positions are 64-bit integers in 1/65536 of a block ("dots").

The routine is called once per drawn creature per frame (callers 0xd3170, 0xdb630,
0xe2e00). In one call it:

1. draws race 0x12e's stack of rings;
2. picks the model ("sprite") of every body part and held weapon;
3. works out the walk swing and the rest places of hands, arm parts and feet;
4. sets **targets** for the pose fields of A from what the creature is doing (the pose
   state A+0x0c, its clock A+0x08, the current ability and its timing, movement, race,
   class and so on);
5. eases the pose fields toward the targets at a rate that depends on the frame time;
6. builds the creature's base matrix and the 18 part matrices kept in A, draws every part,
   places and draws what the creature holds, and updates the weapon trails.

This spec is split over three files:

* **creature-pose.md** (this file): conventions, inputs, the animation state record, the
  shared maths, sprites, the walk swing and rest places, how the pose state is used, what
  happens before and after the per-state targets, the easing, the base, body, head and
  hand matrices, the body parts, and the recorded-pose oracle.
* **creature-pose-tables.md**: the target values of every pose state (the 0x66-way choice),
  keyed by state, ability timing and clock.
* **creature-pose-items.md**: held weapons, quivers and the shield, items held while
  eating, playing or using, the extra models some abilities and races draw, the weapon
  tips and trails, the sparks of state 0x61, item glow and item size, the mount offsets,
  and the draw calls.

---

## 0. Conventions that matter for bit-exact results

The project checks poses bit for bit (§11), so every rounding step matters.

* All arithmetic is IEEE single precision (SSE, round to nearest, no fused multiply-add)
  unless a step says "in double". Each operation is rounded before the next. The order
  given is the game's order and must be kept; parentheses are written where the order is
  not plain left to right. "x + y" and "y + x" are the same (two-operand addition and
  multiplication are commutative in IEEE arithmetic); "(a + b) + c" and "a + (b + c)" are
  not.
* "In double" means: each float operand is widened exactly to double, the operations are
  done in double, and the result is rounded to float once at the end (unless stated).
* **min(K, x)** is the SSE minimum with the constant first: K if K < x, otherwise x. A NaN
  x therefore passes through. **max(K, x)** likewise: K if K > x, otherwise x.
  **clamp01(x)** is max(0, min(1, x)).
* Comparisons are written as the game tests them. "If X > t" is false when either side is
  NaN; it is not the same as "unless t ≥ X".
* "Truncate" converts toward zero: **trunc32** to a 32-bit integer, **trunc64** to a 64-bit
  integer. "a div b" is C signed integer division (truncating toward zero); "float(n)" is
  the exact conversion of a 32-bit integer.
* sinf, cosf and asinf are the Microsoft UCRT's (thunks 0x395456, 0x395450, 0x395468);
  sqrtf (0x395462) is correctly rounded. CubeVulkan already reproduces the UCRT's sinf,
  cosf and asinf (NativeCrt); these must be used.
* A **vector** is three floats (x, y, z) stored in that order. "u + v", "u − v" and "s × v"
  are component by component, x first; "s × v" multiplies each component by s
  (0xc7c40). "Half of u + v" means 0.5 × (u + v), the sum first.
* Writing "P.y += 3" means P.y = P.y + 3 with the field's current value.
* **DOT** is 1/65536 (1.52587890625e-05, float 0x37800000). **DEG** is 0.0174532924 (float
  0x3c8efa35), the degrees-to-radians factor of the rotation helpers.
* Double constants used: π = 3.141592653589793, π/2 = 1.5707963267948966,
  0.7π = 2.199114820062152, 180/π = 57.29577951308232, 0.5, 0.2, 0.7, 0.0625, 180.0, and
  0.800000011920929 (the float 0.8 widened).
* Float constants whose decimal form is not exact: 0.1 (0x3dcccccd), 0.2, 0.3 (0x3e99999a),
  0.4 (0x3ecccccd), 0.6, 0.65 (0x3f266666), 0.8, 0.85, 1.2 (0x3f99999a), 2.1 (0x40066666),
  4.1 (0x40833333), 6.05 (0x40c1999a), 6.2, 7.1, 7.2, 8.2 (as floats), 11.2 (0x41333333),
  0.001, 0.0015, 0.0017, 0.002, 0.0023, 0.0025, 0.003, 0.0045, 0.005, 0.0075, 0.01, 0.02,
  0.03, 0.04, 0.05, 0.075, 0.13, 0.15, 0.0001 (0x38d1b717), 1/14 (0.0714285746, 0x3d924925).
  All are the nearest float to the decimal shown.

---

## 1. Inputs

### 1.1 The call

| Argument | Meaning |
|---|---|
| 1 | pointer to A (C+0xc38). A+0x00 holds the pointer back to C. |
| 2 | the draw context, passed through to the draw calls (creature-pose-items.md §9) |
| 3 | the world's model table: a vector of model pointers (begin at +0x10, end at +0x18) |
| 4 | **dt**, the frame time in ms (float) |
| 5 | pointer to the draw **tint**: four floats |
| 6 | **alpha** (float), passed to draws |
| 7, 8 | pointers to the view origin **camX**, **camY** (64-bit dots) |
| 9 | **hide** mask: parts not drawn (§9) |
| 10 | pointer to the creature being ridden (its record), or none |
| 11 | **depth** (float), passed to draws as their last number |
| 12 | **scale** (float) applied to the body (§8.2) |

### 1.2 Values from the game's rules

These are pure functions of the creature at the time of the call; their specifications are
in `stat-rules.md` §7.1 and §2.6 and `effect-objects.md` §2.2. The routine calls them where
the steps below use them; calling them once up front gives the same values (nothing they
read is changed in between).

| Name | Routine | Meaning |
|---|---|---|
| **T** | 0x4f220(C) | the current ability's duration (integer) |
| **d1** | 0x4ead0(C, −1) | its first part (wind-up) |
| **d2** | 0x660d0(C, −1) | its middle part |
| **d3** | 0x5e9f0(C, −1) | its recovery part |
| **U** | 0x108fa0(C+0x1d8) | the use time of the held item (integer) |
| **buffE** | 0x5e3a0(C, 0x0e) | C has a buff of type 0x0e |
| usable(slot) | 0x50640(C, C+slot) | C may use the item in that slot |
| model(item) | 0x173b10(models, item) | the model of an item (none possible) |
| model#(id) | 0x17190(models, id) | entry *id* of the model table; none when *id* < 0 or *id* ≥ the entry count |
| now | timeGetTime() | the system clock in ms (32-bit unsigned) |
| rand() | the CRT's, slot 0x42cd28 | only for the sparks of state 0x61 |

When an integer from the table is used as a float it is converted with float(·) at the
point of use; sums such as "d1 + d2 div 2" are integer sums converted once.

### 1.3 A model

A model (what the table and 0x173b10 return) has its size in voxels as three 32-bit
integers at +0x88 (x), +0x8c (y), +0x90 (z). Writing **sx, sy, sz** for float of these.
**Centring** a matrix on a model means translating it by (sx × −0.5, sy × −0.5,
sz × −0.5).

### 1.4 Creature fields

R = read, W = written.

| Offset | Type | R/W | Use |
|---|---|---|---|
| +0x34, +0x38, +0x3c | f32 ×3 | R | velocity |
| +0x40, +0x44, +0x48 | f32 ×3 | R | movement intent |
| +0x58 | f32 | R | look pitch (added to head and weapon pitches) |
| +0x5c | u32 | R | movement flags: bit 0 (1) on ground, bit 2 (4) at a climbable surface |
| +0x60 | u8 | R | hostility kind (8 used) |
| +0x64 | i32 | R | race |
| +0x68 | u8 | R | current ability |
| +0x6c | f32 | R | ability clock |
| +0x7a, +0x7b, +0x7c | u8 ×3 | R | hair colour (red, green, blue) |
| +0x80 | u32 | R | appearance flags: bit 0 (hands are front feet), bit 1, bit 2 (state 0xb), bit 10 (0x400), bit 22 (0x400000), bit 24 (0x1000000) |
| +0x84, +0x88, +0x8c | f32 ×3 | R | size: width, depth, height |
| +0x90 … +0x9e | i16 ×8 | R | model ids: head, hair, hands, feet, chest, tail, arm part, wings (§3) |
| +0xa0 | f32 | R | head scale |
| +0xa4 | f32 | R | chest scale |
| +0xa8 | f32 | R | hand scale |
| +0xac | f32 | R | foot scale |
| +0xb0 | f32 | R | arm-part scale |
| +0xb4 | f32 | R | weapon scale |
| +0xb8 | f32 | R | tail scale |
| +0xbc | f32 | R | shoulder scale |
| +0xc0 | f32 | R | wing scale |
| +0xc4 | f32 | R | body pitch |
| +0xc8, +0xcc, +0xd0 | f32 ×3 | R | resting hand turn |
| +0xd4 | f32 | R | foot pitch |
| +0xd8 | f32 | R | wing pitch |
| +0xdc | f32 | R | tail pitch |
| +0xe0 … +0xe8 | f32 ×3 | R | chest place |
| +0xec … +0xf4 | f32 ×3 | R | head place |
| +0xf8 … +0x100 | f32 ×3 | R | hand place (x is the distance from the middle) |
| +0x104 … +0x10c | f32 ×3 | R | foot place (x likewise) |
| +0x110 … +0x118 | f32 ×3 | R | tail place |
| +0x11c … +0x124 | f32 ×3 | R | wing place (x likewise) |
| +0x128 | u16 | R | state flags: bit 0 (1) draws the glove grip, bit 2 (4) swimming, bit 6 (0x40) sprinting, bit 10 (0x400) gliding |
| +0x12c | f32 | R | dodge (roll) timer |
| +0x130 | f32 | R | sitting timer |
| +0x14c, +0x14d | u8 ×2 | R | class, specialisation |
| +0x158 | f32 | R | a value used by states 0x49 and the left weapon's glow |
| +0x15c, +0x168 | f32 ×3 each | R | two stances (states 0xf, 0x10, 7) |
| +0x174, +0x178, +0x17c | f32 ×3 | R | look direction; +0x17c is also race 0x12e's height |
| +0x188 | f32 | R | mana (race 0x11d's size) |
| +0x19c | u8 | R | a race-specific mode byte |
| +0x1d8 … +0x6d8 | items, 0xa0 each | R | +0x1d8 the item in use, +0x278 head, +0x318 neck, +0x3b8 chest, +0x458 feet, +0x4f8 hands, +0x598 shoulders, +0x638 left hand, +0x6d8 right hand |
| +0x990 | f32 | R | a height offset (blocks) subtracted from the drawn height |
| +0x994 | f32 | R | passed to most draws as their fourth number |
| +0x998 | f32 | R W | stride length; **state 1 writes 0 here** |
| +0x99c | f32 | R | stride phase (radians) |
| +0xad8 | i32 | R | swing counter (every third swing spins) |
| +0xb10, +0xb18, +0xb20 | i64 ×3 | R | drawn position (dots) |
| +0xb34, +0xb38, +0xb3c | f32 ×3 | R | drawn pitch, roll, yaw |
| +0xba8 | f32 | R | state 0x48's throw value |

An item (0xa0 bytes): category u8 +0x00, subtype u32 +0x04, material u8 +0x1c (the only
fields read here). "Left item kind" means the subtype at +0x63c; "right item kind" the
subtype at +0x6dc. Many tests read the kind without checking the category; the steps say
when the category is checked.

---

## 2. The animation state record A (at C+0xc38)

| Offset | Type | Written here | Meaning |
|---|---|---|---|
| +0x00 | pointer | no | the creature C |
| +0x08 | f32 | yes (set to 0 when the state is 0) | **t**, the state clock (ms); set by the world draw just before this call (§12) |
| +0x0c | i32 | yes (set to 0 when a blow ends) | **state**, the pose state; set by the world draw just before this call (§12) |
| +0x20 | f32 ×3 | eased | left hand shift (**LS**) |
| +0x2c | f32 ×3 | eased | right hand shift (**RS**) |
| +0x38 | f32 ×3 | eased | left hand turn (**LT**) |
| +0x44 | f32 ×3 | eased | right hand turn (**RT**) |
| +0x68 | f32 ×3 | eased | body turn (**BT**): x pitch, y roll, z yaw |
| +0x74 | f32 ×3 | eased by angle | whole-body angles (**WA**) |
| +0x80 | f32 ×3 | eased | body shift (**BS**) |
| +0x8c | f32 ×3 | eased | head shift (**HS**) |
| +0x98 | f32 ×3 | eased by angle | head angles (**HA**) |
| +0xa4, +0xa8, +0xac | f32 ×3 | eased by angle | tail angles (**TA**); the +0xa8 target is always 0 |
| +0xb0 | f32 ×3 | eased | stance (**ST**); its y component also leans the hands (§8.7) |
| +0xbc | f32 | eased | swim lean (**LN**) |
| +0xc0 | f32 ×3 | eased | whole-body shift (**WS**); z is in blocks |
| +0xcc | f32 | eased | foot kick (**KS**) |
| +0xd0 | f32 | eased | drop (**DR**), the height the board ability lowers the pivot by |
| +0xd8 … +0x130 | pointers ×12 | yes | sprites (§3) |
| +0x138 … +0x5b7 | 18 matrices | yes, when the part exists | part matrices (§8, items file) |
| +0x5b8 | f32 | no | size factor applied to the base matrix |
| +0x5bc … +0x8bb | 4 × 16 points | yes | weapon trails (items file §6) |
| +0x8bc | f32 ×3 | yes | eased look direction (ability 0x1d) |
| +0x8c8 | u8 | yes | set to 1 |
| +0x8d0, +0x8d8 | i64 ×2 | yes | copies of camX and camY |
| +0x8e0, +0x8e8 | list head, count | yes | the sparks of state 0x61 (items file §7) |

A matrix is 16 floats, column-major: elements 0–3 are the first column (the x axis), 4–7
the second, 8–11 the third, 12–15 the translation column.

The matrices kept in A:

| Offset | Part | Built from |
|---|---|---|
| +0x138 | body frame | base (§8.2) |
| +0x178 | hands frame | base (§8.7) |
| +0x1b8 | chest | body frame |
| +0x1f8 | head | body frame |
| +0x238 | hair | body frame |
| +0x278 | left arm part | hands frame |
| +0x2b8 | right arm part | hands frame |
| +0x2f8 | right weapon | hands or body frame |
| +0x338 | left weapon | hands or body frame |
| +0x378 | left hand | hands frame |
| +0x3b8 | right hand | hands frame |
| +0x3f8 | shoulders item | body frame |
| +0x438 | neck item | body frame |
| +0x478 | left wing | hands frame |
| +0x4b8 | right wing | hands frame |
| +0x4f8 | left foot | base |
| +0x538 | right foot | base |
| +0x578 | tail | body frame |

A matrix whose part is absent this call (no sprite, or hidden) is **not written**: it keeps
the previous call's contents. The body frame and hands frame are always written.

---

## 2a. Shared routines

### Matrices
All operate on a matrix M in place, applying the new transform on the right (in the part's
local frame). "c" and "s" are cosf and sinf of (angle × DEG), the product rounded to float
first.

* **identity** (0x17800).
* **translate(x, y, z)** (0x2b7a0, and 0x2b780 for a vector): for each row r = 0..3,
  M[12+r] = ((x × M[r] + y × M[4+r]) + z × M[8+r]) + M[12+r].
* **scale(x, y, z)** (0x2b580): if x ≠ 1, column 0 (M[0..3]) is multiplied by x; if y ≠ 1,
  column 1 by y; if z ≠ 1, column 2 by z (each element x × M[i]). A factor of exactly 1
  leaves its column untouched (this matters for −0 and NaN). "scale(v, uniform)" is scale(v, v, v).
* **rotZ(a)** (0x13050): for r = 0..3, with p = M[r], q = M[4+r]: M[r] = s × q + p × c,
  M[4+r] = c × q − p × s.
* **rotY(a)** (0x2b470): with p = M[r], q = M[8+r]: M[r] = p × c − s × q,
  M[8+r] = c × q + p × s.
* **rotX(a)** (0x2b360): with p = M[4+r], q = M[8+r]: M[4+r] = s × q + p × c,
  M[8+r] = c × q − p × s.
* **rotAxis(a, x, y, z)** (0x2b160; 0x2b130 takes the axis as a vector): len =
  sqrtf((x × x + y × y) + z × z); if len = 0 nothing happens. Otherwise x, y, z are divided
  by len, k = 1 − c, and the rotation R (column-major) is: R0 = (x × x) × k + c,
  R1 = (x × y) × k + s × z, R2 = (x × z) × k − s × y, R4 = (x × y) × k − s × z,
  R5 = (y × y) × k + c, R6 = (y × z) × k + s × x, R8 = (x × z) × k + s × y,
  R9 = (y × z) × k − s × x, R10 = (z × z) × k + c, R15 = 1, all others 0. Then M = M × R
  (0x173e0): each new element (row r, column j) = ((M[r] × R[4j] + M[4+r] × R[4j+1]) +
  M[8+r] × R[4j+2]) + M[12+r] × R[4j+3], all computed from the old M. A rotation by 0
  degrees is still performed in full (it can turn −0 into +0).
* **point(M, v)** (0x2b680): w = 1 / (((v.x × M[3] + v.y × M[7]) + v.z × M[11]) + M[15]);
  the result is (w × (((v.x × M[0] + v.y × M[4]) + v.z × M[8]) + M[12]), w × (… row 1 …),
  w × (… row 2 …)).

### Vectors
* length² (0x2b650): (x × x + y × y) + z × z. Planar length² (0x2b630): x × x + y × y.
* dot (0x17320) of u and v: (v.y × u.y + v.x × u.x) + v.z × u.z.
* cross (0x17380) of a and b: x = a.y × b.z − a.z × b.y, y = b.x × a.z − a.x × b.z,
  z = a.x × b.y − b.x × a.y.
* normalise (0x127f0): k = 1 / sqrtf(length²); each component × k. 0x1779b0 does the same
  into a copy.
* fabs (0x177c0): the absolute value (exact).
* xorshift (0x1777e0) of a 32-bit unsigned v: v ^= v << 13; v ^= v >> 17; v ^= v << 5.

### Easing
* **ease(dt, rate)** (0xa7a0): n = trunc32(double(dt) × 100.0). If n ≤ 0 the result is 0.
  Otherwise, in double, k = double(rate) × 0.02, acc = 0, and for i = 0 … n − 1:
  acc = acc + ((double(i) + 1.0) / double(n) − acc) × k. The result is acc rounded to float.
  (The game unrolls this loop by eight; the arithmetic per step is identical.)
* **Ease a float** (0x16ee0) or a vector component by component (0x16f20, and the 0x17250 /
  0xc7c40 / 0x17730 chain used for A's vectors): value = (target − value) × s + value, with
  s = ease(dt, rate).
* **Ease an angle** (0x17840), from **a** toward **b** by **s**: in float, ra = float(double(a
  / 180.0) × π), rb likewise; ca, sa = cosf, sinf of ra; cb, sb of rb. If
  (double(cb) × double(ca) + double(sb) × double(sa)) ≥ −0.8999999761581421 (in double):
  d = float(double(sb) × double(ca) − double(cb) × double(sa)); d = max(−1, min(1, d)); the
  result is float(double(asinf(d)) × ((double(s) × 180.0) / π)) + a (the float sum).
  Otherwise (nearly opposite) the result is b: the angle jumps.

---

## 3. Sprites

Done first after the rings of race 0x12e (creature-pose-items.md §5.1), in this order:

1. A+0x8c8 = 1; A+0x8d0 = camX; A+0x8d8 = camY.
2. Clear the head, hair and arm-part sprites (A+0xd8, +0xe0, +0xf0).
3. "Worn(slot)" is model(item) when usable(slot), else none.
   * **head item** (kept for §8.6, not in A) = worn(+0x278);
   * hands A+0xf8 = worn(+0x4f8); feet A+0x110 = worn(+0x458); shoulders item A+0x128 =
     worn(+0x598); neck item A+0x130 = worn(+0x318);
   * **right weapon** A+0x118 = model(+0x6d8) when usable(+0x6d8) and the left item is not
     two-handed; else none;
   * **left weapon** A+0x120 = model(+0x638) when usable(+0x638) and the right item is not
     two-handed; else none;
   * chest A+0xe8 = worn(+0x3b8).
   "Two-handed": category 3 and subtype < 24 and the subtype is one of 5, 6, 7, 8, 0xa,
   0xb, 0xf, 0x10, 0x11, 0x12, 0x17 (mask 0x878de0).
4. Clear the tail and wing sprites (A+0x108, +0x100).
5. Fill from the appearance ids (model#(id) of C's i16 fields) whatever is still none, in
   this order: head A+0xd8 = model#(+0x90) and hair A+0xe0 = model#(+0x92) (always, since
   both were cleared); hands A+0xf8 = model#(+0x94); feet A+0x110 = model#(+0x96); chest
   A+0xe8 = model#(+0x98); tail A+0x108 = model#(+0x9a); wings A+0x100 = model#(+0x9e);
   arm part A+0xf0 = model#(+0x9c).

Later steps change sprites: the shield (§4.5), some states (the tables), hidden parts
(§9). "The creature holds two weapons" below means both A+0x118 and A+0x120 are set at
that moment; "**one weapon**" means A+0x118 is set and A+0x120 is not.

**holdsTwoHander** (0x5e9b0): the right item is two-handed, or the left item is.

---

## 4. Appearance values and the walk swing

### 4.1 Scales and places
Read once: the scales of §1.4 (head, chest, hand, foot, arm part, weapon, tail, shoulder).
**heldScale** starts as the weapon scale (state 0x4d multiplies it by 1.5).
**Cp** (chest place) = C+0xe0..0xe8; **Hp** (head place) = C+0xec..0xf4. State 0x2a changes
Hp (its table entry); nothing else does.

**Resting hand turns**: if the state is 0, **RTr** = (C+0xc8, C+0xcc, C+0xd0) and
**LTr** = (C+0xc8, −C+0xcc, −C+0xd0) (sign flips are exact); otherwise both are (0, 0, 0).

### 4.2 The swing
1. **phase** = sinf(float(double(C+0x99c) + π/2)).
2. **swing** = float(double(C+0x998) × 0.7π × double(phase)) (in double, left to right).
3. If the ability is 0x50, or it is 0x7a and C+0x5c bit 0 is set: swing × 0.5.
4. If the ability is 0x76, 0x77 or 0x79: swing × 0.1.
5. **climbing** = the ability is 0x7a and C+0x5c bit 2 is set.
6. **running** is set when the ability is 0x32 or 0x52, or when all of: not climbing,
   C+0x128 bit 6 set, the intent's length² ((ix × ix + iy × iy) + iz × iz) > 0.1, and the
   velocity's planar length² (vx × vx + vy × vy) > 0.1. When running: swing × 1.25.
7. If C+0x80 bit 1 is set and C+0x5c bit 0 is clear: swing = 0.
8. If C+0x80 bit 0 is set: swing × 0.6.
9. **baseSwing** = swing (kept from here on).
10. If the right item kind is 5 and the ability is not 0: swing × 0.25.
11. If the state is 0x34 or 0x35 and float(U) > C+0x6c: swing × 0.1.
12. If the state is 0x36 or 0x51: swing × 0.1.
13. If (the state is 0x5a … 0x60 or 0x62) and 5000 > t: swing × 0.1.
14. If the ability is 9, 0x17, 0x1d, 0x25 or 0x40, or the state is 2, 3, 0x26, 0x31 or
    0x32: swing × 0.5.
15. If the ability is 0x78: swing = 0.

### 4.3 Values derived before the state is consulted
* **DR target** = C+0x8c × 0.5 when (0x5ee70) the ability is 0x78, C+0x5c bit 0 is clear,
  C+0x130 ≤ 0 and C+0x12c ≤ 0; else 0.
* All other targets start at 0 (every component).
* **Swim lean target LN**: only when C+0x128 bit 2 is set. ix, iy = C+0x40, +0x44; fx, fy =
  C+0x174, +0x178; v2 = ix × ix + iy × iy. If v2 > 5 and f2 = fx × fx + fy × fy > 0:
  ix and iy are each × (1 / sqrtf(v2)); fx and fy each × (1 / sqrtf(f2));
  d = max(−1, min(1, fx × iy − fy × ix)); LN = float(double(asinf(d)) × 57.29577951308232);
  if (fx × ix + fy × iy) < −0.2, LN = −LN.
* **splay** = float(double((baseSwing × 0) × 80) / π) (a signed zero, or NaN).
* **a** = float(double(baseSwing × 180) / π); **legL** = a × 1.5; **legR** = (−a) × 1.5.
  (The game multiplies each by 1.5 once whatever its sign.)
* stride4 = C+0x998 × 4; **liftR** = (1 − phase) × stride4; **liftL** = stride4 × phase.
  If C+0x80 bit 1 is set and C+0x5c bit 0 is clear, both are 0.
* Overrides, in order: if the state is 0x3e, 0x3f, 0x40 or 0x35, or the ability is 0x9c
  or 0x9d: splay = 10, legL = legR = 40. State 0x56: legL = 30, legR = −10. State 0x38:
  legL = legR = −10. State 0x3a: legL = legR = −40, splay = 5.
* **turnOut** = false; it turns the left foot outward (§8.9). States set it in the tables.

### 4.4 Rest places
**Hands.** If C+0x80 bit 0 is set: **Lr** = (−C+0xf8, C+0xfc, C+0x100), **Rr** = (C+0xf8,
C+0xfc, C+0x100). Otherwise, with r = C+0xf8 × C+0x998 + C+0xf8:
* Lr.x = −(cosf(swing) × r); Rr.x = −Lr.x;
* Lr.y = −(sinf(swing) × r); Rr.y = −Lr.y;
* Lr.z = (C+0x100 − 4.1) + C+0x998 × 6; Rr.z = C+0x998 × 6 + (C+0x100 − 4.1) (the same);
* Lr.y += C+0xfc; Rr.y += C+0xfc;
* if climbing: k = sinf(float(double(C+0x998) × π × double(phase))) × 8; Lr.z += k;
  Rr.z −= k.

Then in both cases Lr.y += (2.1 − C+0x998 × 5) and Rr.y += (2.1 − C+0x998 × 5).

**Arm parts** (between hand and chest):
* **Lm** = ((C+0xf8 × −0.3 + Lr.x) × 0.5, ((Lr.y − 8) × 0.5) × 0.5, (Lr.z + (C+0x100 + 12)) × 0.5);
* **Rm** = ((Rr.x + C+0xf8 × 0.3) × 0.5, float(double((Rr.y − 8) × 0.5) × 0.5),
  (Rr.z + (C+0x100 + 12)) × 0.5).

**Feet**: **Fl** = (−C+0x104, C+0x108, C+0x10c); **Fr** = (C+0x104, C+0x108, C+0x10c).
**Tail**: **Tp** = (C+0x110, C+0x114, C+0x118).

### 4.5 The shield
If the left item's category is 3 and its kind is 0xe: **shield** = model(+0x638) (not
checked for usable) and the left weapon sprite A+0x120 is cleared. Otherwise there is no
shield. It is drawn on the back (items file §3.4).

### 4.6 The rate and gliding
* **rate** = 0.01 (the default easing rate).
* **Gliding** (C+0x128 bit 10 set): WA = (−70, 0, 0); HS = (0, −1, 0); WS.z =
  (C+0x88 − C+0x8c) × 0.5; RS = (0, 3, 1); LS = (0, 3, 1); HA.x = 70; if the right item kind
  is 7, RS.y = 11; legL = legR = −20. Then, if the ability is not 0 and float(T) > C+0x6c:
  RS.x += 0, LS.x += 0, RS.y += −5, RS.z += 6, LS.y += −5, LS.z += 6.

---

## 5. Choosing the pose

### 5.1 The state
A+0x0c holds the pose state. The world's creature draw sets it, and its clock A+0x08,
from C's current ability immediately before calling this routine (§12); this routine
reads it, and resets it to 0 only when certain blows finish (the tables say where). The
routine never picks a state itself; everything it does per state
is in **creature-pose-tables.md**. States 1–0x66 have entries; states 0x0c, 0x0d, 0x0e,
0x25, 0x46, 0x53 and every value outside 1..0x66 (including 0) have none: nothing is set
for them, as for the "nothing" rows of the tables.

The tables use: **t** = A+0x08 read when the state is consulted; **c6c** = C+0x6c; d1, d2,
d3, T, U (§1.2). A comparison "X > t" with an integer X means float(X) > t.

### 5.2 Values a state may change besides the targets
* the rest places Lr, Rr, Lm, Rm, Fl, Fr, Tp, Hp;
* sprites A+0xd8, +0xf8, +0x110, +0x108, +0x118, +0x120 (and the model #0x401, #0xb04 that
  some states give the hands);
* **rate** (each table row states its rate; "rate 0.01" is the default);
* **splay**, **turnOut**, **heldScale**;
* the state A+0x0c (set to 0 when a blow ends);
* C+0x998 (state 1).

### 5.3 The kept values
After the state's entry, six values are used by the common steps of §5.4. They are always
**K90** = 90, **K10** = 10, **K5** = 5, **K2** = 2, **K30** = 30, and **K4** = 4, with one
exception: in state 0x37 when C's intent length² is ≤ 0 (the "not moving" row), K4 is the
right hand's rest height Rr.z as it was when the state was consulted. (The game keeps
these in registers; that path is the only one that does not reload the 4.)

### 5.4 After the state (common to every state), in order
1. If the ability is 0x52: legR = K30, turnOut = true.
2. If the state (A+0x0c, possibly just reset) is 0: t (A+0x08) = 0.
3. Lm = (Lr + Lm) × 0.5; Rm = (Rr + Rm) × 0.5 (sum first, then each × 0.5).
4. **Sitting** (C+0x130 > 0): if C+0x80 bit 0 is clear: WA = (K90, 0, 0), WS.z =
   (C+0x88 − C+0x8c) × 0.5; else WA = (0, K90, 0), WS.z = (C+0x84 − C+0x8c) × 0.5.
5. If climbing: RS.z += 7; LS.z += 7; LT.x += K90; RT.x += K90.
6. If the left item kind is 0x14 (category not checked): LS.y += K4; LS.z += K4; LT.x = 0.
7. **Moving lean**, only if C+0x80 bit 0 is clear and the ability is none of 0x76, 0x77,
   0x79, 0x80, 0x9c, 0x9d:
   * if running: WA.x += −25; HA.x += K10; if the ability is 0x51, 0x52 or 0x92: WA.x +=
     −20, LS.y += −5, RS.y += −5; then if the ability is 0x52: BT.z = 30;
   * otherwise, if the velocity's planar length² > 0.1 and dot(intent, velocity) > 0:
     WA.x += −10; HA.x += K5.
8. If C's hostility kind is 8 and the race is 0xf4 … 0xf7: if C+0x19c ≠ 0, WS.z = 10, else
   WS.x = 0.
9. Races 0xb5 … 0xb8: HA.x = 45 if C+0x19c is 2, else 0.
10. Races 0xea, 0xeb: HA.x = 45 if C+0x19c ≠ 0, else −45.
11. Hostility kind 8 and race 0xc2 … 0xc5: HA = (0, 0, 120) if C+0x19c ≠ 0, else (0, 0, 0).
12. If buffE: WS.z = WS.z − C+0x8c × 0.25.
13. If the ability is 0x7a and (C+0x5c & 5) = 1: BS.y += K2; BS.z += −3; BT.x = −30;
    LS.y = 2; RS.y = 2; LS.x += 1; RS.x += −1.

---

## 6. Easing

s = ease(dt, rate) (computed once, as the ease of 0 toward 1). Then, in this order:

| Field | Target | Kind |
|---|---|---|
| A+0x44 | RT | vector |
| A+0x38 | LT | vector |
| A+0x2c | RS | vector |
| A+0x20 | LS | vector |
| A+0x68 | BT | vector |
| A+0xc0 | WS | vector |
| A+0xd0 | DR | float |
| A+0x74, +0x78, +0x7c | WA.x, .y, .z | angle |
| A+0x98, +0x9c, +0xa0 | HA.x, .y, .z | angle |
| A+0xa4, +0xa8, +0xac | TA.x, 0, TA.z | angle |
| A+0xbc | LN | float |
| A+0xb0 | ST | vector |
| A+0xcc | KS | float |
| A+0x80 | BS | vector |
| A+0x8c | HS | vector |

"Vector": each component (target − value) × s + value. "Angle": §2a angle easing.

**Look direction** (A+0x8bc): if the ability is 0x1d, v = C+0x174..0x17c; if length²(v) > 0,
v is normalised; A+0x8bc is eased toward v by ease(dt, 0.01). Otherwise A+0x8bc = (0, 0, 0).

**Final hand places** (not stored, used from here on):
* **RTurn** = RTr + A+0x44; **LTurn** = LTr + A+0x38;
* Rr = Rr + A+0x2c; Lr = Lr + A+0x20;
* Rm = 0.4 × A+0x2c + Rm; Lm = 0.4 × A+0x20 + Lm.

---

## 7. Hidden parts and sparks

1. Hide bit 1 (§9): the head sprite A+0xd8 = none. Bit 2: hair A+0xe0 = none. Race 0x10c with
   C+0x19c = 0: hair = none. Bit 8: chest A+0xe8 = none. Bit 4: shoulders item A+0x128 =
   none. Bit 0x40: both weapon sprites = none.
2. Sparks: if the state is 0x61 or the spark list is not empty, see creature-pose-items.md
   §7 (this step comes here, before the base matrix).

---

## 8. Matrices

### 8.1 The roll
**rolling** applies when C+0x12c ≠ 0, unless all of: class 4, specialisation 1,
float(T) > C+0x6c, and the ability is 6, 0x12 or 0x15.

### 8.2 The base matrix and the body frame
**Base** (a local matrix, also the start of the feet):
1. identity; translate(float(C+0xb10 + camX) × DOT, float(C+0xb18 + camY) × DOT,
   float(z) × DOT) with z = trunc64(A+0xc8 × 65536) + (C+0xb20 − trunc64(C+0x990 × 65536))
   (64-bit integer sums, the products in float).
2. If rolling: angle = (1 − C+0x12c / 600) × −360; but for the abilities 0x6c and 0xa0,
   angle = (C+0x6c / float(T)) × −360. axis = cross(velocity C+0x34, (0, 0, 1)); if its
   length² < 0.0001, axis = cross(C+0x174, (0, 0, 1)); race 0x65: axis = the velocity. If
   length²(axis) > 0.01: normalise it and rotAxis(angle, axis). Then, unless the ability
   is 0x6c: Lr.z += (C+0x8c × 5) / C+0x84 and Rr.z += (C+0x8c × 5) / C+0x84.
3. If the ability is 0x78: Lr.z and Rr.z each += (C+0x8c × 5) / C+0x84 (again, if it also
   rolled).
4. rotZ(C+0xb3c); translate(0, 0, A+0xd0); rotY(C+0xb38); rotX(C+0xb34);
   translate(A+0xc0, A+0xc4, 0); rotZ(A+0x7c); rotY(A+0x78); rotX(A+0x74);
   translate(0, 0, −A+0xd0).
5. size = C+0x88 if C+0x88 > C+0x84, else C+0x84; size = size / 11.2; race 0x11d: size ×
   C+0x188; size = size × A+0x5b8; scale(size, size, size).
6. **Beat**: for race 0x11d or races 0x25 … 0x28 with k = 0.1; races 0x125, 0x126 with
   k = 0.05; races 0xef … 0xf1 with k = 0.03: u = float(now mod 1000) / 1000; x =
   cosf(float(double(u + u) × π)) × k; scale(1 − x, 1 − x, x + 1). (The game reads the
   clock here, at this point of the call.)

**bob** = fabs(baseSwing) × 4; ×4 again if the ability is 0x50.

**Body frame** A+0x138 = base; translate(0, 0, bob); translate(A+0x80, A+0x84, A+0x88);
scale(scale, scale, scale) (argument 12); rotZ(A+0x70); rotY(A+0x6c);
rotX(C+0xc4 + A+0x68).

Then come the board, raft and eagle (items file §5.2), and the item held while eating,
playing or using (items file §4.1, which uses the hands frame **as the previous call left
it**).

### 8.3 The body parts (in this order, each only if its sprite is set)
Every draw below passes the call's tint, alpha, **x9** (−1 if C+0x80 bit 22 is set, else 0)
and depth; the glow (third number) and fourth number are given per part (items file §9).
**glow(slot)** is the item glow of C's item at that offset (items file §8.1). **glowAll** =
C+0x80 bit 24. "scale(v, uniform)" means scale(v, v, v). **bodyTurn** = float(double(swing × 0.5 × 180) / π +
double(A+0xbc × 0.5)).

1. **Chest** A+0x1b8: body frame; translate(Cp); rotZ(bodyTurn); scale(chest scale, uniform);
   race 0x127: k = cosf(C+0x6c × 0.01) × 0.05, scale(1 − k, 1 − k, k + 1); centre on the
   model. Glow: 1 if glowAll else glow(+0x3b8). Fourth: C+0x994.
2. **Neck item** A+0x438: body frame; translate((0, 3, 6.05) + Cp); rotZ(bodyTurn);
   rotX(float(double(swing) × 0.7 × double(swing) × 180.0 / π)) (in double, left to right);
   scale(chest scale, uniform); translate(sx × −0.5, −1.5, −sz). Glow: 1 if glowAll else
   glow(+0x318). Fourth: C+0x994.
3. **Shoulders item** A+0x3f8: body frame; translate(0, 0, (C+0x998 + C+0x998) + 4);
   translate(Cp); rotZ(float(double(swing × 0.5 × 180) / π)); scale(shoulder scale, uniform);
   scale(chest scale + 0.1, uniform); centre. Glow: 1 if glowAll else glow(+0x598). Fourth:
   C+0x994.
4. **Tail** A+0x578: body frame; translate(Cp); translate(Tp); rotX(C+0xdc);
   rotZ(float(double(swing × −180) / π)); rotZ(A+0xac); rotY(A+0xa8); rotX(A+0xa4);
   scale(tail scale, uniform); scale(chest scale, uniform); translate(sx × −0.5, −sy, sz × −0.5).
   Glow 0. Fourth: C+0x994.
5. **Head** A+0x1f8: see §8.4. Glow 0. Fourth: C+0x994.
6. Ability 0x8a: the piece over the head (items file §5.3).
7. **Hair** A+0x238: §8.5.
8. **Head item** (worn(+0x278)), not stored: §8.6.
9. The **hands frame** A+0x178 (§8.7) is built here, then the arm parts, items held
   between the hands, quivers, weapons, the shield, the trails (items file), then the
   hands (§8.8), wings and feet (§8.9).

### 8.4 The head
**horned** = race 0xb5 … 0xb8 (0x5ee10). **special** = hostility kind 8 and race 0xc2 … 0xc5
(0x5edb0).

A+0x1f8 = body frame; translate(Hp.x + A+0x8c, Hp.y + A+0x90, (Hp.z + A+0x94) − bob × 0.5);
translate(0, 0, −3); if horned: translate(0, sy × −0.25, 0); else if special:
translate((sx × −0.5) × head scale, (sy × 0.5) × head scale, 0); rotX(−A+0x68 − C+0xc4);
rotY(−A+0x6c); rotZ(A+0xa0 − A+0x70); rotY(A+0x9c); rotX((C+0x58 − C+0xb34) + A+0x98);
translate(0, 0, 3); scale(head scale, uniform); then: horned: translate(sx × −0.5, 0, sz × −0.5);
race 0x65: translate(sx × −0.5, sy × −0.2, sz × −0.5); special: translate(0, −sy, sz ×
−0.5); otherwise centre.

The **head frame** used by the hair and the head item is the same up to and including
translate(0, 0, 3), without the two horned/special translations.

### 8.5 The hair
A+0x238 = head frame; scale(head scale + 0.001, uniform); centre. Colour = (tint[0] ×
(float(C+0x7a) / 255), tint[1] × (float(C+0x7b) / 255), tint[2] × (float(C+0x7c) / 255),
tint[3] × 1). Second number = 1 if C+0x80 bit 10 is set, else 0. If C+0x80 bit 22 is set:
second number = −1 and every colour component = (g × tint[i]) / 255 with g =
float(C+0x7b). Draw with that colour, alpha, the second number, glow 0, fourth C+0x994, and
fifth depth + 0.0001.

### 8.6 The head item (worn +0x278)
A local matrix: head frame; scale(head scale, uniform); translate(sx × −0.5, sy × −0.5, hsz ×
−0.5) where sx, sy are the item model's and hsz is the **hair** sprite's z size (as A+0xe0
is now; the game does not check that a hair sprite exists). Glow: glow(+0x278) (glowAll
not applied). Fourth: C+0x994.

### 8.7 The hands frame
A+0x178 = base; translate(0, 0, bob); translate(A+0x80, A+0x84, A+0x88); unless climbing,
or the state is 2 or 3, or the state is 0x26: rotX(C+0x58 + C+0x58); if gliding (C+0x128
bit 10): rotX(60); translate(0, 0, −4); rotY(A+0xb4); translate(0, 0, 4); rotZ(A+0x70);
rotY(A+0x6c); rotX(A+0x68). (The call's scale is not applied here.)

**Arm parts** (sprite A+0xf0): left A+0x278 = hands frame; translate(Lm); scale(arm-part
scale, uniform); centre. Right A+0x2b8 likewise with Rm. Both: glow 0, fourth C+0x994.

### 8.8 The hands (sprite A+0xf8)
**Left** (A+0x378), unless hide bit 0x10: hands frame; if C+0x80 bit 0: rotAxis(legR, 1, 0,
0); translate(Lr); rotZ(LTurn.z); rotY(LTurn.y); if C+0x80 bit 0: rotAxis(legR, 1, 0, 0);
rotX(LTurn.x); scale(hand scale, uniform); translate(0.0017 − sx × 0.5, 0.0017 − sy × 0.5,
0.0017 − sz × 0.5). Glow: glow(+0x4f8). Fourth: C+0x994. Then, if C+0x128 bit 0 is set, the
glove grip (items file §5.4).

**Right** (A+0x3b8), unless hide bit 0x20: the model is the hand sprite with k = hand scale;
but if the ability is 0x93, or it is 0x94 and float(T) > C+0x6c, the model is model#(0xe21)
and k = hand scale × 0.75. Hands frame; if bit 0: rotAxis(legL, 1, 0, 0); translate(Rr);
rotZ(RTurn.z); rotY(RTurn.y); if bit 0: rotAxis(legL, 1, 0, 0); rotX(RTurn.x);
scale(−k, k, k); translate(0.0023 − sx × 0.5, 0.0023 − sy × 0.5, 0.0023 − sz × 0.5).
Drawn **mirrored** (0xee990). Glow 0. Fourth: C+0x994.

### 8.9 Wings and feet
**Wings** (sprite A+0x100): flap = cosf(C+0x99c × 0.5) × (C+0x998 × 90) − 45. Left A+0x478:
hands frame; translate(−C+0x11c, C+0x120, C+0x124); rotX(C+0xd8); rotY(flap); rotZ(−90);
rotX(90); scale(wing scale, uniform); translate(sx × −0.5, 0, sz × −0.5). Right A+0x4b8: the same
with translate(C+0x11c, …) and rotY(−flap). Glow 0, fourth C+0x994.

**Feet** (sprite A+0x110), with sb = fabs(baseSwing):
* **Left** A+0x4f8, unless hide bit 0x80: base; rotZ(A+0xbc); rotAxis((legL + A+0xcc) +
  C+0xd4, 1, 0, 0); rotY(splay); translate(Fl.x, Fl.y, (sb × 3 + Fl.z) + liftL); ability
  0x50: rotX(legL × −1.5); scale(foot scale, uniform); if turnOut: rotZ(60), else
  rotAxis(legL + A+0xcc, 1, 0, 0); centre. Glow: glow(+0x458). Fourth C+0x994.
* **Right** A+0x538, unless hide bit 0x100: base; rotZ(A+0xbc); rotAxis(legR + A+0xcc, 1, 0,
  0); rotY(−splay); translate(Fr.x, Fr.y, (sb × 3 + Fr.z) + liftR); ability 0x50:
  rotX(legR × −1.5); scale(−foot scale, foot scale, foot scale); rotAxis((legR + A+0xcc) +
  C+0xd4, 1, 0, 0); centre. Drawn mirrored. Glow: glow(+0x458). Fourth C+0x994.

## 9. The hide mask (argument 9)

| Bit | Effect |
|---|---|
| 0x01 | no head |
| 0x02 | no hair |
| 0x04 | no shoulders item |
| 0x08 | no chest |
| 0x10 | no left hand (nor glove grip) |
| 0x20 | no right hand |
| 0x40 | no weapons |
| 0x80 | no left foot |
| 0x100 | no right foot |

---

## 10. Order of the whole call

1. Race 0x12e's rings (items §5.1).
2. Sprites (§3).
3. Scales, places, resting turns (§4.1); swing (§4.2); §4.3; rest places (§4.4); shield
   (§4.5); rate and gliding (§4.6).
4. The state's entry (tables).
5. The common steps (§5.4).
6. Easing (§6), look direction, final hand places.
7. Hidden parts; sparks (§7).
8. Base matrix (§8.2), bob, body frame.
9. Board, raft, eagle (items §5.2); items held in the old hands frame (items §4.1).
10. Chest, neck item, shoulders item, tail, head, head piece (0x8a), hair, head item.
11. Hands frame; arm parts; items between the hands (items §4.2); quivers (items §3.3);
    right weapon; left weapon (items §3); shield (items §3.4).
12. Weapon tips relative to the creature, then the trails (items §6).
13. Left hand (and glove grip), right hand, wings, left foot, right foot.

---

## 11. The recorded-pose oracle

`tests/EngineTests.cpp` (the `animoracle` command, function `animOracle`) checks the
port against recordings made by the ZoneExtractor tool's animation probe
(`tools/ZoneExtractor/ZoneExtractor.cpp`, namespace `animprobe`), which hooks 0x17ab0 and
records each call before letting it run. Recordings are `Extract/oracle/anim-*.bin`.

### File format
4 bytes "CWA2" or "CWA3", then records, each preceded by its byte length (u32). A record:

| Field | Size | Notes |
|---|---|---|
| creature address | u64 | identifies the creature across records |
| creature bytes | 0x15f8 | C as the call found it (A included) |
| dt | f32 | argument 4 |
| stack arguments | 8 × u64 | arguments 5–12: [0] tint pointer, [1] alpha (low 4 bytes, f32), [2] camX pointer, [3] camY pointer, [4] hide (low 32 bits), [5] ridden creature pointer, [6] depth (f32), [7] scale (f32) |
| tint | 16 | the 4 floats at argument 5 |
| camX, camY | 8 + 8 | the i64 at arguments 7 and 8 |
| ridden creature | 1 + 0x200 | CWA3 only: a byte (1 if there is one) then its first 0x200 bytes (zeros if none). CWA2 has 8 bytes here instead, skipped |
| now | u32 | timeGetTime() read just before the game's call |
| T, d1, d2 | i32 ×3 | |
| d3 | i32 | CWA3 only |
| U | i32 | |
| buffE | u8, then 3 pad | |
| item slots | 9 × (1 + 7 + 8 + 0xa0) | for +0x1d8, +0x278, +0x318, +0x3b8, +0x458, +0x4f8, +0x598, +0x638, +0x6d8: usable byte, 7 pad, the model's address (u64, 0 = none), the model's first 0xa0 bytes (sizes at +0x88) |
| model ids | u32 count, then count × (4 + 4 + 8 + 0xa0) | id (i32), pad, address, first 0xa0 bytes: ids 0x400, 0x401, 0xa46, 0xb04, 0xd0a, 0xdeb, 0xe21, 0xe2f, 0xe30, 0xe33, 0xe35, then the creature's eight i16 ids at +0x90..+0x9e |

A failing rule call is recorded as −0x7fffffff.

### What is compared
For each record that has an earlier record of the same creature address, the port is run
on the earlier record (its creature bytes, arguments and rule values; model lookups answer
from the recorded slots and ids; models compare by recorded address) and the result is
compared, byte for byte, with the later record's creature bytes in two ranges:
* A+0x20 … A+0xd3 (every eased pose field);
* A+0x138 … A+0x8c7 (the 18 matrices, the size factor and the trails).

If they differ, the port is run once more with now + 1 (the probe reads the clock a moment
before the game does; this only matters for the races that beat, §8.2) and that result is
used if it matches. Draw calls are not recorded or compared; neither are the sprite
pointers, the look direction A+0x8bc, the spark list or the state and clock fields (those
are set before the call, §12, and checked there).

This relies on nothing else writing those ranges of A between two calls for the same
creature.

---

## 12. Setting the state and its clock (0xd3170)

Nothing in the ability, AI, upkeep or ability-start routines (0x60460, 0x60720, 0x2c5250
and the rest) writes A+0x08 or A+0x0c. The only writers in the executable are the world's
creature draw 0xd3170, the pose routine itself (§5.4 step 2 and the blow-end resets of the
tables) and a whole-record copy (§12.5). The state is not a lasting animation state: it is
worked out again from the current ability every time the creature is drawn in the world,
and the clock is simply the ability clock at that moment.

### 12.1 When

0xd3170 walks the world's creature list (world +0x40) once per frame. A creature reaches the
pose call only if all of these hold, in this order:

1. not (draw flag bit 0 is set and C is the local player); draw flag bit 0x80 set means only
   the local player is drawn (the flags are the world draw's, 0xe2e00's last argument);
2. C+0x180 (health) > 0;
3. it passes the **visibility test** 0xe08c0 (§12.4).

When it does, the draw, right after the visibility test and before it works out the
creature's tint and calls the pose routine (nothing in between reads or writes these):

1. **A+0x08 = C+0x6c** (the ability clock, copied as it is, bit for bit);
2. **A+0x0c = state(C)** (§12.2);
3. if C+0x134 (the lock timer) ≠ 0 (NaN counts as ≠ 0), A+0x0c = **0x3d** instead.

A creature that fails any test keeps the state and clock of the last frame it was drawn:
they are not cleared, and nothing advances the clock while it is not drawn. The two other
callers of the pose routine, the screen-space circle of creatures that 0xe2e00 draws from
its creature-list argument and the preview widget 0xdb630, do **not** set them; they draw
with whatever the last world draw left.

The pose routine then runs on that state (the tables) and, if the state is 0 or a blow-end
row resets it to 0, sets A+0x08 = 0 (§5.4 step 2). The reset does not stick: the next drawn
frame sets both again from the ability, and the table's own end test (on t, which is again
the ability clock) decides once more whether to reset.

### 12.2 The state from the ability

state(C) depends on the current ability C+0x68 (a byte) and, for three abilities, on items:

| Ability | State | Ability | State | Ability | State | Ability | State | Ability | State | Ability | State |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 0x02 | 0x10 | 0x03 | 0x0f | 0x04 | 0x12 | 0x05 | 0x11 | 0x06 | 0x22 | 0x07 | 0x14 |
| 0x08 | 0x13 | 0x09 | 0x09 | 0x0a | 0x12 | 0x0b | 0x0a | 0x0c | 0x1d | 0x0d | 0x1e |
| 0x0e | 0x1f | 0x0f | 0x20 | 0x10 | 0x21 | 0x11 | 0x24 | 0x12 | 0x24 | 0x13 | 0x11 |
| 0x14 | 0x12 | 0x15 | 0x33 | 0x16 | 0x33 | 0x17 | 0x15 | 0x18 | 0x15 | 0x19 | 0x16 |
| 0x1a | 0x16 | 0x1b | 0x48 | 0x1c | 0x49 | 0x1d | 0x0b | 0x1f | 0x42 | 0x20 | 0x42 |
| 0x21 | 0x42 | 0x22 | 0x42 | 0x23 | 0x42 | 0x24 | 0x44 | 0x25 | 0x1c | 0x26 | 0x17 |
| 0x27 | 0x1a | 0x28 | 0x18 | 0x29 | 0x19 | 0x2a | 0x18 | 0x2b | 0x19 | 0x2c | 0x17 |
| 0x2d | 0x1a | 0x2e | 0x1a | 0x2f | 0x17 | 0x30 | 0x1a | 0x31 | 0x01 | 0x32 | below |
| 0x33 | 0x42 | 0x35 | 0x4c | 0x37 | 0x01 | 0x38 | 0x16 | 0x3a | 0x02 | 0x3b | 0x03 |
| 0x3c | 0x04 | 0x3d | 0x02 | 0x3e | 0x32 | 0x3f | 0x12 | 0x40 | 0x06 | 0x41 | 0x05 |
| 0x42 | 0x31 | 0x43 | 0x32 | 0x44 | 0x26 | 0x45 | 0x27 | 0x46 | 0x28 | 0x47 | 0x29 |
| 0x48 | 0x2a | 0x49 | 0x2a | 0x4a | 0x2b | 0x4b | 0x2c | 0x4c | 0x2d | 0x4d | 0x2e |
| 0x4e | 0x2f | 0x4f | 0x30 | 0x50 | 0x37 | 0x53 | 0x1b | 0x54 | 0x34 | 0x55 | 0x35 |
| 0x56 | 0x36 | 0x57 | 0x3e | 0x58 | 0x38 | 0x59 | 0x39 | 0x5a | 0x38 | 0x5b | 0x3a |
| 0x5c | 0x3b | 0x5d | 0x3c | 0x5f | 0x41 | 0x60 | 0x42 | 0x61 | 0x42 | 0x62 | 0x42 |
| 0x63 | 0x42 | 0x64 | 0x45 | 0x65 | 0x42 | 0x67 | below | 0x68 | 0x1a | 0x69 | 0x1a |
| 0x6a | 0x1a | 0x6b | 0x1a | 0x6c | 0x47 | 0x6e | 0x08 | 0x71 | 0x4a | 0x73 | 0x4a |
| 0x74 | 0x4b | 0x75 | 0x42 | 0x76 | 0x3f | 0x77 | 0x3f | 0x79 | 0x40 | 0x7b | 0x4d |
| 0x7c | 0x4e | 0x7d | 0x4f | 0x7e | 0x50 | 0x80 | 0x51 | 0x81 | 0x52 | 0x82 | 0x54 |
| 0x84 | 0x42 | 0x85 | 0x42 | 0x86 | 0x43 | 0x87 | 0x43 | 0x88 | 0x55 | 0x89 | below |
| 0x8c | 0x56 | 0x90 | 0x43 | 0x93 | 0x58 | 0x94 | 0x57 | 0x95 | 0x59 | 0x96 | 0x42 |
| 0x97 | 0x42 | 0x98 | 0x61 | 0x9a | 0x63 | 0x9b | 0x62 | 0x9e | 0x1d | 0x9f | 0x41 |
| 0xa0 | 0x47 | 0xa1 | 0x64 | 0xa2 | 0x65 | 0xa3 | 0x66 |  |  |  |  |

Every ability not listed (0, 1, 0x1e, 0x34, 0x36, 0x39, 0x51, 0x52, 0x5e, 0x66, 0x6d, 0x6f,
0x70, 0x72, 0x78, 0x7a, 0x7f, 0x83, 0x8a, 0x8b, 0x8d … 0x8f, 0x91, 0x92, 0x99, 0x9c, 0x9d
and everything above 0xa3) gives state **0** (the game's jump table covers 2 … 0xa3). (Several of these have their own pose
handling outside the state: 0x52, 0x78, 0x7a, 0x9c, 0x9d in §4 and §5.4.)

The three item-dependent abilities ("category" is the item's byte +0x00, "kind" its
subtype +0x04; right item C+0x6d8, left item C+0x638, item in use C+0x1d8):

* **0x32**: if the right item's category is 3 and its kind is 6 or 7: state 0x16.
  Otherwise, if the left item's category is 3: state 9 if the left kind is 0xd, else 0x24.
  Otherwise state 0x23.
* **0x67**: state 0x32 if the left item's category is 0 (no left item), else 0x24.
* **0x89**: if the item in use has category 0x18 and kind 4, 5, 6, 7, 8, 0xa or 0xb: state
  0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f or 0x60 respectively. Otherwise state 0x4e.

Then the lock-timer override of §12.1 step 3.

So the choice depends on the ability, three item checks and the lock timer; it does not
read the weapon class, combo counter, class or timing. Differences seen between a
creature's ability and its state in tick-rate recordings (3 with 0x10, 0x13 with 0x12, 0x3a
with 0) are the frames the creature was not drawn, or the pose's own blow-end reset (§12.6).

### 12.3 For CubeVulkan

Every creature drawn in the world gets A+0x08 = C+0x6c and A+0x0c = state(C) right before
its pose is built, each frame it is drawn. Creatures culled that frame keep their values.
Doing it for every drawn creature is enough; the exact culling only matters for what a
culled creature shows the moment it reappears (one frame of its previous state, then the
ease of §6 toward the new targets).

### 12.4 The visibility test (0xe08c0)

Inputs: the renderer R (0xd3170's object), a point **p** (three i64, dots), a radius **r**
(blocks) and a distance **D** (blocks).

1. With the camera at R+0x338, +0x340, +0x348 (dots): for each axis, dd = (p − camera)²
   as a 64-bit product, then dd div 65536 (rounding toward zero); sum the three. If
   trunc64(D × D × 65536) < that sum, not visible.
2. Otherwise, for each of six planes (a, b, c, d) at R+0x43c (16 bytes each, in order):
   with x = float(p.x + R+0x428) × DOT, y = float(p.y + R+0x430) × DOT, z = float(p.z) × DOT,
   if ((y × b + x × a) + z × c) + d + r < 0, not visible.
3. Otherwise visible.

For the creature: p = C's drawn position C+0xb10, +0xb18, +0xb20 with z lowered by
trunc64(C+0x990 × 65536) (race 0x12e: z raised by trunc64(C+0x17c × 0.5 × 65536) instead);
r = max(C+0x84, C+0x88) × 0.6 (race 0x12e: C+0x17c × 1.2 + that); D = R+0x4a0, the view
distance.

### 12.5 The copy

0x2f4290 copies a creature into a second record kept in a map by id (made on first use with
the creature constructor 0x4ce80), then copies C+0x998, C+0x99c, A+0x0c and A+0x08 across.
It only carries the values over; it does not make new ones.

### 12.6 Checked against the recordings

Rule: §12.1 steps 1–3 with §12.2, compared with the creature bytes that the pose probe
records at the start of each pose call (so after 0xd3170 has set the fields):

| Recording | Records | State matches | State and clock both bit-exact |
|---|---|---|---|
| `Extract/oracle/anim3-*.bin` (berserker, fight2, guardian) | 13,000 | 13,000 | 13,000 |
| `Extract/oracle/cwa2/anim-*.bin` (attack, idle, special, walk) | 5,300 | 5,300 | 5,300 |

The tick-rate `.cwa` frames (`Extract/**/*.cwa`, 600,865 frames in 2,240 files, ZoneExtractor
`writeFrame`) record the state and clock at the game's update, not at the draw, and carry no
items or lock timer. Taking the item-dependent abilities as "one of the listed states" and
allowing 0x3d:

* 591,066 frames: the state is state(C) for the frame's own ability;
* 8,059 frames: state 0 with clock 0, the pose's own blow-end reset (§5.4 step 2);
* 1,063 frames: the clock is stale (≠ the frame's ability clock) and the state is state(C)
  of the ability the creature had when its ability clock had that value: it was not drawn
  since;
* 677 frames: stale the same way, but from before the file starts (cannot be checked);
* 0 frames unexplained.

(Scripts: `pairs.py`, `rule.py`, `cwarule.py`, `cwastale2.py` in the session scratchpad
`clean/atk`.)

---

## Open questions

1. **Meanings.** Several creature fields are named here only by what the pose does with
   them: +0x80 bits 1, 10, 22, 24; +0x128 bit 0 (a grip sprite is drawn on the left hand);
   +0x158, +0x15c/+0x168 (two stances), +0x19c, +0x990, +0x994, +0xad8, +0xba8; the meaning
   of A+0x5b8 and who sets it. The pose state numbers are not names: the tables describe
   what each looks like.
2. **Who sets A+0x0c and A+0x08**: answered in §12 (the world draw, 0xd3170). Still open
   there: states for abilities that occur in no recording (most of the table), and the
   creature-copy routine 0x2f4290 (§12.5), whose caller 0x98180 was not traced.
3. **The kept value K4 in state 0x37 (not moving).** The game reuses a register there; the
   spec follows the assembly (Rr.z as consulted). Its only effect is with a left item of
   kind 0x14 (§5.4 step 6). Not covered by a recording that we know of.
4. **Unchecked model reads.** The game reads the size of model #0xe33 (race 0x12e), of the
   hair sprite (head item), of model #0xe21 (right hand) and of model #0xdeb without
   checking that they exist; a missing model would crash the game. An implementation
   should treat a missing model as "draw nothing" and use size 0 where only the size is
   needed.
5. **Brief versus record.** The task describes "33 part matrices"; the record holds 18
   matrix slots (+0x138 … +0x5b7) plus the size factor and the trails, which is what the
   oracle compares. No further matrices are written by this routine.
6. **NaN paths.** Comparisons and minima are specified as the game performs them; whether
   any recorded call carries a NaN (for example 0 / 0 in a timing ratio when a part of the
   ability lasts 0 ms) is not known.
