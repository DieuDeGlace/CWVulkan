# Steam GUI keyboard, editing and creation dispatch

Status: FINAL / STABLE / HELD after complete saved-file readback. This independent source-side contract is dated 2026-11-09. It contains behavioral prose and evidence limits, not implementation guidance.

## Authority, identity and scope

The authority image is the original-side file C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe: 5,757,952 bytes, AMD64 machine 0x8664, image base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. All addresses below are RVAs in that image unless explicitly identified as asset file offsets. An address names an original owner or operand, not a replacement component.

This audit independently followed the complete window procedure 0x133C70, GUI key-down/up/character owners 0x340DF0, 0x340E60 and 0x340DD0, the concrete Edit virtual targets, the relevant setup and connection owners, interface polling, and confirmation/conversion 0x9EA10/0x354C30. The native ranges in the manifest were compared with the raw image; original exported C was used to locate relationships, and claim-bearing native branches and widths were checked directly. Supplementary application and renderer contexts are separately bounded. Complete native-byte comparison does not make every decompiler expression an independent authority.

The symbols G, M, F, T and E below mean the ordinary Game object, its GUI manager, a GUI frame/node, a type-3 caption object and a concrete plasma::Edit widget. They are local labels for pointers. In the ordinary construction at 0x7EA80, the interface context is embedded at G+0x118; its +0x5C0 stores G, and G+0x20 is the manager used by its bridges. In the window procedure the global Game and manager pointers are read at image RVAs 0x551A80 and 0x551AC0. Supported startup passes the same manager into the concrete renderer. This does not establish identities for arbitrary other callers.

## The stored objects and creation binding

The creation-name composite is not itself one generic native edit control. Setup 0xF9AC0 loads the original GUI resource, finds a frame named “edit” by 0x332190, detaches that template, and later clones it below a frame named “charactername”. The charactername frame is stored at interface+0x78, hence G+0x190. Its creation-page parent is interface+0x58, hence G+0x170. The ordinary template has an outer Widget and an inner concrete Edit with its own caption and frame.

Original RTTI identifies plasma::Edit at type descriptor RVA 0x54BE58, whose name bytes start at 0x54BE68. Its offset-zero complete-object locator is 0x4B13F0, primary virtual table 0x4775C8. The supported table selects 0x327E20 for key down at byte slot 0xC0, the no-op 0xE8A20 for key up at 0xC8, 0x327B40 for character input at 0xD0, 0x328120 for left press at 0x58, the release thunk at 0x328320 for 0x60, and 0x328330 for motion at 0xA8. Its clone target at slot 0x140 is 0x3274E0; its extra drawing target at slot 8 is 0x327630. These are direct table relationships, not names inferred from offsets. Widget and Button RTTI and primary tables were also checked; their tables are 0x477988 and 0x47B3A0.

The original-side game-files/gui.plx is separate evidence: 2,005,316 bytes, SHA256 3E8FC1C334DF173EF90AE8B2D28EA816835A1DADA1F89051F6D50B0D8142E7F7. Its selected length-delimited records identify Node 306 named “edit” at file offset 0x1E03C7. It references Widget 39, has children 307 and 309, and flags zero. Node 307, “clip”, has no own widget and contains Node 308. Node 308, “text”, references Widget 13 and caption/shape index 215; its flags are zero. Node 309, “Shape”, has no own widget. Widget 39 is a Widget named “edit”; Widget 13 is the single Edit record, at file offset 0x1CFC73, named “text”, with flags zero and initially empty caption. The file’s local dictionary maps 87 to Widget, 100 to Button, 102 to Edit and 103 to Node. Its local property identifiers 88, 89, 99, 104, 108, 109 and 110 correspond to Widget.wname, Widget.caption, Widget.flags, Node.wname, Node.widget, Node.child and Node.flags.

Those asset names were decoded only from the selected original asset and native resource-reader operations. Inverting the selected Seal payload against its native fixed fourteen-byte marker gives the byte key 5C CD C7 5E 0B 26 3A FB; the selected dictionary definitions and record associations agree under that key. This is a static asset correspondence, not a runtime capture or proof of which externally supplied Seal candidate is first tried at every startup. The supplying caller’s entire key-list history and unrelated resource formats are outside this contract.

The reached PLX route in 0x3429A0 creates reader 0x3802B0 and invokes 0x3809A0. The Edit dispatch in that reader reaches 0x382680, which creates the widget through 0x33CB30/0x327440 and reads its properties through 0x386560. That property owner writes base Widget attributes, text/name and flags; it contains no Edit-specific validator assignment. The Node route 0x382F00 obtains the indexed widget and attaches it by 0x335150/0x32D190. Reader completion establishes child links. The separate 0x343AD0/0x348F70 route is for the separately selected PLA-format dispatch, not the ordinary PLX factory; it must not be substituted for the PLX route.

The Edit allocation is 0x1D0 bytes. Constructor 0x327440 establishes its tables, the 32-bit value 2 at +0x50, null caption cache at +0x1A8, null validator pointer at +0x1B0, and one eight-byte zero store spanning signed 32-bit caret +0x1C4 and signed 32-bit selection displacement +0x1C8. Its clone uses base copy 0x329210, creates a new callback map, resets the caption cache and caret/selection, and copies the validator pointer as a raw pointer. This is not a whole-object zeroing claim; in particular these constructors do not establish every extra byte, including the drag byte +0x1B8. The ordinary left-press path writes that byte before using it.

Attachment/refresh 0x327AA0 caches F+0x58 only when that object exists and its 32-bit type at +0x70 is 3. It then clamps the caret/selection. For a parent with a shape it sets the parent display’s selected indexed integer at index +0x134 and array +0x168 to 1. Finally it remembers the own frame’s selected position pair in Edit+0x1BC/+0x1C0. Setup ends with manager/frame refresh through 0x340D60/0x333820, so descendant frames receive the own-or-inherited widget pointer at F+0x78 and Edit attachment is reached.

The Create Character button is a separate clone stored at interface+0x70, hence G+0x188. Its frame is a child of the creation page. Setup sets its caption to “Create Character” and, at 0xFCD06..0xFCD30, binds event integer 3 to raw owner G and target 0x9EA10. Both one-byte arguments for recursive binding and enabled gating are 1. Binding 0xF8410 installs that connection on the button widget and descendants enumerated by 0x32AC80. Setup disables the button template’s child frames for direct events by 0xF9770: that helper recursively sets frame flag bit 1 when given zero. This is a frame-event flag, not a store to the widget-enabled byte.

The name editor is a sibling of this button, not its descendant. The complete setup body contains no event-0x14 binding from this name editor to confirmation. The editor’s own changes and the button’s event-3 connection must therefore be treated as separate paths. Pressing Enter in the selected Edit is not a direct call to 0x9EA10.

Connection storage is an ordered map at Widget+0x188 with a signed 32-bit event key. Replacement 0xF8380 frees an existing connection before allocating 0x20 bytes. Allocation failure leaves a null connection. A successful allocation stores one-byte flag +8, the raw Game pointer +0x10 and raw target address +0x18, with table 0x441BA8. The complete eleven-byte invoke thunk at 0xF99A0 loads that raw owner as its receiver and transfers directly to the stored target. No Game reference increment, ownership transfer or queued dispatch occurs in this binding/invocation closure. Generic event dispatcher 0x329D60 looks up its supplied signed 32-bit event key and invokes a nonnull connection synchronously; the Edit change paths supply 0x14. It supplies no independent enabled check.

## Window delivery and its ordering

The complete 0x133C70 procedure uses the 32-bit message number. For the following three messages it invokes the GUI first, then rereads the global Game pointer and invokes the Game handler, and returns zero:

| Message | First call and input | Following Game call and input |
| --- | --- | --- |
| 0x100, key down | 0x340DF0, low unsigned 16 bits of wParam | 0x96E70, low unsigned 8 bits |
| 0x101, key up | 0x340E60, low unsigned 16 bits | 0x97870, low unsigned 8 bits |
| 0x102, character | 0x340DD0, low unsigned 16 bits | 0x9F670, low 8 bits, subsequently used as a signed byte by that Game path |

A GUI callback result does not consume or cancel the following Game call. The subsequent Game handler observes state after the synchronous GUI handler and any callbacks it reached.

Handled keyboard paths do not inspect lParam’s repeat count, prior-key bit, transition bit or scan code. They neither multiply one message by its repeat count nor suppress an individually delivered repeated key-down/character message. Each delivered message reaches its respective path once. The manager’s held-key collection prevents duplicate set membership, but the key-down virtual call is still made after duplicate insertion. This is a dispatch fact, not a guarantee of the operating system’s repeat generation or message translation.

Messages 7 and 8 return 1 without clearing the GUI selected widget or held keys. Message 0x200 also returns 1 without storing mouse coordinates from lParam. The procedure has no explicit selected handler for 0x104/0x105/0x106 system-key/system-character messages, 0x109, or the 0x203/0x206/0x209 double-click messages. The latter three jump-table entries go to the unhandled path. Such messages reach DefWindowProcW; this audit does not assign that imported provider’s effects.

Messages 0x201/0x204/0x207 call imported SetCapture(window) before Game mouse press 0x94900 with 32-bit button identifiers 0/1/2. Messages 0x202/0x205/0x208 call imported ReleaseCapture before Game release 0x965E0 with the same identifiers. They return 1. These are operating-system capture calls; GUI frame capture at M+0x168 is a separate pointer.

Message 0x20A examines the signed 16-bit high word of wParam, reduces it to -1, 0 or +1 and calls Game 0x97CC0, then returns 1. Message 5 calls resize owner 0x135310 with zero and returns 1. Message 0x10 invokes imported DestroyWindow and returns 1. Message 2 either clears global byte 0x551A8E when it is nonzero, or calls imported PostQuitMessage(0), then returns 1. Every other message tail-transfers its original arguments to DefWindowProcW.

The selected application-loop context in 0x1355A0 calls PeekMessageW without removing, obtains a message through GetMessageW, and for a nonzero result calls TranslateMessage before DispatchMessageW, then checks again for messages. GetMessageW zero enters the quit path. No application update is inserted explicitly between the two dispatch calls for adjacent messages already being drained by that loop. After the available-message loop, the selected ordinary route samples/normalizes cursor coordinates through imported window APIs and input providers, calls Game motion 0x97880, and eventually calls frame owner 0x134420 before returning to the message loop. This is static call order. It does not guarantee that imports cannot reenter, that callbacks cannot alter the graph, or that another thread cannot access it.

## Manager key, focus and ancestor admission

Manager+0x170 is the selected widget. Manager+0x168 is GUI captured frame; +0x160 is the previous hover frame; +0x178 is the popup widget; +0x1B0 is an independently tracked drag-related widget. The current mouse pair +0x180/+0x184 and previous pair +0x188/+0x18C are binary32. The 32-bit button mask is +0x1A8. These fields have distinct roles.

Key down writes the zero-extended 16-bit key into M+0x1AC, inserts its unsigned 16-bit value into the ordered held-key set at +0x198, then fetches M+0x170. It calls virtual slot 0xC0 only if that pointer is nonnull and its one-byte +0x1A0 is nonzero. Key up writes the same last-key field, erases the unsigned 16-bit key and then applies the same selected/enabled test for slot 0xC8. A key-up for a key absent from the set still reaches that virtual call when the widget test passes. Character dispatch uses only the selected/enabled test and slot 0xD0; it does not add a key-set entry or write +0x1AC.

These three owners do not traverse ancestors, inspect page visibility, run a hit test, require mouse capture or check operating-system focus. A pointer remaining selected can therefore continue receiving these dispatches even if a separate frame visibility or ancestor flag has changed, provided the selected widget’s enabled byte is still nonzero. This is conditional on its lifetime and the message actually reaching this procedure.

Focus setter 0x349A90 returns immediately for an already-selected nonnull widget. Otherwise, an old nonnull widget is marked dirty at +0x15C and the manager invokes event 0x13 through 0x342560 using the old widget’s own frame. It then stores the requested selected pointer. For a nonnull new pointer it considers event 0x12 on the new frame’s inherited widget. Event invocation requires a nonnull frame, frame flag bit 1 clear, a nonnull inherited widget, a nonnull connection, and either the widget enabled byte nonzero or connection flag +8 zero. The setter does not reject the new selected pointer merely because that widget is disabled. Old-event invocation occurs before publishing the new selected pointer; new-event invocation occurs afterward. The handlers are synchronous and may change state; no reentrancy stabilization is added here.

The ordinary left-press manager 0x340EC0 first sets button-mask bit 0 and selects the captured frame when M+0x168 is nonnull, otherwise it runs hit test 0x333F60 from M+0x140 using the stored mouse pair. It handles popup ancestry through 0x3308C0 and the separate drag-related field before focus work. Unless the hit frame’s own widget F+0x70 equals the selected widget, it dirties the old selected widget, dispatches old event 0x13 and clears M+0x170. It then calls the hit frame’s inherited enabled widget at virtual +0x58, followed by the frame’s event-2 connection subject to the frame-bit/enabled/connection-flag gate. A generic widget inherited from a parent and a frame’s own widget are deliberately different operands in these comparisons.

The noncaptured hit test requires each traversed frame’s selected 32-bit visibility integer to be nonzero, frame flag bit 2 clear and bit 12 clear. It uses the selected transforms and shape/widget providers for geometric admission, visits children from the back of the native child list before admitting the current frame, and for flags argument zero requires current frame bit 1 clear. It can recurse through frames whose own direct event admission is disabled. Caption/shape and inherited-widget hit predicates remain calls to their original providers; enabled +0x1A0 is checked later by the dispatch owners. This is not a generic rectangular-control rule.

Middle and right press use button-mask bits 2 and 1 and corresponding virtual/event pairs; their complete selected manager bodies likewise perform the old-focus clearing test. Their Edit targets in the supported table are no-ops. They do not provide an independent character-editor confirmation path.

The left-press manager’s admitted-tree traversal uses complete thunk 0x33AAC4 for virtual slot 0xE0 during its drag-related phase and 0x33AAD0 for slot 0xE8 at its end. The corresponding left-release traversal uses 0x33AADC for slot 0xF0, and manager motion uses 0x33AAB8 for slot 0x130. All four selected slots in the concrete Edit, Widget and Button primary tables point to the supported no-op 0xE8A20. Thus these extra traversals do not add a name-edit or Create callback on those concrete receivers; other widget classes reached by a graph traversal remain their own virtual boundaries.


## Current text, caret and character editing

The current text belongs to the type-3 caption cached in Edit+0x1A8, not to Entity.name and not to the outer name frame’s generic Widget name. Its current string object is obtained from caption+0x108 using the signed 32-bit index at caption+0xD4 and a 0x20-byte stride. Each string has unsigned 64-bit length at +0x10 and capacity at +0x18, two-byte units, and inline capacity 7; at capacity 8 or greater its data pointer is used. Edit operations commonly read only the low signed 32 bits of that length for caret/selection comparisons. The ordinary short-name domain does not justify claims about strings exceeding signed 32-bit length.

Name lookup 0x333690 compares exact two-byte frame names with explicit lengths. At a matched frame, its one-byte argument 1 requests recursive text retrieval through 0x332990. That owner returns the current string immediately for a type-3 own caption, even when that string is empty. For a frame without such a caption it walks children from the front and returns the first nonempty recursive result. It returns an empty string if none is found. Nonmatching named search likewise walks the stored child order. Therefore confirmation’s “edit” lookup is a bounded graph/text lookup, not a direct pointer to Edit+0x1A8. For the selected template it follows the edit/clip/text subtree to the inner caption.

Setter 0x334C50 skips a subtree whose frame bit 2 is set. Otherwise it compares and copies a changed current string on any own type-3 caption, then visits every child from the front. It does not move the Edit caret, reset selection, invoke event 0x14 or call the caption refresh virtual merely because it copied text. Named setter 0x3348E0 locates the named frame and with recursive argument 1 reaches that setter. Setup uses this to clear the selected “edit” subtree. It does not publish the edited text into the live entity on every character.

Clamp 0x328DF0 treats caret +0x1C4 and displacement +0x1C8 as signed 32-bit fields. With no cached caption it clears the pair with an eight-byte store. Otherwise it first raises a negative caret to zero and lowers a caret exceeding the low signed 32-bit length to that length. If the 32-bit caret-plus-displacement sum is negative, it changes the displacement to the negative of the caret. It then recomputes the endpoint; if that exceeds the caption length it changes the caret to length minus displacement. The last correction writes the caret, not the displacement. It performs one ordered pass, not a repeated general repair of arbitrary corrupted values. The arithmetic is the original fixed-width instruction arithmetic; no C++ overflow theory is being substituted for it.

Character owner 0x327B40 returns before mutation when the caption cache is null or the unsigned 16-bit character is below 0x20. Otherwise it marks Edit+0x15C dirty and constructs a one-unit two-byte string. It does not reject 0x007F, surrogate units, or units whose low byte is zero merely for those properties.

When a selection displacement is nonzero, the optional raw validator at +0x1B0 is first called on the incoming one-unit string. A false low-byte result returns before selection deletion. A missing validator or accepted one-unit input reaches selection deletion 0x327580, including that helper’s event and layout work. Only afterward does the character owner copy the current caption string to a temporary, clamp the caret again and insert the one unit at the signed caret position through 0x3DF40. It then calls the optional validator on the complete candidate. If accepted or absent, it publishes the candidate recursively through 0x334C50 on the Edit’s own frame, increments the 32-bit caret, performs layout 0x328470 and dispatches event 0x14. If complete-candidate validation rejects, that final publication/increment/event does not occur. An already completed selection deletion is not rolled back. Thus replacing a nonempty selection can dispatch event 0x14 twice, once for deletion and once for accepted insertion. These are synchronous events, not an aggregated “text changed” queue.

The ordinary constructor, PLX property route and cloned name template establish a null validator in this closure. The optional-validator ordering is still a native class contract for other callers. This is not a universal census of every possible later +0x1B0 writer.

Deletion 0x327580 first clamps. If the caret is greater than the low signed 32-bit length it returns. Zero displacement requests one unit at the caret. A positive displacement requests that many units starting at the caret; a negative displacement starts at caret plus displacement and requests its 32-bit negation. The reached erase helper 0x3DE90 takes unsigned 64-bit position/count after sign extension from the supplied 32-bit operands. In the valid short-string domain it truncates erasure at the remaining length, moves the tail in two-byte units and writes a two-byte terminator. Position beyond the string length reaches the original out-of-range failure provider. These helpers do not establish well-behaved outcomes for corrupt negative indices or extreme displacement values.

After erasure, a negative selection moves the caret by that displacement; selection is cleared. The caption refresh virtual at slot 8 is invoked with low byte 1, followed by event 0x14 and layout. Delete at the end of an ordinary string can therefore perform no textual removal yet still refresh, dispatch event 0x14 and perform layout. No event is coalesced based on a changed-string comparison in this helper.

## Keys and selection in the concrete Edit

The Edit key-down owner returns immediately if the caption cache is null. Otherwise it clamps and marks dirty before testing the key. It has the following explicit key cases; selection changes are signed 32-bit stores.

| Unsigned 16-bit key | Reached effect |
| --- | --- |
| 0x0D, Enter | Calls focus setter 0x349A90 with null, then layout. There is no confirmation call here. |
| 0x25, Left | If caret is positive, decrements it. If held-key set contains exactly 0x10, increments selection displacement; otherwise clears selection. It then performs layout. At the left edge it only reaches layout after the initial clamp/dirty work. |
| 0x27, Right | If caret is below low signed 32-bit string length, increments it. Held key 0x10 decrements displacement; otherwise selection is cleared. It then performs layout. |
| 0x2E, Delete | Calls 0x327580 and then performs another layout call. |
| 0x08, Backspace with selection | Uses the same deletion path and the subsequent additional layout. |
| 0x08, Backspace without selection | Returns if caret is below 1; otherwise erases the preceding single two-byte unit, decrements caret, refreshes caption, performs layout, then dispatches event 0x14. |
| 0x24, Home | Held key 0x10 adds the old caret to displacement; otherwise clears displacement. Stores caret zero and performs layout. |
| 0x23, End | Held key 0x10 adds old caret minus the low signed 32-bit string length to displacement; otherwise clears displacement. Stores caret to that length and performs layout. |

Other keys return after the initial clamp/dirty work. There are no Ctrl+A, Ctrl+C, Ctrl+V, clipboard, word-boundary or text-composition branches in this complete Edit owner. Its key-up virtual is the supported no-op. The held-key test is exact unsigned 16-bit membership for 0x10; no left/right modifier normalization occurs in the selected manager or Edit owners.

The distinct virtual handler at 0x3280C0 stores caret to the low 32-bit length, selection to its 32-bit negation and tail-transfers to layout. It is a complete selected table target. The ordinary window procedure does not directly dispatch double-click messages to that slot, and this audit does not assert an ordinary caller for it merely from its position in the table.

Edit left press 0x328120 first requires a cached caption and writes drag byte +0x1B8 to zero. It converts the manager’s stored binary32 mouse pair to own-frame coordinates using 0x100EE0 and the selected matrix arithmetic. If it is not currently selected, it sets caret to current length and selection to its negation, then selects itself through 0x349A90. That branch leaves drag byte zero, so the first press selects all current text without starting this handler’s ordinary drag extension.

When already selected, a press without held key 0x10 picks a new caret and clears selection; with 0x10 it saves the old caret, picks a new one, and subtracts the sum of the new and old caret values from the existing displacement. Native 0x3282B3..0x3282C3 explicitly uses that sum. It must not be replaced in this description with a conventional anchor-preserving difference. The owner then sets drag byte 1, clamps, performs layout and publishes its own frame as GUI capture at M+0x168.

The picked caret in 0x328B30 starts at zero. For each signed index from zero through length minus one it asks original metric owner 0x352310 for the current unit position and size using the caption/font/transform inputs. It advances the caret to index plus one when point X is strictly greater than returned position X plus returned width X multiplied by binary32 word 0x3F333333 at image RVA 0x4328AC. Equality does not advance for that iteration. It then clamps and marks dirty. This is a provider-dependent selection rule, not a fixed-width font assumption; font measurement, unusual metric returns and full transform numerics remain dependencies.

Motion 0x328330 requires manager button bit 0, selected pointer equal to this Edit, and nonzero drag byte. It transforms the stored point, saves the old caret, picks the new caret, adds old caret minus new caret to displacement, then clamps and lays out. This ordinary drag path uses a difference, unlike the selected Shift-press path. Release thunk 0x328320 obtains the manager and reaches 0x3493B0, which clears GUI capture and invokes manager mouse-motion recomputation using the current stored pair. That recomputation can synchronously invoke leave, enter and motion targets/connections; it is not merely a pointer clear.

## Release admission and confirmation

The left-release manager 0x3410E0 first clears button-mask bit 0 and saves a frame: captured frame when nonnull, otherwise a fresh hit test using the stored point. It may dismiss a popup under its separate popup/drag gate. For the saved frame it invokes the inherited enabled widget’s virtual +0x60. It then uses that same saved frame, after the virtual has returned, to consider event 3: frame bit 1 must be clear; inherited widget and connection must exist; widget enabled must be nonzero unless connection flag +8 is zero. The Create connection’s flag is 1, so its widget must be enabled.

There is no selected-widget requirement, ancestor visibility recheck, fresh name-length validation, stored prior-press comparison or release-inside comparison in this event-3 invocation block. Noncaptured hit testing supplies its own visibility/geometry gates. A captured frame bypasses that hit test, and the generic button release virtual does not add a confirmation validity test.

For the supported Button table, press 0x36A140 first captures its own frame, writes 32-bit fields +0x2C0 to -500 and +0x2C4 to 150, and sets pressed byte +0x2B8. It then updates display state according to its 32-bit type +0x2BC and checked byte +0x2B9; its type-2 path can clear checked state in the linked button group. Release 0x36A240 clears capture through 0x3493B0, clears pressed byte and chooses a display-state string according to type and checked byte. It has no test that a corresponding press happened, and no physical-inside/name test. The manager still invokes the saved frame’s event-3 connection afterward. The Button clone initializes its pressed/checked bytes with a two-byte zero store and copies its 32-bit type. Its ordinary update target 0x36A2F0, selected by slot 0xD8, acts only while pressed: it adds the manager’s signed 32-bit update amount at +0x190 to +0x2C0, and while +0x2C0 is strictly greater than +0x2C4 it dispatches event 0x15 through 0x342560, subtracts the current interval, and lowers an interval above 50 by 10. It can repeat within one call. That is event 0x15, not Create’s event 3 or Edit’s event 0x14; the selected Create binding does not install its target on event 0x15. For stable objects with unchanged gating, dragging a captured Create button outside before release does not by itself prevent that connection.

After the connection block the manager refetches capture or hit state and separately compares it with a physical hit test to dispatch a leave target/event if appropriate, then traverses admitted visible frames through thunk 0x33AADC and own-widget virtual slot 0xF0. These later phases do not retroactively undo a completed confirmation. Right/middle releases clear their corresponding mask bits and use their own event/virtual pairs; they do not use Create’s event 3.

Confirmation 0x9EA10 immediately writes zero to the creation page’s selected 32-bit visibility integer through G+0x170. It then searches “edit” below G+0x190 with recursive flag 1 and converts the returned current wide text through 0x354C30. The conversion walks exactly the source’s 64-bit stored length in two-byte units and appends each unit’s low eight bits to a native narrow string. That string has 64-bit length/capacity and inline capacity 15. The reached 0x6EDA0 loop and 0xA23B0 reserve helper do not call a Unicode conversion/code-page import. This is not UTF-8 conversion and does not combine surrogate pairs.

The confirmation then copies bytes, including the first zero byte, from the narrow C string to the local entity obtained afresh through G+8, World+0x448, destination Entity+0x958. The copy loop has no length bound or repeat of the interface’s 2..15-unit test. Embedded low-byte zero can terminate this destination copy before the converter’s stored length. For accepted ordinary short names, the one-byte destination has room for the converted 2..15 low bytes and its terminator; this does not prove safety for arbitrary direct/stale callbacks or malformed state.

Only after that copy does confirmation store 32-bit level 1 at Entity+0x144, reach the helper using the current two 32-bit home fields at Entity+0x1D0/+0x1D4, and show the own frame of the widget reached through G+0x390. It writes the new numeric count and selected slot through the persistence entry points, brackets reached World work with 0x2ABB70/0x2B9890, and calls 0xA27B0 between them. It allocates a separate entity and copies core/inventory/cursor/gold/other selected aggregate and buff state through the explicitly reached copy helpers, creates a CharacterData object, copies selected CharacterData fields/collection, appends the new entity pointer to the Game’s character vector, rebuilds character-selection UI through 0x107440, rebuilds the selected interface collection through 0x107690, passes the no-op 0xE8A20, then calls 0xB2180. Those save/copy/World helpers remain effect boundaries here; this keyboard audit does not derive their entire serialization, placement, reset or worker behavior. In particular 0x107440 and 0x107690 are synchronous UI rebuild work, not evidence of an immediate draw or message-pump iteration.

The complete confirmation owner does not reread name length for admission, select/focus the name editor, or itself call its decoder/load entry 0x9BB20. Its stores to page visibility do not by themselves free the editor or connection. The Game release handler continues after GUI release and can reach other admitted Game work; no universal promise that all unrelated roots are inactive is added.

## New selection, polling, update and drawing

The ordinary Select callback bound separately in setup reaches 0x9F7C0. It hides G+0x130 and enters World guard 0x2ABB70 before reading signed selected index G+0x6C0 and copying it to G+0x2264. A negative index or an index at least the signed current character count reaches the new-character branch. That branch shows G+0x170, calls the concrete entity reset 0x64A90, clears the first Entity+0x958 name byte and writes the selected initial entity fields; it also replaces CharacterData through its reached helpers. Near the end of that branch it constructs an empty wide string and calls 0x334C50 on G+0x190, then performs the two selected style-widget calls on G+0x178. Thus the name-frame caption clearing is synchronous on new selection, not deferred to a character message. This does not infer the full reset/style-helper effects beyond their explicit receivers.

The in-range existing-character branch instead reaches loader 0x9BB20 on the current local entity, selected-slot persistence, 0x107690 and 0xB2180. It does not reach the new branch’s empty-name-frame setter. Both branches subsequently reach World rebuild 0x2E0B50 and guard release 0x2B9890. Existing selection and confirmation are distinct callbacks. The pre-GUI part of Game release 0x965E0 can also invoke 0x9BB20 for menu selection value 3, but only after its G+0x118 and G+0x120 visibility gates and left-button test. This is not a call made by every character or every Create release.

Interface owner 0x103200 receives the context G+0x118 at viewed-update call site 0xAB5CE. Its selected creation block is at 0x10458A..0x10466D. Only when G+0x170’s indexed visibility integer is nonzero does it query “edit” under G+0x190, read the returned unsigned 64-bit length, subtract 2 in 64 bits and set the Create button frame’s indexed 32-bit visibility integer to 1 for unsigned results at most 13, otherwise zero. In the ordinary length domain this admits exactly 2 through 15 two-byte units, inclusive. It is a visibility gate, not a string-length limit in Edit and not a store to Widget+0x1A0. Lengths 0, 1 and 16 fail this test. The handler does not trim spaces, inspect low-byte conversion, restrict alphabet/category, or invoke confirmation. Other interface blocks validate separate world-creation fields; they do not substitute for this creation-name test.

Near the end of the complete interface owner, 0x349CF0 updates the manager’s 32-bit elapsed/cumulative fields and invokes frame-update wrapper 0x3358E0. The wrapper first clears animation-ready bytes recursively through 0x334DC0, performs 0x335920, then clears dirty markers through 0x334E30 on the reached dirty subtree. Frame update respects frame bit 2 and selected visibility; flag bit 13 can carry update admission to descendants. It refreshes inherited widgets, asks the selected shape/transformation/display owners to update as needed, and calls each own widget’s virtual +0xD8. Edit’s supported +0xD8 target is a no-op. A frame whose own widget is currently selected is marked dirty on each admitted update, even without another keyboard message. None of these update owners manufactures a character message or calls the Create connection merely because the text is eligible.

Actual extra Edit drawing is reached from graph draw 0x330B20, not from the character callback or from 0x107440’s name. Graph drawing requires selected visibility nonzero and bit 2 clear, considers dirty/cached-target and draw-mode admission, draws shapes/children using original providers, and invokes an own widget’s slot 8 before or after its children according to Widget+0x150 bit 9. The Edit target 0x327630 itself requires that this widget equal M+0x170 and that its cached caption’s font/resource pointer +0x2F0 be nonnull. It obtains font setup and position/size outputs from 0x354190/0x352310, orders selection endpoints, clamps their drawing indices against the low signed 32-bit text length, and submits a selection rectangle through the manager’s virtual +0x20. For a caret index between zero and length inclusive it obtains another metric result and submits a width-1 caret through virtual +0x18; the checked color words at 0x438050 are binary32 1, 0, 0, 1. Rectangle height is the caption’s +0x2B8 binary32 value multiplied by the exact binary32 factor 1.25, word 0x3FA00000 at image RVA 0x42E71C and its vertical origin uses that value’s sign change. This body does not edit current text or invoke confirmation. Complete font rasterization, clipping and imported graphics output are outside this contract.

The ordinary concrete renderer is constructed at 0xC3150 with the manager as its fourth argument and stores it at renderer+0x540. Its table 0x4385E8 has slot 0x10 at 0x4385F8 pointing to complete thunk 0xC4AF0. That thunk loads renderer+0x540, supplies 32-bit draw argument zero and transfers to 0x33E170. The renderer owner 0xE2E00 reaches its virtual slot 0x10 at 0xE4F35 after its selected World draw work; 0x33E170 reaches graph draw 0x330B20 on manager+0x140. The viewed update calls 0x103200 at 0xAB5CE and later 0xE2E00 at 0xAD9D4. Consequently those reached ordinary interface updates precede that reached GUI graph draw in the same update. This is static order on the ordinary concrete receiver and reached branch, not a universal renderer or alternate-caller census.

The selected motion bridge is also separate from window 0x200. Application call 0x13683E reaches Game 0x97880. When interface predicate 0x1014F0 is true, that owner converts its binary32 coordinates to signed 32-bit integers and calls 0x101160; the latter converts them back to binary32 for manager motion 0x3418D0 before its other selected work. The predicate tests the indexed visibility/state of the selected interface roots, including creation at context+0x58; creation visibility alone is sufficient for a true result. Manager motion stores previous/current pairs, chooses capture or hit, emits ordered leave/enter/motion virtuals and events, stores hover frame, then traverses admitted visible frames through thunk 0x33AAB8 and own-widget virtual slot 0x130. Coordinates used by a window button message are therefore the manager’s stored coordinates at dispatch time, not a direct use of that message’s lParam.


## Conditional message sequences

The following are independently chosen mathematical/control-flow examples. They assume valid stable objects, the selected inner Edit enabled, its cached caption attached, a null validator, no intervening callback mutation, and the stated messages reaching 0x133C70. They are not original runtime observations or prescriptions for a scheduler.

* With empty text, caret zero and no selection, separate character messages for U+0041 and U+0042 immediately leave caption text “AB”, caret 2 and selection zero. Each message dispatches one event 0x14. The live entity name has not been copied by these messages. A later reached creation-page polling block sees length 2 and makes the Create frame visible.
* One delivered character message for U+0041 still inserts one unit if lParam’s repeat-count bits say 3. Three separately delivered such messages insert three units. Repeated Left key-down messages each reach their movement case until its boundary, even though the held-key set has only one entry.
* With text “AB” selected by a first focus-acquiring press, a character message U+0043 first deletes the two selected units and then inserts “C”. It reaches deletion’s event 0x14 before insertion’s event 0x14, and ends at caret 1 with zero selection. A later name-length poll hides Create because length 1 fails.
* With four ordinary units, prior caret 3 and selection zero, a selected Shift-press whose original metric provider picks caret 1 subtracts 4 from selection. Clamp then changes displacement to -1 because the endpoint was negative. The resulting pair is caret 1, displacement -1. This example illustrates the original sum in the press path; the motion path uses the different difference described above.
* A selected Enter key-down clears the GUI selected pointer before the following Game key-down handler runs. It does not invoke Create. The later Game path still writes its low-byte key state and evaluates its own gates against this changed selected pointer.
* Two accepted character units U+0100 and U+0041 give wide length 2 and can pass the later visibility test. If the Create connection is then admitted, the converter’s stored narrow result has bytes 00 and 41, but the confirmation C-string copy writes a first zero at Entity+0x958 and stops. Thus the wide-length gate alone is not a nonempty-low-byte-name guarantee.
* Suppose a valid-length poll has made Create visible, the stable Button receives a press and captures its frame, and later release is dispatched after the physical point leaves the button. Its release virtual clears capture, but the manager’s already saved frame is still the event-3 receiver. If its frame-event/enabled/connection gates remain admitted, confirmation is invoked. A physical leave alone is not a tested cancellation condition in this closure.
* Suppose character messages change a name from length 15 to length 16 after the last length poll. Edit does not itself reject the sixteenth unit or recompute Create visibility. The stored button gate remains the prior value until a reached polling block updates it; confirmation itself supplies no fresh length test. This describes the missing synchronous revalidation, not a guaranteed wall-clock input window or an assertion of valid behavior beyond the ordinary short-name domain.

Game key-down 0x96E70 always writes byte 1 at G+0x1A87 plus the low unsigned 8-bit key before its switch. Escape toggles G+0x420 independently of GUI selection. Its explicit function-key branches can toggle the frame reached from G+0x390 or the manager root and adjust selected configuration scalars before the later text/chat admission. Those branches are not suppressed merely because the Edit received the same key first. Game key up 0x97870 writes byte zero in the same low-byte array.

The later chat-related key gate in 0x96E70 requires the selected indexed visibility of G+0x220 nonzero, GUI selected pointer null, and signed 32-bit field at the widget reached through G+0x658 plus 0x2E0 negative. Its Enter case can then change that widget’s separate chat flag and process its existing text through further helpers. Game character owner 0x9F670 independently requires the manager selected pointer null and the widget at G+0x398 byte +0x1D8 nonzero before reaching 0x26B170 with the sign-extended low character byte. These Game gates are separate from the GUI’s selected/enabled gate. The selected Edit, Entity name, and that Game text buffer are not one storage object.

## Lifetime, protection and limits

The original selected callback uses raw owner and target addresses. It does not take a reference to the Game or defer the invocation. Base Widget copy creates an empty callback map; setup establishes the selected Create connection after cloning. Base destruction 0x329500 frees nonnull connection allocations in its map, destroys that map and the selected owned strings/resource, then reaches the base registry destructor 0x366A10. The concrete Edit deleting target 0x327490 reaches this destructor and conditionally frees the 0x1D0-byte object according to deleting flag bit 0. None of this makes the Game the connection’s owned payload.

The manager-owned frame removal helper 0x33DDA0 first checks that the frame’s manager pointer equals its receiver. If that frame is captured it clears M+0x168 and recomputes motion; if it is the hover frame it clears M+0x160. It processes a copied child list recursively, detaches from its parent, releases/unlinks selected transformation/shape/display resources, and reaches widget removal when an own widget exists. Widget removal 0x33E0C0 clears matching selected/popup/drag-related pointers, removes the widget’s manager registry node while holding critical section M+0x90, releases that lock, and then invokes the deleting virtual. Frame removal also holds that section while removing its own manager registry node, releasing it before the deleting virtual. These are concrete helper effects and scopes, not a proof that every conceivable direct deletion follows them or that callbacks are universally quiescent.

The selected ordinary page show/hide stores and text setters do not themselves destroy the name editor. New selection reaches an explicit caption clear, confirmation reaches page hiding and later UI rebuilds, and generic deletion follows its separate helper route. There is no promise that hiding alone cancels keyboard selection or GUI capture. The Back target bound by setup is visible as an original callback address 0x9DDB0, but its body is not decoded in the selected original instruction export and was not independently expanded here. This contract makes no claim about that target’s page, text, focus or lifetime effects.

The selected quit context in application owner 0x1355A0 destroys the Game before the GUI manager and contains no intervening explicit ordinary DispatchMessageW call in that teardown branch. It does not establish imported-destructor reentrancy, worker stopping, race freedom or safe invocation of an old raw Game pointer after destruction. Frame/message owners in this audit do not wrap their whole key/character/callback bodies in a GUI critical section. The particular registry operations’ lock cannot be expanded into a universal GUI lock.

No source-proved timing guarantee is supplied for OS repeat generation, TranslateMessage’s generated messages, GetMessageW errors, malformed external messages, input-provider coordinates, asynchronous focus/capture changes, font/provider values, GPU presentation, callbacks that replace/delete receivers, or concurrent worker access. The source explicitly exposes provider boundaries, fixed-width arithmetic and string/container failure calls; it does not validate every impossible pointer/index/length combination. Imported providers and uninspected helper closures remain external facts. No alternate-caller census, generic edit-control feature set, clipboard behavior, IME behavior, full Game save/load/reset behavior or whole renderer numerical contract is claimed.

## Evidence manifest and correspondence

Only the standalone source-author policy and filename-selected original-side material were read for this assignment. The policy path is C:/Users/DieuDeGlace/Documents/Cubeworld-Vulkanengine-Steam/docs/CLEANROOM_AUTHOR.md. The original evidence root is C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Metadata/locator evidence is confined to decompiled/functions.csv, decompiled/function_ranges.csv, decompiled/calls.csv, decompiled/strings.csv and decompiled/string_references.csv; native instructions are from decompiled/assembly.txt and were compared at their PE-mapped raw offsets with the image identified above. Function exports in the following table are individually selected files under decompiled/functions, named 140 followed by their six-digit lowercase RVA and .c.

The table is the complete owner-level byte-comparison manifest: 126 owner entries, 35,269 exported instructions, 163,025 compared instruction bytes, zero mismatching bytes. The selected original owner ranges, including discontiguous ranges, are listed as start-inclusive/end-exclusive RVAs. Unsupported gaps between original ranges are not silently included as decoded instructions. Original failure-call traps/padding omitted from recognized ranges are not assigned invented semantics.

Primary semantic coverage includes the complete window/key/character/focus/press-release/Edit owners, whole setup and confirmation/conversion bodies, complete selected connection/text helpers, the complete interface-update body, and the selected native resource/Edit/Node reader bodies. Complete supplementary bodies were expanded for the bridges, constructor, selection, deletion and graph traversal described above. Game update 0xA5A50, renderer 0xE2E00 and application 0x1355A0 received complete owner-level byte comparison but only the specifically cited context/control-flow slices were used for semantics; their unrelated systems are not audited by this document. The selected PLA reader 0x3448A0 was read only as a locator/format-dispatch contrast and is not represented as complete semantic coverage.


| Owner/file RVA | Original instruction ranges | Compared bytes | Instructions |
| --- | --- | ---: | ---: |
| 0x133C70 | 0x133C70–0x133EEC | 636 | 174 |
| 0x340DF0 | 0x340DF0–0x340E57 | 103 | 26 |
| 0x340E60 | 0x340E60–0x340EB8 | 88 | 22 |
| 0x340DD0 | 0x340DD0–0x340DF0 | 32 | 8 |
| 0x0F9AC0 | 0x0F9AC0–0x10089E | 28126 | 5496 |
| 0x09EA10 | 0x09EA10–0x9EC73; 0x09EC74–0x9EC83; 0x09EC84–0x9EC93; 0x09EC94–0x9ECA0; 0x09ECA1–0x9ED5F; 0x09ED60–0x9ED6F; 0x09ED70–0x9ED7F; 0x09ED80–0x9ED8C; 0x09ED8D–0x9EFEE; 0x09EFEF–0x9EFFE; 0x09EFFF–0x9F00E; 0x09F00F–0x9F01B; 0x09F01C–0x9F051 | 1589 | 377 |
| 0x354C30 | 0x354C30–0x354CE5 | 181 | 50 |
| 0x096E70 | 0x096E70–0x977F3 | 2435 | 550 |
| 0x097870 | 0x097870–0x9787C | 12 | 3 |
| 0x09F670 | 0x09F670–0x9F6B1 | 65 | 18 |
| 0x094900 | 0x094900–0x965E0 | 7392 | 1583 |
| 0x0965E0 | 0x0965E0–0x96833; 0x096834–0x96843; 0x096844–0x96853; 0x096854–0x96860; 0x096861–0x96E70 | 2188 | 508 |
| 0x101020 | 0x101020–0x10111F | 255 | 58 |
| 0x101120 | 0x101120–0x10115F | 63 | 16 |
| 0x103200 | 0x103200–0x106C19 | 14873 | 3103 |
| 0x332190 | 0x332190–0x33221C | 140 | 44 |
| 0x332990 | 0x332990–0x332AC7 | 311 | 79 |
| 0x333690 | 0x333690–0x33381E | 398 | 106 |
| 0x3308E0 | 0x3308E0–0x330A78 | 408 | 115 |
| 0x0F8410 | 0x0F8410–0xF858E | 382 | 99 |
| 0x0F8380 | 0x0F8380–0xF8402 | 130 | 35 |
| 0x3348E0 | 0x3348E0–0x334A13 | 307 | 82 |
| 0x334C50 | 0x334C50–0x334D25 | 213 | 58 |
| 0x342090 | 0x342090–0x342098 | 8 | 2 |
| 0x32AC80 | 0x32AC80–0x32ACE3 | 99 | 26 |
| 0x33CD30 | 0x33CD30–0x33CED3 | 419 | 117 |
| 0x06EDA0 | 0x06EDA0–0x6EE9A | 250 | 70 |
| 0x0A23B0 | 0x0A23B0–0xA245D | 173 | 53 |
| 0x3380E0 | 0x3380E0–0x338237 | 343 | 99 |
| 0x33F010 | 0x33F010–0x33F0DD | 205 | 59 |
| 0x340EC0 | 0x340EC0–0x3410D9 | 537 | 138 |
| 0x3410E0 | 0x3410E0–0x3412F5 | 533 | 141 |
| 0x341300 | 0x341300–0x341495 | 405 | 103 |
| 0x3414A0 | 0x3414A0–0x3415E5 | 325 | 87 |
| 0x3415F0 | 0x3415F0–0x341777 | 391 | 102 |
| 0x341780 | 0x341780–0x3418C5 | 325 | 87 |
| 0x349A90 | 0x349A90–0x349B34 | 164 | 45 |
| 0x3493B0 | 0x3493B0–0x3493D0 | 32 | 4 |
| 0x33C410 | 0x33C410–0x33C418 | 8 | 2 |
| 0x329D60 | 0x329D60–0x329D80 | 32 | 10 |
| 0x32A210 | 0x32A210–0x32A2B5 | 165 | 46 |
| 0x33C360 | 0x33C360–0x33C407 | 167 | 49 |
| 0x333F60 | 0x333F60–0x33429B | 827 | 215 |
| 0x3308C0 | 0x3308C0–0x3308DC | 28 | 13 |
| 0x342560 | 0x342560–0x3425C0 | 96 | 29 |
| 0x342270 | 0x342270–0x342557 | 743 | 161 |
| 0x3418D0 | 0x3418D0–0x341B29 | 601 | 163 |
| 0x327440 | 0x327440–0x32748A | 74 | 17 |
| 0x327490 | 0x327490–0x3274D9 | 73 | 20 |
| 0x3274E0 | 0x3274E0–0x327573 | 147 | 34 |
| 0x327630 | 0x327630–0x327A88 | 1112 | 206 |
| 0x327A90 | 0x327A90–0x327A93 | 3 | 2 |
| 0x327AA0 | 0x327AA0–0x327B39 | 153 | 36 |
| 0x327B40 | 0x327B40–0x327E1B | 731 | 177 |
| 0x327E20 | 0x327E20–0x3280B3 | 659 | 158 |
| 0x328120 | 0x328120–0x328317 | 503 | 107 |
| 0x328330 | 0x328330–0x32846C | 316 | 63 |
| 0x0E8A20 | 0x0E8A20–0xE8A23 | 3 | 1 |
| 0x327580 | 0x327580–0x327630 | 176 | 44 |
| 0x328DF0 | 0x328DF0–0x328EB2 | 194 | 42 |
| 0x328B30 | 0x328B30–0x328D73; 0x328D74–0x328D8C; 0x328D8D–0x328D9C; 0x328D9D–0x328DAC; 0x328DAD–0x328DB9; 0x328DBA–0x328DEF | 698 | 140 |
| 0x328470 | 0x328470–0x328B21 | 1713 | 343 |
| 0x329210 | 0x329210–0x329465 | 597 | 126 |
| 0x329500 | 0x329500–0x329622; 0x329623–0x329639; 0x32963A–0x329649; 0x32964A–0x329659; 0x32965A–0x329666; 0x329667–0x3296AA; 0x3296AB–0x3296C1; 0x3296C2–0x3296D1; 0x3296D2–0x3296E1; 0x3296E2–0x3296EE; 0x3296EF–0x32972D | 547 | 143 |
| 0x328FF0 | 0x328FF0–0x329207 | 535 | 96 |
| 0x09AEB0 | 0x09AEB0–0x9AEF2 | 66 | 21 |
| 0x03DE90 | 0x03DE90–0x3DF3B | 171 | 54 |
| 0x03DF40 | 0x03DF40–0x3E105 | 453 | 131 |
| 0x33DFA0 | 0x33DFA0–0x33DFB7 | 23 | 8 |
| 0x366A10 | 0x366A10–0x366A48 | 56 | 15 |
| 0x348F70 | 0x348F70–0x349355 | 997 | 236 |
| 0x343AD0 | 0x343AD0–0x343C6E | 414 | 107 |
| 0x33CB30 | 0x33CB30–0x33CB8C | 92 | 26 |
| 0x3429A0 | 0x3429A0–0x342F1E | 1406 | 350 |
| 0x07EA80 | 0x07EA80–0x7F35E; 0x07F35F–0x7F36E; 0x07F36F–0x7F37E; 0x07F37F–0x7F38B; 0x07F38C–0x7F9E0 | 3932 | 806 |
| 0x107440 | 0x107440–0x10768C | 588 | 146 |
| 0x107690 | 0x107690–0x1081AA | 2842 | 681 |
| 0x32BFD0 | 0x32BFD0–0x32C089 | 185 | 43 |
| 0x333820 | 0x333820–0x33388D | 109 | 35 |
| 0x340D60 | 0x340D60–0x340DC9 | 105 | 29 |
| 0x334410 | 0x334410–0x334507 | 247 | 60 |
| 0x349CF0 | 0x349CF0–0x349D0F | 31 | 5 |
| 0x3358E0 | 0x3358E0–0x33591A | 58 | 18 |
| 0x335920 | 0x335920–0x335BA2 | 642 | 168 |
| 0x334DC0 | 0x334DC0–0x334E2D | 109 | 34 |
| 0x334E30 | 0x334E30–0x334EBD | 141 | 41 |
| 0x0F8690 | 0x0F8690–0xF87CA | 314 | 67 |
| 0x33E0C0 | 0x33E0C0–0x33E166 | 166 | 37 |
| 0x33C110 | 0x33C110–0x33C35F | 591 | 151 |
| 0x33E170 | 0x33E170–0x33E1D4 | 100 | 28 |
| 0x330B20 | 0x330B20–0x3318E8 | 3528 | 825 |
| 0x33E220 | 0x33E220–0x33E2B1 | 145 | 35 |
| 0x349B40 | 0x349B40–0x349BD2 | 146 | 32 |
| 0x347CC0 | 0x347CC0–0x347DDD; 0x347DDE–0x347DED; 0x347DEE–0x347DFD; 0x347DFE–0x347E0A; 0x347E0B–0x347E34 | 368 | 93 |
| 0x3809A0 | 0x3809A0–0x380D35; 0x380D36–0x380D45; 0x380D46–0x380D55; 0x380D56–0x380D62; 0x380D63–0x380D8C; 0x380D8D–0x381797; 0x381798–0x3817F2; 0x3817F3–0x38181D; 0x38181E–0x38182D; 0x38182E–0x38183D; 0x38183E–0x38184A; 0x38184B–0x3819BE | 4115 | 895 |
| 0x382680 | 0x382680–0x3827A5; 0x3827A6–0x3827AC; 0x3827AD–0x3827B3; 0x3827B4–0x3827BA; 0x3827BB–0x3827DD | 345 | 87 |
| 0x382F00 | 0x382F00–0x383616; 0x383617–0x38361D; 0x38361E–0x383624; 0x383625–0x38362B; 0x38362C–0x383632; 0x383633–0x383639; 0x38363A–0x383640; 0x383641–0x383647; 0x383648–0x38364E; 0x38364F–0x3837CB | 2242 | 555 |
| 0x386560 | 0x386560–0x386670; 0x386671–0x386680; 0x386681–0x386690; 0x386691–0x38669D; 0x38669E–0x386C18 | 1716 | 450 |
| 0x38D8A0 | 0x38D8A0–0x38DA34; 0x38DA35–0x38DA44; 0x38DA45–0x38DA54; 0x38DA55–0x38DA61; 0x38DA62–0x38DAA6; 0x38DAA7–0x38DAB6; 0x38DAB7–0x38DAC6; 0x38DAC7–0x38DAD3; 0x38DAD4–0x38DBB3; 0x38DBB4–0x38DBC3; 0x38DBC4–0x38DBD3; 0x38DBD4–0x38DBE0; 0x38DBE1–0x38DCAD | 1025 | 268 |
| 0x38D6A0 | 0x38D6A0–0x38D761; 0x38D762–0x38D771; 0x38D772–0x38D781; 0x38D782–0x38D78E; 0x38D78F–0x38D7B5 | 273 | 78 |
| 0x3802B0 | 0x3802B0–0x380416 | 358 | 90 |
| 0x38D7C0 | 0x38D7C0–0x38D891 | 209 | 50 |
| 0x38DCB0 | 0x38DCB0–0x38DD30 | 128 | 38 |
| 0x32D190 | 0x32D190–0x32D1A1 | 17 | 5 |
| 0x09F7C0 | 0x09F7C0–0x9F8E9; 0x09F8EA–0x9F8F9; 0x09F8FA–0x9F909; 0x09F90A–0x9F916; 0x09F917–0x9FD03 | 1343 | 276 |
| 0x0A3A20 | 0x0A3A20–0xA3A40 | 32 | 8 |
| 0x09AF00 | 0x09AF00–0x9AF1A | 26 | 6 |
| 0x0F9770 | 0x0F9770–0xF97D6 | 102 | 27 |
| 0x335150 | 0x335150–0x33516C | 28 | 8 |
| 0x330A80 | 0x330A80–0x330B18 | 152 | 41 |
| 0x101160 | 0x101160–0x1014E4 | 900 | 187 |
| 0x33DDA0 | 0x33DDA0–0x33DF9D | 509 | 121 |
| 0x3347F0 | 0x3347F0–0x334842 | 82 | 25 |
| 0x097880 | 0x097880–0x97CBA | 1082 | 201 |
| 0x1014F0 | 0x1014F0–0x10178C | 668 | 119 |
| 0x134420 | 0x134420–0x1347C6 | 934 | 192 |
| 0x0C3150 | 0x0C3150–0xC394B; 0x0C394C–0xC3968; 0x0C3969–0xC3978; 0x0C3979–0xC3988; 0x0C3989–0xC3995; 0x0C3996–0xC3A07 | 2226 | 442 |
| 0x097CC0 | 0x097CC0–0x9803E | 894 | 190 |
| 0x0A5A50 | 0x0A5A50–0xA74D8; 0x0A7E6C–0xAA9FA; 0x0AAC40–0xABDE2; 0x0ABE34–0xABECA; 0x0ABF82–0xADBD8 | 29860 | 6106 |
| 0x0E2E00 | 0x0E2E00–0xE5070 | 8816 | 1849 |
| 0x36A140 | 0x36A140–0x36A23C | 252 | 56 |
| 0x36A240 | 0x36A240–0x36A28D | 77 | 17 |
| 0x1355A0 | 0x1355A0–0x137E1C | 10364 | 2217 |
| 0x36A2F0 | 0x36A2F0–0x36A364 | 116 | 27 |
| 0x369C20 | 0x369C20–0x369C6C | 76 | 22 |
| 0x3694D0 | 0x3694D0–0x369724 | 596 | 148 |


Additional complete decoded thunks absent from a named owner export were individually read and byte-matched: 0xF99A0–0xF99AB, 11 bytes/3 instructions; 0x3280C0–0x328113, 83 bytes/18 instructions; 0x328320–0x328329, 9 bytes/2 instructions; and 0xC4AF0–0xC4AFE, 14 bytes/3 instructions. Every compared byte matched. The selected application subranges 0x1361E2–0x13622D and 0x136440–0x136843 were also checked independently: 75 bytes/19 instructions and 1,027 bytes/239 instructions, with zero mismatches. These overlapping checks are not added again to the owner totals.

Four further complete nine-byte/two-instruction dispatch thunks were individually read and byte-matched: 0x33AAB8–0x33AAC1 for virtual slot 0x130, 0x33AAC4–0x33AACD for 0xE0, 0x33AAD0–0x33AAD9 for 0xE8, and 0x33AADC–0x33AAE5 for 0xF0. The first, third and fourth also have individually selected original C exports; the second is an unassigned original instruction range. Each has zero byte mismatches. These additional ranges are separate from the 126-owner totals above.

The forty raw bytes at 0x133EEC–0x133F14 were read as the window’s ten-entry relative jump table. They map 0x201/0x202 to 0x133DE8/0x133E0C, 0x204/0x205 to 0x133E2D/0x133E51, 0x207/0x208 to 0x133E72/0x133E99, and 0x20A to 0x133EBD; 0x203/0x206/0x209 map to 0x133CBF. The raw exact two-byte literal “edit” was checked at 0x437B38. The raw binary32 constants checked for the editing/drawing statements are 0x4328AC word 0x3F333333, 0x42E71C word 0x3FA00000, and 0x438050’s four words 0x3F800000, 0, 0, 0x3F800000. Integer immediates for keys, event IDs, lengths, comparison boundaries and offsets are in the byte-matched owner instructions.

Original RTTI and table checks used Edit descriptors/locator/table 0x54BE58, 0x4B13F0 and 0x4775C8; Widget descriptor/table 0x54B3A0/0x477988; Button descriptor/table 0x54C238/0x47B3A0; the connection table entry at 0x441BA8; and concrete renderer table 0x4385E8, including pointer entry 0x4385F8. Only valid selected primary slots and the specifically identified secondary-object relationship were used; unrelated adjacent metadata was not treated as virtual entries. Constructor table stores independently agree with those original data pointers.

Original asset evidence is confined to game-files/gui.plx, its selected type/property dictionary records and the edit/clip/text/Shape subtree, the original file’s native count/index ordering, and its stated hash. Its binary payload is separate from the PE and is not claimed to be cryptographically embedded in the supported executable. Asset correspondence does not substitute for native instruction correspondence, and the untraced external Seal-candidate producer remains explicit.

Exposure statement: clean source-only exposure for this assignment. No active AGENTS.md, WORKSPACE-MEMORY.md, combined project history, replacement engine/exporter/extractor, implementation, tests/results, owner captures/data/outputs, SDK/dependency/reconstruction source or earlier held handoff was read or modified. No additional agent was spawned and no implementation/runtime feedback was requested or received. The only project file read was the standalone author policy. The new artifact was created exclusively, fully read back from its saved bytes and held; all earlier held artifacts remain unread and unchanged. All author writes stop at the reported FINAL / STABLE / HELD boundary.
