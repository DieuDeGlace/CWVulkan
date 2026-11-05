# Original Zone and region-role cleanup lifetimes

This bounded ownership contract supports cubeworld.original.exe.unpacked.exe, AMD64 PE32+, 5,757,952 bytes, preferred base 0x140000000, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Addresses below are RVAs. Evidence is restricted to the original image and original native function, assembly and call records. No game, probe or build was run.

The callable Zone cleanup entries and the ordinary published owner are established. This audit does not establish that a normally generated but unpublished temporary Zone is safe to dispose of in every generation path. No inspected pre-publication edge proved that the Zone pointer escapes into a World queue, worker or cache, but absence of such an escape throughout generation and its transitive callees remains unproved. Destructor existence alone does not answer that lifetime question.

## Zone cleanup ABI and local ownership

Destructor 0x2f9940 takes the live, fully constructed Zone destination in RCX under the Windows x64 convention. Its examined direct body requires no coordinate pointer, World argument, deletion flag or caller-provided output buffer. It destroys owned state without freeing the top-level Zone allocation. No meaningful status/result return is established. A zero pointer, already destroyed destination, arbitrary zero-filled buffer or partial construction is not an established valid input.

Deleting wrapper 0x2fa3b0 receives the Zone base in RCX and deletion flags in EDX. It invokes 0x2f9940, then frees the original base when the low flag bit is set. Assembly at 0x2fa3ba through 0x2fa3d1 establishes this order and flag test. Its normal return in RAX is the original address, not a live-object guarantee or a success flag. With the low bit clear, member destruction still occurs; it is not a no-op. The normal create route uses a 0x58518-byte allocation for this Zone type.

The destructor writes the native Zone vtable and frees numerous collections, sentinels and buffers. It destroys the inline array of 0x1000 elements with a 0x58-byte stride at Zone+0x518 through element destructor 0x6cc40. These are real member cleanup operations, not merely a top-level free.

One independently traced owning edge is the collection rooted at Zone+0x460. Helper 0x119530 registers a separately allocated child pointer there, keyed by the child's eight-byte identifier at child+0x38. Replacing an occupied entry invokes 0x118d30 on the previous nonnull child and frees it. Zone destructor 0x2f9940 traverses this collection and likewise invokes 0x118d30 and frees each nonnull child before destroying the collection nodes. This establishes local ownership on those paths; it does not identify those children as ordinary World-owned Creatures.

Near generator call site 0x318de4, original assembly passes the Zone base and a generated child pointer to 0x119530. The preceding path allocates and constructs that child through 0x118a60. Thus the destructor covers a concrete kind of storage created during generation, beyond constructor-only sentinels. The identical numeric offset in a different object must not be confused with World+0x460: Zone+0x460 is this local child collection, while World+0x460 is the publication collection discussed below.

The examined direct Zone destructor body does not use the World pointer stored at Zone+0x08 to remove a publication entry. It does not establish a general detach operation that first removes the Zone from every external owner. Its nested destructors release additional storage, but this bounded audit does not prove that every nested cleanup is free of external references or callbacks. Once cleanup starts, borrowed member pointers may be invalid before the top-level allocation is freed. Repeated cleanup is not established as valid or idempotent.

## Ordinary creation and publication

Creator 0x2ae0c0 takes a World and two signed four-byte coordinate components. It first searches the World collection rooted at +0x460. If it finds an existing nonnull Zone pointer, it returns that pointer; the return is therefore not universally a new allocation owned by the immediate caller.

On the creation path, the routine reseeds the CRT RNG, allocates 0x58518 bytes, invokes Zone constructor 0x2f9400, invokes generator 0x2ff6f0, and applies additional native state before its explicit publication section. That section enters World+0x3f0, then World+0x3c8, obtains the coordinate-keyed collection entry and stores the generated Zone pointer at entry+0x18. It releases +0x3c8 before +0x3f0 and returns the Zone pointer. This is an established publication order on that route, not evidence that the preceding construction and generation were protected by those same explicit sections.

World destructor 0x29b290 traverses the collection whose sentinel is stored at World+0x468 and invokes the first virtual entry of nonnull pointers at node+0x18 with deletion flag 1. This supplies an ordinary owning deletion route for Zones published by 0x2ae0c0. The World teardown can then destroy its container storage. Direct Zone destruction is not shown removing that owner's stored pointer before freeing a Zone independently.

The examined ordinary create route has no normal-return branch that generates a new Zone, abandons publication and destroys that temporary Zone. It ordinarily returns either an existing published pointer or the newly published one. No native precedent for discarding a fully generated unpublished Zone was established here. Allocation, constructor or generator exceptions are a different issue; this note does not supply a complete unwind or rollback contract for them.

Creator 0x2ae0c0 is reached by native parallel generation work. For example, 0x7d5c0 contains a direct call for a small range and dispatches larger ranges through native concurrency machinery. Other direct callers include 0x858e0 and hosted generation routine 0xf2340. This establishes a worker context for ordinary creation, but it does not prove that the particular Zone or its children are submitted to another worker before publication. It also does not establish universal exclusion against another creator of the same coordinates.

## Bounded generation and escape findings

Both available original decompilation attempts for 0x2ff6f0 report timeouts. The original assembly and call records remain available and were used for selected ownership edges. This audit did not recreate the generator or establish a full transitive pointer-retention census across its roughly 0x18000 bytes of native code and many calls.

The inspected assembly establishes that the generator receives the Zone base, loads its associated World through Zone+0x08, and invokes World-oriented spatial queries. It also constructs a temporary region-role object through 0x115df0 at 0x2ffba5 and later destroys that same temporary through 0x82450 at 0x318f79. Calls near the generator's end include 0x31d470 and 0x31d700 with the Zone base. The inspected bodies rebuild or modify Zone-local state, including the header at Zone+0x4b8 and spatial data associated with Zone+0xe8. Those offsets are not World event-queue offsets on these calls.

Helper 0x119530 and its destructor counterpart establish the local child ownership described above. The inspected call and assembly windows did not establish a direct store of the Zone pointer into a World event queue or a worker record before 0x2ae0c0's explicit publication store. Neither that finding nor the Zone-local lists proves the absence of another escape through an unexamined generator branch, helper, callback, cache or shared record.

Generation is also not side-effect-free merely because its Zone has not yet been published. The ordinary creator calls srand before construction, and the generator's original call records include numerous rand and srand calls. World queries and allocations occur before publication. Their complete transitive mutation and reference-retention behavior is unresolved. Global RNG mutation is an established side effect, but is not itself proof that a pointer into the temporary Zone escapes.

Consequently, 0x2f9940/0x2fa3b0 establish how native owned storage is released for a valid Zone under the ordinary lifetime assumptions. They do not establish an unconditional temporary-generation/disposal operation. Proving that operation would additionally require the relevant generation paths' external-reference and ownership facts. This note neither assumes those facts nor concludes that an escape definitely exists.

## Destination-only region-role destructor

Entry 0x82450 takes one constructed region-role destination in RCX. It does not require the original coordinate pair, a separately supplied World pointer, a deletion flag or an output destination. Its examined body uses the constructed object's own members and prepares any additional scratch arguments required by nested cleanup. No meaningful return value or success/status channel is established.

It destroys the collection headers at destination+0x10, +0x20, +0x60 and +0x70 and releases vector storage associated with +0x30, +0x48 and +0x90. It does not free the destination base, nor does its direct body destroy the World stored at destination+0x00. The caller remains responsible for its own destination storage. Some vector fields are zeroed during release; freed collection headers are not thereby reconstructed as valid empty members. A second destructor call is not an established operation.

The normal interval is directly shown in 0x2b4260: it constructs fresh stack storage through 0x115df0, consumes the returned object through 0x128400, invokes 0x82450 on the original destination and then returns the separately saved scalar result. Assembly at 0x2b42b8 through 0x2b42c2 supplies only the destination in RCX for destruction. The World/coordinate inputs are not cleanup arguments. Routine 0x2bb290 similarly constructs a temporary, obtains a scalar through 0x129190 and destroys it before returning. These normal intervals support destination-only cleanup after successful construction and completed use.

The generator's 0x2ffba5-to-0x318f79 temporary interval is another established normal-use cleanup pair. Its presence does not make an outstanding pointer into the role object's owned collections valid after cleanup. Constructor exceptions, partially built objects, aliases retained by arbitrary consumers and concurrent use are outside the established normal-return interval. No universal lifetime or locking guarantee is inferred from the stack storage or destructor ABI.

Status: FINAL / STABLE / HELD after readback. Safe disposal of an arbitrary generated unpublished Zone remains unproved within the explicit bounded scope above.
