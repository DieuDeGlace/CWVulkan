# Steam ordinary WORLD pose material and pixel opacity preparation

Independent native-only bounded audit of **0xd3170**, immediately before
its ordinary pose call **0x17ab0 at0xd67b2**. No engine, tests, fixtures,
extractor, SDK, reconstruction, owner assets/captures or game execution.
Admission and current-scene inputs remain
[pose invocation §3](steam-world-pose-invocation.md#3-exact-normal-pose-admission-and-sphere-inputs).
This is separate from the character-card route's fixed white input.

## 1. Physical-position channel0 sample

At **0xd6593–0xd65bd**, use the creature's **physical fixed** X/Y at
**+0x10/+0x18** and physical Z+0x20 minus a fixed offset. The offset source
is float32 **6553.60009765625** at **0x43efd0**. Native **CVTTSS2SI to signed64**
at0xd6593 truncates it to **6553 fixed units**, then0xd65a1 subtracts that
integer from Z with low64 arithmetic. It is not a rounded6554-unit offset,
a drawn-position sample or an origin-relative GPU coordinate.

Call **0xdab20 at0xd65bd** with the current renderer World (Renderer+8)
and **channel0**. Its coordinate conversion is the existing
[block-light publication §3](steam-block-light-publication.md#3-exact-channel0-query):
each fixed coordinate is divided by65536 toward zero and narrowed to signed32;
for a negative original coordinate, decrement that narrowed result if its
reconstructed fixed value differs from the original. The decrement wraps
in32 bits. There is no16-block-cell conversion here.

This binding uses the effective-block query, not a direct raw mesh-light,
red-byte or flags-byte read. Its established missing-zone result is255;
types10/12/19 are skipped upward; final type0/2 uses max(red,5,auxiliary
byte3), other final types use auxiliary byte3. All other column/substitution
and publication/lifetime details remain in that accessor contract.

Convert the returned integer to float32 and **divide by float255** at
**0xd65c2–0xd65c9**. For race **0x114**, replace this factor with float1
at **0xd65d1–0xd65dc**, after the query. This override does not skip the
query itself.

## 2. Frame material and ordered creature tint factors

At **0xd65e0–0xd6624**, initialize the local four-float pose material:

- RGB are copied from **Renderer+0x530/+0x534/+0x538**.
- Alpha is **float32(sample factor × Renderer+0x53c)**.

Thus the renderer's frame material is already included **before pose and
enqueue**. This reads the renderer fields, not the preceding draw's shader
material cache. The block-light factor enters material alpha, not RGB or
the separate pixel Opacity argument.

Next visit the creature's **+0x980 buff list forward**. At each node, its
type byte at node+0x10 selects a component-by-component float32 multiplier:

| Type byte | RGBA multiplier | Native float4 source |
| --- | --- | --- |
| 1,2,0x12 | (1,0.5,0.5,1) | 0x4380f0 |
| 4 | (0.5,1.5,0.5,1) | 0x43f120 |
| 0x0f | (0.2f,0.1f,0.3f,1) | 0x43f0b0 |
| 0x1d | (0.5,0.5,1.5,1) | 0x43f190 |
| Other | No change in this loop | — |

Every matching occurrence multiplies the result left by preceding entries;
there is no one-per-type reduction here. For the0x0f row the stored words
are **0x3e4ccccd,0x3dcccccd,0x3e99999a,0x3f800000**. The multiplier alpha
is1 for all rows. This reached loop does not add a buff-clock or source-ID
test before applying these factors.

After that traversal, if creature **+0x80 bit0x04000000** is set, multiply
all four current components once more by **(1,0.5,0.5,1)**, using the same
float4 at0x4380f0. This is in addition to any matching list entries. It is
not a replacement white material or a source-sprite recoloring operation.

The resulting local RGBA pointer is supplied as **pose argument5** at
**0xd6792–0xd6799**, before the call0xd67b2. Per-part color combinations
remain the pose contract, including hair's saved color multiplication.
The ensuing descriptor retains the already combined material through
[WORLD controls §2.1](steam-world-pass-controls.md#21-pose-material-delivery-readiness-and-explicit-winding).

## 3. Separate Opacity argument6

Prepare a visibility factor by float32 multiplication then subtraction:
**1−float32(creature float+0x18c ×0.8f)**, where0.8f is word0x3f4ccccd.
If **+0x80 bit0x00100000** is set **or** lookup0x5e3a0 finds any buff type
**0x13**, multiply that factor once by float0.5. Both conditions together
do not cause two halvings. The lookup finds a matching list record without
an additional duration test in this helper. Then, for race **0x11d**, multiply
by float **0.75** (constant0x42e710, word0x3f400000).

At **0xd6737–0xd6746**, obtain **0xce300** fade with range
**Renderer+0x4a0**, without +64, sampled at the raw **drawn fixed tuple
Creature+0xb10/+0xb18/+0xb20**. Its pointer is established at0xd5d09.
This is not the earlier sphere center with its step-offset or special-race
height adjustment. The scalar fade arithmetic is
[WORLD parity §9.6](worldgen-parity-gaps.md#96-ordinary-model-fade-uses-its-raised-center-without-a64-block-extension);
the position/range inputs for this actor call are the ones given here.

Multiply fade by the visibility factor in float32 at **0xd674b** and submit
that scalar as **pose argument6** at **0xd678c**. There is no additional
clamp in this preparation. Subsequent part-specific opacity and the enqueue's
positive/opaque/faded classification remain their existing contracts.
This scalar is not substituted for argument5's light-bearing material alpha.

## 4. Limits

This contract covers reached ordinary finite inputs after the existing pose
admission. It does not define new meanings or producers for the cited state
bits/+0x18c, change buff expiry ownership, or audit alternate preview/map/effect
callers. Frame-material production, shader composition, accessor publication
and per-part modifications remain their separate specifications.
