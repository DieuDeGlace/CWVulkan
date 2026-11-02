# Steam owned-ground-item repeated cube consumer

Independent native-only bounded audit of **0xe6270 →0xdad40** and the
ordinary scene caller0xe2e00. No engine, tests, fixtures, extractor, SDK,
reconstruction, owner assets/captures or game execution. Producer fields/gates
are [ground-item rendering §6](steam-ground-item-scene-rendering.md#6-additional-effect-producer-and-explicit-limits).

## 1. Superseding label, list lifetime and pass order

**Renderer+0xc0 record+0x44 is a signed32 repetition count, not an effect-kind
selector.** The owned-ground producer writes8, and0xdad40 performs eight
iterations. This explicitly supersedes the historical "kind8" label in the
earlier producer summary. List membership selects this consumer family.
The0x48-byte record is neither a World+0xc0 block-change notice nor an ordinary
burst/live-particle record.

At the start of ordinary **0xe2e00**, the old Renderer+0xc0 linked nodes are
freed, the sentinel's next/previous links reset and **Renderer+0xc8 count=0**,
before0xd3170 collects the new scene.0xe2800 appends the new record at the tail.
The consumer walks this list forward from sentinel.next in append order;
it does not sort, age, retire or write back its records. No separate live
particle or persistent per-cube age is created by this path.

The normal caller runs second-context faded models at **0xe3c86**, then
**0xe6270 at0xe3c97**, optional ordinary particles at **0xe3ca7**, then water
at **0xe3cd2** and later ordinary-context faded models.0xe6270 itself is not
inside the ordinary-particle enable branch. It visits its preceding Renderer
+0xb0 list, then Renderer+0xc0 through **0xdad40 at0xe63a3**, then the other
lists. Only the +0xc0 consumer is specified here. Its draws are immediate
WORLD draws at this cut, not additions to the model opacity queues.

## 2. Inputs and owner resource

The forwarded fields are:

| Record offset | Consumer meaning | Owned-ground value |
| --- | --- | --- |
| +0/+8/+0x10 | signed64 fixed base XYZ | Original ground position |
| +0x18..+0x24 | Four-float color C1 | Regional rarity color |
| +0x28..+0x34 | Four-float color C2 | Another such color multiplied by0.9f |
| +0x38 | **Unsigned32 clock U0** | Reinterpreted low32 of the producer's signed64 truncation of float World+0x440 |
| +0x3c | Float orbit radius R | Item scale×10 |
| +0x40 | Float base geometry scale W |0.10000000149011612 |
| +0x44 | Signed32 count N |8 |

For N<=0,0xdad40 does no iterations. The positive-count loop uses catalog
**0xa46**, the owner file **cube.cub**, through World+0x70's resource vector
begin+0x5230. The verified filename/string/load call and CUB extraction layout
are [particle bursts §7.1](steam-particle-bursts.md). This is the same owner
resource binding, not the burst-particle consumer or a generated replacement
cube primitive. Its actual full allocation dimensions are read from resource
+0x88/+0x8c/+0x90 for centering; do not substitute occupied-vertex bounds.

At **0xe62d4**, the outer batch calls0x16eba0 on this resource. That helper
requires nonnull wrappers+0x70/+0x78 and invokes their+8 methods, but the batch
**does not branch on its Boolean result**. Each iteration later calls
**0x16ecc0**, which checks only that those two wrappers are nonnull before
forwarding draw counts+0x80/+0x84 to the backend. It does not repeat those
readiness-method calls. Resource/header presence is assumed by the intervening
dimension reads; no missing-resource fallback geometry is defined here.
The visible outcome for a nonnull but unsuccessful buffer-method path is
not established by this bounded trace.

## 3. Three clock phases and per-iteration math

All arithmetic below is float32 unless an explicit double or integer stage
is stated. The producer's clock is interpreted as **uint32**, including its
wrap behavior. Let the iteration index i run0..7. Work with a local copy U,
initially U0, and after each draw add **443** with unsigned32 wrap. This local
addition does not change record+0x38 or World+0x440.

Two values are computed **once from U0 before the loop**:

- Scale-pulse phase: convert U0 modulo500 to float and divide by500f. Add
  that result to itself in float, widen to double, multiply by doubleπ
  at0x42e758, and retain this double phase P.
- Orbit phase: convert U0 modulo10000 to float and multiply by stored
  **0.00009999999747378752f**, constant0x4ac954. Retain the float result B.

Each iteration computes its separate rising/color phase **a** by converting
**U modulo2000** to float and dividing by2000f. Compute **q=(a−0.5f)+(a−0.5f)**
with float subtractions/addition. For the scale pulse, form double
**P+double(13×i)**, narrow to float and take cosf. The integer13×i is added
as **radians**, without a degree conversion. Compute the geometry scale by
float multiply/add **cosine×0.5f+1**, multiply by W, then multiply by
**1−q×q**. There is no additional clamp.

For the orbit angle, divide float(i) by10f, add B in float, add that result
to itself in float, widen to double, multiply by doubleπ, then narrow to
float for sinf/cosf. Thus the count8 does not choose eight equally spaced
45-degree positions: its index contribution is i/10 turns.

Interpolate **all four** material components in this order: compute C2×a,
compute C1×(1−a), then form **C1 term +C2 term** componentwise. This is
not pixel Opacity and is not reduced to a different real-equivalent formula
before float rounding. No frame-material or block-light sample is inserted.
The consumer submits this material through renderer virtual+0x98 before
constructing/submitting that iteration's WORLD matrix.

## 4. Position, rotations, uniform scale and centering

For base X/Y, add Renderer+0x428/+0x430 to the respective signed64 fixed
coordinate in low64 arithmetic, then convert directly to float and multiply
by float1/65536. Base Z converts directly to float×1/65536, with no Z origin.
Add **cos(orbit angle)×R** to X and **sin(orbit angle)×R** to Y. For the rise,
compute a×R in float and add that product to itself; add the result to Z.
These positions are current-scene WORLD coordinates under the existing
[separate-matrix coordinate contract](steam-world-gpu-coordinates.md).

The finite affine transform is: translate to this orbit/rise position,
rotate locally about **X by a×40 degrees**, then **Y by a×30 degrees**, then
**Z by a×10 degrees**, uniformly scale by the geometry scale from§3, then
locally translate by **(−Vx/2,−Vy/2,−Vz/2)**. Each degree product is float,
then multiplied by stored float **0.01745329238474369** before float
sine/cosine. The three full header dimensions convert to float before
multiplication by−0.5f. The final X translation accumulates its centering
terms in **Y, X, Z** order after the orbit-position term; Y and Z accumulate
their centering terms in **X, Y, Z** order. These are float matrix operations,
not a double camera/world transform.

The helper submits WORLD through virtual+0x50 and calls0x16ecc0, then advances
the local clock/index. No frustum, distance, point-table, terrain LOS, opacity,
zero-scale or lifetime test intervenes. For example U0=0,i=0 gives a=0 and
zero geometry scale, yet still reaches the matrix submission/draw attempt.
Ordinary producer admission at the raw ground position remains separate;
the eight displaced cubes are not individually admitted by that sphere.

## 5. Shader, pipeline and inherited state

At entry0xe6270 selects **state0**, explicitly sets **White0** through
virtual+0xb8 and **Glow1** through+0x78. State0 uses the ordinary WORLD shader
pair, **no blending**, enabled **LESS_EQUAL depth testing and depth writes**,
native normal winding (**CullMode2=FRONT, FrontCounterClockwise0**), depth
clipping on and scissoring off. No private depth clear, viewport or target is
introduced by this batch/consumer. See
[native pass-family state contract](steam-retained-item-and-map-pass-family.md#4-native-consumer-conditions-and-pipeline-states).

The preceding +0xb0 helper0xdc610 changes WORLD/material, not these controls.
The +0xc0 helper changes **only material and WORLD** among the shader inputs.
In particular it does **not reset or choose Opacity, Shininess, DepthOffset,
point-light tables, directional/ambient palette, view/projection, sky/fog or
SunPos**. These retain the state active before this batch; do not presume
zero points or default scalar controls. There is no transformed-center
point-table lookup for these cubes. Their explicit Glow/White behavior and
remaining WORLD fog/ripple/pixel-opacity stages are the existing
[shader-output contract](steam-world-shader-output.md#2-white-glow-and-pixel-opacity-are-distinct-stages).

No per-cube shader-state restoration follows. Subsequent consumers have
their own contracts. This completes the reached count8 path for ordinary
valid resources and finite representable inputs; it does not audit other
list families, every producer/count, failed GPU resources or invalid assets.
