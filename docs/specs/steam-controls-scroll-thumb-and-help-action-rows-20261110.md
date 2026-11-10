Controls scroll/thumb and Help action rows — supported Steam original

Independent source-only behavioral contract. FINAL / STABLE / HELD applies after the complete saved-file readback and final integrity report accompanying this file. No code, pseudocode, disassembly, instruction encodings, reconstruction or implementation guidance is included.

Authority and separation

The authority is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. The independently reverified file is 5,757,952 bytes, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. It is AMD64, image base 0x140000000, timestamp 0x5D93AB0D, entry RVA 0x3939F0 and SizeOfImage 0x58A000. Routine and data addresses below are RVAs; additions to named receivers are field offsets.

I read only the standalone docs/CLEANROOM_AUTHOR.md role policy, this source-side assignment and individually selected original executable/export evidence. I did not read prior held contracts, active AGENTS.md, memory, combined policy/history, replacement code/exporter/extractor, tests/results, owner files/data/outputs/captures/configuration/assets, SDK/provider/dependency implementations or reconstruction material. No original executable or imported provider was run, and no project implementation/runtime feedback was requested or received. Earlier held artifacts remained unread and unchanged.

This account closes the selected widget, registration, scroll, row-draw and dispatch leaves. Larger Game, Interface, constructor and provider bodies are bounded expressly. Native source operands govern widths, receiver joins and call arguments where a C presentation omits arguments or presents a floating return as an integer. Raw-image correspondence is recorded at the end without reproducing instruction bytes.

Receivers, ordinary setup and separate state

G denotes the Game of constructor 0x7EA80; I is its Interface subobject at G+0x118. The constructor supplies G to 0xF8690, which retains it at I+0x5C0. M is current G+0x20. W is current G+0x8, and E is current W+0x448. These denote source loads, not a promise that every call or thread preserves the pointers.

Q is the Controls widget published at I+0x540, hence G+0x658. The selected original primary table and RTTI identify cube::ControlsWidget. Setup 0xF9AC0 allocates 0x2E8 bytes and, on its ordinary nonnull path, obtains a root by cloning the I+0x310 template under current M+0x140. It calls constructor 0x275A70, publishes Q, requests size 500 by 600 through 0x32C9A0 with its final byte argument one, and deselects Q's root. These are native size requests; transformed/rendered dimensions depend on the current layout properties and providers.

The setup obtains the button template named “button” and the optional templates named “upbutton”, “downbutton” and “scrollbutton” from the original loaded node collection. Their native strings are identified; their external shapes, bounds, class implementations and resource contents were not read. In the actual constructor argument join, the three optional inputs arrive in up, down, scroll order. The code does not establish meaningful fallback geometry when a required template/provider result is absent.

H is the separately allocated 0x1F0-byte Help widget published at I+0x278, hence G+0x390, through constructor 0x277EC0. The original table/RTTI identify cube::HelpWidget. Setup supplies a newly created root from 0x33CD30 on M, attaches it beneath parent I+0x108 through the selected 0x32F640 join, and initially deselects H's root. This does not identify H with a similarly named page, any direct Game page pointer, or Q.

Both constructors use 0x268A70 and its complete base constructor 0x328FF0. The latter retains the supplied root at widget+0x178, manager at widget+0x8 through the selected base join, creates the event registry at +0x188, initializes the active byte +0x1A0 to one and initializes flag storage beginning at +0x150 to zero. Q and H retain G at +0x1E8. Both load the resource identified by the native string “resource1.dat”, length 13 UTF-16 code units, through 0x269130 into +0x1A8. The imported/resource-manager outcome is a dependency, not an assertion about installed assets.

A node's selected integer is accessed through node+0x68, its signed four-byte index +0xAC, and its +0xE0 array, at four-byte stride. The selected root of a widget is the node at widget+0x178. These selected integers differ from widget active byte +0x1A0, M's keyboard target +0x170, retained pointer +0x168, and Q's row fields. The reached small operations do not add index validation or pointer lifetime protection.

Q has 23 staged binding records beginning at +0x1F0, each eight bytes: a signed four-byte code followed by a four-byte category. Its extra nodes are +0x2B8 Defaults, +0x2A8 OK, +0x2B0 Cancel, +0x2C0 up, +0x2C8 down and +0x2D0 scroll/thumb. The last three are node pointers, whose current controls are reached through node+0x70. The constructor retains the optional inputs, replaces each nonnull optional input by its new clone, and leaves a null optional input null.

The constructor clears signed four-byte Q+0x2D8, the scroll-row operand. A single eight-byte all-ones store establishes signed candidate row +0x2DC and capture row +0x2E0 as -1. The candidate is later written by row drawing. The capture row is later used by binding capture and the “Press a key” prompt. It is not M's retained pointer or OS capture.

Staged defaults, buttons and commit/hide effects

The constructor calls 0x41DD0 on Q+0x1F0. That helper clears exactly 23 eight-byte records before calling 0x43760's native default assignments. The Defaults callback 0x276A30 builds the same aggregate in temporary storage and copies exactly 23 eight-byte records into Q in ascending destination order. Neither selected path makes Q's staged aggregate a fresh copy of current G+0x1BDC. No additional ordinary opening-time copy is established here.

These are the resulting default pairs, stated as numeric source data rather than OS/SDK key names. The zero category is part of the stored pair, not a proof of an imported enumeration meaning.

| Row | Code | Category |
| --- | --- | --- |
| 0 | 0 | 1 |
| 1 | 1 | 1 |
| 2 | 42 | 0 |
| 3 | 19 | 0 |
| 4 | 16 | 0 |
| 5 | 17 | 0 |
| 6 | 31 | 0 |
| 7 | 30 | 0 |
| 8 | 32 | 0 |
| 9 | 57 | 0 |
| 10 | 2 | 1 |
| 11 | 18 | 0 |
| 12 | 33 | 0 |
| 13 | 29 | 0 |
| 14 | 20 | 0 |
| 15 | 50 | 0 |
| 16 | 15 | 0 |
| 17 | 48 | 0 |
| 18 | 46 | 0 |
| 19 | 24 | 0 |
| 20 | 36 | 0 |
| 21 | 0 | 0 |
| 22 | 0 | 0 |

After the zero pass, 0x43760's store order is rows 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 3, 13, 2, 14, 15, 16, 17, 20, 18 and 19. Rows 21 and 22 retain their preceding zero initialization.

Q constructs the Defaults node first, then OK, then Cancel, through 0x3308E0 with the common button template and supplied root. It requests each node's control size 140 by 15 and assigns the exact UTF-16 captions “Defaults” (8), “OK” (2) and “Cancel” (6). It registers event identifier 3 with receiver Q and targets 0x276A30, 0x276C80 and 0x276A00 respectively, with the selected propagation and active-required bytes set.

Complete OK callback 0x276C80 first calls 0x89B80(G, Q+0x1F0). That helper copies the 23 staged pairs into current bindings G+0x1BDC through G+0x1C93, preserving the reached native grouping: the first 18 records are copied in eight-byte groups, the next four through individual four-byte stores, then the final record through eight bytes. It subsequently calls 0x438E0. This is a synchronous current-binding publication followed by a reached configuration/persistence helper boundary; successful backend persistence or rollback on failure is not inferred. Only after normal return does OK set Q's root selected integer to zero.

The Cancel target at 0x276A00 is a separately verified linear leaf absent from the ordinary owning-function metadata. Its complete reached window follows Q's root and selected-index chain, stores the selected integer as zero and returns. It does not copy staged records to G. Defaults, OK and Cancel do not directly reset Q+0x2D8, +0x2DC or +0x2E0 in their selected bodies. Thus hiding the root and clearing row capture are distinct source operations.

Optional scroll construction and registration

For each nonnull optional template, Q calls 0x333890 with Q's supplied root as parent input and replaces the corresponding field with the returned node. It next calls virtual slot +0x10 on the node's current +0x60 object, assigns the returned object through 0x334D30, then calls virtual +0x10 on its current +0x68 object and assigns that return through 0x334760. These complete assignment helpers compare old/new pointers, update their selected membership lists or reach original deletion helpers when the selected count is one, and then publish the new pointer. The concrete virtual copy providers are not identified from a template name.

The generic node clone 0x333890 creates the root through 0x33CD30, traverses original child-list order while excluding nodes with mask 0x4 at node+0x80, copies its selected flags/collections and invokes the original control's virtual +0x140 factory when nonnull. Asset-dependent concrete child control types and the deeper factory are limits; cloning a named template is not proof of a rectangle or a universal widget class.

For up, registration matches the exact UTF-16 control name “upbutton” and binds event 2 to 0x276F10(Q). For down it matches “downbutton” and binds event 2 to 0x276EA0(Q). The constructor checks the cloned root's control name when nonnull and traverses its children in original list order through 0x2757B0. For the thumb it calls 0x2757B0 on the cloned root with exact name “scrollbutton”, event 12 and target 0x276AF0(Q). Matching is the original length-aware UTF-16 comparison returning zero, not a guessed tag prefix or rendered caption.

Complete name-registration helper 0x2757B0 checks the current node+0x70 control, compares its name, registers a match, then recursively visits the node's original child-list order. Complete registry helper 0x2758B0 selects an event by signed four-byte integer key in control+0x188. It creates an absent entry through the original tree helpers. An existing callback object is freed before allocating the replacement 0x28-byte object; there is no transactional preservation of the old callback after that free. On the allocator's null path the entry receives null.

The callback object retains the active-required byte at +0x8, raw receiver Q at +0x10, eight-byte target at +0x18, signed four-byte receiver adjustment at +0x20 and the remaining copied descriptor word at +0x24. Constructor registrations establish the adjustment as zero. They do not establish zero for every padding/unused word; the reached invocation adapter does not consume +0x24. No ownership extension of Q is established by storing its raw pointer.

The selected propagation path uses 0x32AC80 and complete collector 0x32ABA0. It excludes node mask 0x4, descends through the current widget root or nodes without their own control, and collects a different nonnull control as a boundary rather than continuing through that control's subtree. The registry helper recursively registers the same event/receiver/descriptor on that returned control collection and releases the temporary list on normal completion. Name recursion and registry propagation are therefore separate traversals with different stopping rules.

The callback adapter at 0x288C40 is not decoded as an ordinary owner in the supplied export. Its complete original 15-byte linear tail-transfer window was independently checked: it forms the adjusted receiver from callback+0x10 and signed +0x20, then transfers to callback+0x18. It neither tests active state nor retains Q. The dispatcher performs the active admission before reaching it. No neighboring routine was substituted for this window.



Scroll measurements and complete layout/adjustment leaves

The following measurements are original helper outputs, not assumed OS pixels. The untransformed pair from 0x32A110 is derived from the control's current binary32 bounds, padding and size operands at +0x70/+0x74, +0x80/+0x84 and +0x90..+0x9C, with the original ordered lower/upper comparisons. Wrappers 0x32A1D0 and 0x329E00 select its first and second components. The transformed pair 0x32D1D0 additionally applies the selected matrix from 0x32C0C0; 0x32DA30 and 0x32AD10 select its components. The source does not take absolute values to turn every transformed result into a generic axis-aligned rectangle.

Complete Q layout virtual target 0x276CD0 is at primary slot +0x50. It positions Defaults first: first coordinate 20, second coordinate formed by transformed Q height minus transformed Defaults-control height, then minus 20. For OK it takes transformed Q width minus 40, subtracts transformed OK-control width, multiplies by binary32 0.5 and adds 20; its second coordinate is transformed Q height minus transformed OK-control height, then minus 20. Cancel's first coordinate is transformed Q width minus 20, then minus transformed Cancel-control width. Its second coordinate uses transformed OK-control height, not Cancel-control height. Every selected position call receives the final byte argument one.

Layout then calls thumb publication 0x276F40. After its normal return, a nonnull up node gets its control positioned at untransformed Q width minus 30, second coordinate 10. A nonnull down node gets first coordinate untransformed Q width minus 30 and second coordinate untransformed Q height minus 140. Up is positioned before down. No row-candidate or capture reset accompanies these positions.

Complete thumb publication 0x276F40 returns immediately when Q+0x2D0 is null. Otherwise it obtains untransformed Q height, subtracts binary32 150, then 35, and truncates that result to signed 32 bits. Call this carried integer the remaining-height operand. It divides that signed integer by 30 toward zero to obtain a carried row-count operand. It multiplies those two integers at native 32-bit width, divides the signed product by 23 toward zero and converts the quotient to binary32 for the requested thumb height.

It reads current untransformed thumb-control width and retains that binary32 result. The remaining-height integer minus the thumb-height integer is multiplied by current signed Q+0x2D8 at 32-bit width. The signed product is divided by 22 minus the carried row count, toward zero; 35 is added as a 32-bit integer before conversion to binary32 for the thumb's second coordinate. The first coordinate comes from a later fresh untransformed Q-width query minus 28. It calls 0x32C680 on the current thumb control with those position values, retained thumb width, computed height and final byte one.

The complete 0x32C680 wrapper publishes position before size: its 0x32B250 call receives byte zero, followed by 0x32C6F0 receiving the supplied final byte. The C presentation's first-call receiver is misleading; native operands retain the control receiver. The generic position setter can do nothing when both current components compare equal. When changed it performs the selected matrix/property conversions, publishes the current selected position pair, invokes the property virtual update and re-queries its selected geometry; byte zero restores the captured property byte +0x49 on that path. The complete size setter updates its selected dimensions, performs its original child/ancestor resize notifications and ends at the receiver's virtual +0x50. Those calls can invoke original virtual code between position and later size/layout work. Their providers and matrix closures are not replaced here by a simple screen rectangle.

Thumb publication itself does not clamp or rewrite Q+0x2D8. It does not guard a zero or exceptional signed divisor 22 minus row count. Native-width products, out-of-range float-to-int conversions, division exceptions and callback changes are not given an invented safe result. A requested height outside ordinary assumptions need not produce a nonnegative or finite thumb.

Complete up callback 0x276F10 first decrements the four-byte scroll operand at native width, then applies its sign-based lower correction to zero, then calls 0x276F40. It has no upper clamp, capture-row test, focus test or local visibility test.

Complete down callback 0x276EA0 first queries untransformed Q height and subtracts binary32 150. It increments Q+0x2D8 before subtracting the additional 35 from the carried float, truncating and dividing by 30. A current scroll value at least 23 minus this row count is replaced by 22 minus the row count. It then calls 0x276F40. It does not run a preceding lower clamp. Thus the up and down leaves are not a shared symmetric clamp helper.

Complete thumb-motion callback 0x276AF0 first reads current Q+0x8 manager and requires mask 0x1 through a one-byte read at manager+0x1A8, within its four-byte button-state storage. Otherwise it returns without querying geometry. It does not additionally require an earlier thumb press, a particular M+0x168 pointer, GUI keyboard focus, candidate row, capture row or selected Q root in its own body.

On admission it obtains the same ordered height-minus-150-minus-35 integer and row count. It queries transformed thumb height and truncates that to signed 32 bits; the available travel is the remaining-height integer minus that truncated height. It queries the thumb's current parent-relative position through 0x329F50, then Q's current relative pointer delta through 0x32B0D0, and adds their second binary32 components. It applies the original maximum with 35 and minimum with available travel plus 35, retaining the clamped binary32 second coordinate.

Only positive available travel admits a fresh scroll calculation: it subtracts 35 from the clamped float, multiplies by the binary32 conversion of 22 minus row count, divides by the binary32 conversion of available travel, truncates and stores signed Q+0x2D8. Nonpositive travel retains the prior scroll operand up to the following corrections. It then changes a negative scroll operand to zero and, afterwards, replaces an operand at least 23 minus row count with 22 minus row count. The final correction can undo nonnegativity for sufficiently large row counts.

Finally the motion callback freshly queries current thumb parent-relative position again, preserves that query's first component and calls 0x32B560 with the clamped second coordinate and byte one. It does not call 0x276F40. Thus dragging can publish a continuous thumb position and a truncated row at the same visit; a later layout/up/down call can snap the thumb back to the discrete publication formula.

0x329F50 searches the root's ancestors for a control, uses the selected current projective matrices and offsets to return a relative position, and has its own original fallback query when no ancestor is found. 0x32B0D0 obtains the Q-relative transform, reaches its original inversion boundary and transforms current M+0x180/+0x184 minus previous +0x188/+0x18C. These inputs are two binary32 coordinate pairs. Their source formulas are directly combined by the callback; common pixel units, matrix invertibility, asset dimensions and imported provider determinism are not inferred.

Complete Controls draw owner 0x276200

The selected draw slot is primary +0x8. This complete body does not first check Q root selection, GUI keyboard focus or M capture. Ordinary admission of the draw call belongs to the surrounding GUI traversal; a direct call can still execute the body under its ordinary pointer preconditions.

Before calculating rows, draw queries node bounds through 0x332AD0 for Defaults, then OK, then Cancel. For each it adds the two returned first-coordinate components and multiplies by binary32 0.5, does the same for the second coordinate, truncates both to signed 32 bits, and appends that eight-byte integer point to current G+0x6C8's list, increasing its eight-byte count at +0x6D0. Each append has the original length-error boundary before final links. Failure there can precede every later row reset. These are exactly the three explicit candidate points produced by this owner; no claim is made that other GUI owners cannot produce more points.

Draw obtains untransformed Q height, subtracts binary32 120, truncates to signed 32 bits and divides by 30 toward zero. It adds the raw old signed scroll operand for its exclusive ending-row candidate. Separately it clamps the starting scroll and that ending candidate to 0 through 23. The loop visits start inclusive, end exclusive, so every reached normal row is in 0..22. It does not clamp the stored Q+0x2D8 field. The margins for this draw range and for thumb arithmetic are different: 120 here versus sequential 150 and 35 in the thumb/scroll leaves.

It next queries Q-relative current pointer position with 0x32B180 and sets Q+0x2DC to -1. It publishes the original text/color settings, including scalar size 12 and secondary scalar 3. If current byte G+0x1958 is nonzero, it sets Q+0x2E0 to -1 before the row loop. The first row's supplied second coordinate is 27, advancing by binary32 30 each iteration, independently of the numeric row identifier chosen as the start.

An active row equal to current capture row uses the native color vector (1, 0.3499999940395355, 0.3499999940395355, 1), publishes its action label with alignment operand zero at first coordinate 20, changes alignment operand to one, and publishes “Press a key”, length 11. The binding/prompt first coordinate is obtained by subtracting 250 from a fresh untransformed Q-width result, multiplying by 0.5 and adding 240. The row's second coordinate is supplied to both calls.

Other rows start with color vector (1,1,1,1); the all-zero staged pair chooses (0.20000000298023224,1,0.20000000298023224,1). Hover can override either choice only when current capture row is negative, current G+0x1958 is zero, relative first coordinate is at least zero and strictly below a fresh Q width minus 40, and relative second coordinate is at least row coordinate minus 20 and strictly below row coordinate plus 10. Success writes the signed row index to Q+0x2DC and uses vector (0.20000000298023224,1,1,1). These comparisons do not make the whole window or thumb a generic rectangle.

Each ordinary row publishes its action label at first coordinate 20 with alignment zero, then the current staged binding through formatter 0x4C830 at the binding coordinate with alignment one. The action-label and formatter calls occur in increasing row order. They do not clear the sampled Game action-state bytes or commit the staged binding aggregate.

After the rows, draw visits nonnull up, down and thumb nodes in that order. It queries 0x32B0C0 on each node's current +0x70 control. That selects vector (0,1,1,1) when the original hit predicate is nonzero, otherwise (1,1,1,1). It stores four binary32 components to the node+0x68 object's selected four-component array at +0x1F0, using that object's signed +0x1BC index and 16-byte stride. This property is separate from the selected-integer array +0xE0. The body itself does not establish every render/provider meaning of those four components.

The complete 0x32B0C0 delegates to 0x32AE90 with the control and its current root. That hit leaf requires node mask 0x4 clear, its selected integer nonzero and either no own control or the supplied control. It transforms current M coordinates through current node/ancestor matrices; then original shape +0x20 or control +0x28 virtual results can admit. If neither admits, a zero selected integer through the separate +0x168 array and signed index +0x134 permits reverse child-list traversal. No asset-independent geometry or semantic property name is inferred for that separate integer.

Exact action-label strings used for the valid Controls row domain

These strings are native UTF-16 literals selected by complete helper 0x42670; lengths exclude the terminator. They are display text, not evidence of imported device code names or every downstream gameplay meaning.

| Row | Native label | UTF-16 length |
| --- | --- | --- |
| 0 | Normal Attack | 13 |
| 1 | Special Attack | 14 |
| 2 | Class Skill | 11 |
| 3 | Ultimate Skill | 14 |
| 4 | Use Quick Item | 14 |
| 5 | Move Forward | 12 |
| 6 | Move Backward | 13 |
| 7 | Move Left | 9 |
| 8 | Move Right | 10 |
| 9 | Jump | 4 |
| 10 | Dodge | 5 |
| 11 | Use/Interact | 12 |
| 12 | Lamp | 4 |
| 13 | Walk | 4 |
| 14 | Call Pet | 8 |
| 15 | World Map | 9 |
| 16 | Ring Menu | 9 |
| 17 | Inventory | 9 |
| 18 | Crafting Menu | 13 |
| 19 | System Menu | 11 |
| 20 | Multiplayer Menu | 16 |
| 21 | Zoom In | 7 |
| 22 | Zoom Out | 8 |

The helper's out-of-domain fallback is not a valid draw-row input in this owner; it is not used to fabricate a 24th row.



Event ownership, pointer dispatch, keys and capture-row publication

Registration names a control at node+0x70; ordinary pointer dispatch uses the possibly different pointer at node+0x78. These fields are not assumed aliases. The selected construction closes their ordinary initialization join: 0x328FF0 attaches the supplied widget through 0x334410 and 0x32D190, which store the node's own +0x70 and widget's +0x178. After constructing the Interface widgets, 0xF9AC0 calls manager refresh 0x340D60 and then 0x333820 on current M+0x140.

Complete refresh 0x333820 skips a node with mask 0x4. For an admitted node with its own control, it calls that control's virtual +0x38, rereads node+0x70 and stores that current pointer at node+0x78. Without an own control it inherits the parent's +0x78 when there is a parent; a parentless node without an own control does not receive a new explicit zero from that branch. It then visits children in original forward list order, without a selected-integer visibility gate. Thus an ordinarily retained cloned control can become its node's dispatch owner even while the widget root is initially deselected. A virtual call can intervene before the fresh +0x70 read.

Complete 0x340D60 first refreshes the current main root this way, then visits its selected additional control list: virtual +0x38 precedes lookup/invocation of event zero. The separate complete update traversal 0x334EF0 also refreshes node+0x78 from own or inherited control, under its mask-0x4 and selected-integer/forced-update gates; it can call shape/property updates and control virtual +0xD8 before recursing. Its bit-0x2000 force rule is distinct from pointer hit admission. General GUI update owner 0x3448A0 is only a discoverable caller boundary here, not a completely analyzed scheduler.

The complete generic callback lookup 0x32A210 selects a signed four-byte event key in control+0x188 and can insert an absent entry through the original tree helpers. Callback invocation uses the returned object only when nonnull. The selected pointer dispatch gate requires node mask 0x2 clear, a nonnull current dispatch owner, and either owner active byte +0x1A0 nonzero or callback active-required byte +0x8 zero. The constructor's selected bindings use required byte one. The 0x288C40 adapter then adjusts Q by its stored signed adjustment zero and transfers to the bound leaf.

Game press 0x94900 calls Interface adapter 0x101020 before its later Game admissions. That complete adapter first copies current M+0x184 into I+0x548, then routes integer button 0 to 0x340EC0, 1 to 0x3415F0, 2 to 0x341300; other values have no such route. The selected button-1 postlude additionally has its own retained/hit test and selected menu clears. This is not a universal conversion of every press to the same GUI event.

The complete button-0 manager press owner 0x340EC0 sets mask 0x1 of its four-byte +0x1A8 input field and chooses retained node M+0x168 when nonnull, otherwise original point query 0x333F60 against current root M+0x140 and point +0x180/+0x184. It performs its selected target/focus work before the chosen dispatch owner's active virtual +0x58, then looks up and invokes event 2 through the current node/owner gate. The virtual call can intervene before the event-owner reread. Its later visible-tree broadcast is a separate traversal.

Manager motion 0x3418D0 copies current point +0x180/+0x184 to previous +0x188/+0x18C, then publishes the two supplied binary32 coordinates. Its target is retained M+0x168 or a fresh query, independent of the keyboard target M+0x170. A changed dispatch owner triggers previous active virtual +0xB8/event 14 before new active virtual +0xB0/event 13. It then reaches the current target's active virtual +0xA8, followed by event 12 using fresh node/owner/callback reads and the stated active-required gate. It stores the target at M+0x160 and subsequently traverses the selected visible-tree broadcast. Hence the thumb callback is reached at event 12 after the target's virtual motion work; target, callback and input operands need not be frozen across those calls.

0x333F60 has native selected-integer and node-mask gates, current transform/shape/control queries and reverse child-list priority. It is not a universal axis-aligned rectangle test. Its complete selected body distinguishes the node's own +0x70 from dispatch owner +0x78 and uses the separate +0x168 selected integer to admit child traversal. Asset shape virtuals, current matrices, indirect controls and allocation/mutation during provider calls remain boundaries.

The event-2 up/down leaves do not implement their own pointer-hit or capture-row test; those admissions belong to the path that invoked the registered callback. The event-12 thumb leaf adds its manager mask-0x1 level test after all preceding motion/target work. The body alone does not demand a particular press origin. A directly invoked callback or retained target can have different admission from a new ordinary point query.

A normal pointer sequence need not correspond one-for-one to window motion messages. The selected window procedure 0x133C70 does not dispatch its 0x200 case directly to these widget callbacks. Ordinary Game motion 0x97880 first queries the Interface composite; on a nonzero low byte it calls 0x101160 with its supplied coordinates truncated to signed 32 bits. The complete selected entry of that adapter converts those integer arguments back to binary32 and calls 0x3418D0 before its timing and unrelated scrolling work. Q's selected root is one component of that composite, which is distinct from focus and capture. The ordinary loop reaches this Game motion after its selected device/sampling branch. This is a source call-order fact, not a claim about every OS cursor event.

Additional resolved motion callers include capture-release helper 0x3493B0, manager removal 0x33DDA0 and a selected Game-update call at 0xAB1A1. That Game call is under both byte G+0x668 and byte G+0x1958 nonzero and supplies current screen-size/coordinate-derived float operands; its large surrounding owner is not otherwise closed here. Complete release helper 0x3493B0 first clears M+0x168, then immediately calls motion at the current coordinates. Reentry or additional callbacks during capture retirement are therefore source-visible possibilities; no no-reentrancy guarantee is supplied.

Game release 0x965E0 performs selected Game work before calling complete Interface release adapter 0x101120. Button 0 reaches 0x3410E0, 1 reaches 0x341780 and 2 reaches 0x3414A0. In the complete button-0 manager owner, the held mask is cleared before choosing retained-or-queried target. Current-target virtual +0x60 precedes event 3. A subsequent fresh retained/hit comparison can reach virtual +0xB8 and event 14 before its later tree broadcast. The window procedure calls the imported ReleaseCapture boundary before Game release; OS capture is separate from M+0x168.

At the later reached Game release postlude, 0x2761F0 is called on current Q and stores capture row -1 before any new capture selection. A fresh Q/root read then requires selected root nonzero, integer button exactly zero and signed current candidate row Q+0x2DC nonnegative before 0x276F30 copies that four-byte candidate to capture row +0x2E0. There is no upper-below-23 check in this copying helper or that selected admission. Valid row drawing produces 0..22, but malformed/alternate writers are not silently normalized. The candidate is not recomputed by release or the scroll callbacks, so a retained candidate can refer to a previous draw.

Complete binding writer 0x2769A0 admits unsigned current capture row below 23. It scans 23 eight-byte staged pairs in ascending order, comparing category then code to the supplied pair, clears every equal pair's two four-byte fields, then rereads the selected capture row and writes the supplied eight-byte pair at that row. It subsequently sets candidate row -1. It does not itself clear capture row. The input record is an actual pointer; aliasing it with the staged storage is not guaranteed to behave as a frozen copy.

Keyboard/character focus is separate. Complete GUI owners 0x340DF0, 0x340E60 and 0x340DD0 dispatch to current M+0x170 only when the target's byte +0x1A0 is nonzero, using slots +0xC0, +0xC8 and +0xD0 respectively. Keydown/up consume 16-bit keys, update the selected manager key bookkeeping and four-byte +0x1AC before virtual dispatch. Q and H's selected primary tables point all those slots to return-only 0xE8A20. No Controls-specific arrow-key scroll behavior or Help-specific key editing is established by those slots. Separately, selected Game keydown 0x96E70 first writes its raw window-key byte, then raw key value 0x70 toggles H's selected root integer between zero and one. This switch precedes its later Chat/capture-row admissions and has no old-key-level guard. The window procedure sends GUI key dispatch before this Game phase without suppressing the latter from a GUI return value. Repeated admitted keydown calls can therefore toggle H repeatedly. The complete composite 0x1014F0 includes Q's selected root but has no direct H-root component; H selected alone does not prove that composite's result. A focused child/template with a different concrete table is a distinct boundary.

Binding capture comes from the selected ordinary physical-input path, not Q's no-op keyboard virtuals. Getter 0x9AE50 returns full unsigned 32-bit one for signed Q+0x2E0 nonnegative, otherwise zero; it does not require Q's root selected or row below 23. In the admitted ordinary foreground/process device branch of 0x1355A0, the nonnegative route scans current-versus-previous key bytes in ascending 0..255 order. A newly nonzero key byte admits the category-zero pair unless its index is 1, 0x1C or 0x9C; it then calls the capture-clear wrapper even for those excluded codes and exits that key scan. The wrapper 0x8A120 loads current Q and tail-calls 0x2769A0; 0x8A4F0 loads current Q and tail-calls 0x2761F0.

At each selected key index without that new-key admission, the source scans four mouse-byte transitions in ascending 0..3 order and can submit category-one pairs. Those writes do not themselves clear capture, so repeated scans and multiple admitted button transitions can revisit the same row; a later admitted pair can replace an earlier one. Imported device-return, unchanged-buffer and source scheduling details beyond this selected producer branch remain explicit limits. The alternative noncapture path contains the sampled-action refresh and XInput block; capture-row admission skips that alternative rather than clearing all prior action states.

Wheel and selected controller routes

Complete Game wheel owner 0x97CC0 considers roots/hit queries for current G+0x330, +0x350 and +0x340 before Q. Q is admitted only when its selected root is nonzero and the original point query 0x333E70 on that root returns nonzero. An admitted negative signed wheel argument calls down leaf 0x276EA0; a positive argument calls up leaf 0x276F10; zero calls neither. The magnitude is not a multiple-step count. This selected route does not test capture row, Chat, M+0x170 or M+0x168. A different earlier admitted widget consumes the branch first.

Complete wrapper 0x333E70 copies current manager coordinates and returns whether 0x333F60 finds any admitted node under the supplied root. That original subtree/shape query is the source predicate; the external template geometry is not inferred.

The supported window procedure normalizes the signed high 16 bits of its 0x20A input to -1, zero or +1, then calls 0x97CC0. The selected controller-provider path can also call the same wheel owner. It requires its admitted ordinary device path, noncapture alternative and successful XInputGetState result zero. In the selected composite-visible branch, the repeat's outer point/root eligibility tests only current G+0x330, +0x340 and +0x350, in that order. It does not include Q. If none admits, its counter is reset and another scalar-input route is used. Q alone being visible under the pointer does not establish controller-generated scroll through this particular branch.

When an earlier eligible root/hit exists, signed four-byte global 0x551CD0 is reduced by the carried loop operand, corrected below at zero, and a zero result admits the selected signed-16 provider component's magnitude test strictly above 17378. It selects counter 400 when retained byte 0x551CCD is zero, otherwise 100, and supplies -1 or +1 from that component's sign to 0x97CC0. The wheel owner's own fresh priority tests then run. Later the source stores the current magnitude predicate to 0x551CCD and clears the counter when false. The full provider record, carried clock operand and scheduler are not assigned SDK guarantees.

The same successful provider route has a separate current-composite nonzero plus selected 16-bit word-mask 0x2000 admission that clears several interface roots and ultimately Q's selected root. This is a level branch, not a proved rising-only callback; the selected stores do not call OK or copy staged bindings. Capture-row admission had skipped this alternative provider path.

Q draw appends Defaults/OK/Cancel points to G+0x6C8, which the selected larger controller branch later reads while selecting cursor candidates. That establishes a concrete list relation, not a universal claim that the up/down/thumb templates are added to that list by Q draw or that any device necessarily selects them. These three explicit points do not prove a navigation target for every loaded thumb or child control.


Inherited virtual targets and their separate flag gates

The two selected primary tables share original targets +0x38 to 0x32B830, +0x40 to 0x32BFD0, +0x48 to 0x32BD70, +0x58 to 0x32B990, +0x60 to 0x32BB40 and +0xA8 to 0x32BB80. Slots +0x20 and +0x28 target the complete three-byte zero-result leaf 0x368230. Slots +0xB0/+0xB8, +0xC0/+0xC8/+0xD0 and +0xD8 target return-only 0xE8A20. Q alone replaces +0x50 with the layout leaf described above; H's +0x50 is return-only. This identifies the concrete ordinary Q/H receiver rather than assigning a template child's unknown table to it.

In selected press virtual 0x32B990, the only work is under current four-byte flag +0x150 mask 0x80. On that branch a two-byte store first clears signed one-byte edge operands +0x180/+0x181, then it queries relative pointer position. For the first axis the lower edge is formed by adding current +0x88 and +0x68, then subtracting +0x78. The competing span is formed from +0x90 plus +0x70 minus +0x80, compared with +0x70 plus +0x98 minus +0x80 after that latter candidate's lower correction to zero. Their larger selected span is added to the lower edge for the upper comparison. A point strictly below the lower edge captures the root through 0x33C410 and writes -1 to +0x180; a point strictly above the upper edge captures it and writes 1. These are sequential tests, not an else pair. The second axis repeats that sequence with +0x8C/+0x6C/+0x7C and +0x94/+0x74/+0x84/+0x9C, storing +0x181. These are ordered finite-value comparisons on current bounds; arbitrary nonfinite geometry is not assigned an invented rectangle.

Complete release virtual 0x32BB40 reads current manager retained node +0x168. If nonnull and its current dispatch owner +0x78 equals this widget, it calls 0x3493B0 on the manager. That can immediately perform another motion traversal after clearing the retained node. Only after normal return does the virtual clear the two edge bytes together. Thus the later event-3 lookup can follow intervening motion/callback work even for Q's inherited release target; returning from a callback does not restore previous target or row values.

Motion virtual 0x32BB80 has two independently tested parts. Its first requires flag mask 0x80, manager held mask 1 and either edge byte nonzero. It gets the current position pair through 0x32C340 and transformed size pair through 0x32D1D0. In first-axis then second-axis order, an edge byte -1 gets a fresh relative delta for the position addition and another fresh relative delta for subtraction from size; edge byte 1 adds a fresh delta to size alone. Other edge-byte values do not apply either change. It then calls the position setter with byte zero before the size setter with byte one. Those repeated delta calls and later setters are ordered provider boundaries, not one frozen delta sample.

After that part, the motion virtual freshly tests control flag mask 3 and manager four-byte held mask 4. On admission it queries current parent-relative pointer position through 0x32C230 and subtracts widget+0xC0/+0xC4. It zeroes the first difference when flag mask 1 is absent and the second when mask 2 is absent. It adds the remaining differences to retained +0xC8/+0xCC, storing the two binary32 components into the current root+0x60 object's selected pair at +0xE0 using signed index +0xAC and eight-byte stride. Property virtual +0x8 with byte one precedes 0x3298B0 and event identifier 0x11 through 0x342560. This is separate property motion; it does not directly update Q's scroll/candidate/capture fields.

The tables also share +0x118 to 0x32B5A0, +0x120 to the separately verified 0x32B6B0 window and +0x138 to 0x32B6E0. The first requires control flag mask 3, a parent chain containing an own control and a successful hit query on that ancestor control. It then retains this root at manager+0x168, gets the pointer pair through 0x32C230 into widget+0xC0/+0xC4, and copies the current root+0x60 selected pair into +0xC8/+0xCC. The +0x120 leaf requires mask 3 and exact equality between retained manager node and this root; it then transfers to 0x3493B0, otherwise returns. No local visibility or capture-row normalization is added.

The +0x138 target requires current flag mask 2 and the same ancestor-control/hit admission. It queries current position through 0x32C340, retains its second component, reads signed four-byte manager+0x1B8, and queries position again for its first component. It multiplies the retained integer by 10 at native 32-bit width, converts to binary32 and adds to the earlier second component. Position publication with byte one precedes 0x3298B0 and event 0x11. This does not directly change Q+0x2D8. The selected Game/controller-to-wheel route proved above is not, by that fact alone, a caller of this virtual slot.

Complete 0x32C230 returns current manager point directly when the root has no parent. Otherwise it reaches the selected ancestor matrix helper 0x100EE0 and applies its current projective transformation, including the ordered reciprocal denominator and two-component output. Complete 0x32C340 applies the current 0x32C0C0 matrix to a zero input point, preserving the explicit zero products and reciprocal denominator before its second and first output stores. Their deeper matrix providers, singular and nonfinite inputs and callback changes remain explicit dependencies.

Q/H base construction initializes the selected flag storage to zero. Thus the nonzero-mask generic press/motion/+0x118/+0x120/+0x138 branches are not new ordinary Controls scroll admissions merely because these targets occur in its table. The inherited release leaf remains meaningful independently of those flag gates. Later generic flag writers and concrete child-control overrides are not universally censused here. Refresh +0x38 and resize +0x40/+0x48 also have their own selected property/resource work; their complete bytes are matched, while arbitrary flagged resizing and all transitive layout providers are outside the specifically closed Q thumb formulas.



Detailed binding formatter and dictionary receivers

Both Q's ordinary row text and H's binding text call complete formatter 0x4C830 with receiver current G+0x30. Game construction calls 0x49080 on G+0x30. The selected constructor initializes its first dictionary head at receiver+0 and its separate compact dictionary at receiver+0x10. This formatter selects the first head, not the compact head at G+0x40. Neither is the action-description dictionary at G+0x50.

A binding record has signed four-byte code at its start and four-byte category at +0x4. The formatter first reads code. Only when it is zero does it read category for the all-zero-pair branch, which returns exact UTF-16 “Unassigned”, length 10. Otherwise it reads category for equality to one. That branch constructs a wide stream, rereads the current code after the stream setup, adds one at native 32-bit width and writes the signed numeric result after exact prefix “Mouse ”, length 6 including the space. Imported wide-stream formatting/locale and exceptional behavior are explicit dependencies.

All other categories use the initially read signed code to search the first dictionary by signed four-byte ordering and equality. A miss returns exact “Unknown”, length 7. A hit copies its stored UTF-16 string. The category is not a universal hardware-enumeration gate; values other than one reach this dictionary branch unless the initial pair was all zero. The formatter does not read OS layouts or synthesize names from a key code. The selected dictionary construction and formatter receiver are proved; its full contents and all later writers are outside this contract rather than imported from a previous dictionary handoff.

The original string objects use two-byte UTF-16 code units with eight-byte length and capacity operands, with the selected inline capacity threshold seven. A copied/displayed string's code-unit length is not a font width. The formatter's code/category reads and its mouse-branch code reread are not an atomic binding-pair snapshot.

Complete Help draw owner 0x278040

The selected primary virtual slot +0x8 is 0x278040. The complete owner was read through its final return and byte-matched. It has no entry guard requiring a selected H root, a name, a health value, keyboard focus, capture row, physical key edge or a particular controller. Surrounding GUI traversal admission is a separate boundary. The body chooses and draws rows each time it is called; it does not install the displayed prose as a persistent H caption.

At entry it prepares a local matrix from the current root's parent, or identity when absent, and combines the current root+0x60 matrix when present. The later shared text helper independently requests the current manager matrix and selected root offsets; this initial local preparation is not evidence of a universal frozen matrix for all later calls.

H sets its supplied size scalar to 12, secondary scalar to 3 and four-component secondary vector to (0,0,0,1). It then reads current G+0x1A72 once as an eight-bit modifier and caches it across the two numeric action-helper calls. It calls 0x5FC40 on the current actor with that byte, retains the full 32-bit numeric result A, and later calls 0x60120 on an actor reloaded at that later call with the same cached byte, retaining full 32-bit result B. Native return/consumer operands resolve the C presentation's misleading char result for the latter. A and B are different inputs to the following source decisions.

Before the first action description, H lazily initializes three process color vectors in order. Guards at 0x5530D8, 0x5530F0 and 0x553108 select copies into 0x5530C8, 0x5530E0 and 0x5530F8 from native constants. The selected source compares its thread epoch, calls the imported initialization-header boundary, copies when the guard is -1 and then calls the footer boundary. Their runtime synchronization implementation is not inspected. The vectors are respectively (0.6000000238418579,1,1,1), (0.6000000238418579,0.6000000238418579,1,1) and (1,0.5,1,1).

Binding-label color is the native vector (0.25,1,1,1); normal description color is (1,1,1,1). Annotation colors use those three guarded vectors as specified below. These are exact selected source components, not an assumed display/color-space transformation.

Source call ordering and coordinates

The following coordinates are binary32 arguments to 0x269210 before that helper's current root/matrix/provider operations. The description column is 180 and annotation column is 320. They are not claimed to be OS pixel coordinates or final glyph bounds.

| Ordered group | Binding/input text | Description text | Annotation calls |
| --- | --- | --- | --- |
| First, second coordinate 0 | Current binding G+0x1BDC, record 0, at first coordinate 0 | Description resolver for A at first coordinate 180 | If A is 0x6C and current actor lookup 0x5E3A0(actor,0x0D) returns null: “[25 STAMINA]” at first coordinate 320 |
| Second, second coordinate 20 | Current binding G+0x1BE4, record 1, normally at first coordinate 0; the complete owner's conditional uppercase-prefix branch is described below | Description resolver for B at first coordinate 180 | Exact B-dependent cases below, at first coordinate 320 |
| Third, second coordinate 40 | Exact prefix “Hold ” plus current binding G+0x1BEC, record 2, at first coordinate 0 | Actor-byte-dependent numeric description key below, at first coordinate 180 | Selected “[STAMINA]” cases, first coordinate 320 |
| Fourth, second coordinate 60 | Exact prefix “Move + ” plus current binding G+0x1C2C, record 10, at first coordinate 0 | Exact literal “Dodge” at first coordinate 180 | Exact “[25 STAMINA]” at first coordinate 320 |
| Optional fifth, second coordinate 80 | Exact prefix “Tap ” plus a fresh current binding G+0x1C2C at first coordinate 0 | The selected numeric description below, at first coordinate 180 | Selected cost literal below, at first coordinate 320 |
| Final | Current binding G+0x1C4C, record 14, at first coordinate 0 | Exact literal “Call/Ride Pet” at first coordinate 180 | None in this selected final group |

Each group formats/publishes its binding first, then publishes its description, then any annotation calls. Temporary strings are released after their reached calls. The exact prefixes have distinct case and spaces: “HOLD ”, “Hold ”, “Move + ” and “Tap ” are not interchangeable. Text at equal coordinates is submitted in the stated order rather than combined into a guessed rich-text line.

The owner's conditional uppercase-prefix test uses retained A, not B: its exact set is 0x09, 0x19, 0x1A, 0x1C, 0x3C and 0x40, bounded by the native unsigned range check. Within that branch it prepends “HOLD ” to record-1 binding text and supplies first coordinate 180, second coordinate 20. The alternative supplies the plain binding at first coordinate zero. This unusual coordinate difference is a native call-argument fact, not a generic two-column convention.

The complete selected normal-return producer 0x5FC40 has return population 0x02, 0x03, 0x07, 0x08, 0x0A, 0x0B, 0x0E, 0x0F, 0x13, 0x14, 0x17, 0x1B, 0x1F, 0x21, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2D, 0x3A, 0x3B, 0x42, 0x43, 0x44, 0x4C, 0x61, 0x68, 0x6B, 0x6C, 0x87 and 0x9E. Its literal/derived return routes are disjoint from that uppercase-prefix set, and the draw owner retains A unchanged through the test. Therefore the mere presence of that branch is not evidence of an ordinarily reached “HOLD ” row through this selected normal-return closure. Its complete owner behavior is recorded without inventing another producer or malformed-control-flow guarantee.

For the B annotations, 0x06, 0x12 or 0x15 produces “[ALL MP]” using the second guarded vector. Members 0x09, 0x19, 0x1A, 0x1C, 0x3C and 0x40 produce “[CHARGE]” using the third vector. B equal to 0x32 first submits “[ALL MP]”, then performs a fresh current-actor 0x0D lookup and, on null, additionally submits “[25 STAMINA]” in the first vector at the same coordinates. B equal to 0x6C only performs that latter lookup/optional annotation. Other B values produce none of these selected second-group annotations. The complete ordinary 0x60120 result population does not contain 0x6C; the B-equals-0x6C branch is retained as owner control flow, not asserted as an ordinary return from this selected helper.

The third group always submits its “Hold ” binding label before checking the current actor byte +0x14C. Its selected numeric description/cost cases are:

| Current byte +0x14C | Byte +0x14D admission | Numeric description identifier | Annotation |
| --- | --- | --- | --- |
| 1 | Any | 0x5F | [STAMINA] |
| 2 | Exactly 1 | 0x51 | [STAMINA] |
| 2 | Other | 0x50 | None |
| 3 | Any | 0x53 | None |
| 4 | Exactly 1 | 0x51 | [STAMINA] |
| 4 | Other | 0x50 | None |
| Other | Any | No description call in this selected group | None |

The fourth group is unconditional once the body reaches it; “Move + ” is literal display text, not a test that a movement action was sampled. The fifth group reloads the actor and selects as follows:

| Current byte +0x14C | Byte +0x14D admission | Numeric description identifier | Literal annotation |
| --- | --- | --- | --- |
| 1 | Any | 0x37 | [100 STAMINA] |
| 2 | Exactly 1 | 0x16 | [50 STAMINA] |
| 2 | Other | 0x34 | [100 STAMINA] |
| 3 | Any | 0x33 | [100 MANA] |
| 4 | Exactly 0 | 0xA0 | [100 STAMINA] |
| Other combinations | — | No fifth-group binding/description/annotation calls | None |

The final group's second coordinate is 100 after a selected fifth group and 80 when that group is omitted. It does not reserve a blank fifth row in the latter case. The third group's omission of a description does not omit its binding label, and the fourth group remains at 60.

Action result inputs and description lookup boundary

0x5FC40 and 0x60120 first distinguish a nonzero supplied modifier or a reached type-0x0D record in actor+0x980. The selected override routes use current bytes +0x14C/+0x14D: the first helper can select 0x61/0x87 for byte 3 and 0x6C for pair 4/1; the second can select 0x86 for pair 3/1 and 0x32 for byte 4. Subsequent routes include actor four-byte +0x64 equal to 0x123, numeric byte ranges beginning 0x91, state/timing operands +0x68/+0x6C, record fields +0x6D8/+0x6DC and +0x638/+0x63C, and reached 0x4F220/0x5EDD0/0x5E9B0 predicate results. These are operand-derived numeric routes, not gameplay labels inferred from equipment offsets. The complete helpers were read and matched; transitive predicate/provider effects and malformed lists are not assumed.

0x60120's normal-return population is 0x02, 0x03, 0x06, 0x09, 0x12, 0x15, 0x18, 0x19, 0x1A, 0x1C, 0x20, 0x22, 0x26, 0x2C, 0x32, 0x3C, 0x40, 0x41, 0x4C, 0x69, 0x6A, 0x86 and 0x9E. Both populations concern reached normal returns with the selected code and ordinary calling convention; they are not promises for corrupted state, invalid pointers, exceptional execution or an alternate caller modifying control flow.

For every dynamic description in the table, H calls complete 0x16C5E0 on current G+0x50 with the numeric identifier. That helper searches the signed-32-key dictionary at receiver+0x18, copying its current UTF-16 token on a hit and producing an empty token on a miss. H then calls 0x16BE20 on current G+0x50, supplying that token, exact category string “default” and an explicitly constructed empty string. This is a distinct dictionary/formatter route from binding-name formatter G+0x30.

The complete selected wrappers 0x16BE20 and 0x16A790 build their temporary context, reach original term processing and assemble returned text; 0x167C80 includes current term lookup at receiver+0x98, special leading-character routes and a selected entry-variant route before its further 0x166CB0 boundary. These source data/provider dependencies can affect returned text. A token miss is not silently assigned a guessed English description.

The Game constructor calls 0x1510F0 on G+0x50. Its main and identified longer-budget C exports both report timeout; this contract does not pretend those presentations close all initialized action/term map contents. No language/configuration/resource assets were inspected. Therefore the fixed literal inventory and exact resolver identifiers/call order are complete for this selected Help owner, while all possible dynamic description strings are conditional on the current original dictionaries and reached text-processing results. The Controls label “Normal Attack”, for example, is not substituted for an unresolved Help description. No prior text/dictionary artifact was used to fill that boundary.

Complete fixed Help literal inventory

These literal strings and code-unit lengths were independently checked in the original image. Spaces and case shown below are significant; the terminator is excluded.

| Native text | UTF-16 length | Role |
| --- | --- | --- |
| default | 7 | Description resolver category |
| HOLD  | 5 | Conditional uppercase binding prefix, ending in a space |
| Hold  | 5 | Third-group prefix, ending in a space |
| Move +  | 7 | Fourth-group prefix, ending in a space |
| Tap  | 4 | Optional fifth-group prefix, ending in a space |
| Dodge | 5 | Fourth-group fixed description |
| Call/Ride Pet | 13 | Final fixed description |
| [25 STAMINA] | 12 | Selected fixed annotation |
| [ALL MP] | 8 | Selected fixed annotation |
| [CHARGE] | 8 | Selected fixed annotation |
| [STAMINA] | 9 | Selected fixed annotation |
| [100 STAMINA] | 13 | Selected fixed annotation |
| [50 STAMINA] | 12 | Selected fixed annotation |
| [100 MANA] | 10 | Selected fixed annotation |

The empty string has zero code units. Prefix rows explicitly describe their trailing spaces because table-cell padding itself is not a string representation. Resource name and binding formatter literals were listed separately above. There are no additional fixed description sentences in this selected Help body.

Shared text publication and provider order

Complete helper 0x269210 receives widget, output pair, current UTF-16 text, supplied first coordinate and supplied second coordinate. If widget+0x1A8 is null, it writes two four-byte zeros to the output and does no selected manager/font draw calls. The surrounding widget can still perform its string selection, row/candidate work and other callbacks.

With a nonnull text resource it calls current manager virtual +0x88 with the current widget root, then virtual +0x70 for its matrix. It takes the root+0x60 object's selected two-component offset at array +0x278 using signed index +0x244, adds the supplied coordinate separately to each component and multiplies both by the matrix's first binary32 component. It supplies current four-byte operands at widget+0x1B0/+0x1B4/+0x1B8/+0x1BC/+0x1C0, alignment operand +0x1E4, four-component vectors at +0x1C4 and +0x1D4, and text to original 0x34FA60. A nonpositive secondary scalar uses the selected single-call route; a positive scalar uses its original first call with a zero-vector argument, then a second call with the normal vector and scalar zero. The selected Q/H draws set that scalar to 3, while later writers remain a limit.

It retains the first selected returned pair for its output, calls manager virtual +0x90 with the root after the draw calls, divides the returned components by the earlier matrix component and subtracts freshly reread selected root offsets. These are ordered operations and provider returns, not hardcoded glyph dimensions. Manager virtuals, imported font implementation, actual resource availability, shaping, font metrics, clipping and final raster appearance are not closed here. No guarantee that matrix scale is nonzero or stable across callbacks is inferred.



Visibility, lifetime and alternative routes

Q root selection, row capture and the scroll position are independent retained operands. Constructor deselection, OK/Cancel deselection and the selected controller root-clear route do not by themselves reset the scroll/candidate/capture fields. The normal release postlude and mode-dependent draw store are separately proved capture resets. There is no assumed fresh-stage reload or scroll reset merely because the root becomes selected again. A complete ordinary root-opening producer and universal later selected-property writer census are not claimed from these bounded joins.

Q's primary draw target is 0x276200, layout target +0x50 is 0x276CD0 and deleting target is 0x2761B0. H's draw target is 0x278040 and deleting target 0x278000. Both complete deleting targets call base teardown 0x329500, then free the object only when the low flag bit is set. The complete base teardown visits its own event registry and frees nonnull callback objects, performs the selected manager/unregister and original container cleanup calls, releases its selected strings and reaches 0x366A10. It does not establish that every child callback retaining Q, every indirect target, captured node or imported resource is safely retired before every possible late call. Those GUI ownership/teardown closures and concurrency are explicit limits, not inferred cleanup from a destructor label.

The stored callback receiver is a raw eight-byte pointer; no selected registration operation proves shared ownership or extends Q's lifetime. Member adjustment, active-required gate and table identity establish normal call operands, not lifetime safety after replacement/deletion. A callback, shape/property virtual, resource call or allocator can intervene before a later current pointer, flag, index or selected value is read. Exception, null, malformed list/index and allocation-failure behavior is not generalized into an ordinary successful UI contract. Setup has unguarded follow-up uses on some null-result paths; no recovery screen is invented.

The selected name recursion, propagation collector, alias refresh, pointer-hit search and update traversal have different gates and iteration orders. A root being deselected does not automatically erase previously stored dispatch/capture pointers. An active control is not synonymous with a visible root. The current callback gate does not itself recheck every ancestor's selected integer; an ordinary fresh hit query and an already retained target can differ. Geometry from shapes or the loaded templates is deliberately not replaced by an inferred rectangle.

The selected ordinary Game/Interface owners and call metadata are not a census of every alternate direct or computed caller. Extra calls to motion can occur during manager removal and release; full removal and arbitrary callback reentrancy are not closed. General frame/update/draw scheduling, all direct-root changes, all later binding/text/map writers, provider implementations, external files, GUI/global lock coverage and process/thread wall-clock interleavings remain unresolved beyond the exact source order stated. No OS/SDK input enumeration names or scheduler guarantees were imported.

Independent conditional arithmetic and event references

These are chosen source operands/provider returns, not runtime observations. They assume valid live receivers, admitted normal paths, normal return from called providers and no intervening mutation of the stated inputs.

- Take an untransformed Q-height return of exactly 600. Thumb arithmetic subtracts 150 then 35 and truncates to 415; signed division by 30 gives 13, and its selected height quotient is 234. With scroll zero the thumb second-coordinate request is 35; with scroll nine it is 216. A separate draw at height 600 adds 16 to the raw starting scroll for its exclusive row end, because its margin is 120. The two row-count calculations are not silently unified.

- With that same height, a transformed thumb-height return of exactly 234 gives available travel 181. A current relative thumb second coordinate plus Q-relative delta of exactly 80 stays inside the finite clamp 35..216. The selected drag stores row two and requests second coordinate 80. A later thumb-publication call with row two instead requests 75, because its integer division truncates before adding 35.

- With width return 500 and first row coordinate 27, ordinary row hover admits relative first coordinate zero and second coordinate 7. It rejects first coordinate 460 because the right comparison is strict, and rejects second coordinate 37 for that row; the next row's lower boundary is 37. These are Q's row tests, not a statement about the external thumb shape.

- Suppose scroll changes after a draw left candidate seven, then a normal button-zero release reaches the Q postlude before another draw. With Q still selected, release first clears capture, then can copy that retained candidate seven back into capture. No native scroll leaf recomputed the candidate. A subsequent accepted binding pair clears duplicates before writing that selected staged row.

- A nonnegative capture row outside 0..22 makes getter 0x9AE50 true but fails 0x2769A0's insertion admission. This can skip the ordinary noncapture producer alternative without creating a valid active row in the draw range. By contrast the mode-byte-nonzero draw path explicitly resets capture to -1 even if its row loop has no iterations, after the earlier center-point work.

- Choose actor bytes +0x14C/+0x14D as 4/1, cached modifier nonzero and no type-0x0D buff record at the reached cost lookups. The selected helper results are A=0x6C and B=0x32. The first group submits “[25 STAMINA]”; the second submits “[ALL MP]” and then “[25 STAMINA]” at the same supplied annotation coordinates. The third group requests description 0x51 and “[STAMINA]”; the fourth is its fixed Dodge group; the fifth is omitted and the final group is at second coordinate 80. The actual dynamic description strings remain the stated current resolver outputs.

- Choose bytes 3/1 and a nonzero cached modifier. A=0x87 and B=0x86 do not admit the second-group cost/charge annotations. The selected third description is 0x53, the optional fifth description is 0x33 with “[100 MANA]”, and the final group is at 100. The displayed primary/secondary description text is not inferred from these identifiers.

- A selected Q root under the pointer, with the three earlier controller-eligibility widgets unselected, admits ordinary Game wheel routing to Q if no earlier wheel-priority widget admits. It does not, by itself, admit the selected XInput-generated wheel-repeat branch, whose outer eligibility list excludes Q. The two call routes are not conflated.

Evidence files, correspondence and coverage

Reference filenames were identified before scoped searches. Original authority/export files were decompiled/functions.csv, function_ranges.csv, calls.csv, assembly.txt and symbols.csv. C presentations were individually selected under decompiled/functions using the complete owner RVA in the filename. The original successful retry presentation decompiled/retry-180-phase1/functions/1400a5a50.c was used only for selected caller structure; native operands govern its actual calls. The main and decompiled/retry-180-phase2/functions/1401510f0.c timeout headers were inspected to bound the action/term constructor presentation gap, not treated as a decoded implementation. No previous prose handoff was opened.

Whole-body semantic inspection covered the selected Q/H constructors, Q draw/layout/scroll/capture leaves, H draw, callback registration/name/propagation leaves, selected binding formatter and label leaf, base/alias/pointer adapters, helper wrappers and selected teardown bodies described above. Larger Game/Interface/setup/provider owners were read only in the expressly identified joins, even where their complete exported instruction ranges were checked. Discovery-only neighboring owners are not assigned widget identities from their offsets. In particular the read of 0x33AAF0 rejected a tempting node-alias interpretation: its concrete receiver is a different destructor, and its +0x78 write is not used to explain the Controls node alias.

For every tabled owner, exported original instruction encodings were compared against the supported executable through its PE section mapping. Within each inclusive exported range, the instruction coverage was checked continuously from first through final byte. Separate ranges are not joined by attributing intervening bytes to a function. The table gives complete matched bounds while preserving the semantic scope limits above.


Matched original owner manifest

The following bounds are inclusive RVAs. All 133 metadata-owned entries below matched the raw image with no differing instruction encodings and no missing or overlapping instruction coverage inside any listed range. They comprise 33,205 original exported instructions and 153,942 matched instruction bytes. Byte matching authenticates correspondence; it does not enlarge the semantic scopes stated above.

| Owner RVA | Inclusive matched range or ranges | Instructions | Matched bytes |
| --- | --- | --- | --- |
| 0x41DD0 | 0x41DD0–0x41E00 | 16 | 49 |
| 0x42670 | 0x42670–0x429FA | 186 | 907 |
| 0x43760 | 0x43760–0x438D4 | 71 | 373 |
| 0x49080 | 0x49080–0x4C5BA | 2909 | 13627 |
| 0x4C830 | 0x4C830–0x4CA09 | 110 | 474 |
| 0x5E3A0 | 0x5E3A0–0x5E3C9 | 15 | 42 |
| 0x5FC40 | 0x5FC40–0x600F2 | 358 | 1203 |
| 0x60120 | 0x60120–0x603C4 | 201 | 677 |
| 0x6ECE0 | 0x6ECE0–0x6ED14 | 16 | 53 |
| 0x7EA80 | 0x7EA80–0x7F35D; 0x7F35F–0x7F36D; 0x7F36F–0x7F37D; 0x7F37F–0x7F38A; 0x7F38C–0x7F9DF | 806 | 3932 |
| 0x89B80 | 0x89B80–0x89C01 | 26 | 130 |
| 0x8A120 | 0x8A120–0x8A12B | 2 | 12 |
| 0x8A4F0 | 0x8A4F0–0x8A4FB | 2 | 12 |
| 0x94580 | 0x94580–0x945F2 | 26 | 115 |
| 0x94900 | 0x94900–0x965DF | 1583 | 7392 |
| 0x965E0 | 0x965E0–0x96832; 0x96834–0x96842; 0x96844–0x96852; 0x96854–0x9685F; 0x96861–0x96E6F | 508 | 2188 |
| 0x96E70 | 0x96E70–0x977F2 | 550 | 2435 |
| 0x97880 | 0x97880–0x97CB9 | 201 | 1082 |
| 0x97CC0 | 0x97CC0–0x9803D | 190 | 894 |
| 0x9AE50 | 0x9AE50–0x9AE62 | 5 | 19 |
| 0xA3A20 | 0xA3A20–0xA3A3F | 8 | 32 |
| 0xA5A50 | 0xA5A50–0xA74D7; 0xA7E6C–0xAA9F9; 0xAAC40–0xABDE1; 0xABE34–0xABEC9; 0xABF82–0xADBD7 | 6106 | 29860 |
| 0xE8A20 | 0xE8A20–0xE8A22 | 1 | 3 |
| 0xF8690 | 0xF8690–0xF87C9 | 67 | 314 |
| 0xF9AC0 | 0xF9AC0–0x10089D | 5496 | 28126 |
| 0x101020 | 0x101020–0x10111E | 58 | 255 |
| 0x101120 | 0x101120–0x10115E | 16 | 63 |
| 0x101160 | 0x101160–0x1014E3 | 187 | 900 |
| 0x1014F0 | 0x1014F0–0x10178B | 119 | 668 |
| 0x101BD0 | 0x101BD0–0x1028A8 | 665 | 3289 |
| 0x133C70 | 0x133C70–0x133EEB | 174 | 636 |
| 0x1355A0 | 0x1355A0–0x137E1B | 2217 | 10364 |
| 0x167C80 | 0x167C80–0x16811E | 292 | 1183 |
| 0x16A790 | 0x16A790–0x16AF2F | 509 | 1952 |
| 0x16BE20 | 0x16BE20–0x16C091 | 169 | 626 |
| 0x16C5E0 | 0x16C5E0–0x16C67D | 46 | 158 |
| 0x268A70 | 0x268A70–0x268B39 | 36 | 202 |
| 0x269130 | 0x269130–0x269152 | 10 | 35 |
| 0x269160 | 0x269160–0x269183 | 9 | 36 |
| 0x269190 | 0x269190–0x269196 | 2 | 7 |
| 0x2691B0 | 0x2691B0–0x2691B8 | 2 | 9 |
| 0x2691C0 | 0x2691C0–0x2691E3 | 9 | 36 |
| 0x2691F0 | 0x2691F0–0x2691F8 | 2 | 9 |
| 0x269210 | 0x269210–0x26958E | 158 | 895 |
| 0x275770 | 0x275770–0x2757AB | 17 | 60 |
| 0x2757B0 | 0x2757B0–0x2758A1 | 65 | 242 |
| 0x2758B0 | 0x2758B0–0x275A6D | 111 | 446 |
| 0x275A70 | 0x275A70–0x2761AC | 425 | 1853 |
| 0x2761B0 | 0x2761B0–0x2761E3 | 16 | 52 |
| 0x2761F0 | 0x2761F0–0x2761FA | 2 | 11 |
| 0x276200 | 0x276200–0x276993 | 429 | 1940 |
| 0x2769A0 | 0x2769A0–0x2769FA | 23 | 91 |
| 0x276A30 | 0x276A30–0x276AED | 39 | 190 |
| 0x276AF0 | 0x276AF0–0x276C73 | 90 | 388 |
| 0x276C80 | 0x276C80–0x276CC1 | 14 | 66 |
| 0x276CD0 | 0x276CD0–0x276E95 | 107 | 454 |
| 0x276EA0 | 0x276EA0–0x276F06 | 26 | 103 |
| 0x276F10 | 0x276F10–0x276F2F | 6 | 32 |
| 0x276F30 | 0x276F30–0x276F3C | 3 | 13 |
| 0x276F40 | 0x276F40–0x27705C | 70 | 285 |
| 0x277EC0 | 0x277EC0–0x277FF6 | 74 | 311 |
| 0x278000 | 0x278000–0x278033 | 16 | 52 |
| 0x278040 | 0x278040–0x279A72 | 1494 | 6707 |
| 0x328F00 | 0x328F00–0x328F6F | 31 | 112 |
| 0x328FF0 | 0x328FF0–0x329206 | 96 | 535 |
| 0x329500 | 0x329500–0x329621; 0x329623–0x329638; 0x32963A–0x329648; 0x32964A–0x329658; 0x32965A–0x329665; 0x329667–0x3296A9; 0x3296AB–0x3296C0; 0x3296C2–0x3296D0; 0x3296D2–0x3296E0; 0x3296E2–0x3296ED; 0x3296EF–0x32972C | 143 | 547 |
| 0x3298B0 | 0x3298B0–0x329C64 | 237 | 949 |
| 0x329E00 | 0x329E00–0x329E33 | 12 | 52 |
| 0x329F50 | 0x329F50–0x32A109 | 91 | 442 |
| 0x32A110 | 0x32A110–0x32A1CE | 45 | 191 |
| 0x32A1D0 | 0x32A1D0–0x32A202 | 12 | 51 |
| 0x32A210 | 0x32A210–0x32A2B4 | 46 | 165 |
| 0x32A2C0 | 0x32A2C0–0x32A30B | 22 | 76 |
| 0x32A8D0 | 0x32A8D0–0x32AA70 | 100 | 417 |
| 0x32ABA0 | 0x32ABA0–0x32AC7C | 58 | 221 |
| 0x32AC80 | 0x32AC80–0x32ACE2 | 26 | 99 |
| 0x32AD10 | 0x32AD10–0x32AD43 | 12 | 52 |
| 0x32AE90 | 0x32AE90–0x32B0B8 | 138 | 553 |
| 0x32B0C0 | 0x32B0C0–0x32B0CB | 2 | 12 |
| 0x32B0D0 | 0x32B0D0–0x32B17B | 37 | 172 |
| 0x32B180 | 0x32B180–0x32B248 | 45 | 201 |
| 0x32B250 | 0x32B250–0x32B553 | 166 | 772 |
| 0x32B560 | 0x32B560–0x32B59E | 14 | 63 |
| 0x32B5A0 | 0x32B5A0–0x32B6AD | 64 | 270 |
| 0x32B6E0 | 0x32B6E0–0x32B820 | 79 | 321 |
| 0x32B830 | 0x32B830–0x32B98C | 74 | 349 |
| 0x32B990 | 0x32B990–0x32BB3D | 92 | 430 |
| 0x32BB40 | 0x32BB40–0x32BB7C | 16 | 61 |
| 0x32BB80 | 0x32BB80–0x32BD67 | 109 | 488 |
| 0x32BD70 | 0x32BD70–0x32BFCC | 148 | 605 |
| 0x32BFD0 | 0x32BFD0–0x32C088 | 43 | 185 |
| 0x32C230 | 0x32C230–0x32C333 | 57 | 260 |
| 0x32C340 | 0x32C340–0x32C3F0 | 42 | 177 |
| 0x32C680 | 0x32C680–0x32C6E8 | 24 | 105 |
| 0x32C6F0 | 0x32C6F0–0x32C99E | 160 | 687 |
| 0x32C9A0 | 0x32C9A0–0x32C9DE | 14 | 63 |
| 0x32D190 | 0x32D190–0x32D1A0 | 5 | 17 |
| 0x32D1D0 | 0x32D1D0–0x32D2DC | 61 | 269 |
| 0x32DA30 | 0x32DA30–0x32DA62 | 12 | 51 |
| 0x32EE60 | 0x32EE60–0x32F051 | 114 | 498 |
| 0x32F640 | 0x32F640–0x32F6D0 | 37 | 145 |
| 0x3308E0 | 0x3308E0–0x330A77 | 115 | 408 |
| 0x332AD0 | 0x332AD0–0x332B96 | 47 | 199 |
| 0x333820 | 0x333820–0x33388C | 35 | 109 |
| 0x333890 | 0x333890–0x3339BC | 75 | 301 |
| 0x333E70 | 0x333E70–0x333EC7 | 19 | 88 |
| 0x333F60 | 0x333F60–0x33429A | 215 | 827 |
| 0x334410 | 0x334410–0x334506 | 60 | 247 |
| 0x334760 | 0x334760–0x3347E6 | 35 | 135 |
| 0x334D30 | 0x334D30–0x334DB6 | 35 | 135 |
| 0x334EF0 | 0x334EF0–0x33500C | 78 | 285 |
| 0x3356D0 | 0x3356D0–0x335773 | 42 | 164 |
| 0x335830 | 0x335830–0x3358D3 | 42 | 164 |
| 0x33AAF0 | 0x33AAF0–0x33AB62 | 31 | 115 |
| 0x33C360 | 0x33C360–0x33C406 | 49 | 167 |
| 0x33C410 | 0x33C410–0x33C417 | 2 | 8 |
| 0x33CD30 | 0x33CD30–0x33CED2 | 117 | 419 |
| 0x33DDA0 | 0x33DDA0–0x33DF9C | 121 | 509 |
| 0x340D60 | 0x340D60–0x340DC8 | 29 | 105 |
| 0x340DD0 | 0x340DD0–0x340DEF | 8 | 32 |
| 0x340DF0 | 0x340DF0–0x340E56 | 26 | 103 |
| 0x340E60 | 0x340E60–0x340EB7 | 22 | 88 |
| 0x340EC0 | 0x340EC0–0x3410D8 | 138 | 537 |
| 0x3410E0 | 0x3410E0–0x3412F4 | 141 | 533 |
| 0x341300 | 0x341300–0x341494 | 103 | 405 |
| 0x3415F0 | 0x3415F0–0x341776 | 102 | 391 |
| 0x3418D0 | 0x3418D0–0x341B28 | 163 | 601 |
| 0x342560 | 0x342560–0x3425BF | 29 | 96 |
| 0x3448A0 | 0x3448A0–0x346508 | 1607 | 7273 |
| 0x3493B0 | 0x3493B0–0x3493CF | 4 | 32 |
| 0x349A90 | 0x349A90–0x349B33 | 45 | 164 |
| 0x3669C0 | 0x3669C0–0x366A01 | 18 | 66 |
| 0x368230 | 0x368230–0x368232 | 2 | 3 |

Separately matched linear windows are Cancel 0x276A00–0x276A20, six instructions and 33 bytes, and inherited virtual target 0x32B6B0–0x32B6D5, nine instructions and 38 bytes. The ordinary export attributes neither window to a complete metadata owner; each selected window was checked from entry through its return or outgoing tail transfer. Adapter 0x288C40–0x288C4E is a separately manually decoded 15-byte, four-instruction linear window absent from ordinary decoded owner rows; its complete receiver-adjustment/tail-transfer operation was checked directly from the supported image. The next alignment byte was excluded. These three windows total 19 instructions and 86 bytes. The combined checked instruction coverage is 33224 instructions and 154028 bytes; no raw encodings are reproduced here.

Additional selected original data correspondence

- Q's primary table begins at 0x46E468 and H's at 0x46E8A8. Each selected table span through slot +0x140 contains 41 eight-byte pointers, 328 bytes. Their original locator pointers lead to 0x4B02D8/0x4B04E8 and type objects 0x54B5F0/0x54B6A8, respectively. The original class-name data identify ControlsWidget and HelpWidget in namespace cube. The table operands, locator/type links and selected targets were read from the supported raw image, without taking a concrete child table from a template name.

- The class-range dispatch tables used by 0x5FC40 and 0x60120 occupy 0x600F4–0x60113 and 0x603C8–0x603E7, respectively. Each is eight four-byte target RVAs for the native bounded byte range 0x91..0x98. Their entry order and original target correspondence were checked separately from owner instruction coverage. No untraced target was added to either normal-return population.

- The scalar source-data reads checked for the selected Q/H arithmetic include binary32 3 at 0x42E580, 10 at 0x42E598, 12 at 0x42E59C, 20 at 0x42E5A0, 30 at 0x42E5A8, 40 at 0x42E5AC, 500 at 0x42E5C8, 15 at 0x42E7A4, 60 at 0x42E7B8, 120 at 0x42E7C8, 600 at 0x42E7EC, 150 at 0x42E938, 240 at 0x432D94, 250 at 0x43293C, 35 at 0x441BC0, 28 at 0x46E620 and 140 at 0x4ACA38. Integral constants and store widths were also reconciled with the matched native instruction operands. Arithmetic descriptions preserve the ordered binary32 and signed-width operations actually reached; this inventory does not reinterpret adjacent four-byte storage as a double.

- The four-component source vectors occupy 16-byte spans beginning at 0x46D970, 0x42E8C0, 0x4381E0, 0x46EB50, 0x46EB40, 0x46E640, 0x46E630, 0x4381C0 and 0x4381B0. Their individual binary32 components were read independently and are stated with the appropriate row/style/annotation decisions above. The guarded output/guard locations at 0x5530C8/0x5530D8, 0x5530E0/0x5530F0 and 0x5530F8/0x553108 were matched to the selected Help owner operands. Imported initialization locking remains uninspected.

Literal-address manifest

Each entry below identifies the beginning of a separately checked native UTF-16 literal. The length is the number of two-byte code units before its terminator. Exact display contents, significant spaces and valid row associations are given in the earlier tables; this manifest adds address provenance rather than a resource-file assumption.

| Literal RVA | Native text or identified role | UTF-16 length |
| --- | --- | --- |
| 0x4303B8 | Normal Attack | 13 |
| 0x4303D8 | Special Attack | 14 |
| 0x4303F8 | Move Forward | 12 |
| 0x430418 | Move Backward | 13 |
| 0x430438 | Move Left | 9 |
| 0x430450 | Move Right | 10 |
| 0x430468 | Jump | 4 |
| 0x430478 | Dodge | 5 |
| 0x430488 | Use/Interact | 12 |
| 0x4304A8 | Lamp | 4 |
| 0x4304B8 | Ultimate Skill | 14 |
| 0x4304D8 | Class Skill | 11 |
| 0x4304F0 | Walk | 4 |
| 0x430500 | Call Pet | 8 |
| 0x430518 | World Map | 9 |
| 0x430530 | Use Quick Item | 14 |
| 0x430550 | Ring Menu | 9 |
| 0x430568 | Inventory | 9 |
| 0x430580 | Multiplayer Menu | 16 |
| 0x4305A8 | Crafting Menu | 13 |
| 0x4305C8 | System Menu | 11 |
| 0x4305E0 | Zoom In | 7 |
| 0x4305F0 | Zoom Out | 8 |
| 0x430608 | Unknown  (trailing space) | 8 |
| 0x4317E8 | Unassigned | 10 |
| 0x431800 | Mouse  (trailing space) | 6 |
| 0x431810 | Unknown | 7 |
| 0x436340 | resource1.dat | 13 |
| 0x4363F8 | default | 7 |
| 0x437350 | Cancel | 6 |
| 0x441058 | upbutton | 8 |
| 0x441070 | downbutton | 10 |
| 0x441088 | scrollbutton | 12 |
| 0x46E5D8 | Defaults | 8 |
| 0x46E5EC | OK | 2 |
| 0x46E5F8 | Press a key | 11 |
| 0x46EA18 | [25 STAMINA] | 12 |
| 0x46EA38 | HOLD  (trailing space) | 5 |
| 0x46EA48 | [ALL MP] | 8 |
| 0x46EA60 | [CHARGE] | 8 |
| 0x46EA78 | Hold  (trailing space) | 5 |
| 0x46EA88 | [STAMINA] | 9 |
| 0x46EAA0 | Move +  (trailing space) | 7 |
| 0x46EAB0 | Tap  (trailing space) | 4 |
| 0x46EAC0 | [100 STAMINA] | 13 |
| 0x46EAE0 | [100 MANA] | 10 |
| 0x46EAF8 | [50 STAMINA] | 12 |
| 0x46EB18 | Call/Ride Pet | 13 |

The selected setup's common “button” template name and explicitly empty temporary strings were checked at their native setup/use sites. The table does not imply that separately loaded asset text is a native literal. Action descriptions returned through the current G+0x50 maps are the expressly unresolved text-data boundary described above.

Final scope and integrity

This is one complete independent source-side contract for the bounded Controls scroll/thumb and Help action-row question. It contains selected literal text and factual numeric operands, not an original routine transcription, pseudocode or replacement guidance. Whole saved-file readback and byte-for-byte UTF-8 verification precede the accompanying FINAL / STABLE / HELD report. That report supplies the exact saved byte count and SHA256; the digest is external to this file to avoid a self-referential hash. After that report all writes to this artifact cease. Prior held artifacts were neither read nor changed.
