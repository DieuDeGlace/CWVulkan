# Alpha raw base-height arithmetic references

Independent native-only supplement to alpha-worldgen-terrain.md §4 and
the independent region/random contracts. These are **conditional arithmetic
fixtures, not runtime captures**. Original Alpha client code/constants and
independent prose were the only evidence. No engine, tests, exporter,
sister implementation, SDK implementation, owner output/captures,
benchmarks or packages were inspected. No game was executed.

The result is raw **H in blocks**, from **VA0x5c5e20 / RVA0x1c5e20**.
It is not a generated surface, relief, cliff, cave, structure or final
block-column height. Image base is0x400000. Hex float words below are
IEEE754 numeric words. References assume ordinary nearest-even
binary32/binary64 evaluation under the independent native noise contract.

## 1. Exact query state and ABI boundary

The World is the receiver, with signed32 integer block X/Y and a null
optional Zone selector as the three stack arguments. The reached return
loads the stored float32 result into the floating return register at
VA0x5c711e / RVA0x1c711e and cleans twelve stack bytes at0x5c712f /
0x1c712f. No double-precision result hidden behind the float return is
used in these references.

Pin seed123, base offset A=440 at World+0x800188, and original home
float32 coordinates(8396928.0,8396928.0) at World+0x8000f0/+0x8000f4.
Both query points are in region cell(512,512). Its **region-object pointer
at World+0xbc is null**, so the applicable location and zone lookups
return null. Together with the null Zone selector, this also excludes a
cached column-fire value in roughness: the direct cell-based fire query
is reached and returns0 from this window's flags.

The same nine immutable cell pointers as the earlier climate fixture
are present in World+0x4000bc, indexed by1024×X+Y. The fields needed here
are point signed32 X/Y at+0/+4, elevation signed32 at+0x18, and fire
byte at+0x08. Warmth/moisture words do not enter these reached H branches.

| Cell(X,Y) | Point X | Point Y | Elevation | Fire |
|---|---|---|---|---|
|(511,511)|8377344|8381440|56|0|
|(511,512)|8385536|8391680|−100|0|
|(511,513)|8375296|8420352|51|0|
|(512,511)|8401920|8381440|9|0|
|(512,512)|8397824|8397824|74|0|
|(512,513)|8403968|8412160|−100|0|
|(513,511)|8418304|8385536|−100|0|
|(513,512)|8408064|8395776|−100|0|
|(513,513)|8416256|8408064|117|0|

The first point's link helper can additionally look at cell **(510,512)**,
outside that nine-cell window. Pin that pointer **null**. This is an
explicit partial-world prerequisite for this arithmetic fixture, not a
claim that the cell is absent in a normally fully prepared region. No
other out-of-window cell is a required link neighbor for these two points.
Other unvisited world data is not assigned invented defaults.

The nine records and their native seed provenance are established in
[alpha-climate-blend-vectors.md](alpha-climate-blend-vectors.md) §§1–2.
This note does not regenerate a complete region or assume a full-world
streaming/preparation state from the seed alone.

## 2. Required seed-offset fields

These signed32 offsets are independently obtained from original world
initialization VA0x5a52e0 / RVA0x1a52e0 and the seed123 CRT draw order in
alpha-worldgen-random.md §2. The first draw gives A=440. Draws2–5 give
the roughness fields, draws6–25 the twenty height fields, and draws34–41
the link-warp fields. The query itself does not draw a new random value
to replace any of them.

All offsets in this table are relative to World:

| Fields | Values, in listed order |
|---|---|
|+0x800168,+0x80016c,+0x800170,+0x800174|19053,23075,13104,32363|
|+0x80018c,+0x800190,+0x800194,+0x800198|3265,30749,32678,9760|
|+0x80019c,+0x8001a0,+0x8001a4,+0x8001a8|28064,20429,750,4480|
|+0x8001ac,+0x8001b0,+0x8001b4,+0x8001b8|3863,30429,18586,30758|
|+0x8001bc,+0x8001c0,+0x8001c4,+0x8001c8|31595,31567,24630,16059|
|+0x8001cc,+0x8001d0,+0x8001d4,+0x8001d8|2734,703,3966,12383|
|+0x8001fc,+0x800200,+0x800204,+0x800208|3541,2251,22174,14324|
|+0x80020c,+0x800210,+0x800214,+0x800218|24238,18619,29848,25829|

This is the reached dependency set for these raw H branches, not all77
initialization outputs or a claim that every terrain stage needs only
these fields. The mountain-mask offsets and surface-color state belong
to later/different routines.

## 3. Verified scale words and lattice arguments

The actual double constants are important even where their shortest
decimal names look exact. For these two integer points, rounding each
native double product gives the exact integers in the last two columns.
Adding the listed integer offsets is then exact in double.

| Scale | Binary64 word | VA / RVA | Products(X,Y) at(8390000,8390000) | Products(X,Y) at(8400000,8390000) |
|---|---|---|---|---|
|0.0001|0x3f1a36e2eb1c432d|0x702b30 /0x302b30|(839,839)|(840,839)|
|0.0002|0x3f2a36e2eb1c432d|0x71dd10 /0x31dd10|(1678,1678)|(1680,1678)|
|0.0005|0x3f40624dd2f1a9fc|0x702b38 /0x302b38|(4195,4195)|(4200,4195)|
|0.001|0x3f50624dd2f1a9fc|0x702b40 /0x302b40|(8390,8390)|(8400,8390)|
|0.002|0x3f60624dd2f1a9fc|0x71dd20 /0x31dd20|(16780,16780)|(16800,16780)|
|0.01|0x3f847ae147ae147b|0x702a48 /0x302a48|(83900,83900)|(84000,83900)|

Thus the reached height, roughness, link-warp and displacement noises
are on integer lattice arguments. The stored angle multiplier
**0x400921fb5a7ed197** at VA0x702b48 / RVA0x302b48 still governs the
native kernel, but multiplies zero fractions here. No mathematical-pi
substitution, nonlattice cosine approximation or noise library is needed
to derive these query-noise float words. The prerequisite cell records
retain their separate previously bounded construction provenance.

The nominal0.0025/0.005 mountain/surface scales are not additional noise
inputs required by the reached raw H path with these absent locations.
Their behavior is not inferred from this lattice check.

## 4. Pure elevation blend and saturated link/location controls

Unlike the direct climate blend's whole-block sample, H preserves the
displacement fraction in its fixed signed64 sample. Each float32
displacement component is multiplied by float65536, converted toward
zero to signed64, then added to the integer block position in fixed
space. These are the derived exact component words and fixed offsets:

| Point(X,Y) | Displacement X/Y words | Fixed offset X/Y |
|---|---|---|
|(8390000,8390000)|0x3f97f4be /0xc21af588|(77801,−2538850)|
|(8400000,8390000)|0x3f97f4be /0x4437d509|(77801,48190500)|

Cell distances remain float32 rather than being truncated to integer.
For each fixed difference, float32 conversion, exact2^-16 scaling,
separate squares and Y-square plus X-square precede the nearest pass.
The elevation weight uses the float32 scale **0x3356bf95** at
VA0x71dcd8 / RVA0x31dcd8, then squares one minus the capped scaled
distance difference. This is not the climate blend's0x350637bd scale.

| Point | Nearest cell / distance² | Next-nearest distance² | Float32 difference from nearest | Elevation / land share |
|---|---|---|---|---|
|(8390000,8390000)|(511,512) /22891962|123019080|100127120|−100 /0|
|(8400000,8390000)|(512,512) /54989408|90084984|35095576|74 /1|

Both differences exceed the20-million weight cutoff, so only the nearest
cell has weight1 and every other weight is0. The first point's blended
elevation E is therefore exactly−100 and land share0; the second has
E=74 and land share1. Missing cells would be skipped by H's elevation
pass, rather than invoking the direct climate all-or-default rule; all
nine required cells here are present.

The link helper VA0x5eea60 / RVA0x1eea60 chooses its owner through
0x5cade0 /0x1cade0. The owners are(511,512) and(512,512), respectively.
Its seed-offset warp and segment distances use the existing regions
contract §5.2. With §1's explicit absent neighbor, all reached segment
distances² exceed250000 at both points; the smaller second-point value
is approximately298522.75. The native float scale is
**0x3951b717** at0x71dce0 /0x31dce0, approximately0.00019999999495.
These margins establish **B>49**, without requiring an exact B word.

The null region/location yields town-ness q=0 and no location modifiers.
Consequently flatness equals the large B value; its H coefficient clamp
saturates. Both0.1/0.9 coefficients for the two medium terms round to1,
as does the0.5/0.5 coefficient for the final term. In roughness,
1−0.75B is negative, so the optional positive link correction is absent.
The direct fire contribution is0. Thus only the native lattice-derived
roughness value remains. The relevant routines are roughness
VA0x5f9340 / RVA0x1f9340, town-ness0x5989d0 /0x1989d0, and flatness
0x5f9f70 /0x1f9f70. No location-island, town attenuation, lake, valley
or spire rule is admitted.

## 5. Scalar-operation staging and diagnostic words

The ten ordinary H noises are listed in native role order. An offset
pair denotes the two World fields from §2, and the scale multiplies X/Y
before those integer offsets are added in double.

| Role | Offset suffixes | Scale | Noise word at first point | Noise word at second point |
|---|---|---|---|---|
|Squared unit term1|0x18c/0x190|0.0001|0xbe9f0a28|0x3f5261c0|
|Squared unit term2|0x194/0x198|0.0001|0x3f41ebfe|0x3ef694b1|
|Squared unit term3|0x19c/0x1a0|0.001|0x3f35a763|0xbe7e56f8|
|Squared unit term4|0x1a4/0x1a8|0.001|0xbf36d97b|0x3ed21f7c|
|Squared unit term5|0x1ac/0x1b0|0.002|0x3dc67a67|0x3f557268|
|First coarse contribution, using squared unit term2|0x1bc/0x1c0|0.0002|0x3d29cea2|0x3f14a725|
|Second coarse contribution, using squared unit term1|0x1b4/0x1b8|0.0002|0xbef140e1|0xbdd33445|
|First detail contribution, using gated term4|0x1cc/0x1d0|0.002|0xbefd6e06|0xbee25cb4|
|Second detail contribution, using gated term3|0x1c4/0x1c8|0.002|0x3f706afd|0xbe28a690|
|Third detail contribution, using gated term5|0x1d4/0x1d8|0.01|0xbf3e7ec4|0xbf3b5c39|

For each squared unit term, the noise is already float32. Add float1,
round, multiply by float0.5, round, then square and round. The five
resulting words are:

| Point | Terms1..5, in order |
|---|---|
|First|0x3df3527f,0x3f45af6b,0x3f3b0d3a,0x3ca737ed,0x3e9a030f|
|Second|0x3f546a01,0x3f0c7d4e,0x3e109f8f,0x3efe9eb3,0x3f573719|

The nontrivial roughness gate must retain its actual scalar order.
First form float32 v=min(4R,1). One multiplication chain multiplies v
by3 and then by v, rounding each operation. A separate chain multiplies
v by2, then by v, then by v, rounding each operation. Subtract the
second chain from the first, round, and square that result in float32.
This is the verified path at VA0x5c6100–0x5c6145 onward /
RVA0x1c6100–0x1c6145 onward, not permission to substitute an algebraically
factored polynomial.

| Quantity | First point word | Second point word |
|---|---|---|
|Roughness R|0x3dbd8dad|0x3f3a6231|
|Clamped v|0x3ebd8dad|0x3f800000|
|Difference of the two multiplication chains|0x3e9e9182|0x3f800000|
|Squared gate|0x3dc46fde|0x3f800000|
|Gated term3|0x3d8f87dd|0x3e109f8f|
|Gated term4|0x3b004fe4|0x3efe9eb3|
|Gated term5|0x3cec5b49|0x3f573719|

Each coarse contribution adds1 to its noise, multiplies by100, then
by its corresponding ungated squared unit term, rounding every operation
to float32. Add the two contributions in their listed order, multiply
by land share, then add E. Detail contributions similarly add1, multiply
by50 or20 as appropriate, then by the corresponding gated term.

The **final add order** is: add first detail to second detail; add third
detail to that rounded sum; add the rounded coarse/elevation result
last. All three additions are float32. The directly verified sequence
is VA0x5c6a98–0x5c6bac / RVA0x1c6a98–0x1c6bac, including the last two
additions at0x5c6ba2/0x5c6ba6. It does not add the base before the third
detail or accumulate in double.

| Quantity | First point word | Second point word |
|---|---|---|
|First coarse contribution|0x42a0d820|0x42ad7da6|
|Second coarse contribution|0x40c90bb6|0x4294d5b0|
|After land share and E|0xc2c80000|0x436b29ab|
|First detail|0x3d4a8020|0x415df39a|
|Second detail|0x40d97103|0x40bcc1a7|
|Third detail|0x3e17323d|0x409042cd|
|Rounded sum of all three details|0x40dfbf95|0x41c23aea|

## 6. Final raw H references and limits

| Integer block point(X,Y) | Raw H float32 word | Exact float32 value, blocks | Mathematical floor(H) | Truncation toward zero |
|---|---|---|---|---|
|(8390000,8390000)|**0xc2ba0407**|−93.00786590576171875|−94|−93|
|(8400000,8390000)|**0x4381b884**|259.4415283203125|259|259|

The floor/truncation columns are arithmetic diagnostics of H itself.
Neither is asserted to be a final generated surface Z. The zone's later
mountain/relief/surface/structure passes are outside this note.

The exact words follow from the stated immutable records, seed-derived
offsets, optional selector0, null applicable region object, and explicit
first-point absent link neighbor. They establish raw conditional H for
these two points, not a naturally fully prepared region or all streaming
states. Different locations/cached fire data, changed cell/neighbor
state, altered rounding/control modes, server parity and nonlattice
height queries are not established. No new runtime oracle or
implementation comparison is claimed.

## 7. Synthetic roughness-to-squared-gate references

These are **synthetic scalar-stage fixtures**, independently evaluated
from the exact float32 input words below using the verified §5 native
operation sequence and IEEE nearest-even arithmetic. They do not assert
that either input roughness occurs at the two world points in §6.
No engine, probe, native runtime or captured output supplied the values.

Each multiplication, subtraction and final square rounds separately to
float32. First multiply R by4 and cap above at1 to obtain v. The
three-times chain multiplies v by3, then by v. The twice chain multiplies
v by2, then by v, then by v. Subtract the completed twice chain from the
completed three-times chain and square that rounded difference. Both
chosen inputs remain below the v upper clamp.

| Stage | Input R word0x3cf5c28f | Input R word0x3e2e147b |
|---|---|---|
|R, exact input word|0x3cf5c28f|0x3e2e147b|
|Clamped v|0x3df5c28f|0x3f2e147b|
|Three-times chain: after multiplication by3|0x3eb851eb|0x40028f5c|
|Three-times chain: after multiplication by v|0x3d30f27b|0x3fb18fc5|
|Twice chain: after multiplication by2|0x3e75c28f|0x3fae147b|
|Twice chain: after first multiplication by v|0x3cebedfa|0x3f6cbfb2|
|Twice chain: after second multiplication by v|0x3b627e0e|0x3f20fd3c|
|Rounded difference of completed chains|0x3d22ca9a|0x3f42224e|
|**Final squared gate**|**0x3acf0a16**|**0x3f133803**|

The input values are approximately0.029999999329447746 and
0.17000000178813934. The final squared gates are respectively
approximately **0.0015795852523297071** and **0.5750734210014343**.
The words, rather than nominal decimal0.03/0.17 or an algebraically
factored smooth polynomial, define these scalar references. Existing
raw-H references and their prerequisites are unchanged.
