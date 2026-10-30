# Bounded Steam dungeon decision and block oracle

Independent clean-room recorder specification from the owner's Steam native
decompile/assembly. Addresses are RVAs in the inspected executable; no engine,
test or exporter sources were read, no game was launched, and this document
contains no native implementation excerpts. This supplies the labeled evidence
requested by `worldgen-parity-gaps.md` §§7.2/8/10, rather than asserting that
every dungeon branch is already verified by the existing zone totals.

## 1. Scope, bounds and provenance

The proposed recording has two linked parts: compact native **decision records**
for selected16-block cells and their eight neighbours, and exact **stored
six-byte column records** from the corresponding freshly generated zones.
The portable record names below are a proposed logical schema, not claims
about a native file format. An exporter may keep its existing encoding.

A practical first run is at most **16 explicitly selected target cells**,
their3×3 cell neighbourhoods and the complete zones containing those cells.
Deduplicate cells/zones by coordinates while retaining each target's links.
Use explicit configurable bounds for zones, bytes and trace events; sensible
recording defaults are16 targets,64 zones,256 MiB block payload and65536
decision/probe events. These are recorder limits, **not game constants**.
On a limit, mark the affected case incomplete and report the omitted range;
never emit a truncated case labeled complete. Filtering must not alter or
early-return native generation. No search, capture or launch has been run
by this specification author.

Every case needs an owner executable fingerprint/module image identity,
spec/schema version, public world seed **world+0x220**, actual signed32 zone
seed **world+0x244**, world+0x330/+0x334 terrain values, selected zone/cell
coordinates, generation invocation ID and stage. Preserve32-bit values as
raw words as well as signed interpretations where relevant. Native address
or allocation identity may be diagnostic but must not be an equality key.
Cell coordinates here are world **16-block cells**, zones64-block squares,
place cells1024-block squares, and rooms128-block squares. Do not call all
of these “cells” in the wire format. Floor division for spatial partitions
includes negatives; the separate division by3 in §4 is toward zero.

Use bounded explicit seed/coordinate manifests from existing owner captures
first. A search may query a bounded declared coordinate rectangle and report
the predicates it found; failure to find a branch is not proof of absence.
At minimum select named **big-structure42/detail11** and **44/detail12** cases,
style1 dome and non-style1 cases, all four floor edges and corners, a room
with flags bit4 set and clear, and signed-negative floor values if reachable.
Record which rows remain uncovered. Arithmetic-only synthetic coordinates
from the earlier audit are not automatically reachable dungeon fixtures.

## 2. Stable native helper interfaces

These entries are useful as bounded query points or call/return observation
points. The table gives actual argument order, independent of an exporter's
language syntax. World is a pointer, coordinates/results signed32 unless
stated otherwise. Windows x64 uses RCX,RDX,R8,R9 for the first four integer/
pointer arguments; at **function entry**, argument5 starts at stack+0x28,
then8-byte slots. A char/bool input uses its low byte; don't substitute a
pointer to that byte. An instruction-site observer must preserve all live
integer/SIMD registers and flags; the ordinary function ABI's nonvolatile-only
guarantee is insufficient in the middle of a native routine.

| RVA | Arguments in order | Result |
| --- | --- | --- |
|0x36160|World,placeX,placeY|Low-byte big flag |
|0x2b99d0|World,placeX,placeY,big|Place kind |
|0x362a0|World,out8,placeX,placeY,big|Returns out8: two signed32 landmark block coordinates |
|0x2aa270|World,placeX,placeY,placeKind,big|Big-structure kind,0 when none; internally rounds big place coordinates down to even |
|0x31a5d0|World,placeX,placeY,out32|Unbounded-by-distance place record, layout §3 |
|0x2bb060|World,placeX,placeY,big|Realm/style integer; not itself culture0..6 |
|0x69c60|World,cellX,cellY|Cell/building type |
|0x2a8290|World,out4,cellX,cellY|Returns out4: packed castle-cell record §3 |
|0x2aadd0|World,cellX,cellY|Raw castle block-top height, before edge smoothing |
|0x2ab0a0|World,cellX,cellY|Final castle floor after smoothing and optional room increment |
|0x2aa450|out8,roomX,roomY,direction,anchorRoomX,anchorRoomY,bigStructure,placeX,placeY|Returns out8: room tuple §3 |
|0x2a04e0|Same9 arguments as0x2aa450|Base corridor-room tuple before off-corridor activation |
|0x2aa250|World,packedCastleCell32|Rise:20 for castle style byte1, otherwise16 |
|0x2feaf0|pointer to packed castle-cell record|Depth:40 for kind9,20 for kind10,8 for kinds11/12,otherwise0 |

The small room/castle helpers use coordinate choices rather than decorative
CRT draws in their bodies. The giant zone generator and furnishing/detail
passes do consume CRT state. Do not re-run those passes inside an observer
to obtain labels. A helper-query result and an observed result from the
actual generation invocation should carry distinct provenance tags.

## 3. Native output layouts and validity

**Castle-cell record0x2a8290:** four consecutive unsigned bytes, in order
**kind,detail,style,steps**. Export each byte and the packed little-endian
word; don't confuse detail11/12 with kind11/12 or room type11/12. Its empty
return writes the whole word0. Its early ordinary-building return writes
the whole word1, hence the other three bytes0. The main generation path
consumes these four-byte records for current and neighbouring cells.

**Room record0x2a04e0/0x2aa450:** five meaningful bytes at offsets0..4:
**type,flags,style,side,steps**. There is no native16-byte semantic tuple.
Use at least8 writable bytes for a helper output, but don't serialize tail
padding as initialized data. For a nonempty base corridor room, side is0;
off-corridor activation chooses side1=west,2=east,3=south,4=north.

**Correction to the older room summary:** an absent base room writes bytes
0,1,2 and4 to zero, **leaving byte3 unwritten**. When off-corridor activation
fails, that absence and unwritten side remain. Record raw bytes optionally,
but use per-byte validity **0x17** for a type0 room and **0x1f** for nonzero
type. Do not make a zero-initialized capture buffer into evidence that the
native absent-room side is zero. Bytes5..7 have no semantic equality role.

**Place output0x31a5d0, nominal32-byte record:**

| Offset | Type | Meaning/validity |
| --- | --- | --- |
|0|signed32|Kind, always written |
|4|byte|Big, always written; bytes5..7 are padding |
|8|signed32|Variant, always written,0 for kind0 |
|0x0c|signed32|Radius,250 small/500 big; written only when kind nonzero |
|0x10,0x14|signed32 pair|Landmark block XY; written only when kind nonzero |
|0x18|float32|Squared-distance field: not filled by this helper |
|0x1c|float32|Weight field: not filled by this helper |

The per-column completion0x31a690 fills the last two fields later. A zone's
initial place record is not the distance-gated camera-place result0x35ce0.
An oracle must not give it camera reach semantics or compare its unwritten
distance/weight slots as generation outputs.

For each queried world cell, retain both the raw1024-block place coordinate
and the even-rounded big-place coordinate, big flag, kind and big-structure
kind. The room input context needs all9 arguments listed above, not just room
XY. **Castle topology** uses the anchor supplied by0x2a9df0, converted from
16-block cells to128-block rooms; **floor increment** computes its anchor
room from landmark block XY divided by128. Record those two contexts under
separate roles, even if their values happen to agree in a selected fixture.

For an observer on0x2aa450, the primary castle-cell room call returns to
**0x2a8574** (call site0x2a856f); the floor-increment call returns to
**0x2ab360** (call site0x2ab35b). Other calls from0x2a8290 query adjacent
rooms and should retain their actual room coordinates and a separate role.
Use the return address and enclosing invocation ID, not just a coordinate
match, to identify these roles. A castle-cell early return can skip room
queries entirely; record that absence instead of inventing a tuple.

An optional once-per-zone header observation at **0x2ffb75**, after the native
biome result was stored, provides the generator's own selected context:
zone pointer frame−0x80; place record frame+0x1568 with §3's validity;
big-structure kind signed32 at frame+0xa0; realm/style signed32 at frame−0x54;
biome signed32 at frame−0x4c; selected region-cell signed32 XY at
frame+0x1890/+0x1894. R12D/EDI hold the place coordinates used by the preceding
realm query, including big-place rounding. These are frame-RBP-relative.
Save the raw unrounded zone-place coordinate separately from the zone header
and spatial partition rule. Do not confuse the region-cell pair with either
place-cell or16-block-cell coordinates.

## 4. Exact floor decision observation points

**0x2ab0a0** has small, verified stable observation sites in this executable.
An entry/exit wrapper can retain World/cellX/cellY and a per-thread call ID.
The following locations refer to state **before the instruction at that RVA**;
there are no copied instructions here. Use a nested per-thread invocation
stack if helpers are also queried by the recorder, and tag such queries.

| Site | Native state available | Event |
| --- | --- | --- |
|0x2ab0d1|EAX signed32|Raw current block top returned by the first0x2aadd0 call |
|0x2ab14f|EDX/R8D signed32 pair|Actual selected neighbour cell XY, immediately before its raw-top call; this site is skipped when no edge qualifies |
|0x2ab154|EAX neighbour raw top; EBX current raw top|Pre-clamp/pre-average operands |
|0x2ab162|EBX signed32|Post-smoothing floor, also reached when there was no neighbour; then equals raw current top |
|0x2ab35b|RCX output pointer; all9 room-call arguments in the normal call ABI|Floor-increment room context; this site is skipped when big-structure kind is0 |
|0x2ab360|Five room bytes at current stack+0x50; EBX smoothed floor|Native room tuple before flags bit4 is tested |
|0x2ab3dc|EAX signed32|Final floor after optional increment, before common return handling |

The selected edge is the first matching **west,south,east,north** edge of the
8×8 cell room. Exactly one is used at a corner. Record `neighbourPresent`
separately; don't manufacture a neighbour raw top when the site was skipped.
Smoothing clamps the chosen neighbour to at least the current raw top, adds
it to that current top in32-bit arithmetic and divides signed toward zero by2.
Record currentRaw,neighbourRaw,clampedNeighbour,postSmooth,final and all
observation-presence bits. `clampedNeighbour` may be a recorder-derived
diagnostic; label it as derived if not observed directly.

The optional increment exists only when the floor-specific room flags have
bit0x04. Its coordinate input is **cellX/3,cellY/3 with signed truncation
toward zero**; its R result is reduced as unsigned32 modulo6. The difference
final−postSmooth labels the actual increment0..5 when that branch occurs,
but a difference0 alone cannot prove that the branch was skipped. Preserve
the observed room flags and whether the room site ran.

At the call site0x2ab35b, argument5 is at current stack+0x20 (before the CALL),
argument6 at+0x28,7 at+0x30,8 at+0x38,9 at+0x40. This differs by8 from the
function-entry offsets in §2. The output pointer points at stack+0x50.
Do not mix the pre-call and function-entry stack conventions.

If instruction-site observation is unavailable, separately querying
0x2aadd0(current),0x2aadd0(the specified neighbour),0x2ab0a0(current) still
provides a useful bounded oracle. Mark `postSmooth` and edge selection as
**derived from the independently specified rule**, not as observed intermediate
native state. It cannot claim the same evidence strength as the table above.

## 5. Generation stages and exact stored block payload

**0x2ae0c0(World,zoneX,zoneY)** may return an already cached zone without
generating anything. A new zone is seeded with the32-bit CRT input
**zoneY×1024+world+0x244+zoneX**, constructed by0x2f9400, then passed to
**0x2ff6f0(zonePointer)**. Observe the actual call, not merely a successful
zone lookup. Native zone header: World pointer+8, signed32 zoneX+0x10,
zoneY+0x14; allocation size0x58518.

| Stage label proposed here | Stable native site/state | Meaning |
| --- | --- | --- |
|initial_columns|0x3022c3 inside0x2ff6f0; zone pointer at frame RBP−0x80|Initial whole-zone column loop completed; later overhang/structure/content passes have not all run |
|pre_occupancy|0x318f6c; RCX is zone pointer|Derived lists0x31d470 completed; about to run occupancy0x31d700 |
|post_occupancy|0x318f71; zone pointer at frame RBP−0x80|Occupancy marking completed, before generator cleanup |
|fresh_full_generation|0x2ae17e in caller0x2ae0c0; RBP is zone pointer, R14 World|0x2ff6f0 has returned, before stored-zone state is applied |

The **fresh_full_generation** site is the preferred final oracle.0x2ae0c0
subsequently applies stored-zone state when world byte+0x358=0, then publishes
the zone. A snapshot after that caller returns requires its own
`loaded_with_stored_state` label unless absence of applicable stored state is
established separately. Existing live, refreshed or simulated zones must keep
those stage labels. Mesh-time model+0x28/flag8 changes are a later stage too.
Do not compare intermediate counts/blocks with fresh full generation under
one undifferentiated “final” label.

Stored column ABI, verified with the existing terrain spec and native zone
layout: **zone+0x518+0x58×(localX×64+localY)**, local axes0..63. Base Z is
signed32 at column+0x3c. Vector begin/end/capacity are64-bit pointers at
+0x40/+0x48/+0x50. The byte span end−begin must be divisible by6; its records
run upward from baseZ in order. Each block is exactly **red,green,blue,glow,
type,flags**, six unsigned bytes. This is stored data, not the effective
getter's implicit below-sea water or a solver's temporary light-map byte.

For every recorded column preserve local/world XY, baseZ, exact record count
and all six bytes in increasing Z. Serialize columns in localX outer/localY
inner order and zones in explicit manifest order. A lossless run encoding may
combine consecutive equal **six-byte records**, but must preserve total run
length and exact sequence. Do not strip air, normalize RGB/glow, discard
occupancy bits or trim “unused” lower/upper stored records. Empty columns need
an explicit empty/count label; their base word must not be promoted into a
meaningful span when count0.

Copy owned bytes at the chosen stage. Deferred serialization may not retain
raw vector pointers past generation/loading or a later mutation. A generic
live-save lock pair cannot turn a later live snapshot into a pre-load snapshot.

## 6. Realized level setup and intermediate column capture

For direct evidence of what the common castle-column pass actually consumed,
**0x308e7a** in0x2ff6f0 is a useful once-per-floor site: the9-cell type/kind/
floor/top arrays have just been assembled and the16×16 column loops have not
started. The following are **frame-RBP-relative**, not zone offsets:

| Frame offset | Type/meaning at this site |
| --- | --- |
|−0x80|Zone pointer |
|−0x78,−0x68|Signed32 current world16-block cell XY |
|−0x5c|Signed32 floor index; negative denotes castle level |
|−0x28|Packed4-byte current castle cell |
|−0x48|Signed32 current floor z0 |
|+0x48|Signed32 current top0 |
|+0x120|Signed32 current room depth0 |
|+0x28|Byte dome enabled |
|−0x18|Signed32 wall thickness |
|+0x84|Signed32 door width |
|+0xa0|Signed32 zone big-structure kind |
|+0x8160|Nine packed4-byte castle-cell records |
|+0x8188|Nine signed32 floor heights |
|+0x81b0|Nine signed32 cell/building types |
|+0x81d8|Nine signed32 tops |

The9-element arrays are X outer/Y inner over offsets−1..1. Indices are
**SW0,W1,NW2,S3,C4,N5,SE6,E7,NE8**. Link by world coordinates rather than
assuming this is the W,E,S,N,SW,SE,NW,NE order used elsewhere. On the castle
level the presence decision is the corresponding castle kind!=0. Retain
absent entries' raw native arrays as diagnostics, but label their presence;
they do not contribute to the castle interpolation merely because a numeric
floor value exists. Record current and neighbour kind/detail/style/steps,
floor and top, with depths either observed from0x2feaf0 or explicitly derived
from its verified table in §2.

**0x30b255** is the corresponding after-common-columns site, before the
castle liquids/spiral stairs/beam and later furnishing/detail pieces. Zone
and current cell/floor fields above are still usable; floor index−0x5c,
z0−0x48,top0+0x48 and depth+0x120 are not per-column scratch outputs. Copy
the target cell's currently owned column spans, label `after_common_columns`
with the cell and floor index, and don't call that a completed zone. Earlier
cells/floors and initial terrain coexist with the current result here.
The halo structure loop can process portions outside the owning zone;
record only its actual owned columns and link an adjoining-zone capture
when the target crosses an ownership boundary.

This intermediate plus pre/post-occupancy and final captures distinguishes
common ceiling blocks from later detail11 beams/detail12 cones and final
occupancy flags. The raw stored block values at a final surface alone do
not identify which earlier pass wrote them.

### 6.1 Optional dome and completed-floor probes

These additional sites are verified in the same assembly and remove the need
to guess which temporary owns a dome result. Filter by the selected owning
zone, target cell and floor index; use the cell/floor setup from §6 as context.
For the column loop, **R15D is localX, R14D localY**, both0..15.

| Site, before its instruction | Fields available |
| --- | --- |
|0x30aca8|Dome byte frame+0x28; weighted floor double XMM11 low64; weighted top double XMM10 low64; current pre-dome integer ceiling frame+0xd4 |
|0x30aced|Dome's noise-lowered weighted top double XMM10 low64, before edge processing |
|0x30afa0|Final edge weight float XMM2 low32, before complement/cubing |
|0x30affe|Cubed blend weight float XMM3 low32; weighted floor double XMM11; lowered top minus1 double XMM10; interpolated pre-floor double at pointer RCX (also frame+0x15a8) |
|0x30b003|Signed32 integer dome ceiling EAX returned by0x2ff6c0 |
|0x30b009|Final ceiling signed32 at frame+0xd4, for dome and non-dome paths |

**0x2ff6c0 takes a pointer to a double**, not a double value in XMM0, and
returns signed32 mathematical floor for the finite in-range values used
here. The three middle dome sites and0x30b003 are skipped when dome is false.
Pillar/skipped columns may bypass this whole set; retain explicit event
presence rather than filling missing results with zero. Save floating values
as raw IEEE bits in addition to any printed decimal. The9-cell input arrays
from §6 provide the corresponding presence/weighted-height inputs; weights
at these probes are observations, not a recorder's reimplementation.

**0x312dc3** is a stable completed-floor point immediately before incrementing
the floor index. For a floor that reached the §6 setup, all of its common
columns, liquids/stairs, furniture/room objects and detail pieces have finished.
Save the target owned-column spans as `after_floor_details`, linked to that
setup ID. Current floor index is R14D and frame−0x5c before the increment;
zone/cell fields remain those in §6. This address is also reached by a skipped
floor, so require an observed matching setup ID; otherwise label `floor_skipped`
and do not falsely claim its pieces ran. Keep z0/top/depth from the earlier
setup record rather than reading unrelated reused scratch slots here.
Later floors, other cells, roofs and later generation stages can still
overwrite these columns; this remains an intermediate, not the full-zone
final capture.

### 6.2 Optional detail11/12 draw provenance

The following **return sites** expose EAX before the caller reduces a CRT rand
result. Pair them with a generation-wide CRT draw ordinal when such a trace
already exists; don't restart/reseed or draw a replacement number in a probe.

| Return site | Meaning of the raw EAX draw |
| --- | --- |
|0x3127ab|Detail12 height: modulo5+8 |
|0x3127cb|Detail12 radius: modulo4+3 |
|0x3128c8|Detail11 axis: nonzero modulo2 gives along-X beam,zero along-Y |
|0x31291a|Detail11 along-X optional lamp test: modulo4=0 permits lamp |
|0x312985|Along-X lamp along-position: modulo2 selects4 or12 |
|0x312a40|Detail11 along-Y optional lamp test: modulo4=0 permits lamp |
|0x312a84|Along-Y lamp along-position: modulo2 selects4 or12 |

The lamp position draw is absent when its preceding lamp test fails. At
**0x3127f8**, just before the native hanging-cone helper0xc0cd0, its actual
local XYZ are EDX,R8D,R9D and radius/height are signed32 in current stack
+0x20/+0x28. At **0x312883**, the detail11 downward search has finished:
EBX is the resulting Z and EDI counts the downward advances, at most10.
Together these fields and the before/after-floor block snapshots distinguish
an incorrect native decision label from a shape-writing mismatch. No extra
draw occurs in the cone helper itself, as previously specified in the mason
audit.

## 7. Coverage and still-untraced evidence

The proposed records prove individual native helper decisions and final
ordered stored bytes for labeled cases. They do not establish every culture,
roof, room, reward, guard or ceiling branch by a zone-count total. Keep a
coverage manifest listing big structure,place kind,big,realm style,biome,
culture,cell/building type,room type/flags/side,castle kind/detail/style/steps,
floor index,dome and selected edge. Culture may be queried through0x25ea50
from realm/style and biome as in the existing tableT9; do not mislabel the
realm/style integer itself as culture.

For a bounded first diagnostic, helper records, realized level setup and
fresh full-generation column spans are sufficient. Optional intermediate
hooks can then localize an observed block difference. No source trace here
establishes that the selected16 targets cover every native reachable branch.

The optional dome and detail11/12 probes are now mapped in §§6.1/6.2. Still
untraced here are equivalent draw/site maps for **other** decorative detail
codes, individual per-neighbour accumulation totals before the weighted
floor/top result, and a standalone portable ABI for every giant-generator
temporary. These aren't needed to collect the bounded target cases above;
if a mismatch requires one, extend the observation list from that specific
branch rather than inventing a stack offset or recomputing a purported native
intermediate in the recorder.

The kind8 dungeon's last-qualifying F/D scan, guard activation cells and
spawn guard-area deep payloads remain the separate evidence request in the
earlier audit/creature-zone spec. This document's room/floor/block capture
does not silently claim those deep spawn fields were recorded. Likewise,
unwritten object lamp turns and raw generation model+0x28 must keep their
existing provenance; this oracle does not turn them into initialized data.
