# Zone objects and the other interaction kinds (0x2c3710, 0x2d3a10)

Behavioural specification of the interaction handler 0x2c3710 for every kind it handles
except 1, 11, 12 and 13 (those are in `skills-and-interactions.md` §2.3–§2.6), and of the
open/shut action 0x2d3a10. Written from the decompiled executable, its assembly and the
binary's constants. Routines are cited by RVA (image base 0x140000000). Offsets are byte
offsets into the record named. Times are ms, distances blocks, world positions 64-bit
integers in 1/65536 of a block. Conventions (float rounding, comparisons, rand()) are those
of `creature-upkeep.md` §0: rand() is the CRT's (0..32767), shared with the whole game.

Kinds covered: **5** (using a zone object: seats and beds, doors, gates and hatches, objects
that take an item, and sleeping at the innkeeper's), **2** (an item of type 0x18 subtype 5
used: opens gates nearby; left open by `creature-abilities-ranged.md`), **3** (activating a
creature), **7** (picking up a ground item), **8** (dropping an item) and **10** (calling the
pet). Any kind not listed here or in `skills-and-interactions.md` is ignored.

---

## 0. Records

### 0.1 The interaction record

As `skills-and-interactions.md` §2.1: a 0xc8-byte record (item +0x00, zone x +0xa0, zone y
+0xa4, object index +0xa8, target creature id +0xb0, kind byte +0xc4, +0xc6 = 1). The
creature handling it is called **U** (the user).

### 0.2 Zone object record (zone +0x60 list, 0xe0 bytes each)

| Offset | Type | Use here |
|---|---|---|
| +0x00 | i32 | object type |
| +0x08, +0x10, +0x18 | i64 ×3 | position |
| +0x28 | u8 | **state**: for doors non-zero means shut; for objects that take an item non-zero means not yet given |
| +0x2c | f32 | **swing time**, ms (the open/shut animation's progress) |
| +0x30 | i64 | **user**: the creature sitting or lying on it (0 = free) |
| +0x38 | 0xa0 | the item it takes (0 type byte = takes none); its rarity is at object +0x4c |

### 0.3 Ground item record (zone +0x30 list, 0xe0 bytes each)

As `use-prompt.md` §0.4: the item +0x00, position +0xa0..+0xb7, flags byte +0xc4 (bit 1 = 2:
not to be picked up automatically), a float +0xc8, +0xd0, +0xd4, owner +0xd8. Here +0xd4 is
read as a 16-bit count.

### 0.4 Zone fields

| Offset | Use |
|---|---|
| +0x30, +0x38 | ground items (begin, end; 0xe0 bytes each) |
| +0x60, +0x68 | zone objects (begin, end; 0xe0 bytes each) |
| +0x450 | ordered map object index (i32, node +0x1c) → item (0xa0 bytes, node +0x20): what a chest holds |

A zone is found by 0x16e00 (world, zone x, zone y): the world's hash map at +0x460 keyed by
zone x (low 32 bits) and zone y (high 32 bits); none when the zone is not loaded. The zone
of a position is floor(floor(p / 65536) / 64) (0x16e50).

### 0.5 Sound request (world +0xa0 list, 0x30 bytes, appended at the end)

Position (3 × i64) +0x00, sound id i32 +0x18, pitch f32 +0x1c, volume f32 +0x20, owner i64
+0x28 (always 0 here). +0x24 is not written.

### 0.6 World fields

| Offset | Use |
|---|---|
| +0x08 | i32: day counter |
| +0x0c | f32: time of day, ms |
| +0x30 | ordered set of i64: creatures whose reward was given (kind 3) |
| +0x40 | list of creatures, in update order |
| +0xa0 | sound requests (§0.5) |
| +0x110 | list of **pick-up notices** (0xd0-byte payloads, §6) |
| +0x160 | list of 4-byte values: **sleep notices** (§2.2) |
| +0x190 | ordered set of zones whose ground items changed (keyed by the zone object's identity) |
| +0x1a0 | ordered set of zones whose objects changed (same keying) |
| +0x358 | remote-client flag |
| +0x450 | creature by id (0xe720) |

### 0.7 Item equality (0x4df60)

Two items are **equal** when their type, subtype (+0x04), modifier (+0x08), region x and y
(+0x0c, +0x10), +0x18, material (+0x1c), attribute count (+0x9d) and rarity (+0x14) are
equal, and so are the first *count* attribute entries, four bytes each from +0x1d. Padding is
not compared.

### 0.8 Random offset

"A random spot around Q" (used by drops) draws r1 = rand(), then r2 = rand(); its x is Q.x +
trunc64((1.0 − (float(r2) + float(r2)) / 32767.0) × 65536.0), its y is Q.y + trunc64((1.0 −
(float(r1) + float(r1)) / 32767.0) × 65536.0), its z is Q.z + trunc64(0.0). Note the second
draw goes to x. Where a drop angle follows, it is a third draw r3: float(r3) × 360.0 /
32767.0.

---

## 1. Where the handler runs and how the list is emptied

0x2c3710 (world, U) runs in the world tick 0x2da6c0 for every living creature, after its
ability update (`skills-and-interactions.md` §2.2). In short:

1. If the world's remote-client flag (+0x358) is set, return; the list is kept (a client
   sends its interactions to the server one by one).
2. Walk U's list (+0xab8) from front to back. Each record is handled by its kind (§2–§8;
   kinds 1, 11, 12, 13 in `skills-and-interactions.md`). Records of other kinds are passed
   over. Nothing is removed during the walk.
3. After the walk, if the remote-client flag is still clear, the whole list is emptied
   (0x3c9c0), handled or not. An interaction that found nothing to do is simply gone.

Producers:

| Kind | Queued by |
|---|---|
| 2 | the item ability for item 0x18 (`creature-abilities-ranged.md` §16) |
| 3 | talking and using a creature (`use-prompt.md` §3.6) |
| 5 | the use key on a zone object (`use-prompt.md` §3.7); creatures taking seats (`creature-ai.md`, and 0x13830 when a creature reaches its seat); the producer that sets a target creature (the innkeeper, §2.2) was not found (Open questions) |
| 7 | the use key on a targeted ground item (`use-prompt.md` §4.2) |
| 8 | clicking with an item on the mouse cursor (`player-control.md` §2 step 1) |
| 10 | the pet key (`player-control.md` §4.8) |

---

## 2. Kind 5: using a zone object

### 2.1 Sneaking ends

If U's ability (+0x68) is 0x50: ability = 0 and U's stealth +0x18c = 0.0. (Always, before
anything else.)

### 2.2 Sleeping at the innkeeper's

Let T = the creature with the record's target id (0xe720 on +0xb0). If T exists, T's class
(+0x14c) is 0x84, and it is night (the time of day t is not strictly between 21,600,000 and
64,800,000; that is, t ≥ 64,800,000 or t ≤ 21,600,000; a NaN time counts as day):

1. A sound request at U's position: id 0x1f, pitch 1.0, volume 0.75.
2. Append a sleep notice (the 32-bit value 0) to world +0x160.
3. World +0x08 (day counter) + 1.
4. Time of day = 25,200,000.0 (7:00).

Nothing else happens for this record (no zone object is touched). Otherwise go on.

### 2.3 Finding the object

The zone at (record +0xa0, +0xa4); if not loaded, stop. The index i = record +0xa8; if i < 0
or i ≥ the number of zone objects, stop. O = object i. The steps below are tried in order and
the first that applies ends the record.

### 2.4 Seats and beds

If O's type is 0x1c, 0x1e, 0x1f, 0x20, 0x5f, 0x60 or 0x73, O's user (+0x30) is 0, and U is
free (U's ability is 0, 0x57, 0x58 or 0x76, or U's ability has finished: +0x6c >
float(0x4f220(U))):

1. old = O +0x30; O +0x30 = U's id; if old ≠ U's id, insert the zone into world +0x1a0.
2. U's ability = 0; U's intent (+0x40, +0x44, +0x48) = (0, 0, 0).

A taken seat (including one U already holds) and a busy U fall through to §2.5 and §2.6,
which do nothing for seat types. Standing up is not done here (Open questions).

### 2.5 Objects with nothing to give: doors, gates and hatches

If O takes no item (its item type byte +0x38 is 0): if O's type is 3, 4, 5, 6, 0x14, 0x15,
0x54, 0x55, 0x56, 0x57 or 0x69, then:

1. s = O +0x28.
2. 0x2d3a10(world, O, new state = 1 if s is 0, else 0) (§3): a shut door opens, an open one
   shuts.
3. If O +0x28 now differs from s, insert the zone into world +0x1a0.

Any other type: nothing.

### 2.6 Objects that take an item: locked doors, chests, braziers

If O takes an item (+0x38 non-zero), O +0x28 is non-zero (not yet given) and the record's
item is equal to O's item (§0.7):

1. 0x2d3a10(world, O, 0) (§3): O +0x28 becomes 0 (opened / given), with its sound.
2. Insert the zone into world +0x1a0 (whether or not the state changed).
3. **Contents:** in the zone's map +0x450, look up i. If there is an entry and its item's type
   is not 0: drop it (0x2a5b70, `drops-and-items.md` §1) at a random spot around O's
   position, with a random angle (§0.8: three draws), scale 1.0, count 1, no creature, owner
   0. The entry stays in the map.
4. If O's type is 0x15 or 0x16: the reward 0x2a7ca0(world, O's position, tier = the rarity
   of O's item, O +0x4c) (`drops-and-items.md`).

The item handed in is not taken from U here (the client removes it when it queues the
interaction, `use-prompt.md` §3.7.1). If O +0x28 is already 0, or the items differ, nothing
happens.

---

## 3. Opening and shutting (0x2d3a10)

0x2d3a10 (world, O, new state *n*):

1. If O +0x28 equals *n*, return (no change, no sound).
2. v = 1000.0 − O +0x2c (floats); O +0x28 = *n*; O +0x2c = v; if 0.0 > v, O +0x2c = 0.0.
   (A swing reversed half-way restarts from where it was: the animation counts the swing
   time back.)
3. The sound, by O's type:

| Type | Sound id |
|---|---|
| below 8 | 0x39 |
| 8, 9, 0x10 | 0x36 |
| 0x11 | 0x38, only when *n* ≠ 0 (shutting); none when opening |
| 0x12 | 0x37, only when *n* ≠ 0; none when opening |
| any other (0x0a..0x0f, 0x13 and up) | 0x39 |

4. If there is a sound: append a sound request at O's position with that id, pitch 1.0,
   volume 1.0, owner 0.

The swing time +0x2c is a float; what advances it toward 1000 is outside this routine (Open
questions).

---

## 4. Kind 2: item 0x18 subtype 5 (the gate opener)

Only when the record's item has type 0x18 and subtype 5; otherwise nothing.

1. For each creature C in the world list (+0x40), in order: if the squared distance from U
   to C (positions, × 1/65536, (x² + y²) + z²) is ≤ 400.0, C's race (+0x64) is 0xf6 and C
   +0x19c is 0: C +0x19c = 1, and a sound request at U's position, id 0x70, pitch 1.0,
   volume 1.0.
2. For zone x from Z(U.x − 0x140000) to Z(U.x + 0x140000), and for each, zone y from
   Z(U.y − 0x140000) to Z(U.y + 0x140000) (Z = 0x16e50; 0x140000 = 20 blocks; x outer, y
   inner, both increasing): if the zone is loaded, for each of its objects O in index order:
   if O's type is 0x10, O +0x28 is non-zero (shut) and the squared distance from O to U is
   < 64.0:
   1. s = O +0x28; 0x2d3a10(world, O, 0) (§3; type 0x10 sounds 0x36 at the gate);
   2. if O +0x28 differs from s, insert the zone into world +0x1a0;
   3. a sound request at U's position, id 0x70, pitch 1.0, volume 1.0.

No rand() is drawn.

---

## 5. Kind 3: activating a creature

T = the creature with the record's target id; if none, nothing.

1. If T's race is 0xf5, or T's flags +0x80 have 0x200000: T +0x19c = 1; if T's race is
   0x12e: T's ability = 0x99, T +0x6c = 0 (a lift starts rising).
2. Else, if T's class is 0xa1 (the arena master) and T +0x19c is not 0xff: if T +0x19c is
   even, it goes up by 1, and the game prints "Arena Master: " and the new value to its
   console. An odd value is left alone (the arena is running).
3. Else, if T's state flags +0x128 have 0x100 (a task done) and T +0x19c is 0:
   1. T +0x19c = 0xff; clear 0x100 in T +0x128.
   2. If T's id is not in world +0x30: insert it, then:
      * the reward 0x2a7ca0(world, a random spot around T's position (§0.8, two draws),
        tier = min(T's level +0x144 + 1, 4));
      * for every inventory tab of T (+0x9f0, each a list of 0xa4-byte entries: count i32
        +0, item +4), in order, for every entry: drop the entry's item at a random spot
        around T with a random angle (§0.8, three draws), scale 1.0, count = the entry's
        count, no creature, owner 0. (T's inventory is not emptied.)

---

## 6. Kind 7: picking up a ground item

1. The zone at (record +0xa0, +0xa4); the index i = record +0xa8, 0 ≤ i < the number of
   ground items; G = ground item i. Otherwise nothing.
2. If the squared distance from G's position to U's position is > 16.0 (or NaN), nothing.
3. If G's item is not equal (§0.7) to the record's item, nothing.
4. A sound request at U's position: id 0x30, pitch = float(rand()) × 0.1 / 32767.0 + 1.0,
   volume 1.0. If G's item is type 0x0b with subtype 0x25: id 0x2d and pitch × 2.0.
5. Append a **pick-up notice** to world +0x110, a 0xd0-byte payload:

| Offset | Value |
|---|---|
| +0x00 | U's id |
| +0x08 | a copy of G's item (fields copied, the rest zero) |
| +0xa8 | G +0xd4 as a 16-bit count |
| +0xb0..+0xc7 | G's position |
| +0xc8 | G's flags byte +0xc4, sign-extended to 32 bits |

6. If G's flags have bit 1 (2) clear: insert the zone into world +0x190, and overwrite G as an
   **empty ground item**: item cleared (type 0 and every field 0), +0xb8..+0xbf all ones,
   +0xc4 = 0, +0xc8..+0xcf = 0, +0xd0 = −1, +0xd4 = 1, +0xd8 = 0. The position bytes
   +0xa0..+0xb7 and +0xc0..+0xc3 receive leftover values (not meaningful). With bit 1 set,
   G stays where it is (the notice is still sent).

The notice is what gives the item to the picker; its consumer is outside this routine.

---

## 7. Kind 8: dropping an item

1. Draw r1, r2. Position = U's position + (x: trunc64(((float(r2) + float(r2)) / 32767.0 −
   1.0) × 65536.0), y: the same with r1, z: trunc64(0.0)). (The sign is the reverse of §0.8.)
2. Drop the record's item there (0x2a5b70): angle 0.0, scale 1.0, count 1, creature U, owner
   0.
3. A sound request at U's position: id 0x3c, pitch 1.0, volume 1.0.

---

## 8. Kind 10: calling the pet

1. P = the creature with U's pet id (+0x9d8). If P exists: empty P's map at +0xb88 (its
   aggro); if the squared distance from P to U is > 40,000.0 (200 blocks), P's position = U's
   position.
2. Empty U's map at +0xb98 and set U +0x9e0 (8 bytes) to 0, whether or not P exists.

---

## 9. rand() order

* Kind 5: none, except when an object that takes an item has contents: three draws (spot r1,
  r2, angle r3); then 0x2a7ca0 for types 0x15 and 0x16 draws its own.
* Kind 2: none. Kind 8: two. Kind 10: none.
* Kind 3 (case 3 only, and only the first time for that creature): two for the reward spot,
  then 0x2a7ca0's own, then three per inventory entry.
* Kind 7: one (pitch), and only when the pick-up succeeds.

---

## 10. Constants

| RVA | Value | Use |
|---|---|---|
| 0x42e510 | 1/65536 | position units to blocks |
| 0x42e5f8 | 65536.0 | random spot scale |
| 0x42e5f0 | 32767.0 | rand() divisor |
| 0x4ac980 | 1.0 | random spot base; pitch, volume, scale |
| 0x42e7e4 | 360.0 | drop angle |
| 0x42e528 | 0.1 | pick-up pitch spread |
| 0x4ac9d0 | 2.0 | pitch factor of item 0x0b/0x25 |
| 0x4aca34 | 16.0 | pick-up range² |
| 0x42e7e8 | 400.0 | kind 2 creature range² |
| 0x42e5b4 | 64.0 | kind 2 gate range² |
| immediate | 0x140000 | kind 2 zone scan half-width (20 blocks) |
| 0x4463c0 | 40,000.0 | pet teleport range² |
| 0x43801c | 64,800,000.0 | 18:00 |
| 0x438014 | 21,600,000.0 | 6:00 |
| immediate | 25,200,000.0 (0x4bc042c0) | wake-up time, 7:00 |
| immediate | 0.75 (0x3f400000) | sleep sound volume |
| 0x42e5d4 | 1000.0 | full swing time |

Sound ids: 0x1f sleep, 0x30 pick-up, 0x2d pick-up of item 0x0b/0x25, 0x3c drop, 0x70 the gate
opener, 0x36–0x39 doors and gates (§3). Object types: seats and beds 0x1c, 0x1e, 0x1f, 0x20,
0x5f, 0x60, 0x73; doors 3–6, 0x14, 0x15, 0x54–0x57, 0x69; gates 0x10; reward-giving 0x15,
0x16. Creature ids: race 0xf5, 0xf6, 0x12e; class 0x84 (innkeeper), 0xa1 (arena master).

---

## 11. How it is checked

* `EngineTests interactoracle <file>`: `Extract/oracle/interact.bin` (CIN1, tools/
  ZoneExtractor's `interact.request`), recorded 0x2c3710 calls with the creature, its
  interactions and, for kind 5, the named objects before and after; the creature record, the
  objects and the sounds must match. No call has been recorded yet (`docs/creatures.md`), so
  nothing here is verified.
* Kinds 1, 3, 11, 12, 13 have unit checks in `tests/EngineTests.cpp`.

---

## Open questions

1. **Standing up and freeing seats.** Kind 5 only takes a free seat. Nothing here clears
   +0x30 when its user leaves, stands up or dies; no other routine writing 0 to an object's
   +0x30 was found (0x10b90, the townsfolk's seat search, only reads it).
2. **What advances the swing time** +0x2c toward 1000.0 (and so drives the door animation)
   was not traced.
3. **Doors' sounds 0x11 and 0x12** play only when shutting. That types 0x11/0x12 are doors at
   all is inferred from this table; no use-prompt row lists them.
4. **Consumers of the notices**: world +0x110 (pick-ups), +0x160 (sleep), +0x190 and +0x1a0
   (changed zones) are read elsewhere, presumably by the server's network sender; not traced.
5. **U +0xb98 and +0x9e0** (cleared by kind 10) were not identified.
6. **Ground item +0xd0 / +0xd4**: the emptied slot gets +0xd0 = −1 and +0xd4 = 1, while
   `use-prompt.md` §0.4 says a 0 at +0xd4 marks an empty slot. One of the two readings is
   wrong; how the game recognises an empty slot should be checked.
7. **Race 0xf5 / 0xf6 and flag 0x200000** (the creatures that kind 2 and kind 3 activate) and
   the meaning of +0x19c for them are named by behaviour only.
8. **The reward tier of a locked door or chest** is the rarity of the item it took; whether
   that is intended (keys of a rarity) or a quirk is not known.
9. **CubeVulkan's current port** treats the swing time +0x2c as an integer; the game reads and
   writes it as a float (instructions at 0x2d3a32..0x2d3a53).
10. **Who asks to sleep at the innkeeper's.** No caller of the list append 0x129a0 queues a
    kind-5 record with a target creature; the record may come from the conversation code or
    the network (packet 9). `use-prompt.md` §3.6.9 "Sleep" queues kind 3, not 5.
