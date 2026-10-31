# Steam encounter26 and constructor-produced role18

Stable bounded native audit, 2026-10-31. Sources are original **0x1224c0**,
**0x115df0**, **0x119a00**, **0x93480**, reached native helpers and binary
constants. Existing independent world-generation helper contracts are reused
below. No implementation, owner data or runtime capture was consulted.

## 1. Exact reachability conclusion and its limit

**0x93480** reads the planner's **+0x10 role map**, keyed by the supplied
signed-32 place pair. It returns the stored signed-32 role, or0 for a missing
pair. It does not insert, synthesize or update a role. Big-place normalization
is performed by callers such as **0x119a00**, not by this lookup itself.

The original planner constructor **0x115df0** builds the role map as described
in [worldgen-creatures.md §2.2](worldgen-creatures.md). Its role18 assignment
belongs to the late pairing pass. At **0x11860a**, that pass calls0x119a00 on
an original-pool place; **0x11860f–0x118612** skips the pairing work unless
the result is **24**. The role18 store is **0x1187ec**. The original selector's
direct results and candidate lists do not contain24, as independently
specified in [conversation-script.md §5.5](conversation-script.md#55-encounter-of-a-place-0x119a00).
The constructor's other assignment passes do not assign18. Thus the ordinary
**constructor-produced map has no role18**, and this construction path does
not enter the associated pairing work.

For a planner that **does contain18**, **0x119a00 returns26 directly**, before
its role16, role17/place-kind24 and candidate-list routes. Dispatcher
**0x1274d0 calls0x1224c0 at 0x127c77** for that result. The other identified
direct caller is **0x123210 at 0x1240b4**: its encounter24 helper invokes26
when its place has no pairing-map entry. The ordinary encounter selector does
not emit24 either.

**Later mutations of an already-built planner's role map were not
exhaustively audited here.** This is a constructor-path conclusion, not a
proof that every later ordinary runtime state lacks role18. Do not discard
the executable encounter26 behavior on that stronger assumption. The helper
has the concrete output contract below whenever it is invoked.

## 2. Entry values, seed and prototype item

Inputs are the planner and current zone. Derive the place P from the zone's
64-block origin divided down to a 1024-block place; floor the signed values,
and round both place components down to even when **0x36160** reports a big
place. This is the shared P convention in
[worldgen-creatures-roles.md §1](worldgen-creatures-roles.md#1-entry).
Read place kind through **0x2b99d0** and planner level through **0x12a200**.
Let H(P) be the existing wrapping32 P.x+57×P.y convention.

At **0x12264a**, seed the executing thread's CRT stream with
**R(H(P)+0x9a11)**. At **0x122653**, call **0x1408e0(-1)** and discard its
returned race. This nevertheless consumes **two CRT draws**: the base-list
choice and the sibling-variant choice. See worldgen-creatures §3.3 and
conversation-script §5.6. There is no restoration of the caller's incoming
CRT state.

Query the place's landmark block pair with **0x362a0 at 0x12267a**. Obtain
its region through **0x2ada20 at 0x122691**, using those block coordinates.
Then construct one reusable item prototype through **0x11a6c0 at 0x1226b3**,
with place kind, P.x/P.y, **low8 of planner level**, and the landmark's returned
region pair. Native argument loads explicitly supply level and region as the
fifth and sixth arguments.

The prototype's initialized semantic item fields are:

| Item offset | Value |
|---|---|
| +0x00 category byte | **0x16** |
| +0x04 signed32 subtype | low bit of wrapped32(P.x+64×P.y), plus **0x30** for place-kind20; otherwise plus **0x1b** |
| +0x08 unsigned32 modifier/seed | **R(H(P)+0xb9ad0e)** |
| +0x0c/+0x10 signed32 region | region of the landmark block pair |
| +0x14 byte | low8 of planner level |
| +0x18 full32 formula/realm field | **R(H(P)+0x38fba)** |
| +0x1c attribute count | zero |
| +0x1d..+0x9c attribute payload | all zero |
| +0x9d flags byte | zero |

These hashes do not consume CRT draws. Unlisted item padding is not guaranteed
by this construction; do not substitute a raw160-byte initialization claim.
Every ground item below receives the same prototype's semantic fields.

## 3. Candidate cells, shuffle and fixed per-cell budget

Build the shared **Camp cells** sequence: scan the place's 16-block cells in
X-outer/Y-inner order; require place weight **strictly greater than0.5**, a
plain procedural cell label **0–5,10–12 or99**, and the shared open-ground
predicate **0x128dc0**. The inline cell-label query at **0x122881/0x122894/
0x1228af** is the same **0x2d8630 → 0x35ce0 → 0x2d53d0** sequence wrapped by
**0x69c60**. It does not scan the zone's static-object vector.

Weight uses the existing place-cell distance/weight arithmetic: radius250
for a small place,500 for big; require d<radius² and
(1−d/radius²)²>0.5. Place-kind0 contributes no qualifying weighted cells.
The scan covers1024 blocks per axis for small places,2048 for big places.
If none qualify, use the single cell containing the landmark, floor-dividing
its block X/Y by16. These are the existing Camp cells/Place cells contracts
in worldgen-creatures-roles §1.1; no additional fits-the-kind test is applied.

**Shuffle the entire candidate sequence first**, through **0x115b80 at
0x122c46**. Only then, if its count exceeds20, truncate to its **first20
shuffled cells**, through **0x12a320 at 0x122c65**. There is no sort or
second shuffle. The CRT rejection-sampling shuffle is the existing §1.1
Shuffle contract: increasing index i from1, select within0..i using the
32767-based rejection rule, then swap. It consumes draws for the entire
pre-truncation sequence, not just the retained20. Ordinary candidate counts
are at most16384, within that shared single-draw/retry range.

For retained count N, compute signed32 **32 divided by N toward zero**, then
raise a result≤1 to1. This yields the fixed item cap per cell at
**0x122c80–0x122cad**. N is at least1 because of the fallback. Examples:
N=1 gives32; N=10 gives3; N=20 gives1. The cap is not a remaining global
budget and unused capacity is not redistributed.

Now visit retained cells in their shuffled order. For each, apply the zone's
ordinary **half-open4×4 cell bounds**. Out-of-zone cells cause no item or
guard creation and no further per-cell draws. Seeding, discarded race choice,
prototype construction and the full-place shuffle already occurred before
this clipping, even if no retained cell belongs to the current zone.

## 4. Ground items: count, placement, fields and draw order

For each admitted cell, query **0x1299a0 at 0x122de5** for its **kind16 spot
count in the original spot map +0x100**, not the remaining working map.
Narrow that count to signed32 and take its signed minimum with the fixed cap.
If the result≤0, create no ground items for this cell. The guard in §5 is
still created afterward.

For each of this cell's items, in order:

1. Copy the prototype's semantic item fields into a fresh 0xe0-byte ground
   record and initialize the fields listed below.
2. Take a **kind16** spot through **0x31bd90 at 0x122ee9**, copying its fixed
   XYZ to record+0xa0/+0xa8/+0xb0 without an additional height offset.
3. Consume one CRT draw at **0x122f0e**. Convert it to float, multiply by
   **360.0f** (RVA **0x42e7e4**, word0x43b40000), then divide by
   **32767.0f** (RVA **0x42e5f0**, word0x46fffe00). Store this float yaw at
   **+0xc0**. Preserve multiplication before division.
4. Append the record to the current zone's **+0x30 ground-item vector**,
   retaining item-loop order. The semantic copy helper is **0x3aee0**.

| Ground-record offset | Initialized value |
|---|---|
| +0x00..+0x9f semantic item | prototype from §2 |
| +0xa0/+0xa8/+0xb0 signed64 XYZ | taken spot tuple |
| +0xb8 signed64 | **-1** |
| +0xc0 float | yaw draw above |
| +0xc4 flag byte | **2** |
| +0xc8/+0xcc four bytes each | zero |
| +0xd0 signed32 | **-1** |
| +0xd4 signed32 quantity | **1** |
| +0xd8 signed64 reserved owner | zero |

Unlisted padding is not assigned a portable value by this contract. Flag2 is
the already specified per-player/no-automatic-pickup flag; ordinary ground
item handling remains in [use-prompt.md](use-prompt.md).

Taking a spot consumes its own conditional CRT draw **before** the yaw draw.
Use worldgen-creatures §§5.2–5.4 exactly: an existing empty working list is
refilled from the original map; taking a nonempty list uses rand remainder
size and removes that entry while preserving the rest's order. An absent or
still-empty working entry uses the documented zone-center fallback without
a draw. Do not lower the item count merely because earlier passes consumed
working entries: counting and taking intentionally use different maps.

## 5. One guard after each admitted cell's item batch

After that cell's complete item loop, allocate one fresh0x950-byte spawn
record through **0x118a60 at 0x122feb**. This happens even if the item count
was zero. Set these initial fields before the shared appearance/gear steps:

| Spawn offset | Value |
|---|---|
| +0x1c kind | **1** |
| +0x24 level | full signed32 planner level |
| +0x20 race | variant of **0x104**, as below |
| +0x28 people | **3** |
| +0x8b8 roaming field | **0x30** |
| appearance flags +0x58 | **0x2000**, before looks ORs its flags |
| +0x34 class byte | zero before gear |
| +0x8f8/+0x8fc home-place pair | P |

The routine first creates a one-element race pool containing0x104. At
**0x123013** it nevertheless takes a CRT draw to select that element, then
calls **0x1420f0 at 0x123027** for another draw. The native sibling group in
**0x13f0d0** is **(0x104,0x105)**, in that order. Thus the second draw's
remainder2 chooses the actual race.

Call **0x53e80 at 0x123077** with race by reference, the appearance block,
no supplied looks record, and wild0. For these two races it consumes the
shared **five initial appearance draws**, uses the established per-race
appearance contract, and retains the race. Then call **0x511e0 at 0x1230de**
with kind/race/level/current appearance flags/people and the existing output
fields for class/spec, equipment, ability collections, pick and inventory.
Preserve all its outputs and conditional draws according to
worldgen-creatures §§4.2–4.3. The initial class0 is not an instruction to
discard that helper's resulting class/spec or equipment.

After looks and gear, obtain position from **0x31c0e0 at 0x12315e**, the shared
Cell spot operation, and copy its raw fixed tuple to spawn+0/+8/+0x10. It
chooses the first original spot kind with a nonzero count from **2,7,16,1**,
else0, then uses the ordinary working-map take operation. This occurs after
all item takes and all gear draws; kind16 may be refilled/reused if selected.
Finally call **0x119530 at 0x123180** to assign/store the point under the
existing zone-ID contract.

The executable also contains the common alternate position branch for a
post-gear race0xec/0xed: draw at0x1230f1, and on an odd result take kind0
through0x31bd90 and set mode+0x8bc=0x8a; an even result uses Cell spot. The
verified built-in pool and no-record looks path produce only0x104/0x105,
and gear receives race by value. Thus that extra draw/alternate branch does
not run for this helper's built-in records. Their mode retains its default0.

This helper does not add cell prerequisites+0x928, dead-ID prerequisites+0x938,
or required-stage/count requirements to these points. Other fields retain
constructor values except the documented looks/gear outputs and placement/ID
writes. It does not add the points to tracked/mission-ID lists. Do not copy
encounter24's one-cell prerequisites or its static object/index producer here.

## 6. Overall order, outputs and bounded limits

The complete caller-level random order is: entry seed; discarded race
picker's two draws; prototype and candidate calculations without CRT draws;
the full candidate shuffle; then each in-zone cell's **all item spot/yaw
pairs**, followed by **guard pool selection, sibling selection, looks, gear,
guard spot take**. Referenced spot/gear helpers retain their documented
conditional draws. The next cell follows only after that guard's registration.
There is no balancing draw for an out-of-zone cell, no post-loop reseed, and
no caller-state restoration.

The direct outputs are ground records in Zone+0x30, spawn points in the
Zone+0x460 map, working spot-list consumption/refill, and the advanced CRT
stream. This routine does not place a static object or model and does not
populate Zone+0x4a8 or the planner's role/pairing maps. Temporary candidate
and race-pool storage is freed at the end. It has meaningful seed/shuffle
effects even on an invocation whose selected cells all fail the zone bounds.

This is a complete bounded contract for the native helper with original
helper/data semantics, composed with the independently specified appearance,
gear, spot and height/classification routines. It does not newly audit every
later writer of an already-built planner's role map, nor invent a route that
assigns role18 or restores the constructor's dormant pairing pass.
