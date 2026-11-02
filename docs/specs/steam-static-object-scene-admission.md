# Steam ordinary static-object scene admission

Independent native-only bounded audit of the ordinary **0xd3170 →0xe0ac0**
route, especially generic doors/windows. Native sources and existing independent
prose only; no engine, tests, fixtures, extractor, owner data, SDK or reconstruction
were inspected, and the game was not run. This specifies admission before the
existing object transform/enqueue contract, not alternate map, GUI or effect routes.

## 1. Containing-zone traversal is separate from environment models

The later ground-item/static-object traversal in **0xd3170** reads the local
player pointer at **World+0x448**, then physical fixed X/Y at **+0x10/+0x18**.
Calls **0x16e50 at0xd8741/0xd874c** convert those coordinates to signed32 zone
coordinates. This helper forms a signed32 block coordinate before its negative
coordinate reconstruction and floor division by64; it is not a float camera,
drawn-position or renderer-origin query.

This traversal requires **draw flag0x80 clear** at **0xd8751–0xd8755**.
For ordinary nonoverflowing loop coordinates, visit the inclusive square
**player-zone X−3..X+3, Y−3..Y+3**: increasing X outside, increasing Y inside.
The bounds and integer setup are **0xd875b–0xd87a9**. Lookup **0x16e00 at0xd87b9**
only queries the World's loaded-zone map; it returns null for an absent zone.
It does not load or generate one.

A nonnull zone is the containing-zone admission on this route. Its ground-item
loop precedes its static-object loop. This later route does **not** require that
zone's cached mesh-group count, active mesh/visibility result, cached-center
distance or zone-box frustum test to pass. Those earlier environment-model and
terrain gates must not be applied to this static-object route.

Within each admitted zone, visit the vector **begin+0x60/end+0x68** forward,
with **0xe0-byte records**, including records independently of their type or
interaction-state byte. There is no sort or selected-object-index-list filter
before the following individual test. The list setup is **0xd9152–0xd9167**;
the record test/enqueue call is **0xd9190–0xd9244**.

## 2. Individual center, radius and current-scene test

For each record, **0x13a960 at0xd919a** obtains float32 extents **Sx,Sy,Sz**:
the type's0x13a520 base extents multiplied separately by record scale+0x24.
The helper does not apply the record's turn, hinge/swing pose or open/closed
state to these extents. Existing sizes and visual transforms remain
[world-generation parity §4](worldgen-parity-gaps.md#4-verified-static-object-scale-centering-and-doorwindow-pivots).

The sphere center is object fixed **X+8, Y+0x10**, and fixed **Z+0x18 plus
half Sz**. Promote Sz to double, multiply by double0.5, then **0xcb9a0**
multiplies by double65536 and truncates toward zero to signed64. Add that
fixed offset to Z with the native low64 addition. This uses the object's
base-extents height, not a voxel resource AABB or its occupied vertices.

The supplied radius is **float32(max(Sx,Sy,Sz)+1)**. Native max operations
and the addition are **0xd919f–0xd91ba /0xd91f3**; the retained addend is
float1 loaded at **0xd3257**, constant0x4ac980. This is the full largest
extent plus1, without a half-size or0.6 factor. For example, a scale1 type3
door with extents(2,1,3) has center Z+98304 fixed units and radius4 blocks.

At **0xd9222–0xd9231**, call **0xe08c0** with that center/radius and distance
limit **renderer+0x49c**, without adding64 or the sphere radius to the limit.
Use its existing [integer XYZ early-distance arithmetic](steam-scene-lighting.md#12-exact-early-distance-arithmetic-versus-float-fade-distance)
and all-six-plane sphere test. Range equality is admitted; a strictly negative
plane result is rejected. The distance anchor is current Renderer+0x338 fixed
eye; plane coordinates use current Renderer+0x428/+0x430 origin and the six
installed planes. These are the already established distinct scene inputs.
Only success reaches **0xe0ac0 at0xd9244**. There is no terrain LOS query here.

## 3. Generic helper admission and enqueue

**0xe0ac0** recomputes the extents at **0xe0b43** and its half-height fixed
center, then obtains the ordinary **0xce300** fade at **0xe0b93**. Its distance
input is **renderer+0x49c**, explicitly loaded at **0xe0b55**, without +64.
Use the scalar fade formula in
[WORLD parity §9.6](worldgen-parity-gaps.md#96-ordinary-model-fade-uses-its-raised-center-without-a64-block-extension),
with the static-object center defined in §2 here. This does not replace the
preceding range0x49c sphere test. With unchanged record inputs the caller and
helper's two center constructions agree.

The base model is selected through **0x1e6330 at0xe0c0b**. The generic base
model branch requires a **nonnull resource**. For doors/windows3..7, the
already specified hinge/static transforms lead to the ordinary normal-family
enqueue **0xee2f0 at0xe2613**, context Renderer+0x138, with that fade as pixel
Opacity. No additional object-center, voxel-bounds, radius, frustum or distance
test is inserted within this generic branch. Open/closed state changes its
specified pose rather than removing these door/window records from this route.

The enqueue's finite Opacity admission and later GPU-resource readiness remain
[WORLD controls §2.1](steam-world-pass-controls.md#21-pose-material-delivery-readiness-and-explicit-winding).
There is no producer-side conversion of a not-ready resource to a different
queue or containing-zone visibility requirement.

## 4. Limits

This covers stable ordinary records and representable finite arithmetic. It
does not establish recovery for null local-player state, concurrent record/list
mutation, overflowing zone-loop bounds or invalid extents/conversions. Other
object-specific extra geometry/effect branches, alternate scene callers and
catalog loading/provenance are outside this supplement. The earlier environment
model admission retains its own zone and per-resource sphere rules.
