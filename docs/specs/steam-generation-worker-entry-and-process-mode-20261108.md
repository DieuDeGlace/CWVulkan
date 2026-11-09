# Generation worker entry, carried time samples, and process byte

This independent, code-free original-side contract covers Game worker RVA 0x84F70, its reached constructor/thread handoff, the timing and protection scopes around its periodic operation, its bounded lifetime route, and the process-wide byte actually tested by this worker. All addresses below are RVAs in the supported original x64 Steam image.

The supported file is binary/cubeworld.original.exe.unpacked.exe beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244; Machine is 0x8664, image base 0x140000000 and PE timestamp 0x5D93AB0D. These were checked against the executable and original export metadata. Forty-two selected native instruction sites covering the thread handoff, initialization, clocks, gates, carried sample assignment, shutdown and distinct process-byte operands were compared with file-backed executable bytes and matched. The relevant thread dispatch table was also read from this executable.

Only the standalone source-author policy and filename-selected original executable, native/decompiled routines and original metadata were read. No project implementation, exporter, tests, SDK/dependency sources, startup/memory/history, owner results/captures, or earlier held handoffs were read. Prior held artifacts were not changed. This document gives no implementation guidance.

G denotes the Game object captured by this worker. W denotes the World at G+8. The local entity read by the worker is the pointer at W+0x448. These operands establish the selected owner; they do not identify every possible caller or World instance.

## Concrete thread entry and startup order

The ordinary Game constructor is RVA 0x7EA80. It uses 0x7D560 to allocate an eight-byte capture containing G. At 0x7F80F it invokes thread-start helper 0x762D0 with this capture. On normal return the resulting sixteen-byte thread record is copied to G+0x1FA8 at 0x7F83C; its identifier is at G+0x1FB0. The constructor terminates if the destination thread identifier was already nonzero.

The start helper creates a condition variable and mutex, locks the mutex, publishes a dispatch object, and calls the imported thread-start operation at 0x76390. The dispatch object's executable table is at RVA 0x437EB8. Its first pointer is to the native jump at 0x87BF0, which targets 0x89160. The generic thread trampoline 0x3C7D0 invokes the first dispatch pointer, then reaches the imported thread-exit condition broadcast operation after that call returns.

RVA 0x89160 takes ownership of the eight-byte capture by clearing its slot in the dispatch object. It locks the startup mutex, writes the one-byte handoff flag to 1 at 0x8919B, signals the condition variable at 0x891A3, unlocks the mutex at 0x891BB, and only then calls worker 0x84F70 at 0x891D2. The constructor-side helper waits for that handoff flag before its cleanup and return. Nonzero reported condition/mutex/thread-start results follow the displayed error-throw paths; this contract describes the normal successful route.

This handoff flag means that the captured argument has been transferred. It is set before the worker's first time sample. It is not a worker-clock-ready, first-generation-complete, or periodic-save-complete signal. After the unlock, the worker may obtain its first samples before or after the constructor-side thread helper finishes returning; the source does not choose that interleaving.

Several startup operations statically precede this worker's launch:

- The UI aggregate at G+0x118 is initialized through 0xF8690 at 0x7EB1E.
- The Game lifetime byte G+0x2068 is written to 1 at 0x7EF63.
- G+0x206C is initialized to 123. G+0x2070 is initialized and assigned the nonempty narrow string “world” through the call at 0x7EFA9.
- At 0x7F279, World configuration 0x2B5F20 receives W, that seed and G+0x2070. Its beginning stores W+0x220 and copies the supplied string to W+0x200 through 0x337F0 at 0x2B5F93. The later name-length test in the worker is W+0x210. The constructor's selected preceding configuration is therefore named “world”; the worker baseline does not precede that configuration call.
- Roster loading precedes the UI construction call 0xF9AC0 at 0x7F4D4. That selected UI routine constructs the roots held at G+0x118, +0x130 and +0x170. At their construction sites their selected indexed four-byte values are initialized to 1, 0 and 0 respectively. The value is reached through root+0x68, selected index +0xAC, and integer-array pointer +0xE0. No screen name is inferred solely from these offsets.
- The local entity is created and published to W+0x448 before the optional selected-character load at 0x7F71A and its following derived/UI work.
- The first of the constructor's shown worker-start calls, at 0x7F7C2, precedes the generation worker's start at 0x7F80F. Other starts, including the one at 0x7F879, follow it.

All of those preceding constructor calls have returned before the selected start helper can reach this worker. That does not prove that every value is unchanged at every later worker read, or that no other thread has run meanwhile.

The selected outer startup caller 0x1355A0 constructs the World and other owners before calling Game construction at 0x136331. Its subsequent window show/update/focus operations, displayed “START” output and main message/update loop occur after Game construction returns on that caller. The generation worker has already been launched, but its handoff does not establish that its first baseline has been sampled before any of those later operations. No wall-clock latency or scheduling order between those threads is established.

## The three entry samples

Worker 0x84F70 makes three consecutive calls to the imported timeGetTime operation before its scheduling setup and first lifetime test:

1. The call at 0x84FB0 produces a four-byte carried sample held initially in R13D. This is the sample subsequently used by the worker's separate strict greater-than-1000 comparison.
2. The call at 0x84FB9 produces the four-byte periodic-operation baseline, stored in the stack slot shown at 0x84FBF. This second sample is the baseline used for the strict greater-than-60000 periodic check.
3. The call at 0x84FC2 has no retained return-value use in the selected worker body; the following call obtains the current thread handle.

These are separate readings, not one tick copied into three roles. The periodic baseline is initialized at entry before any generation loop work, name test, UI admission test, or wait for all three indexed UI values to become zero. It is not initialized when a menu is hidden, when generation first succeeds, or when the first periodic helper call completes.

After these samples the worker obtains its current thread handle, requests priority -1, creates an initial scheduler policy, sets concurrency limits with operands 1 and 4, sets policy selector 6 to 0xFFFFFFFF, and reaches scheduler creation. It initializes a remembered scheduling integer to 1, then reads G+0x2068. Even if that first lifetime read observes zero, the three clock calls and this initial setup precede the selected exit path.

The time operands remain four-byte timeGetTime values. The worker shows no floating-point conversion, simulation-step scaling, elapsed-time cap, or multiplication of them before the two threshold comparisons. Its subtraction and threshold branches operate on unsigned 32-bit differences, including ordinary modulo-32-bit wrap behavior. These facts do not establish a separate wall-clock origin or timer resolution.

## Work surrounding the per-iteration sample

While G+0x2068 is nonzero, the global byte at RVA 0x5514C7 selects the worker body. Zero admits the full iteration. Nonzero reaches Sleep with operand 100, then rechecks the lifetime byte. That nonzero branch does not refresh either entry baseline or take the ordinary per-iteration sample.

The zero-byte branch first enters the World critical section through 0x2ABB70 at 0x85060. The concrete helper enters W+0x3C8. Inside this scope it reads G+0x1BA0 and, when it differs from the remembered integer, adjusts the thread-priority and scheduler-policy request operands. The zero case requests priority -1 and concurrency limits 1 and 4; the nonzero case requests priority 0 and limits 1 and 8. Policy selector 6 receives -1 or 0 respectively. The remembered integer is updated. This is scheduling configuration, not the 0x5514C7 process-byte test.

Still inside W+0x3C8, the worker reads its center from the local entity's eight-byte X/Y slots at +0x10/+0x18, converts them for the spatial search, and uses the four-byte target coordinates at +0x1B0/+0x1B4 instead when the one-byte action operand +0x68 equals 0x9C. It derives the search range from G+0x1B9C with the displayed signed multiply/divide and minimum-64 selection. It then leaves W+0x3C8 through 0x2B9890 at 0x85157.

Only after that unlock does it call timeGetTime at 0x8515C. This four-byte result is the current iteration sample. It is carried across the subsequent candidate enumeration, generation work, sleeps and periodic test. There is no fresh clock call immediately before or after the periodic helper.

The current sample is first compared with the first entry sample. A strict unsigned difference greater than 1000 replaces that first carried sample; equality at 1000 does not. That carried value is preserved through the candidate work and restored for the next iteration. In the selected worker it does not supply the 60000 baseline and is not passed as a generation elapsed-time argument.

Outside the initial World critical section, the worker builds a temporary vector of twelve-byte candidates while consulting the World collection rooted at W+0x460. It tests candidate availability and distance, orders the candidate vector through 0xF0790, limits a longer vector to four through 0xA2630, and reaches 0x7D5C0 at 0x855D7 when candidates are present. An empty vector instead reaches Sleep with operand 5. Both routes then reach Sleep with operand 0 at 0x855DE.

The single-record route in 0x7D5C0 directly reaches 0x2AE0C0 for W and the candidate pair. Its multiple-record route reaches 0x76DA0, whose selected original body includes the named structured-task RunAndWait operation before returning. Within the inspected 0x2AE0C0 publication tail, W+0x3F0 is entered before W+0x3C8 and those scopes are left in reverse order after publishing the generated pointer. Generation interiors, all scheduler-library behavior and every indirect task target were not comprehensively audited. The directly proved order is that the selected generation helper returns before this worker's Sleep(0) and periodic test.

Consequently the time sample used for that test precedes candidate construction and any selected generation work in that iteration. Lock acquisition delays before 0x8515C are reflected in that iteration's sampled value. Delays after 0x8515C do not cause this worker to resample before deciding whether its carried difference exceeds 60000.

## Periodic admission and baseline advancement

After the generation/empty-vector route and Sleep(0), the worker compares the carried current iteration sample with the second entry sample or its most recently assigned replacement. At 0x855EA/0x855EF the unsigned difference must be strictly greater than 60000. Equality at 60000 skips the periodic call.

The worker then requires W+0x210 to be nonzero and tests three indexed four-byte UI values, in order: the root held at G+0x130, then G+0x118, then G+0x170. All must be zero. It finally rereads G+0x2068 and requires it to remain nonzero. These checks are after the initial World scope has ended; the worker does not surround this name/UI/lifetime admission sequence with a new World lock.

If those checks pass, 0xA27B0 is called at 0x85671. Only after its normal return is the periodic baseline assigned the earlier current iteration sample at 0x85676. The replacement is not the call-completion time. The temporary candidate storage is then released when present, the remembered scheduling integer is restored and the lifetime byte is read again for loop continuation.

Failed threshold, name, UI or final lifetime admission leaves the periodic baseline unchanged. The nonzero 0x5514C7 sleep route also leaves it unchanged. Time spent with any of the three UI values nonzero or with this worker body skipped is therefore not deliberately subtracted by rebasing the periodic sample. A later admitted full iteration still compares its own sampled counter against the old baseline. There is at most one such periodic helper call per full iteration; the worker has no catch-up loop issuing one call per crossed interval.

RVA 0xA27B0 is character-state persistence on this selected route. It enters W+0x3C8, reads the signed four-byte selected index at G+0x2264 and returns after unlocking if that index is negative. Otherwise it constructs an entity snapshot, copies the selected entity/core/CharacterData/inventory/cursor/gold/buff state and temporary World-state collections, then leaves the World scope. Serialization through 0x149760 and the subsequent protected database operation on G+0x2268 occur later; the database critical section is G+0x1C98. The temporary World copies are destroyed during cleanup. This is not evidence that this generation-worker call writes a retained World-state record.

The worker advances its baseline after a normal helper return even when that helper's negative-index admission caused it to do no serialization. No returned success value controls the store at 0x85676. Exceptions, allocation failure, blocking backend behavior and abnormal helper returns are not assigned recovery or timing guarantees here.

## Lifetime stop and final call

The constructor's concrete lifetime-byte store is G+0x2068 equal to 1 at 0x7EF63. The selected destructor 0x818D0 reaches client disconnect 0x911E0 at 0x81916 and then writes G+0x2068 equal to zero at 0x8191C. These are the directly identified stores for this ordinary Game owner. The process-wide 0x5514C7 operand is a separate address and is not cleared by that lifetime store.

The worker checks G+0x2068 at its first loop admission, after each full or skipped iteration, and immediately before the admitted periodic call. It is not a cancellation test between every candidate or inside each untraced generation helper. Observing the stop byte may therefore follow in-progress work.

On the selected exit path, the worker tests the same UI values in the order G+0x130, +0x118, +0x170 and calls 0xA27B0 at 0x8572D only if all three are zero. This final route has no 60000 threshold and no W+0x210 name gate. It is also outside the full-iteration 0x5514C7 zero gate. The lifetime byte being zero is the reason for reaching it, rather than an admission failure that forbids the final call. The worker then destroys the initial scheduler policy and returns. Its dispatch wrapper frees the eight-byte capture after that return.

The destructor labels the G+0x1FA8 owner as the “generation thread” and joins it through 0x9AF20 at 0x819BE, after the displayed music and AI waits and before the displayed chunk-thread wait. The join helper checks the thread identifier, rejects self-join and reaches the imported join operation before clearing the record. This ties the worker to that concrete lifetime owner; it does not guarantee how soon another thread observes the stop store.

## Process-byte writers and limits

The directly tested worker byte is RVA 0x5514C7, confirmed by the native comparison at 0x85040. RVA 0x5514CF is eight bytes later and is not the same operand. The selected ordinary viewed update has its separate comparison of 0x5514CF at 0xA9C4F. No use of that latter byte admits or skips the 0x84F70 worker body.

The supported image places both bytes in the virtual zero-fill tail of its .data section: section RVA 0x530000, raw size 0x1DE00 and virtual size 0x23C40. Thus their initial loaded-image value is zero. This initial-image fact is separate from a proof that no runtime writer can exist.

The bounded original native operand search found three direct references to 0x5514C7: comparisons in 0x83C70 at 0x83CB0, 0x842D0 at 0x84340 and this worker at 0x85040. The selected other two worker bodies likewise take their active work on zero and a Sleep(100) route on nonzero. No direct store to 0x5514C7, and no ordinary command-line, configuration or UI writer that selects its nonzero state, was established. A raw-image search also found no embedded full 64-bit literal pointer to this byte, but that absence does not rule out computed addresses or aliases.

For 0x5514CF the bounded native search found the separate viewed-update comparison and no direct writer. The adjacent native initialization fragments at 0x1D00 write only 0x5514C0 through 0x5514C5; those at 0x1CE0 write only 0x5514C8 through 0x5514CD. Those stores do not set 0x5514C7 or 0x5514CF. Neither adjacency nor a shared process-wide storage region justifies treating the bytes as one flag.

The ordinary image-start admission demonstrated for this worker is therefore its zero-byte route. Its nonzero behavior is directly specified, but a supported ordinary producer of that nonzero value remains unproved. No named feature, user-facing toggle or universal immutable-zero assertion is attached to that byte. Unrecognized overlapping/computed writes, indirect startup paths, external mutation, process instrumentation and other builds remain outside the discovered direct-reference set.

Static call order does not establish exact timer readings, start-to-first-sample latency, interval between actual persistence completions, UI transition timing, thread scheduling, lock wait duration, backend duration, or cross-thread visibility guarantees beyond the displayed operations. No runtime capture, scheduler/SDK implementation, exhaustive caller/writer census, or untraced generation algorithm is used to fill those gaps.
