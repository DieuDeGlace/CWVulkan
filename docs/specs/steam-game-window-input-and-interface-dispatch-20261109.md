# Ordinary Game window input and interface dispatch — supported Steam image

Status: FINAL / STABLE / HELD after the saved-artifact readback reported with this handoff. This is an independent original-source behavioral contract dated 2026-11-09. It contains no replacement-engine advice, source transcription or pseudocode.

## Authority, identity and scope

The authority is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe: 5,757,952 bytes, AMD64 machine 0x8664, preferred image base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. The executable hash was freshly checked for this assignment. All routine and data addresses below are RVAs in that image. Widths and ordering describe the original machine operations, not an inferred C++ abstract-machine contract.

The seven complete selected Game owners are 0x96E70, 0x97870, 0x9F670, 0x94900, 0x965E0, 0x97880 and 0x97CC0. The complete window procedure is 0x133C70. Complete selected interface, GUI and text helpers are listed in the correspondence manifest. Large startup, interface-update, setup and viewed-update routines are used only at the separately identified scoped edges. Unrelated inventory, entity, terrain, rendering, connection and provider internals remain call boundaries.

Only the standalone docs/CLEANROOM_AUTHOR.md policy, this original-side assignment, the original executable and filename-selected original export evidence were accessed. No active AGENTS.md, memory, CLEANROOM.md, combined history, replacement implementation/exporter/extractor, tests, results, packages, owner outputs/data/captures, SDK/dependency/reconstruction source or previous held handoff was read. No implementation or runtime feedback was requested or received. No additional agent was used. Earlier artifacts remain unread and unchanged.

## Receivers and state that must remain distinct

G denotes the Game receiver constructed at 0x7EA80. W is the pointer at G+0x8, M the pointer at G+0x10, and U the GUI manager pointer at G+0x20. P denotes the current pointer at W+0x448 when a particular operand is read; many branches reload this chain after calls. I is the embedded interface at G+0x118, not a separately allocated Game. Its Game pointer is I+0x5C0. Thus I+0x308 is precisely the byte G+0x420; I+0x550 is G+0x668; I+0x548 is the binary32 field G+0x660. Treating an unrelated receiver's +0x420 or +0x308 as these fields is unsupported.

For a node N, the selected-value operation uses its component pointer N+0x68, that component's signed 32-bit index at +0xAC, and the 32-bit integer array through +0xE0. The current indexed integer is compared with zero or written with zero/one. For a widget held in a Game/interface pointer field, its associated node is through widget+0x178. A direct node field and a widget field are distinguished below. These selected integers alone do not establish ancestor visibility, physical focus or a point hit.

U+0x140 is the root node; U+0x160 is the previously retained motion target node; U+0x168 is an optional node override used before hit testing; U+0x170 is the focus-control pointer; U+0x178 is the popup widget pointer; U+0x1B0 is a separately retained press-control pointer. All are 64-bit pointers. U+0x180/+0x184 are current cached binary32 X/Y; U+0x188/+0x18C are the previous cached binary32 pair. U+0x1A8 is a 32-bit button mask with left/right/middle at bits 0/1/2. This mask and U+0x168 are distinct from OS window capture.

Game raw dispatched keys occupy the byte array G+0x1A87 through G+0x1B86, indexed by the low eight bits of the Game key argument. Physical/action states occupy the separate 23-byte array G+0x1A70 through G+0x1A86. Neither array is the GUI's 16-bit-key set at U+0x198. G+0x1958 is another byte, changed by the physical/controller route and tested in motion. G+0x1DB0 is a byte that records admitted cached-coordinate movement and is reset by the press owner after its initial work. The chat widget is the pointer at G+0x398; its +0x1D8 byte is a separate active-text flag.

Construction at 0x7EA80 establishes the W/M/U receiver fields and constructs I through 0xF8690 with the Game pointer. That helper initializes I+0x308 to zero and the two bytes at I+0x550/+0x551 to zero. The constructor clears exactly 0x100 raw key bytes at G+0x1A87. This is not evidence that every neighboring byte is cleared or that focus loss clears keys. Binding initialization/configuration and the later self-copy/save are described separately below.

In ordinary application construction, the call at 0x136331 reaches 0x7EA80; its result is published to the global Game pointer at RVA 0x551A80 at 0x13633E. The GUI pointer passed as the fifth constructor argument comes from global 0x551AC0. The window procedure reloads these globals at its individual dispatch sites. This establishes the selected Game/interface aliasing for the input routes; it does not prove all alternate receivers or all global writers.

## Complete window-message boundary

The 32-bit message identifier selects the following behavior in 0x133C70. The input cases do not inspect repeat, transition, scan-code or coordinate bits in lParam. There is no repeat suppression in these cases.

| Message | Ordered native work and return |
| --- | --- |
| 0x100 | GUI keydown 0x340DF0 with the low 16 bits of wParam first; reload global Game; Game keydown 0x96E70 with low 8 bits; return zero. |
| 0x101 | GUI keyup 0x340E60 with low 16 bits first; reload Game; Game keyup 0x97870 with low 8 bits; return zero. |
| 0x102 | GUI character 0x340DD0 with low 16 bits first; reload Game; Game character 0x9F670 with low 8 bits; return zero. |
| 0x200 | Return one. There is no call to Game motion and no cached-coordinate ingestion here. |
| 0x201 / 0x204 / 0x207 | Call SetCapture on the supplied window, then Game press 0x94900 with 32-bit button 0 / 1 / 2; return one. |
| 0x202 / 0x205 / 0x208 | Call ReleaseCapture, then Game release 0x965E0 with button 0 / 1 / 2; return one. |
| 0x20A | Extract the signed high 16 bits of wParam, reduce them to -1, zero or +1, call Game wheel 0x97CC0 with that signed 32-bit value; return one. |

The three intervening double-click messages 0x203, 0x206 and 0x209 take the default DefWindowProcW route; they do not call these press/release owners. Other unspecified messages also take that default route. Messages 7 and 8 return one without clearing the Game raw-key array, GUI key set, button mask, chat byte or override pointer in this owner. Message 5 calls 0x135310 with zero and returns one. Message 0x10 calls DestroyWindow and returns one. Message 2 posts a zero quit code when global byte 0x551A8E is zero; otherwise it clears that byte, then returns one.

SetCapture/ReleaseCapture return values are not checked before the Game call. No pressed-button count or last-button test gates ReleaseCapture. This specifies calls made to imported OS providers, not guaranteed OS delivery/capture policy. In particular, release does not itself clear U+0x168 in the selected GUI owners.

GUI key/character callbacks occur before the Game receiver is reloaded from the global. Their effects can therefore alter the state seen by the Game phase of the same message. The original bodies provide no transaction or lifetime snapshot encompassing both phases.

## GUI keyboard/focus phase

0x340DF0 stores the zero-extended 16-bit key to the 32-bit U+0x1AC field, inserts the key into the set at U+0x198 through 0x3380E0, then reads the current U+0x170 focus pointer. A nonnull focus control with nonzero byte +0x1A0 receives virtual slot +0xC0. Null focus or a zero enabled byte ends this phase. There is no current-root, ancestor-selected or Game binding-capture test around that virtual call.

0x340E60 stores the same-width key to U+0x1AC, removes it from the set through 0x33F010, then rereads current focus and the enabled byte and calls virtual slot +0xC8 when admitted. 0x340DD0 performs the focus/enabled test and calls slot +0xD0 with the character unit; it does not update that key set or U+0x1AC here.

These are interface slots on the actual focus receiver. Their concrete control implementations, bindings and possible reentrancy are not universally resolved by these dispatcher bodies. This contract does not import a generic editor behavior for every focus control. The separate Game chat editing closure below is directly reached and independently traced.

## Complete Game keydown and keyup behavior

0x96E70 zero-extends the low byte of its key argument and first writes byte one at the corresponding G+0x1A87 raw-key location. It then handles the following numeric keys, without a GUI root, focus, chat or physical-binding gate before their effects.

| Low-byte key | Direct ordered effects |
| --- | --- |
| 0x1B | Test G+0x420 and replace it with one if it was zero, otherwise zero. |
| 0x70 | Toggle the current selected integer of the node of widget G+0x390, normalizing the result to zero/one. |
| 0x71 | Divide binary32 G+0x664 by 1.25; store that intermediate; apply the lower bound word 0x3ED1B716; call 0x101BD0 on I with current signed 32-bit G+0x1D60/+0x1D64; reread G+0x664 and copy its word to G+0x1BCC; save options through 0x13B1D0 at G+0x1B88. |
| 0x72 | Multiply G+0x664 by 1.25; store; apply the upper bound word 0x40435000; then the same layout/copy/save sequence. |
| 0x73 | Toggle the selected integer of current U+0x140 root, normalizing zero/one. |
| 0x74 | Divide binary32 G+0x1BD0 by word 0x3F8CCCCD; store; apply lower bound 0x3F40562A; save options. |
| 0x75 | Multiply G+0x1BD0 by word 0x3F8CCCCD; store; apply upper bound 0x3FAA5E36; save options. |

The 0x71/0x72 factor is word 0x3FA00000. Approximate bounds are 0.40959996, 3.0517578, 0.75131476 and 1.3310001. The lower-bound compares keep the computed result when ordered at/above the bound or unordered; the upper-bound compares keep it when ordered at/below or unordered. Only the opposite ordered side selects the literal bound. These statements assume the compare completes; floating exception masks and signaling-NaN traps are not established.

Only keys 0x71, 0x72, 0x74 and 0x75 call the options writer in this switch. The layout helper is synchronous and makes further layout/component calls before the subsequent G+0x664 reread and options save. The copy after layout is not a pre-call cached value. This contract bounds the layout/metric closure rather than predicting every widget coordinate.

The complete 0x13B1D0 body opens the literal options.cfg route for output through its stream helper and emits named scalar lines in this order: fullscreen, resolutionX, resolutionY, refreshRate, antiAliasing, renderDistance, cpuPerformance, soundVolume, musicVolume, cameraSpeed, cameraSmoothness, language, invertY, minTimeStep, smallGUI, vsync, showRarity, musicLoop, guiScale, mapScale, antiAliasingSamples. The two scale fields are loaded and passed as binary32 to the selected output overload; any further internal conversion is a provider boundary. Stream formatting, actual file success and storage durability remain provider boundaries. The key owner does not check a save-result flag.

After this switch, keydown separately considers chat. It requires the selected integer of direct node G+0x220 to be nonzero, 0x342090(U) to return a null focus pointer, and the signed 32-bit field at widget G+0x658+0x2E0 to be negative. Failure of any gate leaves the already performed raw-key and switch effects intact.

When these gates pass and the current chat +0x1D8 byte is zero, only key 0x0D activates it by writing one; that activation branch does not forward the key to 0x26B300. Other inactive-chat keys do no chat work. When the chat byte is nonzero, key 0x0D first sets chat+0x15C to one and clears the current chat+0x1D8 byte. For nonempty current text it constructs an input stream and extracts the first 16-bit-string token through 0x6D6F0. The exact token comparisons and effects follow in order:

* /check toggles byte +0x1F0 of widget G+0x388 and adds a formatted status to chat through 0x26BF10.
* /namepet extracts another token. Length zero does nothing; lengths 1 through 15 call 0x10A3C0 only if the current P+0x8B8 type byte is 0x14; length 16 or greater adds the literal length error to chat.
* /sit, /wave, /pet and /dance each call 0x4F220 on the current player. Its signed integer result is converted to binary32 and must be strictly less than current P+0x6C. Success stores action byte P+0x68 as 0x57, 0xA1, 0xA2 or 0xA3 respectively, then stores binary32 zero at P+0x6C.
* A token not matching these commands creates a record containing current P+0x8 identity and a copy of the entire current chat text, and passes it to 0xA1610 at G+0x21A0. This proves a record insertion call, not immediate transport delivery.

The text/token/provider helpers' parsing, command-helper internals and eventual queued-record consumers are excluded. After this active Return handling and temporary cleanup, the current chat string is assigned length zero. The active-chat path then reaches 0x26B300 with the key, including Return; other active keys reach it directly. Empty Return still deactivates and clears text and reaches this helper. Calls can change current widget/player pointers; the loads stated as current are rereads in the owner.

0x97870 performs exactly the raw-key clear: low-eight-bit key index, one byte zero at G+0x1A87+index. It has no F-key reversal, Escape reversal, chat work or physical action-array update.

## Complete Game character and directly reached chat editor

0x9F670 first queries 0x342090(U). A nonnull focus pointer ends its Game-specific character work. Otherwise it reads current chat+0x1D8; a nonzero byte admits 0x26B170. The Game argument is not the window's original 16-bit character: the low eight bits are sign-extended before this helper receives the low 16-bit unit. Consequently a low byte 0xE9 becomes unit 0xFFE9 on this Game chat route. The preceding GUI route still received the original low 16 bits of wParam. There is no G+0x220 or G+0x658 binding-capture check in this character owner.

0x26B170 clamps first through 0x26B600, rejects units below 0x20, constructs a one-unit 16-bit string, erases an existing nonzero selection through 0x26A8C0, clamps again, inserts at the signed 32-bit caret +0x1E8 through 0x3DF40, and increments that caret by one. Current text is the string aggregate beginning +0x1B8 with 64-bit length at +0x1C8; selection is signed 32-bit +0x1EC. Imported/runtime allocation and string failure behavior are bounded, not reimplemented here.

0x26B600 clamps a negative caret to zero and an over-length caret to the signed low 32 bits of text length. It then tests the signed 32-bit caret-plus-selection endpoint. A negative endpoint changes selection to negative caret. An endpoint above length changes the caret to length minus selection. It does not simply normalize every selection into an independent nonnegative count. No arbitrary oversized/malformed string or signed-overflow safety promise follows.

0x26A8C0 calls that clamp, then, when caret is no greater than length, erases through 0x3DE90. A zero selection asks to erase one unit at the caret; a positive selection starts there with that count; a negative selection starts at caret plus selection with the negated count. A still-negative selection moves the caret by that selection afterward; selection is then zero. The runtime erase helper determines any remaining out-of-range behavior.

0x26B300 clamps and sets chat+0x15C to one before key-specific handling. Return then ends without another edit. Left 0x25 moves the caret down one only above zero; on a successful move, held GUI key 0x10 increments selection, otherwise selection becomes zero. Right 0x27 similarly moves only below length; held GUI key 0x10 decrements selection, otherwise clears it. An edge at which no movement occurs preserves the previous selection. Delete 0x2E calls the erase helper even with zero selection. Backspace 8 erases a nonzero selection, or, when selection is zero and caret positive, erases the preceding unit and decrements caret. Home 0x24 adds old caret to selection when GUI 0x10 is held, otherwise clears selection, then sets caret zero. End 0x23 adds old caret minus length to selection when held, otherwise clears selection, then sets caret to length. Other keys make no further edit.

GUI Shift membership uses the unsigned 16-bit key in the U+0x198 tree, including 0x9AEB0(U,0x10). This is not a physical OS query and is not the Game raw byte G+0x1A97, although both can be changed by a window-key sequence.

## GUI pointer targeting, callbacks and interface entry

0x33F1E0(U) first returns nonnull U+0x168; otherwise it calls 0x333F60 on current root U+0x140, the cached point at U+0x180 and flags zero. A nonnull result therefore need not mean capture: it can be an ordinary hit. 0x333E70(N) reads that node's manager and its cached point, calls the same hit routine on N with flags zero and returns whether a node was found. It is a point-query admission, not a generic ancestor-visibility predicate.

The complete hit routine 0x333F60 rejects a node with selected integer zero, node-flag bit 2 set, or node-flag bit 12 set. A selected clipping route also depends on component N+0x70, shape N+0x58, manager flags at +0x288 and component flag bit 8 at +0x150. Its rectangular rejection uses strict outside comparisons; equality is admitted at that stage. It prepares the local transform from N+0x60 or the identity state, composes a parent transform through 0x100EE0/0x173E0 when there is a parent at N+0x40, transforms/projectively divides the point, and calls the reached shape/control virtual query boundaries. Shape slot +0x20, component slot +0x28 and component flag bit 8 participate in the own-hit decision; shape slot +0x28 and the control's +0x38 subobject virtual slot +8 are additional point-query boundaries. Children are considered in reverse linked order when the selected integer from component array +0x168/index +0x134 is zero or the node has its own hit. The first successful child wins. After children, query flags 4, 1 and 2 govern node-flag bits 3, 0 and 1 respectively. Ordinary flags-zero queries require bit 1 clear and an own hit for the node itself. Transform, shape and alternate virtual-query numerics are explicit dependencies; no universal rectangle, font or OS hit convention is imported.

For widget hit checks used by the inventory helpers, 0x32B0C0 tail-calls 0x32AE90(widget, widget+0x178 node). This separate recursive boolean query requires node flag bit 2 clear, a nonzero selected integer, and node+0x70 either null or equal to the widget receiver. It prepares the selected node/parent transform, reads the widget's manager cached point, calls the widget +0x38-subobject slot +8, then tests shape slot +0x20 or component slot +0x28. A successful own test returns one. Otherwise it recurses over children in reverse order only when the component +0x168/index +0x134 integer is zero, returning on the first success; other routes return zero. It does not compare the widget against a global topmost target.

0x101020(I, button) copies the current cached GUI Y word to I+0x548 before dispatching. Button zero tail-calls GUI 0x340EC0, button two tail-calls 0x341300, and button one calls 0x3415F0. After the button-one call returns, it reloads I's Game and GUI pointers and calls 0x33F1E0. If that result is null, it clears I+0x308/G+0x420, then clears selected integers of the nodes of widgets I+0x218, I+0x228 and I+0x230, in that order. These correspond to G+0x330, G+0x340 and G+0x348. An unrecognized button still made the cached-Y copy but does not call a GUI button helper.

0x101120(I, button) dispatches zero to 0x3410E0, one to 0x341780, and two to 0x3414A0, with no other button work. There is no equivalent empty-target Escape-byte clear in this release adapter.

The GUI button helpers first set or clear their bit in the 32-bit U+0x1A8 mask, then select the target from U+0x168 or a flags-zero current-root/cached-point query. A target's +0x78 pointer is the direct control receiver. A nonnull control with nonzero +0x1A0 enabled byte receives the direct virtual event. Node flag bit 1 does not additionally gate that direct virtual call. After it, the binding phase rereads target-node fields and requires node flag bit 1 clear, a nonnull current +0x78 control and a nonnull binding object from 0x32A210(control,event). Enabled controls pass; a disabled control can still pass if the binding's byte +8 is zero. The binding is invoked through its own virtual slot zero.

| GUI owner | Button-mask change | Direct control slot | Binding event | Final root broadcast slot |
| --- | --- | --- | --- | --- |
| 0x340EC0 | Set bit 0 | +0x58 | 2 | +0xE8 through 0x33AAD0 |
| 0x3410E0 | Clear bit 0 | +0x60 | 3 | +0xF0 through 0x33AADC |
| 0x3415F0 | Set bit 1 | +0x70 | 5 | +0x100 through 0x33AA88 |
| 0x341780 | Clear bit 1 | +0x78 | 6 | +0x108 through 0x33AA94 |
| 0x341300 | Set bit 2 | +0x88 | 8 | +0x118 through 0x33AAA0 |
| 0x3414A0 | Clear bit 2 | +0x90 | 9 | +0x120 through 0x33AAAC |

0x32A210 searches the control's binding tree at +0x188 by signed 32-bit event identifier and can insert a default entry when missing. The object pointer is the entry payload at +0x28. A missing event lookup is not proved to be a read-only operation. The dispatched callback's effects and lifetime remain concrete callback boundaries; no assumption that it leaves target/control/root unchanged is made.

All three presses clear focus when the target is null or target+0x70 differs from U+0x170. A nonnull old focus first gets byte +0x15C set to one and event 0x13 through 0x342560 on its node; U+0x170 is cleared after that callback. A callback can therefore run before this later focus clear. The comparison uses target+0x70, not target+0x78. 0x342560 itself applies node-flag bit 1, binding existence and enabled/binding-byte admission, without the root-selected or node-flag-bit-2 traversal gates.

Left press has additional work before focus/direct/binding dispatch. A target control with flag bit 5 at +0x150 and nonzero enabled byte is assigned to U+0x1B0; then 0x33C360 broadcasts the callback at 0x33AAC4, whose concrete thunk tail-calls component slot +0xE0. Otherwise U+0x1B0 becomes null. If a popup is present and 0x3308C0 does not find the target within its node's parent chain, 0x342270(U,0,0) hides the old popup node by setting its selected integer to zero and clears U+0x178. Null-target behavior follows the same tested parent-chain route. The generic nonnull-popup placement branch of 0x342270 is not reached by these calls and is not assigned a numerical layout contract here.

Left release, after choosing its initial target, hides a popup through the same zero-argument call when both U+0x178 and U+0x1B0 are nonnull. After direct release and binding event 3, it rereads U+0x168 or does a new hit query. If this target exists, it performs another ordinary hit query without the override, compares the two +0x78 controls, and, if different, sends the override/selected control virtual slot +0xB8 followed by binding event 0xE on its node. Other releases do not perform this additional leave comparison.

The final broadcasts in the table reload current U+0x140. Root and recursive nodes require nonzero selected integer, node-flag bits 2 and 1 clear. A nonnull component at node+0x70 receives its event before children. Children are traversed forward through the linked list at node+0x48, using payload node pointers at link+0x10. 0x33C360 copies the callback carrier for recursion. This is component-first forward traversal, unlike reverse child hit selection. No thread lock, mutation-safe snapshot or callback non-reentrancy guarantee is supplied by these bodies.

For GUI motion 0x3418D0, old current coordinate words are copied to U+0x188/+0x18C before new binary32 words go to U+0x180/+0x184. The target is override-or-hit. The old control comes through U+0x160+0x78; the new through the selected target+0x78. If they differ, an enabled old control gets virtual +0xB8, then the old-node binding event 0xE is resolved using a reread of U+0x160. An enabled carried new control gets +0xB0, then binding event 0xD uses current fields of the carried target node. A nonnull target then gets enabled carried-control slot +0xA8 followed by binding event 0xC using reread node fields. Only after these callbacks is U+0x160 overwritten with the carried target. The final root broadcast is slot +0x130 through 0x33AAB8 with the same forward selected-tree gates.

## Complete Game press: GUI first, then current Game state

0x94900 saves the 32-bit button, calls 0x101020 first and then snapshots whether widget G+0x360's current node-selected integer is nonzero into global byte 0x5514D7. This byte is not a per-button ownership record. If G+0x668 is nonzero, it then point-queries direct nodes G+0x3A8 and G+0x3B0 in that order; a hit from either returns from the entire Game owner. These returns happen after GUI dispatch and the global snapshot, and before its time sample, movement-byte clear, inventory work and common scalar postlude.

The following initial branches precede the common press-time work.

Button two with G+0x668 nonzero tests signed 64-bit M+0x110 at least zero. If admitted, it queries 0x35EA0(W, converted X, converted Y, mode zero) from M+0x100/+0x108 fixed-point coordinates divided by 65536 toward zero. The returned binary32 height is replaced by zero when ordered nonpositive or unordered if the compare completes, then truncated to a signed 32-bit integer. XY are likewise narrowed to 32-bit coordinate integers; their region pair is divided by 4096 toward zero. 0x7C280 on P's CharacterData pointer at +0x1528, subobject +0x108, selects/obtains the region entry. The linked list through entry+0x28 is scanned forward for the matching 32-bit XY pair; the first match is unlinked, the entry's 64-bit count at +0x30 decremented, and that link freed. If absent, 0x6E6C0 constructs the selected new record and the owner increments/relinks the count/list; the reached count-limit path raises the original length error rather than completing normally. After this coordinate branch, a still-nonzero current G+0x668 toggles byte G+0x1E54. A negative M+0x110 skips the coordinate query/list work but can still reach that toggle.

Otherwise button zero, G+0x1DB0 zero, G+0x668 nonzero and signed 32-bit M+0x150 different from 0x7FFFFFFF admit the following test. It calls 0x5F310 on current P. A false return and zero M+0x158 byte take the cost route: 0x2ADCE0(W,&M+0x150) supplies a signed 32-bit cost, compared with P+0xAAC. Insufficient value adds the literal money warning to chat and calls the sound boundary 0xA0420 with identifier 0x34. Otherwise it subtracts cost, inserts the selected M+0x150/+0x154 pair through 0x745E0 at CharacterData+0x28, then calls the sound boundary with 0x3D.

The other predicate side copies M+0x150/+0x154 into P+0x1B0/+0x1B4, then P's three current 64-bit position words +0x10/+0x18/+0x20 into +0x1B8/+0x1C0/+0x1C8, stores action byte +0x68 as 0x9C, stores 32-bit zero at +0x6C, and clears G+0x668 then G+0x420. These are direct synchronous writes before later common press work. No inferred back/navigation action is needed to describe them.

All surviving buttons, including values outside zero/one/two, next sample timeGetTime. The unsigned 32-bit difference from global timestamp 0x5514E8 is formed and the new sample stored there before admission. A difference strictly below 200, current G+0x668 nonzero and signed 64-bit M+0x110 strictly positive copy M's three 64-bit coordinates to G+0x1E08/+0x1E10/+0x1E18, recompute height through 0x35EA0 at the new XY with mode zero, multiply its binary32 return by 65536 and truncate to signed 64-bit G+0x1E18. The 32-bit G+0x1E04 is decremented, then a negative result becomes zero, and byte G+0x1E38 becomes one. Exactly 200 fails. The difference uses modulo-2^32 subtraction. There is no same-button, same-position or double-click-message predicate here; the earlier global timestamp's initialization/other writers are not universally enumerated.

Next G+0x1DB0 is zeroed. The owner calls 0x5FAC0 on current P and carries the returned binary32 scalar across the subsequent item/UI branches for the common P+0x180 postlude.

### Press selection and item branches

0x94600 supplies two signed 32-bit row/slot indices, or the 64-bit all-ones pair. It requires the node of widget G+0x330 selected, a successful 0x32B0C0 widget point query, a nonnegative in-range row against P+0x9F0/+0x9F8's array of 0x18-byte rows, a nonnegative in-range slot against the row's array of 0xA4-byte count/item records, and a nonzero 32-bit count. The count precedes an item beginning at record+4. The helper rereads current selected indices +0x270/+0x274 when publishing the pair.

Button two uses this helper, and, for a valid pair, permits the item only when its type byte is outside 0x16, 0x17 and 0x18, and P's action byte is not 0x56 or 0x4DF60(P+0x1D8) returns zero. It builds/copies a record through 0x9CD0 and 0x7330 into the list at P+0xAB8, checks the reached count-limit error, increments P+0xAC0 and links the record. Only then it decrements the original slot's 32-bit count. A signed result below one clears count, item type byte and the 32-bit item field at record+8. It then goes to the common scalar postlude. This is no proof of immediate delivery or later entity behavior.

Buttons zero and one use the same index helper. A valid pair first checks widget G+0x360's selected state; if selected, it writes the two indices to that widget+0x1E8/+0x1EC and bypasses item transfer. Otherwise it tests widget G+0x350. When that is not selected, a selected widget G+0x370 admits only type bytes 3 through 9 inclusive or 0x1A and stores the item pointer at that widget+0x1E8; other types call the 0x34 sound boundary.

With neither of these selected routes, 0x50640(P,item) must succeed; failure calls the 0x35 sound boundary. A now-zero count skips further transfer. The remaining type-specific owner effects are:

| Item type byte | Selected direct route |
| --- | --- |
| 3 | Copy the item through 0x35400, decrement/clear the source count first, then perform the two-slot selection described below at P+0x638/+0x6D8. |
| 0x14 | Copy item; return the old P+0x8B8 item through 0x4E810(P+0x9F0,...,1); copy the new item there through 0x9CD0; then decrement/clear source count. |
| 0x15 | Require action byte zero, or signed 0x4F220 return converted to float strictly below P+0x6C. Copy item to P+0x1D8, store action 0x56 and +0x6C zero, then clear G+0x330 widget-node selected integer. There is no direct source-count decrement in this branch. |
| 0x1A | Copy item; decrement/clear count; return old P+0x3B8 through 0x4E810, but copy the new item to P+0x278. These two offsets are deliberately different. |
| 4 / 7 / 8 / 5 / 6 | Copy item; decrement/clear count; return the old target item and copy to P+0x3B8 / +0x598 / +0x318 / +0x4F8 / +0x458 respectively. |
| 9 | The same sequence, using P+0x818 for button zero or P+0x778 for button one. |
| 1 / 0x18 / 0x1B / 0x1C | Calls 0x2ABB70(W), then 0xB7740(G,slot record), then 0x2B9890(W). Their gameplay closure is not expanded. |
| Other type | No further item-specific work in this owner. |

Here and below decrement/clear means a 32-bit decrement store followed by the signed-less-than-one test, then count zero, item-type byte zero and record+8 DWORD zero when clearing. This is machine-width behavior, not a claim of checked arithmetic. The transfer helpers can have additional effects; they are not replaced with guessed container semantics.

For type 3 the copied 32-bit subtype is used unsigned. The mask-selected subtype set is 5, 6, 7, 8, 10, 11, 15, 16, 17, 18 and 23, bounded below 24. Subtypes 13 or 14 always use P+0x638, returning/clearing an old P+0x6D8 item first only when its type is 3 with subtype in that mask set, then returning the old +0x638 item before copying the new one.

For subtype 6 or 7, button zero returns old +0x638, copies the new item there, returns old +0x6D8 and clears +0x6D8. Button one returns old +0x6D8, copies there, then returns/clears +0x638. These cases precede the raw-key alternative.

For other subtypes, button zero with zero raw Game byte G+0x1A97 uses +0x6D8; the complementary button-one or nonzero-raw-key route uses +0x6D8 when the new subtype is in the mask set, otherwise +0x638. A copied mask-selected type-3 item going to +0x6D8 returns/clears old +0x638. A new item going to +0x638 returns/clears old +0x6D8 only if that old item is type 3 with a mask-selected subtype. Explicit empty-item construction and 0x9CD0 copies perform those clears; the owner does not zero whole entity spans. Each completed type-3 transfer calls 0x280A40 and 0x280C80 on widget G+0x330, the 0x5B sound boundary, then 0xB2180.

The other ordinary target-copy branches also refresh through 0x280A40/0x280C80 and sound 0x5B. The exact Game helper used for sound differs at joins between 0xA0420 and 0xA03F0. Their provider-internal effects are excluded. Type 0x15 has its own earlier exit to common work.

When widget G+0x350 is selected for a valid indexed pair, rows 1 and 5 are rejected with sound 0x34. Other rows copy the item, decrement/clear its source, add signed 0x10A390(item) to current P+0xAAC, call sound 0x3D, append a count-one item record to G+0x21B0, pop through 0xA0720 if its 64-bit count exceeds ten, then call 0xB75D0 and 0xB2180. Reached insertion count limits have the original length-error exit.

For an invalid indexed pair, 0x94260 tests G+0x330 widget selected and point-queries the ten widgets in G+0x3C8's pointer array in ascending order. The first hit returns the corresponding current P item pointer: +0x638, +0x6D8, +0x778, +0x818, +0x318, +0x598, +0x3B8, +0x4F8, +0x458, +0x8B8. No hit returns null. A nonnull, nonzero-type item follows these priorities: selected G+0x350 adds its price to P+0xAAC, calls sound 0x3D, appends count one to G+0x21B0 with the same ten-record bound, refreshes through 0xB75D0, then clears the source type byte and source bytes +4 through +7; otherwise selected G+0x370 admits/stores only types 3..9 or 0x1A, with sound 0x34 for rejection; otherwise 0x4E810 returns the item to P+0x9F0, those source fields are cleared, widget G+0x330 is refreshed, sound 0x5B and 0xB2180 follow. This is a selected P item-field pointer route, not proof that a separate cursor aggregate was addressed.

A null/zero-type result instead calls 0x94750. That helper requires G+0x350 widget selected, its own 0x32B0C0 query true, and its +0x270/+0x274 row/slot within the G+0x1870/+0x1878 0x18-byte rows and 0xA4-byte item records. It returns item+4 without the nonzero-count test of 0x94600. A null result leaves item transfer unchanged.

For a returned item with G+0x350+0x2B4 not equal to one, the owner checks price from 0x109D30 against P+0xAAC, then 0x50670 capacity. Insufficient value/capacity emits the selected warning and sound 0x34. Success adds through 0x4E810, subtracts a fresh 0x109D30 result, records the item under CharacterData+0x138 through 0x7BFF0/0xA17D0, calls sound 0x3C, refreshes widget G+0x330, then calls 0xB75D0, 0x9D6F0 and 0xB2180 in order.

When +0x2B4 equals one, the G+0x21B0 forward list is located by decrementing a counter initially taken from its 64-bit size until it matches widget+0x274. A match processes at most its positive 32-bit count, checking 0x50670 before each attempt. A capacity failure emits warning/sound 0x34 and leaves the loop. Price from 0x10A390 must be no greater than current P+0xAAC for an attempt to add through 0x4E810, subtract a fresh price and call 0x9D6F0. An unaffordable attempt is skipped while the attempt counter still advances. Only successful additions reduce the stored record count. A zero remainder is unlinked, the list size decremented and the selected deleting helper called. Sound 0x3C, 0xB75D0 and the two G+0x330 refresh calls then follow, including a zero-success matched route.

### Press common UI and scalar tail

Only button zero performs the later G+0x340/G+0x368 work. Selected G+0x340 first calls 0x280E30, then validates its signed +0x278/+0x27C pair against G+0x17A8/+0x17B0's 0x18-byte rows and 0xA4-byte elements. Invalid indices assign an empty aggregate through 0x834A0 to G+0x348 widget+0x1F8 and set that widget's two DWORDs +0x2B8/+0x2BC to -1. Valid indices construct a temporary through 0x7EA10 and call 0x2AEC30(W,item,out,byte(row==6)). A false return gives the same empty/reset result; success moves through 0x834F0 and publishes the pair. Temporary teardown is through 0x81870. No terrain/item transformation meaning is inferred for 0x2AEC30.

Next selected G+0x368 requires nonnull 0x294CC0. A nonnegative widget+0x2D8 calls 0x294C70. Otherwise negative widget+0x2C4 reloads 0x294CC0 and calls 0x292650 with the byte at return+0x1C; nonnegative +0x2C4 calls 0x294CA0. These remain reached widget-helper boundaries.

The common tail calls 0x5FAC0 again, divides its binary32 return by the carried earlier return, multiplies that rounded quotient by the then-current P+0x180 and stores P+0x180. It calls 0x5FAC0 once more, rereads current P+0x180, and retains the current value when the returned cap is ordered at least that value; the other compare side, including unordered when the compare completes, selects the returned cap. The resulting word is stored. There is no additional zero denominator, finite-value or nonnegative clamp in this owner. It then clears the current chat+0x1D8 byte and the selected integer of direct node G+0x310. The earlier point-hit and original length/error exits do not reach this normal tail.

## Complete Game release: work before and after GUI callbacks

0x965E0 performs a direct-node/menu test before calling the GUI release adapter. The selected integers of direct nodes G+0x118 and G+0x120 must both be nonzero, and button must be zero. It then reads the signed 32-bit selector at widget G+0x128+0x1F0. Negative or values above three do no selected action. Zero clears the selected integer of node G+0x118, then sets direct node G+0x130 to one. One sets the selected integer of the node of widget G+0x650 to one. Two stores byte one at G+0x1D68. Three calls 0x9BB20 with G, signed 32-bit G+0x2264 and current P; then clears direct node G+0x118, stores binary32 8 at G+0x1DB8 and G+0x1DB4, and calls 0x2E0B50(W). Those decoder/World helper bodies are boundaries in this input contract. This is a real pre-GUI call site; it is not a deferred callback inferred from a menu name.

Only after this work does 0x101120 dispatch the selected GUI release, including all its direct and bound callbacks. The rest of the Game owner then reads current state:

1. Selected widget G+0x378 and button zero call 0x94840 for a temporary value, read the widget's 64-bit +0x200 identifier, and search W+0x450's tree by signed 64-bit key. The found entity payload or null, plus that temporary value, go to 0x89CB0. Temporary-string cleanup follows. This owner does not manufacture a replacement entity on lookup failure.
2. Selected widget G+0x360, nonzero global 0x5514D7, and button zero admit the item operation below. Both current selected state and the global press-time snapshot are required. A matching press/button identity is not recorded by that byte.
3. Only button zero reaches the G+0x668/M-coordinate operation below; other buttons continue to the later G+0x370/binding-control tail.
4. Selected widget G+0x370 with button zero calls 0x268670; true calls 0x267930. It then calls 0x268540 and clears that widget-node selected integer if true.
5. Every normal route calls 0x2761F0 on the current widget G+0x658. After that call, current widget/node selected state is reread. Selected, button zero and signed +0x2DC nonnegative call 0x276F30.

The G+0x360 operation first calls 0x277D70. A true return, 0x277BC0 cost no greater than current P+0xAAC, widget+0x1F0 equal to one, nonnegative/in-range widget+0x1E8/+0x1EC against current P inventory rows/slots, nonzero 32-bit count, and true 0x277E30 are all required for its transfer. It subtracts a fresh 0x277BC0 result, copies the current item through 0x35400, then decrements and signed-tests the source count before any random call. Count below one clears count/type/record+8 DWORD.

It then makes one directly visible rand call. The sign-adjusted low-bit calculation chooses 0x2D15D0 for a zero remainder and 0x2D0680 otherwise. For both branches, machine arguments prepared before the branch include current W, a branch-specific temporary destination, the copied item's byte at +0x14 zero-extended, 32-bit 0xFFFFFFFF and a stack byte zero. Their meanings and any transitive draws are not inferred from these operands. The returned item is copied through 0x35400. If its type byte is at most 0x1E and the corresponding bit in mask 0x454083FC is set, the original copied item's eight-byte field at +0xC is copied to the result's +0xC. It adds the result through 0x4E810(P+0x9F0,...,1,0xFFFFFFFF), refreshes widget G+0x330 through 0x280A40/0x280C80, calls sound 0x31, then calls 0x9D6F0 with current P.

After this admitted G+0x360/button-zero route, 0x277CD0 is called even when one of the inner transfer predicates failed. A true result clears selected integers of widgets G+0x360 then G+0x330. No provider success is invented beyond the tested returns.

The G+0x668 coordinate operation requires current G+0x668 nonzero, G+0x1DB0 zero and M+0x15C not 0x7FFFFFFF. It converts signed 32-bit X at M+0x15C, then Y at +0x160 into a signed coordinate pair: nonnegative coordinates divide by 16; negative coordinates increment by one, divide toward zero by 16, then decrement by one. For ordinary signed 32-bit inputs this is floor division by 16. It searches CharacterData+0xA8 using signed lexicographic X-then-Y 32-bit keys at tree-node+0x1C/+0x20. No match makes no selected player write. A match stores P+0x68 as 0x75 and P+0x6C as zero; reloads M and converts Y then X again; copies X and Y as 32-bit words to current P+0x1D0/+0x1D4; then clears G+0x668. Coordinates published after the writes are reread, not a saved copy of the lookup pair.

0x2761F0 sets widget+0x2E0 to signed 32-bit -1. 0x276F30 copies widget+0x2DC to +0x2E0. Their order makes release clear a previous capture index before conditionally selecting the current nonnegative +0x2DC index for subsequent polling. This has no dependency on a keyup message in these owners. The +0x2DC producer and complete widget selection/class lifetime are not universally enumerated.

## Complete Game motion and its repeated admission reads

0x1014F0(I) returns low byte one as soon as any following condition is true, testing in this order. A direct-node or widget-node test means nonzero selected integer; byte tests use nonzero.

| Order | I-relative operand |
| --- | --- |
| 1–5 | Direct nodes at +0, +0x18, +0x58, +0x80, +0xE8 |
| 6 | Byte +0x550, alias G+0x668 |
| 7–10 | Nodes of widgets +0x218, +0x228, +0x270, +0x238 |
| 11 | Byte +0x308, alias G+0x420 |
| 12–14 | Nodes of widgets +0x258, +0x250, +0x248 |
| 15 | Byte +0x1D8 of widget +0x280, alias current Game chat flag |
| 16–19 | Nodes of widgets +0x260, +0x240, +0x540, +0x538 |

Only when all tests fail is the low result byte zero. Upper return-register bytes are not a Boolean contract. This helper performs neither an ancestor walk nor a point hit nor a focus/key-binding query. It is the actual source admission used by motion and selected physical cursor work.

0x97880 first calls that helper. A true result truncates its binary32 Y argument to signed 32-bit first, then X to signed 32-bit, and calls 0x101160(I,X,Y). The adapter converts those integers back to binary32 and calls GUI motion 0x3418D0 before its later time/layout work. Thus this route quantizes the Game arguments before publishing GUI cached coordinates.

After the adapter returns, nonzero current G+0x1958 returns from the whole Game motion owner. This early return still leaves already performed GUI coordinate/callback and adapter work intact. Otherwise current U is reloaded. A nonzero or unordered binary32 difference in cached X, or failing that in cached Y, writes G+0x1DB0 one. Two finite zero differences leave the previous byte unchanged; they do not clear it.

Current G+0x668 and GUI right bit 1 admit the selected M/coordinate movement route. The owner prepares the matrix and calls 0x13050 using G+0x1D90 converted from binary64 to binary32. It captures cached Y difference and X difference and negates the carried Y difference before the imported powf call with binary32 base 2 and converted signed G+0x1E04 exponent. The returned scale multiplies carried X first, then carried negated Y; a third component explicitly multiplies scale by binary32 zero. For G+0x1E08, matrix offsets +0/+0x10/+0x20 multiply these three components, with the X and Y products added first, then the third. For G+0x1E10, offsets +0x14 and +4 supply the Y and X products in that addition order, followed by the +0x24 third product. For G+0x1E18, offsets +8/+0x18/+0x28 use X, Y and third products with the first two added first. Each resulting binary32 sum is multiplied by 65536, truncated to signed 64-bit and added to its Game coordinate. Byte G+0x1E38 becomes one. The matrix helper/provider numerical closure, exceptional conversions and overflow safety are not supplied here; the explicit scale-times-zero term is not silently omitted for nonfinite inputs.

Selected widget G+0x368 with current GUI right bit then adjusts its binary32 fields: convert signed G+0x1BAC to float, multiply by constant word 0x3BA3D70A (approximately 0.005), multiply current cached Y difference by this rounded factor, subtract from widget+0x1E8 and store. The widget/GUI/speed values are reloaded before the analogous X computation, which adds to widget+0x1F0. The native factor is formed before multiplying the coordinate difference.

After this initial true-admission work, or immediately after an initial false result, the Game owner calls 0x1014F0 again. A false result admits the later camera-scalar route directly. A true result requires GUI left bit 0 and a null return from 0x33F1E0; otherwise the owner returns. Because that query is override-or-hit, null means neither was returned at that query; it is not merely an OS capture condition.

Once admitted, the owner calls 0x1014F0 a third time. True substitutes the then-current cached binary32 differences for the original carried X/Y arguments; false retains the original arguments. The first, second and third reads can therefore observe different state after callbacks. No initial-result caching may be inferred.

For the vertical double field G+0x1D98, native work converts signed camera-speed G+0x1BAC to float, multiplies by the same binary32 0.005 word, multiplies by selected Y and promotes that rounded result to binary64. G+0x1BB4 equal to zero adds it to the double field; nonzero subtracts it. An ordered result above 180 becomes 180, and an ordered result below zero becomes zero. Unordered comparisons leave that value unchanged when they complete. The horizontal double G+0x1DA8 subtracts the similarly ordered float-speed/factor/X product promoted to double, without the vertical clamp. The options writer's original literal names independently associate +0x1BAC with cameraSpeed and +0x1BB4 with invertY; no additional physical-camera interpretation is needed.

### Motion adapter's elapsed/layout tail

After GUI motion, 0x101160 uses a process-static initialization guard for timestamp DWORD 0x551770 and guard state 0x551774. Its first admitted initialization samples timeGetTime; the ordinary path samples timeGetTime again. It subtracts the carried timestamp modulo 2^32, stores the new sample, interprets the difference as signed 32-bit and converts it to binary64. This is not the unsigned press-time 200 test.

0xA7A0 receives that double difference and binary32 word 0x3C23D70A, approximately 0.01. It multiplies the double input by exact 100, truncates to signed 32-bit step count, starts a binary64 accumulator at zero and advances in increasing integer-index order. For each admitted step it converts the index to double, adds exact one, divides by the converted total count, subtracts the current accumulator, multiplies by a precomputed double gain, and adds to the accumulator. Gain is the binary32 second argument promoted to double then multiplied by double word 0x3F947AE147AE147B, approximately 0.02. Eight-step groups and the remainder use this same precomputed-gain association in the original instructions. A nonpositive count performs no steps and returns binary32 zero; otherwise final double accumulation is narrowed to binary32. No exponent/pow substitute, nominal framerate or nonnegative elapsed guarantee is inferred.

Back in the adapter, the old binary32 I+0x548 is carried. It subtracts that value from current U+0x184, multiplies by the returned binary32 gain, adds the old value and stores I+0x548. The carried layout displacement is this new value minus the old one, rounded as binary32.

If direct node I+0x18 is selected and GUI left bit is set, the pointer vector I+0x20..+0x28 is traversed forward with count captured from its extent, rounded up to pointer-sized units and treated as zero for reversed bounds. For each component pointer it captures current signed G+0x1D60, calls 0x32DAB0 for Y, adds the carried displacement, calls 0x32DA30 for X extent, and calls 0x32B560 with signed-half-screen-width converted to float minus half that extent, the adjusted Y, and byte one. Half-screen-width uses the signed divide-by-two machine sequence. After the loop, the current cached Y difference is multiplied by converted signed elapsed, doubled by addition, added to I+0x50 and stored.

Only if that first node/left-bit branch fails does selected direct node I+0x80 with left bit perform the corresponding loop over nodes in I+0x88..+0x90, using each node's +0x70 component, and then update I+0xE0 by the same elapsed/difference sequence. Thus these are mutually prioritized adapter layout branches. Layout/metric callbacks and concurrent list mutation are not proved snapshot-safe by the captured count. Static initialization runtime internals and all alternate calls to this timestamp remain explicit limits.

## Complete Game wheel

0x97CC0 uses the signed 32-bit wheel value, which the window path already reduced to its sign. It checks, in priority order, widget G+0x330, +0x350, +0x340, +0x658 and +0x388. Each must have a nonzero node-selected integer and a successful 0x333E70 query on that node. The first qualifying widget consumes this owner path. Negative/positive calls are respectively 0x27F7A0/0x280270 for the first three, 0x276EA0/0x276F10 for G+0x658, and 0x27B360/0x27B470 for G+0x388. Zero calls neither directional helper and does not fall through to later widgets or the fallback.

Only when none qualifies does it inspect byte G+0x1A7D, action index 13. Nonzero doubles the wheel in 32-bit arithmetic, converts that result to binary32, subtracts it from binary32 G+0x1DB8 and stores. Ordered negative results become zero; ordered above-50 results become 50. An unordered result remains through those comparisons when they complete. This path does not change G+0x1E04.

Otherwise it subtracts the wheel in 32-bit arithmetic from G+0x1E04, applies the native sign-based zero lower bound and signed six upper bound, and stores the result. Unchanged result ends the owner. A changed result proceeds only when signed 64-bit M+0x100 differs from sentinel 0x7FFFFFFF0000. It subtracts G+0x1E08/+0x1E10 from M+0x100/+0x108. A lower new G+0x1E04 uses integer 32768; a higher new value uses -65536, each obtained from the original double constant. Each 64-bit difference is multiplied at machine width, negative products receive 65535, then arithmetic right shift 16 performs the selected toward-zero scale. The resulting offsets are added to G+0x1E08/+0x1E10.

The owner queries 0x35EA0(W,new XY divided by 65536 toward zero,mode zero), multiplies its binary32 result by 65536, truncates to signed 64-bit G+0x1E18, copies all three 64-bit words +0x1E08/+0x1E10/+0x1E18 to +0x1E20/+0x1E28/+0x1E30 in order, and stores byte G+0x1E38 one. These fixed-width operations are not a checked-wide-integer promise. The native query/provider closure is left conditional. Direct alternate callers can supply magnitudes other than one; no generic GUI-wheel dispatch is called here.

## Physical polling, binding capture and message bursts

These are separate original startup-loop facts from 0x1355A0, not extra effects inferred inside the seven Game owners. At the loop's message boundary, nonzero G+0x1D68 calls PostQuitMessage with zero. PeekMessageW uses zero removal flags; while its result equals one, GetMessageW is called. A zero GetMessage result enters teardown; a nonzero result reaches TranslateMessage then DispatchMessageW. After each dispatched message, G+0x1D68 is checked again, then PeekMessageW repeats. There is no one-message-per-update limit in this loop. Negative/error return handling, nested messages caused by imported calls, other message pumps and cross-thread OS dispatch are not promised beyond these branches.

After that burst, zero global active byte 0x536CE8 takes Sleep(16). The active path calls GetClientRect, GetCursorPos and ScreenToClient before its cursor-mode checks. A false 0x1014F0 result can recenter when a signed absolute distance from the client center exceeds 500 on either axis and the foreground window matches the global window. That route clears byte 0x551A8C, converts center to screen coordinates and calls SetCursorPos. A true helper result with zero 0x551A8C converts current cached GUI XY, truncated to 32-bit integers, to screen coordinates and conditionally calls SetCursorPos, then writes 0x551A8C one. Imported cursor positioning does not itself prove a synchronous cached-GUI-coordinate write.

Current active byte and foreground-window equality are tested again before device polling; failure sleeps. Two indirect virtual calls at slot +0x48 request first 0x100 bytes of keyboard state and then 0x10 bytes of mouse state into separate local buffers. Return statuses are not used here as a success gate. Concrete provider acquisition, stale-buffer behavior and device scan-code semantics are excluded; these calls are not replaced by an assumed GetKeyboardState API.

0x9AE50(G) returns low-byte true exactly when the signed 32-bit field at current widget G+0x658+0x2E0 is nonnegative. It does not test the widget's selected node. A false result enters ordinary binding/action polling and Game motion; a true result enters the binding-capture branch instead and bypasses that ordinary polling/motion/controller block at this point.

In ordinary polling, action indices zero through 22 are visited in increasing order. 0xA36C0 first clears the action byte. Its own unsigned index limit is at most 22; it stores only the low byte supplied at G+0x1A70+index. The binding array at G+0x1BDC has 23 eight-byte pairs: 32-bit code then 32-bit kind. Kind zero ignores code zero, accepts unsigned code at most 255 and reads that keyboard-buffer byte; kind one accepts unsigned code at most three and reads that mouse-button byte. Other kinds/codes leave the action cleared. A nonzero source byte is normalized to one by the caller before 0xA36C0; zero becomes zero. Any admitted nonzero source also writes G+0x1958 zero. This is independently polled action state, not replay of Game keydown and keyup.

The caller then evaluates 0x1014F0 again. True obtains cursor coordinates and, when global DWORD 0x551BF0 is zero, converts them to client coordinates. Its two signed integer components are converted to binary32 for Game motion at 0x13683E. False instead converts the two signed 32-bit relative fields at mouse-buffer offsets zero and four to binary32. Thus 0x200 window mouse messages do not supply this ordinary motion call.

After motion, action indices 21 and 22 are cleared before XInputGetState for controller zero. Successful provider branches can set these and other action bytes, set G+0x1958 one, invoke direct Game button press/release calls and invoke wheel. Controller helper/motion behavior and dead-zone/targeting numerics are bounded alternatives, not a second universally specified device contract here. The exact direct caller sites are recorded in the manifest; those direct Game calls are not preceded by the window procedure's SetCapture/ReleaseCapture merely because they reach the same owners.

The binding-capture branch traverses keyboard indices zero through 255. A previous-zero/current-nonzero keyboard byte selects the keyboard transition route: indices 1, 0x1C and 0x9C skip the binding-write call; other indices call 0x8A120 with that index and kind zero. All such keyboard-transition routes then call 0x8A4F0 and leave the scan. If the previous keyboard byte is nonzero or the current one is zero, that index instead checks four mouse buttons in increasing order for previous-zero/current-nonzero and calls 0x8A120 with that mouse index and kind one on each admitted pair. There is no break after such a mouse call; these mouse checks lie inside the keyboard-index loop and may repeat for subsequent nontransition keyboard indices, including held keys. No assumption of one mouse attempt per frame is made.

0x8A120 tail-calls 0x2769A0 on current widget G+0x658. That complete helper requires unsigned widget+0x2E0 below 23. It scans all 23 code/kind pairs at widget+0x1F0 and clears both DWORDs of every pair equal to the supplied pair; then writes the supplied eight-byte pair at the selected index and sets widget+0x2DC to -1. It does not clear +0x2E0. 0x8A4F0 calls 0x2761F0 and therefore clears +0x2E0 to -1. A nonnegative out-of-range index admits the caller's capture mode but fails 0x2769A0's unsigned below-23 write gate. No Game binding-array commit or file save is directly made by these capture helpers.

Construction initializes binding storage through 0x41DD0: clear 23 eight-byte pairs, then call 0x43760. In ascending index order the resulting code/kind pairs are: (0,1), (1,1), (0x2A,0), (0x13,0), (0x10,0), (0x11,0), (0x1F,0), (0x1E,0), (0x20,0), (0x39,0), (2,1), (0x12,0), (0x21,0), (0x1D,0), (0x14,0), (0x32,0), (0xF,0), (0x30,0), (0x2E,0), (0x18,0), (0x24,0), (0,0), (0,0). The setter's native write order differs from sorting by index; it sets indices 0,1,4,5,6,7,8,9,10,11,12,3,13,2,14,15,16,17,20,18,19. Indices 21 and 22 remain from the earlier clear.

Later construction calls 0x42B30 on G+0x1BDC. It opens literal controls.cfg for input and repeatedly reads a byte-string name and, for recognized names, two signed 32-bit integers. Names map to action indices: ButtonAttack 0, ButtonSpecialAttack 1, ButtonClassSkill 2, ButtonUltimateSkill 3, ButtonQuickItem 4, ButtonUp 5, ButtonDown 6, ButtonLeft 7, ButtonRight 8, ButtonJump 9, ButtonDodge 10, ButtonUse 11, ButtonLamp 12, ButtonWalk 13, ButtonCallPet 14, ButtonWorldMap 15, ButtonRingMenu 16, ButtonInventory 17, ButtonCraftingMenu 18, ButtonSystemMenu 19, ButtonMultiplayerMenu 20, ButtonZoomIn 21, ButtonZoomOut 22. Only after both integer reads have clear stream state does it publish the pair. Unknown names do not select a pair write. A read/state failure exits with previously completed pair assignments preserved. The provider's tokenization and filesystem behavior remain boundaries.

After setup, constructor 0x7EA80 calls 0x89B80(G,G+0x1BDC). That helper copies the 0xB8 bytes of supplied bindings into Game storage, using eleven ordered 16-byte transfers followed by eight bytes, then tail-calls 0x438E0 on the destination. The ordinary selected constructor passes that destination itself as source. The writer emits the 23 named pairs to controls.cfg; index 12's output literal is ButtonLamp. No success result is checked by the constructor. This path is distinct from the F-key options.cfg saves and from temporary binding-capture widget storage. Alternative callers and eventual UI commit callbacks are not universally traced.

## G+0x420: bounded writes and concrete consumers

The verified receiver chain establishes the following selected writers; this is a bounded discoverable census, not a proof that every indirect/aliased writer in the image has been found.

| Selected source | Exact byte effect and gate |
| --- | --- |
| I constructor 0xF8690 reached by Game 0x7EA80 | Initialize I+0x308/G+0x420 to zero. |
| Game keydown 0x96E70 | Low-byte key 0x1B toggles it on every dispatch, before chat gates. |
| Interface press 0x101020 | Button one, after GUI callbacks and a null override-or-hit result, clears it. |
| Game press 0x94900 | The admitted G+0x668/M+0x150 action branch described above clears it after P state/position bookkeeping. |
| Complete selected Game helper 0x9FDF0 | After its preliminary call chain and node work described below, clears it. |
| Viewed-update scoped site 0xAB330 in 0xA5A50 | Clears it within the admitted action/vector predicate branch described below. |
| Application scoped site 0x137A47 in 0x1355A0 | Clears it during the selected successful-controller/menu branch described below. |

The complete 0x9FDF0 first calls 0xA27B0 and 0x911E0, then iterates the entity-pointer range G+0x1778..+0x1780 with an increasing numeric index, calling 0x9BB20 on each selected entity. It sets direct node G+0x118 selected to one, clears G+0x420, clears G+0x668, then clears node-selected values of widgets G+0x330, +0x340, +0x350, +0x360, +0x388, +0x418 and +0x378. The preliminary helper/decoder/disconnect effects are not imported from any previous contract. The direct metadata edge into this helper at 0x29234C lacks a resolved owning function entry in the selected export; it is not claimed as a universal ordinary input or Back callback.

In the viewed-update path, Game receiver provenance is established by the 0xA5A50 entry and I is explicitly formed from G+0x118 at 0xAAF6C. Once execution reaches 0xAB14F, simultaneous nonzero G+0x668 and G+0x1958 take a different movement path that bypasses the following clear branch. If either byte is zero, 0xAB283 tests action bytes G+0x1A75, +0x1A77, +0x1A78, +0x1A76, in that order. Any nonzero byte with chat+0x1D8 zero directly admits the clear branch. Otherwise 0x2B630 reads the two binary32 values at G+0x1D6C/+0x1D70, squares the first, squares the second and adds the rounded products. Only an ordered result strictly above word 0x3C23D70B at RVA 0x437F74 admits clearing; below, equal or unordered skips it when the compare completes.

That branch clears G+0x668, clears selected integers of widgets G+0x330, +0x350, +0x340, +0x348, clears G+0x420, then clears widgets G+0x358, +0x368, +0x360, +0x370, +0x388. A selected G+0x378 widget calls 0x8ADF0 on its +0x1E8 range afterward. Upstream admission to this scoped part of the large update owner and unrelated action consumers are not converted into an all-frames assertion.

The application controller block, after a successful controller-state return and its selected earlier work, calls 0x1014F0. A true result plus controller buttons-word bit 0x2000 set writes global byte 0x551CBC one, clears G+0x668 and widget-node selected integers G+0x330, +0x350, +0x340, +0x348, +0x388, +0x368, +0x360, then clears G+0x420, then clears selected widget nodes G+0x358, +0x650 and +0x658. This is not edge-gated by a prior value of 0x551CBC in that branch: it can repeat while the bit is held and admission stays true. The false-helper route tests the same buttons word for later continuation; an unset bit clears 0x551CBC. Unrelated earlier controller targeting and button helpers remain outside the bounded claim.

One direct consumer is the G+0x420 term in 0x1014F0, at its precise position among the other admission tests. A second is the scoped interface-update loop 0x1065CB..0x10661C within 0x103200. It captures the pointer-vector extent I+0x128..+0x130, uses a zero count for reversed bounds and otherwise pointer-sized rounded-up count, and iterates forward. Each iteration rereads I+0x308 and passes its byte to 0xA3A20 for that node; the helper normalizes its byte argument into a selected DWORD zero/one. The byte is reread per element, not saved once for the entire list.

The ordinary setup scope in 0xF9AC0 clones the literal menubutton template into I+0x108's root six times and appends them to that vector in loop order zero through five. Background literals and event-2 callback boundaries establish the selected labels: index zero crafting-menu/0x102F10, one inventory-menu/0x102FE0, two worldmap-menu/0x1031E0, three system-menu/0x103150, four multiplayer-menu/0x1030C0, five help-menu/0x102FA0. The latter targets' business effects and the complete setup/update bodies are not claimed closed here. This proves selected menu-button publication and an input-admission flag. It does not prove a general Back/navigation action, a stack of screens, or that keydown synchronously redraws all six nodes.

The viewed-update call site 0xAB5CE reaches 0x103200 with I. This is later ordinary interface work, distinct from the dispatched input owners; its complete owner and draw schedule are excluded. None of the seven input owners directly calls a renderer or World-step routine. They do call selected terrain queries and explicit World/decoder helper boundaries described above, and arbitrary direct/bound callbacks can perform more work. Consequently no claim that input is free of World changes, that GUI updates are draws, or that an entire message burst is indivisible is warranted.

## Independent conditional sequences

These examples are source-only conditions, not observations of an original run and not project validation vectors.

* Two 0x100 messages whose low-byte key is 0x1B, with G+0x420 initially zero and no intervening callback/other writer changing it, set it to one then zero. The raw key byte remains one until a corresponding Game keyup. Key repeat bits in lParam do not suppress the second toggle. If ordinary interface update occurs only after both, its selected six-node loop reads zero.
* A 0x102 message with wParam 0x00E9 first offers GUI unit 0x00E9 to a currently enabled focus control. If a callback leaves/replaces focus with a nonnull pointer, the later Game character phase does no chat edit. If focus is null and chat active at that later read, the Game chat insertion receives unit 0xFFE9. These are conditional paths, not an assertion about which focus callback runs.
* Suppose selected direct menu nodes G+0x118/+0x120 and selector zero exist at the beginning of left release. The node clear/set occurs before the GUI release callback. A callback that changes root selection is therefore followed by Game's post-GUI reads of the changed state; it does not retroactively suppress the pre-GUI operation.
* A right press performs GUI callbacks, then the interface's new override-or-hit query. A callback can leave a new U+0x168 override, preventing the null-result clear even when the original cached-point hit was absent. OS capture success does not determine this query.
* For chat caret zero with a negative selection admitted to 0x26B300's clamp, the clamp first brings its endpoint to zero; Left at the edge then performs no further movement/selection rewrite. For a valid positive caret and held GUI key 0x10, a successful Left decreases caret and increases signed selection. No physical Shift scan is needed by that helper.
* Suppose wheel input is zero and the first widget G+0x330 has selected integer nonzero and a positive point query. That branch consumes the call, invokes neither directional helper and does not test later wheel targets or fallback fields.
* Suppose two surviving Game presses yield unsigned timestamp differences 199 and 200, with G+0x668 nonzero and M+0x110 positive at each later guard. Only the 199 case reaches the special three-coordinate/height/decrement operation. Neither case requires matching buttons.
* A nonnegative widget G+0x658+0x2E0 equal to 30 enters the application's capture branch but fails the below-23 pair write in 0x2769A0. A subsequent Game release clears +0x2E0 first and can select a different nonnegative +0x2DC afterward when its release tail gates pass.
* An initial true 0x1014F0 motion result can dispatch GUI movement and then return early because current G+0x1958 is nonzero. If it is zero, later helper calls are fresh admission reads: an intervening callback-induced state change can make the camera phase retain original relative inputs instead of substituting cached differences.

## Evidence method and correspondence

Reference filenames were identified before scoped content reads. The original evidence root was C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Export authorities were decompiled/functions/140 plus the six-digit lowercase RVA and .c, decompiled/assembly.txt, decompiled/functions.csv, decompiled/function_ranges.csv and decompiled/calls.csv. File names and original metadata selected owners; there was no cross-project content search. C exports supplied navigational leads and complete owner control flow; operand widths, truncated arguments, float associations, compare flags, callback slots and disputed joins were resolved against original instructions. In particular the Game character sign extension, physical byte normalization, release item +0x14/+0xC fields, and 0xA7A0's precomputed gain follow instructions rather than incomplete decompiler arguments or associations.

The byte checker mapped original RVAs using the PE section table, read each selected exported instruction's recorded bytes, and compared every byte against the raw-backed original image. The 69 complete owners below total 39,102 instruction bytes, all matched, with zero mismatches. Every exported owner range was selected through original function-range metadata; fragmented owners retain their separate ranges. End addresses below are exclusive. This is complete selected-owner correspondence, not a proof that every possible indirect callee or alternate caller in the executable is known.

| Complete owner RVA | Native range(s), exclusive ends | Matched bytes | Instructions |
| --- | --- | ---: | ---: |
| 0x096E70 | 0x96E70–0x977F3 | 2435 | 550 |
| 0x097870 | 0x97870–0x9787C | 12 | 3 |
| 0x09F670 | 0x9F670–0x9F6B1 | 65 | 18 |
| 0x094900 | 0x94900–0x965E0 | 7392 | 1583 |
| 0x0965E0 | 0x965E0–0x96833; 0x96834–0x96843; 0x96844–0x96853; 0x96854–0x96860; 0x96861–0x96E70 | 2188 | 508 |
| 0x097880 | 0x97880–0x97CBA | 1082 | 201 |
| 0x097CC0 | 0x97CC0–0x9803E | 894 | 190 |
| 0x133C70 | 0x133C70–0x133EEC | 636 | 174 |
| 0x07EA80 | 0x7EA80–0x7F35E; 0x7F35F–0x7F36E; 0x7F36F–0x7F37E; 0x7F37F–0x7F38B; 0x7F38C–0x7F9E0 | 3932 | 806 |
| 0x101020 | 0x101020–0x10111F | 255 | 58 |
| 0x101120 | 0x101120–0x10115F | 63 | 16 |
| 0x101160 | 0x101160–0x1014E4 | 900 | 187 |
| 0x1014F0 | 0x1014F0–0x10178C | 668 | 119 |
| 0x342090 | 0x342090–0x342098 | 8 | 2 |
| 0x33F1E0 | 0x33F1E0–0x33F203 | 35 | 8 |
| 0x333E70 | 0x333E70–0x333EC8 | 88 | 19 |
| 0x340DF0 | 0x340DF0–0x340E57 | 103 | 26 |
| 0x340E60 | 0x340E60–0x340EB8 | 88 | 22 |
| 0x340DD0 | 0x340DD0–0x340DF0 | 32 | 8 |
| 0x26B170 | 0x26B170–0x26B2FE | 398 | 100 |
| 0x26B300 | 0x26B300–0x26B517 | 535 | 143 |
| 0x101BD0 | 0x101BD0–0x1028A9 | 3289 | 665 |
| 0x13B1D0 | 0x13B1D0–0x13B623 | 1107 | 221 |
| 0x0F8690 | 0xF8690–0xF87CA | 314 | 67 |
| 0x340EC0 | 0x340EC0–0x3410D9 | 537 | 138 |
| 0x3410E0 | 0x3410E0–0x3412F5 | 533 | 141 |
| 0x341300 | 0x341300–0x341495 | 405 | 103 |
| 0x3414A0 | 0x3414A0–0x3415E5 | 325 | 87 |
| 0x3415F0 | 0x3415F0–0x341777 | 391 | 102 |
| 0x341780 | 0x341780–0x3418C5 | 325 | 87 |
| 0x3418D0 | 0x3418D0–0x341B29 | 601 | 163 |
| 0x333F60 | 0x333F60–0x33429B | 827 | 215 |
| 0x09AEB0 | 0x9AEB0–0x9AEF2 | 66 | 21 |
| 0x09FDF0 | 0x9FDF0–0x9FF99 | 425 | 84 |
| 0x33C360 | 0x33C360–0x33C407 | 167 | 49 |
| 0x342560 | 0x342560–0x3425C0 | 96 | 29 |
| 0x342270 | 0x342270–0x342557 | 743 | 161 |
| 0x3308C0 | 0x3308C0–0x3308DC | 28 | 13 |
| 0x32A210 | 0x32A210–0x32A2B5 | 165 | 46 |
| 0x26B600 | 0x26B600–0x26B666 | 102 | 23 |
| 0x26A8C0 | 0x26A8C0–0x26A925 | 101 | 27 |
| 0x09AE50 | 0x9AE50–0x9AE63 | 19 | 5 |
| 0x0A36C0 | 0xA36C0–0xA36D1 | 17 | 5 |
| 0x0A3A20 | 0xA3A20–0xA3A40 | 32 | 8 |
| 0x09AF00 | 0x9AF00–0x9AF1A | 26 | 6 |
| 0x33AA88 | 0x33AA88–0x33AA91 | 9 | 2 |
| 0x33AA94 | 0x33AA94–0x33AA9D | 9 | 2 |
| 0x33AAA0 | 0x33AAA0–0x33AAA9 | 9 | 2 |
| 0x33AAAC | 0x33AAAC–0x33AAB5 | 9 | 2 |
| 0x33AAB8 | 0x33AAB8–0x33AAC1 | 9 | 2 |
| 0x33AAD0 | 0x33AAD0–0x33AAD9 | 9 | 2 |
| 0x33AADC | 0x33AADC–0x33AAE5 | 9 | 2 |
| 0x042B30 | 0x42B30–0x43208; 0x43209–0x4320F; 0x43210–0x43216; 0x43217–0x4321D; 0x4321E–0x43242; 0x43243–0x43252; 0x43253–0x43262; 0x43263–0x4326F; 0x43270–0x432B9 | 1921 | 502 |
| 0x089B80 | 0x89B80–0x89C02 | 130 | 26 |
| 0x00A7A0 | 0xA7A0–0xA946 | 422 | 106 |
| 0x041DD0 | 0x41DD0–0x41E01 | 49 | 16 |
| 0x0438E0 | 0x438E0–0x44020 | 1856 | 377 |
| 0x08A120 | 0x8A120–0x8A12C | 12 | 2 |
| 0x08A4F0 | 0x8A4F0–0x8A4FC | 12 | 2 |
| 0x094260 | 0x94260–0x9448E | 558 | 134 |
| 0x094600 | 0x94600–0x94744 | 324 | 67 |
| 0x094750 | 0x94750–0x94836 | 230 | 54 |
| 0x02B630 | 0x2B630–0x2B646 | 22 | 6 |
| 0x043760 | 0x43760–0x438D5 | 373 | 71 |
| 0x32B0C0 | 0x32B0C0–0x32B0CC | 12 | 2 |
| 0x2769A0 | 0x2769A0–0x2769FB | 91 | 23 |
| 0x2761F0 | 0x2761F0–0x2761FB | 11 | 2 |
| 0x32AE90 | 0x32AE90–0x32B0B9 | 553 | 138 |
| 0x276F30 | 0x276F30–0x276F3D | 13 | 3 |

The selected large-owner edge windows below were separately byte-compared, also with zero mismatches. These are instruction-start selection windows; a final instruction can extend past the window endpoint. They are deliberately not described as complete large-owner bodies.

| Scoped original edge window | Matched instruction bytes | Instructions |
| --- | ---: | ---: |
| 0xA5A50–0xA5B10 | 195 | 34 |
| 0xAAF50–0xAB3D3 | 1155 | 223 |
| 0xAB5B6–0xAB5D3 | 29 | 6 |
| 0x103200–0x103290 | 146 | 30 |
| 0x1065CB–0x10661C | 81 | 19 |
| 0xFFEE0–0x1003DF | 1274 | 244 |
| 0x1362DA–0x136345 | 107 | 26 |
| 0x136440–0x136843 | 1027 | 239 |
| 0x1378B5–0x137ADE | 553 | 95 |
| 0x33AAC4–0x33AACD | 9 | 2 |

The scopes cover Game/viewed receiver provenance and the selected clear/consumer path, interface-update receiver/selected-byte publication, six-node setup, constructor-result/global publication, message burst and initial device/binding/motion work, the selected controller clear, and the nine-byte +0xE0 thunk lacking a named function owner. Earlier/later unrelated large-owner behavior is not part of this scope. Original instructions were also followed at the direct controller button/wheel call sites identified below; their complete targeting/device owner closure is explicitly not supplied.

The raw data comparison/decoding included the window-message ten-entry jump table at 0x133EEC, the eight DWORD destinations at 0x977F4 and key-index byte table at 0x97814, and the original scalar words referenced in this contract. The key destinations correspond to Escape, 0x70..0x75 and default; the byte indices for those seven special keys are zero through six. The window table's default entries include the three double-click values. Neither data table is counted as instructions in the complete-owner totals.

Selected original constant facts are: 0x42E71C word 0x3FA00000; 0x437F80 word 0x3ED1B716; 0x437FD0 word 0x40435000; 0x42E538 word 0x3F8CCCCD; 0x437F8C word 0x3F40562A; 0x437FB0 word 0x3FAA5E36; 0x42E518 word 0x3BA3D70A; 0x42E51C word 0x3C23D70A; 0x437F74 word 0x3C23D70B; 0x4AC964 word 0x3F000000; 0x4AC9D0 word 0x40000000; 0x42E5B0 word 0x42480000; 0x42E5F8 word 0x47800000; 0x42E588 double word 0x40E0000000000000; 0x42E620 double word 0xC0F0000000000000; 0x42E778 double word 0x4066800000000000; 0x4ACA18 double word 0x4059000000000000; 0x4AC9B8 double word 0x3FF0000000000000; 0x42E540 double word 0x3F947AE147AE147B. Inline immediate clamp words were checked as part of the owner instructions. Source literals identify the file routes and selected labels; no file contents or UI asset packages were accessed.

The selected direct-call metadata identifies window sites: character 0x133D5D, keyup 0x133D88, keydown 0x133DB3, presses 0x133DFA/0x133E3F/0x133E87, releases 0x133E1B/0x133E60/0x133EAB, wheel 0x133EDA. It identifies application motion 0x13683E, wheel 0x136B60, presses 0x1372D6/0x13738D/0x1374AE/0x137511/0x137576, and releases 0x137334/0x1373C6/0x1374DC/0x137541/0x1375A6. The selected export identifies no additional resolved direct owning-function callers to these seven entries. That is a bounded metadata census; indirect calls, unassigned instruction owners and alternate application routes remain possible.

## Explicit limits and hold

The contract specifies the selected native branch and synchronous call/store order. It does not infer OS message availability, per-frame frequency, physical key scan-code meanings, provider return sequences, file success/durability, GUI asset availability, font/shape/transform numerical results, allocator exception cleanup, arbitrary malformed object safety, controller backend internals or scheduler/thread/reentrancy guarantees. Out-of-range/NaN float-to-integer results and machine-width arithmetic have not been converted into a checked portable-language contract. Compare-side unordered statements apply when the original instructions complete under the actual floating environment; exception masks, rounding-mode writers and concurrent global writers are not universally enumerated.

Raw message state, GUI focus/key set, GUI cached point/override/buttons, chat editing, binding-capture widget state and physical action polling have separate owners and admissions. Callback effects can change later rereads, while carried node/control pointers and captured traversal counts still exist. This body does not establish a lifetime safeguard for such changes. No universal world-tick/draw schedule or all-writers census follows from the selected edges.

This saved artifact is complete for the selected seven Game owners and the documented original-only helper/admission boundary. After full saved-file readback, byte equality and SHA256 reporting it is FINAL / STABLE / HELD. All writes to this file stop at HELD. All preceding held artifacts remain unread and immutable.
