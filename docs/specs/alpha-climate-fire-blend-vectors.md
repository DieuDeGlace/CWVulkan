# Alpha positive-fire climate arithmetic fixtures

These two independent arithmetic fixtures extend
[alpha-climate-blend-vectors.md](alpha-climate-blend-vectors.md) without
changing it. Sources are original Alpha client code/constants and the
independent region/random contracts. No engine, tests, exporter, sister
implementation, SDK implementation, owner data/captures, benchmarks or
packages were inspected, and no game was executed. They are **not native
runtime captures or a whole-world parity claim**.

Client image base is0x400000. The direct queries remain warmth
VA0x5c4dd0 / RVA0x1c4dd0, moisture0x5c4800 /0x1c4800, sea share
0x5f0720 /0x1f0720 and fire share0x5ef040 /0x1ef040. Input coordinates
below are signed32 **integer blocks**; outputs are float32. Hex values
are IEEE754 numeric words, not memory-byte-order strings.

## 1. Seed and complete immutable window

Use seed123, its original base offset **A=440** at World+0x800188, and
home float32 coordinates **(8396928.0,8396928.0)** at
World+0x8000f0/+0x8000f4. Both points are in region cell(511,515).
Their undisplaced query window is therefore **X510..512, Y514..516**.
All nine pointers in that window of World+0x4000bc must be nonnull and
unchanged throughout each query. The grid index is1024×X+Y.

The records below are the seed-derived final records, after the
constructor's elevation/sea handling. Point X/Y are signed32 at+0/+4,
fire is the byte at+0x08, moisture/warmth are float32 at+0x0c/+0x10,
and elevation is signed32 at+0x18.

| Cell(X,Y) | Per-cell CRT seed | Point X | Point Y | Warmth word | Moisture word | Elevation | Fire |
|---|---|---|---|---|---|---|---|
|(510,514)|532400|8371200|8424448|0x3f680c69|0x3f7bfe5e|−100|0|
|(510,515)|533424|8369152|8446976|0x3edc601f|0x3efd1a61|77|0|
|(510,516)|534448|8358912|8459264|0x3f0856aa|0x3ee61a33|42|0|
|(511,514)|532401|8381440|8430592|0x3f0bdde5|0x3f2ba3f1|51|0|
|(511,515)|533425|8383488|8449024|0x3f69dd6e|0x3ef6c1ee|36|1|
|(511,516)|534449|8379392|8469504|0x3f7b97f7|0x3f7cb2c6|−100|0|
|(512,514)|532402|8395776|8434688|0x3d621b5e|0x3dba9642|57|0|
|(512,515)|533426|8403968|8440832|0x3db4bb03|0x3f6aa908|48|0|
|(512,516)|534450|8397824|8465408|0x3f6bd9a4|0x3f71327c|46|0|

The separate region-object pointer at **World+0xbc, region(511,515),
is null**. This pins moisture's applicable location lookup to null,
independently of the nine present region-cell pointers. No zones,
column caches, meshes or generated location records are prerequisites
for these direct-query arithmetic fixtures. Unlisted cell bytes do not
participate in the four blends.

## 2. Evidence that the fire record is naturally generated

Original cell constructor **VA0x5d7a70 / RVA0x1d7a70** reseeds for each
cell with X+0x108a+1024×Y+3×A. The table was derived from this seed
arithmetic, original CRT recurrence, draw order and float stages, not
chosen by assigning synthetic climates to points.

For cell(511,515), the seed is533425. Its first eight draws are:
**5276,26668,20357,29070,18601,31584,11286,11415**. Their roles are:

- Draw1 is even, entering the temperate branch.
- Draws2/3 provide its initial moisture/warmth.
- Draw4 is divisible by10, setting the fire byte to1.
- Draw5 replaces warmth using the native separate float32 multiply by
  float0.2, division by32767 and addition of float0.8. It produces
  **0x3f69dd6e**.
- Draw6 replaces moisture using float32 multiplication by0.5 and
  division by32767. It produces **0x3ef6c1ee**.
- Draws7/8 provide point Y then X. The native2048-block snap with
  1024-block offset gives **(8383488,8449024)**.

The relevant scalar words are float0.2=0x3e4ccccd at VA0x6fcdbc /
RVA0x2fcdbc, float0.8=0x3f4ccccd at0x6fcdc8 /0x2fcdc8, float0.5=
0x3f000000 at0x745da0 /0x345da0, and float32767=0x46fffe00 at
0x6fd338 /0x2fd338. The cell's integer elevation is36, so it does
**not** enter the sea branch that would clear fire and redraw climate.

All other table entries were derived through the same native constructor
with their own listed seeds. Their initial fire/climate branches, point
draws and any post-elevation sea redraw remain in the native order.

## 3. Stored angle constant and bounded nonlattice derivation

The verified angle multiplier at **VA0x702b48 / RVA0x302b48** is the
binary64 word **0x400921fb5a7ed197**, conventionally rendered3.1415927.
It exceeds mathematical pi by approximately4.641020666628484e−8.
Mathematical fifth-angle identities were used only as centers for
enclosures, not substituted for this actual multiplier.

The enclosure method is the explicit one in
[alpha-climate-blend-vectors.md](alpha-climate-blend-vectors.md) §2:
compare each actual binary64 fraction/product with the mathematical
fifth angle, bound cosine's change using absolute derivative≤1, and
multiply half that angular error by the actual native hash-edge
amplitudes. An additional1e−12 allowance covers ordinary binary64
evaluation under the independent noise contract. No cosine or noise
library was invoked to obtain the fixtures.

For this nine-cell window's elevation inputs, the largest calculated
noise-error bound is below4.243e−8. A conservative **4.3e−8** allowance
propagated through the native float32 elevation operations leaves the
same signed integers at both ends, in table scan order:
**−20,77,42,51,36,−22,57,48,46** before sea conversion. The negative
entries take the native sea/clamp/redraw branch and become−100; the
positive ones retain their integer elevations. Thus the record table,
including the positive fire byte, does not depend on resolving an exact
nonlattice elevation-noise float word.

For the query displacements, the needed argument/edge bounds are below.
The second noise coordinate is an exact integer, so its interpolation
weight is0 and the horizontal first-edge amplitude suffices. The
denominator of each rational amplitude is exactly2^30, from the native
masked hash values.

| Nominal noise arguments | Horizontal hash-edge amplitude | Angle-error upper bound | Noise-error upper bound |
|---|---|---|---|
|(4224.6,3423)|360501652/1073741824|2.784728e−8|4.676e−9|
|(4191.8,23421)|139637944/1073741824|3.712875e−8|2.416e−9|
|(4224,3423)|integer lattice|0|0 from the angle perturbation|
|(4188.4,23421)|582564138/1073741824|1.856295e−8|5.037e−9|

These are upward-rounded bounds; the nonlattice noise bounds include
the arithmetic allowance. They assume the ordinary double cosine
semantics and nearest-even evaluation of the independent native noise
contract, not an arbitrary replacement runtime or modified control mode.
They are not a measured library-error guarantee.

## 4. Coordinate conversion and resulting sample points

Each query derives its three-by-three window from the **undisplaced**
integer inputs. Displacement X uses scaled input Y with noise's second
coordinate3423; displacement Y uses scaled X with second coordinate23421.
The input scale is binary64 **0x3f40624dd2f1a9fc** at VA0x702b38 /
RVA0x302b38. Each float32 noise result is separately multiplied by
float3, then float256, with float32 rounding at each multiplication.
The direct climate routines inline the displacement arithmetic described
by VA0x5eefa0 / RVA0x1eefa0.

The result is added to the float32 input position **in float32**, before
signed32 truncation. Here the input X coordinates have float32 spacing
**0.5 block**, while input Y has spacing **1 block**. This differs from
the one-block X spacing of the earlier home/east fixtures.

Propagating the §3 bounds gives these inclusive component enclosures:

| Input(X,Y) | Displacement X, blocks | Displacement Y, blocks |
|---|---|---|
|(8383600,8449200)|[−671.00775146484375,−671.00775146484375]|[−280.325439453125,−280.325439453125]|
|(8376800,8448000)|[−502.24212646484375,−502.24212646484375]|[142.335693359375,142.3357086181640625]|

The native float32 addition then gives the same position at both ends
of each enclosure:

| Input(X,Y) | Post-add float32 X/Y words | Integer displaced point |
|---|---|---|
|(8383600,8449200)|0x4affd3a2 /0x4b00eb98|(8382929,8448920)|
|(8376800,8448000)|0x4aff9fd4 /0x4b00e88e|(8376298,8448142)|

The mixed point's intermediate Y-displacement word is not asserted
unique. Its final integer sample point is unique, which is sufficient
for the climate references below.

Distance and weighting stages are unchanged: fixed signed64 difference,
float32 conversion and exact2^-16 scaling, separate float32 squares,
Y-square plus X-square in float32, signed32 truncation, nearest integer
distance, then float32 weight construction using **0x350637bd** at
VA0x71dcdc / RVA0x31dcdc. Numerators/denominator accumulate in ascending
X outer/Y inner order and divide in float32. The full native sites and
field/rounding definitions are in the earlier climate note §3.

## 5. Two literal four-field references

Under exactly the prerequisites above:

| Integer input(X,Y) | Warmth word | Moisture word | Sea-share word | Fire-share word |
|---|---|---|---|---|
|(8383600,8449200)|0x3f69dd6e|0x3ef6c1ee|0x00000000|0x3f800000|
|(8376800,8448000)|0x3f2b41cc|0x3ef9f842|0x00000000|0x3efcd0ac|

The first point is a pure positive-fire reference. Only cell(511,515)
has nonzero weight, equal to1. Its distance² is323297. The next nearest
cell's distance² is193584864, so every other weight is0. Warmth/moisture
are approximately0.9135349988937378/0.4819483160972595; fire share is1.

The second point is an **actually mixed fire/nonfire land boundary**
in this native-derived seed123 window, not an average of invented
climates. Its nonzero contributors are:

| Cell | Fire byte | Signed32 distance² | Difference from minimum | Float32 weight word |
|---|---|---|---|---|
|(510,515)|0|52424872|0|0x3f800000|
|(511,515)|1|52474024|49152|0x3f79b563|

The scaled difference is **0x3cc9539c** and the fire-cell weight is
approximately0.9754239916801453. The denominator is **0x3ffcdab2**,
approximately1.97542405128479. Pre-division numerator words are:
warmth **0x3fa92712**, moisture **0x3f76e5ec**, sea **0x00000000**, and
fire **0x3f79b563**. The returned warmth/moisture/fire are approximately
**0.6689727306365967 /0.4882221817970276 /0.4937795400619507**.

Both nonzero-weight contributors are land cells. The window does contain
negative-elevation cells, but their weights are0. Thus sea returns0
after its admitted indicator blend; this is not an assertion that its
pre-scan found no sea cell. Likewise fire's positive result uses the
flag-equals1 indicator blend, not a warmth threshold or an inferred biome.

## 6. Oasis and scope boundaries

Successful moisture performs its location lookup before testing the
dryness threshold, as established by native VA0x5c4ceb–0x5c4d10 /
RVA0x1c4ceb–0x1c4d10 and the earlier climate note §5. The relevant
region-object lookup here is region(511,515), whose pointer is pinned
null in §1. The returned location is therefore null. Both base moisture
results are also above0.2, so no oasis adjustment changes either word.
These numeric results do not imply that the location lookup was skipped.

Only these two direct original-client query fixtures and their explicit
seed/home/cell/region inputs are established. Missing-cell fallback is
the separate earlier contract, not a partially renormalized variant of
this table. No owner capture, whole-world preparation, server precision,
positive oasis, concurrent mutation, runtime-cosine implementation audit
or exact ambiguous noise word is claimed. Every literal climate word
above is an independently derived conditional arithmetic reference.
