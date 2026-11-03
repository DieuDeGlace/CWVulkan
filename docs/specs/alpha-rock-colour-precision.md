# Alpha raw rock color: precision and external inputs

This independently authored supplement covers the original Alpha client
VA0x5f9620 (RVA0x1f9620), Rk in the sister independent
alpha-worldgen-terrain.md §6.6 and the sample used by
[implicit terrain color](alpha-implicit-terrain-colour.md). Image base is
VA0x400000. Evidence is original native instructions/constants and existing
independent prose only. No engine, tests, exporter, SDK/reconstruction,
owner exports/captures or game execution was inspected. Numeric references
below are conditional arithmetic fixtures, not captured native world samples.

## 1. Inputs, return and state dependencies

World is the member-call receiver. The binary consumes five stack inputs:
output pointer, signed32 x, signed32 y, signed32 z, and an optional Zone
selector. In the routine's established frame these are at +0x08, +0x0c,
+0x10, +0x14 and +0x18 respectively; the return cleans up0x14 bytes at
VA0x5f9baf (RVA0x1f9baf). The abbreviated decompiler header omits the final
selector, but its native loads and calls establish that input.

Coordinates are integer blocks, not fixed64 positions. The routine writes
three float32 values at output+0/+4/+8 in byte-color units and returns the
output pointer. It does not normalize by255, truncate to RGB bytes, write
alpha or return a single scalar color through the floating return register.

The reached dependencies and order are:

1. Two strata noise calls use x/y/z and fixed constants.
2. Column query VA0x4347a0 (RVA0x347a0), with x/y and the supplied Zone
   selector, provides warmth from column+0x08 when available. Otherwise
   VA0x5c4dd0 (RVA0x1c4dd0) computes warmth from World/x/y; its result is
   narrowed to float32. These paths are at VA0x5f96f0–0x5f9714
   (RVA0x1f96f0–0x1f9714).
3. A second column query occurs at VA0x5f9723 (RVA0x1f9723). If missing,
   VA0x5c4800 (RVA0x1c4800) is called for moisture at VA0x5f9733
   (RVA0x1f9733), but its return is discarded. There is no direct moisture
   operand in the ensuing Rk RGB arithmetic. The reached call is nevertheless
   part of the native query order; this note does not erase its possible
   nested preparation/cache effects.
4. The warmth branch can make one additional noise query.
5. Detail noise reads **signed32 World+0x80022c and +0x800230**. These are
   the O22c/O230 labels in the independent terrain/random prose, not fields
   at World+0x22c/+0x230. They are converted to double as signed integers,
   not normalized or treated as unsigned hashes. Reads are at
   VA0x5f98bd–0x5f98fe (RVA0x1f98bd–0x1f98fe).
6. After constructing and clamping the preliminary RGB, a third column
   query provides fire from column+0x0c, or VA0x5ef040 (RVA0x1ef040) supplies
   the fallback fire blend, narrowed to float32. This is at
   VA0x5f99cb–0x5f99f9 (RVA0x1f99cb–0x1f99f9). Positive fire makes three
   further noise calls and a final blend.

The optional selector is a real cache input: null lets0x4347a0 resolve a
loaded Zone; a supplied Zone must contain the requested x/y. Its ordinary
coordinate admission is0≤x,y<0x1000000. A successful query returns a column
from that Zone's column array. The three lookups remain separate calls,
rather than a single assumed climate snapshot. Stable memory is an explicit
condition of the fixtures below.

O22c/O230 and cached/fallback climate values must therefore be supplied as
actual inputs for an ordinary world comparison. This audit does not assign
them zero defaults or infer them from the current time or a guessed seed.
No direct rand/srand call occurs in this Rk routine; broader climate-query
preparation behavior is outside this bounded precision contract.

## 2. Constant words and numeric conventions

Every float operation described below independently rounds to float32.
Multiplication and addition are separate; there is no fused multiply-add.
Noise coordinates are constructed in double. Signed32 coordinates and
offsets convert exactly to double before those scale/add operations. The
noise kernel VA0x4c0ef0 (RVA0xc0ef0) itself returns an already narrowed
float32 value; call sites also store it to float32 before the Rk arithmetic.
Its verified hash/return boundary is in
[cloud noise constants](alpha-cloud-noise-constant.md).

| Float value | Word | Constant VA / RVA |
| --- | --- | --- |
|0.1|0x3dcccccd|0x6fcb7c /0x2fcb7c|
|0.5|0x3f000000|0x745da0 /0x345da0|
|0.75|0x3f400000|0x6fcdc4 /0x2fcdc4|
|1|0x3f800000|0x745dc0 /0x345dc0|
|2|0x40000000|0x745e10 /0x345e10|
|4|0x40800000|0x745e60 /0x345e60|
|10|0x41200000|0x745e74 /0x345e74|
|20|0x41a00000|0x6fcd74 /0x2fcd74|
|50|0x42480000|0x6fcd7c /0x2fcd7c|
|100|0x42c80000|0x6fd0b4 /0x2fd0b4|
|120|0x42f00000|0x6fce58 /0x2fce58|
|130|0x43020000|0x6fd594 /0x2fd594|
|160|0x43200000|0x703ef0 /0x303ef0|
|170|0x432a0000|0x6fd598 /0x2fd598|
|200|0x43480000|0x6fce60 /0x2fce60|
|255|0x437f0000|0x6fce64 /0x2fce64|

| Double value | Word | Constant VA / RVA |
| --- | --- | --- |
|0.001|0x3f50624dd2f1a9fc|0x702b40 /0x302b40|
|0.005|0x3f747ae147ae147b|0x745db8 /0x345db8|
|0.02|0x3f947ae147ae147b|0x71db38 /0x31db38|
|0.05|0x3fa999999999999a|0x71db48 /0x31db48|
|0.1|0x3fb999999999999a|0x71a048 /0x31a048|
|0.4|0x3fd999999999999a|0x71dd70 /0x31dd70|
|123|0x405ec00000000000|0x71ddc0 /0x31ddc0|
|6544|0x40b9900000000000|0x71de68 /0x31de68|
|98984|0x40f82a8000000000|0x71df18 /0x31df18|

Float0.1 and double0.1 are deliberately different rounded constants and
have different roles below. They are not interchangeable decimal shorthand.

## 3. Strata, warmth and detail stages

The first noise sample is N(0.1y+98984,0.4z), and the second is
N(0.1x,0.4z), with all coordinate scale/add work in double. Their order and
stores are VA0x5f9632–0x5f96be (RVA0x1f9632–0x1f96be). The float32 sum of
those returned samples is multiplied by float0.5; float1 is added; the
result is multiplied by float0.5 again. Call this float32 strata value v.
There is no additional clamp of v. Later, v is multiplied once by float160
to make the shared strata contribution c.

Warmth w is the float32 value obtained in §1. The base RGB starts at
(120,120,130). The comparisons use float thresholds:

* For w<0.5, t is the float32 product2w, and u is the float32 difference
  1−t. The three base channels combine products10u+160t,20u+160t and
  50u+170t. Each product and final addition rounds separately; the product
  160t is shared by red and green. Evidence: VA0x5f9766–0x5f97cc
  (RVA0x1f9766–0x1f97cc).
* For 0.5≤w≤0.75, the starting base is retained. In particular exact0.5
  belongs to this branch; the cold branch is not smoothed across it.
* For w>0.75, subtract float0.75, then multiply that float result by4
  to obtain t. The extra noise coordinates are **0.001t+6544 and
  0.001y+123**, in double after widening t. The first coordinate really
  uses transformed warmth t, not x. Query/store is at VA0x5f97e7–0x5f9849
  (RVA0x1f97e7–0x1f9849). With u the float difference1−t, red is the
  sum of separately rounded120u and200t; blue similarly combines130u
  and100t. Green first multiplies the hot noise by20, adds130, multiplies
  that float result by t, then adds the shared product120u. The hot base
  stage is VA0x5f984e–0x5f98ae (RVA0x1f984e–0x1f98ae).

The next detail query is N(O22c+0.005x,O230+0.005y), with double coordinate
multiplication followed by double addition of each widened signed offset.
The call/store is VA0x5f9907–0x5f990c (RVA0x1f9907–0x1f990c).

Red and green each add c to their base channel in float, then add the
separately produced float zero-times10 term. Blue first adds c to its base;
its additional detail is formed by adding float1 to the detail noise and
multiplying that result by10, then adding it to the preliminary blue.
These operations are VA0x5f990f–0x5f995f (RVA0x1f990f–0x1f995f).

All three preliminary channels are then limited to0..255, by the lower
and upper comparisons/stores at VA0x5f9963–0x5f99cb
(RVA0x1f9963–0x1f99cb). This clamp precedes the fire lookup/blend. Neither
warmth nor its transformed t receives a separate clamp here.

## 4. Fire: upper-only noise cap, sequential power, final blend

Only fire F strictly greater than float zero enters this stage, at
VA0x5f99f9–0x5f9a05 (RVA0x1f99f9–0x1f9a05). F is not clamped by Rk.
The native noise calls are ordered N(0.05x,0.05y), N(0.02x,0.02y), then
later N(0.1x,0.1y), all with double coordinate products.

The first returned float is multiplied by **float0.1** and stored as
float32 before the second call. Its product is added to the second returned
float. The sum s is capped only when greater than1. **There is no lower
cap at−1 or0.** Call/combination evidence is VA0x5f9a3c–0x5f9aa6
(RVA0x1f9a3c–0x1f9aa6).

For its magnitude, s is widened to double, its double sign bit is cleared
with mask0x7fffffffffffffff at VA0x745f50 (RVA0x345f50), and it is narrowed
back to float. The float difference1−|s| is raised to the fifth power by
**a square followed by three successive multiplications by that original
difference**, rounding after each product. This is not a pow call or a
square-of-a-square reassociation. A sum below−1 can make that difference
and its fifth power negative. Evidence: VA0x5f9aa9–0x5f9af5
(RVA0x1f9aa9–0x1f9af5).

Let p denote that float fifth power. The fire target red multiplies p by200
then adds10, with separate float results. Target green adds1 to the final
noise sample N(0.1x,0.1y), multiplies by p, multiplies by50, then adds10,
rounding at every operation in that order. Target blue is10.

The original already-clamped RGB channels are separately multiplied by the
float difference1−F. Each fire target is multiplied by F, and each channel's
two contributions are then added in float32. Blue's fire contribution is
the separate product F times10. Final arithmetic/stores are at
VA0x5f9af9–0x5f9ba3 (RVA0x1f9af9–0x1f9ba3). **No RGB clamp follows this
blend.** There is no division, packing or saturation added at return.

## 5. Conditional arithmetic fixtures with complete external inputs

These fixtures use x=y=z=0, **O22c=O230=0**, and a supplied Zone whose
signed32 Zone coordinates are(0,0), with a valid allocated column array.
All three column lookups resolve its column(0,0); that column's float32
warmth and fire are the table's w/F. The memory remains unchanged throughout
the call. Thus no fallback climate function or missing-Zone path runs, and
no moisture value is read. No other World data is needed by these reached
branches. This is a synthetic cached-climate state, not a claim that a
particular seed generates that column or those O values.

No replacement noise library is used. Every noise argument in these rows
is an integer lattice point: first(98984,0), all others(0,0). None enters
the hot-warmth branch. Native hash arithmetic and zero-fraction cosine
weights give these already narrowed noise words:

| Input | Float32 word | Decimal reference |
| --- | --- | --- |
|N(98984,0)|0xbeb78ecb|−0.3585112988948822|
|N(0,0)|0xbe9046e8|−0.28179097175598145|

With ordinary nearest-even float32 rounding, the shared v word is
0x3eae0a93, c is0x42598d38 and the blue detail contribution is0x40e5d3af.
For positive fire, s is0xbe9eb466,1−|s| is0x3f30a5cd and p is0x3e203109.
The resulting returned RGB float words are:

| w | F | Red word | Green word | Blue word |
| --- | --- | --- | --- | --- |
|0|0|0x4280c69c|0x4294c69c|0x42df23d7|
|0.25|0|0x430b634e|0x4310634e|0x432b91eb|
|0.5|0|0x432e634e|0x432e634e|0x433f91eb|
|0.75|0|0x432e634e|0x432e634e|0x433f91eb|
|0.5|0.5|0x42d7ace2|0x42be0171|0x42c991eb|
|0.5|1|0x4225264f|0x4179e234|0x41200000|

For orientation, the w=0.5/F=0 row is approximately
(174.38790893554688,174.38790893554688,191.5699920654297); the F=1 row is
approximately(41.287410736083984,15.617725372314453,10). These are returned
floats in byte-color units, not bytes or normalized RGB. They were derived
from the pinned inputs, native constants and individually rounded operations;
no original executable was called to produce them.

A separate **hot-base stage-only** arithmetic check is t=1 with a supplied
hot-noise return0: the base is(200,130,100), words
(0x43480000,0x43020000,0x42c80000). This conditional stage value is not a
full Rk sample at an asserted coordinate; no claim is made that the actual
hot noise at(6544.001,123) equals0.

### 5.1 Native seed123 offsets with the same synthetic cached climate

This additional fixture set replaces only the zero O offsets above with the
offsets actually produced by the native **World initialization for seed123**.
It retains x=y=z=0, the valid supplied Zone/column and each row's synthetic
cached w/F. It does not claim that seed123's generated column(0,0) actually
has any of those cached climate values.

Native World constructor VA0x58eb00 (RVA0x18eb00) passes literal123 at
VA0x58ee17 (RVA0x18ee17) to initialization VA0x5a52e0 (RVA0x1a52e0), called
at VA0x58ee1d (RVA0x18ee1d). That initializer stores the supplied seed and
calls srand at VA0x5a532a (RVA0x1a532a), then obtains its offset draws.
Subsequent World reinitialization can use another seed; this constructor
path does not prove the current running world's seed is123.

The imported MSVCR110 rand contract is the independently documented
32-bit state transition with multiplier214013, increment2531011 and a
15-bit result taken from the updated state's bits16..30. This fixture
uses that original stream contract, not an engine's current random provider.
The native offset initializer's draw counts and stores were independently
checked here: after its first draw for World+0x800188, it draws groups of
4,20,2,2,2,2 and8 values. Thus41 draws precede the six-field group starting
at World+0x80021c. Its native loop is VA0x5a5434–0x5a5453
(RVA0x1a5434–0x1a5453), with one rand call and one signed remainder by100000
per field. The draw is0..32767, so that remainder preserves its value.

The resulting group is:

| One-based draw after srand(123) | World field | Signed32 stored value |
| --- | --- | --- |
|42|+0x80021c|25215|
|43|+0x800220|16284|
|44|+0x800224|8757|
|45|+0x800228|22925|
|46|+0x80022c, O22c|**28488**|
|47|+0x800230, O230|**12097**|

This draw placement is determined by constructor/initializer order, not by
sorting the offset fields numerically. In particular the earlier-addressed
ravine offsets at+0x800178 are initialized later and do not take the draws
used here. The first five stream values440,19053,23075,13104,32363 also
agree with the independently specified native seed123 prefix.

At x=y=0, the detail query is therefore exactly **N(28488,12097)**. Both
coordinates remain integer lattice points after the double coordinate
operations; no non-lattice cosine/noise library is substituted. The initial
lattice index is718017, and the verified native32-bit hash/mask yields
**0x6b54bcf9**. Its float32 noise result is **0xbf2d52f4**, exactly
**−0.6770470142364501953125**. Adding float1 then multiplying by float10
gives blue detail contribution **0x404eb09e**, approximately3.229529857635498.

All strata and fire-noise inputs remain those in §5. With the same
individually rounded arithmetic, the returned RGB words become:

| w | F | Red word | Green word | Blue word |
| --- | --- | --- | --- | --- |
|0|0|0x4280c69c|0x4294c69c|0x42d73c21|
|0.25|0|0x430b634e|0x4310634e|0x43279e10|
|0.5|0|0x432e634e|0x432e634e|0x433b9e10|
|0.75|0|0x432e634e|0x432e634e|0x433b9e10|
|0.5|0.5|0x42d7ace2|0x42be0171|0x42c59e10|
|0.5|1|0x4225264f|0x4179e234|0x41200000|

For example, w=0.5/F=0 returns approximately
(174.38790893554688,174.38790893554688,187.617431640625); w=0.5/F=0.5
returns approximately(107.83766174316406,95.00281524658203,98.8087158203125).
The red/green equality with §5 is a consequence of changing only the detail
offsets while keeping climate and coordinates pinned. It is not a statement
that changing a whole generated World's seed leaves those channels unchanged.
These are instruction-derived references with **native seed-produced offsets
and synthetic cached climate**, not actual captured world samples. No original
game or exporter was executed.

## 6. Limits

The precision contract covers finite ordinary values and the reached
client path. It does not establish server equivalence, altered floating-point
control modes, NaN/exception recovery, invalid pointers or every nested
climate-preparation effect. The native state producers for an actual World's
O offsets and climate remain their existing independent contracts; they
were not fabricated to make an unpinned world sample fit these fixtures.
No general non-lattice cosine implementation was substituted to create
additional vectors, and no original-game oracle is asserted.
