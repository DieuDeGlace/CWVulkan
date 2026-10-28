# Creature upkeep (0x2de4c0)

Behavioural specification of Steam Cube World's per-creature upkeep, written from the
decompiled executable, its assembly and the binary's constants. Routines are cited by RVA
(image base 0x140000000). Offsets are byte offsets into the record named. Times are
milliseconds, distances are blocks, and world positions are 64-bit integers in 1/65536 of a
block. Vocabulary (buff entries, hostility kinds, sound requests, buff-change notices, "give
buff" 0x65390) is shared with `skills-and-interactions.md`.

The world tick 0x2da6c0 calls 0x2de4c0 (world, creature, *dt*) once per creature per step,
for **every** creature in the world list, alive or dead, after that creature's physics
(0x2bb8e0), ability update (0x2c5250) and interaction handler (0x2c3710). *dt* is the step's
elapsed milliseconds as a 32-bit float.

What it does, in one line each: group-fight health buff, health cap, ownership and dungeon
flags, special races' drains, riders and carried creatures following their carrier, the
short timers, buffs (countdown, poison hits, healing, expiry), health rescaling, stun, the
ability clock, the end-of-attack "missed targets" pass, mana, block power, stealth, stamina
(gliding, swimming and drowning, sprint-like drains, regeneration) and the dodge timer.

It does **not** count down the cooldown map (+0xb78; that happens at the end of 0x2c5250)
and it never ends an ability because its duration ran out: it only advances the ability
clock and cancels abilities in the specific cases listed below.

---

## 0. Conventions that matter for bit-exact results

* All arithmetic is IEEE single precision (SSE, round to nearest, no fused multiply-add, no
  extended precision) unless a step says "in double". Each operation is rounded to float
  before the next one; the order of operations given in each step is the game's order and
  must be kept (float addition and multiplication are not associative).
* "Truncate" means conversion toward zero. Two widths are used: **trunc32** (float to 32-bit
  integer) and **trunc64** (float to 64-bit integer). A value outside the range gives the
  "integer indefinite" value (0x80000000 or 0x8000000000000000). Integer divisions below
  (by 2, 400, 500, 1000) are C signed divisions, truncating toward zero.
* Comparisons are written exactly as the game tests them ("if 0 > x" is not the same as
  "unless 0 ≤ x" when x is NaN). Where a NaN would behave differently from the obvious
  reading, the step says so; otherwise any ordered comparison with NaN is false.
* "Float of an integer" is an exact conversion of a signed 32-bit integer unless stated.
* sinf, cosf, sqrtf and powf are the Microsoft UCRT's (`api-ms-win-crt-math`, thunks at
  0x395456, 0x395450, 0x395462, 0x395492). sqrtf is correctly rounded; for powf, sinf and
  cosf see Open questions.
* rand() is the CRT's (0..32767), imported through the slot at 0x42cd28 and shared with the
  rest of the game. Every draw here is used as rand × A / 32767.0 + B, computed as
  ((float of rand) × A) / 32767.0, then + B.
* Where a record field is read twice in a step, it is read again from memory (a value
  written earlier in the same step is seen).

---

## 1. Inputs

### 1.1 Creature record fields (record size 0x15f8)

R = read, W = written. "Other" means the field is read or written on another creature.

| Offset | Type | R/W | Use here |
|---|---|---|---|
| +0x08 | i64 | R | id |
| +0x10, +0x18, +0x20 | i64 ×3 | R W | position |
| +0x28, +0x2c, +0x30 | f32 ×3 | R W | orientation; +0x30 (of another creature) is an angle in degrees |
| +0x34, +0x38, +0x3c | f32 ×3 | R W | velocity |
| +0x40, +0x44, +0x48 | f32 ×3 | R W | movement intent (where the creature wants to go) |
| +0x4c, +0x50, +0x54 | f32 ×3 | W | zeroed by ability 0x8c |
| +0x5c | u32 | R | movement flags: bit 0 on ground, bit 1 in water, bit 2, bit 4, bit 8 |
| +0x60 | u8 | R W | hostility kind (0 player; 6, 7, 8 used below) |
| +0x64 | i32 | R | race |
| +0x68 | u8 | R W | current ability (0 = none) |
| +0x6c | f32 | R W | ability time (clock) |
| +0x70 | i32 | R | combo counter (attack-speed factor) |
| +0x80 | u32 | R | flags (bits 4, 9, 12 = 0x1000, 17, 18, 26 used) |
| +0x84, +0x88, +0x8c | f32 ×3 | R | size factors |
| +0x128 | u16 | R W | state flags: 0x2 read; 0x40 read; 0x100 written; 0x400 read and cleared |
| +0x12c | f32 | R W | dodge timer |
| +0x130 | f32 | R W | stun timer |
| +0x134 | f32 | R W | stamina-regeneration lock timer |
| +0x138 | f32 | R W (also other) | timer |
| +0x13c | f32 | R W | timer |
| +0x140 | f32 | R W | a value ≤ 0 that recovers toward 0 |
| +0x14c | u8 | R | class |
| +0x14d | u8 | R | specialisation |
| +0x150, +0x154 | i32 ×2 | R | the creature's current region (item scaling, inventory template) |
| +0x174, +0x178, +0x17c | f32 ×3 | R | offset from the carrier (ability 0x91); +0x17c is also race 0x12e's rising value (R W) |
| +0x180 | f32 | R W | health |
| +0x184 | f32 | R W | block power (0..1) |
| +0x188 | f32 | R W | mana (0..1) |
| +0x18c | f32 | R W | stealth (0..1) |
| +0x19c | u8 | R | race 0x12e's "rising" flag |
| +0x1a0 | i64 | R | owner id |
| +0x1a8 | i64 | R | linked creature id (mount for 0x77, carrier for 0x91, focus target for 0x8c) |
| +0x318 … +0x818 | items, 0xa0 each | R | equipment (§4.1) |
| +0x980 / +0x988 | list / u64 | R W | buff list and its element count |
| +0x990 | f32 | W | copied from the mount / owner / focus target |
| +0x994 | f32 | R W | a value that decays toward 0 |
| +0x998, +0x99c | f32 ×2 | W | copied from mount / owner; zeroed by 0x8c |
| +0x9a0 | f32 | R W | stamina (0..1) |
| +0x9b8 / +0x9c0 | ordered set of i64 / count | R W | id set "A" (cleared when the ability is 0) |
| +0x9c8 / +0x9d0 | ordered set of i64 / count | R W | id set "B" (cleared when the ability is 0) |
| +0x9f0, +0x9f8 | vector | R | inventory tabs (§4.6) |
| +0xadc | f32 | W | copy of the dodge timer before its countdown |
| +0xae0 | 24 bytes | R | a position handed to the hit routine (poison) |
| +0xb10, +0xb18, +0xb20 | i64 ×3 | R W | a second position kept in step with +0x10..+0x20 |
| +0xb34, +0xb38, +0xb3c | f32 ×3 | W | copied from the owner (kind 6) |
| +0xb58 | 4 bytes | W | zeroed (kind 6) |
| +0xb64 | i32 | R | swimming skill points (stamina drain in water) |
| +0xb88 | ordered map i64 → f32 | R | aggro |

Buff entry (0x18 bytes): type u8 +0, (3 padding bytes), strength f32 +4, duration f32 +8,
(4 padding bytes), source creature id i64 +0x10. A list node holds the entry at node+0x10.

Item (0xa0 bytes): type u8 +0, subtype u32 +4, modifier u32 +8, region i32 +0xc and +0x10,
rarity u8 +0x14, u32 +0x18, material u8 +0x1c, attributes (4 bytes each) from +0x20,
attribute count u8 +0x9d.

### 1.2 World fields

| Offset | Type | Use |
|---|---|---|
| +0x0c | f32 | time of day, ms (a day is 86 400 000) |
| +0x20 | ordered set of i64 | ids of dead creatures ("the dead set") |
| +0x90 | list, 0x48-byte entries | damage numbers (§2.3) |
| +0xa0 | list, 0x30-byte entries | sound requests (§2.2) |
| +0x150 | list, 0x30-byte entries | buff-change notices (§2.4) |
| +0x358 | u8 | remote-client flag (non-zero: this world only mirrors a server) |
| +0x440 | f32 | world time, ms |
| +0x448 | pointer | the local player's record |
| +0x450 | ordered map i64 → record | creature by id (0xe720 looks up; absent → none) |
| +0x460 … +0x490 | hash map | zones by zone coordinates (key: zone x as the low 32 bits, zone y as the high 32 bits) |

A zone object (value of the zone map) has: +0x470 an ordered set of linked zone keys (node
key at node+0x1c: zone x i32, zone y i32), +0x478 its element count, +0x480 a list of
creature ids (id at node+0x10).

A position's **block** coordinate is floor(position / 65536) (exact integer floor, also for
negatives); its **zone** coordinate is floor(block / 64).

---

## 2. Records this routine produces

### 2.1 Hit request (0x29eb60)
Only poison makes one (§3 step 8). The hit routine 0x29eb60 is the game's general "apply a
hit" and is specified elsewhere; it may draw rand(), change the target's record and buffs and
append to world lists. Arguments: world; target = this creature; attacker = the buff's
source creature looked up by id (none if absent); damage = the buff's strength; critical
byte 0; a second byte 0; a float 0.0; a pointer to this creature's 24 bytes at +0xae0; a
push vector (0, 0, 0); type 6; ability 0; an 8-byte value 0; a final byte 0. Its result is
ignored.

### 2.2 Sound request (appended to world+0xa0)
+0x00 / +0x08 / +0x10 position (i64 ×3), +0x18 sound id (i32), +0x1c pitch (f32), +0x20
volume (f32), +0x24 four bytes never written (the list copy leaves them as found), +0x28
i64 "owner" id. All sounds here have volume 1.0.

| Id | File (load order of 0xa3a40) | Made by |
|---|---|---|
| 0x1a | roll.wav | dodge timer crossing 590 (step 23) |
| 0x2d | watersplash-hit.wav | water bubble running out of mana (step 6) |
| 0x2f | drink2.wav | drowning damage (step 22, case 5) |
| 0x3c | drop.wav | race 0x12e rising (step 1e) |

### 2.3 Damage number (appended to world+0x90, 0x48 bytes)
+0x00 i64 "from" id, +0x08 i64 "to" id, +0x10 f32 value, +0x14 u8, +0x15..+0x17 not
written, +0x18 8 bytes 0, +0x20/+0x28/+0x30 position (i64 ×3), +0x38 8 bytes 0, +0x40 4
bytes 0, +0x44 u8, +0x45 u8, +0x46 u8, +0x47 not written.

### 2.4 Buff-change notice (appended to world+0x150, 0x30 bytes)
+0x00 source id, +0x08 target id, +0x10 a copy of the 0x18-byte buff (padding included),
+0x28 flag byte, +0x29..+0x2f copied from wherever the notice was assembled (not
initialised by the game; see Open questions).

---

## 3. The upkeep, in order

C is the creature, W the world, *dt* the elapsed ms. "Authoritative" means W+0x358 is 0.
"Local" means C is W+0x448.

### Step 0. Remember the maximum health
*oldMax* = maxHealth(C) (§4.2), computed now, before anything changes.

### Step 1. Authoritative-only bookkeeping (skipped entirely when W+0x358 ≠ 0)

**1a. Group-fight health buff.** Walk C's aggro map (+0xb88) in ascending id order. For each
id, look the creature up in W+0x450; count it if it exists, its health (+0x180) is > 0 and
its hostility kind (+0x60) is 0 (a player). If the count *n* is greater than 1:
* *s* = (float of (*n* − 1)) × 0.5, then + 1.0.
* Find the first buff of type 0x20 in C's list (front to back). If there is none, or its
  strength is not equal to *s* (a NaN strength counts as not equal), then:
  * give C (refresh flag **set**) the buff: type 0x20, strength *s*, duration 10000.0
    (0x42e5e8), source = C's id. The buff's padding bytes are uninitialised (Open questions).
  * append a buff-change notice: source = C's id, target = C's id, that buff, flag 1.

Because a type-0x20 buff multiplies maximum health (§4.2), a monster that two or more living
players are angry at gets 1.5×, 2×, … health.

**1b. Health cap.** If health > maxHealth(C) (recomputed now), health = maxHealth(C)
(computed again).

**1c. Orphaned summons.** If the hostility kind is 8 and the owner id (+0x1a0) is not 0 and
the owner id is in the dead set (W+0x20), the hostility kind becomes 7.

**1d. Zone-cleared flag.** Only if bit 0x1000 of +0x80 is set:
1. Find C's zone in the zone map (zone coordinates of +0x10 and +0x18). If it is absent or
   its object is null, nothing changes; go to 1e.
2. If the zone's linked-zone count (+0x478) is 0: if bit 0x2 of +0x128 is clear, set bit
   0x100 of +0x128; if bit 0x2 is set, clear bit 0x100. Go to 1e.
3. Otherwise, for each linked zone key in ascending key order: look the key up in the zone
   map; if absent or null, **stop, leaving +0x128 unchanged**. For each id in that zone's id
   list (+0x480, list order): if the id is not in the dead set, stop, leaving +0x128
   unchanged.
4. If every check passed, set bit 0x100 of +0x128.

**1e. Race 0x12e (302) rising.** Only if the race is 0x12e and the byte +0x19c is non-zero:
1. Probe the block at (x, y, z′) where z′ = +0x20 + trunc64(+0x17c × 65536.0) + 0x10000
   (integer additions), through 0xeab0/0xe9a0 (§4.7).
2. Continue only if the block's type byte is 0 or 2 (air or water) **and** not (+0x17c >
   100.0).
3. *v* = +0x17c; *new* = *dt* × 0.01, then + *v*.
4. If trunc32(*new*) / 2 differs from trunc32(*v*) / 2: **rand** — append a sound request:
   C's position, id 0x3c, pitch = rand × 0.4 / 32767 + 0.8, volume 1.0, owner 0.
5. +0x17c = *new*.

### Step 2. Ability 0x77: riding a mount
Only if the ability (+0x68) is 0x77. Let M be the creature whose id is +0x1a8.
* If +0x1a8 is 0, or M is not found, or M's health is not > 0, or not (M's stun +0x130 ≤ 0)
  (a NaN stun also fails), the ability becomes 0. Nothing else in this step.
* Otherwise:
  1. (*ox*, *oy*, *oz*) = the mount offset for M's race (0x65040, §4.8); *oz* = *oz* + (M's
     +0x8c × 0.75) — computed as M+0x8c × 0.75 first, then added to *oz*.
  2. *dz* = trunc64((((M+0x8c × 0.5) − (C+0x8c × 0.5)) + 0.01) × 65536.0).
  3. C+0x10 = trunc64(*ox* × 65536) + M+0x10; C+0x18 = trunc64(*oy* × 65536) + M+0x18;
     C+0x20 = trunc64(*oz* × 65536) + (M+0x20 − *dz*).
  4. The same with M's +0xb10, +0xb18, +0xb20 into C's +0xb10, +0xb18, +0xb20 (same offsets,
     *dz* recomputed identically).
  5. Copy M's +0x28, +0x2c, +0x30, +0x990, +0x998, +0x99c into C.
  6. Zero C's +0x34 through +0x4b (velocity and movement intent).

### Step 3. Ability 0x91: being carried
Only if the ability is 0x91, +0x1a8 is non-zero, and the creature K with that id exists and
has health > 0:
1. C+0x10 = K+0x10 + trunc64(C+0x174 × 65536); C+0x18 = K+0x18 + trunc64(C+0x178 × 65536);
   C+0x20 = K+0x20 + trunc64(C+0x17c × 65536).
2. The same from K's +0xb10/+0xb18/+0xb20 into C's +0xb10/+0xb18/+0xb20.
3. If 100.0 > K+0x138, K+0x138 = 100.0 (this writes the **carrier's** record).
Otherwise nothing (the ability is not cancelled here).

### Step 4. Ability 0x8c: circling a focus target
Only if the ability is 0x8c. Let *T* = float of duration(C) (§4.4; 0x8c's duration). Let F be
the creature whose id is +0x1a8.
* **Ends** if not (*T* > +0x6c), or +0x1a8 is 0, or F is not found, or F's health is not
  > 0. Then: the text "FOCUS ", the id and a newline are written to standard output (no
  effect on the simulation); the ability becomes 0; and if +0x1a8 is non-zero and F exists
  with health > 0, C's +0x10, +0x18, +0x20 are set to F's.
* **Otherwise:**
  1. (*ox*, *oy*, *oz*) = mount offset for F's race (§4.8); *oz* = *oz* + F+0x8c × 0.75.
  2. Angle *a* = in double: (F+0x30 widened to double) × π (0x42e758) / 180.0 (0x42e778)
     + π/2 (0x42e740); then rounded to float.
  3. *r* = (F+0x84 unless F+0x84 > F+0x88, in which case F+0x88) × −0.5 (0x42e80c).
  4. Offsets from the angle: *ax* = cosf(*a*) × *r*; *ay* = *r* × sinf(*a*); *az* = *r* × 0.0.
  5. *dz* = trunc64((((F+0x8c × 0.5) − (C+0x8c × 0.5)) + 0.01) × 65536.0).
  6. C+0x10 = trunc64(*ax* × 65536) + trunc64(*ox* × 65536) + F+0x10;
     C+0x18 = trunc64(*ay* × 65536) + trunc64(*oy* × 65536) + F+0x18;
     C+0x20 = trunc64(*az* × 65536) + trunc64(*oz* × 65536) + (F+0x20 − *dz*).
     (64-bit integer sums; their order does not matter.)
  7. The same with F's +0xb10/+0xb18/+0xb20 into C's.
  8. Copy F's +0x28, +0x2c, +0x30, +0x990 into C. Zero C's +0x998..+0x99f, +0x34..+0x57
     and +0x12c (the dodge timer).

### Step 5. Hostility kind 6: riding along with the owner
Only if +0x60 is 6, the owner O (id +0x1a0) exists and O's ability is 0x76:
1. *dz* = trunc64((((C+0x8c × 0.5) − (O+0x8c × 0.5)) + 0.01) × 65536.0) (note: C first).
2. C+0xb10 = O+0xb10; C+0xb18 = O+0xb18; C+0xb20 = O+0xb20 + *dz*.
3. Copy O's +0xb34, +0xb38, +0xb3c.
4. C+0x10 = O+0x10; C+0x18 = O+0x18; C+0x20 = O+0x20 + *dz*.
5. Copy O's +0x28, +0x2c, +0x30. C+0xb58 = 0 (4 bytes). Copy O's +0x990, +0x998.
   C+0x99c = (O+0x99c × O+0x84) / C+0x84.
6. Zero C's +0x34 through +0x4b.

### Step 6. Races that drain themselves
Let *h* = health read now. Only if *h* > 0:
* **Race 0x90 (bomb)**, authoritative only: health = *h* − ((maxHealth(C) × 0.00025) × *dt*).
  If 0 > health: health = 0 and +0x6c = 0.
* **Race 0x11d (water bubble)**, authoritative only: mana (+0x188) = mana − *dt* × 0.00005.
  If 0.1 > mana: health = 0, +0x6c = 0, and append a sound request: C's position, id 0x2d,
  pitch 1.0, volume 1.0, owner 0 (no rand).

### Step 7. Short timers
1. For +0x134, then +0x138, then +0x13c: subtract *dt*; if 0 > result, set it to 0.
2. If 0 > +0x140: +0x140 = ((*dt* × 0.001) / 180.0) + +0x140; if the result > 0, set it to 0.
3. +0x994: *k* = the smoothing factor 0xa7a0(*dt*, 0.0075) (§4.9); +0x994 = (*k* × (0.0 −
   old)) + old.

### Step 8. Buffs
1. If not (0 < health) — i.e. health ≤ 0; a NaN health does not clear — remove **all** buffs
   (the list is emptied, count 0).
2. Walk the buff list front to back. For each entry (the next entry is read after the
   entry has been handled, so a hit that appends buffs extends the walk):
   1. duration = duration − *dt* (stored). Call the stored value *d*.
   2. **Types 0x1e and 0x1f:** if 0.00001 > strength, mark the entry for removal and go to
      the next entry. Otherwise handle its effect (2.4; these types have none). Their
      duration never removes them.
   3. **Every other type:** if 0 < *d*, handle its effect (2.4). Otherwise (*d* ≤ 0 or NaN):
      if the type is 0x0f (shadow-clone aura), health = 0; mark the entry for removal.
   4. **Effects:**
      * **Type 1:** if the stun timer (+0x130) > 0, set it to 0.
      * **Type 4 (poison):** authoritative only. Let *a* = trunc32(*d*) / 400 and
        *b* = trunc32(*dt* + *d*) / 400 (the sum in float). If *a* ≠ *b* and strength > 0,
        make a hit request (§2.1) with damage = strength and attacker = the creature with
        the buff's source id (or none). So a poison buff hits once every 400 ms of its
        remaining duration.
      * **Type 0x11 (17, heroic-shout heal):** authoritative only. If trunc32(*d*) / 1000
        and trunc32(*dt* + *d*) / 1000 differ: *heal* = maxHealth(C) × strength. Append a
        damage number: from = the buff's source id, to = C's id, value = −*heal* (sign bit
        flipped), +0x14 = 0, position = C's, +0x44/+0x45/+0x46 = 0. Then health = *heal* +
        health; if health > maxHealth(C) (recomputed), health = maxHealth(C) (again
        recomputed).
      * Any other type: nothing.
   Entries marked for removal are still in the list during the walk, so maxHealth() inside
   the walk still sees them.
3. After the walk, remove the marked entries (in the order marked), keeping the order of
   the others; decrement +0x988 once per removal.

### Step 9. Health follows maximum health (non-players on the authoritative world)
Only if authoritative and the hostility kind is not 0:
*newMax* = maxHealth(C). If *oldMax* > 0: health = (*newMax* / *oldMax*) × health; then if
*newMax* > health keep health, otherwise health = *newMax*.

### Step 10. Stun
stun (+0x130) = stun − *dt*. If the result > 0: +0x6c = 0 and the ability = 0. (The stun is
not clamped; it keeps going below 0.)

### Step 11. The ability clock
The clock runs only if all of:
* C is local, **or** (C's hostility kind is not 0 **and** authoritative);
* health > 0;
* +0x6c ≥ 0, **or** the ability is 0. (A negative time with an ability set — a queued
  start — does not advance; a NaN time with an ability set does not advance.)

When it runs: +0x6c = *dt* + +0x6c. Then, if the ability is 0x37 and +0x6c > 400 (float of
0x4ead0's 400) and 900 > +0x6c (duration, §4.4) and 500 > +0x6c (400 + 100 from 0x660d0),
set +0x3c (vertical velocity) to −60.0. (That is, during 400 < t < 500 ms of ability 0x37.)

### Step 12. End-of-attack pass (only when the ability is 0)
If the ability (+0x68) is 0 now:
1. Walk set B (+0x9c8) in ascending id order. For each id that is **not** in set A
   (+0x9b8) and whose creature V exists:
   1. Evade reward on V (0x4ee50, §4.10).
   2. Append a damage number: from = C's id, to = V's id, value 0.0, +0x14 = 0, position =
      V's, +0x44 = 0, +0x45 = 4, +0x46 = 0. (A "missed" marker on V.)
2. Empty set A and set B.

### Step 13. Ability 0x80: following the owner
If the ability is 0x80 and the owner (+0x1a0) does not exist or not (0 < owner's health),
the ability becomes 0.

### Step 14. Mana (+0x188)
* **Ability 0x92:** mana = mana − *dt* × 0.0002. If not (0 < mana), mana = 0.
* **Else, if C uses mana** (0x5edd0, §4.5):
  * ability 0x1d: no change;
  * ability in { 0x20, 0x22, 0x23, 0x26, 0x2c, 0x2e, 0x30, 0x69, 0x6a, 0x86, 0x87, 0x88 }
    and float of duration(C) > +0x6c: no change;
  * otherwise mana = manaFactor(C) × (*dt* × 0.0001) + mana (manaFactor §4.5).
* **Else** (does not use mana): if the ability is one of { 0x09, 0x19, 0x1a, 0x1c, 0x3c,
  0x40, 0x41 }: no change; otherwise mana = mana − *dt* × 0.00005.

Then: if 0 > mana, mana = 0; if mana > 1.0, mana = 1.0.

### Step 15. Block power (+0x184)
Only if the hostility kind is not 7:
* If C is not blocking (0x5ed30, §4.5) **or** the ability is 0x5f or 0x9f:
  +0x184 = *dt* / 2000.0 + +0x184.
* If +0x184 > 1.0, set it to 1.0.

### Step 16. Movement intent
Let *m* = ((+0x40 × +0x40) + (+0x44 × +0x44)) + (+0x48 × +0x48), recomputed each time
below.
1. If *m* > 0, clear bit 0x400 of +0x128.
2. If the ability is 0x57, 0x58, 0x59 or 0x5a and *m* > 0, the ability becomes 0.

### Step 17. Ability 0x7a: local player only
If the ability is 0x7a, C is local, C does **not** carry the inventory item type 0x18
subtype 0x12 (§4.6), bit 8 of +0x5c is clear, *m* > 0, the ability is not 0x78, and
(+0x5c & 5) == 4 (bit 2 set, bit 0 clear): stamina = stamina − *dt* × 0.00025. If not
(0 < stamina): stamina = 0, ability = 0, +0x134 = 800.0.

### Step 18. Ability 0x8a ends on landing
If the ability is 0x8a and bit 0 of +0x5c is set, the ability becomes 0.

### Step 19. Stealth (+0x18c)
C is **hiding** if any of:
* the ability is 0x50;
* class 4, specialisation 0, and dodge timer (+0x12c) > 0;
* class 2, specialisation 0, and (the ability is 0x19, 0x1a or 0x1c, or the dodge timer
  > 0), and the skill level 0x65690(C, 0x50) > 0 (always 1000, so always true).

**If hiding:**
* If bit 1 (in water) of +0x5c is set: if the ability is 0x50 it becomes 0. Stealth is not
  changed. Go to step 20.
* Otherwise:
  1. *u* = powf(((W+0x0c + W+0x0c) / 86400000.0) − 1.0, 2.0);
     *night* = powf(1.0 − *u*, 2.0).
  2. *L* = light at C's block (0xdab20 / 0xda830 with channel 0, §4.7); *lit* =
     ((float of *L*) / 255.0) × *night*.
  3. *base* = 0x656a0(1000) = 1.1809342 (bit pattern 0x3f9728da; computed as
     (1 − 1 / ((1000 − 1) / 50 + 1)) × 0.4 + 0.8 in float).
  4. If the dodge timer > 0: *rate* = *base* − *lit* × 0.75.
     Otherwise: *speed* = sqrtf(((+0x34²) + (+0x38²)) + (+0x3c²));
     *rate* = (*base* − *lit* × 0.75) − *speed* × 0.05.
  5. stealth = *rate* × (*dt* × 0.0005) + stealth. If 0 > stealth, stealth = 0. If
     stealth > 1.0, stealth = 1.0.

**If not hiding:**
* If bit 0x400 of +0x128 is set: stealth = *dt* × 0.00005 + stealth; if > 1.0, 1.0.
* Otherwise: stealth = stealth − *dt* × 0.0005; if 0 > stealth, 0.

### Step 20. Buff 3 forces full stealth
If C has a buff of type 3, stealth = 1.0.

### Step 21. Mana from stealth
mana = manaFactor(C) × ((*dt* × 0.00025) × stealth) + mana. If mana > 1.0, mana = 1.0.
(No lower clamp here.)

### Step 22. Stamina (+0x9a0): the first matching case applies
1. **Gliding** — ability 0x78 and bit 0 of +0x5c clear:
   * if bit 0x40 of +0x128 is set: stamina = stamina − *dt* × 0.0001625;
   * else if stamina > 0.001: *f* = +0x3c × −0.1; if not (*f* > 0) use 0; if not
     (1.0 > that) use 1.0. stamina = (*dt* × 0.0001) × *f* + stamina;
   * else no change.
2. **Ability 0x7a:** no change.
3. **Ability 0x51 while moving** — *m* > 0.01: stamina = stamina − (*dt* × 0.001) × 0.25.
4. **Ability 0x5f:** stamina = stamina − *dt* × 0.00025; if not (0 < stamina), the ability
   becomes 0.
5. **Local player swimming** — C is local and (+0x5c & 0x12) == 2 (in water, bit 4 clear):
   stamina = stamina − swimDrain(C) × (*dt* × 0.001) (swimDrain §4.5). If not
   (0 < stamina), and the local player's health > 0, and
   trunc32(W+0x440 − *dt*) / 500 ≠ trunc32(W+0x440) / 500, then **drown**:
   1. *loss* = in double: (maxHealth(C) widened) × 0.05 (0x438200), rounded to float.
   2. health = health − *loss*.
   3. **rand:** append a sound request: C's position, id 0x2f, pitch = rand × 0.1 / 32767
      + 0.5, volume 1.0, owner 0.
   4. If 0 > health, health = 0.
   5. Append a damage number: from = 0, to = C's id, value = *loss*, +0x14 = 0, position =
      C's, +0x44/+0x45/+0x46 = 0.
6. **Ability 0x93:** no change.
7. **Regeneration** — only if +0x134 == 0.0 exactly (NaN fails) and (the ability is not
   0x9f, or +0x6c > float of duration(C), which is 1000 for 0x9f):
   stamina = staminaFactor(C) × (*dt* × 0.0001) + stamina (staminaFactor §4.5).
8. Otherwise no change.

Then: if 0 > stamina, 0; if stamina > 1.0, 1.0.

Notes: case 3 applies only when moving; a non-moving 0x51 falls through to cases 4–8. A
gliding creature on the ground (0x78 with bit 0 set) also falls through. Case 5 is taken
whenever its condition holds, so a swimming local player never regenerates.

### Step 23. Dodge sound
+0xadc = the dodge timer (+0x12c). If ((authoritative and kind ≠ 0) or C is local) and the
dodge timer ≥ 590.0 and 590.0 > (dodge timer − *dt*): **rand:** append a sound request: C's
position, id 0x1a, pitch = rand × 0.2 / 32767 + 0.9, volume 1.0, owner = C's id.

### Step 24. Dodge timer countdown
If the dodge timer is not exactly 0 (NaN counts as not 0), subtract *dt*. Then if 0 > it,
set it to 0.

---

## 4. Callees

### 4.1 Equipment slots
Item slots are checked by item type; a slot whose type differs counts as empty:

| Slot | Required type | In maxHealth | In attack speed | In staminaFactor |
|---|---|---|---|---|
| +0x638 | 3 (weapon) | yes | yes | yes |
| +0x6d8 | 3 (weapon) | yes | yes | yes |
| +0x3b8 | 4 | yes | yes | yes |
| +0x458 | 6 | yes | yes | yes |
| +0x4f8 | 5 | yes | yes | yes |
| +0x598 | 7 | yes | yes | yes |
| +0x318 | 8 | – | yes | yes |
| +0x778 | 9 | – | yes | yes |
| +0x818 | 9 | – | yes | yes |

Sums are made in the table's order (top to bottom), each added to the running float.

"Two-handed" means item type 3 with subtype < 24 and subtype in {5, 6, 7, 8, 10, 11, 15, 16,
17, 18, 23} (mask 0x878de0).

"xorshift(*v*)" on a 32-bit unsigned *v*: *v* ^= *v* << 13; *v* ^= *v* >> 17; *v* ^= *v* << 5
(all 32-bit).

**Item power scale (0x109c50, third argument always 0 here).**
1. The reference region is C's (+0x150, +0x154) if C's hostility kind is 0, 4 or 6;
   otherwise the item's own region.
2. *lvl* (0x109720): if (the item's type is 3..9 or 0x1a) and (modifier mod ((rarity + 1)
   × 10) == 0, unsigned) and the Chebyshev distance between the reference region and the
   item's region is < 2, *lvl* = rarity. Otherwise *lvl* = rarity if the reference region
   equals the item's region exactly, else −1.
3. *r* = xorshift(modifier) mod 1000.
4. Result = powf(2.0, (float of *lvl*) + (float of *r*) / 2000.0). (The game adds
   (0 × 0.5) / 16-or-32 = 0.0 first; it changes nothing.)

**Item health (0x1097b0).** 0 unless the type is 3..7.
1. *u* = xorshift(modifier + 0x2673); *f* = (((float of (*u* mod 100000)) × 0.25) /
   100000.0 + 1.0) × 10.0.
2. Type 4: *f* = *f* + *f*, and then if the subtype is 1, *f* = *f* + *f* again.
3. Two-handed: *f* = *f* + *f*.
4. *mat* = 2.0 for material 1, 1.5 for 0x1b, 1.75 for 0x1c, else 1.0.
5. Result = (scale × *f*) × *mat*.

**Item attack speed (0x10a490).** 0 unless the type is 3..9.
1. *f* = 0.02 if two-handed or the type is 4, 8 or 9; else 0.01. If type 4 and subtype 1,
   *f* = *f* + *f*.
2. *g* = (float of (modifier mod 21)) / 20.0.
3. Material 4, 0x21, 0x22, 0x23: *g* = *g* × 0.9. Material 6: *f* = *f* × 0.5. Material
   0x0c: *g* = *g* + 1.0. Material 0x1d: *g* = *g* + 0.5.
4. *v* = (((scale × 0.0625) × *f*) × *g*) × 3.0. Result = *v* if 0.001 ≤ *v*, else 0.

**Item stamina regeneration (0x109f30).** 0 unless the type is 4..9 or 0x1a.
1. *u* = xorshift(modifier + 0x157); *f* = (((float of (*u* mod 100000)) × 0.25) /
   100000.0 + 1.0) × 0.01. Type 4: *f* = *f* + *f*.
2. *g* = (float of ((modifier × 8) mod 21, 32-bit unsigned)) / 20.0. Material 0x1b or 0x1f:
   + 0.5; 0x1c or 0x1e: + 1.0; 0x1d: + 0.1; 0x21: + 0.4; 0x22: + 0.75.
3. *v* = in double: ((scale × *f*) × *g*, float) widened × 1.5, rounded to float. Result =
   *v* if 0.001 ≤ *v*, else 0.

### 4.2 Maximum health (0x5fac0)
1. 100.0, or 125.0 if class 1 and specialisation 1.
2. Add the item health of the six slots (§4.1).
3. × 10.0 if bit 9 of +0x80; × 1.5 if bit 17; × 15.0 if bit 18.
4. × the strength of the first buff of type 0x16, if any.
5. × the strength of **every** buff of type 0x20, in list order.
6. × 1.1 if bit 26.

### 4.3 Attack-speed factor (0x66460)
1. *s* = 1.0. Let *T* = combo threshold (0x5f9e0) and *c* = min(float of +0x70 / float of
   *T*, 1.0) (if the quotient is not below 1.0, 1.0).
2. Class 1 spec 0: *s* = *c* × 0.5 + 1.0. Class 3 spec 1: *s* = *s* + *c*. Class 2 spec 0:
   *s* = *s* + *c* × 0.5. (At most one applies.)
3. Add the item attack speed of all nine slots (§4.1), in order.
4. If C has a buff of type 12: *s* = *s* + *s*.
5. × 0.75 if bit 9 of +0x80; × 0.6 if bit 18; × 1.1 if bit 26.

Combo threshold (0x5f9e0): if the item at +0x6d8 is type 3, by its subtype (+0x6dc): 3, 4,
10 → 50; 6 → 60; 8 → 80; 11, 12 → 20; 15, 16, 17 → 15; else 30. Without such a weapon: 15
if bit 4 of +0x80 is set, else 30.

### 4.4 Ability duration (0x4f220), for the abilities this routine asks about
Duration = three integer parts added: the effect time (0x4ead0), the middle part (0x660d0)
and the recovery part. Each part that depends on *S* (the attack-speed factor, §4.3) is
trunc32(constant / *S*), computed separately.

| Ability | Effect | Middle | Recovery |
|---|---|---|---|
| 0x20, 0x22 | 1600/*S* | 1200 | 100/*S* |
| 0x23 | 1600/*S* | 1200/*S* | 100/*S* |
| 0x26, 0x2c | 600/*S* if the +0x6d8 item's subtype is 12, else 1200/*S* | 100/*S* | 100/*S* |
| 0x2e, 0x30 | 1200/*S* | 500/*S* | 100/*S* |
| 0x37 | 400 | 100 | 400 |
| 0x5f | 0 | 20000 | 0 |
| 0x69, 0x6a | 1000/*S* | 2000 | 100/*S* |
| 0x86, 0x87, 0x88 | 2000/*S* | 0 | 100/*S* |
| 0x8c | 100 | 5000/*S* | 500/*S* |
| 0x9f | 0 | 1000 | 0 |

The routine compares float(duration) with the float time +0x6c.

### 4.5 Small predicates and factors
* **Uses mana (0x5edd0):** true if class is 3, or class is 0x97 or 0x98, or race is 0x75 or
  0x56, or the +0x6d8 item is type 3 with subtype 10, 11 or 12.
* **Blocking (0x5ed30):** false if +0x134 > 0. Otherwise true if the ability is 9; or 0x5f
  while +0x6c < float of duration (20000); or 0x6e; or 0x3c or 0x40 for class 1 spec 1.
  Otherwise false.
* **manaFactor (0x5f8d0):** 1.0 − (strength of the first type-0x22 buff × 0.1), or 1.0
  without one.
* **staminaFactor (0x647b0):** 1.0 plus the item stamina regeneration of all nine slots, in
  order.
* **swimDrain (0x51190):** 0 if the hostility kind is not 0; else
  (1.0 − (1.0 − 1.0 / ((float of +0xb64) / 20.0 + 1.0))) × 0.1, evaluated exactly in that
  order.
* **Skill level (0x65690):** always 1000.

### 4.6 Inventory item test (0x5e890 with 0x4df60)
True when the total count of inventory slots holding an item equal to a template is > 0
(type 4 is always true). Inventory: +0x9f0..+0x9f8 is a sequence of tabs, each a sequence
of 0xa4-byte slots (i32 count, then the 0xa0-byte item). Template: type 0x18, subtype as
asked (0x12 here), modifier 0, region = C's +0x150/+0x154, rarity 0, +0x18 = 0, material 0,
no attributes. Two items are equal when type, subtype, modifier, both region values, +0x18,
material, attribute count and rarity are equal and, for each attribute, its 4 bytes are
equal.

### 4.7 Blocks and light
* **Block at (0xeab0 → 0xe9a0):** the block coordinates of the three positions; a missing
  zone (or null zone object) gives the default block c8 c8 c8 00 01 00 (type 1); otherwise
  the zone's column (0xeb90). Byte 4 of the six-byte block is its type.
* **Light (0xdab20 → 0xda830, channel 0):** if C's zone is missing or null, 255. Otherwise
  read the block; while its type is 10, 12 or 19, move one block up (+1 in z) and read
  again (a missing zone there gives the default block). Then if the type is 0 or 2: let *c*
  = byte 0 of the block and *e* = byte 5; the light is max(5, *e*) if *c* < 5, else
  max(*c*, *e*). For any other type the light is byte 5.

### 4.8 Mount offsets (0x65040), by the carrier's race
(x, y, z) is (0, 0, *z*) with *z* from this table, except race 0xd2: (0, −0.3, −0.3).

| Race | z | Race | z |
|---|---|---|---|
| 0x1a, 0x1e | −0.9 (0xbf666666) | 0xd3 | −0.6 (0xbf19999a) |
| 0x21 | −0.2 (0xbe4ccccd) | 0xd5 | 0.25 |
| 0x4a, 0xc8, 0xc9 | −0.3 (0xbe99999a) | 0xd7 | 0.4 (0x3ecccccd) |
| 0x4b, 0x117 | 0.1 (0x3dcccccd) | 0x106 | −1.0 |
| 0x97 | −0.35 (0xbeb33333) | 0x107 | −2.0 |
| 0x125, 0x126 | −0.4 (0xbecccccd) | 0x10e | −1.6 (0xbfcccccd) |
| 0x127 | 0.4 (0x3ecccccd) | any other | 0 |

### 4.9 Smoothing factor (0xa7a0), all in double
Arguments: *t* (*dt* widened to double) and *k* (0.0075 as float, widened). *n* =
trunc32(*t* × 100.0). *q* = *k* × 0.02. *acc* = 0. For *i* = 0 … *n* − 1:
*acc* = *acc* + (((float of *i* as double) + 1.0) / (double of *n*) − *acc*) × *q*.
Result: *acc* rounded to float. (With *n* ≤ 0 the result is 0.)

### 4.10 Evade reward (0x4ee50), on the other creature V
If V's dodge timer (+0x12c) is not 0 and V is class 4 specialisation 1:
1. V's mana = V's mana + 0.25; if > 1.0, 1.0.
2. Give V (refresh flag clear; type 11 refreshes anyway, matching type and source) the buff:
   type 0x0b, strength 0, duration 30000.0 (0x432954), source 0. The padding bytes are
   uninitialised.
3. Append a buff-change notice: source = V's id, target = V's id, that buff, flag 0.

### 4.11 Containers
0x3c9c0 empties a list; 0x3d6c0 empties an ordered set; 0xa18b0, 0xa16f0 and 0x7380 append
to a world list (sound, damage number, notice); 0x29c490, 0x296090, 0x36bc40, 0xdabe0 manage
the temporary list of buff entries to remove. 0x12700 looks up the zone hash map.

---

## 5. rand() order

At most three draws are made directly, always in this order, each only if its condition
holds: race 0x12e sound (step 1e) → any draws inside poison hits (step 8, one hit per
qualifying poison buff, in buff-list order) → drowning sound (step 22, case 5) → dodge sound (step
23).

---

## 6. Constants

| RVA | Value | Use |
|---|---|---|
| 0x42e5f8 | 65536.0 | blocks → position units |
| 0x42e5bc | 100.0 | race 0x12e ceiling; base health |
| 0x42e51c | 0.01 | race 0x12e rate; height offset; 0x51 intent threshold; item speed |
| 0x4ac964 | 0.5 | |
| 0x4ac980 | 1.0 | |
| 0x42e5e8 | 10000.0 | group buff duration |
| 0x42e52c | 0.4 | pitch range (0x3c); stealth base |
| 0x42e5f0 | 32767.0 | rand divisor |
| 0x42e530 | 0.8 | pitch base (0x3c); stealth base |
| 0x42e710 | 0.75 | mount height factor; stealth light factor |
| 0x42e758 / 0x42e778 / 0x42e740 | π / 180.0 / π/2 (doubles) | focus angle |
| 0x42e80c | −0.5 | focus radius |
| 0x472178 | 0.00025 | bomb drain; 0x7a and 0x5f drains; stealth→mana |
| 0x472168 | 0.00005 | bubble drain; non-caster mana drain; stealth gain |
| 0x42e528 | 0.1 | bubble threshold; drown pitch range |
| 0x42e514 | 0.001 | +0x140 rate; glide threshold; swim drain; 0x51 drain |
| 0x42e7d4 | 180.0 | +0x140 rate divisor |
| 0x42e6e4 | 0.0075 | +0x994 smoothing |
| 0x43ef28 | 0.00001 | buff 0x1e/0x1f removal |
| 0x4acb30 | sign mask 0x80000000 | heal number sign |
| 0x4ac954 | 0.0001 | mana and stamina regeneration; glide gain |
| 0x472174 | 0.0002 | ability 0x92 mana drain |
| 0x42e5d8 | 2000.0 | block power rate |
| 0x4ac9d0 | 2.0 | powf exponent |
| 0x438028 | 86 400 000.0 | ms per day |
| 0x42e7d8 | 255.0 | light divisor |
| 0x42e524 | 0.05 | stealth speed penalty |
| 0x43ef2c | 0.0005 | stealth rates |
| 0x42e7f8 | −0.1 | glide factor |
| 0x472170 | 0.0001625 | glide drain with +0x128 bit 0x40 |
| 0x4ac95c | 0.25 | 0x51 drain factor; item rolls |
| 0x438200 | 0.05 (double) | drowning damage |
| 0x472590 | 590.0 | dodge sound threshold |
| 0x42e700 | 0.2 | dodge pitch range |
| 0x42e534 | 0.9 | dodge pitch base; material factor |
| 0x4aca18 / 0x42e540 / 0x4ac9b8 | 100.0 / 0.02 / 1.0 (doubles) | smoothing |
| 0x432938 | 125.0 | warrior spec 1 base health |
| 0x42e598 | 10.0 | |
| 0x42e548 | 1.5 | |
| 0x42e7a4 | 15.0 | |
| 0x42e538 | 1.1 | |
| 0x42e708 | 0.6 | |
| 0x43800c | 100000.0 | |
| 0x4328cc | 1.75 | |
| 0x42e5a0 | 20.0 | |
| 0x42e6e8 | 0.02 | |
| 0x4328a4 | 0.0625 | |
| 0x42e580 | 3.0 | |
| 0x441c78 | 1.5 (double) | |
| 0x432954 | 30000.0 | evade buff duration |
| 0x42e5b0, 0x42e5c0, 0x42e5c8, 0x42e5d4, 0x42e5e4, 0x42e7e0, 0x42e7e8, 0x42e7ec, 0x42e938, 0x432940, 0x432944, 0x432948, 0x43293c, 0x43294c | 50, 200, 500, 1000, 5000, 300, 400, 600, 150, 800, 1200, 1500, 250, 1600 | duration tables |
| immediates | 100.0 (0x42c80000), −60.0 (0xc2700000), 800.0 (0x44480000), 1.0 | |

---

## 7. Verification

The recordings `Extract/oracle/upkeep-idle.bin` and `upkeep-fight.bin` hold real calls of
0x2de4c0 captured by ZoneExtractor's `update.request` ("<count> 2de4c0"), and
`EngineTests upkeeporacle <files>` replays them.

**File:** magic "CWU3" or "CWU4", a 4-byte world seed, then one record per call, each
preceded by its u32 size.

**A call record, in order:**
1. u64 creature address; u8 "is the local player"; u8 W+0x358; 2 padding; f32 *dt*.
2. The creature's 0x15f8 bytes **before** the call, then **after** it.
3. f32 world time (W+0x440).
4. The buffs before (u32 count, 0x18 bytes each).
5. The world creature list in order: u32 count; per creature its u64 address and a u8
   "near" (within 40 blocks horizontally); near creatures are followed by their first 0x200
   bytes. (The harness places the others 100 000 blocks away with everything else zero.)
6. Objects near it (u32 count; u64 address + 0x200 bytes each); then the creature's effect
   objects (u32 count, 0x20 bytes each).
7. The records it names, at +0x1a8, +0x1a0 and +0x9d8: a u64 address each; when non-zero
   and not the creature itself, 0x9b0 bytes follow.
8. A block box: i32 x0, y0, z0, nx, ny, nz; for each x then y: u8 "zone loaded", then nz
   six-byte blocks.
9. Every rand() value the call drew (u32 count, i32 each), including draws inside hits.
10. u32 RVA (0x2de4c0 for these) and u32 size of the rest, which holds:
    * the buffs before again; the "sides" before: aggro map (+0xb88) and the damage map
      (+0xb98) as (u32 count, (i64, f32) pairs), set A (+0x9b8) and set B (+0x9c8) as
      (u32 count, i64s);
    * the world lists before: aggro notes (W+0x140, 0x18 each), buff notices (W+0x150,
      0x30), calls (W+0x170, 8), damage numbers (W+0x90, 0x48), sounds (W+0xa0, 0x30),
      particles (W+0xb0, 0x48), and in CWU4 effect objects (W+0x60, 0x80); each a u32
      count and the entries;
    * the buffs after, the sides after, and per list its count after followed by only the
      entries it gained;
    * the hits applied during the call: u32 count, 79 bytes each (target i64, attacker i64,
      damage f32, critical u8, flag u8, float f32, position 24 bytes, push 12 bytes, type
      i32, ability i32, extra 8 bytes, last u8);
    * newer recordings then hold the dead set (u32 count, i64s), a u8 "C's zone found", the
      zone's links (u32 count; each i64 key, u8 found, u32 count, i64 ids), and per hit its
      rand span (u32 draws before, u32 draws after, u8 result).

**How to check an implementation with them:**
1. Skip calls of the local player (the harness does).
2. Build C from the "before" bytes, its buffs and sides; build the other creatures from
   their recorded heads and named records; use the world flag, *dt* and world time given.
3. Feed rand() the recorded values in order. When the implementation makes a hit request,
   skip that hit's recorded draws (after − before) and treat its recorded result as the
   outcome; the implementation must not apply the hit itself beyond that.
4. Run the upkeep once.
5. Expect, exactly: the record's 0x15f8 bytes equal "after" (compared in 4-byte words, with
   the container counts +0x988, +0x9c0, +0x9d0 taken from the implementation's
   containers); the buff list equal to "buffs after" (0x18 bytes each); the maps and sets
   equal to "sides after"; the buff notices, damage numbers and sounds it appended equal
   to what the lists gained (masking damage-number bytes +0x15..+0x17 and +0x47, and sound
   bytes +0x24..+0x27); nothing appended to aggro notes, calls or particles; exactly the
   recorded number of rand() draws consumed; and, for calls with hits, the hit requests
   equal the recorded hits in the 79-byte form above.
6. Treat a call as "not checkable" (not a failure) when it reads a block outside the
   recorded box (race 0x12e's probe), needs the light or the time of day (the hiding
   branch of step 19: neither W+0x0c nor light is recorded), or reads parts of another
   creature that were not recorded.

**What the recordings cannot check:** the hiding branch of stealth; fields of others beyond
their recorded bytes — the mount, carrier and owner steps read +0xb10..+0xb3c of the other
creature, beyond the 0x9b0 recorded; writes to other creatures (step 3's +0x138 on the
carrier, the evade reward of §4.10) are not compared; others' buffs (they affect nothing
read here except through the hit routine); the local player's own branches (steps 17 and
22, case 5); creatures more than 40 blocks away that are in the aggro map (they are recorded
without health, so step 1a would not count them even if the game did).

---

## Open questions

1. **Uninitialised bytes.** The group buff of step 1a and the evade buff of §4.10 are built
   on the stack with only type, strength, duration and source set: buff bytes +1..+3 and
   +0xc..+0xf, and notice bytes +0x29..+0x2f, hold whatever the stack held. Give-buff and
   the notice list copy them verbatim, so the recorded buff lists and notices may contain
   arbitrary bytes there. Comparisons should mask them; the "after" values cannot be
   reproduced.
2. **powf, sinf, cosf exactness.** Bit-exact results need the UCRT's implementations (or a
   proof that the values used here agree). powf here is only ever 2^x (item scaling) and
   x² (stealth); whether x × x reproduces the UCRT's powf(x, 2.0) in every case was not
   checked.
3. **The hit routine 0x29eb60** is not specified here; its effects on the target (health,
   buffs, death) and its draws come from its own specification.
4. **Meanings not established:** +0x138, +0x13c, +0x140, +0x994, +0x990..+0x99c,
   +0xb10..+0xb20 (a second position, perhaps the drawn one), +0xb34..+0xb3c, +0xb58,
   +0x128 bits 0x2, 0x40, 0x100, 0x400, +0x5c bits 2, 4, 8, and what race 0x12e and its
   +0x17c / +0x19c are (a creature that rises while there is air or water above it,
   clicking every 2 units).
5. **Ability names** (0x37, 0x50, 0x51, 0x57–0x5a, 0x5f, 0x76, 0x77, 0x78, 0x7a, 0x80,
   0x8a, 0x8c, 0x91, 0x92, 0x93, 0x9f) are inferred only from what this routine does with
   them (0x77 riding, 0x78 gliding, 0x8c circling a target, 0x91 carried, 0x50 sneaking).
6. **Inventory item type 0x18 subtype 0x12** (step 17) was not identified.
7. **Set A / set B (+0x9b8 / +0x9c8)** are filled elsewhere; reading B-minus-A as "targets
   the last attack missed" fits step 12 but was not traced to the code that fills them.
8. **The zone-cleared flag (step 1d)** and the dead set: who fills W+0x20 and the zones'
   link and id lists, and who reads +0x128 bit 0x100, were not traced.
9. **Where abilities end by duration** is not in this routine; presumably 0x2c5250 (not
   examined here).
10. **Item field names** (rarity +0x14, material +0x1c) follow common usage; only the bytes
    are certain.
