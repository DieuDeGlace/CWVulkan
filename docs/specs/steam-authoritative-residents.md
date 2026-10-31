# Steam authoritative near-player resident pass

Stable bounded native audit, 2026-10-31. Sources are the original Steam native
routine **0x2e17d0**, its reached collection/insertion helpers, and the previously
specified daily refresh and creature defaults. No replacement implementation or
runtime capture was consulted. This document refines
[worldgen-creatures.md §7.1](worldgen-creatures.md#71-which-spawn-points-become-creatures)
and [remaining-runtime-gaps.md §6.1](remaining-runtime-gaps.md#61-refresh-is-visited-by-the-authoritative-near-player-spawn-pass).

## 1. Admission and collection phase

The routine returns without this work when **World+0x358 is nonzero**. Its
ordinary World-step caller is **0x2da8df**, inside **0x2da6c0**. The helper has no
elapsed-time argument or positive-time test. Thus an admitted zero-time World
substep reaches the same resident admission work. This is World-substep work,
not a once-per-rendered-frame operation. See
[steam-world-creature-visitation.md](steam-world-creature-visitation.md) for its
relationship to the later creature update.

The first traversal is the **World+0x40 registration list**, forward from its
head. Every entry whose **kind/hostility byte +0x60 equals zero** contributes.
There is no HP, local-player-ID, visibility, distance-to-camera, or class test.
In particular, a registered dead kind-zero creature still contributes. The local
player is included through this ordinary membership, without separate insertion
or priority in this helper.

Three collections are completed before any selected zone is refreshed:

* A temporary list of borrowed kind-zero creature pointers, appended in World
  registration order, using **0x36c20**.
* A deduplicated ordered set of borrowed **Zone pointers**. Its key is the
  pointer value, compared as **unsigned 64-bit**, using **0x73f40**.
* Two temporary sets of existing region-coordinate pairs. Each pair consists of
  signed 32-bit **creature+0x150/+0x154**, ordered lexicographically by X then Y
  using **0x745e0**. State **+0x128 bit 0x8000** contributes to the set used by
  the final petrification pass; **bit 0x0800** contributes to the set used by
  new residents with appearance flag 0x04000000. These keys are copied from the
  existing player fields; this phase does not resample region from position.

These are snapshots of membership/region keys for this call. A new kind-zero
resident appended later does not expand these collections during the same call.
The player list holds pointers, not copied positions; the later distance test
reads each pointed-to creature's physical position.

## 2. Inclusive loaded-zone bounds and actual zone order

For each contributing creature, use physical signed-64 fixed **X/Y at +0x10/
+0x18**, with **65536 fixed units per block**. Drawn position is not an input.
First add/subtract **0x1000000 fixed units = 256 blocks**, independently for each
axis. Each of the four endpoints is then converted to a block coordinate and
then a 64-block zone coordinate.

The block conversion first divides fixed signed-64 toward zero by 65536 and
narrows to signed 32-bit. For a negative fixed input, the routine compares that
original fixed input with the sign-extended narrowed block result multiplied by
65536, and decrements the narrowed block result when unequal. It then floors
the resulting signed-32 block coordinate divided by 64. In the ordinary
coordinate range this is the usual mathematical floor. Preserve the narrowing
stage for extreme coordinates; do not substitute an unlimited-precision floor.
The same conversion detail is discussed in
[steam-positive-terrain-ray.md §2](steam-positive-terrain-ray.md).

The minimum and maximum zone coordinates are **inclusive**. Within one player's
rectangle, X increases in the outer traversal and Y increases in the inner
traversal. Each key is looked up in the existing **World+0x460 loaded-zone map**.
A missing key or a null mapped Zone pointer is skipped. This collection does
not request creation/loading of a missing zone.

For ordinary finite coordinates the rectangle is 9 by 9 keys. For example,
physical XY=(0,0) selects X=-4 through 4 and Y=-4 through 4, before the loaded
map test. At XY=(63.5,-0.5) blocks it selects X=-4 through 4 and Y=-5 through 3.
Z is not used to collect these keys.

After all players have contributed, actual processing traverses the Zone-pointer
set **in ascending unsigned pointer order**. This is neither coordinate order
nor first-player/first-insertion order. An identical pointer reached from
several keys/players is processed once. The key collection order above must not
be mistaken for the processing order. Native pointer addresses, or the actual
selected-zone processing sequence, are needed to reproduce this order in an
external oracle; coordinates alone do not encode it.

## 3. Per-zone refresh and common spawn admission

At **0x2e1d7c**, call **0x31c540** for the selected zone, **before** obtaining and
walking its current **+0x460 spawn-point map**. Daily refresh may remove old
daily point records and add new ones, so a pre-refresh copy is not the walked
collection. Refresh behavior is specified in
[worldgen-creatures-zone.md §4](worldgen-creatures-zone.md) and
[remaining-runtime-gaps.md §6](remaining-runtime-gaps.md).

Visit the resulting spawn map in its native in-order traversal: ascending
**signed-64 map key**. The signed comparison is also directly present in the
spawn-map range/erase helper **0x2ff4f0**. The record's stored ID is **S+0x38**;
it is that stored ID which is used for creature admission/construction.

The common admission decisions are, in order:

1. Look up that ID in **World+0x450**, the live-ID map. An existing **nonnull
   creature pointer** suppresses construction, regardless of that creature's
   HP. A present key with a null pointer does not suppress construction.
2. If not suppressed there, exact membership in **World+0x20**, the dead-ID
   set, suppresses construction.
3. Otherwise traverse the temporary kind-zero player list in its registration
   order. Some player must satisfy **0xf3e20** with the spawn's original fixed
   position S+0/+8/+0x10, the player's current physical tuple, and radius
   **256.0f**. Stop on the first success. There is no player-HP or LOS test.

The distance helper subtracts fixed signed-64 coordinates before converting
differences to double block units. It rejects absolute X/Y differences greater
than 256, then admits the full XYZ squared sum at **less than or equal to
256²**. The precise shared precheck arithmetic is in
[steam-gameplay-camera.md §6.4](steam-gameplay-camera.md). Thus the loaded-zone
rectangle is only a broad collection window; it is not the final spawn range.
The later mode-0x8a altitude change is not part of this distance test.

The existing spawn prerequisites at S+0x920/+0x928/+0x938/+0x948 remain additional
gates; see worldgen-creatures §7.1. One correction: the required-stage scan tests
registered **class byte +0x14c == 0xa1** and stage byte **+0x19c** against the
requested stage, **without an HP test**. “Living” in the old description was too
restrictive. The nonempty +0x928-cell consumer is now completed in
[steam-resident-effects-prerequisites.md §§1–2](steam-resident-effects-prerequisites.md),
including its retained-byte quirk. A filling producer remains unidentified.

## 4. New resident writes and registration

A successful point creates a **fresh 0x15f8-byte creature**, initialized by
**0x4ce80 at caller 0x2e23e3**, using S+0x38 as its ID. There is no creature
object reuse or copying into an existing nonnull live-ID entry. Constructor
defaults are in worldgen-creatures §7.3; they do not consume CRT random draws.
The existing §7.1 transfer list remains applicable. These ordering/width details
are independently confirmed here:

* Physical XYZ and home XYZ are initially the spawn's fixed XYZ. Home-cell XY
  uses the fixed-to-block floor/narrowing operation of §2, followed by floor
  division by **16**, rather than 64.
* Yaw is the stored float at S+0x18. Kind is the byte S+0x1c, race the 32-bit
  S+0x20, level S+0x24, people the low byte of S+0x28, +0x974 the 32-bit S+0x2c,
  and style +0x978 is the **zero-extended unsigned-32 S+0x30 stored in 64 bits**.
* Class/spec and stage, appearance, the fixed 16-byte name, equipment,
  collection, reward item/quantity, gold/auxiliary fields, roaming/ability
  collections, owner, and home-place pair are copied before region/HP/mode.
  The source/destination offsets remain those in worldgen-creatures §7.1.
* Region XY is obtained at **0x2e2651** through **0x2ada20**, from the copied
  physical X/Y divided by 65536 **toward zero**, then narrowed to that helper's
  signed-32 block arguments. This region conversion does not use the negative
  floor correction used for home cells. Store the returned pair at +0x150/154.
* At **0x2e266a**, compute **0x5fac0(new creature)** and store HP at +0x180.
  Then copy the low mode byte from S+0x8bc to +0x68 and apply §5 if it is 0x8a.
  Copy the S+0x910 group collection to +0xc08 afterward.

For a new creature whose appearance flags +0x80 contain **0x04000000**, test its
newly computed region against the bit-0x0800 player-region set from §1. On a
match, clear only 0x04000000 here. Without a match, set kind+0x60 to **1**,
multiply each size float +0x84/+0x88/+0x8c separately by float word
**0x3f8ccccd = 1.100000023841858** (constant RVA **0x42e538**), and clear appearance
flag **0x8000**. This latter route retains flag 0x04000000. HP was computed
before these changes and is not recomputed here. This is a new-resident branch,
not an operation repeated on every existing creature on subsequent passes.

Finally **0x29d0d0 at 0x2e283a** appends/registers the fresh creature in World+0x40
and World+0x450. Consequently a later point with the same stored ID sees the
new nonnull entry and is suppressed. Its birth occurs before this World step's
later main creature traversal, as described in the visitation specification.

## 5. Actual random stream and mode-0x8a altitude

The direct random call is the imported **CRT rand at 0x2e2686**. It is not the
coordinate hash R, not a per-spawn-ID generator, and not a new generator
constructed for each resident. The CRT stream/state contract is
[worldgen-creatures.md §0.1](worldgen-creatures.md). In particular, this is the
executing thread's persistent stream. **0x2e17d0 has no srand call and neither
saves nor restores that stream.**

For a newly copied mode byte **0x8a**, consume one draw r. Increase only raw
physical Z+0x20 by **(r remainder 50 + 80) × 65536 fixed units**: integer
arithmetic giving an integral **80 through 129 block** offset. Home Z retains
the original spawn Z. There is no float jitter calculation, second range test,
or reroll for an already registered nonnull creature. Suppression by the
common gates occurs before this draw.

The immediately preceding daily refresh is an actual possible seed producer.
If zone+0x4f8 already equals World day+8, **0x31c540 returns without reseeding**.
Otherwise its seed call at **0x31c72f** sets the same CRT stream to
**R(H(zoneX,zoneY)+0x4373e1)+day**, with 32-bit wrapping arithmetic. Its subsequent
shopping/camp and reached helper draws run before resident creation. Refresh
does not restore the former stream afterward. See worldgen-creatures-zone §4
for the complete refresh draw ordering, including reached helpers' seeds.

Thus a mode-0x8a height depends on the incoming thread stream, refresh work,
the actual pointer-ordered zone sequence, and which preceding resident points
reached a draw. It is deterministic given those inputs; the older “not
reproducible” phrase was imprecise. The first seed of the executing thread and
all other possible CRT consumers outside this pass/refresh are **not newly
audited here**. Do not replace that missing incoming state with a guessed
per-ID seed or default seed.

## 6. Final flag pass and cleanup scope

After all selected zones, traverse the **current** World+0x40 registration list,
including newly appended creatures. For each creature whose appearance +0x80
contains **0x00400000**, test its existing region+0x150/154 against the player
bit-0x8000 region set from §1. On a match, clear **only 0x00400000**, at
**0x2e2d36**. There is no kind or HP test. No HP, position, size, or owner field
is changed by this flag-clearing operation. This pass is inside **0x2e17d0**,
not a separate later World operation.

Temporary region sets, player-pointer list nodes/sentinel, and Zone-pointer
set nodes/sentinel are then freed. Their pointed-to players/zones are borrowed
and are not destroyed by that cleanup. This routine does not remove an old
registered creature merely because its source point disappeared or it moved
out of the collected zones. Broader residency/despawn helpers and the daily
World reset are separate; see [steam-world-daily-reset.md](steam-world-daily-reset.md).

## 7. Explicit remaining boundary

This checkpoint closes collection order, common admission, fresh-resident
transfer/postprocessing, CRT-state use, and the final flag pass. The remaining
per-zone **kind9 effect** producer and **nonempty S+0x928 consumer** are now
specified independently in [steam-resident-effects-prerequisites.md](steam-resident-effects-prerequisites.md),
including the cell traversal's retained-byte quirk. A filling producer for
that cell tree and broader arena/cleanup helpers retain explicit limits. No
claim is made here that every child helper in the entire World step is
random-free, or that incoming CRT state can be reconstructed from a spawn
record alone.
