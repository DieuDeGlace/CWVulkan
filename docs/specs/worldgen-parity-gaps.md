# Steam world-generation parity audit

Independent clean-room specification audit, 2026-10-29. Native sources and existing
specifications were inspected; engine sources/tests and third-party reconstructions
were not. No original game execution. The reported fixture results are evidence
from the primary implementer, not new tests run by this author.

## 1. Priority finding: street-lamp turns are undefined but consumed

Sources: **0x2ff6f0, 0x315fd8–0x316270** (street lamps), **0x2f93a0**
(object defaults), **0x13a520 / 0x13a960** (size), **0x31b2d0 / 0x31d700**
(settling/marking), and **0xe0ac0** (object drawing).

Decimal object types **63 = 0x3f** and **65 = 0x41** occur in the street-lamp
placement pass described in `worldgen-structures-buildings.md` §13. That placement
constructs a temporary object, selects its type, writes its position, settles it
and appends it when settling succeeds. It **does not write turn byte +0x20**.
The constructor also does not initialize that byte. It initializes scale **+0x24
to 1**, active **+0x28 to 1**, animation **+0x2c to 0**, owner **+0x30 to 0**, and
an empty item. The lamp turn is therefore prior stack data, not a specified zero,
random quarter turn, or another deterministic output of generation.

This explains why the reported same-position type-65 lamp has turn **48** in one
capture and **191** in later captures, and why four type-63 lamps carry 191.
Those observations corroborate the initialization trace; they are not by
themselves proof that the byte is unused.

**Consumers do read it:**

* Settling and occupancy marking test **turn bit 0** to swap footprint x/y.
  Scale-one sizes are **(2,2,8)** for type 63 and **(1,1,4)** for type 65.
  Swapping their square footprints changes neither dimension.
* The generic object renderer **0xe0ac0** includes both types in its normal
  model path and uses the **entire unsigned turn byte multiplied by 90 degrees**
  in its sine/cosine rotation. There is **no modulo-four mask before that
  calculation**. The byte is consequently an input to rendering. This audit
  has not established visual rotational symmetry of the actual lamp assets,
  and does not claim that their rendered orientation is unaffected.

Classify the **street-lamp producer's +0x20 only** as **uninitialized and consumed**.
It cannot have a deterministic byte-equality generation oracle across independent
native runs. Preserve/report the raw captured value separately; do not describe
it as padding, mask all object turns, or call it proven behaviorally irrelevant.
All initialized placement fields and all other producers' turn bytes remain
eligible for strict comparison. Reproducing one recorded lamp's rendering would
require retaining that captured byte; generating zero is a deterministic engine
policy, not a native guaranteed value.

## 2. Verified model table and size/yaw consumers

Sources: **0x264670 / 0x263210 / 0x2626e0 / 0x263ea0 / 0x68790** (tree/leaf
markers), **0x3191b0** (plant pass), **0x69b30** (model append), **0x177f50 at
0x1e2d30–0x1e2d58** (table initialization), **0x1739f0** (lookup), **0xd3170 / 0xc4b00**
(drawing), **0xce5f0** (lights), and **0xaf110** (ambient emitters).

For the ordinary tree producers, leaf marker **0x4d** and fruit markers
**0x4e–0x50** do not initialize model size **+0x20**. They explicitly write yaw
**+0x24 = 0**, position, tint and flags. Thus the known unwritten-size exception
must **not** be broadened to yaw or other model types. Model append copies both
fields; copying an unwritten field does not make its value initialized.

The stock loader initializes the zone-model table at model-store **+0x38** to
**100 null pointers**, then assigns model references. The entries it leaves null
are exactly **0x1f, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x59**. The other indices
0..99 receive catalog references. Lookup rejects negative/out-of-range ids and
returns the stored pointer otherwise. **Type 0x59 has a special drawing path
before lookup**, so its null table entry does not mean it has no visible effect.

| Model type/path | +0x20 | +0x24 | Verified consumer meaning |
|---|---|---|---|
| Ordinary drawable table entry | Size in blocks per voxel | Yaw in degrees | Both are used for the model's transform. Size also affects bounds/culling and the sway normalization when flag 4 applies. Neither is an optional comparison field. |
| 0x59, special overhang/light shaft | Vertical extent in blocks | Width parameter, **not yaw** | Both are copied into the special draw record and consumed by 0xc4b00. Its lower end is extent blocks below the record position; the upper half-width is width/2 and lower half-width is width×extent/4, oriented toward the camera's yaw. Do not normalize +0x24 as an angle. |
| 0x4c, invisible light marker | Retained plant-record size; can be unwritten on first use | Explicitly 0 in its plant producer | No model is drawn. 0xce5f0 consumes its position/tint and computes its light radius from clock/noise, without reading either size or yaw. |
| 0x4d, invisible leaf marker | Unwritten by the checked leaf producers | Explicitly 0 | No model is drawn. 0xaf110 consumes type, position and tint for falling-leaf particles, without reading size or yaw. |
| 0x4e–0x50, invisible ambient markers | Unwritten by the checked fruit producers | Explicitly 0 | No model is drawn. 0xaf110 uses type/position for bird/owl sounds, without reading size or yaw. |
| 0x1f, invisible water-sound marker | No originating generator established | No originating generator established | Null voxel model; known ambient consumer uses type/position, not these fields. Its absence from checked generation is not grounds to invent a producer. |

For ordinary models, the renderer starts at the record's fixed position, converts
to blocks, scales uniformly by +0x20, applies its flag-driven deformation and yaw,
and centers the model's **voxel x/y dimensions**, leaving its bottom at the record's
z. It does not independently fit x/y/z to a collision size. Model types **0x40 and
0x41** additionally use size/yaw to position their emitted effects. Their fields
must remain checked even when comparing only a simplified static draw path.

The stock-marker size exceptions are therefore **uninitialized/inherited and
behaviorally unused by the traced stock consumers**, unlike lamp turns in §1.
The narrower valid exclusion is **+0x20 for the identified 0x4d–0x50 marker
producers**, plus first-use uncertainty of 0x4c described below. **Keep checking
their initialized yaw = 0**, type, position, tint, flags and list order. A field
being unused by rendering does not justify ignoring its initialized generation
value.

**Plant-record correction:** `worldgen-structures-contents.md` C2 says the reused
size starts at zero. The assembly of 0x3191b0 does **not** initialize its size slot
before the branch-specific writes. The 0x4c branch writes yaw, tint, flags, type
and position but retains that size slot. If a prior plant wrote size, its value
carries forward; otherwise it is stack data. Its light consumer still does not
read it. This is a producer/state qualification, not a size-zero rule.

Do not drop the invisible marker records. They carry ambient/light effects and
also advance 0xd3170's **one-based per-zone record counter**, which determines
later vegetation's **counter modulo 12** sway phase. A null model consumes its
list position even though it does not consume size/yaw in a voxel transform.

0x69b30 copies defined fields including +0x20/+0x24, but leaves padding
**+0x04..+0x07 and +0x3c..+0x3f** untouched. Raw pointer values and these padding
bytes are not geometric placement fields. No blanket size/yaw or turn mask follows
from those padding facts.

### 2.1 Type 0x59 is a generated translucent primitive

**Primitive/asset source:** 0xc4b00 builds **four vertices and six indices** on
the CPU. It loads no voxel model or texture for this effect. Indices are
**0, 1, 2, 2, 1, 3**: upper left, upper right, lower left, lower right. The vertex
format is position float4, normalized RGBA bytes, then two UV floats (28 bytes).
All UVs are zero. The generic color shader is selected with texture use disabled;
the pixel shader at **0x43e250** outputs the interpolated vertex color in that
mode. Sources: **0xc4b00, 0xc4250, 0xc5ec0, 0xc6060, 0xc60d0**.

**Geometry:** the record position is the midpoint of the **upper** edge. Let
height H be +0x20 and width W be +0x24. The horizontal width direction is
**(cos θ, sin θ)**, where θ comes from renderer **+0x358** in degrees, divided
by float 180, multiplied by double π and rounded to float. The upper vertices
are displaced **±W/2** along that direction. The lower midpoint is **H blocks
below** the upper one, with lower vertices displaced **±W×H/4**. It is one
camera-oriented tapered vertical quad, not a cylinder, cone mesh or cuboid.
For H=20 and W=1 its upper width is 1 block, lower width 10 blocks and height
20 blocks. The native renderer converts each directional offset and H to
16.16 fixed units by truncation toward zero, adds/subtracts these offsets from
the fixed record position, then converts the final coordinates to float blocks
(with its normal x/y render-origin translation).

**Camera-angle clarification (0xadcd0, 0xad9d4, 0x295d00, 0xe2e00):** renderer
+0x358 is the **float conversion of the current displayed yaw double at
game+0x1d90**. The frame passes the three current pitch/roll/yaw values from
game+0x1d80 through 0x295d00; 0xe2e00 copies the third float to +0x358. It is
not recomputed from eye and target. With native roll0 and nonvertical viewing,
forward=(sin(yaw)×sin(pitch), −cos(yaw)×sin(pitch), cos(pitch)), while the
beam's horizontal width direction is **(cos(yaw),sin(yaw),0)**. Thus its angle
can equivalently be recovered as **atan2(forward.x,−forward.y)** in that
restricted case. **atan2(−forward.x,forward.y)** gives the opposite direction,
180 degrees away. That swaps the symmetric quad's two sides; it is not the
native stored angle. Use the displayed yaw for exact behavior, especially with
roll, nearly vertical viewing or eye collision adjustments. At yaw0 the beam's
positive-width direction is +worldX; at yaw90 it is +worldY.

**Fixed-corner arithmetic order (0xc4b00):** compute sine/cosine from the float
angle conversion described above. For an upper x/y directional offset, multiply
the relevant cosine/sine by **float0.5**, then by W, then by **float65536**, with
float rounding after each multiplication, and truncate that result to signed64.
For a lower x/y offset the order is relevant sine/cosine **×W ×float0.25 ×H
×float65536**, again with float rounding at every step before signed64
truncation. The vertical drop is **float(H×65536)** truncated to signed64.
Add the fixed x/y render-origin translation to P first; subtract/add those
integer offsets for the two sides. For each final integer coordinate, convert
that combined signed64 value to float, then multiply by **float1/65536**.
Do not independently convert P and a floating offset to block coordinates and
add them there: the native rounding and truncation occur in the order above.

**Colors and fade:** every vertex's RGB is white. The two lower alpha bytes are
zero. The upper alpha byte is the truncated integer **fade×(100×D)**, retained
as a byte. D is the integer read through the executable's Windows CRT
**`__daylight` import at IAT RVA 0x42cd10**, called at **0xc4cb0**; this is not a
read from a game day/night field. The static audit does not establish the
process's runtime timezone/D value. For D=1 and full fade the upper alpha is
**100/255**, decreasing linearly to zero at the lower edge. The model record's
tint, +0x28 and flags do not determine this primitive's color or opacity.

Let L be renderer **+0x49c** in blocks and d the camera-to-record-position
distance. 0xd3170 queues the primitive only when **d² < L²** (and its containing
zone passes that renderer's zone visibility test). Distance is measured from the
upper midpoint, not the quad's center or its expanded lower corners. 0xce300
returns fade 1 until d² exceeds **0.809999942779541×L²**, then returns
**max(0, 1 − (d − 0.8999999761581421×L)/(0.10000000149011612×L))**.
0xcb7b0 / 0xcae70 / 0xc8770 sort the collected descriptors by **descending
distance squared**. Equal-distance ordering is not guaranteed stable.

**Material:** renderer creation **0xc3150** creates the states selected by
0xc4b00. Color uses source-alpha / one-minus-source-alpha with add; alpha uses
one / zero with add. Depth testing is enabled with **less-or-equal**, with depth
writes disabled. Rasterization is solid with culling disabled. Depth clipping
is **enabled** in the selected native rasterizer state: the constructor's
descriptor at0xc324c/0xc3254 sets DepthClipEnable=1. This corrects the earlier
reading of that descriptor. The model tint and flag
bits are not substituted for those fixed states. Thus skipping every null model
entry would omit this actual native render path.

**Generation and comparison:** the terrain producers at **0x30212b–0x3021bf**
and **0x3023e4–0x302488** explicitly set type 0x59, position, height, width=1,
white tint and flags=2. The tree interior helper **0x68790** also creates type
0x59 with explicit height and width **rand mod 3 + 2**. Its width is not always 1.
These initialized fields remain strict generation comparisons even though the
primitive ignores tint/flags. These checked producer blocks do not initialize
+0x28; its later mesh-build initialization is now specified in §9. Its value
is not consumed by this primitive. For a pixel oracle,
capture D, renderer angle, L, camera position and the complete source record.

### 2.2 Comparison policy for null-table markers

Compare initialized source fields independently of whether a voxel model exists.
For known leaf/fruit producers this includes **yaw=0**. For 0x4c this includes
**yaw=0**, position, tint, flags and source/list order; size has the first-use/
inheritance qualification above. For 0x59, **both height and width are meaningful
and initialized**; keep white tint and flags=2 checked for the terrain producers
because those generation values are specified even though the special shader
ignores them. A different documented producer can set different width values.
For 0x1f, **no generation initializer is verified**, so classify any occurrence
by its actual producer before deciding which raw fields can be compared. Do not
declare arbitrary 0x1f data initialized, zero, or ignorable merely from its id.
The later initialization of +0x28 is now resolved in §9; its phase must be
identified before comparing it. This does not permit a blanket drawable-model mask.

### 2.3 Zone+0x90 is an auxiliary light list, not extra geometry

**0xd3170** visits the zone's model vector **start+0x18/end+0x20**, once in
record order, for ordinary voxel models and the special0x59 primitive. The
zone's **+0x90/+0x98** vector is not a second voxel/building-parts array.

At the end of **0x3191b0**, the +0x90 list is rebuilt by copying records from
+0x18 whose type is **0x4c** or whose flags have **bit0 (mask0x01)** set, using
0x69b30. The ambient list **+0xa8/+0xb0** is rebuilt separately from the same
model vector for types0x4d..0x50 or0x1f. These are auxiliary record copies with
the same0x40 stride; identical stride does not give them another geometry role.

**0xce5f0** consumes +0x90 records to collect light effects/point-light data;
it does not draw another copy of their voxel models. Thus a visible lamp model
with flag1 is drawn once from +0x18, and its copied +0x90 entry contributes
lighting. Type0x4c can contribute its light through +0x90 while having no stock
voxel model. Do not emit extra static-model or building-part geometry from the
auxiliary entries, and do not delete their lighting/ambient role. A reader
regression should preserve real +0x18 geometry and its order while proving that
duplicate +0x90 entries add **no second geometry instance**.

## 3. Scope of currently reported evidence

The primary reports 789 snapshots across gen_seed_123 (125), gen2 (122), genb
(271) and genr (271), with the specified lamp-turn differences. These are
**snapshots**, not necessarily 789 distinct coordinates or generation contexts.
Earlier six-zone final-stage counts were objects **3/3**, models **442/442**,
items **13/13**, spawn records **25/25**. Matching those totals is not a substitute
for initialized-field comparisons or dungeon/theme coverage. The primary also
reported that the full-stage command lacked hard object/model-equality failure
checks. The subsequent implementation checkpoint added opt-in
`WC_STRICT_RECORDS=1` checks for object keys, model counts/positions and item
keys/angles. The 789-snapshot run now reports three failing dataset checks for
the lamp-turn record differences. This does not complete model-transform coverage.

The broader dungeon/building, theme-coverage and generation-stage questions are
listed in §7; the focused ceiling audit closes at §8. Intermediate terrain/
structure stages must not be compared as though they were completed generation.
The primary subsequently reports **all 50,116 initialized model records in list
order matching across 789 snapshots** after the §5 metadata corrections. That is
record evidence; it does not independently verify the block oracles in §7.

## 4. Verified static-object scale, centering and door/window pivots

Sources: **0xe0ac0** (object drawing), **0x13a470** (type-specific visual size
factor), **0x13a520 / 0x13a960** (base extents and record-scale multiplication),
and the door/window producers described in `worldgen-structures-buildings.md` §5.
These are **static-object ids**, separate from the model ids of §2.

### 4.1 Generic voxel-model transform

The record's +0x24 is a dimensionless object-scale multiplier. 0x13a960
multiplies **all three** base extents from 0x13a520 by that multiplier. Call
those resulting extents Sx, Sy, Sz; call the loaded model's voxel dimensions
Vx, Vy, Vz. The generic renderer uses one uniform voxel scale:

**max(Sx, Sy) / Vx × visual factor for the object type.**

The denominator is specifically **Vx**, not max(Vx,Vy), Vz, or a collision-box
height. There is no independent x/y/z fit. The visual factor is 1 for doors
and windows 3..7. Other nondefault factors returned by 0x13a470 are:

| Object type | Factor as a stored native float |
|---|---|
| 0x41 | 1.7999999523162842 |
| 0x42 | 2 |
| 0x54..0x57 | 1.100000023841858 |
| 0x58 | 1.25 |
| 0x5a | 3 |
| 0x5b, 0x5d | 1.2999999523162842 |
| all other types | 1 |

The record's fixed position P converts to blocks and denotes the model's
**bottom, horizontally centered position**. Geometry is centered by subtracting
**Vx/2 from voxel x and Vy/2 from voxel y**, without subtracting any voxel z.
Apply uniform scale, the record's Z rotation and translation to P. Do not add
an extra half-block offset to the position already supplied by a producer.
The static turn is **the full unsigned byte×90 degrees**, converted with float
**0.01745329238474369** radians/degree for sine/cosine. Values above 3 remain
valid inputs to this native calculation, as detailed for lamps in §1.

### 4.2 Hinged objects 3..6; fixed large pane 7

The base extents are **3: (2,1,3)**; **4/5: (4,1,4)**; **6: (2,1,2)**;
**7: (2,1,4)**, in blocks before record-scale multiplication. **Only types
3,4,5,6** enter this hinge branch. Type **6 is a hinged small window pane**;
type **7 is not hinged by this branch**.

Let T be the float milliseconds at +0x2c. The low byte at +0x28 selects the
state. For state zero, openness is **min(1,T×0.0010000000474974513)**. For
nonzero state it is **max(0,1−T×0.0010000000474974513)**. The native operations
do not add a lower clamp to the first case or an upper clamp to the second.
The additional opening rotation is **+90×openness degrees** about model-voxel
point **(0,Vy/2,0)**. With generic uniform scale s and the static record rotation,
the world hinge is P plus the static-rotated vector **(−s×Vx/2,0,0)**: the
model's left x edge, at its depth center. Rotate the raw voxel geometry around
that pivot, retaining the outer static rotation. It does not rotate around P
or the voxel model's geometric center.

Building placement initializes state nonzero via the object constructor and
T=1000, so the generated pane/door is closed. The door producer sets record
scale to **opening doorH divided by the type's unscaled base x extent**;
window panes keep constructor scale 1. Wall turns are **west=3, east=1,
south=0, north=2**. The window producer chooses type7 for windowH=4 and type6
otherwise, at z0+1, with the already specified 0.5-block wall inset. These
placement rules and the generic scale must be combined as written; substituting
an independent height-fit changes the native transform.

### 4.3 Sliding and other door-family motion

Types **8,9,10 and 0x10** use the same one-second openness rule as §4.2, but
translate in world Z by **+0.800000011920929×Sz×openness** before the ordinary
centering/rotation/scale. They do not use the hinged pivot, and the multiplier
is **0.8**, not 1. Their Sz includes record scale.

Types **0x12 and 0x13** have a distinct rule: state zero uses the same
T×0.001 opening; nonzero state uses **max(0,1−T×0.009999999776482582)**.
Type0x12 translates by **−Sz×openness−1 block**, including that fixed −1
offset when closed. Type0x13 translates by **+Sz×openness**. Neither gains
the 0.8 multiplier or the hinged-door pivot. Other type-specific articulated
objects are outside this door/window subsection.

For a transform oracle, capture object type, full turn byte, scale, state and T,
the actual loaded model's **three voxel dimensions**, and world positions of
its base corners at openness 0 and 1. A bounding box alone cannot distinguish
the specified pivot and width-based uniform scale from an incorrect box fit.

### 4.4 Object swing-time advancement and reset ordering

This resolves the open clock question in `zone-object-interaction.md`.
**0x2da6c0**, the world simulation step, calls **0x2e3510** at **0x2da73c**
near its beginning. That routine updates items and then iterates every live
static-object record in each selected zone. At **0x2e4b15–0x2e4b1b** it adds the
step's **float milliseconds** directly to the object's **float +0x2c**.

The addition applies to **all object types and both states**, not only a door
whose animation is incomplete. There is **no 1000-ms cap**, zeroing at completion,
integer conversion, or host-only gate on the addition. It can grow beyond 1000;
the drawing rules clamp openness. It changes the live list **zone+0x60**,
not the generation backup at +0x78. The addition itself does not mark an object
change or insert a dirty-zone notice.

**Zone eligibility:** 0x2e3510 first builds a deduplicated set of existing
non-null zones from the world's loaded-zone map **+0x460**. For each creature
in the world's **+0x40** list whose hostility byte **+0x60 equals 0** (player),
take floor(x/64), floor(y/64), in block coordinates, and visit the inclusive
rectangle **zone x−3..x+3, zone y−3..y+3**. Only successful loaded-zone lookups
enter the set; it does not generate missing zones. Overlapping player rectangles
still update a zone once per step. This is not an unconditional traversal of
every cached zone. The selection has no HP/alive test in addition to that
hostility-byte test. With no eligible players, the set is empty.

**Frame versus substep:** local-frame calls at **0xa7328 / 0xa734c** and server
calls at **0xf6917 / 0xf692d** split elapsed milliseconds into **16-ms steps**
while more than 16 remain, then one remainder step. The object clock advances
once per such world step, with its supplied delta, rather than once per rendered
frame or using a second wall-clock sample. The local-frame call block
containing those calls checks that game **+0x206c equals world+0x220** and the realm identifiers
at **game+0x2070/world+0x200** match through 0x6dd20 before stepping. The
object-clock routine does not add a separate menu/pause gate or a remote-client
gate. Use the existing world-step scheduling, not an extra render clock.

**Resets:** the ordinary state-change function **0x2d3a10** first returns if the
new low-byte state equals the existing state. Otherwise it writes the new state
and sets time to **max(0,1000−old time)** using float operations; it does not
unconditionally set zero. This preserves a partially reversed swing. The native
object clock runs in the early world object pass, before that step's later
per-creature interaction handling described in the interaction spec.

There is also a distinct automatic reset immediately **after** the addition:
when a local player exists and object type is **0 or 0x16**, lookup the object's
16-block x/y cell (0x137e0, floor of fixed-position coordinates divided by 16
blocks) in the local player's personal set **player+0x1528 → +0xa8**. Desired
state is 1 when absent, 0 when present. If that differs from the object's low
state byte, write it and set the whole float timer to **0**, without calling
the ordinary reversal rule. The reset is at **0x2e4b8a**. Other automatic
interactions may subsequently call 0x2d3a10; they do not change the unconditional
timer-addition rule above.

## 5. Verified initialized model metadata omitted from older placement prose

These fields are **deterministic producer outputs**. They must be repaired and
compared, not masked because a consumer omits one field. Model tint is three
floats at **+0x2c/+0x30/+0x34**, flags are i32 at **+0x38**.

### 5.1 Crop models

The model initializer **0x2f92c0** sets **yaw=0, tint=(1,1,1), flags=2**. It
does **not** initialize type, position, size or +0x28. The crop branches call it
before filling their actual model records. Sources: **0x305cb7–0x305d71** for
type0x51 and **0x305f94–0x306371** for rows. The switch cases at
**0x305fcb–0x30604d** are present in the native executable even where the
assembly export omits their decoded instructions; their jump table is at
**0x319044**.

All crop models below have **white tint (1,1,1)**. The row branch explicitly
overrides the initializer's flags with its local selection; it does not keep 2.

| Cell crop kind | Model type (hex; decimal) | Final flags | Size |
|---|---|---|---|
| 27 | 0x24; 36 | 0 | 0.1 |
| 28 | 0x26; 38 | 4 | 0.2 |
| 29 | 0x27; 39 | 0 | 0.2 |
| 30, rare rand mod 10 = 0 choice | 0x22; 34 | 4 | 0.05 |
| 30, other choice | 0x2a; 42 | 4 | 0.2 |
| 31 | 0x2b; 43 | 4 | 0.2 |
| 32 | 0x25; 37 | 0 | 0.15 |
| 34, model alternative to the large object | 0x51; 81 | 2 | 0.07 |

0x26 has yaw 0; the others use their already specified random full-angle yaw.
The existing crop placement tests/draw order in `worldgen-structures-buildings.md`
§13 still apply. Initializing tint and flags introduces **no new random draws**.

### 5.2 Outdoor furniture, side models and ceiling models

**Outdoor wall furniture models 0x36..0x38 (decimal 54..56):** the repeated
outdoor branches in 0x2ff6f0 call 0x2f92c0, retaining **white tint**, then
explicitly override flags to **0** before inside-tested append. One complete
example is **0x30c103–0x30c267**; corresponding branches use the same rule on
the other wall orientations. Do not transfer this flags=0 rule to the indoor
side models below. 0x69860 is the room furnishing orchestrator; its direct
side-model helper is **0x6a9a0**.

**Side-model helper 0x6a9a0:** after its wall validity tests it initializes tint
**(1,1,1)** and flags **2** before selecting kind. These remain for kinds
**0,2,3,4,5** (model ids 0x55..0x58, 0x3d, 0x3e, 0x3f, 0x44..0x47).
Only **kind1 / model0x43 (decimal67)** overrides tint to
**(0.6000000238418579, 0.5, 0.20000000298023224)** and flags **1**. In
particular side kind4 / model0x3f (decimal63) is **white, flags2**. These writes
are independent of the selected model's random size/turn.

**Ceiling-model helper 0x68510:** kind0 / model**0x48 (decimal72)** has
**white tint, flags2**. Kind1 / model**0x49 (decimal73)** overrides tint to
**(0.550000011920929, 0.5, 0.4000000059604645)** and flags **1**. The older
building spec's two open tint/light-flag questions are resolved by these native
assignments. Their existing position, size and random-draw rules are unchanged.

### 5.3 Interior overhang/chain markers 0x59 (decimal89)

**0x68790**, the interior leaf/overhang helper, explicitly sets the type0x59
record's tint to **(1,1,1)** and flags **2**, with height equal to its computed
top-minus-floor interval and width **rand mod 3 + 2**. It reuses the leaf
record but replaces its earlier leaf tint. Do not carry the leaf color into the
beam. **0x68b00**, the hanging-lamp/chain helper, also explicitly writes
**white tint and flags2**; its width is the native float
**0.699999988079071** and height is the specified top-minus-floor-minus-one
interval. Both use the special primitive of §2.1, not an external chain mesh.
The already described terrain 0x59 producers likewise write white/2, with
width=1. These producer differences remain meaningful record comparisons even
though the common beam shader uses its fixed white vertex colors.

## 6. Additional verified building-placement corrections

### 6.1 Planner-selected door predicates use R parity, not raw hash parity

The older building spec §5 incorrectly labels **0x31a720** and **0x31a770** as
tests of raw hash parity. They test the low bit of a signed reciprocal quotient.
For the native masked hash's full range **0..0x7fffffff**, that quotient equals
**−floor(hash/21474)**. Therefore its low two bits equal those of the existing
world-generation helper **R = 100000−floor(hash/21474)**. The native signed
multiply by **−0x61a8f94d**, signed upper word, arithmetic shift by 13 and sign
correction produce this result exactly over that range. It is safe to express
the behavior using the independently specified R helper.

* **0x31a720 returns true when R(H(C1)+0x10581) is even**. Roof5's caller at
  **0x306e1b–0x306e3b** calls the planner's door selector only in that case;
  otherwise it keeps door type8.
* **0x31a770 returns true when R(H(C1)+0x134cb8) is even**. Use that predicate
  for the existing special-cell door decision, retaining the cell-type100
  override and surrounding decision order from the building spec.
* The planner selector **0x119800** first requires its item list at
  planner **+0x90/+0x98** to be nonempty and
  **R(H(C1)+0x29fa4) modulo 4 nonzero**. It then uses
  **R(H(C1)+0x89d85), reinterpreted as unsigned32, modulo list length** to
  select the entry; otherwise it returns type8. R can be −3..−1 in rare cases;
  this is not a signed remainder or positive-modulo normalization. There are
  no CRT random draws in these decisions.

The raw masked hash is always odd for this hash polynomial, so substituting its
parity would collapse these predicates to constants and omit actual planner
door choices. Exact predicate examples, with C1 already rounded as required
for a big place:

| C1 | 0x31a720 | 0x31a770 | Selector's R(...+0x29fa4) modulo 4 |
|---|---|---|---|
| (0,0) | true | true | 2 |
| (1,0) | true | true | 0 |
| (0,1) | true | true | 1 |
| (−1,0) | false | false | 3 |
| (−16,−20) | false | false | 2 |

These are function-input cases, not a claim that each coordinate naturally
contains a roof5 building. The primary must determine whether its implementation
used the old raw-hash wording; this author has not inspected implementation code.

An unsigned-index edge case is **C1=(157894,0)**: the roof5 parity predicate is
true, the selector's +0x29fa4 low-two-bit value is **2**, and its +0x89d85 R
value is **−3**. With a five-entry nonempty list, the native index is therefore
**3**, from unsigned32 value4294967293; positive modulo of signed −3 would
incorrectly give2. This is another function-input case, independent of whether
that coordinate actually produces a roof5 building.

### 6.2 Crop-kind34 object turns are initialized

The older crop prose says the alternative objects **0x7f..0x81** leave turn
unset. Native **0x305c2e** explicitly writes their **turn byte = 0** before
position filling and append at 0x305cad. Thus this path is **not** another
undefined-turn exception. Keep its turn checked. This correction is separate
from the street-lamp producer, whose turn remains unwritten and consumed (§1).

### 6.3 Kind8 dungeon guard branch: opposite sense to the roof5 predicate

**0x2fa7b0** inlines the same reciprocal-hash operation as 0x31a720. The branch
at **0x2fa970–0x2fa997** attempts the guard path when
**R(H(P)+0x10581) is odd OR regionItem(P) equals8**. Equivalently, the first
term is **NOT 0x31a720(P)**. Do not use its even/true sense directly for guards.
The region-item call at **0x2fa989** is short-circuited when the odd term already
selects that path. Odd means the low bit is1, including rare negative R values.
This confirms `worldgen-creatures-zone.md` §2.6's R-based wording.

P comes from the original **16-block cell coordinates** passed into 0x2fa7b0:
multiply each coordinate by16 with native32-bit arithmetic, then take its signed
floor division by1024 to obtain the 1024-block place cell P0. Determine the
big-place flag with **0x36160(world,P0)**. If big, independently round both
coordinates down to even: **P=2×floor(P0/2)**; otherwise P=P0. This rounding
precedes the parity hash, region-item lookup, style and place-level queries.
For example cell(−1,0) yields P0=(−1,0), and P=(−2,0) when big; truncating
negative coordinates toward zero is incorrect. The native place-kind query at
**0x2fa891** uses the initial P0 and its big flag before this rounding.

The subsequent cell-type and gate-neighbor queries retain the **original
16-block cell coordinates**, not P. Even after the outer guard predicate is
selected, a current cell of type**45..47** is routed to the fortress path by
**0x25eb30**, called at **0x2fa9a9**. For the other cell types the guard path
searches neighbors in **west, east, south, north** order for type0x2e; no matching
gate returns without spawning rather than falling back to fortress generation.
If the outer predicate is false, use the existing fortress path. This is native
behavior; this subsection makes no claim about the primary's current helper
names or implementation correctness.

## 7. Generation coverage and concrete remaining oracle needs

### 7.1 Stages that must remain distinct

The existing terrain, structures, buildings, contents, tables and creature-zone
specs already describe a complete generator path, including castles/fortresses,
wizard towers, crop fields, town furniture and the kind8 dungeon creature pass.
That does not establish current implementation parity in every branch.

Native **0x2ff6f0** produces a **fresh full-generation zone**. Its specified
order is: place/dungeon context; columns and their overhang markers; the 8×8
halo-cell structure pass; street lamps; spot-map creation; per-cell things;
plants and derived light/emitter lists; region-role things; per-cell creatures
and items with their per-cell reseeding; marked-region spawn flags; object/item
backups and live-item region restamping; derived object-light/treasure lists
**0x31d470**; then object occupancy marking **0x31d700**. Occupancy changes
block flags, so a final block oracle includes those changes. The working spot
map's resets/copies in this sequence are specified in the existing contents spec.

The caller **0x2ae0c0** first reuses an already cached zone if available.
For a new zone it seeds the CRT stream with **zoneY×1024+world+0x244+zoneX**
(32-bit arithmetic), constructs it and calls 0x2ff6f0. When world **+0x358=0**,
it then applies stored-zone state from world **+0x10**, including applicable
saved lists and block changes. **0x31c540** refresh and **0x2e17d0** live-creature
realization are later stages. Simulation also advances the object timers in §4.4.
Therefore label captures as **fresh full generation**, **loaded with stored
state**, **refreshed**, or **simulated**, rather than calling all of them final.
An intermediate terrain/structure record count is not required to equal a fresh
full-generation count, and a fresh-generation object timer is not its timer
after simulation steps.

### 7.2 Findings versus gaps that still require evidence

| Area | Verified coverage/finding | Specific remaining evidence needed |
|---|---|---|
| Object/model placement metadata | §§1,2,4,5,6 give the verified field and transform rules, including deterministic crop/furniture omissions. | Re-run initialized fields **in list order** at fresh full-generation stage; retain raw unknown lamp turns as separate diagnostics. Verify actual asset dimensions and transformed door/window corners, rather than only positions. |
| Missing rendering path | Model0x59's generated primitive is real; §2.1 specifies it. The invisible marker entries have separate effects. | A render oracle with camera, L and D proves beam appearance. This is a renderer obligation even when all generated record fields match. Do not label it missing dungeon geometry. |
| Dungeon/building blocks | Building foundations, wall openings, roofs, dome ceilings and kind8 dungeon floor/guard logic are already specified. The historical castle detail11/12 warning concerns the ceilings of halls in big structures42/44, not room kinds11/12. | Compare complete **block type, RGB/light and flags**, including interiors and cross-zone neighbors, for each ceiling/detail branch. Existing object/model/item/spawn counts cannot prove ceiling or wall parity. Current engine status of the historical ceiling warning is unverified by this author. |
| Kind8 dungeon rewards/guards | The native context scan uses the **last qualifying floor/cell**, with D unwritten if none qualifies; existing creature-zone §1/§2.6 specifies this. | A named kind8 fixture with its chosen F/D, cell types45..47, gate-neighbor branch, region item and full item/spawn fields. Deep-dump guard areas at **spawn+0x900** and activation cells at **+0x928**; raw pointers/counts are insufficient. Treat the no-qualifying-D path explicitly. |
| Chest and lying-item contents | Existing contents rules select chest reward region from the zone place center; later item restamping acts on live items after backups. | Deep item records including category/id, modifier, region, rarity, quantity and object-indexed chest rewards. Distinguish live items from backup+0x48, and item loot from object count. The 789 snapshots' item matches do not enumerate which dungeon/reward branches they covered. |
| Theme/culture coverage | TableT9 / **0x25ea50** maps regional biome and realm style to cultures0..6. The mapping has explicit biome branches and a style-dependent default; style comes from **0x2bb060**. | Annotate fixtures by public seed, actual **world+0x244 zone seed**, realm/style, biome, culture, place/big/variant, cell type, roof and room/detail kinds. Ensure cultures0..6 and all biome branches are represented; multiple datasets or zone totals alone do not prove that matrix. |
| Old unspecific furniture/mason branches | The old open list names wall-piece kinds8..10, center kinds1..3, mason pattern2 and additional cap kinds. It also says the relevant room lists were not filled in the checked stock setup. | Establish a reachable stock producer/list and a fixture exercising it before treating it as a mandatory missing generated case. Do not invent those choices or fill unused lists to increase visual variety. |
| River model0x1f | Null table and ambient consumer are verified; the old audit found no such model in31,940 exported zones and no producer. | A source producer or actual captured occurrence. Its consumer alone does not establish a missing river-generation pass. |
| Model+0x28 | **Resolved in §9:** mesh building fills f32 light samples; fresh generation leaves this field unwritten in the checked producers. Ordinary geometry consumes it as a sunlight multiplier. | Compare mesh-built values using the light-volume and neighboring-column inputs, separately from the fresh-generation oracle. |

The current fixture data establishes many exact deterministic paths; it is not
a coverage manifest for every dungeon, culture, room and generation stage.
The next useful captures are the labeled branch/block/deep-record oracles above,
not further undifferentiated zone-count totals. This spec author has changed
only this prose file and has not run the original game or edited engine/tests.

## Implementation checkpoint before runtime continuation (historical)

The engine now derives generic object scale from its type extents, visual factor
and the actual owner's voxel-X dimension, applying the record multiplier once.
Captured state/time is retained for the specified hinge/slide transforms. The
renderer's static-object draw uses those transforms; live animation-clock/state
synchronization remains unfinished. No native code was inspected by the implementer.

The placement reader now accepts only the live +0x18 model list and +0x60 object
list as scenery. Its older treatment of +0x30/+0x48 ground items as building parts
and +0x90 lights as extra voxel geometry was removed. Full captured turn bytes
are preserved, and finite positive scales are no longer discarded by arbitrary
upper cutoffs. Missing owner geometry remains visible as a data-check warning.

The initialized metadata corrections in §5 introduce no random draws. Rechecking
the four overlapping fixture sets gives **50,116/50,116** model records matching
in list order on type, position, defined size/width and angle, tint and flags.
The verified unused/unwritten marker-size slots are reported independently;
their initialized yaw remains checked. This does not establish every dungeon,
theme or rendered-pixel case. The raw object audit still reports the consumed
uninitialized lamp turns; they were not masked or imitated with guessed bytes.

The implementer checked its hash helper definitions: existing `random()` already
returns R, so §6.1 corrected older specification wording rather than establishing
a collapsed engine predicate. The equivalent opaque reciprocal calculation was
simplified, and coordinate/unsigned-index regressions preserve the specified behavior.

## 8. Closing clarification: historical castle detail11/12 ceiling warning

**The named native behavior is already described** by the existing building
spec's **§3/§4** (columns and dome ceiling), **§6.3** (detail pieces) and mason
spec's **§2–§4** (scheme, writes and shapes). The old main-file §9.5 statement
that the engine's ceilings were not exact is a historical implementation note;
it does not identify an additional missing native ceiling algorithm. Its
reference to building §8 points to the general roof/extras section, while the
actual detail11/12 pieces are in **building §6.3**. This author has not inspected
the current implementation or established a present ceiling mismatch.

### 8.1 Three different selectors must stay separate

* **Castle detail11/12**, assigned by **0x2a8290**, are decoration codes. The
  normal interior decision can select detail11 for big structure42 and detail12
  for big structure44 when its parity test passes, after the earlier flagged
  detail choices specified in building §9. They are not room kind11/12.
* **Castle kind11/12** are different fields; the level-floor liquid handling
  at **0x30b255** gives them lava/poison behavior already specified in building
  §4. Do not substitute those liquids for the detail pieces.
* The **dome flag** belongs to the floor setup: a negative-index castle level
  with castle-cell **style byte1** has wall thickness1 and enables the dome.
  Detail11/12 does not itself enable a different ceiling formula. The general
  level column scheme uses pattern0 for big structure42 and pattern1 otherwise,
  as building §3 states. The detail pieces' pattern0 does not replace that
  preceding column scheme globally.

### 8.2 Confirmed native detail pieces

**Detail12, 0x31273b–0x3127f8:** set the three absolute scheme slots to the
current level light/floor/floor2 colors at **z0−depth0, z0, top0**, all with
**pattern0**. Draw height **rand mod5+8** first, then radius **rand mod4+3**.
Call the mason's hanging cone at local **(8,8,top0−2)** with that radius/height.
**0xc0cd0** writes its noisy circular slices from **z−height through z+1
inclusive**, radius at slice Z equal to **(Z−z+height)/height × radius** in
float. Its x/y bounds, two world-coordinate noise displacements and circle test
are already given in the mason spec. Accepted blocks have **type6, glow0,
flags0x22**, with packed color from that scheme. The shape introduces no extra
CRT random draws. It is a downward hanging solid feature, not a replacement
height map for the whole room ceiling.

**Detail11, 0x312802 onward:** set one constant color **(150,120,50)** with
**pattern0**. Start z at **top0−2**; up to10 times, inspect local (8,8,z−1).
Stop when that block is open with **no flag0x40 or with flag0x80**; otherwise
decrement z. The open test is **0x31abf0** with its three options enabled.
Build the foundation **(7,7,z0), size(2,2,z−z0)**. Draw one axis choice:
nonzero rand mod2 gives box **(0,7,z−2), size(16,2,2)**; zero gives
**(7,0,z−2), size(2,16,2)**. Then draw rand mod4; only on zero draw rand mod2
for along-position **4 or12** and place the already specified type0x17 lamp
at z−3. The piece follows furniture and room-object placement, as building §6.3
states, so its blocks may be the final writes at those coordinates.

### 8.3 Confirmed common dome ceiling and precise verification target

The dome calculation is at **0x30aca8–0x30b003** and matches building §4j:
use the weighted floor/top values from §4d; lower the top by
**(noise(wx×0.1+808348,wy×0.1+8553)+1)×3**; derive edge weight from the
specified side/corner distances divided by8; cube **min(1−edgeWeight,1)**;
blend lowered-top-minus-one with weighted floor in double, then **floor** the
result. The final floor helper is **0x2ff6c0**, called at **0x30affe**.
In that edge rule, an **open side/corner means its neighboring castle-cell
presence is false (kind0/no neighboring floor)**, not that a door opening was
chosen. Side and corner order remains the existing §4j order.

The subsequent common column pass carves air through **ceiling−2** and lays
the cap at **ceiling−1**, using the previously computed color/light rules;
these precede the later detail pieces. A final-stage type6 block near the roof
can consequently come from the common cap, the cone or the detail11 beam.
Inspecting only one surface height cannot distinguish those writes.

If the existing full-block fixtures do not already contain labeled cases,
the exact further oracle is: **one big42/detail11 and one big44/detail12 level
cell**, with their zone and 16-block cell coordinates; world/zone seed, realm
style, floor index; the center and eight neighbors' **castle kind, detail,
style, steps, floor, top and depth**; the selected dome flag; the CRT draws
entering the detail (height/radius, or axis/lamp decisions); and native
**six-byte block values plus column base/runs for the entire16×16 cell**,
covering floor through top+1. Include adjoining zones when a feature crosses a
zone boundary. For a dome mismatch, also record the sample column's lx/ly,
weighted floor/top, neighbor-presence bits, edge weight, cubic weight and
computed integer ceiling. Label whether blocks were captured after columns,
after details, or after **full generation including occupancy marking**.

That evidence can establish a current geometry/color/flag mismatch. Matching
model metadata alone cannot, and the historical warning alone cannot. This is
a **stable written checkpoint** from the previous pass; the then-unfinished
model+0x28 trace is continued and resolved in §9. No game execution was performed.

## 9. Model+0x28 lifecycle resolved: baked light is produced during mesh build

This continues and supersedes the earlier unfinished +0x28 notes. Sources:
**0xe9530**, its light-volume helpers **0xcb9c0 / 0x2a3260 / 0xe0a20**,
the model consumer **0xd3170**, and the frame setup **0xe2e00**.

### 9.1 Field type, production and material consumption

**+0x28 is f32, not an integer or a normalized0..1 opacity.** It stores a light
sample on a **0..255 scale** after mesh building. For example a stored word
0x437f0000 means **float255**, not integer1132396544. The constructor0x2f92c0
and the already audited fresh-generation producers leave it unwritten; append
0x69b30 copies those raw bytes. Fresh-generation comparisons therefore cannot
require a deterministic +0x28 value from those producers. This differs from
the later meshed/render-ready state.

**0xe9530(renderer,zoneX,zoneY)** looks up the zone in the world's +0x460 map,
caches pointers to the neighboring zones through **0xcbfc0**, constructs a temporary
light volume with **8-column horizontal padding**, and computes that volume
through **0x2a3260**. Then it visits the zone's model records **+0x18/+0x20**
in list order. For each record it floors **all three fixed coordinates to
integer blocks**, samples the completed light volume at that exact block
through **0xe0a20**, converts the returned unsigned byte to float and writes
**record+0x28** at **0xe97c5–0xe97d4**. It does not sample a model-height
midpoint or add the creature renderer's0.1-block z offset.

The same routine then repeats this operation over the auxiliary light-record
copies **+0x90/+0x98**, writing their own +0x28 at **0xe98e5–0xe98f4**.
It does not rely on aliases between those lists. Immediately after each light
write it obtains the block at the record's fixed position via **0xda750**.
When that block's **type byte equals2 (water)** it **ORs flag0x08** into the
record's flags+0x38. It does not clear that bit in the non-water case. This
applies separately to both lists, and explains a runtime flag difference
between fresh-generation and mesh-built captures.
This is the effective block getter through **0xda660 → 0xeb90**, including
the ordinary implicit sea-water conversion of eligible air at z≤0, rather
than a direct read of a raw stored block's type. The light-volume sample and
this effective-water test are separate operations.

**Sampling boundary behavior:** 0xe0a20 returns byte255 outside the temporary
volume's horizontal bounds. Within a column, below its stored base returns0;
inside its stored byte range returns that byte; at/above its stored end returns
255. Positions use floor, including negatives. These are light-volume lookup
rules, not reads of a terrain block's emitted-light/glow byte. The precise
volume-construction algorithm is a separate step; do not replace the lookup
with block glow or a global sky color.

**Consumer:** at **0xd3d9b**, an ordinary drawable model with **flag0x01 set**
gets material alpha **1**, bypassing this field. Otherwise it receives
**renderer+0x53c × (float(record+0x28)/255)**. No integer reinterpretation,
normalization clamp or new world-block lookup occurs in this consumer. The
frame initializes renderer material RGBA to **(1,1,1,1)** in0xe2e00; its
underwater-color branch also keeps alpha1. Thus the ordinary initial material
factor is the baked sample/255. Distance fade is supplied separately.

This material-alpha channel scales the Steam shader's **sun/back light term**,
as described in `alpha-rendering.md` §14; it does not eliminate the unscaled
ambient term. A light sample0 is not a request to remove the model or set its
geometric opacity to0. The special0x59 beam and null-marker consumers retain
their separate rules in §2.

**Oracle consequence:** record +0x28 and runtime water flag0x08 should be checked
after this mesh-build phase, with the neighboring column/light-volume inputs
identified. Preserve the distinction from the fresh full-generation record
oracle. Existing owner captures confirm different states: gen_seed_123 raw
models contain unwritten float values, while sampled seed_123 loaded records
include actual float255 and float21 light values. Those observations corroborate
the traced writer; they are not a substitute for its phase-specific rules.

### 9.2 Steam light-volume solver, confirmed against the existing terrain spec

The Steam solver is **0x2a3260**. `worldgen-terrain-zone.md` §6 already specifies
it, including its erratum about retaining glow beneath a glowing open block.
The following clarifies its implementation inputs and buffer/update ordering.
No equivalence to an unspecified Alpha solver is assumed.

For zone lower corner (X0,Y0), construct light columns over the half-open square
**[X0−8,X0+72) × [Y0−8,Y0+72)** from the **3×3 existing-zone neighborhood**
captured by0xcbfc0. Each available terrain column contributes its actual base
and number of stored six-byte blocks; there is one light byte per such block.
0xcb9c0 /0xe8a40 initialize these byte arrays to0. Missing columns have no stored
light bytes. Light lookup is0 below a stored base and255 at/above the end or
outside the horizontal map. The six-byte terrain block order is **R,G,B,glow,
type,flags**; the glow input is byte3, not the air-light output described below.

**Vertical initialization**, x outer/y inner, each available column top down:
incoming light starts255, with direct-sky status true. Type**20 (0x14)** resets direct
sky true and running light to that block's glow, storing that value. Types**0
and2** are open: with direct sky, set running light to255 and store255;
otherwise set running light to the
maximum of itself and glow, then store that running value. For other types,
store the incoming running value first, set direct sky false, then lower the
running light to min(previous running light, that solid block's glow). Store
these initialized values in **both** the primary and work maps. This vertical
pass does not impose a distance attenuation between successive open blocks.

**Propagation:** padding−3 gives **five rounds**. Each round visits the same
horizontal domain with the outermost single column removed:
**[X0−7,X0+71) × [Y0−7,Y0+71)**. Its bounds do not shrink again each round.
Only stored **open type0/2** blocks with current primary light below255 are
updated. Read six neighbors from the primary map, in west/east/south/north/
down/up order; clamp each neighbor value to at least**25**, choose the maximum,
then write **floor(maximum×85/100)** to the work map. Existing255 values remain
255. In particular six zero/dark neighbors yield **21**, not0, and a neighbor255
can propagate **216**. The current cell is not a seventh neighbor, and this
round does not additionally maximize against its own glow or prior value.
There is no averaging, exponential convergence test,
unbounded flood queue, or additional RNG.

After computing the entire round, copy work-map values back to the primary map
for **open** blocks over the full padded square. Solid/type20 initialized values
remain fixed inputs; work cells that were skipped retain their prior values.
Consequently this is a two-buffer round, not an in-place sweep whose outcome
depends on the column iteration direction.

After the fifth round, the target zone's own open stored blocks get their
computed light as **red byte0 only**. **0xa36e0** writes back the six-byte block
with its other five bytes preserved. Thus this post-generation phase changes
air/water stored RGB data without recoloring a solid surface block. For example
a stored water block can become **(light,127,127,0,2,flags)**. The temporary
map also retains the light entries at solid coordinates, which can be sampled
by models; it is not equivalent to sampling an arbitrary rendered vertex's AO.
0xe9530 reads model light from this completed map as specified in §9.1, before
constructing its zone mesh faces.

### 9.3 Model tint versus voxel material; exact Steam sunlight composition

**Model-record RGB at +0x2c/+0x30/+0x34 is not multiplied into the ordinary
model's voxel palette color.** The ordinary model branch of0xd3170 submits
material RGB from **renderer+0x530/+0x534/+0x538**, and alpha by §9.1. It does
not read those three record tint fields for this geometry material. The record
tint is consumed by the auxiliary light effects in0xce5f0 and by relevant
emitter effects such as leaf particles in0xaf110. Its light can affect nearby
geometry indirectly through point-light shading. This distinction does not
excuse any initialized generation tint mismatch. The0x59 primitive retains
its fixed white vertices.

There is a second +0x28 consumer in the auxiliary-list light collector:
**0xceaa1–0xcecc5** within0xce5f0. For visible flag0x01 models of types
**0x17/0x18/0x19**, form U by multiplying float+0x28 by renderer+0x52c,
then dividing by255. The light is admitted only when **U ≤0.4000000059604645**.
Its record RGB is multiplied by **1−U** before registration. Other types in
that ordinary flag1 branch do not get this particular gate or attenuation.
The separate0x4c light branch also uses record RGB. Renderer+0x52c is a
frame-dependent input; the baked model sample itself is not recomputed as
time of day changes. This is further evidence that record tint and baked
light are light-effect data, rather than a tint applied to voxel geometry.

The compiled Steam world vertex shader is at **RVA0x4387b0**, world pixel shader
at **0x43a010**. These shader constants come from the normal draw material;
they are separate from the special untextured beam shader. For the ordinary
non-flag1 model let A be its submitted material alpha, V the normalized vertex
color-alpha/light input, N the normalized world-transformed face normal, D the
submitted sunlight direction, and F/B/C the submitted front/back/ambient RGB.
The shader uses a fixed normalized axis
**(0.597022294998169,0.3980148732662201,0.696526050567627)**.

The directional weight is **max(0.7×dot(N,D)+0.3,0)**. The fixed-axis weight is
**0.5+0.5×abs(dot(N,axis))**. Front RGB times its directional weight plus back
RGB times the fixed-axis weight is **componentwise clamped0..1 first**. That
clamped front/back sum is then multiplied by **V×A**. Add ambient RGB times
the fixed-axis weight, and the point-light contribution, then clamp the resulting
RGB0..1. Ambient and point lights are **not** multiplied by A or V. Thus the
order of the first clamp matters; do not scale the front/back terms before
their clamp or darken the ambient term by the model's baked light.

The shader considers up to**10 point lights**. For a positive-radius light,
let q be vertex-minus-light position and r its radius. Radial weight is
**(1−min(dot(q,q)/(r×r),1))²**. Its normal weight is
**clamp(−4×dot(N,normalize(q)),0.3,1)**. Multiply these weights by that light's
RGB and sum before the final lighting clamp. The light-list collector supplies
positions/radii/colors; this formula does not itself choose or register lights.

For the ordinary non-glow color path, double the normalized stored vertex RGB
to get palette color P. Let L be the clamped lighting RGB. Saturation gain is
**clamp(1+0.5×(ambient.w−L.green),1,2)**. Using luminance weights
**(0.299,0.587,0.114)**, apply that gain about P's luminance and clamp the
boosted palette to0..2. Multiply it by L and the submitted material RGB.
The separate glow/white effects, fog and final color grading remain described
in `alpha-rendering.md` §14. Pixel output alpha is a **separate pixel constant**;
the model's material A is not substituted for that geometric-opacity value.

### 9.4 Mesh scheduling and a useful light oracle

The normal scheduling path in **0x842d0** considers a zone for a new mesh when
its triangle-count field **+0x444 is0**, or its dirty byte **+0x23c is nonzero**.
It also requires **0xce3e0** to succeed: that helper locks the world and checks
that all nine zone-map entries in the centered3×3 square exist and have
non-null zone pointers. A mesh batch accepts at most**8 zones**, with additional
budget/distance selection before scheduling. These are mesh scheduling rules,
not additional generation stages. The task wrapper **0x7d680** reaches0xe9530;
0xe9530 clears the target zone's dirty byte at **0xecb1b** after constructing
its mesh. The neighboring-zone cache is a pointer cache, not a deep copy of
the nine zones' columns.

Consequently the light sample is a result of a mesh job, rather than a value
updated on every model draw or every simulation tick. To verify it without
mixing phases, label a capture **before mesh/light solve** or **after mesh/light
solve**, and preserve the target zone coordinates, all nine available zones,
each padded column's signed base and ordered six-byte block records, the
ordinary and auxiliary model positions, and the resulting float+0x28 and
water flag. A finer oracle can capture the completed temporary light volume
at the return of0x2a3260 to **0xe96f7**, before model sampling and face building.
Its primary and work light bytes distinguish vertical initialization from the
five propagation rounds. A raw fresh-generation record cannot establish this
post-mesh output, nor does a changed float+0x28 prove a generation mismatch.

### 9.5 Flag 0x08 model queue and exact position relative to water

**Flag 0x08 selects renderer descriptor queue+0x1d0**, while ordinary models
without that bit use queue**+0x138**. These are per-frame draw descriptor
queues, separate from the zone's model and auxiliary-light record lists.
0xd3170 makes this selection immediately before **0xee2f0** appends the
model descriptor. It retains the same world transform, material, lighting,
glow bit and distance fade. This flag does not itself select the water pixel
shader or replace the model material by a blue tint.

0xee2f0 omits nonpositive fades. A fade≥1 goes into the queue's opaque,
model-grouped descriptors. A positive fade<1 goes into its first faded vector,
at queue+0x20/+0x28. For the audited ordinary model append it is this first
vector that0xe7980 consumes; the general queue routine also supports a second
vector with reversed winding.

The relevant order in **0xe2e00** is:

1. **0xe5070:** solid terrain meshes.
2. **0xe6e60(queue+0x138,2):** ordinary queue's opaque models.
3. **0xe6e60(queue+0x1d0,2):** flag0x08 queue's opaque models.
4. **0xe7980(queue+0x1d0,2):** flag0x08 queue's faded models.
5. **0xe6270**, conditional0xe6840 and light-constant setup.
6. **0xe85e0:** water depth pre-pass and water color pass.
7. A projected-effect pass at **0xc43d0**, then
   **0xe7980(queue+0x138,2):** ordinary queue's faded models.
8. Later clouds/stars and the0x59 beam pass **0xc4b00**.

Thus **both opaque model queues precede water**, and the flag0x08 queue's
faded models also precede water. Only the ordinary queue's faded models are
deferred until after water. A model's flag0x08 must not be interpreted as an
instruction to draw that model after the water surface.

**State:**0xe6e60 selects state0;0xe7980 first selects state3 for the first
faded vector's depth-only draw, then state6 for that vector's color draw.
The general second vector uses corresponding states4 and7. All of these use
the **ordinary world shaders**, bound through **0xc5de0**; the state selection
is **0xc3dc0**, and state creation is0xc3150. They use depth testing enabled,
**less-or-equal**, depth writes enabled, stencil disabled and solid fill.
State0 disables alpha blending and writes RGBA. State3 writes **no color**.
State6 writes RGBA with source-alpha / inverse-source-alpha blending, add;
its alpha channel uses one/zero, add. **Depth writes remain enabled in the
faded color pass.** These are not a conventional no-depth-write transparency
pass. States4/7 reverse the front-winding setting relative to3/6.

The native ordinary raster state uses **CullMode=FRONT** and
FrontCounterClockwise=false; the alternate winding state changes the latter
to true. Depth clipping is enabled. State descriptors are created from
**0x438710/0x4386f0** and **0x438740**. The water shader and its two-pass
states remain as specified in `water-rendering.md` §B.2. No flag0x08-specific
water shader or distinct depth-comparison rule was found.

### 9.6 Ordinary model fade uses its raised center, without a64-block extension

For the ordinary asset-backed zone models, **0xd3a19–0xd3a90** and
**0xd3e0f–0xd3ecb** establish two facts that distinguish them from the zone
gate and the0x59 primitive:

- Their fade radius is **L=renderer+0x49c**, with **no +64**.
- The sampled position is **record position with its Z raised by half the
  model's scaled voxel-Z extent**, not its unshifted foot position.

The fixed-point Z increment is computed in float32 order: convert voxel-Z
count to float, multiply by0.5, by the record's size+0x20, then by65536;
truncate that result to signed64, then add it to the record's fixed Z. Fixed
X/Y remain the record's X/Y. Both the ordinary and flag0x08 queues use this
same raised position and L when calling **0xce300**.

0xce300 is the same fade function as for the beam. Convert each fixed
camera-to-sampled-position difference to float and multiply by float1/65536;
sum the three squared components in float32. Its full-fade threshold is
**float32(float32(L×0.809999942779541)×L)**. At/below that threshold return1;
above it, return **max(0,1−(sqrt(d²)−L×0.8999999761581421) /
(L×0.10000000149011612))**, with float32 operations.0xee2f0 omits a descriptor
whose resulting fade is not positive.

Before this fade, **0xe08c0** tests that raised center against the render
distance L and all six frustum planes, using radius
**max(voxel-X count,voxel-Z count)×record size** for the plane tests. Its
distance comparison itself uses the fixed-point squared-distance sum and a
float32 L²×65536 threshold, rejecting only when the distance is greater.
Thus there is no separate ordinary-model test of strict d²<L² at the raw
record foot. The later nonpositive-fade omission supplies the fade cutoff.
The beam retains its separate strict distance test at its raw record
position, and does not get this asset-height/radius test.

The **+64** mentioned in older vegetation prose belongs to the **containing
zone's broad distance gates**, described next. It must not be added to the
ordinary model's0xce300 input. This corrects that ambiguous distance wording.

### 9.7 Containing-zone visibility, including zones containing beams

**Yes: normal zone-model rendering requires the containing zone's visibility
gate.** In the normal-world branch of0xd3170 (frame flag0x80 clear), a zone
must exist, have nonzero **zone+0x1d0** meshed-group count, and pass **0xe0680**
at **0xd37d5** before its ordinary model records are considered. This zone
field is unrelated to the identically numbered renderer descriptor-queue
offset in §9.5. The0x59 branch is inside this zone gate too.

0xe0680 receives the cached fixed-point box **minimum at zone+0x1e8**, maximum
at **zone+0x200**, and distance radius **renderer+0x4a4+64 blocks**. First it
compares horizontal camera distance from the box's XY midpoint against that
radius. Its comparison retains equality and uses fixed-point squared
differences and the float32 radius²×65536 threshold. It then tests the box
against **five** of the six renderer planes at+0x43c, stride16: indices
**0,1,2,4,5**. **Index3 is deliberately omitted.**

For each tested plane choose, independently per axis, the maximum bound when
that plane's coefficient is positive and the minimum otherwise. Reject only
when this selected corner's plane expression is strictly negative. Apply
renderer fixed XY origin translation+0x428/+0x430 before conversion to the
render-coordinate floats; Z has no corresponding origin translation here.
Touching a plane is retained. Plane normalization is unnecessary for this box
test. **0xdf8f0** constructs the planes from the combined projection/view
matrix:0/1 are clip-w±clip-x,2/3 are clip-w±clip-y,4/5 are clip-w±clip-z.
Therefore the omitted index3 is the **upper screen edge**, not the far plane.

After that gate,0xd3170 also requires the cached zone center
**+0x218/+0x220/+0x228** to be strictly closer than
**renderer+0x49c+64** in three dimensions before iterating model records.
The individual asset-backed model test/fade is then §9.6. A beam instead
gets its raw-position strict d²<L² test and fade from §2.1; it does not get
the model-asset sphere test. There is no individual expanded beam-quad
frustum test in this branch.

The cached box is written by mesh builder0xe9530 at **0xec804–0xec82b**;
its center and radius are established just before it. Its horizontal bounds
are **[64×zoneX,64×zoneX+65]** and the corresponding Y interval, in blocks.
Vertical extents come from that meshing job's column/face prepass, not from
the ordinary model heights or the beam's width/height. The native box is not
enlarged to contain the beam's lower corners. Exact native visibility therefore
requires the cached mesh-stage box and center, not a raw-record point test.

**Conservative fallback, explicitly an inference rather than another native
algorithm:** where a renderer cannot yet reproduce the cached vertical box,
keep the native XY interval and use a vertical interval enclosing all columns
of the loaded3×3 zone neighborhood, using the same mesh-stage column inputs.
A sufficient interval is from
**min(0,all available stored bases−1)** to
**max(2,all available stored exclusive ends+1)**, in blocks. The meshing prepass
uses the target columns and their immediate neighbors, so this interval
contains its vertical bounds, including its z=0/1 treatment. Use the five
plane indices above and preserve the original XY midpoint for the horizontal
gate. Enlarging the box cannot introduce a new positive-corner plane rejection.
If the required neighboring column bounds are unknown, retaining that zone
for the frustum test is conservative; guessing a finite Z range is not proven.

For a beam-geometry-conservative box, additionally union in the four current
beam vertices from §2.1. That also cannot reject a zone the original box
accepted. **Do not recenter an expanded box and apply the native3D center
distance gate to the new center:** that can introduce a false rejection.
Retain the original cached center when available; otherwise omit that broad
center rejection and retain the beam's individual distance/fade checks.
These fallbacks may draw a beam that native zone culling would omit, so they
must not be represented as exact native culling parity. The exact oracle is
the mesh-stage min/max/center, renderer radii and six planes, plus camera and
render origin, with the zone gate and beam gate outcomes labeled separately.

### 9.8 Zone flag0x02 is not the ordinary model's glow input

**Zone-model flags bit0x02 does not set shader glow or white in the audited
ordinary voxel-model path.** In0xd3170, the submitted glow scalar is exactly
**float(flags&0x01)**:0 or1. The other effect scalars submitted alongside it,
including white and depth offset, are0. This applies to both descriptor
queues in §9.5. The submission is at **0xd3e33–0xd3f09**;0xee2f0 preserves
these scalars in both the opaque grouped descriptors and faded descriptors.
The draw consumers0xe6e60/0xe7980 restore them for each model draw. Thus a
record with flags2 alone has glow0 and white0, while flags1 supplies glow1.

This agrees with the checked producers in §5: constructor0x2f92c0 supplies
default flags2, and many crops/ordinary decorations keep that value. The
audited warm light furnishings explicitly use flags1. Flag0x02 remains
initialized record metadata and must still be compared; absence of a direct
glow mapping here is not a claim that the bit has no other consumer anywhere.
An engine's separate per-vertex flag whose numeric value is2 is a different
format; its number must not determine this zone-record mapping.

**Exact shader effect:** the world vertex shader at **0x4387b0** has separate
scalar **Glow at vertex constant buffer9** and **White at buffer7**. The glow
setter is **0xc59f0 → 0xc6c50** and the white setter **0xc5aa0 → 0xc7a20**;
0xc68c0 binds those buffers. This is a per-draw effect, not an additional
zone-record RGB tint or an emitted-light value written into nearby blocks.

Let P be doubled normalized voxel RGB, M the submitted material RGB, and C
the normally lit, saturation-adjusted color from §9.3 before the final white
operation. For positive glow g, the shader replaces C by the interpolation
**C+g×(P×M−C)**. Therefore the zone flag1 value **g=1** yields exactly
**P×M** before final clamping: it bypasses directional, ambient, point-light,
vertex-light/AO and shade-saturation contributions to this geometry color.
This is distinct from the same flag's separate auxiliary point-light effect.
For glow0 the normal lighting path remains. The shader also supports negative
glow values, replacing the saturation-adjusted palette by P's luminance
before multiplying by normal lighting and M; ordinary zone-model flags do
not select that negative branch.

After choosing the glow result, the white scalar w applies
**componentwise clamp(C+w×(1−C),0,1)**. White1 gives white; white0 still
performs the final0..1 clamp. For all ordinary zone-model submissions traced
above, white is0. Glow does not bypass the later pixel-shader fog/color
grading or replace the separate opacity/fade constant. In particular flags2
alone must not make a crop or furnishing emissive, white or independent of
its baked/world light.

## 10. Additional castle-floor and room arithmetic clarification

The re-audit of **0x2a8290 / 0x2aa450 / 0x2ab0a0** confirms the existing
building spec's room topology, floor-neighbor ordering and detail11/12 rules.
No additional missing ceiling shape or generator pass was established by
this trace. One precision gap in that prose is the type of several R
remainders. R can represent signed−1/−2/−3; the following native sites take
its **unsigned32-bit representation** before remainder:

- **0x2a8290:** all four castle room-margin draws, salts **0x4e6289,0x21d4a,
  0x11a43f,0x14f747**, use unsigned remainder3.
- **0x2aa450:** the off-corridor room activation draw at **0x71504** uses
  unsigned remainder5; candidate selection at **0x55260** uses unsigned
  remainder by the number of available candidates; type selection at
  **0x57b071** uses unsigned remainder3. The candidate list remains the
  existing ordered nonempty neighbors **west,east,south,north**.
- **0x2ab0a0:** the floor increment for room **flags&0x04** uses unsigned
  remainder6 at salt **0x7b7e8**. Its hash coordinates are the cell coordinates
  divided by3 with **signed division toward zero**, independently on each
  axis. This coordinate division is different from the unsigned R remainder.

These statements are site-specific; they do not change signed uses of R
elsewhere. Useful arithmetic checks, using the terrain spec's H/R definition:

| Input to the named draw | Signed R | Native unsigned remainder/result |
| --- | --- | --- |
| Room(26966,0), margin salt0x4e6289 | −1 |0 modulo3 |
| Room(26541,0), activation salt0x71504 | −1 |0 modulo5, so this activation test passes |
| Room(70386,0), candidate salt0x55260, three candidates | −1 |0 modulo3 |
| Cell(63861,0), floor salt0x7b7e8 after division by3 | −2 |2 modulo6 |

These are arithmetic test inputs, not claims that a particular world seed
naturally places those cells in a qualifying castle room. Earlier corridor,
place and candidate conditions still apply. The implementer's existing
unsigned R helper may already satisfy them; the trace does not establish an
implementation defect.

For **0x2ab0a0** boundary smoothing, choose only the first matching room edge
in **west,south,east,north** order. Raise the selected neighbor's height to at
least the current floor height, add that height to the current one, then use
**signed integer division toward zero by2**. A corner does not combine both
adjacent neighbor heights. For example current−41 and chosen neighbor−40
produce−40, whereas current40 and chosen neighbor41 produce40. Apply the
qualifying room's unsigned modulo6 increment afterward. This confirms and
makes explicit the existing floor formula; it is not the common dome's
floating-point blend followed by floor in §8.3.

The remaining dungeon verification need is still **labeled branch coverage**:
native place kind/big flag, room tuple including flags and side, selected
neighbor and both pre-average floor heights, floor after smoothing and after
the optional increment, style/detail/top/depth and the ordered final six-byte
block runs. In particular a detail11/12 ceiling claim needs the §8.3 inputs
and a final-stage block difference; counts or model agreement alone cannot
decide it. This pass has not run the original game or inspected the engine.


## 11. Runtime implementation checkpoint (separate implementer)

The implementer used only this independent prose, existing engine sources and
owner exports. No native/decompiled code was inspected. The runtime now advances
all live static-object timers in the documented selected zones/substeps, before
interactions, with local personal-state resets and no timer-only dirty notices.
Render poses read live state/time after simulation; placement identity checks
prevent a shifted snapshot list from animating an unrelated object. Captured
placement scale and turn remain intact, including consumed undefined lamp turns.

The generated 0x59 path now uses a CPU-defined quad and dedicated untextured shader,
displayed camera yaw, fixed-point corners, process CRT daylight alpha, distance
fade and descending-distance order. The corrected depth-clipping fact in §2.1
supersedes the earlier draft's disabled-clipping statement.

Ordinary model light is sampled from the existing independently implemented Steam
solver over current neighboring terrain, including solid light-map entries. The
mesh path requires all nine neighbor inputs for a new bake. Incomplete exports
retain available captured light/default behavior, not a full native mesh claim.
Water flags are ORed and retained for the loaded zone's lifetime across terrain
snapshot copies. Record RGB is not applied as a geometry tint. Native flag1 maps
to glow, not flag 2. Raised-center model fade and the dry/water opaque/faded queues
use the specified depth/color phases; baked sunlight is separate from opacity.

The site-specific unsigned castle remainders in §10 are implemented; the four
margin choices and signed floor averaging already had the specified semantics.
Broader checks now compare complete auxiliary light lists, including side torches
previously excluded by an outdated test. Partial refreshed-only RNG captures are
labeled as inapplicable to the fresh A/B prefix test; block/model/list checks still
run. The raw object parity gate remains open for the consumed undefined lamp turns.
Exact native cached-zone culling, complete auxiliary light registration and shader
color details, and the broader labeled dungeon/theme oracles remain unfinished.
