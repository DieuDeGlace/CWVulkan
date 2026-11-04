# Steam host and AI actor-protection scopes

## 1. Scope and distinct owners

This bounded native-only supplement extends the protection/lifetime facts of [World creature views](steam-world-creature-views.md) without changing that finalized note. It traces the ordinary host tick, reached host packet mutations, AI phases, host generation worker and normal shutdown joins. It does not provide a complete generic snapshot/quiescence API or a complete inventory of every nested Creature-field writer.

The supported original AMD64 image is `cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256 **7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244**. All addresses are RVAs. Only original native/decompiled evidence and independent prose were inspected; no SDK, engine, exporter, tests, owner outputs/probes, reconstruction implementation or game execution was used.

Let G be the Game, H the host and W the host's World:

|Address/field|Established meaning|
|---|---|
|G+0x6e8|Host object H, embedded in Game|
|H+0x08, equivalently G+0x6f0|The **in-place host World W**; this address is not a World-pointer field to dereference|
|G+0x08|Pointer to the separate client/rendered World|
|H+0x5f8|Inline outer host CRITICAL_SECTION|
|H+0x5d0|Inline loopback-buffer queue CRITICAL_SECTION|
|W+0x3c8, equivalently H+0x3d0|Inline World guard used by the host simulation and AI phases|
|W+0x3f0, equivalently H+0x3f8|Inline World loaded-zone guard used by AI terrain/path work|
|W+0x418, equivalently H+0x420|Another initialized inline critical section; no actor-lock role is established here|
|G+0x768, equivalently W+0x78|AI-state object; its first eight-byte field points to W|

Host constructor **0xf17a0** constructs W at H+0x08 through 0x29a130. It initializes H+0x5d0 and H+0x5f8 with spin count 16 at **0xf191e/0xf192e** before launching its generation worker. World construction 0x29a130 constructs AI state at W+0x78 through **0x9590**, storing the supplied W pointer followed by its separate AI-map member. The AI map is not the World+0x450 creature registry. Its established record semantics remain [creature-ai.md §§0–1](creature-ai.md).

The host World has its own registration list/tree and ordinarily no local-player pointer, while its connected kind0 players are registered creatures. Client and host actor collections must not be substituted for one another. The generic World container/node layouts remain the finalized creature-view contract.

World constructor 0x29a130 initializes W+0x3c8, W+0x3f0 and W+0x418 through the native Windows critical-section API at **0x29aeeb/0x29aefd/0x29af0f**, each with spin count 16. Helpers **0x2abb70/0x2b9890** enter/leave W+0x3c8; **0x2abb80/0x2b98a0** enter/leave W+0x3f0. Their established API identity does not give the third section an actor-protection role.

## 2. Host tick: outer guard versus later World guard

The Game-owned host loop **0x83e60** uses G+0x2068 as its run byte. Its admitted tick calls **0xf5520(H,elapsed)**; its remote-client/idle branch sleeps instead. That idle choice is not a stop/join barrier for the separate AI or generation worker.

In 0xf5520 the reached normal order is:

|Phase|Actual protection and ordering|
|---|---|
|Tick entry|Enter **H+0x5f8 at 0xf5582**|
|Network receive/dispatch|Dispatch received channel1 packets through **0xf4490 at 0xf5646**, while the outer host guard is held|
|Loopback detach|Additionally enter **H+0x5d0 at 0xf5734**, detach/copy the pending queue, then leave that queue guard at **0xf5787**|
|Loopback dispatch|Dispatch the detached records through **0xf4490 at 0xf57b9**, with H+0x5f8 still held and H+0x5d0 released|
|Invite/send/disconnect work|Continue under H+0x5f8; this precedes the simulation World-guard acquisition|
|Apply gathered events and update W|Additionally enter **W+0x3c8 through 0x2abb70 at 0xf68cd**|
|World batch/common tail|Keep both guards across event application, World substeps, the whole-interval drawn/linked pass, zone/object publication, reset/output handling and distribution/clearing|
|Normal tick exit|Leave **W+0x3c8 through 0x2b9890 at 0xf69f8**, then **H+0x5f8 at 0xf6a00**|

This establishes the reached host-to-World acquisition order **H+0x5f8 before W+0x3c8**, and reverse release. The loopback lock is a separate earlier nested queue scope; holding it does not retain actors after records have been detached. Exact event/substep/output semantics remain [netcode roles §2](netcode-roles.md#2-the-host-tick-0xf5520) and [daily reset/output boundaries](steam-world-daily-reset.md).

**The pre-simulation packet phase is not covered by the later W+0x3c8 acquisition.** In particular, host packet3 at **0xf487d** performs lookup-before-register under this caller's H+0x5f8 scope. 0xf4490,0xe720 and 0x29d0d0 do not add a W+0x3c8 acquisition around that path. Connected-guest removal reached through 0xf3cf0 likewise calls the ordinary actor remover 0x2d2260; the host tick's disconnect work at 0xf6897 is still before 0xf68cd. Simulation births/reset mutations occur later under both outer host and World guards. A World-only guard is therefore not a demonstrated exclusion for every host registry mutation.

Conversely, the AI does not take the outer host guard around its actor work. A host-only guard is not demonstrated exclusion for AI changes to actor state. These statements concern the exact distinct addresses above, not two names for one lock.

For contrast, the finalized client-frame scope is **0xa6464 →0x2abb70** through **0xadb23 →0x2b9890**, on G's separate client World. It includes the World batch/drawn/local-control work and the later **0xaaebf →0x3f4f0 →0x3e450** inbox application. The client receive thread 0x3c920 instead queues buffers through 0x3bae0 under the client object's **+0xa18** section; the frame detaches that queue under its own lock before dispatching. That buffer lock is not a creature-registry guard or a retained-actor lifetime mechanism. These facts are restated only to distinguish the established client frame from the host phases, not to change the finalized creature-view contract.

## 3. AI thread: registry scans, private work and escaped target reads

The Game-owned AI loop **0x83f20** also tests G+0x2068 and calls **0x13830 at 0x83f66** with **G+0x768**. Through the AI state's stored World pointer, that reached loop operates on the host World W, not G's client-World pointer. This binding follows the actual in-place offsets and 0x9590 constructor, not a generic assumption that all AI helpers use the same World.

The outer AI loop itself takes no host critical section. The reached 0x13830 phases are:

|Phase|Reached lock boundary and actor access|
|---|---|
|Initial registration-list pass|Enter **W+0x3c8 at 0x138bc**, traverse W+0x40 in registration order, copy player/actor facts into AI-owned state and call behavior helper 0x14e00 for admitted actors; leave at **0x13aff**|
|Per-record goal/target sampling|Iterate the separate AI map in its signed-ID order; for the reached target path, enter **W+0x3c8 at 0x13bd9** and resolve/sample the target from the World ID tree|
|Resolved-target branch|Leave **W+0x3c8 at 0x1400e**, then perform the target-size/height uses detailed below|
|Other target branch|The corresponding alternate release is **0x140bc**|
|Terrain/path phase|Enter **W+0x3f0 at 0x140c4**, operate on AI/path/terrain data, then leave at **0x148b2**|
|Final registration-list pass|Enter **W+0x3c8 at 0x1492a**, traverse the current W+0x40 list again and apply AI outputs; normal release at **0x14ce2**|

The first/final actor scans use the actual World registration sequence, including their native predicates. The generic helper admits authoritative World actors; on a mirror it restricts actor work to the local pointer. For AI action/application, the ordinary additional filters include positive HP, nonpositive stun and kind/bot-state rules; player-position collection has its separate kind0 path. These predicates are not substitutes for the registration-list membership or synchronization contract.

Concrete protected writes include behavior changes made by 0x14e00 during the first guarded phase, and, in the final guarded phase, Creature+0x128 bit 0x80, movement intent+0x40/+0x44/+0x48, climb ability state, and queued interaction-list work at Creature+0xab8. Final application looks up the AI record for the currently visited creature. AI-record/path buffers also change between these actor-guarded phases; their being owned under W+0x78 does not make every write to them a W+0x3c8 transaction. Full AI decisions remain the existing AI spec rather than a new algorithm here.

The **resolved target Creature pointer is actually used after the World guard is released**. After 0x1400e, **0x14061/0x1406f** pass target+0x84 as the dimensions input to 0x2b7ad0, and **0x14081** reads its float height at target+0x8c for the subsequent position adjustment. The terrain/path guard is acquired only later at 0x140c4. These uses are not merely copied scalar snapshots taken while the target was locked, and no target lifetime reference/pin is acquired here. This is a concrete reason not to turn the phase diagram into an all-target-fields/lifetime guarantee.

The AI releases W+0x3c8 before taking W+0x3f0 on these paths, and releases W+0x3f0 before the later W+0x3c8 pass. It does not demonstrate a continuously nested pair across the whole AI tick, nor an outer H+0x5f8 scope. Initialization/deletion of W+0x418 does not establish a missing AI/actor guard; no such role is assigned to it.

## 4. Host generation worker and other worker limits

The host constructor's additional worker is distinct from the Game-owned host tick and AI threads. Its private run byte is **H+0x550**, initialized 1, and its owned native thread object begins **H+0x558**. The constructor launch/handshake reaches **0xf3220 →0xf2340**; the worker payload carries H. It is not stopped merely by relabeling G+0x2068 as a common host run flag.

In 0xf2340 the actor-window collection phase acquires **W+0x3c8 at 0xf23d0** and releases it at **0xf24de**. It walks **H+0x48, which is W+0x40**, follows registration links, selects kind0 players, and copies the appropriate position/window coordinates into worker-owned records. Its alternate ability 0x9c anchor rule remains the independent source-window contract. This collection is not a retained array of Creature pointers that independently keeps the original actors alive after release.

The worker later reaches zone work through **0x2d2670 at 0xf2718** and **0x2ae0c0 at 0xf2cbe**, as well as further World-state transfer scopes at **0xf2d04–0xf2d5e**. Its direct body does not acquire H+0x5f8 or call the common actor registration/removal primitives. That does not assert that every transitive zone/helper effect has no actor relationship. The loaded-zone lifetime/locking and publication contracts remain separate; in particular, reached zone insertion has its own **W+0x3f0 then W+0x3c8** scope in [loaded-zone enumeration](steam-loaded-zone-container.md).

On seeing H+0x550 clear, this worker still performs its exit state-copy/save tail: **W+0x3c8 at 0xf2eee**, copy through 0xf1c90, release at **0xf2f0d**, then 0x2d38e0 and owned temporary cleanup. The loop flag being clear is not evidence that this final access has finished. A successful join is the later completion boundary.

This bounded audit does not inventory every Game map/chunk/source/audio worker's nested field access. Their observed joins below are lifecycle evidence, not a blanket statement that all their work uses one actor mutex. The already traced client frame guard and queued receive-buffer scope remain [creature views §6](steam-world-creature-views.md#6-reached-protection-scopes-and-snapshot-limits).

## 5. Shutdown and successful-join lifetime boundaries

Game deleting-destructor entry **0x86630** reaches **0x818d0**. That destructor performs earlier cleanup calls, then writes the common Game run byte 0 at **0x8191c**. This is a stop request, not an immediate wait for a currently executing AI/tick/worker phase. The following successful ordinary wait sequence is directly reached; labels below are the original native wait messages, not SDK type names:

|Order|Game-owned thread object|Join site / native label|
|---:|---|---|
|1|G+0x1fe8|0x8194f through 0x9af20; music|
|2|G+0x1fd8|0x8198c through 0x9af20; AI|
|3|G+0x1fa8|0x819be through 0x9af20; generation|
|4|G+0x1fb8|0x819f0 through 0x9af20; chunk|
|5|G+0x2018|Conditional inline _Thrd_join at 0x81a55; host|
|6|G+0x1fc8|Conditional inline _Thrd_join at 0x81ad6; map|
|7|G+0x1ff8|Conditional inline _Thrd_join at 0x81b57; receive|
|8|G+0x2008|Conditional inline _Thrd_join at 0x81bd8; send|

Helper **0x9af20** calls imported **_Thrd_join**, reports invalid/nonjoinable/self-join/error cases through its native error paths, and clears the owned thread handle/ID on success. The conditional inline joins also clear their thread state on success. This note describes the successful ordinary path, not recovery from those errors. A wait message or stop-byte write alone is not the completed join.

The host-tick loop has its own final action: after leaving the G+0x2068 loop, **0x83e60 calls 0xf4ff0 at 0x83f10** before returning. 0xf4ff0 enters **H+0x5f8 at 0xf501d**, performs its connection/output cleanup, briefly nests H+0x5d0 and releases that queue lock at 0xf5371, then calls **0x2d22e0(W) at 0xf537b** before releasing H+0x5f8 through its 0xf53a0 tail. It does not acquire W+0x3c8 in this reached cleanup body.

**0x2d22e0 destroys all registered nonnull creatures, frees the registration nodes, clears the ID-tree nodes/count and empties the separate W+0x50 pointer list**, retaining the constructed container headers. It does not merely mark actors dead for a later tick. Thus an actor pointer can become invalid during host-thread exit **before that host join returns**, even though W's containing allocation still exists. The fact that the Game destructor calls the AI join before the host join does not order all independently executing exit work: the host may enter its final clear while the main thread is still waiting elsewhere. This trace does not establish a cross-thread quiescence barrier preceding that clear.

After the common worker waits, Game teardown reaches the client destructor **0x3b100 at 0x8221d**, then the host-member destructor **0xf1f20 at 0x8222a**. Host destruction writes **H+0x550=0**, then joins its separate generation worker through **0x9af20 at 0xf1f78**. Only afterward does it delete **H+0x5d0/H+0x5f8 at 0xf1f85/0xf1f93**, clean its remaining owned state and call embedded World destructor **0x29b290 at 0xf2334**. Therefore this successful destructor path waits for that worker's exit-tail World access before destroying W. It does not preserve previously cleared actors until World destruction.

No destructor/read guard here increments a World/Creature lifetime reference for an observer. Client destructor internals, exceptional shutdown and all externally supplied callbacks are not elevated into a general lifetime guarantee by this call-order evidence.

## 6. What the observed scopes do and do not establish

- H+0x5f8 covers the named host dispatch/send/cleanup phases, including registry writes that occur before the tick takes W+0x3c8. It does not by itself block the independently running AI or generation worker.
- W+0x3c8 covers the named World-update, AI actor and worker snapshot phases, but is not acquired by every reached host registry mutation. It is not a replacement name for H+0x5f8.
- The host tick's reached nested order is **host guard then World guard**. The AI's World/zone phases are separated releases/acquisitions. Zone insertion's own zone-then-World scope is separately documented. No single universal order for every possible lock combination is inferred.
- These named guards exclude other work that cooperates with the same scopes. They do not turn AI-private path buffers, escaped target pointers, every nested Creature allocation or an already-entered destructor into one immutable transaction.
- A header/count read followed by unlocked traversal does not retain nodes. Copying Creature pointers does not retain the creatures. A held lock does not, from this evidence alone, keep its owning H/W allocation alive through destruction.

The ordinary main/client frame's World guard therefore cannot be generalized into a universal host capture guard, and an outer host guard alone is also insufficient for all AI state. The documented combination of named critical sections describes which reached work they exclude; it is **not a proved complete generic capture/quiescence rule** for arbitrary asynchronous snapshots or every nested actor field. Stopped flags, idle host ticks, absent local player, empty queues and zero elapsed time do not independently establish that rule.

## 7. Explicit remaining scope

The note closes the actual host/AI/worker owner bindings, named lock phases and normal successful-join/destructor ordering. It leaves the complete transitive field-writer census, all external/indirect entrypoints, every zone/AI helper's retained-reference lifetime, allocator/exception failures and modified-build behavior unproved. No role was invented for W+0x418, no snapshot API or repaired synchronization policy was supplied, and the finalized creature-view note was not changed. Only this new independent prose contract was written.
