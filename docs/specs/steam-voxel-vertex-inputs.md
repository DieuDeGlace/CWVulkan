# Steam voxel vertex colors, light input and model glow

Independent clean-room specification from the owner's Steam native functions,
assembly, embedded constants and compiled shader. No engine/test sources were
read, no original game was executed, and no source/pseudocode is reproduced.
Addresses are RVAs. This completes the CPU input side of
`worldgen-parity-gaps.md` §§9.3/9.8 and uses `steam-scene-lighting.md` for the
frame palette and point-light collector.

## 1. Shared packed vertex input

**0xc3a10** packs a vertex into8 bytes, in order:
**positionX,positionY,positionZ,normalIndex,R,G,B,V**. Position bytes are the
low8 bits of integer local corner coordinates. Normal index0..5 corresponds
to **+X,−X,+Y,−Y,+Z,−Z**. Terrain uses its existing250-block vertical bands
to keep local Z coordinates in this byte representation.

For each input float color/light component, packing computes its float32
product with255, converts to signed integer toward zero and stores the low
byte. The packer does **not** add a new clamp or round-to-nearest step. Earlier
color construction normally supplies bounded RGB; the special alpha2 case
below demonstrates why the low-byte behavior matters.

**0xc60d0** creates the ordinary world input layout: POSITION and COLOR are
both **DXGI format28, R8G8B8A8_UNORM**, at byte offsets0 and4. Thus both groups
arrive as normalized unsigned bytes. The world VS **0x4387b0** multiplies
position XYZ by255 to recover integer corners, interprets the position W
as normalIndex/255, and reads COLOR as normalized RGB plus V. This is not a
floating RGB buffer, signed-byte normal vector or a per-vertex glow-bit format.

The shader defines palette RGB **P=2×normalized stored vertex RGB**. It does
**not** double V. P can reach2; don't clamp P to1 before the shader's
luminance/saturation/color operations. The eventual vertex-color clamp and
pixel processing are later steps, not a reason to discard these intermediate
values. Byte normalization performs no sRGB transfer or gamma decoding.

## 2. CPU color operation shared by terrain and CUB sprites

The two producers use the same color-shaping arithmetic, followed by different
encoding scales. Let c be the incoming normalized RGB and a the powered
occlusion factor specified below. First multiply c by a. Compute luminance
**L=0.3R+0.59G+0.11B**, using the native float constants at
0x42e704/0x43ef64/0x43ef48. If L>1, multiply RGB by1/L and replace L by1.
Let **s=3−2a**. For each channel form **(1−s)×L+s×channel**, then clamp that
channel to0..1. These are float32 operations; the exponent is powf with
float **1.2000000476837158** at0x42e718.

This operation darkens and changes saturation in the **CPU-generated RGB**.
Its occlusion factor is not stored in V and cannot be removed merely by
changing a later shader's world-light input. Keep the CPU operation distinct
from the shader's additional shade-dependent saturation in the earlier audit.

## 3. Ordinary CUB sprite color and V

Sources: bulk owner-asset loader **0x177f50**, constructor **0x16e6b0**,
file loader **0x16f320**, voxel copy **0x170030**, mesher **0x170700**,
corner color **0x170530**, occlusion **0x171db0**, packer **0xc3a10**.

CUB voxel colors are raw RGB triplets; the file loader copies them without
halving or gamma conversion. Indexing is **3×((z×sizeY+y)×sizeX+x)**, with
dimensions at sprite+0x88/+0x8c/+0x90 and RGB data pointer+0x68. Empty means
the raw triplet(0,0,0). Some load modes suppress pure RGB attachment-marker
voxels from geometry; that separate+0x99 policy does not turn the color scale
into a gamma rule.

For a corner with face normal n,0x171db0 samples the layer immediately in
front of the face. Its integer sampling radius is sprite+0x9c, initialized
to **3**. Along a zero-normal axis, offsets are **−N through N−1**; along
a positive-normal axis the only offset is0; along a negative-normal axis
the only offset is−1. Iterate X outer,Y middle,Z inner. Sample weight is
**(N−max(|dx+0.5|,|dy+0.5|,|dz+0.5|))/N**, float32. Out-of-bounds voxels
count as empty; within the array only the raw black triplet is empty for
this occlusion test. A hidden nonblack attachment marker can therefore still
occlude; this helper doesn't call the separate geometry-marker filter.

Let open be weighted empty samples divided by all sample weights. For
positive total weight the native occlusion result is **max(0.5,open)**;
for zero total weight it returns0. With ordinary N=3 the total is positive.
Set **a=powf(occlusion,1.2)**. Before §2's color operation, use
**c=rawVoxelRGB/255 + sprite byteRGB(+0xa0/+0xa1/+0xa2)/255**. The constructor
initializes those additive shade/color bytes0. They are not the zone model's
separate tint record+0x2c..+0x34.

0x170530 sets the resulting color's fourth component to **2.0**, then checks
sprite byte **+0x9a**. When that byte is0 it multiplies **all four components
by0.5** (float4 at0x4329b0), before packing. Thus this mode produces half
RGB and V=1 before the byte conversion. Ordinary stock CUB sprite instances
in the bulk loader are constructed with this byte0: the constructor call at
**0x177ffa** has its third argument0. File loading doesn't replace that mode.
The mode is an explicit sprite setting, not derived from zone flags.

Consequently ordinary stock CUB **V byte is255**, normalized V=1, at every
corner independently of occlusion. Its AO is in RGB. The model's mesh-time
world-light sample enters separately as draw material alpha A; V is not
replaced by that sample during asset meshing.

With sprite+0x9a nonzero, the halving is omitted: RGB remains the full §2
result, fourth component2.0 packs as **low8(510)=254**, and shader V is254/255.
Map region sprites created via0x1737b0 use mode1; the special map behavior is
already specified in `worldmap-rendering.md`. Don't impose that map encoding
on the ordinary stock CUB asset path, or silently saturate its2.0 alpha to255.

**Exact simple example:** a fully open white CUB corner with zero additive
shade, ordinary mode0, has CPU RGB0.5 and V1. It stores **(127,127,127,255)**;
the shader's P is **(254/255,254/255,254/255)**. It is not encoded white255
followed by another factor2, nor exactly unquantized white1. For general
colors retain the float32 operations and byte truncation rather than assuming
that divide-by-two followed by shader doubling cancels losslessly.

## 4. Terrain color and vertex light V

Sources: zone mesh job **0xe9530**, corner computation **0xe8f70** and the
same packer. For an ordinary effective non-air stored block, the incoming
RGB is its first three bytes divided by255; incoming fourth component is1.
The mesh job can synthesize buried-block colors below a column's stored base
through0x2d8e00/0xba820, as already specified in the terrain rules. Such colors
are not evidence of an extra stored six-byte record. **Water(type2)** instead
uses the fixed incoming RGBA **(0.25,0.5,0.5,1)** at0x43f0e0; the propagated
light byte temporarily stored in water's red channel is not its surface RGB.

Ordinary0xe8f70 uses the same layer offsets as §3 with **fixed N=3**, same
weight and X/Y/Z iteration order. It queries effective blocks through
**0xda660**. A sample is open for these calculations exactly when its type
is **0 or2**; the test is on type, not occupancy flags or raw RGB. Compute
open as weighted open samples divided by all weights, using0 when total0.
**Unlike the CUB helper, terrain does not raise open to a minimum0.5.** Set
a=powf(open,1.2), and use §2's CPU color operation.

Independently, only for those open samples, accumulate the **completed mesh
light-volume byte** times the same weight, and accumulate their open weights.
The color's fourth component is **weightedLightSum/openWeightSum/255**,
or0 if openWeightSum0. This is the source of terrain **V**. The map is the
one solved before face building in0xe9530/0x2a3260, including the five flood
rounds from the previous audit. Its lookup returns255 outside horizontal
bounds,0 below a stored light-column base, and255 at/above its exclusive end.
Solid samples don't enter that weighted light mean even though the completed
map retains light at solid positions for model sampling.

Terrain RGB then goes to0xc3a10 **without CUB's factor0.5**. All four computed
components are packed with the shared multiply255/truncate/low-byte rule.
The shader therefore sees full CPU RGB normalized from those bytes, then
doubles it to P. A fully open raw white ordinary terrain corner with all36
sample light bytes255 stores **(255,255,255,254)** and gives
**P=(2,2,2),V=254/255**. Preserve P>1 through
the specified shader operations. This intentional encoding difference is
not a second light-volume sample or a geometric-opacity alpha.

**Special corner bypass:** the mesh job passes0xe8f70 a true bypass byte for
block **type0x14**. That branch returns the supplied base RGBA unchanged,
skipping corner AO and light averaging; its V is the incoming1. This is a
block type, not the similarly numbered block flag or zone-model glow flag.
The optimized whole-water surface quad also supplies the fixed water RGBA
directly. Water face sampling/draw normals in this mesh path use+Z, including
its side faces; don't substitute an ordinary solid side's sample layer when
checking its V. These are CPU-input facts; individual terrain material/pass
effects remain separate draw behavior.

### 4.1. Exact terrain light-mean precision; correction of the ideal example

The earlier version's all-open terrain V255 example was an ideal real-number
derivation, not a recorded packed result. It was incorrect for the native
single-precision accumulation. This subsection corrects it; ordinary stock
CUB V255, type0x14 V255 and the directly supplied optimized-water V255 remain
separate, unchanged cases.

In0xe8f70, each offset is converted to float32 before adding0.5. The three
absolute offset values, their maximum,3 minus that maximum, and division by3
produce a float32 weight. X/Y/Z traversal remains as above. For every open
sample the byte light value is converted exactly to float32, multiplied by
that weight with float32 rounding, and added to the float32 light sum with
another rounding. The open-weight sum independently adds each float32 weight
in that same order. They start at positive zero. The native instructions at
**0xe9294,0xe929f,0xe92a3** are respectively the single-precision weight-sum
addition, light-times-weight multiplication and light-sum addition. There is
no double accumulator, fused multiply-add or pre-normalized-weight sum.

When the open-weight sum is positive, first divide the completed light sum
by the completed open-weight sum in float32 (**0xe9351**). After the separate
RGB work, divide that retained float32 mean by float32 255
(**0xe947b**, constant0x42e7d8). No final light/V clamp, snapping to255,
round-to-nearest-byte or all-open shortcut intervenes before0xc3a10 performs
the float32 multiply255 and signed truncation. Reassociating the divisions
or normalizing samples first does not preserve this rounding sequence.

For the36 open samples of any of the six unit face normals, with every
sample byte255, an IEEE float32 evaluation in the verified native order
produces open-weight sum **12.666669845581055** (bits0x414aaaae), light sum
**3230** (0x4549e000), mean **254.99993896484375** (0x437efffc), and prepack V
**0.9999997615814209** (0x3f7ffffc). The packer's product is again
254.99993896484375, so its stored V is **254**. AO is exactly1 here because
its open and total weight sums follow matching operations; raw white RGB
still packs255 in each channel. These numbers are a static arithmetic
derivation checked against the native instruction types/order, not a new
original-game runtime recording. Do not replace the general averaging rule
with a forced254: different sample values and the explicit bypass paths
retain their specified behavior.

## 5. What zone-model flag0x01 actually bypasses

The existing finding is confirmed producer-to-consumer:0xd3170 submits
**material alpha1** for flag0x01 models and **Glow=float(flags&1)**, hence1;
white and depth offset are0.0xee2f0 preserves these descriptor controls and
0xe6e60/0xe7980 restore them. Flag0x02 alone doesn't set this glow. Material
alpha1 by itself would still leave vertex light,directional/back,ambient,
point-light and shade-saturation effects; it is the additional glow scalar
that changes the color result.

The compiled world VS0x4387b0 first computes its normal lighting and
saturation-adjusted RGB C, then for positive Glow g selects
**C+g×(P×materialRGB−C)**. For the ordinary flag1 value1 this gives the unlit
P×materialRGB result, subject to shader floating arithmetic. The shader does
not have an early branch that skips all lighting work; the earlier formula
describes the finite color result. Its white operation still performs the
componentwise0..1 clamp even with white0. Subsequent pixel-shader fog/color
grading and the draw's separate opacity/fade remain active.

Crucially P here is the **CPU-shaded,encoded and quantized** vertex palette,
not the original CUB triplet. Flag1 bypasses dynamic shader illumination,
vertex-light V/material-alpha sunlight scaling and the shader's shade
saturation contribution to the resulting geometry color. It **does not undo
the CUB corner AO/saturation/color-offset operation already baked into RGB**.
It also doesn't turn record tint into voxel color. Its auxiliary emitted
point light is a separate collector effect, not this draw's V production.

For ordinary nonglow CUB models V=1 and material A=the mesh-time world sample
/255 times frame material alpha; terrain instead generally supplies its
corner light mean through V with the terrain draw material. The shader uses
**V×A only on the already clamped directional/front+back sum**. Ambient and
point-light terms are not multiplied by V or A. The world VS outputs V as
color alpha, but the ordinary pixel shader's geometric output opacity comes
from its separate pixel constant. None of these fields is interchangeable
with an arbitrary AO-as-alpha convention.

## 6. Verification inputs and scope

Useful small recording cases are an open/occluded ordinary CUB corner, the
same sprite with+0x9a toggled, a terrain corner with mixed open/solid light
samples, water and type0x14. Preserve source bytes, sprite mode/N/additive
bytes, face/corner, ordered sample openness/light/weights, computed floatRGBA,
packed8-byte vertex, draw material/Glow/White and final shader inputs. This
distinguishes a CPU color or byte-quantization mismatch from shader lighting.

No new runtime oracle was requested or performed. Sections7–8 now specify
ordinary cube-face topology, block render groups and water-film geometry. Geometric model
transforms and alternative terrain shader/pass controls are not
re-specified by this color/input document. The ordinary world/CUB/terrain
encoding and the named special cases above are verified; unrelated mesh
producers must retain their own explicit mode rather than assuming one RGB
scale or alpha convention from this shader alone.

## 7. Face corners, normals and triangle selection

Sources: ordinary CUB mesher **0x170700**, terrain mesher **0xe9530**, packed
vertex **0xc3a10**, adaptive quad indices **0xcd7d0**, and fixed quad indices
**0xcdea0**. These are ordinary cube faces; §8.3 separately covers wet-ground
water-film geometry.

### 7.1. Four corners and face order

For a voxel whose lower corner is (x,y,z), add the following unit offsets.
Corner numbers are the order of the four appended packed vertices, starting
at zero for each face. The table applies to both ordinary CUB faces and
ordinary terrain cube faces. W/E/S/N here are coordinate labels: they mean
−X/+X/−Y/+Y, respectively.

| Face | Solid normal / encoded index | Corner0 | Corner1 | Corner2 | Corner3 |
| --- | --- | --- | --- | --- | --- |
| W, −X | (−1,0,0) / 1 | (0,0,0) | (0,0,1) | (0,1,1) | (0,1,0) |
| E, +X | (1,0,0) / 0 | (1,0,0) | (1,1,0) | (1,1,1) | (1,0,1) |
| S, −Y | (0,−1,0) / 3 | (0,0,0) | (1,0,0) | (1,0,1) | (0,0,1) |
| N, +Y | (0,1,0) / 2 | (0,1,0) | (0,1,1) | (1,1,1) | (1,1,0) |
| Bottom, −Z | (0,0,−1) / 5 | (0,0,0) | (0,1,0) | (1,1,0) | (1,0,0) |
| Top, +Z | (0,0,1) / 4 | (0,0,1) | (1,0,1) | (1,1,1) | (0,1,1) |

Within a voxel, emitted ordinary faces follow **W, E, S, N, bottom, top**,
omitting hidden faces. Each existing face appends four new vertices; adjacent
faces do not share a vertex merely because their positions coincide. The
color helper receives each listed physical corner, with that face's sample
normal. CUB sample coordinates are sprite lattice coordinates. Terrain
sample coordinates are world block-lattice coordinates, whereas packed
terrain XY is relative to the zone and packed Z is relative to the selected
250-block band. Do not feed band-relative Z into the world sample query.

The listed triangle winding faces the named outward direction for solid
faces. Water keeps these geometric corner orders, but substitutes **+Z for
both the draw normal and color-sampling normal on every face**, including
side and bottom faces. Thus water's normal index is4 even when its geometric
face is vertical or downward. It does not reorder a side into a horizontal
quad to achieve that normal.

### 7.2. CUB triangle indices are fixed

After generating CUB vertices,0x170700 emits exactly **(0,1,2), (0,2,3)**
for each consecutive group of four vertices, with that group's base index
added. The shared diagonal is **0–2**. The two triangles reference those
four vertices; they do not append six duplicate vertex records. Neither
RGB, V, AO nor a comparison of opposite corner values changes this choice.

The CUB index buffer uses unsigned16 indices when the complete sprite's
vertex count is strictly below65536, and unsigned32 otherwise. Both paths
preserve the same six-index order. This confirms the topology already
recorded for region CUBs in `worldmap-rendering.md`; the terrain rule below
is different.

### 7.3. Ordinary terrain triangle indices use packed RGB

For each completed four-vertex face,0xe9530 calls0xcd7d0 on those last four
**already packed** eight-byte vertices. At each corner, form float32
luminance from the unsigned R/G/B bytes at offsets4/5/6:
**0.299R + 0.587G + 0.114B**. The constants are respectively float32
0.29899999499320984,0.5870000123977661,0.11400000005960464 at
**0x43ef50,0x43ef60,0x43ef4c**. Multiply each byte-converted float separately,
then add the R and G products before the B product, with float32 rounding.
These coefficients differ from the CPU color-shaping luminance in §2.

Compare the absolute float32 luminance difference between corners1 and3
against the absolute float32 difference between corners0 and2. When
**|L1−L3| ≤ |L0−L2|**, including an exact tie, append triangles
**(0,1,3), (2,3,1)**: diagonal **1–3**. Otherwise append
**(0,1,2), (0,2,3)**: diagonal **0–2**. Each index has the current face's
vertex base added. The four vertex records stay in their original order.
In particular, a face with four identical packed colors uses diagonal1–3.

The comparison reads no V bytes. It does not compare a sum of two opposite
AO values, a sum of opposite light values, the raw block RGB, or unquantized
CPU colors. Occlusion can change the chosen diagonal indirectly by changing
the CPU RGB that is subsequently packed. Changing only V while retaining
the packed RGB cannot change it. Use the byte-valued RGB operands directly;
normalizing them first introduces different rounding near ties.

This adaptive rule also applies to **individual water top, side and bottom
faces** produced by the ordinary voxel loop, despite their common +Z normal.
The helpers construct unsigned32 indices; the ordinary-solid and water
upload paths narrow them to unsigned16 when that band's shared vertex count
is strictly below65536. This upload choice does not change triangle order.

### 7.4. Optimized whole-zone water surface is a separate fixed quad

After its exposed-height prepass and maximum-height adjustment,0xe9530
considers this path when the minimum is below1 and the maximum is at least0.
It checks all64×64 zone columns through the effective-block accessor:
the block at world Z0 must have **type2**, and the block at world Z1 must
have **first byte255**. The second test is on the first byte, not on the
type field. A failure at any column disables the optimization for this zone
mesh job.

On success, it emits one horizontal quad at **world Z1**, with zone-local
XY corners **(0,0), (64,0), (64,64), (0,64)** in that order. Packed Z is
1 minus that quad's band base. All four normals are +Z/index4 and all four
RGBA inputs are the directly supplied **(0.25,0.5,0.5,1)**;0xe8f70 is not
called. The resulting four color/light byte tuples are each
**(63,127,127,255)**.

Before appending the four vertices, the mesher uses0xcdea0 with their future
base index. That helper appends fixed triangles **(0,1,2), (0,2,3)**, hence
diagonal **0–2**. It does not make the ordinary adaptive helper's tie choice.
After this optimization, the voxel loop suppresses **all ordinary faces of
type2 voxels at world Z0**, not only their top faces. Other water voxels
still use the ordinary face path; when optimization is disabled, Z0 water
also uses that path.

For a bounded geometry oracle, preserve the source block/sprite mode,
physical face/corner, four ordered packed vertices and six ordered indices.
Include identical-RGB/different-V terrain, each of the two strict luminance
inequalities, an exact tie, one water side and the optimized whole-zone water
quad. Those distinguish a corner permutation, a color-quantization change
and an incorrect diagonal heuristic without relying on a rendered picture.

### 7.5. Water optimization observes post-solver terrain red bytes

The effective-block getter used by §7.4 is **0xda660 → 0xeb90**. It reads
the actual terrain columns through the cached neighboring-zone pointers;
it does not obtain that first byte by calling the temporary light-volume
lookup0xe0a20. Nevertheless, for target-zone stored open blocks, the bytes
it sees have already been changed by the solver called inside this mesh job.

The verified synchronous order in0xe9530 is:

1. Obtain the existing target zone and cache pointers to its3×3 neighboring
   zones; construct the padded temporary light volume with0xcb9c0.
2. Call **0x2a3260** to initialize and propagate light. Before returning,
   that helper writes the final computed light into **red byte0 of every
   stored type0 or type2 block in the target zone's64×64 columns**, preserving
   G, B, glow, type and flags. It uses the existing stored indices and
   **0xa36e0**, with the final writeback call at **0x2a3d95**. This target-zone
   writeback is distinct from the temporary primary/work light buffers over
   the padded domain. It does not recolor stored solid blocks.
3. Sample the completed temporary light volume into ordinary/auxiliary model
   records, and perform their effective-water tests.
4. Run the exposed-height prepass and then the whole-zone water optimization
   tests from §7.4; build the faces afterward.

There is no requirement that a separate earlier job must already have called
0x2a3260. The0xe9530 call itself completes the red-byte writeback before its
optimization test. A fresh-generation snapshot with original air/water reds
is therefore a different stage from the accessor state at that test. The
corner V computation independently samples the completed temporary volume,
as specified in §4; these two readers must not be conflated.

For an **actually stored Z1 air/water record**, its red byte at this test is
the newly solved target-zone light byte. For **Z1 at or above the column's
exclusive stored end**,0xeb90 instead returns the implicit air constant
**(255,255,255,0,0,0)**, initialized at **0x10e0** into0x54dcb8. That gives
first byte255 without inserting an air record or consulting a stored original
red byte. Below a stored base the accessor's separate implicit solid constant
has RGB200 and type1. Stored solid Z1 records keep their original RGB: the
optimization still checks only their first byte, not a light sample or an
additional type0 test. Do not replace the native first-byte predicate by an
unconditional temporary-volume sample or a requirement that Z1 be air.

For a phase-labeled oracle retain both **fresh-generation six-byte blocks**
and **post-0x2a3260 target columns**, plus each Z0/Z1 effective getter result
at the optimization test. Label whether Z1 was stored or implicitly supplied;
otherwise equal light-volume results can conceal different accessor inputs.

## 8. Lava, poison, wet-ground and light-block render classification

Sources: effective-block meshing and group selection **0xe9530**, especially
**0xea3c8–0xea443**; wet-ground top branch **0xeb9fc**; band publication in
0xe9530; solid-terrain draw **0xe5070**; frame setup **0xe2e00**; draw-state
selection **0xc3dc0**; light-constant setter **0xc5a10 → 0xc6ce0**, whose final
argument is ambient RGBA through **0xc6820**.

| Effective block type | Ordinary cube-face index group | Additional behavior |
| --- | --- | --- |
| 3 / 0x03, wet ground | Ordinary solid | With effective air directly above, an additional water top quad |
| 17 / 0x11, lava | Separate lava group, drawn opaque | Temporary directional/ambient palette described below |
| 19 / 0x13, poison | Ordinary solid | No poison-specific draw palette or transparency in this path |
| 20 / 0x14, light block | Ordinary solid | CPU corner bypass from §4, not shader Glow |

### 8.1. Renderability and neighboring-face rule

All four types enter the nonzero-type cube-face path. Raw black RGB does not
make such a terrain block empty. They are not omitted because a separate
physics/collision test might treat a liquid or special type differently.

For **each of these four types**, an ordinary face is emitted exactly when
the effective neighboring block in that direction has **type0 or type2**.
The rule is the same on all six sides, including top and bottom. A neighbor
of any other type hides that face. Consequently lava–lava, lava–poison,
poison–wet-ground, light–ordinary-solid and the reverse interfaces do not
produce internal faces. Faces against water do exist for all four types and
stay in their respective opaque group. Water's reciprocal face exists only
against air; a water–solid interface is not a pair of translucent faces.
These are tests on effective accessor results, including sea substitution,
not merely a stored column's raw type byte.

The four types retain ordinary per-face normals and the ordered corners in
§7.1. Their face indices use §7.3's packed-RGB adaptive diagonal. They do not
take water's all-+Z normal substitution. Their base RGB is their ordinary
effective or synthesized terrain RGB, not the fixed water color. Types3,
17 and19 use the ordinary CPU AO and weighted-light computation. Type20
instead supplies the unmodified normalized base RGB and V1 through the
explicit corner bypass. It still has ordinary opaque geometry and the
ordinary shader's later lighting/material/fog operations; the bypass does
**not** submit shader Glow1 or White1.

### 8.2. Groups, pass and lava's actual lighting override

The published mesh-band object is0x60 bytes. Its shared vertex-buffer
reference is **+0x08**, ordinary solid index-buffer reference **+0x10** and
triangle count **+0x44**; water index-buffer reference **+0x18** and triangle
count **+0x48**; lava index-buffer reference **+0x28** and triangle count
**+0x50**. The common vertex count is+0x40 and signed band-base Z is+0x5c.
The four types do not create separate vertex layouts. Group separation is
through index lists referring to that band's shared vertex array.

0xe5070 draws the visible zone list in its existing order, each zone's
published bands in order. Within a band, its ordinary solid group is drawn
before the separate lava group. The intervening optional index groups at
+0x30/+0x38 do not receive ordinary faces from the four types considered
here. Both solid and lava use **state0 and the ordinary world shaders**:
depth test enabled, less-or-equal, depth writes enabled, alpha blending
disabled, ordinary solid culling. The water depth/color passes occur later
as specified in `water-rendering.md` and the corrected frame order in
`worldgen-parity-gaps.md` §9.5. Poison is not sent to those water passes.

The terrain pass clears White to0. Normal frame setup at **0xe39dd–0xe39e7**
sets shader Glow0; lava classification does not introduce a new Glow value.
For a band's lava group,0xe5070 temporarily replaces these light inputs:

| Input | Exact native value | Constant source |
| --- | --- | --- |
| Directional front RGBA | (1,1,0.5,1) | 0x43f110 |
| Directional back RGBA | (0.5,0.30000001192092896,0,1) | 0x43f050 |
| Light direction XYZ | (0,0,1) | Locally supplied vector |
| Ambient RGBA | (0.4000000059604645,0.4000000059604645,0.4000000059604645,1) | 0x43f0d0 |

The light-direction setter normalizes that vector, leaving it (0,0,1).
The gray0.4 vector is **ambient, not material**. The draw retains the current
ordinary frame material, the zone's uploaded point lights, the packed vertex
RGB/V and normal world-shader processing. It does not substitute a lava pixel
shader, animate vertices, select water transparency or bypass the lighting
formula through Glow1. After drawing that band's lava indices,0xe5070 restores
the caller's frame front/back/direction/ambient inputs before moving on.
Thus poison, wet-ground and type20 faces in subsequent bands do not inherit
the lava palette.

### 8.3. Wet-ground top film and ordering

When a type3 block's top face is exposed, the mesher separately requires
the block directly above at Z+1 to have **effective type0** before adding
the film. Water directly above exposes the ordinary opaque top but does
**not** produce this additional film. No analogous side or bottom film is
emitted by this branch.

The film is a second four-vertex top quad at the **same Z+1 plane** as the
ordinary top face, with the top corner order from §7.1, +Z draw/sample normal,
and incoming RGBA **(0.25,0.5,0.5,1)**. It goes through0xe8f70's ordinary
AO/light averaging and0xcd7d0's RGB-dependent split; it is not the direct
constant-color optimized sea quad. Its indices go to the **water group**.
The mesher appends the film's four vertices and water indices **before**
appending the ordinary solid top's four vertices and solid indices. The
ordinary top is still emitted with the wet block's own RGB and usual CPU
color/light calculation. Runtime draw order nevertheless renders the solid
group first and the water film in the later water passes. The coplanar film
uses those passes' existing less-or-equal depth behavior; it has no added
geometric height bias here.

## 9. Exact CPU RGB arithmetic precision and constants

This section makes the arithmetic in §§2–4 explicit at its rounding points.
Sources are the native instructions for **0x170700,0x170530,0xe9530,0xe8f70
and0xc3a10**, plus the embedded float words listed below. These are finite
ordinary byte/color inputs. Separate native single-precision multiply, add,
subtract and divide operations each produce a float32 result; their products
and following additions are not fused multiply-add operations. A wider
accumulator or an algebraically reassociated expression need not pack to the
same bytes.

### 9.1. Byte normalization really divides by255

**Raw CUB RGB:**0x170700 converts each unsigned byte exactly to float32 and
uses scalar float32 **division by255**, at **0x170835,0x17083f,0x170849**.
The divisor is the stored word0x437f0000 at0x42e7d8. This is not multiplication
by a stored rounded1/255, and not division by510 or a preliminary RGB halving.
The resulting normalized voxel RGB is the incoming color for0x170530.

**CUB additive shade:**0x170530 independently converts each unsigned shade
byte at sprite+0xa0/+0xa1/+0xa2 to float32, divides each by that same float255
(**0x17059b,0x1705cd,0x1705c1**, respectively), and adds each rounded quotient
to its already normalized voxel channel with another float32 operation.
These sums are formed before multiplication by the corner's powered AO.
There is no integer byte addition before normalization, no single division
of a combined raw-plus-shade numerator, and no clamp between shade addition
and AO multiplication.

**Ordinary terrain RGB:**0xe9530 converts the three unsigned RGB bytes to
float32 and supplies float255 in the fourth lane. It divides the four lanes
by the float4 **(255,255,255,255)** at0x43f1b0, using single-precision vector
division at **0xea253**. The buried-color replacement, when selected, makes
the same lane-wise division at **0xea38c**. Thus the fourth base component
is exactly1. The type0x14 corner bypass happens after this normalization.
Water's fixed float RGBA is supplied separately; it is not treated as raw
bytes and divided by255 again.

As a static IEEE float32 check, byte3 divided by float255 gives
**0.0117647061124444**, word **0x3c40c0c1**. Multiplying float3 by the rounded
reciprocal **0.003921568859368563**, word0x3b808081, instead gives
**0.011764707043766975**, word **0x3c40c0c2**. The latter is not the native
normalization. This is an arithmetic derivation, not a runtime capture.

### 9.2. Powered AO, luminance normalization and final shaping

The CUB and ordinary terrain shapers both call **powf**, with the earlier
AO result as the float base and float exponent word **0x3f99999a**. They use
its returned float32 result as the powered AO a. Native call sites are
**0x1705dd** and **0xe9379**. An alternative logarithm/exponential formula
is not an additional specified native stage. CUB's minimum-AO rule and the
terrain weight/light sums remain as specified earlier.

Each incoming RGB channel is first multiplied by a, rounding separately to
float32. Luminance is computed from **those multiplied channels**, not from
an earlier unoccluded luminance multiplied afterward by a. Form the separate
float32 products with0.3,0.59,0.11; add the R product and G product first,
round, then add the B product and round. The relevant native ranges are
**0x1705ed–0x170620** and **0xe9387–0xe93c2**. Do not substitute §7.3's
different0.299/0.587/0.114 coefficients from triangle selection.

Only when the resulting L is **strictly greater than1** does normalization
occur. Compute **1/L once using float32 division**, then multiply each
current RGB channel by that rounded reciprocal, rounding each product.
The retained L is replaced by the exact literal float1. It is **not**
recomputed from the newly rounded RGB. Directly dividing each channel by L
or using an approximate reciprocal instruction changes the operation.
The scalar reciprocal division is at **0x17062c** or **0xe93ce**.

For final shaping, first multiply a by float2 and round; subtract that result
from float3 and round to obtain s. Separately subtract s from float1 and
round, then multiply by retained L and round to obtain the **one shared
luminance contribution**. For each channel, multiply the current channel by
s and round; add that product to the shared contribution and round. These
are the rounding points of the shorthand **(1−s)L+s×channel**. They are not
a single fused operation, and the common contribution is not obtained by
expanding or distributing the expression into another polynomial.

Clamp each final channel to0..1 after that addition: values below0 become0,
values above1 become1. No RGB component clamp was inserted between the
preceding AO multiplication, luminance calculation or saturation products.
The native final-shaping ranges are **0x170640–0x1706b8** and
**0xe93e2–0xe947b**.

For the ordinary CUB mode, only **after the final RGB clamp** set the fourth
component2 and multiply all four components by float0.5. This is one
lane-wise float32 multiply at **0x1706e3**, using0x4329b0. Terrain omits that
halving. The shared packer then separately multiplies each supplied RGB/V
float by255 in float32 and converts the rounded product toward zero to a
signed integer, storing its low byte. Do not combine CUB normalization,
post-shaping halving and final packing into a different division constant.
The terrain V averaging/division order and its V254 example remain exactly
as corrected in §4.1; RGB processing does not recompute or clamp that V.

### 9.3. Exact constant words

Words below are the32-bit float bit patterns, not integer operands. The four
lanes of each named float4 are identical words. Decimal values round-trip to
those words; the words specify the constants without decimal ambiguity.

| Meaning | RVA | Float32 word | Decimal value |
| --- | --- | --- | --- |
| Byte divisor / pack multiplier | 0x42e7d8 | 0x437f0000 | 255 |
| Terrain normalization float4, each lane | 0x43f1b0 | 0x437f0000 | 255 |
| CPU red luminance coefficient | 0x42e704 | 0x3e99999a | 0.30000001192092896 |
| CPU green luminance coefficient | 0x43ef64 | 0x3f170a3d | 0.5899999737739563 |
| CPU blue luminance coefficient | 0x43ef48 | 0x3de147ae | 0.10999999940395355 |
| powf exponent | 0x42e718 | 0x3f99999a | 1.2000000476837158 |
| Terrain radius / shaping constant | 0x42e580 | 0x40400000 | 3 |
| AO multiplier in s | 0x4ac9d0 | 0x40000000 | 2 |
| Unit / normalized L replacement | 0x4ac980 | 0x3f800000 | 1 |
| AO offset / CUB minimum | 0x4ac964 | 0x3f000000 | 0.5 |
| CUB post-shaping float4, each lane | 0x4329b0 | 0x3f000000 | 0.5 |

No newly required native runtime recording was performed. If a numerical
oracle is added, preserve the normalized input float words, the separate
shade quotients/sums, powered AO return word, AO-multiplied RGB, retained L,
conditional reciprocal, shaped/clamped RGB and final packed bytes. That
separates a powf-return difference from a division/reassociation or packing
difference.
