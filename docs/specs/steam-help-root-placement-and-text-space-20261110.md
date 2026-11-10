# Original Steam Help root placement and text-space ownership

Source-side behavioral contract, 2026-11-10.

## Authority and scope

This contract concerns the concrete Help object constructed at RVA 0x277EC0 by the selected Interface setup route 0xF9AC0, its node and parent chain, ordinary placement at 0x101BD0, resize notifications, and the text route from Help draw 0x278040 through 0x269210. It independently derives the relationships below from selected original evidence. It does not import any earlier handoff.

The supported original file is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. Its length is 5,757,952 bytes and SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. It is AMD64, machine 0x8664, preferred image base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0 and SizeOfImage 0x58A000. The file was read as data, not executed. All addresses below are RVAs; field offsets are hexadecimal.

Offsets identify operands, not a generic GUI convention. Four-byte geometric values below are binary32 unless a signed integer or a raw word is expressly identified. Pointers and collection links in this x64 image are eight bytes. No claim that an allocation is wholly zeroed follows from selected constructor stores.

## Concrete receivers and construction

The selected Game constructor 0x7EA80 stores its fifth argument at G+0x20 and constructs an Interface inline at G+0x118 through 0xF8690. The latter stores G at I+0x5C0. Thus I here is exactly G+0x118. This is a proved address relationship, not an offset-name match.

The ordinary startup joins identify the object passed as G+0x20. At 0x134CC0's selected construction window, an allocation of 0x5D8 is passed to 0x38E100 and the result is published at process RVA 0x551AC0. Startup 0x1355A0 subsequently supplies that pointer as the Game's fifth argument. The concrete constructor installs the primary table at 0x480408 after calling its base constructor 0x339B70. M below denotes this manager. Allocation and initialization failure paths are not asserted to provide a usable M, G or Help.

The base manager constructor creates a node with the UTF-16 name root and publishes it at M+0x140. It also creates separate sources and fonts nodes at M+0x148 and M+0x150; those are not the Help parent. For the root node R, the selected factory inputs request fresh spatial and display receivers, an absent optional node+0x58 receiver, and no parent. Initial manager viewport operands include signed DWORD width 1000 at +0x1C4 and height 800 at +0x1C8, with the two origin operands at +0x1BC/+0x1C0 zero. These constructor values do not prove the values at an eventual Help draw after startup/resize providers run.

The selected Interface setup window calls node factory 0x33CD30 with M, no supplied spatial, optional node+0x58 or display receivers, R as parent, and an empty UTF-16 name. The resulting node P is stored at I+0x108. Later in the same setup body, a 0x1F0 allocation is prepared for the Help object. A second call to 0x33CD30 uses M, fresh receiver requests again, P as parent, and an empty name. Its result is N, the concrete Help node passed to 0x277EC0. The Help result H is stored at I+0x278. Therefore the pointer P is stored at G+0x220, while the pointer H is stored at G+0x390. N is reached through H+0x178; P and H are different objects.

Factory 0x33CD30 creates a 0x328 spatial receiver when its spatial argument is null, a 0x338 display receiver when its display argument is null, then a 0xC8 node. The actual native argument joins distinguish the node's spatial receiver at N+0x60, optional receiver at N+0x58 and display receiver at N+0x68. The fresh Help factory supplies null for N+0x58. Parent attachment 0x32F640 appends the node to the parent's linked child collection and stores the parent at the child's +0x40. A preexisting parent would be detached first by another helper; that alternate route is not needed to identify the new node's initial parent.

The widget base route 0x328FF0 stores the supplied node at H+0x178 and registers the widget in M's collection at +0xD0 under M+0x90's critical section, through 0x33BFE0. Binding 0x334410 reaches 0x32D190 on the fresh empty node: N+0x70 becomes H and H+0x178 becomes N. N+0x70 is the widget binding; N+0x58 is a separate optional receiver. The replacement-binding branch of 0x334410 first queries and transfers the old widget's selected geometry, unregisters it and clears the old binding before binding a new receiver; the newly created Help node takes the empty-binding branch.

Help construction calls 0x268A70 with M and N, installs its primary table 0x46E8A8 and secondary table 0x46EA08, then stores G at H+0x1E8. It requests the UTF-16 literal resource1.dat, thirteen code units, through 0x269130. That helper obtains a font/resource object through the manager's receiver at M+0x48 and stores the returned pointer at H+0x1A8. The selected original lookup 0x34D060 can reuse a cached object or allocate one, attempt loading through 0x352DF0, try registered search paths, and delete/remove an unsuccessful object before returning null. No external resource or font file was read for this contract, so neither resource availability nor the value of its internal branch operand F+0x148 is assumed.

The font-manager constructor 0x34C410 stores M at its +0x8; a font/resource allocated by 0x34EBF0 likewise stores that manager argument at F+0x8. Consequently the ordinary font calls below use the same concrete M, subject to a successful selected construction/lookup. The backend/loading/allocator closure remains a boundary.

After publishing H, Interface setup passes N and a zero byte to 0xA3A20. That writes DWORD zero to the display receiver's currently selected visibility entry. It neither writes a geometric position nor destroys H. The constructor and this selected setup sequence contain no explicit Help placement call to 0x334700 or 0x334510.

## Relevant initialized fields and property ownership

The following table describes the selected fields at the end of the base Help construction, before draw-time style writes and later layout/resize changes. It deliberately does not assign a value to every other byte.

| Receiver fields | Selected initialization or relationship |
|---|---|
| H+0x8, +0x178, +0x1E8 | M, N, G respectively |
| H+0x150 and +0x154 | Both DWORD zero, supplied by one eight-byte zero store |
| H+0x158 | DWORD zero |
| H+0x168/+0x170 | Null eight-byte pointers |
| H+0x188/+0x190 | Empty callback map head and zero count, established by the selected tree-head constructor |
| H+0x1A8 | Initially null, then replaced by the selected resource lookup result |
| H+0x1B0/+0x1B4 | Binary32 12 and zero, from one eight-byte store |
| H+0x1B8/+0x1BC/+0x1C0 | Binary32 zero |
| H+0x1C4 through +0x1E0 | Eight binary32 values initially one |
| H+0x1E4 | DWORD zero, supplied by the high half of the eight-byte store starting at +0x1E0 |
| H+0x110..+0x14F | A sixteen-element binary32 identity matrix |
| N+0x40/+0x60/+0x68/+0x70 | P, its spatial receiver, its display receiver, H |
| N+0x58/+0x80/+0x84/+0x88 | Null optional receiver, zero DWORD, zero byte, null pointer in the selected node construction |

The selected direct constructor stores do not establish zero defaults for the cached viewport DWORDs H+0x160/+0x164. No assumption that their first-notification values are zero is used below.

For any freshly requested spatial receiver T, original property-name construction at 0x372BD0 proves these names and storage relationships:

| Original property literal | Property subobject | Signed DWORD selected index | Array begin | Element width |
|---|---:|---:|---:|---:|
| translation | T+0x70 | T+0xAC | T+0xE0 | Two binary32 values, 8 bytes |
| deformation | T+0xF8 | T+0x134 | T+0x168 | Sixteen binary32 values, 0x40 bytes |
| rotation | T+0x180 | T+0x1BC | T+0x1F0 | Three binary32 values, 0xC bytes |
| pivot | T+0x208 | T+0x244 | T+0x278 | Two binary32 values, 8 bytes |

The common property constructor's eight-byte zero store at its +0x38 includes the selected index at its +0x3C. Each of these four arrays is resized to two elements. Selected translation and pivot pairs are zero; rotation triples are zero. The constructor explicitly replaces the selected deformation element with identity. It also explicitly initializes the cached matrices at T+0x2A0 and T+0x2E0 to identity. This does not claim all unselected deformation source bytes or allocation padding were initialized by a whole-span zero operation.

The analogous fresh display receiver is built by 0x3736C0. Its named visibility property uses a signed DWORD index at +0xAC and DWORD array begin +0xE0; the selected entry starts at one. Its separately named clipping property uses index +0x134 and DWORD array begin +0x168; that selected entry starts at zero. Its blurRadius entry is independently initialized to binary32 zero. Thus the display receiver's +0x168 is not a matrix array, despite the same numeric offset being a matrix array in the spatial receiver. The setup visibility-zero store above changes the visibility entry, not clipping.

The two base calls 0x335830 and 0x3356D0 operate on the node's spatial/display ownership. A nonnull receiver with more than one registered user can be cloned and rebound, with the old membership removed first. The selected new factory creates a fresh receiver and registers this node as its first user. The concrete clone targets, 0x373310 and 0x373DE0, are checked separately in the evidence manifest; arbitrary shared receivers or later clone/copy producers are not silently treated as fresh default state.

## Ordinary placement and matrix publication

The complete selected layout owner is 0x101BD0. Its receiver is I; its second and third native integer arguments are retained signed 32-bit dimensions for this invocation. It first resolves the manager root R through the current I+0x5C0 Game, G+0x20 and M+0x140. It resolves R's spatial receiver and current signed deformation index, then writes an identity matrix into that selected 0x40-byte element. It subsequently reloads the Game/manager/root/spatial chain, reads binary32 I+0x54C, supplies that same scalar to both scale inputs of 0x1028B0, and reloads the chain once more for the spatial receiver's virtual +0x8 call.

I+0x54C is exactly G+0x664 because I is inline at G+0x118. Interface construction initializes this scalar to binary32 one. The selected Game constructor later copies the caller's configuration block into G+0x1B88; the four bytes at input+0x44 become G+0x1BCC. After 0xF9AC0 returns, those four bytes are copied to G+0x664. Thus the Interface's constructor-one value is not a proof that its ordinary current scale is one. This task does not inspect an installation configuration or establish every producer of the caller's block.

Scale helper 0x1028B0 operates on the supplied matrix. A scale operand equal to binary32 one suppresses that axis's multiplication. Otherwise it multiplies the first four elements by the first scalar and the next four by the second scalar, using separate binary32 operations. Unordered comparisons take the multiplication side. It leaves the other eight elements alone. There is no zero/positive/finite admission check in this helper.

The concrete spatial virtual +0x8 target is 0x3733F0. Its selected publication order is:

1. Replace the cached forward matrix at T+0x2A0 with identity.
2. Apply the current selected translation pair, then the current selected pivot pair, to the translation column using the current basis.
3. Read the selected rotation triple and call 0x2B360, 0x2B470 and 0x13050 in that order. Each selected rotation helper multiplies its angle by the original binary32 word 0x3C8EFA35, calls the imported cosine operation before the imported sine operation, and updates its selected matrix columns with separate products and sums/differences. Imported mathematical internals and floating control state are not supplied here.
4. Multiply by the current selected deformation matrix through 0x173E0.
5. Apply the negative of the current selected pivot pair, using the original binary32 minus-one constant.
6. Overwrite forward-matrix offsets T+0x2A8, +0x2B8 and +0x2D8 with zero and +0x2C8 with one.
7. Copy all sixteen forward elements to T+0x2E0 and call 0xDFC30 with that latter matrix as the destination/input. Finally set byte T+0x49 to one.

The forward cache and the second cache therefore have distinct ownership and update order. Helper 0xDFC30 attempts a four-pivot elimination/inversion in a temporary workspace. Zero pivot checks can return failure before publishing the sixteen destination elements. On that selected failure route, the second cache still contains the forward matrix just copied into it. The caller does not branch on the inversion result before setting T+0x49. It is consequently incorrect to assume a valid inverse for every arbitrary deformation or zero scale. This contract checks the whole native owner but does not replace its ordered elimination with a universal exact-real inverse rule, or promise identical behavior for unordered operands and every floating environment.

The multiply helper 0x173E0 is an in-place sixteen-element binary32 multiplication. It retains each four-element source row while publishing that row's four results, in element order 0/4/8/12, then 1/5/9/13, then 2/6/10/14, then 3/7/11/15. Each result is assembled from four products and three separate additions; no fused operation is present in the selected original instructions. Identity simplifications used in the conditional examples below are assumptions about those example inputs, not permission to replace the original arithmetic for arbitrary inputs.

After rebuilding R's spatial cache, 0x101BD0 performs a sequence of other node placements and geometry queries. The Help placement is later in that same owner, at callsite 0x102303. It follows the selected placement for I+0x270 and precedes the placement for I+0x1B0. The actual native arguments to 0x334700 are important because the C presentation omits some of them:

| Input to 0x334700 for Help | Actual source and width |
|---|---|
| Node receiver | Reload current H from I+0x278, then N from H+0x178 |
| First point component | Binary32 zero |
| Second point component | Signed 32-bit invocation height converted to binary32 |
| First additional component | Binary32 20, carried from original word 0x41A00000 |
| Second additional component | Binary32 -250, original word 0xC37A0000 read at this placement |

This is not a direct store of the pair zero/-250, nor a horizontal half-width placement. The width argument does not enter this particular four-component Help call. Other operations earlier in the layout invocation use width and geometry/provider returns; their effects on other nodes are not assigned to N.

The complete 0x334700 wrapper forms two two-component binary32 records, retaining the first point separately from the additional pair, and forwards them with the unchanged node receiver to 0x334510. It does not consume the fifth stack argument as a node receiver, despite the misleading reduced C signature.

### Exact selected position computation

Helper 0x334510 first retains the incoming point's two components, then reads N+0x40. With no parent, those retained components pass directly to its later offset step. With a parent, it obtains that parent's cached second matrix at parent spatial+0x2E0, or identity when the parent's spatial receiver is null. If that parent itself has a parent, 0x100EE0 recursively assembles the ancestor second matrices and 0x173E0 combines them with the local second matrix. These are the actual cached operands; no check proves they are successful inverses at this use.

For the parent-matrix point transform, the native order is bounded precisely as follows. The divisor accumulator starts with the incoming first component times matrix element 3, adds the incoming second component times element 7, then adds element 15. A separate binary32 division forms one divided by that accumulator. The transformed first-component numerator first adds the second component times element 4 to the first component times element 0, then adds element 12. The second-component numerator first adds the first component times element 1 to the second component times element 5, then adds element 13. Each numerator is multiplied by the reciprocal. The products, additions, division and final multiplications are separate scalar binary32 instructions.

Next the helper reloads N+0x60, adds the supplied additional first/second components separately, and subtracts the currently selected pivot first/second components separately. The pivot index at T+0x244 and translation index at T+0xAC are signed DWORDs extended to 64 bits for their eight-byte element indexing. The resulting pair is compared with the current selected translation entry, first component before second. An unordered comparison or an unequal component causes both candidate components to be stored, first then second; the spatial receiver is reloaded from N+0x60 and its virtual +0x8 rebuild is called. Equal components, including signed zero equality, suppress both stores and that rebuild. There is no array-index validation or geometric clamp in this selected position helper.

The fixed layout inputs do not bypass parent transforms, pivot subtraction or the update comparison. In particular, P's current spatial state and every current ancestor cache can affect the stored translation even though P was initially fresh. No renderer, font, selection or current-text query is part of this helper's point computation. The subsequent rebuild reaches the original angle providers as described above.

Forward helper 0x100FA0 recursively initializes identity at the top and accumulates ancestor forward matrices before each node's own T+0x2A0. Its inverse-direction counterpart 0x100EE0 begins with the node's cached T+0x2E0 and accumulates the ancestor cached second matrices afterward. These helpers, and the complete local-matrix query chain 0x32C0C0/0x32CAE0/0x32CD10, distinguish forward drawing state from the cached operands used to place a point through its parents.

## Resize, scale-input and selected publication routes

The selected resolved direct-call census for layout 0x101BD0 contains two sites: Game key owner 0x96E70 at 0x96F7D, and 0xA2720 at 0xA2734. The latter stores its supplied DWORD dimensions at G+0x1D60 and G+0x1D64 before forwarding the unchanged argument registers with receiver I. This establishes actual dimension ownership, rather than treating an arbitrary helper's inputs as current screen dimensions.

The only resolved direct caller of 0xA2720 in the selected original call metadata is 0x135310 at 0x135576. This complete resize owner first requires the process receiver at RVA 0x551A98 to be nonnull. It calls the imported GetClientRect with the current window handle. The selected right and bottom signed DWORD outputs are the candidate dimensions; left and top are not subtracted in this owner. Its return code is not tested. A temporary DWORD is initialized to zero, and, when the selected graphics receivers exist, a virtual +0x58 query can replace it. A nonzero result selects the signed DWORDs at process RVAs 0x551BF4 and 0x551BF8 instead of those rectangle outputs. Failed or unsupported provider behavior is not supplied here.

There are several imported/virtual resource and viewport operations before this owner reaches the GUI portion. If current M at process RVA 0x551AC0 is nonnull, 0x349940 is called with origin components zero, the selected width/height and a true notification byte. Only afterward, if current G at RVA 0x551A80 is nonnull, 0xA2720 stores Game dimensions and invokes 0x101BD0. The GUI viewport notification therefore precedes this explicit Help layout call on that route. No positivity or finite-range check on the selected integer dimensions is added by these selected owners.

In 0x349940 the new manager viewport operands are written before notifications. The code stores the height at M+0x1C8 and the origin/width operands at +0x1BC/+0x1C0/+0x1C4. With its notification byte nonzero, it walks the current registered widget list at M+0xD0. For each current payload it invokes virtual +0x40, then queries callback key one through 0x32A210 and invokes a nonnull returned callback object. The widget registration helper protects insertion with M+0x90; this notification owner does not itself enter that critical section around its traversal or callback calls. The traversal and calls are sequential native operations, not a guarantee that callbacks cannot mutate other state or change collection lifetime.

For H, primary-table slot +0x40 resolves to 0x32BFD0. That helper starts at N's parent and follows parent links, looking for a nonnull node+0x70 widget binding. If it finds one, it skips H's virtual +0x48 call. Otherwise it builds a zero first pair and a second pair from current manager width/height minus cached H+0x160/+0x164, and invokes H's +0x48 target 0x32BD70. The subtractions use native 32-bit operands before signed conversion to binary32; no initial zero cache is assumed. After that conditional call returns, 0x32BFD0 copies the current manager width and height to those two cached DWORDs. It therefore rereads the manager operands after the callback boundary.

The complete 0x32BD70 first returns when N has no parent. Otherwise its position-changing branches depend on DWORDs H+0x158 and H+0x154 being one or two, with separate flag-bit exclusions at H+0x150. Other branches can change size when bits 2/3 of H+0x150 are set, and invoke/update subordinate receivers when H+0x168 or H+0x170 is nonnull. The selected fresh Help construction has zero in both alignment-selector DWORDs, zero flags, and null subordinate pointers. With those values retained, none of those branches writes Help position or size. The remaining matrix query uses 0x32C0C0 and its selected original query closure; it computes temporary matrices rather than assigning N's selected translation. This distinguishes a resize notification from the explicit 0x101BD0 placement. Changed selector/flag/subordinate values admit alternate behavior and are not assigned the fresh Help result.

The Help callback map starts empty. The selected 0x32A210 lookup uses a signed DWORD key, and a miss constructs a node whose eight-byte callback payload is null through 0xF6CC0 before insertion. Consequently a fresh key-one miss does not manufacture a callable Help layout handler. The selected Help setup window contains no callback registration for H. Later writers or registrations through arbitrary aliases are not globally excluded by this task.

The resolved direct calls to 0x135310 are window procedure 0x133C70 at 0x133CD8 and process/frame owner 0x134420 at 0x13469B. In the complete window procedure, message DWORD five calls 0x135310 and returns one. The selected message route does not derive dimensions from that message's packed arguments. This states original branch values without assuming when the operating-system provider will deliver them.

In complete owner 0x134420, the outer admission requires nonnull process graphics/Game receivers. It calls the ordinary Game update before its later display-state comparison. Six selected current Game DWORDs are compared with corresponding process DWORDs: G+0x1B8C/0x551BF4, G+0x1B90/0x551BF8, G+0x1B94/0x551BFC, G+0x1B88/0x551BF0, G+0x1B98/0x551C00 and G+0x1BC4/0x551C2C. The changed-state route additionally requires the selected graphics receivers, copies the current configuration block to the process block, invokes a virtual provider operation, calls 0x135310, and then invokes the next provider operation. These are selected direct caller facts; this contract does not close the display provider, frame pacing, or the full startup/message schedule.

### Scale and visibility through the selected key owner

The complete Game key owner 0x96E70 receives an unsigned byte code and first stores byte one at G+0x1A87 plus that code. Its scale branches have no prior Help/root/focus admission within that owner. For ordinary finite operands, byte code 0x71 divides current G+0x664 by binary32 1.25 and limits it at the lower binary32 value represented by 0x3ED1B716. Byte code 0x72 multiplies by 1.25 and limits it at the upper binary32 value represented by 0x40435000. These constants are approximately 0.4095999598503113 and exactly 3.0517578125. No universal unordered-input rule for this separate key owner is inferred from the finite description.

Both selected scale routes call 0x101BD0 with I and the current Game dimension DWORDs. After the call returns, they copy the then-current G+0x664 word to G+0x1BCC and call 0x13B1D0. Thus the persisted word is read after layout's called helpers, rather than guaranteed to remain the initially calculated scale candidate. The transitive configuration/persistence helper is not closed here.

Byte code 0x70 in the same owner toggles the selected visibility DWORD on the current H's N: zero becomes one; any nonzero value becomes zero. It does not run a Help placement or text draw. Code 0x73 similarly toggles the selected visibility on current R. The selected window-procedure key-message route calls the GUI key entry before it calls the Game key entry, reading current process receiver pointers for those calls; GUI callback processing is an earlier boundary, not a guarantee that the Game branch sees the pre-message state.

The selected ordinary Interface-update window at 0x1066C7..0x10671F queries the selected visibility entries of I's nodes at offsets 0, +0x18, +0x58, +0x80 and +0xE8, in that short-circuit order. It publishes P visibility one only when all five are zero; the first nonzero result selects zero. These reads use the complete 0x9AF00 helper, which tests the current node display receiver's selected visibility DWORD. They do not themselves perform an ancestor-composite visibility query. This window is a visibility publication, not a producer of P's translation or N's text origin. A nonzero N visibility can still fail to reach draw when P or R prevents traversal.

No static ordering above proves that a post-construction size notification has occurred before the first possible Help draw. The selected initial setup creates/attaches the node and hides it; the verified layout routes establish its later position when actually called. Imported window delivery, earlier/later indirect layout calls and other geometry writers are not converted into a first-frame or wall-clock guarantee.

## Ordinary draw join and parent HUD transforms

The selected ordinary Game body has a current-I join at 0xAAF6C and an Interface update call at 0xAB5CE. Its later renderer call is at 0xAD9D4, using G+0x18. When both selected sites are reached on that path, the Interface publication precedes this renderer call. The large Game owner has a timeout C-export record, so only the byte-verified native windows in the manifest support these caller claims. They do not constitute a complete contract for World ticking, all Game early exits or every renderer admission.

The concrete renderer constructor 0xC3150 installs primary table 0x4385E8 and stores the same startup M at renderer+0x540. In the selected late 0xE2E00 renderer window, its current primary virtual +0x10 is invoked. That selected concrete table slot resolves to the complete raw thunk 0xC4AF0: it reads renderer+0x540 and transfers to 0x33E170 with DWORD flags zero. The preceding renderer work, dynamic receiver replacement, and alternate renderer tables remain outside this bounded join.

The complete manager root-draw owner 0x33E170 first invokes M's virtual +0xA0, conditionally sizes its separate receiver at M+0x68, clears selected count/index DWORDs, draws the node at M+0x140 through 0x330B20 with the supplied flags, then invokes virtual +0xA8. For the concrete M, +0xA0 is the native leaf at 0x38EA80 and +0xA8 is the three-byte return target 0xE8A20.

The raw 0x38EA80 body explicitly replaces M+0x290..+0x2CF with a sixteen-element binary32 identity matrix. It separately prepares the matrix at M+0x2D0..+0x30F from the current signed viewport width/height converted to binary32. Its first diagonal component is binary32 two divided by width; its second is two divided by zero minus height. Its translation operands are width plus zero divided by zero minus width, and height plus zero divided by height, in their original ordered operations. The third diagonal and third translation components have raw DWORD values 0xB3D6BF93 and 0x33D6BF93 respectively; the final element is one, and the remaining components are explicitly zero. These are original matrix operands, not a promise of an operating-system screen origin, pixel convention, clipping volume or imported graphics execution. Zero/negative dimensions and floating exceptions are not rejected in this leaf.

### Node traversal and matrix lifetime

Complete node-draw owner 0x330B20 first tests the node's selected visibility DWORD and its own +0x80 flag bit 2. Visibility zero or that bit set returns zero before a node matrix push or widget draw. Therefore R and P can prevent traversal from reaching N even when N's own visibility is nonzero.

After admission, the concrete manager virtual +0xB0 is 0x38EB90. It allocates/copies the current sixteen elements of M+0x290 into a linked matrix-stack entry at M+0x320, checks/increments the eight-byte count at +0x328, publishes the links, then calls 0x173E0 to multiply the current M matrix by the admitted node's forward matrix at node spatial+0x2A0. Copy helper 0x38E0B0 preserves all sixteen input words. The maximum-count/error route is not treated as a successful push or a proved unwind cleanup.

For the ordinary root chain, this push/multiply is reached for R, then P, then N as their respective child-list traversals admit them. Their cached forward matrices, in that order, affect M's current matrix when Help text is called. This is a hierarchical transform. It is not a rule that every node's local coordinates are already window coordinates.

The selected fresh nodes have null node+0x58 receivers, no node cache request bit 0x400, initial clipping zero and blur radius zero. H's relevant flags are zero. On that unchanged selected path, the Help primary +0x8 draw target 0x278040 is invoked after N's child traversal at callsite 0x33160C. With the fresh empty child list there is no child draw before it. The alternative H flag 0x200 can move a widget draw before children at 0x331529; it is not the fresh Help value. Other branches of the complete traversal admit clipping, offscreen caches, blur and target operations based on node/widget/manager state and supplied flags. Their transitive geometry and graphics/provider behavior are expressly not collapsed into the fresh Help path.

At the ordinary common tail, manager virtual +0xB8 reaches raw leaf 0x38FA40. A zero stack count returns without restoring anything. Otherwise it copies the last saved sixteen matrix elements back to M+0x290, unlinks that stack node, decrements the eight-byte count and tail-transfers to the imported deallocation target. Thus returning from a child's normal admitted draw restores the parent's saved matrix. The tail is a native sequence, not an assertion that arbitrary callbacks, provider failures or invalid lifetimes cannot disrupt it.

The first selected Help text request really supplies binary32 zero for both row arguments. It does so while the current manager matrix can contain R/P/N transforms. The later text helper reads that matrix; it does not receive a newly invented absolute-zero screen origin.

At the beginning of 0x278040, Help also assembles an automatic matrix by accumulating N's parent forward chain through 0x100FA0, or identity when there is no parent, and multiplying N's own current forward cache when present. That calculation does not pass a point or this automatic matrix to the first 0x269210 call. In particular, it does not replace the concrete manager's current-matrix query in 0x269210. The ordered text calls below use the manager state in effect when they are actually reached.

## Help text style and local row arguments

On each admitted Help draw, 0x278040 first uses the selected style setters to store binary32 12 at H+0x1B0, binary32 3 at H+0x1B4, and four words zero/zero/zero/one at H+0x1D4..+0x1E0. It subsequently changes the primary four-component value at H+0x1C4..+0x1D0 between binding, description and annotation text calls. The first primary value is one-quarter/one/one/one. These draw-time writes persist in H until another selected writer changes them; they are not position stores into N.

All text calls in this selected Help owner use local binary32 row arguments. The first binding text at 0x27819A uses first component zero and second component zero. Its next description uses first component 180 and second component zero. The regular second row uses second component 20, the third 40 and the next 60. The regular binding inputs mostly use first component zero, descriptions use 180, and most annotations use 320. The selected second-row HOLD branch instead supplies 180 for its binding input; its alternate branch supplies zero. These constants were checked in the native argument registers and stack operands, rather than read from an incomplete C signature.

The text content providers and current actor/category predicates can add or omit annotations and the extra row at second component 80. The final row uses the retained signed integer row value plus 20: ordinarily its second component is 80 when no extra row is produced, or 100 after the selected extra row. One annotation branch truncates a returned first component from 0x269210 to a signed 32-bit integer, adds 20 in native DWORD arithmetic, converts it to binary32 and uses it as that annotation's first component. That is text-flow placement within a row, not a mutation of N's translation.

The complete 0x278040 body and its numeric remap/destination data are included in correspondence. Its binding-name and description providers, current World/player lookups, imported thread-initialization operations, string allocation and detailed content dictionaries remain named call boundaries for this placement contract. The text helpers receive those current returned strings; no external language/resource description or concrete glyph metrics are supplied by this artifact. The placement does not obtain its root translation from a World coordinate.

## Text helper: supplied point, current matrix and returned flow position

The complete owner 0x269210 retains H, its output pair pointer, string receiver, binary32 first argument and fifth-stack binary32 second argument. If current H+0x1A8 is null, it stores one zero QWORD to the output pair and returns. It makes none of the selected manager/font calls on that branch.

With a nonnull resource, the first call is M virtual +0x88 with the current H+0x178 node. For the concrete ordinary table, that is 0x38EC30. The complete native owner performs its manager/render preparation calls and conditional display-clipping work; its first concrete virtual +0x80 target is 0x38E800. Those selected owners reach graphics/provider calls. This preparation is a boundary before the following matrix snapshot, not an assignment of a zero Help origin. Its transitive resource, clipping and graphics execution is not inferred from virtual-slot names.

Next 0x269210 invokes current M virtual +0x70 into a sixteen-element automatic matrix. The concrete target is the raw leaf 0x38FB60. It copies all sixty-four bytes from M+0x290. The helper then reads current H+0x1B4, current H+0x178, that node's spatial receiver, its signed pivot index and its pivot pair. It adds each supplied local row argument to the corresponding pivot component, then multiplies both sums separately by element zero of the saved matrix. The same first matrix element is used for both components. The helper does not substitute the length of a matrix column or an independent vertical scale.

These two scaled components are the actual point inputs to 0x34FA60. The saved full matrix also remains current manager state for the font route described next. Therefore a first-row point of zero/zero does not mean the parent's matrix translation has been removed. Nonzero pivot or later/current parent state can also affect the helper inputs.

The remaining selected argument ownership is:

| Font-route input | Source at the selected call |
|---|---|
| Resource receiver | Current eight-byte H+0x1A8 |
| String | The retained string receiver supplied to 0x269210 |
| Point | The two pivot-adjusted, matrix-element-zero-scaled values |
| Requested scalar | H+0x1B0, normally 12 after Help draw's setter |
| Extra scalar | Captured H+0x1B4 for the first pass, or zero on the second pass |
| Other scalar/style words | Current H+0x1B8, +0x1BC and +0x1C0 |
| Primary vector | H+0x1C4, except the first positive-extra pass supplies a zero temporary vector |
| Second vector | H+0x1D4 |
| Third vector | A zero-initialized temporary four-component vector |
| Flags and final admission | DWORD H+0x1E4 and byte one |

The branch on the captured extra scalar is a native comparison with zero followed by the below-or-equal destination. Positive ordered values take two calls. First the captured positive scalar is passed with a zero primary vector; its returned two-component result is retained. Then a second call passes zero for that scalar and current H's primary vector. The second call's returned pair is ignored for 0x269210's return value. Style words, flags and resource operands are reread at the selected second-call preparation; it is not a frozen object snapshot. Nonpositive values, and unordered comparisons under the selected comparison flags, take one call with the captured scalar and current primary vector. The Help draw's literal three takes the two-call route when unchanged.

After the font call or calls, current M virtual +0x90 is called with current H+0x178. Its concrete target is 0xE8A20, which returns without stores. The helper nevertheless then reloads H+0x178, its spatial receiver, pivot index and pivot array. It divides each retained font result component once by the original saved matrix element zero. It first stores the two divided components to the output pair, then subtracts the now-current pivot components and stores the two adjusted results. The native sequence performs two divisions, not a second pair of divisions suggested by repeated C expressions. It does not reload a new matrix scale for this return conversion.

No zero-scale, finite-result, pointer-lifetime or index-domain guard is added on this reached branch. A changed pivot between the input and output reads can change the return conversion. A zero matrix element admits division by zero. The output is a text-flow position derived from the selected provider result; it is not a stored node position or proof of a glyph rectangle.

### Font matrix and glyph boundaries

The complete 0x34FA60 locks F+0x168, snapshots the current manager matrix through +0x70, and calls 0x354600 with the current requested/style operands, a pointer to that snapshot and the true byte supplied by 0x269210. It later restores the original sixty-four-byte snapshot through manager +0x78 and leaves the critical section. The concrete +0x78 target is raw leaf 0x390190, a sixty-four-byte copy into M+0x290. The font critical section protects this selected font owner; it does not prove a lock shared by every manager/node/resize writer.

When F+0x148 is null and the true-byte route is retained, 0x354600 reaches 0x354CF0. That complete helper derives two column lengths from the supplied binary32 matrix through ordered products/additions and square-root operations. It scales them by the requested size and quantizes through multiplication by 64, addition of one-half, signed 32-bit truncating conversion and multiplication by 1/64. It next applies the selected maximum 1000 to each size. Before applying the minimum-one size clamps, it derives the extra scalar from the mean of the two size/request ratios multiplied by the incoming extra scalar, and derives the two spacing values from their corresponding size/request ratios multiplied by their incoming spacing scalars. It then applies those minimum-one size clamps and finally caps the derived extra scalar at 500. These are separate ordered binary32 operations. These scalar limits are not a promise of font availability or a universal result for unsupported/nonfinite operands.

The reached 0x3541A0 admits its size/extra values through its original positive/nonnegative tests, stores the current size/extra fields at F+0x130/+0x134/+0x138, calls imported font operations, initializes F+0xC0's sixteen-element matrix to identity and sets byte F+0x160 to one. It chooses or creates selected glyph-cache records keyed by the quantized numeric values and supplies the extra-value provider where admitted. The imported operations' return values do not constitute a closed success contract here. If its admission fails, current prior font state can remain; the task does not invent new defaults for that case.

The 0x34FA60 null-F+0x148/true-byte route then takes its saved current matrix, scales the first four elements by requested size divided by F+0x130 and the next four by requested size divided by F+0x134. It changes each of its first two translation components by adding binary32 one-half, truncating to a signed 32-bit integer, and converting that integer back to binary32. It supplies this matrix through current M's +0x78. For finite representable negative inputs this truncation is toward zero after the addition, not a general floor or symmetric nearest-integer rule. No instruction here guarantees valid integer conversion for all NaNs, infinities or out-of-range values.

With nonnull F+0x148, 0x354600 instead assigns the requested/style scalar fields directly and clears F+0x160; 0x34FA60 bypasses that null-F+0x148 matrix-adjustment block. The alternative false-byte route also has a distinct matrix construction, but the selected 0x269210 supplies true. The value of F+0x148 for resource1.dat is not established without its uninspected load/provider state, so neither font branch is declared universally ordinary merely from the resource filename.

After a concrete manager +0x98 call, which is the selected return-only target, 0x34FA60 chooses its text kernel. When flag bit 0x10 is clear it calls 0x34FE40 directly. A set bit copies/wraps the string through 0x354F20 before that kernel; the wrapping helper is an explicit unclosed transitive boundary in this placement task. H+0x1E4 starts at zero and the selected Help draw does not change it, so unchanged selected Help state takes the direct kernel route.

The complete kernel 0x34FE40 obtains its initial bounds/pen adjustments from repeated 0x350EB0 queries, then iterates the string as sixteen-bit code units using the selected stored length. Code unit ten takes its line-break continuation; other units select current glyph/provider records. Current F+0x148, F+0x160, style fields, glyph existence and provider results decide the detailed glyph route. A missing glyph can skip a glyph draw while later advancement/kerning work remains distinct. The image-backed route projects the current pen with F+0xC0, uses its original reciprocal/homogeneous arithmetic and truncating integer conversions, then calls manager virtual +0x28 with the computed geometry and current vector inputs. The nested-node glyph route temporarily translates/scales the current manager matrix by the current pen, glyph fields and selected scalar ratio, draws that glyph node through 0x330B20, and restores its saved matrix. Imported glyph/kerning operations and optional-receiver/graphics providers remain boundaries.

This identifies the matrices and point operands that actually reach text. It does not turn the two-dimensional flow return into an observed pixel baseline, assign a concrete font bearing/advance/kerning, or claim that resource text cannot contain a line break. Language/description providers can change returned text and thereby its metric-dependent continuation. No external font, resource, rendering capture or native runtime observation was used.

## Independent conditional references

These examples use literal selected inputs and stated provider/cache assumptions. They are mathematical/source references, not observations of a running original and not a numerical closure of imported providers.

| Conditional selected state or event | Directly determined result |
|---|---|
| Fresh unchanged P forward/second caches are identity; root forward is uniform binary32 scale two with a valid half-scale second cache; N pivot is zero; layout height is signed DWORD 800; selected rotation-provider returns preserve those matrices; no intervening mutation | The explicit Help point maps to parent-local zero/400, adds 20/-250 and stores selected translation 20/150 when changed. The R/P/N forward drawing chain then has first two translation components 40/300 and first matrix element two. |
| Same conditions with height 1000 | The selected N translation becomes 20/250; the hierarchical manager matrix translation becomes 40/500. Width has not entered this particular Help point call. |
| First condition at the first Help text request | The supplied local arguments are zero/zero and the point passed to the font route is zero/zero. The current matrix still carries translation 40/300. A zero point did not reset or remove that matrix translation. |
| First condition at a description input of 180/zero | The font-route point is 360/zero before its own current-font metric/matrix behavior. This does not predict a complete glyph rectangle. |
| Null F+0x148, valid size configuration, saved manager linear scale two and translation 40/300, requested size 12 | The selected column-length quantization supplies size values 24/24. The font wrapper's corresponding size ratios are one-half, so its drawing matrix's first two linear columns become unit scale and its translation conversions retain 40/300. Actual glyph/provider operands remain additional inputs. |
| In 0x269210, saved matrix element zero is two, unchanged current pivot is 5/7, supplied row is 3/4, and the retained selected font result is conditionally 50/70 | The font-route point is 16/22. The returned relative flow pair is 20/28 after division and pivot subtraction, irrespective of the ignored second-pass result on a positive-extra two-call route. |
| A selected candidate translation compares equal to its retained pair, including a positive-zero/negative-zero equality | The placement helper makes no pair store and no spatial rebuild call for that comparison. An unordered comparison instead reaches the store/rebuild branch. |
| H's selected resource pointer is null | 0x269210 returns the two-zero output pair without its manager preparation, current-matrix query or font calls. Help draw's other reached providers/branches are not thereby declared absent. |

For nonuniform, rotated, singular, unordered or concurrently changed matrices, the exact original operands and ordered operations above apply. None of the uniform example simplifications establishes every allowed matrix, first-frame ordering, font branch or final provider output.

## Coverage, correspondence method and limits

The standalone docs/CLEANROOM_AUTHOR.md policy was read first for this assignment. Original filenames were identified before content searches. The individually selected original instruction export, function/range/call metadata and C presentations were used only beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled; the original image path is stated above. No installation file, font/resource asset, SDK or provider implementation was opened.

The image's PE section headers map each selected RVA to raw file bytes. Every original instruction encoding in each complete owned range listed below was compared with the corresponding raw image bytes. Range checks require continuous instruction coverage from inclusive first through inclusive last byte. All listed complete-owner and selected caller-window checks matched, with no within-range gap. The PE exception directory at RVA 0x554000, length 188,928 bytes, was independently read as 15,744 runtime-function records. Its records are additional metadata, not a replacement for logical original-owner ranges: some begin-at-entry records describe a shorter unwind fragment, and some include trailing switch data/padding beyond the terminating instruction range. Such differences are retained explicitly.

For Help draw, the exported instruction range terminates with a return at 0x279A72. A padding byte at 0x279A73, the five-DWORD destination table at 0x279A74 and the 103-byte numeric remap at 0x279A88 account for the trailing selected PE extent. Each destination was matched to an original instruction and each remap value was checked to select one of the five entries. These data bytes are not silently counted as code instructions.

The six raw leaf/thunk targets below lack a function-owned C export in the selected metadata. Their complete instruction spans are present in the original instruction export with a blank function owner. They were read directly, byte-matched continuously to the supported image, and bounded by their actual return/tail transfer and the following distinct instruction/padding boundary. They were not invented from a neighboring C function or inferred merely from a virtual-slot name.

All complete C presentations use the original functions directory and the preferred-base entry address as filename. For example, RVA 0x277EC0 corresponds to functions/140277ec0.c and RVA 0x101BD0 to functions/140101bd0.c. The native manifests distinguish complete body correspondence from the specific behavioral purpose for which a body is used. Complete byte coverage of supporting owners is not a claim that every transitive callee, renderer, font backend or GUI asset they reach has been specified here.

The following large owners are expressly bounded: Game constructor 0x7EA80; Interface setup 0xF9AC0; Interface update 0x103200; process startup 0x1355A0; graphics/manager setup 0x134CC0; renderer constructor 0xC3150; renderer owner 0xE2E00; and ordinary Game owner 0xA5A50. Only the caller windows below and selected scoped original C evidence support claims about them. The last of these has a timeout C-export record; no successful whole-Game C body was inferred from it. This contract does not claim all their branch/lifetime/World-update effects.

The resolved direct-call census used for selected admissions is original calls.csv, reconciled with the selected original call instructions. It proves the listed concrete sites for Help construction, explicit layout, resize and the ordinary constructor/render joins. It does not enumerate every computed/indirect caller, alias-based writer, callback replacement or dynamically selected receiver. Numeric-offset searches alone were not treated as receiver provenance.

Current-pointer rereads, provider calls and selected critical sections are preserved above. No inspected owner establishes a universal GUI/message thread, a wall-clock interleaving, callback non-reentry, atomic resize/draw transition, or ownership retained through arbitrary teardown. Matrix-stack operations, the font critical section and the manager widget-registration critical section have different selected scopes. This task does not establish that every participant acquires one shared lock. No concrete Game/Interface/Help destructor census or complete shutdown schedule is claimed.

Valid live receivers, intact native containers and completed selected calls are preconditions of the ordinary paths. Allocation failures, invalid indices/pointers, short or unsuccessful imported providers, singular matrices, nonfinite values, integer conversion exceptions and floating environment changes are not given invented fallback states. In particular the inverse attempt can fail, the text helper can divide by zero, and cached viewport defaults not explicitly stored are not assumed zero. Finite conditional examples do not extend to all such inputs.

### Complete original-owner manifest

Ranges below are inclusive exported owned ranges. Every listed C body was read completely, every listed owned instruction encoding was checked against the original image, and every owned range passed continuous coverage. Supporting geometry/content/registration leads are included for audit completeness even when their broader semantics are outside this placement contract.

| Entry RVA | Inclusive owned RVA ranges | Instructions | Code bytes |
|---|---|---:|---:|
| 0x13050 | 0x13050..0x13156 | 65 | 263 |
| 0x173E0 | 0x173E0..0x17727 | 200 | 840 |
| 0x2B360 | 0x2B360..0x2B468 | 65 | 265 |
| 0x2B470 | 0x2B470..0x2B573 | 64 | 260 |
| 0x96E70 | 0x96E70..0x977F2 | 550 | 2435 |
| 0x9AF00 | 0x9AF00..0x9AF19 | 6 | 26 |
| 0xA2720 | 0xA2720..0xA2738 | 4 | 25 |
| 0xA3A20 | 0xA3A20..0xA3A3F | 8 | 32 |
| 0xDFC30 | 0xDFC30..0xE062F | 576 | 2560 |
| 0xE8A20 | 0xE8A20..0xE8A22 | 1 | 3 |
| 0xF6CC0 | 0xF6CC0..0xF6D04 | 18 | 69 |
| 0xF8690 | 0xF8690..0xF87C9 | 67 | 314 |
| 0xF97E0 | 0xF97E0..0xF9990 | 96 | 433 |
| 0x100EE0 | 0x100EE0..0x100F95 | 43 | 182 |
| 0x100FA0 | 0x100FA0..0x101017 | 31 | 120 |
| 0x101BD0 | 0x101BD0..0x1028A8 | 665 | 3289 |
| 0x1028B0 | 0x1028B0..0x102926 | 30 | 119 |
| 0x102930 | 0x102930..0x102DA5 | 250 | 1142 |
| 0x133C70 | 0x133C70..0x133EEB | 174 | 636 |
| 0x134420 | 0x134420..0x1347C5 | 192 | 934 |
| 0x135310 | 0x135310..0x13559F | 150 | 656 |
| 0x268A70 | 0x268A70..0x268B39 | 36 | 202 |
| 0x269130 | 0x269130..0x269152 | 10 | 35 |
| 0x269160 | 0x269160..0x269183 | 9 | 36 |
| 0x2691B0 | 0x2691B0..0x2691B8 | 2 | 9 |
| 0x2691C0 | 0x2691C0..0x2691E3 | 9 | 36 |
| 0x2691F0 | 0x2691F0..0x2691F8 | 2 | 9 |
| 0x269210 | 0x269210..0x26958E | 158 | 895 |
| 0x277EC0 | 0x277EC0..0x277FF6 | 74 | 311 |
| 0x278040 | 0x278040..0x279A72 | 1494 | 6707 |
| 0x328F00 | 0x328F00..0x328F6F | 31 | 112 |
| 0x328FF0 | 0x328FF0..0x329206 | 96 | 535 |
| 0x3297F0 | 0x3297F0..0x32981B; 0x32981D..0x329842 | 21 | 82 |
| 0x32A210 | 0x32A210..0x32A2B4 | 46 | 165 |
| 0x32B6E0 | 0x32B6E0..0x32B820 | 79 | 321 |
| 0x32BD70 | 0x32BD70..0x32BFCC | 148 | 605 |
| 0x32BFD0 | 0x32BFD0..0x32C088 | 43 | 185 |
| 0x32C0C0 | 0x32C0C0..0x32C222 | 82 | 355 |
| 0x32CAE0 | 0x32CAE0..0x32CC4D | 78 | 366 |
| 0x32CD10 | 0x32CD10..0x32CE3A | 65 | 299 |
| 0x32D190 | 0x32D190..0x32D1A0 | 5 | 17 |
| 0x32EE60 | 0x32EE60..0x32F051 | 114 | 498 |
| 0x32F640 | 0x32F640..0x32F6D0 | 37 | 145 |
| 0x330B20 | 0x330B20..0x3318E7 | 825 | 3528 |
| 0x334410 | 0x334410..0x334506 | 60 | 247 |
| 0x334510 | 0x334510..0x3346FD | 113 | 494 |
| 0x334700 | 0x334700..0x334751 | 17 | 82 |
| 0x3356D0 | 0x3356D0..0x335773 | 42 | 164 |
| 0x335830 | 0x335830..0x3358D3 | 42 | 164 |
| 0x339B70 | 0x339B70..0x33A112 | 323 | 1443 |
| 0x33BFE0 | 0x33BFE0..0x33C09D | 42 | 190 |
| 0x33CD30 | 0x33CD30..0x33CED2 | 117 | 419 |
| 0x33E170 | 0x33E170..0x33E1D3 | 28 | 100 |
| 0x349940 | 0x349940..0x3499BD | 33 | 126 |
| 0x34A420 | 0x34A420..0x34A4C4 | 46 | 165 |
| 0x34C410 | 0x34C410..0x34C4BF | 46 | 176 |
| 0x34D060 | 0x34D060..0x34D354 | 204 | 757 |
| 0x34EBF0 | 0x34EBF0..0x34EDB8 | 91 | 457 |
| 0x34FA60 | 0x34FA60..0x34FE37 | 223 | 984 |
| 0x34FE40 | 0x34FE40..0x350BF3 | 749 | 3508 |
| 0x3541A0 | 0x3541A0..0x3545F6 | 258 | 1111 |
| 0x354600 | 0x354600..0x35473B | 57 | 316 |
| 0x354CF0 | 0x354CF0..0x354F15 | 114 | 550 |
| 0x3669C0 | 0x3669C0..0x366A01 | 18 | 66 |
| 0x368D90 | 0x368D90..0x368E15 | 35 | 134 |
| 0x36C230 | 0x36C230..0x36C277 | 19 | 72 |
| 0x36C280 | 0x36C280..0x36C2B1 | 15 | 50 |
| 0x36C2C0 | 0x36C2C0..0x36C2F8 | 17 | 57 |
| 0x371450 | 0x371450..0x37151F | 62 | 208 |
| 0x371520 | 0x371520..0x3715F0 | 62 | 209 |
| 0x371600 | 0x371600..0x371746 | 91 | 327 |
| 0x372BD0 | 0x372BD0..0x372CAA; 0x372CAC..0x372CBA; 0x372CBC..0x372CCA; 0x372CCC..0x372CD7; 0x372CD9..0x372D7B; 0x372D7D..0x372D8B; 0x372D8D..0x372D9B; 0x372D9D..0x372DA8; 0x372DAA..0x372E5B; 0x372E5D..0x372E6B; 0x372E6D..0x372E7B; 0x372E7D..0x372E88; 0x372E8A..0x372F34; 0x372F36..0x372F44; 0x372F46..0x372F54; 0x372F56..0x372F61; 0x372F63..0x3730FE | 301 | 1311 |
| 0x373310 | 0x373310..0x3733ED | 45 | 222 |
| 0x3733F0 | 0x3733F0..0x3736BE | 152 | 719 |
| 0x3736C0 | 0x3736C0..0x37379D; 0x37379F..0x3737AD; 0x3737AF..0x3737BD; 0x3737BF..0x3737CA; 0x3737CC..0x37386A; 0x37386C..0x37387A; 0x37387C..0x37388A; 0x37388C..0x373897; 0x373899..0x373948; 0x37394A..0x373958; 0x37395A..0x373968; 0x37396A..0x373975; 0x373977..0x373A1D; 0x373A1F..0x373A2D; 0x373A2F..0x373A3D; 0x373A3F..0x373A4A; 0x373A4C..0x373AF6; 0x373AF8..0x373B06; 0x373B08..0x373B16; 0x373B18..0x373B23; 0x373B25..0x373BE8 | 324 | 1301 |
| 0x373DE0 | 0x373DE0..0x373EE2 | 49 | 259 |
| 0x38E0B0 | 0x38E0B0..0x38E0F8 | 20 | 73 |
| 0x38E100 | 0x38E100..0x38E41B | 128 | 796 |
| 0x38E800 | 0x38E800..0x38EA71 | 130 | 626 |
| 0x38EB90 | 0x38EB90..0x38EC2A | 37 | 155 |
| 0x38EC30 | 0x38EC30..0x38ED09 | 58 | 218 |
| 0x38F020 | 0x38F020..0x38F0B4 | 36 | 149 |
| 0x38F0C0 | 0x38F0C0..0x38F3AB | 164 | 748 |

### Complete raw leaf/thunk targets

| Entry RVA | Inclusive raw instruction extent | Instructions | Code bytes | Selected role |
|---|---|---:|---:|---|
| 0x26A720 | 0x26A720..0x26A728 | 4 | 9 | Help primary +0x10/+0x18 zero-pair target |
| 0x38EA80 | 0x38EA80..0x38EB84 | 43 | 261 | initial manager matrix/projection preparation |
| 0x38FA40 | 0x38FA40..0x38FAB0 | 24 | 113 | manager matrix-stack pop |
| 0x38FB60 | 0x38FB60..0x38FB8B | 9 | 44 | manager current-matrix copy |
| 0x390190 | 0x390190..0x3901BB | 9 | 44 | manager current-matrix assignment |
| 0xC4AF0 | 0xC4AF0..0xC4AFD | 3 | 14 | renderer-to-manager root draw transfer |

### Bounded original caller windows

These windows do not imply whole-owner semantic closure.

| Original owning entry | Inclusive selected RVA range | Instructions | Code bytes | Purpose |
|---|---|---:|---:|---|
| 0x134CC0 | 0x1350FD..0x135131 | 12 | 53 | manager allocation/constructor/publication |
| 0x1355A0 | 0x1361F1..0x13621D | 9 | 45 | renderer manager argument |
| 0x1355A0 | 0x136319..0x136344 | 12 | 44 | Game manager argument and process publication |
| 0x7EA80 | 0x7EA80..0x7EB23 | 38 | 164 | Game fields and inline Interface construction |
| 0x7EA80 | 0x7ECC5..0x7ED0A | 13 | 70 | configuration word source for GUI scale |
| 0x7EA80 | 0x7F4C9..0x7F4E4 | 5 | 28 | Interface setup and GUI scale overwrite |
| 0xF9AC0 | 0xF9AC0..0xF9B43 | 25 | 132 | Interface receiver provenance |
| 0xF9AC0 | 0xF9C9A..0xF9D01 | 19 | 104 | parent node construction |
| 0xF9AC0 | 0xFF6F8..0xFF7A3 | 37 | 172 | Help construction/publication/initial visibility |
| 0xC3150 | 0xC3150..0xC31FA | 35 | 171 | renderer table and manager receiver |
| 0xE2E00 | 0xE2E00..0xE2E7C | 26 | 125 | renderer receiver provenance |
| 0xE2E00 | 0xE4F27..0xE4F37 | 5 | 17 | renderer GUI virtual dispatch |
| 0xA5A50 | 0xA5A50..0xA5AEF | 27 | 160 | ordinary Game receiver provenance |
| 0xA5A50 | 0xAAF6C..0xAAF72 | 1 | 7 | inline Interface receiver |
| 0xA5A50 | 0xAB5C7..0xAB5D2 | 3 | 12 | ordinary Interface publication call |
| 0xA5A50 | 0xAD9B3..0xAD9D8 | 7 | 38 | ordinary renderer receiver/call |
| 0x103200 | 0x103200..0x103281 | 27 | 130 | Interface update receiver provenance |
| 0x103200 | 0x1066C7..0x10671F | 25 | 89 | parent selected-visibility publication |

### Selected PE metadata correspondence

The exact-start PE record end below is exclusive. It need not equal the exported logical owner's final byte plus one. The full logical instruction ranges are those in the preceding complete-owner manifest.

| Entry RVA | Exact-start PE record end | Unwind-info RVA |
|---|---|---|
| 0x277EC0 | 0x277FF7 | 0x506604 |
| 0x278040 | 0x279AEF | 0x506650 |
| 0x269210 | 0x269269 | 0x504108 |
| 0x101BD0 | 0x1028A9 | 0x4D4ED8 |
| 0x330B20 | 0x330B82 | 0x512FF8 |
| 0x3733F0 | 0x3736BF | 0x527624 |
| 0x34FA60 | 0x34FE38 | 0x5193CC |
| 0x34FE40 | 0x34FE6E | 0x519338 |
| 0x135310 | 0x135346 | 0x4DD9A0 |
| 0x349940 | 0x349975 | 0x4CF678 |

Coverage totals are 83 complete exported owners, 120 exported owned ranges, 11,087 instructions and 49,209 code bytes; six complete raw leaf/thunk extents with 92 instructions and 485 code bytes; and eighteen bounded caller windows with 326 instructions and 1,561 code bytes. These sets do not overlap. The total is 11,505 original instruction encodings and 51,255 matched code bytes. Original data tables, literals, PE metadata and separately inspected padding are excluded from that code-byte total.

### Selected direct-call and Help text-call census

The following rows concern only the named selected targets and resolved metadata. Direct target displacements in these rows were independently decoded from original bytes and matched. A blank owner in the original export is retained as such. The target 0x278040 has no resolved direct call in this metadata; its concrete indirect draw route is established by the original Help primary table and node traversal above.

| Selected target | Resolved original owner and callsite |
|---|---|
| 0x277EC0 | 0xF9AC0 at 0xFF76A |
| 0x101BD0 | 0x96E70 at 0x96F7D; 0xA2720 at 0xA2734 |
| 0xA2720 | 0x135310 at 0x135576 |
| 0x135310 | 0x133C70 at 0x133CD8; 0x134420 at 0x13469B |
| 0xF8690 | 0x7EA80 at 0x7EB1E |
| 0xF9AC0 | 0x7EA80 at 0x7F4D4 |
| 0x38E100 | 0x134CC0 at 0x135121 |
| 0x33E170 | Blank exported owner at 0xC4AF9 |
| 0x278040 | No resolved direct-call row |

The complete Help draw owner contains 40 distinct selected callsites to 0x269210: 0x27819A, 0x27836C, 0x278492, 0x27853A, 0x2785C1, 0x2786BB, 0x2787E0, 0x27886B, 0x2788D1, 0x278961, 0x2789BF, 0x278A47, 0x278BA8, 0x278C28, 0x278CBC, 0x278D96, 0x278E4D, 0x278EB3, 0x278F2D, 0x279007, 0x279087, 0x279104, 0x279179, 0x2791D8, 0x27929D, 0x27934A, 0x2793AE, 0x279429, 0x2794D5, 0x279539, 0x2795B4, 0x27967F, 0x2796E3, 0x279761, 0x2797C5, 0x279840, 0x2798EC, 0x279952, 0x2799B6, 0x279A26. They are conditional instruction sites, not a claim that every site executes on every draw or that this is a census of all other users of the text helper.

### Exact selected original constants and literal operands

These numeric words are original data values, not instruction-byte transcriptions. The binary32 interpretation is supplied to make the coordinate/style contract readable.

| Data RVA | DWORD value | Binary32 value |
|---|---|---:|
| 0x42E580 | 0x40400000 | 3 |
| 0x42E59C | 0x41400000 | 12 |
| 0x42E5A0 | 0x41A00000 | 20 |
| 0x42E5AC | 0x42200000 | 40 |
| 0x42E5B8 | 0x42A00000 | 80 |
| 0x42E7B8 | 0x42700000 | 60 |
| 0x42E7D4 | 0x43340000 | 180 |
| 0x42E8AC | 0xC37A0000 | -250 |
| 0x441BD8 | 0x43A00000 | 320 |
| 0x42E71C | 0x3FA00000 | 1.25 |
| 0x437F80 | 0x3ED1B716 | 0.4095999598503113 |
| 0x437FD0 | 0x40435000 | 3.0517578125 |
| 0x42E520 | 0x3C8EFA35 | 0.01745329238474369 |
| 0x4AC964 | 0x3F000000 | 0.5 |
| 0x4AC980 | 0x3F800000 | 1 |
| 0x4ACA4C | 0xBF800000 | -1 |
| 0x4AC9D0 | 0x40000000 | 2 |
| 0x42E5B4 | 0x42800000 | 64 |
| 0x446140 | 0x3C800000 | 0.015625 |
| 0x42E5D4 | 0x447A0000 | 1000 |
| 0x42E5C8 | 0x43FA0000 | 500 |

The identity matrix at 0x42E630 has ones on its four diagonal elements and zero in the other twelve. The four-word vector at 0x42E660 is zero/zero/zero/one. The four-word vector at 0x42E8C0 is one/one/one/one. The primary Help vector at 0x46D970 is one-quarter/one/one/one. Annotation source vectors at 0x4381E0, 0x46EB50 and 0x46EB40 are copied into their respective process-local static storage by the selected guarded initialization in 0x278040; their exact data footprints are hashed below. Imported static-initialization/runtime guarantees and later unrelated writers are not supplied.

| Literal data RVA | Native argument site | Code-unit width | Length excluding terminator | Exact selected literal |
|---|---|---:|---:|---|
| 0x42FF38 | 0x277F0B | 16 | 0 | Empty string |
| 0x436340 | 0x277F96 | 16 | 13 | "resource1.dat" |
| 0x4795E8 | 0x339FB6 | 16 | 4 | "root" |
| 0x4795F8 | 0x33A01C | 16 | 7 | "sources" |
| 0x479608 | 0x33A090 | 16 | 5 | "fonts" |
| 0x47C108 | 0x372C35 | 8 | 11 | "translation" |
| 0x47C118 | 0x372D07 | 8 | 11 | "deformation" |
| 0x47C128 | 0x372DD8 | 8 | 8 | "rotation" |
| 0x47C134 | 0x372EB8 | 8 | 5 | "pivot" |

Each listed literal terminator was checked in the original image. Eight-bit property identifiers are distinguished from UTF-16 node/resource names. The literal inventory is limited to the receiver/property/resource joins stated here; description-provider strings and uninspected external resources are not substituted.

### Original export and selected data integrity

| Individually selected original export | Bytes | SHA256 |
|---|---:|---|
| assembly.txt | 62547291 | FE07326A0F4585432A9D4E42E7CF16C570815A8FCD164DE36616095023A0740C |
| function_ranges.csv | 509127 | BC127412C5A8F21D8BA78AA53D9EB35A16AF58C1A85906DA9FF31E968E379601 |
| calls.csv | 10840369 | CF410A7DD84BBEF0E9FC4FC6FD1FF3B4903D97E9126C8F1A1D69EF05CD566DCB |
| functions.csv | 1551545 | 940E3682B9CCC68458531AC8D948A20F8B9530CDFFF5E568C5517BA91365DCEC |

| Original data RVA | Bytes | Selected correspondence purpose | SHA256 of original bytes |
|---|---:|---|---|
| 0x42E630 | 64 | identity matrix | 5998E9D1DD9BF48AF6AF87A1EF8C7783B818AC8329637ADA08CC5FB329042376 |
| 0x46E8A8 | 344 | Help primary virtual table | 20A1C481B827C90E977F64ABC9416584E9AECE5FA581010A4649489E49213591 |
| 0x46EA08 | 16 | Help secondary virtual table | A70217BFA83C68441D415DC8F6CBEACF145F0EF31A4F537CA325EC0BDE466504 |
| 0x480408 | 232 | concrete manager virtual table | 4ACBD527DBFC4F09914714CA4B5C9766B3D8DF61F3823D9E147238EA75981FAE |
| 0x4385E8 | 24 | selected renderer primary slots | 4E93DFF6D2E6194C83D265B46FF4995B163E94A3F372724B18231973DFF9B053 |
| 0x47C0F0 | 24 | spatial property virtual slots | EA551D783908B9B83570F35047FB6CFF9CC9B3AA0AA89113EB4FF5ACFD7771D8 |
| 0x47C170 | 24 | display property virtual slots | C1966982F5634F1C88F955E79DCE1892B7042A59CE5A34DBAD4BF43312EF34D0 |
| 0x42E8C0 | 16 | four-one vector | F6BB1294DA2F78CD935B01C7656280DF5EAA0439E9D97BC03775825A41A508E4 |
| 0x46D970 | 16 | primary Help color | BB0FCEB0AB9A6883F2CE7496A81C9F9CA0E737267A2A980B40C86745C2AAC437 |
| 0x4381E0 | 16 | annotation color source one | 366E876C8CEC5DFE57664C117FA5F7A3FA0F14B0A527CB5E38FF685DB7B28F98 |
| 0x46EB50 | 16 | annotation color source two | 7B7ED1AC9971F04B20DEA26CFFAE1C79359D10A4002553333357F291E07D17E0 |
| 0x46EB40 | 16 | annotation color source three | 4EFFA29A2908DEADE0A3D35C5C76D2ACC37ED364839D908519D48FCC94CE5F25 |
| 0x4ACB10 | 8 | inverse pivot absolute-value mask | 6A69A6CC7473A16302890CD2A9E93E347281F6EA0E1BB784E589753BED0B3324 |
| 0x279A74 | 20 | five Help draw switch destinations | 84D046B70D1A96589E8EACAE639ABBAF55B340E6DC0DBF986292E789777F3B28 |
| 0x279A88 | 103 | Help draw numeric remap | CF3B7154E7857ADE18532FB662ACA33E09536A435CEADA590989C400E66A695C |

## Exposure and immutable handoff

Only the standalone source-author policy, this isolated assignment and individually selected supported original executable/export evidence were used. No active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, combined project history, replacement engine/exporter, tests/results, owner data/configuration/outputs/captures/assets, SDK/provider/dependency implementation, reconstruction source, installation configuration or earlier held handoff was opened for this assignment. Earlier held artifacts remained unread and unchanged. No original executable was run. No additional agent was spawned. No implementation/runtime verification feedback was requested or received.

This is a standalone code-free factual contract for the selected original placement/text-space scope. The saved artifact is subject to a whole-file readback, UTF-8/no-BOM checks, exact byte-length/SHA256 verification and supported-original identity recheck before the separate FINAL / STABLE / HELD report. All artifact writes stop at that hold. No replacement design, pseudocode, source transcription or implementation recommendation is supplied.
