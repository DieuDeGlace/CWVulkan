# Alpha below-column terrain colour

Independent native-only clarification of `alpha-rendering.md` §9.5, restricted
to the ordinary terrain mesher **VA0x49d910 / RVA0x9d910**. The client image
base is 0x400000. No engine, tests, exporter, owner data/captures, reconstruction
or game execution was used.

## 1. Reference source and branch

After admitting a non-air effective block for ordinary face processing, the
mesher checks whether its current signed32 Z is **strictly below the column's
signed32 bottom at +0x10**, at **VA0x49e4e5–0x49e4fc**. The reference is the
column's **first raw stored block, index 0**, read directly through block-array
pointer **column+0x18**, at **VA0x49e5b4** and **0x49e640**. Since stored blocks
start at bottom, that reference block's absolute Z is **column.bottom**.

It is not the last stored block, the first solid found by a search, a neighbour
column, the block at surface-minus-eight, or a sample taken at current Z. Column
field +0x14 is not the reference selector in this branch.

The reference type test uses its raw fourth byte masked with **0x1f**, at
**VA0x49e5b7–0x49e5c9**. A reference of type **0 or 2** selects a pure procedural
rock sample, with no weighted reference contribution. High flags do not change
that type test. If signed32 count **column+0x1c** is nonpositive, the reached
fallback is RGB **255,255,255**, type0, so it also takes the pure-sample branch.
A positive count uses the stored-array pointer directly; malformed missing
storage is not given an invented recovery rule.

## 2. Rock sample and exact float staging

The sampled function is **VA0x5f9620 / RVA0x1f9620**, exactly the **rock Rk**
routine in `alpha-worldgen-terrain.md` §6.6. It returns three float32 color
components in byte-color units, not an already packed three-byte record or
normalized shader RGB. The mesher samples for its current terrain coordinate;
it does not replace this with the final ground-height or cliff-color routine.

In the below-bottom branch, **VA0x49e50a–0x49e529**, the mesher subtracts current
Z from bottom in signed32 arithmetic, converts that integer difference to
float32, and multiplies by float32 **0.1**, word **0x3dcccccd** at VA0x6fcb7c.
If the result is greater than float32 **1**, word **0x3f800000** at VA0x745dc0,
it replaces it by 1. Call this factor k. There is no separate lower clamp in
the reached branch; ordinary below-bottom inputs already have positive depth.

For a reference whose type is neither 0 nor 2, the ordered color stages are:

1. Convert each reference RGB byte as an unsigned integer to float32. Compute
   **1−k in float32**, multiply each converted reference component by that
   factor in float32, then divide each weighted component by float32 **255**,
   word **0x437f0000** at VA0x6fce64. These stages occupy
   **VA0x49e643–0x49e75a**.
2. Invoke Rk at **VA0x49e763 / RVA0x9e763**. Multiply each returned float32
   component by k in float32, then divide each weighted component by float32
   255. These stages occupy **VA0x49e768–0x49e828**.
3. Add the normalized rock contribution to the normalized reference
   contribution separately for R/G/B, in float32, at
   **VA0x49e84a–0x49e899**.

The two contributions are therefore weighted and divided separately before
their final addition. A mathematically equivalent blend followed by one
division, or normalization before the multiplications, does not describe the
checked precision/order. There is no byte truncation or RGB packing between
these stages, and no additional clamp here.

For the raw-air/raw-water/empty-storage-reference branch, Rk is called at
**VA0x49e8d4 / RVA0x9e8d4**. Its three returned float32 components are divided
directly by float32 255 at **VA0x49e8ee–0x49e931**, with no multiplication by k
or the reference bytes. The color result joins ordinary processing with alpha
**1**. It remains floating color input to the later meshing stages; final
vertex-byte packing is the separate `alpha-rendering.md` §9.6 contract.

## 3. Limits

This is a content-color replacement in the mesher. It does not extend the
column's physical storage, write implicit blocks, or supply a light value.
The existing Rk, corner shading/lighting and final packing contracts remain
separate. No new malformed-pointer, signed-overflow recovery, nonfinite-color,
server-equivalence or unrelated block-type behavior is established here.
