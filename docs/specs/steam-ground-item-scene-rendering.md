# Steam ordinary WORLD ground-item scene rendering

Independent native-only bounded audit of **0xd3170**, particularly
**0xd87e0–0xd913a**, and directly reached helpers. No engine, tests, fixtures,
extractor, SDK, reconstruction, owner assets/captures or game execution.
Record layout is drops-and-items§0.4. This specifies renderer consumption,
not which loading/generation phases populate ground vectors.

## 1. Zone/record order and admission

The later **draw-bit0x80-clear**, physical-local-player-centered **inclusive
±3-zone** traversal is [static-object admission §1](steam-static-object-scene-admission.md#1-containing-zone-traversal-is-separate-from-environment-models):
increasing X outside, increasing Y inside, loaded-zone lookup only. No mesh-group,
cached-center-distance or containing-zone box test is added. For each nonnull
zone, visit its ground vector **begin+0x30/end+0x38**, forward in0xe0-byte
steps, before that same zone's static-object vector+0x60. The ground index
increments for every physical record, including skipped entries.

For each ground record, require signed32 **+0xcc exactly0**; negative nonzero
also fails. Then **0x10a720** copies/adapts its original0xa0-byte item into a
temporary displayed item, using World+0x448 local player. Only original
category0x1e takes the class-dependent conversion; the other categories are
copied. See [player item adaptation](player-item-adaptation.md).
Select the resource through **0x173b10(World+0x70,displayed item)**, with
[item-model rules](item-models.md). A null resource skips the record.

Obtain **S=0x10a330(displayed item)**, then call **0xe08c0 at0xd887e**:
center is raw fixed ground **+0xa0/+0xa8/+0xb0**; radius is
**float32(max(resource voxel-X,voxel-Z)×S)**; distance limit is **Renderer+0x49c**.
Voxel-Y, half-height, hop and occupied-vertex bounds do not enter this sphere.
Use the existing fixed XYZ early-distance/all-six-plane contract; radius does
not extend the distance limit, and there is no+64. On success, copy the position
and calculate the positive-delay hop below.

Only afterward call **0x2b8140(World,original ground record)** at0xd88e5.
This is validity/personal ownership, **not terrain LOS**. Original category
and quantity+0xd4 must be nonzero; quantity need not be positive. Existing
carry/used-cell/special-item/reserved-owner exclusions are
[use prompt §2.4](use-prompt.md#24-ground-items). The adapted record does not
replace the original for this query. Resource GPU readiness is tested later
by the queue consumer.

## 2. Common anchor, scale, hop and tumble

Use the resource's full signed32 allocation dimensions **Vx,Vy,Vz** at
**+0x88/+0x8c/+0x90**, converted to float32. Item scale0x10a330 is specified
in creature-pose-items§8.2: category11/subtype36 gives0.05f; category11/subtype0
or32..35 gives0.15f; category12 gives0.04f; category24/subtype1 or2 and
category11/subtype42 give0.15f; otherwise **0.0714285746216774f**.

Only for float **delay+0xc8>0**, stage the hop in float32: subtract250 from
delay, divide by250, square, subtract that from1, multiply by5. Helper
**0x29be70** multiplies this block displacement by float65536 and truncates
to signed64. Add it to copied fixed Z in low64 arithmetic. Source
**0xd889e–0xd88d7**. Delays above500 produce a negative displacement;
there is no height clamp here. Nonpositive delays add no hop.

At **0xd88f2–0xd8992**, start identity, translate to that copied/hopped
position, then scale all three axes by S. X/Y first add Renderer+0x428/+0x430
to the fixed coordinates in low64 arithmetic, then convert by **CVTSI2SS**
and multiply by float1/65536. Hopped Z converts directly to float×1/65536;
there is no Z origin. See [WORLD coordinates](steam-world-gpu-coordinates.md).
Subsequent translations are local through the matrix's current basis,
in voxel units; they are not raw additions to its translation column.

Apply the style below, followed by this common tumble for **any finite nonzero
delay**, including negative: translate locally by(+0.5Vx,+0.5Vy,+0.5Vz),
rotate **X by delay degrees**, rotate **Y by float32(delay×0.699999988079071f)
degrees**, then translate by(−0.5Vx,−0.5Vy,−0.5Vz). Source
**0xd8e10–0xd8ecc**. Both rotations occur. Half-dimension products are
float32, not integer division. Rotation helpers multiply degrees by float
**0.01745329238474369**, then use float sine/cosine.

## 3. Reached style transforms and material pulse

Style initially equals adapted category. Adapted category0x16/subtype0x3f
uses style3; subtype0x40 uses style4. After that, **original category0x1e
forces style0x1e**, even for an adapted equipment resource. Let A be original
float ground angle+0xc0 and T current float World+0x440. After common
anchor/scale, apply the following local operations in order:

| Style | Operations |
| --- | --- |
| 3,4 | Rotate Z by A; translate(0,0,0.5Vx); rotate Y90 degrees; translate(−0.5Vx,−0.5Vy,−0.5Vz). |
| 12,13 | Same chain, but Z angle is float32(A+float32(T×0.2f)). Apply the RGBA pulse below. |
| 14 | Rotate Z by float32(A+float32(T×0.1f)); translate(0,0,float32(Vx×0.8500000238418579f)); rotate Y45 then X45 degrees; translate(−0.5Vx,−0.5Vy,−0.5Vz). Apply the pulse. |
| 0x17,0x18,0x1d,0x1e | Rotate Z by float32(A+float32(T×0.1f)); translate(0,0,float32(Vx×0.8500000238418579f)); translate(−0.5Vx,−0.5Vy,−0.5Vz). Apply the pulse. |
| Other, except the subtype below | Rotate Z by A; translate(−0.5Vx,−0.5Vy,0). |
| Other-style adapted category11/subtype14 | Replace the default chain: rotate Z by float32(T×0.019999999552965164f), without A; translate(0,0,Vz); rotate X45 then Y45 degrees; uniformly scale by5; translate(−0.5Vx,−0.5Vy,−0.5Vz). |

Dispatch **0xd8a1a–0xd8a6d**; branches **0xd8a6f,0xd8ad0,0xd8bca,
0xd8c91,0xd8d32–0xd8e0b**. Default centering is horizontal/bottom-centered;
the tilted/floating branches use all three half-dimensions. The subtype's
extra scale5 does not replace S or alter the preceding sphere.

The pulse multiplies **all four material components**. Float32-add T+T and
multiply by0.20000000298023224f; widen to double, multiply by doubleπ
**0x42e758**, divide by double180 **0x42e778**, narrow to float, then cosf.
Multiply by0.5f, add0.5f, square, add1.0f, rounding each float operation.
Multiply RGBA by this scalar. Its ordinary range is1..2. The cosine changes
material, **not vertical position or pixel Opacity**; no periodic Z bob is
produced by these branches.

## 4. Material and selected-ground addition

Before the style pulse, **0xd8997–0xd89e5** queries **0xdab20 channel0** at
the **raw ground XYZ**, without hop/height offset. Its effective-block,
coordinate and publication rules remain
[block-light publication §3](steam-block-light-publication.md#3-exact-channel0-query).
Convert the integer result to float and divide by float255; construct
material **(1,1,1,factor)**. This path does **not** multiply in Renderer
frame material+0x530..+0x53c. For adapted category11/subtype14 or19,
or adapted category19, overwrite material alpha with1 before any pulse.

After the style/tumble, copy the material, then compare canonical
**(zoneX,zoneY,physical index)** against the int3 supplied as0xd3170 argument2.
On the ordinary Game route this is **Game+0x1e70** at **0xad8dd/0xad92e**,
forwarded as0xe2e00 argument20 and through0xe3bd1. Exact match adds
**(0.25,0.25,0.25,0.25)** at **0xd8f04–0xd8f31**, after the pulse.
Alpha is included. There is no preparation clamp to0..1; this material
alpha is separate from pixel Opacity.

## 5. Queued model controls, spirits and ordering

**0xce300 at0xd8f49** samples the **raw un-hopped ground position** with
**L=Renderer+0x49c**, without+64 or half-height. Its scalar formula is WORLD
parity§9.6, using these inputs. No second hop-position sphere/range test follows.
At **0xd8fa6**, submit the resource, completed matrix and material through
**0xee2f0**, explicitly normal winding, context **Renderer+0x138**:

| Control | Value |
| --- | --- |
| Opacity | float32(fade×0x108d20(adapted item)); category11/subtype37 or39 returns0.6000000238418579, otherwise1. |
| Glow |0 |
| Shininess |0x10a420(adapted item), GUI item icons§5; not Glow |
| White |0 |
| DepthOffset |0 |

Finite opacity classification, material upload and consumer readiness/pass
states remain [WORLD controls §2.1](steam-world-pass-controls.md#21-pose-material-delivery-readiness-and-explicit-winding).
There is no automatic determinant-based opposite family here.

For **adapted category3**, immediately afterward lookup catalog0xa46 and
call **0xeec50 at0xd9025** with the adapted item, same matrix/final material
and same five controls. Its preceding extra scalar argument is0. Attribute
iteration and signed-voxel placement remain
[pose items §9.1](creature-pose-items.md#91-the-spirit-cubes-0xeec50).
Original-category0x1e style override does not prevent this category3 test.

These are enqueue orders. For each zone its ground base/spirit submissions
precede its static objects, then the next zone. Actual opaque drawing groups
by resource pointer; faded vectors have their own consumption. The traversal
is not an immediate all-ground-before-all-static GPU draw sequence.

## 6. Additional effect producer and explicit limits

After base/spirits, **0xd902a–0xd912b** appends a separate0x48-byte descriptor
through0xe2800 to **Renderer+0xc0** when: original delay equals0; adapted
category is3..9 or0x1a (0x9ae90); World+0x210 is nonnull; original owner ID
+0xd8 equals the local player's+0x968. There is no successful-base-draw or
positive-fade condition here. **0xcb910** initializes:

| Offset | Value |
| --- | --- |
| +0/+8/+0x10 | Raw fixed ground XYZ |
| +0x18..+0x24 |0x10a600(adapted item), relative to local-player region+0x150 |
| +0x28..+0x34 | Another such rarity-color RGBA multiplied by0.8999999761581421f through0xc7c70 |
| +0x38 | Low32 of signed64 truncation of float World+0x440 |
| +0x3c | float32(S×10) |
| +0x40 | float0.10000000149011612 |
| +0x44 | signed32 kind8 |

This identifies the producer/data, not an inferred visual name or its
downstream effect draw. This is the renderer list, not World+0xc0 block
changes. Alternate scene/GUI callers, every resource factory, invalid assets,
nonfinite/overflowing arithmetic, concurrent mutations and loaded-but-unmeshed
zone content production remain outside this bounded renderer contract.
