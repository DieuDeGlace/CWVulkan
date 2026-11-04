# Original native object allocation spans

This bounded contract supports cubeworld.original.exe.unpacked.exe, an AMD64 PE32+ image of 5,757,952 bytes with preferred base 0x140000000 and SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. All routine and image addresses below are RVAs. Evidence is restricted to the original binary, original native function sources, assembly and call records. No game, probe or build was run. Offsets describe allocated objects or explicitly identified inline subobjects; they do not establish that every byte is initialized, meaningful, currently alive or readable without further lifetime facts.

## Allocator evidence and alignment limit

The ordinary object routes below call allocation entry 0x392bac with a size argument. That entry invokes CRT malloc with the requested size and includes the native new-handler/failure path. The inspected routes do not pass a separate alignment argument, align the returned pointer manually or use an aligned-allocation entry for these objects. The requested allocation size is the supported object-storage extent; allocator rounding or a larger backing heap block is not evidence for extra object bytes.

An exact numeric alignment guarantee of the external CRT malloc implementation is not established by this original-image audit. In particular, 16-byte or 32-byte alignment is not inferred from allocation size, nearby vector operations or the alignment machinery used for other large buffers. The object constructors contain ordinary pointer/scalar accesses and expect a valid allocation, but this note does not turn that expectation into an independently measured runtime alignment. Numeric allocator alignment remains an explicit limit.

## Creature allocation and inline pose-state end

Ordinary Creature construction requests 0x15f8 bytes and passes the returned base to constructor 0x4ce80. One direct assembly example is 0x2f42d5, which loads 0x15f8 before allocation call 0x2f42da and then reaches constructor call 0x2f42f0. The same request is established in the examined construction paths at 0x3e450, 0x7ea80, 0x9ea10, 0xa27b0, 0xf4490, 0x2c3710, 0x2e0db0 and 0x2e17d0. These include received-object creation, Game-created actors and other native simulation/copy paths. Their examined constructor calls do not use a larger allocation or move the Creature base into the middle of the allocation.

For these normal allocations, the supported storage interval is Creature+0x0000 through Creature+0x15f7 inclusive; the next address, Creature+0x15f8, is outside the requested allocation. Constructor 0x4ce80 establishes an actual touch at the upper bound: it creates the final list header at +0x15e8 and initializes its eight-byte count at +0x15f0, reaching byte +0x15f7. This is both a requested size and a highest established constructor touch, not a size guessed solely from the last convenient scalar.

Constructor 0x4ce80 invokes nested initializer 0x4d600 at Creature+0x10. It also invokes pose-state initializer 0x17010 with an inline destination at Creature+0xc38 and supplies the Creature base as the associated owner argument. Assembly at 0x4d144 through 0x4d14e establishes both arguments directly. The pose-state base is therefore inside the same 0x15f8 allocation, rather than a separately allocated object reached through a pointer at +0xc38.

The final established member of that pose state is a list header at state+0x8e0. Its sentinel pointer occupies +0x8e0 through +0x8e7 and its count occupies +0x8e8 through +0x8ef. In Creature coordinates these are +0x1518 through +0x151f and +0x1520 through +0x1527. Initializer 0x17010 creates this header; pose routine 0x17ab0 accesses it through state indices corresponding to these offsets; destructor 0x4d840 clears the elements and frees its sentinel. Thus the established inline state spans 0x8f0 bytes from Creature+0xc38, ending immediately before Creature+0x1528. Its dynamically allocated list nodes and sentinel are separate storage.

Creature+0x1528 is a distinct eight-byte owner pointer initialized to zero by 0x4ce80. It is not the end count of the pose-state list. The ordinary Game actor path in 0x7ea80 separately requests 0x300 bytes, constructs that record through 0x7ff20 and stores its pointer at Creature+0x1528. Native creation paths at 0x9ea10 and 0xa27b0 also show separate 0x300-byte construction. A null pointer is possible immediately after the Creature constructor; a 0x300-byte child must not be assumed for every Creature. Destructor 0x4d8b0 checks this pointer, invokes 0x4db70 and frees the separate allocation when present. The child's bytes do not extend the contiguous Creature span.

The full Creature allocation is not a fully initialized byte record. Constructor 0x4ce80 and its nested initializers set selected members and regions while leaving gaps. The inline pose initializer likewise initializes selected scalars, a 0x300-byte region at state+0x5bc and the final list header; it does not perform a complete zero-fill of its 0x8f0-byte span. Allocation bounds and high-offset constructor touches therefore do not establish initialized contents for every intervening byte.

The examined direct creation routes all use the same 0x15f8 request. No larger derived Creature allocation is established by this bounded research. This is not a proof that an unexamined indirect construction route or another image cannot introduce a variant. A pointer's resemblance to a Creature or a vtable value alone does not establish which live allocation it belongs to.

## Creature destruction and ownership

The original image entry at vtable RVA 0x431d58 points to deleting wrapper 0x4e0a0. Constructor 0x4ce80 writes that vtable address. The wrapper receives the object base in RCX and deletion flags in RDX, invokes destructor 0x4d8b0 and calls free on the original object base when the low flag bit is set. Destruction without that bit performs member destruction without freeing the top-level allocation. The wrapper's returned object address is not evidence that the destroyed object remains usable.

Destructor 0x4d8b0 destroys the inline pose state at +0xc38 through 0x4d840 and clears/frees many separately owned collections, including the final header at +0x15e8. It does not free Creature+0xc38 as a top-level object allocation. World destructor 0x29b290 traverses its object list at World+0x40 and invokes the first virtual entry of nonnull stored objects with deletion flag 1. Combined with the Creature vtable entry, this establishes a native owning deletion route for Creatures stored there; it does not establish that every Creature pointer found anywhere is owned by that list.

After member destruction, child pointers, sentinels and nodes can already be invalid even before the deleting wrapper releases the top-level Creature storage. An allocation size does not preserve those lifetimes or guarantee that a retained base pointer is still live.

## Ordinary Game

The ordinary startup route at 0x1355a0 requests 0x2288 bytes for Game. Assembly loads this size at 0x1362e9, calls allocation entry 0x392bac at 0x1362ee and supplies the returned base to constructor 0x7ea80 at 0x136331. The supported top-level interval is therefore Game+0x0000 through Game+0x2287 inclusive.

The upper bound is reached by an established constructor member: 0x7ea80 invokes 0x66970 for the inline object at Game+0x2278, and that initializer writes an eight-byte value at its +0x8, Game+0x2280 through +0x2287. The previous analogous inline object begins at Game+0x2268. This confirms a constructor touch to the allocation's last byte. It does not imply complete initialization of the full Game span.

Game constructor 0x7ea80 stores separately supplied World, world-map and Renderer pointers at Game+0x8, +0x10 and +0x18. The Renderer identity is independently established by the startup route's native create-renderer text, constructor call 0xc3150 and the transfer of its result into the fourth Game-constructor argument at 0x136325. These are pointer relationships, not inline copies of the three allocated objects.

Game also constructs a hosted wrapper inline at +0x6e8 through 0xf17a0 and a client object inline at +0xd18 through 0x3aa10. The hosted wrapper constructs a World at wrapper+0x8, hence Game+0x6f0. Its own constructor touches through wrapper+0x62f, including the count at +0x628. Those established inline touches occupy the 0x630-byte region beginning at Game+0x6e8 and preceding the client base. This is an inline storage relationship, not another heap request of 0x630 bytes, and does not by itself settle source-language inheritance versus composition.

Game's image vtable entry at RVA 0x436068 points to deleting wrapper 0x86630. That wrapper invokes destructor 0x818d0 and frees the Game allocation when the low deletion-flag bit is set. The destructor invokes the client and hosted-wrapper destructors on their inline bases, rather than freeing those interior bases as standalone allocations. It also performs native worker shutdown/join operations; their presence is not a universal proof that every external pointer or callback has ceased using the object.

Startup teardown in 0x1355a0 separately invokes deleting virtual entries for Game, Renderer, world map and World. Thus the separate supplied World/Renderer pointers must not be treated as extra contiguous bytes after Game. This note does not establish a complete shared-ownership model for all other Game pointer members.

## Standalone and embedded World

The ordinary standalone World request is 0x548 bytes. Startup assembly loads 0x548 at 0x13614e, allocates at 0x136153 and invokes constructor 0x29a130 at 0x136163. That constructor writes an eight-byte zero at World+0x540, reaching byte +0x547. The supported standalone interval is World+0x000 through World+0x547 inclusive; World+0x548 is outside the requested object storage.

The hosted constructor 0xf17a0 invokes the same World constructor at wrapper+0x8. The established 0x548-byte World subobject therefore occupies wrapper+0x8 through wrapper+0x54f, or Game+0x6f0 through Game+0xc37 on the examined Game path. The wrapper has additional storage after that subobject. Its outer extent must not be substituted for the World extent, and the separately supplied pointer at Game+0x8 must not be presumed to alias this embedded World.

The World constructor writes vtable RVA 0x472040, whose first original-image entry points to deleting wrapper 0x29c140. It invokes destructor 0x29b290 and conditionally frees the standalone object base when the low flag bit is set. World destructor 0x29b290 deletes owned objects and collections, releases separately allocated buffers and destroys inline critical sections. The hosted destructor 0xf1f20 instead invokes World destruction for wrapper+0x8 as part of its own member cleanup; it does not use the standalone deleting wrapper to free that interior World address. The hosted wrapper has its own vtable entry at RVA 0x43fc28 pointing to wrapper 0xf2f70, which conditionally frees the wrapper base after 0xf1f20. On the examined Game path, the hosted destructor is called directly on inline Game storage.

World's 0x548 allocation includes collection headers, pointer members and inline synchronization objects. The sentinels, nodes, vectors and owned objects reached through those members are separate allocations. The last pointer/capacity store at +0x540 and the allocation size do not establish that the entire World is zero-filled or that its owned data is contiguous with it.

## Readily identified ordinary Renderer

The ordinary Renderer is independently identified in startup 0x1355a0. It requests 0xa48 bytes at allocation call 0x1361e7 and invokes constructor 0xc3150 at 0x136219. That constructor first invokes 0xcbd10 on the same base, then writes the Renderer vtable at RVA 0x4385e8 and initializes additional members. This establishes a constructor/base-prefix relationship within the same allocation, not two adjacent heap objects.

Constructor 0xc3150 writes an eight-byte value at Renderer+0xa40, reaching +0xa47, so its highest established constructor touch matches the requested interval Renderer+0x000 through Renderer+0xa47 inclusive. It stores and uses pointers to external graphics resources and contains other inline state; neither these pointers nor the nested state establish a complete initialization map for the allocation. The first vtable entry at 0x4385e8 points to deleting wrapper 0xc3d80, which invokes destructor 0xc3c90 and conditionally frees the top-level Renderer base. That destructor releases resources and invokes nested/base cleanup. Other Renderer variants, interfaces and all resource-allocation spans remain outside this bounded contract.

## Limits of these boundaries

The strongest bounds here combine an explicit allocation request, a constructor using that returned base and a member touch reaching the same upper bound. They apply to those identified allocations and inline subobjects while live. They do not establish a universal safe memory-copy operation, a serialized object format, a complete initialized-byte map, or an exclusion protocol against concurrent destruction. Pointer targets, interior bases and top-level allocation bases have different ownership roles even when their numeric addresses are close. No raw-dump layout, capture policy or engine implementation follows from these allocation facts.

Status: FINAL / STABLE / HELD after readback.
