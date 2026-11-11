# Frontend Chat ingress and retained owner lifetime — supported Steam original

This is an independent, bounded original-source behavioral contract. It concerns ordinary delivered window keyboard/character messages, the GUI manager and current Game handlers they reach, the separately retained Chat editor and displayed-message records, and the selected frontend entry/exit joins. It describes normal returning paths with the concrete owners below. It does not prescribe a replacement implementation or a transport protocol, and its conditional examples are mathematical/source conditions, not runtime observations.

## Original authority, method and scope

The authority is the original file C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe: 5,757,952 bytes, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244; AMD64 machine 0x8664; preferred image base 0x140000000; PE timestamp 0x5D93AB0D; entry RVA 0x3939F0; image size 0x58A000. Every routine address in this document is an RVA relative to that base. This is one supported image, not a cross-version assertion.

Only the standalone docs/CLEANROOM_AUTHOR.md role policy, this assignment, and individually filename-selected original executable/export evidence were used. The original export directory is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled. Original functions/*.c presentation guided the review; complete selected native owners and the specified caller windows were reconciled with assembly.txt instruction operands and raw PE bytes. The manifest at the end distinguishes complete owners from bounded windows. Native operands decide widths, receiver aliases, comparison polarity and order when C presentation loses them. No native instructions, code, pseudocode or byte sequences are reproduced here.

The complete-owner manifest has 128 selected original owners, 197 exported owning ranges, 14,708 instructions and 63,131 matched instruction bytes. Twenty additional bounded caller/provenance windows contain 4,942 matched bytes; these overlap some complete owners and must not be added as a count of unique image coverage. No compared instruction byte differed, and no listed owning-range instruction interval was left unmatched. This is not a census of every possible indirect caller, alias or memory writer. In particular, large Interface setup/update, application startup and Game update are expressly bounded to the windows identified below. Reading their selected windows does not establish their entire transitive behavior.

## Concrete objects and independent state

For the ordinary construction traced here, G denotes the allocated Game, W the current pointer read from G+0x8, E the current pointer read from W+0x448, I the embedded Interface at G+0x118, and M the GUI manager read from G+0x20. I+0x5C0 stores G. The process Game pointer is the eight-byte global at RVA 0x551A80; the process manager pointer is the separate eight-byte global at 0x551AC0. The selected startup publishes M after its constructor and G after the complete Game constructor returns. Both globals lie in the original .data virtual zero-fill extension: that section begins at RVA 0x530000, has raw extent 0x1DE00 and virtual extent 0x23C40. This establishes image initial zero-fill, not a safe dispatch interval before publication or after destruction.

The following fields are distinct even when their offsets resemble fields in other object types.

| Receiver and field | Width and reached identity |
| --- | --- |
| G+0x220, equivalently I+0x108 | Eight-byte pointer to the unnamed parent GUI node created under M+0x140; it is the parent used for Chat and numerous ordinary HUD nodes. It is not the Chat widget or a character-name field. |
| G+0x398, equivalently I+0x280 | Eight-byte pointer C to the concrete 0x1F0-byte Chat widget constructed at 0x26A2E0. Its node is a child of the preceding parent node. |
| C+0x1A8 / C+0x1B0 | Outer displayed-row list head / eight-byte row count. Each row owns a separate list of text/color pieces. |
| C+0x1B8 / +0x1C8 / +0x1D0 | UTF-16 draft string record / eight-byte code-unit length / eight-byte capacity. Capacity at most seven selects inline storage; larger capacity selects the pointer in the record. The terminator is a two-byte unit. |
| C+0x1D8 | One-byte typing admission state, separately changed from the draft string, caret, selected range and displayed rows. |
| C+0x1E0 | Eight-byte resource pointer returned by the selected original resource lookup. |
| C+0x1E8 / C+0x1EC | Signed four-byte caret and signed four-byte selection displacement. |
| C+0x15C | One-byte update/draw dirty indication inherited from the widget base; this is not the typing byte. |
| M+0x170 | Eight-byte current focused widget pointer; it is neither the captured node M+0x168 nor the separate popup widget M+0x178. |
| M+0x198 | Head of a unique ordered collection whose keys are unsigned two-byte values. |
| M+0x1AC | Four-byte last delivered GUI key value, written from the zero-extended two-byte key on both down and up. |
| G+0x1A87 through G+0x1B86 | 256 separate raw key bytes indexed by the unsigned low input byte. These are not the GUI collection or the sampled action bytes elsewhere in G. |
| G+0x21A0 / G+0x21A8 | Separate pending message list head / eight-byte count. Its payload contains an eight-byte identifier followed by a UTF-16 string. It is not C's displayed-row collection. |
| G+0x658 | Current eight-byte Controls widget pointer Q. The selected Chat keydown gate reads signed four-byte Q+0x2E0; it does not query Q's selected GUI node instead. |

For a node N, the selected integer used in these routes belongs to the separate receiver at N+0x68. Its array pointer is at receiver+0xE0, and its signed four-byte selected index is at receiver+0xAC; the element width is four bytes. Helpers 0x9AF00 and 0xA3A20 respectively test the current selected element for nonzero and store a normalized zero/one from an input byte. These operations concern that node's selected integer. They do not read ancestors or M's focus/capture state. The physical element address is selected anew wherever the original reloads the receiver and index; it is not a boolean stored directly at G+0x220.

## Window to GUI to current Game

The complete window owner 0x133C70 recognizes low-four-byte message identifiers 0x100, 0x101 and 0x102. For each, it first reads the process M global and calls the appropriate GUI owner with the zero-extended low 16 bits of the saved input word. After that GUI call returns, it reads the process G global again and calls the corresponding Game owner with the zero-extended low eight bits of that same saved input word. The selected order is:

| Message identifier | First GUI call | Following current-Game call |
| --- | --- | --- |
| 0x100 | 0x340DF0, key down | 0x96E70, key down |
| 0x101 | 0x340E60, key up | 0x97870, key up |
| 0x102 | 0x340DD0, character | 0x9F670, character |

The respective GUI/Game call-site pairs are 0x133DA4/0x133DB3, 0x133D79/0x133D88 and 0x133D4E/0x133D5D. No consumed-event result from the GUI suppresses the following Game call. The manager or its focused widget can perform callbacks before the process G pointer is reloaded. The original does not promise that the current Game, focused widget, page selection, typing state or other callback-visible data stayed unchanged during that call.

There is no selected title-page, selected-character, named-character, live-entity-health or Chat-visibility test in this window routing. There is also no repeat-bit inspection or repeat suppression in the selected window owner. Repeated delivered down/character messages reach these calls again. This is a statement about handling delivered messages, not an assertion about OS generation of messages or the number or timing of key repeats.

Message identifiers 7 and 8 return directly in this owner. They do not clear M+0x170, the GUI key collection, the Game raw array, C+0x1D8 or the draft. Thus OS focus notifications and GUI widget focus must not be treated as the same state transition here. The selected owner also calls SetCapture before ordinary button-press dispatch and ReleaseCapture before button-release dispatch. Those imported calls are distinct from M+0x168; their implementation, return behavior and possible reentry are not closed.

GUI character owner 0x340DD0 reads current M+0x170 once and returns when it is null or the widget's byte+0x1A0 is zero. Otherwise it calls virtual slot +0xD0 with the carried two-byte character argument. The owner has no ancestor/page-selected test. GUI keydown 0x340DF0 first writes M+0x1AC and inserts the unsigned two-byte key into M+0x198, then reloads focused widget and applies the same null/byte+0x1A0 gate before virtual +0xC0. Keyup 0x340E60 writes the same last-key field, erases that unsigned two-byte key, reloads focus, and conditionally calls virtual +0xC8. A duplicate keydown may leave membership unchanged while still invoking the focused widget again. The insertion/erasure helpers preserve unique unsigned-two-byte key membership; allocation, balancing and failure internals below the selected helpers are not a message-delivery guarantee.

## Game keydown, keyup and character admission

Complete Game keydown 0x96E70 captures the unsigned low input byte and first stores one into its raw-array element. Complete keyup 0x97870 stores zero into that element and returns. These stores do not depend on GUI consumption, focus, typing state or a frontend page. They record current delivered down/up levels; neither owner constructs a separate press edge or debounces repeated downs.

Before any Chat gate, keydown handles seven particular byte values. The native dispatch table selects 0x1B and 0x70 through 0x75; the other entries go to the common Chat gate. Byte 0x1B toggles the one-byte G+0x420 between zero and one according to whether its prior byte was zero. Byte 0x70 toggles the current selected integer of the node reached through G+0x390 and widget+0x178. Byte 0x73 toggles the current selected integer of M+0x140. These three cases continue directly to the Chat gate.

Byte 0x71 divides binary32 G+0x664 by 1.25 and applies the lower constant whose original word is 0x3ED1B716, approximately 0.4095999598503113. Byte 0x72 multiplies the same field by 1.25 and applies the upper constant 0x40435000, exactly 3.0517578125. Each stores its arithmetic result before the comparison, chooses between that stored value and its constant, calls Interface layout 0x101BD0 on I with current four-byte G+0x1D60 and G+0x1D64, then copies the resulting G+0x664 word to G+0x1BCC. Byte 0x74 divides binary32 G+0x1BD0 by original constant 0x3F8CCCCD, approximately 1.100000023841858, with lower constant 0x3F40562A, approximately 0.7513147592544556. Byte 0x75 multiplies that field by the same constant with upper constant 0x3FAA5E36, approximately 1.3310000896453857. Each of 0x71, 0x72, 0x74 and 0x75 then calls configuration boundary 0x13B1D0 on G+0x1B88 before the common Chat gate. The native comparisons select the arithmetic value at equality and also on unordered flags. These finite-value bounds therefore do not establish NaN repair. The complete internal layout/configuration effects are outside this contract.

The Chat portion then reads and tests these current values in strict order:

1. The selected four-byte integer of the node currently in G+0x220 must be nonzero.
2. Getter 0x342090 on current G+0x20 must return a null focused-widget pointer.
3. Signed four-byte field +0x2E0 of the current Q from G+0x658 must be negative.

Only after all three pass does keydown load the current C from G+0x398 and test C+0x1D8. Neither GUI capture M+0x168 nor the node's ancestor visibility is independently queried at this point. The root operand here is specifically the retained I+0x108 parent node, whose ordinary publication is described below. Failed Chat admission leaves earlier raw/F-key/menu-byte effects already performed.

If C+0x1D8 was zero, only key byte 0x0D sets that byte to one. It does not clear the draft, caret, selected displacement or displayed rows. That branch returns without calling Chat's key-edit owner. Other keys leave the typing byte unchanged.

If C+0x1D8 was nonzero, ordinary keys proceed to 0x26B300 on a fresh load of current G+0x398. For key byte 0x0D, the Game first sets the initially loaded C+0x15C byte to one, reloads current C and clears its typing byte, then reloads C again and tests its full eight-byte draft length. A nonempty draft is tokenized through the original wide-stream boundary. The selected command processing and pending record follow in the next section. After that processing, or immediately for an empty draft, the Game clears the draft of the then-current C through the original UTF-16 string assignment helper. It then calls 0x26B300 on another fresh current C with key 0x0D. Thus typing is cleared before token parsing, while the final string clear and key normalization occur later. A provider/callback exceptional exit does not imply those later effects completed.

There is no Chat cancel action attached to key 0x1B in the complete Game keydown owner or the complete Chat key-edit owner. Its early G+0x420 toggle still occurs; when the three Chat gates pass and typing was already active, the Chat key owner normalizes indices and marks dirty but does not close typing or clear the draft for this key. Keys 0x26 and 0x28 likewise do not recall previously submitted/displayed text in these selected owners. This is bounded absence in complete selected owners, not a prohibition on every other caller or input route.

Complete Game character owner 0x9F670 has a different admission. It captures the unsigned low byte, queries focus on current G+0x20, and returns if the focus pointer is nonnull. With null focus it reloads current C and requires C+0x1D8 nonzero. It does not test the G+0x220 selected integer, Q+0x2E0, page roots, name, selected slot, actor action or health. At 0x9F69D it sign-extends the captured low eight bits to the four-byte argument register, then calls 0x26B170 at 0x9F6A1. That callee consumes the low unsigned 16 bits. Consequently byte values 0x80 through 0xFF become UTF-16 units 0xFF80 through 0xFFFF on this Game route; values below 0x80 retain their value. This is neither a full input-word Unicode conversion nor a claim about any imported text-input system.

The GUI phase receives the original low 16 bits before this Game conversion. Since the Game checks focus after the GUI callback returned, a callback may change the admission of the later Game route for that same delivered message. A null focus test here is stronger than testing whether a nonnull focused widget is enabled: even an unavailable/disabled nonnull focus pointer blocks the Game character route.

## Exact Chat draft editing

The concrete Chat primary table at RVA 0x46D1E8 selects 0x26A600 for deleting slot zero and 0x26A930 for draw slot +8. Its keyboard/character slots +0xC0, +0xC8 and +0xD0 select the original no-effect owner 0xE8A20. The ordinary Game calls 0x26B170 and 0x26B300 directly. They must not be identified as the automatic result of focusing Chat and invoking those three GUI virtual slots.

Complete 0x26B600 normalizes signed four-byte C+0x1E8 and C+0x1EC against the signed low four bytes of the eight-byte draft length. It first raises a negative caret to zero, then lowers a caret greater than that signed length to the length. Using the resulting caret, it tests the native four-byte sum of selection displacement and caret. A negative sum changes the selection displacement to the negative of the caret. It next reads the current displacement again and, if that sum exceeds the signed low length, changes the caret to the length minus displacement. This final correction changes the caret, not the displacement. The native four-byte adds/subtracts and signed comparisons are the established behavior; no unbounded mathematical-index rule or whole-string safety claim is made for huge lengths, signed overflow or corrupt records.

Complete 0x26B170 calls this normalization before it tests the unsigned two-byte character against 0x20. A rejected control unit can therefore still normalize the indices. For an admitted unit it builds a one-unit UTF-16 temporary; if the current selection displacement is nonzero it calls deletion owner 0x26A8C0; it normalizes again, inserts the one-unit temporary at the sign-extended current caret, increments the four-byte caret by one after the string helper returns, and releases the temporary. There is no typing-byte test inside this direct callee and no store to C+0x15C in its own body. Its ordinary Game caller supplies the typing gate; Interface's later dirty marker is separate.

Complete 0x26A8C0 first normalizes. If the signed caret is no greater than the signed low length, a zero displacement requests deletion of one unit at the caret; a positive displacement requests that many units starting at the caret; a negative displacement requests the units beginning at caret plus displacement and ending at the former caret. It then adds a still-negative current displacement to the current caret and clears the displacement. The record helpers' allocation, exceptions and invalid-range behavior are boundaries; this contract does not turn a native signed comparison into a proof that every arbitrary string index is valid.

Complete 0x26B300 normalizes first and unconditionally sets C+0x15C to one, then handles the following two-byte key values. It does not itself inspect the typing byte.

| Key value | Ordered reached draft/index behavior |
| --- | --- |
| 0x0D | Returns after normalization and dirty marking. Game submission/closing occurred in the outer owner. |
| 0x25 | Only if signed caret is positive: decrements caret, then tests unsigned GUI-key collection membership for numeric key 0x10. Membership increments selection displacement; absence clears it. At the left edge there is no membership query or selection-clear store. |
| 0x27 | Only if signed caret is less than signed low length: increments caret, then queries membership 0x10. Membership decrements displacement; absence clears it. At the right edge there is no membership query or selection-clear store. |
| 0x2E | Calls 0x26A8C0, including for zero selected displacement. |
| 0x08 | With nonzero displacement calls 0x26A8C0. Otherwise, only a positive caret erases one unit immediately before it and then decrements caret. |
| 0x24 | Queries membership 0x10. Absence clears displacement; membership adds the old caret to displacement. Then caret becomes zero. |
| 0x23 | Queries membership 0x10. Absence clears displacement; membership adds old caret minus signed low length to displacement. Then caret becomes that signed low length. |
| Every other value | No further draft/index action after normalization and dirty marking. |

These are code-unit operations. No grapheme, surrogate-pair, OS keyboard-layout, clipboard, composition or command-history behavior is imported. Membership numeric 0x10 is the value actually looked up; the GUI insertion/erase order determines which currently delivered keys that collection contains. Keyup has no direct Chat editor operation on the selected Game route.

## Submission commands and pending message records

The complete keydown owner compares the first UTF-16 stream token against exact literals in this order. Comparison includes the token's full eight-byte length, so a prefix alone does not match. Token extraction and stream failure/locale behavior remain original runtime boundaries.

| Exact first token | Reached selected behavior |
| --- | --- |
| /check, six units | Toggles the separate byte at current G+0x388 widget+0x1F0. Formats that byte through 0x44320, prefixes the exact 31-unit literal “multiplayerWidget->checkGame = ”, appends one LF, and calls current Chat log wrapper 0x26BF10 with the selected original color record. The provider's boolean spelling is not invented here. |
| /namepet, eight units | Extracts a second token. An empty second token produces no selected effect. For a nonempty token shorter than 16 UTF-16 units, reloads E and admits 0x10A3C0 on E+0x8B8 only when that record's first byte is 0x14. Length at least 16 instead logs the exact 18-unit literal “Name is too long.” followed by LF. The helper's transitive entity behavior is not closed. |
| /sit, four units | Uses the actor comparison below; admitted stores are current E+0x68 byte 0x57 and freshly reloaded E+0x6C four-byte zero. |
| /wave, five units | Same comparison and store order, byte value 0xA1. |
| /pet, four units | Same comparison and store order, byte value 0xA2. |
| /dance, six units | Same comparison and store order, byte value 0xA3. |
| Any other token | Constructs the pending identifier/text record described below. |

For each of the four actor-command cases, the original first obtains E, calls 0x4F220 on that receiver, converts the returned signed four-byte integer to binary32, and compares that result with the binary32 E+0x6C of that captured E. For ordinary finite operands, E+0x6C must be strictly greater. Equality, lesser values and unordered comparison flags skip the stores. If admitted, the stores use successive fresh W/E loads from G. Recognizing a command whose additional guard fails does not fall through to ordinary-message enqueueing. These operand facts do not assign a gameplay meaning to 0x4F220 or the action byte values.

The ordinary-message branch captures the eight-byte E+0x8 identifier through the then-current G/W/E, then copies the full draft string from a fresh current C, rather than enqueuing only the first token. It calls 0xA1610 on G+0x21A0, then destroys the temporary identifier/string record through 0x3B0B0. Original node constructor 0x36D30 allocates through 0x3C230, places identifier at node+0x10 and a separately owned UTF-16 string record at node+0x18. The list append constructs the node before testing its eight-byte count limit, then increments count and links the node at the tail. The selected maximum-count failure calls the original length-error boundary; this is not a transactional or allocation-failure guarantee.

G+0x21A0 is proved here as a pending identifier/text collection later used to add displayed Chat rows. Its append alone is not an imported send call or a proof of network delivery. Another direct producer 0x3CAE0 appends the same kind of record to this Game collection. Resolved direct-call metadata also identifies 0xF4490 as another caller of 0xA1610; that owner's transitive production is outside this bounded question. Transport ingestion, distribution, sender authentication and remote delivery are not inferred from the shape of a text record.

The selected Game update block 0xA84D9–0xA8626 checks its eight-byte count once. If nonzero it repeatedly obtains the tail payload through complete getter 0x8A130; this getter returns the node at head+8, plus payload offset 0x10. There is no loop back to process the remaining queue in this selected block. It processes at most one tail record per traversal of the block, and finally removes that tail through complete 0xA0670. Since 0xA1610 appends at that same tail, these selected append/consume operations choose the most recently appended remaining record, not a first-arrival snapshot order.

For a signed-negative eight-byte identifier the block skips the sender-prefix path. For a nonnegative identifier it queries the lookup at current W+0x450 through complete 0xE720. A null first result also skips the prefix. A nonnull first result causes a second tail getter and a second current W lookup; the second result, without a new null check in this window, supplies the narrow name beginning at entity+0x958. The selected conversion boundary 0x349C30 supplies a UTF-16 name, the original appends the two-unit literal colon and space, and 0x26B670 receives that prefix with three color bytes 0, 255, 255. Lookup 0xE720 searches signed eight-byte identifiers and returns the stored entity pointer or zero. There is no stable entity snapshot across the two lookups proved by this block.

Regardless of prefix admission, it obtains the current tail text again, builds that text followed by one LF through 0x6E070, and adds it through 0x26B670 on the current C using color bytes 255, 255, 255. It then releases temporaries and removes the tail node. Tail removal unlinks before decrementing count, frees an out-of-line string when its capacity exceeds seven, resets the string length/capacity/terminator and frees the node. Failure, exceptions, producer interleaving or callback mutation between these operations do not acquire any additional protection from this prose. This selected block has no own Enter, page-root or typing-byte gate; its reachability within the large Game update and that update's other message producers are not generalized into a universal drain schedule.

The source also separates the Game's pending collection from C's displayed rows. Enter clearing the draft does not erase queued records or displayed rows. Displayed history is output history in these owners; no selected Up/Down input path recalls it into the draft.

## Displayed rows, drawing and cleanup

Complete log wrapper 0x26BF10 receives C, a UTF-16 string and three binary32 color components. It multiplies components by original binary32 255, truncates each conversion to a signed eight-byte integer and keeps its low byte for 0x26B670. The first and third multiplications precede conversion of the first; the second multiplication/conversion follows; conversion of the third is last. It does not clamp arbitrary component values to a presumed color range. Floating exceptions, NaNs and out-of-range conversion results remain bounded by the actual native conversion and floating environment, not an imported graphics convention.

Complete row producer 0x26B670 tokenizes the string in code-unit order. It uses signed low-four-byte length for its inclusive scan, separates on U+0020, LF U+000A and CR U+000D, preserves a one-unit delimiter token before the final terminator, and copies each nonempty intervening span. Its temporary token list is separate from the retained rows. It samples widget width through 0x32A1D0, subtracts binary32 30 and truncates the result to a signed four-byte width limit. It ensures at least one outer row even when the supplied text yields no token.

For metrics it obtains the parent/spatial matrix through the reached original matrix boundaries and current node components. It scans the pieces of the last retained row to obtain the carried binary32 width; a one-unit U+0020 piece contributes exactly 5, while other pieces contribute an ordered returned-position difference from 0x352110. It also records whether that row initially ends in a non-space piece. Each temporary token is then presented to the same metric boundary, including LF/CR tokens before their delimiter decision. An exact one-unit LF or CR appends a new empty row and resets carried width to zero. A space contributes signed integer 5 for the token-width path; another token uses truncation to signed four bytes of the returned-position difference, then conversion back to binary32. This token width is added to the carried binary32 width before comparison against the integer limit converted to binary32.

For ordered finite values a sum strictly below the limit appends the token to the current last row; the native comparison also takes this append branch on unordered flags. Otherwise a single carried allowance derived from the initially non-space-ending row can permit the append and is consumed. Without that allowance, the producer appends a new row, carries the token width, and appends the token there unless it compares exactly equal to a single U+0020. In that exact-space case the piece is omitted, while the carried token-width state follows the selected native path. Equality with the width limit follows the overflow/allowance route, not the ordinary less-than route. No guessed font width, available resource or general-purpose line-breaking convention is supplied.

Each retained piece owns its UTF-16 text and three color bytes; complete 0x26A470 constructs the temporary piece, 0x26A150 copies its text/color into a node, and 0x26B520 appends it to the last row. The outer-row constructor 0x26A250 transfers the supplied inner-list ownership, and 0x26B590 appends an outer row. On normal completion the producer sets C+0x15C to one, clears/releases temporary tokens and returns. It does not change C+0x1D8 or clear C+0x1B8. Its providers and storage helpers are call boundaries rather than source-proved font determinism.

Complete draw owner 0x26A930 first calls 0x329E00, a binary32 second-component getter through 0x32A110. It divides the returned height by exactly 18, truncates to signed four bytes, sign-extends to eight bytes, then uses an unsigned eight-byte comparison against C's row count. While the count exceeds that converted value it removes the front outer row, decrements count, clears its pieces and frees the inner head and row. For ordinary nonnegative finite geometry this is a height-derived row-capacity trim. Negative/nonfinite or overflowing conversion inputs are not promoted into a sensible positive row-capacity guarantee.

The draw then obtains the parent/spatial matrix and enters the manager's selected drawing boundary. It traverses outer rows and each row's pieces forward, using a starting signed row coordinate 19 and increments of 18; spaces contribute width 5 and other pieces use original metric returns. It submits each non-space piece through two original font draw calls with the selected color/shadow arguments. The draft itself is submitted through two draw calls even when the typing byte is zero. Exact pixels, transformed screen origin, font availability, imported rendering behavior and final geometry are not closed by these calls.

Only the additional caret branch requires C+0x1D8 nonzero and a nonzero low bit of the signed quotient of current M+0x194 divided by 500. That branch normalizes caret/selection, calls 0x352310 for the current caret, and submits the one-unit U+007C literal through two original font calls. The counter and its update are not an independent wall-clock guarantee. Complete manager update 0x349CF0 adds its four-byte input to M+0x194, stores that input at M+0x190, clears M+0x20 and enters original node update traversal. The selected Interface update marks C+0x15C whenever typing remains nonzero before that later manager update. Drawing may trim displayed rows, but neither drawing nor that marker clears typing or draft in the selected bodies. No selected Chat owner implements time-based row expiry or submission recall.

## Creation-name GUI route and callback boundaries

Original setup's selected windows establish an independent creation-name path. It creates caption descriptors through 0x33DA70 and complete constructor 0x367590, sets their +0x2F8 four-byte values to one, resolves the selected original font resource through descriptor virtual +8, and creates the named node “charactername” under the creation-page root I+0x58. I+0x78, equivalently G+0x190, retains that parent node. Another caption node is created beneath it, and a clone of the original GUI template obtained under the exact name “edit” is attached beneath the same parent. A named-descendant text write then supplies an empty UTF-16 string to “edit”. That text record is independent of C+0x1B8.

The template lookup and cloning are real original caller joins, not permission to assume the contents of unavailable GUI assets. No asset was read. Complete factory 0x33CB30 and original parser 0x343AD0 construct the native Edit prototype through 0x327440; its primary table is 0x4775C8, type field +0x50 is four-byte value two, and clone slot +0x140 selects complete 0x3274E0. Complete clone callers 0x3308E0 and 0x333890 invoke the current source widget's +0x140 virtual slot when it exists. The ordinary setup resolves and clones the source node named “edit”; this source-only evidence does not universally prove the resource provider returned a particular template, a filter, or even a nonnull node. The following concrete Edit behavior applies when that reached source/instance has the independently verified Edit table. It is not a guessed generic edit-control contract or a claim that unavailable assets are well formed.

The native Edit instance has its current caption descriptor at +0x1A8, optional validator receiver at +0x1B0, signed four-byte caret +0x1C4 and selection displacement +0x1C8. Factory 0x3274E0 copies the validator pointer from the source widget; the prototype constructor initializes it to zero, but a parsed source may have later state. Attachment 0x327AA0 accepts the node+0x58 descriptor only when its four-byte +0x70 type is three, otherwise retains null; it normalizes the indices and captures selected spatial position components. Its separate parent property store does not make C or the node at G+0x220 into the name editor. Caption text is selected through descriptor's signed four-byte +0xD4 into its array at +0x108 with 0x20-byte string records, whose code-unit length is eight bytes at record+0x10.

For this table, GUI keydown slot +0xC0 selects 0x327E20, keyup +0xC8 selects 0xE8A20 and character +0xD0 selects 0x327B40. These GUI calls receive the low 16 bits from the window route, independently of the Game route's later low-byte conversion. Complete character owner 0x327B40 returns immediately for null caption or unsigned two-byte input below 0x20. Otherwise it marks the widget dirty and builds a one-unit temporary. For a nonzero selection, a nonnull validator is first invoked with that one-unit string; a zero returned byte rejects without deleting the selection. An admitted selection is deleted through 0x327580, including its caption update, layout and event-0x14 callback effects. The character owner then copies the current selected caption string, normalizes current indices, and inserts the unit in the temporary at the current caret. It reloads the optional validator and, if present, calls it with the full proposed string. A zero result at this later stage rejects the new insertion; any earlier selection deletion/callback has already occurred. On acceptance it publishes text through 0x334C50 on the current attached node, increments caret, calls layout 0x328470, then dispatches registered event 0x14 through 0x329D60. This order differs from Chat's direct editor and is not one atomic replace operation.

Complete 0x327E20 also requires a nonnull current caption. It normalizes indices through 0x328DF0 and sets dirty. Enter 0x0D calls focus setter 0x349A90 on the manager with null, then calls layout; it does not directly call the character-confirmation owner. Left/Right have the same conditional decrement/increment and numeric key-0x10 selection conventions as Chat, using the Edit caret/displacement fields and current selected caption's signed low length; an edge key still reaches layout but does not clear selection. Delete and Backspace use the selected caption deletion helpers; Home/End use current GUI key membership and selected caption length. Unsupported keys return after initial normalization/dirty marking. The complete Edit key, character, deletion and clamp owners do not operate on C's draft, typing byte or displayed rows.

Complete focus setter 0x349A90 can call a registered loss callback before publishing the replacement focus. When the previous focus is nonnull and equals the new pointer it returns immediately. Otherwise a prior focus is marked dirty and event 0x13 is dispatched through its current node before M+0x170 is assigned the captured new pointer. A nonnull new widget with a node can then dispatch event 0x12 through that node's current inherited widget. The event gate tests node flags, a nonnull inherited widget, a current callback record, and either widget byte+0x1A0 nonzero or callback-record byte+8 zero. Complete 0x342560 supplies the same selected event gate for the loss callback. These callbacks can reenter before the following focus store or later Edit operand reads; no old focus, caption or node lifetime snapshot is acquired by the owner.

The reached Edit press owner 0x328120 is one concrete focus producer. With an attached caption it clears drag byte +0x1B8, queries current point/matrix providers, and distinguishes already-current focus from a different focus. The different-focus branch sets caret to the current caption's signed low length, displacement to its negative, then calls 0x349A90 to focus this Edit; it continues to normalization, layout and capture publication. It does not set its drag byte back to one on this branch. The already-focused branch invokes its point-index helper and selected numeric-0x10 handling, sets drag byte to one, then joins the same normalization/layout/capture continuation. Point metrics, layout arithmetic and capture-release helper internals are outside this Chat-ingress question. No lifetime revalidation after arbitrary focus callbacks is inferred.

The original creation button is separately registered: setup's selected window binds event three on the “Create Character” button to Game receiver G and target 0x9EA10; the “Select” button under I+0x18 similarly binds event three to 0x9F7C0. Complete 0xF8410 installs or replaces the signed-four-byte event entry and optionally recurses to child widgets. Complete 0xF8380 owns a 0x20-byte callback record, replaces any previous allocation, stores the captured receiver at +0x10, target at +0x18, and the callback admission byte at +8. Its table at 0x441BA8 selects the independently byte-matched three-instruction leaf at 0xF99A0. That leaf forwards the stored receiver to the stored target; it does not reload the process G global. This separates a callback's captured Game receiver from the window procedure's later current-Game global reload.

Game release 0x965E0 invokes complete Interface adapter 0x101120 after its own frontend prefix. For numeric button zero that adapter calls complete GUI release 0x3410E0. GUI release clears bit zero of four-byte M+0x1A8, obtains captured node M+0x168 or a hit-provider result from current root/cached coordinates, applies popup handling, invokes an enabled inherited widget's virtual +0x60, then rereads current node fields and resolves event three. A registered callback is admitted by the node flag and enabled/record-byte gate above. It does not require a proved press/release match in this event-three test. The owner then rereads capture/hit state, performs its separate mismatch virtual/event-0x0E route, and traverses admitted nodes for the final virtual notification. Each earlier callback can change the later operands. Imported hit/shape providers, arbitrary callback replacements and asset geometry remain unclosed; event names do not manufacture a successful click.

Confirmation 0x9EA10 first hides the creation root G+0x170 through its current selected integer, then collects current named-descendant “edit” text under G+0x190 via complete 0x333690/0x332990. These helpers follow original current-child traversal and caption selection, returning the first nonempty reached caption text under their selected conditions, with an empty return when no accepted text is found. It converts the returned UTF-16 range through 0x354C30 and its original conversion boundary, then copies narrow bytes including the terminator into the then-current E+0x958. No draft string from C is used. It publishes additional entity/storage/GUI state and calls selected refresh/save helpers; their non-Chat internals are outside this contract. The complete confirmation owner neither reconstructs C nor directly clears its typing byte, draft, indices or displayed rows. Its own narrow-byte copy has no independent bound test here; this document does not supply malformed text, validator or encoding guarantees.

## Frontend entry, exit and retained Chat ownership

Complete Game constructor 0x7EA80 stores the supplied W and M pointers, constructs embedded I through 0xF8690 on G+0x118, initializes the separate pending-message list at G+0x21A0 through 0x3C230, clears the 256 raw key bytes through the explicitly sized constructor call, and later calls Interface setup 0xF9AC0 once on that same I in the selected ordinary constructor. Complete I constructor stores G at I+0x5C0 and initializes its selected scalars/collections; it does not mean every byte of I or G was zeroed.

Setup's selected root windows create the unnamed I+0x108 parent under M+0x140 and the separate roots at I+0, +0x18, +0x58, +0x80 and +0xE8. The first is initially selected one, the other four initially zero. The selected caller joins identify I+0 as the title/frontend root, I+0x18 as the character-selection root and I+0x58 as the creation root. The other two remain identified by operands in this scope. These are siblings under the manager root, not five values stored in C.

Later in setup a 0x1F0-byte allocation and a new unnamed child node of I+0x108 are passed to 0x26A2E0; the returned pointer is published at I+0x280. Complete Chat constructor first constructs the widget base with M, that node and an empty UTF-16 string, then installs its concrete tables. It creates an empty outer history list and zero eight-byte count; initializes the draft to length zero, capacity seven and a two-byte zero terminator; clears byte +0x1D8; clears the eight bytes spanning caret and selection; obtains the resource for the exact 13-unit UTF-16 name “resource1.dat”; and sets bit 0x40 of the inherited four-byte +0x150 field. It does not perform whole-allocation zeroing.

Base construction 0x3669C0 stores M at object+8 and registers the object in M's +0x10 collection under its critical section at +0x90, retaining the returned registration node at object+0x10. Widget base construction separately registers the widget in M+0xD0 through 0x33BFE0, and attaches it through 0x334410/0x32D190 so node+0x70 and widget+0x178 refer to one another on the normal supplied-node path. This registration lifetime is independent of the selected integer used to show/hide a page. The selected setup separately publishes Q at I+0x540, equivalently G+0x658, after complete constructor 0x275A70. That constructor writes eight all-one bytes at Q+0x2DC, initializing its two signed four-byte fields at +0x2DC and +0x2E0 to minus one; this does not initialize or clear C. I constructor also explicitly initializes byte I+0x308, equivalently G+0x420, to zero.

The ordinary Interface update's selected late block calls 0x9AF00 on the current roots in this exact order: I+0, I+0x18, I+0x58, I+0x80, I+0xE8. The first nonzero result selects zero for the I+0x108 parent; only five zero results select one. It then writes that normalized byte through 0xA3A20 to I+0x108 and separately copies its selected condition to another current widget node. This is the actual reached producer of the G+0x220 keydown admission value. It is not a test that every ancestor is selected, that rendering occurred, or that the current player is named. For example, toggling manager root selection through key 0x73 does not itself change this child's selected integer.

This publication occurs in 0x103200 at 0x1066C7–0x106744. Its selected earlier typing marker is at 0x1033EF–0x103405, and the later call to 0x349CF0 is at 0x106BA7. A selected Game update window calls 0x103200 on embedded I at 0xAB5CE. No page callback is silently turned into an immediate execution of this later publication. Between a page-store callback and the next traversal of this update block, the retained G+0x220 selected value can still be the previously published value.

The selected frontend paths reuse G and embedded I. Complete release 0x965E0's prefix first tests selected G+0x118 and G+0x120 and numeric button zero. With a nonnegative current selection from G+0x128+0x1F0, value zero hides G+0x118 and shows character-selection G+0x130. Value three loads the stored slot G+0x2264 into current E through 0x9BB20, hides the title root, sets the selected camera fields and calls 0x2E0B50; values one and two have their separate selected option/exit publications. It then enters GUI release, so GUI callbacks observe the already-performed prefix. None of these direct page-prefix stores clears C's state. Remaining non-Chat gameplay work in that full owner is not assigned new meanings here.

Complete 0x9F7C0 hides character-selection G+0x130, locks the current W through the selected lock boundary and copies signed G+0x6C0 to G+0x2264. For a negative or out-of-range selection it shows creation G+0x170, resets/rebuilds the current E through the selected entity helpers, clears its name's first narrow byte, constructs/replaces its separate CharacterData, clears the creation-name caption through G+0x190 and enters selected style helpers. For an existing selection it calls 0x9BB20 on current E and updates stored selection/GUI state. The complete loader locks current W, takes G+0x1C98 around the selected backend read and decoding admission, and passes the supplied entity receiver to the reached character reader when that read succeeds. No Chat reset is implied by resetting/decoding a different entity. Complete 0x913B0 was also independently inspected as a selected entity rebuild boundary; its field stores and subordinate resets concern current E and selected Game/entity helpers, not a direct reconstruction of I or C. Their transitive serialization, random, backend and entity-container closures are not imported into this contract.

Complete return-to-title publisher 0x9FDF0 first calls selected save boundary 0xA27B0 and disconnect 0x911E0, reloads stored character entries through 0x9BB20, then selects title root G+0x118, clears G+0x420 and G+0x668 bytes, and hides its listed current widget nodes. It does not call the Game/Interface/Chat constructors or directly clear Chat typing, draft, caret, selection, rows or pending records. An independently matched native menu-dispatch window at 0x292300 routes its selected value two to this Game receiver's 0x9FDF0. That window has no decoded complete owning export in this evidence; its whole surrounding owner, alternative routes and resource identity are not claimed. The name “Continue” here identifies the title selection value-three load path; 0x9FDF0 is identified by its proved stores that reselect title.

Complete disconnect 0x911E0 first stores four-byte G+0x2260 zero. Only a nonzero G+0x21E8 active byte enters its stop path: it clears that byte, conditionally closes/zeros its socket, performs selected thread-id/join gates, acquires current W's selected lock, writes W+0x358 byte zero, and adds exact UTF-16 “Disconnected.” followed by LF to the current C through 0x26BF10 before releasing that lock. Join errors and current-thread checks reach original error boundaries. This adds output history; it is not a direct Chat draft/typing/history reset. It does not close an unproved Steam-peer transport by implication.

These selected constructors, callbacks and frontend stores establish retention of the same C allocation and independent data records on their direct ordinary paths. They do not establish a universal guarantee that arbitrary indirect callbacks, resource providers, pointer aliases, a different Game instance, exceptions or other uninspected callers cannot mutate or replace them. Hidden-page selection alone has no direct Chat reset effect in the complete selected transition bodies. Complete keyboard routing can therefore observe retained typing state while a frontend page is selected, subject to the distinct current-focus guard; it need not admit keydown editing at the same time.

## Pointer cancellation, destruction and lifetime limits

Complete Game press owner 0x94900 calls Interface press adapter 0x101020 first. The adapter copies current manager cached-Y's four-byte word from M+0x184 to I+0x548, equivalently G+0x660, before its selected GUI button dispatch. Game press then samples a separate selected widget condition into process byte 0x5514D7. Only afterward does it test G+0x668 and, when nonzero, call point-query wrapper 0x333E70 first on current G+0x3A8 and then, only after a zero result, on current G+0x3B0. A nonzero result from either query returns through the epilogue without reaching Chat cancellation. The wrapper copies the current manager's two cached coordinate words and invokes original hit provider 0x333F60 on the supplied node. Native operands, not the C presentation's lost first argument, establish the node receiver. This contract supplies no unavailable shape/asset rectangle or business name for those nodes.

The complete native press owner has those two normal early-return branches to 0x965AC. Its other normal returning paths reach the postlude at 0x9651F before that epilogue. The many intervening gameplay, item, random and provider operations are not expanded here. The selected postlude performs its reached current-entity health helper/rescale/clamp operations before reloading current C and storing C+0x1D8 byte zero at 0x96588. It then clears the current selected integer of the node in G+0x310. It does not clear C's draft, caret, selection or rows. This is a pointer-press cancellation of typing on completed postlude paths, not an Escape cancellation, not an unconditional effect of every delivered press and not a guarantee after failure or nonreturning providers. GUI callbacks already ran before the early-return tests or this postlude and can have changed C, focus or other current receivers.

The ordinary frontend transitions inspected here do not reach Game destruction merely because a page or stored character changes. On actual selected shutdown, application window 0x137B98–0x137CA0 invokes the current Game's deleting slot before the subsequent selected World/provider objects and, later, the process GUI manager's deleting slot. Game primary table 0x436068 selects complete 0x86630; this calls complete 0x818D0 and frees the allocation only when deleting flags' low bit is set. The complete Game destructor first performs its selected object/disconnect work, clears the worker-active byte and completes its reached thread-stop/join sequence, then destroys its owned collections and embedded members. No general guarantee that all external message producers or arbitrary callbacks have stopped follows from the thread log strings or imported joins.

For the pending-message collection, Game teardown calls complete 0x8ABD0 on G+0x21A0 before freeing its head. That owner resets head links and the eight-byte count, traverses the previous nodes, releases each out-of-line UTF-16 payload when capacity exceeds seven, resets capacity/length/terminator and frees each node. This is independent of C's row history. Complete Interface destructor 0x82340 and its selected subordinate 0x82270 release their selected bookkeeping collections and storage. Neither complete Interface body contains a direct Chat-specific typing/draft reset or a direct call to Chat's deleting target; generic subordinate release behavior outside the inspected closures is not replaced by a guessed ownership model.

M's original concrete table 0x480408 selects complete deleting owner 0x38E700, which calls 0x38E420 and conditionally frees M. The derived teardown releases its selected imported/provider objects and lists, then enters complete base teardown 0x33A440. The reached final registration teardown 0x33A860 marks manager byte+8 nonzero, traverses its M+0x10 registered-object list and invokes each remaining nonnull object's deleting slot, then resets and frees that list. A still-registered ordinary C has the concrete Chat deleting target 0x26A600. This is an established registered-object path, not an assertion that an arbitrary object at an unrelated deleting slot is Chat.

Complete 0x26A600 calls 0x26A580, then frees C only when the low deleting flag bit is set. Complete 0x26A580 releases an out-of-line draft, resets its UTF-16 length/capacity/terminator, clears all outer history through 0x26A840, frees the outer head and calls complete widget-base cleanup 0x329500. Outer cleanup resets count/links and clears each row's inner pieces through complete 0x26A730 before freeing its inner head and row. Piece cleanup releases owned out-of-line text, resets that record and frees its node. These are actual owned containers, not whole-allocation zeroing.

Widget-base cleanup frees its selected callback allocations and strings and reaches registration-base 0x366A10. The latter unlinks/frees the object's retained M+0x10 registration node and decrements that list's count only when manager byte+8 is zero. It skips that unlink when the manager's registration teardown has set the byte, allowing that traversal to retain its own link ownership. This states the selected operation order, not a general concurrent-deletion safety proof. There is no newly inferred lifetime guarantee for a stale focused widget, captured node, process global, replaced callback record or reader interleaving with teardown. The selected application shutdown windows do not null the process G/M globals, and the selected keyboard window cases have no null guard; arbitrary later window dispatch is therefore outside the proved ordinary lifetime preconditions.

## Independent conditional source sequences

These examples use independently chosen current field values and successful normal provider returns. They are not captures, executable observations or assertions about an OS-generated message sequence. Unless explicitly changed in an example, callbacks/providers are assumed to leave the stated current receivers and gates intact.

* With the current G+0x220 selected integer one, M focus null, Q+0x2E0 equal to minus one and typing zero, a delivered keydown 0x0D opens typing without clearing an existing draft. A delivered character 0x0D then normalizes indices but adds no unit. A second delivered keydown 0x0D closes typing, handles/queues the nonempty current draft, clears it and normalizes through the Chat key owner. A third such down opens typing again. There is no original repeat suppression in this chain.
* With typing nonzero and focus null, character input word 0x00E9 reaches the Game Chat insertion as U+FFE9. Word 0x0080 reaches U+FF80. Word 0x0100 reaches unit zero and is rejected after normalization. GUI's preceding two-byte argument remains respectively 0x00E9, 0x0080 or 0x0100; its own current focused target may have already performed different work.
* With typing nonzero, null focus and current HUD-parent selected integer zero, a character for byte 0x41 can still insert U+0041. A keydown Backspace in the same field state only performs the raw/pre-Chat work and fails at the first Chat gate. The character and keydown owners intentionally have different reached operands; a hidden parent alone is not a character guard.
* With typing nonzero, parent selected one, null focus and Q+0x2E0 zero, keydown editing is blocked by the signed Q gate, while a character can still be admitted. With a nonnull M focus, the Game character route is blocked regardless of that focused widget's enabled byte. The earlier GUI phase separately tests its enabled byte and may edit that widget's caption.
* With an enabled native Edit focused and a valid attached caption, GUI character delivery uses the full low 16 bits and its optional validator. The following Game character call sees the then-current nonnull focus and returns. For an Enter keydown, the Edit's GUI key owner can first clear focus and invoke a loss callback; the later current Game key owner evaluates its own current parent/Q/typing gates. It does not inherit a consumed result or a fixed snapshot of the former page state.
* With typing nonzero and all three Chat keydown gates passing, key 0x1B toggles G+0x420 and marks/normalizes the editor without clearing the draft or typing byte. A completed ordinary pointer postlude instead clears the typing byte and preserves the draft. If G+0x668 is nonzero and the first selected point query returns nonzero, that postlude is bypassed; a second node is not queried and no postlude typing clear occurs.
* A selected page store followed by a later Interface update is two ordered operations. The title-to-selection stores or creation confirmation do not themselves execute the late five-root HUD publication. No universal wall-clock ordering against a delivered message between those operations is proved. Once that publication selects zero, keydown tests that current selected element; the character route still lacks it.
* If pending record A is appended and then B, with no intervening removal and successful selected update/log providers, the next traversal of 0xA84D9–0xA8626 chooses B's tail payload and removes B after its log calls. The block does not loop to A; a later traversal can choose A. A page/root change alone does not clear this independent collection in the selected transition bodies. This gives no transport arrival-order or delivery guarantee.

## Writer/caller census and explicit exclusions

The direct-call metadata was freshly inspected for the selected constructors, direct editor entry points and message producers. It identifies the ordinary 0x1355A0 call to Game construction at 0x136331, Game's I constructor/setup calls at 0x7EB1E/0x7F4D4, and setup's Chat constructor call at 0xFFE0D. The only resolved direct callers of 0x26B170 and 0x26B300 in that metadata are respectively Game character 0x9F670 at 0x9F6A1 and Game keydown 0x96E70 at 0x977B5. That direct-call observation is not a no-other-indirect-call proof; Chat's inspected GUI virtual slots are separately proved no-effect targets.

The four identity-resolved selected stores to C's typing byte are constructor zero at 0x26A3D5, keydown submission zero at 0x970CC, keydown opening one at 0x977C2 and pointer-postlude zero at 0x96588. The wider original listing contains other literal stores at an offset +0x1D8 on other receiver registers/owners. They were not treated as aliases of C without a concrete receiver join. Computed addresses, bulk stores, uninspected callers and arbitrary provider mutation prevent a universal writer census. Similarly, the process G/M stores identified by literal RIP-relative references establish selected publication sites, not a proof against every indirect write.

Direct log-wrapper callers also exist in original packet dispatch, other Game/UI owners and entity/item messaging. Complete original caller bodies not selected here were not expanded to import transport or unrelated gameplay semantics. The independently read 0x913B0 rebuild owner has a resolved direct caller in 0x272D90 at 0x27317D; this contract does not close that full style owner or its provider/admission closure. Existing-character reader 0x147800, entity reset/default owners, backend serialization, peer/socket providers, generic allocation/string runtime, matrix/font/shape resource providers, the detailed layout/point-index helpers 0x328470/0x328B30, and full 0x101BD0 layout remain explicit boundaries. Their names do not establish hidden reset, encoding, delivery, numerical or safety guarantees.

No executable was run. No font/asset/configuration file was read. There is no supplied English localization for an unresolved token, no OS key-enumeration inference, no assumed rectangle or screen origin, no guessed import/transport implementation, and no claim that callbacks cannot reenter. Ordinary return assumptions, valid current receivers and complete well-formed strings/collections are required for the stated normal paths. Partial records, allocation/stream/backend failures, invalid pointer/index inputs, floating environment, exception unwinding beyond selected bodies, arbitrary alternate callers, process scheduling and concurrent mutation remain bounded rather than repaired by speculation.

## Evidence manifest and correspondence limits

All paths in this manifest belong to the original reference directory given above. Metadata files were independently read for this assignment; their integrity identifiers are:

| Original evidence file | Bytes | SHA256 |
| --- | ---: | --- |
| decompiled/assembly.txt | 62,547,291 | FE07326A0F4585432A9D4E42E7CF16C570815A8FCD164DE36616095023A0740C |
| decompiled/function_ranges.csv | 509,127 | BC127412C5A8F21D8BA78AA53D9EB35A16AF58C1A85906DA9FF31E968E379601 |
| decompiled/calls.csv | 10,840,369 | CF410A7DD84BBEF0E9FC4FC6FD1FF3B4903D97E9126C8F1A1D69EF05CD566DCB |
| decompiled/functions.csv | 1,551,545 | 940E3682B9CCC68458531AC8D948A20F8B9530CDFFF5E568C5517BA91365DCEC |

For each complete-owner row, the individually selected C export is decompiled/functions/140 followed by the six-digit zero-padded lowercase RVA and .c. These selected owners were read completely. The listed inclusive RVA extents are the original exported owning ranges; multiple extents are intentionally preserved. Every listed instruction interval was mapped through the original PE sections and compared against original file bytes, rather than assuming C presentation describes a native operand. Range holes are not claimed as native owner instructions. Counts below are instruction count / matched instruction bytes, not C text length. Complete owner coverage does not recursively close imported or otherwise bounded callees and does not mean all unrelated gameplay effects are described in this Chat-focused contract.

| Complete owner RVA | Inclusive original owning ranges | Instructions / bytes |
| --- | --- | ---: |
| 0xE720 | 0xE720–0xE768 | 24 / 73 |
| 0x36620 | 0x36620–0x3668C | 31 / 109 |
| 0x36D30 | 0x36D30–0x36DAF | 35 / 128 |
| 0x3B0B0 | 0x3B0B0–0x3B0FB | 24 / 76 |
| 0x3C230 | 0x3C230–0x3C267; 0x3C269–0x3C28D | 26 / 93 |
| 0x3CAE0 | 0x3CAE0–0x3CB4A | 28 / 107 |
| 0x3CB50 | 0x3CB50–0x3CC3E | 53 / 239 |
| 0x4CE70 | 0x4CE70–0x4CE7D | 5 / 14 |
| 0x6E070 | 0x6E070–0x6E142 | 56 / 211 |
| 0x7EA80 | 0x7EA80–0x7F35D; 0x7F35F–0x7F36D; 0x7F36F–0x7F37D; 0x7F37F–0x7F38A; 0x7F38C–0x7F9DF | 806 / 3932 |
| 0x818D0 | 0x818D0–0x82267 | 592 / 2456 |
| 0x82270 | 0x82270–0x8233F | 57 / 208 |
| 0x82340 | 0x82340–0x82444 | 58 / 261 |
| 0x86630 | 0x86630–0x86663 | 16 / 52 |
| 0x8A130 | 0x8A130–0x8A13B | 4 / 12 |
| 0x8ABD0 | 0x8ABD0–0x8ACB6; 0x8ACB8–0x8ACBD; 0x8ACBF–0x8ACC4; 0x8ACC6–0x8ACCB; 0x8ACCD–0x8ACD2 | 68 / 255 |
| 0x911E0 | 0x911E0–0x913A4 | 92 / 453 |
| 0x913B0 | 0x913B0–0x91C35 | 428 / 2182 |
| 0x94900 | 0x94900–0x965DF | 1583 / 7392 |
| 0x965E0 | 0x965E0–0x96832; 0x96834–0x96842; 0x96844–0x96852; 0x96854–0x9685F; 0x96861–0x96E6F | 508 / 2188 |
| 0x96E70 | 0x96E70–0x977F2 | 550 / 2435 |
| 0x97870 | 0x97870–0x9787B | 3 / 12 |
| 0x9AEB0 | 0x9AEB0–0x9AEF1 | 21 / 66 |
| 0x9AF00 | 0x9AF00–0x9AF19 | 6 / 26 |
| 0x9BB20 | 0x9BB20–0x9BC7A; 0x9BC7C–0x9BC8A; 0x9BC8C–0x9BC9A; 0x9BC9C–0x9BCA7; 0x9BCA9–0x9BDF1 | 160 / 718 |
| 0x9EA10 | 0x9EA10–0x9EC72; 0x9EC74–0x9EC82; 0x9EC84–0x9EC92; 0x9EC94–0x9EC9F; 0x9ECA1–0x9ED5E; 0x9ED60–0x9ED6E; 0x9ED70–0x9ED7E; 0x9ED80–0x9ED8B; 0x9ED8D–0x9EFED; 0x9EFEF–0x9EFFD; 0x9EFFF–0x9F00D; 0x9F00F–0x9F01A; 0x9F01C–0x9F050 | 377 / 1589 |
| 0x9F670 | 0x9F670–0x9F6B0 | 18 / 65 |
| 0x9F7C0 | 0x9F7C0–0x9F8E8; 0x9F8EA–0x9F8F8; 0x9F8FA–0x9F908; 0x9F90A–0x9F915; 0x9F917–0x9FD02 | 276 / 1343 |
| 0x9FDF0 | 0x9FDF0–0x9FF98 | 84 / 425 |
| 0xA0670 | 0xA0670–0xA06E2 | 32 / 115 |
| 0xA1610 | 0xA1610–0xA1673 | 27 / 100 |
| 0xA27B0 | 0xA27B0–0xA2ADC; 0xA2ADE–0xA2AEC; 0xA2AEE–0xA2AFC; 0xA2AFE–0xA2B09; 0xA2B0B–0xA2BD6 | 240 / 1059 |
| 0xA3A20 | 0xA3A20–0xA3A3F | 8 / 32 |
| 0xE8A20 | 0xE8A20–0xE8A22 | 1 / 3 |
| 0xF8380 | 0xF8380–0xF8401 | 35 / 130 |
| 0xF8410 | 0xF8410–0xF858D | 99 / 382 |
| 0xF8690 | 0xF8690–0xF87C9 | 67 / 314 |
| 0x101020 | 0x101020–0x10111E | 58 / 255 |
| 0x101120 | 0x101120–0x10115E | 16 / 63 |
| 0x1014F0 | 0x1014F0–0x10178B | 119 / 668 |
| 0x107440 | 0x107440–0x10768B | 146 / 588 |
| 0x107690 | 0x107690–0x1081A9 | 681 / 2842 |
| 0x133C70 | 0x133C70–0x133EEB | 174 / 636 |
| 0x26A150 | 0x26A150–0x26A1D4 | 36 / 133 |
| 0x26A1E0 | 0x26A1E0–0x26A243 | 28 / 100 |
| 0x26A250 | 0x26A250–0x26A2D0 | 35 / 129 |
| 0x26A2E0 | 0x26A2E0–0x26A466 | 91 / 391 |
| 0x26A470 | 0x26A470–0x26A530 | 54 / 193 |
| 0x26A580 | 0x26A580–0x26A5F0 | 28 / 113 |
| 0x26A600 | 0x26A600–0x26A633 | 16 / 52 |
| 0x26A640 | 0x26A640–0x26A677; 0x26A679–0x26A69D | 26 / 93 |
| 0x26A6A0 | 0x26A6A0–0x26A6D7; 0x26A6D9–0x26A6FD | 26 / 93 |
| 0x26A700 | 0x26A700–0x26A716 | 8 / 23 |
| 0x26A730 | 0x26A730–0x26A816; 0x26A818–0x26A81D; 0x26A81F–0x26A824; 0x26A826–0x26A82B; 0x26A82D–0x26A832 | 68 / 255 |
| 0x26A840 | 0x26A840–0x26A8BA | 31 / 123 |
| 0x26A8C0 | 0x26A8C0–0x26A924 | 27 / 101 |
| 0x26A930 | 0x26A930–0x26B16A | 447 / 2107 |
| 0x26B170 | 0x26B170–0x26B2FD | 100 / 398 |
| 0x26B300 | 0x26B300–0x26B516 | 143 / 535 |
| 0x26B520 | 0x26B520–0x26B583 | 27 / 100 |
| 0x26B590 | 0x26B590–0x26B5F3 | 27 / 100 |
| 0x26B600 | 0x26B600–0x26B665 | 23 / 102 |
| 0x26B670 | 0x26B670–0x26BF0C | 563 / 2205 |
| 0x26BF10 | 0x26BF10–0x26BF62 | 18 / 83 |
| 0x275A70 | 0x275A70–0x2761AC | 425 / 1853 |
| 0x327440 | 0x327440–0x327489 | 17 / 74 |
| 0x327490 | 0x327490–0x3274D8 | 20 / 73 |
| 0x3274E0 | 0x3274E0–0x327572 | 34 / 147 |
| 0x327580 | 0x327580–0x32762F | 44 / 176 |
| 0x327AA0 | 0x327AA0–0x327B38 | 36 / 153 |
| 0x327B40 | 0x327B40–0x327E1A | 177 / 731 |
| 0x327E20 | 0x327E20–0x3280B2 | 158 / 659 |
| 0x328120 | 0x328120–0x328316 | 107 / 503 |
| 0x328DF0 | 0x328DF0–0x328EB1 | 42 / 194 |
| 0x328F00 | 0x328F00–0x328F6F | 31 / 112 |
| 0x328FF0 | 0x328FF0–0x329206 | 96 / 535 |
| 0x329210 | 0x329210–0x329464 | 126 / 597 |
| 0x329500 | 0x329500–0x329621; 0x329623–0x329638; 0x32963A–0x329648; 0x32964A–0x329658; 0x32965A–0x329665; 0x329667–0x3296A9; 0x3296AB–0x3296C0; 0x3296C2–0x3296D0; 0x3296D2–0x3296E0; 0x3296E2–0x3296ED; 0x3296EF–0x32972C | 143 / 547 |
| 0x329D60 | 0x329D60–0x329D7F | 10 / 32 |
| 0x329E00 | 0x329E00–0x329E33 | 12 / 52 |
| 0x32A210 | 0x32A210–0x32A2B4 | 46 / 165 |
| 0x32D190 | 0x32D190–0x32D1A0 | 5 / 17 |
| 0x32EE60 | 0x32EE60–0x32F051 | 114 / 498 |
| 0x3308E0 | 0x3308E0–0x330A77 | 115 / 408 |
| 0x332990 | 0x332990–0x332AC6 | 79 / 311 |
| 0x333690 | 0x333690–0x33381D | 106 / 398 |
| 0x333890 | 0x333890–0x3339BC | 75 / 301 |
| 0x333E70 | 0x333E70–0x333EC7 | 19 / 88 |
| 0x334410 | 0x334410–0x334506 | 60 / 247 |
| 0x3348E0 | 0x3348E0–0x334A12 | 82 / 307 |
| 0x334C50 | 0x334C50–0x334D24 | 58 / 213 |
| 0x3358E0 | 0x3358E0–0x335919 | 18 / 58 |
| 0x335920 | 0x335920–0x335BA1 | 168 / 642 |
| 0x3380E0 | 0x3380E0–0x338236 | 99 / 343 |
| 0x339B70 | 0x339B70–0x33A112 | 323 / 1443 |
| 0x33A440 | 0x33A440–0x33A7B8 | 219 / 889 |
| 0x33A860 | 0x33A860–0x33A8E6 | 38 / 135 |
| 0x33BCF0 | 0x33BCF0–0x33BD9E | 42 / 175 |
| 0x33BFE0 | 0x33BFE0–0x33C09D | 42 / 190 |
| 0x33C360 | 0x33C360–0x33C406 | 49 / 167 |
| 0x33CB30 | 0x33CB30–0x33CB8B | 26 / 92 |
| 0x33CD30 | 0x33CD30–0x33CED2 | 117 / 419 |
| 0x33DA70 | 0x33DA70–0x33DACB | 26 / 92 |
| 0x33DFA0 | 0x33DFA0–0x33DFB6 | 8 / 23 |
| 0x33F010 | 0x33F010–0x33F0DC | 59 / 205 |
| 0x340D60 | 0x340D60–0x340DC8 | 29 / 105 |
| 0x340DD0 | 0x340DD0–0x340DEF | 8 / 32 |
| 0x340DF0 | 0x340DF0–0x340E56 | 26 / 103 |
| 0x340E60 | 0x340E60–0x340EB7 | 22 / 88 |
| 0x340EC0 | 0x340EC0–0x3410D8 | 138 / 537 |
| 0x3410E0 | 0x3410E0–0x3412F4 | 141 / 533 |
| 0x341300 | 0x341300–0x341494 | 103 / 405 |
| 0x3415F0 | 0x3415F0–0x341776 | 102 / 391 |
| 0x342090 | 0x342090–0x342097 | 2 / 8 |
| 0x342270 | 0x342270–0x342556 | 161 / 743 |
| 0x342560 | 0x342560–0x3425BF | 29 / 96 |
| 0x343AD0 | 0x343AD0–0x343C6D | 107 / 414 |
| 0x349A90 | 0x349A90–0x349B33 | 45 / 164 |
| 0x349CF0 | 0x349CF0–0x349D0E | 5 / 31 |
| 0x354C30 | 0x354C30–0x354CE4 | 50 / 181 |
| 0x3669C0 | 0x3669C0–0x366A01 | 18 / 66 |
| 0x366A10 | 0x366A10–0x366A47 | 15 / 56 |
| 0x367590 | 0x367590–0x367667; 0x367669–0x367677; 0x367679–0x367687; 0x367689–0x367694; 0x367696–0x367743; 0x367745–0x367753; 0x367755–0x367763; 0x367765–0x367770; 0x367772–0x367818; 0x36781A–0x367828; 0x36782A–0x367838; 0x36783A–0x367845; 0x367847–0x3678ED; 0x3678EF–0x3678FD; 0x3678FF–0x36790D; 0x36790F–0x36791A; 0x36791C–0x3679BE | 264 / 1055 |
| 0x368020 | 0x368020–0x368181 | 67 / 354 |
| 0x368B90 | 0x368B90–0x368C54 | 45 / 197 |
| 0x38E100 | 0x38E100–0x38E41B | 128 / 796 |
| 0x38E420 | 0x38E420–0x38E6F3 | 202 / 724 |
| 0x38E700 | 0x38E700–0x38E733 | 16 / 52 |

The following are separately bounded original caller/provenance windows. The named containing owners are 0xF9AC0 for setup, 0x103200 for Interface update, 0xA5A50 for Game update, 0x134CC0 for manager publication and 0x1355A0 for Game publication/shutdown. The native menu-dispatch window at 0x292300 and stored callback leaf at 0xF99A0 have no complete decoded owning C export selected here. The latter leaf's complete three-instruction transfer ends at 0xF99AA; it is verified independently of a neighboring export. The former remains a bounded join only, with no invented surrounding owner. The large setup/update/startup routines are not promoted to full semantic coverage by these rows. Navigation-only original C exports 0x101BD0, 0x328470 and 0x328B30 were selected/loaded as helper leads; their unclosed internals remain explicit boundaries rather than complete-owner claims.

| Bounded native window | Actual inclusive matched extents | Instructions / bytes |
| --- | --- | ---: |
| setup receiver | 0xF9AC0–0xF9B27 | 19 / 104 |
| setup roots | 0xF9C80–0xFA102 | 203 / 1155 |
| edit template lookup | 0xFC721–0xFC811 | 54 / 241 |
| Controls publication | 0xFC9A6–0xFCA95 | 47 / 240 |
| selected callback registrations | 0xFCB55–0xFCD33 | 102 / 479 |
| creation name setup | 0xFD7C3–0xFDBB5 | 203 / 1011 |
| Chat allocation publication | 0xFFD71–0xFFE57 | 49 / 231 |
| typing dirty marker | 0x1033EF–0x103405 | 4 / 23 |
| HUD publication | 0x1066C7–0x106744 | 32 / 126 |
| manager update join | 0x106B70–0x106BB6 | 14 / 71 |
| pending log consumption | 0xA84D9–0xA8626 | 70 / 334 |
| Interface update caller | 0xAB5B6–0xAB5E6 | 10 / 49 |
| manager publication | 0x1350F0–0x135131 | 14 / 66 |
| Game publication | 0x1362E4–0x136344 | 24 / 97 |
| ordinary shutdown | 0x137B98–0x137CA0 | 59 / 265 |
| native menu callback join | 0x292300–0x292377 | 25 / 120 |
| stored callback thunk | 0xF99A0–0xF99AA | 3 / 11 |
| Game update receiver | 0xA5A50–0xA5AF2 | 28 / 163 |
| Interface update receiver | 0x103200–0x10328D | 29 / 142 |
| Interface address in Game update | 0xAAF6C–0xAAF79 | 2 / 14 |

The selected data correspondence includes the 0x96E70 dispatch tables at 0x977F4–0x9786E: eight four-byte RVA targets and 91 one-byte selectors. They select only 0x1B and 0x70–0x75 as special down-key cases; the other in-range selectors and keys outside the admitted range reach the common gate. Native table data, not OS F-key labels, establishes this mapping.

Selected eight-byte virtual/callback entries were read directly from original image data: Game table 0x436068 slot zero to 0x86630; manager table 0x480408 slot zero to 0x38E700; Chat table 0x46D1E8 slots zero/+8 to 0x26A600/0x26A930 and +0xC0/+0xC8/+0xD0 to 0xE8A20; Edit table 0x4775C8 slots zero/+0x58/+0xC0/+0xC8/+0xD0/+0x140 to 0x327490/0x328120/0x327E20/0xE8A20/0x327B40/0x3274E0; caption table 0x47B130 slot +8 to 0x368B90; callback table 0x441BA8 slot zero to the 0xF99A0 transfer leaf. No adjacent data was interpreted as an additional virtual slot once its table extent or receiver identity was unproved.

The original literal inventory needed for the selected Chat command/body assembly was checked against image operands and two-byte code units. These are exact UTF-16 unit lengths excluding terminators:

| Literal RVA | Exact literal or unit description | Units |
| --- | --- | ---: |
| 0x4361E0 | /check | 6 |
| 0x436200 | The exact 31-unit prefix quoted in the command table, ending in U+0020 | 31 |
| 0x436240 | /namepet | 8 |
| 0x436258 | Name is too long. followed by LF | 18 |
| 0x436280 | /sit | 4 |
| 0x436290 | /wave | 5 |
| 0x4362A0 | /pet | 4 |
| 0x4362B0 | /dance | 6 |
| 0x4361F0 | U+000A | 1 |
| 0x436550 | U+003A, U+0020 | 2 |
| 0x437E68 | Disconnected. followed by LF | 14 |
| 0x436340 | resource1.dat | 13 |
| 0x437B38 | edit | 4 |
| 0x4410F0 | Select | 6 |
| 0x441128 | Create Character | 16 |
| 0x4411A0 | Name | 4 |
| 0x441230 | charactername | 13 |
| 0x46D358 | U+007C | 1 |
| 0x46D35C | U+000D | 1 |
| 0x43760C | U+0020 | 1 |

The empty UTF-16 literal operand is at 0x42FF38. Caption setup additionally uses the original exact names “charactername”, “Name”, “Select” and “Create Character” in the bounded native/C setup windows. Their semantic use is established by the named-node, caption-write and callback joins, not by a generic screen convention. The supplied first-token tests do not import any additional localized command inventory.

Relevant original binary32 constants were independently read with their four-byte words: RVA 0x437FE0 is exactly 18, 0x42E584 exactly 5, 0x42E580 exactly 3, 0x42E59C exactly 12, 0x42E5A8 exactly 30 and 0x42E7D8 exactly 255. The key-prework constants and their words are stated with that owner above. Raw float operations and comparison flags are distinguished from imported font/resource numerical results. Signed-low-width string scans, native integer wrap/indefinite conversion cases and unordered flags do not imply a repaired input model.

## Exposure and hold boundary

Exposure remained source-only: the standalone author policy, current source-side assignment, filename-selected original C/native metadata and the original supported executable. No active AGENTS.md, WORKSPACE-MEMORY.md, combined policy/history, earlier held contract, replacement engine/exporter, tests/results, owner files/captures/outputs, assets/configuration, SDK/provider/dependency implementation, packages or reconstruction source was read. No extra agent was spawned, no original executable was run, and no implementation or runtime feedback was requested or received. Earlier held artifacts were left unread and unchanged.

This is the complete bounded prose handoff. Its release as FINAL / STABLE / HELD is contingent on the author's separate whole-saved-file UTF-8/no-BOM, byte-length and SHA256 readback report and fresh original identity check. Once that report is made, this artifact is immutable and no further writes are authorized within this handoff.
