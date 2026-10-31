# Steam once-per-frame linked world-space placement

Stable finite ordinary Pass B contract: **0x2a12b0** and the remaining
caller stages in **0x2b68e0**. This complements
[Pass A](steam-creature-drawn-interpolation.md); it is world-space
relationship placement, not skeletal mesh animation. Sources are native
instructions/constants and independent prose only. No replacement code,
tests, SDK, owner data or original-game execution were used.

## 1. Phase, records and arithmetic conventions

After every entry has completed Pass A, visit the World+0x40 registration
list again in existing order. For the current creature **C**, perform:

1. Relationship helper0x2a12b0 at0x2b6c5e.
2. The current ability0x91 placement, if admitted.
3. The current ability0x8c placement/cancellation, if admitted.
4. The current kind6/owner-ability0x76 placement, if admitted.

Then fetch the next node at0x2b760a. These are live state reads after
earlier stages; they are not four entry-time snapshots. C has no outer
HP, local-player, visibility, range or authority admission test. Internal
tests below apply to the specified resolved target. Later list entries
can observe earlier entries' writes. See
[visitation §4](steam-world-creature-visitation.md#4-once-per-frame0x2b68e0-has-two-separate-full-list-passes).

**T** below denotes an actor resolved by64-bit ID from World+0x450.
Lookup follows that map's signed64 key comparisons. A missing entry or
null mapped pointer is unavailable. The helpers do not replace lookup
with iteration by numeric ID or by mesh ownership.

|Field|Meaning here|
|---|---|
|+0x08|Actor ID,64 bits|
|+0x10/+0x18/+0x20|Raw signed64 fixed XYZ|
|+0x28/+0x2c/+0x30|Raw float pitch/lean/yaw in degrees|
|+0x34/+0x38/+0x3c|Float velocity XYZ|
|+0x40/+0x44/+0x48|Float movement intent XYZ|
|+0x4c/+0x50/+0x54|Float knockback XYZ|
|+0x60/+0x64/+0x68|Kind byte/race signed32/ability byte|
|+0x6c/+0x12c|Float ability timer/dodge timer|
|+0x80/+0x128|32-bit appearance flags/16-bit state flags|
|+0x84/+0x88/+0x8c|Float size XYZ, blocks|
|+0x174/+0x178/+0x17c|Raw float look XYZ, used as offsets by0x91|
|+0x180|Float HP|
|+0x1a0/+0x1a8/+0x9d8|Owner ID/linked ID/pet ID,64 bits each|
|+0x990/+0x998/+0x99c|Float step offset and two further pose values|
|+0xb10/+0xb18/+0xb20|Drawn signed64 fixed XYZ|
|+0xb34/+0xb38/+0xb3c|Drawn float angles|
|+0xb58|Float smoothing control|

All height expressions below evaluate in **float32, rounding after each
binary operation**. Parentheses give the order. A block offset becomes
fixed by float multiplication by65536 and truncation toward zero to
signed64. Add/subtract fixed components with low64 integer arithmetic.
Do not merge separately converted offsets into one floating sum before
truncation. Copies preserve the stored field words.

Constants:0.5 at0x4ac964 (word0x3f000000),0.25 at0x4ac95c
(0x3e800000),0.03 at0x42e6ec (0x3cf5c28f),0.2 at0x42e700
(0x3e4ccccd),0.75 at0x42e710 (0x3f400000),0.01 at0x42e51c
(0x3c23d70a),−0.5 at0x42e80c (0xbf000000), and65536 at0x42e5f8.
These are stored float values, not exact decimal reals.

There is also a **pre-scaled float fixed-unit constant at0x4725a0**,
word**0x44f5c28f**, value**1966.0799560546875**. Native converts this
float directly to signed64, yielding **1966 fixed units**. Call that
horizontal offset **J**. It is not a zero vector, not1966 blocks, and
is not rotated with an actor's yaw.

## 2. Helper0x2a12b0: ability0x80 owner route

This route has priority over all other helper routes, beginning at
**0x2a12cf**. If C's ability is0x80, resolve **C.owner+0x1a0** as T.
Require T available and HP>0, then either:

- T.state+0x128 bit0x02 is clear, and either T.flags+0x80 bit0x4000
  is set or T's ability is0x82; this is predicate0x5ef50(T).
- Or T.kind is6 and **C.petID+0x9d8 equals T.ID**.

The second alternative bypasses that predicate's flag/ability tests.
It still requires the available, positive-HP target. C's own HP is not
tested. On failure, clear **C.link+0x1a8** and set C.ability0, then return.
The cleared field is1a8 even though lookup used1a0; do not substitute
clearing the owner field. There is no timer or b58 reset on this failure.

On success, in order:

1. OR T.state bit0x10 and set T.ability0.
2. Put T.drawn XY exactly at C.drawn XY. For Z, add the fixed conversion
   of **(C.sizeZ×0.5)+(T.sizeZ×0.25)** to C.drawn Z. Copy C's three
   drawn angles to T's three drawn angles.
3. Independently put T.raw XY exactly at C.raw XY, and T.raw Z at C.raw Z
   plus the same ordered height expression's fixed conversion. Copy C's
   raw angles to T's raw angles.
4. Copy C.step+0x990 to T. If T.flags+0x80 bit0x02 is **clear**, zero the
   eight bytes at T+0x998, clearing both floats998/99c; otherwise retain
   them. Copy C's three velocities to T and zero T's three intent words.

No J offset applies here. This success path does not add state bit0x02,
reset the ability timer or write b58. Native route0x2a12cf–0x2a14b1.

## 3. Helper: remaining admission and shared writes

If C was not in ability0x80, proceed only when **C.race is0xbb or0xbc,
or C.ability is0x83**; otherwise return. Resolve **C.link+0x1a8** as T.
An unavailable target returns without clearing that field.

Form T's intent squared length as float **(X²+Y²)+Z²**. If C.ability
is not0x83 and this is **≥float0.001** (word0x3a83126f at0x42e514),
clear C.link and return, before the HP test. Otherwise require T.HP>0;
failed HP returns without another link clear. Native then repeats the
intent check; it has the same result for ordinary unchanged finite
inputs, again bypassed by ability0x83. These are **0x2a14b6–0x2a15b3**.

Before choosing a placement row, copy C.step to T.step, copy C's three
velocities to T, zero T's three intent words, and zero T+0x998's eight
bytes. This common prefix is **0x2a15b3–0x2a15e4**. It does not zero
knockback, b58 or the ability timer.

## 4. Helper placement rows and exact finish behavior

Let **h** be float(C.sizeZ×0.5), **q** float(T.sizeZ×0.25),
**u** float(T.sizeZ×0.5), and **v** float(T.sizeZ×0.2).
The table's Z expression is converted to fixed as §1 describes and
added separately to C.raw Z and C.drawn Z. Unless a row says otherwise,
T.raw XY is C.raw XY plus **J on each axis**, and T.drawn XY is C.drawn
XY plus J on each axis. No trigonometric rotation applies to J.

First apply predicate **0x5edb0(C)**: kind8 and race0xc2..0xc5. If true,
use the first row regardless of the later race table. Otherwise select
by C.race and, for the final two rows, C.ability. This outer predicate
does not itself admit C past §3's race/ability gate.

|Ordered choice|Raw and drawn Z offset in blocks|T ability|T timer+6c|Angle/state finish|
|---|---|---|---|---|
|0x5edb0(C) true|**(0.03−h)+u**|0x57|Retain|Copy C angles; add state0x02|
|Race0x10f|**(h+0.03)+q**|0x5b|Zero|Copy C angles; add state0x02|
|Race0x111|**(0.03−h)+u**|0x57|Retain|Copy C angles; add state0x02|
|Race0x112|**(0.03−h)+u**|0x5c|Zero|Copy C angles; add state0x02|
|Race0x113|**(0.03−h)−v**|0x5d|Retain|Copy C angles; add state0x02|
|Race0xdd or0xde|**(h+0.03)+u**|0|Retain|Preserve T angles; add state0x02|
|Race0x12b|**(h+0.03)+u** for raw; then drawn copies new raw tuple|0|Retain|Spin T's own yaw as below; add state0x02|
|Any remaining race, ability0x83|Raw **h**; drawn **h+0.03**|0|Retain|Copy C angles; add state0x02|
|Any remaining race, other ability|**(h+0.03)+q**|0x58|Retain|Copy C angles; return without adding state0x02|

The ability0x83 default row has an additional XY exception: **raw XY
copies C.raw XY without J**, while drawn XY still adds J to C.drawn XY.
Native performs a float multiplication by65536 after h even in the
raw h-only Z expression. Do not add the other rows'0.03/T-height terms.

For race0x12b, place raw XYZ first using its row. Multiply the **float
frame milliseconds by float0.01**, then add that rounded float to T's
existing raw yaw in float32, with no angle wrap. Copy T's own raw
pitch/lean/new yaw into its drawn angles, and copy its entire newly
placed raw XYZ tuple into drawn XYZ. This is not a copy of C.drawn
position/angles. Native yaw sequence **0x2a1a0b–0x2a1a34**.

“Copy C angles” means separately copy C.drawn angles to T.drawn angles
and C.raw angles to T.raw angles, not blend either set. The common
angle-copy block is0x2a16bf; the default0x58 route has its own copies
then returns early. The dd/de rows preserve both of T's angle sets.
State0x02 is ORed at0x2a1cea only for the specified finishes. Existing
other state bits remain. **No helper row writes b58**. Table branch
selection and writes are0x2a15e4–0x2a1cea; the race comparisons begin
at0x2a16fd,0x2a17e4,0x2a17f8,0x2a18d2 and0x2a19b1.

## 5. Caller ability0x91: look-vector relative placement

After the helper returns, at **0x2b6c63**, if C's then-current ability
is0x91 and C.link is nonzero, resolve that linked actor T. Require T
available and HP>0. For each component, multiply **C.raw look** by
float65536 and truncate that float result toward zero to signed64.
Add the resulting fixed offset to T.raw XYZ for C.raw XYZ, and to
T.drawn XYZ for C.drawn XYZ. Use the same unnormalized look components
for both tuples. There is no step/height/race offset in this route.

Failure leaves this route's fields alone; it does not cancel C's
ability or clear its link. Success does not copy angles, change timers,
zero movement or write b58. Processing continues to the later stages.
Native0x2b6c63–0x2b6e5d.

## 6. Caller ability0x8c: focus placement and cancellation

At **0x2b6e5d**, if C's current ability is0x8c, require its raw timer
**strictly below float(duration0x4f220(C))**, nonzero link, available
linked T, and T.HP>0. The duration helper call is0x2b6e71. All tests
precede the successful placement below. Equality is a failure.

### 6.1 Successful placement and conversion order

Query the float3 offset **B=0x65040(T.race)** into a local vector; this
is a race-constant lookup, not a resource-bounds query or actor reset.
Its complete table is §7. Form float **Zbase=B.z+(T.sizeZ×0.75)**, with
separate multiplication then addition. Convert B.x, B.y and Zbase
individually to fixed by multiplication by65536 and truncation.

From T's **raw yaw**, form radians by widening that float to double,
**multiplying by doubleπ, dividing by double180, adding doubleπ/2**,
then narrowing to float. Double constants are0x42e758,0x42e778 and
0x42e740 (π/2 word0x3ff921fb54442d18). This order differs from0x17840's
initial float divide. Call cosf/sinf of that float angle.

Form float **R=min(T.sizeX,T.sizeY)×−0.5**. Convert the radial X term
by **float(cos(angle)×R), then float times65536, then trunc64**; Y uses
sin in the same order. The finite radial Z term is zero. Independently
form the height correction as float
**((T.sizeZ×0.5)−(C.sizeZ×0.5))+0.01**, then multiply by65536 in
float and truncate to signed64.

For C.raw XYZ, start with T.raw XYZ, add B.x/B.y's fixed conversions
and the separately converted radial X/Y respectively. For Z, subtract
the fixed height correction, then add Zbase's fixed conversion. Repeat
for drawn XYZ using **T.drawn** as the origin but the **same raw-yaw
radial/race/height terms**. Do not rotate B.x/B.y, merge the independent
truncations, or derive the raw tuple from the drawn one. This arithmetic
begins with0x65040 at0x2b6f90 and extends through the placement stores.

Then copy T's **raw** angles into C's raw angles and T.step into C.step.
Zero C+0x998's eight bytes, all three velocities, all three intents,
all three knockback floats and dodge timer+0x12c. **Do not copy drawn
angles here**: C's drawn angles remain those left by earlier stages.
This success leaves C.ability0x8c, its timer, link and b58 unchanged.
Continue directly to §8, bypassing the failure block.

### 6.2 Any failed0x8c test

The failure block at **0x2b7257** writes a diagnostic containing literal
`FOCUS `, C's linked ID and a newline, then sets **C.ability0**. If that
retained nonzero linked ID now resolves to an actor with HP>0, copy that
actor's **raw XYZ only** to C.raw XYZ, without offsets. Missing/dead
targets leave C.raw XYZ as it was. There is no draw-position/angle snap,
link clear, timer reset, movement reset or b58 write in this failure
block. The final kind6 stage can still override its results afterward.

## 7. Complete0x65040 race-offset table

All unlisted races return(0,0,0). X is0 for every race. Y is0 except
race0xd2, where it is word0xbe99999a (approximately−0.3). Z is:

|Steam race IDs|Z float word|Decimal shorthand|
|---|---|---|
|0x1a,0x1e|0xbf666666|−0.9|
|0x21|0xbe4ccccd|−0.2|
|0x4a,0xc8,0xc9,0xd2|0xbe99999a|−0.3|
|0x4b,0x117|0x3dcccccd|0.1|
|0x97|0xbeb33333|−0.35|
|0xd3|0xbf19999a|−0.6|
|0xd5|0x3e800000|0.25|
|0xd7,0x127|0x3ecccccd|0.4|
|0x106|0xbf800000|−1|
|0x107|0xc0000000|−2|
|0x10e|0xbfcccccd|−1.6|
|0x125,0x126|0xbecccccd|−0.4|

Words are authoritative float32 values; the decimal column is shorthand.
These numerical race IDs and constants are Steam-specific native behavior,
not a cross-edition catalog mapping. Source0x65040–0x65190.

## 8. Final caller stage: kind6 following an owner in ability0x76

At **0x2b738c**, if C.kind is6, resolve **C.owner+0x1a0**. Require
that owner available and its **current ability0x76**. There is **no
owner HP test**, no pet-ID equality test and no range test in this stage.
Failed admission performs no fallback link clear or position change.

Compute float height offset
**((C.sizeZ×0.5)−(owner.sizeZ×0.5))+0.01** and convert it to fixed.
Put C.drawn XY at owner.drawn XY and C.drawn Z at owner.drawn Z plus
that fixed offset; copy the owner's drawn angles. Independently put
C.raw XY at owner.raw XY and C.raw Z at owner.raw Z plus the same
ordered expression's conversion, then copy owner.raw angles.

Then, in order: set **C.b58=0 at0x2b75ab**; copy owner.step to C.step;
copy the single owner float+0x998 to C+0x998; set C+0x99c to
**float(float(owner+0x99c × owner.sizeX) / C.sizeX)**, multiplication
before division; zero C's three velocities and three intents. There
is no zero-size division fallback in the traced ordinary path. This
stage does not zero C's knockback or dodge timer and does not change
C's ability, ability timer, link, kind or drawn-look fields.

The owner-ability comparison is0x2b7435 and the stage ends with the
next-link load0x2b760a. This is the **only b58 store in Pass B**, and
the only b58 write in all of0x2b68e0. Pass A only tests it.

## 9. Scope and remaining limits

This closes the finite ordinary relationship/caller routes above,
including their gates, field effects and ordered arithmetic. It adds no
simulation step, draw visibility gate, network award or asset geometry.
The tables assume ordinary distinct linked actors and stable ID-map
entries during a single invocation. Native has no explicit self-link
rejection, but pathological self-aliasing, concurrent erasure, invalid
float-to-integer conversions and nonfinite input behavior were not
expanded into additional contracts here. The broader producer/lifetime
of smoothing control b58 remains distinct from this complete pass audit.
