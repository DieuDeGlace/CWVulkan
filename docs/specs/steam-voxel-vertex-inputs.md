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
doubles it to P. A fully open raw white ordinary terrain corner with light255
stores **(255,255,255,255)** and gives **P=(2,2,2),V=1**. Preserve P>1 through
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

No new runtime oracle was requested or performed. Face topology, geometric
model transforms and all alternative terrain shader/pass controls are not
re-specified by this color/input document. The ordinary world/CUB/terrain
encoding and the named special cases above are verified; unrelated mesh
producers must retain their own explicit mode rather than assuming one RGB
scale or alpha convention from this shader alone.
