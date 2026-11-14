# Steam Zone actor-reference pre-pass (Zone +0x498) in the World step

Image identity: Steam Cube World x64 executable, unpacked image SHA-256
7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244, preferred
base 0x140000000. All addresses below are RVAs into that image.

Author role: isolated source-side specification author. This contract is
behavioral prose derived only from the original Steam evidence listed at the end.

## 1. Answer in brief

A pass over the Zone +0x498 actor references does exist in the World step. It is
not in the World step routine at 0x2DA6C0 itself, but in the routine at 0x2E3510,
which 0x2DA6C0 calls once per step, as its third call: after the routines at
0x2E2E80 and 0x2ADFE0, and before everything else the World step does.

The 0x2E3510 routine contains the per-Zone ground-item pass (Zone +0x30) and the
per-Zone static-object pass (Zone +0x60). For each selected Zone it runs three
passes in this order:

1. the actor-reference pass (Zone +0x498);
2. the ground-item pass (Zone +0x30);
3. the static-object pass (Zone +0x60).

Then it moves to the next Zone. The passes are interleaved per Zone. They are not
three global sweeps.

For each reference, the pass resolves the referenced creature by its 64-bit
identifier. If the creature exists and its kind byte is not 8, the pass sets bit
0x0100 in the creature's 16-bit flag word at creature +0x128. That is all it does.
There are no removals or insertions, no timers, no sounds and no events. The whole
pass runs only when the World byte at +0x358 is zero.

## 2. Placement within the World step (0x2DA6C0)

Each step, 0x2DA6C0 calls these routines in this order:

1. 0x2E2E80;
2. 0x2ADFE0 (with the World);
3. 0x2E3510 (with the World and the step time). This routine holds the Zone
   actor-reference pass, the ground-item pass and the static-object pass.
4. Local-player-only bookkeeping, then 0x2E17D0, 0x2A56C0 and 0x2A45E0 (these
   three only when World +0x358 is zero), then 0x2E0DB0, and further
   local-player work.
5. The creature body/movement loop over the World creature list (World +0x40),
   in list order:
   - First, a sweep clears bit 0x0010 of creature +0x128 on every creature.
   - Then, for each creature: if the float at creature +0x180 is greater than
     zero, 0x2BB8E0, 0x2C5250 and 0x2C3710 run in that order. After that,
     0x2DE4C0 runs for every creature.
6. Later work: 0x2D2EB0 and 0x2C34C0 (conditional), 0x2DBA40, a pass over the
   World list at +0xC0, and 0x2E1750.

So the Zone actor-reference pass, together with the ground-item and static-object
passes of the same routine, runs strictly before the creature body/movement loop
of the same step. Bit 0x0100, set by the actor-reference pass, is therefore already
visible to the step-5 routines of the same step. Section 6 lists the two observed
readers and clearers of that bit.

## 3. Work inside 0x2E3510 before the per-Zone loop

The work below runs in this order, before any Zone is visited.

### 3a. Optional local-player block

This block runs only when a local player exists (World +0x448 is non-null) and
the local-player capability query with code 9 holds. It computes a nearest-point
vector into the local player's auxiliary record (+0x1528, fields +0x2F0..+0x2F8).
It does not touch Zone +0x498.

### 3b. Creature sweep and building the Zone set

The routine walks the World creature list at World +0x40, in list order. For
every creature:

1. It clears bits 0x0002 and 0x2000 of creature +0x128 (the mask leaves every
   other bit). Bit 0x0100 is not cleared here.
2. It checks the creature kind byte at creature +0x60. If that byte is zero, the
   creature is appended to a temporary creature list, which later feeds a
   ground-item sub-check. The routine then computes the creature's Zone
   coordinate from its 64-bit fixed-point position:
   - X position at +0x10, Y position at +0x18;
   - first a floor division by 0x10000 gives the block coordinate;
   - then a floor division by 64 gives the Zone coordinate.
3. For that creature, it visits every Zone coordinate within plus or minus 3 on
   both axes, a 7 by 7 square. X is the outer loop and Y the inner loop, both
   ascending. For each coordinate it looks up the World's loaded-Zone map at
   World +0x460 and keeps the Zone if the map entry exists and its Zone pointer
   (entry +0x18) is non-null. Kept Zones are inserted into a temporary
   unique-key ordered set of Zone pointers.
4. Creatures whose kind byte at +0x60 is non-zero add no Zones.
5. The same creature walk also handles the arena-master byte cases (creature
   +0x14C equal to 0xA1 with creature +0x19C not equal to 0xFF), which look at
   the creatures registered at Zone +0x460. These cases are unrelated to +0x498.

### 3c. Pre-pass over the Zone set

For each Zone in the set, in set order, the routine does three things. It resets
the Zone map at +0x4D8 and refills it from Zone +0x460 entries. It zeroes the
32-bit counter at Zone +0x490. It then increments that counter once for each
Zone +0x480 entry whose identifier is present in the World creature map at
World +0x20. This pre-pass does not read Zone +0x498.

## 4. Zone selection and visiting order for the actor-reference pass

- Selection: only Zones in the temporary set from section 3b are visited. These
  are loaded Zones with a non-null Zone object, within the 7 by 7 Zone square
  around at least one creature of the World creature list whose kind byte at
  +0x60 is zero. Every other loaded Zone is skipped for this pass, and also for
  the ground-item and static-object passes in this routine. The selection is
  rebuilt every step. The set is discarded when the routine ends.
- De-duplication: each Zone is visited at most once per step, even when several
  qualifying creatures cover it.
- Order: the set is keyed by the Zone object pointer value, compared as an
  unsigned 64-bit integer, and is walked in ascending key order. The visiting
  order is therefore ascending Zone object address. It is not ordered by Zone
  coordinate, load order or creature order. Because this order depends on
  allocation addresses, it is not a reproducible gameplay order. A
  reimplementation cannot recover it from game data. Section 5 shows that the
  actor-reference pass itself is order-insensitive.

## 5. The actor-reference pass (per Zone, first of the three passes)

Gate: the pass runs only when the World byte at +0x358 is zero. The byte is
re-checked for each Zone. When it is non-zero, the pass is skipped for every Zone,
and the ground-item and static-object passes still run with their own internal
+0x358 gates.

Container: Zone +0x498 holds the head pointer of a circular doubly linked list.
Each list node holds a forward link at node +0x00, a backward link at node +0x08,
and the payload at node +0x10. The payload is one 64-bit signed creature
identifier. The pass walks the list forward from the head's first node until it
returns to the head, in list order.

For each entry, in list order:

1. The 64-bit identifier is resolved through the World creature map at World
   +0x450. That map is an ordered map keyed by signed 64-bit identifier, and its
   value at map node +0x28 is the creature object pointer. An identifier that is
   not present resolves to no creature (routine 0xE720).
2. If no creature resolves, nothing happens for this entry.
3. If a creature resolves and its kind byte at creature +0x60 is not 8, bit
   0x0100 is set (OR) in the 16-bit word at creature +0x128.
4. If the kind byte is 8, nothing happens.

Nothing else is read or written for an entry:

- The Zone +0x498 list is not modified. Nothing is removed, inserted or reordered.
- The creature is not removed from, or moved between, Zones.
- No timer, sound, network or other event is produced.
- No random draws are made.

Setting a bit is idempotent, so a duplicate identifier in the list, or the same
creature listed by several Zones, has the same effect as a single entry. The pass
has the same outcome whatever order the Zones or entries are visited in.

The pass never clears bit 0x0100. Neither the earlier creature sweep in 0x2E3510
(which clears only 0x0002 and 0x2000) nor the 0x0010 sweep in 0x2DA6C0 touches
it. Once set, the bit persists across steps until one of the consumers in
section 6 clears it.

After the actor-reference pass, the same Zone's ground-item pass (Zone +0x30,
records of 0xE0 bytes) and then its static-object pass (Zone +0x60, records of
0xE0 bytes) run before the next Zone is taken. Their behavior is outside this
contract.

## 6. Observed consumers of creature bit 0x0100

These are leads only. The readers and clearers below were found by bounded checks
inside the routines that the World step calls directly. They were not analyzed in
full.

- 0x2C3710 (run in the creature loop only for creatures whose float at +0x180 is
  greater than zero). In one case of its per-creature record handling, the case
  value is 3 and the record resolves a creature by identifier. If that resolved
  creature has bit 0x0100 set and its byte at +0x19C is zero, the routine sets
  +0x19C to 0xFF, clears bit 0x0100 and continues with further work that was not
  analyzed here.
- 0x2DE4C0 (run for every creature in the creature loop). It clears bit 0x0100 on
  one path. That path depends on the creature's flag at +0x80 bit 0x1000, on the
  creature's current Zone having an empty +0x470/+0x478 container, and on
  creature +0x128 bit 0x0002 being set. The full condition set was not analyzed.

## 7. Authoritative versus viewed World

- When World +0x358 is zero, the actor-reference pass runs as described. The same
  byte gates other mutating work in this routine and in 0x2DA6C0, for example
  0x2E17D0, 0x2A56C0 and 0x2A45E0, and the ground-item timer-expiry spawn event.
  This is consistent with the zero value marking the authoritative (host/local)
  World.
- When World +0x358 is non-zero, the pass is skipped entirely. No creature
  receives bit 0x0100 from Zone +0x498 references in that World during this
  routine.
- The Zone-set construction (section 3b) and the Zone pre-pass (section 3c) are
  not gated by +0x358.
- Whether a local player exists (World +0x448) does not affect the
  actor-reference pass.

## 8. Explicit limits (unproved)

- Who populates or erases Zone +0x498 entries, and when, was not established. A
  global search of the export for that offset did not complete. Only the reader
  in 0x2E3510 is proved.
- The meaning of the kind byte at creature +0x60 is unproved. The pass excludes
  value 8, and value 0 qualifies a creature as a Zone-set anchor. The meaning of
  World +0x358 is likewise unproved; its host/local reading is inferred only from
  the gating pattern in section 7.
- The semantic meaning of creature bit 0x0100 is unproved, as is the complete
  set of readers and clearers outside the routines named in section 6.
- That the set inserter (0x73F40) uses a unique ascending unsigned comparison is
  observed directly. That the walk is an in-order successor traversal is observed
  from the loop structure. Allocation-address ordering is the consequence of that
  key and has not been confirmed at runtime.
- The ground-item and static-object passes are referenced here only for their
  placement. Their contents are not specified here.
- No runtime or owner-data verification was performed in this role.

## Evidence boundaries

Statements in sections 1 to 5 are read directly from the decompiled export of the
original Steam executable for routines 0x2DA6C0, 0x2E3510, 0xE720 and 0x73F40.
Section 6 rests on short excerpts of 0x2DE4C0 and 0x2C3710. The interpretations
in section 7 and the inference about the ordering consequences in section 4 are
marked as inference. No reconstruction source, implementation material,
replacement-engine file, SDK, web material, runtime capture or owner data was used.

Exposure statement. Files read in this task:

- C:\Users\DieuDeGlace\Documents\Cubeworld-Vulkanengine-Steam\docs\CLEANROOM_AUTHOR.md
  (role policy, read in full).
- C:\Users\DieuDeGlace\Documents\decompiled cube world steam\decompiled\functions.csv
  (one matching row for 0x2DA6C0 via search).
- C:\Users\DieuDeGlace\Documents\decompiled cube world steam\decompiled\functions\1402da6c0.c
  (in full).
- C:\Users\DieuDeGlace\Documents\decompiled cube world steam\decompiled\functions\1402e3510.c
  (lines 1 to 1358 and 1690 to 1765, plus pattern searches).
- C:\Users\DieuDeGlace\Documents\decompiled cube world steam\decompiled\functions\14000e720.c
  (in full).
- C:\Users\DieuDeGlace\Documents\decompiled cube world steam\decompiled\functions\140073f40.c
  (in full).
- C:\Users\DieuDeGlace\Documents\decompiled cube world steam\decompiled\functions\1402de4c0.c
  (lines 200 to 289).
- C:\Users\DieuDeGlace\Documents\decompiled cube world steam\decompiled\functions\1402c3710.c
  (lines 290 to 334).
- Pattern-search hit lines only (offset 0x498/0x490/0x4A0 and bit-0x0100 masks)
  in the decompiled function files for the direct callees of 0x2DA6C0. An
  incomplete repository-wide offset search of the functions directory returned
  only file names and counts in 140044b40.c, 1400df8f0.c, 1400f2340.c,
  14011a820.c and 140131620.c, which were not opened.
- Directory listings of the decompiled export folder.

No prohibited material was exposed.
