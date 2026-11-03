# Alpha cloud noise: additive constant and zero-lattice value

This bounded consistency note checks the original Alpha client noise kernel
VA0x4c0ef0 (RVA0xc0ef0; image base VA0x400000). It leaves all earlier specs
unchanged. Evidence is the original executable/decompile and its constants;
no engine, owner data, captures or game execution was used.

## 1. Native hash constants and arithmetic widths

The additive constant in this kernel is **3523796237, word0xd208dd0d**. The
different decimal3524844813 printed in sky-and-clouds.md §A.5 is word0xd218dd0d
and is incorrect for this reached kernel. The native four-corner calculations
all use the same correct constant: the relevant sites are VA0x4c0fe7,
VA0x4c1034, VA0x4c1073 and VA0x4c10ba (RVAs0xc0fe7,0xc1034,0xc1073,0xc10ba).
Its native representation there is the modulo-2^32 equivalent of subtracting
0x2df722f3. This is a transcription discrepancy, not evidence for a different
cloud noise family.

The existing independent alpha-worldgen-random.md §3 correctly describes the
kernel. Its directly verified constants are:

| Operation or term | Constant |
| --- | --- |
| Y stride in a lattice index | 57, word0x39 |
| Left shift combined with the original index by XOR | 13 bits |
| Cubic coefficient of the mixed index | 60493, word0xec4d |
| Linear coefficient of the mixed index | 19990303, word0x0131071f |
| Additive term | 3523796237, word0xd208dd0d |
| Final integer mask | 0x7fffffff |

Corner indices and shift/XOR mixing are **32-bit**. The cubic, linear and
additive arithmetic also wraps to32 bits; this is not an unbounded or
unmasked64-bit polynomial. Masking afterward leaves a nonnegative31-bit
integer. The corner indexing/mixing is at VA0x4c0f1b–0x4c0f77
(RVA0xc0f1b–0xc0f77); the four polynomial/mask paths are at
VA0x4c0fc3–0x4c10bf (RVA0xc0fc3–0xc10bf).

The masked integer is converted to double, multiplied by exact double2^-30,
then subtracted from double1. The scale is double word0x3e10000000000000 at
VA0x702b20 (RVA0x302b20), not a float reciprocal or a division by2^31.
Double1 is at VA0x745df8 (RVA0x345df8). The cosine interpolation continues in
double as specified in the independent random note.

The kernel then **rounds its final interpolated result to float32 before
returning**. It reloads that already narrowed value into the floating return
register. Thus a wider return register does not expose an unrounded double
noise result. The narrowing/reload is at VA0x4c10e8–0x4c10f3
(RVA0xc10e8–0xc10f3). The later cloud caller's float store is consistent with
this return contract.

## 2. Exact zero-lattice arithmetic reference

For the double inputs(0,0), both integer lattice indices and fractions are
zero. The mixed base index is zero, and zero cosine-interpolation weights
select that base corner. Its masked hash is **0x5208dd0d =1376312589**.

The resulting double lattice value is exactly the rational
**−302570765/1073741824**, approximately **−0.2817909838631749**. Its ordinary
nearest-even float32 result is:

| Representation | Value |
| --- | --- |
| Float32 word | **0xbe9046e8** |
| Exact float32 decimal | **−0.2817909717559814453125** |

These are arithmetic results derived from the verified native constants,
normal zero-angle cosine/interpolation and the final narrowing. They are not
new runtime captures. They agree with the sister independent random note's
zero-lattice vector and with the cloud-placement note's rounded approximation
**N(0,0)≈−0.28179**. That approximation needs no correction.

Using the erroneous additive instead would yield approximately
−0.2827675463631749 before narrowing and −0.28276753425598145 as float32.
The two additives differ by0x00100000; after the31-bit mask their zero-corner
values differ by exactly1/1024 before float narrowing. Therefore the erroneous
decimal cannot be reconciled with the existing zero-lattice approximation by
changing float/double staging alone.

## 3. Scope and remaining limits

This closes the Alpha client kernel's constants, width/mask and float-return
consistency question. It does not re-audit every caller, the server executable,
star hash producers, the internal cosine implementation, nonfinite or
out-of-range coordinate conversions, or altered floating-point control modes.
In particular, the zero reference above is an instruction-derived ordinary
rounding reference, not an assertion that a new native runtime oracle was
collected. No previous cloud altitude/placement note was changed.
