# Steam once-per-frame drawn-state interpolation

Stable native-only contract for **Pass A of0x2b68e0**, including its
raw-state side effects and the boundary with Pass B. This specifies
finite ordinary inputs; it does not supply replacement animation data.
Sources are native instructions/constants and existing independent prose.
No replacement code, tests, owner data or game execution were used.

## 1. Invocation, list membership and fields

The ordinary Game call at0xa7e80 follows the **entire completed World
substep batch**, before candidate building and local control. Its input
is the full capped double frame interval **narrowed to float32**, not
the last substep remainder. The routine widens that float back to double
for each0xa7a0 call. It does not recover the original double fraction.
There is no new cap or elapsed>0 admission test inside this pass.
See [frame-order §7](steam-gameplay-frame-order.md#7-drawn-position-pass-is-after-world-stepping-and-before-local-input).

Pass A visits the World+0x40 registration list in its existing order,
including the local player, remote players and dead entries. No HP,
visibility, distance, frontend or ordinary-UI test admits individual
entries. Its first branch reads the current entry's float+0xb58.
Pass A finishes for **all** entries before Pass B begins; exact iterator
frontiers remain [World visitation §4](steam-world-creature-visitation.md#4-once-per-frame0x2b68e0-has-two-separate-full-list-passes).

| Meaning | Raw/current fields | Drawn fields |
|---|---|---|
|Signed64 fixed XYZ|+0x10/+0x18/+0x20|+0xb10/+0xb18/+0xb20|
|Float pitch/lean/yaw, degrees|+0x28/+0x2c/+0x30|+0xb34/+0xb38/+0xb3c|
|Float look XYZ|+0x174/+0x178/+0x17c|+0xb4c/+0xb50/+0xb54|

Other inputs/outputs are hostility kind byte+0x60, raw ability timer
float+0x6c, step offset float+0x990 and smoothing-control float+0xb58.
One block is65536 fixed units.

## 2. Nonpositive+0xb58: exact snap branch

At **0x2b6994–0x2b69a0**, a finite smoothing control **≤0** chooses
the branch at **0x2b6b1c–0x2b6b56**. Copy the three raw signed64
positions directly to their drawn counterparts, then copy the three
raw float angle words directly to their drawn counterparts. There is
no coordinate conversion, interpolation factor or angle-turn helper
in these copies.

This branch does not add to+0x6c and does not apply the positive branch's
negative-step-offset clamp. It still proceeds to the common look update
in §5. Zero elapsed time does not suppress the snap.

## 3. Positive+0xb58: quantized XYZ blend and additional Z correction

Only **the sign** of+0xb58 matters to this branch. Its positive magnitude
does not divide the elapsed time, scale the factor or normalize a60-ms
window. Invoke **0xa7a0(double(float frameMS), rate)** at0x2b69b2,
with rate word**0x3ca3d70a**, approximately0.02, from0x42e6e8. Use
the existing precisely specified0xa7a0 helper, rather than a replacement
linear/exponential easing rule. Its result is float32.

At0x2b69bd, add float+0 to that returned float. Call the resulting
float **C**. Multiply C by float65536, round that product to float32,
then truncate it toward zero to **signed64 K**, at
**0x2b69c2–0x2b69ca**. There is no extra clamp on C or K.

Capture raw XYZ. Subtract the three **old drawn** components from those
raw values in64-bit fixed arithmetic, retaining low64 bits. For each
axis independently, multiply that signed64 difference by K, retain
the low64 product and interpret it as signed64. Divide this signed
product by65536 **toward zero**, then add the quotient to the old
drawn component, retaining low64 bits. Native uses the negative-product
65535 correction followed by arithmetic shift16. This first XYZ stage
is **0x2b69d0–0x2b6a73**. Do not convert the absolute positions to float
or use a floating XYZ lerp in place of this integer stage.

Z then receives a **second, different correction** at
**0x2b6a7a–0x2b6a97**. Re-read raw Z and subtract the **already updated
drawn Z** in signed64 fixed units, retaining low64 bits. Convert this
whole fixed-unit residual to float32, multiply by unquantized float C,
round in float32, truncate toward zero to signed64, and add that integer
to the newly drawn Z, retaining low64 bits. There is **no division by
65536** in this second correction: the residual already represents
fixed units. X/Y do not receive it.

Thus uniform XYZ interpolation is not the native operation. Also,
using C in the first stage rather than K/65536 changes its integer
quantization; using K/65536 in the second stage changes its float rule.

## 4. Positive branch angles and raw-state side effects

After both positional stages, call **0x17840(old drawn angle, raw angle,
C)** for pitch, lean and yaw, in that order, at
**0x2b6aaf/0x2b6acc/0x2b6ae9**. Store each returned float in its
corresponding drawn field. There is no angle normalization after these
stores. Section6 gives the exact angle helper's relevant arithmetic.

Next, at **0x2b6af6–0x2b6b00**, multiply C by float**32**
(word0x42000000 at0x42e7ac), then add that float product to the
**raw ability timer+0x6c**, with float32 rounding at both operations.
This increment is not frameMS and is not an update to a separate drawn
timer. It occurs only in the positive-control branch, even for dead
entries, independently of whatever the earlier World batch did.

At **0x2b6b09–0x2b6b13**, if raw step offset+0x990 is **strictly
negative**, overwrite its word with0. Zero and positive finite values
remain unchanged. This clamp is also confined to the positive branch.

Pass A **does not decrement, consume or overwrite+0xb58**. Treating its
positive magnitude as a timer fraction is unsupported by this function.
Network arming/reset facts remain in netcode-packets/netcode-roles;
the exhaustive lifetime of all other writers/countdown candidates is
not newly established by this bounded audit.

## 5. Look update common to both position branches

At **0x2b6b5c–0x2b6b77**, choose rate by the creature's **kind byte**,
not by pointer equality with the local player:

- Kind0: word**0x3d4ccccd**, approximately0.05, from0x42e524.
- Any other kind: word**0x3c23d70a**, approximately0.01, from0x42e51c.

Call0xa7a0 with the same widened float frameMS and that rate. Add
float+0 to its float return. For each look component, subtract old
drawn look from raw look in float32, multiply that difference by the
common returned factor in float32, then add old drawn look in float32.
Store the resulting float to+0xb4c/+0xb50/+0xb54. The native arithmetic
and stores are **0x2b6b7c–0x2b6bf1**. These three operations have no
normalization, vector-length test or angle wrapping.

This uses a new helper factor, not the positive branch's C or its
later C×32 timer increment. Both kinds of position branch execute it;
remote/dead kind0 players use the same0.05 rate as the local player.
The next-list-node fetch at0x2b6bb8 occurs before the final component
stores, which still target the old current creature.

## 6. Exact angle helper boundary0x17840

For each old/target angle, first divide its float degrees by float180
(0x42e7d4), widen that rounded result to double, multiply by doubleπ
at0x42e758, then narrow to float radians. Obtain float cosf and sinf
values and widen those values to double for the following products.

Compute **cos(target)×cos(old)+sin(target)×sin(old)** in double. If it
is strictly below the stored double threshold at0x42e820,
word**0xbfecccccc0000000**, return the **raw target float** immediately.
That threshold is exactly **−0.89999997615814208984375**, not the
nearest-double literal−0.9. This refines the shorthand in
creature-physics §7.3. The raw-target return does not depend on C.

Otherwise compute the double cross expression
**sin(target)×cos(old)−cos(target)×sin(old)**, narrow to float, clamp
first to maximum float1 and then minimum float−1, and call asinf.
Widen that float result to double. Separately widen float C, multiply
by double180 at0x42e778, then divide by doubleπ at0x42e758. Multiply
the widened asin result by that double gain, narrow the product to
float and add the old float angle in float32. Instructions
**0x17904–0x17972** establish the decision and final order.

This is not an atan2 shortest-angle lerp. For example, with finite
old0/target180, the dot fails the threshold and the helper returns180
even when C is0. Therefore a called zero-ms positive-control pass can
still snap such an angle and clamp a negative+0x990.

## 7. What Pass B owns, and what remains outside this contract

Only after every Pass A entry finishes does Pass B start again at the
registration-list head. Its **0x2a12b0 call at0x2b6c5e** owns relational
world-space pose/attachment state, rather than the above current-record
network blend. It resolves other actors through World+0x450 using
owner+0x1a0 in its ability0x80 route, or linked ID+0x1a8 on its other
admitted routes. Depending on that relationship it can place a resolved
other actor's **raw and drawn fixed positions**, copy raw and drawn
angles, copy/zero velocity/intent fields, change abilities/state flags,
or cancel an invalid link. Some branches therefore overwrite a result
that Pass A just produced, after all actors have had their first pass.

This helper is not wholly independent of elapsed time: a reached branch
at **0x2a1a0b–0x2a1a34** adds **float(frameMS×0.01f)** to a linked
actor's raw yaw before copying its raw angles/position to drawn state.
It is not the0.02 interpolation factor or a b58 countdown. Full
relationship/race admission and all attachment offsets are now specified
in [the linked-placement contract](steam-creature-linked-placement.md);
that isolated yaw fact is not a standalone trigger.

The rest of Pass B's caller also performs ability/link pose operations;
the later store **0x2b75ab** can set+0xb58 to0 in its own admitted
branch. That is separate from Pass A and from0x2a12b0 itself, which
has no+0xb58 store in the traced body. Complete finite ordinary Pass B
geometry is given in the linked-placement contract rather than this
**complete Pass A** contract.
No new birth/removal event or independent simulation step is implied.

## 8. Derived finite arithmetic reference

This is an arithmetic derivation, **not a native runtime capture**.
For float frameMS16, the precise0xa7a0 recurrence has1600 iterations.
For rate word0x3ca3d70a it returns C word**0x3e85ee64**,
**0.26158440113067627**. The quantized signed64 K is**17143**.

With positive+0xb58, old drawn XYZ all0 and raw XYZ all65536, the first
integer stage yields(17143,17143,17143). The extra Z correction is12658
fixed units, so final drawn XYZ is **(17143,17143,29801)**. Starting
raw ability timer0, its increment/result is float
**8.37070083618164**. With raw look components1 and old drawn look0,
the common factor/result is word**0x3f0067c3**,
**0.5015832781791687**, for kind0; for another kind it is
word**0x3e13c720**, **0.14431428909301758**. Angle outputs additionally
depend on §6 and the float trigonometric functions.

For a simple quantization-only check, if C is exactly0.5, raw fixed
XYZ all1001 and old drawn XYZ all0, the first stage gives500 on all
axes and the additional Z correction gives250: final(500,500,750).
Negative1001 gives(−500,−500,−750), showing truncation toward zero.
This isolated coefficient example does not assert a frame interval
whose helper return is0.5.

## 9. Derived regression distinguishing the two angle thresholds

This is independent arithmetic using the host UCRT's float cosf/sinf,
**not original-game execution or a runtime capture**. Use old angle
**1**, word0x3f800000, and target angle
**155.1580657958984375**, word**0x431b2877**. Following §6's float-divide,
double-π-multiply, float-narrow sequence gives radians words
**0x3c8efa35** and **0x402d502f** respectively.

|Input|cosf word|sinf word|
|---|---|---|
|Old angle|0x3f7ff605|0x3c8ef859|
|Target angle|0xbf684ff8|0x3ed71981|

Widen those float trig results to double, multiply the two cosine values
and the two sine values separately, then add in double. The dot is
word**0xbfecccccc4b69014**, approximately**−0.8999999849370419**. It is
strictly **above the nearest-double literal−0.9** and strictly **below
the native threshold−0.89999997615814208984375**.

Consequently native0x17840 returns the raw target angle at this pair.
With factor0 it still returns word**0x431b2877**. Substituting the
double-literal−0.9 threshold would instead take the smoothing branch
and, at factor0, return old angle1. The listed intermediate words make
the intended arithmetic boundary explicit independently of trigonometric
library last-bit variation.
