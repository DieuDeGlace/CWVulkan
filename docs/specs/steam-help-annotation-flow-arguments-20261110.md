# Help annotation flow arguments in the supported Steam image

This is an independent, bounded original-source contract for the annotation calls made by the complete Help draw owner at RVA 0x278040 to the text-flow helper at RVA 0x269210. It records the supplied coordinate arguments, the predicates selecting each literal annotation, and the sole annotation continuation that uses an earlier returned flow component. It does not establish final screen coordinates, font measurements, gameplay costs, or the implementation of the action-description providers. All statements about execution order concern normal returning native calls unless a narrower qualification is stated.

The author used only the standalone source-author policy and individually selected original executable/export evidence. Earlier held contracts were neither read nor changed. No replacement implementation, exporter, tests, results, captures, owner configuration, assets, combined project history or memory, SDK, provider implementation, dependency implementation, or reconstruction source was read. No executable was run, and no implementation or runtime feedback was requested or received. This document contains factual prose and tables, not original code, instruction transcription, pseudocode, or replacement guidance.

## Supported original and correspondence method

The image is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. Its length is 5,757,952 bytes and its SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. The PE machine is AMD64, numeric 0x8664; preferred image base is 0x140000000; timestamp is 0x5D93AB0D; entry RVA is 0x3939F0; SizeOfImage is 0x58A000. Addresses below are RVAs in this image, not raw-file offsets.

Filename discovery preceded scoped original reads. The reference metadata are the original decompiled/assembly.txt, decompiled/function_ranges.csv, decompiled/calls.csv and decompiled/functions.csv. The eight selected C exports are in decompiled/functions. Their complete selected bodies were read. The assembly rows for each selected body were mapped through the original PE section table to raw image bytes. Every instruction row matched its original bytes, and each selected function range was continuously covered without gaps. Direct relative call targets were independently decoded from the image-matched operands and checked against the original assembly presentation. The relevant coordinate constants, UTF-16 literals, action switch data, and selected constructor virtual-table entries were read from the raw image. C presentation alone was insufficient for the hidden coordinate arguments and the returned-component conversion; those facts below follow the native operands.

| Complete selected owner | Inclusive original extent | Instructions | Code bytes | Role in this contract |
| --- | --- | ---: | ---: | --- |
| 0x278040 | 0x278040–0x279A72 | 1,494 | 6,707 | Complete Help draw owner and every text-flow call in it |
| 0x269210 | 0x269210–0x26958E | 158 | 895 | Complete supplied-coordinate and returned-flow boundary |
| 0x5E3A0 | 0x5E3A0–0x5E3C9 | 15 | 42 | Complete byte-tag lookup used by three annotation admissions |
| 0x277EC0 | 0x277EC0–0x277FF6 | 74 | 311 | Complete selected Help construction and owner/table stores |
| 0x2691B0 | 0x2691B0–0x2691B8 | 2 | 9 | First selected style scalar store |
| 0x2691F0 | 0x2691F0–0x2691F8 | 2 | 9 | Second selected style scalar store |
| 0x269160 | 0x269160–0x269183 | 9 | 36 | Current primary four-component state copy |
| 0x2691C0 | 0x2691C0–0x2691E3 | 9 | 36 | Current secondary four-component state copy |

These are eight complete owners, eight continuous ranges, 1,763 instructions and 8,045 code bytes, with no byte mismatch or coverage gap. The census of direct relative calls across these owners contains 264 call sites. It includes all 40 calls to 0x269210 in 0x278040; exactly 15 of those sites construct and supply the bracketed annotation literals listed below. Matching a call operand does not close the callee's internals unless that callee is included in the complete-owner table.

## Receivers, widths and normal draw preparation

H denotes the selected Help receiver. G denotes the eight-byte pointer currently loaded from H+0x1E8; W denotes the eight-byte pointer currently loaded from G+8; E denotes the eight-byte pointer currently loaded from W+0x448. These are operand aliases for the selected path. They do not imply that all later loads return the same objects. In the complete constructor at 0x277EC0, the second constructor argument is retained at H+0x1E8, the supplied node and the pointer at that argument's +0x20 are passed to the base construction boundary 0x268A70, and the primary and secondary tables installed at H and H+0x38 are 0x46E8A8 and 0x46EA08. The eight-byte entry at 0x46E8B0, primary-table offset +8, resolves to 0x278040 in the supported image. The constructor requests the 13-code-unit UTF-16 resource name resource1.dat through 0x269130. That request proves neither resource availability nor a particular font implementation.

The complete draw owner first prepares a local matrix from its node's parent, or an original default matrix when there is no parent, then conditionally composes the node's current spatial matrix. The selected calls are 0x100FA0 and 0x173E0. This local work is not an extra addition to the first or second coordinate arguments in the annotation table. The separate matrix and spatial reads actually used inside 0x269210 are described later; no screen origin is inferred from the local preparation.

Before the text rows, 0x278040 writes binary32 value 12 to H+0x1B0 through 0x2691B0 and binary32 value 3 to H+0x1B4 through 0x2691F0. It copies the four binary32 values 0, 0, 0, 1 into H+0x1D4 through H+0x1E0 through 0x2691C0. The selected primary-state setter 0x269160 copies four current four-byte components into H+0x1C4 through H+0x1D0. Calls to that setter occur between binding, description and annotation text. The source vectors differ by those roles; the annotation strings do not themselves supply an explicit color argument to 0x269210. Subsequent provider calls and current-state rereads remain relevant to the actual text helper.

Every coordinate in the annotation table is a four-byte IEEE binary32 input. The first coordinate occupies the low 32-bit lane of the fourth native argument; the second is the four-byte fifth argument in the caller's stack argument area. The output pair is eight bytes, two binary32 components in order, written to caller-supplied storage, and the helper returns the pointer to that storage. The native locations establish the argument order even where the C export omits a visible coordinate argument.

The fixed coordinate values are read from these original four-byte constants: 320 at 0x441BD8, 180 at 0x42E7D4, 20 at 0x42E5A0, 40 at 0x42E5AC, 60 at 0x42E7B8, and 80 at 0x42E5B8. Positive zero is also generated directly in the draw owner. All of these integers have exact binary32 representations. The style constants 12 and 3 come from 0x42E59C and 0x42E580. Their use establishes supplied values, not physical pixels or a scale-independent screen location.

## Action values and description strings before the annotations

At 0x27812C the owner samples one byte from current G+0x1A72, zero-extends it, and retains that value for both selected action queries in this draw. Call 0x278143 to 0x5FC40 receives the then-current E and this retained byte. Its low 32-bit return is retained as A. No interpretation of the action selector's internals is supplied here. The first binding string is requested by 0x4C830 with receiver current G+0x30 and binding-record address current G+0x1BDC. Text call 0x27819A supplies coordinates 0, 0 for that returned string. Its returned flow pair is not used to place the next description or annotation.

The numeric description for A is requested from 0x16C5E0 on current G+0x50, then passed to 0x16BE20 on a newly read current G+0x50 together with the UTF-16 category default and an empty extra string. Text call 0x27836C supplies coordinates 180, 0 for the resulting current temporary string. Normal string cleanup follows. There is no Help-side requirement that this description be nonempty or that the numeric lookup succeed before its later A-dependent annotation predicate is considered. The providers' actual output, missing-token behavior and allocation/error behavior are not closed by this audit.

After the A-dependent annotation, a separate binding row is formed from current G+0x1BE4 through 0x4C830. Captured A values 9, 25, 26, 28, 60 and 64 select the uppercase prefix HOLD followed by one space; its text call is 0x27853A at 180, 20. Other A values use the unprefixed binding at 0x2785C1 at 0, 20. Native membership is bounded by an unsigned 32-bit difference from 9 below 56 and the literal bit mask 0x880000000B0001. The branch alternatives join before the second action query. Neither binding row's returned pair supplies any annotation coordinate.

Call 0x278613 to 0x60120 receives the newly loaded current E and the same previously retained byte from G+0x1A72. The low 32-bit return is retained as B. The numeric description path again calls 0x16C5E0 and 0x16BE20 with current G+0x50, the category default and an empty extra string. Its text call 0x2786BB uses coordinates 180, 20. Description cleanup completes before B's annotation dispatch. A and B are two captured 32-bit return values, not two direct reads of one common entity field, and the selected E need not be unchanged across the intervening providers.

B's native dispatch admits values 6 through 108 by an unsigned 32-bit range test, then uses the original 103-byte remapping table at 0x279A88 and five four-byte destinations at 0x279A74. Destination 0x27878F handles B values 6, 18 and 21; 0x278973 handles 9, 25, 26, 28, 60 and 64; 0x278880 handles 50; 0x2787F5 handles 108. Every other admitted value maps to 0x2789DF, as do values outside the range after the native admission test. The annotation mappings below use those exact groups. This dispatch order is native control flow; listing branch blocks in ascending address order does not turn mutually exclusive branches into sequential calls.

At the three call sites 0x27843D, 0x27880C and 0x2788FB, the complete helper 0x5E3A0 receives a freshly loaded current E and the 32-bit query value 13, numeric 0xD. It loads the eight-byte list head at E+0x980 once, follows the first and subsequent eight-byte links until that captured head, and zero-extends each node's byte at +0x10 for the 32-bit comparison. The first match returns node+0x10; exhaustion returns a null pointer. The helper makes no stores and contains no lock. In each annotation admission below, null admits the annotation and nonnull skips it. The source proves a byte-tag presence test, without proving that the displayed cost literal causes or measures a gameplay resource change.

## Complete annotation call inventory

The following table enumerates all 15 bracketed-literal calls to 0x269210 in the complete owner. Coordinates are supplied in first-component, second-component order. Except for the one marked continuation, every coordinate pair is fixed at the call site. An earlier description can be empty, can return an arbitrary flow pair, or can take a resource-null text-helper return without changing a later fixed coordinate on a normally returning path. Each listed predicate is evaluated at the specific point described in the chronology below; it is not a promise of an immutable actor snapshot.

| Annotation text-call RVA | Exact literal | Admission on a normal returning path | First coordinate | Second coordinate | Earlier related text call and source |
| --- | --- | --- | ---: | ---: | --- |
| 0x278492 | [25 STAMINA] | Captured A equals 108, and the later tag-13 lookup at 0x27843D is null | 320 | 0 | 0x27836C: description for captured A, default category; returned pair ignored |
| 0x2787E0 | [ALL MP] | Captured B is 6, 18 or 21 | 320 | 20 | 0x2786BB: description for captured B, default category; returned pair ignored |
| 0x27886B | [25 STAMINA] | Captured B equals 108, and the later tag-13 lookup at 0x27880C is null | 320 | 20 | 0x2786BB: description for captured B, default category; returned pair ignored |
| 0x2788D1 | [ALL MP] | Captured B equals 50, numeric 0x32 | 320 | 20 | 0x2786BB: description for captured B, default category; its returned pair is ignored; this annotation's own first returned component is retained for the next row of this table |
| 0x278961 | [25 STAMINA] | Captured B equals 50 and the new tag-13 lookup at 0x2788FB, after 0x2788D1 and its temporary cleanup, is null | Returned-flow continuation specified below | 20 | 0x2788D1: literal [ALL MP]; its first returned binary32 component alone supplies the continuation |
| 0x2789BF | [CHARGE] | Captured B is 9, 25, 26, 28, 60 or 64 | 320 | 20 | 0x2786BB: description for captured B, default category; returned pair ignored |
| 0x278C28 | [STAMINA] | Row-40 captured byte C is 4, and that branch's byte S is 1 | 320 | 40 | 0x278BA8: numeric description identifier 81, numeric 0x51, default category; returned pair ignored |
| 0x278EB3 | [STAMINA] | Row-40 captured byte C is 2, and that branch's byte S is 1 | 320 | 40 | 0x278E4D: numeric description identifier 81, numeric 0x51, default category; returned pair ignored |
| 0x279087 | [STAMINA] | Row-40 captured byte C is 1 | 320 | 40 | 0x279007: numeric description identifier 95, numeric 0x5F, default category; returned pair ignored |
| 0x2791D8 | [25 STAMINA] | Unconditional after the ordinary completed Dodge row; no tag-13 lookup here | 320 | 60 | 0x279179: the five-code-unit literal Dodge; returned pair ignored |
| 0x2793AE | [100 STAMINA] | Row-80 captured byte C is 4, and its then-read same-entity byte S is 0 | 320 | 80 | 0x27934A: numeric description identifier 160, numeric 0xA0, default category; returned pair ignored |
| 0x279539 | [100 MANA] | Row-80 captured byte C is 3 | 320 | 80 | 0x2794D5: numeric description identifier 51, numeric 0x33, default category; returned pair ignored |
| 0x2796E3 | [50 STAMINA] | Row-80 captured byte C is 2, and the later current-entity byte S-prime is 1 | 320 | 80 | 0x27967F: numeric description identifier 22, numeric 0x16, default category; returned pair ignored |
| 0x2797C5 | [100 STAMINA] | Row-80 captured byte C is 2, and the later current-entity byte S-prime is any value other than 1 | 320 | 80 | 0x279761: numeric description identifier 52, numeric 0x34, default category; returned pair ignored |
| 0x279952 | [100 STAMINA] | Row-80 captured byte C is 1 | 320 | 80 | 0x2798EC: numeric description identifier 55, numeric 0x37, default category; returned pair ignored |

The decimal values and hexadecimal identifiers in this table describe native numeric inputs. This contract does not assign class, specialization or action names to C, S, A or B. The annotation labels are exact original text, not evidence of the downstream mechanics their English words might suggest.

## The one returned-flow continuation

Only the annotation at 0x278961 uses a preceding returned flow component for an annotation coordinate. It follows the B-equals-50 annotation at 0x2788D1, whose supplied pair is the fixed 320, 20 and whose supplied string is the current temporary containing [ALL MP]. The description at 0x2786BB does not supply this continuation.

At 0x2788D6 the caller reads the first four bytes at the returned pointer from 0x2788D1 as binary32 and converts that scalar to a signed 32-bit integer by truncation toward zero. It retains the resulting 32-bit value in a preserved register. This is not a read of the second component and not a conversion of the input coordinate 320. Writing the 32-bit register zero-extends its containing 64-bit register; subsequent behavior must therefore be described by the actual operand widths rather than a guessed source-language signed expression.

The [ALL MP] temporary is cleaned up at 0x2788DF. The owner then reloads G, W and E and performs the new tag-13 lookup at 0x2788FB. A nonnull result branches directly to the common continuation at 0x2789DF, skipping the second annotation. A null result selects the annotation primary state and constructs a new [25 STAMINA] temporary. That cleanup, current-entity reload, tag lookup and new literal construction occur before the first coordinate for 0x278961 is finalized; the already converted 32-bit value survives these normally returning calls.

At 0x278944 the caller adds decimal 20 to the retained integer through an effective-address operation whose destination is a 32-bit register. The published result is the low 32 bits of that sum. It then transfers those 32 bits to a SIMD register and converts the signed 32-bit low lane back to binary32 at 0x27894B. This converted result is the first coordinate supplied at 0x278961. The second coordinate remains binary32 20. There is no further addition of 320 and no use of the earlier returned second component. The native-width addition is modulo 2 to the power 32 at its 32-bit publication boundary, followed by a signed interpretation for the integer-to-binary32 conversion.

For a finite first returned component whose truncation is representable as signed 32-bit, truncation occurs before the addition and before conversion back to binary32. The final integer-to-binary32 step can round values that are not exactly representable. This audit does not establish the process SIMD rounding mode, exceptional-conversion handling or masking state. It therefore supplies no universal numeric result for a NaN, infinity or an out-of-range first component, and makes no source-language undefined-behavior inference. If execution returns from that native conversion and reaches the later annotation, the stated operand widths and low-32-bit addition still describe the reached operations.

These source-only examples are conditional on the helper returning the chosen first component, normal completion of intervening calls, and a null tag-13 lookup; they are not original runtime observations or font measurements:

| Chosen first component returned by 0x2788D1 | Signed truncation | After the native 32-bit addition of 20 | Supplied pair at 0x278961 |
| ---: | ---: | ---: | --- |
| 339.875 | 339 | 359 | 359, 20 |
| -1.75 | -1 | 19 | 19, 20 |
| 0 | 0 | 20 | 20, 20 |

The zero example can also arise from the independently proved resource-null return of 0x269210 described below. If the resource becomes available or changes before the later annotation, the captured continuation remains the value already computed. If the later tag lookup is nonnull, no second annotation is called regardless of the returned flow value.

## Actor-byte reads and per-row order

After the B branch rejoins at 0x2789DF, the owner obtains a binding string from current G+0x1BEC through 0x4C830, prefixes Hold followed by one space, and calls 0x269210 at 0x278A47 with coordinates 0, 40. It cleans up those strings and changes primary state before loading the current E for the row-40 choice at 0x278ABE. The zero-extended byte at E+0x14C is read at 0x278AC5 as C for this row.

For C equal to 1, identifier 0x5F is described at 0x279007 and [STAMINA] is supplied at 0x279087. For C equal to 2, the same selected E's byte at +0x14D is read at 0x278DCA. Value 1 selects identifier 0x51 at 0x278E4D and [STAMINA] at 0x278EB3; every other byte selects identifier 0x50 at 0x278F2D with no annotation. For C equal to 3, identifier 0x53 is described at 0x278D96 with no annotation. For C equal to 4, the same selected E's byte at +0x14D is read at 0x278B0B. Value 1 selects identifier 0x51 at 0x278BA8 and [STAMINA] at 0x278C28; every other byte selects identifier 0x50 at 0x278CBC with no annotation. Other C values skip these row-40 descriptions and annotations, while the preceding Hold binding row has already been called.

Each of these row-40 numeric descriptions uses 0x16C5E0 followed by 0x16BE20 with category default and an empty extra string. Each supplies coordinates 180, 40. Its normal temporary cleanup precedes the annotation primary-state change and literal construction. The captured branch values are not read again between their description and annotation, and no returned description component is used as a coordinate.

After the row-40 join, the owner forms Move followed by one space, a plus sign and one space, then the binding from current G+0x1C2C. It supplies that string at 0x279104 with coordinates 0, 60. The next text call, 0x279179, supplies the literal Dodge at 180, 60. Its cleanup and primary-state change precede the unconditional [25 STAMINA] call at 0x2791D8 at 320, 60. This annotation has no fresh action lookup and no tag-list query. The earlier A or B tag tests do not gate it.

After cleanup of that annotation, the owner reloads current E at 0x2791F3 and reads its zero-extended byte +0x14C at 0x2791FA as the new C for the optional row 80. This is independent of the earlier row-40 C read. C equal to 4 additionally reads the same selected E's byte +0x14D at 0x279225; only zero admits that route. C values 1, 2 and 3 admit their respective routes without that initial S test. Other C values, and C equal to 4 with nonzero S, skip the optional Tap binding, description and annotation.

Each admitted optional route forms the prefix Tap followed by one space and the binding at current G+0x1C2C. Their text calls at 0x27929D, 0x279429, 0x2795B4 and 0x279840 all supply 0, 80. Those correspond respectively to captured C values 4, 3, 2 and 1. Description calls then supply 180, 80; annotation calls supply 320, 80. The C-equals-4 route uses numeric description 0xA0 and [100 STAMINA]; C equal to 3 uses 0x33 and [100 MANA]; C equal to 1 uses 0x37 and [100 STAMINA].

The C-equals-2 route has a deliberate later read. After Tap text call 0x2795B4, its cleanup and the description primary-state change, the owner reloads H+0x1E8, G+8 and W+0x448, then compares that current E's byte +0x14D at 0x279605. It does not reread C. S-prime equal to 1 selects description 0x16 and [50 STAMINA]; every other value selects description 0x34 and [100 STAMINA]. Consequently the S-prime receiver can differ from the receiver whose C admitted this route if an intervening provider or reentry changes current ownership. That possibility is a boundary, not a claim that such a mutation normally occurs.

The final nonannotation row uses the binding at current G+0x1C4C and the literal Call/Ride Pet. Its text calls are 0x2799B6 and 0x279A26. The second coordinate is 80 when the optional row was skipped, or 100 when it was admitted, derived from a retained 32-bit row value 60 or 80 and a native addition of 20 before integer-to-binary32 conversion. This row value is set independently after the B join; it is not the earlier [ALL MP] returned-flow integer. Neither final text call adds another bracketed annotation.

## What the returned text-flow pair does and does not establish

The complete helper at 0x269210 first tests the current eight-byte resource pointer H+0x1A8. If null, it writes one zero eight-byte result pair and returns its address without calling the manager or font provider. This return admits the caller's normal subsequent annotation predicates; it does not itself skip the rest of the Help owner. For the one continuation, the returned first zero is therefore a defined input to the later signed truncation.

With a nonnull resource, the helper calls virtual offset +0x88 on current manager H+8 with current node H+0x178, then virtual offset +0x70 on a newly loaded current manager to obtain a 16-component binary32 matrix in local storage. The concrete manager implementations behind these virtual calls are not selected complete owners in this bounded audit. No general transform-stack, clipping or screen-origin semantics are imported from their offsets.

After those manager calls, the helper captures the binary32 scalar at H+0x1B4. It reads the current node's eight-byte spatial pointer at +0x60, a signed 32-bit selected index at spatial+0x244, and the eight-byte array pointer at spatial+0x278. The selected record stride is eight bytes, with two binary32 components. For each component separately, it adds that selected current component to the supplied coordinate using one binary32 addition, then multiplies by the saved matrix's first binary32 component using one binary32 multiplication. The same saved matrix component is used for both axes. There is no Help-side integer conversion of the fixed supplied 320, 180 or row coordinates in this helper.

The font boundary is 0x34FA60. Its reached input bundle includes the string supplied by the caller, the transformed two-component point, current style fields H+0x1B0, H+0x1B8, H+0x1BC and H+0x1C0, the captured extra scalar from H+0x1B4, the current resource, the current four-byte field H+0x1E4, and a one-byte value 1. The current vectors at H+0x1C4 and H+0x1D4 also participate according to the branch. This audit verifies their original call-boundary operands without closing the font routine, glyph lookup, string layout, backend draw, resource load or imported font-library behavior.

A positive ordered comparison of the captured H+0x1B4 against zero selects two font calls. The first supplies a zero primary vector, the selected secondary vector and a zero additional vector, and its first and second returned binary32 components are retained. The second supplies the current primary vector, rereads the relevant current style/resource/flag fields, and supplies zero for the extra scalar; its returned pair is ignored. The input point for both calls is the previously computed point. A nonpositive comparison selects one font call with the current primary vector and the captured extra scalar, and retains that call's returned pair. If an unordered comparison completes, its native branch takes the same one-call route. The selected owner does not establish the process's floating exception masking, so no claim is made that every signaling/unordered input completes that comparison.

After the selected font call or calls, the helper calls virtual offset +0x90 on current H+8 with current H+0x178. It then rereads the current node/spatial/index/array chain. It divides the retained first component once by the previously saved matrix's first component, and divides the retained second component once by that same saved scalar. It stores those divided components, then subtracts the respective components from the newly selected spatial record and overwrites the two result fields in first-component, second-component order. These are ordered scalar binary32 divisions and subtractions, not algebraically combined operations. The post-provider spatial record need not be the pre-provider record if current state changes between the reads.

Accordingly the returned pair used at 0x2788D1 is already the output of that helper boundary. It is neither promised to equal the initial 320, 20 nor promised to be the final pixel position. The later continuation truncates the first returned component exactly as returned. There is no guard here against zero scale, a nonfinite matrix component, an out-of-domain selected spatial index, a dead receiver, or provider reentry. The selected native owner contains no lock, and no imported or transitive locking, scheduling or reentry guarantee is inferred. The normal initial style writes of 12 and 3 do not freeze those fields against later provider-side changes.

## Literal and coordinate-data evidence

All annotation text is native UTF-16 with two-byte code units. The original literal storage is null-terminated; explicit call lengths count the displayed code units and exclude the terminator. Where the owner uses a zero-terminated string constructor instead of an explicit-length constructor, the same image-backed terminator and unit count were checked. No operating-system layout, language resource or external asset supplies these seven annotation literals.

| Literal RVA | Exact annotation | Code units excluding terminator | Exact decimal UTF-16 units |
| --- | --- | ---: | --- |
| 0x46EA18 | [25 STAMINA] | 12 | 91, 50, 53, 32, 83, 84, 65, 77, 73, 78, 65, 93 |
| 0x46EA48 | [ALL MP] | 8 | 91, 65, 76, 76, 32, 77, 80, 93 |
| 0x46EA60 | [CHARGE] | 8 | 91, 67, 72, 65, 82, 71, 69, 93 |
| 0x46EA88 | [STAMINA] | 9 | 91, 83, 84, 65, 77, 73, 78, 65, 93 |
| 0x46EAC0 | [100 STAMINA] | 13 | 91, 49, 48, 48, 32, 83, 84, 65, 77, 73, 78, 65, 93 |
| 0x46EAE0 | [100 MANA] | 10 | 91, 49, 48, 48, 32, 77, 65, 78, 65, 93 |
| 0x46EAF8 | [50 STAMINA] | 12 | 91, 53, 48, 32, 83, 84, 65, 77, 73, 78, 65, 93 |

Other directly relevant native strings were checked in the image: default at 0x4363F8 has seven code units; the empty string at 0x42FF38 has zero; Dodge at 0x430478 has five; uppercase HOLD followed by a space at 0x46EA38 has five; mixed-case Hold followed by a space at 0x46EA78 has five; Move followed by a space, a plus sign and a space at 0x46EAA0 has seven; Tap followed by a space at 0x46EAB0 has four. The constructor's resource1.dat has thirteen code units. Numeric description output remains provider output; this audit does not invent the text returned for identifiers A, B, 0x50, 0x51, 0x53, 0x5F, 0x16, 0x33, 0x34, 0x37 or 0xA0.

The original four-byte constants for first coordinate 320 and description coordinate 180 represent binary32 values exactly. The five row values 0, 20, 40, 60 and 80 are also exact binary32. All seven annotation literals and their terminators, their current literal-load operands, their construction lengths or terminator-based constructors, their 15 text call operands, the 20-byte destination table at 0x279A74, the 103-byte remapping table at 0x279A88, and the coordinate/style constant operands were reconciled with the image. Table data were not counted as instruction coverage. The complete Help instruction body ends at 0x279A72; the destination table follows one padding byte and is followed by the remapping table, whose exclusive end is 0x279AEF.

The 15 annotation calls are 0x278492, 0x2787E0, 0x27886B, 0x2788D1, 0x278961, 0x2789BF, 0x278C28, 0x278EB3, 0x279087, 0x2791D8, 0x2793AE, 0x279539, 0x2796E3, 0x2797C5 and 0x279952. For completeness, the other 25 text call sites in that same complete owner are 0x27819A, 0x27836C, 0x27853A, 0x2785C1, 0x2786BB, 0x278A47, 0x278BA8, 0x278CBC, 0x278D96, 0x278E4D, 0x278F2D, 0x279007, 0x279104, 0x279179, 0x27929D, 0x27934A, 0x279429, 0x2794D5, 0x2795B4, 0x27967F, 0x279761, 0x279840, 0x2798EC, 0x2799B6 and 0x279A26. This census is exhaustive for direct calls to 0x269210 inside 0x278040; it is not a census of all text calls in the process or all callers of those providers.

## Original evidence manifest and limits

The original C export files, all under C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled/functions, are:

| Export filename | UTF-8 bytes | SHA256 |
| --- | ---: | --- |
| 140278040.c | 26,323 | 21749849C0263F907BB47AB298B1877188068598390B5C4C249759CBBC117D51 |
| 140269210.c | 3,984 | 20FF44D49B407CB1125DA1AFC0B9B860DB3279330E935A62AC76E75CC1AA1977 |
| 14005e3a0.c | 476 | 85C6B6AB160FFF0DAB6A1344C1978126F59F15E24F4DD97B5FDC9D6544C633A9 |
| 140277ec0.c | 1,376 | E1F9F82E3E788F3041A141A72717A6E87804043285EAA18D7BB8223A47FF17A3 |
| 1402691b0.c | 214 | B60671170F6E71505ED25BDB8BE0EBED01AF178EF71A902E264AC757F122E52E |
| 1402691f0.c | 214 | CC893D3A02E5E35AE808C4B982CAACEA87597C1D9B877ABF5E553871834C84B0 |
| 140269160.c | 367 | 57547A0F62D47C4047B66B05F1609D89AB781292E8DE3582CD55528860261CEB |
| 1402691c0.c | 367 | CFFFD6F64BA05AB27B229441B72F36FEE0FC360BADA25B0D36308A2EB5B86BB3 |

The original metadata and assembly sources under the same decompiled directory have these identities:

| File | Bytes | SHA256 |
| --- | ---: | --- |
| assembly.txt | 62,547,291 | FE07326A0F4585432A9D4E42E7CF16C570815A8FCD164DE36616095023A0740C |
| function_ranges.csv | 509,127 | BC127412C5A8F21D8BA78AA53D9EB35A16AF58C1A85906DA9FF31E968E379601 |
| calls.csv | 10,840,369 | CF410A7DD84BBEF0E9FC4FC6FD1FF3B4903D97E9126C8F1A1D69EF05CD566DCB |
| functions.csv | 1,551,545 | 940E3682B9CCC68458531AC8D948A20F8B9530CDFFF5E568C5517BA91365DCEC |

The direct-call verification across the eight selected owners covered calls to 0x173E0, 0x336F0, 0x33B10, 0x36690, 0x3D4F0, 0x41D80, 0x4C830, 0x5E3A0, 0x5FC40, 0x60120, 0x100FA0, 0x15C9A0, 0x16BE20, 0x16C5E0, 0x268A70, 0x269130, 0x269160, 0x2691B0, 0x2691C0, 0x2691F0, 0x269210, 0x34FA60, 0x392D68, 0x392DC8 and 0x393030. The image-matched call operands establish the call boundaries and order within each selected body. Only the eight complete owners in the extent table have their internal behavior closed here. The other routines, imported initialization/string/runtime services, font resources and manager virtual calls remain external boundaries for this contract.

The work does not establish ordinary UI visibility admission, Help dismissal, lifetime of the global Game owner, matrix producers outside the selected calls, a complete action-selector or dictionary closure, actual glyph metrics, resource availability, allocation-failure handling, teardown, concurrent writers, all virtual-table implementations, arbitrary alternate callers, or a universal process-wide writer census. The constructor/table join identifies the selected Help draw implementation but does not prove that every object ever passed to 0x269210 has the same history or manager type. Direct tag-list traversal presupposes a valid live list; malformed lists, dead objects and asynchronous mutation are not assigned invented behavior.

There is no selected Help-side lock or cached whole-entity snapshot around these text rows. Captured U, A, B, C and branch-local S values retain their described widths, while explicit later G/W/E and style/spatial loads observe the then-current fields if normal execution reaches them. Provider callbacks and reentry can occur only to the extent those unclosed providers permit; this contract neither guarantees nor denies such behavior. Static native order gives no wall-clock, scheduling or interthread guarantees.

The prose was prepared as a new independent artifact. The completed saved file is to be read wholly back, checked as strict UTF-8 without BOM, checked against the prepared prose byte for byte, and hashed before its FINAL / STABLE / HELD metadata is reported. The original image identity and selected correspondence are verified before that report. Once reported held, all writes to this artifact cease. Every earlier held artifact remains unread and unchanged.
