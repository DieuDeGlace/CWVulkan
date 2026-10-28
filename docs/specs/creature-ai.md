# Creature AI tick (0x13830)

Behavioural specification of Steam Cube World's creature AI, written from the decompiled
executable, its assembly and the binary's constants. Routines are cited by RVA (image base
0x140000000). Offsets are byte offsets into the record named. Times are milliseconds,
distances are blocks, world positions are 64-bit integers in 1/65536 of a block ("dots").

Vocabulary shared with other specs:

* `creature-upkeep.md` §0 (numeric conventions: single precision, operation order,
  trunc32/trunc64, comparisons as written), §4.2 maximum health 0x5fac0, §4.3 attack-speed
  factor 0x66460, §4.5 "uses mana" 0x5edd0, §4.7 the block lookups 0xeab0/0xe9a0.
* `effect-objects.md` §2.2 ability duration 0x4f220 and its parts: **effect time** (Part A,
  0x4ead0), **middle** (Part B, 0x660d0), **recovery** (Part C, which is 0x5e9f0); §2.4 the
  mana cost 0x5f7c0 and stamina cost 0x65ea0; §2.5 line of sight 0x2b5d90 and the block ray
  0x2d1d20.
* `creature-hit.md` §4.1 aggro notice 0x2c3200, §4.2 distance test 0xf3e20, §5.9 ranged
  attacker 0x5eca0.
* `skills-and-interactions.md` §0 give buff 0x65390 and the hostility test 0x50550; §2.1 the
  interaction record (0xc8 bytes, kind byte at +0xc4).
* `use-prompt.md`: object box size 0x13a960 (base size of the type 0x13a520 × scale).

"Duration" below always means 0x4f220 of the creature named, for its current ability,
compared as a float of the integer. "Block of v" is floor(v / 65536) as an exact integer
floor; "zone of a block" is floor(block / 64); "cell of a block" is floor(block / 16).
"Solid" for a block means its type byte (+4) has a bit other than bit 1 set, i.e. the type
is neither 0 (air) nor 2 (water). A **block** is the six bytes the lookup returns: three
colour bytes, the type at +4, a flags byte at +5.

---

## 0. Threads, clocks and random numbers

* 0x83f20 is the AI thread's loop. While the server's run flag (server+0x2068) is non-zero,
  it reads timeGetTime(), calls 0x13830 with the server's AI state (server+0x768) and *dt* =
  the time now minus the time the previous call started (unsigned 32-bit), then sleeps 40 −
  (time the call took) ms if the call took under 40 ms, else 1 ms. The first *dt* is measured
  from the moment the loop started.
* The AI state is two words: [0] the world, [1] the **AI map**, an ordered map from creature
  id (i64, ascending) to an AI record (§1).
* Every rand() in this document is the CRT's rand() (0..32767) called **on the AI thread**.
  The Microsoft CRT keeps rand()'s state per thread, so this stream is separate from the
  rand() the game's main/world thread uses (see Open questions for its seed).
  "rand % n" is the C remainder of the non-negative value; "pick from a list of *n*" is
  element (rand converted to an unsigned 64-bit value) mod *n*, in list order.
* timeGetTime() is read by 0x83f20 and by the call-out check (§3 step 4).
* The tick takes the world's lock (0x2abb70 / 0x2abb80, released by 0x2b9890 / 0x2b98a0)
  around its phases and calls Sleep(0) in loops. Neither affects results.
* sinf, cosf and sqrtf are the CRT's, as in the other specs.

---

## 1. Records

### 1.1 The AI record (a node of the AI map, 0x178 bytes)

The node is an ordered-map node: +0x00 left, +0x08 parent, +0x10 right, +0x18 colour byte,
+0x19 "is sentinel" byte, +0x20 the key (creature id, i64), the value from +0x28. Offsets
below are **node offsets**.

When a creature's id is first looked up with "find or make" (0x9440), a node is made (0x7470
with 0xa2e0) and the value initialised by 0x96e0 as shown in the "Initial" column. Bytes not
named there are left as the allocator gave them (see Open questions).

| Offset | Type | Initial | Meaning |
|---|---|---|---|
| +0x28 | i32 | 0 | idle timer: ms until the idle behaviour picks again (§4.7) |
| +0x2c | i32 | 0 | chase timer: ms spent not in reach of the target (§4.3, §4.4) |
| +0x30 | i32 | 0 | give-up cooldown: while > 0 the creature does not look for new foes (§4.4) |
| +0x34 | u8 | 0 | call-out: "a player is in sight now" |
| +0x35 | u8 | 0 | call-out: "was asleep" (copy of creature +0x128 bit 1) |
| +0x38 | u32 | 0 | call-out: timeGetTime() of the last call |
| +0x40 | i64 | 0 | focus id: a creature this one is told to fight (never written by the tick) |
| +0x48 | i32 | 10000 | special-skill countdown (ms) |
| +0x4c..+0x54 | f32 ×3 | 0 | **direction**: the movement intent handed to the creature (+0x40) |
| +0x58..+0x60 | f32 ×3 | 0 | copy of the creature's aim (+0x174..+0x17c) |
| +0x64, +0x68 | i32 ×2 | −1, −1 | not used by the tick |
| +0x6c | u8 | 0 | "goal 2 blocks ahead of the target" (§5.2) |
| +0x6d | u8 | 0 | "moving": set when the straight move has length² > 0.001 (§2.2) |
| +0x6e | u8 | 0 | "jump": a drop or step is next on the path (§2.2) |
| +0x6f | u8 | 0 | "glider": the creature has a buff of type 0x13 (refreshed each turn) |
| +0x70 | i32 | 50 | search budget: block expansions allowed per tick |
| +0x74 | f32 | 0 | follow speed: the target's intent speed to match, or 0 |
| +0x78 / +0x80 | list / count | empty | "waiting" interactions (0xc8-byte records) |
| +0x88 | u32 | 0 | copy of creature +0x5c (movement flags) |
| +0x8c | u32 | 0 | copy of creature +0x128 (state flags, zero-extended) |
| +0x90..+0xa0 | i64 ×3 | 0 | copy of the creature's position |
| +0xa8..+0xb0 | f32 ×3 | 1.0 ×3 | copy of the creature's size (+0x84..+0x8c) |
| +0xb8 | pointer | null | **best**: the step-map node with the smallest *h*, or the map's sentinel ("none") |
| +0xc0 / +0xc8 | map / count | empty | **steps**: block (3 × i32) → step (§1.2) |
| +0xd0 / +0xd8 | set / count | empty | **open**: blocks still to expand |
| +0xe0..+0xe8 | i32 ×3 | 0 | the search's start block |
| +0xf0..+0x100 | i64 ×3 | — | search **start** position (feet) |
| +0x108..+0x118 | i64 ×3 | — | search **goal** position |
| +0x120 | f32 | — | **radius**: how close counts as arrived |
| +0x128 | i64 | 0 | **target id** the goal follows (0: a fixed place) |
| +0x130 | i32 | — | set to 0 by a search start; not otherwise used |
| +0x138 / +0x140 | list / count | empty | **path**: blocks from the start toward the goal |
| +0x148 / +0x150 | set / count | empty | **taken**: object addresses (u64) |
| +0x158 / +0x160 | list / count | empty | 16-byte entries; never touched by the tick |
| +0x168 | 3 × i32 | — | object reference: zone x, zone y (as one i64 key: x low, y high) |
| +0x170 | i32 | — | object index in that zone; < 0 = none |

Blocks in the step map, the open set and the path are ordered lexicographically by (x, y,
z) as signed 32-bit integers.

"**Forget**" (0xba20) empties steps, open and path, sets best to none, and empties taken.

### 1.2 A step (value of the step map)

g (i32), h (i32), f (i32), from (3 × i32). The map node holds the key at node+0x1c..+0x27
and the step at node+0x28..+0x3f.

**Heuristic** H(a, goal) between two blocks: dx = |a.x − goal.x|, dy = |a.y − goal.y|, dz =
|a.z − goal.z|; flat = dx×10 + dy×4 when dy < dx, else dx×4 + dy×10; H = (flat + dz×10) × 2.

### 1.3 Creature fields used

R read, W written. The record is 0x15f8 bytes; the tick reads only the first 0xc10.

| Offset | Type | R/W | Use |
|---|---|---|---|
| +0x08 | i64 | R | id |
| +0x10..+0x20 | i64 ×3 | R W | position |
| +0x30 | f32 | R | heading, degrees |
| +0x34..+0x3c | f32 ×3 | R W | velocity |
| +0x40..+0x48 | f32 ×3 | R W | movement intent |
| +0x5c | u32 | R | movement flags: bit 0 on ground, bit 1 in water, 0x4000 opens doors |
| +0x60 | u8 | R | hostility kind (0 player, 1 monster, 3 townsperson, 6 pet, 7/8 object creatures) |
| +0x64 | i32 | R | race |
| +0x68 | u8 | R W | ability |
| +0x6c | f32 | R W | ability time |
| +0x70 | i32 | R W | combo counter |
| +0x80 | u32 | R | creature flags (bits 2, 1|4, 9, 11, 13, 15, 18, 22 used) |
| +0x84..+0x8c | f32 ×3 | R | size |
| +0x90, +0x94, +0x98, +0x9a | u16 | R | appearance ids (races 0x76, 0x77) |
| +0x128 | u16 | R W | state flags (bits 0, 1, 4, 5, 6, 7, 9, 12 used) |
| +0x12c | f32 | R W | dodge timer |
| +0x130 | f32 | R | stun |
| +0x14c, +0x14d | u8 ×2 | R | class, specialisation |
| +0x158 | f32 | R | charge level |
| +0x174..+0x17c | f32 ×3 | R W | aim (offset to what it looks at, blocks) |
| +0x180 | f32 | R W | health (also of the owner, §4.3) |
| +0x188 | f32 | R W | mana |
| +0x18c | f32 | R | stealth |
| +0x1a0 | i64 | R | owner id |
| +0x1a8 | i64 | R W | linked creature id |
| +0x1b8..+0x1c8 | i64 ×3 | R | home position |
| +0x1d8 | 0xa0 | R W | the item in use (type +0x1d8, subtype +0x1dc, modifier +0x1e0, …) |
| +0x638 / +0x63c | u8 / i32 | R | equipment slot A: item type, subtype |
| +0x6d8 / +0x6dc | u8 / i32 | R | equipment slot B: item type, subtype |
| +0x980 | list | R W | buffs (entry at node+0x10: type +0, strength +4, duration +8, source +0x10) |
| +0x9a0 | f32 | R | stamina |
| +0x9d8 | i64 | R | mount id |
| +0x9e0 | i64 | W | id of the creature whose call it answers |
| +0x9e8 | f32 | W | travel time for skill 0x37 |
| +0x9f0 | vector of tabs | R W | inventory: each tab 0x18 bytes (begin, end, capacity) of 0xa4-byte stacks (count i32 +0, item +4) |
| +0xab8 / +0xac0 | list / count | W | pending interactions |
| +0xae0..+0xaf0 | i64 ×3 | R W | remembered aim point (staff users) |
| +0xb10..+0xb20 | i64 ×3 | R | a second position (of the target) |
| +0xb88 | map i64→f32 | R W | aggro |
| +0xb98 | map i64→f32 | R | damage taken from each id |
| +0xbcc | u32 | R | **AI flags** (§3.2) |
| +0xbd0..+0xbd8 | vector of i32 | R | special skills it may use |
| +0xc08 | set of i64 | R | its group (ids, ascending) |

### 1.4 World fields

| Offset | Use |
|---|---|
| +0x0c | f32 time of day (ms) |
| +0x40 | list of creature pointers ("world order"; pointer at node+0x10) |
| +0x140 | aggro notices (0x18 bytes: i64, i64, f32, 4 padding bytes) — written by 0x2c3200 |
| +0x150 | buff-change notices (0x30 bytes; `creature-upkeep.md` §2.4) |
| +0x170 | list of i64 ids: "calls" (townsfolk calling out, §3 step 4) |
| +0x358 | u8 remote-client flag |
| +0x448 | the local player's record |
| +0x450 | ordered map id → creature record ("look up by id"; absent or null value = none) |
| +0x460.. | zone hash map keyed by (zone x low 32 bits, zone y high 32 bits); value node+0x18 = zone object or null |

A zone object holds its objects at zone+0x60..+0x68 (a vector, 0xe0 bytes each). Object
fields used: type i32 +0, position i64 ×3 at +8, flags byte +0x20 (bit 0: swap x and y
size), scale +0x24, "shut" byte +0x28, user i64 +0x30 (0 = free). **Object by reference**
(0x2c3150): the zone found by the 8-byte key, non-null, and 0 ≤ index < object count;
otherwise none.

### 1.5 Records the tick produces

* **Buff notice** via 0x29d2f0 (creature, buff, refresh flag): give the buff (0x65390 with the
  flag), then append a buff-change notice with source = target = the creature's id, a copy of
  the 0x18-byte buff, flag byte = the refresh flag.
* **Aggro notices** via 0x2c3200.
* **Call**: the creature's id appended to world+0x170 (0x12a10).
* **Object interaction** (kind 5): an interaction record initialised as by 0x9660 (item all
  zero except as below; +0xa0 i64 −1, +0xa8 i32 −1, +0xb0..+0xc3 zero, +0xc6 u16 1) with the
  zone key at +0xa0 (x at +0xa0, y at +0xa4), the object index at +0xa8 and kind 5 at +0xc4.
  Bytes 0x15–0x17, 0xac–0xaf and 0xc5 are not set (stack contents). Appended to the back of
  creature+0xab8.

---

## 2. The tick, 0x13830 (AI state, dt)

W is the world. "Runs here" for a creature C means: W+0x358 is 0, or C is W+0x448.

### 2.1 Phase A: the turns

For each creature C in world order:

1. Skip unless C runs here.
2. If C's kind is 0: its position is appended to a local list that is never used. Then, if
   C's +0x128 bit 0x200 is clear, skip C.
3. Skip unless C's health > 0 and C's stun ≤ 0.
4. Find or make C's AI record E (§1.1). Copy into E: position (+0x90..), size (+0xa8..),
   +0x5c into +0x88, +0x128 into +0x8c, aim (+0x174..+0x17c) into +0x58..+0x60. Empty E's waiting list.
   E+0x6f = 1 if C has a buff of type 0x13, else 0.
5. C's turn: 0x14e00 (§3) with *dt*.
6. Copy C's aim into E+0x58.. again.

### 2.2 Phase B: moving each record

For each AI record E in ascending id order:

1. E+0x6d = 0, E+0x6e = 0.
2. If E+0x8c bit 1 (value 2) is set: forget, E+0x128 = 0, E+0x74 = 0. Next record.
3. If 500.0 < float(E+0x70): E+0x70 = 500.
4. If E's open set is empty: next record.
5. **Where to go.** G = E's goal (+0x108..). Look up E+0x128 by id. If a creature T is found:
   G = T's position; if E+0x6c is set, G = the **ground ahead** of T (§5.3, from G); then
   **settle on the ground**: repeat { if the box of T's size (+0x84..) centred at (G.x, G.y,
   G.z − 32768) touches (§5.10) stop; else G.z −= 65536 } (no limit); then G.z −=
   trunc64((T+0x8c × 0.5 − 0.1) × 65536).
6. Aim point A = (G.x, G.y, trunc64(E+0xb0 × 0.5 × 65536) + G.z).
7. If **not** walkable(E size, E position, A, radius E+0x120, glider E+0x6f, flags E+0x8c)
   (§5.8) — **search**:
   1. budget = E+0x70, or 2000 if E+0x8c has bit 0x200.
   2. Start (+0xf0..) = E position, start.z −= trunc64((E+0xb0 × 0.5 − 0.1) × 65536). Goal
      (+0x108..) = G. Start the search (§5.4).
   3. Repeat *budget* times: if the path is not empty and, with p = the path's **last** block
      and g = block of the goal, float((p.x−g.x)² + (p.y−g.y)² + (p.z−g.z)²) ≤ E+0xa8², stop;
      otherwise expand (§5.6) and rebuild the path (§5.7).
   4. If the path is not empty and E+0xa8² < that same float distance: E+0x70 += E+0x70 / 10
      (integer division).
   5. Direction = (0, 0, 0).
   6. If E+0x8c lacks bit 0x200, or the path has more than 10 blocks: **follow the path**:
      walk candidates from the path's last block back to its first; *left* is the
      candidate's 0-based position from the front (size − 1 for the last block, 0 for the
      first). For each candidate b: target point P = (b.x × 65536 + 32768,
      b.y × 65536 + 32768, b.z × 65536 + trunc64(E+0xb0 × 0.5 × 65536)). The candidate is
      taken when *left* < 2, or walkable(E size, E position, P, 2.0, glider, flags). For the
      first one taken:
      * direction = (P − E position) per axis as float of the i64 difference × 1/65536;
      * if the block at E's position (0xeab0) is not water (type ≠ 2) and E.z < P.z,
        direction.z = 0;
      * if the squared length > 0: k = 1.0 / sqrtf(squared length); each component × k
        (x as x × k, y and z as k × y, k × z);
      * each component × 80.0;
      * if *left* < 4 and the candidate is not the path's last block: let n = the block after
        it on the path. If the block (n.x, n.y, n.z − 1) is not solid: E+0x6e = 1 and each
        component × 0.1. Then if E.z < trunc64(E+0xb0 × 0.5 × 65536) + n.z × 65536:
        direction.z = 20.0.
      * stop.
8. Else (walkable) — **straight**: empty steps, open and path, best = none, empty taken,
   E+0x128 = 0. d = (G − E position) per axis × 1/65536 (float of the i64 difference times
   the constant). speed = 80.0; if 0.01 < E+0x74 and d.x²+d.y²+d.z² < ((E+0x120 × E+0x120) ×
   10.0) × 10.0: speed = E+0x74. flat = d.x² + d.y²; if d.z² + flat < (E+0x120 × E+0x120) ×
   4.0: speed × 0.5. If the block at E's position is not water: d.z = 0. len² = d.z² + flat;
   if > 0: k = 1.0 / sqrtf(len²), d = d × k. direction = d × speed. If 0.001 < squared
   length of the direction: E+0x6d = 1. E+0x70 −= E+0x70 / 4 (C division); if < 50, 50.

### 2.3 Phase C: handing the result to the creatures

For each creature C in world order that runs here, whose kind ≠ 0 **or** +0x128 has bit
0x200, with health > 0 and stun ≤ 0:

1. C+0x128 &= ~0x80.
2. Find C's AI record E (no making). If none, next creature.
3. C's intent (+0x40..) = E's direction.
4. If E+0x6e: C's ability = 0x7a. Else if C's ability is 0x7a: ability = 0.
5. Copy E's waiting interactions to the back of C's list (the waiting list keeps them).
6. If E+0x170 ≥ 0 and the object by reference (E+0x168, E+0x170) exists and the squared
   distance (C position − object position, per axis × 1/65536) < 25.0: append an object
   interaction (§1.5) for that reference; C's intent = 0; E's direction = 0; E+0x168 (8
   bytes) = −1; E+0x170 = −1.

### 2.4 Phase D: dropping records

Every AI record whose id is not in W+0x450 (or maps to null) is removed. On a remote client
(W+0x358 ≠ 0) every record whose creature is not the local player is removed too. Removal
happens after the walk over the map, in ascending id order.

---

## 3. One creature's turn, 0x14e00 (creature C, dt)

### 3.1 Order

1. If C+0x80 bit 22 is set: return.
2. If C's class is 0x99: if duration < C+0x6c, ability = 0x66 and +0x6c = 0. Return.
3. **Riding a mount.** Unless C's kind is 7 or 8, or owner id is 0: look up the owner O.
   If O exists, O's health > 0, O's stun is not > 0, and O's race is 0x106: C's ability =
   0x77, C+0x1a8 = O's id. Return.
4. **Calling out** (townsfolk): only if C+0x80 bit 13 is set and C's kind is 3.
   1. now = timeGetTime(). asleep = C+0x128 bit 1. E = find-or-make C's record.
   2. If not asleep and E+0x35 ≠ 0: E+0x34 = 0, E+0x38 = 0.
   3. saw = E+0x34; E+0x34 = 0.
   4. For each creature P in world order with kind 0: if in sight (§5.11, range 20.0) from C,
      E+0x34 = 1 and stop looking.
   5. If E+0x34 and not saw and (now − E+0x38, unsigned) > 4000: append C's id to the calls
      list, E+0x38 = now.
   6. E+0x35 = asleep.
5. If C+0x128 bit 1 is set: return.
6. **Behaviours.** If C+0xbcc bit 0x400 is clear, or ride-along (§4.1) returns false:
   * If C's kind is 7 or 8: if C+0xbcc has 0x200, attach (§4.2).
   * Otherwise:
     1. If C+0xbcc has 0x4000 and guard (§4.3) returns true: **return** (steps 7 and 8 are
        skipped).
     2. If fight (§4.4) returns false:
        * if C+0xbcc bit 0x80 is clear: greet (§4.5); if false, follow the group (§4.6); if
          false, idle (§4.7);
        * else follow the owner (§4.6).
7. **Doors** (§4.8): if C's intent has squared length > 0 and C+0x5c has bit 0x4000.
8. **Night flag:** clear C+0x128 bit 0 if C's ability is 0x58, or 25 200 000 ≤ time of day <
   72 000 000, or the race category of C's race (0x65750) is not 3. Otherwise set it.

Race category 3 (0x65750): races 0x00–0x10, 0x12, 0x2b–0x2e, 0x30, 0x31, 0x33, 0x34, 0x36,
0x4c–0x51, 0x53, 0x54, 0x5f, 0x94, 0x95, 0xa7, 0xbf, 0xc7, 0xca, 0xcb, 0xd0, 0xd8, 0xe3,
0xe4, 0xe6, 0xec, 0xed, 0xfc–0xfe, 0x100, 0x103–0x105, 0x128, 0x130, 0x131.

### 3.2 AI flags (+0xbcc)

| Bit | Meaning here |
|---|---|
| 0x2 | stays home: sits (ability 0x58), returns home from 4 blocks |
| 0x4 | idle option "stand" |
| 0x8 | idle option "sleep" (ability 0x57) |
| 0x10 | idle option "stroll" |
| 0x20 | idle option "sit at an object" |
| 0x80 | companion: follows its owner instead of group/idle |
| 0x100 | idle option "dance" (ability 0x82) |
| 0x200 | (kinds 7, 8) attach to a nearby monster |
| 0x400 | ride along with the owner |
| 0x1000 | idle option "walk to a spot in a nearby cell" |
| 0x4000 | guard and heal the owner |

---

## 4. The behaviours

### 4.1 Ride along, 0x11df0 → bool

O = owner by id. If none: false. Otherwise:

1. Copy from O into C: +0x128, ability, +0x6c, mana (+0x188), +0x158, and the item in use
   (+0x1d8, field by field as 0x9cd0 does: bytes +0, +4..+0x13, +0x14, +0x18..+0x1b, +0x1c,
   +0x1d..+0x9c, +0x9d; padding bytes are not copied).
2. If O's dodge timer > 0 and C's ≤ 0: C's dodge timer and velocity = O's.
3. C's aim = O's aim, then recomputed: aim.x = float((O.x + trunc64(O.aim.x × 65536)) − C.x) ×
   1/65536; then aim.z the same with z; then aim.y with y (that order).
4. Proximity flag: with each axis difference d = O − C as i64, s = the 64-bit product d×d
   (wrapping), q = s / 65536 truncated toward zero; if q.x + q.y + q.z < 0x400001, clear
   C+0x128 bit 6 (0x40), else set it.
5. **Pet healing** (kind 6 only): h = ((maxHealth × float(dt)) × 0.001) × 0.1 + health;
   health = h; if maxHealth < h, health = maxHealth (maxHealth recomputed at each use).
6. d = (O − C) in blocks; E = find-or-make C's record; s = d.x² + d.y² + d.z².
7. If 25.0 ≤ s:
   * if C's kind ≠ 6, or s ≤ 10000.0, or O+0x5c has neither bit 0 nor bit 1, or C's ability
     is 0x79: follow O (§5.2) with radius 5.0, ahead off, match speed on. Return **true**.
   * else C's position = O's position. Return false.
8. Else: E's direction = 0, C's intent = 0. Return false.

### 4.2 Attach to a monster, 0x13530 (kinds 7 and 8 with flag 0x200)

1. C's ability = 0.
2. If the creature C+0x1a8 exists with health > 0: ability = 0x91. Done.
3. nearest = 25.0. For each creature M in world order with kind 1: with dx the i64
   difference M.x − C.x and k = 1/65536, s = ((float(dx) × k) × float(dx)) × k + dy² + dz²,
   dy and dz being (M − C) × k in float. If s < nearest:
   * C+0x1a8 = M's id, ability = 0x91;
   * **rand a**, **rand b**, **rand c** (in that order; a is drawn before reading the
     sizes);
   * aim.y = (1.0 − (b + b) / 32767.0) × M+0x88 × 0.5;
   * aim.z = (1.0 − (a + a) / 32767.0) × (M+0x8c − C+0x8c) × 0.5 + C+0x8c × 0.5;
   * aim.x = (1.0 − (c + c) / 32767.0) × M+0x84 × 0.5;
   * nearest = s.
   (Here "(a + a)" is float(a) + float(a). Ability 0x91 with +0x1a8 and the aim as offset is
   "being carried", `creature-upkeep.md` step 3.)

### 4.3 Guard the owner, 0xb330 → bool

1. If C+0x128 has bit 0x02 or 0x10: false.
2. E = find-or-make. O = owner by id; if none or O's health ≤ 0: false.
3. sees = line of sight 0x2b5d90 from C's position to O's, flag 1, limit 200.0.
   reach = 30.0 if sees, else 1.0.
4. If reach² < |C − O|² (blocks) or not sees:
   * if C's stun ≤ 0: E+0x2c += dt;
   * if E+0x2c < 15001 or C's race is 0x114: follow O (§5.2) with radius *reach*, ahead off,
     match speed off; return (C's ability is 0x1d and C+0x6c < duration);
   * else: clear C's aggro map, forget, E+0x128 = 0, E+0x30 = 3000, E+0x2c = 0. Return false.
5. Close enough and seen:
   * If C's ability ≠ 0x1d: if duration ≤ C+0x6c, then ability = 0x1d, C+0x1a8 = O's id,
     +0x6c = 0. If the ability is still not 0x1d: false.
   * C's aim = (O − C) in blocks.
   * If O has a buff with source = C's id, type 0x20 and duration > 500.0: return (ability
     is 0x1d and +0x6c < duration).
   * before = maxHealth(O). Give O (0x65390, refresh 1) the buff {type 0x20, strength 1.1,
     duration 3000.0, source C's id} and append a buff notice with source C's id, target O's
     id, the buff, flag 1. after = maxHealth(O). O's health = (after / before) × health.
     Return true. (The buff's bytes 1–3 and 0xc–0xf are not set; see Open questions.)

### 4.4 Fight, 0xbac0 → bool

Let E be C's record (made at step 6 if missing). T is the target, "none" until chosen.

1. If C+0x128 has bit 0x02 or 0x10: return false. Clear C+0x128 bit 12 (0x1000).
2. If C's ability is not one of 0x50, 0x56–0x59, 0x7e, 0x8a and C+0x6c > 10000.0: ability =
   0.
3. Return true if: the dodge timer ≠ 0; or ability 0x54 and +0x6c < 3000.0; or ability 0x55
   and +0x6c < 5000.0.
4. If ability is 0x82: if +0x6c < duration, intent = 0 and return true; else ability = 0.
5. **Potion.** If health < maxHealth × 0.25 and duration < +0x6c: scan the inventory tabs in
   order and each tab's stacks in order for the first stack with count ≠ 0, item type 1 and
   subtype 1. If found: ability = 0x54, +0x6c = 0, copy the stack's item into C+0x1d8 (as in
   §4.1 step 1); count −= 1; if count < 1: count = 0, item type = 0, subtype = 0.
   **rand a**, **rand b**; intent.x = (b × 4.0) / 32767.0 − 2.0, intent.y = (a × 4.0) /
   32767.0 − 2.0, intent.z = 0. Return true.
6. **Focus.** F = E+0x40. If F ≠ 0, F exists, C's health > 0 and C's aggro[F] (made as 0 if
   absent) < 0.1: aggro[F] = 0.1.
7. If C's race is 0x114: remove from C's aggro every id whose creature exists (non-null) with
   kind ≠ 0.
8. Remove from C's aggro every id ≠ F whose creature exists with health ≤ 0 or +0x128 bit 1.
   (Ids of absent creatures stay.)
9. **Preferred target** (0x5f920): the source id of C's first buff of type 0x12, if any;
   otherwise the aggro entry with the largest value among ids ≠ 0, ties going to the later
   (higher) id ("value ≥ best" replaces), starting from best = 0.0 and id 0. T = that id
   looked up.
10. If T is none and C's race is 0x114 and |home − C|² > 900.0: C's position = home.
11. **Pets** (kind 6): O = owner. If O exists: T = none; if O's ability is 0x76 return false;
    otherwise walk O's damage map (+0xb98) in ascending id order with best = 0.0: when best <
    value and that creature exists with health > 0, T = it and best = value.
12. **Cower.** If T is none, and health < maxHealth × 0.5, and C's kind is 3, and C+0x80 bit
    11 is clear: ability = 0x55, +0x6c = 0, item type +0x1d8 = 1, +0x1dc = 4, intent = 0.
    Return true.
13. E+0x48 −= dt; if < 0, 0. frozen = C has a buff of type 8.
14. **Looking for a foe.** If T is none, and (C's kind ≠ 6 or owner id = 0), and E+0x30 < 1:
    for each creature X in world order, the last X passing all of the following becomes T:
    X ≠ C; X's health > 0; X+0x128 bit 1 clear; X's kind ≠ 6; C's race ≠ 0x114 or X's kind is
    0; F = 0 or X's id = F; not frozen; hostile(C, X) (§5.12) or (C's owner id = 0 and X
    **calls** C); in sight (§5.11, range 20.0); and C+0x80 bit 15 clear or X calls C.
    "X calls C" = X's ability is 0x56, X's item type (+0x1d8) is 0x15 and X's +0x1dc equals
    C's race. If T was found: if aggro[T] (made as 0) < 0.1, set it to 0.1 and send the
    aggro notice 0x2c3200(world, T, C).
15. **Hurry flag.** If T is none: clear C+0x128 bit 6. Otherwise: if C's race is 0x114 set
    C+0x128 bit 12; then if C's ability ≠ 0 and +0x6c ≤ float(middle + recovery / 2 + effect
    time) (integer sum; recovery 0x5e9f0, effect 0x4ead0, middle 0x660d0, each with −1, in
    that call order), clear bit 6; else set bit 6.
16. E+0x30 −= dt; if < 0, 0.
17. If T is none: E+0x2c = 0; **rand**: E+0x48 = rand % 10000. Return false.
18. If C's ability is 0x57 or 0x58: ability = 0, +0x6c = float(duration).
19. d = (T − C) in blocks; reach = C+0x84 + C+0x84 + T+0x84. sees = 0x2b5d90(C position,
    T position, flag 1, limit 200.0).
20. If T's ability is 0x8c and T+0x6c < T's duration: C's aggro[T] = 0. Return false.
21. **Range** R = reach. Then in order:
    * if C+0x128 bit 12 (0x5f0d0): sees = true, R = 0; if duration < +0x6c: ability =
      0x8b, +0x6c = 0;
    * if C is a ranged attacker (0x5eca0) and sees: unless (ability is 0x5f, 0x60 or 0x62
      and +0x6c < effect time), R = 50.0;
    * if ability is 0x69, 0x1d, 0x2f or 0x7b and +0x6c < duration: R = 50.0;
    * ability 0x88: R = 4.0; ability 0x49: R = T+0x84 + C+0x84;
    * if the creature C+0x9d8 exists, has race 0x106 and health > 0 ("mounted"): R = 4.0;
    * if T's stealth > 0: k = 1 − stealth, but 0.1 if 0.1 > k; v = (k × k) × R; R = reach,
      or v if reach ≤ v.
22. **Special skill.** S = 0. If E+0x48 = 0 and C's skill list is not empty: **rand**: S =
    skills[pick]; **rand**: if even: if S is 0x45, 0x46, 0x64 or 0x67, R = 20.0; if S is 0x80
    and C is in water (+0x5c bit 1), S = 0; else if S is 0x5f or 0x62, R = reach (recomputed
    the same way). If odd: S = 0, E+0x48 = 10000.
23. If not sees: R = 1.0.
24. s = d.x² + d.y² + d.z².
25. **Out of reach** — unless C's ability is 0x37, or (s ≤ R² and sees and C+0x5c has bit 0
    or 1):
    * if C's stun ≤ 0: E+0x2c += dt;
    * if E+0x2c > 15000 and C's race ≠ 0x114: clear aggro, forget, E+0x128 = 0, E+0x30 =
      3000, E+0x2c = 0; return false;
    * else follow T (§5.2) with radius R, ahead off, match speed off. Return true.
26. **In reach**: E+0x2c = 0; forget; E+0x128 = 0; E+0x74 = 0; E's direction = 0.
27. **Answering a call.** If none of these holds: frozen; C+0x80 has bit 9 or bit 18
    (0x40200); T does not call C; C's kind is 6; C has an owner and C+0x80 bit 11 is clear;
    R² < s — then:
    * C+0x9e0 = T's id. If duration < +0x6c: if ability ≠ 0x7e, combo (+0x70) = 0; if combo
      < 5: ability = 0x7e, +0x6c = 0; else: if C has buffs of type 7, take the **last** one
      in list order, set its strength and duration to 0 and give it back with a buff notice
      (refresh 1); then give C (with notice, refresh 1) the buff {type 8, strength 0,
      duration 20000.0, source T's id}.
    * Return true.
28. If not sees: if ability is 0x69 or 0x1d, ability = 0. Return true.
29. d and s are recomputed the same way. Then the first that applies:
    1. mounted: ability = 0x76, +0x6c = 0;
    2. ability 0x37: if duration < +0x6c and C+0x5c has bit 0 or 1, ability = 0;
    3. S ≠ 0 (a special skill; the game also prints "SUPER SKILL S" to stdout): if S ≠ 0x82
       or C is on ground (+0x5c bit 0): E+0x48 = 10000, ability = S, +0x6c = 0; for S = 0x37:
       velocity = (q.x, q.y, q.z) × 1/65536 with q = ((T − C) × 131072 as i64) / 65536
       truncated toward zero (twice the offset); +0x9e8 = (sqrtf(v.x² + v.y² + v.z²) / 50.0)
       × 1000.0; velocity.z = 20.0;
    4. C+0x128 bit 12 set: nothing;
    5. otherwise the **attack** (§4.4.1), which may change d.
30. C's aim = d. Return true.

#### 4.4.1 The attack (S = 0, not mounted, ability ≠ 0x37)

The first branch that matches C:

* **Race 0x74:** if duration < +0x6c: **rand**: ability = pick of (0x46, 0x4b, 0x4a), +0x6c =
  0.
* **Race 0x76:** if duration < +0x6c: **rand**: ability = pick of (8, 7, 0x15, X) with X =
  0x69 when C+0x90 = 0xb39, else 0x5f; +0x6c = 0. Then if ability is 0x69: d = T's position
  − the shot point of C (§5.14), in blocks.
* **Race 0x77:** if duration < +0x6c, build a list in this order: 70 if (C+0x90 − 0xb32) as
  u16 < 3, else 69; 71 if (C+0x9a − 0xb36) < 2; 76 if (C+0x94 − 0xb2e) < 2, else 75; 78 and
  79 if (C+0x98 − 0xb2b) < 2; 77 if (C+0x98 − 0xb26) < 5; if ability is not 0x48/0x49 and
  C+0x98 = 0xb24: **rand**, 72 if odd; if ability is not 0x48/0x49 and C+0x98 = 0xb25:
  **rand**, 73 if odd; 74 if ability ≠ 0x4a. If the list is not empty: **rand**: ability =
  pick, +0x6c = 0.
* **Mana users** (0x5edd0): only if s < 3600.0. If mana ≤ 0.5: start a hit (§5.15).
  Otherwise **rand**: even → start a hit, odd → start a charge (§5.15). Then:
  * if ability is 0x1f–0x22: if float(effect time) < +0x6c − float(dt as i32): d = (C+0xae0..
    − C position) in blocks; else C+0xae0.. = T+0xb10...
  * if ability is 0x1d and 25.0 < s: a = +0x6c × 0.005; d.x += cosf(a) × 1.5; d.y +=
    sinf(a) × 1.5.
* **Race 0x65:** if 64.0 ≤ s: if s < 3600.0 and duration < +0x6c: +0x6c = 0, ability =
  0x26. Else (s < 64): if +0x6c > 2000.0: **rand**: if even, or (C.z − T.z) > 524288: **leap**;
  else velocity.z = 30.0. (This branch also writes an aim that step 30 overwrites.)
* **Race 0x123:** start a hit.
* **Leapers** (C+0x80 has both bits 0x02 and 0x10): if s < 64.0 and +0x6c > 2000.0: **rand**:
  if even, or (C.z − T.z) > 131072: leap; else velocity.z = 5.0.
* **Bows** (class 0x95 or 0x96, or slot-B subtype 6, 7 or 8): only if s < 3600.0.
  * Dodge if all hold, tested in this order: (ability not 0x19–0x1c, or ability 0x1b); dodge
    timer = 0; **rand** % 10 = 0; C+0x80 bit 18 clear; C+0x84 < 2.0; duration ≤ +0x6c;
    C+0x5c bit 0 or 1. Then **dodge** and stop.
  * Else if 0.8 ≤ mana (compared in double) or the ability is 0x19, 0x1a or 0x1c: if the
    dodge timer = 0 and duration < +0x6c: m = charge move (§5.15) with force off:
    * m = 0x19: if ability is 0x19 and min(mana × 0.9, 0.8) ≤ +0x158: ability 0x17, +0x6c
      = 0; otherwise (if ability ≠ 0x19, +0x6c = 0 first) ability = 0x19;
    * m = 0x1c: the same with release 0x1b and hold 0x1c;
    * m = 0x1a: the same with release 0x38 and hold 0x1a;
    * other: start a charge.
  * Else start a hit.
* **Melee** (everything else): only if s < ((T+0x84 + C+0x84) × 3.0)².
  * Dodge if all hold, in order: ability not 9, 0x3c, 0x40 or 0x41; dodge timer = 0; **rand**
    % 10 = 0; C+0x80 bit 18 clear; C+0x84 < 2.0; duration ≤ +0x6c; C+0x5c bit 0 or 1.
  * Else if 0.8 ≤ mana or the ability is 9, 0x19, 0x1a, 0x1c, 0x3c or 0x40: if dodge timer =
    0 and duration < +0x6c: m = charge move, force off. "Charged" = mana ≤ +0x158.
    "Release X" = ability X and +0x6c = float(effect time of X).
    | m | If the ability is already m | Otherwise |
    |---|---|---|
    | 6 | ability 6, +0x6c = 0 | same |
    | 0x3c | charged: spec ≠ 1 → release 0x3d; spec 1 → ability 0x3e, +0x6c = 0. Not charged: stays 0x3c | +0x6c = 0, ability 0x3c |
    | 9 | charged: ability 0x74, +0x6c = 0; else stays 9 | +0x6c = 0, ability 9 |
    | 0x41 | charged: release 0x11; else stays | +0x6c = 0, ability 0x41 |
    | 0x40 | charged: release 0x0c; else nothing | +0x6c = 0, ability 0x40 |
    | 0x1a | charged: release 0x38; else nothing | +0x6c = 0, ability 0x1a |
    | 0x19 | charged: release 0x17; else nothing | +0x6c = 0, ability 0x19 |
    | 0x1c | charged: release 0x1b; else nothing | +0x6c = 0, ability **0x1b** |
    | other | start a charge | start a charge |
  * Else start a hit.

**Leap:** +0x6c = 0, ability 0x35, velocity = (T − C) in blocks; if its squared length > 0:
normalise (§5.13) and × 10.0.

**Dodge:** v = ((T − C).x, (T − C).y, 0) in blocks; only if v.x² + v.y² + 0 > 0: normalise
v; **rand** r = rand mod 4 (rand & 3): r 0: velocity = v × 20.0; r 1: v × −20.0; r 2:
velocity = ((v.y − v.z × 0) × 20, (v.z × 0 − v.x) × 20, (v.x × 0 − v.y × 0) × 20); r 3: the
same with −20.0. Then velocity.z += 5.0 and dodge timer = 600.0. (No dodge and no rand when
v is zero.)

### 4.5 Greet a player, 0x12400 → bool

1. If C+0x128 has bit 0x02 or 0x10: false.
2. P = the nearest creature in world order with kind 0, health > 0, in sight (range 20.0),
   with squared distance (blocks) < the best so far, starting from 100.0 (strictly less
   replaces).
3. If no P: false. If C's ability is 0x57 or 0x58: false. If P's ability is 0x8c, P+0x6c <
   P's duration and P+0x1a8 = C's id: false.
4. If duration < C+0x6c: E = find-or-make; if C's kind is 3 and ability is neither 0xa1 nor
   1, **rand**, and rand % 3 = 0: ability = 0xa1; otherwise ability = 1 (no rand when the
   first conditions fail). +0x6c = 0; aim = (P − C) in blocks; E's direction = 0; forget;
   E+0x128 = 0; E+0x74 = 0.
5. If the ability is 0xa1: aim = (P − C) again. Return true.

### 4.6 Follow the group leader (0x103f0) or the owner (0xdff0) → bool

Leader L: for 0x103f0, walk C's group (+0xc08) in ascending order: if the id is C's own,
return false; the first id whose creature exists with health > 0 is L; none → false. For
0xdff0, L = owner by id (no health check); none → false.

| | far² | near² | radius | ahead |
|---|---|---|---|---|
| group 0x103f0 | 256.0 | 64.0 | 8.0 | off |
| owner 0xdff0 | 64.0 | 16.0 | 4.0 | on when C's kind is 6 |

1. **Catch up:** if L's ability is 0x9b, L+0x6c < 500.0, L+0x6c < 1000.0 and |L − C|² >
   10000.0: C's position = L's.
2. Pet healing (kind 6) as §4.1 step 5.
3. If L's ability is 0x50: C's ability = 0x50; else if C's is 0x50: 0.
4. If L has a buff of type 3 and C has none: give C L's first type-3 buff with a buff notice
   (0x29d2f0, refresh 1).
5. Spot Q = the ground ahead of L (§5.3) if found, else L's position.
6. If |Q − C|² > far² or L+0x128 has bit 6: set C+0x128 bit 6; else clear it.
7. If near² ≤ |Q − C|²: follow L (§5.2) with the radius, the ahead flag, match speed on.
   Return true.
8. Else E's direction = 0, forget, E+0x128 = 0, E+0x74 = 0. Return false.

### 4.7 Idle, 0x10b90 → bool

1. If C+0x128 has bit 0x02 or 0x10: false. Clear C+0x128 bit 6. E = find-or-make. E+0x28 −=
   dt; if < 0, 0.
2. **Edge guard:** if C+0x8c < 6.0, class ≠ 0xa0 and C is on ground (+0x5c bit 0): z = C.z −
   trunc64(C+0x8c × 0.5 × 65536); if the blocks at (C.x, C.y, z), (…, z − 65536) and (…, z −
   131072) are all not solid: h = (home − C) in blocks; flat = h.x² + h.y²; if h.z² + flat >
   0 and flat + 0.0 > 0.01: k = 1.0 / sqrtf(flat + 0.0); direction = (h.x × k × 10.0, h.y ×
   k × 10.0, k × 0.0 × 10.0); velocity = 0.
3. **Too far from home:** a = sq(C.x − home.x) + sq(C.y − home.y), sq(v) = (v × v as a
   wrapping i64) / 65536 truncated toward zero. If a > 0x4000000 (64 blocks), or (+0xbcc
   bit 0x2 and a > 0x100000, 4 blocks): **go to** home (§5.1), E+0x74 = 0, start the search
   (§5.4). Then: if ability ≠ 0 return true; class 0x9f in water: **take off** 0x79 with
   kind 2; if the ability is now ≠ 0 return true; if class ≠ 0xa0 return true; if not in
   water: take off 0x78 with kind 1. Return true.
4. If E+0x28 ≠ 0, or C+0x5c has neither bit 0 nor 1: return true.
5. If +0xbcc bit 0x2: direction = 0, ability = 0x58, C+0x128 &= 1 (all bits but bit 0
   cleared). Return true.
6. **Options**, built in this order:
   * 0x20 if +0xbcc has 0x20 and at least one seat exists. Seats: for zone x from Z(C.x −
     0x400000) to Z(C.x + 0x400000), and inside it zone y from Z(C.y − 0x400000) to Z(C.y +
     0x400000), where Z(v) is the block of v **truncated toward zero** and then divided by 64
     truncated toward zero; for each loaded zone with a non-null object and each object
     index in order: a seat when its user (+0x30) is 0 and its type is 0x1c, 0x1e, 0x1f,
     0x5f, 0x60 or 0x73. Seats are kept as (zone x, zone y, index).
   * then 0x10, 0x8, 0x100, 0x4, 0x1000, each when its +0xbcc bit is set.
7. No options: direction = 0; **rand**: E+0x28 = rand % 2000 + 1000; ability = 0.
8. Otherwise **rand**: o = pick from the options:
   * **0x20:** clear C+0x128 bit 6; ability = 0; **rand**: E+0x28 = rand % 8000 + 5000;
     **rand**: seat = pick. If the object exists: if not 0x2b5d90(C position, object position,
     flag 0, limit 5.0): go to the object's position (§5.1), add the object's address to
     taken, E+0x168 = the seat's zone key, E+0x170 = its index, E+0x74 = 0, start the search
     (which empties taken again). Else append an object interaction for the seat, C's intent
     = 0, direction = 0.
   * **0x4:** direction = 0; **rand**: E+0x28 = rand % 8000 + 5000; ability = 0.
   * **0x8:** the same with ability = 0x57.
   * **0x10:** clear C+0x128 bit 6; **rand a**, **rand b**; direction = (((b + b) / 32767.0
     − 1.0) × 10.0, ((a + a) / 32767.0 − 1.0) × 10.0, 0); **rand**: E+0x28 = rand % 5000 +
     3000; ability = 0.
   * **0x100:** direction = 0; **rand**: E+0x28 = rand % 8000 + 5000; ability = 0x82, +0x6c =
     0.
   * **0x1000:** **rand a**: cx = a % 11 − 5 + cell of C.x's block; **rand b**: cy = b % 11 −
     5 + cell of C.y's block. If the zone (floor(cx × 16 / 64), floor(cy × 16 / 64)) is
     loaded and non-null: clear C+0x128 bit 6; **rand**: E+0x28 = rand % 10000 + 20000;
     ability = 0; spot = 0x31c0e0(zone, cx, cy) (§5.16, may draw one rand); go to the spot,
     E+0x74 = 0, start the search.
9. If the ability is 0: class 0x9f in water → take off 0x79 kind 2; then if still 0, class
   0xa0 and on ground → take off 0x78 kind 1 and velocity.z = 20.0.
10. If the ability is 0x78: set C+0x128 bit 6. Return true.

**Take off** (ability A, kind K): ability = A, +0x6c = 0, item type +0x1d8 = 0x18, +0x1dc =
K, +0x1e0 = x % 10 + 1 (unsigned) where x is the low 32 bits of C's id passed through x ^= x
<< 13, x ^= x >> 17, x ^= x << 5 (32-bit).

### 4.8 Doors (in 0x14e00)

Zone range: x from Z(C.x − trunc64(C+0x84 (as double) × 0.5 × 65536.0) − 65536) to
Z(C.x − trunc64(C+0x84 × 0.5 × −65536.0) + 65536), y likewise with +0x88, Z here being the
floor zone (0x16e50). x outer, y inner. For each loaded zone with a non-null object, objects
in index order: a door when its type is 3, 4, 5 or 0x69 and its +0x28 byte ≠ 0. Box size =
0x13a960 (swap x and y when +0x20 bit 0). With a = trunc64(C+0x84 × 0.5 × 65536) and b =
trunc64(size.x × 0.5 × 65536): overlap in x when obj.x − b ≤ C.x + 65536 + a and C.x − a −
65536 < obj.x + b; y the same with +0x88 and size.y; z when obj.z ≤ C.z + c and C.z − c <
trunc64(size.z × 65536) + obj.z, c = trunc64(C+0x8c × 0.5 × 65536). Each overlapping door
appends an object interaction (§1.5; the record is zeroed in full except the fields named,
with +0xc6 = 1).

---

## 5. Shared routines

### 5.1 Go to a place

goal = the place; start = E's position with start.z −= trunc64((E+0xb0 × 0.5 − 0.1) ×
65536); radius = 5.0. (E+0x128 and E+0x6c are not touched.)

### 5.2 Follow a creature, 0xfe60 (E, T, radius, ahead, match)

1. goal = T's position. If E+0x6c (its value **before** this call) is set: goal = the ground
   ahead of T (§5.3) when found.
2. Settle on the ground as in §2.2 step 5, then goal.z −= trunc64((T+0x8c × 0.5 − 0.1) ×
   65536).
3. start = E's position, start.z −= trunc64((E+0xb0 × 0.5 − 0.1) × 65536).
4. E+0x120 = (radius + E+0xa8) + T+0x84. E+0x128 = T's id. E+0x6c = ahead.
5. E+0x74 = sqrtf(T intent.x² + y² + z²) if match is on and ahead is off, else 0.
6. Start the search (§5.4).

### 5.3 The ground ahead of a creature

a = heading × 0.017453292 (float); c = cosf(a), s = sinf(a). Q = (pos.x + trunc64(c × 2.0 ×
65536), pos.y + trunc64(s × 2.0 × 65536), pos.z). (The game forms these through a rotated
identity matrix; the extra terms are exact zeros.) Then up to 3 times: if the block at
(Q.x, Q.y, Q.z − 65536) is solid stop, else Q.z −= 65536. Then up to 3 times: if the block
at Q is not solid stop, else Q.z += 65536. Found when the block at (Q.x, Q.y, Q.z − 65536)
is solid and the block at Q is not.

### 5.4 Start a search, 0x133d0

Forget; E+0xe0.. = block of start; E+0x130 = 0; record the start block (§5.5) with g = 1, h
= f = H(start block, block of goal) (passed through a float, exact); its "from" x and y are
stack contents and its from.z is the start's z (never read, see §5.7); then empty taken.

### 5.5 Record a step, 0x13160

Insert or overwrite the step for the block; insert the block into open. Then, unless best
is not none and best's h ≤ the new h, best = this block's node.

### 5.6 Expand, 0x15640

1. If there are no steps, or open is empty: done.
2. c = the open block whose step has the smallest f (first in block order on ties; blocks
   without a step are skipped). Remove c from open. If c is the goal's block: done. g = c's g.
3. Body box: cx = c.x × 65536 + 32768, cy = c.y × 65536 + 32768, cz = c.z × 65536 +
   trunc64(double(E+0xb0) × 0.5 × 65536.0). With hx = trunc64(E+0xa8 × 0.5 × 65536) (float),
   hy likewise with +0xac, hz with +0xb0: x0 = block(cx − hx) − 1, x1 = block(cx + hx) + 1,
   y0 = block(cy − hy) − 1, y1 = block(cy + hy) + 1, z0 = block(cz − hz + 655) − 1, z1 =
   block(cz + hz) + 2. here = type of the block at (cx, cy, cz + 65) (0xeab0).
4. Visit x from x0 to x1, inside y from y0 to y1, inside z from z0 to z1, skipping blocks with
   x0 < x < x1, y0 < y < y1 and z0 < z < z1. For each visited block b (0xe9a0); "inner" means
   x0 < x < x1 and y0 < y < y1; *glider* = E+0x6f:
   * if inner and z = z0: **under** is set when b is solid, or (b flags has 0x40 and (not
     glider or flags lacks 0x80)), or b's type is 2; then the block below it (z0 − 1): **below**
     is set when solid, or (flags 0x40 and (not glider or flags lacks 0x80));
   * b **blocks** when solid, or (flags has 0x40, lacks 0x04, and (not glider or lacks
     0x80)). A blocking b sets: floor corner (x0,y0), (x1,y0), (x0,y1), (x1,y1) when at that
     corner with z = z0; edge corner (same four) at that corner with z0 < z < z1; roof when
     inner and z = z1; floor sides −X (x = x0, inner y, z = z0), +X, −Y, +Y likewise; walls
     −X, +X, −Y, +Y the same with z0 < z < z1.
5. If not below and E+0x8c has 0x200 and there is more than one step: done.
6. water = (here = 2). Offer (§5.6.1) in this order:
   * −X (c.x − 1) unless wall −X, when under or floor −X or wall −Y or wall +Y or edge
     (x0,y0) or edge (x0,y1) or water; cost g + 10 if under, else g + 20;
   * +X likewise with wall +X, floor +X, walls ±Y, edges (x1,y0), (x1,y1);
   * −Y with wall −Y, floor −Y, walls ±X, edges (x0,y0), (x1,y0);
   * +Y with wall +Y, floor +Y, walls ±X, edges (x0,y1), (x1,y1);
   * (−1,−1) unless edge (x0,y0), when −X and −Y were both offered successfully and (under or
     floor corner (x0,y0) or water); cost g + 14 if under, else g + 24;
   * (+1,−1), (−1,+1), (+1,+1) likewise with their edge, the two sides and their floor
     corner;
   * down (c.z − 1) when not under, cost g;
   * stop if roof; stop if not under and E+0x8c has 0x200; stop if none of the four walls and
     not water; else up (c.z + 1), cost g + 14 if under, else g + 28.

#### 5.6.1 Offer (block n, cost)

If n has no step: record it (§5.5) with g = cost, h = H(n, goal block), f = cost + h, from =
c: success. If n has a step but is not in open, or its g ≤ cost: failure. Otherwise
overwrite with g = cost, the old h, f = cost + h, from = c: success.

### 5.7 Rebuild the path, 0xedb0

Empty the path. cur = best's block if best is not none, else the goal's block. Put cur in
front. Then loop with a counter k from 0: stop if cur = the start's block; stop if k > the
step count (signed 32-bit compare); k += 1; look up cur's step, stop if none; stop if its
from equals cur; cur = from; put cur in front.

### 5.8 Walkable, 0xa950 (size, pos, goal, r, glider, flags) → bool

*Passes* (a block a body can be in): not solid and not (flags 0x40 set, 0x04 clear and (not
glider or 0x80 clear)).

1. zc = block(pos.z + trunc64((0.1 − size.z × 0.5) × 65536)); cell = (block(pos.x),
   block(pos.y)).
2. f = (goal − pos).xy in blocks; len = sqrtf(f.x² + f.y² + 0.0); dir = f / len; most = len
   if len ≤ 50.0, else 50.0. t = 0; footing = false.
3. While 0 < most (checked once), loop:
   1. px = pos.x + trunc64((dir.x × t) × 65536), py likewise. x0 = block(px −
      trunc64(double(size.x) × 0.5 × 65536.0)), x1 = block(px − trunc64(double(size.x) ×
      0.5 × −65536.0)), y0/y1 likewise; top = block(trunc64(((float(zc) + size.z) + 1.0) ×
      65536)).
   2. If t > 0: for x0..x1, y0..y1, z from zc+1 to top: any block that does not pass →
      **false**.
   3. Step up: for x0..x1, y0..y1: if the block at zc is solid: for z from zc+1 to top, a block
      that does not pass gives **false** when t > 0; then step = true. If step: zc += 1.
   4. footing = false; for x0..x1, y0..y1: footing when the block at zc − 1 is solid or (that
      not being so) the block at zc is water; if flags has 0x200: "lower" when the block at
      zc − 2 is solid (any column). If flags has 0x200 and no lower: **false**.
   5. If the block at (cell, zc) is water: level = trunc32(size.z × 0.5 + float(zc)); want =
      goal.z / 65536 truncated toward zero; zc −= 1 if want < level, += 1 if level < want.
      Else if not footing: zc −= 1.
   6. step = 10.0, axis = x; if dir.x² ≥ 0.000001: l = cell.x × 65536 − px if dir.x ≤ 0, else
      (cell.x + 1) × 65536 − px; s = (float(l) × 1/65536) / dir.x; if s < 10.0, step = s. Then
      y the same, taking it when s < step (axis = y). cell[axis] −= 1 if dir[axis] ≤ 0, else
      += 1. t += step; leave when most ≤ t.
4. end = (cell.x × 65536 + 32768, trunc64((double(cell.y) + 0.5) × 65536.0),
   trunc64((size.z × 0.5 + float(zc)) × 65536)).
5. True when (footing, or the block at (cell, zc) is water) and (r² < ey² + ex², e = (end −
   goal).xy in blocks, or the open line from end to goal (§5.9) with r).

### 5.9 Open line, 0x121a0 (from, to, r) and its ray 0x12b10

d = (to − from) in blocks; s = d.x² + d.y² + d.z². True if s < 0.0001. False if s > r². Else
L = sqrtf(s), u = d / L, t = the ray 0x12b10(from, u, L); true when L − 0.01 ≤ t.

0x12b10 is the grid walk of 0x2d1d20 (`effect-objects.md` §2.5, same steps, same 500-step
cap, same zone handling) with its own pass test: a block passes when its type is 0 or 2 and
(flags lacks 0x40, or has 0x04, or has 0x80).

### 5.10 Box touches, 0x2b7ad0 (centre, size)

lo/hi per axis = block(centre ∓ trunc64(size × 0.5 × 65536)). Over the shell of that box
(x outer, z inner, interior skipped): touches at the first block that is solid, or has flags
0x40 without 0x80, or is water.

### 5.11 In sight, 0x2a1250 (A, B, range)

If B's stealth > 0: range × k with k = 1 − stealth, or 0.1 if 0.1 > k. Then 0x2b5d90 from A's
position to B's, flag 0, that range.

### 5.12 Hostile, 0x4ef80 (A, B)

False when both kinds are 0, 3 or 6; when either class is 0xa1 or either race is 0x12f; when
A+0x128 bit 1 or B+0x128 bit 1; when either kind is 7 or 8; when a kind-0 side has +0x128
bit 7 set. True when either has +0x128 bit 5. Otherwise each side is "monster" when its
kind is 1 and its +0x80 bit 15 is clear; hostile when exactly one side is a monster.

### 5.13 Normalise, 0x127f0

k = 1.0 / sqrtf(x² + y² + z²); x = x × k, y = k × y, z = k × z.

### 5.14 Shot point, 0x654a0

If C+0x80 bit 2 is clear: C's position. Else a = heading × 0.017453292, u = C+0x88 × 0.5,
v = C+0x8c × 0.35: position + (trunc64((−sinf(a) × u) × 65536), trunc64((cosf(a) × u) ×
65536), trunc64(v × 65536)). (Computed through the rotated identity matrix; the homogeneous
divisor is exactly 1.)

### 5.15 Choosing and starting attacks

"within(X)" = ability is X and +0x6c < duration + 200 (as float). "boosted" = force is on,
or C has a buff of type 13.

**Next hit, 0x5fc40 (force):** if boosted: class 3 → 0x87 (spec 1) or 0x61; class 4 spec 1 →
0x6c. Race 0x123 → 0x9e. Class 0x91/0x92 → within(7) ? 8 : 7; 0x93 → within(2) ? 3 : 2;
0x94 → within(0x42) ? 0x43 : 0x42; 0x95/0x96 → 0x17; 0x97 → 0x68; 0x98 → 0x2d (spec 1) or
0x27. Else w = slot-B subtype: w = 0x17 → 0x4c; w = 0x16 → the final rule. If mana user: w =
0xb → 0x6b/0x68 (spec 1/else); w = 10 → 0x21/0x1f; spec ≠ 1 → within(0x29) ? 0x28 : 0x29;
spec 1 → within(0x2b) ? 0x2a : 0x2b. Not a mana user: race 0x68, or slot B type 3 with w 6
or 7, or slot A type 3 with subtype 6 or 7 → 0x17; w = 3 → within(0x14) ? 0x13 : 0x14; w = 4
or slot B type 0 → within(7) ? 8 : 7; w = 5 → within(0xf) ? 0xe : 0xf; slot B type 3 and w =
8 → 0x1b; large weapon (0x5e9b0: slot B or slot A of type 3 with subtype 5, 6, 7, 8, 10, 11,
15, 16, 17, 18 or 23) → within(0x3a) ? 0x44 : within(0x44) ? 0x3b : 0x3a; slot A type 3
subtype 13 → within(0xb) ? 10 : 0xb. Final rule: within(2) ? 3 : 2.

**Charge move, 0x60120 (force):** if boosted: class 3 spec 1 → 0x86; class 4 → 0x32. Race
0x123 → 0x9e. Class 0x91 → 0x15; 0x92 → 0x41; 0x93, 0x94 → 0x40; 0x95 → 0x19; 0x96 → 0x18;
0x97 → 0x69; 0x98 → 0x2c (spec 1) or 0x26. w = 0x17 → 0x4c; w = 0x16 → 3 if ability is 2 and
+0x6c < duration + 200, else 2. Slot A type 3 subtype 6 or slot B type 3 w 6 → 0x1a; the same
with 7 → 0x19; with 8 → 0x1c. Not a mana user: slot B type 3 with w 5 → 6, with w 3 → 0x12;
slot A type 3 subtype 13 → 9; large weapon → 0x3c; w = 4 or slot B type 0 → 0x15; else 0x40.
Mana user: w = 0xb → 0x6a (spec 1) or 0x69; w = 10 → 0x22 or 0x20; else 0x2c or 0x26.

**Start a hit, 0x60460 (force off here):** fails if the dodge timer ≠ 0 or the ability is
0x32. t = duration; if combo ≠ 0 and the ability is neither 0x1f nor 0x21: t −= recovery of
the current ability (Part C). m = next hit. Succeeds when float(t) ≤ +0x6c, mana cost(m) ≤
mana and stamina cost(m) ≤ stamina: ability = m, +0x6c = 0.

**Start a charge, 0x60720 (force off here):**
1. Fails if the ability is 0x93, 0x32 or 0x56.
2. t = duration; if combo ≠ 0: t −= recovery of the current ability.
3. m = charge move. If m is 9, 0x19, 0x1a, 0x1c, 0x3c or 0x40, or (m is 6, 0x12 or 0x15
   and +0x6c < duration): fail if m equals the ability; unless the ability is 0x0c, 0x3d,
   0x5f or 0x74, t = 200 if the ability is 0, else 0.
4. held = class 4, spec 1 and m is 6, 0x12 or 0x15.
5. Fail if +0x6c ≤ float(t); or dodge timer ≠ 0 and not held; or mana < mana cost(m); or m
   is 6, 0x12, 0x15 or 0x32 and mana < 0.1.
6. ability = charge move (asked again), +0x6c = 0. If that move is 0x20, 0x22, 0x26, 0x30 or
   0x69 and C has a buff of type 9: +0x6c = float(effect time of that move) (Part A).
7. If not held: dodge timer = 0. Succeeds.

### 5.16 Cell spot, 0x31c0e0 (zone, cell x, cell y)

Part of the zone's generated data, summarised: the kind is the first of 2, 7, 0x10 for
which 0x1299a0 reports spots in that cell, else 1; 0x31bd90 then takes a spot of that
kind out of the cell's list with **one rand** (index = rand mod count; the spot is removed)
when the list is not empty, or returns the zone's middle (x and y = zone origin + 32 blocks
+ 32768 dots, z from the zone's column data) without a rand. Kind 3 is never asked here.
The oracle records each answer (§7).

---

## 6. rand() order in one tick

Draws happen only in phase A, in world order of the creatures, and inside one creature's
turn in this order: attach (3 per nearer monster), potion (2), special skill (1 + 1),
attacks (race picks, dodge roll and its direction, leap rolls, mana-user choice, race 0x77's
odd/even rolls before its pick), call answering (none), the no-target reset (1), greet (0 or
1), idle (as listed in §4.7, including the cell spot's own draw after the timer draw).
Phases B–D draw nothing. Each "**rand**" in §3–§5 is one call, made only when the text
reaches it (all conditions before it are short-circuit).

---

## 7. Oracle

`tests/EngineTests.cpp`, namespace `aioracle`, run as `EngineTests aioracle <tick or −1>
<files…>`, checks an implementation against **1100 recorded ticks** in
`Extract/oracle/ai-fight.bin`, `ai-fight2.bin`, `ai-idle.bin`, `ai-idle2.bin` and
`ai-town.bin`, made by ZoneExtractor's `ai.request` probe (it wraps 0x13830 under the
world's lock and logs every 0xeb90 block lookup, rand(), timeGetTime() and 0x31c0e0 answer
the tick made).

File: magic "CWA1" or "CAI2" (the digit is the version), 4 more bytes, then records, each a
u32 byte length followed by:

1. u32 dt; f32 world time (+0x440); (v2) f32 time of day (+0xc); u64 local player address.
2. Zones: u32 n; each i32 zone x, i32 zone y, u64 zone address; when non-zero, u64 address
   of its first object, u32 object count, the objects (0xe0 bytes each).
3. State **before**, then state **after**, each:
   * creatures: u32 n; each u64 address, the first 0xc10 bytes of the record, u32 + buffs
     (0x18 each), u32 + interactions (0xc8 each), aggro then damage maps (u32 n; i64 id, f32
     value), (v2) group (u32 n; i64), skills (u32 n; i32), inventory (u32 tabs; per tab u32
     stacks, 0xa4 bytes each);
   * id map: u32 n; i64 id, u64 address;
   * AI records: u32 n; each the 0x178 raw node bytes, waiting (u32 n; 0xc8 each), steps
     (u32 n; 12-byte block, i32 g, h, f, 12-byte from), open (u32 n; 12 bytes), best (u8 1 =
     none, else 0 followed by its 12-byte block), path (u32 n; 12 bytes), taken (u32 n;
     u64), the +0x158 list (u32 n; 16 bytes);
   * aggro notices (u32 n; 0x18 each), buff notices (u32 n; 0x30 each), calls (u32 n; i64).
4. Blocks looked up: u32 n; each two i32 (not used), i32 x, y, z, 6 block bytes.
5. rand() results: u32 n; i32 each. timeGetTime() results: u32 n; u32 each.
6. (v2) cell spots: u32 n; i32 cell x, cell y, i64 ×3, in the order asked.

The test replays each tick from "before" (a block outside a loaded zone is the default solid
block c8 c8 c8 00 01 00; a missing recorded block is treated as that too and counted) and
demands exactly "after": for every non-player creature the 0xc10 record bytes except
container words (+0x0, +0x980..+0x98f, +0x9f0..+0xa07, +0xab8..+0xac7, +0xb88..+0xba7,
+0xbd0..+0xbe7), and for every creature its buffs, interactions, aggro and damage maps;
every AI record's bytes except +0x00..+0x1f, +0x78..+0x87, +0xb8..+0xdf, +0x138..+0x147,
+0x148..+0x157, +0x158..+0x167, plus its waiting list, open set, path, taken set, steps and
best; the number of records; the aggro notices (bytes +0x14..+0x17 ignored), buff notices and
calls; and that exactly as many rand() and timeGetTime() values were taken as recorded. The
"taken" set holds addresses; the test maps them as the zone's first-object address + index ×
0xe0. A player's record moving during a recorded tick (the network writes it) is reported
separately.

---

## 8. Constants

| Value | Address | Use |
|---|---|---|
| 1/65536 | 0x42e510 | dots to blocks |
| 65536.0 | 0x42e5f8 | blocks to dots |
| 32768.0 | 0x42e5f4, 0x42e588 (double) | half a block (dots) |
| −32768.0 (double) | 0x42e618 | cell centre in walkable's end point |
| 655.36 | 0x42e5cc | expand: floor offset (truncated to 655) |
| −65.536 (double) | 0x42e610 | expand: "here" probe offset (truncated to −65, i.e. +65) |
| 0.5 | 0x4ac964; 0.5 double 0x4ac9a8 | half sizes |
| 65536.0 / −65536.0 double | 0x42e590 / 0x42e620 | half-size spans in double |
| 0.1 | 0x42e528 | feet offset, stealth floor, pet heal, path drop factor |
| 0.01 | 0x42e51c | ray tolerance, straight-speed threshold, idle flat threshold |
| 0.001 | 0x42e514 | "moving" threshold; ms to s |
| 0.0001 | 0x4ac954 | open line minimum |
| 0.000001 | 0x42e50c | grid-walk axis threshold |
| 1.0 | 0x4ac980 | |
| 2.0 | 0x4ac9d0 | ground ahead distance, path walk r, potion intent offset |
| 4.0 | 0x4aca20 | radius ratio, owner radius, mounted range |
| 8.0 / 16.0 | 0x4aca30 / 0x4aca34 | group radius / owner near² |
| 5.0 | 0x42e584 | home/object radius, ride-along radius, seat sight, dodge lift |
| 6.0 | 0x4aca24 | edge-guard height limit |
| 10.0 | 0x42e598 | straight speed ×10, step cap, leap speed, stroll speed |
| 20.0 | 0x42e5a0 | sight ranges, dodge speed, skill range |
| −20.0 | 0x42e628 | dodge backwards |
| 25.0 | 0x42e5a4 | seat arrival, ride-along, staff spin threshold |
| 30.0 | 0x42e5a8 | guard reach |
| 50.0 | 0x42e5b0 | walkable cap, bow range, skill 0x37 speed |
| 64.0 / 256.0 | 0x42e5b4 / 0x42e5c4 | group near² / far² (and leap distances²) |
| 80.0 | 0x42e5b8 | walking speed |
| 100.0 | 0x42e5bc | greet nearest start; recovery K |
| 200.0 | 0x42e5c0 | sight limit |
| 500.0 | 0x42e5c8 | budget cap; teleport window |
| 900.0 | 0x42e5d0 | race 0x114 home leash² |
| 1000.0 | 0x42e5d4 | teleport window; skill 0x37 ms |
| 2000.0 | 0x42e5d8 | leap wait |
| 3000.0 / 5000.0 | 0x42e5dc / 0x42e5e4 | potion / cower lock; heal buff duration |
| 3600.0 | 0x42e5e0 | ranged reach² |
| 10000.0 | 0x42e5e8 | stuck-ability limit; catch-up distance² |
| 20000.0 | 0x42e5ec | buff 8 duration |
| 32767.0 | 0x42e5f0 | rand scale |
| 0.017453292 | 0x42e520 | degrees to radians |
| 0.005 | 0x42e518 | staff spin rate |
| 1.5 | 0x42e548 | staff spin radius |
| 1.1 | 0x42e538 | heal buff strength |
| 0.25 | 0x4ac95c | potion threshold |
| 0.8 (double) / 0.8 / 0.9 | 0x42e558 / 0x42e530 / 0x42e534 | charge thresholds |
| 3.0 | 0x42e580 | melee reach factor |
| 0.35 | 0x4328a8 | shot height |
| 524288.0 / 131072.0 | 0x42e600 / 0x42e5fc | leap height differences (8 and 2 blocks) |
| 25 200 000 / 72 000 000 | 0x42e604 / 0x42e608 | day window (7:00 to 20:00) |
| identity matrix | 0x42e630..0x42e66c | rotation base |
| 300, 400, 800, 600, 1200, 1500, 1600 | 0x42e7e0, 0x42e7e8, 0x432940, 0x42e7ec, 0x432944, 0x432948, 0x43294c | duration parts |

---

## 9. Open questions

1. **Uninitialised record bytes.** A new AI record leaves +0xec..+0x127, +0x130 and +0x168..
   +0x177 as the heap gave them. +0x170 is tested (≥ 0) in phase C before anything writes
   it; in practice a fresh record is only reached after a search or seat sets it, but a
   creature that never sits keeps whatever was there. Recordings should show whether the
   allocator hands back zeroed or recycled memory (current CubeVulkan sets −1).
2. **Padding bytes in notices and buffs.** The heal buff (§4.3) and the type-8 / re-given
   type-7 buffs (§4.4 step 27) carry unset bytes (1–3, 0xc–0xf) that come from the stack;
   buff-change notices copy them. The oracle compares buff notices in full; whether the
   recordings contain any of these cases is unknown.
3. **The AI thread's rand() seed.** Not established here (no srand on that thread was
   seen); the oracle supplies the draws.
4. **E+0x40 (focus id) and the waiting list E+0x78** are never written by the tick; who
   writes them (if anyone) was not traced.
5. **0x31c0e0 / 0x31bd90 / 0x1299a0** (cell spots) are summarised only; their full rules
   belong with the zone-generation spec.
6. **C+0x128 bit 12 with no skill** (§4.4 step 29.4) does nothing in the decompile and the
   assembly (jump straight to writing the aim). CubeVulkan's current transcription sets
   ability 0x76 there; the spec follows the game.
7. **0x2b5d90's last argument** (a creature, passed 0 here, or the target by 0x2a1250)
   plays no part in `effect-objects.md` §2.5's description; confirm it changes nothing.
8. **Infinite goal settling.** §2.2 step 5 and §5.2 lower the goal until the target's box
   touches something, with no limit; with no blocks below (e.g. over an unloaded zone the
   default block is solid, so it ends) it relies on the default block.
9. **Path when the start's "from" is read.** The start step's from x/y are stack contents;
   §5.7 never reads them because it stops at the start block first. Confirm no other reader.
