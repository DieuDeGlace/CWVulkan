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
is disabled in the selected native rasterizer state. The model tint and flag
bits are not substituted for those fixed states. Thus skipping every null model
entry would omit this actual native render path.

**Generation and comparison:** the terrain producers at **0x30212b–0x3021bf**
and **0x3023e4–0x302488** explicitly set type 0x59, position, height, width=1,
white tint and flags=2. The tree interior helper **0x68790** also creates type
0x59 with explicit height and width **rand mod 3 + 2**. Its width is not always 1.
These initialized fields remain strict generation comparisons even though the
primitive ignores tint/flags. These checked producer blocks do not initialize
+0x28; its later initialization for a complete loaded-world record is not
established here. Its value is not consumed by this primitive. For a pixel oracle,
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
The unverified later initialization of +0x28 is an explicit remaining question,
not permission to apply a blanket mask to drawable models.

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
| Model+0x28 | Initializer0x2f92c0 leaves it unwritten; ordinary voxel rendering reads it for non-flag1 material alpha, while the null/special marker consumers above do not. | Determine whether a later loaded-world/mesh-light update establishes it, or whether a producer leaves it consumed as stack data. Do not conflate the proven unused leaf-size exception with this **unfinished** lighting-field audit. |

The current fixture data establishes many exact deterministic paths; it is not
a coverage manifest for every dungeon, culture, room and generation stage.
The next useful captures are the labeled branch/block/deep-record oracles above,
not further undifferentiated zone-count totals. This spec author has changed
only this prose file and has not run the original game or edited engine/tests.

## 9. Implementation checkpoint (reported by the separate implementer)

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
a **stable written checkpoint**; the model+0x28 lighting-field trace remains
explicitly unfinished in §7.2, and no further game execution was performed.
