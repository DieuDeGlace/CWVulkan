# Edit attachment: display clipping field identity

FINAL / STABLE / HELD after the complete saved-artifact readback reported with this contract. Independent original-source research, 2026-11-09.

## Finding and supported authority

The integer written by attachment owner RVA 0x327AA0 through the parent node's +0x68 receiver, its array pointer +0x168 and signed index +0x134 is the original registered property named clipping in the selected concrete display-state class. It is separate from that same receiver's property named visibility, whose integer array is at +0xE0 and signed index at +0xAC. The property names are established by original construction and registration operands, not inferred from adjacency or a generic UI convention.

The authority is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, 5,757,952 bytes, AMD64 0x8664, preferred base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Identity and hash were independently checked in the current original-only research run. Addresses below are RVAs in this image.

Only docs/CLEANROOM_AUTHOR.md, the source-side assignment and individually selected original executable/export evidence were read. No active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, implementation, tests, results, captures, owner outputs/data, combined history, SDK/dependency/reconstruction source, packages or prior held handoff was accessed. No implementation or runtime feedback was supplied, requested or received. No additional agent was used. Prior held artifacts remain unread and unchanged.

## Exact attachment receiver and ordered behavior

Let E be the receiver of 0x327AA0, N the node pointer at E+0x178, PN the parent pointer at N+0x40, and D the display-state receiver at PN+0x68. The write concerns the parent's D. It does not concern the caption object, the child's transform receiver at N+0x60, the GUI manager, or an arbitrary field at the same numerical offset on another object.

The complete attachment body proceeds as follows.

First it reads the shape pointer at N+0x58. A nonnull pointer whose signed 32-bit tag at shape+0x70 equals 3 is retained; otherwise null is selected. The resulting 64-bit pointer is stored at E+0x1A8. It then calls 0x328DF0 on E.

That complete clamp helper stores eight zero bytes over E+0x1C4/+0x1C8 and returns when E+0x1A8 is null. Otherwise it bounds the signed 32-bit caret at E+0x1C4 against zero and the selected caption string's low signed 32-bit length. Caption selection uses signed index at caption+0xD4, 0x20-byte string records through caption+0x108, and the length DWORD at record+0x10. It then checks the signed caret-plus-selection endpoint using E+0x1C8. A negative endpoint changes selection to negative caret; an endpoint above length changes caret to length minus selection. It does not independently normalize every selection count, and no oversized/malformed-input arithmetic guarantee follows.

After the clamp returns, 0x327AA0 tests current E+0x178 for nonnull, then its parent N+0x40 for nonnull, then the parent's shape pointer PN+0x58 for nonnull. Only those three guards admit the parent property store. It loads D through PN+0x68, loads the signed 32-bit index D+0x134, sign-extends it, loads the 64-bit array pointer D+0x168 and stores signed 32-bit one at that selected element. There is no independent null or bounds test for D, its array or index. The parent need not have shape tag 3; that earlier tag test applied to the child node's shape.

The owner then rereads E+0x178, obtains that node's transform receiver through +0x60, uses the transform's own signed 32-bit index +0xAC and array pointer +0xE0 at an eight-byte element stride, and copies two 32-bit words into E+0x1BC then E+0x1C0. These are different receiver and element-width operands from the parent's four-byte display-property arrays. In particular, this last transform +0xE0 read cannot identify the parent's +0x168 property.

The initial N and shape read occur before the later null-node guard. The last N/transform read occurs even if a parent-property guard failed. Accordingly the guards are conditional admission to that one parent store, not whole-function null safety. The only explicit call between initial caption publication and the parent test is the complete non-callback clamp helper. Concurrent mutations and alternate callers are not assumed absent; there is no lock or lifetime pin in these bodies.

## Concrete property declaration and registration

0x3736C0 constructs the selected concrete 0x338-byte display-state object. It calls its base initialization, installs original table 0x47C170, constructs the property headers and storage below, initializes the other declared properties and bookkeeping, registers the five property headers, then returns the same object. The original ASCII data at 0x47C188 is visibility, ten bytes; at 0x47C198 it is clipping, eight bytes. Both raw literals were checked in the image, as were the complete constructor instructions that pair those names with the member receivers.

| Declared member of D | Original name | Integer property header | Current signed index | 64-bit integer-array pointer | Constructor elements |
| --- | --- | --- | --- | --- | --- |
| First property | visibility | D+0x70 | D+0xAC | D+0xE0 | Two 32-bit elements initialized to one |
| Second property | clipping | D+0xF8 | D+0x134 | D+0x168 | Two 32-bit elements initialized to zero |

Each header is constructed through 0x34A420 with the corresponding original name string. That helper copies the supplied byte-string name into header+0x40, initializes the header's selected-index word at +0x3C as part of its eight-byte zero store at +0x38, initializes its other state and returns the header. The constructor replaces each integer property's table pointer with 0x479470, clears its vector's three 64-bit words, and calls 0x485D0 with element count two and a pointer to the corresponding 32-bit default value. The four-byte vector stride is independently present in that complete resize helper.

The arithmetic relationship of the header and storage is part of the ownership proof: header+0x3C and header+0x70 are precisely the index and array fields in each row. The two arrays and two indices belong to different declared members; they are not alternate names for one current integer.

The constructor also declares the original fill, stroke and blurRadius properties. It registers visibility, clipping, fill, stroke and blurRadius in that order through 0x368FA0. The complete registration helper appends the supplied property pointer to the owning receiver's linked property list at +0x38, increments its 64-bit count at +0x40 and links the new record, with the reached original list-length error boundary. This gives original property ownership and registration evidence without assuming an uninspected file format or serializer. General property animation/state writers are not closed by this small contract.

## Node ownership and selected construction/copy routes

0x33CD30 prepares a node from a manager, transform, shape, display receiver, optional parent and name. A missing transform takes the selected 0x328-byte transform construction route. A missing display receiver takes a 0x338-byte allocation and 0x3736C0 construction. It then allocates a 0xC8-byte node and reaches 0x32EE60 with these receivers; a nonnull supplied parent reaches 0x32F640 afterward. Allocation/runtime behavior is a boundary, including the explicit null-allocation routes in the owner.

The complete node constructor 0x32EE60 stores the shape at node+0x58, transform at node+0x60 and display receiver at node+0x68. It initializes parent/child/control/flag and registry state, and registers the node with the supplied nonnull shape, transform and display receiver. Thus default ordinary node construction supplies the very display-state class whose declared members are identified above. It does not prove that every alternate externally supplied fourth component has that concrete class.

The complete sharing-copy route 0x333890 passes the source node's existing +0x60/+0x58/+0x68 receivers to 0x33CD30 and recursively copies permitted child nodes. Its later flag/maps/component virtual-copy operations do not rename the display declaration. This selected route shares the supplied display receiver rather than constructing a guessed new property layout.

The separate deep-copy node route 0x3308E0 calls the source display receiver's virtual slot +0x10 when nonnull, then passes that returned display receiver to 0x33CD30. For the concrete table 0x47C170, raw table entry +0x10 points to 0x373DE0. The complete concrete copy owner calls 0x33CAA0, whose allocation/constructor route reaches 0x3736C0, then pairs property-header and vector copies at visibility offsets +0x70/+0xE0 and clipping offsets +0xF8/+0x168. It separately copies stroke, fill, blur and its final scalar. The corresponding original header-copy helper 0x34A7C0 copies selected header state and reconstructs its keyed auxiliary records; it is not evidence that all header bytes or both index DWORDs are copied. In particular this contract makes no claim that deep copying preserves the source's current signed +0x3C index.

This traces the selected concrete default/sharing/deep-copy ownership, while leaving other virtual component classes and untraced node loaders unresolved. Attachment itself does not check D's dynamic type. The declared clipping identity applies to the proved concrete layout and its selected original construction/copy routes; it is not inferred for arbitrary malformed or substituted receivers.

## Selected consumers distinguish clipping from visibility

The complete native hit owner 0x333F60 first reads the current visibility integer through node+0x68, array +0xE0 and signed index +0xAC. Zero rejects the node before its hit/child work. It also tests node flag bits 2 and 12; their distinct admission is not folded into the visibility property.

Later, after the owner establishes its own-hit result through the reached transform/shape/control boundaries, it reads the clipping integer from that same display receiver through array +0x168 and signed index +0x134. A zero clipping integer admits child traversal regardless of the own-hit result; a nonzero clipping integer admits it only when the own-hit result is nonzero. An admitted child search traverses in reverse linked order and returns the first successful child; the node's own return remains subject to query flags and its own-hit result. Therefore attachment's store of one selects the latter clipping-dependent hit-traversal condition. It does not turn visibility on.

The rest of that selected owner prepares transforms and possible parent composition, calls the original shape/control query boundaries, handles the independently gated rectangle test and final query-flag tests. Their detailed provider numerics and rendering/scissor effects are outside this field-identity contract. The literal clipping property name and this reached hit consumer are proved; a universal statement about every draw backend or clip rectangle is not.

Constructor default initialization and attachment's conditional write are selected clipping writers. Default visibility initialization and the hit owner's independent early visibility test are the selected contrast here. This is not an exhaustive writer/consumer census, and no earlier menu or editor handoff was used to fill it.

## Conditional examples

If the selected concrete parent display currently has visibility zero and clipping zero, a successful parent-store admission writes clipping one while leaving visibility zero. The hit owner's visibility-zero early return still applies; the attachment write does not make the parent visible.

If visibility is nonzero, a child's own-hit computation is false and clipping is zero, the selected hit owner can still search that node's children. With the same inputs and clipping one, that child search is skipped. All shape/query/provider returns in this example are conditional inputs, not original runtime observations.

If the child has no tag-3 caption shape, the caption pointer becomes null and the clamp zeroes the two caret/selection DWORDs. A nonnull parent shape can nevertheless admit the clipping-one store afterward. Conversely a missing parent shape skips the clipping store while the final child-transform pair copy still occurs.

## Original correspondence and evidence limits

Reference filenames were discovered before scoped reads. Evidence was restricted to the supported original executable, decompiled/assembly.txt, original functions/function_ranges metadata and the individually selected decompiled/functions/140 plus six-digit RVA .c files. Original assembly operands and raw data, rather than decompiler naming guesses, establish the field identity.

All instruction bytes of the fourteen complete selected owners in the manifest below were compared to their raw-backed image bytes using the original PE section mapping. Every byte matched. Exported noncontiguous ranges are retained separately; endpoints are exclusive. This verifies complete selected instruction correspondence, without pretending to be a universal call graph or all-class proof.

| Complete owner RVA | Original range(s), exclusive ends | Matched bytes | Instructions |
| --- | --- | ---: | ---: |
| 0x327AA0 | 0x327AA0–0x327B39 | 153 | 36 |
| 0x328DF0 | 0x328DF0–0x328EB2 | 194 | 42 |
| 0x3736C0 | 0x3736C0–0x37379E; 0x37379F–0x3737AE; 0x3737AF–0x3737BE; 0x3737BF–0x3737CB; 0x3737CC–0x37386B; 0x37386C–0x37387B; 0x37387C–0x37388B; 0x37388C–0x373898; 0x373899–0x373949; 0x37394A–0x373959; 0x37395A–0x373969; 0x37396A–0x373976; 0x373977–0x373A1E; 0x373A1F–0x373A2E; 0x373A2F–0x373A3E; 0x373A3F–0x373A4B; 0x373A4C–0x373AF7; 0x373AF8–0x373B07; 0x373B08–0x373B17; 0x373B18–0x373B24; 0x373B25–0x373BE9 | 1301 | 324 |
| 0x34A420 | 0x34A420–0x34A4C5 | 165 | 46 |
| 0x368FA0 | 0x368FA0–0x369010 | 112 | 29 |
| 0x33CD30 | 0x33CD30–0x33CED3 | 419 | 117 |
| 0x32EE60 | 0x32EE60–0x32F052 | 498 | 114 |
| 0x3308E0 | 0x3308E0–0x330A78 | 408 | 115 |
| 0x333890 | 0x333890–0x3339BD | 301 | 75 |
| 0x373DE0 | 0x373DE0–0x373EE3 | 259 | 49 |
| 0x333F60 | 0x333F60–0x33429B | 827 | 215 |
| 0x0485D0 | 0x485D0–0x486A1 | 209 | 62 |
| 0x33CAA0 | 0x33CAA0–0x33CAEC | 76 | 22 |
| 0x34A7C0 | 0x34A7C0–0x34A926; 0x34A927–0x34A92D; 0x34A92E–0x34A934; 0x34A935–0x34A93B; 0x34A93C–0x34A942; 0x34A943–0x34A9F9; 0x34AA00–0x34AAF1 | 805 | 222 |

The raw checks also covered visibility/clipping name bytes at 0x47C188/0x47C198 and the concrete display table entry at 0x47C180 pointing to 0x373DE0. Constants directly material here are shape tag 3, signed 32-bit stored one, integer defaults one/zero, two default elements, four-byte integer stride, eight-byte transform-pair stride, 0x20-byte caption records, 0x338-byte display allocation and 0xC8-byte node allocation. Other constructor/default/provider constants were not assigned a broader meaning.

No OS/font/imported-provider guarantees, arbitrary index/null safety, hidden clipping writer, rendering behavior or concurrency/lifetime protection is inferred. The exact dynamic type of an arbitrary parent supplied through an alternate path remains unproved. The selected original property declaration, concrete ownership route, attachment write and hit-consumer distinction are closed independently.

After complete saved-artifact readback, byte equality and SHA256 reporting, this artifact is FINAL / STABLE / HELD. All writes to it stop at HELD. All preceding held artifacts remain unread and unchanged.
