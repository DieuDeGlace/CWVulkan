# Alpha direct climate-blend arithmetic fixtures

Independent native-only numeric supplement to alpha-worldgen-regions.md
§§2–4 and alpha-worldgen-random.md. These are **arithmetic fixtures, not
native runtime captures**. Original Alpha client code/constants and those
independent contracts supplied the inputs and arithmetic. No engine,
tests, exporter, sister implementation, SDK implementation, owner data,
captures, benchmarks or package output were inspected. No game was run.

Alpha client image base is0x400000. The four direct World queries are:

| Output | VA / RVA | Cell value being blended |
|---|---|---|
|Warmth|0x5c4dd0 /0x1c4dd0|float32 at+0x10|
|Moisture|0x5c4800 /0x1c4800|float32 at+0x0c, before any oasis adjustment|
|Sea share|0x5f0720 /0x1f0720|indicator of signed32 elevation+0x18 below0|
|Fire share|0x5ef040 /0x1ef040|indicator of byte+0x08 equal to1|

The numeric values below are ordinary nearest-even binary32/binary64
references under the existing noise contract. Hex words are IEEE754
numeric words, not memory-byte-order strings.

## 1. Exact prerequisite state: one nine-cell window

The world seed is123, with original initialization's first CRT draw
giving base offset **A=440** at World+0x800188. The home coordinates at
World+0x8000f0/+0x8000f4 are the original **8396928.0 float32** values.
All three test points are in region cell(512,512), so each query's
undisplaced search window is **X511..513, Y511..513**, inclusive.

Every one of the following nine cell pointers must be nonnull and remain
unchanged during the query. They are in the cell grid at
World+0x4000bc, indexed by1024×X+Y. Point X/Y are signed32 block fields
at cell+0/+4; warmth/moisture are the raw float32 words at+0x10/+0x0c;
elevation is the signed32 field+0x18 and fire the byte+0x08.

| Cell(X,Y) | Per-cell CRT seed | Point X | Point Y | Warmth word | Moisture word | Elevation | Fire |
|---|---|---|---|---|---|---|---|
|(511,511)|529329|8377344|8381440|0x3f23d215|0x3ecf7a06|56|0|
|(511,512)|530353|8385536|8391680|0x3f7ce793|0x3f7a60c1|−100|0|
|(511,513)|531377|8375296|8420352|0x3ee36e94|0x3ea179aa|51|0|
|(512,511)|529330|8401920|8381440|0x3d65a4fe|0x3c6d81db|9|0|
|(512,512)|530354|8397824|8397824|0x3f0f651f|0x3efba72b|74|0|
|(512,513)|531378|8403968|8412160|0x3f75a41e|0x3f6fe046|−100|0|
|(513,511)|529331|8418304|8385536|0x3f7d6e94|0x3f686c6a|−100|0|
|(513,512)|530355|8408064|8395776|0x3f41371c|0x3f0a6ae2|−100|0|
|(513,513)|531379|8416256|8408064|0x3ef5f052|0x3f13eaf4|117|0|

The other cell bytes, including the cell ID, do not enter these four
blends. This table pins the climate-relevant record fields, not unknown
padding. It does not require any zone columns, meshes or light solution.

For a completely pinned oasis boundary, the separate **region-object**
pointer at World+0xbc for region(512,512) is **null**. This does not mean
the nine region-cell pointers above are null: they are separate grids.
The location lookup reached by moisture therefore returns null, as §5
details. No location type, radius or distance helper input is left to an
unknown generated region object in these fixtures.

## 2. Independent derivation of the prerequisite cells and displacement

The records were reconstructed from original cell constructor
VA0x5d7a70 / RVA0x1d7a70, which reseeds for each cell with its signed32
X+0x108a+1024×Y+3×A. The independent CRT recurrence and exact native
draw/branch order determine the climate draws and point coordinates.
The point's Y draw precedes X; snapping uses the2048-block location grid
with1024-block center offset. The home cell uses the constructor's home
branch. The post-elevation sea branch clears fire and consumes its
additional climate draws. The table includes those final climate values,
not the values before a sea-cell redraw.

Cell elevation uses the native double1.4 scaling, integer base A, double843
offset and second noise's double4 scaling. Constants are at VA0x71dd90,
0x71db70 and0x702b50 respectively (RVA0x31dd90,0x31db70,0x302b50).
The first noise's fractional arguments for these cells are fifths of a
unit, apart from binary input-rounding error; the second noise is exactly
on the integer lattice. The existing native32-bit hash and float32 noise
return were used, without substituting a noise library.

The native interpolation angle multiplier is **not mathematical pi**.
The stored binary64 word at **VA0x702b48 / RVA0x302b48** is
**0x400921fb5a7ed197**, rendered as3.1415927. Its exact binary value exceeds
mathematical pi by approximately **4.641020666628484e−8**. The older random
contract's3.1415927 shorthand is correct. Exact fifth-angle identities
therefore provide reference centers for an enclosure, not the actual
native cosine arguments. The earlier version's generic1e−9 noise
allowance is superseded by the explicit perturbation bounds below.

The positive mathematical fifth-angle cosine values are(1+sqrt(5))/4 and
(sqrt(5)−1)/4, with their negative counterparts and endpoints1/−1. These
identities were evaluated with high-precision arithmetic. For each noise
input, the comparison is between its actual binary64 fractional coordinate
multiplied/rounded with the stored constant, and the corresponding
mathematical fifth angle. It includes both the constant perturbation and
the native coordinate-product rounding; it is not just an estimate of
the latter.

Cosine's absolute derivative is at most1, so an angle error of magnitude
delta gives a cosine error at most delta and an interpolation-weight error
at most delta/2. For each native lattice square, the relevant X amplitude
is the larger absolute difference along its two horizontal hash-value
edges, and the Y amplitude is the analogous vertical maximum. Bilinear
interpolation then changes by at most half the sum of each amplitude
times its angle-error bound, with an additional1e−12 arithmetic allowance
for ordinary binary64 evaluation. For an exactly integer second noise
coordinate, its vertical weight stays0, and only the first horizontal
edge is needed. The hash-value differences are obtained from the original
masked integer polynomial; their denominators are exactly2^30.

For the nine cell-elevation inputs, the largest such calculated noise
bound is below3.489e−8. Propagating a conservative **3.5e−8** bound through
the native float32 elevation stages gives the same pre-sea-conversion
signed integers at both enclosure ends: in table scan order,
**56,−36,51,9,74,−12,−3,−39,117**. The negative cells then take the native
sea/clamp/redraw branch. Thus all final integer elevations and climate
record fields in §1 survive the full constant perturbation. No exact
nonlattice elevation-noise word is asserted.

Query displacement is the crossed-axis native calculation established at
VA0x5eefa0 / RVA0x1eefa0: displacement X uses noise of scaled Y with3423,
and displacement Y uses scaled X with23421. The scale's double word is
0x3f40624dd2f1a9fc. Noise is narrowed to float32 before the separate
float32 multiplications by3 and256. See
[alpha-region-displacement-vectors.md](alpha-region-displacement-vectors.md)
§1 for the native sites and constants.

For the chosen queries, the nominal scaled coordinates are4198.4,
4201.2 and4201.4. Their binary64 rounding and the stored angle constant
are included in these **upward-rounded bounds**; the second coordinate
is an exact integer, so its angle error is0:

| Nominal noise arguments | Actual horizontal hash-edge amplitude | Angle-error upper bound | Noise-error upper bound, including arithmetic allowance |
|---|---|---|---|
|(4198.4,3423)|441909286/1073741824|1.856295e−8|3.821e−9|
|(4198.4,23421)|589466808/1073741824|1.856295e−8|5.097e−9|
|(4201.2,23421)|943700742/1073741824|9.28148e−9|4.080e−9|
|(4201.4,23421)|943700742/1073741824|1.856295e−8|8.159e−9|

After float32 noise rounding, three Y-displacement cases have enclosures
spanning adjacent noise words. **Their earlier exact-noise-word and exact
Y-displacement claims are withdrawn.** The bounded component intervals
in §3, and the unique integer displaced points obtained from them, suffice
to establish the final climate words. No cosine library was called for
this derivation.

The arithmetic allowance and cosine error bound are conditional on the
ordinary double cosine semantics of the independent random contract.
They are not a measured native-runtime error guarantee or an exhaustive
audit of its approximation, replacements or floating-point control modes.

## 3. Integer and float stages used for these queries

All input points below are **integer blocks**. Each routine takes its
three-by-three window from the **undisplaced** coordinates. It then adds
the float32 displacement to the float32 conversion of each input
coordinate, rounding the addition to float32 **before** truncating it to
signed32. At these magnitudes the float32 position spacing is one block;
adding the displacement in double and truncating would be a different
operation.

For each cell, point-minus-displaced-point is formed in fixed signed64
space with65536 units/block. Each difference is converted to float32,
scaled by exact float32 2^-16, squared in float32, and the **Y square plus
X square** is added in float32. That result is truncated to signed32 d².
The first pass finds the smallest d². The second visits **ascending X
outer, ascending Y inner**, converts the signed32 difference d²−minimum
to float32 and multiplies by the stored float32 scale word
**0x350637bd** at VA0x71dcdc / RVA0x31dcdc. The result is capped above at1;
weight is float32 one minus that capped result.

The denominator and each weighted-value numerator accumulate in float32
in that same order, with separate multiply and add stages, followed by
float32 division. Warmth's reached weighting/accumulation/division is
VA0x5c51f1–0x5c526b / RVA0x1c51f1–0x1c526b. Moisture's corresponding
sequence is VA0x5c4c2e–0x5c4cbc / RVA0x1c4c2e–0x1c4cbc. The existing
regions contract supplies the shared sea/fire pre-scan and indicator rules.

Propagating the §2 bounds through float32 noise rounding and both output
multiplications gives these component enclosures, in blocks. Both ends
then give the **same** integer point after the native float32 position
addition and truncation:

| Input point(X,Y) | Displacement X enclosure | Displacement Y enclosure | Unique integer displaced point |
|---|---|---|---|
|(8396800,8396800)|[433.81427001953125,433.81427001953125]|[133.7900848388671875,133.79010009765625]|(8397234,8396934)|
|(8402400,8396800)|[433.81427001953125,433.81427001953125]|[−605.90679931640625,−605.90673828125]|(8402834,8396194)|
|(8402800,8396800)|[433.81427001953125,433.81427001953125]|[−437.15997314453125,−437.159912109375]|(8403234,8396363)|

These are inclusive bounds, not a claim that a captured native noise
return chose a particular endpoint. The final integer points, and hence
all distances/weights/outputs in §4, remain uniquely determined despite
the unresolved intermediate Y-word choice. The literal final climate
words in §4 are unchanged by this precision correction.

## 4. Literal four-field references and mixed-boundary diagnostics

With exactly the prerequisites above, the direct query outputs are:

| Integer point(X,Y) | Warmth word | Moisture word | Sea-share word | Fire-share word |
|---|---|---|---|---|
|(8396800,8396800)|0x3f0f651f|0x3efba72b|0x00000000|0x00000000|
|(8402400,8396800)|0x3f29d20a|0x3f048146|0x3f07c95b|0x00000000|
|(8402800,8396800)|0x3f41371c|0x3f0a6ae2|0x3f800000|0x00000000|

The first point has only home cell(512,512) at nonzero weight. Its
warmth/moisture are approximately0.5601367354393005/0.4915097653865814.
The last has only sea cell(513,512) at nonzero weight, returning
approximately0.7547471523284912/0.5406934022903442 and sea share1.
“Sea cell” here means its region elevation is negative; it is not a
statement that the query column's effective block is water.

The middle point is an **actually mixed home/sea-cell boundary in this
native-derived seed123 window**, not a synthetic averaging of arbitrary
climate records. Its warmth/moisture/sea values are approximately
0.6633611917495728/0.5175975561141968/0.5304161906242371. Only two
cells have nonzero weight:

| Cell | Signed32 d² | Difference from minimum | Float32 weight word |
|---|---|---|---|
|(512,512)|27757000|229376|0x3f62a3cf|
|(513,512)|27527624|0|0x3f800000|

The home cell's scaled difference is0x3deae18b; the float32 weight is
0.885312020778656. The denominator is **0x3ff151e8**,
approximately1.8853120803833008. In X/Y scan order, the pre-division
warmth numerator is **0x3fa01511**, moisture numerator **0x3f79d030**,
and sea numerator **0x3f800000**. All other cells have weight+0.

Every cell fire byte in this window is0, so fire returns+0 through its
no-fire pre-scan route. These fixtures do not claim to test a positive
fire-share blend.

## 5. Oasis lookup ordering versus numeric influence

Native moisture first completes the successful blend/division, then
**always performs** the location lookup at VA0x5c4ceb / RVA0x1c4ceb.
Only afterward, at VA0x5c4cf8–0x5c4d10 /
RVA0x1c4cf8–0x1c4d10, does it require moisture below the stored float0.2,
a nonnull location, and location type3 for the oasis adjustment.
This qualifies the older §4 shorthand: dryness gates the adjustment,
not the preceding location lookup.

The three queries use location subcell(4100,4100) or(4102,4100), all
within region(512,512). Lookup VA0x487da0 / RVA0x87da0 consults the
region-object lookup VA0x434a90 / RVA0x34a90. The pinned null region
pointer in §1 therefore yields a null location without constructing a
region. In addition, all three base moisture values are above0.2, so
no oasis adjustment would numerically apply even for a valid type3
location. The literal words in §4 describe returned numeric values;
they must not be used to infer that the location lookup was skipped.

## 6. Missing-cell fallback and remaining limits

As a separate partial-window fixture, use point **(8402400,8396800)**
and all the same prerequisites, but make cell pointer **(511,511) null**.
That cell's weight in the complete window was0, yet its absence forces:

| Warmth | Moisture | Sea share | Fire share |
|---|---|---|---|
|0x3f000000|0x3f000000|0x00000000|0x00000000|

Thus the defaults are0.5/0.5/+0/+0, with no renormalization of the
remaining cells. Warmth and moisture return their defaults from their
failed nearest pass before a successful blend or moisture location
lookup; sea/fire missing-cell/pre-scan routes likewise return0. Default
sites include VA0x5c5286 / RVA0x1c5286 and VA0x5c4db1 /
RVA0x1c4db1. This reference does not depend on keeping the two nonzero-
weight source cells absent: removing only the stated zero-weight cell
is sufficient.

The scope is these original-client direct queries, one explicit seed/home
construction, one immutable nine-cell window and the absent region-object
condition. No whole-world preparation, cached column climate, positive
fire/oasis case, server numerical parity, concurrent cell mutation or
modified floating-point mode is established. No expected words were
taken from an implementation or a captured/generated world. The
interpolation/rounding references are conditional arithmetic fixtures
under the independent native noise contract, not a claim that a new
native runtime oracle was collected.
