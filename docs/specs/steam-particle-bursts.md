# Steam World-burst particles: styles1,2,5

Independent native-only specification. This covers **ordinary burst
expansion, live update and draw** for styles1/2/5. Other style families
are not made complete by these shared facts. No engine/test/extractor, SDK,
reconstruction, owner-export/capture data or game execution was used.
All addresses are original Steam RVAs; fixed positions use65536 units/block.

## 1. Frame phase, queues and burst layout

The ordinary Game frame **0xa5a50** expands the current **World+0xb0**
linked list in forward record order, at **0xaa1f0–0xaa51d**. Received bursts
have already been appended to this World list, as specified in netcode-roles§8.
No per-burst renderer range/frustum test, time-derived random seed, source
creature lookup or new packet boundary filters this expansion.

Copy each burst through **0x7fb50 at0xaa1fb**. If its signed32 count is<=0,
emit nothing and consume no expansion draws for that burst. Otherwise emit
exactly that many live particles before moving to the next burst. Expansion
does not stop at the later1000-particle live-list limit and does not remove
the outgoing burst record. Outgoing-group lifetime is a separate frame step.

| Burst offset | Native field |
|---|---|
| +0x00/+0x08/+0x10 | signed64 fixed XYZ |
| +0x18/+0x1c/+0x20 | float32 base velocity XYZ, blocks/second |
| +0x24/+0x28/+0x2c/+0x30 | float32 RGBA |
| +0x34 | float32 size |
| +0x38 | signed32 particle count |
| +0x3c | signed32 style |
| +0x40 | float32 velocity spread |
| +0x44..+0x47 | padding, not assigned/copied by these field-copy helpers |

The Game live list is at **Game+0x2090**, with64-bit count at+0x2098.
Its node has a16-byte link header followed by a **0xb8-byte particle**.
Append new particles at the tail through **0xa21a0 at0xaa505**; existing
particles are retained. The copy chain **0x6e710→0x7f9e0** preserves the
fields below; it supplies no randomization or missing-field initialization.

## 2. Live record produced by expansion

Offsets below are from the live payload, excluding the16-byte node header.
The constructor is **0x7fad0 at0xaa247**, followed by explicit expansion
writes and a final identity-matrix assignment through0x17800 at0xaa4f2.

| Live offset | Final expansion state for the bounded styles |
|---|---|
| +0x00 | signed32 burst style |
| +0x04 | signed32 age ms:0 for1/2; positive rand remainder500 for5 |
| +0x08/+0x10/+0x18 | fixed position from§3 |
| +0x20..+0x37 | not initialized by this constructor/expansion path |
| +0x38/+0x3c/+0x40 | float32 initial X/Y/Z angles, degrees |
| +0x44/+0x48/+0x4c/+0x50 | float32 RGBA; style-specific rule in§3 |
| +0x54/+0x58/+0x5c | float32 velocity XYZ, blocks/second |
| +0x60/+0x64/+0x68 | float32 angular velocity XYZ, degrees/second |
| +0x6c..+0xab | float32 identity4×4 matrix |
| +0xac | copied burst size float32 |
| +0xb0 | collision byte:1 for5,0 for1/2 |
| +0xb1 | freeze byte0 |
| +0xb2..+0xb3 | uninitialized padding, not copied as fields |
| +0xb4 | signed32 lifetime4000 ms |

Do not validate the uninitialized ranges as deterministic initialized data.
Burst alpha is copied as a real fourth component, not omitted. Its known
first-burst producer/visible-material distinction remains in
[steam-world-death-area.md§2.1](steam-world-death-area.md#21-native-consumer-stored-alpha-versus-submitted-color).
No new meaning for the uninitialized+0x20..+0x37 range or other styles is
inferred from this ordinary producer.

## 3. Per-particle CRT order and float staging

Use the executing thread's current CRT stream; this expander performs no
direct srand or save/restore of that state. Each rand result is converted
to float32 before the operations below. All displayed scalar operations
are separately rounded float32; there is no double normalization or FMA.
The denominator is **32767.0f**, word0x46fffe00 at0x42e5f0.

Draws are numbered within one emitted particle. Complete all draws and
append that particle before beginning the next one.

1. **Draws1,2,3: position Z,Y,X.** Z uses rand÷32767 without centering.
   X/Y each use rand÷32767 minus0.5. Convert each resulting float to fixed
   by multiplying by float65536 and truncating toward zero to signed64
   through **0x6d240 at0xaa2b6**. Add each to its burst fixed coordinate
   with low64 addition, through0x83c30 at0xaa2d0. Thus XY jitter is bounded
   by−0.5..+0.5 blocks and Z by0..1 inclusive. **Neither burst spread nor
   size scales this position jitter.** Relevant draw range:0xaa25a–0xaa2a7.
2. **Draws4,5,6: velocity Z,Y,X.** Each uses rand÷32767 minus0.5.
   Once per burst, first multiply its spread by **2.0f**, word0x40000000
   at0x4ac9d0, storing the rounded result at0xaa20e–0xaa217. Multiply
   each centered component by that result, then add the corresponding
   base burst velocity in float32. The multiply/add helper calls are
   **0xc7c40 at0xaa349** and **0x172b0 at0xaa35f**. Position jitter's
   asymmetric Z rule does not apply to velocity: all three are centered.
3. **Draws7,8,9: initial angles Z,Y,X.** For each, multiply converted rand
   by **360.0f** before dividing by32767, preserving that operation order.
   Store all three angles without wrapping. The multiplier is word0x43b40000
   at0x42e7e4; native draw/packing range is0xaa373–0xaa3d4.
4. **Draws10,11,12: angular velocity Z,Y,X.** Each uses rand÷32767 minus0.5,
   then multiplication by **100.0f**, word0x42c80000 at0x42e5bc. The native
   scalar is loaded at0xa8ff9 and used by **0xc7c40 at0xaa43e**. No angular
   normalization follows. Store the three results, bounded by−50..+50
   degrees/second for standard CRT results.

Then copy all four burst color components through **0x17170 at0xaa460**.
For **style1 only**, consume **draw13 at0xaa46b** and replace the entire
RGBA with **(1,rand÷32767,0,1)** through0x16ff0/0x17170 at0xaa493/0xaa4a2.
For **style2**, retain all copied components and take no extra draw.
For **style5**, retain the copied components, set collision byte1 and
consume **draw13 at0xaa4b7** for **initial age=rand remainder500**, stored
at0xaa4db. This age is nonnegative0..499, not a negative start delay.

Finally copy burst size, set the extra matrix to identity and append to
the live list. With the ordinary nonnegative CRT range, totals are
**13 draws for1,12 for2,13 for5** per emitted particle. No draw depends
on frustum admission, later collision, particle age or eventual cap removal.
The later updater's1000-entry trim discards newest tail entries only after
the expansion has already consumed these draws for all emitted records.

**Shared expansion applies to every burst style tag.** In particular,
styles0/3/4/6/7/8 also consume the same12 base draws, copy RGBA/size, set
the extra matrix to identity, and receive age0, collision0, freeze0 and
lifetime4000. The only style comparisons in this expansion block are the
style1 color/draw13 case and style5 age/collision/draw13 case above. An
unsupported later style must not be skipped at expansion in a way that
omits its CRT draws ahead of a bounded style. This common producer fact
does not establish those other styles' update, draw or resource branches.

## 4. Once-per-frame update, age and cap

The Game frame calls **0xb37f0 at0xaaed9**, after expansion and before
scene draw. **0xaaed0** truncates its retained capped double frame-ms value
to signed32 and passes that interval. This is one integer-ms update per
admitted Game frame, not one per World substep or a simulation-time clock.
The ordinary frame cap is1000 ms; fractional milliseconds are discarded
for this consumer, without a carried particle remainder. See the established
Game interval producer in steam-gameplay-camera§7.2/steam-scene-lighting§5.
Newly expanded particles receive this same frame's update immediately.

At entry, when the valid live count exceeds1000, **0xa2460 at0xb38a8**
trims it to1000 by repeatedly unlinking/freeing the **tail**, i.e. newest
entries. This is before age or motion. It does not undo expansion's CRT
draws and does not preferentially preserve new bursts or remove the oldest.
No packet/burst grouping survives this cap.

Visit the remaining live list forward. Add the signed32 interval to
**age+0x04** first, using low32 addition (**0xb38f3**). For the bounded
styles, motion runs only when updated age>=0 and freeze byte+0xb1=0.
Freeze does not stop age or retirement. The selected ordinary constructor
sets freeze0, and style5's nonnegative birth age is not a delayed-start flag.

After the visit's motion or its skip, queue the particle for retirement
only when **age>lifetime+0xb4**, strictly; equality remains alive. Then,
after all visits, unlink/free queued nodes in queue order and decrement
the live count. There is no final fade, final callback or particle-generated
sound/hit/block-change event on this retirement path. With the constructor's
lifetime4000, style5's initial0..499 age shortens its remaining duration.

## 5. Motion for styles1/2/5

For a reached motion visit, convert interval to float32 and multiply by
**0.001f**, word0x3a83126f at0x42e514, to obtain seconds. Scalar operations
below use separate float32 rounding. First subtract **seconds×30.0f**
from velocity Z, word0x41f00000 at0x42e5a8. Then advance each angle by
angular velocity×seconds. This first angle advance is **0xb3948–0xb3988**;
there is no angle wrapping or smoothing.

### 5.1 Collision-off route, ordinary styles1/2

With byte+0xb0=0, advance each fixed position by its updated velocity
component×seconds×65536, rounded in that multiply order, then truncated
toward zero to signed64; add the delta in low64 arithmetic. There is no
terrain query or sweep. This is the gravity-updated velocity, not a
half-acceleration displacement or old-velocity integration.

Then **advance all three angles a second time** by angular velocity×seconds
(**0xb3d3e onward**). This second advance is a real extra operation and
must not be removed as a duplicate. Consequently ordinary styles1/2 have
two sequential rounded angular increments per frame; computing one
combined angular-velocity×2 increment can round differently.

### 5.2 Collision-on route, ordinary style5

With byte+0xb0 nonzero, compute displacement XYZ as updated velocity×seconds
in float32. Compute float32 squared X plus squared Y, then add squared Z,
and sqrtf. The point-step count is **trunc32(length)+1**, capped above at10.
For ordinary finite displacement it is at least1, even at zero interval.
Divide1.0f by that count, then multiply each displacement component by
that reciprocal to form a cached per-step vector. This is not a block-boundary
ray or an unlimited small-step sweep.

For every step, try **X, then Y, then Z**. Add that axis's cached component
times65536, truncated toward zero to signed64, to its fixed position. Query
the block at the **current full XYZ after this single-axis attempt**, through
**0xe9a0 at0xb3c77**, using the same signed64→narrowed signed32 block/floor
conversion documented in steam-world-traps§2. A zero component still makes
its query. No point radius or particle header extent is added.

For style5, **only block types0 and2 pass**. Every other type rejects the
axis attempt; byte3, RGB and block flags supply no additional admission.
Missing-zone default solid therefore rejects. On rejection, subtract the
same fixed delta back and zero that axis's stored velocity. For a rejected
**Z** attempt, additionally zero **all velocity XYZ and angular velocity
XYZ**, through the stores at **0xb3cb2–0xb3cc2**.

The cached per-step vector is **not recomputed** after those velocity writes:
remaining axes/steps still attempt their original cached displacement.
There is no break on collision, bounce coefficient, sliding projection or
random draw. This route does **not** perform the collision-off route's
second angle advance. Thus ordinary style5 spins once per frame until its
stored angular velocity is zeroed, while1/2 spin twice.

An admitted zero-ms style5 update still performs one step's three queries;
being inside a rejecting block can therefore zero velocity/spin without
position or age changing. No such queries occur for collision-off1/2.
No direct CRT draws occur in this updater's bounded paths.

## 6. Ordinary world draw admission and material

The normal scene branch calls **0xe6840 at0xe3ca7**, after the earlier
opaque/faded-second-list/effect draws and **before water**. Follow the
established overall order in steam-world-pass-controls§1; this is not a
GUI overlay or a special after-water particle composition pass.

Entry selects renderer **state2 at0xe6879** and sets **White0 at0xe688a**.
State2 through **0xc3dc0/0xc3150** binds the ordinary WORLD shader pair:
depth test/write enabled, **LESS_EQUAL**, stencil disabled, RGBA writes,
**alpha blending disabled**, full sample mask. It uses solid raster fill3,
**CullMode1=NONE**, FrontCounterClockwise1, depth clipping enabled and
scissor disabled. This is the same disabled-blend state object used by
state0, with the distinct no-cull raster state. Do not substitute a normal
blended/no-depth-write particle pipeline merely because the payload has RGBA.

Walk the live particle list forward, with no distance sort. Require the
retained renderer's **0xe08c0 at0xe6957**: position is live fixed XYZ,
sphere radius is **the particle size+0xac itself**, and range is
**renderer+0x49c**, without+64. The helper's fixed distance gate and all
six CPU clip planes apply as specified in steam-scene-lighting. There is
no additional containing-zone active-mesh test, HP test or age/fade test
here. Surviving age==4000 particles still draw. The updater, not this
routine, owns retirement.

For each admitted particle, material RGB is particle RGB multiplied
componentwise by **renderer frame material+0x530/+0x534/+0x538**, with
separate float32 products. The material-alpha/Glow distinction is:

| Style | Light input and fourth material component | Glow | Shininess |
|---|---|---|---|
|1/2|Fourth component explicitly1; do not use stored alpha|1|0|
|5|Stored alpha×frame alpha+0x53c, then×max(0.2f,L)|0|0|

For5, **L=float32(0xdab20 result)÷255.0f** at the current particle fixed
XYZ, channel0, through **0xdab20 at0xe6999**. It is not a trilinear light
sample: use the block-light accessor contract in creature-upkeep§4.7
(including its upward type10/12/19 handling and missing-zone255 result).
The float floor0.2 is word0x3e4ccccd at0x42e700. The type1/2 replacement
occurs at **0xe6a2c–0xe6a38**, after the candidate alpha products; it does
not make their outgoing burst alpha initialized data.

Submit material through **0xc5a20 via slot+0x98 at0xe6b1b**, Glow through
**0xc59f0 via+0x78 at0xe6b2b**, and Shininess through **0xc5a60 via+0xb0
at0xe6b38**. Glow1 here is the ordinary WORLD parameter, not a different
ungraded/unfogged particle shader. The full WORLD color/fog/grade contract
remains in steam-world-shader-output.md and steam-voxel-vertex-inputs.md.

This draw routine does not set separate pixel Opacity or DepthOffset, clear
or select a new point-light table per particle, upload a new sky/directional
palette, or replace the current view/projection. Those buffers are inherited
from the reached scene/pass state; a state bind does not reset them. It
does not clear depth or install a private viewport/scissor. Its material
alpha is not a replacement for the separate pixel-opacity constant.

## 7. Resource and model transform

Styles1/2/5 all use catalog index **0xa46**, the file-backed resource
**`cube.cub`**, through the pointer at **World+0x70**. The resource-catalog
object has pointer-vector begin/end at **+0x10/+0x18**; entries are8-byte
resource pointers. The count must exceed0xa46 (at least0xa47); select the
pointer at **begin+0x5230**. This is a catalog binding, not a filename sort
position or a particle-specific procedural resource cache. Native resource
readiness in **0x16ec60 at0xe6dc8** requires its two draw-buffer wrappers
at resource+0x70/+0x78 to be nonnull and report ready. That does not remove
the earlier requirement for a valid resource/header pointer.

### 7.1 Verified asset initialization and extraction/load data

In catalog initializer **0x177f50**, **0x19c48b** references the exact narrow
filename **`cube.cub`** at **0x455b50**. At **0x19c4a2**, **0x6dec0**
constructs the initializer's common catalog filename prefix followed by
that filename. It copies the prefix and appends the literal; it does not
introduce a special particle path or generate cube voxel data.

At **0x19c4cb–0x19c4d3**, index0xa46 is selected through **0x1734c0**
on the catalog vector at+0x10. That helper returns the slot address
begin+8×index; the initializer dereferences the slot's existing resource
pointer and calls **0x16f320 at0x19c4f0** with the constructed filename.
The exact load flags are **fourth argument1, fifth argument0**.

The loader0x16f320 writes its first byte flag to **resource+0x99**;
here it is1, the established voxel-classification policy under which
RGB(0,0,0) is empty and other RGB values are occupied. With a null optional
data-source object, it opens the constructed filename as a binary file;
with a supplied source object, it retrieves the named byte data through
**0x670f0**. Both paths read the same CUB content and pass it to **0x170030**:
three consecutive little-endian signed32 dimensions X/Y/Z, followed by
**3×X×Y×Z raw RGB bytes**, with no voxel alpha/type/light channel.

The raw byte index is **3×((z×sizeY+y)×sizeX+x)**, with X fastest and Z
slowest.0x170030 stores the full allocation dimensions at resource
**+0x88/+0x8c/+0x90**, allocates the RGB volume at pointer **+0x68**, and
copies these bytes unchanged. Do not halve, gamma-convert, shade or crop
that owner data during extraction. Native mesh/vertex encoding is a later
stage specified in steam-voxel-vertex-inputs.md.

The fifth load flag0 follows the common rebuild path:0x170030 invokes
**0x170700(resource,0)** after loading. This flag is not itself the resource's
mesh color-mode byte+0x9a. A live-resource capture should preserve actual
**+0x99 classification, +0x9a color mode, +0x9c corner-sampling radius and
+0xa0/+0xa1/+0xa2 additive color bytes**, as well as dimensions and raw RGB,
under their established shared resource contract. Do not derive those
metadata values from the filename or from the final rebuild flag alone.

This closes identity as **owner file-backed CUB geometry**. The catalog
loader builds its ordinary drawable resource from those bytes; this entry
does not select a native procedural cube-shape generator. The same catalog
index is also returned for item category10 by0x173b10. No dimensions,
occupied-voxel bounds or white RGB value are asserted merely because the
filename is cube.cub. Use the owner's file or that bound live RGB allocation.
The outer catalog-prefix configuration and optional data-source/container
format are not newly specified by this narrow identity trace.

### 7.2 Particle model transform

Start the model matrix with translation in scene-relative blocks:
add **renderer fixed offsets+0x428/+0x430** to particle fixed X/Y in
low64 arithmetic **before** signed64→float32 conversion and multiplication
by1/65536. Z converts directly from its signed64 fixed coordinate, with
no corresponding renderer Z offset in this path.

Postmultiply rotations in order **Z by angle+0x40, Y by+0x3c, X by+0x38**,
through **0x2b160 at0xe6c17/0xe6c34/0xe6c51**. They use the positive unit
axes. That helper multiplies degrees by float32 **0.01745329238474369**,
word0x3c8efa35 at0x42e520, then uses cosf/sinf and float32 matrix products.
It is the established postmultiplication convention, not a reversed Euler
order or a double-degree conversion.

Uniformly scale the first three matrix columns by particle size; do not
scale translation. Then postmultiply the particle's extra matrix+0x6c
through **0x173e0 at0xe6ca7**. This matrix is identity for the ordinary
burst producer. There is no age-dependent size factor for styles1/2/5.

Finally posttranslate by **minus half the full signed32 header dimensions**,
resource+0x88/+0x8c/+0x90, converting each dimension to float32 and multiplying
by−0.5f. This centers the full voxel allocation, not an occupied-vertex AABB,
and does not normalize scale by header width. A header dimension3 with size0.5
therefore gives a **full allocation span** of1.5 world blocks on that axis
before rotation; occupied geometry can cover only a subset of that allocation.

For exact float staging of this final inline centering, form the three
basis-column products separately. The X row adds Y-product+X-product,
then Z-product; the Y/Z/W rows add X-product+Y-product, then Z-product.
Each row then adds its previously stored translation component to that
three-product sum. These are separate float32 operations at
**0xe6cac–0xe6dbc**. Submit the resulting model matrix at **0xe6dc2**,
then draw the resource. View/projection, encoded owner voxel colors/normals
and the scene render target/depth remain the ordinary inherited inputs.

## 8. Scope and remaining limits

This closes the finite ordinary burst-generated1/2/5 slice with valid
catalog/resource data and representable motion/step counts. It does not
specify independent producers of other live-particle records, recover from
nonfinite data/overflow or a missing resource. The remaining0/3/4/6/7/8
consumer contracts are now independently specified in
[particle style consumers](steam-particle-style-consumers.md), including
style7's separate note.cub resource. Their branches must not be inferred
from1/2/5. Catalog0xa46 is identified as cube.cub
in§7.1; its actual dimensions/RGB remain owner data, and the outer asset-root/
container configuration remains outside this bounded particle contract.
