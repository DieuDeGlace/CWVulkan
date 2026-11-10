Original sampled-control consumers and Game admissions — supported Steam image

Status: FINAL / STABLE / HELD after the saved-file readback and identity checks reported with this contract. This is an independent original-source behavioral contract. It contains no implementation proposal, pseudocode, runtime observation or replacement-engine comparison.

Authority, identity and exposure

The authority is the original unpacked AMD64 Steam executable at C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. Its exact length is 5,757,952 bytes and SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Its PE machine is 0x8664, image base 0x140000000, timestamp 0x5D93AB0D, entry RVA 0x3939F0 and SizeOfImage 0x58A000. Addresses in this contract are RVAs unless expressly identified as field offsets.

I read the standalone docs/CLEANROOM_AUTHOR.md policy first and used only individually selected original-side executable/export evidence for this assignment. I did not read prior held handoffs, active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, combined history, replacement implementation, exporter/extractor, tests/results, owner data/assets/configuration/captures/outputs, SDK/provider/dependency implementations or reconstruction source. No game or provider was executed. No implementation or runtime feedback was requested or received. Earlier held artifacts were neither opened nor changed.

The direct-address census used the original assembly export to locate the 23 field operands, then followed proven Game receiver aliases and selected native call joins. An operand resembling one of these offsets on a stack or unrelated receiver was not treated as a Game field. The selected owner ranges were compared instruction by instruction against raw PE bytes; the continuous-range results and limits appear at the end. This is a census of resolved exported operands and selected joins, not proof that computed addressing, bulk stores, indirect callers or uninspected code cannot access the same storage.

Receivers and distinct kinds of state

G denotes the Game constructed at 0x7EA80. The current pointer at G+0x8 is W; the current pointer at W+0x448 is E. These names denote the current loads at each source site. They do not promise that a callback or another thread leaves either pointer unchanged. The Interface subobject I begins at G+0x118: the Game constructor calls 0xF8690 on that address, and that complete constructor stores G at I+0x5C0. M is the current pointer at G+0x20.

The 23 independently stored bytes occupy G+0x1A70 through G+0x1A86, inclusive. The checked setter 0xA36C0 accepts an unsigned 32-bit index below 23 and stores the low eight bits of its value argument at the selected byte. It does not canonicalize all nonzero values to one. The consumers below normally test zero versus nonzero. They do not implement a shared pop, acknowledge or clear operation on the sampled array.

The separate 256-byte raw window-key array starts at G+0x1A87. The Game constructor explicitly clears those 256 bytes. That clear does not cover the preceding 23-byte array. The selected constructor evidence does not establish a whole-Game zeroing rule or the initial values of all 23 sampled bytes. Keydown 0x96E70 writes one to the raw array at the unsigned low-eight-bit key; keyup 0x97870 writes zero at the same index. Neither is the indexed sampled-state setter.

A selected node integer is a four-byte load or store through the node's pointer at +0x68, its signed 32-bit selected index at +0xAC, and its integer-array pointer at +0xE0. A widget's root is the pointer at widget+0x178, followed by that same node operation. There is no added bounds or null check in the small helpers 0x9AF00 and 0xA3A20. The former returns the selected integer's nonzero test in its low byte; the latter stores a four-byte zero or one from its low-byte argument. Upper return bits from the former are not a clean Boolean contract.

S is the direct node I+0x1F8, hence G+0x310. Q is the widget I+0x540, hence G+0x658. The setup at 0xF9AC0 publishes Q after its constructor 0x275A70; Q+0x1E8 stores G. The original RTTI at the selected table identifies Q as cube::ControlsWidget. Its signed four-byte Q+0x2E0 is a capture-row operand, distinct from Q's root selection and from M's retained pointers. The constructor establishes Q+0x2DC and Q+0x2E0 as -1 and clears Q+0x2D8.

C is the widget I+0x280, hence G+0x398, constructed at 0x26A2E0 and published by the selected setup. Its original RTTI identifies cube::ChatWidget. The constructor clears its byte +0x1D8. That byte is not Entity E+0x1D8: the latter is an unrelated actor-owned aggregate reached by some action branches.

H is the widget I+0x278, hence G+0x390, constructed by 0x277EC0. The selected original RTTI identifies cube::HelpWidget, H+0x1E8 stores G, and primary virtual slot +0x8 points to 0x278040. The setup supplies a separately created root through 0x33CD30 on current M, with the direct I+0x108 node (G+0x220) among that selected creation call's operands, then initially selects H's root zero. The deeper node-creation/parenting helper is an explicit boundary here. This does not identify H with the direct node G+0x170. Offsets or an English widget name are insufficient to merge these receivers.

Actual accessors and distinct admission tests

No generic indexed sampled-byte getter is established by the selected exact-operand and resolved-call census. The gameplay and menu consumers below read their current bytes directly. Several nearby accessors instead report interface admission:

• 0x9AE50 loads current Q and returns full unsigned 32-bit one when signed Q+0x2E0 is nonnegative, otherwise zero. It does not test Q's root, M's pointers, row less than 23, or any sampled byte. The resolved original callers are 0x1355A0 at 0x13667C and Interface update 0x103200 at 0x103289.

• 0x9AE70 adjusts G to I and tail-jumps to 0x1014F0. Its observable result is the latter's low byte, despite the C export's void presentation. The selected direct caller is Game update 0xA5A50 at 0xAD792. It is not a sampled-state getter.

• 0x342090 returns the eight-byte pointer at M+0x170. The complete GUI key and character dispatchers 0x340DF0 and 0x340DD0 use that pointer as their keyboard/character target, test target byte +0x1A0, and call virtual slots +0xC0 and +0xD0 respectively. The controller's nonnull test of this pointer does not repeat the target-active test. Thus a nonnull inactive target can still satisfy the controller's early-return test.

• 0x33F1E0 returns the retained pointer M+0x168 when nonnull. Otherwise it performs 0x333F60 against root M+0x140 with the current point at M+0x180. This retained hit/capture override is a different operand from the keyboard/character target M+0x170. The complete selected GUI press/release bodies 0x340EC0 and 0x3410E0 route through retained or queried targets and invoke virtual/event callbacks, with later rereads. They do not establish that the two pointers are interchangeable. Complete capture creation, retirement and general hit geometry are outside this contract.

The complete composite test 0x1014F0 takes I, short-circuits at the first nonzero operand, and produces low-byte one or zero. In this exact order it tests: direct nodes at I+0, +0x18, +0x58, +0x80 and +0xE8; byte I+0x550; widget roots at I+0x218, +0x228, +0x270 and +0x238; byte I+0x308; widget roots at I+0x258, +0x250 and +0x248; C+0x1D8; widget roots at I+0x260, +0x240, +0x540 and +0x538. In G terms its first five nodes are G+0x118, +0x130, +0x170, +0x198 and +0x200; its two direct bytes are G+0x668 and G+0x420. The widget-root G offsets are respectively +0x330, +0x340, +0x388, +0x350, +0x370, +0x368, +0x360, +0x378, +0x358, +0x658 and +0x650.

This composite has no direct test of S, H's root, M+0x168, M+0x170 or Q+0x2E0. Its low byte is a composite of the stated selected integers and bytes, not a proof of GUI focus, OS capture or universal control prohibition. Nonzero selected integers of either sign satisfy its test. Its upper return bits are not normalized.

Ordinary sampled-refresh and frame-update joins

The selected ordinary application owner 0x1355A0 handles the native message pump before its device/sampling block. In the window procedure 0x133C70, messages 0x100 and 0x101 first deliver a 16-bit key to the GUI, then the low-eight-bit key to Game keydown or keyup; no GUI return value suppresses that subsequent Game call. Repeated keydown messages consequently rewrite a raw one. The sampled-state consumers are not called once per such message merely because a window message occurred.

In the selected ordinary device block, foreground/process admission and provider-return paths precede the Q+0x2E0 getter. When 0x9AE50 returns zero, the loop's 23-state path refreshes entries through 0xA36C0, including later explicit resets of indices 21 and 22 before selected controller-provider handling. When it returns one, a different binding-capture path is selected, and this 23-state refresh/reset path is skipped. The getter itself does not clear old sampled bytes. Selected successful, failed and unsupported imported device branches have their own producer behavior; this contract does not rederive their numeric categories or provider guarantees.

Both selected paths converge to the ordinary frame helper 0x134420 after previous-buffer bookkeeping. Therefore a skipped refresh is not, by itself, a skipped frame update or a forced release of all sampled levels. The frame helper has its own global-pointer and clock admissions. With its required original globals nonnull and its selected timing path reached, it calls 0xA5A50 directly at 0x134558 with the Game receiver and a binary64 elapsed input derived from QueryPerformance counter/frequency work. That calculation scales its seconds quantity by binary64 1000. No vtable claim is needed for this join.

At entry to 0xA5A50 the native binary64 minimum instruction caps ordinary finite elapsed input above at 1000; no lower clamp is established there. The capped value is saved and reloaded. The local-action call at 0xA9B2C passes it to 0x8AE50; the latter forwards the same binary64 operand to 0x9C050 at 0x8BDBB. The later 0xB40D0 call at 0xAA1BC and selector timer work use reloads of that saved elapsed operand, rather than the C presentation's potentially misleading reused local names. Relevant reloads include 0xA9F96, 0xAA006 and 0xAA53B. These facts specify call operands and source order, not exact real-time spacing or a clock/provider scheduling guarantee.

The update first performs the selected index-21/22 bookkeeping described below, before its later World substeps and local controller call. Admission to the selected 0x8AE50 call requires W+0x210 nonzero, current E+0x68 not 0x9C or 0x9D, and the selected integers of direct nodes G+0x118, G+0x130 and G+0x170 all zero. Those are three specific page/root tests, not the complete composite. Other World work can already have occurred before a failed local-action admission.

Inside 0x8AE50, the call to 0x9C050 is reached when the selected current E+0x130 and E+0x134 binary32 comparisons both admit nonpositive ordinary finite values. The caller has already performed other selected target/World work. Immediately after that call, 0x8AE50 separately returns when current M+0x170 is nonnull, then when current C+0x1D8 is nonzero. Thus even these outer early returns do not erase controller work already performed. The later 0xB40D0 call, late interface toggles and Interface update are separate consumers with their own gates.

Index-to-consumer map

All offsets in this table are relative to G. “Rising” means that the selected consumer's own retained byte was zero and its currently read sampled byte is nonzero. “Falling” means current zero after that consumer's retained byte was nonzero. This terminology does not describe a common producer-generated event.

| Index | Sampled offset | Direct selected downstream use |
| --- | --- | --- |
| 0 | 0x1A70 | 0x8AE50 effective action level and later release tests; 0xB40D0 falling test |
| 1 | 0x1A71 | 0x8AE50 direct action level, expiration/release gates and selected helper call |
| 2 | 0x1A72 | 0x9C050 guarded actor changes; 0x8AE50 helper argument, held/old checks; cached argument in H draw 0x278040 |
| 3 | 0x1A73 | 0x8AE50 rising latch 0x5515A2; Interface publication 0x103200 |
| 4 | 0x1A74 | 0x8AE50 rising latch 0x5515A5; Interface publication |
| 5 | 0x1A75 | 0x9C050 level/forward override; later Game interface/selector clearing |
| 6 | 0x1A76 | 0x9C050 level; later Game interface/selector clearing |
| 7 | 0x1A77 | 0x9C050 level; later selector publication and repeat |
| 8 | 0x1A78 | 0x9C050 level; later selector publication and repeat |
| 9 | 0x1A79 | 0x9C050 level plus three retained global operands; 0x8AE50 actor-bit gate |
| 10 | 0x1A7A | 0x9C050 composite-dependent candidate; 0x98180 return gate; 0x8AE50 rising latch 0x5515A8 |
| 11 | 0x1A7B | Separate rising latches in 0x98180 and 0x8AE50; falling latch in 0xB40D0; Interface publication |
| 12 | 0x1A7C | 0x8AE50 rising toggle of actor word +0x128 bit 1 |
| 13 | 0x1A7D | 0x8AE50 actor-bit/effective-action gates; 0x98180 return gate; 0x97CC0 branch |
| 14 | 0x1A7E | 0x8AE50 rising latch 0x5515A4 |
| 15 | 0x1A7F | Late Game rising callback 0x1031E0 |
| 16 | 0x1A80 | Late Game rising toggle of S |
| 17 | 0x1A81 | Late Game rising callback 0x102FE0 |
| 18 | 0x1A82 | Late Game rising callback 0x102F10 |
| 19 | 0x1A83 | Late Game rising callback 0x103150 |
| 20 | 0x1A84 | Late Game rising callback 0x1030C0 |
| 21 | 0x1A85 | Early Game rising decrement of G+0x1E04 |
| 22 | 0x1A86 | Early Game rising increment of G+0x1E04 |

No English binding token is used here to assign a gameplay meaning to an index. The map describes proved operands and calls. Each consumer can see a later value after a callback or source reread; the 23 bytes are not locally copied as one immutable snapshot by these owners.

Complete selected controller owner 0x9C050

The complete body, including every normal return, was read and byte-matched over 0x9C050–0x9D6EA. It does not begin with a universal “input enabled” test. Its early actor preparation precedes the keyboard-target, Chat, S and map/process admissions.

For the following account, the pure original predicate at 0x5EE70 is called P. It admits E only when byte E+0x68 is 0x78, flag mask 0x1 at E+0x5C is clear, and the binary32 values E+0x130 and E+0x12C admit nonpositive ordinary finite comparisons. This predicate does not query GUI state. All actor state identifiers below are byte values, flag tests use the stated native masks, and actor scalar/vector components mentioned below are binary32 unless another width is given.

First, the owner initializes its coefficient to 80 and calls P on the currently loaded actor. When P is false and state is 0x79, it may clear the three components E+0x40/+0x44/+0x48: their squared length must be strictly below 225, sampled indices 5, 6, 7 and 8 must all be zero, and the squared sum of G+0x1D6C/+0x1D70 must be strictly below the binary32 word 0x3C23D70A. It then selects coefficient 5 for state 0x79. If that three-component squared length is strictly above 1600, it calls the imported square-root boundary, divides one by the returned binary32 value, normalizes the components, and scales them by 40. The normalization stores +0x44, then +0x40, then +0x48; the subsequent scaling stores +0x40, +0x48, +0x44. If P is false and state is not 0x79, it clears all three components. Equality at the two length thresholds does not take the respective strict branch.

When the first P call is true, the owner instead reads E+0x34/+0x38. A positive horizontal squared sum admits square-root normalization. It calls 0xA7A0 with the carried binary64 elapsed operand and binary32 word 0x3A83126F, then uses its returned binary32 factor to blend +0x40 toward normalized first component times 10, stores that result, reloads the current actor and blends +0x44 toward normalized second component times 10. It subsequently forms the original cross-component terms, including literal-zero multiplications, scales by E+0x2C and word 0x3D4CCCCD, and subtracts/stores at +0x44, +0x40 and +0x48 in that order. The current actor is reloaded between the blend stages and again before these later operands.

Still in that true-P branch, E+0x9A0 strictly below word 0x3A83126F admits a normal early return. It clears mask 0x40 of the two-byte word E+0x128, calls 0xA7A0 with elapsed and word 0x3B03126F, blends E+0x48 toward zero and returns. Equality does not take this return. The later process-global counters and previous-index-9 byte are not updated by this return.

The owner calls P again. If true, it performs a second scalar preparation before any GUI admission. Index 9 nonzero, E+0x9A0 positive and E+0x3C at most 5 select one route. It multiplies E+0xBB4 by word 0x3A83126F, subtracts binary32 0.5, clamps the ordinary finite result to zero through one, multiplies by word 0x3B03126F, widens the product, multiplies by the binary64 elapsed operand and by the widened nonnegative negation of E+0x3C, adds widened old E+0x48, and narrows the result to E+0x48.

Otherwise, index 5 nonzero supplies a forward operand of one; when it is zero, the current G+0x1D70 supplies that operand, and only a positive value admits the increment. This route uses the same clamped E+0xBB4 term, multiplying first by the selected forward operand and then binary32 word 0x3AC49BA6 before the binary64 elapsed/nonnegative-negation stages and final narrowing. After this alternative route, 0xA7A0 with elapsed and word 0x3B03126F blends E+0x48 toward zero. Both second-P routes then cap E+0x48 to 12 only when E+0x3C is positive and E+0x48 is strictly above 12. These operations can therefore occur even when a later interface guard prevents the directional branches.

The following four return tests then occur in order:

• Return if current 0x342090(M) is nonnull.
• Return if current C+0x1D8 is nonzero.
• Return if S's selected four-byte integer is nonzero.
• Return if current byte G+0x668 is nonzero and current byte G+0x1958 is also nonzero.

These are separate current reads. There is no direct Q+0x2E0 capture-row test in this owner. There is no direct M+0x168 test. The last conjunction does not prohibit the subsequent work merely because G+0x668 alone is nonzero.

After these tests, a third P call selects coefficient narrowed from binary64 elapsed times binary64 4 when true; when false, the earlier coefficient 80 or 5 remains. The owner copies the original 16-word identity matrix at 0x42E630–0x42E66F into local storage and calls 0x13050 with the narrowed current binary64 G+0x1D90. The complete matrix helper multiplies that binary32 angle by word 0x3C8EFA35, calls the original cosine boundary before the sine boundary, and rotates the first two columns in component order. Imported trigonometric behavior is not supplied by this contract.

Next it calls the full Interface composite on I. This does not become a return when true. It only participates here in forming a retained local candidate: that candidate requires composite low byte zero, current index 10 nonzero, E+0x5C mask 0x1 set, E+0x12C equal to zero and E+0x9A0 at least 0.25. Directional work still follows even when the composite is nonzero.

The directional level branches are visited in index order 5, 6, 8, 7. Each active branch reloads W and its current E. A branch can change actor components before the next branch rereads actor state, flags or Game basis operands. Opposite nonzero sampled bytes are not resolved to a single winner.

For ordinary finite operands, the source relationships of these branches are:

| Actor branch | Index 5 | Index 6 | Index 8 | Index 7 |
| --- | --- | --- | --- | --- |
| State 0x7A and E+0x5C mask 0x4 set | Add coefficient times word 0x3E4CCCCD to +0x48 | Subtract the same scaled coefficient from +0x48 | Subtract the selected side terms formed from E+0x9AC/+0x9B0/+0x9B4 | Add those selected side terms |
| Otherwise mask 0x2 clear, or state 0x79 | Add the negative second rotated local basis column, scaled by coefficient | Subtract that same negative second column | Subtract first rotated local basis column, scaled by coefficient | Add first rotated local basis column, scaled by coefficient |
| Otherwise | Add third stored Game basis vector +0x1EC4/+0x1EC8/+0x1ECC, scaled by coefficient | Add its negation, scaled by coefficient | Add negated first stored Game basis vector +0x1EA4/+0x1EA8/+0x1EAC, scaled by coefficient | Add first stored Game basis vector, scaled by coefficient |

The middle route individually suppresses a directional change for state 0x78 with mask 0x1 clear and nonpositive E+0x9A0. For the last route the other stored vector at G+0x1EB4/+0x1EB8/+0x1EBC also participates through literal-zero products. In the first route the side terms retain literal-zero products involving the actor's third component and combine the second component, negated first component and zero-derived third component before coefficient times word 0x3E4CCCCD scaling. The table states finite source relationships, not permission to algebraically remove those native zero products for infinities, NaNs or signed-zero questions.

The digital component stores generally visit +0x40, +0x44, +0x48. Index 7's ordinary final join stores +0x48, +0x44, +0x40. All four branches are level tests on their current bytes; none updates a common previous-direction latch.

After those branches, the owner separately evaluates the current squared sum of G+0x1D6C and +0x1D70. Ordered zero skips the next work; its explicit unordered branch instead enters it, assuming execution continues without a floating exception. In state 0x7A with mask 0x4, it uses sign-negated G+0x1D6C in the side terms, scales by coefficient times word 0x3E4CCCCD, adds into +0x40/+0x44/+0x48 and then reloads the actor for a further +0x48 increment using G+0x1D70. Otherwise mask 0x2 clear selects the rotated local basis route, using sign-negated first and second Game float operands. State 0x78, mask 0x1 clear and nonpositive E+0x9A0 suppress that route. Mask 0x2 set selects the stored Game basis route with negated first float and positive second float. Unlike the digital branches, this analog-pair branch has no state-0x79 override of its mask-0x2 choice. The ordinary final component stores are +0x48, +0x44, +0x40.

The owner then changes actor state 0x36 to zero. If the previously formed index-10 candidate is true, it now retests current three-component +0x40/+0x44/+0x48 length for positivity, current E+0x9A0 for at least 0.25 and current E+0x12C for nonpositivity. Success stores state zero, reloads and normalizes only the horizontal +0x40/+0x44 pair through the square-root boundary, scales it by 15, and publishes actor +0x34, +0x3C, +0x38 in that order. For ordinary nondegenerate finite input the middle component is 5; the native path retains its normalization-derived zero terms. It then reloads current E to store +0x12C as binary32 600, reloads again and subtracts binary32 0.25 from +0x9A0. A positive three-dimensional length alone does not prove the horizontal divisor is nonzero.

The final index-9 retained-state section is also part of this complete owner. It first selects a signed 32-bit delay of 100. State 0x80, a nonnull current-World lookup by E+0x1A8, and target byte +0x80 mask 0x2 set select delay 500 instead. The lookup helper 0xE720 is an explicit current-World dependency; this contract does not extend its lifetime guarantees.

If current index 9 is zero, process-global signed four-byte counter 0x551598 is set to zero. The same happens when state is 0x33 and E+0x6C is at most the binary32 conversion of 0x4F220's numeric return. Otherwise the owner first checks state 0x7A with previous-index-9 byte 0x551594 zero. That rising combination stores state zero. If the actor flag mask 0x1000 is set, it copies the E+0x9AC/+0x9B0/+0x9B4 components scaled by 15 into each of actor triples +0x34/+0x38/+0x3C, +0x40/+0x44/+0x48 and +0x4C/+0x50/+0x54. Within each triple the stores are second, first, third. It reloads current E between triples, stores +0x12C as 600, calls 0x17840 with the current +0x30 increased by 180 and the supplied one operand, publishes the returned word to the selected actor +0x30, then subtracts 25 from current +0x9A0 and clamps a nonpositive finite result to zero. The exact transform inside 0x17840 is not closed here.

The owner adds elapsed to the signed old 0x551598 in binary64, truncates to signed 32 bits and stores the new counter before testing positivity. A positive value below 200, combined with mask 0x1 set or E+0x12C nonzero, and nonpositive E+0x3C, admits the following changes. It stores +0x3C as 5 for state 0x53 or for states 0x86/0x87 with E+0x6C strictly below 0x4F220's converted return; otherwise it stores 10. Within this same admission it tests index 2. If nonzero, E+0x9A0 at least 0.25 and byte pair E+0x14C/+0x14D equal to 4/1 or 2/1, it stores +0x12C as 600, +0x3C as 20 and subtracts 0.25 from current +0x9A0. It then rereads index 2 for a separate condition: nonzero, state not 0x9F, +0x9A0 at least 0.25 and byte +0x14C equal to 1 store state 0x9F, +0x6C zero, +0x3C as 20 and subtract 0.25. These are ordered tests, not an exclusive abstract action choice. It sets signed global 0x55159C to the selected delay and clears the three components +0x4C/+0x50/+0x54.

Still when the carried counter result is positive, mask 0x2 set and E+0x3C strictly below 10 set 0x55159C to delay and add elapsed times the binary64 constant at 0x437F90 to widened old +0x3C before narrowing. That constant is exactly the binary64 value 0.0010000000474974513. The native route does not immediately clamp this increment at 10.

Next, independently of the counter-reset route above, signed 0x55159C positive and current index 9 nonzero admit another test. State 0x53 is excluded; states 0x86/0x87 additionally require E+0x6C at least the converted 0x4F220 return. If admitted and current E+0x3C is below 10, the owner adds elapsed times binary64 0.07999999821186066, narrows to +0x3C, reloads the current actor and clamps a result at least 10 to exactly 10. Finally it subtracts elapsed from signed old 0x55159C in binary64, truncates to signed 32 bits, clamps a negative integer result to zero, and stores a freshly reread index-9 byte to global 0x551594. Every earlier normal return skips this final previous-byte store and timer decrement.

The selected original numeric dependencies

0xA7A0 is an original complete iterative scalar helper, not an imported easing guarantee. It derives a signed iteration count by truncating binary64 elapsed times binary64 100, starts a binary64 accumulator at zero, advances through the original increasing unit numerators over that count, and returns a binary32 result. Its eight-step unrolled portion precomputes widened supplied factor times binary64 0.02; its tail retains the separate multiply-by-factor then multiply-by-0.02 association. Nonpositive counts have no iterations and return zero. The selected controller supplies word 0x3A83126F to the first blend call and word 0x3B03126F to the zero-blend calls. Overflow, exceptional conversion and huge/negative supplied elapsed domains are not normalized into an abstract duration contract.

0x13050, the square-root, sine and cosine boundaries, 0x4F220, 0xE720 and 0x17840 are distinguished from sampled-state storage. The complete native selected controller determines when they are called and when its current actor is reloaded. Apart from the complete small helper account just given and the matrix helper's source operations, their deeper numerical, lookup, actor-lifetime or provider closures are not supplied. No result for arbitrary malformed actor fields is inferred.

Selected action owner 0x8AE50 and independent index-11 latch

The complete exported owner ranges were byte-matched, while semantic closure is limited to the sampled-byte branches and their admission joins. The preceding target scans, spatial queries, event-record production and the bodies of the called action helpers are not a full gameplay specification. Where a helper's result selects an action below, the result is an explicit dependency.

Before its selected controller join, 0x8AE50 can itself clear E+0x40/+0x44/+0x48 when P is false and state is not 0x79. Following the controller call and the keyboard-target/Chat return tests described earlier, true P with E+0x9A0 below word 0x3A83126F admits another return; state 0x7D with E+0x6C below binary32 2000 admits a return as well. These returns also skip later latches belonging to this owner.

The next selected word operation concerns E+0x128 mask 0x40. For state 0x78 with flag mask 0x1 clear, the mask is set only with positive E+0x9A0 and nonzero current index 9; otherwise it is cleared. Outside that special state/flag case, current index 13 zero sets the mask and nonzero clears it. This is a direct current-level effect, distinct from the controller's retained index-9 logic.

Index 12 nonzero with retained byte 0x5515A1 zero toggles mask 0x1 of the two-byte E+0x128 word. The owner then stores freshly read index 12 to that retained byte. A surrounding state-expiration branch also reads index 1 zero. Its excluded byte-state set is 0x09, 0x1D, 0x1E, 0x24, 0x25, 0x50, 0x51, 0x53, 0x56, 0x57, 0x58, 0x5F, 0x76, 0x78, 0x79, 0x7A, 0x80, 0x92, 0x98, 0x9C and 0x9D. Outside that set, index 1 zero and E+0x6C above both 0x4F220's converted return and binary32 10000 can clear state to zero. The state set describes this branch only.

Index 3 uses retained byte 0x5515A2. On its rising admission, a nonzero result from 0x656E0 is queried again and then admitted by 0x507B0 with the original one argument. Selected numeric results 0x6D and 0x8D reach 0x29D3C0 on the current W/E. Other accepted results are stored as the actor state byte, with E+0x6C and +0x12C zeroed. Subsequent selected actor bookkeeping uses 0x50BD0 and clears G+0x1E60. The latch store rereads index 3 after that work. No provider or actor action meaning is inferred from a numeric result.

Later selected target/record preparation reaches 0x98180. Its complete native body was matched, but only its entry admission and its returned-low-byte join are closed here; its large action payload is not. Importantly, it snapshots index 11 into a byte at entry, computes a local rising condition using retained byte 0x5515B4, and stores that cached sampled byte to 0x5515B4 before its later gates. The native store is at 0x981EC, before the index-10 test at 0x981F2, index-13 test at 0x981FF, and selected-root test through the current G+0x418 object at 0x9822C. Those selected returns produce low-byte zero. Thus they can consume this helper's notion of a rise even though another caller-side latch has not yet been updated.

The caller has its own independent index-11 retained byte 0x5515A3. When 0x98180 returns low-byte zero and S is selected, an index-11 rise admits 0x93E60; a nonempty returned pair collection supplies the current G+0x20C0 selection for 0xB7740 on the selected current World/actor record, and S is reloaded and hidden. When S is not selected, other original candidate routes use the G+0x1E70 sentinel/current point-query result, the selected G+0x418 root, current actor state, E+0x1528, target type and relation/helper returns. Depending on those guards, the rise can clear states 0x78/0x79/0x76/0x7A, copy a selected record into the actor-owned E+0x1D8 aggregate and select state 0x78, 0x79 or 0x89 with +0x6C zero, or reach further selected record/query work. These candidates are not reduced here to a screen hit or unconditional action. Their imported/query and action closures remain dependencies.

When 0x98180 instead returns nonzero, the caller has a different selected-record route: S unselected permits clearing the current point-selection fields; an index-11 rise, positive current E+0x180, nonnegative signed G+0x1E7C and the selected G+0x418 root unselected admit 0x2C3150(W, current G+0x1E7C). The common reached join at 0x8EB40 stores a freshly read index-11 byte to 0x5515A3. The separate 0x98180 byte is not substituted for it.

Index 14 then uses 0x5515A4. Its rise requires E+0x6C above 0x4F220's converted result before the selected changes. State 0x76 can be cleared. Otherwise a current E+0x9D8 target lookup, distance-squared comparison below 36 and accepted 0x5E890(E,3) can select state 0x76. The alternative selected branch writes first byte 0x21 into the actor-owned +0x1D8 aggregate, selects state 0x9B, clears +0x6C, calls 0xA0420 with numeric identifier 0x7C, and reaches the original type-0x10 record insertion route through actor+0xAB8. The latch is stored from current index 14 after the branch. Target lookup, event payload, audio/resource and queue internals are not closed by their names.

Index 4 uses 0x5515A5. On its rise, nonempty 0x93E60 results admit the selected 0xB7740 route and a later reload hides S. Its latch is then stored from a fresh index-4 read.

Only now does 0x8AE50 call the complete Interface composite. A true low byte causes cleanup and return before the following action-level section and before stores to 0x5515A6, 0x5515A7 or 0x5515A8. The earlier controller, index-12/3/11/14/4 work and their reached latches have already happened.

With composite false, the owner snapshots index 0 as an effective byte. Current index 13 nonzero combined with old retained byte 0x5515A6 nonzero forces this effective byte to zero. It then stores current index 0 to 0x5515A6 before the subsequent action-helper calls. This retained byte is the prior reached raw index-0 level, not the filtered effective byte.

Current index 1 nonzero calls 0x60720 on the current actor with current index 2 as a byte argument. A nonzero returned low byte admits the selected target-identifier store at actor+0x1A8. Otherwise, with index 1 zero, the current actor-state switch can call 0x60460 using effective index 0 and current index 2, or perform its state-specific scalar/transition work. The selected resulting numeric transitions include 0x19 to 0x18, 0x1A to 0x38, 0x1C to 0x1B, 0x24 to zero, 0x25 to 0x26, 0x3C to 0x3D/0x3E depending on byte +0x14D, 0x40 to 0x0C, 0x41 to 0x11 with +0x6C from 0x4EAD0, and 0x93 to 0x94. Other charge/action scalar routes remain actor/helper-dependent; this list is not presented as the switch's universal outcome. The called action bodies are explicit transitive limits.

A zero current index 2 independently clears selected states: 0x50 when the selected vector length is positive, 0x51, 0x5F when its timing comparison admits, 0x53 and 0x92. Further class/specification branches require current indices 0 and 1 both zero. They read actor bytes +0x14C/+0x14D, actor state/scalars/flags and original 0x65690/0x507B0/0x4F220 results:

• Pairs 4/0 or 2/0 with held index 2 can select state 0x50 under their helper, timing, mask-0x1 and state-exclusion gates.
• Pairs 4/1 or 2/1 use held index 2 and retained byte 0x5515A7. A rise stores timeGetTime's four-byte return at global 0x5515AC. Selected helper/timing/state conditions can select state 0x51. The 2/1 route additionally excludes state 0x78.
• Byte +0x14C equal to 1 with held index 2 can select state 0x5F, clear +0x6C and use the +0x9A0 threshold 0.25.
• Byte +0x14C equal to 3 makes the selected timeGetTime call even before testing held index 2. On a rise it calls timeGetTime again and stores that second four-byte result at 0x5515B0. Its selected +0x6C/+0x12C/state guards can select state 0x53, bound +0x3C below by the original negative-0.2 operand, and clear +0x4C/+0x50/+0x54.
• Within the same zero-index-0/1 section, index 10 uses independent retained byte 0x5515A8. Its rise additionally requires the selected helper admissions and squared vector length below the original 0.01 operand. Actor byte/class/specification routes select numeric states 0x33, 0x37, 0x16, 0xA0 or 0x34, with branch-specific scalar/vector work. They are not unconditional meanings of index 10.

At the reached common postlude the owner stores current index 2 to 0x5515A7 and current index 10 to 0x5515A8, even when the inner zero-index-0/1 requirement prevented those class branches. The earlier composite return skips both stores. Final release conditions separately read zero index 1 or zero index 0 in conjunction with selected numeric action identifiers and states 0x1D/0x1E. Cleanup does not clear the sampled array.

Complete selected late-record consumer branch in 0xB40D0

The ordinary direct caller at 0xAA1BC passes G and the saved binary64 elapsed operand. Its selected G+0x418 object Z has a record range at Z+0x1E8 through +0x1F0, with record stride 0x1B0. The complete native owner was byte-matched; earlier remote/record/reward branches and transitive helper payloads are bounded separately from this selected sampled-byte branch.

When this selected record range is empty, the owner clears Z's selected root, four-byte Z+0x204/+0x200/+0x208 and eight-byte +0x210. That branch does not explicitly clear retained bytes 0x5515B5 or 0x5515B6. For a nonempty range, other selected record work can occur, and the owner advances binary32 Z+0x200 by widening old value, adding binary64 elapsed, then narrowing. It selects Z's root.

The numeric helper 0x290430(Z) supplies a signed integer; the owner adds 100, converts the threshold to binary32 and compares the current Z+0x200. While the latter is at most the threshold, it explicitly clears both retained bytes. Above that threshold it tests a falling index 0 against old 0x5515B5, then a falling index 11 against old 0x5515B6 if needed. A fall with the current collection nonempty and selected Z+0x204 below its count advances that four-byte index, clears +0x200/+0x208, and calls 0xA0420 with numeric identifier 0x7A at the final count, otherwise 0x7B. It then rereads index 0 and index 11 and stores their bytes to 0x5515B5 and 0x5515B6, after the reached call. A selected exhausted index subsequently clears the record collection through 0x8ADF0, deselects the root and clears the selected scalar/index fields.

This branch therefore uses falling levels retained by this consumer, independently of both index-11 rising consumers. It has no selected direct Chat, keyboard-target, capture-row or full-composite guard. It is reached later than the local 0x8AE50 phase of the same Game update; that fact does not impose a universal interthread observation order.

Early and late Game menu/selection consumers

At 0xA6EB9 onward, index 21 nonzero with retained byte 0x5514F4 zero decrements the four-byte G+0x1E04. Native-width subtraction occurs before the signed lower-bound test. A negative result is changed to zero, after which a result at least the selected signed count is changed to count minus one. That count comes from the current G+0x10 object's +0x170/+0x178 range with stride 0x68. The byte is then reread and stored to 0x5514F4. Index 22 and 0x5514F5 follow, incrementing first and applying the same ordered lower/upper tests. With a zero count, the final upper correction can yield -1; this is not an unconditional nonnegative clamp. These two selected branches have no Chat, keyboard-target or full-composite admission.

Much later, after the reached packet drain at 0xAAEBF and selected event-record work, a single outer menu block admits only if current C+0x1D8 is zero and the selected integers of G+0x118, G+0x130 and G+0x170 are all zero. Within it, the exact rising-check/call/latch order is:

| Order | Index | Retained byte RVA | Reached operation on a rise |
| --- | --- | --- | --- |
| 1 | 15 | 0x5514FC | 0x1031E0(I) |
| 2 | 17 | 0x5514FD | 0x102FE0(I) |
| 3 | 18 | 0x5514FE | 0x102F10(I) |
| 4 | 16 | 0x5514FF | Query S and store its inverse selected Boolean |
| 5 | 20 | 0x551500 | 0x1030C0(I) |
| 6 | 19 | 0x551501 | 0x103150(I) |

Every retained byte store occurs after its own call and rereads the corresponding sampled byte. Between orders 2 and 3, current roots from the G+0x3C8..+0x3D0 widget-pointer range mirror the selected root of current G+0x330. Between orders 3 and 4, current G+0x348's root mirrors current G+0x340's root. These helper calls and receiver rereads sit inside the block; its outer Chat/page admission is not rerun after each callback. If the outer block is skipped, all six retained-byte updates are skipped too.

The complete selected callback bodies establish the following narrow effects. 0x1031E0 toggles byte I+0x550, hence G+0x668, between zero and one. 0x102FE0 toggles the I+0x218 widget root; when newly/currently open, it copies the current actor's four-byte +0x150/+0x154 operands into widget +0x1F0/+0x1F4, calls 0x280A40 and 0x280C80, and hides the I+0x240 widget root. 0x102F10 toggles I+0x228's root, always calls 0xB2180(G), then rereads that root before its open-only 0x280A40/0x280C80 calls. 0x103150 toggles I+0x240's root and, when open, hides I+0x218's root. 0x1030C0 calls 0xB33F0(G) first and only then toggles the I+0x270 root. These helper payloads remain transitive boundaries. No page name or gameplay operation is inferred merely from a button token.

After this block, indices 5, 7, 8 and 6 are tested in the selected digital interface-clearing condition, combined with Chat byte zero; a separate 0x2B630 result on G+0x1D6C provides an alternative numeric admission only when strictly above binary32 word 0x3C23D70B (0.010000000707805157). The containing gate admits if G+0x668 is zero or G+0x1958 is zero. Success clears G+0x668, selected widget roots reached in that block and G+0x420. Those clears can reverse menu state established earlier in the same update. This contract does not equate the numeric helper result with an OS pointer event.

S has further selected clearing and publication. Current index 5 or 6 nonzero, or finite G+0x1D70 strictly outside -0.25 through +0.25, clears S; exact equality is retained by this comparison. Interface update 0x103200 follows at 0xAB5CE. At its selected S-publication point that Interface update forces S clear when G+0x668 is nonzero. S selected otherwise uses index 7 or G+0x1D6C strictly below word 0xBE4CCCCD for left publication, then index 8 or G+0x1D6C strictly above word 0x3E4CCCCD for right publication.

The later S-selection repeat section has its own process globals, independent of each sampled byte's level. If S is not selected, it clears armed byte 0x551554 and G+0x1E8C/+0x1E90, but does not explicitly reset its two counters in that branch. S selected with an empty available-pair list publishes its empty-selection text and clears those Game scalar fields; that branch likewise does not explicitly clear the armed byte or counters. S selected with a nonempty list first normalizes the signed G+0x20C0 selection by the current count and publishes that selection's text before repeat adjustments.

The left condition uses current index 7 nonzero or G+0x1D6C strictly below zero; the right condition uses index 8 or G+0x1D6C strictly above zero. These two repeat comparisons use zero, unlike the earlier directional publication's approximately 0.2 thresholds. A false left condition sets signed four-byte counter 0x551558 to zero; false right sets 0x55155C to zero. With neither condition held, the owner sets armed byte 0x551554 to one and reaches timer decrement. With either held but the armed byte zero, it skips both repeat work and the timer-decrement join. With armed byte nonzero, an admitted left counter below one decrements G+0x20C0, resets that counter to 200 and calls 0xA0420 with 0x58. It then independently checks the right side, increments G+0x20C0, resets its counter to 200 and can call 0xA0420 with 0x58 again. Both sides can execute in one visit. The counters are then reduced by binary64 elapsed, truncated to signed 32 bits and clamped below at zero. The repeat operation does not itself disarm the byte. A held condition across frames is not represented solely by a rising sampled-byte latch.

Interface, Controls, Help and wheel consumers

The complete selected entry of Interface update 0x103200 calls the Q capture-row getter. If false, it publishes the composite test's Boolean into the selected integer of direct node I+0x330 and performs its selected current-point/layout call. If true, it instead stores that node's selected integer as zero. This is a local publication branch, not a return from the entire Interface update. A subsequent independent composite query publishes the inverse Boolean at direct node I+0x560. Imported positioning/geometry and unrelated later Interface work are not closed here.

Later in the same owner, the selected three-element Interface publication reads indices 4, 11 and 3 for elements zero, one and two respectively. The corresponding named-frame descendant publication repeats the same index order. It queries the composite afresh before each selected result and admits a sampled level only when that query is false. Therefore a composite change between calls can change later publication within the same invocation; the first result is not a cached permission for all elements. These are levels for UI publication, not acknowledgments that clear gameplay latches. Separate current-actor, GUI-resource and unrelated text/layout branches in this large owner remain outside the semantic closure.

The Q capture-row helpers confirm the separation of row capture, root selection and focus. Complete 0x276F30 copies the four-byte hovered row Q+0x2DC into Q+0x2E0. Complete 0x2761F0 stores -1 at Q+0x2E0. Complete 0x2769A0 admits only unsigned Q+0x2E0 below 23; it scans the 23 eight-byte local binding pairs beginning at Q+0x1F0, clears pairs matching the supplied two four-byte operands, writes the supplied pair at the selected row and sets Q+0x2DC to -1. It does not itself clear Q+0x2E0.

Q draw 0x276200 resets hovered row Q+0x2DC to -1 on each call, can set capture row Q+0x2E0 to -1 when G+0x1958 is nonzero, and bounds its selected visible-row work to 0..23. Its active capture row publishes the original prompt “Press a key”; other rows have different hover/formatting admission. It does not update the sampled array. The selected primary virtual table's +0xC0/+0xD0/+0xE0 targets are not 0x2761F0; a generic assertion that a GUI key callback clears capture would be unfounded. Resolved additional helper callers exist at 0x8A120, 0x8A4F0 and Game release 0x965E0; their complete mouse/capture lifecycle is not imported into this sampled-consumer contract.

H draw 0x278040 reads index 2 once through its current H+0x1E8 Game receiver after the selected drawing/style preparation, caches that byte, and supplies the same cached low byte to numeric helpers 0x5FC40 and later 0x60120 on actors loaded at their respective calls. The complete selected 0x5FC40 body shows that a nonzero supplied modifier, or an actor buff record with numeric type 0x0D, admits a class/specification-dependent numeric route. Examples of that route are 0x87 or 0x61 for byte +0x14C equal to 3 depending on +0x14D, and 0x6C for pair 4/1. Other routes depend on actor state, timing, equipment fields and helpers; no universal one-to-one interpretation of index 2 follows. The resulting numeric codes affect the draw owner's selected text/presentation work. General GUI traversal/visibility admission, imported drawing/text providers and 0x60120's deeper closure are not established here. This is an actual draw-body use, separate from message dispatch and Game's controller call.

Complete wheel owner 0x97CC0 reads index 13 only after its selected root/hit admissions for current widgets at G+0x330, +0x350, +0x340, +0x658 and +0x388 have failed in order. A nonzero selected root alone is not sufficient for those earlier routes: the selected point query must also admit. In the final admitted route, index 13 zero changes G+0x1E04 by signed wheel subtraction and clamps to 0..6, with further selected current-World/map coordinate and 0x35EA0 height preparation when that value changed. Index 13 nonzero instead subtracts twice the signed wheel input from binary32 G+0x1DB8 and clamps the ordinary finite result to 0..50. Those are two native branches, not a general meaning for index 13. Other widget scroll calls, point-query geometry, terrain and imported input normalization remain explicit boundaries.

Retained-state scope, initial image and refresh limits

All retained operands listed below are process globals in the original image, not fields of G or E. The selected owners do not share one “previous input” snapshot.

| Global RVA(s) | Native width | Reached role / updating owner |
| --- | --- | --- |
| 0x5514F4, 0x5514F5 | One byte each | Previous reached index 21 and 22; early 0xA5A50 section |
| 0x5514FC, 0x5514FD, 0x5514FE, 0x5514FF, 0x551500, 0x551501 | One byte each | Previous reached indices 15, 17, 18, 16, 20, 19 in late 0xA5A50 menu section |
| 0x551554 | One byte | S-repeat armed operand in 0xA5A50 |
| 0x551558, 0x55155C | Signed four bytes each | S-repeat counters in 0xA5A50 |
| 0x551594 | One byte | Previous reached index 9 at final 0x9C050 postlude |
| 0x551598, 0x55159C | Signed four bytes each | Index-9 carried count and secondary delay in 0x9C050 |
| 0x5515A1 through 0x5515A8 | One byte each | Previous reached indices 12, 3, 11, 14, 4, 0, 2, 10 respectively; 0x8AE50 |
| 0x5515AC, 0x5515B0 | Four bytes each | Selected timeGetTime returns stored by 0x8AE50; no direct read established by the exact-address census |
| 0x5515B4 | One byte | Entry-cached previous index 11 in 0x98180 |
| 0x5515B5, 0x5515B6 | One byte each | Selected falling-level state for indices 0 and 11 in 0xB40D0 |

The fresh original section table places these globals in .data. That section begins at RVA 0x530000, has raw-backed size 0x1DE00 and virtual size 0x23C40. The selected globals are after raw-backed end 0x54DE00 and before virtual end 0x553C40. Their image-initial storage is therefore in the PE zero-fill tail, rather than containing nonzero raw-file initializer words. That is an image initialization fact, not a read of the running process or proof of their values after all startup work.

For each listed exact address, the exported explicit-address search resolved the selected references to the owner shown in the table. No separate constructor reset of these globals is proved by the checked constructor/setup material. Computed aliases, overlapping wider writes, bulk operations, other threads and unrepresented indirect paths remain unproved. This bounded census cannot make a universal “only writer” claim.

The direct consumer visits do not clear the 23 sampled bytes. Their private retained stores occur only along their own reached paths. Skipping a menu block can retain an old menu latch while a controller latch changes; returning early in 0x9C050 can retain 0x551594 and timers while 0x98180 subsequently updates its independent entry latch if that later owner is reached. A sampled level becoming zero in a skipped frame is not inherently remembered as a release by every consumer.

Current object rereads and callbacks

These are ordered native accesses, not whole-frame immutable receiver or state snapshots. 0x9C050 repeatedly loads G+0x8 and W+0x448 around calls and store groups. The selected action and menu sections likewise use current node/root pointers and reread sampled bytes after reached helper calls. H draw instead caches its one index-2 byte across two numeric helper calls while reloading actor pointers for those calls. 0x98180 deliberately caches its entry index-11 byte for its local edge/store, whereas its caller later stores a fresh index-11 read.

Window GUI dispatch is a separate phase. The complete selected key and character owners choose current M+0x170 and its active byte before virtual dispatch. The selected pointer owners can call virtual/event callbacks and subsequently use current retained/focus operands. Windows SetCapture/ReleaseCapture boundaries in the window procedure do not establish equality with M+0x168 or M+0x170, and no automatic clear of the raw window array or the sampled array is established merely by GUI focus in these selected owners.

No selected sampled-byte read is made atomic with all 22 other reads by these consumers. The source does not establish a cross-thread transaction, a guaranteed lack of reentrancy inside imported/indirect calls, or a scheduling interval between producer refresh, controller work, menu callbacks and draw. The statements about “after” describe reached instruction/call order in one invocation under normal return, with pointed-to objects alive as required by the original operations. They are not universal wall-clock or concurrent lifetime guarantees.

Independent conditional source examples

These examples choose source operands and helper results, not captured original execution. Every example assumes its stated native path is reached, relevant pointers/indices are valid, imported calls return normally and unmentioned guards permit the described work.

• A setter call with index 23 or unsigned index 0xFFFFFFFF stores nothing; index 5 with value byte 0x80 stores 0x80. The selected downstream zero/nonzero direction tests admit the latter just as any other nonzero level. That does not create a shared rising event.

• Suppose Q+0x2E0 is 23. The getter returns one and can select the ordinary loop's capture branch, skipping sampled refresh, although 0x2769A0's unsigned-below-23 insertion gate rejects that row. A root being visible or invisible does not alter either predicate by itself.

• Suppose a skipped-refresh path leaves index 5 nonzero, Q's root is selected, M+0x170 is zero, C+0x1D8 is zero, S is unselected and the map/process conjunction is false. If the outer three-page and actor guards admit the controller call, 0x9C050 can reach its index-5 level branch. Q's selected root makes the full composite true and prevents its index-10 candidate; it does not return from this controller's later direction section. A later true-composite return in 0x8AE50 can still skip that owner's final action/latch section.

• If M+0x170 is nonnull, 0x9C050 can first perform its predicate-dependent actor preparation and then return at the keyboard-target test. Its final 0x551594 update and timer decrement are skipped. The GUI target's byte +0x1A0 need not be nonzero for this controller return, although that byte separately controls GUI virtual key/character dispatch.

• If index 11 changes from zero to nonzero while index 10 is nonzero, 0x98180 can cache the index-11 rise, store 0x5515B4, then return low-byte zero at its index-10 gate. The caller's index-11 latch 0x5515A3 is independent. Its later selected route and eventual fresh-byte store depend on its own gates. The falling latch in 0xB40D0 is a third distinct observation.

• With outer late-menu admission true, zero retained bytes and indices 15 and 17 nonzero, the update calls 0x1031E0 first, stores the fresh index-15 byte, then calls 0x102FE0 and stores fresh index 17. A helper changing a later sampled byte can affect a later test. A newly set G+0x668 does not rerun the enclosing three-page/Chat gate, and later digital clearing can clear it again within the same update.

• If both repeat directions are held, S is selected, its list is nonempty, armed byte is one and both counters are zero, the selected repeat section decrements then increments the selection and makes two ordered 0x58 calls. With armed byte zero, the same held values do not admit either repeat and do not enter that timer-decrement join. A later neutral visit is what sets the armed byte.

• With a nonempty Z record list and its elapsed scalar above the helper-derived threshold, a previously retained nonzero index 0 followed by current zero admits the selected falling branch, subject to current record-index bounds. If Z's list was empty during an intervening zero level, that empty branch alone does not prove the retained byte was reset.

• H draw can cache a nonzero index-2 byte, use it for 0x5FC40, and later use that same cached byte for 0x60120 even if the underlying sampled byte changes between calls. This does not predict either helper's complete numeric result or establish that the draw call was admitted by general GUI traversal in any particular frame.

Evidence manifest and raw-image correspondence

Reference filenames were identified before scoped content work. The selected original export sources are decompiled/functions.csv, function_ranges.csv, calls.csv, assembly.txt and symbols.csv under the original reference root. Individually selected C presentations are decompiled/functions/140<RVA>.c for the owner entries in the table below, with leading zeros to six RVA digits. The main C presentation for 0xA5A50 reports timeout, so its successful original retry presentation at decompiled/retry-180-phase1/functions/1400a5a50.c was used for selected structure and joins. Native operands and raw bytes, rather than C return types, reused local names or character literals, resolve the stated widths and values.

All 56 tabled owners' exported instructions were matched against the original executable using its PE section mapping. Within each separately listed inclusive exported range, instruction encodings concatenate continuously from the first byte to the last. The comparison covered 32,383 instructions and 154,413 bytes, with zero encoding mismatches and zero internal-range coverage gaps. Separately listed ranges are not joined by a claim about intervening bytes or neighboring routines. The count includes selected discovery helpers and large owners for correspondence; it does not enlarge the semantic scope of their prose account.

| Owner RVA | Inclusive exported range(s), RVAs | Instructions matched | Bytes matched |
| --- | --- | --- | --- |
| 0xA7A0 | 0xA7A0–0xA945 | 106 | 422 |
| 0x13050 | 0x13050–0x13156 | 65 | 263 |
| 0x5EE70 | 0x5EE70–0x5EE96 | 13 | 39 |
| 0x5FC40 | 0x5FC40–0x600F2 | 358 | 1,203 |
| 0x7EA80 | 0x7EA80–0x7F35D; 0x7F35F–0x7F36D; 0x7F36F–0x7F37D; 0x7F37F–0x7F38A; 0x7F38C–0x7F9DF | 806 | 3,932 |
| 0x8AE50 | 0x8AE50–0x8B61D; 0x8B620–0x8B635; 0x8B640–0x8B650; 0x8B660–0x8FB06 | 3,980 | 19,612 |
| 0x96E70 | 0x96E70–0x977F2 | 550 | 2,435 |
| 0x97870 | 0x97870–0x9787B | 3 | 12 |
| 0x97CC0 | 0x97CC0–0x9803D | 190 | 894 |
| 0x98180 | 0x98180–0x9AD82 | 2,492 | 11,267 |
| 0x9AE50 | 0x9AE50–0x9AE62 | 5 | 19 |
| 0x9AE70 | 0x9AE70–0x9AE7B | 2 | 12 |
| 0x9AE80 | 0x9AE80–0x9AE8A | 4 | 11 |
| 0x9AE90 | 0x9AE90–0x9AEA4 | 10 | 21 |
| 0x9AEB0 | 0x9AEB0–0x9AEF1 | 21 | 66 |
| 0x9AF00 | 0x9AF00–0x9AF19 | 6 | 26 |
| 0x9AFA0 | 0x9AFA0–0x9AFAA | 3 | 11 |
| 0x9C050 | 0x9C050–0x9D6EA | 1,168 | 5,787 |
| 0xA35B0 | 0xA35B0–0xA36B9 | 82 | 266 |
| 0xA36C0 | 0xA36C0–0xA36D0 | 5 | 17 |
| 0xA36E0 | 0xA36E0–0xA37CE | 70 | 239 |
| 0xA37D0 | 0xA37D0–0xA389E | 63 | 207 |
| 0xA38A0 | 0xA38A0–0xA38B7 | 7 | 24 |
| 0xA38C0 | 0xA38C0–0xA38F5 | 12 | 54 |
| 0xA3900 | 0xA3900–0xA3935 | 12 | 54 |
| 0xA3A20 | 0xA3A20–0xA3A3F | 8 | 32 |
| 0xA5A50 | 0xA5A50–0xA74D7; 0xA7E6C–0xAA9F9; 0xAAC40–0xABDE1; 0xABE34–0xABEC9; 0xABF82–0xADBD7 | 6,106 | 29,860 |
| 0xB40D0 | 0xB40D0–0xB4A83; 0xB4A85–0xB4A8A; 0xB4A8C–0xB4A91; 0xB4A93–0xB4A98; 0xB4A9A–0xB6236 | 1,884 | 8,547 |
| 0xE8A20 | 0xE8A20–0xE8A22 | 1 | 3 |
| 0xF8690 | 0xF8690–0xF87C9 | 67 | 314 |
| 0xF9AC0 | 0xF9AC0–0x10089D | 5,496 | 28,126 |
| 0x1014F0 | 0x1014F0–0x10178B | 119 | 668 |
| 0x102F10 | 0x102F10–0x102F9C | 30 | 141 |
| 0x102FE0 | 0x102FE0–0x1030B5 | 40 | 214 |
| 0x1030C0 | 0x1030C0–0x10310C | 18 | 77 |
| 0x103150 | 0x103150–0x1031D6 | 25 | 135 |
| 0x1031E0 | 0x1031E0–0x1031F0 | 4 | 17 |
| 0x103200 | 0x103200–0x106C18 | 3,103 | 14,873 |
| 0x133C70 | 0x133C70–0x133EEB | 174 | 636 |
| 0x134420 | 0x134420–0x1347C5 | 192 | 934 |
| 0x1355A0 | 0x1355A0–0x137E1B | 2,217 | 10,364 |
| 0x26A2E0 | 0x26A2E0–0x26A466 | 91 | 391 |
| 0x275A70 | 0x275A70–0x2761AC | 425 | 1,853 |
| 0x2761F0 | 0x2761F0–0x2761FA | 2 | 11 |
| 0x276200 | 0x276200–0x276993 | 429 | 1,940 |
| 0x2769A0 | 0x2769A0–0x2769FA | 23 | 91 |
| 0x276F30 | 0x276F30–0x276F3C | 3 | 13 |
| 0x277EC0 | 0x277EC0–0x277FF6 | 74 | 311 |
| 0x278040 | 0x278040–0x279A72 | 1,494 | 6,707 |
| 0x33F1E0 | 0x33F1E0–0x33F202 | 8 | 35 |
| 0x340DD0 | 0x340DD0–0x340DEF | 8 | 32 |
| 0x340DF0 | 0x340DF0–0x340E56 | 26 | 103 |
| 0x340EC0 | 0x340EC0–0x3410D8 | 138 | 537 |
| 0x3410E0 | 0x3410E0–0x3412F4 | 141 | 533 |
| 0x342090 | 0x342090–0x342097 | 2 | 8 |
| 0x3420A0 | 0x3420A0–0x3420AD | 2 | 14 |

Complete-body and selected-scope distinctions

The whole 0x9C050 controller, the whole composite/admission accessors and the small setters/capture helpers were read through all returns. The whole selected menu callback bodies, constructor receiver joins, GUI key/character/pointer adapters, wheel owner and frame helper were read to resolve the stated effects. The complete 0x5FC40 and H draw bodies were inspected to establish their current-byte/provider call relation; the handoff does not claim to reproduce all their actor-dependent result or drawing closures.

Large owners 0x7EA80, 0xF9AC0, 0x1355A0, 0x96E70, 0xA5A50, 0x8AE50, 0x98180, 0xB40D0 and 0x103200 have complete exported-range byte correspondence, but their semantic coverage is expressly limited to the constructor/publication, sampling/frame, raw-key/Chat, sampled-consumer, entry-latch and admission branches described above. Their unrelated world generation, camera, networking, rendering, inventory/reward, spatial or general interface branches are not a universal behavioral census. The original main 0xA5A50 export's discontiguous owner ranges and timeout are both retained in the manifest rather than silently assigning intervening neighbor bytes to it.

Nearby exports 0x9AE80, 0x9AE90, 0x9AEB0, 0x9AFA0, 0xA35B0, 0xA36E0, 0xA37D0, 0xA38A0 and 0x3420A0 were individually inspected to disambiguate apparent accessor/address leads; their complete native bytes are included in the table. No unproved Game receiver alias is assigned to them. In particular 0x3420A0 is not used as a capture setter merely because its address follows 0x342090.

The selected resolved direct-call census finds the one 0x9C050 call at 0x8BDBB and the one 0x8AE50 call at 0xA9B2C. It also finds other callers of the composite and focus accessor outside the selected owners, including original mouse-motion, character, other actor/update and Edit/GUI bodies. These are discoverable alternate callers, not evidence that their complete behavior or all computed callers is covered. The additional capture-row helper callers identified above are likewise not expanded into a full binding-editor producer lifecycle.

Selected raw data and constant correspondence

The selected primary table/RTTI joins are Q table 0x46E468, locator 0x4B02D8 and type descriptor 0x54B5F0; C table 0x46D1E8, locator 0x4AFE78 and type descriptor 0x54B4A0; H table 0x46E8A8, locator 0x4B04E8 and type descriptor 0x54B6A8. The H draw pointer is the eight-byte word at 0x46E8B0. These are original image receiver-identity facts, not a reconstruction class layout. The selected Q table separates its draw slot from its key/character targets at 0xE8A20. That complete three-byte owner is a return-only body, independently byte-matched in the manifest.

The following constants were read from their original raw-image addresses at the width used by the selected instructions. Decimal shorthand in this contract means the indicated native binary value, not an arbitrary-precision substitute.

| Constant RVA | Selected width and raw value | Reached value / use |
| --- | --- | --- |
| 0x42E514 | Four bytes 0x3A83126F | 0.0010000000474974513; controller predicate/blend input |
| 0x42E51C | Four bytes 0x3C23D70A | 0.009999999776482582; selected squared-sum comparison |
| 0x42E520 | Four bytes 0x3C8EFA35 | 0.01745329238474369; matrix-helper angle factor |
| 0x42E524 | Four bytes 0x3D4CCCCD | 0.05000000074505806; selected controller term |
| 0x42E540 | Eight bytes 0x3F947AE147AE147B | Binary64 0.02; iterative helper |
| 0x42E584 | Four bytes 0x40A00000 | Binary32 5 |
| 0x42E598 | Four bytes 0x41200000 | Binary32 10 |
| 0x42E59C | Four bytes 0x41400000 | Binary32 12 |
| 0x42E5A4 | Four bytes 0x41C80000 | Binary32 25 |
| 0x42E5AC | Four bytes 0x42200000 | Binary32 40 |
| 0x42E5B8 | Four bytes 0x42A00000 | Binary32 80 |
| 0x42E5D8 | Four bytes 0x44FA0000 | Binary32 2000 |
| 0x42E5E8 | Four bytes 0x461C4000 | Binary32 10000 |
| 0x42E6C8 | Four bytes 0x3AC49BA6 | 0.001500000013038516; not an eight-byte operand |
| 0x42E6D0 | Four bytes 0x3B03126F | 0.0020000000949949026 |
| 0x42E700 | Four bytes 0x3E4CCCCD | 0.20000000298023224 |
| 0x42E7A4 | Four bytes 0x41700000 | Binary32 15 |
| 0x42E7D4 | Four bytes 0x43340000 | Binary32 180 |
| 0x42E800 | Four bytes 0xBE4CCCCD | -0.20000000298023224 |
| 0x42E8E8 | Eight bytes 0x4010000000000000 | Binary64 4; not the low four-byte zero |
| 0x43294C | Four bytes 0x44C80000 | Binary32 1600 |
| 0x437F74 | Four bytes 0x3C23D70B | 0.010000000707805157 |
| 0x437F7C | Four bytes 0x3D23D70B | 0.04000000283122063; selected two-float setter threshold |
| 0x437F90 | Eight bytes 0x3F50624DE0000000 | Binary64 0.0010000000474974513 |
| 0x437FB8 | Eight bytes 0x3FB47AE140000000 | Binary64 0.07999999821186066 |
| 0x437FD8 | Eight bytes 0x408F400000000000 | Binary64 1000 |
| 0x437FE8 | Four bytes 0x42100000 | Binary32 36 |
| 0x437FF4 | Four bytes 0x43610000 | Binary32 225 |
| 0x4AC95C | Four bytes 0x3E800000 | Binary32 0.25 |
| 0x4AC964 | Four bytes 0x3F000000 | Binary32 0.5 |
| 0x4AC980 | Four bytes 0x3F800000 | Binary32 1 |
| 0x4AC9B8 | Eight bytes 0x3FF0000000000000 | Binary64 1; iterative helper |
| 0x4ACA18 | Eight bytes 0x4059000000000000 | Binary64 100; iterative helper |
| 0x4ACB30 | Four bytes 0x80000000 | Sign-bit toggle mask |

The matrix source span 0x42E630–0x42E66F contains 16 binary32 words, with ones in its four diagonal positions and zeros elsewhere. Actor scalar stores described as 20 and 600 correspond to original words 0x41A00000 and 0x44160000. The selected digital/analog setters 0xA38C0 and 0xA3900 write their two supplied binary32 operands first, then clear the corresponding eight-byte pair when their squared sum is strictly below word 0x3D23D70B. They are upstream scalar writers, not sampled-byte getters; their provider-input production is bounded to the selected ordinary caller joins.

Floating-point, provider and negative limits

The stated inequality paraphrases concern ordinary finite operands unless a native unordered branch is expressly identified. P's two COMISS/carry branches reject an unordered comparison if execution continues. The controller's low-resource strict comparison does not take its early return for an unordered E+0x9A0 comparison. Its later analog squared-sum test has an explicit parity branch into the work, as stated above. The composite and sampled-byte tests are integer/pointer operations and do not acquire floating tolerance from these scalar paths.

The selected instructions preserve binary32 versus binary64 widths, widening, narrowing and truncating integer conversions. Literal-zero products and separately ordered stores cannot be replaced in this contract by a promise of ideal vector algebra. NaNs, infinities, signed zero, denormals, out-of-range conversions, integer wrap and floating exceptions are not given an invented universal gameplay result. At the early G+0x1E04 adjustment, native 32-bit arithmetic precedes the sign-based lower correction and signed upper comparison; an abstract C overflow assumption is not used.

This contract establishes neither current physical-device levels at a chosen instant nor a deterministic imported clock, font, graphics, audio, input, runtime-math or OS scheduling result. It does not derive undocumented provider enumeration meanings. It does not prove every callback target leaves G/W/E/nodes unchanged, every indirect call's reentrancy, actor lookup lifetime, general GUI capture/geometry, source-thread mutual exclusion, all construction order outside the selected joins, or a complete writer/caller census over computed aliases. No downstream creature/control helper semantics are imported from a name. Its closed facts are the stated native sampled-byte reads, widths, branch/call order, selected root/focus/capture distinctions, retained-state updates and complete selected controller behavior within those explicit dependencies.

Saved-artifact completion

This file is the new independent contract at the task's exclusive output path. Its final saved bytes were read back in full, compared with the complete authored text, and checked for UTF-8 encoding, exact length and SHA256 before the hold was reported. The original executable identity and complete selected instruction-byte correspondence were reverified for this assignment. The reported SHA256 is an external digest of this complete file, avoiding a self-referential digest claim inside its bytes. After FINAL / STABLE / HELD, no further writes to this artifact are authorized by this assignment.
