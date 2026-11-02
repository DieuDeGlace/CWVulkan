# Steam ordinary terrain: 250-block band boundaries and order

Stable bounded native-only supplement, 2026-11-01. Sources are the ordinary
face producer **0xe9530**, its reached packing/resource/publication calls,
and terrain draw **0xe5070**. No game, implementation, tests, extractor,
SDK, owner data or captures were used. Other geometry families retain
their existing contracts.

## 1. Existing prepass establishes the anchor

[Scene lighting §1.1](steam-scene-lighting.md#11-exact-bounds-prepass-and-bandgroup-count)
already specifies the full ordered bounds prepass. Retain its **lowest
exposed block Z, L**, and **upper tracker U**, including the north-column
update ordering and final U>=1. The allocated band-array count is
**N=trunc((U−L)/250)+1**. The signed division/count construction is at
**0xe9c61 onward**. The ordinary finite contract assumes nonoverflowing,
allocatable signed32 spans.

The grid is anchored at **L**, not zero or an absolute multiple of250.
The prepass has not merely found a maximum terrain height. Its initial
no-exposure values are L0 and U0, with U subsequently raised to1; that
case still allocates one band array. It does not itself force a drawable
group when no vertices are produced.

## 2. Source-block band and signed boundaries

In the ordinary face pass, for each available column scan Z upward from
**minimum available own/west/east/south/north stored base−1**, stopping
before the column's **own exclusive stored end**. XY traversal is ascending
X outer/Y inner. This scan start is not newly clamped to L.

For each scanned source block at signed32 Z, compute band index by signed32
**(Z−L)/250 with division toward zero**, then base **L+250×index**. These
are integer operations, including the ordinary native32-bit difference/
base arithmetic; no float division, nearest rounding or absolute-Z floor
is used. The direct calculation is **0xea1a6–0xea1c2**. It occurs before
the effective-type/face-admission decisions for that source block.

All faces of a source block use **that source block's band**. A block at
L+249 belongs to band0; its top corners at world Z=L+250 are stored in
band0 with local Z250. The block at L+250 belongs to band1 and starts at
local Z0. A face is not split/reassigned because one corner touches the
next band's plane.

Arithmetic reference, not a capture: with **L=−251**, source Z−2 has
index0/base−251/local block Z249, with its top at local250. Source Z−1
has index1/base−1/local0; source Z0 also has index1/base−1, at local1.
Negative absolute Z therefore needs no separate absolute-height band rule.

The below-L scan may calculate a signed quotient before rejecting hidden
geometry; the reached code adds no new band-index zero clamp. L is defined
from exposed effective blocks in the preceding prepass. Behavior when
columns mutate inconsistently between those phases is not established
here, and no recovery/remapping for that situation is inferred.

## 3. Local corners and array insertion order

Each band's shared vertex array receives the reached faces in the source
scan's order, filtered to that band. Within a block, ordinary faces are
**west,east,south,north,bottom,top**, omitting hidden faces. Each face appends
four vertices and its index list references that band's current vertex
base; vertices are not shared merely because corners coincide.

The packed corner coordinates are signed32 arithmetic before low-byte
packing: **world corner X/Y minus the target zone's block X/Y origin**,
and **world corner Z minus the selected band base**. Color/light corner
sampling still receives the physical world block-lattice coordinates.
The exact four-corner order and diagonal selection remain
[vertex inputs §7.1–§7.3](steam-voxel-vertex-inputs.md#71-four-corners-and-face-order).
The source block's ordinary local Z is0..249, while upper corners may
reach250. These values fit the existing eight-byte vertex representation.

Each band has one shared vertex array and separate surface index arrays.
Ordinary nonwater faces enter the solid category, or the lava category
for effective lava17; their exact admission/material rules remain
[vertex inputs §8](steam-voxel-vertex-inputs.md#82-groups-pass-and-lavas-actual-lighting-override).
Other categories can also contribute to the same shared vertex array;
absence of solid indices alone does not establish an empty band.

## 4. Empty bands, publication and directly reached draw order

After face production, resource preparation visits allocated band indices
ascending. It creates a shared vertex resource only for a nonempty shared
CPU vertex array, and an index resource only for a nonempty respective
index array. With ordinary successful resource creation, a populated
band can have some null category index resources. The existing strict
shared-vertex-count cutoff below65536 selects16-bit GPU indices; otherwise
32-bit indices are retained. That changes width, not order/count.

The final pending group list is cleared before new groups are appended.
Visit the original allocated band indices ascending and append a group
**only when its shared vertex-resource pointer is nonnull**. Its signed32
base is stored at group+0x5c. Advance the base by250 for **every allocated
band**, including omitted empty bands; the native increment is at
**0xecaee**. Published group order is thus ascending original band order,
without replacing the retained base by dense published index×250.

For example, allocated bases **−500,−250,0** with only the first and third
populated yield two published groups whose bases remain **−500 and0**.
With no shared vertices/resources in any band, the pending group list
is empty even though the prepass allocated at least one band array.
Group/count/bounds publication and active-list handoff remain scene-lighting
§1.1; the count is the number of published groups, not N unconditionally.

Terrain0xe5070 walks each zone's published groups forward. Within a group,
its directly reached index-resource order is **+0x10, +0x30, +0x38,
then +0x28**, subject to the existing nonnull/readiness checks. For this
ordinary producer, nonwater faces populate solid+0x10 or lava+0x28 as appropriate;
the intervening optional+0x30/+0x38 categories receive no ordinary indices.
Their established pass/light rules remain vertex-inputs§8.2. GPU translation
from the retained signed band base is
[WORLD coordinates §2.1](steam-world-gpu-coordinates.md#21-terrain-uses-zone-origin-plus-a-vertical-band-base).

This closes the ordinary band formula, negative/boundary-height behavior,
local emission and directly reached list/category order. It does not add
other geometry-family rules, an exhaustive concurrent-column snapshot
guarantee or a new safety policy for inconsistent/overflowing inputs.
