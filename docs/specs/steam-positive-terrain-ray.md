# Steam positive-limit terrain ray

Stable native-only behavioral checkpoint. This covers **0x2d1d20** as
called at **0x8b26d**: float limit60, first flag0, second flag1. It is a
cell-boundary traversal, not a succession of equally spaced point probes.
Sources are the original Steam instructions and independent specifications;
no replacement implementation or runtime capture was inspected.

## 1. Inputs and result

The first four arguments are World, a pointer to three signed64 fixed
coordinates, a pointer to three float32 direction components, and float32
maximum ray parameter. One block is65536 fixed units. Arguments5 and6 are
the two byte flags. Native **0x8b24b–0x8b26d** supplies0 and1 in their
respective stack argument slots and float60, word0x42700000 at0x42e7b8.
The further stack slot written by that caller is not read by this helper.

For this caller the origin is the stored look-at point, and the direction
is the stored unshaken camera/world forward basis. These are retained
inputs, not a new eye-origin ray or current-frame camera calculation.
See [frame order §8.2](steam-gameplay-frame-order.md#82-initial-working-point-and-the-terrain-scalar)
for their fields and the returned scalar's subsequent use.

The helper does not normalize direction. Its result is a ray parameter;
it has distance-in-blocks meaning for a unit direction. The finite
positive-limit path returns **min(maximum, traveled)**, instructions
**0x2d221e–0x2d2223**. An unobstructed capped ray returns60. An unavailable
or obstructed initial cell returns0. It does not return a negative
no-hit sentinel or subtract a safety skin.

## 2. Initial cell and coordinate widths

Traveled is float32 zero and the crossing counter is signed32 zero.
The initial block tuple comes from **0xfd80**. Its output components are
signed32; the input fixed components remain signed64. For coordinates
whose mathematically floored block index fits signed32, this produces
mathematical
floor(fixed/65536), including negative fractional coordinates.

The precise narrowing order matters outside that range. For each
negative fixed component, first obtain its signed64 quotient by65536
with truncation toward zero. Narrow that quotient to signed32, sign
extend it, and multiply by65536 for comparison with the original full
fixed component. If unequal, decrement the narrowed signed32 quotient.
If equal, retain the quotient's low32 bits. Nonnegative components use
the quotient's low32 bits directly. These operations are visible at
**0xfd8b–0xfe54**; the negative comparison uses sign extension of the
already narrowed quotient, not the original signed64 quotient.

Thus fixed−1 yields block−1 and fixed−65537 yields block−2. The exact
negative fixed value **−140737488420864**, representing block−2147483649,
yields signed32 block**2147483646**. Merely taking mathematical floor
and then narrowing would instead yield2147483647. This is a native
large-coordinate quirk, not a reason to change ordinary negative floor.

From the signed32 block X/Y, obtain the signed32 zone key by mathematical
floor division by64 on each axis. For example block−1 and−64 both use
zone−1; block−65 uses zone−2. Initial lookup is in **World+0x460** through
**0x12700**. A missing entry, identified against the map sentinel at
World+0x468, gives a null zone; a found entry's zone pointer is at+0x18.
This is a lookup of existing loaded data; the ray does not load a zone.

## 3. Cell admission and effective data

At each reached cell, a null zone terminates the ray at its current
traveled parameter. Otherwise read the effective six-byte block with
**0xeb90**, using the maintained signed32 block X,Y,Z. The Z stack
argument is explicitly supplied at **0x2d1e69–0x2d1e81**. Do not replace
this read with a raw stored-column byte or a mesh-light-volume read.
The effective accessor's implicit water/air and below-column behavior
is specified in [the block store](worldgen-structures.md#3-the-block-store).

For the actual flags0,1, continue only if both conditions hold:

- Type byte+4 is **0 or2**, equivalently its bitwise AND with0xfd is0.
- Flags byte+5 has **bit0x40 clear**.

Instructions **0x2d1e86–0x2d1eae** implement those conditions. The second
flag enables the0x40 test. The first flag0 selects continuation through
the cells satisfying that test, rather than through their complement.
No other type is passable here. Bit0x80 does **not** override0x40, and
does not itself obstruct an otherwise passable cell. An air or water
cell with flags0x40 or0xc0 therefore stops this ray; flags0x80 alone do
not. RGB and the fourth block byte do not enter this decision.

There is no actor, static-object model or CUB geometry test in this
helper. Such separate targeting passes do not change this terrain
walk's cell predicate.

## 4. Boundary candidates and float precision

Every admitted advance recomputes the current fixed ray point from the
**original fixed origin**, separately on all three axes. Multiply
direction by the current traveled parameter in float32, multiply that
float32 result by float65536, truncate toward zero to signed64, then add
the original signed64 fixed component. The instruction sequence is
**0x2d1ec1–0x2d1f3b**. Do not accumulate a previously rounded point or
refloor this reconstructed point to replace the maintained cell tuple.

Each advance starts with candidate increment **float10** and selected
axis **X**. Visit axes in **X,Y,Z order**. An axis participates when its
float32 direction-component square is at least the stored float at
**0x42e50c**, word**0x358637bd**, approximately
**0.0000009999999974752427**. This is a squared-component comparison,
not a comparison against an independently rounded absolute threshold.

For a participating positive component, its boundary is the maintained
block index plus1, computed in signed32 before sign extension, then
multiplied by65536. For a nonpositive component, sign extend the current
signed32 index and multiply by65536. Subtract the reconstructed fixed
point component using signed64 integer arithmetic.

The conversion sequence for this boundary difference is significant:
convert signed64 difference to float32; divide by the direction
component in float32; truncate that quotient toward zero to **signed64**;
convert this integer back to float32; then multiply by float**1/65536**.
The resulting float32 is that axis's candidate increment. The native
X sequence is **0x2d1f50–0x2d1f84**, Y uses
**0x2d1fb6–0x2d1fdd**, and Z uses **0x2d2012–0x2d2039**. Removing the
intermediate integer conversion changes the contract.

Accept a candidate only when **strictly smaller** than the best current
increment. Equal candidates keep the earlier axis. If none improves
the initial10, the chosen increment is still10 and the selected axis
is stillX, even if X did not participate. There is no infinity default,
zero-direction special case, epsilon increment or simultaneous-axis
corner step. The relevant constants are float10 at0x42e598
(word0x41200000), float1/65536 at0x42e510 (word0x37800000), and
float65536 at0x42e5f8 (word0x47800000).

## 5. Advancement, zone transitions, budget and exit order

The current effective block is read and its predicate evaluated before
the crossing-budget test. An admitted advance requires fewer than501
previous crossings; the native signed comparison is against500 at
**0x2d1eb4–0x2d1ebb**. Increment the crossing counter once for that
advance. At most **501 cell-index changes** are permitted. If all
intermediate cells remain available and range has not ended, the cell
after the501st change is still queried before budget termination: up
to502 effective-block reads. Zero-length advances consume this budget.

Change **only the selected signed32 block index**, by+1 if its direction
component is positive and−1 otherwise, including zero. These are32-bit
integer changes; positive-boundary index+1 also occurs before widening.
The native operations retain the low32 bits on overflow. Other indices
remain unchanged, even when multiple boundaries share the same parameter.
This occurs at **0x2d204b–0x2d2060**.

Next recompute zone X/Y by floor division of the maintained block X/Y
by64. Compare that new pair with the current zone's own signed32 header
coordinates at **zone+0x10/+0x14**. If they differ, look up the new pair
in the same loaded-zone map; retain a null zone if absent. A Z-only
change does not require a different horizontal zone. The comparison
and lookup occupy **0x2d2060–0x2d21d8**.

**Only after that zone resolution**, add the selected increment to
traveled in float32. If traveled is now at least the positive maximum,
return the smaller of the two immediately. Otherwise begin the next
cell query, which may terminate on the newly null zone or new obstructing
cell. Thus a newly entered cell exactly at the limit is **not queried**,
although the preceding zone-key resolution has already happened. This
order is explicit at **0x2d21cb–0x2d21e4**.

Obstruction, missing zone, budget exhaustion and range completion all
use the same scalar return. No output distinguishes those reasons.

## 6. Finite arithmetic references

These are derived examples, not native runtime captures. All unspecified
cells are effective air with flags0 in available zones.

- Origin fixed(16384,32768,32768), direction(1,0,0): initial cell(0,0,0)
  is tested first. Its next X boundary gives parameter0.75. If cell(1,0,0)
  obstructs, return0.75. If every reached cell is clear through the cap,
  return60.
- Origin fixed(65536,32768,32768), direction(−1,0,0): first test cell(1,0,0),
  then advance to cell(0,0,0) with increment0. A block in either of these
  two cells returns0. No negative-facing boundary bias is added.
- To isolate tie order, use the intentionally unnormalized helper input
  direction(1,1,0) and fixed origin(32768,32768,32768). X and Y tie at0.5.
  The next queried cell is(1,0,0) at0.5. If clear, a zero Y increment then
  reaches(1,1,0), still at0.5. Cell(0,1,0) is not visited. A point sampler
  or a traversal stepping both axes at once misses that intermediate
  native cell. This example does not assert that the camera produces an
  unnormalized basis.

## 7. Relation to camera traversal and audit limits

The camera helper **0x2a0d40** was compared independently: it uses the
same initial0xfd80 conversion, effective0xeb90 read, float candidate
constants and integer-quantized candidate metric, strict X/Y/Z selection,
501-crossing budget, zone-key transition and scalar cap. Its admission
tests **only type0/2**, not the block's0x40 flag. The terrain helper here
has the additional two used byte flags; they must not be silently
discarded by sharing the camera predicate. Camera behavior and its
separate outer guard/eye construction remain in
[camera §6](steam-gameplay-camera.md#6-orbit-eye-and-terrain-obstruction).

This checkpoint specifies the finite positive-limit flags0,1 path used
by0x8b26d. It does not newly specify negative-limit modes, complement
mode callers, nonfinite inputs or invalid float-to-integer conversions.
It does not add a ray normalization, range skin, alternative collision
shape or post-hit correction absent from this caller.
