# Steam live-resident retention — final held contract

This bounded original-Steam-only contract describes the ordinary live-resident distance retention/removal at RVA 0x2A56C0, its reached World-step caller, the World list/map erasures, and the selected deleting-virtual cleanup. It concerns resident objects already present in World+0x40. It does not describe physical source-zone demand, source-map erasure or retained source-state restoration.

The supported image is binary/cubeworld.original.exe.unpacked.exe under C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Its base is 0x140000000, timestamp 0x5D93AB0D, and verified SHA-256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. All addresses below are RVAs in that image.

Original filenames were identified before scoped content reads. Principal evidence is decompiled/functions/1402a56c0.c, 1402da6c0.c, 1402ac530.c, 1402ac450.c, 14000f8c0.c and 1402a30c0.c. The ordinary resident vtable was resolved from the limited constructor prefix in 14004ce80.c and original image, then followed through 14004e0a0.c, 14004d8b0.c, 14004db70.c, 14004d840.c and 14004e7a0.c, with selected container helpers. Outer guard evidence is 1400f5520.c and decompiled/retry-180-phase1/functions/1400a5a50.c, plus guard wrappers 1402abb70.c and 1402b9890.c. Exact native decisions were checked in the individually selected original decompiled/assembly.txt listing. No earlier held file or implementation, reconstruction, SDK/dependency source, history, test, capture or log was used as behavior evidence.

## Ordinary admission and centre collection

The directly identified ordinary caller is World-step RVA 0x2DA6C0. Its byte test of World+0x358 at 0x2DA8CE admits the zero case to a sequence containing resident admission through 0x2E17D0 at 0x2DA8DF, live-resident retention through 0x2A56C0 at 0x2DA8E7, and the following update call at 0x2DA8F3. The retained-state meaning or every writer of World+0x358 is not inferred here. The retention routine itself does not repeat this World-byte test.

RVA 0x2A56C0 first traverses World+0x40 in forward list order. Every resident whose byte+0x60 is zero is copied as a raw pointer into a temporary centre list. The admission is at 0x2A5738, and the temporary-list append is at 0x2A574F. There is no health test, current-actor-pointer test, coordinate test or loaded-source test in this centre admission.

The routine then traverses the same World list again. A resident whose byte+0x60 is zero is skipped as a removal candidate at 0x2A57EC. Every nonzero value admits the resident to the distance tests against the collected centres. These are byte-zero and byte-nonzero operational categories; no full player, NPC, ownership or alive/dead label is assigned to them.

The World list payloads are dereferenced to read the byte before a null-pointer check at these collection stages. Ordinary valid resident-list payloads are therefore assumed. The later removal loop's null check does not establish safety for a null payload during either selection pass.

## Exact horizontal test and vertical independence

For each admitted nonzero-byte resident, the collected centres are tested in their forward list order. The first centre that passes retains the candidate immediately; later centres need not be inspected for that candidate. A removal candidate is collected only after every centre has failed. If the centre list is empty, every nonzero-byte resident is collected for removal. Zero-byte residents remain outside this candidate path.

The positions read for this retention calculation are the signed 64-bit X and Y fields at resident+0x10 and +0x18. When the centre list is nonempty, the candidate's X and Y are loaded once for its centre loop at 0x2A5802 and 0x2A5806; each tested centre is still reached by its saved pointer and has its coordinates read during that test. The temporary centre list does not capture coordinate values. The native calculation subtracts the centre's X from the candidate's X in 64-bit integer arithmetic at 0x2A581B, then converts that signed difference to double at 0x2A5822 and scales it by the double at image RVA 0x43EF58. That value is one divided by 65536, converting the fixed-point difference to blocks. The Y difference receives the corresponding treatment at 0x2A5838 and 0x2A583E.

The absolute-value mask was read from image RVA 0x4ACB10 as 0x7FFFFFFFFFFFFFFF. The X and Y magnitudes must each be at most the double 320 from image RVA 0x472220. The native comparisons at 0x2A582E and 0x2A584A reject that centre only when the respective magnitude is greater than 320. Equality reaches the subsequent tests.

If both component bounds pass, the signed differences are converted and scaled again as doubles. Their double squares are added, with the final sum compared at 0x2A5875 against the independent double literal 102400 from image RVA 0x4724A8. The decision at 0x2A587A treats a strictly greater squared sum as outside. Equality retains the resident. This is a horizontal disk with inclusive boundary, preceded by inclusive horizontal component bounds.

No coordinate flooring, integer-block narrowing, zone division or single-precision distance calculation is used in this retention test. The subtraction precedes double conversion. Extremely large signed differences, native integer overflow and floating-point precision outside ordinary representable positions are not given additional guarantees.

The vertical position field at resident+0x20 is not read by this retention distance calculation. There is no vertical subtraction, vertical bound or Z contribution to the squared sum in the inspected body. A candidate's vertical separation does not alter the result of these X/Y tests.

As arithmetic consequences for ordinary exactly representable positions, an offset of 320 blocks on one horizontal axis and zero on the other is retained by that centre. Horizontal offsets of 192 and 256 blocks are also exactly on the inclusive boundary. An offset of 320 on one horizontal axis and one block on the other fails the disk test. The same outcomes hold for any vertical field values because that field is not read. These examples are derivations from the inspected arithmetic, not native runtime captures.

## Candidate order and erasure order

The removal list is a second temporary forward list of raw resident pointers. A failed candidate is appended at 0x2A589A. Consequently, the collected removal order follows the second traversal's World+0x40 order. It is not sorted by identity, distance, source coordinate or pointer value.

Both temporary lists are ordinary lists; this routine adds no deduplication by pointer. The audit does not establish a safe outcome for malformed World lists containing duplicate pointers. It also does not treat the temporary pointers as reference-counted leases or a barrier against an untraced concurrent remover.

After candidate collection is complete, the routine processes removal-list entries in forward order. For each saved nonnull pointer, it starts a fresh forward search of World+0x40 for the first node whose resident pointer equals the saved pointer. On a match it unlinks that one list node, decrements the native 64-bit World+0x48 count at 0x2A5935, frees the list node at 0x2A5939, and stops that search.

This first match is a pointer-identity match. It is not a search by the resident's numeric identity. If no list node is found, the inspected path still proceeds to the map-erasure and deleting-virtual calls for the saved nonnull pointer; there is no return or renewed distance test based on list-search failure.

Next, at 0x2A5949, helper 0x2AC530 is called on World+0x450 with the 64-bit identity at resident+0x8. The helper locates the range equal to that identity using signed 64-bit comparisons and passes it to range eraser 0x2AC450. Matching map nodes are erased and released. The selected single-node eraser 0xF8C0 decrements the map's native count when nonzero; the whole-map clear path 0x2A30C0 resets its count to zero. For this owner, that count is World+0x458.

Map removal is therefore keyed by resident+0x8, after the pointer-matching World-list removal. It does not compare the map payload pointer against the saved candidate pointer before erasing an equal identity. No separate consistency guarantee among numeric identities, pointers and aliases is inferred.

Only after the list and identity-map operations does the routine call the resident's first virtual slot at 0x2A5959, supplying literal flag 1. The saved nonnull pointer is passed even when its earlier World-list search found no match. The routine then advances to the next collected candidate. It does not wait for a source-generation batch or defer this virtual call through a new work queue in the inspected path.

After candidate processing, both temporary lists' nodes and sentinels are freed. Their cleanup releases the temporary list storage; the centre-list cleanup does not delete the centre residents.

## Resolved ordinary deleting virtual and owned cleanup

For the ordinary resident class identified by constructor RVA 0x4CE80, the installed original vtable is at image RVA 0x431D58. Its first image pointer resolves to deleting wrapper RVA 0x4E0A0. This resolution used the original image entry and constructor prefix only; unrelated resident construction was not re-audited.

That wrapper calls destructor 0x4D8B0 at 0x4E0AF. With the supplied flag 1, it then releases the resident allocation through the native deallocation call at 0x4E0C1, whose supplied allocation size is 0x15F8. The normal return from destructor precedes allocation release. Other possible dynamic vtables were not exhaustively identified.

The selected destructor restores the ordinary vtable and performs resident-owned cleanup. It first destroys the secondary owned object pointed to by resident+0x1528 through 0x4DB70 at 0x4D8E5 and frees that secondary object. The inspected 0x4DB70 body destroys its own strings and collection storage; no wider gameplay identity is assigned to those members.

The destructor clears the resident-owned list at +0x15E8 and frees its list storage. Through helper 0x4D840 at 0x4D94C, applied to the resident subobject at +0xC38, it also clears and frees the list rooted at resident+0x1518. That selected helper frees list nodes and their sentinel; it does not show deletion of independently pointed-to resident objects stored as payloads.

Other directly selected cleanup includes the resident+0x9F0 vector through 0x4E7A0 at 0x4DAD5. Its reached helper 0x372C0 releases the nested buffers of its visited twenty-four-byte elements, then the outer vector storage is released and its three header pointers are zeroed. The selected list helper 0xCC140 likewise frees the nodes and sentinel of its supplied owned list. The destructor reaches additional owned vector and collection cleanup calls; this contract does not assign external spatial or ownership semantics to their offsets.

These observations establish owned-memory cleanup for the resolved ordinary virtual route. They do not establish that every external spatial index, another resident's reference, source-owned template relation, owner pointer or cached raw pointer has been cleared. No universal external-reference cleanup claim follows from freeing these owned containers. The source, lifetime and possible external aliases of such references remain outside the bounded evidence.

## Proved outer guard scopes

RVA 0x2A56C0 contains no critical-section acquisition or release of its own. The inspected 0x2DA6C0 route supplies no new local acquisition around this retention call. Protection on the following ordinary paths comes from their outer callers.

Wrappers 0x2ABB70 and 0x2B9890 were independently resolved to entry and exit of the critical section at the supplied World plus 0x3C8. In ordinary host update 0xF5520, that World is addressed at Host+0x8. The host enters its World+0x3C8 scope at 0xF68CD and leaves at 0xF69F8. Its reached World-step calls at 0xF6917 and 0xF692D, including the zero-World+0x358 retention route and the reached deletion, lie within that scope. Host+0x5F8 is also held by this inspected host-update body.

The host can call the World step more than once in one update while dividing its admitted elapsed input into sixteen-millisecond steps and a remainder. The retention call is reached from each such World-step invocation that passes its World+0x358 gate. There is no independent wall-clock retention timer in 0x2A56C0.

The ordinary Game update 0xA5A50 enters the World+0x3C8 scope through the World pointer at Game+0x8 at 0xA6464 and releases it at 0xADB23 on the inspected forward path. Its conditional World-step calls at 0xA7328 and 0xA734C also lie inside that scope. If they reach the zero-World+0x358 retention branch, the list/map removals and deleting virtual are therefore inside this caller's acquired scope. The broader Game conditions deciding whether those World-step calls occur were not re-audited.

These are observed scopes for the actual World arguments. They do not establish that the viewed and host World addresses always coincide, that another thread owns the same acquisition, or that every list/map writer follows this guard discipline. No World+0x3F0 acquisition spanning these resident-retention calls was proved by these selected caller scopes. That field is not acquired by the retention body itself.

## Separation from physical source retirement and limits

The distance predicate reads resident X/Y positions and byte+0x60 categories from World+0x40. Its directly visible residency removals affect World+0x40, World+0x450 and the reached resident allocation. Loaded source-map presence, source demand-set membership, source mesh state and source coordinates are not inputs to the inspected retention predicate.

This is a live-resident lifetime boundary. It does not establish physical source-zone erasure, a call to source retained-state recording, or a change to the required source set. A retained or removed live resident does not, from this evidence alone, imply any corresponding source-zone retention or removal decision.

Only the stated ordinary caller and resolved ordinary deleting route were closed. Untraced direct or indirect callers, other dynamic vtables, malformed or duplicate resident lists, concurrent position/category changes, external spatial or ownership aliases, shutdown ordering, exception paths and alternate source writers remain unresolved. In particular, the temporary raw pointers and the proved outer guard scopes do not establish a general source-lifetime or race-freedom guarantee.

FINAL / STABLE / HELD after complete final readback. This is a new bounded original-only contract. Every earlier held file remains unchanged. Exposure status: clean; standalone author policy and individually selected original Steam evidence only. Further writes stop after readback.
