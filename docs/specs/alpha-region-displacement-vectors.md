# Alpha region-displacement arithmetic fixtures

This bounded note independently verifies the original Alpha client helper
VA0x5eefa0 / RVA0x1eefa0 and derives four finite arithmetic fixtures. They
are **not runtime captures**. Sources are original native code/constants
and the independent alpha-worldgen-regions.md §3.1 contract. No engine,
tests, exporter, SDK implementation, sister implementation, owner data or
captures were inspected, and no game was executed.

## 1. Native inputs, axis order and precision

The helper accepts an output address and two **signed32 integer block
coordinates**, X then Y. It writes two float32 displacement components at
output+0/+4, in blocks, and returns the output address. These are offsets,
not already-displaced positions or integer region-cell indices. The
reached helper does not read a World, seed, climate cache or region record
to construct these two values.

It computes the first output, displacement X, from **input Y**. It converts
that signed integer exactly to double, multiplies by the stored double
scale, and calls noise VA0x4c0ef0 / RVA0xc0ef0 with that result as the
first coordinate and double3423 as the second. This input/call sequence
is VA0x5eefa3–0x5eefcb / RVA0x1eefa3–0x1eefcb.

The noise helper has already narrowed its final result to float32 before
returning it through the floating return register. The caller stores it
as float32 again, multiplies by **float32 3**, then multiplies that rounded
result by **float32 256**, and stores displacement X. These two separate
multiplications are VA0x5eefd8/0x5eefe3
(RVA0x1eefd8/0x1eefe3).

It then computes displacement Y from **input X**, with the same double
scale and double23421 as the second noise coordinate. That call is
VA0x5ef013 / RVA0x1ef013. After the noise result's float32 store, the
separate float32 multiplications by3 and256 are VA0x5ef020/0x5ef02d
(RVA0x1ef020/0x1ef02d). The store at VA0x5ef035 / RVA0x1ef035 completes
the output pair. No coordinate addition, integer truncation, clamping or
normalization follows inside this helper.

The exact native constant words are:

| Meaning | Representation and word | VA / RVA |
|---|---|---|
| Input scale | double,0x3f40624dd2f1a9fc, conventionally written0.0005 |0x702b38 /0x302b38|
| Second noise coordinate for displacement X | double3423,0x40aabe0000000000 |0x71de40 /0x31de40|
| Second noise coordinate for displacement Y | double23421,0x40d6df4000000000 |0x71deb8 /0x31deb8|
| First output multiplier | float32 3,0x40400000 |0x6fcd70 /0x2fcd70|
| Second output multiplier | float32 256,0x43800000 |0x702ab8 /0x302ab8|

The scale is the stored **double** word, not a float32 decimal constant
subsequently widened to double. Integer inputs0,2000,4000 are exactly
representable in double; their products with this stored scale round to
exactly double0,1,2 respectively under ordinary nearest-even rounding.

## 2. Why these fixtures need no substitute cosine/noise library

All requested noise arguments therefore lie on integer lattice points.
The two fractional coordinates are0. The native interpolation receives
zero-angle cosine inputs, whose ordinary value1 gives zero blend weights;
the base lattice corner supplies the result. No approximate interpolation
library or off-lattice world-state assumption is needed for these fixtures.

The original kernel forms its lattice index with Y stride57, mixes it
with its13-bit left shift using32-bit XOR, and uses the native32-bit
wrapping polynomial terms60493,19990303 and additive3523796237
(0xd208dd0d). Its final mask is0x7fffffff. The directly reached index and
polynomial paths are VA0x4c0f1b–0x4c10bf / RVA0xc0f1b–0xc10bf.

For each listed lattice corner, its masked hash is converted to double,
scaled by exact double2^-30 and subtracted from double1. The scale word is
0x3e10000000000000 at VA0x702b20 / RVA0x302b20. The resulting lattice
value is narrowed to float32 at VA0x4c10e8–0x4c10f3 /
RVA0xc10e8–0xc10f3. The hash/return facts also agree with
[alpha-cloud-noise-constant.md](alpha-cloud-noise-constant.md); the
fixtures below use the verified additive, not the older erroneous
3524844813 decimal.

## 3. Intermediate float32 words

These are instruction-derived arithmetic results using ordinary
nearest-even binary32/binary64 rounding. “After3” is after the first
float32 multiplication; “after256” is the final displacement component.
Hex values are IEEE754 numeric words, not memory-byte-order strings.

| Noise arguments | Base lattice index | Masked hash | Noise float32 | After3 float32 | After256 float32 |
|---|---|---|---|---|---|
|(0,3423)|195111|0x305094d1|0x3e7af6b3|0x3f3c3906|0x433c3906|
|(1,3423)|195112|0x558bf1e5|0xbeac5f8f|0xbf8147ab|0xc38147ab|
|(0,23421)|1334997|0x02f52ea1|0x3f742b45|0x40372074|0x44372074|
|(1,23421)|1334998|0x4228caaf|0xbd0a32ac|0xbdcf4c02|0xc1cf4c02|
|(2,23421)|1334999|0x3922bcf1|0x3ddba862|0x3ea4be4a|0x42a4be4a|

The first output always uses the3423 row chosen by scaled input Y. The
second uses the23421 row chosen by scaled input X. Equalities between
fixtures below are consequences of this crossed axis dependence.

## 4. Requested output pairs

The word pairs are the authoritative arithmetic fixtures. Decimal values
are included for convenient reading, in block-displacement units.

| Integer input(X,Y) | Displacement X word | Displacement Y word | Displacement X | Displacement Y |
|---|---|---|---|---|
|(0,0)|0x433c3906|0x44372074|188.222747802734375|732.507080078125|
|(2000,0)|0x433c3906|0xc1cf4c02|188.222747802734375|−25.912113189697265625|
|(0,2000)|0xc38147ab|0x44372074|−258.559906005859375|732.507080078125|
|(4000,2000)|0xc38147ab|0x42a4be4a|−258.559906005859375|82.3716583251953125|

These do not include the later float32 addition of the original position,
integer conversion, candidate-cell search or nearest-cell tie selection.
They are the raw output of the displacement helper itself. They are not
actual captured region ownership or biome samples.

## 5. Scope and limits

The four inputs require no seed-offset construction or synthetic climate
injection: the direct helper has neither dependency. This does not make
the later region-cell construction seed-independent. Its subsequent
ownership/cell behavior remains the separate independent regions contract.

This audit establishes the original client helper and the listed ordinary
rounding fixtures. It does not re-audit a server counterpart, altered
floating-point control modes, off-lattice cosine precision, nonfinite
inputs, another coordinate-width ABI or every caller's later rounding.
The four lattice fixtures do not distinguish every possible error in
off-lattice scaling/interpolation. No runtime oracle or implementation
comparison is claimed.
