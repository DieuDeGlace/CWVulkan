# Steam source-zone demand, mesh preparation and scene windows

This note describes the ordinary original Steam routes, not a replacement
engine policy or an observed owner's loaded-zone count. Evidence is the
original executable/decompile at the cited RVAs and existing independent
prose. No implementation, exporter, captures, SDK implementation or other
engine was inspected, and the game was not executed.

## 1. What the default integer 15 means

The setting is the signed32 value at Game+0x1b9c. Options' normal range is
5 through100 in steps of5, with default50; see
[frontend-options.md](frontend-options.md) §1. The ordinary scene caller
0xa5a50, at0xad863–0xad8b3, multiplies the setting by30 using signed32
arithmetic and divides by100 toward zero. A quotient below4 is replaced
with3. Call0xad9d4 supplies the resulting integer N to0xe2e00, which stores
it at renderer+0x438 at0xe2fe1. See
[steam-scene-lighting.md](steam-scene-lighting.md) §6.1.

At setting50, N=15. It is an inclusive **zone-index radius**, not a
diameter or a count of15 zones. A zone is64 blocks wide in both horizontal
directions. The adaptive scene scan0xecda0 visits camera-zone X−15 through
X+15 and Y−15 through Y+15:31 by31, or961 candidate positions. Its
iteration is ascending X outer, ascending Y inner. These are candidate
positions; they are not961 promised loaded, prepared or drawn zones.

There are separate source-generation and mesh-preparation workers. Their
batches are not single rendered frames, and their centers need not agree
with the final camera's zone even with a stationary player.

## 2. Signed fixed coordinates and distinct centers

One block is65536 signed64 fixed units. Native narrowing below means
retaining the low32 bits and interpreting them as signed32. The bounded
ordinary counts in this note assume that subsequent signed32 additions,
subtractions and squared integer keys do not overflow. No additional
world-boundary clamp was found in the cited coordinate producers.

### 2.1 Source-generation center

Client source worker0x84f70 takes the local creature through
Game+8 → World+0x448. It reads physical fixed X/Y at creature+0x10/+0x18.
For each component it divides by65536 toward zero, then narrows to signed32
blocks. It does **not** perform a negative-fraction correction before
using those block coordinates. This is directly visible at0x850ed–0x85111.
The host source worker0xf2340 uses the same truncation/narrowing for its
collected player positions.

For creature byte+0x68 equal to0x9c, the client worker instead takes
signed32 block X/Y from+0x1b0/+0x1b4 at0x85115–0x85122. The mesh worker
and host source collection have the corresponding substitution. This note
does not assign additional semantics to that ability state.

The source zone containing such a block coordinate is its mathematical
floor divided by64. Its lower and upper demand bounds are obtained by
first subtracting/adding the block radius in signed32, then applying this
floor division; the native boundary calculations are0x8518b–0x8522a.

### 2.2 Mesh, eviction and adaptive-scene center

Mesh coordinator0x842d0 instead performs the following additional check
after division/narrowing: for a negative original fixed component, it
reconstructs a fixed coordinate from the **sign-extended narrowed block
integer**, and compares that with the original signed64 value. If unequal,
it decrements the narrowed block integer with a32-bit result. It then floors that
block integer divided by64. Evidence is0x84373–0x8442c. Its0x9c
substitution uses the supplied signed32 block coordinates directly.

Client eviction0x2d2410 uses this same negative reconstruction/check on
the local creature's physical XY. It does not itself use the0x9c block
coordinate substitution. Renderer adaptive scan0xecda0 applies the same
negative reconstruction/check to fixed camera XY+0x338/+0x340; see
0xece14–0xeceaf.

Where both division and correction stay in signed32 block range, the corrected route gives
mathematical floor from fixed position to blocks and then zones. Outside
that domain, narrowing occurs **before** the negative reconstruction test;
it is not a full signed64 zone-floor operation. A wrapped negative exact
block coordinate can still fail the reconstruction test and be decremented.
The broad “floor” wording in older scene-window prose must be read with
this narrowing boundary.

For ordinary physical positions, representative one-axis results are:

| Fixed position, in blocks | Source block / source zone | Corrected mesh or same-position camera block / zone |
|---|---|---|
| 0 | 0 /0 | 0 /0 |
| 63.5 | 63 /0 | 63 /0 |
| 64 | 64 /1 | 64 /1 |
| −0.5 | 0 /0 | −1 /−1 |
| −64 | −64 /−1 | −64 /−1 |
| −64.5 | −64 /−1 | −65 /−2 |

The scene's fixed tuple is the supplied final-eye tuple, not the local
player physical position or the negative look-at render origin. See
[steam-world-pose-invocation.md](steam-world-pose-invocation.md) §2 and
[steam-body-particle-coordinate-lifetime.md](steam-body-particle-coordinate-lifetime.md).
The ordinary scenery zone walk in0xd3170 additionally converts the computed
signed32 camera-zone components to float32 and back to signed32 before
forming its ±N loops. This is exact at ordinary coordinates; beyond2^24
zone indices it can round an odd index. The adaptive0xecda0 scan keeps its
zone center integer. Neither route is a substitute for the player-centered
worker windows.

## 3. Client source demand: setting50 admits793 fresh positions

Source worker0x84f70 independently derives a block radius R. At0x85128 it
multiplies the setting by1920 in signed32, divides by100 toward zero, and
replaces results≤64 with64; the fallback integer is at constant RVA0x432e84.
This is a different producer from renderer N. There is no upper setting
clamp in these calculations themselves.

Using the source block center from §2.1, it scans both inclusive tile bounds
floor((block−R)/64) through floor((block+R)/64). In the resulting square,
the zone-offset squared distance from the source center zone must be at
most **(trunc(R/64)+1) squared**. Offset components are signed32, each is
converted to double before its square; the sum and inclusive comparison
are double. The radius integer is squared before conversion. This gate
occurs at0x853d1 onward.

Only a missing loaded-map entry or an entry whose Zone pointer is null
enters the candidate vector. Existing nonnull zones do not become new
source candidates just because their mesh is absent or dirty. The map is
the client's World+0x460 loaded-zone map.

At setting50, R=960 blocks and trunc(R/64)+1=16. For any ordinary source
integer block phase, the tile bounds are source-zone±15 inclusive. Thus
the fresh demand offsets satisfy both −15≤X,Y≤15 and X²+Y²≤256. There
are **793** such positions. The complete integer radius16 disk has797
positions, but the four axis endpoints at±16 lie outside this square.
The count793 is an arithmetic consequence of the native gates, not a
runtime capture or a guarantee that all requests have completed.

For example, source integer block center(6400,6400) is zone(100,100).
The bounds are zones85..115 on both axes. Their complete tile-box extent
is blocks5440 through7424 exclusive on each axis, a1984-block span before
the circle removes corner tiles. A camera in a different zone has a
different scene window even though the player is stationary here.

Candidate priority uses squared horizontal distance from the source block
point to the tile's block-coordinate box. That is a scheduling key, not a
second radial rejection. The vector is sorted by0xf0790 and, if necessary,
shrunk to its first **four** entries through0xa2630 before dispatch0x7d5c0
at0x855d7. Reached generation calls0x2ae0c0 insert/generate client-owned
zones. The worker can perform further batches; four is not a loaded-zone
or per-rendered-frame limit.

At setting5 the distinction from the scene minimum matters: N=3, whereas
R=96 and the source circle radius is2. The inclusive source bounds contain
four tile indices on each axis, positioned according to the block's
within-tile phase. Intersecting that square with the radius2 disk gives11
source positions. If a lower/out-of-UI setting invokes R's64-block
fallback, the source bounds are three by three and all nine pass its
radius2 circle. It does not acquire the scene's seven-by-seven N=3 window.

## 4. Retention and eviction are separate from fresh demand

Mesh coordinator0x842d0 calls0x2d2410 eight times with N+1; one call site
is0x8456b. The helper scans loaded nonnull zones and can remove **one**
selected far zone per call. It starts the distance threshold at
(N+1)² and replaces its choice only on a strictly greater squared zone
distance. Equal-radius zones are retained. For very far offsets exceeding
twice the supplied radius in either component, it substitutes four times
the radius squared as its comparison distance. Consequently selection
among such very far entries is not a promise of exact geometric farthest
order. The local physical center uses §2.2, not the source truncation route.

At N=15, its ordinary retention threshold is radius16. A zone at distance²
256 survives this helper. The four radius16 axis endpoints excluded from
fresh source demand can therefore remain loaded if already present.
Movement, worker progress, center differences and other callers can leave
additional transient loaded entries. Neither793 nor797 is an invariant
loaded-map size. The eight calls permit at most eight removals during this
coordinator iteration; they are not extra source-generation rings.

## 5. Mesh preparation: loaded square, neighbors, cost and eight jobs

Coordinator0x842d0 derives N from the same setting×30/100 and below4→3
rule, including at0x84526. Around the §2.2 player/ability center it scans
the inclusive ±N square, looks up the client's loaded map and retains
nonnull Zone pointers. It sorts these24-byte candidate records by squared
zone-offset distance through0x7bb20. This collection has no additional
source-demand circle or camera-frustum test. At default50 it considers at
most961 positions before missing-zone filtering.

The selection pass reads the setting-derived N again at0x849ab. Its
preparation cost budget is obtained from signed64 N×N×98304, converted to
float32, then multiplied by the stored float32 value0.8 twice, separately,
and truncated to signed64. The0.8 word is0x3f4ccccd at RVA0x42e530.
At N=15 this budget is14,155,776. This is distinct from the per-frame
adaptive scene budget in §6.

In sorted order, the running signed64 cost starts at0. A candidate with
zero signed32 cost Zone+0x444 is excluded if the running cost is already
strictly above budget. A nonzero-cost candidate remains eligible for the
remaining tests even above budget. An eligible candidate becomes a job
only if its cost is zero **or** dirty byte+0x23c is nonzero, its3×3 loaded
neighborhood passes0xce3e0, and fewer than eight jobs have been selected.
The cost is added for each candidate admitted to this outer cost branch,
even if its dirty/neighborhood/job-count test did not produce a job.
The reached selection is0x84a30–0x84beb, followed by dispatch0x7d680 at
0x84c19. Eight is the per-iteration selected-job limit, not an eight-zone
world or an eight-job-per-render-frame guarantee.

Helper0xce3e0 requires nonnull loaded Zone pointers at every offset
X=−1..1 and Y=−1..1 around the candidate, including the candidate itself.
It looks them up through the **renderer's installed World+0x460**, whereas
the coordinator collected its candidates from Game+8. In the ordinary
matching-world case these refer to the same client's world. This helper
does not test a frustum, mesh-group count or resource readiness. A requested
world mismatch must not be silently replaced with an assumed common map.

As a conditional count only, if the complete loaded set were precisely
the793 default source positions with matching centers/worlds,673 centers
would have all nine neighbors in that set. Cost, dirty state and the job
limit still apply. This derived673 is not a native constant, a guarantee
of eventual meshing or an observed settled-zone count.

Completed mesh work, pending versus active groups, bounds and publication
are distinct later states; see
[steam-scene-lighting.md](steam-scene-lighting.md) §1 and
[steam-local-world-zone-ownership.md](steam-local-world-zone-ownership.md).
The availability of a Zone pointer alone does not imply visible geometry.

### 5.1 Candidate key, input order and exact tie algorithm

The candidate vector has a 24-byte stride. Its defined fields are the Zone
pointer at+0, signed32 squared-distance key at+8, and signed32 zone X/Y
at+0x0c/+0x10. Construction at0x8470a–0x8472e computes the two squared
zone offsets and their sum with 32-bit multiplication/addition. These are
exact nonnegative integer keys at the normal UI settings; the sorter does
not convert them to float. Arithmetic outside that range retains the
32-bit result and uses its signed interpretation, rather than an unsigned
or widened-distance comparison.

Before sorting, the coordinator enumerates **ascending signed X outer,
ascending signed Y inner**, appending only the nonnull loaded entries
encountered by those lookups. Evidence is the loop setup at0x84597–0x845ea
and the append construction above. This is not loaded-map hash iteration.
Membership can change between lookups; the enumeration order does not
assert an atomic snapshot of zone availability.

The call to0x7bb20 supplies the actual appended candidate count as its
initial partition budget. All reached comparisons use the signed32 key
at+8 only. The Zone pointer and X/Y remain associated with that key and
provide no secondary comparison. The later cost comes from the pointed
Zone+0x444, not from a cost field participating in this sort.

For these integer keys, this is the **same decision algorithm** specified
in [steam-controller-handoff.md](steam-controller-handoff.md) §§4.1–4.3
and, for finite float keys, in
[steam-scene-lighting.md](steam-scene-lighting.md) §2.6.2. The reached
specialized helpers are:

| Decision or operation | Native RVA |
|---|---|
| Sort control, recursion and budget reduction |0x7bb20|
| Stable insertion for subranges of at most32 entries |0x758d0|
| Three-way partition and equal-band movement |0x79cb0|
| In-place pivot sampling / associated-field exchange |0x705a0 /0x7d8b0|
| Heap construction / extraction / repair |0x76970 /0x7b460 /0x7a600|

The same three-sample case applies through41 entries and the same
nine-sample case from42. Sample rearrangement, right-then-left partition
searches and the two equal-band rotations have the order in the linked
contract. Each partition reduces the budget by first taking its integer
half, then adding the integer half of that half. The shorter remaining
partition is recursively sorted; equal lengths recurse on the right and
continue on the left. A still-large subrange with exhausted budget uses
the heap fallback.

Heap repair chooses the right child on equal keys and stops upward
insertion on an equal parent. During heap construction the upward limit
is the original parent being repaired; extraction repairs from root0.
Insertion moves a record only across strictly greater keys. Thus a total
input of at most32 records preserves its initial grid order among ties.
Above32, the partition/heap exchanges can change tie order, and no later
pass restores the initial order or adds a coordinate/pointer tie-break.

The differences from the linked specializations are the record layout,
signed integer key and initial enumeration source, not a distinct sorting
policy. This establishes the reached mesh-preparation sorter, not every
other native sorting caller or a portable memory-address equivalence.

## 6. Actual frame drawing and range are additional gates

The adaptive scan's camera-centered961-position square is established in
§1. Its missing/unmeshed lower-corner tests, published bounds admission,
float-key sort and signed geometry-cost accumulation are independently
specified in [steam-scene-lighting.md](steam-scene-lighting.md) §§1,2.6
and2.6.2. Its budget is N×N×12288, or2,764,800 at N=15. The resulting
float32 adaptive range at renderer+0x4a4 depends on availability, geometry
cost, camera position and its existing transition state; it is not always
N×64.

The ordinary scenery lookup in0xd3170 also visits camera-zone±N when its
scene exclusion flag0x80 is clear, with the float-center qualification in
§2.2. Family-specific mesh/readiness/bounds/distance tests occur after a
zone lookup. Ordinary static-object admission is separately documented in
[steam-static-object-scene-admission.md](steam-static-object-scene-admission.md);
the environment-model and ordinary solid terrain bounds/pass rules are in
[worldgen-parity-gaps.md](worldgen-parity-gaps.md) §§9.5–9.7.

The ordinary scene uses two distinct derived float32 ranges:
renderer+0x49c is the minimum of the adaptive range, float(N×64)×float0.2,
and200; renderer+0x4a0 is the minimum of the adaptive range,
float(N×64)×0.5, and256. At N=15 the respective setting-only ceilings are
192 and256 blocks. Consumer-specific margins such as+64 remain specific
to those consumers. None of these facts converts the source-demand square
or disk into a guarantee that every candidate contributes visible pixels.

## 7. Host near-player source demand is independent of render distance

Host source worker0xf2340 collects physical XY from kind0 creatures in
its embedded World's creature registration list, with the truncation and
0x9c substitution in §2.1. This is a per-player union, not the client's
camera window. The reached producer does not read the client render-distance
setting to form its wanted set.

For each collected signed32 block point, it scans inclusive tile bounds
floor((block−270)/64) through floor((block+270)/64) in each axis. It also
requires each absolute zone offset below6 and the double sum of squared
zone offsets≤25. The double25 constant is at RVA0x43fd20. Accepted tile
pairs are deduplicated in the wanted set before0x2d2670 is called.

Let r be the source integer block's nonnegative remainder within its
64-block zone. One axis has these relative bounds:

| r | Inclusive tile offsets |
|---|---|
| 0..13 | −5..+4 |
| 14..49 | −4..+4 |
| 50..63 | −4..+5 |

Intersecting the two-axis box with the radius5 integer disk gives77
wanted tiles if both phases are in14..49,78 if exactly one is, and79
if neither is. For example, an integer point with both remainders0 gives
79; both remainders14 give77. These are arithmetic single-player-set
counts, not observed loaded sizes. Multiple players produce the union,
whose size depends on overlap.

The directly reached0x2d2670 removes loaded entries outside the supplied
wanted set by first collecting them, then unregistering/destroying them;
this is distinct from the client's eight calls to one-at-a-time farthest
eviction. The subsequent host cache/job scheduling is not exhaustively
specified here, so the wanted-set count does not assert a host per-batch
completion count. Client packet15 zone-answer handling is another path:
[netcode-roles.md](netcode-roles.md) §4.2 documents that an absent/null
host zone produces no answer in that tick, rather than establishing an
additional render-distance-sized host demand window.

## 8. Bounds of this contract

The exact50-setting counts describe the cited ordinary routes and finite
nonoverflowing tile arithmetic. Source and mesh workers can sample the
setting at different times; the mesh worker itself resamples it. They are
not an atomic per-frame snapshot. The note does not establish a universal
settled mesh count, all connection/request/cache producers, all exceptional
world replacements, or loaded lifetimes under concurrent edits/unloading.
The exact called gates and narrowing distinctions above remain facts even
where those broader lifetimes are open. No performance-motivated reduction
of the original footprint, portable substitute sort order, or missing-zone
generation fallback is specified.
