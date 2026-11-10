# Original six menu captions and binding text

This independent contract follows the ordinary six menubutton nodes from native construction to their later text arguments and selected TextShape string writes. The ordinary Game update formats five current binding records and supplies a fixed F1 string for the sixth node. Those arguments are passed to a recursive node text helper. The icon-selection identifiers used during setup are separate operands from the later caption arguments.

## Authority, identity and receiver joins

Only the standalone docs/CLEANROOM_AUTHOR.md policy, this isolated assignment and individually selected original executable/export evidence were used. The supported image is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe: AMD64 machine 0x8664, preferred base 0x140000000, timestamp 0x5D93AB0D, entry RVA 0x3939F0, image size 0x58A000 and file length 5,757,952 bytes. Its independently checked SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Addresses below are RVAs unless described as object offsets.

G denotes the ordinary Game receiver. I is its embedded interface at G+0x118; U is the GUI manager pointer stored at G+0x20. Complete Game constructor 0x7EA80 calls 0xF8690 at 0x7EB1E with I and G. That interface constructor stores the eight-byte G pointer at I+0x5C0 and initializes the three eight-byte vector words I+0x128, +0x130 and +0x138 to zero. They serve as begin, end and capacity-end for eight-byte node-pointer elements. These words alias G+0x240, +0x248 and +0x250.

Setup 0xF9AC0 is called on I at 0x7F4D4. It creates the empty-named node I+0x108 below U+0x140. It separately creates a detached node at I+0x328 and requests icons.plx into it through 0x3429A0 with option 0x20. A later detached template tree is supplied first to a gui.plx request and then to a quest-tag.plx request, also with option 0x20. No asset file was opened in this audit.

The ordinary first construction starts with the empty vector described above. Setup finds literal menubutton in the template tree with 0x332190, then performs six iterations numbered zero through five. Each iteration first clones the returned template node below I+0x108 using 0x3308E0 and appends the returned eight-byte node pointer through 0x101A00. Call sites 0xFFF2A and 0xFFF40 establish clone-before-append order. The selected callback/icon work for that iteration follows the append. The loop increments its signed 32-bit counter and repeats while below six.

Call the six pointers N0 through N5 in vector order. They are direct node pointers; they are not widget objects requiring a root-node lookup at offset 0x178. Node creation 0x33CD30 allocates a 0xC8-byte node and attaches it to the supplied nonnull parent through 0x32F640. The latter appends a child-list cell and then publishes the node's parent pointer at offset 0x40. The complete vector append and growth helpers preserve existing pointer-element order on successful allocation and append at the end. No sorting or priority rule selects the six indices.

A later call to setup without the ordinary empty-vector initialization is not proved to replace those first six entries. This contract does not assert idempotent setup, allocation-failure recovery or valid captions for an incomplete vector.

## Setup identifiers, initial text and callback binding

The setup association is fixed by its native branch destinations, not by an inferred visual order:

| Index | Original icon-registry identifier | Event-2 target | Later caption input |
| --- | --- | --- | --- |
| 0 | crafting-menu | 0x102F10 | current pair at G+0x1C6C |
| 1 | inventory-menu | 0x102FE0 | current pair at G+0x1C64 |
| 2 | worldmap-menu | 0x1031E0 | current pair at G+0x1C54 |
| 3 | system-menu | 0x103150 | current pair at G+0x1C74 |
| 4 | multiplayer-menu | 0x1030C0 | current pair at G+0x1C7C |
| 5 | help-menu | 0x102FA0 | literal F1, two UTF-16 code units |

For each iteration, setup obtains its newly appended node by subtracting one eight-byte element from the current vector end through 0xF8D60. It calls 0x102930 on I with that node, literal background and the selected registry identifier. It obtains the last node again and passes that node's own control at offset 0x70 to 0xF8200, with signed 32-bit event key 2, captured receiver I, the selected target and both additional byte arguments equal to one. The common registration call is 0x1003CF; only afterward is the loop counter advanced.

The complete name lookup 0x332190 compares the queried node's UTF-16 name first, then visits children in forward depth-first order and returns the first nonnull match. It has no visibility, enabled-control or clipping predicate. It does not prove uniqueness or direct-child identity for a name.

The complete clone helper 0x3308E0 obtains nonnull shape, transform and display copies through their virtual clone slots, then creates a new node using the source node's name. A present source own control receives a virtual attachment/clone call at table offset 0x140; the new node's effective control is set from its resulting own control. Without a source own control, an effective control can be inherited from the new parent. A present shape with an effective control receives its virtual slot 0x8 with byte one. Eligible children with node flag bit 2 clear are cloned in forward list order. The helper copies node flags and two trailing native collections as well. The virtual providers' exact derived classes and returned component identities are not supplied by the unavailable assets.

Consequently, the selected setup initially carries template-derived shape/control state into each node. It does not explicitly assign the strings crafting-menu, inventory-menu, worldmap-menu, system-menu, multiplayer-menu or help-menu as the six caption strings. It does not explicitly set those captions to empty, to six English menu names, or to their later binding names. Initial text content and the existence and names of caption-bearing TextShapes remain asset/provider dependencies.

The complete 0x102930 first looks for background within the supplied node. A miss returns without further icon work. It then looks for the chosen identifier beneath I+0x328. A miss also returns. On success it searches the background subtree for an existing icon and, when found, calls manager removal 0x33DDA0 before creating the replacement. It obtains the background bounds, creates nodes from the selected registry subtree through 0x333890, makes the new root's transform independent when required through 0x335830, and writes the new root's node name to icon. It then performs pivot/deformation/scale preparation and transform virtual calls using the reached native bounds/matrix providers. Those geometric numerical closures are outside this text contract.

The node-name write at new-node offset 0x18 is distinct from a TextShape's selected string property. Moreover, 0x333890 passes the source shape, transform and display pointers directly to the node creator and recurses through eligible children; it does not use the deep shape-clone call of 0x3308E0. Its selected transform helper can clone a multiply referenced transform and update that node's transform registration. It does not thereby prove that shape/string storage has been made independent. If an icon subtree contains an eligible shared type-3 shape, the later recursive caption helper can reach that same shape. This is a conditional aliasing consequence, not a claim that any particular icon asset contains text.

Manager removal 0x33DDA0 performs its reached removal only when the node's manager pointer matches the supplied manager. Its complete body includes descendant removal, component-reference changes, manager-list removal under that manager's critical section and a deleting virtual call. The invoked deletion/component providers and arbitrary callbacks are not closed here. Neither successful icon removal nor normal loaded geometry is manufactured for missing or invalid assets.

Registration helper 0xF8200 selects or inserts a signed 32-bit event-map entry and replaces its callback using 0xF8170. The callback object is 0x20 bytes: a flag byte at 0x8, an eight-byte captured receiver at 0x10 and an eight-byte target at 0x18. Replacement frees the old object; allocation failure stores null. With the selected recursive byte nonzero, it registers the same event on discovered child controls through 0x32AC80/0x32ABA0. That walk excludes node bit 2 and stops descending through a discovered non-root own control after collecting it.

The first callback-vtable target at image RVA 0x441BB8 is 0xF99A0. Its complete 11-byte transfer body reads the capture at 0x10 and transfers to the target at 0x18. No ownership reference or lifetime extension is proved by this capture. Registration itself is not a caption-string publication.

The five available complete callback bodies at 0x102F10, 0x102FE0, 0x1031E0, 0x103150 and 0x1030C0 contain visibility/state changes and reached helpers, without a direct six-node caption or binding-pair text write in those bodies. Their transitive widget/gameplay helpers are not given a universal no-text guarantee. For 0x102FA0, there is no corresponding decoded C export or catalogued owner extent in this evidence. The selected target begins a straight-line, byte-verified 51-byte sequence through RET at 0x102FD2 that toggles the selected visibility integer of the widget root through I+0x278. That sequence contains no caption call. This bounded instruction fact does not invent a missing exported owner or exclude alternate entries.

## Ordinary Game caption production and exact order

The native owner 0xA5A50 has no usable decoded C body in the selected export: its status is timeout. The following claims come from the original instruction listing matched against the supported image. Its complete entry-to-caption prefix, 0xA5A50 through 0xA643F, is continuous and was inspected in full.

The prefix preserves G in R13 and clears the native pointer list at G+0x6C8 through 0x3C9C0. That helper resets the list header/count and frees list cells; it does not acquire a lock. The prefix then branches on byte G+0x1958 for other named-text/color work through the separate vector G+0x228. The nonzero route jumps at 0xA6006 to 0xA62D1; the zero route reaches that same join after its own work. These preceding named-text calls are not passed the six-node vector at G+0x240. These selected calls use complete helper 0x3348E0 with its final byte argument equal to one. That helper rejects a node/subtree whose bit 2 is set, compares the current node's UTF-16 name, and on a matching name calls 0x334C50 for that entire subtree. On a name miss it visits children forward. Thus these named-descendant publications and the six direct subtree publications are separate joins. Its zero-byte alternative updates only a reached local type-3 shape through a separate comparison/copy and shape virtual call; that alternative is not selected by the preceding calls described here.

From 0xA62D1 there are six ordered caption publications:

| Index | Formatter call | Recursive text call | Argument source |
| --- | --- | --- | --- |
| 0 | 0xA62E3 | 0xA62F6 | pair G+0x1C6C |
| 1 | 0xA631A | 0xA632E | pair G+0x1C64 |
| 2 | 0xA6352 | 0xA6366 | pair G+0x1C54 |
| 3 | 0xA638A | 0xA639E | pair G+0x1C74 |
| 4 | 0xA63C2 | 0xA63D6 | pair G+0x1C7C |
| 5 | none | 0xA642E | local UTF-16 F1 string made at 0xA6416 |

Each of the first five calls supplies receiver G+0x30 to 0x4CA10, a local output-string address and a pointer to the current two-word record in the table. After the formatter returns, the Game owner reloads the vector begin pointer from G+0x240, selects the relevant eight-byte node pointer at offsets zero, 0x8, 0x10, 0x18 or 0x20, and calls 0x334C50 with that node and the returned string. It destroys that temporary string before beginning the next record's formatting. These record inputs and node pointers are not copied as a six-entry snapshot.

For index 5, it initializes a separate local wide string, copies exactly two code units from literal F1 at image RVA 0x430F44, reloads the vector begin and selects offset 0x28, calls 0x334C50, then destroys the local string. It does not look up a Help binding record for this selected publication. Rebinding the first five records is therefore a different source path from this sixth literal.

There is no node-count check against G+0x248, no null-node fallback and no selected visibility test in this six-call block. Its ordinary construction precondition is six valid nodes. It is not an empty-vector skip loop. Byte G+0x420, the visibility of I+0x108 or the individual nodes, focused/captured control state, player name and current entity fields are not tested as admissions for these six calls within this complete prefix. Both the zero and nonzero branches of G+0x1958 reach them when preceding work returns normally.

Thus this owner makes the same six calls on a repeated update even when their text already matches. Actual no-write decisions occur inside the recursive text helper. Exceptional allocation/string/provider failure, invalid nodes or a nonreturning earlier helper is not reclassified as an ordinary successful skip.

This caption block precedes the subsequent time sample and World work beginning after 0xA6440, and precedes the selected later interface update call at 0xAB5CE. R15 is established as G+0x118 at 0xAAF6C and is passed as that later interface receiver. No additional wall-clock or scheduler relation follows from these static sites.

## Binding records, configuration and key-name formatting

Each selected binding is two consecutive signed 32-bit operands. Call the first its numeric key and the second its category word, without importing an SDK input-enumeration meaning. The records belong to the 0xB8-byte binding aggregate beginning G+0x1BDC.

Constructor call 0x7ED12 invokes 0x41DD0 on that aggregate. The complete initializer writes zero to 23 eight-byte records and calls 0x43760. The latter writes the native defaults. The five records used here receive the following complete pairs:

| Caption index | Aggregate record index | Default key | Default category | Compact dictionary text |
| --- | --- | ---: | ---: | --- |
| 0 | 0x12 | 0x2E | 0 | C |
| 1 | 0x11 | 0x30 | 0 | B |
| 2 | 0x0F | 0x32 | 0 | M |
| 3 | 0x13 | 0x18 | 0 | O |
| 4 | 0x14 | 0x24 | 0 | J |

These are initialization facts. They are not a claim that a particular installation retains those bindings. At 0x7F267 the constructor calls 0x42B30 on the same aggregate before interface setup. That original reader requests controls.cfg through native stream helpers. Its selected exact narrow-string tokens associate ButtonWorldMap with record 0x0F, ButtonInventory with 0x11, ButtonCraftingMenu with 0x12, ButtonSystemMenu with 0x13 and ButtonMultiplayerMenu with 0x14.

For a recognized record token, the complete reader initializes a local signed 32-bit first value to zero and requests extraction; only if the stream-state check succeeds does it initialize and extract the second value. Only after both checks succeed does it write the two 32-bit record words, first then second. A failed extraction exits without publishing that incomplete pair. Previously completed records remain published. Repeated complete records overwrite the earlier complete pair in input order. No native key/category range clamp is inserted before those stores. Token parsing, integer conversion, stream states, locale, actual file presence and imported I/O behavior remain provider boundaries. This research did not read controls.cfg.

After interface setup, constructor call 0x7F4EF invokes the complete 0x89B80 with G and the aggregate beginning G+0x1BDC. It copies eleven consecutive 16-byte blocks followed by one eight-byte block into that same destination aggregate, then tail-transfers to 0x438E0 with the aggregate receiver. The complete latter body requests controls.cfg for output and supplies the binding tokens and their signed integer pairs to imported stream operations. This is a persistence route, not a normalization or a six-node caption write. It provides no immediate native caption refresh call.

Another direct caller, 0x276C80, passes its pointer at offset 0x1E8 and aggregate at offset 0x1F0 to 0x89B80, then clears its own widget root's selected visibility. The concrete provenance of that first pointer and the complete editor/input lifecycle are not established here. This identifies an alternate update route without treating every receiver of that offset as G or constructing a universal binding-writer census.

Game constructor call 0x7EB04 constructs the key-name object at G+0x30 using 0x49080. That owner constructs two signed-integer keyed dictionaries, first at its receiver and then at receiver+0x10. The latter is the dictionary used by 0x4CA10. Original literal insertions in the latter provide compact names, including key 1 as ESC, 0x18 as O, 0x24 as J, 0x2E as C, 0x30 as B, 0x32 as M and 0x3B as F1. The formatter reads this dictionary through G+0x40. A different entry's current string remains a data input; this contract does not enumerate every key-name string or exclude later dictionary writers.

Complete 0x4CA10 has these return branches and read order:

- It reads the signed 32-bit first word initially. If that word is zero and the second 32-bit word is also zero, it initializes the output and supplies the single UTF-16 question-mark code unit from RVA 0x431820.
- Otherwise, if the second word equals one, it constructs its temporary wide stream. After those constructor/provider calls it rereads the first word at 0x4CB17. It inserts literal M from RVA 0x430ECC through 0x486B0, adds one to that reread key in a 32-bit destination at 0x4CB2D, inserts the resulting signed 32-bit value through the imported integer-stream operation, and obtains the current stream string through 0x415A0. That native addition wraps at 32 bits before the provider receives the integer; no inferred C++ overflow rule replaces its operand.
- Every other second-word value uses the initially read key for a signed 32-bit tree lookup in the dictionary at receiver+0x10. It compares tree keys at node+0x20. An exact match copies the current wide-string object at node+0x28 into the initialized output. A miss supplies the same single question-mark string.

The last branch does not first require category zero. Unknown non-one categories are not automatically rejected or routed to the one-category formatting branch. The two zero words have their explicit earlier branch. No operating-system key-name query occurs in this formatter's selected body.

The output is a 0x20-byte native wide-string object: eight-byte length at offset 0x10, eight-byte capacity at 0x18, inline UTF-16 storage for capacity below eight and otherwise an eight-byte data pointer at offset zero. Dictionary matching does not modify the binding record. Its returned string is copied before the Game owner destroys the temporary.

The complete prefix helper 0x486B0 scans the supplied UTF-16 prefix to its zero terminator, observes stream state/width/fill and adjustment flags, can invoke stream-buffer virtual slots 0x8 and 0x10 around its provider work, performs padding and bulk writes through the reached stream providers, resets width on the reached good-state route, and calls setstate and the ordinary stream suffix providers. The complete 0x415A0 forwards the string-buffer subobject to 0x414A0. That helper chooses a current buffer interval or an empty string from its mode/pointer gates, copies its two-byte units and transfers the resulting wide string through 0x3BF00. No font or GUI geometry is involved in this formatting chain.

Consequently, M followed by an integer representation is a native composition order, while the imported integer formatting, buffer/provider failures and actual resulting stream contents are explicitly not replaced with a guessed numerical provider contract. The record is not atomically sampled over those provider calls.

## Actual string destinations and no-write cases

Complete recursive helper 0x334C50 takes a node and a wide-string object. Bit 2 of the node's 32-bit flags at offset 0x80 is its first gate. When set, the entire node/subtree call returns without processing that node or visiting its children.

With bit 2 clear it reads the node's shape pointer at 0x58. A nonnull shape is eligible for a local text update only when its 32-bit type word at shape+0x70 equals 3. Constructor 0x367590 establishes this type and names the property at shape+0x98 string. The active string record is found with signed 32-bit index shape+0xD4 into the 0x20-byte records beginning at shape+0x108.

It compares that selected string against the supplied string using complete 0x89470. The comparison reads unsigned 16-bit code units for the smaller of the two 64-bit lengths, compares the first unequal code unit, and uses length only when the shared prefix is equal. It does not stop at an embedded zero code unit or perform localization, trimming or case folding.

If the text compares equal, there is no local copy. If it differs, the helper recomputes the active record address from the current index and base pointer. A record address equal to the supplied string-object address also suppresses the copy. Otherwise it calls 0x3D3B0 with source offset zero and an unbounded requested count, which in this selected valid-string route copies the source's entire 64-bit code-unit length, updates destination length and writes a two-byte terminator. Its storage growth and copies are bounded by the inspected original string helpers and imported allocation/memory operations; out-of-range, excessive-length, trap and allocation-failure behavior is not converted into a valid caption.

Only after this node's local decision does 0x334C50 visit children in forward child-list order with the same supplied string. An absent shape or a type other than 3 suppresses only this node's local copy; it does not by itself stop descendant traversal. The selected call walks the whole eligible subtree. It does not search for a child named text, caption, background or icon.

The helper does not test selected visibility, clipping, node flag bit 1 or 12, enabled-control state, focus, capture, G+0x420 or an ancestor's visibility. It does not write node/control names, binding records, the active property index, root-control caption fields or selected display visibility. It updates the selected TextShape string storage that it actually reaches. Multiple eligible type-3 shapes below a menu node receive the same current argument in this traversal order; shared shape storage can make writes visible through other node references. No asset-specific count of such shapes is established.

This selected helper contains no explicit shape virtual slot-0x8 call or draw call after its copy. Native string storage publication, subsequent shape/provider refresh and actual rendering are therefore separate facts. This contract does not assert when cached glyph/layout data changes or promise a rendered label during the same uninterrupted frame.

The ordinary later interface loop at 0x1065CB snapshots its vector begin and end, derives an unsigned 64-bit iteration count by adding seven to their difference and shifting right by three, and substitutes zero when end precedes begin. For the valid aligned six-pointer vector, that count is six. Each iteration reloads byte I+0x308, alias G+0x420, and calls 0xA3A20 on the current vector element. That helper stores a four-byte zero-or-one value to the current selected visibility integer, using signed index display+0xAC and four-byte elements at display+0xE0. Display construction 0x3736C0 names that property visibility. This loop is a visibility publication; it has no caption-formatting or TextShape string copy.

The complete placement owner 0x101BD0 also contains a vector loop over I+0x128/+0x130, passing those nodes to position helper 0x334700. That loop does not directly supply text. General position, transform, asset layout and draw-provider effects are not inferred from these calls.

## Independent conditional sequences

These are source-only conditional consequences, not original runtime captures, installation observations or replacement-engine tests.

1. With the five default binding pairs retained, the compact dictionary unchanged and valid eligible text storage, the six supplied caption arguments are C, B, M, O, J and F1 in that order. If no eligible shape is reached, those arguments still do not manufacture a visible label.

2. Suppose index 1's record becomes key 0x2E with category zero before its formatter is called. That update supplies C to N1. N0's argument was obtained earlier, and the sixth still receives the fixed F1 argument. No copied six-record snapshot makes the other records change together.

3. Give a formatter both words zero. Its output argument is the question mark. Give it category one with reread key zero and choose successful stream-provider behavior that produces the usual signed integer representation 1. Its composed text is M1 under that stated provider condition. Give it a non-one category and a key absent from the current dictionary: it again supplies the question mark. No category label is inferred from those values.

4. Let all six menu nodes currently have selected visibility zero while their node bit 2 is clear and eligible strings differ. The Game prefix still invokes all six text calls and those strings can change. The later visibility loop can keep the nodes hidden when G+0x420 is zero. Hidden state is not a text-preservation admission in the selected recursive helper.

5. Let N2's bit 2 be set. Its entire text subtree is skipped. Let N3 instead have a non-text root shape but an eligible type-3 child with bit 2 clear: the child can still receive index 3's argument. Let N4 already contain the supplied string: the local comparison prevents its local copy but does not stop traversal into children.

6. During setup, a missing background or missing registry identifier causes 0x102930 to return without its icon replacement. No native fallback caption string is supplied by that miss. The later six text publications remain their own route, conditional on a valid constructed node vector.

Repeated updates rerun formatting and recursive comparison. They need not perform a local string write when content is unchanged. No wall-clock period, atomic group publication, fixed number of eligible text shapes or original asset geometry is implied.

## Evidence and correspondence

The original filenames were discovered before scoped content reads. Selected original export metadata were functions.csv, function_ranges.csv, calls.csv and assembly.txt. C presentations were used with the selected original instructions; the absent Game-update C body was not invented, and missing call arguments were resolved from native register/stack inputs.

The following table gives complete exported-owner instruction-byte coverage, not a claim that every unrelated behavior inside a large owner is specified here. All 18,228 instructions and 84,366 bytes in these 53 rows matched the identified original image; every listed range had zero mismatches and zero instruction-coverage gaps within its own bounds. Gaps between separate exported ranges are not silently treated as decoded instructions. Behavioral interpretation of 0xA5A50 is bounded to its complete caption prefix, selected receiver/call sites and the relevant pointer/text references. Interpretation of 0x103200 is bounded to the selected vector visibility/reference path. Setup 0xF9AC0 is bounded separately below. The reached formatter, text-copy, construction and selected helper bodies were inspected to the stated inputs and provider boundaries; this is not a universal closure of the dictionary contents, asset providers or unrelated large-owner work.

| Byte-checked owner RVA | Exported inclusive ranges | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| 0x0334F0 | 0x0334F0 through 0x033573; 0x033588 through 0x033607 | 78 | 260 |
| 0x03BF00 | 0x03BF00 through 0x03BF8C | 41 | 141 |
| 0x03C9C0 | 0x03C9C0 through 0x03CA0A | 22 | 75 |
| 0x03D3B0 | 0x03D3B0 through 0x03D4E1 | 88 | 306 |
| 0x03D4F0 | 0x03D4F0 through 0x03D634 | 100 | 325 |
| 0x0414A0 | 0x0414A0 through 0x041598 | 65 | 249 |
| 0x0415A0 | 0x0415A0 through 0x0415C2 | 10 | 35 |
| 0x041DD0 | 0x041DD0 through 0x041E00 | 16 | 49 |
| 0x042B30 | 0x042B30 through 0x043207; 0x043209 through 0x04320E; 0x043210 through 0x043215; 0x043217 through 0x04321C; 0x04321E through 0x043241; 0x043243 through 0x043251; 0x043253 through 0x043261; 0x043263 through 0x04326E; 0x043270 through 0x0432B8 | 502 | 1921 |
| 0x043760 | 0x043760 through 0x0438D4 | 71 | 373 |
| 0x0438E0 | 0x0438E0 through 0x04401F | 377 | 1856 |
| 0x0486B0 | 0x0486B0 through 0x048816; 0x048825 through 0x048884 | 138 | 455 |
| 0x048FB0 | 0x048FB0 through 0x049072 | 54 | 195 |
| 0x049080 | 0x049080 through 0x04C5BA | 2909 | 13627 |
| 0x04C680 | 0x04C680 through 0x04C6AB; 0x04C6AD through 0x04C6D2 | 21 | 82 |
| 0x04CA10 | 0x04CA10 through 0x04CBEA | 110 | 475 |
| 0x07EA80 | 0x07EA80 through 0x07F35D; 0x07F35F through 0x07F36D; 0x07F36F through 0x07F37D; 0x07F37F through 0x07F38A; 0x07F38C through 0x07F9DF | 806 | 3932 |
| 0x089470 | 0x089470 through 0x0894A7; 0x0894B0 through 0x0894F9 | 39 | 130 |
| 0x089B80 | 0x089B80 through 0x089C01 | 26 | 130 |
| 0x09AF00 | 0x09AF00 through 0x09AF19 | 6 | 26 |
| 0x0A3A20 | 0x0A3A20 through 0x0A3A3F | 8 | 32 |
| 0x0A5A50 | 0x0A5A50 through 0x0A74D7; 0x0A7E6C through 0x0AA9F9; 0x0AAC40 through 0x0ABDE1; 0x0ABE34 through 0x0ABEC9; 0x0ABF82 through 0x0ADBD7 | 6106 | 29860 |
| 0x0F8170 | 0x0F8170 through 0x0F81F1 | 35 | 130 |
| 0x0F8200 | 0x0F8200 through 0x0F837D | 99 | 382 |
| 0x0F8690 | 0x0F8690 through 0x0F87C9 | 67 | 314 |
| 0x0F8D60 | 0x0F8D60 through 0x0F8D6D | 5 | 14 |
| 0x0F92B0 | 0x0F92B0 through 0x0F933D | 41 | 142 |
| 0x0F9510 | 0x0F9510 through 0x0F9591 | 37 | 130 |
| 0x101A00 | 0x101A00 through 0x101A8F | 44 | 144 |
| 0x101BD0 | 0x101BD0 through 0x1028A8 | 665 | 3289 |
| 0x102930 | 0x102930 through 0x102DA5 | 250 | 1142 |
| 0x102F10 | 0x102F10 through 0x102F9C | 30 | 141 |
| 0x102FE0 | 0x102FE0 through 0x1030B5 | 40 | 214 |
| 0x1030C0 | 0x1030C0 through 0x10310C | 18 | 77 |
| 0x103150 | 0x103150 through 0x1031D6 | 25 | 135 |
| 0x1031E0 | 0x1031E0 through 0x1031F0 | 4 | 17 |
| 0x103200 | 0x103200 through 0x106C18 | 3103 | 14873 |
| 0x107690 | 0x107690 through 0x1081A9 | 681 | 2842 |
| 0x276C80 | 0x276C80 through 0x276CC1 | 14 | 66 |
| 0x32ABA0 | 0x32ABA0 through 0x32AC7C | 58 | 221 |
| 0x32AC80 | 0x32AC80 through 0x32ACE2 | 26 | 99 |
| 0x32EE60 | 0x32EE60 through 0x32F051 | 114 | 498 |
| 0x32F640 | 0x32F640 through 0x32F6D0 | 37 | 145 |
| 0x3308E0 | 0x3308E0 through 0x330A77 | 115 | 408 |
| 0x332190 | 0x332190 through 0x33221B | 44 | 140 |
| 0x333890 | 0x333890 through 0x3339BC | 75 | 301 |
| 0x3348E0 | 0x3348E0 through 0x334A12 | 82 | 307 |
| 0x334C50 | 0x334C50 through 0x334D24 | 58 | 213 |
| 0x335830 | 0x335830 through 0x3358D3 | 42 | 164 |
| 0x33CD30 | 0x33CD30 through 0x33CED2 | 117 | 419 |
| 0x33DDA0 | 0x33DDA0 through 0x33DF9C | 121 | 509 |
| 0x367590 | 0x367590 through 0x367667; 0x367669 through 0x367677; 0x367679 through 0x367687; 0x367689 through 0x367694; 0x367696 through 0x367743; 0x367745 through 0x367753; 0x367755 through 0x367763; 0x367765 through 0x367770; 0x367772 through 0x367818; 0x36781A through 0x367828; 0x36782A through 0x367838; 0x36783A through 0x367845; 0x367847 through 0x3678ED; 0x3678EF through 0x3678FD; 0x3678FF through 0x36790D; 0x36790F through 0x36791A; 0x36791C through 0x3679BE | 264 | 1055 |
| 0x3736C0 | 0x3736C0 through 0x37379D; 0x37379F through 0x3737AD; 0x3737AF through 0x3737BD; 0x3737BF through 0x3737CA; 0x3737CC through 0x37386A; 0x37386C through 0x37387A; 0x37387C through 0x37388A; 0x37388C through 0x373897; 0x373899 through 0x373948; 0x37394A through 0x373958; 0x37395A through 0x373968; 0x37396A through 0x373975; 0x373977 through 0x373A1D; 0x373A1F through 0x373A2D; 0x373A2F through 0x373A3D; 0x373A3F through 0x373A4A; 0x373A4C through 0x373AF6; 0x373AF8 through 0x373B06; 0x373B08 through 0x373B16; 0x373B18 through 0x373B23; 0x373B25 through 0x373BE8 | 324 | 1301 |

The following selected slices identify the bounded larger-owner joins and the two uncatalogued straight-line targets. All 936 included instructions and 5,000 bytes matched the image with zero mismatches or internal coverage gaps. Slices inside an owner above overlap that owner's byte check and are not additional unique-image totals. Complete byte coverage of a larger owner is distinguished from the narrower behavioral interpretation in this contract.

| Selected scope | Inclusive range | Instructions | Bytes |
| --- | --- | ---: | ---: |
| Interface setup receiver | 0xF9AC0 through 0xF9B69 | 31 | 170 |
| Root creation | 0xF9CAC through 0xF9D01 | 17 | 86 |
| Icon registry setup | 0xFA1C5 through 0xFA2B7 | 44 | 243 |
| Template loading | 0xFB4D1 through 0xFB62B | 70 | 347 |
| Six-node construction and registration | 0xFFEDE through 0x1003DE | 245 | 1281 |
| Game entry and complete caption prefix | 0xA5A50 through 0xA643F | 458 | 2544 |
| Interface update receiver | 0x103200 through 0x10329C | 32 | 157 |
| Six-node visibility loop | 0x1065CB through 0x10661B | 19 | 81 |
| Later interface update call | 0xAB5B6 through 0xAB5D2 | 6 | 29 |
| Unowned help target | 0x102FA0 through 0x102FD2 | 11 | 51 |
| Unowned callback transfer | 0xF99A0 through 0xF99AA | 3 | 11 |

For the complete-owner table, the individually selected C filename is the full image VA for that RVA followed by .c beneath decompiled/functions. Larger setup source slices came from 1400f9ac0.c. The 1400a5a50.c timeout record was inspected only to establish the absence of a usable C body; the caption prefix was taken from the matched native listing. Original navigation leads 140083b60.c, 140100c10.c, 140100cf0.c, 140100fa0.c, 140101920.c, 140101a90.c and 140101b20.c supplied no additional six-node caption contract. No fact here comes from a prior held prose handoff.

The image data independently checked for the selected literal associations include menubutton at 0x441738; crafting-menu, inventory-menu, worldmap-menu, system-menu, multiplayer-menu and help-menu at 0x441750, 0x441770, 0x441790, 0x4417B0, 0x4417C8 and 0x4417F0; background at 0x4414F8; and icon at 0x4419A8. The selected key-name data include C at 0x430EBC, B at 0x430EC4, M at 0x430ECC, O at 0x430E28, J at 0x430E7C, F1 at 0x430F44, ESC at 0x4315B0 and the question mark at 0x431820. The six setup branches, five record-pointer offsets and fixed two-code-unit sixth copy were checked against their original instruction operands. The callback first-target pointer at 0x441BB8 and the complete transfer bytes at 0xF99A0 were checked independently.

## Evidence limits and exposure

This closes the selected ordinary six-node construction associations, the complete native Game caption prefix, every return branch of its reached key-name formatter, the selected string destination/copy/traversal gates, binding defaults and selected file-record input/copy order, and the later selected visibility no-text loop. It distinguishes a direct node subtree from root controls, node names, icon identifiers, selected TextShape strings and current binding records.

It does not supply the contents of icons.plx, gui.plx, quest-tag.plx or any other asset; the loaded template's initial captions, concrete derived control/shape classes or number of TextShapes; full geometric results of the icon/placement helpers; font metrics, cache refresh/draw scheduling or a numerical imported-stream contract; every writer of the binding/name dictionaries, active property indices, node flags or component pointers; every callback's transitive text effects; or a universal caller/alias/writer census.

The source requests for controls.cfg are native literals and calls only. No installation configuration, owner file, asset/package, capture or runtime output was opened, and no original executable was run. File existence, extraction/formatting results, locale, allocation failures, stream failures, imported provider internals and any storage backend guarantees remain outside the facts established by these native calls.

Valid successful construction and valid native strings/trees are prerequisites for the conditional examples. There is no invented fallback for a null template, absent own control during unguarded registration, fewer than six nodes, invalid selected string/display indices, malformed vector bounds, cycles, stale component pointers or allocation failure followed by use. The selected caption prefix has no acquired lock protecting a snapshot of all five records and six subtrees. The stream-buffer virtual calls and the manager-removal critical section do not establish that broader snapshot. Enclosing caller protection, other threads, reentrant provider calls, retirement and scheduler/message ordering are unproved.

Exposure statement: the author read the standalone source-author policy and this isolated assignment, then filename-selected supported original executable/export evidence and metadata. No active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, combined policy/history, replacement engine/exporter/extractor, tests/results, owner outputs/files/data/captures, assets, packages, SDK, dependency or reconstruction source was accessed. Every prior held artifact remained unread and unchanged. No additional agent was spawned, and no implementation or runtime feedback was requested or consumed.

This complete independent code-free artifact is released only after exclusive creation, whole saved-file readback and exact byte/hash verification. At the separately reported FINAL / STABLE / HELD boundary all author edits stop.
