Original Steam final-height ordered arithmetic and conditional reference vectors.

This independent original-only contract covers the complete preparation owner at RVA 0x35EA0 and complete final-height owner at RVA 0x2B4450. It describes pairwise floating arithmetic, conditional provider-call order and literal synthetic reference cases. The reference cases are conditional mathematical calculations. They are not original runtime observations, a claim that an untraced provider produces the chosen words, or a closure of World/noise state.

The supported original authority is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, 5,757,952 bytes, AMD64 machine 0x8664, preferred image base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0. Its independently rechecked SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. All addresses below are RVAs in that image.

The full selected C exports and full original instruction streams of both owners were read anew. Their 1,165 instructions and 5,322 instruction bytes match the raw PE at every mapped RVA, with zero mismatches and no gaps. The complete eight-instruction, 33-byte cookie-check owner at 0x393030 was also read and byte-matched to establish the normal return boundary without assuming that a generic call preserves the scalar result. Full selected-owner byte correspondence preceded final interpretation. Original raw constant data and selected original call metadata supply the other evidence. An apparent void or missing arithmetic in a C export does not override the original floating result and instruction operands.

Exposure remains clean under the standalone docs/CLEANROOM_AUTHOR.md policy. No earlier held handoff was read or changed. No active AGENTS.md, memory, combined project history, implementation, exporter/extractor, SDK/dependency/reconstruction source, tests/results, owner data, captures or outputs was inspected. No implementation/runtime feedback or additional agent was requested or received. The exact requested destination was checked before research and did not exist; final creation uses exclusive new-file creation.

A 32-bit hexadecimal floating word in the reference cases identifies the exact IEEE binary32 encoding, independent of a printed decimal. All worked calculations assume round-to-nearest, ties-to-even at each binary32 operation and at each binary64 operation or narrowing, ordinary successful provider/guard returns, finite stated operands and in-range integer conversions. No vector relies on subnormal arithmetic, overflow, NaNs or floating exceptions. Exact rational values of the stated binary32 words were used to round each arithmetic result; host single rounding was separately cross-checked for these literal operations. No original executable was run to obtain these vectors.

For the prose arithmetic, each stated multiplication, addition, subtraction or division produces its own rounded binary32 result unless expressly called binary64. A separately prepared product is rounded before the subsequent multiplication consumes it. A complement means one subtraction from binary32 1, not an algebraic cancellation. Doubling means adding an operand to itself. A finite clamp or upper/lower selection is described at its actual position; it is not a universal NaN policy. The inline smoothing used in several branches has two separate products: multiply its input by 3 and then by the input; separately double the input by addition, multiply by the input and multiply by it again; subtract the latter product from the former. The selected instruction streams contain no fused multiply-add operation.

The final owner's inputs are World, signed low-32-bit X and Y, incoming binary32 base B, relief scalar H, scalars R, L, U, Q and r, an additional scalar F, a descriptor pointer D and a 32-bit mode. These labels distinguish operands and do not assign unproved gameplay meanings. F's input slot is never loaded by the selected final-height body. The descriptor reads have the following concrete widths and uses.

| Descriptor field | Width | Use in the selected final owner |
| --- | --- | --- |
| D+0 | 32 bits | Numeric category, with equality and unsigned mask-range tests |
| D+4 | 8 bits | Nonzero grouping gate in the variant-2/-3 coordinate hash preparation |
| D+8 | 32 bits | Variant selector |
| D+0xC | Signed 32 bits | Radius converted to binary32; some derived radii are truncated back to signed 32 bits |
| D+0x10, D+0x14 | Signed 32 bits each | Reference X/Y for the variant-2/-3 fixed-point displacement |
| D+0x18 | Binary32 | Stored squared-distance operand for variant radial tests |
| D+0x1C | Binary32 | Stored weight, including the distinct initial variant-2/-3 support comparison |

The selected body does not directly write D or World fields. It uses stack-local scalar state and directly reads the descriptor at multiple times. It has no directly visible World lock. It is not a demonstrated atomic snapshot of every input or provider state.

RVA 0x35EA0 copies the mode's low 32 bits and the coordinates' low 32 bits into retained registers. It prepares two separate stack objects, S beginning at its stack offset 0x90 and D beginning at offset 0x70. The selected preparation body performs scalar moves and argument assembly, not the numerical arithmetic of the called providers. Its exact ordinary call order is below. Each scalar return is copied as a four-byte value from the floating return register. All World arguments in this table use the retained original World pointer.

| Call site | Target | Supplied arguments relevant to the handoff | Result label |
| --- | --- | --- | --- |
| 0x35F02 | 0x358E0 | World, X, Y, output S | S object |
| 0x35F14 | 0x35CE0 | World, X, Y, output D | D object |
| 0x35F21 | 0x2BB7E0 | S | F |
| 0x35F3F | 0x2A0840 | S, current D category, current D weight | B |
| 0x35F64 | 0x2D4730 | World, X, Y, current D category, current D weight | r |
| 0x35F82 | 0x2B9750 | S, current D category, current D weight | a |
| 0x35F9F | 0x2D9F30 | S, current D category, current D weight | b |
| 0x35FAF | 0x2E64E0 | S | c |
| 0x35FE3 | 0x2E5F80 | World, X, Y, r, a, b, c, F, D | R |
| 0x3600F | 0x2B92F0 | World, X, Y, r, a, F, D | L |
| 0x3603B | 0x2D01A0 | World, X, Y, r, b, F, D | U |
| 0x3607D | 0x2BB310 | World, X, Y, B, F, R, L, U, r, D | H |
| 0x3608D | 0x2A44A0 | S | v |
| 0x3609D | 0x2B5960 | S | t |
| 0x360B9 | 0x2D36F0 | World, X, Y, t, H, v | Q |
| 0x36107 | 0x2B4450 | World, X, Y, B, H, R, L, U, Q, r, F, D, mode | Final binary32 result |
| 0x36117 | 0x393030 | Recovered stack-cookie operand | Normal epilogue guard |

There is no low-mode-bit branch around any preparation call. D is the same object pointer when passed to successive providers; this selected caller does not prove that the provider bodies leave all its fields unchanged. The providers' internal arithmetic, World-table reads, random draws, locks and side effects are not closed by inspecting this preparation owner. The mathematical cases below start at the final-height owner's stated inputs. Their extension through preparation is conditional on the earlier providers delivering those words and descriptor values at the actual reads, rather than an assertion of their attainable outputs.

RVA 0x2B4450 first sign-extends the signed X/Y values for integer-coordinate use and converts each signed low-32-bit value to binary64 for noise coordinates. The first direct noise call, 0x2B4538 to 0x2E0C0, uses X times binary64 0.005 plus 3363 and Y times binary64 0.005 plus 7874. Increase the returned binary32 word by 1, multiply that sum by 25 and subtract 10. This is the initial small-relief amplitude. Two separate attenuation factors start at 1: a small-relief factor and a peak factor. The low-reference flag starts false.

The category-zero test occurs before the reference call. For a nonzero initial category, the weight is loaded from D+0x1C and tested ordered strictly above binary32 0.0001. Failure skips descriptor work. Success calls 0x2BA200 at 0x2B4595 with World and D. For a finite returned reference value below 10, substitute 10 and set the low-reference flag; for a finite value at least 10, retain it. The follow-up comparison with 1 admits every finite value in that latter set. Unordered reference value takes the skip branch. The weight used in this descriptor arithmetic is the cached pre-call weight; the category is reloaded after the reference call. Provider mutation or concurrent changes outside the stable-input reference cases must therefore not be silently turned into one descriptor snapshot.

Double the cached weight, cap above at 1 and smooth by the pairwise procedure already specified. The ensuing category branches are exclusive.

| Reloaded category | Ordered arithmetic and relevant mode admission |
| --- | --- |
| 10 | If mode mask 2 is clear, square cached weight, multiply by 180 and add incoming B to that product. Mask 2 skips this addition. Both attenuation factors stay 1. |
| 18 | Cap doubled weight above at 1, square, multiply by binary32 0.1, multiply by radius converted from signed 32 bits, then add B. Separately subtract 0.5 from weight, double that result and select at least zero; complement, square, complement, multiply by the converted radius and then 0.25. Subtract that second product from the first updated base. The peak factor becomes one minus the smoothed doubled weight; small-relief factor remains 1. |
| 19 | Divide weight by binary32 0.85, cap above at 1, square, multiply by the same capped quotient, multiply by 180, and add B. Separately subtract binary32 0.85 from weight, divide by binary32 0.15 and select at least zero; complement, square, complement and multiply by 20. Subtract that second product from the updated base. Set only the peak factor to the smoothing complement. |
| 17, 21, 28, 36 | Skip the remaining descriptor arithmetic; both factors stay 1 and B is unchanged by this block. |
| All remaining categories reaching this branch | If amplitude is ordered above 10, calculate 10 minus amplitude, multiply by the smoothed doubled weight and add to amplitude. Set the peak factor to the smoothing complement. Then perform the reference blend below. |

The special reference-blend group is exactly 8, 13, 14 and 24. Subtract B from the admitted reference value, multiply by the smoothed doubled weight and add B; set the small-relief factor to the same smoothing complement. Every other category in the final row multiplies the reference-minus-B difference by the cached original weight and adds B, while retaining small-relief factor 1. The verified immediate membership masks are 0x1010220000, bounded through unsigned category 36, and 0x1006100, bounded through unsigned category 24. Negative or out-of-range numeric category words bypass membership in these bounded masks. Mode mask 2 does not skip category 18, category 19 or general reference blending.

The reference value is an output of 0x2BA200. The selected owners alone do not establish that it is final terrain height at a reference coordinate. It is kept distinct from incoming B and from the later saved pre-variant baseline. The category-18/-19 arithmetic adds B before the final subtraction, so an algebraic net contribution does not guarantee that final-minus-incoming has exactly that net word for every B.

Next select at least -256 from the updated base. This is a base selection before further height changes, not a floor on the returned final height. Double R, L, U and r separately and cap each above at 1; there is no lower clamp on these four doubled operands. Their operands and associations are essential. First subtract capped doubled L from 1 and capped doubled R from 1; multiply those two complements at 0x2B47E3. Subtract capped doubled U from 1 and multiply the first product by that complement at 0x2B47F9. Retain this three-channel product for the following two stages.

For the first small-relief envelope, multiply capped doubled r by 0.5, subtract that half-product from 1, multiply this r complement by the retained three-channel product, then multiply by the small-relief factor. Complement and square once before the next provider call. Separately multiply the floored base by binary32 0.05 and select at least zero before that call; its upper-at-1 selection is completed after the return.

Call noise at 0x2B4866, with X times binary64 0.015 plus 4447657 and Y times binary64 0.015 plus 87878. After the return, increase its binary32 value by 1; square the previously prepared square at 0x2B4874 and complement that fourth power; multiply the returned-value-plus-1 by 0.5 to prepare a noise-half product. Multiply the fourth-power complement by the bounded base gate at 0x2B488B, then multiply that rounded product by amplitude at 0x2B48A1. Multiply the prepared noise-half product by this already rounded envelope-gate-amplitude product at 0x2B48AD. Only then add to floored base at 0x2B48BF. A sequential product with the noise-half product multiplied first is a different arithmetic association.

Multiply this updated height by binary32 0.1. Its upper-at-1 selection precedes the next two calls. Call noise at 0x2B4916 with equal scale 0.015 and offsets 234234/12233, then at 0x2B493A with equal scale 0.01 and offsets 10943/98433. Add the two binary32 returns at 0x2B4943. For the peak mask, subtract capped doubled r from 1, multiply by the retained three-channel product, and multiply that rounded result by the peak factor. Subtract it from 1, then subtract that result from 1 again. Both binary32 subtractions are present. The lower-at-zero selection of the height gate is completed after these provider calls.

The next association is also explicit: multiply the double-complement peak mask by the bounded height gate at 0x2B497B. Multiply the sum of the two noise returns by this prepared mask-gate product at 0x2B497F. This is the peak-admission scalar. Multiplying the provider sum by the mask first and the height gate last is a different pairwise association. No mode bit suppresses the initial amplitude call, reference-admitted call, first small-relief call or the two peak-scalar calls in this normal continuation.

Only ordered positive peak scalar together with H ordered strictly below 0.5 admits the first peak-provider call. Category 28 reloads stored weight, doubles it and caps above at 1 for the bias. Every other category uses zero bias. Multiply the peak scalar by 20, cap above at 1 and retain its complement before the peak's provider call. That call is 0x2B49EA, at equal binary64 scale 0.005 with offsets 7743/98834. Add the bias and bound finitely to zero through 1. Then square the retained peak-envelope complement and complement its rounded square. Separately double H, complement and select at least zero. The first peak change is formed by successive multiplication of its envelope by this H complement, then by 20, then by the bounded noise word, then by that same noise word again. Add the last product to running height. Squaring the noise first is not the stated pairwise multiplication chain.

A peak scalar ordered strictly above 0.5 admits the second peak after the first addition. The 0.5 comparison is issued after the first provider/clamp and before the first height addition; its branch is taken after that addition. Subtract 0.5, multiply by 20 and cap above at 1 for its envelope. The call at 0x2B4A9E uses scale 0.005 and offsets 6777/8887. Apply the same bias, finite noise bounds and successive envelope/H-complement/20/noise/noise products, then add to the already updated height. A peak scalar ordered strictly above 0.75 next admits the third peak. It uses peak scalar minus 0.75, scale factor 20, and call 0x2B4B40 at scale 0.005 with offsets 2133/444, followed by the same product association and addition. Equality at either threshold fails its extra admission. The saved baseline is the running height after every admitted peak, at 0x2B4BC2.

Mode mask 2 now skips the whole descriptor-variant and subsequent H-height block. With that mask clear, category zero or mask 1 skips only the descriptor variants and still reaches the H-height admission. Otherwise the current 32-bit variant is selected. Codes zero, 4 and unrecognized values make no direct variant-height change in this owner.

Variant 1 converts the signed radius to binary32, multiplies separately by binary32 0.7 and by 0.5 to obtain outer and inner radii, and squares the outer radius. Stored squared distance must be ordered strictly less than that square. Subtract inner from outer to prepare width. Call the sqrtf boundary at 0x2B4C3C on stored squared distance. Subtract inner radius from the return, divide by the prepared width, complement and cap above at 1. Smooth this value, multiply by width, then by 0.75, and add to height. There is no separate lower envelope clamp in this branch.

Variant 5 requires the retained low-reference flag false. Multiply the converted radius separately by binary32 0.3 and 0.75, truncating each result to signed 32 bits for inner and outer integer radii. The outer integer square is calculated in 32 bits, converted to binary32 and compared with stored squared distance, with strict ordered less-than admission. This admits a noise call at 0x2B4D11, scale 0.02 and offsets 98453466/8743. Multiply its return by 25 and add 30 for band width. Subtract band width from converted outer integer radius and square.

If finite stored squared distance is strictly above this latter edge square, call sqrtf at 0x2B4D50, subtract its return from outer radius and divide by band width. Otherwise compare the stored distance with the signed 32-bit inner radius squared converted to binary32. No-greater-than inner square makes no change and does not call square root. Strictly greater calls sqrtf at 0x2B4F80; subtract inner radius, then call noise again at 0x2B4F9B at the same coordinates. Recompute band width from this second return, divide the already prepared radial difference by it and cap above at 1. For either admitted subtractive envelope, square the quotient, multiply by 100 and subtract from height. The first noise call occurs even when its later inner-hole check makes no change. The inner-annulus route makes a second independent provider call; equality of those two provider returns is not assumed.

Variants 2 and 3 use a shared route, with variant 3 additionally requiring low-reference flag false. Multiply the converted radius by binary32 0.6 and square it. The initial comparison requires this square ordered strictly greater than D+0x1C, the stored weight. It does not initially compare with the distance field at D+0x18. Call 0x2BAF90 at 0x2B5006 with World, an output pair and world X/Y. Multiply each returned binary32 output component by 65536 and truncate to signed 64 bits. The corresponding original signed X/Y minus signed reference coordinate is a signed 64-bit difference shifted left 16; add the truncated component. Convert each sum to binary32, multiply by 1/65536, square each, and add Y square to X square. Call sqrtf at 0x2B5085. Divide each displacement by that returned radial distance without a zero-distance guard. Call acosf at 0x2B509B on normalized X. If normalized Y is ordered negative, widen the angle, subtract from binary64 two-pi and narrow; otherwise retain it.

The original X and Y are floor-divided by 1024, including explicit negative correction. A nonzero D+4 grouping byte snaps both hash grid coordinates down to even integers. Three separate native coordinate hashes follow. Each starts with 32-bit X plus 57 times Y plus its salt, retaining low 32 bits; XOR with its left shift by 13; square, multiply by 0xEC4D, add 0x131071F, multiply by the XOR value and add 0xD208DD0D, retaining low 32 bits at every arithmetic step; then clear bit 31.

For the first salt, 0x1966AB2C, take the high 32 bits of the unsigned product with 0x61A8F94D, shift this high half right 13 and subtract from 100000 in 32 bits. Its unsigned remainder by 1000000 is converted from a zero-extended integer to binary32, divided by binary32 1000000 and doubled by addition. Widen, multiply by binary64 pi, add the widened existing angle and narrow. If the resulting angle widened to binary64 is strictly above two-pi, subtract two-pi once and narrow again. This is a single optional subtraction, not a repeated wrap loop.

The second salt is 0x711BEC9F. After the same masked polynomial, use signed multiply-high with 0x9E5706B3, arithmetic shift the high result by 13, and add that shifted result's sign bit. An odd low bit replaces angle by binary64 two-pi minus its widened value, narrowed to binary32. The third salt is 0x4E3E1C; its unsigned hash form, unsigned remainder by 20 and addition of 25 select an integer from 25 through 44. Convert this integer to binary32. Divide the 0.6-radius value by it, multiply by 15 and cap above at 150 for a height-change ceiling. Independently divide 0.6-radius by 6 for a final scale.

Widen angle, divide by binary64 two-pi and multiply by the widened selected integer. Subtract this product from widened 0.6-radius and narrow to binary32. Subtract the earlier returned radial distance and divide by the selected binary32 integer. A result below -0.25, or unordered at this comparison if it retires, skips the variant-height change. For finite admitted phase, obtain its floor using signed 32-bit truncation and an explicit negative fractional correction. Subtract the converted floor from phase and then 0.75. An ordered positive fraction is multiplied by 4, capped above at 1 and smoothed; otherwise use zero. Add converted floor. If ordered positive, widen this value, multiply by two-pi, add widened current angle and narrow. A negative phase multiplies angle by the separately prepared binary32 phase-times-4-plus-1 factor. If angle is ordered below 1, square it. Call sqrtf at 0x2B53BF, multiply its return by the saved 0.6-radius-divided-by-6 scale, and select the lesser of this product and the prepared ceiling. Variant 3 subtracts this selected change; variant 2 adds it.

Following variants, H ordered strictly above 0.5 admits H-height work, unless mode mask 2 skipped the whole block. Subtract 0.75 from H, multiply by 4 and bound finitely to zero through 1. Its shoulder product is prepared in two parts: complement that bounded value; separately multiply the bounded value by 4; then multiply the complement by that latter rounded product. Cap above at 1. This association is not a left-associated complement-times-value-times-4 chain.

Separately subtract 0.5 from H and double before the noise call at 0x2B4E1F, which uses equal scale 0.01 and offsets 8757/9087. After the return, multiply its value by 0.25; square the prepared doubled H difference; add 0.75 to the scaled return; multiply the squared H difference by 100. Multiply the prepared returned-value gain by that squared/scaled H value and add current height. Next call 0x2B5670 at 0x2B4E53 with World and world X/Y. Select the shoulder at least zero, multiply the returned helper word by that selected shoulder and then by 16, and add to the already updated height. The second helper call is admitted even when the shoulder's numerical contribution is zero.

The final channel block is considered with mode mask 8 clear. Its branches are exclusive and tested in the following order. R ordered strictly above 0.5 admits a noise call at 0x2B4F1A, equal scale 0.025 with offsets 35/8933. Before that call, subtract 0.5 from R, double that difference and complement; separately add 5 to current height, multiply by binary32 0.1 and select at least zero. After the return, increase the returned value by 1; square the retained R complement, multiply that rounded square by the same complement and complement the cube. Multiply the returned-value-plus-1 by 0.5, then by the cube complement, then by 9.5. Complete the height gate's upper-at-1 selection, then multiply by that bounded gate. Add 2, then subtract this entire rounded amount from running height.

If R is not admitted, L ordered strictly above 0.5 adds -2 and skips the remaining channel choices. If L also fails, U ordered strictly above 0.5 adds -2. Otherwise Q ordered strictly above 0.5 admits the last decrement route; failing that, r ordered strictly above 0.5 admits the same route. Only when both fail is 0x2B7730 called at 0x2B5458 with World, X, Y and current binary32 height. Any nonzero returned low byte admits the decrement. That route adds -1, then adds another -1 only if H is ordered strictly above 0.5. No successful earlier channel branch calls the later boolean helper. Mask 8 skips all of this final channel block, including the relevant noise or boolean-provider call, while leaving preparation and earlier channel-envelope arithmetic intact.

Lastly mode mask 4 together with H ordered strictly above 0.5 calls 0x2A2C50 at 0x2B54AB, with World, X, Y, current D category and freshly loaded D weight. Only an ordered positive return continues. This is a separate call after all final-channel work. Noise at 0x2B54E0 uses scale 0.01 and offsets 7474/98984; increase its return by 1, multiply by 20 and add the saved pre-variant baseline. Noise at 0x2B552A uses scale 0.02 and offsets 8744/904. After this second return, the incoming-height-versus-baseline comparison is issued before its returned value is multiplied by binary32 0.025 and increased by binary32 0.06. The exit branch follows completion of that frequency arithmetic. Both noise calls therefore occur before the height-based exit.

If incoming height is ordered no greater than baseline, return it unchanged; an unordered comparison likewise takes the exit if it retires. Otherwise start a descending candidate at incoming height. Floor the candidate using signed 32-bit truncation and the explicit negative-fraction correction, then convert that integer back to binary32. Subtract the prepared comparison center and multiply by the prepared frequency. Widen, clear the sign, narrow, cap above at 1 and square. Complement that square, multiply by the positive 0x2A2C50 return, then complement and smooth. Separately subtract floored candidate from the fixed incoming height of this block, multiply by binary32 0.1 and add 0.5. Multiply the smoothed factor by this prepared depth factor. Ordered result strictly above 0.5 returns the current floored candidate. Otherwise add -1 to that candidate, repeat only while the new candidate is ordered strictly above saved baseline, and return the candidate when the loop ends. Incoming height used in the depth factor is fixed across repetitions; it does not become the descending candidate.

This loop can return below a nonintegral baseline and contains no final baseline clamp. It has no fixed iteration limit. A finite descent bound depends on in-range conversions and candidate-minus-1 actually decreasing at the current binary32 spacing. No guarantee for arbitrary nonfinite or excessively large candidate values is inferred.

The final binary32 result is selected before the cookie check at 0x2B560F. The complete selected guard compares the recovered cookie with the global cookie and also checks its high 16 bits. Its normal successful path does not access the floating result register; failure jumps to untraced failure owner 0x393534. Preparation's 0x36117 guard uses the same normal-return boundary. Failure reporting, exceptions and a failure path returning normally are outside the reference conditions.

The following table records the logical provider-call order inside the selected final owner. Addresses are call sites, not a claim of ascending-address execution. In particular, variant-5 inner calls located at higher addresses return to the common variant/H continuation before the later channel block.

| Phase | Direct call-site order and admission |
| --- | --- |
| Initial amplitude | 0x2B4538 to noise, on normal entry |
| Optional reference | 0x2B4595 to 0x2BA200 after initial nonzero category and cached weight ordered above binary32 0.0001 |
| First relief and peak scalar | 0x2B4866, 0x2B4916, 0x2B493A to noise, in that order on normal continuation |
| Small peaks | 0x2B49EA; then 0x2B4A9E only above peak scalar 0.5; then 0x2B4B40 only above 0.75; the first requires positive peak scalar and H below 0.5 |
| Variant 1 | 0x2B4C3C to sqrtf after strict outer-distance admission |
| Variant 5 | 0x2B4D11 to noise after strict outer-distance admission; outer-band route then 0x2B4D50 to sqrtf; inner-annulus route instead 0x2B4F80 to sqrtf followed by 0x2B4F9B to noise; inner-hole route makes neither later call |
| Variant 2 or 3 | 0x2B5006 to pair provider, 0x2B5085 to sqrtf, 0x2B509B to acosf; only admitted phase continues to 0x2B53BF to sqrtf |
| H-height work | 0x2B4E1F to noise, then 0x2B4E53 to 0x2B5670, only with mask 2 clear and H above 0.5 |
| Exclusive channel work | With mask 8 clear, R branch calls 0x2B4F1A to noise; L/U and Q/r successes make no direct provider call here; otherwise 0x2B5458 to boolean helper |
| Optional terrace | Mask 4 and H above 0.5 call 0x2B54AB; a positive return then calls 0x2B54E0 and 0x2B552A to noise before testing height versus baseline |
| Normal exit guard | 0x2B560F to 0x393030 |

Mask 1 only suppresses variants. Mask 2 suppresses category-10's direct addition and the later entire variant/H-height block. It leaves the small-relief and peak stages, other descriptor arithmetic, final channels and mask-4 terrace admission in place. Mask 4 adds its guarded terrace work. Mask 8 suppresses only final channels. The original preparation owner still makes its full provider sequence for all these masks. Higher mode bits have no direct tested branch in the selected final owner.

Every noise coordinate in the selected final owner is prepared by a separate binary64 multiply by the stated stored scale and a separate binary64 offset addition. Both X and Y use that row's scale. The direct non-noise helper arguments and imported square-root/inverse-cosine arguments are the previously described retained values. Original call metadata identifies sqrtf at target 0x395462 and acosf at 0x3954A4; their imported implementations are not inspected here. A noise call's internal provider work is not counted as an additional direct call of the selected owner.

The worked references use stable, separately allocated argument/descriptor storage, providers preserving the stated floating environment and the listed input values until their reads, and normal guard returns. All X/Y pairs below are signed 17 and -9, encoded 0x00000011 and 0xFFFFFFF7. World is an opaque valid pointer passed unchanged; provider-return conditions replace any need to guess its internal state. Descriptor fields not varied below are category 0, grouping byte 0, variant 0, signed radius 250, signed reference pair 0/0, distance word 0x00000000 and weight word 0x00000000. No claim about descriptor padding is needed. F is the literal input word 0x3F200000, or 0.625, and is unread in the final body.

Reference family A uses mode 8 and the following exact scalar inputs.

| Input | Binary32 word | Exact decimal value |
| --- | --- | --- |
| B | 0x40500000 | 3.25 |
| H | 0x3E800000 | 0.25 |
| R | 0x3DC00000 | 0.09375 |
| L | 0x3E200000 | 0.15625 |
| U | 0x3E600000 | 0.21875 |
| Q | 0x3E800000 | 0.25 |
| r | 0x3DE00000 | 0.109375 |

Its direct provider-return conditions are all positive binary32 words: at 0x2B4538 return 0x3E800000, or 0.25; at 0x2B4866 return 0x3E000000, or 0.125; at 0x2B4916 return 0x3E800000, or 0.25; at 0x2B493A return 0x3E000000, or 0.125; and at the admitted first peak call 0x2B49EA return 0x3EC00000, or 0.375. The category-zero path makes no reference call. Peak scalar is below 0.5, so neither second nor third peak is called. H is below 0.5, so H-height and mask-4 work do not occur. Mode 8 skips final channels. The direct numerical call sequence is exactly the five call sites just listed, followed by the normal guard.

The following complete family-A intermediate table follows arithmetic dependency order; independent native instructions can be interleaved as identified by their source anchors and the separately specified provider boundaries. It is a conditional word calculation, not an execution transcript. Selection ranges include both possible selection instructions; a listed range does not assert that an untaken branch instruction executed in this case.

| Prepared value or operation | Source anchor | Binary32 result word | Exact value represented |
| --- | --- | --- | --- |
| initial noise plus one | 0x2B4551 | 0x3FA00000 | 1.25 |
| amplitude product by twenty-five | 0x2B4569 | 0x41FA0000 | 31.25 |
| initial amplitude | 0x2B4572 | 0x41AA0000 | 21.25 |
| admitted base before floor | 0x2B474B | 0x40500000 | 3.25 |
| R doubled and upper selected | 0x2B47AF | 0x3E400000 | 0.1875 |
| L doubled and upper selected | 0x2B47B4 | 0x3EA00000 | 0.3125 |
| U doubled and upper selected | 0x2B47B9 | 0x3EE00000 | 0.4375 |
| r doubled and upper selected | 0x2B47BE | 0x3E600000 | 0.21875 |
| L complement | 0x2B47C3 | 0x3F300000 | 0.6875 |
| R complement | 0x2B47CB | 0x3F500000 | 0.8125 |
| L complement times R complement | 0x2B47E3 | 0x3F0F0000 | 0.55859375 |
| U complement | 0x2B47EB | 0x3F100000 | 0.5625 |
| retained L-R-U product | 0x2B47F9 | 0x3EA0E000 | 0.314208984375 |
| capped doubled r times one-half | 0x2B4800 | 0x3DE00000 | 0.109375 |
| complement of preceding r half-product | 0x2B480B | 0x3F640000 | 0.890625 |
| r half-complement times retained product | 0x2B481B | 0x3E8F4780 | 0.2798423767089844 |
| preceding product times small-relief attenuation | 0x2B4823 | 0x3E8F4780 | 0.2798423767089844 |
| complement before powers | 0x2B483E | 0x3F385C40 | 0.7201576232910156 |
| first square of complement | 0x2B4854 | 0x3F04C4BD | 0.5186269879341125 |
| second square of complement | 0x2B4874 | 0x3E89B6F4 | 0.2689739465713501 |
| complement of fourth power | 0x2B487C | 0x3F3B2486 | 0.7310260534286499 |
| floored base times binary32 0.05, bounded | 0x2B4813..0x2B488B | 0x3E266667 | 0.16250000894069672 |
| small-relief provider return plus one | 0x2B486F | 0x3F900000 | 1.125 |
| preceding return sum times one-half | 0x2B4880 | 0x3F100000 | 0.5625 |
| fourth-power complement times base gate | 0x2B488B | 0x3DF34915 | 0.11879173666238785 |
| preceding envelope-gate product times amplitude | 0x2B48A1 | 0x40218E88 | 2.524324417114258 |
| noise-half product times prepared envelope product | 0x2B48AD | 0x3FB5C059 | 1.41993248462677 |
| height after first small relief | 0x2B48BF | 0x40957016 | 4.6699323654174805 |
| height times binary32 0.1, bounded | 0x2B48C8..0x2B4972 | 0x3EEF19BD | 0.4669932425022125 |
| two peak providers added | 0x2B4943 | 0x3EC00000 | 0.375 |
| capped doubled r complement | 0x2B494B | 0x3F480000 | 0.78125 |
| preceding r complement times retained product | 0x2B4959 | 0x3E7B5E00 | 0.24547576904296875 |
| preceding product times peak attenuation | 0x2B495F | 0x3E7B5E00 | 0.24547576904296875 |
| first peak-mask complement | 0x2B4964 | 0x3F412880 | 0.7545242309570312 |
| second peak-mask complement | 0x2B4968 | 0x3E7B5E00 | 0.24547576904296875 |
| double-complement times height gate | 0x2B497B | 0x3DEAC608 | 0.11463552713394165 |
| provider sum times prepared weighted mask | 0x2B497F | 0x3D301486 | 0.04298832267522812 |
| first peak scalar times twenty, upper selected | 0x2B49D5 | 0x3F5C19A8 | 0.8597664833068848 |
| first peak envelope complement | 0x2B49DA | 0x3E0F9960 | 0.14023351669311523 |
| first peak complement squared | 0x2B4A37 | 0x3CA1196A | 0.019665438681840897 |
| first peak square complement | 0x2B4A48 | 0x3F7AF735 | 0.9803345799446106 |
| twice-H complement, lower selected | 0x2B4A4C | 0x3F000000 | 0.5 |
| first peak envelope times H complement | 0x2B4A51 | 0x3EFAF735 | 0.4901672899723053 |
| preceding product times twenty | 0x2B4A56 | 0x411CDA81 | 9.803345680236816 |
| preceding product times positive peak-provider return | 0x2B4A5A | 0x406B47C2 | 3.6762547492980957 |
| preceding product times the same return again | 0x2B4A5E | 0x3FB075D2 | 1.3785955905914307 |
| height after first peak | 0x2B4A62 | 0x40C18D8A | 6.048527717590332 |
| retained pre-variant baseline | 0x2B4BC2 | 0x40C18D8A | 6.048527717590332 |

Family A returns 0x40C18D8A, representing 6.048527717590332. With mode changed to 0 and otherwise identical inputs, append direct boolean call 0x2B5458 after the first-peak addition. Conditional low-byte return 0x00 leaves output 0x40C18D8A; low-byte 0x01 gives 0x40A18D8A, or 5.048527717590332. H is below 0.5, so only one -1 addition occurs. Changing only Q to 0x3F400000, or 0.75, in that mode-0 case gives the same decremented output while skipping 0x2B5458. The helper result is not consumed in this Q-admitted case.

Reference family B uses the same descriptor, X/Y, F, Q and mode 8, but deliberately different literal operands. These values were chosen independently as synthetic arithmetic inputs.

| Input or provider return | Exact binary32 word |
| --- | --- |
| B | 0x40100003 |
| H | 0x3E400000 |
| R | 0x3DCCCCCD |
| L | 0x3E0F5C29 |
| U | 0x3E3851EC |
| r | 0x3E570A3D |
| Return at 0x2B4538 | 0x3E2E147B |
| Return at 0x2B4866 | 0x3E6B851F |
| Return at 0x2B4916 | 0x3E428F5C |
| Return at 0x2B493A | 0x3E570A3D |
| Return at 0x2B49EA | 0x3EBD70A4 |

B is 2.2500007152557373 and H is 0.1875. All five provider returns are positive. The direct call sequence and first-peak-only admission match family A. Selected detailed results establish the nontrivial pairwise associations.

| Prepared value or operation | Source anchor | Binary32 result word | Exact value represented |
| --- | --- | --- | --- |
| initial amplitude | 0x2B4572 | 0x4199FFFF | 19.249998092651367 |
| L complement times R complement | 0x2B47E3 | 0x3F1374BD | 0.5760000348091125 |
| retained L-R-U product | 0x2B47F9 | 0x3EBCBE62 | 0.3686400055885315 |
| complement of preceding r half-product | 0x2B480B | 0x3F4A3D71 | 0.7900000214576721 |
| r half-complement times retained product | 0x2B481B | 0x3E951B86 | 0.2912256121635437 |
| complement before powers | 0x2B483E | 0x3F35723D | 0.7087743878364563 |
| first square of complement | 0x2B4854 | 0x3F009ABD | 0.5023611187934875 |
| second square of complement | 0x2B4874 | 0x3E813635 | 0.25236669182777405 |
| complement of fourth power | 0x2B487C | 0x3F3F64E6 | 0.7476333379745483 |
| floored base times binary32 0.05, bounded | 0x2B4813..0x2B488B | 0x3DE6666B | 0.11250003427267075 |
| preceding return sum times one-half | 0x2B4880 | 0x3F1D70A4 | 0.6150000095367432 |
| fourth-power complement times base gate | 0x2B488B | 0x3DAC4139 | 0.08410877734422684 |
| preceding envelope-gate product times amplitude | 0x2B48A1 | 0x3FCF3E77 | 1.6190937757492065 |
| noise-half product times prepared envelope product | 0x2B48AD | 0x3F7EE8FE | 0.995742678642273 |
| height after first small relief | 0x2B48BF | 0x404FBA42 | 3.2457432746887207 |
| height times binary32 0.1, bounded | 0x2B48C8..0x2B4972 | 0x3EA62E9B | 0.3245743215084076 |
| two peak providers added | 0x2B4943 | 0x3ECCCCCC | 0.3999999761581421 |
| preceding product times peak attenuation | 0x2B495F | 0x3E5AF154 | 0.2138112187385559 |
| first peak-mask complement | 0x2B4964 | 0x3F4943AB | 0.7861887812614441 |
| second peak-mask complement | 0x2B4968 | 0x3E5AF154 | 0.2138112187385559 |
| double-complement times height gate | 0x2B497B | 0x3D8E2058 | 0.06939762830734253 |
| provider sum times prepared weighted mask | 0x2B497F | 0x3CE366F2 | 0.027759049087762833 |
| first peak scalar times twenty, upper selected | 0x2B49D5 | 0x3F0E2057 | 0.5551809668540955 |
| first peak complement squared | 0x2B4A37 | 0x3E4A9CDA | 0.19786396622657776 |
| first peak square complement | 0x2B4A48 | 0x3F4D58CA | 0.8021360635757446 |
| first peak envelope times H complement | 0x2B4A51 | 0x3F00577E | 0.5013350248336792 |
| preceding product times twenty | 0x2B4A56 | 0x41206D5E | 10.026700973510742 |
| preceding product times positive peak-provider return | 0x2B4A5A | 0x406D6EAA | 3.7098793983459473 |
| preceding product times the same return again | 0x2B4A5E | 0x3FAFB32C | 1.3726553916931152 |
| height after first peak | 0x2B4A62 | 0x4093C9EC | 4.618398666381836 |

Family B returns 0x4093C9EC, or 4.618398666381836. For these same literal words, a mathematically reparenthesized first-relief chain that multiplies noise-half by fourth-power complement first, then base gate, then amplitude yields intermediate change 0x3F7EE8FF. The source prepares complement-times-gate, then amplitude, then multiplies by noise-half and yields 0x3F7EE8FE. Likewise, provider-sum times mask followed by height gate yields 0x3CE366F3, while the source's mask-times-gate followed by provider-sum yields 0x3CE366F2. These one-word differences are conditional arithmetic association facts; neither alternative is an original observation or another implementation result.

Reference family C retains A's X/Y, descriptor, Q, F and mode 8, but uses B 0x40B00000, or 5.5; H 0x3E000000, or 0.125; and R/L/U/r each 0x3D000000, or 0.03125. Keep A's initial-amplitude and first-relief provider words. Set returns at 0x2B4916 and 0x2B493A each to 0x3F400000, or 0.75. The first, second and third peak returns are respectively 0x3EC00000, 0x3E800000 and 0x3E000000, or 0.375, 0.25 and 0.125. All are positive. The resulting peak scalar is 0x3F823EDF, representing 1.0175436735153198, so all three peaks are admitted.

| Family-C stage | Binary32 word | Exact value represented |
| --- | --- | --- |
| Height before small peaks | 0x410C81AF | 8.781661033630371 |
| Provider sum | 0x3FC00000 | 1.5 |
| Prepared peak mask-times-height-gate | 0x3F2DA929 | 0.6783624291419983 |
| Peak scalar | 0x3F823EDF | 1.0175436735153198 |
| Retained H complement | 0x3F400000 | 0.75 |
| First peak change | 0x40070000 | 2.109375 |
| Height after first peak | 0x412E41AF | 10.891036033630371 |
| Second peak change | 0x3F700000 | 0.9375 |
| Height after second peak | 0x413D41AF | 11.828536033630371 |
| Third peak change | 0x3E700000 | 0.234375 |
| Saved baseline and final output | 0x414101AF | 12.062911033630371 |

Its direct numerical calls are 0x2B4538, 0x2B4866, 0x2B4916, 0x2B493A, 0x2B49EA, 0x2B4A9E and 0x2B4B40, followed by the guard. Both extra peak envelopes cap at 1. There is no final-channel or H-height provider call.

Reference family D returns to A's incoming scalar words and positive noise-return words. Set grouping byte zero, variant zero, signed radius 250 and weight 0x3E800000, or 0.25. Reference pair stays 0/0 and distance stays 0x00000000. The named descriptor fields are literal synthetic inputs; no claim is made that an untraced descriptor producer emits this combination. For every row below, the admitted reference provider at 0x2B4595 returns 0x40A00000, or 5. This finite value between zero and 10 is replaced by reference operand 10 and sets low-reference flag true. Incoming B remains 3.25 in every row; the table's base is the later value after the descriptor block.

| Category and mode | Base after descriptor work | Amplitude after descriptor work | Small-relief factor | Peak factor | First-relief height | Peak scalar | Final output |
| --- | --- | --- | --- | --- | --- | --- | --- |
| category 10, mode 8 or 9 | 0x41680000 | 0x41AA0000 | 0x3F800000 | 0x3F800000 | 0x41A6AE40 | 0x3DBC8680 | 0x41B1EE40 |
| category 10, mode 10 | 0x40500000 | 0x41AA0000 | 0x3F800000 | 0x3F800000 | 0x40957016 | 0x3D301486 | 0x40C18D8A |
| category 18, mode 8 or 10 | 0x41180000 | 0x41AA0000 | 0x3F800000 | 0x3F000000 | 0x415A68BE | 0x3D3C8680 | 0x4170C45E |
| category 19, mode 8 or 10 | 0x40FA8CC9 | 0x41AA0000 | 0x3F800000 | 0x3F000000 | 0x41340205 | 0x3D3C8680 | 0x414A5DA5 |
| category 8, mode 8 or 10 | 0x40D40000 | 0x417A0000 | 0x3F000000 | 0x3F000000 | 0x40FE2F13 | 0x3D15C018 | 0x4113F75E |
| category 1, variant 0, mode 8 | 0x409E0000 | 0x417A0000 | 0x3F800000 | 0x3F000000 | 0x40D0C1FD | 0x3CF5F9D3 | 0x40F693A4 |
| category 17, mode 8 | 0x40500000 | 0x41AA0000 | 0x3F800000 | 0x3F800000 | 0x40957016 | 0x3D301486 | 0x40C18D8A |

For category 10 with mode 8 or 9, the direct addition operand is 0x41340000, or 11.25, and the post-descriptor base is 14.5; final result 0x41B1EE40 is 22.2413330078125. With mode 10, the reference call and smoothing are still admitted, but mask 2 skips the addition; final result is A's 0x40C18D8A. Mode mask 1 does not suppress this direct category addition.

For category 18, its positive direct amount is 0x40C80000, or 6.25, and its subtractive amount is positive zero. Post-descriptor base is 9.5 and final 0x4170C45E is 15.047941207885742. For category 19, its positive amount is 0x40928CC9, or 4.579685688018799, and its subtractive amount is positive zero; post-descriptor base is 0x40FA8CC9, or 7.829685688018799, and final 0x414A5DA5 is 12.647862434387207. Both categories retain these outputs for modes 8 and 10 with the same conditional returns and H below 0.5.

Category 8 uses the smoothed reference blend, giving post-descriptor base 6.625 and amplitude 15.625, with both attenuation factors 0.5. Its final 0x4113F75E is 9.247892379760742. Category 1 with variant zero uses the original-weight blend instead, giving base 4.9375, the same amplitude 15.625, small-relief factor 1 and peak factor 0.5; final 0x40F693A4 is 7.705522537231445. Category 17 admits the reference call but its exclusion branch leaves A's base/amplitude/factors, giving A's final output. These cases separate cached base, reference operand, smoothed weight, original weight and the two distinct attenuation operands.

Every D-row direct call sequence is 0x2B4538, 0x2B4595, 0x2B4866, 0x2B4916, 0x2B493A and 0x2B49EA, then guard. None reaches second/third peak, H-height or terrace; mask 8 is set in every listed mode.

Reference family E uses A's incoming scalars, first five positive provider words and world coordinates. Set descriptor category 1, grouping byte zero, variant 1, signed radius 8, distance 0x3F800000, or 1, and weight positive zero. The reference call is not admitted because weight is zero. With mode 8, the strict outer-distance test admits sqrtf at 0x2B4C3C. Conditionally return 0x3F800000, or 1, at that call.

| Family-E stage | Binary32 word | Exact value represented |
| --- | --- | --- |
| Radius times binary32 0.7 | 0x40B33333 | 5.599999904632568 |
| Outer radius squared | 0x41FAE147 | 31.35999870300293 |
| Inner radius | 0x40800000 | 4 |
| Outer minus inner width | 0x3FCCCCCC | 1.5999999046325684 |
| Root-minus-inner divided by width | 0xBFF00001 | -1.8750001192092896 |
| Upper-selected envelope | 0x3F800000 | 1 |
| Smoothed envelope | 0x3F800000 | 1 |
| Variant change after width and 0.75 products | 0x3F999999 | 1.1999999284744263 |
| Height before variant | 0x40C18D8A | 6.048527717590332 |
| Mode-8 final output | 0x40E7F3F0 | 7.248527526855469 |

Mode 8 appends this single sqrtf call after A's five calls. Modes 9 and 10 skip the variant call and return 0x40C18D8A. No reference, H-height, final-channel or terrace provider is admitted in these E cases. They demonstrate mask 1 and mask 2 suppressing a variant while leaving the positive earlier peak.

Reference family F uses A's inputs except R becomes 0x3F200000, or 0.625. Its capped doubled R is 1, causing the retained three-channel product to be zero. Initial amplitude and all first-relief/peak-scalar provider calls still occur. First-relief height remains B 0x40500000 and peak scalar is zero, so there is no peak provider call. For mode 0 the exclusive R branch calls noise at 0x2B4F1A and conditionally receives 0x3E800000, or 0.25.

| Family-F stage | Binary32 word | Exact value represented |
| --- | --- | --- |
| R minus 0.5 | 0x3E000000 | 0.125 |
| Doubled difference | 0x3E800000 | 0.25 |
| Complement | 0x3F400000 | 0.75 |
| Rounded complement cube | 0x3ED80000 | 0.421875 |
| Cube complement | 0x3F140000 | 0.578125 |
| Bounded current-height-plus-5 gate | 0x3F533333 | 0.824999988079071 |
| Returned-value-plus-1 times 0.5 | 0x3F200000 | 0.625 |
| Previous word times cube complement | 0x3EB90000 | 0.361328125 |
| Previous product times 9.5 | 0x405BB000 | 3.4326171875 |
| Previous product times bounded height gate | 0x40353E00 | 2.8319091796875 |
| Previous amount plus 2 | 0x409A9F00 | 4.8319091796875 |
| Mode-0 final output | 0xBFCA7C00 | -1.5819091796875 |
| Mode-8 final output | 0x40500000 | 3.25 |

The mode-0 F sequence is 0x2B4538, 0x2B4866, 0x2B4916, 0x2B493A and 0x2B4F1A, then guard. Mode 8 omits the last noise call. Neither calls the boolean helper. Separately, changing only L to 0.625 while leaving R at A's value causes its L branch to return 0x3FA00000, or 1.25, with no final-channel provider call. Changing only U to 0.625 has the analogous exclusive U result. These latter cases also have zero three-channel product, no small peak, and mode 0.

Reference family G uses A's inputs except H becomes 0x3F400000, or 0.75. Its first-relief height and saved pre-variant baseline are 0x40957016, or 4.6699323654174805. Peak scalar is still calculated but H fails strict below-0.5 admission, so no peak providers are called. With mode 12, masks 4 and 8 are set. The H-height call at 0x2B4E1F conditionally returns 0x3E800000, or 0.25, and helper 0x2B4E53 also returns positive 0x3E800000.

The clamped H-minus-0.75 shoulder input is zero, so the shoulder product is zero. Both H-height providers are nevertheless called. The squared/scaled doubled H-minus-0.5 operand is 0x41C80000, or 25. The noise return times 0.25 plus 0.75 is 0x3F500000, or 0.8125; its product by 25 is 0x41A28000, or 20.3125. Addition to baseline gives incoming terrace height 0x41C7DC06, or 24.982433319091797. The positive helper's multiplication by zero shoulder contributes zero.

At 0x2B54AB conditionally return 0x3F000000, or 0.5; at 0x2B54E0 return 0x3E000000, or 0.125; at 0x2B552A return 0x3E800000, or 0.25. Comparison center is 0x41D95C06, or 27.169933319091797. Prepared frequency is 0x3D87AE14, or 0.06624999642372131. The fixed incoming height is used for every depth factor.

| Floored candidate | Scaled signed distance word | Squared bounded absolute word | Prepared bend word | Smoothed bend word | Depth factor word | Score word | Strictly above 0.5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 24 | 0xBE570C5B | 0x3D34A5C1 | 0x3F05A52E | 0x3F08765D | 0x3F19267A | 0x3EA34680 | No |
| 23 | 0xBE8D71B3 | 0x3D9C4CE3 | 0x3F09C4CE | 0x3F0E9FEC | 0x3F32C013 | 0x3EC72C69 | No |
| 22 | 0xBEAF5D38 | 0x3DF04129 | 0x3F0F0412 | 0x3F166BA8 | 0x3F4C59AD | 0x3EF024F6 | No |
| 21 | 0xBED148BD | 0x3E2B17D9 | 0x3F1562FB | 0x3F1FC80B | 0x3F65F346 | 0x3F0F85C8 | Yes |

The accepted candidate is 21, word 0x41A80000, and this is mode 12's final result. The source's successive depth factors represent approximately 0.5982433557510376, 0.6982433199882507, 0.7982433438301086 and 0.8982433080673218; they depend on fixed incoming 24.982433319091797 and each floored candidate. The last score is 0x3F0F85C8, or 0.5606350898742676.

The exact G mode-12 direct numerical call order is 0x2B4538, 0x2B4866, 0x2B4916, 0x2B493A, 0x2B4E1F, 0x2B4E53, 0x2B54AB, 0x2B54E0 and 0x2B552A, then guard. With mode 8, omit the final three terrace calls and return the un-terraced incoming height 0x41C7DC06.

With mode 14, mask 2 suppresses both H-height providers. It does not suppress the mask-4 helper and its two positive noise-return calls. Under the same terrace conditions these still occur before the incoming-height-versus-baseline comparison; both heights are now 0x40957016, so the loop is never entered and final output is 0x40957016. Its direct calls are 0x2B4538, 0x2B4866, 0x2B4916, 0x2B493A, 0x2B54AB, 0x2B54E0 and 0x2B552A, then guard.

As a strict-equality reference, set only H to 0x3F000000, or 0.5, and mode to 12 while retaining A's other inputs and first four provider words. H admits neither small peaks, H-height nor terrace helper. The direct numerical sequence is only the first four noise calls, then guard; final result is 0x40957016. The equality cases are not inferred from a non-strict approximation to the source comparisons.

The following binary64 coordinate words are independently calculated for signed X=17 and Y=-9 under the stated nearest-even model. They identify the exact direct noise inputs for these synthetic examples and for any admitted listed route with that same pair. A decimal shown above is only a readable description of the encoded arithmetic. The coordinate table does not supply or predict a noise result.

| Direct noise call site | X binary64 input word | Y binary64 input word |
| --- | --- | --- |
| 0x2B4538 | 0x40AA462B851EB852 | 0x40BEC1F47AE147AE |
| 0x2B4866 | 0x4150F76A5051EB85 | 0x40F5745DD70A3D71 |
| 0x2B4916 | 0x410C97D20A3D70A4 | 0x40C7E46EB851EB85 |
| 0x2B493A | 0x40C55F95C28F5C29 | 0x40F8080E8F5C28F6 |
| 0x2B49EA | 0x40BE3F15C28F5C29 | 0x40F8211F47AE147B |
| 0x2B4A9E | 0x40BA7915C28F5C29 | 0x40C15B7A3D70A3D7 |
| 0x2B4B40 | 0x40A0AA2B851EB852 | 0x407BBF47AE147AE1 |
| 0x2B4D11 or 0x2B4F9B | 0x4197791F695C28F6 | 0x40C11368F5C28F5C |
| 0x2B4E1F | 0x40C11A95C28F5C29 | 0x40C1BF747AE147AE |
| 0x2B4F1A | 0x4041B66666666666 | 0x40C1726333333333 |
| 0x2B54E0 | 0x40BD322B851EB852 | 0x40F82A7E8F5C28F6 |
| 0x2B552A | 0x40C1142B851EB852 | 0x408C3E8F5C28F5C3 |

Numerical vector coverage is deliberately conditional. Families A and B work finite low-base, fractional-channel and positive-return arithmetic through the first peak; C works all three peak calls; D works the selected category/reference associations and mode-2 distinction; E works variant-1 and masks 1/2; F works exclusive R carving and mask 8; G works H-height providers, mask 4, a multi-candidate terrace and its mask-2 interaction. The complete source control flow for variants 2, 3 and 5 is described, but no numeric vector purports to close their untraced displacement/math/noise providers. The mathematical cases are not a census of attainable native terrain or every possible input.

The selected owners do not set the floating rounding mode or establish the whole lifetime of the provider/global state. The vector model explicitly assumes providers preserve the stated environment and the relevant stable input words. All provider returns in a case are per-call conditions; two calls at identical coordinates need not be equated. An untraced provider changing rounding controls, exception masks, denormal modes, World/global state or a descriptor field can invalidate an unconditional numerical extension. No original runtime default for these controls is asserted from this selected-body audit.

Unordered branch facts follow the original comparisons if they retire without trapping. NaN stored weight fails the initial ordered-above-0.0001 reference admission. Unordered reference return takes the later carry exit, skipping descriptor work. The first small-relief base gate uses its lower comparison/select before the provider and its upper comparison after the provider; unordered input to that lower selection chooses zero. The later current-height-times-0.1 gate instead performs its upper comparison/select first; unordered input there chooses 1 before its eventual lower selection. These are distinct native selections, not interchangeable clamps.

The scalar min/max instructions choose their second operand for unordered or equal comparisons. The base selection at -256 has the current base as its second operand, so it does not sanitize NaN B. The upper selections for the doubled channel operands use 1 as the second operand. Other selections use explicit comparisons and selected memory/register values; no universal NaN policy is substituted. Peak-positive admission, H below 0.5, H-height above 0.5, terrace H above 0.5 and positive terrace-helper admission all require their stated ordered relations. Unordered fails those admissions. Strict outer variant-support tests and the initial variant-2/-3 radius-square-versus-weight test reject unordered at those comparisons. The phase-versus-minus-0.25 carry exit also rejects unordered.

R/L/U final-channel ordered-above admissions fail for NaN. The later Q/r above tests likewise fail for unordered, leaving the boolean-provider route available when earlier channel gates did not succeed. The incoming-height-versus-baseline below-or-equal exit in terracing includes unordered. These local branch outcomes do not promise a complete normal return or a particular NaN payload from an invocation containing nonfinite values. Imported math behavior, signaling NaNs, exception ordering of independent arithmetic and unmasked floating exceptions are outside the conditional vectors.

All original coordinate/category and hash arithmetic retains its native width. Input X/Y are signed low-32-bit values; radius/reference fields are converted from signed 32 bits. The category mask range checks are unsigned. The descriptor variant is compared as a 32-bit selector. The grouping test reads one byte. Variant integer radius squares, grid arithmetic, coordinate hash shifts/products/sums, sign-bit clearing and unsigned remainders retain low 32-bit integer behavior where described. Fixed-point displacement differences and additions in variants 2/3 use signed 64-bit operands and 16-bit shifts before binary32 conversion. Width-specific truncating conversions, negative fractional floor correction and possible machine conversion failure remain distinct from a mathematical floor on every real number. No inferred C++ undefined behavior is used to discard an original branch.

The source does not clamp normalized X before acosf, guard zero radial distance before normalization, lower-bound all upper-only envelopes, enforce an iteration limit in terracing, or prove nonoverflowing arbitrary integer radius squares. No universal result for malformed descriptor fields, out-of-range conversions, infinities or all signed coordinates is inferred. The worked fields are literal synthetic conditions, not a claim of loader/producer validity.

Only the ordinary normal-return cookie path was closed. Failure owner 0x393534 and imported math internals are untraced. Noise provider 0x2E0C0, reference provider 0x2BA200, pair provider 0x2BAF90, relief helper 0x2B5670, boolean helper 0x2B7730, terrace helper 0x2A2C50 and the preparation owner's other provider bodies remain explicit dependencies in this independent audit. Their internal World-table relationships, seeds/draws, global initialization, ownership, locks, cache lifetime and concurrency are not inferred from names. The two selected owners expose no World lock or global provider snapshot. Static program order establishes the direct call ordering; no wall-clock, frame-interleaving or repeated-call equality guarantee is added.

The complete call metadata for the selected preparation owner contains 17 direct call sites including its guard. The final-height owner contains 25 including its guard. These counts identify static call instructions, not a claim that every site is reached in one invocation. No alternate caller census or universal sole-caller claim is made beyond the fully selected 0x35EA0 route.

The original constant catalog below identifies the exact encodings used in the selected owners. Decimal strings identify the encoded values and do not replace their bits. The sign-clear-mask row is data for a bit operation, not a floating NaN operand.

| Binary32 constant RVA | Image bits | Encoded value description |
| --- | --- | --- |
| 0x42E510 | 0x37800000 | 1.5258789E-05 |
| 0x42E524 | 0x3D4CCCCD | 0.05 |
| 0x42E528 | 0x3DCCCCCD | 0.1 |
| 0x42E580 | 0x40400000 | 3 |
| 0x42E584 | 0x40A00000 | 5 |
| 0x42E598 | 0x41200000 | 10 |
| 0x42E5A0 | 0x41A00000 | 20 |
| 0x42E5A4 | 0x41C80000 | 25 |
| 0x42E5A8 | 0x41F00000 | 30 |
| 0x42E5BC | 0x42C80000 | 100 |
| 0x42E5F8 | 0x47800000 | 65536 |
| 0x42E60C | 0xC0000000 | -2 |
| 0x42E6FC | 0x3E19999A | 0.15 |
| 0x42E704 | 0x3E99999A | 0.3 |
| 0x42E708 | 0x3F19999A | 0.6 |
| 0x42E710 | 0x3F400000 | 0.75 |
| 0x42E714 | 0x3F59999A | 0.85 |
| 0x42E7A4 | 0x41700000 | 15 |
| 0x42E7D4 | 0x43340000 | 180 |
| 0x42E804 | 0xBE800000 | -0.25 |
| 0x42E938 | 0x43160000 | 150 |
| 0x4328AC | 0x3F333333 | 0.7 |
| 0x432924 | 0x41180000 | 9.5 |
| 0x432D68 | 0x3D75C28F | 0.06 |
| 0x46D654 | 0x3CCCCCCD | 0.025 |
| 0x4725C0 | 0x49742400 | 1000000 |
| 0x4725E8 | 0xC3800000 | -256 |
| 0x4AC954 | 0x38D1B717 | 0.0001 |
| 0x4AC95C | 0x3E800000 | 0.25 |
| 0x4AC964 | 0x3F000000 | 0.5 |
| 0x4AC980 | 0x3F800000 | 1 |
| 0x4AC9D0 | 0x40000000 | 2 |
| 0x4ACA20 | 0x40800000 | 4 |
| 0x4ACA24 | 0x40C00000 | 6 |
| 0x4ACA34 | 0x41800000 | 16 |
| 0x4ACA4C | 0xBF800000 | -1 |

| Binary64 constant or mask RVA | Image bits | Encoded value description |
| --- | --- | --- |
| 0x42E540 | 0x3F947AE147AE147B | 0.02 |
| 0x42E758 | 0x400921FB54442D18 | 3.141592653589793 |
| 0x42EED0 | 0x3F847AE147AE147B | 0.01 |
| 0x438258 | 0x40C1140000000000 | 8744 |
| 0x43EF98 | 0x401921FB54442D18 | 6.283185307179586 |
| 0x446160 | 0x40A0AA0000000000 | 2133 |
| 0x4721C8 | 0x3F8EB851EB851EB8 | 0.015 |
| 0x4721D0 | 0x3F9999999999999A | 0.025 |
| 0x472218 | 0x4041800000000000 | 35 |
| 0x472238 | 0x407BC00000000000 | 444 |
| 0x472258 | 0x408C400000000000 | 904 |
| 0x472290 | 0x40AA460000000000 | 3363 |
| 0x4722F8 | 0x40BA790000000000 | 6777 |
| 0x472308 | 0x40BD320000000000 | 7474 |
| 0x472320 | 0x40BE3F0000000000 | 7743 |
| 0x472328 | 0x40BEC20000000000 | 7874 |
| 0x472338 | 0x40C1138000000000 | 8743 |
| 0x472340 | 0x40C11A8000000000 | 8757 |
| 0x472358 | 0x40C15B8000000000 | 8887 |
| 0x472360 | 0x40C1728000000000 | 8933 |
| 0x472388 | 0x40C1BF8000000000 | 9087 |
| 0x4723A8 | 0x40C55F8000000000 | 10943 |
| 0x4723B8 | 0x40C7E48000000000 | 12233 |
| 0x472460 | 0x40F5746000000000 | 87878 |
| 0x472480 | 0x40F8081000000000 | 98433 |
| 0x472488 | 0x40F8212000000000 | 98834 |
| 0x472498 | 0x40F82A8000000000 | 98984 |
| 0x4724D0 | 0x410C97D000000000 | 234234 |
| 0x472528 | 0x4150F76A40000000 | 4447657 |
| 0x472568 | 0x4197791F68000000 | 98453466 |
| 0x4AC978 | 0x3F747AE147AE147B | 0.005 |
| 0x4ACB10 | 0x7FFFFFFFFFFFFFFF | 64-bit sign-clear mask |

Binary32 positive zero and exact 1 also occur as immediate words in the selected local stores. The mode masks 1, 2, 4 and 8, category membership masks, hash salts, hash multipliers, integer step/range constants and direct target RVAs were checked in the byte-matching instruction records. No stored constant encodes a complete numerical closure of a called provider.

The complete selected-native-range manifest follows. End RVAs are exclusive. Each range includes every original instruction through its native return or failure tail boundary, with no internal gap; the raw image correspondence count excludes unrelated alignment outside those ranges.

| Owner RVA | Exclusive end RVA | Instructions | Matched original bytes | Mismatched bytes |
| --- | --- | --- | --- | --- |
| 0x35EA0 | 0x36157 | 146 | 695 | 0 |
| 0x2B4450 | 0x2B5663 | 1019 | 4627 | 0 |
| 0x393030 | 0x393051 | 8 | 33 | 0 |

The two requested owners total 1,165 instructions and 5,322 matched bytes. Including the separately selected normal-return guard gives 1,173 instructions and 5,355 matched bytes. Full original C-export and instruction-body read coverage for these owners was completed before final numerical interpretation. Raw-image constant reads and original call metadata were selected by these owners and direct operand addresses. Original assembly content was searched only for the individually selected owner identifiers.

| Exact evidence container | Bytes | SHA256 |
| --- | --- | --- |
| C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled/functions/140035ea0.c | 2807 | D98D675CBA73138B27E3BCA3860612527F684792CBB1300C550CB7DCE7BB3F00 |
| C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled/functions/1402b4450.c | 18277 | CB4DE0706A18B2EB1D5C43145F88E98CB23A55EE218E3135104E9E7B87C9ACC9 |
| C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled/functions/140393030.c | 565 | 81859DB821EC73B2A18D9DB997EB03800B0E66D1B08EA87A19550788BBF43DBD |
| C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled/assembly.txt | 62547291 | FE07326A0F4585432A9D4E42E7CF16C570815A8FCD164DE36616095023A0740C |
| C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled/calls.csv | 10840369 | CF410A7DD84BBEF0E9FC4FC6FD1FF3B4903D97E9126C8F1A1D69EF05CD566DCB |
| C:/Users/DieuDeGlace/Documents/Cubeworld-Vulkanengine-Steam/docs/CLEANROOM_AUTHOR.md | 3344 | E92910FC1E5566A052746F4D215637B7AE5468F471677E3DF1F4D3C0E2941933 |

The whole-container hashes identify original-side evidence files without claiming that unrelated functions or call records were semantically inspected. The original executable identity is given at the start of this contract. The clean exposure statement and named dependency limits apply to the entire contract, including its conditional mathematical vectors. Release occurs only after the new saved file is fully read back and its exact byte length and SHA256 are reported. All earlier held artifacts remain unread and unchanged.
