# Interface menu-button and Back callbacks — supported Steam image

FINAL / STABLE / HELD after complete saved-artifact readback and the accompanying hash report. Independent original-source contract, 2026-11-09.

## Authority and exposure

The supported authority is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe: AMD64 0x8664, 5,757,952 bytes, preferred image base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Metadata, raw image and hash were independently checked in this original-only assignment. Addresses below are RVAs.

Only the standalone docs/CLEANROOM_AUTHOR.md policy, the source-side assignment and individually selected original executable/export evidence were read. No active AGENTS.md, memory, CLEANROOM.md, combined history, project implementation/exporter/extractor, tests/results, owner outputs/data/captures, packages, SDK/dependency/reconstruction source or previous held handoff was accessed. No implementation or runtime feedback was requested or received. No additional agent was used. All earlier held artifacts, including the separately completed attachment-field clarification, remain unread and unchanged. That separate prose was not used as evidence here.

The complete selected callback targets are 0x102F10, 0x102FE0, 0x1031E0, 0x103150, 0x1030C0, 0x102FA0 and 0x9DDB0. The last two have no decoded function export in the selected metadata; their raw/native closure and the remaining metadata gap are expressly distinguished below. Allocation, shape, layout, item-generation and imported provider internals are bounded rather than assigned guessed semantics.

## Concrete receivers and ordinary setup

G is the Game receiver at constructor 0x7EA80. I is its embedded interface at G+0x118. The constructor call at 0x7EB1E passes G and I to 0xF8690; that complete helper stores G as the 64-bit pointer at I+0x5C0. The setup call at 0x7F4D4 passes I to 0xF9AC0. The selected setup body preserves that receiver and passes it to the six menu bindings. In contrast, the Back binding explicitly passes the pointer at I+0x5C0: its target receiver is G.

For a widget pointer, its node is through widget+0x178. For a node N, the compared or stored selected integer is obtained through N+0x68, that receiver's signed 32-bit index +0xAC and its integer-array pointer +0xE0. All such element accesses below are 32-bit with a four-byte stride, and sign-extend that index. The callbacks normalize a toggle to zero or one based on whether the previous integer equals zero. Negative or other nonzero integers take the same toggle-off side. These bodies do not validate arbitrary pointers or indices.

The alias table follows only the concrete Game/interface relationship:

| Target | Literal background label in six-button setup | Captured receiver | Direct toggled operand | Game alias |
| --- | --- | --- | --- | --- |
| 0x102F10 | crafting-menu | I | Node of widget I+0x228 | Widget G+0x340 |
| 0x102FE0 | inventory-menu | I | Node of widget I+0x218 | Widget G+0x330 |
| 0x1031E0 | worldmap-menu | I | Byte I+0x550 | Byte G+0x668 |
| 0x103150 | system-menu | I | Node of widget I+0x240 | Widget G+0x358 |
| 0x1030C0 | multiplayer-menu | I | Node of widget I+0x270 | Widget G+0x388 |
| 0x102FA0 | help-menu | I | Node of widget I+0x278 | Widget G+0x390 |
| 0x9DDB0 | Back caption | G | The prioritized direct-node transitions below | G-relative direct nodes |

The labels above come from original UTF-16 literals used to replace background content, not from inferred screen meanings. At setup, a menubutton template is cloned through 0x3308E0 into the direct root at I+0x108 six times. Returned node pointers are appended through 0x101A00 to the vector I+0x128..+0x130, with loop indices zero through five. Indices map to crafting, inventory, worldmap, system, multiplayer and help in that order. Each selected component at node+0x70 receives event identifier 2, captured I, the corresponding target and both supplied flag bytes one through 0xF8200.

The separately cloned Back node is stored at I+0x340 under the current GUI root. Its caption is assigned the original literal Back through 0x334C50. Event identifier 2 is registered through 0xF8410 on its component at node+0x70, with captured G, target 0x9DDB0 and both supplied flag bytes one. This is direct receiver provenance; 0x9DDB0's +0x200 is a Game field, not I+0x200.

Selected deep cloning 0x3308E0 calls the source component's virtual copy slot +0x140 when present, then makes the clone's effective node+0x78 pointer equal its own node+0x70 control. A node without an own component can inherit a parent's effective control through +0x78. Child nodes with distinct controls obtain their own propagated event binding through the binder recursion. No generic UI asset/layout contents or all possible component classes are assumed.

I+0x308 is the separate byte alias G+0x420. Constructor 0xF8690 initializes it to zero and initializes the two bytes I+0x550/+0x551 to zero. None of these seven complete target bodies directly stores I+0x308/G+0x420. A page-node transition must not be silently equated with clearing that byte.

## Registration, invocation and lifetime

The complete binders 0xF8200 and 0xF8410 search the receiver's tree at control+0x188 by signed 32-bit event key. Missing keys are constructed/inserted through 0xF6CC0 and 0xF7170. The selected event payload at tree-node+0x28 is passed to 0xF8170 for the six buttons or 0xF8380 for Back.

Those complete payload constructors first pass a nonnull old payload pointer to the original deallocation boundary 0x393488 with size argument 0x10, then request a new 0x20-byte object through 0x392BAC. A null allocation publishes a null payload. Otherwise they store the supplied byte at object+8, table pointer at object+0, captured 64-bit receiver at +0x10 and 64-bit target address at +0x18, then publish the object pointer in the event entry. The two tables are 0x441BB8 and 0x441BA8 respectively. Both first entries are the original leaf 0xF99A0.

The complete 11-byte leaf 0xF99A0 loads the captured receiver from callback-object+0x10 and tail-transfers to the address at callback-object+0x18. It performs no new enabled, page, focus or byte test. It neither increments a reference count nor takes a lifetime lock. Other incoming argument registers are not the target bodies' state contract.

The fifth binder argument is the recursion gate. When nonzero, the binder obtains a temporary child-control list through 0x32AC80, visits it forward and recursively installs the same event, captured receiver, target and flag bytes on each returned control, then destroys that temporary list. This is repeated installation with separate payload allocations; it does not share a single callback allocation across every descendant.

0x32AC80 constructs the list and calls 0x32ABA0 starting at the control's node. The complete traversal rejects node flag bit 2. At the root or a node without an own component it traverses children forward. At another node with a component at +0x70 it appends that component and stops descending on that traversal step; recursive binding then handles that component's own descendants. It does not require current selected integer or enabled byte for registration. List/allocation errors remain original runtime boundaries.

The captured G/I addresses are raw retained pointers. The selected evidence proves replacement/allocation and invocation order, but not a global teardown census, safe concurrent replacement, callback-owned Game lifetime, allocator non-reentrancy or recovery after arbitrary construction failure.

## Event-2 admission and work around the callback

The selected ordinary left-press path is interface 0x101020 to GUI owner 0x340EC0. The adapter first copies current GUI cached Y to binary32 I+0x548, then tail-calls the GUI owner for button zero. Its empty-target right-button clear of I+0x308 belongs to another branch; it is not an automatic tail of event-2 dispatch.

In 0x340EC0, U denotes the current GUI manager from G+0x20. It sets bit zero in the 32-bit mask U+0x1A8. It chooses a node from nonnull U+0x168, otherwise calls 0x333F60 on current root U+0x140 and the cached binary32 point U+0x180/+0x184 with flags zero. No fresh OS coordinate is decoded by this selected GUI owner.

The complete hit owner has independent selected-integer/node-flag/shape/transform/clipping admissions and reverse child selection. A current override bypasses that query. Its exact shape/transform numerical closure is outside this callback contract; no universal rectangle or physical-capture interpretation is imposed.

Before event 2, the GUI owner may assign U+0x1B0 from the target's +0x78 control when that control is enabled and has flag bit 5 at +0x150, then broadcast through 0x33C360 and the leaf at 0x33AAC4 to component virtual slot +0xE0. Otherwise U+0x1B0 is nulled. A present popup whose node is not an ancestor of the carried target, according to 0x3308C0, is hidden by 0x342270(U,zero,zero). The reached zero-popup branch clears the previous popup node's selected integer and stores null at U+0x178. The helper's alternate nonnull placement path is not reached by this call.

If the target is null or its +0x70 component differs from focus pointer U+0x170, nonnull old focus first receives byte +0x15C one and binding event 0x13 through 0x342560; U+0x170 is cleared after that callback. The comparison is with node+0x70, not effective control +0x78. Thus prior focus or broadcast callbacks can run before the menu's event-2 lookup.

For a nonnull target, enabled effective control node+0x78 first receives direct virtual slot +0x58. After that virtual call, the event-binding phase rereads fields on the carried target node. It requires node flag bit 1 clear, a nonnull current effective control, and a nonnull payload returned by 0x32A210(control,2). That helper can insert a missing default event entry; it is not necessarily a read-only lookup. An enabled control passes; a disabled control only passes when payload byte +8 is zero. Ordinary setup stored that byte as one, so an unmodified ordinary payload requires enabled control.

The admitted object is invoked through virtual slot zero and reaches the target via 0xF99A0. There is no second selected-menu or Game+0x420 test inside the thunk. A direct callback target also has no implicit event-2 gate merely because setup bound it to that event.

After the callback, 0x340EC0 reloads current U+0x140 and broadcasts component virtual +0xE8 through 0x33AAD0/0x33C360. Root and recursively visited nodes require nonzero selected integer and node flag bits 2 and 1 clear; the component is visited before children, and children are visited forward. Callback changes to selected state therefore affect this subsequent traversal. The carried target/control pointers and mutable child traversal are not protected by a whole-event snapshot.

The seven target bodies below do not directly clear GUI focus U+0x170 or node override U+0x168. Focus/popup/press-control work is attributable to the selected caller above. Transitive layout, imported and arbitrary virtual callback effects remain bounded. The contract makes no universal guarantee that callbacks cannot reenter dispatch or change later receiver operands.

## Complete seven target bodies

0x102F10 toggles the selected integer of widget I+0x228's node. It always loads captured Game through I+0x5C0 and calls 0xB2180, even on the toggle-off side. After that helper returns it reloads widget I+0x228, node and indexed integer. A zero current integer returns. A nonzero current integer calls 0x280A40 on the then-current widget, reloads I+0x228 again, and tail-calls 0x280C80. The decision is based on the post-helper integer, not the toggle result carried before 0xB2180.

0x102FE0 toggles widget I+0x218's node integer, then reloads that widget/node/integer. A zero value ends. On a nonzero value, it reads W through current captured Game+8 and P through W+0x448. It copies two 32-bit player words P+0x150 then P+0x154 into widget+0x1F0 then +0x1F4. It calls 0x280A40 with a fresh I+0x218 load, reloads that widget for 0x280C80, then loads current widget I+0x240 and clears its node's selected integer. No further selected-state test after the refresh calls gates that final clear.

0x1031E0 tests byte I+0x550 and writes one when zero, otherwise zero. That complete 17-byte owner has no helper call, selected-node test, position reset or I+0x308 clear.

0x103150 toggles widget I+0x240's node integer, then reloads it. Nonzero clears widget I+0x218's current node integer. Zero simply returns. There is no helper/provider call between those stores in this complete body. It does not hide the other four menu operands.

0x1030C0 first loads captured Game and calls 0xB33F0. Only after that returns does it load current widget I+0x270, node and selected integer, then toggle it to normalized zero/one. Thus provider/list refresh precedes the toggle on both opening and closing. The body does not clear I+0x308, or require provider success before reading/toggling the node after a normal return.

0x102FA0 loads widget I+0x278, its node and indexed selected integer, and toggles that integer to normalized zero/one, then returns. The complete 51-byte native leaf contains no call or page/character-reset operation. Its absence as a named decoded function in the selected export is not evidence that the registered Help target does nothing.

0x9DDB0 uses G directly and prioritizes four direct-node integers:

| First admitted condition, in evaluation order | Ordered 32-bit stores |
| --- | --- |
| Direct node G+0x200 has nonzero selected integer | Clear that node's current integer; load direct node G+0x198 and set its selected integer to one. |
| Otherwise direct node G+0x198 is nonzero | Clear its current integer; load direct node G+0x130 and set its selected integer to one. |
| Otherwise direct node G+0x170 is nonzero | Clear its current integer; load direct node G+0x130 and set its selected integer to one. |
| Otherwise direct node G+0x130 is nonzero | Clear its current integer; load direct node G+0x118 and set its selected integer to one. |
| All four are zero | Return without a state store. |

The tested Game node fields alias I+0xE8, I+0x80, I+0x58 and I+0x18 respectively; G+0x118 is I's direct node at offset zero. It does not test G+0x118 before returning, clear all pages, enforce exactly one selected page, or inspect a byte named Back. It contains no call, indirect transfer, decoder, character rebuild, World operation, focus/capture write or Game+0x420 write. No saved screen-stack convention is used to describe it. Its branch join at 0x9DE0B deliberately serves the G+0x198 and G+0x170 clear routes with the component/array operands prepared by the respective route; it must not be assigned exclusively to the nearby G+0x198 load.

### Back and Help native closure despite export gaps

Neither 0x9DDB0 nor 0x102FA0 has a matching C export or function-range entry in the selected functions/functions_ranges metadata. The selected assembly export does decode the complete Help leaf at 0x102FA0..0x102FD3 with unassigned function ownership. Those 51 bytes and all eleven decoded instructions were matched against the supported raw image.

For Back, the selected assembly export ends the preceding function 0x9D6F0 at its return 0x9DDAA and resumes at a different function 0x9DE90. The executable exception-directory entries independently bound 0x9D6F0 at exclusive end 0x9DDAB and the next unwind-described body beginning 0x9DEA0; there is no runtime-function entry covering 0x9DDB0. This is compatible with a small leaf, but does not supply an exported owner name or unwind extent.

The registered Back target was therefore decoded directly from raw RVA 0x9DDB0 through its terminal return at 0x9DE8E, exclusive end 0x9DE8F: 223 bytes, 41 instructions. All instruction boundaries cover that span without holes or overlaps. There are no calls or indirect branches. The six transfer sites and destinations are 0x9DDD0→0x9DDEC, 0x9DDE7→0x9DE75, 0x9DE09→0x9DE22, 0x9DE20→0x9DE75, 0x9DE3F→0x9DE0B and 0x9DE5E→0x9DE8E. Every destination is an independently decoded instruction boundary inside the same closed leaf. The final return is followed by raw byte 0xCC; the distinct decoded 0x9DE90 function was not borrowed as part of Back.

The manually decoded instructions' expected byte sequences were concatenated over the complete 223-byte span and compared with the raw image, with no mismatch; its span SHA256 is 1ED95B504E8755F730F3AB7439E7FC3B532B73801D213D9FE70CD21325AEEBF0. This closes the reached target's instruction behavior. The remaining gap is formal decoded-export/function-owner attribution and an exhaustive census of other entries into that leaf, not a manufactured body from neighboring routines. No claim about arbitrary execution beginning mid-instruction is made.

## Reached helper effects and explicit closure boundaries

The callbacks' visible state order includes their synchronous helpers. This section closes their selected owners' outer mutations and provider admissions; it does not claim a universal numerical/item/provider closure.

### 0xB33F0 before the multiplayer toggle

The Game helper calls SteamInternal_ContextInit using original context descriptor 0x5314D8, then provider slot +0x18 with 32-bit argument 4. It carries that signed 32-bit count; exactly -1 is replaced by zero. It obtains current widget G+0x388, clears its record range through 0x6FE20 and assigns its 64-bit end at widget+0x200 equal to begin at +0x1F8 before enumerating. A zero or other negative count performs no enumeration but still reaches the final widget refresh.

For signed indices zero through count-minus-one, in increasing order, the helper calls context initialization again and provider slot +0x20 with an eight-byte output address, index and argument 4. Let the returned low/high DWORD pair be L/H. Admission requires H bits 20..23 between one and ten inclusive, and the top byte between one and four inclusive. Type one additionally requires L nonzero and H's low twenty bits below five; type seven requires L nonzero and those bits zero; type three requires L nonzero. The other admitted numeric types have no extra L test here.

Each admitted pair makes another context call then provider slot +0x38 with the combined 64-bit value. The returned byte-string pointer is measured to its terminator, copied through 0x3D280, converted through 0x349C30, combined with the identifier by 0x7D520 and appended through 0xA1B00 to current widget G+0x388+0x1F8. Temporary strings/records are cleaned before the next index. There is no null-pointer check or provider-result Boolean gate invented for these slots. Concrete service-interface identity, validation beyond these masks, conversion behavior and returned strings remain original provider/helper boundaries.

After the loop, 0x27B490 is called on current G+0x388. That complete widget owner requires a nonnull +0x2E0 component pointer. It reads its size/metric dependencies through 0x329E00 and 0x32A1D0, computes the selected record-count/scroll geometry using 0x28-byte records, signed 32-bit +0x2E8 and a 30-unit divisor, and reaches 0x32C680 on the component at widget+0x2E0+0x70. Its two branches distinguish whether converted available rows plus one are less than the unsigned record count. The geometry/query helpers, nonfinite conversions and malformed extents are bounded; no focus/capture/page clear is directly stored in this owner.

The callback then toggles current I+0x270. Refreshing an empty returned list still precedes a normal toggle; toggling off still performs this provider work.

### 0xB2180 even when crafting toggles off

This complete Game helper first reads current W and P. If the inventory-row vector pointers P+0x9F0 and P+0x9F8 are equal, it returns without the later rebuilding/RNG calls. No broader malformed-vector or minimum-row-count validation is implied.

Otherwise it prepares six local vectors of 0xC0-byte records. It traverses the forward list through CharacterData pointer P+0x1528, subobject +0x18. Each source payload at link+0x10 is submitted to 0x2AEC30 with W, a temporary output and byte zero. A successful result is categorized using the source payload's type byte and subtype DWORD: type 3 goes to local group zero; type 0xB to group five; type 1 subtypes 1..3 or 0xC go to group four and other type-1 subtypes to group three; type 0x1C subtypes 4..6 go to group three and its other subtypes to group four. Other types do not append through this branch. Reached output append uses 0xA1BD0. Temporary owned output ranges are cleaned after each source record. W and the current list sentinel are reread at the loop continuation.

The owner next makes a directly reached srand call using current P's two 32-bit words +0x150/+0x154. The original 32-bit mixing order is: multiply the second word by 57, add 0xF5F9, then add the first word; XOR that result with its low-width product by 0x2000; square the folded value, multiply by 0xEC4D, add 0x131071F, multiply by the folded value, subtract 0x2DF722F3 (equivalent low bits to adding 0xD208DD0D), clear bit 31, perform the original unsigned high-product with multiplier 0x61A8F94D followed by right shift 13, then subtract that quotient from 100000. These are machine-width operations. Over the admitted low-31-bit dividend, the high-product/shift quotient equals unsigned division by 0x53E2; this was independently checked at both endpoints of every quotient interval. The imported provider sequence after seeding is not guessed.

Indices zero through four are then processed in order. 0x2B4260(W,index) is called first for each; zero skips that index's generation body. A nonzero result recomputes the same current-player pair mix and reseeds with its result plus the index. It constructs a type-3 temporary source with copied eight-byte P+0x150 pair, the index in source byte +0x14, other expressly initialized fields and fresh output. Current player byte +0x14C selects the following directly reached source/query sequence.

| Player byte +0x14C | Ordered type-3 subtype and auxiliary-byte preparation before queries |
| --- | --- |
| 1 | A rand result's signed remainder by three supplies first subtype, auxiliary byte +0x1C becomes one, then a fresh rand result goes to source DWORD +8 before query; subtype 13 gets another fresh +8 draw/query; a further rand remainder by three plus 15 supplies the third subtype, then a fresh +8 draw/query. |
| 2 | Subtypes 6, 7, 8, auxiliary byte two; each takes a fresh rand result at source+8 then a query. |
| 3 | Subtypes 11 and 10, auxiliary byte two, each with a fresh +8 draw/query; subtype 12 first draws the sign-adjusted parity result to choose auxiliary byte 11 or 12, then takes a fresh +8 draw/query. |
| 4 | Subtypes 3, 4, 5, auxiliary byte one; each takes a fresh +8 draw/query. |
| Other byte | No type-3 class-specific query in this branch. |

Every query in this table is 0x2AEC30 with current W and byte zero; only true results append through 0xA1BD0 to group zero. The table does not assume rand returns only a particular numeric range. Sign-adjusted parity is converted to a nonzero Boolean for the auxiliary choice. Transitive draws and effects inside 0x2AEC30 remain unresolved.

After that class-specific branch the owner rereads current P+0x14C to set the common auxiliary byte to 1, 0x1B, 0x1A or 0x1C for player values 1,2,3,4 respectively; other values retain the currently carried byte. It sets subtype DWORD zero and submits types 4,7,5,6 in that order, each preceded by one new rand value at source+8 and followed by the same success-only append to group one. Type 8 then takes one parity draw for auxiliary byte 11/12, a separate +8 draw and a query to group two. Type 9 repeats that two-draw/query order. This is directly visible owner order, not a complete imported RNG or generation specification.

After all five indices, 0xB2EE0 is called with group numbers zero through five in order. The owner ensures at least seven rows at G+0x17A8 through 0x64F30 when necessary, then sets row six's 64-bit end equal to its begin. It clears the owned tree structures at G+0x1938 and G+0x1948 through the reached subtree cleanup/free loops, resets each head's three links to its sentinel and stores 64-bit zero at G+0x1940 and G+0x1950. The selected later inventory-row loop has no body mutation. It refreshes widget G+0x340 through 0x280A40 then 0x280C80 and destroys all six local result vectors. Allocator/length-error and deeper generation/container helper effects remain explicit boundaries.

0xB2EE0, whose complete selected body was inspected and byte-matched, ensures its requested row exists, sets that row's end to begin, then visits input 0xC0-byte records forward. For each required 0xA4-byte item record it sums matching current inventory counts using 0x4DF60 and divides by that record's signed required-count DWORD. It carries the minimum result; no requirements select count zero. It appends that count and the source item through 0x35400 into a 0xA4-byte destination record and advances the row end by 0xA4. No zero-divisor or overflow safety check is inferred for malformed inputs.

If the current group/record ordinal matches widget G+0x340's two signed DWORDs +0x278/+0x27C, 0xB2EE0 also publishes the selected detail to current widget G+0x348: type byte +0x1F8, DWORDs +0x1FC/+0x200/+0x204/+0x208, byte +0x20C, DWORD +0x210, byte +0x214, 128 selected bytes +0x215..+0x294, tail byte +0x295 and DWORD +0x298. It copies the associated owned requirement range through 0x82F10 at +0x2A0 and writes the two selected index DWORDs +0x2B8/+0x2BC. Other record fields/padding are not implicitly zeroed.

This helper explains the toggle's repeated-frame consequence: 0x102F10 always performs its Game rebuild call; when the post-helper selected integer is nonzero, it makes another pair of widget refresh calls after the helper's own pair. A closed node alone does not suppress the rebuild call or its admitted RNG/provider work.

### Widget refresh reached by inventory and crafting

0x280A40 returns immediately when widget+0x1E8 is null. Otherwise it calls 0x2806A0 on the widget, then 0xB3ED0 using the pointer at widget+0x210, then obtains size inputs through 0x32A1D0 and 0x329E00. Its column count is truncated from the first selected float size minus original constant 0x42E598, divided by converted signed widget+0x2A8 plus five. A nonpositive column count ends without the later scroll-count writes.

The positive-column route resizes the integer vector at widget+0x288 to its 0x18-byte row count through 0x485D0 with zero default. A nonempty row-vector and selected signed widget+0x2B4 below that row count select a row. An empty selected row stores DWORD zero at widget+0x2A4 and its current +0x288-vector integer and returns. Other selected rows compute display count from either all records or a filtered count: widget+0x280 equal to zero and unsigned selected row below two use only nonzero-count records whose two DWORD item fields match widget+0x1F0/+0x1F4. It derives the needed row count using the column count, the other selected float size minus constant 0x42E5AC, and signed widget+0x2AC plus five; stores +0x2A4 with minimum one; then caps the selected +0x288 vector integer to +0x2A4 minus one. Missing/invalid selected rows take the owner's +0x2A4-one fallback where reached. Arbitrary negative indices and nonfinite conversions are not sanitized by a generic inferred GUI rule.

0x2806A0 acts only when widget+0x280 is zero. It constructs a temporary keyed set from the selected inventory row's nonzero-count records' eight-byte field at item+0xC and the current P+0x150 pair. It resets widget+0x200 to its vector begin +0x1F8, walks the selected set in native successor order, appends each eight-byte pair with the selected vector growth/error handling, calls 0x27C420 to order the resulting vector with widget context, then destroys the temporary set. The deeper set/comparator semantics are bounded; they are not page or focus state.

0xB3ED0 synchronously operates on current player's inventory rows: sort/query boundary 0x7B880 on row zero, 0x7B580 for rows two onward in increasing order, then 0x7B700 on row one. It walks each row, counts nonzero 32-bit slot counts and calls 0xA2540 for its selected resulting row length. Those sorting/matching comparator closures are excluded; this is enough to distinguish a real inventory mutation helper from a pure geometry callback.

0x280C80 first requires nonnull widget+0x1E8 and calls 0x32A1D0. It then requires nonnull widget+0x248, nonzero signed DWORD +0x2A4 and a nonempty integer vector +0x288..+0x290. It computes selected geometry from 0x329E00's float size minus constant 0x42E7BC, signed division by +0x2A4 and current selected integer at vector index +0x2B4. A denominator below two takes its separate fixed-position route; a denominator at least two uses the selected scroll integer and denominator-minus-one arithmetic. Both reach 0x32C680 on widget+0x248+0x70 with byte one. The geometry helpers and original constants are not assigned an imported font/scissor meaning here.

These refresh bodies directly mutate counts, vectors, selected detail and component geometry. They do not directly store the menu byte I+0x308/G+0x420 or a GUI focus/override pointer. Untraced virtual/layout/provider calls may have other effects; no universal absence claim extends through those boundaries.

## Selected alternate callers and admissions

The scoped viewed-update caller in 0xA5A50 forms I from G+0x118 at 0xAAF6C. Once execution reaches that scope, nonzero current chat widget G+0x398+0x1D8 skips the action group. Nonzero selected integers of direct nodes G+0x118, G+0x130 and G+0x170, tested in that order, also skip it. These are caller-specific gates, not admission built into the targets.

Within the admitted group, zero prior global byte and nonzero current action byte make the following direct calls in order: 0x5514FC/G+0x1A7F→0x1031E0 at 0xAAFCC; 0x5514FD/G+0x1A81→0x102FE0 at 0xAAFF5; 0x5514FE/G+0x1A82→0x102F10 at 0xAB07F; 0x551500/G+0x1A84→0x1030C0 at 0xAB10F; 0x551501/G+0x1A83→0x103150 at 0xAB137. After each tested action, its global byte is assigned the then-current action byte. Skipping the outer group does not perform those assignments in this scope. Between calls the owner also publishes related widget-node selected values and handles its separate G+0x1A80 edge. This is an ordinary direct route independent of event-2 binding, with no GUI focus/capture check in the selected group.

The complete selector helper 0x102DB0 is another bounded original route. It writes its supplied 32-bit selector to I+0x30C, clears selected integers of widgets I+0x218, +0x228 and +0x270 in that order, then rereads I+0x30C. Value zero reaches 0x102FE0; one tail-reaches 0x102F10; two calls 0xB33F0 then toggles current I+0x270; other values return after the clears. It neither imposes a general menu exclusivity rule on the six event callbacks nor proves every caller to those entries.

The selected direct-call export identifies these update and selector-helper references; Help and Back are established by address registration and the callback thunk rather than by a guessed direct CALL entry. Unassigned and indirect alternate callers remain possible. Full large Game update, message pump, rendering, controller and other menu setup routines are not made universal by these scoped edges.

## Independent conditional sequences

With valid distinct nodes and no intervening writer, two calls to the map target toggle G+0x668 zero→one→zero while leaving G+0x420 unchanged in that target. No position reset is implied by either call.

If inventory and system selected integers start zero and the inventory target opens, it copies the current two player words, refreshes the inventory widget, then clears the system node. If a synchronous refresh changes that system-node pointer, the final clear uses the post-call pointer. Opening system afterward clears the current inventory node but need not close crafting, help or the map byte.

If the crafting integer starts one, its callback first writes zero and still calls 0xB2180. With nonempty ordinary inventory, the helper can mutate result rows and RNG state even though the post-helper target stays closed. If that helper or a permitted callback changes the selected integer back to nonzero, the outer callback's fresh test admits its extra refresh calls.

If the multiplayer provider count is zero, the old list is cleared, the final refresh is still called, and a normally returning callback toggles the current multiplayer node. Provider count zero is not an early exit from the callback.

For distinct direct page nodes with only G+0x200 selected, successive Back calls with no intervening writer select G+0x198, then G+0x130, then G+0x118. A further call makes no store when all four tested nodes are zero. Starting with only G+0x170 selected instead reaches G+0x130, then G+0x118. These are operand-based paths, not assumed page names or a screen stack.

If both G+0x200 and G+0x170 start selected, one Back call clears only G+0x200 and sets G+0x198; G+0x170 remains selected. This demonstrates why the body does not establish a one-selected-page invariant.

An ordinary event payload with stored byte one and a currently disabled effective control fails event-2 binding admission. A prior direct virtual press or focus callback can change enabled state or the payload before its later lookup. A direct call to a target itself does not apply that GUI admission.

## Evidence manifest, constants and bounds

Original authority files were individually identified before content reads: the supported executable; decompiled/assembly.txt; original functions.csv, function_ranges.csv and calls.csv; and the selected decompiled/functions/140 plus six-digit lowercase RVA .c files. No prior prose handoff was read. Function metadata selected bodies; instructions resolved receiver, width, branch joins, tail calls and incomplete decompiler arguments before behavioral interpretation.

Each recorded exported instruction byte was compared to the raw-backed original image using the original PE section mapping. The 31 complete named owners below contain 14,030 matched bytes and 3,436 instructions, with zero mismatches. Fragmented ranges are preserved, and end addresses are exclusive. All five exported menu callbacks are covered in full; the two unexported callback targets are covered separately by complete native spans.

| Complete named owner RVA | Original range(s), exclusive ends | Matched bytes | Instructions |
| --- | --- | ---: | ---: |
| 0x102F10 | 0x102F10–0x102F9D | 141 | 30 |
| 0x102FE0 | 0x102FE0–0x1030B6 | 214 | 40 |
| 0x1031E0 | 0x1031E0–0x1031F1 | 17 | 4 |
| 0x103150 | 0x103150–0x1031D7 | 135 | 25 |
| 0x1030C0 | 0x1030C0–0x10310D | 77 | 18 |
| 0x0F8200 | 0xF8200–0xF837E | 382 | 99 |
| 0x0F8410 | 0xF8410–0xF858E | 382 | 99 |
| 0x0F8170 | 0xF8170–0xF81F2 | 130 | 35 |
| 0x0F8380 | 0xF8380–0xF8402 | 130 | 35 |
| 0x0F8690 | 0xF8690–0xF87CA | 314 | 67 |
| 0x0B2180 | 0xB2180–0xB29B9; 0xB29BA–0xB29C0; 0xB29C1–0xB29C7; 0xB29C8–0xB29CE; 0xB29CF–0xB29D5; 0xB29D6–0xB2C35; 0xB2C36–0xB2C51; 0xB2C52–0xB2C61; 0xB2C62–0xB2C71; 0xB2C72–0xB2C7E; 0xB2C7F–0xB2CD7; 0xB2CD8–0xB2CF3; 0xB2CF4–0xB2D03; 0xB2D04–0xB2D13; 0xB2D14–0xB2D20; 0xB2D21–0xB2D76; 0xB2D77–0xB2D92; 0xB2D93–0xB2DA2; 0xB2DA3–0xB2DB2; 0xB2DB3–0xB2DBF; 0xB2DC0–0xB2EDF | 3403 | 852 |
| 0x0B2EE0 | 0xB2EE0–0xB33E7 | 1287 | 281 |
| 0x0B33F0 | 0xB33F0–0xB3652; 0xB3653–0xB3659; 0xB365A–0xB3660; 0xB3661–0xB3667; 0xB3668–0xB3698 | 676 | 170 |
| 0x280A40 | 0x280A40–0x280C7C | 572 | 135 |
| 0x280C80 | 0x280C80–0x280E29 | 425 | 89 |
| 0x2806A0 | 0x2806A0–0x280A31 | 913 | 243 |
| 0x0B3ED0 | 0xB3ED0–0xB40C3 | 499 | 125 |
| 0x27B490 | 0x27B490–0x27B69B | 523 | 122 |
| 0x32AC80 | 0x32AC80–0x32ACE3 | 99 | 26 |
| 0x32ABA0 | 0x32ABA0–0x32AC7D | 221 | 58 |
| 0x32A210 | 0x32A210–0x32A2B5 | 165 | 46 |
| 0x340EC0 | 0x340EC0–0x3410D9 | 537 | 138 |
| 0x33C360 | 0x33C360–0x33C407 | 167 | 49 |
| 0x3308E0 | 0x3308E0–0x330A78 | 408 | 115 |
| 0x102DB0 | 0x102DB0–0x102EAF | 255 | 57 |
| 0x101020 | 0x101020–0x10111F | 255 | 58 |
| 0x3308C0 | 0x3308C0–0x3308DC | 28 | 13 |
| 0x342270 | 0x342270–0x342557 | 743 | 161 |
| 0x342560 | 0x342560–0x3425C0 | 96 | 29 |
| 0x333F60 | 0x333F60–0x33429B | 827 | 215 |
| 0x33AAD0 | 0x33AAD0–0x33AAD9 | 9 | 2 |

| Complete native span without named function attribution | Bytes | Instructions | Correspondence |
| --- | ---: | ---: | --- |
| Back target 0x9DDB0–0x9DE8F | 223 | 41 | Complete raw decode, all direct branches closed to internal boundaries/return; exact expected-byte comparison. |
| Help target 0x102FA0–0x102FD3 | 51 | 11 | Complete original unassigned instruction listing matched to raw image. |
| Invocation leaf 0xF99A0–0xF99AB | 11 | 3 | Complete original unassigned instruction listing matched to raw image. |
| Pre-press broadcast leaf 0x33AAC4–0x33AACD | 9 | 2 | Complete original unassigned instruction listing matched to raw image; component virtual +0xE0. |

These add 294 bytes and 57 instructions to the complete-body correspondence: 14,324 bytes and 3,493 instructions overall, zero mismatches. The seven requested callbacks themselves comprise 858 bytes and 169 instructions. They are not conflated with generic nearby owner ranges.

Selected setup/caller start-address windows were also byte-matched separately. They are scoped edges, not complete claims for the large constructor/setup/update bodies. A last selected instruction can extend beyond a window endpoint.

| Selected caller/setup window | Matched bytes | Instructions |
| --- | ---: | ---: |
| 0x7EA80–0x7EB23 | 163 | 37 |
| 0x7F4C9–0x7F4D9 | 16 | 3 |
| 0xF9AC0–0xF9B60 | 166 | 30 |
| 0xFCAA0–0xFCB3A | 152 | 29 |
| 0xFFEE0–0x1003DF | 1274 | 244 |
| 0xAAF65–0xAB14F | 490 | 103 |

Raw pointer entries at 0x441BA8 and 0x441BB8 were checked as 64-bit image addresses for 0xF99A0. Raw UTF-16 label checks covered Back at 0x4410E0; crafting-menu at 0x441750; inventory-menu at 0x441770; worldmap-menu at 0x441790; system-menu at 0x4417B0; multiplayer-menu at 0x4417C8; help-menu at 0x4417F0; and background at 0x4414F8. Their association with captured targets is independently present in the selected setup instructions.

Relevant original scalar words checked in the image are 0x42E598=0x41200000 (binary32 10), 0x42E5AC=0x42200000 (40), 0x42E7BC=0x428C0000 (70), 0x46E620=0x41E00000 (28), 0x441BC0=0x420C0000 (35), 0x42E7B8=0x42700000 (60) and 0x42E938=0x43160000 (150). Relevant inline constants/event identifiers, 32-bit zero/one stores, byte-toggle stores, item/list/vector strides, provider argument 4, count normalizer -1, and the selected RNG mixer words were covered by complete original instruction-byte correspondence. Deeper metric/layout/item-provider numerics are not invented from these constants.

The original exception directory and raw boundaries distinguish the Back leaf from its neighbors. Formal exported owner attribution remains absent. All normal entry behavior reached from the proved registration target is closed; an exhaustive indirect-entry/caller census and all possible aliases remain unproved.

## Limits and hold

This contract specifies exact reached state/call/reread order for the seven callback entries, their ordinary registrations, the selected event-2 admission and bounded helper outer effects. It does not infer complete OS dispatch scheduling, GUI asset availability, provider/service internals, random sequences, font/shape/transform values, arbitrary malformed object/index safety, allocator exception recovery or thread/reentrancy/lifetime protection. Imported and original helper boundaries can do work not closed here; a negative direct-store finding is not generalized through an uninspected virtual/provider closure.

The complete Back and Help instruction behaviors are not substituted with menu conventions. Their export gaps and complete native methods remain explicit. Page selected integers, the map byte, the separate Game+0x420 menu byte, GUI focus/override and callback-object ownership are distinct operands with distinct proven writers.

After full saved-artifact readback, exact byte comparison, byte count and SHA256 reporting, this artifact is FINAL / STABLE / HELD. All writes to it stop at HELD. All preceding held artifacts remain unread and immutable.
