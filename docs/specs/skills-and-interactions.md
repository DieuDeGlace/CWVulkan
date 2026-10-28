# Special skill (R) and interactions 1, 11, 12, 13

Behavioural specification of Steam Cube World, written from the decompiled executable and
the binary's constants. Routines are cited by RVA (image base 0x140000000). Offsets are byte
offsets into the record named. Times are milliseconds, distances are blocks, and world
positions are 64-bit integers in 1/65536 of a block (so a position difference is multiplied
by 1/65536, the float at 0x42e510, before it is compared with a distance in blocks).

All "rand" draws below are the C runtime's `rand()` (range 0..32767), shared with the rest
of the game, and are always divided by 32767.0 (the float at 0x42e5f0).

---

## 0. Creature record fields used here

| Offset | Size | Meaning (as used here) |
|---|---|---|
| +0x08 | i64 | creature id |
| +0x10, +0x18, +0x20 | i64 ×3 | position x, y, z |
| +0x5c | byte | movement flags (bit 0: on the ground) |
| +0x60 | byte | hostility kind (0 = player; 1, 4, 7, 8 used below) |
| +0x64 | i32 | race |
| +0x68 | byte | current ability id (0 = none) |
| +0x6c | f32 | time spent in the current ability |
| +0x78 | – | start of the appearance block (its race defaults come from 0x53e80) |
| +0x80 | u32 | appearance / state flags (bits used: 0x800, 0x4000, 0x10000, bit 9, 15, 17, 18, 22, 26) |
| +0x84, +0x88, +0x8c | f32 ×3 | the appearance's three size factors |
| +0x128 | u16 | state flags (bit 5 = 0x20 used by the hostility test) |
| +0x12c | f32 | dodge-roll timer |
| +0x130 | f32 | stun timer (≤ 0 means not stunned) |
| +0x144 | i32 | level |
| +0x14c | byte | class (1..4) |
| +0x14d | byte | specialisation (0 or not 0) |
| +0x180 | f32 | health |
| +0x184 | f32 | a power value (block power for players; see kind 12) |
| +0x188 | f32 | mana (0..1) |
| +0x1a0 | i64 | owner's creature id |
| +0x980 | list | buffs, each 0x18 bytes: type byte +0, strength f32 +4, duration f32 +8, source creature id i64 +0x10 |
| +0x9a0 | f32 | stamina (0..1) |
| +0xab8 | list | pending interactions (see §2.1); +0xac0 holds its element count |
| +0xb78 | map | cooldowns: ability id (i32) → remaining ms (f32) |
| +0xb88 | map | aggro: creature id (i64) → anger (f32) |
| +0xbcc | u32 | AI behaviour flags |

World fields used: +0x40 the list of creatures (in world order), +0x358 the "remote client"
flag (non-zero when this world is a client of a remote server and only the local player is
simulated), +0x448 the local player, +0x450 the id → creature map, +0xa0 the list of sounds
to play, +0x150 the list of buff-change notices.

Two small records are produced repeatedly:

* **Sound request** (appended to world+0xa0): position (3 × i64), sound id (i32), a float at
  +0x1c (pitch / playback speed), a float at +0x20 (volume), then 8 zero bytes.
* **Buff-change notice** (appended to world+0x150): source creature id (i64) at +0,
  target creature id (i64) at +8, a copy of the 0x18-byte buff at +0x10, one flag byte at
  +0x28.

Two helper behaviours are referred to throughout:

* **Give buff** (0x65390): given a creature, a buff and a "refresh" flag. If the flag is set,
  or the buff type is one of 1, 2, 3, 6, 7, 9, 10, 11 (mask 0xece over types below 12), the
  creature's buff list is searched front to back for an entry with the same type *and* the
  same source id; the first such entry is overwritten with the new buff and nothing else
  happens. Otherwise (or if none matched) the buff is appended to the end of the list.
* **Hostility test** (0x50550), between creatures A and B. They are hostile when all of the
  following hold:
  1. A and B are different records;
  2. neither has flag bit 22 (0x400000) at +0x80;
  3. either A's or B's hostility kind (+0x60) is 7, **or** all of: neither class (+0x14c) is
     0xa1, neither race is 0x12f (303), neither hostility kind is 8, and at least one of
     (a) A's kind is 1 and (B's kind is not 1, or bit 15 of their +0x80 flags differs),
     (b) B's kind is 1 and (A's kind is not 1, or bit 15 differs),
     (c) A or B has bit 5 (0x20) set in +0x128.
  Otherwise they are friendly ("not hostile"). A creature tested against itself is friendly.

---

## 1. The special skill on R

### 1.1 Which control is R

The controls file (`controls.cfg`, loaded by 0x42b30, defaults from 0x43760, saved by
0x438e0) holds 23 buttons, each with two bindings (an i32 code and an i32 device: 0 keyboard
scan code, 1 mouse button). Button index 3 is named `ButtonUltimateSkill` (menu label
"Ultimate Skill") and defaults to scan code 0x13, the **R** key. (Index 2,
`ButtonClassSkill`, defaults to scan code 0x2a, left Shift, and is not specified here.)

The input routine of the game controller (0x1355a0) writes each button's held state as one
byte at controller+0x1a70+index (setter 0xa36c0), so R's held state is the byte at
controller+0x1a73. On a gamepad (XInput, when controller+0xcd is 0) the Y button
(0x8000 in the button mask) sets the same button 3.

### 1.2 Which skill each class and specialisation gets (0x656e0)

The skill id comes from the class byte (+0x14c) and whether the specialisation byte (+0x14d)
is zero:

| Class (+0x14c) | Spec 0 | Spec not 0 |
|---|---|---|
| 1 (Warrior) | 0x94 (148) | 0x95 (149) |
| 2 (Ranger) | 0x8f (143) | 0x8e (142) |
| 3 (Mage) | 0x2f (47) | 0x90 (144) |
| 4 (Rogue) | 0x6d (109) | 0x8d (141) |
| anything else | 0 (no skill) | 0 |

(The class names follow the order the game's character menu lists them, 1-based.)

### 1.3 When the press is examined

The per-frame player-input routine 0x8ae50 handles R. The controller's update (0xa5a50, call
at 0xa9b2c) only calls 0x8ae50 when world+0x210 is non-zero, the local player's current
ability is neither 0x9c nor 0x9d, and none of the three panels at controller+0x118, +0x130,
+0x170 is visible (0x9af00). Inside 0x8ae50, before the R check is reached, the routine
returns early (so R is not examined this frame, and its "previous state" below is not
updated) when any of these hold:

* a modal UI element is open (0x342090 on controller+0x20 returns something);
* the object at controller+0x398 has its byte +0x1d8 set (probably the chat input being open);
* the player is gliding (0x5ee70: current ability 0x78, bit 0 of +0x5c clear, not stunned,
  not dodging) and stamina (+0x9a0) is at most 0.001 (0x42e514);
* the current ability is 0x7d and its time (+0x6c) is below 2000 (0x42e5d8).

### 1.4 The press itself (0x8ae50)

1. R must be held this frame (controller+0x1a73 non-zero) **and** must not have been held at
   the previous examination (a global byte at 0x5515a2 keeps the previous state). It is an
   edge trigger.
2. The player's skill id (§1.2) must be non-zero.
3. The skill must be usable (§1.6, 0x507b0).
4. If all three hold:
   * **Rogue skills (0x6d or 0x8d):** the buff routine 0x29d3c0 is called (§1.5). The current
     ability is *not* changed.
   * **All other skills:** the current ability (+0x68) becomes the skill id, its time (+0x6c)
     becomes 0, and the dodge timer (+0x12c) becomes 0. The ability then runs through the
     game's normal ability update; see §1.8 for which of these create interactions.
   * Then, in both cases, the cooldown map entry for **the creature's current ability id
     (+0x68, read after the step above)** is found or created (0x7bca0) and set to the
     cooldown of that id (§1.7) as a float. For non-rogue skills this is the skill's own
     cooldown. For the rogue skills the current ability is unchanged, so this sets the entry
     of whatever ability was running (often 0, whose cooldown is 0); a rogue who presses R
     during another ability restarts that ability's cooldown at full length. This is the
     game's behaviour and is kept.
5. Whatever happened, the previous-state byte is set to this frame's R state.

### 1.5 The rogue buffs (0x29d3c0)

0x29d3c0 takes the world, the creature and an ability id. For the two R skills it gives the
creature one buff, with the refresh flag set (so it replaces an existing buff of the same
type from itself), source = the creature's own id:

| Skill | Buff type | Strength (+4) | Duration (+8) | Notice flag |
|---|---|---|---|---|
| 0x6d | 3 | 0 | 20000 (0x42e5ec) | 0 |
| 0x8d | 13 (0x0d) | 0 | 20000 (0x42e5ec) | 1 |

After giving the buff it appends a buff-change notice (source = target = the creature's id,
the buff, the flag above) to world+0x150. It then finds or inserts the creature's cooldown
map entry for the skill id and sets it to the cooldown of that id (§1.7), as a float.

The same routine also serves other self-buff abilities that are not on R; for completeness
(all refresh, source = self, notice flag 1): 0x70 → type 12, strength 2.0, 10000 ms;
0x71 → type 1, strength 0.5, 10000 ms, and a positive stun timer is cleared to 0;
0x72 → type 2, strength 0, 10000 ms; 0x73 → type 6, strength = 4 × the creature's power
value from 0x65c70 with selector 0, 30000 ms (0x432954); 0x92 → type 16, strength 2.0,
10000 ms. Any other id only sets the cooldown.

Side effects of buff 13 elsewhere (facts, for the implementer's awareness): while a creature
has a buff of type 13, the stamina-cost routine 0x65ea0 returns 0 for every ability, and the
cooldown table gives abilities 0x32 and 0x6c only 1800 ms.

### 1.6 Usable check (0x507b0)

For creature C and ability id S, in this order:

1. If S equals C's current ability (+0x68) and the current ability's time (+0x6c) is strictly
   below that ability's total duration (0x4f220), S is **not usable**.
2. S is **not usable** unless all of:
   * C is not stunned (+0x130 ≤ 0), or S is 0x71;
   * S is not 0x50, or (C's current ability is not 0x50 and the current ability's duration
     is ≤ its time);
   * C's current ability is not 0x32.
3. S is **not usable** unless the mana cost of S (0x5f7c0) is ≤ C's mana (+0x188) and the
   stamina cost of S (0x65ea0) is ≤ C's stamina (+0x9a0). Every R skill costs 0 mana and 0
   stamina (0x5f7c0 returns 0 for ids above 0x88 and for 0x2f, 0x6d; 0x65ea0 returns 0 for
   all eight R ids).
4. C's cooldown map (+0xb78) is looked up for S. If there is no entry, or the entry's value
   is exactly 0.0, S is **usable** (with one exception: for S = 0x1d it is usable only if
   mana is above 0). Otherwise S is **not usable**.

### 1.7 Cooldown table (0x50bd0)

0x50bd0 maps an ability id (and, for two ids, the creature's buffs) to a cooldown in ms.
Ids below 0x16 or above 0x95 give 0. Within 0x16..0x95:

| Ability ids | Cooldown (ms) |
|---|---|
| 0x16, 0x34, 0x61 | 12000 |
| 0x2f, 0x6d, 0x70, 0x71, 0x72, 0x8e, 0x90, 0x92 | 40000 |
| 0x32 | 1800 if the creature has a buff of type 13, else 16000 |
| 0x6c | 1800 if the creature has a buff of type 13, else 16000 |
| 0x33, 0x37, 0x73 | 16000 |
| 0x49 | 15000 |
| 0x5f, 0x62, 0x88, 0x95 | 30000 |
| 0x6f | 4000 |
| 0x8c, 0x8d, 0x8f | 60000 |
| 0x94 | 20000 |
| every other id in 0x16..0x95 | 0 |

So the R skills' cooldowns are: 0x94 → 20 s, 0x95 → 30 s, 0x8f → 60 s, 0x8e → 40 s,
0x2f → 40 s, 0x90 → 40 s, 0x6d → 40 s, 0x8d → 60 s.

(The buff search for 0x32 / 0x6c walks the buff list front to back and stops at the first
entry of type 13.)

### 1.8 Counting down, and what the R abilities emit

**Countdown.** At the end of the per-creature ability update 0x2c5250 (run by the world tick
for every creature with health above 0, with the frame's elapsed ms), every entry of the
cooldown map is reduced by the elapsed ms and clamped to 0 when it goes below 0. Entries are
never removed. (Just before that, if the world is not a remote client or the creature is the
local player, +0x74 accumulates the elapsed ms, and once it exceeds 4000 (0x4725a4) the
combo state is reset: the attack-speed factor (0x66460) is taken, +0x70 is set to 0, the factor
is taken again, and +0x6c is multiplied by (factor before) / (factor after). In that same branch each cooldown entry would be reset to its full value
if 0x368230 said so, but 0x368230 always answers no, so nothing is reset.)

**Interactions emitted by the R abilities.** Inside 0x2c5250 three of the R abilities queue an
interaction on the creature's own interaction list (§2), appended at the end, using a fresh
record (0x9660 defaults: zero item, +0xa0 = −1 as 8 bytes, +0xa8 = −1, +0xb0 = 0, +0xb8 = 0,
+0xc0 = 0, +0xc6 = 1) with only the kind set. All three require that either (the world is not
a remote client **and** the creature's hostility kind is not 0, i.e. a non-player creature on
the authoritative world) **or** the creature is the world's local player. Let *t* be the
ability time (+0x6c) before this frame's advance (the advance happens later, in 0x2de4c0)
and *dt* the frame's elapsed ms.

| Ability | When | Kind queued |
|---|---|---|
| 0x8f | appearance flag 0x800 (+0x80) is clear and *t* is exactly 0, i.e. once, on the ability's first update | 11 (shadow clone) |
| 0x90 | while *t* is below the ability's effect time *T* = 1500 / (attack-speed factor, 0x66460) truncated to an integer (0x4ead0); with *step* = *T* / 8 (integer division truncating toward zero), whenever ⌊*t*⌋ / *step* and ⌊*t* + *dt*⌋ / *step* (integer divisions) differ | 12 (water bubble) |
| 0x95 | 0x95's effect time is 0, so: when *t* ≤ 0 and 0 < *t* + *dt*, i.e. once, on the first update | 13 (heroic shout) |

0x94, 0x8e and 0x2f do their work in the ability updates directly and queue no interaction of
kinds 1, 11, 12 or 13; they are not specified here. The interaction is consumed in the same
world tick on an authoritative world (the handler runs right after 0x2c5250, §2.2). On a
remote client the handler does nothing and the interaction is sent to the server instead
(§2.2), so remote players' R abilities take effect on the server through the network.

---

## 2. The interaction handler (0x2c3710)

### 2.1 The interaction record

A creature's pending interactions are a doubly linked list at creature+0xab8 (count at
+0xac0). Each list node is 0xd8 bytes: next pointer +0x00, previous pointer +0x08, then the
0xc8-byte interaction record at node+0x10. Record offsets (node offset = record offset + 0x10):

| Record offset | Node offset | Size | Meaning |
|---|---|---|---|
| +0x00 | +0x10 | 0xa0 | an item (type byte +0, subtype i32 +4, ...; copied with the item copy 0x9cd0) |
| +0xa0 | +0xb0 | i32 | zone x (or −1) |
| +0xa4 | +0xb4 | i32 | zone y (or −1) |
| +0xa8 | +0xb8 | i32 | index of an object in that zone (or −1) |
| +0xb0 | +0xc0 | i64 | target creature id (0 if none) |
| +0xb8 | +0xc8 | 8 bytes | extra data (0 for the kinds here) |
| +0xc0 | +0xd0 | i32 | extra data (0 for the kinds here) |
| +0xc4 | +0xd4 | byte | **kind** |
| +0xc6 | +0xd6 | i16 | always 1 when created |

(The "target id at +0xc0" of a list node is record+0xb0; the kind at "+0xc4" is record+0xc4,
node+0xd4.) Records are copied field by field by 0x95d0. Over the network a record travels as
200 raw bytes in packet 9 (client → server, written by 0x3f4f0 via 0x3a790, read by 0xf4490).

Kinds handled by 0x2c3710: 1, 2, 3, 5, 7, 8, 10, 11, 12, 13. This document specifies 1, 11,
12 and 13; kinds 2 (an item of type 0x18 subtype 5 used near objects), 3 (talking to a
creature), 5 (using a zone object), 7 (picking up an item), 8 (dropping an item) and 10
(calling the pet) are out of scope. Any other kind is ignored.

### 2.2 How it is reached and how the list is consumed

* The world tick 0x2da6c0 (given the world and the frame's elapsed ms) first clears bit 4
  (0x10) of +0x128 on every creature, then walks the world's creature list (+0x40) in order;
  for each creature whose health (+0x180) is above 0 it calls, in order, 0x2bb8e0, the ability
  update 0x2c5250, and the interaction handler 0x2c3710; then, for every creature (alive or
  not), 0x2de4c0 (which, among other things, advances the ability time).
* Creatures added to the world during the handler are appended to the end of that same list,
  so they receive their own updates later in the same tick.
* 0x2c3710 (world, creature):
  1. If the world's remote-client flag (+0x358) is set, return at once: nothing is consumed.
     On such a client, 0x3f4f0 instead takes the **first** pending interaction of the local
     player each network send, removes it from the list and sends it as packet 9. The server
     (0xf4490, packet 9) appends the received record to the end of the list of the sender's
     creature, where the server's own handler consumes it. Any kind is accepted from the
     network.
  2. Otherwise walk the list from front to back and handle each record by its kind (below).
     The list is not modified during the walk.
  3. After the walk, if the remote-client flag is still clear, empty the list (0x3c9c0).

### 2.3 Kind 1: using an item (bombs)

**Producer.** The inventory "use" action of the client (0xb7740, on an inventory slot: a count
i32 followed by the item) only acts if the item type is 1, 0x16 or 0x1c and the local player is
not stunned (+0x130 ≤ 0):

* type 1 subtype 9 (**bomb**), or type 0x16: a kind-1 interaction carrying a copy of the item
  (all 0xa0 bytes; +0xa0 = −1 as 8 bytes, +0xa8 = −1, +0xb0, +0xb8, +0xc0 = 0, +0xc6 = 1) is
  appended to the player's list. For the bomb the slot count then drops by 1 (and at 0 the
  slot is emptied: type, subtype and count set to 0). Type 0x16 is not consumed.
* type 1 subtype 13: a kind-1 interaction with the item is appended likewise, the two bytes at
  controller+0x668 are both set to 1 and controller+0x21c0 remembers the slot; nothing is
  consumed here.
* every other usable item starts an eating/drinking ability on the player instead and queues
  no interaction (not specified here).

**Handler.** Kind 1 acts only when the record's item type (record+0) is 1 **and** its subtype
(record+4) is 9. Every other kind-1 record (type 0x16, consumable subtype 13, ...) is ignored
by the handler and simply discarded with the list. For a bomb:

1. **Choose an id.** Start from −0x7ffffffffffffc18 (that is INT64_MIN + 1000) and lower it to
   the smallest id (+0x08) of any creature in the world list; the new id is that value − 1.
   (So the first such creature gets INT64_MIN + 999, the next INT64_MIN + 998, and so on.)
2. Construct a creature record with that id (constructor 0x4ce80, all its defaults).
3. Add it to the world (0x29d0d0): appended to the end of world+0x40 and entered in the
   id map at world+0x450.
4. Copy the user's position (+0x10, +0x18, +0x20) to it.
5. Race (+0x64) = 0x90 (144).
6. Health (+0x180) = its maximum health (0x5fac0, computed for the new record).
7. Hostility kind (+0x60) = 7.
8. Flags (+0x80) |= 0x14000.
9. Apply the race's default looks (0x53e80 with the race, the appearance block at +0x78, no
   look source, no extra option).

No owner, level or sound is set, and no rand() is drawn. (Steps 3–9 may be done in any order
by an implementation that adds the creature after setting it up, provided it ends up last in
the world list.)

### 2.4 Kind 11: shadow clone

No record field other than the kind is read; the caster is the creature that owns the list.

1. **Limit and id.** Walk the world creature list. Count the creatures whose owner id
   (+0x1a0) equals the caster's id (any such creature counts: earlier clones, water bubbles,
   pets, living or dead), and track the smallest creature id as in §2.3 step 1. If the count
   is 3 or more, skip this interaction entirely (**at most three** owned creatures). Otherwise
   the new id is (smallest id, starting from INT64_MIN + 1000) − 1.
2. Construct a creature record with that id (0x4ce80).
3. Copy the caster's record bytes from +0x10 up to (not including) +0x980 into the clone
   (0x3b7a0): position, orientation, velocities, hostility, race, ability and its time,
   appearance and flags, level, class, specialisation, health, mana, owner, equipment, name,
   and so on. The id, buffs and all containers from +0x980 on are not copied.
4. Hostility kind (+0x60) = 4 if the caster's hostility kind is 0 (a player), else 1.
5. AI flags (+0xbcc) = 0x480.
6. Flags (+0x80) |= 0x800 (marks a clone; 0x2c5250 will not let a creature with this flag
   cast 0x8f).
7. Current ability (+0x68) = 0 (the ability time is left as copied).
8. Owner (+0x1a0) = the caster's id.
9. Level (+0x144) = the caster's level.
10. Give the clone a buff, without the refresh flag (type 15 always appends):
    * type 0x0f (15, the "shadow clone" aura);
    * strength (+4) = 0.5 + 0.5 × (1 − 1 / ((L − 1) / 50 + 1)), and
    * duration (+8) = the integer part of 15000 + 15000 × (1 − 1 / ((L − 1) / 50 + 1)),
      converted back to float,
    where L is a skill level that 0x65690 returns; 0x65690 always returns 1000 in this build
    (whatever the creature or skill). With L = 1000, computed in 32-bit floats: strength
    0.9761678, duration 29285.0. (Formulas: 0x61150 and 0x65460; constants 50.0 at 0x42e5b0,
    15000.0 at 0x432950, 0.5 at 0x4ac964, 1.0 at 0x4ac980.)
    * source (+0x10) = 0.
11. Append a buff-change notice: source = clone id, target = clone id, that buff, flag 1.
12. Add the clone to the world (0x29d0d0, appended at the end).

No rand() is drawn and no sound is played.

### 2.5 Kind 12: water bubble

No record field other than the kind is read; the caster is the list's owner. There is no
limit on the number of bubbles.

1. **Id.** As §2.3 step 1 (smallest id in the world, starting at INT64_MIN + 1000, minus 1).
2. Construct a creature record with that id (0x4ce80).
3. Race (+0x64) = 0x11d (285).
4. Hostility kind (+0x60) = 7.
5. Position = the caster's position.
6. Owner (+0x1a0) = the caster's id.
7. Level (+0x144) = the caster's level.
8. Apply the race's default looks (0x53e80, as in §2.3 step 9).
9. **rand #1:** *s1* = 0.75 + 0.5 × rand / 32767 (0.5 at 0x4ac964, 0.75 at 0x42e710).
10. *P* = the caster's power value from 0x65c70 with selector 1 (see below). The bubble's
    +0x184 = *P* × *s1*.
11. **rand #2:** *s2* = 0.75 + 0.75 × rand / 32767. Multiply each of the bubble's three size
    factors (+0x84, +0x88, +0x8c) by *s2*.
12. Flags (+0x80) |= 0x800.
13. Health (+0x180) = 1.0, mana (+0x188) = 1.0.
14. AI flags (+0xbcc) = 0x200.
15. **rand #3:** append a sound request at the bubble's position: sound 0x30 (the 49th sound
    loaded by 0xa3a40, `pickup.wav`), pitch = 0.8 + 0.1 × rand / 32767 (0.1 at 0x42e528, 0.8
    at 0x42e530), volume 1.0.
16. Flags (+0x80) |= 0x800 again (no further effect).
17. Add the bubble to the world (0x29d0d0, appended at the end).

**The power value (0x65c70).** Start at 5.0 (0x42e584). Build the list of the creature's
contributing entries (selector 0 uses 0x4fd40, selector 1 uses 0x52ce0) and add, for each,
the value 0x10ad90 gives for it and the creature. Then: × 1.5 (0x42e548) if bit 9 of +0x80 is
set; × 1.2 (0x42e718) if bit 17; × 4.0 (0x4aca20) if bit 18; × the strength (+4) of the first
buff of type 0x17 (23) in the buff list, if there is one; × 1.1 (0x42e538) if bit 26.

### 2.6 Kind 13: heroic shout

No record field other than the kind is read; the shouter is the list's owner. The range test
everywhere below is: squared distance in blocks (position differences × 1/65536) **strictly
below 2500.0** (0x437ffc), i.e. within 50 blocks.

1. Append a sound request at the shouter's position: sound 0x6b (the 108th sound loaded,
   `heroic-shout.wav`), pitch 1.0, volume 1.0.
2. Start an empty set of creatures (unique entries; the game orders it by record address).
3. Walk the world creature list in order. For each creature C:
   1. Skip C if its hostility kind (+0x60) is 7 or 8.
   2. If the shouter and C are **not hostile** (§0 hostility test, shouter first; this
      includes the shouter itself) and C is in range of the shouter:
      * give C (refresh flag set) the buff type 0x11 (17), strength 0.05, duration 10000.0,
        source = the shouter's id;
      * append a buff-change notice: source = shouter id, target = C's id, that buff, flag 0.
      A friendly creature out of range gets nothing.
   3. If the shouter and C **are hostile**: walk C's aggro map (+0xb88) in ascending id
      order. For each id in it, find that creature F in the world (the game does not check
      that F exists). If the shouter and F are not hostile and F is in range of the shouter,
      add C to the set. (C itself need not be in range: an enemy is affected when it is angry
      at an ally of the shouter who is within 50 blocks of the shouter.)
4. For each creature C in the set (ascending record address in the game):
   * give C (refresh flag set) the buff type 0x12 (18), strength 0, duration 5000.0,
     source = the shouter's id;
   * append a buff-change notice: source = shouter id, target = C's id, that buff, flag 0.
5. Discard the set.

No rand() is drawn. Health is not checked: dead creatures still in the list are treated like
the others.

### 2.7 Summary of rand() order inside the handler for these kinds

* Kind 1: none.
* Kind 11: none.
* Kind 12: exactly three, in this order: size-power factor *s1*, body size factor *s2*,
  sound pitch.
* Kind 13: none.

---

## 3. Constants

| RVA | Value | Use |
|---|---|---|
| 0x42e510 | 1/65536 | position units to blocks |
| 0x42e5f0 | 32767.0 | rand() divisor |
| 0x437ffc | 2500.0 | heroic shout range² (50 blocks) |
| 0x4ac980 | 1.0 | |
| 0x4ac964 | 0.5 | |
| 0x42e710 | 0.75 | |
| 0x42e528 | 0.1 | |
| 0x42e530 | 0.8 | |
| 0x42e5b0 | 50.0 | skill-level curve |
| 0x432950 | 15000.0 | clone buff duration base |
| 0x42e5ec | 20000.0 | rogue R buff duration |
| 0x42e5e8 | 10000.0 | other self-buff durations |
| 0x432954 | 30000.0 | ability 0x73 buff duration |
| 0x4ac9d0 | 2.0 | buff strength for 0x70, 0x92 |
| 0x4aca20 | 4.0 | |
| 0x42e584 | 5.0 | power value base |
| 0x42e548 | 1.5 | |
| 0x42e718 | 1.2 | |
| 0x42e538 | 1.1 | |
| 0x432948 | 1500.0 | ability 0x90 effect window (before the speed factor) |
| 0x4725a4 | 4000.0 | combo reset time |
| 0x42e5d8 | 2000.0 | ability 0x7d input lock |
| 0x42e514 | 0.001 | glide stamina threshold |
| immediate | 0.05 (0x3d4ccccd) | heroic shout ally buff strength |
| immediate | 10000.0, 5000.0 | heroic shout buff durations |

Ids: race 0x90 (bomb), 0x11d (water bubble); buff types 0x0f (clone aura), 0x11 (shout,
allies), 0x12 (shout, enemies), 3 and 13 (rogue R skills); sounds 0x30, 0x6b.

---

## Open questions

1. **Gameplay meaning of the buffs.** This spec gives types, strengths, durations and sources
   only. What buff types 3, 13, 15 (0x0f), 17 (0x11) and 18 (0x12) do (e.g. whether 0x12
   forces enemies to target the shouter, how 0x11's 0.05 strength is applied, how the clone
   aura's strength and duration act, whether a clone is removed when its aura runs out) is
   decided by other routines that were not examined.
2. **What 0x94, 0x8e and 0x2f do** while running is in the ability updates (0x2bb8e0,
   0x2c5250) and is not covered.
3. **The water bubble's behaviour** after creation (race 0x11d is special-cased in 0x2bb8e0,
   0x29dcf0, 0x2a45e0, 0x65750) and the meaning of its +0x184 value were not examined.
4. **The bomb's behaviour** after creation (race 0x90, flags 0x14000) was not examined.
5. **0x65c70's contributing entries** (built by 0x4fd40 / 0x52ce0, valued by 0x10ad90) were
   not specified; CubeVulkan already calls selector 0 its spell power.
6. **0x66460** (the attack-speed factor that sets ability 0x90's window) and **0x4f220**
   (ability total durations) were not specified here.
7. **Sound record fields**: +0x1c is read as pitch and +0x20 as volume from how other sounds
   are built; not confirmed against the audio code. The sound-id → file mapping assumes the
   ids index the order 0xa3a40 loads the files in.
8. **The buff-change notice flag byte** (+0x28; 1 for the clone and most self-buffs, 0 for the
   shout and for rogue skill 0x6d) and the purpose of world+0x150's consumer were not traced.
9. **Kind-1 records for item type 0x16 and consumable subtype 13** reach the handler and are
   ignored; whether some other code reads them before the list is cleared (for subtype 13,
   controller+0x668 / +0x21c0 suggest a follow-up elsewhere) was not traced.
10. **Order of the heroic-shout enemy set** is by memory address in the game, which is not
    reproducible; any deterministic order gives the same buffs, only the order of the
    notices differs.
11. **Interaction record +0xb8, +0xc0 and +0xc6** are not used by these kinds; their meaning
    for other kinds was not examined.
