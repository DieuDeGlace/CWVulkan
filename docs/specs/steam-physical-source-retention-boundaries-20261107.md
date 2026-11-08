# Steam physical source-retention boundaries — final held contract

This bounded original-Steam-only contract establishes the ordinary physical source-removal calls to retained-state helper RVA 0x2D3E20, their ordering relative to loaded-map erasure and source-object destruction, the fresh-source restoration and installation boundary in RVA 0x2AE0C0, and the directly established World+0x3C8 scopes in the inspected viewed and host callers. It does not replace any earlier held file or assert a universal locking or source-lifetime rule.

The supported image is binary/cubeworld.original.exe.unpacked.exe under C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Its base is 0x140000000, timestamp 0x5D93AB0D, and verified SHA-256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. All addresses below are RVAs in that image.

Reference filenames were identified before scoped reads. Principal evidence is decompiled/functions/1402d2410.c, 1402d2670.c, 1402d2930.c, 1402d3e20.c, 1402ae0c0.c, 1400842d0.c, 140084f70.c, 1400f2340.c, and 1400f5520.c; ordinary viewed-update evidence is decompiled/retry-180-phase1/functions/1400a5a50.c. Directly selected lookup, erasure, map-copy, lock-wrapper, constructor, deleting-wrapper and caller-wrapper files are identified by their RVAs below. Exact call boundaries were checked in the individually selected original decompiled/assembly.txt listing. The source vtable's first entry was also read from the original image. No earlier contract, replacement implementation, reconstruction, SDK or dependency header, test, capture, log or project history was used as behavior evidence.

## What the retention boundary means

RVA 0x2D3E20 is the reached in-memory retention routine. It uses the source's signed 32-bit zone coordinates at source+0x10 and +0x14 to find or create selected retained records in World+0x10. Its selections include the two flagged arrays, retained edit records, the collection at source+0x4D8, and the scalar at source+0x490. Its per-field admissions can leave some fields unchanged or avoid creating a record. Calling this routine is not a claim that the complete source object, raw generated terrain or every source-owned collection is preserved.

The routine itself neither reads World+0x358 nor acquires World+0x3C8 or +0x3F0. Its caller supplies the directly observed protection scopes. The relevant completion boundary is its normal return after the reached selected copies and writes. This is not a disk-durability, transport, exception-recovery or asynchronous-completion guarantee.

Loaded sources are reached through World+0x460, whose list sentinel is stored at +0x468 and native element count at +0x470. The erasure helper RVA 0x2ABD20 adjusts the selected bucket/list links and count and frees the map node. It does not invoke the source object's deleting virtual. A saved source pointer can therefore remain usable by the subsequent explicit calls even after that map node has been freed on the inspected path.

The ordinary source constructor RVA 0x2F9400 installs the original vtable at image RVA 0x4738C0. The image resolves its first entry to deleting wrapper RVA 0x2FA3B0. That wrapper calls destructor RVA 0x2F9940 at call site 0x2FA3BF and, when bit 0 of its supplied flag is set, releases the source allocation at 0x2FA3D1. The inspected physical-removal paths call this first virtual with literal flag 1. This resolves their destruction boundary independently of a decompiler function label.

## Viewed one-source retirement retains before unlinking

RVA 0x2D2410 first enters World+0x3C8 at 0x2D243D to sample the current actor position and leaves that acquisition at 0x2D2511. It then acquires World+0x3F0 at 0x2D2534 and traverses the loaded-source map to choose its one eligible retirement candidate. Null source payloads are skipped by the candidate scan. The candidate selection and removal remain inside this +0x3F0 acquisition.

For a selected node, it saves the source pointer and enters World+0x3C8 at 0x2D25E9. The nonnull source is passed to retention helper 0x2D3E20 at call site 0x2D25FA. Only after that call returns does the path erase its map node through 0x2ABD20 at 0x2D260E. Thus retention completes before loaded-map unlinking on this particular path.

It releases its +0x3C8 acquisition at 0x2D261A and its +0x3F0 acquisition at 0x2D2627. The saved nonnull source then receives the deleting virtual call at 0x2D2654. Source-object destruction is after retention, map erasure and both releases. If no source is selected, this call does not occur.

There is no World+0x358 test around this retention, erasure or deleting call. The retention call is admitted by the selected nonnull source, not by that World byte. The nested acquisition order at the removal stage is +0x3F0 followed by +0x3C8; the earlier position-sampling acquisition of +0x3C8 was already released before +0x3F0 was acquired.

The ordinary viewed mesh worker RVA 0x842D0 reaches this retirement call at 0x8456B, up to eight invocations in its inspected loop pass. Its earlier World+0x3C8 sampling scope, entered through 0x2ABB70 at 0x84360, ends through 0x2B9890 at 0x8451D before those retirement calls. Consequently, no acquisition from that earlier caller scope spans the deleting virtual. The remover's own acquisitions are the directly proved scopes at this boundary. The worker's running-byte and global-pause admissions are separate from World+0x358; the inspected caller does not add a test of World+0x358 around these calls.

## Host required-set retirement unlinks before retaining

The ordinary host source-loader RVA 0xF2340 calls required-set remover RVA 0x2D2670 at 0xF2718. That remover scans the loaded map while holding World+0x3F0 and constructs a local candidate list containing map-node pointers for sources whose coordinate pair is absent from its supplied required set. Its source-coordinate reads presume nonnull source payloads on this inspected scan; it does not have the null-payload skip seen in 0x2D2410.

The initial +0x3F0 scan acquisition ends at 0x2D284F. During later candidate processing, it reads the source pointer from a saved candidate node at 0x2D2864, then reacquires World+0x3F0 at 0x2D286F and World+0x3C8 at 0x2D287C. The saved raw-node interval and that pre-acquisition pointer read are directly present. They are not proof that an independently executing alternate eraser cannot invalidate the candidate. No universal ownership or race guarantee is inferred from this local list.

For each reached candidate, 0x2ABD20 erases the loaded-map node at call site 0x2D2892. The saved source pointer is then passed to 0x2D3E20 at 0x2D289D. Here map erasure and map-node release precede retention. Retention still precedes source-object destruction: the saved object has not received its deleting virtual when the retention routine reads and copies its selected fields.

After retention returns, this path releases its +0x3C8 acquisition at 0x2D28A9 and +0x3F0 at 0x2D28B6. It then tests the saved source pointer and calls the deleting virtual at 0x2D28CC when nonnull. The retention call itself precedes that null test; the scan and ordinary path rely on a valid source. No missing-source safety is invented for an invalid candidate.

This required-set path also does not test World+0x358 before retention or deletion. Its order is therefore different from one-source retirement: unlink, retain, release the acquired guards, then destroy. Both normal paths complete retention before destroying the source allocation, but only the one-source path completes retention before unlinking its loaded-map entry.

In the inspected host loader, World+0x3C8 is entered at 0xF23D0 for the initial player/state snapshot and released at 0xF24DE before required-set removal. The caller therefore does not hold that initial acquisition across 0x2D2670. Later in the same ordinary loader cycle, fresh-source factory 0x2AE0C0 is called serially at 0xF2CBE. Required-set retirement returns before this later generation loop is reached. This establishes the inspected host call order, not an exclusion of other callers or concurrent World writers.

## Viewed worker exit clears the map before retaining its saved sources

The viewed worker's running-byte exit route calls whole-map remover RVA 0x2D2930 at 0x84F2F. The inspected route reaches it without an outstanding World+0x3C8 acquisition from the worker's earlier sampling scope.

RVA 0x2D2930 enters World+0x3F0 at 0x2D2955. It creates a local map copy through RVA 0x29C840 at 0x2D299F, whose reached range-copy helper is 0x299D40. Node-copy helper 0x295D70 copies the coordinate key and raw source-pointer value into new map nodes; it does not duplicate the source object. It then clears the live loaded-map links and count, frees the live map nodes and resets the map's bucket arrangement through 0x29CA00 at 0x2D29DA. Its +0x3F0 acquisition is released at 0x2D29E7.

Only afterwards does it traverse the local entries. For each nonnull saved source it enters World+0x3C8 at 0x2D2A0E, calls retention at 0x2D2A1B, and releases +0x3C8 at 0x2D2A27. The deleting virtual follows at 0x2D2A3E. Thus the live map has already been cleared before these retained-state calls, but each normal retention call completes before that source's destruction. This routine does not hold +0x3F0 during those later per-source retention calls and does not test World+0x358 at those boundaries.

This is a directly reached worker-exit route, not a claim about every shutdown sequence. Another direct retention caller was identified at call site 0x29B303 in RVA 0x29B290. Its external lifetime and caller protection were not established in this bounded ordinary-path audit.

## Fresh generation, conditional restoration and final map installation

RVA 0x2AE0C0 first looks up the coordinate pair in World+0x460 through RVA 0x12700 at call site 0x2AE10F. A present node with a nonnull source returns that existing pointer directly. This fast return precedes the World+0x358 test and performs no fresh generation or retained-state application.

An absent entry or a present null source payload takes the fresh-source path. After seeding the reached random-state facility, it allocates the source, calls constructor 0x2F9400 at 0x2AE16A, and calls raw source generation RVA 0x2FF6F0 at 0x2AE179. The generator's internals were not re-audited here; the ordering is established at this original call site. This contract concerns ordinary successful construction, not allocation failure, exceptional unwinding or partial generation recovery.

Only after raw generation returns does the factory test the one-byte World+0x358 field, at RVA 0x2AE17E. A nonzero value branches at 0x2AE186 to the final installation stage and skips the entire retained-store lookup and application. A zero value permits the coordinate lookup in World+0x10. An absent retained record then also proceeds to installation with the generated state.

For a found retained record, the inspected restoration order is the first selected array and its source flag, the second selected array and its source flag, the retained edit-list copy and forward edit replay, the source+0x490 scalar, then the source+0x4D8 collection. The array copies are reached at 0x2AE279 and 0x2AE293, the edit-list copy at 0x2AE2B1, each edit replay through 0x31C1C0 at 0x2AE2D8, and the final collection replacement through 0x509E0 and 0x6F000 at 0x2AE304 and 0x2AE315. These reached restorations are complete before the fresh pointer is published in the loaded map.

The factory does not acquire its own World+0x3F0 and +0x3C8 guards until after generation and optional restoration. It enters +0x3F0 at 0x2AE321, then +0x3C8 at 0x2AE32E. Through RVA 0x299640 at 0x2AE34A it obtains an existing coordinate node or inserts one, then writes the fresh source pointer into that node's payload at 0x2AE354. This is the final loaded-map installation boundary. It releases +0x3C8 at 0x2AE35F and +0x3F0 at 0x2AE36C before returning.

The initial lookup, raw generation and optional retained-store restoration precede these factory-owned acquisitions. The final node acquisition can find a coordinate node that already exists; the factory's final pointer store has no repeated nonnull-payload admission test corresponding to its initial fast return. This establishes the local sequence only. It does not establish which alternative installers can run, whether duplicate work occurs in practice, or how any untraced caller serializes such cases.

The viewed source-loader RVA 0x84F70 enters World+0x3C8 at 0x85060 for its initial sampling and leaves at 0x85157 before generating its selected batch. Its ordinary batch dispatcher at 0x855D7 reaches wrapper RVA 0x7D5C0. The direct single-record case calls the factory at 0x7D618; the selected original task body RVA 0x858E0 also reaches the same factory at 0x85994 and 0x85C27. Those inspected wrappers do not add a World+0x3C8 acquisition spanning the factory's initial restoration stage. The host loader likewise released its initial snapshot acquisition at 0xF24DE before its direct factory call at 0xF2CBE. No wider scheduler or cancellation audit is implied.

## Ordinary update scopes and limits

Original wrappers RVA 0x2ABB70 and 0x2B9890 were resolved to entry and exit of the critical section at their World argument plus 0x3C8. The ordinary Game update 0xA5A50 enters through the former at 0xA6464 and leaves through the latter at 0xADB23 on the inspected ordinary path. Its World argument is reached through Game+0x8.

The ordinary host update RVA 0xF5520 enters that guard for the World at Host+0x8 at 0xF68CD and leaves at 0xF69F8. Its separate Host+0x5F8 acquisition is not the World+0x3C8 guard. Neither this update scope nor the viewed update scope is attributed to a different loader invocation merely because it concerns the same logical world. Protection can be compared only when the actual World addresses coincide; no new alias or cross-thread ownership assumption is supplied here.

The inspected removers serialize their selected retained-store writes under their acquired World+0x3C8 scopes. The inspected factory applies retained state before acquiring its own insertion guards, and its ordinary inspected loader callers have already ended their initial sampling scopes. These concrete observations do not establish that every access to World+0x10 or every raw source reference is protected by +0x3C8.

World+0x358 has asymmetric admission in these inspected boundaries: it does not gate the reached physical-retirement retention calls, but nonzero skips fresh-factory restoration after raw generation. This contract assigns no broader local-host, remote-client or networking label to that byte, and does not audit all its writers.

All ordering claims refer to ordinary normal-return paths. Untraced direct or indirect callers, source-map writers, retained-store writers, source-pointer aliases, synchronization outside the selected call chains, exceptional exits, shutdown joining and the raw generator's internal protection remain unresolved. No general race-freedom, retry, atomic retire-and-restore transaction, source residency duration or persistence guarantee is inferred.

FINAL / STABLE / HELD after complete final readback. This is a new bounded contract; every earlier held file remains unchanged. Exposure status is clean: the standalone author policy and individually selected original Steam evidence only. Further writes stop after readback.
