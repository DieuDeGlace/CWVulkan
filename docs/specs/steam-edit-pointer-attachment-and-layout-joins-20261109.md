# Original Steam Edit pointer, attachment and layout contract

This is an independent original-source behavioral contract for the supported Windows x64 Steam image. It covers the complete concrete pointer press, motion, release, pick, caption attachment, clamp, layout and key owners named below, with the reached selection, callback, update, storage and lifetime dependencies needed to interpret their order. “Edit” identifies the concrete object using original table RVA 0x4775C8; it does not import a class definition or a generic operating-system edit-control contract.

The original authority is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe: 5,757,952 bytes, AMD64 machine 0x8664, image base 0x140000000, PE timestamp 0x5D93AB0D and entry RVA 0x3939F0. Its independently checked SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Addresses in this document are RVAs. The evidence procedure, complete selected native ranges and limitations are listed at the end.

## Receiver and storage identities

E denotes the concrete Edit object, U its eight-byte GUI pointer at E+8, and P its eight-byte attached node pointer at E+0x178. P+0x40 is the parent-node pointer, P+0x58 the node's caption-like object, P+0x60 its transform/indexed-position object, P+0x68 its indexed state object, P+0x70 its local widget and P+0x78 its current inherited/local widget association. The use of “caption” below is restricted to the object admitted by a 32-bit type value of three at that receiver's +0x70.

E+0x1A8 is an eight-byte cached caption pointer. E+0x1C4 is a signed 32-bit current text index, and E+0x1C8 a signed 32-bit selection displacement. The second selection endpoint is formed by their native 32-bit addition. Positive and negative displacements are both meaningful in the reached range handling. E+0x1B8 is a one-byte pointer-drag admission field. E+0x15C is a one-byte activity field written to one by several editing paths. Its untraced consumers are not assigned a timing or animation contract here.

E+0x1BC and E+0x1C0 hold the two four-byte position words sampled by attachment. Layout subsequently interprets the first as binary32. E+0x1B0 is a raw eight-byte optional character/text-filter receiver; it is distinct from the cached caption and selection fields.

For a current caption, a signed 32-bit index at caption+0xD4 selects a 0x20-byte string object from the array beginning at the eight-byte pointer caption+0x108. Each string object has a 64-bit code-unit length at +0x10 and 64-bit capacity at +0x18. A capacity below eight uses inline data; a capacity at least eight uses the eight-byte data pointer at the beginning. Code units occupy two bytes, and the reached copy/erase/insert owners preserve a two-byte zero terminator. The Edit index/clamp/loop comparisons use the signed low 32 bits of that length. The contract does not substitute grapheme, Unicode-scalar, glyph-count or byte-count indexing for those operands.

U+0x170 is the eight-byte selected-widget pointer returned by complete accessor 0x342090. U+0x168 is an eight-byte logical pointer-capture node used by the GUI's motion/hit-selection owner. Complete setter 0x33C410 merely assigns that field. Neither setter nor the selected release owner calls an operating-system capture import. U+0x180/+0x184 hold the current binary32 pointer coordinates; U+0x188/+0x18C retain the prior coordinate words during motion recalculation. The low bit of byte U+0x1A8 is an independent button-state admission read by Edit motion.

Key membership is an ordered tree at U+0x198, with 16-bit unsigned keys at node+0x1A and sentinel byte +0x19. Owner 0x9AEB0 and the inlined press search test exact membership of key 0x10. They do not sample an operating-system key-state provider. Calling that key “Shift” below identifies this concrete numeric modifier test.

## Construction, attachment refresh and lifetime

Complete allocation bridge 0x33CB30 requests 0x1D0 bytes and, on a nonnull result, invokes constructor 0x327440. After its base constructor 0x328FF0 returns, that constructor selects tables 0x4775C8 and 0x477728, stores 32-bit value two at E+0x50, clears the cached caption and optional filter pointers, and clears the two selection words with one eight-byte store at E+0x1C4. These selected stores do not initialize the entire object. In particular, these concrete constructor stores do not establish a zero initial drag byte or initial saved-position pair.

The selected clone/allocation owner 0x3274E0 constructs a new base through 0x329210, selects the same concrete tables, clears the cached caption, copies the source's raw filter pointer, clears the two selection words, and stores the same value two at +0x50. It does not clone a caption object into E+0x1A8 or establish a distinct filter lifetime. The base constructors register the widget in the GUI's +0xD0 list through 0x33BFE0 under GUI critical section +0x90. Their node binding and their derived-table installation are different chronological steps.

Complete binding owner 0x32D190, when supplied a nonnull node, writes the widget pointer to node+0x70, then writes the node pointer to widget+0x178. It does not itself execute the derived caption attachment. Complete node-widget replacement owner 0x334410 can first transfer selected old-widget geometry/state to the supplied widget, remove/delete the previous widget through 0x33E0C0, clear node+0x70, and finally bind the supplied nonnull widget through 0x32D190. The selected transfer helper 0x329C70 copies the reached scalar/matrix fields before its later helper boundary. Failure and null-receiver combinations beyond the reached valid replacement route are not normalized into a universal safe replacement operation.

The actual complete Edit attachment owner is 0x327AA0, bound at table offset +0x38. It first dereferences P and obtains P+0x58. It retains that pointer only when nonnull and its 32-bit +0x70 value is exactly three; otherwise it writes zero to E+0x1A8. It then calls clamp 0x328DF0. Next, if P, its parent and that parent's +0x58 are nonnull, it writes the 32-bit value one into the parent state object's +0x168 integer array at its signed +0x134 index. Finally it reads the current P+0x60 receiver, its signed +0xAC index and eight-byte +0xE0 array pointer, and copies the selected two four-byte position words, at an eight-byte element stride, to E+0x1BC and E+0x1C0, in that order.

Attachment does not call layout, pick a pointer index, set the drag byte, select the widget or assign logical capture. Its first P dereference precedes its later optional-parent checks. Its final indexed-position reads have no null/bounds check of their own. An absent caption is supported by the clamp's zeroing branch; an arbitrary absent P or indexed-position receiver is not thereby made valid.

Complete node-association traversal 0x333820 returns immediately for node flag bit two set in its 32-bit +0x80 field. Otherwise a nonnull local widget at +0x70 receives virtual +0x38 before node+0x78 is assigned from the then-current local widget pointer. If there is no local widget but a parent exists, +0x78 inherits parent+0x78. With neither local widget nor parent, this body leaves +0x78 unchanged. It then visits children in forward list order. A child's own admission is evaluated when recursion reaches it.

Complete GUI update owner 0x340D60 first calls that traversal on U+0x140. It then walks the registered widget list at U+0xD0 in forward order. Each entry receives virtual +0x38, followed by callback lookup for numeric event zero and invocation if the returned callback receiver is nonnull. This second loop contains no corresponding node/root visibility or widget-active gate around those calls. An Edit reached in both traversals can therefore refresh attachment twice in one normal call to this owner, and its saved-position pair is sampled anew each time. That static call order does not prove how often an external caller runs the owner.

The callback lookup at 0x32A210 searches the widget's ordered registry at +0x188 using signed 32-bit event identifiers. A missing identifier reaches the selected node allocation/insertion closure 0xF6CC0/0xF7170 and produces a null callback payload at node+0x28. Existing payloads are returned as raw pointers. Calling a lookup is consequently not always a read-only search, and lookup alone does not invoke a callback.

Complete caption replacement owner 0x334850 acts only when the supplied caption pointer differs from P+0x58. It releases the previous association first: a 64-bit previous-caption +0x88 value of one reaches GUI unlink/deletion 0x33DFC0; other nonnull cases reach owner-set removal. It registers a nonnull replacement in that caption's +0x80 owner collection, then publishes the replacement at P+0x58. This owner contains no Edit-cache refresh or virtual +0x38 call. The shared-caption alternatives in complete owners 0x335780 and 0x335440 can obtain a replacement through virtual +0x68 when the selected signed 32-bit count exceeds one, release/remove the prior association, register the new one and then assign P+0x58. The larger owner separately handles transform and state sharing before its guarded child recursion. Those are additional selected node writers, not proof that every caption writer has been found.

Thus E+0x1A8 is a borrowed pointer refreshed by attachment, rather than an ownership count or automatic alias of the latest P+0x58. The selected replacement bodies do not prove the cached pointer is cleared before a previous caption is destroyed. A later attachment can refresh it; that is not a lifetime guarantee during arbitrary reentrant or concurrent replacement.

Concrete Edit deleting owner 0x327490 restores its concrete tables, reaches complete base destructor 0x329500 and frees E when deleting flag bit zero is set. That base visits and frees the callback payloads in its +0x188 registry, destroys that registry/sentinel, tears down its selected owned objects/strings, and reaches 0x366A10 to unlink its base registration when the GUI's selected shutdown byte permits. It contains no caption deletion based on E+0x1A8 and no deletion of the raw filter receiver based on E+0x1B0. Selected GUI widget removal 0x33E0C0 first clears matching U+0x170, +0x178 and +0x1B0 widget pointers, unlinks the widget's list node under GUI+0x90, then invokes its deleting virtual. The captured node at U+0x168 is a different field and is not explicitly cleared by those matching-widget tests.

## Clamp order and its valid domain

Complete clamp 0x328DF0 is synchronous and contains no callback or imported call. With a null cached caption, it clears the two selection words by an eight-byte zero store and returns.

With a nonnull cached caption, it first replaces a negative current index with zero. It then compares the current index with the signed low 32-bit current selected-string length and, if the index is greater, replaces it with that length. It next adds the selection displacement to the current index at 32-bit width. A negative sum replaces the displacement with the 32-bit negation of the current index. Finally it rereads the displacement and current selected-string length, forms the 32-bit endpoint again, and, if that endpoint is greater than the length, replaces the current index with the 32-bit length minus displacement.

The last correction changes the current index, not the displacement. It does not repeat the earlier corrections afterward. For valid nonnegative lengths and ordinary representable endpoints this bounds the current index and selection endpoint. Native wrap, signed comparisons and the exact store order remain the facts for extreme words; this is not a guarantee that malformed indices, negative narrowed lengths or overflowing endpoints are fully normalized. No unmentioned string-array index, pointer or allocation validity check is supplied by this owner.

## Complete left press: 0x328120

A null cached caption returns before clearing drag, computing a point, clamping, laying out or assigning capture. With a nonnull caption, the first state change is a one-byte zero store to E+0x1B8. It then calls complete inverse-coordinate helper 0x100EE0 on the current P, reads U's current pointer coordinates and transforms that pair into a local binary32 pair. Both coordinates are calculated, although pick later uses only its first coordinate.

Owner 0x100EE0 uses the node transform's +0x2E0 64-byte matrix, or explicit identity words when that receiver is absent; it recursively composes parent inverse-coordinate state through the same owner and complete matrix owner 0x173E0. The selected pointer transform forms a denominator from the first coordinate times matrix word three, plus the second coordinate times word seven, then word fifteen. It divides binary32 one by that result. Its first numerator adds the first coordinate times word zero and second coordinate times word four, then word twelve; its second uses words one, five and thirteen. Each numerator is multiplied by the reciprocal. The selected native operations are separate scalar binary32 multiplies, additions and division, without a fused-operation substitution.

After computing the local pair, press reads U+0x170 through 0x342090.

If the selected widget differs from E, it reads the current caption's selected-string length into E+0x1C4, reads that length again and stores its 32-bit negation into E+0x1C8. It then calls selection owner 0x349A90 with U and E. This branch does not call pick and contains no later one-store to the drag byte. Its unconditional jump at 0x328249 reaches the common continuation at 0x3282E1: clamp at 0x3282E4, layout at 0x3282EC, and capture assignment at 0x3282FC. There is no return or bypass of those common calls after the selection callback returns normally.

If E was already selected, press searches the GUI key tree for exact 16-bit key 0x10. Without that key it calls pick 0x328B30 and then clears the selection displacement. With that key it saves the prior current index before calling pick; after pick returns, it reads the then-current index, adds it to that saved index at 32-bit width, and subtracts that sum from the then-current selection displacement. This is the native addition/subtraction sequence, not a replacement with an inferred conventional anchor rule.

Both already-selected branches then store one to E+0x1B8 at 0x3282DA, before joining the common clamp/layout/capture continuation. Pick itself has already performed a clamp before these caller-side displacement writes. Common clamp is therefore a second clamp on those paths.

The common continuation reloads the current caption through clamp and layout. After layout returns, press reads the then-current P from E+0x178 and U from E+8 and assigns that P to U+0x168. A callback's normal return is followed by those later reads; the receiver/node/caption are not an entry-time snapshot across callbacks. Common capture does not recheck the selected-widget pointer or the caption admission that began the press.

In the not-yet-selected branch, the direct stores alone leave drag zero while still reaching clamp, layout and logical capture. A selection callback can reenter and change state before this continuation; the owner does not restore an entry snapshot or force drag back to zero after that callback. In the already-selected branches the one-store is after pick returns and before common layout; there is no second one-store after layout's own virtual call.

## Complete pointer pick: 0x328B30

This owner requires a valid current caption on entry; it contains no initial null-caption return. It first copies the current selected two-byte string into a temporary 0x20-byte string through complete owner 0x3D3B0, requesting its entire remaining length from position zero. It then clears E+0x1C4 and narrows the copied length to signed 32 bits.

It captures the current P for the transform setup. A nonnull parent supplies its forward matrix chain through complete owner 0x100FA0; otherwise the checked 64-byte identity at 0x42E630 is copied. A nonnull P+0x60 then supplies its +0x2A0 matrix to composition owner 0x173E0. The selected string snapshot and this matrix are separate from the caption/style fields subsequently loaded for individual metric calls.

For a positive narrowed length, the loop visits signed indices zero through length minus one in increasing order. Before each metric call it reloads E+0x1A8 and the current metric/style operands listed below. The metric call receives the temporary copied string, the current loop index and the captured matrix. It produces a binary32 position pair and size pair.

Pick multiplies the first size component by the checked binary32 constant 0x3F333333 at RVA 0x4328AC, then adds the first position component. That constant is approximately 0.699999988079071, not one half. If the pointer's local first coordinate is strictly greater than that threshold, it stores loop index plus one into E+0x1C4. Equality does not store. Native COMISS followed by JBE also skips that store for an unordered comparison when execution continues with floating exceptions masked. The loop does not break on either a passing or failing threshold, and it does not compare the pointer's second coordinate. Nonmonotone provider outputs therefore do not become a nearest-point or first-crossing contract.

After all iterations, or immediately for a nonpositive narrowed length, pick clamps against the then-current cached caption, writes one to E+0x15C, and destroys the temporary string. Its normal result is in the Edit's current-index field, not a returned index. The selected long-string cleanup checks the reached size/alignment/allocation-base conditions and can reach the imported invalid-parameter termination boundary; arbitrary malformed string objects are not valid inputs.

## Complete motion: 0x328330

Motion first tests bit zero of byte U+0x1A8. A clear bit returns without a selected-widget query or Edit write. Otherwise it requires the current U+0x170 result to equal E, then requires E+0x1B8 nonzero. Only after those tests does it save E+0x1C4 and calculate a local pointer pair using 0x100EE0 and the current U coordinates, as in press.

It then calls pick, reads the then-current index after pick's own clamp, subtracts that index from the saved pre-pick index, and adds the 32-bit difference to the then-current selection displacement. It calls clamp again, then layout, and returns. It does not assign capture, select the widget or set/clear drag in this owner. Because pick can itself change both selection words through clamp, this complete sequence cannot generally be replaced with a formula that assumes the original displacement stayed untouched throughout picking.

There is no independent initial null-caption or P check after motion's three admission tests. A direct alternate caller that supplies an admitted selected/dragging Edit with an invalid cached caption is not made safe by the press owner's separate admission check.

## Metric receiver inputs and output order

The direct metric owner used by pick and layout is complete 0x352310. Its eight-byte receiver is the current caption+0x2F0 pointer. Its next arguments are the selected 0x20-byte two-byte-string object, a 32-bit integer index and a pointer to the caller's captured 64-byte matrix. Native R8D forwarding establishes the integer index despite an imprecise floating type in the deeper decompilation.

The remaining supplied operands, in order, are binary32 words at caption+0x2B8, +0x2BC, +0x2C0, +0x2C4 and +0x2E8; pointers to the caller's position and size output pairs; the 32-bit flags at caption+0x2F8; and a one-byte boolean obtained from bit zero of caption+0x2FC. Pick reloads this caption/style set for each index. Layout reloads it for its first metric query and, if needed, again for its second query. These are current receiver fields, not constants embedded in the Edit and not values taken from an earlier handoff.

Owner 0x352310 enters receiver critical section +0x168 before calling complete configuration owner 0x354600 with those size/style/matrix/boolean inputs. It then divides the supplied +0x2B8 binary32 value by the receiver's current +0x134 value to obtain the vertical output scale, and by +0x130 to obtain the horizontal scale. When the boolean is nonzero it also divides the supplied +0x2E8 value by the horizontal scale.

Flag bit 0x10 selects a temporary-copy route: copy the entire input string, call 0x354F20 on that copy and the selected width operand, measure it through complete 0x351650, then destroy the copy. Without that bit it measures the supplied string directly through 0x351650. The wrapping helper can replace selected space code units with code unit ten in its temporary string; this does not write that wrapped text into the current caption.

On normal return from measurement, the wrapper multiplies and stores the first position component by the horizontal scale, then the second position component by the vertical scale, then the first size component by the horizontal scale, then the second size component by the vertical scale. It leaves receiver+0x168 only after those four output stores. Those stores are separately rounded binary32 operations. This wrapper supplies no general null-receiver, valid-font, finite-scale or allocation-success guarantee.

The expanded configuration/measurement/wrapping owners show dependence on live receiver font state, per-code-unit metric records and conditional original font-provider calls. Their transitive glyph loading, kerning, font backend, allocator and imported provider internals are not closed into an independent numerical font contract here. The selected call arguments and output-store order are proved; no guessed glyph widths, provider sequence, thread scheduling or reentrancy guarantee is supplied.

Calls that the C export labels as a guard check, at RVA 0xE8A20, require particular care. The complete original body is a return-only native routine, three bytes long. In this supported image the calls around pick/layout/draw metric work are not calls through U+0x48, do not invoke that field as a callback, and do not enter a GUI lock. The passed operand and decompiler name cannot be used to manufacture such effects. Actual indirect virtual/callback calls are distinguished below.

## Complete layout: 0x328470

Layout first requires a nonnull cached caption and a nonnull parent of the current P. Either failed entry test returns before the bound/metric queries and position writes. It does not test selected-widget identity, drag, button state, a root name or font-pointer nonnullness in this owner.

Its ordered preparation is as follows. It calls lower-bound owner 0x32FFA0 on the current parent with 32-bit flags one. After that call returns it reloads E+0x178 and calls upper-bound owner 0x32F7D0 on that then-current parent with flags one. It then captures the then-current P and builds a forward matrix from that node's parent chain through 0x100FA0, or checked identity when no parent is then present, and composes P's nonnull transform+0x2A0 matrix through 0x173E0. It invokes the return-only 0xE8A20 boundary, reloads the current caption/style fields, and makes the first 0x352310 query at the then-current E+0x1C4 using the current selected caption string and captured matrix.

The two bounds are therefore distinct ordered provider queries. A virtual receiver reached by the first can return before the second parent operand is read, and receivers reached by the second can return before the matrix/caption/style operands are read. The owner does not preserve the original parent/caption admission across those indirect calls or recheck every dereference afterward. Invalid replacement during such a call is not given an invented fallback.

After the first metric query returns, layout reloads P again. It copies all sixteen binary32 words of that current parent's transform+0x2A0 matrix and composes the current P transform's +0x2A0 matrix. These particular dereferences require the reached parent/transform receivers to exist; the earlier entry check is not an ownership pin. This new local matrix is retained for the subsequent local numerical adjustment even if later calls change live transform state.

Using that retained matrix, it transforms the returned position homogeneously: multiply the second position component by word seven, add the first component times word three, then word fifteen; divide binary32 one by that denominator. For the transformed first coordinate, multiply the second component by word four, add the first component times word zero, add word twelve, then multiply by the reciprocal. The transformed second coordinate similarly uses words five, one and thirteen. It also transforms the returned size as a vector using the first two rows' selected words, with no translation or homogeneous division for that size vector.

The next branch first adds transformed first-position and first-size components, then compares the sum with the previously returned upper bound's first component. A finite sum equal to or greater than that upper bound selects the plus-six query route. A strictly smaller sum selects the lower-side test. Native COMISS followed by JC also sends an unordered first comparison to that lower-side test when execution continues with masked floating exceptions.

On the lower-side test, layout compares the previously returned lower bound's first component with the transformed position. If the lower bound is at most that position, it reaches the common return-only boundary and exits without an indexed-position write or transform-update virtual. Equality is included. Native JBE also takes this no-adjustment continuation for an unordered second comparison. Otherwise it selects the minus-six query route. Thus unordered comparisons must be evaluated in their actual order; they do not establish a single universal NaN layout rule.

The plus-six route constructs a current forward matrix from the captured node's then-current parent chain, or identity if that parent is absent, and composes the node's nonnull transform. The minus-six route obtains the forward chain on that node itself through 0x100FA0. Neither branch substitutes the inverse-coordinate helper used for pointer picking.

Each route then uses complete temporary-string owner 0x6E070 to copy the then-current selected caption string and append the two-byte zero-terminated literal at RVA 0x43760C. The checked literal is exactly code unit 0x0020 followed by zero: one appended space. The supplied index is the current 32-bit Edit index plus six for the upper route and minus six for the lower route, formed at native 32-bit width. There is no owner-side lower clamp of that supplied minus-six index before the metric call. The selected metric owner and its providers determine those outputs.

After constructing that string, the branch reloads the current caption's metric/style operands and makes its second 0x352310 call with the new temporary string and matrix. It destroys the temporary string after normal return. It then transforms the new returned position with the retained local parent/P matrix copied after the first query, rather than taking a fresh live matrix for this arithmetic.

For the upper route, the horizontal adjustment is the newly transformed first position minus the earlier upper-bound first coordinate. For the lower route, it is that new first position minus the earlier lower-bound first coordinate. Layout converts this binary32 adjustment to signed 32 bits with truncation toward zero for finite in-range values, converts that integer back to binary32, then subtracts it from the current P transform's selected position-array first component. The live receiver, signed +0xAC index and +0xE0 array are read only at this later store. The directly reached store changes the first four-byte component of that eight-byte pair; it does not directly store the pair's second component.

It next rereads the current Edit index, subtracts six at 32-bit width and tests the signed result. A result at most zero forces the saved E+0x1BC word into the current selected first-position component. Otherwise it compares that current binary32 position with the saved E+0x1BC value and performs that same reset only when the current position is strictly greater. Equality preserves it. The native JBE skips the reset for an unordered comparison on this latter branch when execution continues. E+0x1C0's saved second component is not used by these particular layout stores.

After the position/possible-baseline stores, layout reloads the current P+0x60 receiver and invokes its virtual +8 with a low-byte true argument. That is an actual indirect call and may make further receiver/provider effects; a universal transform class or complete effect is not inferred merely from this slot. Only after its normal return does layout reach the final return-only 0xE8A20 call and return to its caller. On a press caller, logical capture is read/assigned after this return. On a key caller, later callback or return ordering is as described separately below.

The in-bounds and early-return paths have no such transform virtual. A zero truncated horizontal adjustment on an admitted adjustment route does not bypass the later baseline test or virtual call. No caption text write is performed by this layout body.

## Bound queries and indirect providers

Complete 0x32FFA0 and 0x32F7D0 are distinct lower/upper component-bound owners. Layout passes its parent node and flags one to each. They read that node's transform receiver and its +0x2A0 matrix; the numeric result is not taken solely from a width stored in E.

With a caption at node+0x58 they call its virtual +0x30 and then its then-current caption's virtual +0x38, obtaining two component pairs. A nonnull node+0x78 association can apply its embedded +0x38 point-map virtual to each pair. The selected state padding at state+0x300, signed index +0x2CC, is applied only with state byte +0x318 bit zero set and a positive selected binary32 value. The selected owners transform the reached rectangle corners and accumulate component minima or maxima respectively.

Without a caption, a local widget can admit its own bounds via virtual +0x20, followed by +0x10 for the lower owner or +0x18 for the upper owner. If that route does not supply bounds, the selected state at +0x168 indexed by signed +0x134, and the node's +0x50 pointer, control whether a transformed-origin result returns immediately or child aggregation remains available.

Child aggregation requires that selected +0x168 integer be zero and flag value 0x8 be clear. It visits children in native forward order. A child must have node flag bit two clear and, unless flag value 0x2 bypasses it, a nonzero selected integer in the child's state +0xE0 array at signed +0xAC index. Flag value 0x4, when supplied, additionally requires child node flag bit one clear. Flag value 0x1, actually supplied by layout, additionally requires the child's local-widget pointer +0x70 to be null. Each admitted child obtains lower then upper results recursively; a reached pair with at least one differing component is transformed through the current node matrix and included in the selected extrema. Actual indirect/provider returns, live node state and floating arithmetic remain inputs to the resulting bounds.

This is why the layout's two bound queries can invoke caption, widget and point-map virtuals before later operands are read. The complete owners and their recognized native bytes were inspected; this contract does not assign universal geometry, safety, callback-freedom or imported font behavior to every possible receiver of those virtuals.

Forward helper 0x100FA0 recursively obtains the parent forward chain first, or defines identity with binary32 ones and zeros, then composes the current node's nonnull +0x60 matrix at +0x2A0. Matrix owner 0x173E0 uses separate binary32 products/additions to overwrite the sixteen destination words in its selected order. No imported transform implementation or numerical inverse guarantee is assumed.

## Selection callbacks before press continuation

Complete selection owner 0x349A90 receives U and the requested raw widget pointer. If U+0x170 is nonnull and already equals that requested pointer, it returns immediately. Otherwise an old nonnull selection first has its +0x15C byte set to one. Event owner 0x342560 is called for numeric event 0x13 on the old selection's then-current attached node.

That event owner requires a nonnull node, node flag bit one clear, a nonnull node+0x78 association, a nonnull callback for the supplied numeric identifier, and either a nonzero association+0x1A0 byte or a zero callback+8 byte. It then invokes the callback receiver through its first virtual. This is a synchronous indirect-call boundary. The old U+0x170 has not yet been replaced by the outer selection owner at that point.

After that call returns, 0x349A90 unconditionally publishes its originally requested widget pointer at U+0x170. A nested callback's intervening selected-pointer write can therefore be superseded by this later outer store. For a nonnull requested widget it then obtains that widget's then-current attached node, requires node flag bit one clear and a nonnull node+0x78 association, looks up event 0x12, applies the same association-active/callback-byte gate, and invokes it if admitted.

The event 0x12 callback runs after the outer selected-pointer store. This owner does not write the selected pointer again after that callback returns. The callback association is the current node+0x78 receiver, which is not assumed to be the same raw pointer as the requested widget.

Back in the not-yet-selected press branch, both these callbacks can precede the common clamp's caption read, layout's current parent/style/metric reads and the final capture's current P/U reads. Neither a same-thread callback return nor logical capture supplies an object-lifetime pin. No callback implementation, permitted mutation, exception behavior or destruction safety is invented for the unresolved concrete callback receiver.

## Complete release and GUI recalculation

Complete native release thunk 0x328320 loads the eight-byte U pointer from E+8 into the receiver argument and transfers directly to complete GUI owner 0x3493B0. That owner first clears eight-byte U+0x168, then reads the current binary32 pointer coordinates and transfers to complete motion/hit-recalculation owner 0x3418D0 with those values. It contains no selected-widget test, caption test, drag-byte store, button-bit store or selection-displacement write.

Owner 0x3418D0 first copies the old current pointer words to U+0x188/+0x18C and stores its supplied coordinates at +0x180/+0x184. It reads U+0x168; a nonnull capture is the candidate node. A null capture reaches complete query owner 0x333F60 on root U+0x140, using the current pointer pair and flags zero. Release has just cleared capture before this candidate read, so its ordinary sequential continuation selects the query route unless intervening reentrancy or another writer changes the field. No such interleaving is asserted.

The complete query body has indexed-root/node flag admissions, optional native clip/geometry providers, inverse-coordinate preparation, caption/widget shape and point-map virtual boundaries, and reverse child traversal returning its first admitted nonnull node. Its selected integer, pointer, flag and transform operands were read from the original owner. Its transitive clip/shape provider closure is not replaced here by a generic rectangular or root-name assumption. A null query result is supported by the recalculation continuation.

Recalculation obtains the old associated widget from the then-current U+0x160 node and captures the candidate node's current +0x78 associated widget. When those widget pointers differ, the old active association can first receive virtual +0xB8. It then reloads the old node from U+0x160 and can dispatch numeric event 0xE with node flag bit one clear and the active/callback-byte gate. Next the captured new active association can receive virtual +0xB0. A nonnull candidate can then dispatch event 0xD through its current +0x78 association under the corresponding node/active gates.

For a nonnull candidate, regardless of whether associations differed, the captured new active association can next receive virtual +0xA8. For the concrete Edit table that is 0x328330, so its current button-bit, selected-widget and drag-byte admissions are evaluated at this later call. Recalculation then can dispatch event 0xC using the candidate's then-current association and the node/active gates. It does not skip this block merely because old/new widget pointers were equal.

Only after those virtuals/callbacks does recalculation assign its captured candidate to U+0x160. It then obtains the current U+0x140 root. With a nonzero selected integer in root state +0xE0 at signed +0xAC, and root flag bits two and one both clear, it invokes local-widget virtual +0x130 and visits children in forward order through complete traversal 0x33C360. That traversal applies the same indexed-active and flag admissions at each node, invokes the supplied callback on a nonnull local widget, and recurses. Complete adapter 0x33AAB8 selects virtual +0x130. The concrete Edit entry for +0x130 is the return-only owner, but other receivers remain indirect boundaries.

These calls can occur before a later association, current node or child-list link is read. The source retains some raw candidate/widget pointers and reloads other operands; it does not perform one immutable all-node snapshot or a final selection/caption consistency test. The selected Edit +0xB0 and +0xB8 entries are return-only, whereas its +0xA8 entry is actual motion.

The complete release route does not itself clear E+0x1B8. Whether the low button-state bit has already been cleared by an upstream release dispatcher is a separate caller fact, not an assumption supplied by this thunk. A direct caller that leaves the bit set can still admit the reached Edit motion if its other current gates hold. There is no operating-system message pump, imported capture guarantee or scheduler promise in these selected bodies.

An additional complete native route at 0x32B6B0 tests whether either low bit of widget+0x150 is set and whether U+0x168 equals its current attached node. Only then does it transfer to 0x3493B0. Its gates do not become universal gates for direct calls through the Edit release table entry.

## Complete key owner and Left/Right edges: 0x327E20

The owner takes the low sixteen-bit key argument. A null cached caption returns immediately. Otherwise every key first calls clamp and writes one to E+0x15C. It does not independently require E to equal U+0x170, inspect ancestor/root visibility, read a repeat count or consult a clock. Such admission must be proved for a particular caller; it is not supplied by this receiver body.

For key 0x25, Left, a signed current index greater than zero is decremented first. Only after storing the decremented index does the owner query exact key 0x10 through 0x9AEB0. A present modifier increments the current selection displacement by one; an absent modifier clears it. Both paths then call layout and return.

When the current index is at most zero after the initial clamp, Left skips that modifier query and all these movement/displacement stores. It nevertheless reaches the common layout call. In particular, a valid nonzero selection displacement at the left edge is preserved even when Shift is absent. It is not unconditionally collapsed to an endpoint.

For key 0x27, Right, the current index must be strictly below the signed low 32-bit current selected-string length. If so the owner increments and stores the index first, then queries key 0x10. A present modifier decrements the current displacement by one; an absent modifier clears it. Layout follows. At an index equal to or greater than that length, it skips the modifier query and those stores and reaches common layout, preserving the selection displacement established by the initial clamp.

Consequently an interior Left/Right without Shift moves one code-unit index from the current endpoint before clearing selection. It does not first jump directly to the minimum/maximum selected endpoint. Edge presses retain a surviving nonzero selection. Repeated calls apply the same branch tests afresh; there is no one-shot edge latch or special repeat admission in this owner.

Key 0xD calls 0x349A90 with requested selected widget zero, then layout, and returns. The old-selection callback described above can occur before that layout reads current state. Key 0x24, Home, queries key 0x10; without it displacement becomes zero, and with it the current index is added to the current displacement at 32-bit width. It then stores current index zero and reaches common layout.

Key 0x23, End, also queries the modifier first. Without it displacement becomes zero. With it the owner reads the current selected-string length, subtracts it from the current index and adds that 32-bit difference to displacement. It then reads the length again for the new current-index store and reaches common layout. These operations have no final call to clamp before layout beyond the initial clamp already described.

Key 0x2E calls complete deletion helper 0x327580, then calls layout again. Key eight, Backspace, uses that same helper when displacement is nonzero. With zero displacement and current index below one, Backspace returns directly after the initial clamp/activity write, without layout or a text-change callback. Otherwise it erases one two-byte code unit immediately before the current index through complete string owner 0x3DE90, decrements the current index, calls the current caption's virtual +8 with low-byte true, calls layout, then invokes numeric event 0x14 through 0x329D60 and returns.

All other key values return after the initial clamp/activity write. There is no additional select-all, clipboard, word-navigation or key-repeat feature in this complete key body.

Complete deletion helper 0x327580 itself starts with clamp. It obtains the current caption/string and current index and returns if that index is greater than the signed narrowed length. Zero displacement chooses a one-code-unit erase at the current index. Positive displacement chooses that many units from the current index. Negative displacement moves the erase start by that displacement and uses its 32-bit negation as the count. It calls 0x3DE90 with sign-extended selected indices/counts interpreted by that helper's 64-bit string operations. On the valid bounded domain these erase the selected span, or the one unit at the current index.

After erase it rereads the current displacement; if negative, it adds it to the current index. It then clears displacement, calls the current caption's virtual +8 with true, invokes event 0x14 through 0x329D60, and finally calls layout. Thus a successful Delete or selected Backspace normally reaches layout once inside the helper and again in 0x327E20. The single-code-unit unselected Backspace instead orders its own layout before event 0x14. These native call orders are different.

String erase 0x3DE90 checks its 64-bit position against length and reaches the original out-of-range exception boundary when it exceeds length. When count reaches/passes the remaining length it shortens length to position and writes the two-byte terminator; otherwise a nonzero count moves the remaining two-byte data, shortens length and writes the terminator. A valid Delete at the end can consequently reach the later caption virtual/event/layout even when no code unit is removed. No “only notify when content changed” rule is inserted.

Complete event adapter 0x329D60 looks up the supplied event on the widget and invokes a nonnull callback through its first virtual. It does not apply 0x342560's active/callback-byte gate itself. Caption virtual +8 is an indirect boundary before later layout or event operands are read. Destruction, caption replacement or nested editing by that receiver is not guaranteed safe by an unproved imported/runtime rule.

## Adjacent text mutation and actual drawing

The complete adjacent character owner 0x327B40 is a separate entry from the key owner. It admits a nonnull cached caption and an unsigned sixteen-bit code unit at least 0x20, sets the activity byte, and constructs a one-unit temporary string. It does not itself assemble surrogate pairs or receive a repeat count.

With a nonzero selection displacement, a nonnull raw filter receiver at E+0x1B0 first receives that one-unit string through its first virtual. A rejected low-byte result skips the remaining edit. Otherwise the selection-deletion helper runs before the character is inserted. The owner then copies the current selected caption string, clamps the Edit, inserts the one-unit temporary at the then-current index through complete string owner 0x3DF40, and, when a filter exists, calls it again with the proposed whole string.

A null filter or accepted whole-string result reaches complete node-text owner 0x334C50 on the current P, increments the then-current Edit index, calls layout, then invokes event 0x14. Temporary strings are destroyed after their selected normal uses. A rejection at this second filter occurs after any earlier selection deletion; no rollback of that earlier helper is supplied by this body. Both filter virtuals are actual synchronous reentrancy boundaries before later cached-caption, current-index or node operands are read.

Owner 0x334C50 returns/prunes recursion for node flag bit two set. At an admitted node, a nonnull caption of 32-bit type three obtains its current selected string, compares it with the supplied string through the reached original comparison helper, and copies it when that helper reports a nonzero result and the string objects differ. It then visits children in forward order with the same supplied string. This node-tree text route does not itself call Edit attachment or publish E+0x1A8. A reentrant change of P's caption can therefore distinguish the caption the setter reaches from a previously cached caption; this contract does not assume they always agree.

Complete native alternate entry 0x3280C0, bound at concrete table offset +0x68, admits only a nonnull cached caption, writes the current selected-string length to the current index, reads that length again and stores its 32-bit negation to displacement, then transfers to layout. It does not directly call clamp, select E, assign logical capture or set drag. The table slot is identified without inventing an upstream button/message meaning.

Actual concrete draw owner 0x327630 is bound at table offset +8. It first requires U+0x170 to equal E, then directly dereferences the cached caption and requires its +0x2F0 metric pointer nonnull. It has no separate null-caption check before that dereference. It reaches unexpanded font-setup boundary 0x354190 with the current caption+0x2B8 word and zero before reading the current selection endpoints.

It orders the current index and its 32-bit displaced endpoint, clips the smaller to zero and the larger to the signed narrowed current-string length, builds the parent/node forward matrix and makes metric queries for those two indices, in that order. The first query's first position is retained across the second. It supplies GUI virtual +0x20 with that earlier first position as horizontal origin, the second position minus the first as horizontal extent, a vertical origin formed by toggling the sign bit of the current caption+0x2B8 word, and a vertical extent obtained by multiplying the unmodified caption+0x2B8 binary32 value by checked binary32 1.25 at RVA 0x42E71C.

After that actual GUI virtual returns, it rereads the current Edit index and current caption/string length. An index from zero through length inclusive admits a third metric query. The next GUI virtual is +0x18, with a one-unit binary32 horizontal extent, the same signed vertical origin and 1.25-scaled vertical extent, and the checked four-binary32 vector at RVA 0x438050: one, zero, zero, one. The GUI/resource interpretation of those parameters remains at that provider boundary. The body finally reaches return-only 0xE8A20. Its two GUI draw virtuals can precede later current index/caption reads.

This distinguishes actual drawing from attachment update, pointer-message response and layout position adjustment. The complete press, motion, clamp and key owners do not directly call 0x327630. Activity writes and layout calls do not by themselves prove a draw occurs immediately, one draw occurs per message, an operating-system paint is sent, or a particular frame schedule/animation is used.

## Independently selected conditional sequences

These are source-only conditional control-flow examples, not runtime observations or numerical claims about an untraced font/geometry provider. They assume valid stable receivers, normal helper returns, representable lengths/indices, no additional changes to Edit fields by unspecified callbacks/providers, and only the stated callbacks or provider outputs.

* With a six-unit current string, an initially different selected widget and no state-mutating selection callbacks, a press writes current index six and displacement minus six. It reaches common clamp, layout and logical capture. The direct press stores leave drag zero. A following motion call still requires the separate button, selection and nonzero-drag gates; capture by itself does not satisfy the drag gate.
* With a ten-unit string, an already-selected Edit, prior index three, displacement zero, Shift present and pick leaving index five, the selected-press arithmetic subtracts eight from the current displacement. The final clamp then changes displacement to minus five because the endpoint would otherwise be negative. The resulting index/displacement pair is five/minus five on these assumptions. This follows the actual saved-index-plus-new-index subtraction, including pick's earlier clamp.
* With length six, index zero and displacement three, a Left call preserves that pair after initial clamp and reaches layout without querying Shift. A Right call without Shift from that same pair instead first advances to index one and clears displacement. With index six and displacement minus six, Right preserves the pair at the right edge. These examples require no guessed text-navigation convention.
* For a copied two-unit string, suppose the metric outputs in an identity coordinate setting are first positions zero and ten, each with first size ten. The checked binary32 0x3F333333 product rounds to seven for this size, so the two reached thresholds are seven and seventeen. Pointer coordinate seven does not pass the first threshold; coordinate seventeen passes the first and equals the second, leaving index one before any clamp interaction. The second coordinate does not select either threshold.
* With finite transformed first position ten, first size two and upper-bound first coordinate twelve, layout's first sum equals the upper bound. It takes the plus-six metric route. If the sum were instead strictly below twelve, the lower-bound test would decide whether to take minus-six or return. No approximate “already inside on equality” assumption replaces the source branch.
* A press's selection event 0x12 can normally return after performing a nested attachment that changes the current caption and saved-position pair. The outer common clamp/layout use later current reads, and the outer capture uses P read after layout. This conditional event ordering does not certify that every destructive mutation would leave those receivers valid.
* A release call can leave the Edit drag byte nonzero while clearing U+0x168 and recalculating the candidate. With the GUI button bit already zero, a reached concrete Edit motion virtual returns at its first test. A direct caller leaving the bit one must instead satisfy the remaining current selection/drag tests; this contract does not infer the caller's button write.
* If caption replacement has published a new P+0x58 but no attachment has run, this selected closure supplies no automatic assignment to E+0x1A8. A later admitted GUI update can refresh that cached pointer and resample the position baseline. It does not establish a universal safe interval for the prior borrowed pointer.

## Evidence procedure, correspondence and limits

The author read the standalone docs/CLEANROOM_AUTHOR.md policy first for this assignment. Reference filename discovery preceded scoped content reads. The original export metadata files used were decompiled/functions.csv, decompiled/function_ranges.csv and decompiled/calls.csv beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. The individually selected C files are decompiled/functions/140 followed by each six-digit lowercase owner RVA below, followed by .c. All seventy-five selected entries report successful original C export; their RVA/file associations and declared body-byte counts were checked against the independent native comparison.

Original native instructions were selected by their owning RVA from decompiled/assembly.txt. For each exported instruction, its exact encoded bytes were compared with raw-file bytes of the supported original executable after PE section mapping. Metadata ranges are inclusive in function_ranges.csv; the manifest below presents start-inclusive/end-exclusive ranges. Discontiguous recognized ranges remain separate, and unsupported intervening bytes/traps/padding are not silently turned into instructions. The three unassigned native bodies were selected by exact address bounds, checked over their entire stated byte spans and compared separately.

This procedure covers every recognized instruction of the complete named press, motion, release target, pick, attachment, clamp, layout and key owners, and the selected complete dependency owners in the manifest. Branch joins, relevant integer/float widths, call/store order and constants were interpreted from corresponding original instruction evidence; ambiguous C-export types did not override native operands. The return-only guard body, the press jump to common continuation, the Shift press arithmetic, the layout comparisons/conversions and Left/Right edge destinations received explicit instruction inspection.

The selected native comparison totals are 75 named owners, 7,798 instructions and 33,511 instruction bytes, with zero mismatching bytes. The three additional complete native bodies total 130 bytes and 29 instructions, also with zero mismatches. Whole-owner byte correspondence is distinguished from transitive numerical/provider closure.

The source reaches additional imported allocation, memory/string, critical-section, exception and font/backend providers, and indirect callbacks/virtuals. Their internals were not read or replaced with imported guarantees. Numerical font/clip/geometry helper closures, every transform/caption concrete receiver class, all aliasing/writers, every upstream key/button/message dispatcher and every alternate lifecycle caller remain outside the claimed closure. Their selected inputs, branch admissions and normal call/return boundaries are identified here; a universal GUI or executable census is not claimed.

In particular, the complete bodies prove no overarching Edit lock, callback lifetime pin, all-state snapshot, reentrancy rollback, thread affinity, message-pump fairness, maximum callback latency, OS capture contract or imported-font determinism. The font metric receiver's own +0x168 critical-section scope and GUI list +0x90 scopes do not establish that every writer or provider uses those locks. Raw node/widget/caption pointers and list traversal across indirect calls remain explicit lifetime boundaries.

Signed text/index arithmetic is native 32-bit arithmetic; string capacities and internal string-length operations are 64 bits. Normal nonnegative representable text indices are distinguished from malformed strings, negative narrowed lengths, overflowing endpoints, invalid signed array indices and pointers that become invalid during a callback. Floating operations use the original separate binary32 instructions and comparison flags. Numerical outcomes for nonfinite operands, invalid/unmasked exceptions, out-of-range conversions, denormal/rounding-mode settings and untraced font returns are not broadened into an ordinary-input guarantee. The explicitly stated unordered branch directions assume control continues past the comparison.

This is an independent source contract. It does not use an earlier prose handoff, implementation behavior, test result, owner capture or runtime observation as evidence. No active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, project history, replacement engine/exporter/extractor, tests/results, owner outputs/data/captures, SDK/dependency/reconstruction source or packages were read. No additional agent was spawned. No implementation or runtime feedback was requested or received.

The target file was created exclusively and completely read back before the FINAL / STABLE / HELD report. Prior held artifacts were neither read nor changed. Its exact saved byte length and SHA256 are supplied in the external handoff. At HELD all writes to this artifact stop.

## Complete selected-owner manifest

All ranges are RVAs, with the end excluded. Every row has zero mismatches. The owner RVA also identifies its individually selected original C-export filename as described above.


| Owner RVA | Complete recognized native ranges | Bytes | Instructions |
| --- | --- | ---: | ---: |
| 0x328120 | 0x328120–0x328317 | 503 | 107 |
| 0x328330 | 0x328330–0x32846C | 316 | 63 |
| 0x3493B0 | 0x3493B0–0x3493D0 | 32 | 4 |
| 0x328B30 | 0x328B30–0x328D73; 0x328D74–0x328D8C; 0x328D8D–0x328D9C; 0x328D9D–0x328DAC; 0x328DAD–0x328DB9; 0x328DBA–0x328DEF | 698 | 140 |
| 0x327AA0 | 0x327AA0–0x327B39 | 153 | 36 |
| 0x328DF0 | 0x328DF0–0x328EB2 | 194 | 42 |
| 0x328470 | 0x328470–0x328B21 | 1713 | 343 |
| 0x327E20 | 0x327E20–0x3280B3 | 659 | 158 |
| 0x342090 | 0x342090–0x342098 | 8 | 2 |
| 0x349A90 | 0x349A90–0x349B34 | 164 | 45 |
| 0x33C410 | 0x33C410–0x33C418 | 8 | 2 |
| 0x3418D0 | 0x3418D0–0x341B29 | 601 | 163 |
| 0x100EE0 | 0x100EE0–0x100F96 | 182 | 43 |
| 0x100FA0 | 0x100FA0–0x101018 | 120 | 31 |
| 0x0173E0 | 0x173E0–0x17728 | 840 | 200 |
| 0x352310 | 0x352310–0x3524ED | 477 | 110 |
| 0x32FFA0 | 0x32FFA0–0x33074D | 1965 | 451 |
| 0x32F7D0 | 0x32F7D0–0x32FF98 | 1992 | 455 |
| 0x09AEB0 | 0x9AEB0–0x9AEF2 | 66 | 21 |
| 0x327580 | 0x327580–0x327630 | 176 | 44 |
| 0x329D60 | 0x329D60–0x329D80 | 32 | 10 |
| 0x03DE90 | 0x3DE90–0x3DF3B | 171 | 54 |
| 0x06E070 | 0x6E070–0x6E143 | 211 | 56 |
| 0x03D3B0 | 0x3D3B0–0x3D4E2 | 306 | 88 |
| 0x327630 | 0x327630–0x327A88 | 1112 | 206 |
| 0x327490 | 0x327490–0x3274D9 | 73 | 20 |
| 0x327440 | 0x327440–0x32748A | 74 | 17 |
| 0x3274E0 | 0x3274E0–0x327573 | 147 | 34 |
| 0x327A90 | 0x327A90–0x327A93 | 3 | 2 |
| 0x327B40 | 0x327B40–0x327E1B | 731 | 177 |
| 0x342560 | 0x342560–0x3425C0 | 96 | 29 |
| 0x32A210 | 0x32A210–0x32A2B5 | 165 | 46 |
| 0x33AAB8 | 0x33AAB8–0x33AAC1 | 9 | 2 |
| 0x33C360 | 0x33C360–0x33C407 | 167 | 49 |
| 0x351650 | 0x351650–0x351B15 | 1221 | 298 |
| 0x354600 | 0x354600–0x35473C | 316 | 57 |
| 0x354F20 | 0x354F20–0x355259 | 825 | 204 |
| 0x033B10 | 0x33B10–0x33B29; 0x33B2A–0x33B41; 0x33B42–0x33B51; 0x33B52–0x33B61; 0x33B62–0x33B6E; 0x33B6F–0x33B7E | 105 | 26 |
| 0x0E8A20 | 0xE8A20–0xE8A23 | 3 | 1 |
| 0x328FF0 | 0x328FF0–0x329207 | 535 | 96 |
| 0x329210 | 0x329210–0x329465 | 597 | 126 |
| 0x329500 | 0x329500–0x329622; 0x329623–0x329639; 0x32963A–0x329649; 0x32964A–0x329659; 0x32965A–0x329666; 0x329667–0x3296AA; 0x3296AB–0x3296C1; 0x3296C2–0x3296D1; 0x3296D2–0x3296E1; 0x3296E2–0x3296EE; 0x3296EF–0x32972D | 547 | 143 |
| 0x333F60 | 0x333F60–0x33429B | 827 | 215 |
| 0x0F6CC0 | 0xF6CC0–0xF6D05 | 69 | 18 |
| 0x0F7170 | 0xF7170–0xF73C4 | 596 | 160 |
| 0x334410 | 0x334410–0x334507 | 247 | 60 |
| 0x32A310 | 0x32A310–0x32A8C4 | 1460 | 314 |
| 0x32D2E0 | 0x32D2E0–0x32D889 | 1449 | 311 |
| 0x32BFD0 | 0x32BFD0–0x32C089 | 185 | 43 |
| 0x32BD70 | 0x32BD70–0x32BFCD | 605 | 148 |
| 0x32B6E0 | 0x32B6E0–0x32B821 | 321 | 79 |
| 0x33DFA0 | 0x33DFA0–0x33DFB7 | 23 | 8 |
| 0x366A10 | 0x366A10–0x366A48 | 56 | 15 |
| 0x328F00 | 0x328F00–0x328F70 | 112 | 31 |
| 0x328F70 | 0x328F70–0x328FE4 | 116 | 31 |
| 0x33BFE0 | 0x33BFE0–0x33C09E | 190 | 42 |
| 0x32D190 | 0x32D190–0x32D1A1 | 17 | 5 |
| 0x329C70 | 0x329C70–0x329D55 | 229 | 50 |
| 0x33E0C0 | 0x33E0C0–0x33E166 | 166 | 37 |
| 0x3669C0 | 0x3669C0–0x366A02 | 66 | 18 |
| 0x334850 | 0x334850–0x3348D4 | 132 | 35 |
| 0x335440 | 0x335440–0x335626 | 486 | 129 |
| 0x335780 | 0x335780–0x335821 | 161 | 42 |
| 0x32B5A0 | 0x32B5A0–0x32B6AE | 270 | 64 |
| 0x3298B0 | 0x3298B0–0x329C65 | 949 | 237 |
| 0x33DFC0 | 0x33DFC0–0x33E031 | 113 | 27 |
| 0x332300 | 0x332300–0x332985 | 1669 | 363 |
| 0x332FD0 | 0x332FD0–0x333690 | 1728 | 420 |
| 0x333820 | 0x333820–0x33388D | 109 | 35 |
| 0x3339C0 | 0x3339C0–0x333E63 | 1187 | 246 |
| 0x33D5E0 | 0x33D5E0–0x33D8FD | 797 | 170 |
| 0x340D60 | 0x340D60–0x340DC9 | 105 | 29 |
| 0x33CB30 | 0x33CB30–0x33CB8C | 92 | 26 |
| 0x334C50 | 0x334C50–0x334D25 | 213 | 58 |
| 0x03DF40 | 0x3DF40–0x3E105 | 453 | 131 |


## Additional complete native bodies and checked data

The following bodies are decoded in the original assembly export without a named owning-function assignment. Every instruction over each stated entire span was compared; they are separate from the seventy-five owner totals.

| Native range | Bytes | Instructions | Selected role |
| --- | ---: | ---: | --- |
| 0x328320–0x328329 | 9 | 2 | Edit release receiver conversion and transfer to 0x3493B0 |
| 0x3280C0–0x328113 | 83 | 18 | Alternate complete selection/layout entry |
| 0x32B6B0–0x32B6D6 | 38 | 9 | Guarded additional logical-release route |

The original image's eight-byte table entries were read to establish the concrete Edit bindings: table 0x4775C8 offset zero to deleting owner 0x327490, +8 to actual draw owner 0x327630, +0x28 to constant-one owner 0x327A90, +0x38 to attachment 0x327AA0, +0x58 to left press 0x328120, +0x60 to release thunk 0x328320, +0x68 to alternate entry 0x3280C0, +0xA8 to motion 0x328330, +0xC0 to key owner 0x327E20 and +0xD0 to character owner 0x327B40. The selected +0xB0, +0xB8 and +0x130 entries point to return-only 0xE8A20. Selected inherited entries +0x118/+0x120 reach 0x32B5A0/0x32B6B0. The embedded two-entry point-map table at 0x477728 selects complete 0x32A310 and 0x32D2E0. These entries establish actual native targets without treating adjacent metadata as additional virtual slots.

The sixteen binary32 words at 0x42E630 were checked as the identity matrix. Other checked raw data are binary32 one, word 0x3F800000, at 0x4AC980; pick threshold multiplier word 0x3F333333 at 0x4328AC; binary32 1.25, word 0x3FA00000, at 0x42E71C; sign-bit word 0x80000000 at 0x4ACB30; and the four-component words 0x3F800000, zero, zero, 0x3F800000 at 0x438050. Only the selected sign-bit operand is given that role; neighbouring raw words are not generalized into another constant contract.

The two-byte literal at 0x43760C is code unit 0x0020 followed by zero. Selected callback-node construction uses original data at 0x477738 and directly defines the new payload as null in 0xF6CC0. Other integer widths, comparison destinations, call/store order, field offsets, numeric event identifiers and guarded admissions stated in this contract are in the byte-matched owner/body ranges.

The complete saved-file readback, exact UTF-8 byte length and artifact SHA256 are reported at handoff. Clean original-only exposure; prior held artifacts unread and unchanged. FINAL / STABLE / HELD ends all writes.
