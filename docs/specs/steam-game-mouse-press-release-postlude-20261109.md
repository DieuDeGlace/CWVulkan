# Original Game mouse press, release and postlude

The complete selected Game press owner is RVA 0x94900 and the complete release owner is 0x965E0. The press owner calls the interface GUI adapter first, then samples current widget state and performs its own admission and work. The release owner has a menu-selection prefix before its GUI adapter, then continues through its own widget, coordinate and final-selection work. A GUI hit or callback is not a general consumed-event return from either Game owner. The two ordinary early returns in the press owner are fresh point-query results for two specific direct nodes while byte G+0x668 is nonzero.

## Authority, image and receiver notation

This independent contract uses only the standalone docs/CLEANROOM_AUTHOR.md policy, this isolated assignment and individually selected original executable/export evidence. Every prior held artifact remained unread and unchanged.

The supported file is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. Its independently verified SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244; file length is 5,757,952 bytes. PE metadata are AMD64 machine 0x8664, preferred base 0x140000000, timestamp 0x5D93AB0D, entry RVA 0x3939F0 and SizeOfImage 0x58A000. Routine/literal/site addresses below are RVAs. Plus-offsets are relative to the explicitly identified receiver.

G is the Game receiver. I is its embedded interface at G+0x118, and U is the current eight-byte manager pointer at G+0x20. Constructor 0xF8690 stores the captured G pointer at I+0x5C0. The selected Game-constructor join calls it with G+0x118 and G. Application construction calls 0x7EA80 at 0x136331 and publishes the returned Game pointer in the eight-byte global at RVA 0x551A80 at 0x13633E; the constructor receives the manager global at RVA 0x551AC0 as its G+0x20 input.

W denotes the current eight-byte pointer read from G+0x8. E denotes a current pointer read from W+0x448. D denotes its current eight-byte state-object pointer at E+0x1528. C denotes the pointer at G+0x10. These are operand aliases: a later load can select a different object. They do not pin an entity or assert authoritative/viewed-owner equivalence, a single thread, or a stable object across callbacks.

For a node N, its selected visibility integer is the four-byte element selected by the signed 32-bit index at N's display+0xAC in the element array at display+0xE0; the display pointer is N+0x68. Constructor 0x3736C0 independently names this property visibility. The different property at display+0x168, selected by signed index+0x134, is named clipping and is not that visibility element.

A direct node field and a widget field require different joins. G+0x118, +0x120, +0x130, +0x310, +0x3A8 and +0x3B0 hold direct nodes in the selected setup. The widget fields below hold control objects whose associated root is their eight-byte +0x178 pointer. Base construction 0x328FF0 stores that root, and base 0x3669C0 stores the manager at object+0x8. Node creation 0x33CD30/0x32EE60 supplies its shape, transform, display and manager references; new default displays come from 0x3736C0.

The selected original setup joins are:

| Game field | Interface alias | Original setup identity relevant here |
| --- | --- | --- |
| G+0x118 | I | empty-named direct node below U+0x140; initially selected visibility one |
| G+0x120 | I+0x8 | empty-named child node below the preceding node |
| G+0x128 | I+0x10 | object constructed by 0x2910C0 on that child; option integer at object+0x1F0 |
| G+0x130 | I+0x18 | separately created empty-named direct node below U+0x140 |
| G+0x310 | I+0x1F8 | direct node found by original literal selector, reparented below I+0x108 and initially hidden |
| G+0x330 | I+0x218 | object returned by 0x27C950 with mode argument zero |
| G+0x340 | I+0x228 | object returned by 0x27C950 with mode argument two |
| G+0x348 | I+0x230 | object returned by 0x273240 |
| G+0x350 | I+0x238 | object returned by 0x27C950 with mode argument three |
| G+0x360 | I+0x248 | object returned by 0x277060 |
| G+0x368 | I+0x250 | object returned by 0x292380 |
| G+0x370 | I+0x258 | object returned by 0x267780 |
| G+0x378 | I+0x260 | object returned by 0x28EBB0 |
| G+0x398 | I+0x280 | object returned by 0x26A2E0; byte+0x1D8 is cleared in the press postlude |
| G+0x3A8 | I+0x290 | direct clone of the template found with literal leftbutton |
| G+0x3B0 | I+0x298 | direct clone of the template found with literal rightbutton |
| G+0x650 | I+0x538 | object returned by 0x287050 |
| G+0x658 | I+0x540 | object returned by 0x275A70 |

In the selected constructors, 0x27C950 captures G at object+0x210, 0x277060 at +0x1F8, 0x292380 at +0x2B8 and 0x267780 at +0x1F0. Their later helpers use those stored Game graphs; replacing such a pointer is not assumed impossible.

The two arrow clones are below I+0x288, a node produced through 0x1008A0 with literal land. The construction stores are 0xFDDE0 for I+0x290 and 0xFDE60 for I+0x298. Setup registers event 2 under the control names leftbutton and rightbutton to captured I targets 0x101860 and 0x101790 respectively. These literal/control associations do not supply actual asset geometry or prove every concrete derived shape/control class.

## Concrete callers and GUI dispatch

Complete window procedure 0x133C70 calls imported SetCapture with its window before Game press, then calls 0x94900 on the current global Game with button zero for message 0x201, one for 0x204 and two for 0x207. It calls imported ReleaseCapture before Game release, then calls 0x965E0 with zero for 0x202, one for 0x205 and two for 0x208. Each of these branches returns one after the Game call returns. It does not pass message-point coordinates to these two-argument Game calls or refresh the manager's cached point in those branches. Imported capture behavior, delivered-message order and any reentry caused by an import are not supplied by its native call order.

Other direct calls to these owners are catalogued within application owner 0x1355A0, including its selected controller-input paths. Those callers mean the Game owners are not universally paired with these window-procedure capture calls. The complete window procedure and the selected construction/call sites are established here; a complete controller/polling, foreground, device-provider or main-loop contract is outside this assignment. No other caller or indirect-entry census is claimed.

The button operand is kept in a full 32-bit register by each Game owner. The ordinary joins just listed supply zero, one or two. Press comparisons against ranges use the native unsigned 32-bit operand where stated. Neither Game owner rejects every other 32-bit value at entry.

The press call at 0x94953 supplies I and the unchanged button to complete adapter 0x101020. Its first store copies the current four-byte word at U+0x184 bit for bit to I+0x548, alias G+0x660. It does not update U's point. It routes button zero to 0x340EC0, one to 0x3415F0 and two to 0x341300. Other values return after that four-byte copy.

After button one's GUI press returns, the adapter reloads G through I+0x5C0, obtains current U and calls 0x33F1E0. That complete helper returns U+0x168 when nonnull, otherwise the fresh root point-query result for U+0x140 and U+0x180/+0x184, with query mask zero. If that returned node is null, the adapter clears byte I+0x308, alias G+0x420, then clears the selected visibility integers of the roots of current I+0x218, +0x228 and +0x230, in that order. It does not cache a handled result from the earlier GUI target. Buttons zero and two do not run this right-button-only postwork.

Complete release adapter 0x101120 supplies current U to 0x3410E0 for button zero, 0x341780 for one and 0x3414A0 for two; other values return. It has no corresponding four-byte point-copy store and no right-button hide block.

### Selected GUI manager behavior

All six manager owners first modify the four-byte word U+0x1A8: press sets bit 0, 1 or 2 for button zero, one or two; release clears that bit and preserves the other bits. This is a native GUI word, not proof of current physical-device state. The imported OS capture calls and the eight-byte GUI override node pointer U+0x168 are distinct state.

Each manager owner selects the current override node U+0x168 when nonnull; otherwise it calls 0x333F60 on root U+0x140 with the two cached binary32 words U+0x180/+0x184 and mask zero. The selected target is not required to have been the target of a matching earlier press. These calls do not acquire a native GUI lock in the inspected owners.

For an ordinary selected node, targeted delivery first checks its current effective control at node+0x78 and control byte+0x1A0. A nonnull control with nonzero byte receives its virtual method. It then rereads the node's flag bit 1 and current effective-control pointer, looks up a signed 32-bit event key through 0x32A210, and invokes the callback object through its first virtual target when nonnull and either the current control byte+0x1A0 is nonzero or callback byte+0x8 is zero. Thus a control virtual call occurs before the event-map lookup and later gates. Neither the effective-control pointer nor its callback is held as an immutable pre-call snapshot.

| Button | Press control slot | Press event key | Release control slot | Release event key | Final root/control broadcast slot |
| --- | --- | ---: | --- | ---: | --- |
| 0 | +0x58 | 2 | +0x60 | 3 | press +0xE8; release +0xF0 |
| 1 | +0x70 | 5 | +0x78 | 6 | press +0x100; release +0x108 |
| 2 | +0x88 | 8 | +0x90 | 9 | press +0x118; release +0x120 |

The event lookup searches control+0x188 by signed 32-bit key. On a missing key its reached constructor initializes a null callback payload before insertion. Allocation, reentrant mutation, replacement and deletion beyond these selected bodies remain boundaries.

Before targeted delivery, all three presses compare the selected node's own-control pointer at node+0x70 with U+0x170. A missing node or unequal pointer causes the current nonnull U+0x170 control's byte+0x15C to be set to one, followed by event 0x13 through 0x342560 on that control's current root; U+0x170 is then cleared. That callback can run before the later clear and target delivery.

Button-zero press has additional earlier work. It assigns U+0x1B0 to a selected effective control only when the node/control exist, control flag bit 5 at +0x150 is set and control byte+0x1A0 is nonzero; otherwise it clears U+0x1B0. On the assignment route it broadcasts control virtual slot+0xE0 through the current GUI root before the later popup and U+0x170 checks. Its complete transfer thunk at 0x33AAC4 is a native nine-byte, uncatalogued tail transfer, not an invented exported owner.

If the current popup/widget pointer U+0x178 is nonnull and its root is not an ancestor of the selected node, the left-press owner calls 0x342270 with null replacement and null positioning control. Complete ancestor helper 0x3308C0 follows node+0x40 parent links and returns false for a null selected node. The selected null-replacement path of 0x342270 hides the old popup root's selected visibility and then writes U+0x178 to zero; its nonnull placement/size/provider branches were read but are not selected by these calls.

Button-zero release, after choosing its initial target, similarly clears the popup through 0x342270 only if both current U+0x178 and U+0x1B0 are nonnull. After its initial release method and event callback, it separately rereads U+0x168 or performs another root query, then always performs a fresh root query without that override. If the two selected nodes' effective controls differ, it sends the override/selected control virtual slot+0xB8 and event 0x0E under the same current-enable, node-bit and callback-byte gates. Those later queries occur after the first callbacks; they are not a cached release-inside test. The other releases do not perform this extra outside-control phase.

All six owners finish with the corresponding table broadcast. Complete 0x33C360 visits the current root then its forward child-list order, admits only nodes whose selected visibility is nonzero and flag bits 2 and 1 are clear, and invokes the selected virtual slot on each admitted node's own control at +0x70. A rejected node prunes that subtree. This broadcast does not add the targeted-delivery control-byte+0x1A0 test. Each child's visitor descriptor is copied before recursion, but node/control/list lifetime is not retained or protected by that copy. The local root control's virtual call can precede reading its children. No handler-return value universally stops the later Game owner.

### Fresh point queries and provider bounds

Complete 0x333E70 obtains the tested node's manager at node+0x8, copies that manager's two cached binary32 point words, and calls 0x333F60 with that node, the local point and mask zero. It returns whether the resulting node pointer is nonnull. It does not query U+0x168.

Complete 0x333F60 first requires selected visibility nonzero and node flag bits 2 and 12 clear. When the own control exists, its +0x58 pointer is nonnull, manager flag bit 1 at +0x288 is set and own-control flag bit 8 at +0x150 is clear, it calls 0x32CF50 then 0x32D010 for its current origin/size inputs. For ordinary finite values it rejects X or Y below the origin or strictly above origin plus size; equality at an edge is not rejected by those comparisons. It then uses current local transform data at node+0x60, specifically the +0x2E0..+0x31C words, or the default image matrix; parent data comes through 0x100EE0 and 0x173E0. It performs its native binary32 homogeneous division. A present effective control and shape can admit the shape slot+0x28 test followed by the effective-control coordinate-adjustment slot at its subobject+0x38. These provider calls precede the local-hit tests. Local hit is true when a current shape slot+0x20 succeeds, or the current own control exists and either its slot+0x28 succeeds or its flag bit 8 at +0x150 is set. The owner permits child queries exactly when selected display clipping is zero or local hit is true; otherwise it returns null. Children are queried in reverse child-list order and the first nonnull descendant result wins; only afterward can the current node itself be returned. With mask zero, node flag bit 1 prevents returning that node itself, while it is not an unconditional entry-time subtree rejection in this helper. Other mask bits/callers are not conflated with this selected mask.

Complete control-root query 0x32B0C0 calls 0x32AE90 with that control and its current root. That query requires node bit 2 clear and current selected visibility nonzero, and requires any present own control to equal the query control. It does not add the +0x80 bit-12 or bit-1 tests of 0x333F60. It uses the query control's manager point and the node's transform/parent data, calls the query control's coordinate-adjustment slot at its +0x38 subobject, then tests the current shape slot+0x20 or own-control slot+0x28. A successful reached hit returns true. Otherwise it queries children in reverse order only when selected clipping is zero. The numerical provider closure, asset bounds, projective degeneracies and exact rendered geometry are not invented here.

The current point and control dimensions in the later widget predicates come through 0x32B180, 0x329E00 and 0x32A1D0. The first subtracts the control's current provider position from its manager's cached point and combines the differences with the selected current matrix coefficients. The latter two obtain the second and first binary32 outputs of complete 0x32A110 respectively. That owner derives its two outputs from the control's +0x70/+0x74, +0x80/+0x84 and +0x90..+0x9C fields, with its ordered native adds/subtracts and clamps. These are source-proved provider/input relationships, not imported or asset geometry guarantees.

## Arrow callback registration and synchronous effects

Complete name registration 0xF8090 compares each encountered own control's UTF-16 name against the specified string using 0x89470's explicit-length unsigned-16-bit comparison. Matching controls receive event registration, and child nodes are visited forward. Its reached 0xF8200 can register on discovered child controls too; 0x32AC80/0x32ABA0 excludes node bit 2, descends through the root/no-own-control route and collects a non-root own control before stopping descent through that control.

Registration replacement 0xF8170 frees the previous callback object, requests a 0x20-byte object and stores null if that allocation returns null. On success it stores a byte at +0x8, captured I at +0x10 and the target address at +0x18. The original first vtable entry at image RVA 0x441BB8 points to 0xF99A0. Its complete uncatalogued 11-byte transfer body loads the current capture and transfers to that target. This is a pointer capture, without a proved lifetime reference or provider reentry guarantee.

The leftbutton target 0x101860 advances cursor I+0x5C8 through its next link only when that next link differs from the current state-object list head at D+0x118. The rightbutton target 0x101790 advances through its previous link only when the current cursor differs from that list's first link. Both use valid existing native cursor/list pointers without an empty/null recovery route in these bodies.

Each then calls 0x1081B0 on I, reads the current cursor's two four-byte words at +0x10/+0x14 into 0x2B9210, calls 0xA3390 on the current captured G with that helper's two-word result, and finally raises the current signed 32-bit G+0x1E04 value to four if it is below four. The cursor and G are reread after the preceding helper calls.

Complete 0x1081B0 first writes four-byte zero/one selected visibility to the right and then left direct arrow nodes using their list-boundary comparisons. If the current cursor is not the list head, it prepares its original named string and record-derived inputs for the reached naming/translation providers and supplies their returned string to recursive text helper 0x334C50 on I+0x288. The exact resulting label is a provider/data dependency. It does not directly set G+0x668 or G+0x420 in this body.

Complete 0x2B9210 does not read its W argument. Its two 32-bit input coordinates produce two 32-bit result words through the original wrapping integer operations. It combines the second input multiplied by 57 with the first, prepares streams offset by 0xE29E and 0x70532, scrambles each with an XOR of its 13-bit left shift, applies the ordered square, multiply by 0xEC4D, add 0x131071F, multiply by the scrambled word, subtract 0x2DF722F3 and clear bit 31. Each resulting word is multiplied unsigned by 0x61A8F94D; its high 32-bit product shifted right 13 is subtracted from 100000 with 32-bit wrap. The subsequent unsigned multiply by 0xAAAAAAAB, high-product shift right 12, tripling and 11-bit left shift determine the subtracted multiple of 6144. The remaining 32-bit value is added to the corresponding input shifted left 13 and offset 0x400. The first result is stored before the second. No imported random draw occurs in this owner.

Complete 0xA3390 calls 0x35EA0 on current W with those two signed 32-bit coordinates and mode zero. After that call returns it multiplies the returned binary32 value by the original 65536.0 constant. It stores sign-extended input X shifted left 16 at G+0x1E08, performs the height conversion toward zero to signed 64 bits, stores sign-extended Y shifted left 16 at G+0x1E10, stores the converted height at G+0x1E18, and sets byte G+0x1E38 to one. The selected provider return and native conversion limits remain explicit; no terrain height is guessed.

The complete callback sequence can therefore change the arrows' visibility and the Game target/counter before the enclosing press owner performs its own fresh arrow point queries. Neither the registration nor these callback bodies establishes that the later point query must still hit the arrow that received the GUI event.

## Complete Game press after GUI dispatch

The press owner retains the original 32-bit button, calls 0x101020, then reloads the current G+0x360 widget root/display and writes global byte RVA 0x5514D7 to one exactly when that root's selected visibility is nonzero. This snapshot is taken after GUI callbacks, for every button that reaches it, before either ordinary early return. It is process-global storage, not an individual widget or per-button field.

It next tests byte G+0x668 once. If zero, neither arrow query occurs. If nonzero, it queries current direct node G+0x3A8 through 0x333E70; a nonzero return transfers to the function epilogue at 0x965AC. Only a zero result admits a reload of G+0x3B0 and its second query; a nonzero second result takes the same epilogue. The gate byte is not retested between the first and second query. Each query reloads its node's manager point and may call shape/control providers.

These are the only two ordinary owner-level early-return admissions. They bypass the clock sample, G+0x1DB0 clear, initial scalar sample, item/secondary-widget work, common scalar rescale, G+0x398 object's byte clear and direct selector-node hide. They do not undo GUI dispatch or the preceding 0x5514D7 snapshot. Native error-provider/INT3 paths for invalid allocation/list lengths are separate nonordinary exits.

### Button-two coordinate-list branch

After those queries fail or are not admitted, button two tests current G+0x668. While nonzero it reads C and requires signed 64-bit C+0x110 to be nonnegative for the coordinate-list work. It divides C+0x100 and +0x108 toward zero by 65536, using sign correction before the 64-bit arithmetic shift. Y is prepared before X for that first provider call, and Y is reread before X after it.

At call 0x94A1C it supplies current W, the X quotient in the 64-bit second-argument register, the low 32 bits of the Y quotient in the third-argument register, and mode zero to 0x35EA0. The returned binary32 H is compared against positive zero. Native COMISS and CMOVBE select zero when H is ordered less than or equal to zero, and also on unordered flags if execution continues without a floating exception. Otherwise they select H. The selected value is converted toward zero to a signed 32-bit height word. Invalid/unrepresentable conversion and floating exception configuration are not assigned a manufactured terrain result.

After that provider call, C's X/Y fields are reread and divided again. Their low 32-bit quotients become the two stored coordinate words. Each is further divided toward zero by 4096 in 32-bit arithmetic to form the two signed tree-key words. The owner calls 0x7C280 on current E's state-object map at D+0x108. That complete helper selects an exact signed lexicographic two-word key or creates a missing node through its reached tree-construction helpers.

The returned map node's linked list is at +0x28, with eight-byte count at +0x30. The owner scans forward for the first record whose two 32-bit coordinate words at list-node+0x14/+0x18 exactly equal the captured coordinates. A match is unlinked, decrements that eight-byte count and is freed, then scanning stops. Height and the record's byte are not equality keys.

With no matching record, it constructs a new node through 0x6E6C0, increments the count and appends at the list's end. The defined payload is byte zero at list-node+0x10, the two coordinate words at +0x14/+0x18, and the converted height at +0x1C. The complete record-copy helper copies those fields; it does not prove zero for payload padding. This owner explicitly detects its list maximum-count error before linking the new node. No safe fallback is inferred for malformed lists, a missing state object or failed providers.

Whether the signed C+0x110 test admitted that work or not, the branch retests current G+0x668 and, if still nonzero, toggles byte G+0x1E54 by storing one when its old byte was zero and zero otherwise. It then joins the common clock phase. A callback/provider changing G+0x668 can therefore affect that later toggle independently.

### Button-zero C+0x150 branch

The mutually exclusive non-button-two route admits its selected coordinate work only for button zero, byte G+0x1DB0 zero, byte G+0x668 nonzero and the signed 32-bit first word C+0x150 different from 0x7FFFFFFF. There is no corresponding second-word sentinel test at this admission.

It calls complete membership helper 0x5F310 with E and the current pair at C+0x150/+0x154. That helper returns a low-byte boolean for exact signed lexicographic membership in D+0x28; a null E+0x1528 returns false. If membership is false and the reread byte C+0x158 is zero, it calls 0x2ADCE0 with W and that pair. Its signed 32-bit return is compared against current E+0xAAC.

When E+0xAAC is less than that return, the owner supplies the original 18-unit UTF-16 string Not enough money. followed by newline to 0x26BF10 on G+0x398, with the original four-word argument at image RVA 0x438070. It then calls 0xA0420 with numeric identifier 0x34 and G+0x1DC0 as the location input.

Otherwise it subtracts that signed 32-bit return from E+0xAAC, copies the current C+0x150/+0x154 pair to a local, and calls 0x745E0 on D+0x28 with that pair and the selected false byte argument. One additional caller stack byte comes from a local byte not defined by the inspected path; its meaning is not invented. It then calls 0xA0420 with identifier 0x3D and G+0x1DC0.

If membership is true or reread C+0x158 is nonzero, it instead copies C+0x150/+0x154 into current E+0x1B0/+0x1B4, copies E's three eight-byte +0x10/+0x18/+0x20 words into E+0x1B8/+0x1C0/+0x1C8, writes E's byte+0x68 to 0x9C and four-byte +0x6C to zero, then clears G+0x668 and G+0x420 bytes. The current E pointer is reread between the selected groups of stores. Every normally returning branch joins the clock phase.

### Clock, target and initial scalar sample

The owner calls imported WINMM timeGetTime at 0x94DB3. It subtracts the previous four-byte global at RVA 0x5514E8 with 32-bit wrap and publishes the new sample to that global before testing the difference. Its admission is unsigned difference strictly below 200, current G+0x668 nonzero, and signed 64-bit C+0x110 strictly greater than zero. There is no same-button, same-node or matching-release comparison. Exactly 200 is rejected; wrap follows unsigned subtraction. The initial global value, other writers and clock/provider behavior beyond the returned word are not established here.

On admission it copies C+0x100, +0x108 and +0x110 into G+0x1E08, +0x1E10 and +0x1E18 in that order. It divides the copied X and Y toward zero by 65536, calls 0x35EA0 with W and mode zero, multiplies that returned binary32 value by 65536.0 and converts toward zero to signed 64 bits, replacing only G+0x1E18. Unlike the button-two list height, this path does not first clamp the provider return to zero.

It subtracts one from the four-byte G+0x1E04 operand, stores the wrapped result, and replaces it with zero only if the result's sign flag is set. A decrement from 0x80000000 wraps to 0x7FFFFFFF and is retained; an original-value comparison does not control this clear. It then sets byte G+0x1E38 to one.

After that conditional target work, every admitted press path clears byte G+0x1DB0 at 0x94E7A. It calls 0x5FAC0 on current E at 0x94E8C and retains its binary32 result A0. This sample occurs before the subsequent item/secondary-widget work.

### Selection helpers, item fields and common calls

Complete 0x94600 supplies either two signed 32-bit indices or the pair -1,-1. It requires G+0x330's root selected visibility nonzero and a successful 0x32B0C0 control-root query. It checks widget+0x270 and +0x274 nonnegative and below the current E inventory outer-vector and selected inner-vector lengths. Outer headers have 0x18-byte stride; slots have 0xA4-byte stride. It additionally requires the slot's signed 32-bit count at offset zero to be nonzero, not strictly positive. On success the output copies the two current widget indices.

Complete 0x94260 independently requires G+0x330's root visibility and tries ten point queries, forward, on the current roots of widgets in the pointer array at G+0x3C8. There is no ten-element vector-size check in that owner. The first hit returns current E plus the following item offsets, by array index: 0x638, 0x6D8, 0x778, 0x818, 0x318, 0x598, 0x3B8, 0x4F8, 0x458 and 0x8B8. No hit returns null. These are fresh point queries without the GUI override pointer.

Complete 0x94750 requires G+0x350's root visibility, its control-root query and valid signed row/slot indices at widget+0x270/+0x274 against the different nested vectors at G+0x1870/+0x1878. It returns the selected slot's item at slot+4; it does not separately require that slot's count to be nonzero.

An item copy here is not an assertion that every byte of a 0xA0-byte span is copied. Complete helpers 0x35400 and 0x9CD0 copy byte zero; four-byte fields at +0x4, +0x8, +0xC, +0x10 and +0x18; byte+0x14 and +0x1C; all 128 bytes +0x1D through +0x9C; and byte+0x9D. They leave the intervening/trailing padding outside those stores. Slot-count decrements in the selected press and release branches wrap at 32 bits, store that result and clear count to zero, item byte zero to zero and item four-byte +0x4 to zero only when the resulting signed count is at most zero. They do not zero the whole slot. The selected native branches compare the decremented operand, including negative and wrapped inputs.

For readability, the following prose identifies the two recurring calls 0x280A40 then 0x280C80 on current G+0x330 as the ordered widget call pair. Their transitive geometry/content work is not assumed to be an atomic refresh. Calls 0xB75D0, 0xB2180 and 0x9D6F0 are identified by their actual receivers/arguments below without inventing their untraced transitive effects.

Calls 0xA03F0 and 0xA0420 are likewise concrete boundaries. Complete 0xA03F0 supplies G+0x1DC0 and original binary32 one operands to 0xA0420. Complete 0xA0420 reads the supplied three signed 64-bit location words, derives its current distance/attenuation and projected scalar inputs from G's fields and original constants, conditionally resolves identifier input through 0xA3A40 and invokes virtual slot+0x38 on the current G+0x28 object. The selected identifiers here are outside its separate 0x6D..0x77 counter-update group. Resource resolution, concrete provider implementation and any resulting audio behavior are not invented from an identifier.

### Button two after A0

For button two the owner calls 0x94600. Invalid indices go directly to the common scalar postlude. A valid slot must still have nonzero count. Its item byte must be outside 0x16, 0x17 and 0x18, using an eight-bit subtraction and unsigned comparison. If current E's action byte+0x68 equals 0x56, full defined-field item equality through complete 0x4DF60 between E+0x1D8 and the selected item also prevents this branch.

On admission it prepares a native record with the selected defined item copied into it, selected all-ones identifier words, zeroed selected scalar/vector fields and the native trailing byte/word operands eight and one. It constructs a list node through 0x7330, increments current E+0xAC0's eight-byte count and appends to E+0xAB8's linked list. The explicit maximum-count error route is separate from normal completion. Only after publication does it decrement the inventory slot count and perform the partial clear described above. It then goes directly to the common scalar postlude, bypassing the button-zero secondary-widget phase. No guessed business meaning is assigned to the queued record.

### Buttons zero and one: selected inventory slot

Other buttons use an unsigned comparison: values greater than one skip this inventory/equipment/other-grid phase and proceed toward the later button-zero test. For zero and one, a valid 0x94600 slot follows this priority:

1. If current G+0x360 root visibility is nonzero, copy the two current selected indices as two four-byte stores to widget+0x1E8/+0x1EC, then join the later phase. This direct publication wins over the following widget tests.

2. Otherwise, if G+0x350 root visibility is nonzero, row indices one and five take identifier 0x34 at G+0x1DC0 without selling/copying that slot. Every other row copies the selected item, decrements/partially clears its slot, calls 0x10A390 on the copy and adds that signed 32-bit return to current E+0xAAC. It calls 0xA0420 identifier 0x3D at E+0x10 with the original binary32 0.75 scale argument, prepares a one-count record, appends it to Game list G+0x21B0/+0x21B8, and calls 0xA0720 when the unsigned eight-byte count exceeds ten. It then calls 0xB75D0 and 0xB2180 on G. The original count/list/error operands and padding are not replaced with an invented serialization.

3. Otherwise, if G+0x370 root visibility is nonzero, item bytes 3 through 9 inclusive or 0x1A publish the current item pointer to that widget's eight-byte +0x1E8 field. Other item bytes call identifier 0x34 at G+0x1DC0.

4. With those roots hidden, call 0x50640 on current E and the selected item. A false return selects identifier 0x35 at G+0x1DC0. A true return still requires the slot count to be nonzero, then uses the item byte cases below.

The row-one/five test in step 2 is the original 32-bit mask test, not a list of guessed row names. The scalar additions/subtractions wrap at 32 bits; the surrounding comparisons use the specified signed or unsigned operands.

For item byte 3, copy the item to a local T, decrement/partially clear its source slot, then select destinations using T's unsigned 32-bit +0x4 word, the button and current byte G+0x1A97. The special subtype set M is exactly 5, 6, 7, 8, 10, 11, 15, 16, 17, 18 and 23, admitted by the native below-24 check and bitmask 0x878DE0. An existing destination qualifies for that set only when its own byte is 3 as well.

Here returning an existing item means a call of 0x4E810 on current E+0x9F0 with that item, count one and selector -1. Clearing an item means copying the reached default defined fields, rather than zeroing its entire span.

| Stable selected conditions at their native reads | Ordered item operations |
| --- | --- |
| T subtype 13 or 14 | If current E+0x6D8 qualifies for M, return it and clear it; then return current E+0x638 and copy T there |
| T subtype 6 or 7, button zero | Return E+0x638; copy T there; return E+0x6D8; clear E+0x6D8 |
| T subtype 6 or 7, button one | Return E+0x6D8; copy T there; return E+0x638; clear E+0x638 |
| Other subtype, button zero and G+0x1A97 zero | Return E+0x6D8; copy T there; if T qualifies for M, return and clear E+0x638 |
| Other subtype, button one or G+0x1A97 nonzero, T qualifies for M | Return E+0x6D8; copy T there; return and clear E+0x638 |
| Other subtype, button one or G+0x1A97 nonzero, T does not qualify for M | Return E+0x638; if the then-current E+0x6D8 qualifies for M, return and clear it; copy T to E+0x638 |

The table expresses native branch choices under stable values at their actual reads; it does not pin G+0x1A97, E, the destination item or vector storage across the calls. After the reached item-byte-3 operations, the owner runs the widget call pair, calls 0xA0420 identifier 0x5B at G+0x1DC0 and calls 0xB2180 on G.

The other selected item-byte cases are:

| Item byte | Ordered local behavior before joining the later phase |
| ---: | --- |
| 0x14 | Copy T; return current E+0x8B8; copy T there; decrement/partially clear source; widget call pair; 0xA03F0 identifier 0x5B |
| 0x15 | If action byte E+0x68 is zero, or its binary32 E+0x6C is ordered strictly greater than the signed 32-bit 0x4F220 return converted to binary32, copy the selected item to E+0x1D8, write action byte 0x56 and four-byte timer zero, then hide G+0x330's root; no source-count decrement in this case |
| 0x1A | Copy T; decrement/partially clear source; return E+0x3B8; copy T to E+0x278; widget call pair; 0xA03F0 identifier 0x5B |
| 4 | Copy T; decrement/partially clear source; return E+0x3B8; copy T to E+0x3B8; widget call pair; 0xA03F0 identifier 0x5B |
| 5 | Same sequence with E+0x4F8 |
| 6 | Same sequence with E+0x458 |
| 7 | Same sequence with E+0x598 |
| 8 | Same sequence with E+0x318 |
| 9 | Same sequence with E+0x818 for button zero, E+0x778 for button one |
| 1, 0x18, 0x1B or 0x1C | Enter W+0x3C8's critical section through 0x2ABB70, call 0xB7740 with G and the selected slot, then leave through 0x2B9890 |
| Other byte | No branch-specific mutation in this switch |

The item-byte-0x1A return source and copy destination really differ; they are not silently made a symmetric swap. The signed 32-bit 0x4F220 return is sampled on current E only when the action byte is nonzero; the native COMISS/JBE path rejects unordered timer comparison if execution continues. That complete scalar owner was read, but its action/stat-provider numerical closure is not expanded here.

The critical-section helpers directly call imported EnterCriticalSection and LeaveCriticalSection at W+0x3C8. The normal call order encloses only the selected 0xB7740 call. It does not prove exception-safe release or broader protection of the preceding UI, item, clock or common postlude work.

### Buttons zero and one: no selected inventory slot

When 0x94600 returns an invalid index pair, the owner first calls 0x94260. A nonnull returned item pointer with nonzero item byte follows these current-root priorities:

- G+0x350 root visible: call 0x10A390 on that item, add its signed 32-bit return to E+0xAAC, call identifier 0x3D at E+0x10 with the 0.75 binary32 scale argument, prepare/append a one-count Game record and apply the unsigned count-above-ten 0xA0720 admission, call 0xB75D0, then clear only the source item byte and four-byte +0x4 field. This selected branch does not itself call 0xB2180.
- That root hidden and G+0x370 root visible: bytes 3..9 or 0x1A publish the pointer at widget+0x1E8; other bytes select identifier 0x34 at G+0x1DC0.
- Both roots hidden: call 0x4E810 on E+0x9F0 with the source item, count one, selector -1; clear its byte and four-byte +0x4; run the widget call pair; call 0xA03F0 identifier 0x5B; call 0xB2180.

If that pointer is null or its item byte zero, the owner calls 0x94750. A null result simply joins the later phase. With a result and current G+0x350 widget+0x2B4 different from one, it samples 0x109D30. A cost greater than current signed E+0xAAC produces the insufficient-money text and 0xA03F0 identifier 0x34. Otherwise it calls 0x50670 on current E and the item; a false result supplies the original 37-unit UTF-16 string You can't carry more of these items. followed by newline, then calls identifier 0x34 with E+0x10 as location.

On the latter admission's success, it inserts one item through 0x4E810, resamples 0x109D30 and subtracts that return from current E+0xAAC. It selects the D+0x138 entry for the pair at G+0x1E98 through 0x7BFF0, calls 0xA17D0 on the selected value+0x28 and the item, calls identifier 0x3C at E+0x10, runs the widget call pair, then calls 0xB75D0, 0x9D6F0 with G/item/count one/current E, and 0xB2180, in that order. It does not assume the two cost samples are equal.

With G+0x350 widget+0x2B4 equal to one, it instead walks Game list G+0x21B0 forward while a 32-bit index starts from the low 32 bits of G+0x21B8 minus one and decreases, seeking widget+0x274. A missing selection joins the later phase. On a selected record it starts integer success count and iteration count at zero. While the current signed record count at node+0x10 admits another iteration, it first tests 0x50670 on node+0x14. A failure emits the capacity text and identifier 0x34 at E+0x10, then leaves the loop. Otherwise it samples 0x10A390; an affordable return admits insertion of one node+0x14 item, a second cost sample and subtraction from E+0xAAC, followed by 0x9D6F0 and increment of the integer success count. An unaffordable iteration is skipped without breaking. The iteration count then advances and the record count is reread.

After the loop it subtracts the integer success count from the record count. Exactly zero unlinks the record, decrements the eight-byte Game list count and frees it. It then calls identifier 0x3C at E+0x10, 0xB75D0 and the widget call pair even when the success count is zero or a capacity failure ended the loop. These are native calls, not a manufactured successful purchase. Provider mutation of the list/count, negative counts and malformed storage do not establish a bounded fixed number of iterations.

Complete 0x4E810 itself has count-zero and item-byte-zero no-work admissions. Selector -1 calls 0x5F100 for its selected index; the selected outer vector can be grown. Item byte 0x0C with item byte+0x1C equal to 0x0B adds the count to aggregate+0xBC; item byte 0x0E adds it to aggregate+0xC0. Other items scan the selected slot vector, remember the first zero-count slot and use the reached stack predicate 0x5F0E0 plus defined-field equality 0x4DF60. A match adjusts count or stores -1 for the helper's count-minus-one mode. Otherwise it copies into a remembered empty slot or constructs/appends a slot. These are synchronous selected native mutations, with allocation, index-selection, growth and item-capacity helpers left as explicit transitive boundaries.

### Button-zero secondary widgets, then the universal admitted-press postlude

At the common later join 0x9622B, only button zero admits the secondary-widget phase. If current G+0x340 root visibility is nonzero, it calls complete 0x280E30 with byte argument one. When that widget's +0x1E8 pointer is nonnull and current +0x274 is nonnegative, the helper copies +0x270/+0x274 to its selected +0x278/+0x27C fields and calls 0xA03F0 identifier 0x58 through widget+0x210. Its marker-node admission uses the current size providers, cell dimensions +0x2A8/+0x2AC, scroll array+0x288/index+0x2B4 and selected index, then shows/positions or hides the marker at +0x230. A negative +0x274 can return from that helper before updating the selected pair; the Game owner still proceeds to its later selected-pair tests.

The Game owner checks selected signed +0x278/+0x27C against the nested vectors at G+0x17A8/+0x17B0, with outer stride 0x18 and slot stride 0xA4. An invalid pair prepares the selected default item/scalar/vector fields and transfers that aggregate into current G+0x348 object+0x1F8 through 0x834A0, then publishes -1,-1 at that object's +0x2B8/+0x2BC.

A valid pair constructs a temporary through 0x7EA10 and calls 0x2AEC30 with W, selected slot's item+4, that temporary and the low-byte condition selected outer index equals six. A false return transfers a separate default aggregate into the secondary object and publishes -1,-1. A true return copies the produced aggregate through 0x834F0 into object+0x1F8, then copies the current G+0x340 selected pair to its +0x2B8/+0x2BC. The temporary is disposed afterward.

The selected aggregate contains defined item fields, a four-byte field+0xA0 and a three-eight-byte-word vector header at +0xA8/+0xB0/+0xB8. Complete 0x834A0 copies the defined item/scalar fields then transfers that vector header through 0x82E80, freeing an existing destination buffer through the reached provider and clearing the source header. Complete 0x834F0 copies those fields and uses complete 0x82F10 for vector copying/growth. Complete disposal owners 0x81870/0x89380 clear their header words after their reached buffer-release provider. Padding and arbitrary outputs of 0x2AEC30 are not replaced with an all-zero-span assumption.

After the G+0x340 phase, a visible current G+0x368 root and nonnull 0x294CC0 result admit its selected work. That complete selector uses widget signed +0x2C0: one selects current E+0x638, zero selects E+0x6D8, other values return null; the selected item's byte must be nonzero. Nonnegative widget+0x2D8 selects 0x294C70, which toggles signed widget+0x210 between that index and -1, then calls identifier 0x58 through widget+0x2B8. Otherwise nonnegative +0x2C4 selects 0x294CA0, which copies it to +0x2C0 and makes the same identifier call. With both negative, the owner calls 0x294CC0 again and calls complete 0x292650 on the widget; the additional loaded item byte at +0x1C is not consumed as a second parameter by that selected body.

Complete 0x292650 independently selects its item again. With byte+0x2D4 zero or no item, a null current GUI query and nonzero pending byte+0x37C can call 0x10A0E0 with stored +0x380/+0x384/+0x388 operands; it then clears selected index+0x210 to -1 and pending byte+0x37C. Otherwise an invalid selected index with a pending byte calls 0x10A0E0 then 0x108B90 with the current +0x2C8/+0x2CC/+0x2D0 words and +0x38C, then clears pending. Without pending, it scans the selected item's byte+0x9D records for a matching three-byte key and, on first match, sets pending and stores the selected byte/three current words at +0x38C/+0x380/+0x384/+0x388. A valid selected index with item record count below 32 calls 0x108B90 using the selected vector record's byte+0x20, then scans current E's inventory forward for the first equal item, decrements/partially clears that slot and returns. The transitive item-edit functions are not assigned guessed meanings.

All normally returning admitted presses, for every button value, reach 0x9651F after their selected work. It calls 0x5FAC0 on current E again, giving A1. It then reloads current E and executes binary32 division A1 by retained A0, followed by binary32 multiplication of that rounded quotient by the current binary32 E+0x180 value, then stores the result at E+0x180. It does not multiply the old scalar first, use a binary64 intermediate, or use a saved pre-work E+0x180 value.

It calls 0x5FAC0 a third time, giving A2, reloads current E+0x180 and compares A2 against it with COMISS. If the carry flag is clear it preserves the current scalar; otherwise it selects A2 and stores it. For ordinary finite operands this replaces the scalar only when A2 is strictly less. Equality preserves the current bits, including an equal signed-zero operand. If execution continues on unordered flags, carry is set and A2 is selected; the C presentation's simple less-than condition alone would not establish that unordered behavior. Floating exception masks/status, rounding configuration, zero denominators, infinities and invalid operations remain processor/configuration conditions. There is no lower-zero clamp in this postlude.

After those scalar stores, it reloads current G+0x398 and clears that object's byte+0x1D8 at 0x96588. It then reloads direct node G+0x310, its display and signed active index and writes four-byte zero to that selected visibility element at 0x965A8, immediately before the epilogue. No menu-root, selected-widget, GUI-hit, capture, button-range or named-character condition guards these final two stores after admission. The two arrow returns bypass them.

Complete 0x5FAC0 begins with binary32 100, or 125 when bytes E+0x14C and +0x14D are both one. In order, eligible item bytes at E+0x638, +0x6D8, +0x3B8, +0x458, +0x4F8 and +0x598 add returns of 0x1097B0. Their required item bytes are respectively 3, 3, 4, 6, 5 and 7. Cached 32-bit E+0x80 flags then select multipliers 10 for bit 9, 1.5 for bit 17 and 15 for bit 18, in that order. It traverses E+0x980's linked list for the first byte-0x16 record and applies that record's binary32 +0x14 multiplier if admitted by the reached pointer guard, then starts from the head again and applies every byte-0x20 record's +0x14 multiplier in list order. Cached flag bit 26 finally applies binary32 word 0x3F8CCCCD, approximately 1.1. No arbitrary nonzero A0, fixed maximum, stable entity or stat-provider return is inferred from this complete helper.

## Complete Game release

The release owner has no counterpart of the press owner's two arrow point-query returns or its shared scalar/chat/selector postlude. Its normal failed admissions continue to later release phases. In particular, its menu-selection prefix precedes GUI release.

### Menu-selection prefix before GUI release

The owner tests selected visibility of direct G+0x118, then direct G+0x120, then requires button zero. It reads signed option integer +0x1F0 from the object at G+0x128. Negative values do nothing here. Its exact selected cases are:

| Option | Synchronous native order |
| ---: | --- |
| 0 | Hide direct G+0x118; show direct G+0x130 |
| 1 | Show the root of current G+0x650 |
| 2 | Store byte one at G+0x1D68 |
| 3 | Call 0x9BB20 with G, current four-byte G+0x2264 and current E; hide direct G+0x118; store binary32 8.0 at G+0x1DB8 then +0x1DB4; call 0x2E0B50 on current W |
| Other nonnegative integer | No selected-case store/call |

These native branches do not require a matching earlier press, a current pointer hit, a saved GUI target or a current software-capture identity. They are admissions on current node/option state and the release button. This contract identifies the reached 0x9BB20/0x2E0B50 calls and their order; it does not substitute a prior character-load/reset contract or invent their transitive effects.

After the prefix's selected work returns, every release calls 0x101120 with I and the original button at 0x96767. Consequently, those menu stores and calls can affect the state seen by the GUI release callbacks themselves.

### Selected string/current-entity route after GUI release

The owner then tests current G+0x378 root selected visibility and button zero. On admission it calls complete 0x94840 on that widget. The helper reads its signed index+0x2F8 and a vector at +0x1E8/+0x1F0 with 0x1B0-byte record stride. A valid index copies the current narrow-string object at record+0x10; invalid index supplies an empty string. The output object has eight-byte length/capacity words at +0x10/+0x18 and inline threshold 16 bytes.

After obtaining that string, the Game owner rereads current widget+0x200 as a signed 64-bit identifier and searches current W+0x450 by signed 64-bit tree keys at node+0x20, using the sentinel byte at node+0x19. It selects the exact match's eight-byte value at +0x28 or null on a miss. It calls 0x89CB0 with G, that selected pointer or null, and the returned string, then destroys the temporary string through its selected capacity/alignment/provider checks. It does not skip this call merely because the selected string is empty or the identifier lookup misses. The command/message meaning, entity lifetime and transitive effects of 0x89CB0 are unproved here.

### G+0x360 route and the preceding press snapshot

Next it reads current G+0x360 root visibility and global byte 0x5514D7. Only if both are nonzero and the button is zero does it run this widget's selected route. The global was sampled by the press owner after GUI dispatch and before its early returns, but this is not a per-button saved node, matching-press token or retained object. Another reached press can replace it, including a press that takes an arrow early return. A GUI release callback can independently change the current root visibility before this test.

Complete 0x277C00 selects a current slot using the widget's captured Game at +0x1F8, that Game's W/E and signed widget indices +0x1E8/+0x1EC. It requires a nonempty outer inventory, valid nonnegative indices, nonzero slot count and item byte 0x0F; it returns that item pointer or null. It is separately sampled by the following helpers.

Complete 0x277D70 first requires a nonnull 0x277C00 result. In provider-return terms, for ordinary finite values it then requires current local point Y strictly above the first sampled dimension-Y minus 30, below a second sampled dimension-Y, point X strictly below half the sampled dimension-X and strictly above zero. Its point comes from 0x32B180; the dimensions come from 0x329E00 and 0x32A1D0 in that order of tests. It does not prove fixed pixel geometry.

A true return from 0x277D70 admits a cost sample through complete 0x277BC0. That helper independently calls 0x277C00; a null result returns signed 32-bit zero, otherwise it calls 0x109D30 and returns at least one, using signed integer division by two truncated toward zero. The Game owner compares that cost against the current selected E+0xAAC with a signed comparison, then requires current G+0x360 widget+0x1F0 to equal one.

It next validates signed row/slot indices at widget+0x1E8/+0x1EC against current E's nested vectors and requires the selected count nonzero. Complete 0x277E30 calls 0x277C00 for a null test, calls it again and then tests the selected item byte against 0x0F; it does not add a second null guard between those two samples. A false return skips the following generation work.

On success, the release owner resamples 0x277BC0 and subtracts that signed 32-bit return from current E+0xAAC. It copies the selected item's defined fields to a local, then rereads the selected indices/current vector and decrements/partially clears that slot. Thus debit precedes item copy/decrement, and the initial and debit cost samples are separate calls.

Only afterward it calls the imported rand provider once at 0x96A54. The native result is masked to bits 31 and 0, with the reached signed-negative normalization before the zero/nonzero decision. A zero result selects 0x2D15D0; nonzero selects 0x2D0680. Both calls supply current W, their separate output buffer, the copied source item's byte+0x14, signed 32-bit -1 and byte zero. No provider sequence, distribution, imported return domain or generated item is guessed.

The returned item is copied through 0x35400 into a local. For item bytes 2, 3, 4, 5, 6, 7, 8, 9, 15, 22, 24, 26 or 30, the original below-31 check and bitmask 0x454083FC additionally copy the source item's eight-byte +0xC/+0x10 pair into the generated item's corresponding fields. Other generated item bytes preserve those provider-produced words. The owner inserts one generated item through 0x4E810 with selector -1, runs the widget call pair, calls 0xA0420 identifier 0x31 at G+0x1DC0 with the original one operands, then calls 0x9D6F0 with G, the generated item, count one and current E.

Regardless of whether the preceding success/cost/index/item conditions admitted generation, the selected G+0x360 route next calls complete 0x277CD0. For ordinary finite dimension/point values it checks Y strictly above dimension-Y minus 30, below a newly sampled dimension-Y, X strictly above half a sampled dimension-X and below another sampled dimension-X. Unlike 0x277D70 it has no selected-item predicate. A true return hides the roots of current G+0x360 and G+0x330, in that order. It does not directly clear G+0x420.

### Subsequent C-coordinate membership route

After that route, a button-zero release admits the following independent work only while current byte G+0x668 is nonzero, byte G+0x1DB0 is zero and signed 32-bit C+0x15C differs from 0x7FFFFFFF. A nonzero button goes directly to the later widget phase even when the G+0x360 route was not admitted. C+0x160 has no matching sentinel rejection here.

It first converts the current signed C+0x15C then +0x160 to two signed 32-bit cell words. For nonnegative inputs the native arithmetic gives integer division by 16. For negative inputs it adds one, divides that result toward zero by 16, then subtracts one; over signed 32-bit inputs this gives floor division by 16, including negative multiples and boundaries.

It looks up that pair in the current E state-object tree at E+0x1528 then +0xA8. The comparison is signed lexicographic X then Y, using node key words +0x1C/+0x20 and the sentinel byte+0x19. A miss performs no insertion and no selected state change.

On a hit, it writes the selected E action byte+0x68 to 0x75, reloads current E for four-byte +0x6C zero, then rereads C+0x160 first and C+0x15C second and converts them again. It publishes X then Y as two four-byte stores at the then-current E+0x1D0/+0x1D4 and clears byte G+0x668. The pair tested for membership and the pair finally published are not one immutable sample. This route does not directly clear G+0x420, reset byte G+0x1DB0 or run the press scalar postlude.

### Final widget phases and normal return

At 0x96D8C, current G+0x370 root visibility and button zero admit a 0x268670 call. Its complete body first requires its selected item pointer at widget+0x1E8, samples 0x109D30, converts the signed result to binary32 and back toward zero to signed 32 bits, and compares that converted value against the captured Game's current E+0xAAC. It then samples its point and current parent/local matrix inputs; for ordinary finite values it checks point Y strictly between two sampled dimension-Y terms using the current scale factor, point X strictly below half the scaled dimension-X and strictly above zero. Size/position/transform/provider returns remain inputs, not a universal rectangle.

A true 0x268670 return calls complete 0x267930 on the widget. That owner returns zero for a null selected item pointer; otherwise it calls 0x109D30, converts the result to binary32 and back to a signed integer. The release caller discards that return. This selected owner does not itself perform an inventory replacement or money debit, and this contract does not manufacture one from its placement after the admission. Any transitive behavior of its price helper is separate.

After that conditional call it calls complete 0x268540 on the current G+0x370 object regardless of the first predicate's result. In the finite provider case, this checks the other X half: Y strictly within the scaled bottom-30 terms, X strictly above half a sampled scaled dimension-X and below another sampled scaled dimension-X. A true return hides the current G+0x370 root. Its coordinate/size/transform samples occur again and are not reused from 0x268670.

Finally, every normally returning release, for every 32-bit button value, calls 0x2761F0 on current G+0x658. That complete helper stores signed 32-bit -1 at widget+0x2E0. The Game owner then rereads that object/root selected visibility; when nonzero, button zero and signed widget+0x2DC nonnegative, it calls 0x276F30, whose complete body copies the current four-byte +0x2DC value into +0x2E0. It then returns. This reset/read/conditional-copy order is not a pre-GUI snapshot and is not bypassed just because another release admission failed.

## Independent conditional sequences

These are source-only consequences and mathematical references with stated inputs, not original runtime captures, owner observations or replacement-engine tests.

- Let a press's GUI adapter return normally, with G+0x668 nonzero and the first direct arrow query returning true. The global 0x5514D7 snapshot is already updated and GUI button state/callback effects may already have occurred. No clock sample or common final byte/visibility clear occurs in that press.
- Let the first query return false while its provider changes a later field, and let the second direct arrow query return true. The second pointer and its manager point are obtained after the first call. No retest of G+0x668 is inserted between those queries.
- Let the GUI arrow callback change that arrow's selected visibility to zero while returning normally. Its later Game point query can fail at the visibility gate; a GUI event on that arrow does not imply that the Game owner necessarily takes an early return.
- With the arrow returns not taken, unsigned clock difference 199, G+0x668 nonzero and signed C+0x110 positive admit the fast-target work even across different button values. Difference 200 rejects it. Both cases publish the new clock sample before that difference test.
- In the common press postlude, conditionally choose normally returned A0=100, A1=200, A2=200 and current E+0x180=50 at the multiplication, with ordinary binary32 configuration and a stable E. Division precedes multiplication, giving the exact scalar 100, which is below A2 and remains 100. With A0=A1=A2=100 and current E+0x180=-5, the scalar remains -5; the postlude adds no lower-zero clamp.
- For a release with both menu visibility integers nonzero, button zero and current option two, G+0x1D68 is set before GUI release callbacks. A missing current pointer hit does not undo that pre-GUI store.
- A release whose current G+0x360 root is visible but global 0x5514D7 is zero skips that widget's selected generation/close route. Its later coordinate and final G+0x658 phases still have their own admissions.
- With button zero, G+0x668 nonzero, G+0x1DB0 zero and initial C words -16 and -17, the cell pair tested is -1,-2. A membership hit rereads the C words before publishing E+0x1D0/+0x1D4; only unchanged words imply that same stored pair.
- A direct call with an otherwise untraced button value three does not acquire a GUI button bit through these adapters. If the press arrow returns are not taken it still reaches the clock and common scalar/byte/selector postlude. A normally returning release with that value still resets G+0x658 object's +0x2E0 to -1 and does not run its button-zero restore. This example does not claim an ordinary window branch supplies three.

## Original evidence and complete-byte correspondence

Reference filenames were identified before scoped content searches. The authority is the supported original binary identified above and selected exports beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled. Metadata inspected were functions.csv, function_ranges.csv and calls.csv; instructions were selected by owning native routine in assembly.txt.

The complete Game press and release bodies were read, as were the complete selected helper C presentations listed below. Their input/call/store/return interpretation was reconciled with original instruction operands. The original C presentation sometimes omits live arguments or labels a register-returning helper void. In particular, the press-to-0x101020 button operand, 0x5F310's pair argument, 0x4DF60's second item pointer, 0x33F1E0's returned node and the XMM0 value consumed after 0x35EA0 were taken from native instructions rather than those incomplete signatures. The scalar postlude's COMISS carry branch and native integer decrements were checked directly.

Complete selected owners have the following raw-image correspondence. Every instruction byte in each listed exported range matched the supported PE section's raw bytes; each individual range has continuous instruction coverage. Gaps between distinct exported ranges are not counted as decoded instructions.

| Owner RVA | Inclusive original exported ranges | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| 0x9CD0 | 0x9CD0 through 0x9D5E | 38 | 143 |
| 0x35400 | 0x35400 through 0x3549F | 44 | 160 |
| 0x4DF60 | 0x4DF60 through 0x4E062 | 73 | 259 |
| 0x4E810 | 0x4E810 through 0x4EAC4 | 163 | 693 |
| 0x4F220 | 0x4F220 through 0x4F76C | 311 | 1357 |
| 0x5F310 | 0x5F310 through 0x5F438 | 91 | 297 |
| 0x5FAC0 | 0x5FAC0 through 0x5FC1E | 93 | 351 |
| 0x6E6C0 | 0x6E6C0 through 0x6E707 | 22 | 72 |
| 0x7C280 | 0x7C280 through 0x7C356 | 68 | 215 |
| 0x7EA10 | 0x7EA10 through 0x7EA75 | 25 | 102 |
| 0x81870 | 0x81870 through 0x818C4 | 24 | 85 |
| 0x834A0 | 0x834A0 through 0x834E1 | 16 | 66 |
| 0x834F0 | 0x834F0 through 0x83531 | 16 | 66 |
| 0x89380 | 0x89380 through 0x893D0 | 24 | 81 |
| 0x94260 | 0x94260 through 0x9448D | 134 | 558 |
| 0x94600 | 0x94600 through 0x94743 | 67 | 324 |
| 0x94750 | 0x94750 through 0x94835 | 54 | 230 |
| 0x94840 | 0x94840 through 0x948F1 | 45 | 178 |
| 0x94900 | 0x94900 through 0x965DF | 1583 | 7392 |
| 0x965E0 | 0x965E0 through 0x96832; 0x96834 through 0x96842; 0x96844 through 0x96852; 0x96854 through 0x9685F; 0x96861 through 0x96E6F | 508 | 2188 |
| 0xA03F0 | 0xA03F0 through 0xA0417 | 8 | 40 |
| 0xA0420 | 0xA0420 through 0xA0660 | 124 | 577 |
| 0xF6CC0 | 0xF6CC0 through 0xF6D04 | 18 | 69 |
| 0xF7170 | 0xF7170 through 0xF73C3 | 160 | 596 |
| 0xF8690 | 0xF8690 through 0xF87C9 | 67 | 314 |
| 0x101020 | 0x101020 through 0x10111E | 58 | 255 |
| 0x101120 | 0x101120 through 0x10115E | 16 | 63 |
| 0x133C70 | 0x133C70 through 0x133EEB | 174 | 636 |
| 0x267930 | 0x267930 through 0x26795B | 13 | 44 |
| 0x268540 | 0x268540 through 0x26866C | 67 | 301 |
| 0x268670 | 0x268670 through 0x2687E5 | 81 | 374 |
| 0x268A70 | 0x268A70 through 0x268B39 | 36 | 202 |
| 0x2761F0 | 0x2761F0 through 0x2761FA | 2 | 11 |
| 0x276F30 | 0x276F30 through 0x276F3C | 3 | 13 |
| 0x277BC0 | 0x277BC0 through 0x277BF0 | 17 | 49 |
| 0x277C00 | 0x277C00 through 0x277CC8 | 48 | 201 |
| 0x277CD0 | 0x277CD0 through 0x277D6F | 42 | 160 |
| 0x277D70 | 0x277D70 through 0x277E2B | 51 | 188 |
| 0x277E30 | 0x277E30 through 0x277E5F | 18 | 48 |
| 0x27C950 | 0x27C950 through 0x27D161 | 457 | 2066 |
| 0x280E30 | 0x280E30 through 0x280FD1 | 91 | 418 |
| 0x292650 | 0x292650 through 0x292984 | 182 | 821 |
| 0x294C70 | 0x294C70 through 0x294C98 | 8 | 41 |
| 0x294CA0 | 0x294CA0 through 0x294CBC | 5 | 29 |
| 0x294CC0 | 0x294CC0 through 0x294D19 | 25 | 90 |
| 0x2ABB70 | 0x2ABB70 through 0x2ABB7D | 2 | 14 |
| 0x2B9890 | 0x2B9890 through 0x2B989D | 2 | 14 |
| 0x328F00 | 0x328F00 through 0x328F6F | 31 | 112 |
| 0x328FF0 | 0x328FF0 through 0x329206 | 96 | 535 |
| 0x329E00 | 0x329E00 through 0x329E33 | 12 | 52 |
| 0x32A110 | 0x32A110 through 0x32A1CE | 45 | 191 |
| 0x32A1D0 | 0x32A1D0 through 0x32A202 | 12 | 51 |
| 0x32A210 | 0x32A210 through 0x32A2B4 | 46 | 165 |
| 0x32AE90 | 0x32AE90 through 0x32B0B8 | 138 | 553 |
| 0x32B0C0 | 0x32B0C0 through 0x32B0CB | 2 | 12 |
| 0x32B180 | 0x32B180 through 0x32B248 | 45 | 201 |
| 0x32EE60 | 0x32EE60 through 0x32F051 | 114 | 498 |
| 0x3308C0 | 0x3308C0 through 0x3308DB | 13 | 28 |
| 0x3308E0 | 0x3308E0 through 0x330A77 | 115 | 408 |
| 0x333890 | 0x333890 through 0x3339BC | 75 | 301 |
| 0x333E70 | 0x333E70 through 0x333EC7 | 19 | 88 |
| 0x333F60 | 0x333F60 through 0x33429A | 215 | 827 |
| 0x334410 | 0x334410 through 0x334506 | 60 | 247 |
| 0x33AA88 | 0x33AA88 through 0x33AA90 | 2 | 9 |
| 0x33AA94 | 0x33AA94 through 0x33AA9C | 2 | 9 |
| 0x33AAA0 | 0x33AAA0 through 0x33AAA8 | 2 | 9 |
| 0x33AAAC | 0x33AAAC through 0x33AAB4 | 2 | 9 |
| 0x33AAD0 | 0x33AAD0 through 0x33AAD8 | 2 | 9 |
| 0x33AADC | 0x33AADC through 0x33AAE4 | 2 | 9 |
| 0x33C360 | 0x33C360 through 0x33C406 | 49 | 167 |
| 0x33CD30 | 0x33CD30 through 0x33CED2 | 117 | 419 |
| 0x33F1E0 | 0x33F1E0 through 0x33F202 | 8 | 35 |
| 0x340EC0 | 0x340EC0 through 0x3410D8 | 138 | 537 |
| 0x3410E0 | 0x3410E0 through 0x3412F4 | 141 | 533 |
| 0x341300 | 0x341300 through 0x341494 | 103 | 405 |
| 0x3414A0 | 0x3414A0 through 0x3415E4 | 87 | 325 |
| 0x3415F0 | 0x3415F0 through 0x341776 | 102 | 391 |
| 0x341780 | 0x341780 through 0x3418C4 | 87 | 325 |
| 0x342270 | 0x342270 through 0x342556 | 161 | 743 |
| 0x342560 | 0x342560 through 0x3425BF | 29 | 96 |
| 0x3669C0 | 0x3669C0 through 0x366A01 | 18 | 66 |
| 0x3736C0 | 0x3736C0 through 0x37379D; 0x37379F through 0x3737AD; 0x3737AF through 0x3737BD; 0x3737BF through 0x3737CA; 0x3737CC through 0x37386A; 0x37386C through 0x37387A; 0x37387C through 0x37388A; 0x37388C through 0x373897; 0x373899 through 0x373948; 0x37394A through 0x373958; 0x37395A through 0x373968; 0x37396A through 0x373975; 0x373977 through 0x373A1D; 0x373A1F through 0x373A2D; 0x373A2F through 0x373A3D; 0x373A3F through 0x373A4A; 0x373A4C through 0x373AF6; 0x373AF8 through 0x373B06; 0x373B08 through 0x373B16; 0x373B18 through 0x373B23; 0x373B25 through 0x373BE8 | 324 | 1301 |
| 0x82E80 | 0x82E80 through 0x82F0F | 41 | 144 |
| 0x82F10 | 0x82F10 through 0x830C4 | 122 | 437 |
| 0x101860 | 0x101860 through 0x10191B | 39 | 188 |
| 0x101790 | 0x101790 through 0x101853 | 40 | 196 |
| 0xF8090 | 0xF8090 through 0xF8164 | 59 | 213 |
| 0x1081B0 | 0x1081B0 through 0x108448 | 152 | 665 |
| 0x2B9210 | 0x2B9210 through 0x2B92EF | 60 | 224 |
| 0xA3390 | 0xA3390 through 0xA340C | 33 | 125 |
| 0xF8200 | 0xF8200 through 0xF837D | 99 | 382 |
| 0xF8170 | 0xF8170 through 0xF81F1 | 35 | 130 |
| 0x32AC80 | 0x32AC80 through 0x32ACE2 | 26 | 99 |
| 0x32ABA0 | 0x32ABA0 through 0x32AC7C | 58 | 221 |
| 0x89470 | 0x89470 through 0x894A7; 0x894B0 through 0x894F9 | 39 | 130 |
| 0x35EA0 | 0x35EA0 through 0x36156 | 146 | 695 |
| 0x277060 | 0x277060 through 0x277206 | 98 | 423 |
| 0x292380 | 0x292380 through 0x292593 | 107 | 532 |
| 0x267780 | 0x267780 through 0x26791E | 99 | 415 |

The 99 complete selected owners above account for 8,841 instructions and 37,336 matched bytes, with zero mismatches and zero internal coverage gaps. This is complete owning-body correspondence for the selected Game owners and helpers, not a universal transitive numeric/provider closure.

The following three larger context owners also had their entire exported instruction ranges byte-compared, with zero mismatches and zero internal gaps. Their semantic interpretation is expressly bounded to the final table column. Only the selected relevant C/native setup/caller slices are used; unrelated startup, persistence, controller navigation, interface layout and worker behavior is not claimed closed by checking all of that owner's bytes.

| Context owner | Inclusive original exported ranges | Instructions | Matched bytes | Interpretation in this contract |
| --- | --- | ---: | ---: | --- |
| 0x7EA80 | 0x7EA80 through 0x7F35D; 0x7F35F through 0x7F36D; 0x7F36F through 0x7F37D; 0x7F37F through 0x7F38A; 0x7F38C through 0x7F9DF | 806 | 3932 | Game receiver/input fields and the interface-construction join |
| 0xF9AC0 | 0xF9AC0 through 0x10089D | 5496 | 28126 | selected node/widget creation, stores and arrow-name callback bindings |
| 0x1355A0 | 0x1355A0 through 0x137E1B | 2217 | 10364 | Game publication and catalogued selected direct input-caller joins |

The complete context byte checks account for 8,519 instructions and 42,422 bytes. Two uncatalogued transfer sequences were separately checked against the image: 0x33AAC4 through 0x33AACC, two instructions and nine bytes, transfers to control virtual slot+0xE0; 0xF99A0 through 0xF99AA, three instructions and eleven bytes, transfers from the callback object's current capture/target words. The raw first pointer at image RVA 0x441BB8 independently equals preferred VA 0x1400F99A0. No exported owning body or alternate-entry guarantee is invented for those sequences.

Individually selected C presentation filenames beneath decompiled/functions were: 140009cd0.c, 140035400.c, 14004df60.c, 14004e810.c, 14004f220.c, 14005f310.c, 14005fac0.c, 14006e6c0.c, 14007c280.c, 14007ea10.c, 140081870.c, 1400834a0.c, 1400834f0.c, 140089380.c, 140094260.c, 140094600.c, 140094750.c, 140094840.c, 140094900.c, 1400965e0.c, 1400a03f0.c, 1400a0420.c, 1400f6cc0.c, 1400f7170.c, 1400f8690.c, 140101020.c, 140101120.c, 140133c70.c, 140267930.c, 140268540.c, 140268670.c, 140268a70.c, 1402761f0.c, 140276f30.c, 140277bc0.c, 140277c00.c, 140277cd0.c, 140277d70.c, 140277e30.c, 14027c950.c, 140280e30.c, 140292650.c, 140294c70.c, 140294ca0.c, 140294cc0.c, 1402abb70.c, 1402b9890.c, 140328f00.c, 140328ff0.c, 140329e00.c, 14032a110.c, 14032a1d0.c, 14032a210.c, 14032ae90.c, 14032b0c0.c, 14032b180.c, 14032ee60.c, 1403308c0.c, 1403308e0.c, 140333890.c, 140333e70.c, 140333f60.c, 140334410.c, 14033aa88.c, 14033aa94.c, 14033aaa0.c, 14033aaac.c, 14033aad0.c, 14033aadc.c, 14033c360.c, 14033cd30.c, 14033f1e0.c, 140340ec0.c, 1403410e0.c, 140341300.c, 1403414a0.c, 1403415f0.c, 140341780.c, 140342270.c, 140342560.c, 1403669c0.c, 1403736c0.c, 140082e80.c, 140082f10.c, 140101860.c, 140101790.c, 1400f8090.c, 1401081b0.c, 1402b9210.c, 1400a3390.c, 1400f8200.c, 1400f8170.c, 14032ac80.c, 14032aba0.c, 140089470.c, 140035ea0.c, 140277060.c, 140292380.c, 140267780.c. The bounded setup and application evidence came from 1400f9ac0.c and 1401355a0.c; their native caller/field joins and the Game constructor prefix were checked in assembly.txt. No earlier held handoff supplies a fact in this contract.

Selected image literal checks confirmed UTF-16 leftbutton at 0x441028, length ten; rightbutton at 0x441040, length eleven; land at 0x441298, length four; selector at 0x4414A8, length eight; and named at 0x436718, length five. The display-construction literals visibility at 0x47C188 and clipping at 0x47C198 are narrow ASCII, lengths ten and eight. The two original message literals at 0x4362C0 and 0x4362F0 were checked as 18 and 37 UTF-16 units plus a following two-byte zero. These are literal/native associations only; no corresponding asset or installation file was opened.

The original binary32 constants explicitly used here were independently checked: 0x4AC980 contains 0x3F800000 (one), 0x4AC964 contains 0x3F000000 (one half), 0x42E5F8 contains 0x47800000 (65536), 0x42E710 contains 0x3F400000 (0.75), 0x42E5BC contains 0x42C80000 (100), 0x432938 contains 0x42FA0000 (125), 0x42E598 contains 0x41200000 (10), 0x42E548 contains 0x3FC00000 (1.5), 0x42E7A4 contains 0x41700000 (15), 0x42E538 contains 0x3F8CCCCD and 0x42E5A8 contains 0x41F00000 (30). The 8.0 release-prefix stores are original immediate word 0x41000000.

## Bounded limits, ownership and exposure

This closes the complete ordinary selected Game press/release owner branches, their two press point-query returns, complete common postludes and the stated GUI/caller/helper joins. Direct synchronous stores, list publication and selected native helper effects are distinguished from calls into other original owners and virtual/imported providers.

It does not close every transitive body called by those owners. Examples of expressly retained boundaries include 0x9BB20 and 0x2E0B50's character/world work; 0x2ADCE0 and 0x109D30/0x10A390/0x1097B0/0x66460's return production; 0x50640/0x50670 and 0x5F100/0x5F0E0's item selection/admission; the transitive insertion, vector allocation and list/tree node construction behind the selected wrappers; 0x2AEC30's output aggregate; 0x2D15D0/0x2D0680's generation; 0x35EA0's descriptor/noise/world-provider closure; 0xB7740, 0xB75D0, 0xB2180, 0x9D6F0, 0xA17D0, 0x108B90 and 0x10A0E0's further effects; label/translation/resource providers; and 0x89CB0's downstream command or message work. A named native call is not silently turned into a successful transaction, event consumption, decoder/reset, network send, world tick or rendering result.

Neither exact asset rectangles nor general layout/draw behavior is established from point-query success. A query on a specific subtree and a GUI-root query have different starting nodes and traversal admissions. Cached manager point reads do not certify the current OS cursor position. An OS capture call, the GUI override pointer, popup pointer, previously selected control and target hit are distinct operands. No physical/action binding semantics, OS key/button-state convention or imported device guarantee is supplied.

Native binary32 operations and the explicitly discussed flag branches are established. The finite descriptions of the deeper widget point predicates are conditional on ordinary finite provider operands; this contract does not assign a universal unordered or exceptional result to every untraced geometric provider. Native integer widths, wraps and stored comparison operands are used without importing C++ overflow assumptions. Invalid/unrepresentable conversions, floating exception masks and rounding configuration remain limits.

The normal statements assume valid live native receiver graphs, valid selected collection headers/indices and normally returning reached calls. They do not supply recovery for null ordinary roots, malformed lists/trees, stale cursors, dangling callback captures, invalid indices outside the checked domains, source-byte/padding values not initialized on a path, corrupted item record counts, allocation failures or unwinding. The entered W+0x3C8 critical section protects only the stated normal call interval; no general Game/UI transaction lock is proved.

Specific price-call intervals retain an actor receiver across the call. In the selected-slot and equipped-item sale routes, the ordinary other-grid purchase route and the selected-record repurchase route, the relevant E is captured before the price sample and that captured object's +0xAAC is used after the sample returns. The release affordability and debit blocks likewise capture E before their respective 0x277BC0 calls; that cost helper independently reads its widget's stored Game graph. These statements do not mean that every callee is followed by an E reload. Later slot, location, root and scalar operations use their separately identified loads.

Calls and provider returns can precede later reads of G, current E, widget roots, active property indices, GUI point/capture state, callback maps and linked-list nodes. A raw pointer capture or retained local pointer does not establish safe deletion/replacement across such work. The inspected window procedure, Game owners and GUI adapters do not provide a whole-message-burst snapshot, scheduler ordering, cross-thread lifetime protection or universal reentrancy exclusion. No wall-clock relationship between native phases beyond the explicitly sampled counter words is asserted. Alternate callers, later writers of the globals/fields and enclosing external protection are not exhaustively enumerated.

Exposure statement: the author read only the standalone source-author policy, this isolated assignment and filename-selected supported original Steam executable/export evidence and metadata. No active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, combined history, replacement engine/exporter/extractor, SDK, dependency or reconstruction source, project tests/results, owner files/data/outputs/captures, installation configuration, assets or packages was read. Every prior held artifact remained unread and unchanged. No original executable was executed, no agent was spawned and no implementation or verification feedback was requested or consumed.

This complete independent code-free artifact is released only after exclusive creation, entire saved-file readback, exact length/hash verification and rechecking the supported original identity. At the separately reported FINAL / STABLE / HELD boundary all artifact edits stop.
