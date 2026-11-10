# Original Steam physical binding-state and Controls-capture producers

This is an independent original-source behavioral contract for the supported x64 Steam executable. It covers the ordinary application's two device samples, production of the Game's 23 binding-state bytes, the Controls editor's pending binding records and capture index, and the selected opening, selection, Defaults, OK and Cancel routes. It distinguishes native stores from imported or virtual-provider behavior. It does not describe downstream creature/action consumers or prescribe an implementation.

The authority is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, 5,757,952 bytes, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Its machine is AMD64 0x8664, preferred image base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0 and image size 0x58A000. Addresses below are RVAs. The complete selected native owner ranges and their instruction/image correspondence are recorded at the end.

**Receivers, records and distinct kinds of input state.**

G denotes the Game receiver. In the ordinary application owner 0x1355A0, its current pointer is loaded from the eight-byte image global 0x551A80. An individual called routine retains the receiver it was passed unless a later load is expressly identified. A fresh global load is not an assertion that the global or an object's lifetime cannot change.

I is the embedded interface receiver at G+0x118. Its constructor 0xF8690 stores the captured Game pointer at I+0x5C0. U is the GUI manager pointer at G+0x20. O is the current object pointer at G+0x658, equivalently I+0x540. Original construction and RTTI identify O as cube::ControlsWidget: constructor 0x275A70 installs the primary table at 0x46E468, whose original type descriptor is at 0x54B5F0 and contains the decorated name .?AVControlsWidget@cube@@.

The active binding records are inline at G+0x1BDC. There are 23 records, indices zero through 22, each eight bytes: a four-byte code followed by a four-byte category. Their end is immediately before G+0x1C94. The pending records in O are a separate inline 184-byte array at O+0x1F0, ending immediately before the node-pointer field at O+0x2A8. They are not aliases of the Game array on the ordinary constructed receiver graph.

The sampled binding-state array is 23 individual bytes at G+0x1A70 through +0x1A86. Complete setter 0xA36C0 accepts an unsigned 32-bit index at most 22 and stores its supplied low byte there. An index outside that unsigned range causes no store. The setter itself does not normalize arbitrary callers' byte arguments; the selected sampling callers supply zero or one.

The separate 256-byte array at G+0x1A87 is written by ordinary window key-down and key-up dispatch. The inspected key-down owner stores byte one at the supplied eight-bit index before its selected key-specific and other work; key-up 0x97870 stores byte zero. Those event-array stores are not the binding-state setter and do not apply the device binding records. No equality between a window-message key number and a sampled device-array index is inferred.

O+0x2D8 is a signed 32-bit scrolling value. O+0x2DC is the current signed 32-bit candidate row produced by the selected draw body. O+0x2E0 is the signed 32-bit binding-capture row. Constructor 0x275A70 writes zero to +0x2D8, then one eight-byte all-ones store covering +0x2DC and +0x2E0. The ordinary initial candidate and capture indices are therefore both -1.

GUI pointer capture at U+0x168, the manager pointer returned by 0x342090 at U+0x170, the popup-related pointer at U+0x178, the dispatched GUI button word at U+0x1A8 and the Controls binding-capture row are different operands. OS SetCapture and ReleaseCapture calls are also separate. None is silently substituted for another.

A node's display object is at node+0x68. Its selected visibility integer is in the array at display+0xE0, at the signed four-byte index display+0xAC. This identity is independently supported by complete original display constructor 0x3736C0, which binds that integer property to the original narrow literal visibility. Its clipping property is separately bound to array+0x168/index+0x134. Tests and writes called visibility below refer to the former selected four-byte integer. They do not establish asset geometry.

**Construction and the two active/pending binding arrays.**

The ordinary Game constructor 0x7EA80 initializes its active binding array through 0x41DD0 and later calls 0x42B30 on that array. The latter is the original controls.cfg reader, discussed below. Interface construction 0xF9AC0 follows that load. In its selected setup slice it allocates 0x2E8 bytes, constructs O through 0x275A70 with the captured Game and a cloned interface node, publishes the object at I+0x540, supplies its selected size inputs and hides its root. After interface construction returns, the Game constructor calls 0x89B80 with G and G's own active binding-array address. That copies the array onto itself and reaches the original persistence writer.

O's constructor independently initializes its own pending array through 0x41DD0. In that selected complete body it does not copy G's active array into the pending array. Complete 0x41DD0 clears 23 successive eight-byte records, then calls 0x43760. The latter writes defined eight-byte records in this index order: 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 3, 13, 2, 14, 15, 16, 17, 20, 18, 19. Records 21 and 22 retain the preceding zero records.

The pairs produced by these complete default-initialization helpers are below. Later constructor/helper calls remain subject to the explicitly stated provider boundaries. The identifiers in the last column are exact native configuration-token associations, not a specification of their downstream gameplay effects. Codes are numeric native operands; no OS-layout or SDK enumeration names are assigned to them.

| Index | Code | Category | Native configuration token |
| ---: | ---: | ---: | --- |
| 0 | 0 | 1 | ButtonAttack |
| 1 | 1 | 1 | ButtonSpecialAttack |
| 2 | 42 | 0 | ButtonClassSkill |
| 3 | 19 | 0 | ButtonUltimateSkill |
| 4 | 16 | 0 | ButtonQuickItem |
| 5 | 17 | 0 | ButtonUp |
| 6 | 31 | 0 | ButtonDown |
| 7 | 30 | 0 | ButtonLeft |
| 8 | 32 | 0 | ButtonRight |
| 9 | 57 | 0 | ButtonJump |
| 10 | 2 | 1 | ButtonDodge |
| 11 | 18 | 0 | ButtonUse |
| 12 | 33 | 0 | ButtonLamp |
| 13 | 29 | 0 | ButtonWalk |
| 14 | 20 | 0 | ButtonCallPet |
| 15 | 50 | 0 | ButtonWorldMap |
| 16 | 15 | 0 | ButtonRingMenu |
| 17 | 48 | 0 | ButtonInventory |
| 18 | 46 | 0 | ButtonCraftingMenu |
| 19 | 24 | 0 | ButtonSystemMenu |
| 20 | 36 | 0 | ButtonMultiplayerMenu |
| 21 | 0 | 0 | ButtonZoomIn |
| 22 | 0 | 0 | ButtonZoomOut |

This establishes these selected constructors and writers, not that a generic newly allocated Game has every binding-state byte initialized to zero before its first admitted sampling pass. Unmentioned bytes and untraced bulk/container constructors are not assigned default values.

The base construction reached through 0x268A70, 0x328FF0, 0x328F00 and 0x3669C0 stores the manager association at object+8, retains the supplied root at object+0x178 and registers the object through the original manager/node helpers. The selected node attachment call is 0x334410. Its replacement path may transfer the prior control's selected fields, remove the prior control through a manager helper, then attach the new control through 0x32D190. That complete helper writes the supplied control to the nonnull node's +0x70 and writes the supplied node to control+0x178. The untraced allocation/registry/attachment providers do not establish reference-counted lifetime protection for the captured Game or O.

**Ordinary application ownership and admission before either sampling branch.**

Complete application owner 0x1355A0 calls the imported DINPUT8.DLL DirectInput8Create with version argument 0x800 and original identifier bytes at 0x473C80. A zero 32-bit return admits its device/setup/Game/input-loop route. A nonzero return selects its failure-message/cleanup route before that Game/input loop.

The first returned device interface, called K here, is requested through the created interface's virtual slot+0x18 with original identifier at 0x473CE0. It is configured through K's slot+0x58 with the original block at 0x473C60, then K's slot+0x38 is called. The second device interface, M, is requested through the created interface's slot+0x18 with identifier at 0x473CD0. It is configured through slot+0x58 with block 0x473C40, then its slot+0x68 is called with the current main-window handle and four-byte argument ten, followed by slot+0x38.

The native provider identifiers are distinguished by their original image contents: 0x473CE0 begins with four-byte word 0x6F1D2B61, whereas 0x473CD0 begins with 0x6F1D2B60; both have the remaining original bytes A0 D5 CF 11 BF C7 44 45 53 54 00 00. The creator's identifier at 0x473C80 begins with word 0xBF798031, followed by 3A 48 A2 4D AA 99 5D 64 ED 36 97 00. These are literal native provider inputs. No SDK header or imported vtable implementation was read to assign additional enumeration/provider guarantees.

The two 32-byte format blocks have initial four-byte words 32, 24, 2 and respectively 256 or 16; their next four-byte words are respectively 256 and seven, followed by a zero word and an eight-byte pointer, respectively preferred VA 0x140546FC0 or 0x140546F10. Their full bytes were checked in the image. The exact transitive provider interpretation of those tables remains outside this contract. The K and M labels identify the ordinary 256-byte indexed source and 16-byte source used by the native category-zero and category-one branches.

The selected device creation, format and acquisition-like slot calls do not test their individual returns or add null-recovery paths. The zero creator return is not proof that every subsequent provider operation succeeds.

The application publishes G to global 0x551A80 only after the ordinary Game constructor returns. It then performs its window-show/cursor/focus/foreground calls. Before its loop it zeros a 256-byte previous-K snapshot and a separate 16-byte previous-M snapshot. The current K/M output buffers are not freshly zeroed before each later sample. No universally zero current first sample is inferred if a provider fails to supply data.

At the start of each loop iteration it tests current byte G+0x1D68 and may post a quit message. It drains the selected pending window messages through PeekMessageW, GetMessageW, TranslateMessage and DispatchMessageW, checking G+0x1D68 again during that drain. GetMessageW returning zero selects the normal teardown path rather than a device sample.

After the message drain, image byte 0x536CE8 must be nonzero. The owner performs its selected client-rectangle/cursor/query work, then tests that byte again and requires GetForegroundWindow's returned handle to equal the current main-window handle at image global 0x551AC8. Failure of either admission goes to the Sleep call with argument 16, then the ordinary 0x134420 frame call. That route performs neither device sample, neither capture/normal binding branch, nor the previous-device snapshot copies.

The supported image stores initial byte one at 0x536CE8. The inspected complete window procedure's messages seven and eight return one without changing it. The exact-address instruction discovery found the two reads in 0x1355A0 and no direct exact-address writer. This is a bounded discovery fact, not a universal writer census or a proof that the byte represents OS focus. Indirect/address-derived writes and external changes remain unresolved.

On admission, the owner calls K's virtual slot+0x48 first, with four-byte size 256 and the current 256-byte output-buffer address. It then calls M's slot+0x48 with four-byte size 16 and the current 16-byte output-buffer address. The native call sites are 0x13665C and 0x136672. Neither returned value is examined before the buffers are read. There is no success-dependent clear or retry/reacquisition in this selected loop. Consequently a normally returning failed/partial provider call leaves the following native decisions dependent on whatever bytes are actually in those output buffers; zero, retained previous bytes and a fabricated fresh sample are not interchangeable assumptions.

The next call, at 0x13667C, is 0x9AE50 on current G. This complete getter loads current O and returns low-byte one precisely when O+0x2E0 has its 32-bit sign bit clear, and zero when the sign bit is set. It does not test O's root visibility, an upper bound of 22, the manager pointer U+0x170, U+0x168 pointer capture, other Game text flags, a name, a World/player pointer or a device return code. The getter is sampled once to choose the two application branches below.

**The noncapturing branch: 23 current levels, in index order.**

A negative capture-row word selects the normal branch at 0x136734. The owner walks the 23 active records in ascending index order, with eight-byte record stride, and uses 0xA36C0 for the corresponding state byte. For each index its first call supplies zero before that record is interpreted. This happens regardless of whether the record's category or code is supported.

For category zero, a first four-byte read of code zero leaves that cleared state unchanged. With that first code read nonzero, the code is read again and sign-extended for addressing. Its low 32 bits must pass the native unsigned comparison at most 255. On that admission the selected K byte is tested for any nonzero value. A second setter call stores normalized byte one for nonzero, zero for zero. An out-of-range code instead makes a second zero setter call.

For category one, the signed-load/unsigned-comparison code must be at most three. On admission it reads the selected byte at M-buffer offset twelve plus that code, then uses the same nonzero normalization and second setter call. Thus code zero/category one is a valid source, distinct from the zero/zero empty record. An out-of-range category-one code makes the second zero setter call.

A category other than zero or one performs no source-array read and no second setter call in this record iteration. The preceding zero setter still occurred. The loop does not stop at the first active record; duplicate active pairs can therefore activate more than one index under stable inputs. Negative code words fail the unsigned bounds in either supported category. Category-zero codes 1..255 and category-one codes 0..3 are the stable-record domains used by this normal producer.

The native producer does not isolate a high bit of either selected source byte. Values one, 0x80 and 0xFF all produce state one. A zero byte produces state zero. These stores represent the current sampled level. This branch does not construct separate pressed/released pulses, compare the previous-device snapshots or invoke the binding's downstream action consumer.

After a selected source byte is found nonzero and its normalized state is stored, the owner reloads current G from the global and writes byte zero at G+0x1958. A zero source byte, an unsupported record and the first zero setter do not themselves clear that byte. The byte is not set back merely because every record is inactive. It is separately initialized and written by other selected input routes; no device-type business label is needed to describe its effect on Controls drawing.

The Game pointer used for a setter and the later global reload for record/mode-byte work are separate loads. The category word is read for branch selection. The category-zero code's initial zero test and its later index load are also separate reads. No whole-array copy, lock, immutable binding-record snapshot or cross-thread consistency is supplied by these leaf setter calls. The stable-record domain statements above apply when those words do not change between the identified reads.

After the 23 iterations, this branch calls 0x1014F0 on I for its selected cursor/motion route. When its low-byte result is nonzero, GetCursorPos is called and ScreenToClient is conditional on the current four-byte global at 0x551BF0 being zero; those signed 32-bit coordinates are converted to binary32. With a zero low-byte result, it instead converts the first two signed 32-bit words of the current M buffer to binary32. It then calls complete Game motion owner 0x97880 with G and those two floats. Its transitive interface/camera work is outside the binding-state contract and can precede later field reloads.

This helper does not gate the preceding 23-record loop on GUI focus. Complete 0x1014F0 returns low-byte one when its ordered composite interface test finds a nonzero selected visibility/byte operand. Its direct-node tests, in order, are I offsets 0, 0x18, 0x58, 0x80 and 0xE8; then byte I+0x550; then the roots at object pointers I+0x218, +0x228, +0x270 and +0x238; then byte I+0x308; then roots at I+0x258, +0x250 and +0x248; then object I+0x280's byte+0x1D8; then roots at I+0x260, +0x240, +0x540 and +0x538. The object-root tests read object+0x178. Only all-zero tested operands produce low-byte zero. Its upper return bits need not be zero and are not the boolean consumed here. None of these tests reads the manager pointer at U+0x170 through 0x342090, whose complete body simply returns U+0x170.

**Later writes from the other sampled provider.**

The normal branch does not end immediately after the keyboard/mouse records. After motion returns, the owner zeros a separate 16-byte buffer, calls 0xA36C0 with zero for index 21 and then index 22, and calls imported XINPUT1_4.DLL XInputGetState with provider index zero and that buffer. These extra zero stores occur even if active keyboard/mouse bindings had just made indices 21 or 22 nonzero. They do not run in the Controls-capturing branch.

A nonzero 32-bit return does not enter the following state-one calls. A zero return admits additional writes to the same 23-byte array. The native numeric inputs are described without imported SDK button names. In the returned 16-byte buffer, the low byte of the word at offset four is called B, the entire unsigned word there is called T, and the unsigned bytes at offsets six/seven are separate scalar inputs.

The initial ordered admitted writes are: B bit zero sets index 21 to one; bit one sets 22; bit five sets 18 then 17, both to one; bit four sets 19; T bit nine sets index zero; byte seven strictly greater than 128 sets index one. Each reached group also writes one to a freshly loaded G+0x1958. None of those direct calls changes its state to zero merely because its condition is false.

The later outer route is selected by a fresh 0x1014F0 result zero or a nonzero G+0x668 read. Within that route, after the selected analog/helper calls, a later zero G+0x668 read admits these further writes in order: T bit 15 sets index three; T bit 13 with global byte 0x551CBC zero and a further 0x1014F0 result zero sets nine; T bit eight sets ten; returned byte six strictly greater than 128 sets two; T bit 14 sets four; T bit twelve sets eleven. Each reached group writes one to G+0x1958.

At that outer route's common continuation, B bit three sets index twelve, bit six sets fourteen, bit seven sets sixteen and bit two sets fifteen, in that order, again with the selected G+0x1958 one stores. The other inner route includes synthetic Game mouse calls and arrow callbacks rather than the six selected state calls just listed. The alternative outer route contains selected GUI/pointer operations. Those transitive operations, analog numerical semantics, imported state meanings and gesture behavior are not expanded.

This is enough to distinguish a keyboard/mouse-derived state from the final state after all reached same-loop writers. Conditions, flags, globals and G are read at their actual native points; earlier helper results and later flag reads are not one transaction. This bounded account does not turn XInputGetState's imported implementation into a keyboard/mouse producer or assign names to its numeric fields.

**The capturing branch: source edges and exact scan priority.**

A nonnegative O+0x2E0 word, including nonnegative values above 22, selects the capture branch at 0x13668C. It bypasses the entire normal 23-record loop, its motion call, the index-21/22 clears and the other state-provider block. No automatic zeroing of the existing binding-state bytes occurs in that branch.

The K scan has 256 candidate indices, zero through 255, in ascending order. At each index the edge predicate requires the previous K snapshot byte to equal zero and the current K byte to be nonzero. A held nonzero byte, including a changed nonzero bit pattern, is not a fresh edge; nonzero-to-zero is not admitted. The predicate again uses any nonzero byte, without a high-bit-only test.

The first admitted K edge ends this scan. Numeric indices 1, 0x1C and 0x9C skip the record-writer call. Other indices prepare an eight-byte stack record by storing the code as a four-byte index and the category as a four-byte zero, then call 0x8A120 with current G and that record. After either an excluded or accepted K edge, the owner reloads current G and calls 0x8A4F0, then goes to the common previous-snapshot-copy continuation. No later K index or mouse subscan runs after that first K edge.

If a K index has no admitted edge, the owner scans all four M button-byte positions, zero through three, before advancing to the next K index. Each M edge likewise requires the corresponding previous-M byte at offset twelve plus index to be zero and the current-M byte there to be nonzero. For each admitted M edge it prepares code=index/category=one as two four-byte stack stores and calls 0x8A120. It does not call 0x8A4F0 and does not break this inner loop merely because a mouse edge was admitted.

The four-entry mouse scan is consequently nested inside the 256-entry K scan. It is not one mouse scan after a completed keyboard scan. With no K edges, the same still-rising M bytes can cause the same captures in each of 256 subscans, using the unchanged snapshots for that application pass. Multiple rising M bytes are offered in ascending order in each subscan. A later accepted K edge can replace the value produced by earlier M subscans. An excluded K edge can terminate capture after prior M writes without undoing them.

Complete wrapper 0x8A120 reloads O from its passed Game's +0x658 and tail-calls 0x2769A0, preserving the record argument seen in the native calling registers. Its C presentation omits that live second argument. Complete 0x8A4F0 independently reloads O and tail-calls 0x2761F0. The latter stores signed four-byte -1 only at O+0x2E0. It does not restore pending records or copy them to G.

Complete record writer 0x2769A0 first requires the unsigned current capture index at most 22. A negative word or any unsigned word above 22 returns without changing pending records or the candidate row. This is stricter than the sign-only application branch getter.

For an admitted index it scans all 23 pending records in ascending order. Each comparison tests the full category word first and the full code word second against the supplied record. Every exact pair match is cleared with one eight-byte zero store. The supplied category/code are reread as the loop proceeds. After that scan, the source eight-byte record is loaded, the signed capture index is reread, and the eight-byte source is stored into that row. The last store in the successful body is four-byte -1 at O+0x2DC. It does not clear O+0x2E0.

For the ordinary distinct stack input records, this removes every previous occurrence of that pair before publishing the chosen row's pair. Category and code together determine a duplicate; equal codes in different categories are different records. Supplying code zero/category zero can publish the empty pair. No reserved-code filter exists inside this writer itself.

The captured O pointer is retained during this leaf writer; there are no calls inside it. The reread source and index nevertheless do not establish safety under concurrent writers or an alternate caller aliasing the source record into the pending array. The successful ordinary behavior assumes the passed stack record remains distinct/live and the selected row remains within the checked domain through its later reload. The contract does not invent a repair for a concurrently replaced object or selector.

**Common snapshot publication and repeat behavior.**

At continuation 0x137B04 the owner copies all 256 current K bytes to the previous-K snapshot. Native copying uses two successive 128-byte groups, each composed of eight 16-byte loads/stores in ascending offsets. It then copies the current 16-byte M buffer to the previous-M snapshot with a single 16-byte transfer at 0x137B72/0x137B79. This happens after either admitted branch, including excluded-key cancellation, no capture edge and normally returning provider failures whose outputs were still consumed. The copies precede the common 0x134420 frame call.

A global-byte or foreground admission failure uses the separate sleep continuation and skips those copies. Subsequent edge comparisons are therefore against the last copied bytes, not necessarily the preceding wall-clock frame. While capture is active and no relevant edge or separate release/draw writer ends it, repeated capture passes advance the snapshots but retain the old 23 action-state bytes. No wall-clock sampling frequency, reliable provider error state, fixed number of rendered frames or scheduler order is supplied.

**Controls opening and its retained pending state.**

The selected ordinary SystemWidget construction is 0x291C70, published by the relevant 0xF9AC0 setup slice at I+0x240, equivalently G+0x358. Original RTTI at its installed primary table identifies cube::SystemWidget. Its stored Game is object+0x1E8 and its signed selected option is +0x1F0.

Complete SystemWidget draw owner 0x291E00 resets that selected option to -1, obtains its local point and tests four successive finite Y bands with current sampled width. For the Controls option it requires local X at least zero and strictly below the separately sampled width, with Y at least 40 and strictly below 70; it writes option one on that admission. The other options use bands 10..40, 70..100 and 100..130, with inclusive lower and exclusive upper edges, and values zero, two and three respectively. Their native captions are Options, Controls, Start Menu and Exit Game. Actual transformed point, width, resource drawing and malformed/unordered-provider behavior remain the reached helpers' inputs and limits.

At that draw body's end it registers numeric event three with a raw captured SystemWidget receiver, zero member adjustment and target 0x292300 through 0x291AB0. That complete native callback sequence first hides its own current root, then reads the selected option. Option one loads the stored Game, adds 0x118 and tail-transfers to 0x102ED0. Other selected branches transfer to separately identified targets 0x103110, 0x9FDF0 or 0x9F660, or return; their further effects are outside the Controls route.

The complete selected leaf sequence 0x102ED0 through 0x102F02 reads I+0x540, its root/display and current active visibility index, then writes four-byte one when the selected visibility integer was zero and four-byte zero otherwise. The index is read again at its native store preparation. It contains no pending-binding import, rollback, capture-row reset, candidate-row reset or scroll reset. The complete selected Cancel sequence at 0x276A00 is described below rather than invented from a neighboring export.

No decoded C owning file is catalogued for these two leaves or for 0x292300. Their complete selected instructions, terminal return/tail paths and original raw-image bytes were independently checked over the bounded extents in the manifest. There is no reliance on a previous handoff or on a guessed neighboring body.

Within the selected construction, opening and Cancel routes, pending records remain in the retained O. Opening changes its root visibility, rather than recreating O or synchronizing pending records from the Game. Under the explicit condition that no other writer/provider changes either array, a pending edit followed by Cancel and this opening route is still present in O on reopening; Cancel alone has not copied it to G. This is a bounded native consequence, not an exhaustive negative claim about every alternate caller or untraced resource operation.

**Candidate-row production is draw work, not a message-pump hit result.**

O's primary virtual table has 0x276200 in slot+8 and 0x276CD0 in slot+0x50. This establishes the concrete selected draw/layout targets. It does not prove how many times a general GUI/render traversal calls them between two device samples or two messages.

Complete draw owner 0x276200 first processes the Defaults node at O+0x2B8, the OK node at +0x2A8 and the Cancel node at +0x2B0, in that order. For each it calls 0x332AD0 for two coordinate pairs, adds the corresponding binary32 endpoints, multiplies each rounded sum by binary32 0.5, converts each to signed 32 bits toward zero and passes the resulting two-word point to 0x6ECE0 on the captured Game's list at G+0x6C8. After the reached count/error check it increments the eight-byte count at G+0x6D0 and publishes the list links. The exact list-construction provider and invalid float conversions are not supplied with invented results.

It then calls 0x329E00 on O for a binary32 height term. It subtracts binary32 120, converts that rounded result toward zero to signed 32 bits, divides that integer toward zero by 30 and adds the original signed O+0x2D8 word with 32-bit arithmetic. The beginning row is the original scroll word clamped to 0..23 inclusive; the ending row is the computed integer clamped to the same domain. The beginning is inclusive, the end exclusive. If beginning is not below end no rows are traversed. These bounds are established before the row loop.

The owner next obtains a local binary32 point through 0x32B180 and only after that call returns clears O+0x2DC to four-byte -1. It makes its selected style-helper calls. A nonzero byte at the captured Game's +0x1958 then clears O+0x2E0 to -1 before the row loop. This store is in the draw body itself; it is not a keyboard edge or button-release store.

The first traversed row's vertical baseline is binary32 27, independent of the starting record index. After each traversed row the baseline advances by binary32 addition of 30, and the record index advances by one. The pending record pointer advances eight bytes per row.

For a row equal to the current O+0x2E0 word, it draws the selected row label and the original 11-code-unit UTF-16 caption Press a key through its current style/text providers. That row does not run the normal candidate admission. The row's binding-name formatter is not called in that selected branch.

For another row, the original zero/zero pending pair selects its distinct color input. Candidate admission additionally requires the capture-row word sampled at that row's branch to be negative and a reread captured-Game byte+0x1958 to be zero. For ordinary finite operands the local point must satisfy all of the following, in order:

- X is at least zero.
- X is strictly less than a newly sampled 0x32A1D0 width return after binary32 subtraction of 40.
- Y is at least the current row baseline after binary32 subtraction of 20.
- Y is strictly less than the current baseline after binary32 addition of ten.

An admitted row writes its signed four-byte index to O+0x2DC and selects the original candidate color input before its text calls. Native COMISS/carry and below-or-equal branches reject unordered operands at these selected point comparisons if execution continues without a floating exception. Equality is accepted only on the two lower edges. No clipping, OS capture, U+0x170 pointer, matching press or GUI-root target comparison is added by these local numeric predicates.

The normal row calls 0x42670 with its numeric row index for its label, then calls 0x4C830 on the captured Game's +0x30 receiver and the current pending eight-byte record for the binding text. The selected column-mode calls are zero then one. Left text uses binary32 X=20. Right text placement samples width again, subtracts 250, multiplies that rounded difference by 0.5 and adds 240, in that order. Text/font/resource outputs, key names and glyph placement are named provider boundaries; they are not inferred from OS keyboard layouts or another dictionary contract.

Calls while drawing an earlier row can precede the next row's capture-index/Game-byte/width loads. The height, starting/end bounds and local point captured for this draw are distinct from those later loads. The retained O and the matrix/coordinate providers do not imply reentrancy or cross-thread protection.

After the rows, each nonnull node at O+0x2C0, +0x2C8 and +0x2D0 is separately queried through its own control at node+0x70 and 0x32B0C0. Depending on that boolean, the owner writes one of its two original four-word color inputs into the current node display's selected four-float element at array+0x1F0/index+0x1BC, in ascending four-byte component stores. This work does not publish another binding record, candidate row or capture row. General geometry inside 0x32AE90 and its providers is not closed by this color decision.

Complete point wrapper 0x32B180 obtains the selected root's matrix terms through 0x100EE0, reads the manager's cached binary32 point at U+0x180/+0x184, obtains the object's position pair through 0x32CF50, subtracts the latter from the cached point and combines those differences with the returned matrix terms in the native ordered binary32 products and sums. It does not call GetCursorPos. The width/height wrappers 0x32A1D0/0x329E00 return the first/second word of 0x32A110's size pair. That complete size owner reads current object floats at +0x70/+0x74, +0x80/+0x84 and +0x90 through +0x9C, applies its original ordered differences, zero/maximum selections and final additions/subtractions. Matrix/position production, stale cached points and asset geometry remain explicit inputs. These local numeric bands do not establish universal screen-pixel rectangles.

**Ordinary release arms or ends capture after GUI and other Game work.**

The complete window procedure 0x133C70 calls OS ReleaseCapture before Game release 0x965E0 for the selected ordinary mouse-release messages, supplying button integers zero, one or two. It does not pass message coordinates into those Game owners. The application also has selected other-provider direct callers of Game mouse routines; the input is not proven to originate exclusively in an OS mouse message.

Complete Game release 0x965E0 performs its selected current menu/option prefix, then calls 0x101120 on I at 0x96767. The adapter routes button zero to 0x3410E0, one to 0x341780 and two to 0x3414A0; other 32-bit values do not invoke one of those three GUI adapters. The selected Controls buttons below are registered for the button-zero numeric event-three route.

Complete button-zero GUI release 0x3410E0 clears the lowest bit of the four-byte U+0x1A8 word. It uses nonnull U+0x168 as its initial target; otherwise it calls 0x333F60 on the current root U+0x140 with the manager's current point address U+0x180 and mask zero. A nonnull U+0x178 together with nonnull U+0x1B0 admits the prior 0x342270 call with zero arguments.

For a target, its effective control at node+0x78 and that control's byte+0x1A0 admit the first virtual slot+0x60 call. After that returns it rereads the retained node's flags at +0x80 and effective-control pointer. With node flag bit one clear and a nonnull effective control it looks up numeric event three through 0x32A210. A nonnull callback is admitted when the current control's byte+0x1A0 is nonzero or the callback object's byte+8 is zero. The constructor-selected Controls callbacks have byte+8 equal to one, so their ordinary event-three calls require that control admission byte to be nonzero.

The selected inherited Controls/System virtual release target is 0x32BB40. It checks current manager pointer U+0x168 and, when its node's effective control equals its receiver, calls 0x3493B0. That helper clears U+0x168 and calls 0x3418D0 with the manager's cached point words. After that returns, 0x32BB40 writes two-byte zero at its own +0x180. The transitive 0x3418D0 dispatch is a boundary that can precede the later effective-control/callback rereads; no immutable initial GUI target/control association is assumed.

After the initial event-three opportunity, 0x3410E0 rereads U+0x168 or performs another root query and performs a separate root query for comparison of effective controls. A mismatch admits its selected virtual+0xB8/event-0xE route. It then performs the selected root/forward-child broadcast through 0x33AADC and 0x33C360. Those broadcasts test current visibility and node flag bits one/two, invoke each admitted node's own control, then traverse children in forward list order. The transfer uses virtual slot+0xF0. A selected target callback therefore precedes further GUI work before the Game's later capture-row stores. No callback consumption return suppresses the whole Game release owner.

Back in 0x965E0, other selected string/widget/provider, coordinate and item-related phases occur before its final Controls suffix. They are separate boundaries in this bounded contract; their transitive gameplay/World effects are not expanded. Every normally returning release path reaches that suffix at 0x96E00. Native invalid-parameter/error/exception paths are not converted into normal returns.

The suffix first reloads O and calls 0x2761F0, unconditionally writing -1 to its capture index for every button value and regardless of current Controls visibility. It then reloads O, its current root/display and selected visibility integer. Only a nonzero visibility integer, button zero and a nonnegative current O+0x2DC admit 0x276F30 at 0x96E40. That complete helper copies the current four-byte candidate word into the capture word.

There is no upper-bound-of-22 guard at this arm call, no new candidate geometry query, no check that this node received GUI release, and no comparison with a matching prior press. Ordinary candidate production supplies indices 0..22; alternate nonnegative candidate words can arm a sign-nonnegative capture index that the record writer subsequently rejects. A nonzero button cancels the capture word and does not rearm it. The candidate can be a value retained from an earlier draw; the window message itself does not recompute it.

Thus the ordinary release suffix can end an outstanding mouse capture, or arm a row after callbacks have returned. Callback-specific writes and this later universal suffix are not the same cleanup operation. A direct alternate call to a callback does not inherit the suffix automatically.

**Button registration, callback records and lifetime.**

The complete Controls constructor clones its supplied button node three times, publishing Defaults at O+0x2B8, OK at +0x2A8 and Cancel at +0x2B0. Their original supplied captions are respectively eight, two and six UTF-16 code units, with original literal operands checked in the image. Their selected sizing provider inputs are binary32 140 and 15, followed by byte one.

It registers numeric event three on each node's own control through 0x2758B0, in Defaults, OK, Cancel order. The raw captured receiver is O, the signed member adjustment used by the selected targets is zero, descendant-registration argument is one and callback admission byte is one. The targets are 0x276A30, 0x276C80 and 0x276A00 respectively. These are release-event registrations, distinct from a direct device edge.

Complete registration helper 0x2758B0 searches the control's event tree at +0x188 using signed 32-bit event keys. A missing key is created through the selected original tree helpers. It frees an existing nonnull callback payload before allocating a replacement 0x28-byte object. The new object stores its table pointer at zero, supplied admission byte at +8, eight-byte captured receiver at +0x10 and the supplied 16-byte member-target descriptor at +0x18 through +0x27. The event node's eight-byte payload at +0x28 is then replaced, including null on the selected allocation-null route.

With the descendant argument nonzero, it builds the selected control collection through 0x32AC80/0x32ABA0, recursively registers on those controls in forward collection order, then frees that temporary collection. The collection traversal prunes nodes with flag bit two; it recurses through the receiver root and through nodes without an own control, and appends a different node's own control without descending further in that same collection pass. Registration can consequently replace additional descendant callback payloads; it is not only a root-control write.

The named-node helper 0x2757B0 compares the complete UTF-16 name/length on a node's own control and recursively visits children. In the constructor, nonnull optional inputs at O+0x2C0/+0x2C8 are cloned and bound by the original upbutton/downbutton names for numeric event two to 0x276F10/0x276EA0. A nonnull input at +0x2D0 is cloned and its scrollbutton descendant is bound for event 0xC to 0x276AF0. The constructor then calls 0x269130 with its original resource1.dat literal; resource contents and provider-defined widget behavior were not read.

The SystemWidget's event helper 0x291AB0 has the same selected callback-record shape and replacement/recursive-registration order, with its own table at 0x470A78. The first pointers at both 0x46E618 and 0x470A78 equal preferred VA 0x140288C40. The complete 15-byte transfer at 0x288C40 loads a signed 32-bit adjustment from callback+0x20, adds the current eight-byte captured receiver at +0x10 and tail-transfers through the current eight-byte target at +0x18. The high four bytes copied at +0x24 are not read by that transfer and are not assigned an invented initialized value.

Event lookup 0x32A210 uses the current event tree and signed key. A miss can insert a node with a null payload, so a lookup is not necessarily read-only. The callback object is a raw capture plus target/adjustment, not proof of a retained/shared O or Game lifetime. Replacement frees the previous payload before publishing the next one; exceptions, concurrent removal, a callback reentering registration and enclosing lifetime protection remain unresolved.

**Defaults, OK and Cancel.**

Complete Defaults target 0x276A30 constructs a fresh local 184-byte binding table through 0x41DD0. It then copies all pending records to O+0x1F0 in ascending offsets. The native widths are eleven 16-byte transfers at offsets zero through 0xA0, followed by one eight-byte transfer at 0xB0. It does not directly write the active Game records, binding-state bytes, scroll value, candidate row, capture row or root visibility. A later normally returning Game release suffix can independently reset/rearm capture.

Complete OK target 0x276C80 loads the captured Game from O+0x1E8 and calls 0x89B80 with that Game and O's pending-array address. The original OK caption is the apply-and-hide control in this selected constructor; no separately named Controls Apply button is manufactured.

Complete 0x89B80 copies all 184 bytes to the passed Game's active array at +0x1BDC. Its native store order is eleven 16-byte writes at ascending offsets zero through 0xA0 and one eight-byte write at +0xB0. It then tail-calls persistence writer 0x438E0 on that same destination-array pointer. It does not recompute the 23 binding-state bytes or call 0xA36C0. The next admitted normal producer samples the newly current records at its own later reads.

After 0x89B80 and its persistence work return, OK rereads its retained O's current root/display/index and hides that root with a four-byte zero visibility store. There is no returned success flag tested before that hide and no native rollback of the already copied Game array in this selected body. Stream/provider failures and exceptions do not justify an invented successful save, atomic transaction or cleanup return.

Complete Cancel at 0x276A00 through 0x276A20 simply reads its current root/display/index, writes four-byte zero to selected visibility and returns. It does not call the Game copier, persistence writer, Defaults constructor or capture-reset helper. It does not directly restore O's pending records from G. The later Game release suffix normally clears the capture word after the GUI callback and intervening Game work; a direct Cancel invocation has no such automatic enclosing guarantee.

**Configuration reader/writer boundaries.**

The native controls.cfg reader 0x42B30 is a directly reached initial producer of G's active records. Its complete selected body opens the literal through 0x41B20 with numeric arguments one and 0x40, checks its stream-state operand and reads tokens through the original stream helpers. It recognizes the 23 exact token/index associations in the table above. For a recognized token it reads a signed 32-bit code and checks stream state, then reads a signed 32-bit category and checks state again. Only after both admissions does it write code then category as two four-byte stores to the selected eight-byte record.

The selected reader does not validate the code/category against the device scan domains, remove duplicate pairs or reset all other records on each successful token. Unknown tokens continue the token loop without a selected record write. A stream-state failure stops the loop; a failed pair does not cause the two selected record stores, and earlier successful stores remain. Repeated successful occurrences of an identifier replace that record in input-token order. Exact imported parsing, encoding, numeric overflow/locale and filesystem behavior remain stream-provider dependencies. No actual installation controls.cfg was opened by this research.

Complete persistence writer 0x438E0 reaches the original stream constructor 0x41C50 with the controls.cfg literal and numeric arguments two, 0x40 and one. In order, it supplies these native token/record indices to its string/integer insertion calls: 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 3, 2, 13, 14, 15, 16, 17, 20, 18, 19, 21, 22. Each original token literal includes one trailing ASCII space. It inserts that record's signed code, the separate original one-space literal at 0x4306C8, its signed category and the selected 0x9550 stream manipulator, then proceeds to the next record. It destroys its stream objects afterward.

There is no native per-record provider-success test, state rollback or selection-specific skip in this writer's body. Its parameter is the active array itself; it samples successive words during those calls rather than first copying another whole-array snapshot. Actual file opening, output formatting/flush success, persistence durability, external file changes and exceptional paths remain provider limits. The literal route and attempted ordered data supply are source facts; successful storage on a particular machine is not an observation from this assignment.

**Scroll/layout producers that affect row choice.**

These selected helpers alter the scroll/layout inputs; they do not directly copy a pending binding pair to the Game. Complete layout owner 0x276CD0 positions the three button controls using current selected dimensions, calls 0x276F40, then positions the optional up/down controls when present. Its current size/position/matrix providers remain boundaries rather than fixed asset geometry.

Complete up target 0x276F10 adds -1 to the four-byte scroll word, reloads that word, selects zero when the addition's sign flag is set and otherwise retains the loaded value, stores the selected four-byte result, then tail-calls 0x276F40. Under stable memory this retains a wrapped result whose sign flag is clear. The sign decision uses the addition's flags; the later word load is distinct from an original-value test before decrement.

Complete down target 0x276EA0 calls 0x329E00 on O, subtracts binary32 150, increments the four-byte scroll word, then subtracts binary32 35 from the retained height result. It converts that result toward zero to signed 32 bits and divides the integer toward zero by 30. A signed scroll value at least 23 less that quotient is replaced with 22 less that quotient. It then tail-calls 0x276F40. Height, increment and the later bound store are distinct native points.

Complete scrollbar-layout helper 0x276F40 returns without its selected geometry work if O+0x2D0 is null. Otherwise it samples the O height, subtracts 150 then 35 in binary32 and converts toward zero to a signed 32-bit length. It divides that length by 30 toward zero, multiplies that quotient by the length with 32-bit arithmetic, and divides the wrapped product toward zero by 23 for its integer thumb-height input. It samples width from the current thumb control, then forms its selected signed integer position from the length-minus-thumb-height, current scroll word, a divisor of 22 less the earlier length/30 quotient and final addition of 35. It samples O width again, subtracts binary32 28 and calls 0x32C680 on the captured thumb control with those X/Y/width/height inputs and byte one. The original variable signed division has no newly added zero-divisor guard; division faults, overflow and invalid provider conversions are not given a fallback.

Complete drag target 0x276AF0 is admitted by the lowest bit of the manager's dispatched button word U+0x1A8. It does not sample M's device button bytes for that admission. It obtains O height, thumb-control height, thumb-control position and the current O point-related return from the selected helpers. For finite values it clamps the rounded sum of their second coordinate terms to at least 35 and at most the selected length-minus-thumb-height plus 35. Only a positive integer length-minus-thumb-height admits its float-to-scroll calculation. It converts the selected 22-less-row-quotient to binary32, multiplies by the clamped coordinate after subtraction of 35, divides by the positive converted integer length and converts toward zero to the stored scroll integer. It then applies the native signed lower/upper scroll tests and calls 0x32B560 using a newly sampled thumb X and its retained selected Y. Native MAXSS/MINSS operand order, exceptional arithmetic and untraced position/size providers are not replaced with generic scrollbar semantics.

Ordinary wheel owner 0x97CC0 first gives priority to visible/query-admitted roots at G+0x330, G+0x350 and G+0x340, in that order. Controls is next: its root must be currently visible and 0x333E70 on that root must return true. Negative wheel input then calls 0x276EA0 on a reloaded O; positive input calls 0x276F10; zero calls neither. A Controls admission does not fall through to the later widget/fallback routes merely because the delta is zero. This selected route adds no capture-row or U+0x170 pointer test.

Complete 0x333E70 reads the manager's two cached point floats, calls 0x333F60 with the supplied root, that local point and mask zero, and converts its returned node pointer to a low-byte boolean. Its C presentation misidentifies a live first argument; original RCX retains the root. Root picking, shape/layout and underlying point providers remain unclosed boundaries. Wheel/scroll changes do not themselves recompute O+0x2DC; the draw producer and later release use their separately timed stores/reads.

**Independent conditional sequences.**

The following are source-only references with stated synthetic native inputs and normally returning providers. They are not original runtime observations, captures or replacement-engine tests.

- With stable G and a negative capture index, a category-zero record with code 42 and K byte 42 equal to one is first cleared, then becomes state one and admits the G+0x1958 zero store. Source byte 0x80 gives the same normalized state. A zero source byte gives zero and does not independently clear G+0x1958.
- Under stable records, category zero/code zero remains cleared even if K byte zero is nonzero. Category one/code zero samples M offset twelve. Code -1, category-one code four or category two leaves the selected state cleared by the first setter, without an invented out-of-range source access.
- With O draw height 420, width 640, scroll zero, negative capture index and G+0x1958 zero, the finite row loop has indices zero through nine. Its first baseline is 27, so local point X=100/Y=8 admits row zero. A subsequent normally returning button-zero Game release with O still visible and that candidate retained arms row zero, without requiring a fresh root pick for the suffix.
- With that valid armed row and no earlier K edge, a new K edge at numeric index 42 supplies pair 42/0. Under stable ordinary stack inputs it clears all pending duplicates of that pair, publishes row zero's pair, clears candidate to -1 and then cancels capture through 0x8A4F0. The active Game array and 23 binding-state bytes are not changed by those record/cancel helpers.
- With a valid armed row, no K edges and rising M positions zero and three, both captures are offered in each mouse subscan, in that order, for all 256 K iterations. The final pending pair is 3/1 when no other writer intervenes. Capture remains armed after the mouse record writers; a later release or separate admitted draw/reset writer can end it.
- With a rising M position three and the first K edge at index five, five earlier mouse subscans can write 3/1 before the code-five/category-zero record replaces it. The K branch then cancels capture and skips subsequent scans. With the first K edge instead at excluded index one, an earlier mouse write can remain while the K branch cancels capture without its own pair write.
- A held nonzero K byte does not make another capture edge merely because repeated window key-down messages were dispatched. The window event-array stores, device-buffer byte transitions and Controls record writer have different producers.
- A nonnegative capture index 23 selects the application's capture branch, but 0x2769A0 rejects its pending-record write. A K edge still reaches the separate capture-cancel wrapper; a mouse edge alone does not obtain that cancellation call.
- If an admitted normal pass makes state index 21 true from a binding record, the later explicit zero call for index 21 replaces it before XInputGetState is called. A following provider error does not restore that earlier true byte.
- With no other writers, Cancel followed by the selected Controls-opening toggle retains O's edited pending records and leaves the Game's active array unchanged. OK first copies the pending array into G, attempts the persistence sequence and only after normal return hides O. The later release suffix is a separate source of capture-index reset.

**Original evidence, extent and byte correspondence.**

Reference filenames were identified before scoped content searches. Original metadata came from decompiled/functions.csv, function_ranges.csv and calls.csv beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Selected C presentations came from the original decompiled/functions directory; instructions came from owner-selected portions of the original decompiled/assembly.txt. The supported original executable was read as bytes and was not executed.

The complete selected sampling/application owner, Controls constructors/draw/capture/callback bodies, Game release owner and the complete relevant guard/wrapper/helper bodies were read. Their C presentations were reconciled with native registers, operand widths, branches and store order before the behavioral interpretation above. The original C display is not treated as an ABI or arithmetic authority where native operands differ.

In particular: 0x8A120 retains the live second record argument omitted by its C display; 0xA36C0's callers' byte argument distinguishes zero from one even where displayed calls omit it; 0x89B80 and Defaults use the native 16-byte/eight-byte transfers rather than the C display's decomposed scalar stores; 0x333E70 keeps its root receiver in RCX; and the result-sign clamp, unsigned code bounds, COMISS branches and cached/reread operands were checked in native instructions.

For each complete exported owner listed below, every exported instruction's entire byte encoding matched the supported PE section's original raw bytes after RVA-to-section mapping. Every individual exported range had continuous instruction coverage, without an internal uncovered interval. Distinct exported ranges are stated separately; bytes between them are not silently counted as decoded instructions or as a general alternate-entry proof.

| Complete selected owner RVA | Inclusive exported native ranges | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| 0x41DD0 | 0x41DD0 through 0x41E00 | 16 | 49 |
| 0x42B30 | 0x42B30 through 0x43207; 0x43209 through 0x4320E; 0x43210 through 0x43215; 0x43217 through 0x4321C; 0x4321E through 0x43241; 0x43243 through 0x43251; 0x43253 through 0x43261; 0x43263 through 0x4326E; 0x43270 through 0x432B8 | 502 | 1,921 |
| 0x43760 | 0x43760 through 0x438D4 | 71 | 373 |
| 0x438E0 | 0x438E0 through 0x4401F | 377 | 1,856 |
| 0x89B80 | 0x89B80 through 0x89C01 | 26 | 130 |
| 0x8A120 | 0x8A120 through 0x8A12B | 2 | 12 |
| 0x8A4F0 | 0x8A4F0 through 0x8A4FB | 2 | 12 |
| 0x965E0 | 0x965E0 through 0x96832; 0x96834 through 0x96842; 0x96844 through 0x96852; 0x96854 through 0x9685F; 0x96861 through 0x96E6F | 508 | 2,188 |
| 0x9AE50 | 0x9AE50 through 0x9AE62 | 5 | 19 |
| 0xA36C0 | 0xA36C0 through 0xA36D0 | 5 | 17 |
| 0xA38C0 | 0xA38C0 through 0xA38F5 | 12 | 54 |
| 0xA3900 | 0xA3900 through 0xA3935 | 12 | 54 |
| 0xF6CC0 | 0xF6CC0 through 0xF6D04 | 18 | 69 |
| 0xF8690 | 0xF8690 through 0xF87C9 | 67 | 314 |
| 0x101120 | 0x101120 through 0x10115E | 16 | 63 |
| 0x1014F0 | 0x1014F0 through 0x10178B | 119 | 668 |
| 0x133C70 | 0x133C70 through 0x133EEB | 174 | 636 |
| 0x1355A0 | 0x1355A0 through 0x137E1B | 2,217 | 10,364 |
| 0x268A70 | 0x268A70 through 0x268B39 | 36 | 202 |
| 0x2757B0 | 0x2757B0 through 0x2758A1 | 65 | 242 |
| 0x2758B0 | 0x2758B0 through 0x275A6D | 111 | 446 |
| 0x275A70 | 0x275A70 through 0x2761AC | 425 | 1,853 |
| 0x2761F0 | 0x2761F0 through 0x2761FA | 2 | 11 |
| 0x276200 | 0x276200 through 0x276993 | 429 | 1,940 |
| 0x2769A0 | 0x2769A0 through 0x2769FA | 23 | 91 |
| 0x276A30 | 0x276A30 through 0x276AED | 39 | 190 |
| 0x276AF0 | 0x276AF0 through 0x276C73 | 90 | 388 |
| 0x276C80 | 0x276C80 through 0x276CC1 | 14 | 66 |
| 0x276CD0 | 0x276CD0 through 0x276E95 | 107 | 454 |
| 0x276EA0 | 0x276EA0 through 0x276F06 | 26 | 103 |
| 0x276F10 | 0x276F10 through 0x276F2F | 6 | 32 |
| 0x276F30 | 0x276F30 through 0x276F3C | 3 | 13 |
| 0x276F40 | 0x276F40 through 0x27705C | 70 | 285 |
| 0x291AB0 | 0x291AB0 through 0x291C6D | 111 | 446 |
| 0x291C70 | 0x291C70 through 0x291DB0 | 75 | 321 |
| 0x291E00 | 0x291E00 through 0x2922F3 | 284 | 1,268 |
| 0x328F00 | 0x328F00 through 0x328F6F | 31 | 112 |
| 0x328FF0 | 0x328FF0 through 0x329206 | 96 | 535 |
| 0x329E00 | 0x329E00 through 0x329E33 | 12 | 52 |
| 0x32A110 | 0x32A110 through 0x32A1CE | 45 | 191 |
| 0x32A1D0 | 0x32A1D0 through 0x32A202 | 12 | 51 |
| 0x32A210 | 0x32A210 through 0x32A2B4 | 46 | 165 |
| 0x32ABA0 | 0x32ABA0 through 0x32AC7C | 58 | 221 |
| 0x32AC80 | 0x32AC80 through 0x32ACE2 | 26 | 99 |
| 0x32B0C0 | 0x32B0C0 through 0x32B0CB | 2 | 12 |
| 0x32B180 | 0x32B180 through 0x32B248 | 45 | 201 |
| 0x32BB40 | 0x32BB40 through 0x32BB7C | 16 | 61 |
| 0x32D190 | 0x32D190 through 0x32D1A0 | 5 | 17 |
| 0x333E70 | 0x333E70 through 0x333EC7 | 19 | 88 |
| 0x334410 | 0x334410 through 0x334506 | 60 | 247 |
| 0x3356D0 | 0x3356D0 through 0x335773 | 42 | 164 |
| 0x335830 | 0x335830 through 0x3358D3 | 42 | 164 |
| 0x33AADC | 0x33AADC through 0x33AAE4 | 2 | 9 |
| 0x33C360 | 0x33C360 through 0x33C406 | 49 | 167 |
| 0x3410E0 | 0x3410E0 through 0x3412F4 | 141 | 533 |
| 0x342090 | 0x342090 through 0x342097 | 2 | 8 |
| 0x3493B0 | 0x3493B0 through 0x3493CF | 4 | 32 |
| 0x3669C0 | 0x3669C0 through 0x366A01 | 18 | 66 |
| 0x3736C0 | 0x3736C0 through 0x37379D; 0x37379F through 0x3737AD; 0x3737AF through 0x3737BD; 0x3737BF through 0x3737CA; 0x3737CC through 0x37386A; 0x37386C through 0x37387A; 0x37387C through 0x37388A; 0x37388C through 0x373897; 0x373899 through 0x373948; 0x37394A through 0x373958; 0x37395A through 0x373968; 0x37396A through 0x373975; 0x373977 through 0x373A1D; 0x373A1F through 0x373A2D; 0x373A2F through 0x373A3D; 0x373A3F through 0x373A4A; 0x373A4C through 0x373AF6; 0x373AF8 through 0x373B06; 0x373B08 through 0x373B16; 0x373B18 through 0x373B23; 0x373B25 through 0x373BE8 | 324 | 1,301 |

These 59 complete selected owners account for 7,162 instructions and 31,646 matched bytes. All mismatch and internal-gap counts were zero. Complete correspondence of 0x1355A0 and 0x965E0 does not claim a transitive closure of every unrelated initialization, renderer, gameplay, connection, item or virtual/imported provider they call; this prose closes the assigned physical/binding/Controls producer behavior and bounds those other calls.

The following larger or adjacent context owners were also byte-compared over all exported ranges, with zero mismatches/internal gaps. Semantic use is limited to the selected joins described after the table. Unrelated body content does not become a separate behavioral specification merely because all of its bytes matched.

| Context owner RVA | Inclusive exported native ranges | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| 0x7EA80 | 0x7EA80 through 0x7F35D; 0x7F35F through 0x7F36D; 0x7F36F through 0x7F37D; 0x7F37F through 0x7F38A; 0x7F38C through 0x7F9DF | 806 | 3,932 |
| 0x96E70 | 0x96E70 through 0x977F2 | 550 | 2,435 |
| 0x97870 | 0x97870 through 0x9787B | 3 | 12 |
| 0x97880 | 0x97880 through 0x97CB9 | 201 | 1,082 |
| 0x97CC0 | 0x97CC0 through 0x9803D | 190 | 894 |
| 0xF9AC0 | 0xF9AC0 through 0x10089D | 5,496 | 28,126 |
| 0x134420 | 0x134420 through 0x1347C5 | 192 | 934 |

Those 7 context owners account for 7,438 instructions and 37,415 matched bytes. The selected meanings are: 0x7EA80's receiver/binding initialization, original reader/interface/self-copy call order; 0xF9AC0's Controls/System construction/publication slices; 0x96E70's raw event-array prefix; 0x97870's raw event-array clear; 0x97880's current motion caller/argument boundary; 0x97CC0's priority/admitted Controls scroll route; and 0x134420's frame call after the sampled/sleep continuations. Complete C presentation was read for the event/motion/wheel/frame owners. The two larger construction contexts were interpreted through their individually selected relevant C/native slices and full native-byte checks, without importing unrelated startup/worker behavior.

The selected uncatalogued sequences have separate complete extent checks:

| Selected original sequence | Inclusive original range | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| Controls root toggle | 0x102ED0 through 0x102F02 | 11 | 51 |
| Controls Cancel | 0x276A00 through 0x276A20 | 6 | 33 |
| System event callback | 0x292300 through 0x292377 | 25 | 120 |
| Captured member transfer | 0x288C40 through 0x288C4E | 4 | 15 |

The four sequences account for 46 instructions and 219 bytes, again with no mismatch or internal gap. The root-toggle, Cancel and System callback extents were followed through their terminal return and all selected branch/tail destinations in the original instruction listing and image. No original runtime-function-table entry covering the root-toggle or Cancel entry was found. The member-transfer sequence is four directly decoded original x64 instructions ending in its indirect tail transfer; its 15 bytes were checked independently because no corresponding decoded owning export was supplied. Following padding/neighboring functions was not substituted for a callback body. Alternate entry points, unseen indirect callers and broader compiler-owned bounds remain explicit limits.

The exact corresponding original C filenames for the complete exported owners above are:
functions/140041dd0.c, functions/140042b30.c, functions/140043760.c, functions/1400438e0.c, functions/140089b80.c, functions/14008a120.c, functions/14008a4f0.c, functions/1400965e0.c, functions/14009ae50.c, functions/1400a36c0.c, functions/1400a38c0.c, functions/1400a3900.c, functions/1400f6cc0.c, functions/1400f8690.c, functions/140101120.c, functions/1401014f0.c, functions/140133c70.c, functions/1401355a0.c, functions/140268a70.c, functions/1402757b0.c, functions/1402758b0.c, functions/140275a70.c, functions/1402761f0.c, functions/140276200.c, functions/1402769a0.c, functions/140276a30.c, functions/140276af0.c, functions/140276c80.c, functions/140276cd0.c, functions/140276ea0.c, functions/140276f10.c, functions/140276f30.c, functions/140276f40.c, functions/140291ab0.c, functions/140291c70.c, functions/140291e00.c, functions/140328f00.c, functions/140328ff0.c, functions/140329e00.c, functions/14032a110.c, functions/14032a1d0.c, functions/14032a210.c, functions/14032aba0.c, functions/14032ac80.c, functions/14032b0c0.c, functions/14032b180.c, functions/14032bb40.c, functions/14032d190.c, functions/140333e70.c, functions/140334410.c, functions/1403356d0.c, functions/140335830.c, functions/14033aadc.c, functions/14033c360.c, functions/1403410e0.c, functions/140342090.c, functions/1403493b0.c, functions/1403669c0.c, functions/1403736c0.c.

The context filenames are:
functions/14007ea80.c, functions/140096e70.c, functions/140097870.c, functions/140097880.c, functions/140097cc0.c, functions/1400f9ac0.c, functions/140134420.c.

Additional filename-selected original lead reads/searches were limited to 140287050.c, 140275760.c, 140275770.c, 14032d1d0.c, selected portions of 140101bd0.c/140103200.c, and the original retry-180-phase1/functions/1400a5a50.c search used while locating the application producer. Their unrelated semantics are not used to fill this contract's explicitly bounded gaps. They are original-side discovery evidence, not implementation/project material.

The original RTTI/table associations were separately checked in the image: Controls table 0x46E468, its locator 0x4B02D8 and type descriptor 0x54B5F0; System table 0x4708B8 and its cube::SystemWidget type identity; the selected Controls draw/layout/release entries; and the two callback-table first pointers at 0x46E618/0x470A78 targeting 0x288C40. These are bounded receiver associations, not a census of every concrete node/control in an asset-created subtree.

Original literal checks included the complete ASCII controls.cfg route at 0x4306B8 and the 23 writer token literals from 0x4306D0 through 0x430870, with their exact one-space suffixes and terminators. The separate space at 0x4306C8 was checked. UTF-16 Defaults at 0x46E5D8, OK at 0x46E5EC, Cancel at 0x437350, Press a key at 0x46E5F8 and Controls at 0x470A28 were checked with their stated code-unit lengths and following two-byte zero. Narrow visibility/clipping literals at 0x47C188/0x47C198 were also checked.

Selected binary32 constant facts used in the row/scroll descriptions were checked as raw words: 0x42E5A0 is 0x41A00000 (20), 0x42E5AC is 0x42200000 (40), 0x42E7C8 is 0x42F00000 (120), 0x46DEB8 is 0x41D80000 (27), 0x42E5A8 is 0x41F00000 (30), 0x42E598 is 0x41200000 (10), 0x42E938 is 0x43160000 (150), 0x441BC0 is 0x420C0000 (35), 0x46E620 is 0x41E00000 (28), 0x4ACA38 is 0x430C0000 (140), 0x43293C is 0x437A0000 (250), 0x432D94 is 0x43700000 (240), 0x4AC964 is 0x3F000000 (0.5), 0x42E59C is 0x41400000 (12), 0x42E580 is 0x40400000 (3), 0x42E7A4 is 0x41700000 (15), 0x42E7BC is 0x428C0000 (70) and 0x42E7CC is 0x43020000 (130). The exact creator/device identifiers, both 32-byte format blocks and initial image byte 0x536CE8 were separately checked as original data.

**Limits and exposure.**

The body-level caller census is bounded. The original direct-call metadata resolves the selected 0xA36C0, 0x8A120, 0x8A4F0 and 0x9AE50 application sites and the selected Game release, constructor, Defaults and OK joins. The independently checked tables/member transfers close the selected indirect callback receiver/target relationships. This does not enumerate every address-derived field writer, alternate caller, external callback, cloned asset control or possible virtual target. A missing exact-address match or exported C body is not a universal absence claim.

The contract establishes widths, comparisons, ordinary scan/store/call order and the stated writer ownership. It does not certify an imported provider's buffer contents, failure convention beyond the actual tested return words, key/button enumeration names, OS layout, exclusive device capture, acquisition survival, thread affinity, message delivery/repeat scheduling, device availability, filesystem persistence, resource/font availability or general GUI geometry. No SDK/provider implementation, installation configuration or asset content was consulted to supply those values.

The first/current output buffers, previous snapshots, Game event array, sampled binding-state bytes, active binding records, pending binding records, candidate index, capture index and GUI/OS capture operands have distinct scopes and update points. There is no whole-loop immutable input snapshot or a demonstrated general lock around them. The selected bodies' raw pointer captures and later rereads do not establish safe object deletion/replacement, cross-thread ordering or callback reentrancy exclusion. Providers can precede later reads; unrelated Game/UI phases between the selected calls are not assumed to be inert.

Ordinary behavior assumes valid live receiver graphs, valid node/display/property arrays and normally returning selected calls. Null ordinary roots/device interfaces, malformed tree/list headers, invalid external indices, concurrent selector changes, aliased alternate record arguments, allocation failure, exceptions/unwinding, invalid float-to-integer conversions and signed division faults are not assigned recovery behavior. Native integer widths and wrap/flag operands are described without importing C++ undefined-behavior assumptions. Floating rounding/exception configuration, unordered providers beyond the stated COMISS point gates and deeper matrix/noise/font/geometry closures remain unresolved.

The application may continue its selected frame/update call after an input-admission sleep. The retained binding-state bytes during skipped/capturing passes are source stores/omissions; this contract does not infer how downstream control consumers treat them. It neither manufactures controls nor expands the selected producer routes into a universal gameplay/input census.

Exposure statement: this author read only the standalone docs/CLEANROOM_AUTHOR.md role policy, this isolated assignment and individually selected supported original Steam executable/export evidence and metadata. No active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, combined project history, replacement engine/exporter/extractor, SDK/dependency/reconstruction source, tests/results, owner files/data/outputs/captures, actual configuration, assets or packages was read. No original executable was run, no additional agent was spawned and no implementation or verification feedback was requested or consumed. Every prior held artifact remained unread and unchanged throughout this assignment.

This complete independent code-free artifact is released only after exclusive creation, full saved-file readback, exact byte-length/SHA256 verification and rechecking the supported original identity. At the separately reported FINAL / STABLE / HELD boundary all artifact edits stop.
