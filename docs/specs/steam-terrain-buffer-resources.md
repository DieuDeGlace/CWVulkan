# Steam terrain buffers: active Zone-list data contract

Independent native-only supplement to
[terrain mesh bands](steam-terrain-mesh-bands.md),
[scene mesh-cache publication](steam-scene-lighting.md#1-cached-zone-mesh-fields-readiness-and-capture-coherence)
and [ordinary buffer resources](steam-sprite-buffer-resources.md).
Only original native evidence and independent prose were read. No SDK, engine,
exporter, test, sister implementation, owner output/capture or game execution was
used. This establishes a bounded active-list view, not full World/Zone classes or
a completed independent capture adapter.

## 1. Image identity and ownership scope

Addresses are RVAs in the analyzed original unpacked AMD64 executable
`cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256
`7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`.
This identifies the analyzed image, not all distributions or modified builds.

The input to this view is a presently valid client **Zone pointer**, with its
World identity/lifetime established separately. This note does not replace
loaded-World enumeration or specify the World hash-container layout. It also
does not identify loaded zones with the renderer's currently selected zones.

Zone constructor **0x2f9400** constructs the mesh-cache member at **Zone+0x1c0**
through **0xc25c0**. That member owns separate active and pending group sequences.
Zone+0x1c0 is not itself the active-list sentinel pointer.

## 2. Exact active/pending sequence fields

| Zone offset | Established native meaning |
| --- | --- |
| +0x1c8 | Eight-byte pointer to the **active-list sentinel** |
| +0x1d0 | **Unsigned64 active-group count** |
| +0x1d8 | Eight-byte pointer to the **pending-list sentinel** |
| +0x1e0 | **Unsigned64 pending-group count** |

Each sequence is a doubly linked list. Its node layout, demonstrated by
allocation **0x86f60**, pointer insertion **0x6e4d0**, producer **0xe9530** and
consumer **0xe5070**, is:

| Node offset | Content |
| --- | --- |
| +0x00 | Eight-byte next-node pointer |
| +0x08 | Eight-byte previous-node pointer |
| +0x10 | Eight-byte pointer to a **separately allocated mesh group** |

The observed node allocation is **0x18 bytes**. This is an allocation size, not
a stride for finding another node in memory. The group payload is a pointer,
not an embedded group record.

Constructor 0xc25c0 creates distinct empty sentinels and zeros both counts.
An empty sentinel's next/previous links point back to itself. Its payload has no
group meaning. Native ordinary terrain consumer **0xe5070** first tests the whole
active-count qword for nonzero, starts from the sentinel's next node, follows
next links, and terminates on equality with the sentinel pointer. It does not
advance by an allocation stride, terminate on null or use the count as the loop's
iteration limit. No node color/nil-byte interpretation belongs to this list.

Inconsistent counts/links, null group payloads, corrupted sentinels and incomplete
construction are not assigned a recovery contract here.

## 3. Mesh group and directly reached vertex wrapper

Producer **0xe9530** requests **0x60-byte mesh groups**, constructing them through
**0xc2650**. Their native table is **0x4382b0**. The limited established view is:

| Group offset | Meaning |
| --- | --- |
| +0x08 | Eight-byte pointer to the **shared terrain vertex-buffer wrapper** |
| +0x40 | 32-bit shared vertex count written by meshing |
| +0x5c | Signed32 vertical band base, in blocks |

The group allocation size is not a list-element stride. Nor is this group a
sprite resource: there is **no intervening sprite object** between group+0x08
and the vertex wrapper. Remaining index-category fields are outside this minimal
view. Their presence does not turn each group into one independently drawable
opaque mesh under every pass.

The ordinary terrain producer obtains this shared wrapper through the backend
at **World+0x70 → catalog+0x08**, invoking backend virtual+0x08. For the reached
ordinary backend this is **0xc2c90**, the same vertex-buffer factory established
in the ordinary buffer contract. Consequently the wrapper's table is
**0x438390**, **wrapper+0x08** is the eight-byte immediate-context pointer, and
**wrapper+0x10** is the eight-byte **ID3D11Buffer interface pointer**. Neither
group+0x08 nor the wrapper itself is that COM interface or a CPU vertex-byte
address.

Ordinary terrain draw **0xe5070** reaches the shared wrapper's virtual+0x08 before
the admitted category draw. On this backend, **0xc2a70** binds slot 0's vertex
buffer with stride **8 bytes**, offset 0, and returns byte 1 unconditionally.
It is not a getter, GPU-fence poll or independent buffer-creation validation.
The distinction between wrapper data field+0x10 and its no-op virtual slot+0x10
remains [buffer resources §§2–4](steam-sprite-buffer-resources.md).

## 4. Band population and active publication

After resource preparation, 0xe9530 appends a pending group only for an allocated
band whose shared vertex-resource pointer is nonnull. It visits allocated bands
in ascending order, preserving their signed band bases. Empty omitted bands do
not make the remaining bases dense list indices. The established 250-block
partition and count/base arithmetic remain in the terrain-band contract; they
are not redefined here.

The mesher installs pending groups while holding the demonstrated World mesh
critical section. It does not directly replace the active sequence. Published
active count therefore describes actual list groups, not all allocated bands or
all loaded Zone contents. A loaded Zone can have no active groups; pending data
can coexist with an earlier active mesh.

Worker **0x842d0** performs the reached publication sequence:

1. **0x7dc20** copies the previous active group-pointer sequence into temporary
   list nodes. It copies pointers rather than moving the original list or adding
   group ownership references. This copying precedes the active-assignment lock.
2. Under the World mesh critical section, **0x7cc20** assigns the pending
   group-pointer sequence into the active list. It can reuse nodes while changing
   their payloads, allocate additional nodes and free surplus active nodes.
3. After releasing that critical section, the worker destroys old groups through
   the temporary pointer sequence, then clears pending nodes and their count.
   Clearing those pending nodes does not destroy the new groups now referenced
   by the active list.

The directly reached branch requires **pending count nonzero**. A zero pending
count does not by itself clear an existing active list. Resource preparation's
virtual+0x10 calls are the concrete no-op wrapper methods already specified;
their presence does not prove a GPU-completion barrier.

## 5. Proven native lock identity and lifetime boundary

The demonstrated mesh lock is an **inline Windows CRITICAL_SECTION at
World+0x3c8**, not a pointer stored there. Native helper **0x2abb70** passes that
address to imported **KERNEL32 EnterCriticalSection**; **0x2b9890** passes the
same address to **LeaveCriticalSection**. Both helper bodies and native imports
were checked. These are actual critical-section calls, not a guessed mutex or
an invented lock field.

Mesher 0xe9530 uses that critical section for pending installation, and worker
0x842d0 uses it for the active-list assignment described above. It is the
demonstrated mutual-exclusion boundary for those structural mutations.
**Reading only a list header/count under this lock and then traversing the list
unlocked is insufficient to establish a coherent read.** The known assignment
can change payloads and free nodes after the header read; old group objects can
then be destroyed after publication releases the lock. The observed native view
provides no stable handle, copied geometry or automatic reference retention.

This mesh critical section alone does not prove the Zone remains alive. The
directly traced eviction **0x2d2410** uses **World+0x3f0** for its loaded-zone
container traversal and nests World+0x3c8 for the selected entry's removal. It
destroys the removed Zone after releasing those locks. This is evidence for a
separate Zone-lifetime boundary, not a universal locking policy for every Zone
removal, reset or external mutation. Outer draw/capture callers and every other
lifecycle route were not exhaustively audited here.

Group cleanup **0xc2770**, reached through **0xc2860**, invokes cleanup on its
nonnull buffer wrappers. Mesh-cache cleanup **0xc2680** destroys groups from both
active and pending sequences, then releases their nodes and sentinels. Underlying
buffer Release/reference limits remain in
[buffer resources §5](steam-sprite-buffer-resources.md#5-replacement-empty-output-and-lifetime).
Numerical Zone, node, group, wrapper and buffer pointers gain no post-unlock
lifetime merely from being observed.

This contract does not establish malformed-data recovery, coherent concurrent
snapshots outside the demonstrated boundaries, all resource/failure paths, CPU
readback, complete World/Zone/group classes or a runtime-oracle result. Loaded
World enumeration remains a separate boundary; no World hash-container schema
is required or supplied by this active-list supplement.
