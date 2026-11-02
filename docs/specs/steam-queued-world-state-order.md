# Steam queued WORLD resource order and retained effect inputs

Independent native-only bounded follow-up: **0xee2f0/0xee990**, opaque
consumer **0xe6e60**, faded consumer **0xe7980**, terrain0xe5070 and their
ordinary0xe2e00 caller. Native sources and independent prose only; no engine,
tests, extractor, owner data/captures, SDK, reconstruction or game execution.
Descriptor fields/setters remain [WORLD controls §2](steam-world-pass-controls.md#2-ordinary-queued-model-controls).

## 1. Exact opaque key and instance ordering

Opaque grouping uses the submitted **resource object's pointer value**, as
an **unsigned64 numeric key**. It does not use catalog ID, filename, geometry
contents, vertex/index-buffer identity, item ID, material or distance.
The key at tree node+0x20 is compared unsigned at **0xee4c4–0xee4e5**;
the opposite helper uses the same comparison at **0xeeb70–0xeeb91**.
The tree lookup/insertion is ascending, and0xe6e60 visits it in ascending
in-order sequence from its leftmost node.

All submissions with exactly the same pointer in the same context/winding
family share one group. Aliased catalog entries returning the same pointer
therefore share that group; distinct resource objects remain separate even
if their data or backing buffers agree. The group's descriptor list sentinel
and count are tree-node+0x28/+0x30.0xee2f0/0xee990 append through0xed7f0
at the existing tail.0xe6e60 visits sentinel.next forward, preserving that
resource's **enqueue order**, without per-resource distance/material sorting.

These rules are local to a context and winding family. For each opaque
consumer call, normal groups precede opposite groups even when an opposite
resource has a lower address. Separate renderer contexts do not merge their
groups. Faded vectors preserve their own append order and are not grouped
or sorted by resource pointer in0xe7980. Full family/pass conditions remain
[pass-family §§3–4](steam-retained-item-and-map-pass-family.md#3-queue-context-versus-winding-family-versus-opacity).

**Native address order is not a portable catalog order.** No equivalence of
catalog IDs, filenames or constructor sequence to the resource pointer order
is established here. The runtime addresses and alias relationships are inputs
to this native order; a catalog-only record does not determine it. This applies
to choosing the final descriptor/state as well as to geometry submission order.

## 2. Readiness and point/control writes

In0xe6e60, **0x16eba0 is tested once per resource group before its descriptor
loop**. Failure skips every descriptor in that group. There is no per-instance
WORLD/material/scalar/point-table submission from that failed group. A ready
but empty descriptor list likewise produces none of those submissions.
Changing a pipeline state to normal/opposite is not a reset of these buffers.

For each visited descriptor of a ready group in ordinary **mode2**, select
point tables from its transformed full-header center, then submit WORLD and
material/effects as already specified. Missing/null selected zone supplies
zero tables, not the preceding descriptor's zone. Exact center conversion,
table offsets and16-cached/10-uploaded distinction remain
[scene lighting §3.2.1](steam-scene-lighting.md#321-exact-per-draw-sprite-center-and-zone-table-binding).
This selection still occurs when descriptor Glow is1.

Faded0xe7980 first visits its nonempty depth-only normal/opposite vectors:
WORLD and DepthOffset change per record, but material, other scalars and
points are retained. Its later color passes visit normal then opposite,
in each vector's append order, selecting points and submitting **material,
Shininess, Glow, Opacity, White, DepthOffset** for every record. These writes
occur **before0x16ec60's buffer-readiness check**. A valid-header record whose
draw fails readiness can therefore become the final source of all those
values, including its selected or zero point tables. Hardware clipping,
zero draw counts or lack of visible pixels likewise does not undo setters.

If both faded vectors are empty,0xe7980 makes none of these writes. Mode2
empty opaque/faded consumers do **not** bind zero points merely because they
contain no drawable descriptors. The mode-without-bit2 zero-table behavior
in0xe6e60 belongs to its separate overlay/direct caller contract.

## 3. State immediately before and after0xe6270 entry

The ordinary sequence is terrain, opaque context **Renderer+0x138**, opaque
context **+0x1d0**, faded context **+0x1d0 at0xe3c86**, then the effect batch
**0xe6270 at0xe3c97**. Ordinary context+0x138's faded consumer runs later,
after water, and cannot determine the earlier effect-entry state.

For Opacity, Shininess, Glow, White, DepthOffset and point tables, the final
source before0xe6270 follows this priority, falling through only when the
listed source does not exist:

| Priority | Final source |
| --- | --- |
| 1 | Last opposite faded **color** record in context+0x1d0, even if its later draw fails readiness. |
| 2 | Last normal faded color record in context+0x1d0, with the same readiness distinction. |
| 3 | Last descriptor of the highest-address **ready, nonempty opposite opaque group** in context+0x1d0. |
| 4 | Last descriptor of the highest-address ready, nonempty normal opaque group in context+0x1d0. |
| 5 | The corresponding final opaque source in context+0x138: opposite family first if any ready nonempty group exists, otherwise normal. |
| 6 | Values entering those model consumers from earlier frame/terrain work. |

The preceding table selects descriptor values and that descriptor's point
tables, not the last successfully rasterized model. Failed opaque groups do
not supersede an earlier source; failed faded color draws can. The final
faded color passes also supersede their own earlier depth-only DepthOffset
writes. No model-distance or catalog-ID tie break is introduced.

For the fallback, the already specified frame scalar initialization is
**Opacity1, Shininess0, Glow0, White0, DepthOffset0**. Terrain0xe5070 sets
White0 and does not replace the other four scalar controls. At **0xe5166**,
it binds each traversed zone's+0x240/+0x340 point tables when that zone's
mesh-group count+0x1d0 is nonzero, before its group draws. If that is the last
point binding, those tables survive through empty/skipped model consumers.
If no such binding occurred, these model consumers retain their incoming
point-table state; this supplement does not assign it a new numeric-zero
default or audit shader construction to establish one.

Distinguish this pre-call snapshot from the batch prologue:0xe6270 immediately
selects **state0**, sets **White0** and **Glow1**. It does **not** set Opacity,
Shininess, DepthOffset or point tables. Its preceding+0xb0 helper0xdc610 changes
WORLD/material, not those retained controls. Thus the +0xc0 repeated-cube path
retains the priority-selected **Opacity/Shininess/DepthOffset/points**, while
its Glow/White are the batch's explicit1/0. Its per-cube material/WORLD writes
and later state limits remain [owned-ground cubes §5](steam-owned-ground-cubes.md#5-shader-pipeline-and-inherited-state).

## 4. Scope and comparison limits

This is a bounded ordinary-mode2 contract for valid resource headers,
consistent collections and finite descriptors. It does not establish recovery
from invalid pointers, concurrent mutation or failed allocation. It does not
equate native heap keys with a portable resource registry. A comparison of the
native terminal state requires the relevant context/family, resource-address
keys and aliases, per-resource enqueue order, readiness outcome and final
descriptor point-table selection; catalog identity alone is insufficient.
