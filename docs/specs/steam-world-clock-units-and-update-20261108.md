# Steam World clock units and update order, 2026-11-08

FINAL / STABLE / HELD. This is a new code-free original-source contract. Previously held artifacts remain unchanged.

## Supported evidence and exposure

Supported original: C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. Independently rechecked SHA-256: 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. PE Machine is x64 0x8664, ImageBase 0x140000000, TimeDateStamp 0x5D93AB0D. All addresses below are RVAs.

Only the standalone source-author policy, filename-selected original-image routines, original native/reference metadata and the identified executable were inspected. No replacement implementation, SDK/dependency source, project history/specifications/tests, owner output/capture, implementation memory or held handoff artifact was read for this assignment. No prohibited exposure occurred.

The worker input, Host cap/substep calls, World-to-clock call and clock arithmetic/branch anchors were checked directly against the supported image. Constant values below were read from its native data. The contract concerns ordinary valid objects and finite clock values unless a boundary is explicitly discussed.

## Host worker: raw milliseconds and pacing

The worker at 0x83E60 initially samples the 32-bit millisecond clock through timeGetTime. While the Game active byte at +0x2068 is nonzero, it calls the client-state test at 0x3E2D0. That test returns true when the selected peer passes its validity test and differs from the stored local-host destination.

When that test is false, the worker takes a new clock sample. At 0x83EBB it subtracts the retained previous start sample using 32-bit integer arithmetic, then passes those elapsed bits directly to Host update 0xF5520 at 0x83EC6. The Host pointer is Game +0x6E8. There is no multiplication, division, seconds conversion, floating-point conversion or gameplay-rate factor between this millisecond subtraction and the Host call.

The retained baseline for the next ordinary Host call is this pre-update sample. A separate sample after the Host call measures execution duration only for pacing. If that unsigned duration is less than 32, the worker sleeps for the remainder of 32 milliseconds; equality or a larger duration does not take that sleep. Thus the next elapsed interval includes prior update execution and any intervening sleep. A fixed actual tick interval is not guaranteed.

When the client-state test is true, this worker sleeps for 33 milliseconds and skips Host update. That branch does not advance the retained Host start baseline. Consequently, the next eventual Host call may include the skipped interval before the Host cap applies. On worker exit it reaches Host reset; that exit is not an extra clock-advance call.

The subtraction is 32-bit clock arithmetic. Its result is supplied to a signed 32-bit Host parameter. This audit does not treat pauses spanning unusual signed-range boundaries, timer behavior or scheduler behavior as ordinary positive intervals.

## Host cap and World-step inputs

At 0xF556D/0xF5570, Host update chooses the signed minimum of its input and 1000. It does not impose a lower bound of zero. An ordinary positive elapsed interval above 1000 is truncated to 1000, with no catch-up of the discarded excess established here.

After its earlier Host work, it locks and updates its embedded World at Host +0x08. The step partition uses integer arithmetic:

- If the capped input is at most 16, it makes no full-step loop call and converts that input directly to binary32 for the final World call.
- If it is greater than 16, it makes enough full calls with binary32 16.0 to leave an integer remainder from 1 through 16. It then converts that remainder to binary32 and makes the final call.
- Therefore an exact positive multiple of 16 retains a final 16.0 call; it does not produce an extra zero remainder call. Zero input still produces the final zero-input World call.

For example, 16 yields one 16.0 call; 17 yields 16.0 then 1.0; 32 yields two 16.0 calls; 1000 yields sixty-two 16.0 calls then 8.0. The proved full-step and final-step call sites are 0xF6917 and 0xF692D, both targeting 0x2DA6C0. Integer-to-binary32 conversion uses the signed integer value. Ordinary capped nonnegative inputs up to 1000 are exactly representable.

No factor is applied to these step values before the World call. On this route their units remain milliseconds. A negative signed input is not normalized by the examined cap/partition; broader abnormal elapsed behavior is outside the ordinary interval contract.

## Independent viewed World route

The independently proved viewed caller at 0x134420 constructs an integer nanosecond-valued timestamp from the high-resolution counter and frequency using quotient and remainder arithmetic. It subtracts the retained previous timestamp. At 0x134536 it converts that signed difference to binary64, divides it by binary64 1,000,000,000 at 0x13454C, then multiplies by binary64 1000 at 0x134550. It calls the viewed Game update 0xA5A50 at 0x134558. Native argument flow confirms binary64 milliseconds despite an inaccurate narrowed expression in one decompilation display.

The viewed update caps its incoming binary64 value against 1000.0. When its World identifier and World-string comparisons admit stepping, it repeatedly passes binary32 16.0 at 0xA7328 while the binary64 remainder is strictly greater than binary64 16.0. It subtracts 16.0 in binary64 after each full call. The final remainder is converted from binary64 to binary32 and passed at 0xA734C. This route also preserves an exact final 16.0 remainder.

These viewed calls supply the World pointer loaded from Game +0x08. The Host route supplies the embedded object at Host +0x08. This establishes their separate owner/address expressions, not a universal aliasing or configuration relationship between those World objects. The high-resolution caller's milliseconds conversion is distinct from the raw Host worker's direct millisecond input.

## World-to-clock forwarding

At the start of 0x2DA6C0, the incoming binary32 step is preserved and passed unchanged to clock updater 0x2E2E80 at 0x2DA728. There is no additional scaling or binary64 conversion before that call. The clock operation precedes the subsequent ordinary World work in this routine.

The discovered original direct-reference evidence gives 0x2DA6C0 as the direct caller of 0x2E2E80, and gives the Host and viewed call sites above as direct callers of 0x2DA6C0. These are bounded discovered-call facts, not a universal caller census.

## Clock fields, constants and precision

The updater reads/writes the following World fields:

- +0x440: a 32-bit floating-point accumulator.
- +0x08: a 32-bit day counter.
- +0x0C: a 32-bit floating-point time-of-day scalar.
- +0x210: the length field of the World string beginning at +0x200.
- +0x358: the byte controlling the crossing-reset branch.

The World string at +0x200 is populated by the original World initialization/configuration path at 0x2B5F20. Calling it the World name/string here does not establish every configuration writer.

Supported image values read by this path are:

| Native RVA | Width/encoding | Value |
| --- | --- | --- |
| 0x4ACA34 | binary32, 0x41800000 | 16.0 |
| 0x42E514 | binary32, 0x3A83126F | nearest binary32 to 0.001 |
| 0x42E524 | binary32, 0x3D4CCCCD | nearest binary32 to 0.05 |
| 0x42E598 | binary32, 0x41200000 | 10.0 |
| 0x42E5BC | binary32, 0x42C80000 | 100.0 |
| 0x438028 | binary32, 0x4CA4CB80 | 86,400,000.0 |
| 0x437FC8 | binary64 | 16.0 |
| 0x438030 | binary64 | -16.0 |
| 0x437FD8 | binary64 | 1000.0 |
| immediate store 0x4BF73140 | binary32 | 32,400,000.0 |

The clock updater's accumulator, multiplier, addition and wrap subtraction use scalar binary32 instructions. They are not binary64 accumulations rounded only at the end, and no fused multiply-add is used in the shown sequence. The viewed outer elapsed calculation and remainder subtraction use binary64 as described separately. Global floating-point control modes and any runtime modification of the referenced data were not audited.

## Actor predicate and scalar adjustment before clock arithmetic

The updater scans the current World entity list at +0x40. An actor qualifies only when its byte at +0x60 equals zero, its byte at +0x68 equals 0x58, and its binary32 scalar at +0x180 is strictly greater than zero. The native comparison does not admit an unordered value as positive.

One boolean records whether any actor qualifies. It is not a count, and multiple qualifying actors do not repeatedly multiply the clock rate. No actor-purpose or gameplay-state name is inferred from these operand values.

For a qualifying actor equal by pointer to World +0x448, the updater also adjusts that actor's +0x180 scalar before advancing the World clock:

1. Obtain a binary32 reference value from 0x5FAC0.
2. Multiply the binary32 step by the binary32 0.001 constant.
3. Multiply that rounded product by the binary32 0.05 constant.
4. Multiply the reference value by the rounded product.
5. Add the actor's current +0x180 scalar and store the binary32 result.
6. Obtain a fresh reference value from 0x5FAC0 and compare. If the stored scalar is strictly greater, call 0x5FAC0 again and store that third returned value as the clamp result.

This preserves the native multiplication order. A rearrangement beginning with reference-value times step can produce different binary32 rounding. Equality does not take the clamp branch. Other qualifying actors set the boolean without this local-pointer scalar adjustment. The complete computation performed by 0x5FAC0 is outside this clock contract; it is a data-dependent reference-value provider, not an assumed constant.

## Accumulator, multiplier and empty-name fallback

After the actor scan, the updater first adds the original binary32 step to World +0x440 and stores the rounded binary32 sum, at 0x2E2F83/0x2E2F8B. This accumulator does not receive the 10 or 100 multiplier.

Next, if any actor qualified, the updater multiplies the original step by binary32 100.0; otherwise it multiplies by binary32 10.0. It adds that rounded product to the current World +0x0C and stores the rounded binary32 result, at 0x2E2FA9/0x2E2FAE.

On the Host route the increment product applies a factor of 10 or 100 to the admitted millisecond step, followed by the separate binary32 addition; this factor is not applied to the Host's input beforehand. World +0x440 accumulates admitted stepped input, subject to capping, skipped updates and binary32 rounding; it is not proved to equal all elapsed wall time.

The updater then tests World +0x210. If the string length is zero, it overwrites World +0x0C with binary32 32,400,000.0 at 0x2E2FBD. This is an assignment on each call, after the ordinary scalar increment. A nonempty name/string skips that override. The earlier actor adjustment and +0x440 accumulation are not undone by the fallback.

Thus a zero input does not establish a side-effect-free clock call: reference clamping, the empty-name assignment, or processing an already out-of-range time scalar can still matter.

## Strict wrapping, repeated crossings and reset gates

After any fallback, the updater compares World +0x0C with binary32 86,400,000.0. For ordinary finite values it returns unless the scalar is strictly greater. Equality remains at the upper boundary without incrementing the day.

For each admitted crossing it performs these actions in order:

1. Increment the 32-bit World day counter at +0x08.
2. Subtract binary32 86,400,000.0 from World +0x0C and store the rounded binary32 remainder.
3. Read World +0x358. If that byte is zero, call 0x2D2EB0, then 0x2C34C0, then append a 32-bit zero-valued record to World +0x160 and update its count at +0x168.
4. Reload the current time scalar and repeat only if it is still strictly greater than the period.

Day increment precedes remainder storage; both precede the reset calls and event append. A nonzero role byte skips those reset/event operations but does not skip the day increment or time subtraction. That role byte is tested within each crossing.

The loop permits multiple crossings in one call; it does not cap the day increment at one. For an exactly represented starting scalar of twice the period, one subtraction leaves equality, so only one crossing occurs. Three periods similarly leave one period after two crossings. These examples express the strict boundary; arbitrary decimal division or a single modulo operation does not describe all binary32 rounding behavior.

There is no reverse-wrap branch that decrements the day for a negative time scalar. Negative, nonfinite, overflow, exception and invalid-memory cases beyond the stated ordinary finite behavior were not generalized into additional policies.

The qualifying-actor scan and multiplier selection occur once before this crossing loop. They are not recomputed after every crossing reset. Each crossing's record production is synchronous; external processing of those records is a separate phase.

The directly reached cleanup at 0x2D2EB0 destroys/unlinks entities selected by nonzero actor byte +0x60, rebuilds the identifier lookup from survivors and clears the World list at +0x50. The reset at 0x2C34C0 clears the World collections at +0x20 and +0x10 and resets selected local-player-owned records/collections. Full business meanings of those states are outside this clock audit.

The enclosing World step has a later, separate role-zero check of World +0x160. On encountering a zero-valued record it invokes 0x2D2EB0 and 0x2C34C0 again, at 0x2DB703 and 0x2DB70B, then breaks that scan. Therefore the direct reset pair inside each crossing is not a universal count of reset calls in the whole World step. The later scan breaks on its first zero record rather than applying that later pair once per zero record.

## Limits and original evidence

No universal census of clock writers, World owners, role-byte writers, configuration changes or concurrent clock access was performed. Network or other explicit assignments to day/time are not replaced by this stepping contract. No claim is made that both owner routes necessarily advance the same World, that their calls are synchronized by this evidence alone, or that their elapsed sources provide fixed scheduling or display timing.

Other actor purposes, a complete reference-value formula, floating-point environment changes, corrupted/aliased objects, arithmetic failure paths, nonreturning cleanup and untraced indirect callers remain unresolved. No replacement algorithm or implementation guidance is supplied.

Original routines: 0x83E60, 0x3E2D0, 0x3BE00, 0xF5520, 0x134420, 0xA5A50, 0x2DA6C0, 0x2E2E80, 0x5FAC0, 0x2B5F20, 0x29A130, 0x2D2EB0, 0x2C34C0 and 0x112F00. Original native instruction/reference metadata, the successful original viewed-update retry, native constants and the supported executable substantiate the call/arithmetic anchors.

The implementer must read this artifact completely before using its facts. After final full readback the author holds all writes.
