# Original Game map press early-return admission

This independent source-side contract describes the complete native Game press owner at RVA 0x94900, the GUI dispatch that precedes its two direct-node point queries, and the selected construction, placement, visibility and callback paths needed to interpret those queries. It establishes which ordinary returns bypass this owner's later byte clear on the widget at Game offset 0x398. It does not turn an asset-derived shape into an assumed rectangle.

## Authority, image and notation

The authority used was the supported original Steam executable at C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe and its individually selected original export evidence beneath the sibling decompiled directory. The image is 5,757,952 bytes, AMD64 machine 0x8664, preferred base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0 and image size 0x58A000. Its independently checked SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Addresses below are RVAs unless a field offset is explicitly indicated. The standalone docs/CLEANROOM_AUTHOR.md policy was read before original evidence.

G denotes the Game receiver supplied to 0x94900. I denotes its embedded interface at G+0x118. The ordinary constructor call at 0x7EB1E supplies I and G to 0xF8690, which stores G as the eight-byte pointer at I+0x5C0. The later constructor call at 0x7F4D4 supplies I to setup 0xF9AC0. This is receiver provenance, not an offset-based identification of an unrelated object.

For this construction, I+0x550 aliases byte G+0x668. I+0x288 aliases the eight-byte node pointer G+0x3A0; call that node P. I+0x290 aliases node pointer G+0x3A8; call that node A. I+0x298 aliases node pointer G+0x3B0; call that node B. The GUI manager pointer U is G+0x20. I+0x54C aliases binary32 G+0x664, and I+0x548 aliases G+0x660. These aliases do not make every field a Boolean or every node a widget.

A node's eight-byte pointers at offsets 0x58, 0x60, 0x68, 0x70 and 0x78 select its shape, transform, display object, own control and effective control respectively. Its parent is at 0x40 and its child-list sentinel pointer at 0x48. These relationships follow construction 0x32EE60, node creation 0x33CD30 and cloning 0x3308E0. The selected display integer is located using a signed 32-bit index at display+0xAC into the four-byte elements beginning at display+0xE0. Original display construction 0x3736C0 names that property visibility. Any nonzero selected integer satisfies the visibility tests here. The separate signed index at display+0x134 into four-byte elements at display+0x168 belongs to the original clipping property. Neither is inferred to be a selected menu page.

The other widget pointer G+0x398 and its byte at offset 0x1D8 are distinct from A and B. The task's chat terminology is retained only as a locator for that later byte clear; this audit does not derive the widget's entire input or text lifecycle.

## Complete press owner and the bypass

The complete owner is 0x94900 through 0x965DF, 7,392 bytes in one continuous exported native range. Its press-code input is the full 32-bit EDX value. It preserves that value before calling 0x101020 at 0x94953 with I as receiver. The native EDX argument survives into that call; the decoded presentation omitting the argument is not used to discard it.

After the GUI adapter returns, the owner reloads G+0x360, that widget's root node at offset 0x178, and that node's display pointer. It reads the current selected visibility integer and writes its nonzero test as one byte at global RVA 0x5514D7. This write precedes both early-return queries.

At 0x94983 it tests the current byte G+0x668 against zero. Zero goes directly to the ordinary continuation at 0x949B4, with no query of A or B. A nonzero value causes the current pointer G+0x3A8 to be loaded at 0x9498C and passed to 0x333E70 at 0x94993. A nonzero returned AL branches at 0x9499A to the common epilogue at 0x965AC.

Only if A's returned AL is zero does the owner load the current pointer G+0x3B0 at 0x949A0 and call 0x333E70 at 0x949A7. Nonzero AL then branches at 0x949AE to that same epilogue. A second zero proceeds to 0x949B4. G+0x668 is not retested between these two calls. B's pointer is not an entry snapshot; it is loaded after A's complete query returns.

There is no press-code restriction before this guard. It applies to every full 32-bit code supplied to this owner, including codes for which 0x101020 makes no GUI press call. This guard does not directly test G+0x420, a focused control, a captured control, P's visibility, a top-level menu root, an enabled byte on A's control, or a GUI-consumed result. A and B are passed as nodes themselves, without a widget-root offset of 0x178.

The ordinary continuation handles the remaining code-specific operations before a shared tail. Its early map-related work for code 2 can query 0x35EA0, find or update a coordinate-keyed entry and toggle byte G+0x1E54 while the current G+0x668 is nonzero. The code-0 branch additionally tests G+0x1DB0, current G+0x668, and the signed 32-bit sentinel 0x7FFFFFFF at the receiver through G+0x10, offset 0x150. Its direct state-changing route can write the current entity's fields at 0x1B0 through 0x1C8, byte 0x68 and 32-bit field 0x6C, then clear both G+0x668 and G+0x420. The G+0x668 clear is at 0x94DA5. These later branches are not prerequisites for either early point query.

The owner next samples the imported timeGetTime at 0x94DB3, subtracts the prior global 32-bit value at RVA 0x5514E8, and stores the new sample there. A difference below 200, current nonzero G+0x668, and a positive signed 64-bit field at the G+0x10 receiver's offset 0x110 admit the following view-field updates and 0x35EA0 query. G+0x1DB0 is cleared before the first 0x5FAC0 call at 0x94E8C. The following slot-count, item-record, linked-list, entity-field and widget branches were inspected for joins and returns; their transitive helper bodies are outside this admission contract.

Every normally returning path after 0x949B4 reaches the common work at 0x9651F. That work calls 0x5FAC0, performs a binary32 division by the earlier saved return and multiplication by the current entity field at 0x180, stores that field, calls 0x5FAC0 again and applies the next native comparison/store. It then loads the current G+0x398 pointer at 0x96581 and stores byte zero to its offset 0x1D8 at 0x96588. Next it reloads node G+0x310 and clears that node's current selected visibility integer with a four-byte store at 0x965A8. Only then does it enter the epilogue.

The complete native branch census within 0x94900 identifies only the two branches at 0x9499A and 0x949AE as ordinary branches directly to the epilogue that bypass this tail. The only RET is 0x965DF. There is no additional indirect jump constituting another normal return in this owner. The three length-error sites followed by INT3 at 0x94B74, 0x94FD3 and 0x9522F are exceptional/trap boundaries, not additional ordinary successful early returns. Unwinding, a nonreturning helper, a fault, invalid pointers, imported failure or security-cookie failure is not converted into a guarantee of normal completion.

An early return proves that this invocation skips its own 0x96588 clear and its own 0x965A8 clear. It does not prove that earlier GUI work or providers left the corresponding objects and bytes unchanged.

## GUI-first press adaptation and callbacks

The complete adapter 0x101020 first follows I+0x5C0 to G, reads the 32-bit cached word U+0x184 and copies it into I+0x548. It then dispatches code 0 to 0x340EC0, code 1 to 0x3415F0, and code 2 to 0x341300. Other 32-bit values return after that copy.

For code 1 only, after 0x3415F0 returns, the adapter rereads the current G and U pointers and calls 0x33F1E0. That helper returns U+0x168 if it is nonnull; otherwise it queries the current root U+0x140 at the cached point beginning U+0x180 with a zero 32-bit query mask. A null result makes the adapter clear byte I+0x308, alias G+0x420, and then clear the current selected visibility integers of the widget roots at I+0x218, I+0x228 and I+0x230 in that order. It does not directly clear I+0x550 in this branch. The adapter returns to 0x94900 regardless of whether any GUI callback ran.

All three complete GUI press owners first set a bit in the 32-bit word U+0x1A8: bit value 1 for code 0, 2 for code 1 and 4 for code 2. They use current node U+0x168 when nonnull. Otherwise they call 0x333F60 on root U+0x140 with the point at U+0x180 and mask zero. This GUI target determination is a separate call from the later Game queries of A and B.

Code 0 additionally assigns U+0x1B0 from the target's effective control only when that control is nonnull, its 32-bit flags at 0x150 have bit 5 set, and its byte at 0x1A0 is nonzero. Otherwise U+0x1B0 becomes null. When that assignment succeeds, it traverses the eligible GUI tree with 0x33C360 and the native thunk at 0x33AAC4, which calls the control virtual slot at table offset 0xE0. This provider work can occur before the later selected-target callback.

Code 0 then examines U+0x178. When nonnull and the chosen target is not within the parent chain rooted at that control's node at 0x178, it calls 0x342270 with null replacement and null anchor. The complete reached null-replacement branch first clears the old control-root node's selected visibility integer and then nulls U+0x178. The complete helper also contains nonnull-replacement attachment and positioning branches, but this selected caller does not enter them.

For each press code, a null target or a target whose own control differs from current U+0x170 causes the old nonnull U+0x170 control's byte at 0x15C to be set to one, dispatch of event 19 through 0x342560 on its current root node, and then a store of null to U+0x170. That event helper excludes a null node, node flag bit 1, a null effective control, a missing callback, and the disabled-control/nonzero-callback-flag combination. A callback can therefore run before the subsequent U+0x170 null store. The owning press routine does not stop because that callback ran.

For a nonnull target, the corresponding enabled effective-control virtual call occurs next: table offset 0x58 for code 0, 0x70 for code 1, and 0x88 for code 2. These use the node's effective control pointer directly. After return, the owner rereads the target node's flag bit 1 and its effective control pointer for callback admission. With bit 1 clear and a nonnull effective control, it looks up signed 32-bit event key 2, 5 or 8 respectively using 0x32A210. That lookup can insert a missing map entry; it is not a guaranteed read-only operation. A nonnull callback is invoked if the effective control's current byte 0x1A0 is nonzero or the callback's byte at 0x8 is zero.

The event-2 objects installed for A and B have flag byte one, so their selected target callback admission requires the current effective control's byte 0x1A0 to be nonzero. The later direct Game point query does not impose that enabled-byte predicate.

After the selected target work, each GUI press owner reloads U+0x140 and performs the eligible global notification traversal. The relevant thunks call own-control virtual table offsets 0xE8, 0x100 and 0x118 for codes 0, 1 and 2 respectively. Eligibility at the root and in 0x33C360 requires current nonzero selected visibility and node bits 2 and 1 clear. The visitor invokes the node's own control first, when present, then traverses children in forward list order. The callback-descriptor adjustment is signed 32-bit; these selected descriptors supply zero adjustment.

These virtual calls, event 19, the selected event callback and global notifications all precede the Game reloads at 0x94958 and 0x94983. The chosen GUI target is retained across parts of this work while later fields are reread. There is no source-proved snapshot of the tree, no dispatch-consumed return suppressing the later Game owner, and no lifetime guarantee for arbitrary reentrant changes.

## What each direct node query actually asks

The complete 0x333E70 receives a node in RCX. It loads that node's manager pointer from offset 0x8 once, copies its current binary32 cached X from U+0x180 and then Y from U+0x184 to a local pair, and calls 0x333F60 with the original node, that pair and mask zero. It converts a nonnull eight-byte node return into AL equal to one, otherwise zero. The native register/stack sequence establishes the original node argument; the malformed decoded first-argument rendering is not a different call contract.

Each invocation copies its own point. A's query finishes before B's node pointer and B's point are obtained. Even in the ordinary constructed graph where their manager is the same U, the point is not copied once for both. There is no lParam point supplied by 0x94900.

The complete recursive query 0x333F60 immediately returns null when the queried node's selected visibility integer is zero, node flag bit 2 is set, or node flag bit 12 is set. These are gates on that node. It does not first walk ancestor visibility. Ancestors enter its transform calculation, not an inherited-visibility admission test. Querying A directly therefore differs from querying a whole root tree containing an invisible P.

A preliminary bounds route is conditional. It requires a nonnull own control at node+0x70, a nonnull control field at 0x58, manager flags at 0x288 with bit 1 set, and control flags at 0x150 with bit 8 clear. It first calls 0x32CF50 on that own control to obtain a two-binary32 origin. It then reloads the node's current own control and calls 0x32D010 to obtain a two-binary32 extent. It compares the original cached point against origin X, origin Y, origin X plus extent X, and origin Y plus extent Y, in that order. For ordinary finite values, a point strictly below either origin or strictly above either sum returns null. Equality at an edge survives this preliminary test. This route does not itself prove that the control or shape admits the point.

If that route is not taken, or all four comparisons survive, the helper copies the node transform's current inverse cache, 16 binary32 words at transform+0x2E0 through 0x31C. A null transform supplies the original identity matrix. A nonnull parent causes 0x100EE0 to obtain the ancestor inverse composition and 0x173E0 to combine it with the node inverse. The helper projects the original point using separate binary32 products, additions, a reciprocal homogeneous divisor and final products. It retains both an unmodified transformed pair and a pair that may be replaced by the following provider.

When the earlier nonnull test of node+0x78 succeeds and the current nonnull shape's virtual slot 0x28 returns nonzero, the helper reloads node+0x78, adjusts that receiver by 0x38, and calls its subobject virtual slot 0x8. The arguments are a local output-pair address, the transformed pair and the queried node. Both binary32 words returned through the provider's result pointer replace the pair used for the shape point test. This 0x38 receiver adjustment is present in the native instructions; it must not be collapsed into a call on the unadjusted control.

The helper next reloads the current shape and, if nonnull, calls shape virtual slot 0x20 with the possibly replaced pair. A nonzero return establishes the local point-admission byte. On a miss or absent shape, it reloads the node's own control and calls own-control virtual slot 0x28 with the separate, original transformed pair. A nonzero return admits the point. After a zero return it rereads the own control's flags at 0x150; bit 8 also admits the point. An absent own control with no shape admission leaves the byte zero. The effective control used to alter the shape-test point and the own control used for this alternative point test are distinct operands.

The helper then reads the current selected clipping integer. A zero clipping value permits descendant traversal even when this node's point-admission byte is zero. A nonzero clipping value with point admission zero returns null before descendants. Descendants are queried in reverse child-list order, with the original point pointer and the same query mask. The first nonnull descendant is returned immediately. The traversal is not a copied child list.

Only after all descendant queries miss does it consider returning this node itself. With the selected mask zero, node flag bit 1 prevents self-return, but did not by itself reject the entire subtree. A zero point-admission byte also returns null. Otherwise the node is returned. The mask-dependent bit-0 and bit-3 exclusions in the generic helper are not active for these mask-zero callers.

This is a subtree point query returning any admitted descendant or the node. It is not a test that a particular named control is selected, focused, captured or enabled, and it is not always a rectangular test. Null own controls and null shapes have the specific alternatives above. The wrapper itself does not guard a null input node before reading node+0x8.

## Native preparation of the preliminary bounds

The complete 0x32CF50 obtains the control matrix through 0x32CE40 and projects local zero, including a reciprocal homogeneous component, to produce the origin pair. The complete 0x32D010 instead uses binary32 control fields at 0x70, 0x74, 0x80, 0x84 and 0x90 through 0x9C before obtaining that matrix.

For its horizontal intermediate, 0x32D010 adds field 0x98 to field 0x70 and subtracts field 0x80. It separately adds field 0x90 to field 0x70 and subtracts field 0x80. It replaces the first intermediate with zero only when the native comparison establishes it is negative, then replaces the second with the first when the first is greater. The vertical route does the corresponding additions from 0x9C and 0x94 with field 0x74, subtracting field 0x84; it clamps the 0x9C-derived value and selects against the 0x94-derived value. It then adds field 0x80 to the selected horizontal value and subtracts field 0x70, and adds field 0x84 to the selected vertical value and subtracts field 0x74.

With the returned matrix, it multiplies the horizontal span by matrix word 1 and the vertical span by word 5, then adds those products for extent Y. It separately multiplies the vertical span by word 4 and the horizontal span by word 0, then adds those products for extent X. It stores Y first, then X. These are binary32 operations, not a call that returns a fixed original button width and height.

The complete 0x32CE40 begins with identity, obtains the composition from 0x32CD10, combines it using 0x173E0 and folds control fields 0x88 and 0x8C into the translation/homogeneous column with separate products and additions. The complete 0x32CD10 starts from the forward cache of the control's current root node at control+0x178, combines the 16-word matrix at control+0x110, and walks parent nodes' forward caches at transform+0x2A0 through 0x2DC. None of those parent traversals supplies an ancestor-visibility test.

The four preliminary reject branches in 0x333F60 use COMISS followed by JA. Subject to the floating environment allowing execution to continue, unordered flags do not take those JA rejects. In 0x32D010 the clamp/select COMISS and JBE pairs likewise do not perform the replacement on an unordered comparison. This is a bounded instruction fact, not a claim that NaN points are ultimately accepted: later reciprocal arithmetic and virtual point providers remain involved, and floating exceptions, masking, denormal handling, infinities and provider treatment have not been established.

## Original setup identity and concrete ownership

Setup 0xF9AC0 creates I+0x108 as an empty-named node below current GUI root U+0x140 through 0x33CD30. Separately it constructs a detached template tree, requests literal gui.plx and then quest-tag.plx through 0x3429A0 using the same tree and option 0x20, and later searches that tree with 0x332190 for literal leftbutton and then rightbutton. It detaches each returned node using 0x3347F0 with null parent.

The complete name lookup compares the current node's UTF-16 name, then walks children in forward depth-first order and returns the first match. It does not establish name uniqueness or direct-child membership. The chosen literals are independently checked original image data, not names guessed from G offsets.

The selected detachment helper removes the node from an existing parent's list through 0x334330 and supports a new-parent append through 0x32F640 when a replacement parent is nonnull. In the selected two calls, replacement parent is null. The complete removal helper searches by node pointer, unlinks matching list cells, adjusts the eight-byte count, frees those list cells and clears the detached node's parent pointer. It does not free the detached node as part of those selected removals. The append helper inserts at the parent's list end, adjusts the count, and publishes the node's eight-byte parent pointer.

Next, at 0xFDD50 setup calls 0x1008A0 with I, text literal land, node-name literal recentlands, parent I+0x108, and the additional 32-bit inputs zero, 16 and 3. It stores the returned node in I+0x288. This returns a node, not a control object whose root must later be obtained at 0x178.

The complete helper creates two TextShape objects through 0x33DA70 and 0x367590. One belongs to the returned recentlands node under I+0x108, and the second belongs to its child, also named recentlands. Both receive the UTF-16 text land, resource literal resource1.dat, selected four-channel color words equal to binary32 one, the zero option, and a virtual slot-0x8 call with byte one. The first receives binary32 16 at shape+0x2B8, binary32 3 at shape+0x2BC and binary32 3 at shape+0x2C4; the second copies the first's words at 0x2B8 and 0x2C4. These are literal native shape/configuration operands, not a proof of the button's asset geometry or font pixel metrics.

Setup enumerates reached shapes using 0xF8590 and an original RTTI cast from plasma Shape to plasma TextShape. It writes 32-bit value 21 at each selected TextShape+0x2F8 and binary32 180, word 0x43340000, at +0x2E8. The selected loop contains no subsequent explicit rebuild call before it starts the two button clones. The RTTI type descriptors at RVAs 0x54A7B8 and 0x54A7E0 independently identify the original Shape and TextShape names.

It clones the detached leftbutton node below P through 0x3308E0 at 0xFDDDB and stores the new node in I+0x290. It then clones the detached rightbutton below P at 0xFDE5B and stores that node in I+0x298. This proves the A/B node ownership joins and the original template names. It does not establish the loaded control's concrete derived class, shape dimensions, glyph data, artwork or the exact contents of either external file.

The complete clone helper clones a nonnull shape via virtual table offset 0x68, a nonnull transform and display object via their offsets 0x10, and creates a node through 0x33CD30. A source own control is attached through its virtual offset 0x140; the new node's effective control is then set from its new own control. Without a source own control, an effective control can be inherited from the new parent. A present shape with an effective control receives virtual slot 0x8 with byte one. Eligible children, with node bit 2 clear, are cloned in forward list order. The helper also copies the node's flags and its two trailing native collections. Those virtual clone/attachment providers are not replaced by assumed types in this contract.

The named TextShape parent and its child are created by native text construction. A and B are clones of loaded template nodes. These are separate provenance classes. The literal leftbutton/rightbutton lookup does not prove that every own or descendant control has that same name.

## Callback binding, cursor movement and visibility writers

After positioning A, then B, setup calls 0xF8090 on A with the name leftbutton, signed 32-bit event key 2, captured receiver I, target 0x101860 and byte one. It then registers rightbutton on B in the same manner with target 0x101790. The exact setup calls are 0xFDF06 and 0xFDF54.

The complete 0xF8090 compares the UTF-16 name at each node's own control, offset 0x18, and recursively visits all children in forward order. This control-name search is separate from the earlier node-name lookup. A matching own control is passed to 0xF8200. No visibility filter limits this registration traversal, and it may bind more than one matching control.

The complete 0xF8200 selects or inserts a signed 32-bit event-map entry, replaces its callback through 0xF8170, and, when its recursive input is nonzero as selected here, obtains reachable child controls through 0x32AC80/0x32ABA0 and registers the same event recursively. That collection walk stops descending through a non-root own control once it appends that control and excludes node bit 2. The callback object is 0x20 bytes. Replacement frees the previous object, allocation failure stores null, byte 0x8 holds its admission flag, and eight-byte fields 0x10 and 0x18 hold I and the target. Its first vtable target, independently read at RVA 0x441BB8, is the complete 11-byte native thunk at 0xF99A0. That thunk obtains the captured receiver at 0x10 and transfers directly to the stored target at 0x18. No owning reference, lifetime extension or automatic unregister rule is proved by that raw capture.

Let E be the current pointer at the receiver through G+0x8, offset 0x448; let D be E+0x1528's eight-byte pointer. The eight-byte pointer D+0x118 is a linked-list sentinel H. List cells have next at 0, previous at 0x8 and two signed 32-bit coordinate fields at 0x10 and 0x14. I+0x5C8 holds a current cell C. This is a different state from a node's selected visibility integer.

The A event target 0x101860 advances C to its current next cell when that next cell differs from H. It then calls 0x1081B0, calls 0x2B9210 with current G+0x8, a local output-pair address and the then-current C coordinate words, calls 0xA3390 on G with the returned pair, and finally stores signed 32-bit value 4 at G+0x1E04 only when its current value is below 4. The B target 0x101790 instead moves C to its previous cell when C differs from H's first cell, then performs the same ordered helper work and minimum-4 store. Both targets still perform the refresh, coordinate helper and later work when their movement predicate fails. Invalid, sentinel-only or concurrently retired cells are not made safe by an inferred no-op rule.

The complete 0x1081B0 writes B's current selected visibility integer first: one when C differs from the first cell at H, otherwise zero. It then writes A's current selected visibility integer: one when C's next cell differs from H, otherwise zero. The two stores are four bytes each. Thus, for an ordinary stable nonempty list, B is unavailable at the first cell and A is unavailable at the last cell. These are explicit visibility writes, not just visual color changes.

Only after those two visibility stores does 0x1081B0 test C against H. For a nonsentinel cell it reads the coordinate pair, performs native unsigned integer hashing and calls 0x611D0, 0x2B9080, 0x16B210 and 0x16BE20 to construct the text subsequently passed to 0x334C50 on P. The string literal named participates in that construction. This audit does not close those text/localization/world-query provider results. It does establish that the two button visibility writes have already happened before that provider work. The complete 0x334C50 changes selected UTF-16 string storage in reached type-3 shapes, when the text differs, and recurses forward through eligible children. It has no explicit shape rebuild call of its own.

The complete reached 0xA3390 calls 0x35EA0 with current G+0x8, the two signed 32-bit coordinates and a zero fourth input. It multiplies the binary32 return by 65,536, word 0x47800000, and converts it to the 64-bit G+0x1E18 field. It sign-extends each coordinate and shifts it left by 16 into the 64-bit G+0x1E08 and +0x1E10 fields, then stores byte one at G+0x1E38. This is a synchronous callback consequence before control returns to the Game point queries. The numerical closure of 0x2B9210 and 0x35EA0 is outside this contract.

The selected later interface update block at 0x106745 first tests I+0x550. When nonzero and P's selected visibility is currently zero, it sets C to H's first cell and calls 0x1081B0. It then reloads I+0x550 and publishes its nonzero test to P's selected visibility through 0xA3A20. Only afterward does it reread P visibility and, when nonzero, query A's own control through 0x32B0C0, write a selected four-channel fill through 0xF88F0, then do the corresponding B query and fill write. Those fill writes are not writes to A/B visibility. The full selected 0x32AE90 route behind 0x32B0C0 itself performs current-point, transform, shape/control and subtree work; its return is not assumed to be a cached byte.

In that update block, a separate map-coordinate publication requires current nonzero I+0x550, cached X strictly below binary32 scale times 300, cached Y strictly above signed screen-height conversion minus scale times 90, and C different from H. It writes C's coordinate words into the receiver through G+0x10 at 0x120/0x124. The alternative writes 0x7FFFFFFF to both words. Text color changes and an empty-string publication are ordered around those branches. These 300-by-90 threshold operands do not replace the A/B subtree point queries or prove their geometry.

Constructor 0xF8690 stores zero to the two-byte span beginning I+0x550, sets binary32 scale I+0x54C to one and C to null. Later constructor instructions at 0x7F4D9/0x7F4DF copy the current 32-bit word G+0x1BCC into G+0x664; its numerical producer is not closed by this audit. The complete callback 0x1031E0 tests current byte I+0x550 and stores one for zero or zero for nonzero. The selected setup path identifies it as the event-2 callback associated with the native worldmap-menu label in the six-node menu binding loop. Its callback does not itself call 0x1081B0, change P/A/B visibility, copy a point or recompute geometry. Ordinary later interface update supplies the selected publication above. The Game owner also contains the direct G+0x668 clear at 0x94DA5.

This is a bounded writer set. Other native instructions using displacement 0x668 and other interface/caller paths were discoverable, but their full admissions and receiver provenance were not closed here. Generic property loading, cloned state, virtual callbacks, alternate owners and indirect writes are not excluded by this contract. It does not assert that G+0x668 is globally written only by the three selected paths or that P/A/B visibility has a universal writer census.

## Literal placement and the geometry limit

For A's transform, setup reads the selected pivot pair using signed index transform+0x244 and the eight-byte elements beginning transform+0x278. It subtracts that pair component by component from the binary32 target pair negative 100 and positive 2. It stores the resulting two words in the currently selected translation pair, indexed by signed 32-bit transform+0xAC into eight-byte elements at transform+0xE0, and calls transform virtual slot 0x8 with byte one. It completes A before doing the same for B with target pair positive 100 and positive 2.

Original transform construction 0x372BD0 names these properties translation and pivot, and separately names deformation and rotation. The pair operands are therefore supported property identities. They are not dimensions. The setup does not assign a literal width and height to either cloned button.

The complete ordinary placement owner 0x101BD0 takes two 32-bit screen-size inputs. Its root preparation replaces the current selected 16-binary32 deformation of U+0x140's transform with identity, applies current scale I+0x54C to both input axes through 0x1028B0 and invokes transform virtual slot 0x8 with byte one. The scale helper skips an axis when its input equals binary32 one; otherwise it multiplies each word of the corresponding four-word matrix group by that scale. It does not inspect A/B visibility.

At call 0x10211F, placement passes P to 0x334700 with an anchor of binary32 zero X and the signed 32-bit screen-height converted to binary32 as Y, and local offsets positive 150 and negative 50. The native XMM/stack arguments establish these values even though the decoded call presentation omits them. This particular placement does not use screen width as its X anchor.

The complete 0x334700 packs the anchor and local offset pairs and calls 0x334510 on the original node receiver. That helper inverse-projects the anchor through the parent transform and remaining ancestors, using identity where the selected transform is null. It then adds the local offset and subtracts the node's currently selected pivot, component by component in binary32, to form the translation pair. It compares the current selected translation words in X-then-Y order. On the first unequal or unordered comparison it writes both words and calls the current node transform's virtual slot 0x8 with byte one; ordered equality for both words produces no store or callback.

The complete Game adapter 0xA2720 stores its two 32-bit inputs into G+0x1D60 and +0x1D64 and forwards them with I to 0x101BD0. Direct call metadata also identifies a placement call in 0x96E70. Those are bounded caller observations; their complete outer control paths are not restated here.

As a conditional arithmetic example, take screen height 720, scale one, compatible identity ancestor transforms and zero selected pivots, and suppose the transform virtual providers expose the resulting ordinary translation matrices. P's translation is then 150 horizontally and 670 vertically. A's translation relative to P is negative 100 and 2; B's is 100 and 2. Their pivot locations under those conditions are 50/672 and 250/672. These values identify conditional native anchor arithmetic only. They establish neither a click rectangle nor the actual loaded artwork's bounds, margins, shape predicates, resource contents or font behavior.

## Source-only conditional event examples

These examples choose valid object graphs, stable list cells and provider returns independently. They are conditional consequences of the selected native code, not original runtime captures or observed asset geometry.

1. After all GUI work, suppose G+0x668 is zero. Neither direct A nor B query occurs. If subsequent ordinary helpers return, the common tail reaches this invocation's byte clear at current G+0x398, offset 0x1D8.

2. Suppose that byte is nonzero and A's subtree query returns a nonnull descendant. The owner takes the first early branch. It never loads B for this guard and skips its later chat-byte and G+0x310 visibility clears. Which descendant was returned is not used by the Game branch.

3. Suppose A returns null and B returns nonnull. There are two sequential point copies and queries, followed by the second early branch. If both return null, the owner enters its normal continuation. A successful earlier GUI callback does not change this branch table into a consumed-event rule.

4. Suppose a provider reached during A's query returns a miss and also changes G+0x668 to zero while leaving the graph valid. The owner still loads and queries B, because the outer byte was tested only once. The point for B is freshly obtained. This example does not assert that any particular untraced provider performs that side effect.

5. For a stable three-cell list, let C initially be the middle cell and both A/B visibility integers be one. Suppose the GUI code-0 target dispatches A's registered event 2. Its target advances C to the last cell and 0x1081B0 writes B visibility one, then A visibility zero, before the callback's remaining work returns. In the later Game guard, A's direct query now returns null at its visibility gate. Choose a B query miss for the cached point. The normal Game continuation and its later clear occur even though the A GUI callback already ran. This is possible at the level of the stated conditional provider returns; no actual button rectangle is inferred.

6. Conversely, let the B callback move from the middle to the first cell. Refresh writes B visibility zero and A visibility one. Choose A's later point query to miss. B's later query returns null on visibility. Again the later normal clear is reached, subject to the remaining helpers returning.

7. Let P visibility be zero, A visibility nonzero and A's own subtree satisfy the other query gates and providers. A's direct query can still return nonnull, because 0x333F60 does not impose ancestor visibility on this direct call. This is a conditional distinction from root-tree GUI picking, not a claim about the ordinary loaded state.

8. For the preliminary bounds route only, choose finite origin 40/660 and extent 20/20. A point 40/660 survives all four strict outside comparisons, while 39/660 fails the first. A point 60/680 also survives those preliminary edge comparisons. Later shape/control and descendant returns still decide the query's result. These numbers are independent provider/matrix conditions and are not supplied as original button geometry.

Repeated calls rerun the GUI-first work and then read the current byte, nodes, selected integers, matrices and cached coordinates. No wall-clock interval, atomic pair sampling, single callback per burst, frame boundary or message-pump serialization guarantee follows from the static call order.

## Selected caller and provider boundaries

The complete window procedure 0x133C70 includes ordinary message values 0x201, 0x204 and 0x207 that call imported SetCapture first and then 0x94900 with codes 0, 1 and 2 respectively, returning integer one. The procedure does not pass lParam coordinates to that owner. Its release messages call ReleaseCapture and 0x965E0 separately. These imported call boundaries do not establish OS capture success, message ordering or absence of reentrancy.

The original application publishes the constructor's returned Game pointer to global RVA 0x551A80 at 0x13633E; the complete window procedure uses that global. Direct call metadata additionally identifies 0x94900 sites 0x1372D6, 0x13738D, 0x1374AE, 0x137511 and 0x137576 in application owner 0x1355A0. Selected instructions establish codes 0, 1, 0, 1 and 2 at those sites. Their full input-source, timer, device and outer control admission is not closed here. This is therefore not an assertion that ordinary presses originate exclusively in the window procedure, nor a universal caller census covering computed calls.

The selected owners contain no acquired lock establishing an atomic snapshot over the GUI-first dispatch and later A/B queries. A caller could provide protection outside these ranges; no absence-of-locks claim is made for every enclosing call. Reentrant virtual calls, event callbacks, external asset/resource/font providers, imported allocation/string/runtime behavior, message dispatch, another thread and object retirement remain bounded dependencies. The selected raw captures and live pointer reloads do not establish safety after arbitrary mutation.

## Evidence and byte correspondence manifest

Original filenames were discovered before contents were read. The selected original export metadata files were functions.csv, function_ranges.csv, calls.csv and assembly.txt. Native C presentations were selected individually by their image entry filenames under decompiled/functions; they were used as navigation aids together with complete selected instructions, not as authority to repair missing arguments by assumption. Selected larger setup, update and application owners were read only for the bounded slices listed below, not claimed to be universally closed.

The native listing was matched instruction by instruction against raw bytes from the identified PE image, mapping RVAs through its section table. For the 65 complete owners below, all 6,831 instructions and 29,339 instruction bytes matched, every listed native range was covered continuously within its own bounds, and there were zero byte mismatches or coverage gaps. Multiple ranges in a row are the original exported owner ranges; bytes between them are not silently claimed as decoded instructions. Complete-owner coverage supports this contract's selected behavior and return census, not the uninspected transitive provider bodies.

| Complete owner RVA | Exported inclusive native ranges | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| 0x0173E0 | 0x0173E0 through 0x017727 | 200 | 840 |
| 0x0485D0 | 0x0485D0 through 0x0486A0 | 62 | 209 |
| 0x094900 | 0x094900 through 0x0965DF | 1583 | 7392 |
| 0x09AF00 | 0x09AF00 through 0x09AF19 | 6 | 26 |
| 0x0A2720 | 0x0A2720 through 0x0A2738 | 4 | 25 |
| 0x0A3390 | 0x0A3390 through 0x0A340C | 33 | 125 |
| 0x0A3A20 | 0x0A3A20 through 0x0A3A3F | 8 | 32 |
| 0x0F8090 | 0x0F8090 through 0x0F8164 | 59 | 213 |
| 0x0F8170 | 0x0F8170 through 0x0F81F1 | 35 | 130 |
| 0x0F8200 | 0x0F8200 through 0x0F837D | 99 | 382 |
| 0x0F8590 | 0x0F8590 through 0x0F8656 | 52 | 199 |
| 0x0F8660 | 0x0F8660 through 0x0F8687 | 14 | 40 |
| 0x0F8690 | 0x0F8690 through 0x0F87C9 | 67 | 314 |
| 0x0F88F0 | 0x0F88F0 through 0x0F8924 | 15 | 53 |
| 0x0F8D20 | 0x0F8D20 through 0x0F8D2C | 4 | 13 |
| 0x0F8D30 | 0x0F8D30 through 0x0F8D50 | 8 | 33 |
| 0x1008A0 | 0x1008A0 through 0x100C08 | 195 | 873 |
| 0x100EE0 | 0x100EE0 through 0x100F95 | 43 | 182 |
| 0x101020 | 0x101020 through 0x10111E | 58 | 255 |
| 0x1014F0 | 0x1014F0 through 0x10178B | 119 | 668 |
| 0x101790 | 0x101790 through 0x101853 | 40 | 196 |
| 0x101860 | 0x101860 through 0x10191B | 39 | 188 |
| 0x101BD0 | 0x101BD0 through 0x1028A8 | 665 | 3289 |
| 0x1028B0 | 0x1028B0 through 0x102926 | 30 | 119 |
| 0x1031E0 | 0x1031E0 through 0x1031F0 | 4 | 17 |
| 0x1081B0 | 0x1081B0 through 0x108448 | 152 | 665 |
| 0x133C70 | 0x133C70 through 0x133EEB | 174 | 636 |
| 0x32A210 | 0x32A210 through 0x32A2B4 | 46 | 165 |
| 0x32ABA0 | 0x32ABA0 through 0x32AC7C | 58 | 221 |
| 0x32AC80 | 0x32AC80 through 0x32ACE2 | 26 | 99 |
| 0x32AE90 | 0x32AE90 through 0x32B0B8 | 138 | 553 |
| 0x32B0C0 | 0x32B0C0 through 0x32B0CB | 2 | 12 |
| 0x32CD10 | 0x32CD10 through 0x32CE3A | 65 | 299 |
| 0x32CE40 | 0x32CE40 through 0x32CF47 | 60 | 264 |
| 0x32CF50 | 0x32CF50 through 0x32D000 | 42 | 177 |
| 0x32D010 | 0x32D010 through 0x32D11C | 61 | 269 |
| 0x32EE60 | 0x32EE60 through 0x32F051 | 114 | 498 |
| 0x32F640 | 0x32F640 through 0x32F6D0 | 37 | 145 |
| 0x3308C0 | 0x3308C0 through 0x3308DB | 13 | 28 |
| 0x3308E0 | 0x3308E0 through 0x330A77 | 115 | 408 |
| 0x332190 | 0x332190 through 0x33221B | 44 | 140 |
| 0x333E70 | 0x333E70 through 0x333EC7 | 19 | 88 |
| 0x333F60 | 0x333F60 through 0x33429A | 215 | 827 |
| 0x334330 | 0x334330 through 0x334409 | 64 | 218 |
| 0x334510 | 0x334510 through 0x3346FD | 113 | 494 |
| 0x334700 | 0x334700 through 0x334751 | 17 | 82 |
| 0x3347F0 | 0x3347F0 through 0x334841 | 25 | 82 |
| 0x334B80 | 0x334B80 through 0x334C46 | 57 | 199 |
| 0x334C50 | 0x334C50 through 0x334D24 | 58 | 213 |
| 0x33AA88 | 0x33AA88 through 0x33AA90 | 2 | 9 |
| 0x33AAA0 | 0x33AAA0 through 0x33AAA8 | 2 | 9 |
| 0x33AAD0 | 0x33AAD0 through 0x33AAD8 | 2 | 9 |
| 0x33C360 | 0x33C360 through 0x33C406 | 49 | 167 |
| 0x33CD30 | 0x33CD30 through 0x33CED2 | 117 | 419 |
| 0x33DA70 | 0x33DA70 through 0x33DACB | 26 | 92 |
| 0x33F1E0 | 0x33F1E0 through 0x33F202 | 8 | 35 |
| 0x340EC0 | 0x340EC0 through 0x3410D8 | 138 | 537 |
| 0x341300 | 0x341300 through 0x341494 | 103 | 405 |
| 0x3415F0 | 0x3415F0 through 0x341776 | 102 | 391 |
| 0x342270 | 0x342270 through 0x342556 | 161 | 743 |
| 0x342560 | 0x342560 through 0x3425BF | 29 | 96 |
| 0x34A420 | 0x34A420 through 0x34A4C4 | 46 | 165 |
| 0x367590 | 0x367590 through 0x367667; 0x367669 through 0x367677; 0x367679 through 0x367687; 0x367689 through 0x367694; 0x367696 through 0x367743; 0x367745 through 0x367753; 0x367755 through 0x367763; 0x367765 through 0x367770; 0x367772 through 0x367818; 0x36781A through 0x367828; 0x36782A through 0x367838; 0x36783A through 0x367845; 0x367847 through 0x3678ED; 0x3678EF through 0x3678FD; 0x3678FF through 0x36790D; 0x36790F through 0x36791A; 0x36791C through 0x3679BE | 264 | 1055 |
| 0x372BD0 | 0x372BD0 through 0x372CAA; 0x372CAC through 0x372CBA; 0x372CBC through 0x372CCA; 0x372CCC through 0x372CD7; 0x372CD9 through 0x372D7B; 0x372D7D through 0x372D8B; 0x372D8D through 0x372D9B; 0x372D9D through 0x372DA8; 0x372DAA through 0x372E5B; 0x372E5D through 0x372E6B; 0x372E6D through 0x372E7B; 0x372E7D through 0x372E88; 0x372E8A through 0x372F34; 0x372F36 through 0x372F44; 0x372F46 through 0x372F54; 0x372F56 through 0x372F61; 0x372F63 through 0x3730FE | 301 | 1311 |
| 0x3736C0 | 0x3736C0 through 0x37379D; 0x37379F through 0x3737AD; 0x3737AF through 0x3737BD; 0x3737BF through 0x3737CA; 0x3737CC through 0x37386A; 0x37386C through 0x37387A; 0x37387C through 0x37388A; 0x37388C through 0x373897; 0x373899 through 0x373948; 0x37394A through 0x373958; 0x37395A through 0x373968; 0x37396A through 0x373975; 0x373977 through 0x373A1D; 0x373A1F through 0x373A2D; 0x373A2F through 0x373A3D; 0x373A3F through 0x373A4A; 0x373A4C through 0x373AF6; 0x373AF8 through 0x373B06; 0x373B08 through 0x373B16; 0x373B18 through 0x373B23; 0x373B25 through 0x373BE8 | 324 | 1301 |

The following are additional bounded instruction slices, not claims of complete larger-owner interpretation. Every included instruction in these slices also matches the original image, without a gap within its reported interval. The two explicitly unowned thunks have their complete transfer bodies verified by bytes and their selected pointer/caller evidence; a neighboring routine has not been substituted for either.

| Scope | Inclusive native slice | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| Game constructor interface alias | 0x7EA80 through 0x7EB2A | 39 | 171 |
| Game setup call and scale publication | 0x7F4C9 through 0x7F4E4 | 5 | 28 |
| Interface setup receiver | 0xF9AC0 through 0xF9B69 | 31 | 170 |
| Interface root construction | 0xF9CAC through 0xF9D01 | 17 | 86 |
| Template tree/load boundary | 0xFB4D1 through 0xFB62B | 70 | 347 |
| Selected template names/detachment | 0xFC63E through 0xFC6BD | 28 | 128 |
| Parent text,clone transforms,bindings | 0xFDCF8 through 0xFDF65 | 121 | 622 |
| Mode callback setup label | 0x10024E through 0x100305 | 34 | 184 |
| Mode callback binding join | 0x1003B3 through 0x1003E8 | 13 | 54 |
| Interface update receiver | 0x103200 through 0x1032A5 | 34 | 166 |
| Selected parent/leaf update | 0x106745 through 0x1069D3 | 127 | 655 |
| App Game publication | 0x136314 through 0x136344 | 13 | 49 |
| Other press caller 1 | 0x1372B6 through 0x1372DA | 8 | 37 |
| Other press caller 2 | 0x137376 through 0x137391 | 6 | 28 |
| Other press caller group | 0x137494 through 0x13757A | 45 | 231 |
| Unowned callback dispatch thunk | 0xF99A0 through 0xF99AA | 3 | 11 |
| Unowned visitor thunk | 0x33AAC4 through 0x33AACC | 2 | 9 |

For each complete-owner row, the individually selected C export filename is the full image VA for that RVA, followed by .c beneath decompiled/functions. The larger selected source files were 14007ea80.c, 1400f9ac0.c, 140103200.c and 1401355a0.c; only their identified construction, field-reference, update and caller slices were needed. The complete window procedure was read as 140133c70.c. The manifest does not elevate a short slice of a large owner into complete ownership of every state writer or caller.

The raw-data checks selected for this contract include the UTF-16 literals leftbutton at RVA 0x441028, rightbutton at 0x441040, recentlands at 0x441280, land at 0x441298, gui.plx at 0x440AA8, quest-tag.plx at 0x440AB8 and worldmap-menu at 0x441790. These were checked from the identified image. The resource1.dat literal was also checked in the original image at RVA 0x436340 and against the selected text-construction call setup; no resource file was opened. The callback-vtable pointer at 0x441BB8 selects 0xF99A0. The two original RTTI descriptors at 0x54A7B8/0x54A7E0 identify plasma Shape/TextShape. No template asset or package was used to infer a concrete button-derived control or its shape.

The selected binary32 image constants were independently read as follows:

| Constant RVA or instruction location | Raw 32-bit word | Value or operand |
| --- | --- | --- |
| 0x441BF8 | 0xC2C80000 | negative 100 |
| setup immediate 0xFDE6B | 0x42C80000 | positive 100 |
| setup immediate 0xFDDFB and 0xFDE75 | 0x40000000 | positive 2 |
| 0x42E938 | 0x43160000 | positive 150 |
| 0x42E878 | 0xC2480000 | negative 50 |
| 0x42E7E0 | 0x43960000 | positive 300 |
| 0x42E7C0 | 0x42B40000 | positive 90 |
| 0x4AC980 | 0x3F800000 | positive 1 |
| 0x42E5F8 | 0x47800000 | positive 65,536 |
| setup immediate 0xFDDBE | 0x43340000 | positive 180 |

The setup inputs 16 and 3 are 32-bit integer call arguments at 0xFDD28/0xFDD20, subsequently converted or stored in the selected TextShape fields described above. They are not additional proved button dimensions. Event keys, property indices, flags, press codes, sentinel values and byte stores are established by the complete selected instructions and matched immediates, not by an asset convention.

## Exclusions, unresolved scope and release boundary

This contract closes the ordinary two-query branch and all normal-return joins of the complete 0x94900 owner. It also closes the selected GUI-first adapter, query recursion, native bounds preparation, A/B construction joins, callback capture/dispatch, list-cursor visibility refresh, selected parent publication and native placement arithmetic to the stated provider boundaries.

It does not close the contents of gui.plx, quest-tag.plx, resource1.dat, imported fonts or other assets; the concrete loaded A/B control-derived classes and all virtual clone, rebuild, point and notification targets; the numerical closures of World/coordinate/name/localization queries; the entire external input-source loop; every writer of cached coordinates, control flags, selected property indices, list contents or G+0x668; every alternative caller; or object destruction and arbitrary callback reentrancy.

The signed selected property indices are used as they are read. These selected readers do not prove general clamping or validation for a negative or out-of-range index. Null or stale G, U, node/display/control pointers, a null C before first ordinary publication, invalid list topology, malformed assets, allocation failure followed by an unguarded use and concurrently removed nodes/cells are outside the valid-object conditional examples. Constructor initialization does not prove when an outer scheduler first admits a callback. No replacement values are invented for these gaps.

Static instruction order is not a promise of an uninterrupted frame, a data-race-free snapshot, imported provider determinism, message-pump behavior, OS capture success, wall-clock timing or teardown safety. No original runtime execution, owner capture, test result or replacement behavior was used for the conditional examples. The shown direct display writes do not prove the absence of indirect or property-loader writes elsewhere.

Exposure statement: the author read the standalone source-author policy, this isolated assignment, filename-selected original executable/export evidence and metadata only. No active repository AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, combined project history, replacement implementation, exporter/extractor code, tests, results, owner outputs/data/captures, assets, packages, SDK, dependency or reconstruction source was opened. No earlier held prose handoff was read or modified, no other agent was spawned, and no implementation or runtime feedback was requested or consumed.

This is a complete independent code-free contract. Its release is reported only after exclusive creation and whole saved-artifact readback with exact byte count and SHA256. At the reported FINAL / STABLE / HELD boundary, all author writes to this artifact stop. Earlier held artifacts remain outside this audit and immutable.
