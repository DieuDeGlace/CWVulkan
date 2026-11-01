# Steam full block setter and history restoration

Independent native-only contract for **0x31c1c0**, its World wrapper
**0xa37d0**, and the reached column writers **0xa36e0/0x6d030**. RVAs are
from the original Steam executable. This author inspected only native
sources/constants and existing independent prose, with no game execution
or engine/test/extractor/SDK/reconstruction/owner-export inspection.
The bounded ordinary contract assumes a valid constructed zone, finite
column material inputs and representable, allocatable column spans with
nonoverflowing ordinary Z-loop ranges.

## 1. Direct zone entry versus loaded-World wrapper

**0x31c1c0 takes a Zone pointer, signed32 global block X/Y/Z and a pointer
to six input bytes**: RGB at0..2, byte3, type at4 and flags at5. These
coordinates are already integer blocks, not fixed65536-unit positions.
The direct setter does not perform a World-map lookup or require that
the passed zone already be published in the loaded-zone map.

Its horizontal guard uses signed32 zone coordinates **Zone+0x10/+0x14**.
Compute each origin as zone coordinate×64 in low32 arithmetic and require
origin<=input<origin+64, using signed comparisons and low32 upper-bound
addition. There is no negative-to-zero clamp. Do not silently widen these
origin/upper-bound calculations to unbounded coordinates. After admission,
the column is at **Zone+0x518+(localX×64+localY)×0x58**, with local XY
obtained by subtracting that origin. X is the outer storage dimension.

The separate **0xa37d0(World,X,Y,Z,bytes)** wrapper first floor-divides
signed32 X/Y by64, finds the existing key in **World+0x460**, and calls
0x31c1c0 at **0xa3885** only for a nonnull loaded zone. It does not load
a missing zone. That loaded-map admission belongs to the wrapper;
direct generation/restoration calls to0x31c1c0 do not inherit it.

The column's **base Z+0x3c** is signed32. Its block vector begin/end/
capacity pointers are **+0x40/+0x48/+0x50**; count is their byte span÷6,
narrowed to signed32 for the setter's comparisons/arithmetic. Ordinary
end-exclusive top is base+count. The setter imposes no independent
world-height, nonnegative-Z or maximum-Z clamp.

## 2. Admission and whole-block sea substitution, in order

First, if input **type=0 and flags=0**, reject the write when
**requested Z>=the old base+old count**. RGB and byte3 do not override
this rejection. This precedes changing an empty column's base or
substituting water (**0x31c26a–0x31c291**).

For an admitted write, if old count is0, first set **base=requested Z**
at **0x31c2bb**. Then retain that current base as the pre-write base used
by the later gap loop. Consequently an empty column has no buried gap
back to its former arbitrary base value.

Next, when **requested Z<=0, input type=0 and input flags&0x02=0**,
replace the **entire six-byte input** with the sea-water block at
**0x553528**, through the choice at **0x31c2c1–0x31c2d3**. Its bytes are
**255,255,255,0,2,0**, also established in worldgen-terrain-tables§7.
This discards the requested RGB, byte3 and other flag bits. The exception
is bit0x02, not a test that all flags are0: type0/flags0x40 below sea level
also becomes this canonical water, whereas type0/flags0x02 remains the
requested type0 block. Nonzero input types do not take this substitution.

Write that requested-or-substituted block through **0xa36e0 at0x31c2e1**,
using relative signed32 index requested Z−the retained base. There is no
RGB shading, material recoloring, normalization or flag merging for this
requested cell. Except for the whole-block sea substitution, all six
bytes are preserved exactly.

## 3. Growth and raw gap bytes

**0xa36e0** writes six bytes directly when the relative index is already
inside the stored vector. It neither shrinks nor trims the vector.

For a negative relative index, it calls **0x6d030 at0xa3723**, growing
logical count by minus that index and shifting every old six-byte block
up by the same number of slots. **0x6d030** decreases column base by the
front shift after copying the vector. Old blocks retain their original
global Z and all six bytes. The requested block is written to the new
first slot, whose global Z is the request.

For an index>=count, call **0x6d030 at0xa3786** with new count=index+1
and front shift0. The base is retained and the requested block is written
at the final new index. Newly added gap slots are **six raw zero bytes
each**, not canonical ff ff ff air and not generated material colors.
The zero initialization is **0x6cb30→0x6caf0**;0x6d030 copies the old
blocks into this zeroed temporary vector before assignment through0x6cca0.
The final backing allocation may be reused or replaced; no pointer-stability
promise follows from growth.

The full setter then fills only the **downward interior gap** described
in§4. It does not recolor old stored blocks or run that fill on an upward
gap. Column base/count change with these operations, but the procedural
height at+0x08 and the cached material/environment fields are not recomputed.

## 4. Downward buried-material fill and exact packing

After writing the requested cell, visit Z=requested Z+1 upward, stopping
**before the retained pre-write base**. Native loop controls are
**0x31c2e6–0x31c2ea** and **0x31c415–0x31c419**. Thus neither the requested
cell nor the old base block is replaced by buried material. In a nonempty
downward extension, this covers exactly the newly inserted interior gap.

At each height call **0x2a3140 at0x31c375** with the World pointer at
**Zone+0x08**, the original global block X/Y, that signed32 Z, and these
unchanged cached column float inputs:

| Material input | Column offset |
|---|---|
| highlands | +0x14 |
| warmth | +0x0c |
| dryness | +0x10 |
| lava-region weight | +0x18 |
| forest-region weight | +0x1c |
| kind27 weight | +0x20 |

This is the existing buried-color contract in
[worldgen-terrain-zone.md§4.3](worldgen-terrain-zone.md#43-buried-colour-0x2a3140):
highlands>0.5 selects the rock material/type6; otherwise ground material/type5.
The sampler returns RGB as three float32 values and bytes3/type/flags
at property offsets+0x0c/+0x0d/+0x0e. Its established material outputs
have byte3=0 and flags=0. The color algorithms themselves remain in that
generation specification; saved history does not supply replacement gap RGB.

For each returned RGB component independently, multiply by **float32
0.5**, word0x3f000000 at0x4ac964; clamp the finite result to **0..255**,
with float32 upper bound255, word0x437f0000 at0x42e7d8. Then convert
toward zero to signed64 with **CVTTSS2SI**, and store its low byte.
The actual multiply/clamp/conversion sequence is **0x31c37d–0x31c3f1**.
This is not255 normalization, nearest rounding or a later mesh-color
encoder. As an arithmetic example, sampler RGB(255,510,−10) packs to
bytes(127,255,0); this is a derived reference, not a native runtime capture.

Copy property bytes+0x0c/+0x0d/+0x0e unchanged into stored bytes3/4/5,
then write the six-byte result through **0xa36e0 at0x31c410**, using the
current column base to form its index. All gap visits occur in ascending Z.
There is no second sea substitution on these generated type5/6 gap writes.

## 5. Finite examples of bounds and growth

Assume a valid XY column with old base10 and three blocks at Z10..12,
and a requested block whose nonzero type admits storage:

| Request | Result |
|---|---|
| Z11 | Replace that cell; base10/count3; other cells unchanged |
| Z15 | Base10/count6; preserve Z10..12; Z13/14 are raw zero blocks; Z15 is requested block |
| Z7 | Base7/count6; Z7 is requested block; generate buried colors at8/9; preserve Z10..12 |

For the same old column, a type0/flags0 request at Z13 is ignored, while
type0/flags0x02 is admitted there. For an empty column with base0, a
type0/flags0 request at Z5 is ignored without changing base. The same
request at Z−2 is admitted, sets base−2, substitutes canonical water and
creates one stored block; it does not fill Z−1 through the former base.

## 6. Restoration, live-receive and side-effect ownership

The archive restore in **0x2ae0c0** first copies the saved history list
into **Zone+0xd8**, then walks the saved entries in list order and calls
**0x31c1c0 at0x2ae2d8** with each entry's **six after-bytes+0x12**.
That replay does not append another history record per write or produce
World+0xc0 notices through the setter. It uses the passed constructed
zone directly rather than requiring the World lookup wrapper.

The received nonempty/type-or-flags path instead calls **0xa37d0** from
**0xa80fa**, and the Game caller separately appends history at0xa817f.
Its later notice-driven mesh invalidation remains the distinct0xaf110
phase. See [steam-block-change-lifecycle.md](steam-block-change-lifecycle.md).
The received empty/type0-flags0 branch still uses0x91160, which has the
different in-column-only guard; it must not be replaced by this growable
setter merely because both can choose sea water.

Neither0x31c1c0 nor the reached0xa36e0/0x6d030 block-write chain appends
history, queues block-change notices, updates the saved-state archive,
marks terrain/light/model meshes dirty, removes unsupported models,
rebuilds auxiliary lists or emits sound/particle/hit events. Their direct
state changes are the column base/vector and stored bytes above. There
is no direct CRT draw/seed operation, day/before-byte validation or
authority-byte check in this setter. Its callers own those separate actions.

No success-code protocol is established by this entry's residual register
return; the reached callers ignore it. Arbitrary aliasing input pointers,
nonfinite material fields, overflowing Z-loop ranges, unrepresentable/
unallocatable spans and the
full saved-state loading/eviction scheduler are outside this bounded
ordinary contract. This specifies the actual block operation without
inventing a generic live-edit notification policy for restoration.
