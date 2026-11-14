# Steam Zone +0x498 creature-identifier list: writers, lifetime and readers

Image identity: Steam Cube World x64 executable, unpacked image
SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244,
preferred base 0x140000000. All addresses below are RVAs relative to that base.
Author role: isolated source-side specification author (original evidence only).
Date: 2026-11-14.

## 1. The container and its neighbours

- Zone +0x498 (8 bytes) holds a pointer to a heap-allocated sentinel node of a
  circular doubly linked list; Zone +0x4A0 (8 bytes, unsigned) holds the element
  count. Every node is 0x18 bytes: +0x00 next, +0x08 previous, +0x10 one signed
  64-bit creature identifier. An empty list is a sentinel whose next and previous
  point to itself, with count 0.
- Every insertion appends at the tail (new node placed immediately before the
  sentinel), so list order equals insertion order. The shared append helper at
  0x12A10 raises the standard "list too long" length error when the count already
  equals 0x0AAAAAAAAAAAAAA9; otherwise it increments Zone +0x4A0 by one.
- Neighbouring Zone fields, as laid out by the Zone constructor (section 2):
  - Zone +0x480 / +0x488: a second list of the same node shape and count width
    (a sibling creature-identifier list, see section 5).
  - Zone +0x490: 32-bit counter, initialised to 0 (section 5).
  - Zone +0x498 / +0x4A0: the list described here.
  - Zone +0x4A8 / +0x4B0: a third list of the same shape; one generator in this
    report appends a structure index to it (section 4.6).
  - Zone +0x460: the Zone's ordered map of spawn records keyed by signed 64-bit
    identifier (map node key at node +0x20, record pointer at node +0x28).

## 2. Construction and destruction

- Construct: 0x2F9400 (Zone constructor; Zone allocation size 0x58518). It stores
  the World pointer at Zone +0x08 and the 32-bit zone coordinates at Zone +0x10
  (X) and +0x14 (Y). In member order it zeroes +0x498 and +0x4A0, allocates the
  0x18-byte sentinel (self-linked) and stores it at +0x498. It also creates the
  +0x480 list (sentinel, count 0), writes 0 to the 32-bit +0x490, and creates the
  +0x4A8 list. No identifiers are inserted here.
- Destroy: 0x2F9940 (Zone destructor; reached through the scalar deleting
  destructor 0x2FA3B0, which is invoked through the Zone vtable, so its callers
  are not resolved in the call table). It first deletes every spawn record held in
  the +0x460 map, then, among other members, empties the +0x498 list (sentinel
  relinked to itself, count 0, each element node freed in list order) and finally
  frees the sentinel. The +0x480 list is destroyed the same way afterwards.
- Zone creation path: 0x2AE0C0 (World get-or-create zone at X, Y). If the World's
  zone table (World +0x460) already has a non-null zone for the coordinate pair it
  is returned. Otherwise the C runtime generator is seeded with
  Y * 0x400 + (32-bit World +0x244) + X, the Zone is constructed (0x2F9400), zone
  generation 0x2FF6F0 runs, then (only when the World byte +0x358 is zero and the
  World's saved-zone map at World +0x10 has an entry for the pair) saved per-zone
  state is applied: flags at Zone +0xF9/+0xFA with their payloads, a list at Zone
  +0xD8, the 32-bit Zone +0x490 (overwritten from the saved entry), and a container
  at Zone +0x4D8. The saved-state application does not touch +0x498/+0x4A0.
  Finally the zone is published in World +0x460 under two critical sections
  (World +0x3F0 then +0x3C8).
- Callers of 0x2AE0C0: 0x7D5C0 (parallel-for worker body), 0x858E0 (two call
  sites) and 0xF2340. Their roles are not resolved in this report.

## 3. The clear-and-repopulate entry point: 0x11A820

- Chain: 0x2AE0C0 -> 0x2FF6F0 (zone generation; call at 0x317145, reached on the
  fall-through after a generation loop; whether any earlier branch in 0x2FF6F0 can
  bypass it was not proved because the decompiler timed out on 0x2FF6F0) ->
  0x11A820 (arguments: a generation context whose first qword is the World, and
  the Zone).
- On entry, before any other work, 0x11A820 CLEARS the +0x498 list: the sentinel
  is relinked to itself, Zone +0x4A0 is set to 0, and every former element node is
  freed. Nothing else near +0x480..+0x4B0 is written at this point.
- It then derives the region of the zone: region = floor(zone coordinate * 64 /
  1024) per axis; if the World's double-size test (0x36160) is true for that region,
  each axis is floored to an even value. The region kind is obtained from 0x93480.
  - Kind 0: return immediately (list stays empty).
  - Kinds 6..13: places a structure/site object; no list insertion.
  - Kind 0x13: places an object, then calls 0x11E7F0 (section 4.2).
  - Kind 0x14: calls 0x121770 (section 4.3).
  - Kind 0x15: tile-pair spawns, insertion described in 4.1(a).
  - Kind 0x16: no list insertion (contents not otherwise examined).
  - Kind 0x17: tile-list spawns, insertion described in 4.1(b).
  - Any other kind: calls the region-site dispatcher 0x1274D0 (section 4.4).
- The branches are mutually exclusive; each zone generation runs exactly one.

## 4. Insertion sites (all INSERTs into Zone +0x498)

Common facts for every insertion site:
- The inserted value is the 64-bit identifier at spawn-record +0x38 (spawn records
  are 0x950-byte objects). The record is registered in Zone +0x460 by 0x119530
  immediately before the insertion. 0x119530 assigns the identifier when record
  +0x38 is zero: base = (((Zone Y + 0x1FFFFFF) as signed 64-bit) shifted left 32,
  bitwise-OR (Zone X + 0x1FFFFFF) as signed 64-bit), all shifted left 6; while the
  Zone +0x460 map already contains the candidate, the candidate is incremented by
  one. Records are created fresh (identifier zero), so identifiers are assigned in
  creation order within the zone and are unique within it. No insertion site
  inserts the same record twice, and the clear at the start of 0x11A820 prevents
  carry-over, so the list has no duplicates under the evidence examined.
- All insertion sites run only inside zone generation (section 3). No insertion,
  erase or clear was found on creature death, network receipt, or World update.
- "Record kind" below is the 32-bit value at record +0x1C; "type" is the 32-bit
  value at record +0x20 (in most sites the constant passes through 0x1420F0 before
  being stored); "flags" is the 32-bit record +0x58; record +0x40 is a 64-bit
  link to another record's identifier.
- Several sites consult a 160-byte region descriptor produced by 0x131540 for the
  region; its first byte is called "descriptor byte" below. When nonzero, the
  descriptor is copied into the record at +0x7F0 by 0x4E810.

### 4.1 Inline sites in 0x11A820

(a) Kind 0x15 (insertion call at 0x11B169). Scans the zone's own 4 x 4 tiles
(tile = 16 blocks), X outer, Y inner. For each tile whose type from 0x69C60 equals
0x34 it creates, in order: a kind-1 record (flags OR 0x2200, random facing
(0..3) * 90, level from 0x12A200), registered; then a kind-8 companion record of
type 0x121 (flags OR 0x2000, random facing (0..3) * 90, +0x40 = the kind-1 record's
identifier), registered, and the COMPANION's identifier is appended. One element
per qualifying tile; the kind-1 record is not listed.

(b) Kind 0x17 (insertion call at 0x11C399). Tiles of the region are scanned X
outer, Y inner into three collections: tile type 0x44, tile type 0x42, and other
tiles passing a threshold test (the last collection is shuffled and truncated; it
is not listed). Processing order: first the 0x44 collection (each in-zone tile
yields a kind-1 type-0xEC record and a kind-8 type-0xF0 companion whose identifier
goes to Zone +0x480, not +0x498, at 0x11BEE0); then the threshold collection (one
unlisted kind-1 record per tile); then the 0x42 collection in scan order, filtered
to tiles inside this zone: each yields a kind-1 record (type from 0xEC, flags
0x2200, +0x28 = 6), registered, then a kind-8 companion of type 0xEF (flags 0x42000,
random facing, +0x40 = the kind-1 record's identifier), registered, and the
COMPANION's identifier is appended. A 50% draw may then add an unlisted kind-1
record of type 0x106 whose identifier is written to the first record's +0x40.

### 4.2 0x11E7F0 (insertion call at 0x11F1CB)

- Reached only from 0x11A820, region kind 0x13 (call at 0x11ADAD) with type
  argument 0x2D, record +0x28 argument 0; the byte (+0x34) and extra-flags
  arguments are both supplied from one register whose value was not resolved.
- Seeds the generator from a region hash, collects candidate tiles of the region
  (distance-band test and a site test), and picks one tile by a second region hash
  (seed offset 0x4192); if none qualified, the region anchor tile is used.
- Only the zone that contains the picked tile creates one kind-1 record
  (+0x8B8 = 0x30, flags 0x2000 OR extra-flags OR 0x200). It is appended only when
  the descriptor byte is zero. At most one element per zone.

### 4.3 0x121770 (insertion call at 0x1220FE)

- Reached from 0x11A820 region kind 0x14 (leader type 0xAF, follower type 0xE3,
  follower +0x28 = 4) and from 0x1274D0 site case 0x1F (leader type 0xB2 or 0xB3 by
  a region hash, follower type 0xE6, follower +0x28 = 5).
- Picks one tile as in 4.2. Only the zone containing it creates a kind-7 leader
  record (+0x8B8 = 0, flags 0x2000, or 0x42000 when the leader type is 0xAF; when
  the descriptor byte is nonzero the descriptor is attached). The LEADER's
  identifier is appended unconditionally, immediately after registration. Then
  rand() mod 4 + 3 kind-1 followers are created (+0x8B8 = 0x4000, flags 0x2000,
  +0x40 = leader identifier); followers are not listed. One element per zone.

### 4.4 Region-site dispatcher 0x1274D0 (callee sites)

Reached from 0x11A820 at 0x11C571 for region kinds not handled inline. It seeds the
generator with a region hash (constant 0x21559A6E) and switches on the site type
from 0x119A00. Site cases that reach a +0x498 writer: 1 -> 0x125720; 0x11..0x17 ->
0x1241B0 (seven call sites); 0x18 -> 0x123210; 0x1B -> 0x11CD80; 0x1E and 0x20 ->
0x120BC0; 0x1F -> 0x121770. Other cases (0, 2..10, 0x1A, 0x1C, 0x1D and those
routed to 0x11F280) contain no +0x498 access; cases 2..8 reach 0x126160, which
writes only the +0x480 list.

### 4.5 0x120BC0 (insertion call at 0x12154B)

- Site 0x1E: leader type from 0x142190, follower type 0xE6, +0x28 = 5.
  Site 0x20: leader type from 0x13FFC0, follower type 0xE3, +0x28 = 4.
- Same picked-tile rule as 4.2. The zone containing the tile creates a kind-1
  leader (+0x28 as above, +0x8B8 = 0x10, flags 0x2000; flags OR 0x20200 and
  descriptor attached when the descriptor byte is nonzero); the LEADER's identifier
  is appended unconditionally; then rand() mod 4 + 3 unlisted kind-1 followers with
  +0x40 = leader identifier. One element per zone.

### 4.6 0x123210 (insertion call at 0x123FB6)

- Site 0x18. Same picked-tile rule. The containing zone places a type-0x58 site
  object, and appends that object's index in the Zone object array (Zone +0x60 ..
  +0x68, 0xE0-byte elements) to the Zone +0x4A8 list. It then creates rand() mod 4
  + 5 unlisted kind-1 records of type 0x25 (+0x8B8 = 0x30), collecting their
  identifiers in a temporary list, then one kind-1 record of type 0x28 (+0x8B8 =
  0x30, flags 0x2200) whose record list at +0x938 receives the collected
  identifiers. That LAST record's identifier is appended unconditionally; flags OR
  0x20200 and descriptor attachment follow the append when the descriptor byte is
  nonzero. One element per zone.

### 4.7 0x1241B0 (insertion call at 0x124FB3)

- Site cases 0x11..0x17 with per-case type, flags and +0x28 arguments.
- Before creating anything it CLEARS the Zone +0x480 list (count +0x488 = 0).
  Then, if the zone contains the picked tile, it creates one kind-1 record (type =
  type argument, +0x28 = case argument, flags = flag argument OR 0x3000, +0x8B8 =
  0x30, +0x920 = half the candidate count clamped to 1..20; descriptor attached and
  flags OR 0x20000 when the descriptor byte is nonzero). It is appended to +0x498
  only when the descriptor byte is zero. The following member records go to the
  +0x480 list (call at 0x125217), never to +0x498. At most one +0x498 element.

### 4.8 0x125720 (insertion call at 0x1260AB)

- Site case 1. Type from 0x1292F0 (region coordinates and kind). Same picked-tile
  rule. The containing zone creates one kind-1 record (+0x8B8 = 0x30, flags 0x2000
  OR 0x40000; descriptor attached when nonzero). It is appended only when the
  descriptor byte is zero. At most one element.

### 4.9 0x11CD80 (insertion call at 0x11D63B)

- Site case 0x1B. Returns unless the region's anchor tile lies inside this zone.
  Runs four groups (index 0..3). Groups 0..2 draw a composition from rand() mod 10
  and are unlisted. Group 3 forces the default composition: exactly one kind-1
  record (flags 0x40000 OR 0x2000, +0x8B8 = 0x10, +0x948 = 7); when the descriptor
  byte is nonzero the descriptor is attached. That record's identifier is appended
  unconditionally. One element per qualifying zone.

### 4.10 Totals

A zone gets at most one element from each site routine except the 0x11A820 inline
sites, which append one element per qualifying tile (kind 0x15: per type-0x34 tile;
kind 0x17: per in-zone type-0x42 tile).

## 5. Relation to the neighbouring counters

- Zone +0x4A0 is only the element count of the +0x498 list; it is written by the
  constructor (0), the clear in 0x11A820 (0), each append (+1) and the destructor
  (0). No routine writes it independently.
- Zone +0x490 is not derived from the +0x498 list. In 0x2E3510, for each zone, it
  is reset to 0 and then incremented once per +0x480 element whose identifier is
  found in the World creature map at World +0x20 (ordered map keyed by signed
  64-bit at node +0x20). It is also restored from saved zone state in 0x2AE0C0.
- Zone +0x480 is written by 0x11A820 kind 0x17 (append), 0x1241B0 (clear then
  append) and 0x126160 (clear then append).

## 6. Readers of the +0x498 list

- 0x2E3510 (World step; already known to the caller): for each zone, when World
  byte +0x358 is zero, for every listed identifier that resolves to a creature
  (0xE720) whose byte +0x60 is not 8, ORs 0x100 into the creature's 16-bit +0x128.
- 0x131620, reached through 0x2E1750 at the end of World update 0x2DA6C0 (after
  0x2E3510), and only when the World's local-player pointer (World +0x448) is
  non-null; 0x131620 itself returns early when the player byte +0x68 equals 0x9C.
  It scans a 7 x 7 window of zone coordinates (player zone -3..+3 on each axis)
  through the World zone table. For each existing zone whose +0x4A0 count is
  nonzero, it checks every listed identifier against the World creature map at
  World +0x20; only if ALL listed identifiers are still present does it mark the
  player's CURRENT region (from the player position, with the same even-alignment
  rule) in the player-owned sets at (pointer at player +0x1528) +0xD8 and, unless
  the region kind is 0xF..0x12, also +0xC8. A zone with an empty list or with any
  missing listed creature contributes nothing. Separately, the same routine marks
  regions of nearby creatures that carry the 0x100 bit at +0x128 set by 0x2E3510.
  The meaning of these player sets is not resolved here.
- 0x2F9940 traverses it only to free nodes.
- No other routine with a Zone +0x498 access was found (section 7).

## 7. Candidates checked and excluded

- 0x44B40, 0xDF8F0, 0xF2340: their +0x498 accesses are a 32-bit constant write, a
  float write, and a hash-mask read of a different object respectively; not this
  list.
- Other assembly accesses at displacement +0x498 (0xCE5F0, 0xD3170, 0xE9530,
  0xF9AC0, 0x12A6F0, 0x1510F0, 0x170700, 0x177F50, 0x27D2B0, 0x2B0400, 0x2BB8E0,
  0x2EB7B0, 0x2F44A0, 0x38E100) address stack frames or unrelated objects.
- Two further appends via 0x12A10 (0x14E00 and 0x126160) target other lists.

## Evidence boundaries

- Static evidence only; nothing was executed. Role of World byte +0x358, of the
  callers of 0x2AE0C0, and therefore whether client/viewed Worlds also generate and
  populate this list, is not proved. The creation path itself contains no
  role test before generation, so any World that creates a zone through 0x2AE0C0
  populates the list; the 0x2E3510 consumer is gated by +0x358 being zero.
- 0x2FF6F0 failed to decompile; that 0x11A820 is always reached from it was
  inferred from the assembly near 0x317145 only.
- Searches covered decompiled functions for displacement 0x498 and assembly for
  "+ 0x498]" and "+ 0x4A0]". An access through a base register already offset to
  another Zone field would evade these searches; none was identified.
- Exact values of creature types produced by 0x1420F0, 0x142190, 0x13FFC0,
  0x1292F0, the descriptor from 0x131540, tile-type semantics, and the meaning of
  record kinds 1, 7 and 8 were not examined.
- Callers of the Zone destructor (vtable dispatch) and therefore the exact unload
  and World-reset triggers were not resolved.

Exposure statement: Read in full the role policy docs/CLEANROOM_AUTHOR.md. From the
original evidence I read: decompiled function files 140012a10, 1400075c0,
140119530, 140129ae0, 1402f9400, 1402f9940, 1402fa3b0, 1402ae0c0, 14007d5c0,
1402e1750, 1401274d0, 14011cd80 in full; excerpts of 14011a820, 140131620,
1400f2340, 1402e3510, 14011e7f0, 140121770, 140120bc0, 140123210, 1401241b0,
140125720, 140126160, 1402da6c0 and the header of 1402ff6f0; content-search hit
lines in other function files; excerpts of assembly.txt; search results from
calls.csv and functions.csv. Before the task began, the session harness
automatically placed the project's AGENTS.md text and a git-status note in this
agent's context; they contain process policy only (no implementation source,
tests, data or behaviour), nothing from them is used here, and the coordinator
should judge that exposure. No reconstruction source, SDK, web or other project
file was read.
