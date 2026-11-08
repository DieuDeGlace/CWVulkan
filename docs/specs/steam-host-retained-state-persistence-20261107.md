# Steam host retained-state persistence — final held contract

This bounded original-Steam-only audit distinguishes the ordinary host update at RVA 0xF5520, in-memory source recording, the periodic state capture in the host source-loader at RVA 0xF2340, and the reached state serializer and database-write wrapper. The inspected original update uses the World at its Host argument plus 0x8; the loader uses the World at its captured Host argument plus 0x8. These owner-addressing routes are proved. A direct call from 0xF5520 to 0xF2340 or a complete launcher and instance-alias audit is not. The periodic state-save calls proved here are in 0xF2340, not in the inspected body of 0xF5520.

The supported image is binary/cubeworld.original.exe.unpacked.exe under C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Its image base is 0x140000000, timestamp 0x5D93AB0D, and verified SHA-256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. All addresses below are RVAs in that image.

Reference filenames were identified before scoped reads. Principal original evidence is decompiled/functions/1400f5520.c, 140083e60.c, 1400f2340.c, 1400f1c90.c, 1402da6c0.c, 1402e2e80.c, 1402d2670.c, 1402d3e20.c, 1402d38e0.c, 14014ab50.c, 140147750.c, 1400672c0.c and 1400672e0.c. Selected copy, guard and daily-reset helpers are identified by RVA below. Native call sites and literal data were checked in the individually selected original decompiled/assembly.txt listing and original image. No earlier held contract, replacement implementation, reconstruction, SDK/dependency source, project history, test, capture or log was used.

## Two distinct clocks

The ordinary host-update caller RVA 0x83E60 samples the 32-bit millisecond clock and supplies an elapsed millisecond value to 0xF5520 at call site 0x83EC6. The host update caps its signed elapsed input at an upper value of 1000. It passes the resulting elapsed amount into World simulation as floating-point milliseconds, divided into sixteen-millisecond steps and a remaining step through 0x2DA6C0 at 0xF6917 and 0xF692D. The original constant at image RVA 0x4ACA34 is float 16. No new behavior for negative or exceptionally large input gaps is inferred.

At the beginning of 0x2DA6C0, the original call at 0x2DA728 reaches time updater RVA 0x2E2E80. It adds the supplied milliseconds to World float+0x440. It also advances the separate float time-of-day at World+0xC, normally by ten times the supplied amount. A directly inspected actor predicate selects a multiplier of 100 instead: at least one actor with byte+0x60 zero, byte+0x68 equal to 0x58 and positive float+0x180. The audit does not assign a wider gameplay name to that predicate.

The literal multipliers 10 and 100 were read from image RVAs 0x42E598 and 0x42E5BC. The time-of-day wrap amount at image RVA 0x438028 is float 86400000. When time-of-day strictly exceeds that amount, 0x2E2E80 increments the 32-bit day at World+0x8 and subtracts one wrap amount, repeating if still above it. Exact equality does not enter the wrap branch. If World pointer+0x210 is null, this routine directly sets time-of-day to float 32400000 before that wrap test. These are the directly proved time values, not unspecified constructor defaults.

The periodic file-state cadence in 0xF2340 uses its own 32-bit wall-clock samples. It does not use the accelerated time-of-day or World+0x440 as the inspected periodic threshold.

## Periodic threshold, reset and loader order

On entry to 0xF2340, the millisecond-clock sample at RVA 0xF2390 initializes the periodic baseline. During each ordinary loader cycle, another sample at 0xF24E3 is taken after the initial player/state snapshot scope ends. The loader then performs required-set retirement and its admitted source-generation batch before reaching the periodic comparison.

At RVA 0xF2CDE the elapsed unsigned 32-bit difference is compared with 60000; the branch at 0xF2CE3 skips the periodic save for values at or below 60000. Thus 60001 milliseconds is the first integer elapsed value admitting the due branch, subject also to the loader's running byte at Host+0x550 still being nonzero. The difference uses the native 32-bit subtraction. No unlimited-gap or clock-wrap recovery policy is inferred.

The due branch takes one capture/save path in that loader cycle; it is not a loop replaying one save for every elapsed minute. After the save wrapper returns, the baseline is replaced at 0xF2D74 with the already sampled cycle time from 0xF24E3. It is not resampled at save completion. This reset is not conditional on proof of a successful disk write. It still occurs when the reached save wrapper returns without writing because World+0x210 is null.

If the running-byte test suppresses the due branch, that branch does not perform its reset. The separately inspected loop-exit path captures and calls the same save wrapper once before returning: World+0x3C8 entry at 0xF2EEE, capture helper 0xF1C90 at 0xF2EFF, release at 0xF2F0D, and save call 0xF2F1E. This is a distinct exit route, not a guarantee that every shutdown reaches it.

Within the inspected ordinary cycle, required-set remover 0x2D2670 is called at 0xF2718, before the serial fresh-source calls at 0xF2CBE and before the periodic capture at 0xF2D04. Therefore selected source recording performed by that remover has returned before this cycle's later periodic snapshot. This is local call order. No fixed scheduling relation between this loader cycle and another invocation of the ordinary host update is asserted.

## Which source values reach in-memory retention

The directly reached host remover selects loaded sources whose coordinate pairs are absent from its supplied required set. In 0x2D2670, the loaded-source traversal is under World+0x3F0. Each selected candidate is later processed under +0x3F0 followed by +0x3C8. The loaded-map node is erased at 0x2D2892, then the saved source pointer is passed to recording helper 0x2D3E20 at 0x2D289D. Its selected in-memory recording returns before the source's deleting virtual at 0x2D28CC. Both acquired guards have been released before that deleting call. The candidate-snapshot interval and alternative map writers remain outside any general lifetime guarantee.

RVA 0x2D3E20 records selected source values into the coordinate-keyed retained map at World+0x10. It does not itself save to the database or acquire a guard. Its inspected caller supplies the +0x3C8 scope. The source's coordinates at +0x10 and +0x14 are copied as signed 32-bit values.

In recording order, nonzero source byte+0xF9 sets retained flag bit 1 and replaces the retained first array from source+0x30. Nonzero source byte+0xFA sets retained flag bit 2 and replaces the second array from source+0x60. The corresponding copy calls are at 0x2D3E9E and 0x2D3EFA. Empty arrays still replace their retained arrays when the associated source flag is nonzero. A zero source flag skips that array's replacement; it does not clear an existing retained flag or array on this path.

The retained edit list is then replaced from source+0xD8 when its count at +0xE0 is nonzero or a retained record already exists for that coordinate. The reached list-copy call is 0x2D402B. Therefore an admitted empty edit list can clear the retained edits, including when an earlier selected-array step has just created the retained record.

The collection at source+0x4D8 is next replaced when its count at +0x4E0 is nonzero or the coordinate record already exists. Calls at 0x2D4141 and 0x2D4151 clear and copy that retained collection. Finally, the 32-bit scalar at source+0x490 is copied when it is nonzero or the record already exists. The helper does not create a retained record solely for an empty edit list, empty extra collection or zero scalar when no record has been established. No broader business meaning for the scalar or every collection member is supplied.

This recording helper has no World+0x358 test. That fact concerns this inspected recording call, not all state writers. The recorded values are selective state; they are not a full source-object image or a saved raw generated terrain field.

The periodic capture itself does not enumerate World+0x460's still-loaded sources and does not call 0x2D3E20. Its source-state input is the retained map that already exists at capture time. It copies every current retained-map entry, without a filter requiring that its coordinate be unloaded or currently outside demand. A coordinate may have a retained record and a presently loaded source at the same time. This audit does not infer that the retained record always equals every later live-source value or that no other retained-map writer exists.

## Captured values and their ownership

The due periodic branch enters World+0x3C8 through wrapper 0x2ABB70 at 0xF2D04. It captures the day from Host+0x10, which is World+0x8, and the float time-of-day from Host+0x14, which is World+0xC. It then copy-constructs the retained map at World+0x10 through 0x6D400 at 0xF2D2A, the identifier collection at World+0x20 through 0x6D5E0 at 0xF2D3D, and the identifier collection at World+0x30 through 0x6D5E0 at 0xF2D50.

The resulting local state has day at +0, time-of-day at +4, retained-map header/count at +0x8, first identifier-set header/count at +0x18, and second identifier-set header/count at +0x28. The exit capture helper 0xF1C90 constructs the same selected arrangement.

The selected copy helpers allocate new tree nodes. The retained-node copy reaches 0x6E2E0 and state-value copy 0x80A70, which copies the flag byte, the two array containers, edit list, extra collection and scalar. The inspected array-copy helpers 0x7E680 and 0x7E5A0 allocate separate storage for their 0xE0-byte elements. The identifier-node copy at 0x6E490 copies the identifier value into a new node. Thus the later serializer traverses captured containers, rather than a saved list of raw loaded-source pointers. This does not specify every internal field of an individual 0xE0-byte value.

The loader releases World+0x3C8 at 0xF2D5E and calls the save wrapper at 0xF2D6F afterwards. It destroys the captured trees and their selected owned contents after that call returns. Capture does not clear the live retained map or either live identifier set.

The state values are therefore captured under the stated guard, while serialization and the reached database wrapper run outside that acquisition. The later save-admission read of World+0x210 and the database owner at World+0x3B8 are not members of the copied state; those are reached from the World again by the save wrapper. Their wider lifetime or alternate writers are not established here.

## Day and identifier contributions

The captured 32-bit day and raw float time-of-day both enter the persisted state. The serializer does not narrow the captured time-of-day to an integer on this route.

The two top-level identifier sets at World+0x20 and +0x30 are also captured and serialized. The question's death/reward categories are bounded here to these exact lanes and the proved uses below; a complete assignment of gameplay meanings or all their producers was not established. Their persisted contribution consists of their current eight-byte identifier values, not complete actors, reward objects or pending game-event lists.

A directly reached use in actor updater 0x2DE4C0 looks in World+0x20 for the nonzero identifier at actor+0x1A0 when actor byte+0x60 equals 8. A match writes that byte to 7. This is the direct effect proved in that selected use; no additional business meaning is inferred from those numbers.

At a day wrap, 0x2E2E80 tests World+0x358. Only its zero case reaches the inspected reset calls at 0x2E300D and 0x2E3015. The latter, 0x2C34C0, clears World+0x20 through 0x3D6C0 at 0x2C3501 and clears the World+0x10 retained map through 0x8A830 at 0x2C350A. The selected clear helpers free the corresponding nodes and set their counts to zero. This reset body does not perform a corresponding clear of World+0x30.

These day-wrap mutations can consequently change what a later state capture contains. They do not prove that every live source field is reset at the same point, or that World+0x30 has no other clearing writer. The other daily-reset actor, inventory and event effects are outside this persistence audit.

## Serialization and the reached storage boundary

Save wrapper RVA 0x2D38E0 checks World+0x210 at 0x2D3909. Null skips its serialization and write body. A nonnull value creates a local byte buffer and stream through 0x147750 at 0x2D3938, serializes the captured state through 0x14AB50 at 0x2D3943, and calls the game database wrapper 0x672C0 at 0x2D3988 using the storage owner at World+0x3B8 and the key consisting of the five characters “state”.

The stream constructor sets the emitted format marker to 44. The directly inspected serializer writes, in order: that 32-bit marker; the 32-bit day; the four raw bytes of float time-of-day; the first identifier-set count and its eight-byte identifiers; the second identifier-set count and its eight-byte identifiers; then the retained-map entry count and entries. Collection counts are emitted as 32-bit values from the native counts. Oversized or corrupt counts are not specified by this contract.

For each retained-map entry, serialization includes its two 32-bit coordinates, one flag byte, first-array element count and selected element encodings, second-array element count and selected element encodings, retained edit count and 0x1C-byte edit records, extra-collection count and twelve-byte member payloads, then the retained 32-bit scalar. The two array element encodings are delegated to RVA 0x1476A0, whose internal value format was not audited. No claim that a 0xE0-byte in-memory element is an unmodified 0xE0-byte disk record follows.

The serializer traverses the captured trees in their native forward tree order, the arrays in element order and the edits in forward list order. It does not re-read loaded sources or re-run their retention admissions while emitting these entries. It emits the stored arrays and flag byte as captured; array serialization itself is not suppressed merely because a captured flag bit is zero.

Wrapper 0x672C0 supplies the byte-buffer pointer and used length to game database-write wrapper 0x672E0. That original game wrapper operates on a “blobs” table, looks up the provided key, and takes an update or insert route for the byte value. It reaches opaque embedded database calls for preparing, binding, executing and finalizing those statements. Those dependency implementations were not inspected. The original SQL statement source is not reproduced here.

The game byte buffer remains allocated through the reached wrapper call and is released by 0x2D38E0 at 0x2D39F0 after the call returns. The loader then releases the captured state. This proves the game-side capture, serialization call and storage-attempt order. It does not prove a completed operating-system disk write, a filesystem flush, a durable transaction, successful recovery after a crash, or the lifetime of any opaque library-owned internal copy. The periodic caller does not condition its baseline reset on a verified write result. Failed I/O, lower-level buffering and codec or storage-library internals remain unresolved.

## Owner and guard limits

The ordinary host update 0xF5520 holds Host+0x5F8 across its inspected update body and enters the host World's +0x3C8 scope at 0xF68CD, leaving at 0xF69F8. Its reached World simulation includes the time/day updater and the selected day-reset path within that scope. Wrappers 0x2ABB70 and 0x2B9890 were independently resolved to entry and exit of the critical section at their World argument plus 0x3C8.

In the source loader, the initial player snapshot, each selected physical source-recording stage, and the later periodic state capture have separate protection scopes. The periodic state capture holds +0x3C8 only; it does not keep the earlier source-removal +0x3F0 acquisition through the later save. The directly inspected source-recording and daily-reset writers therefore use the same World+0x3C8 address as the capture when their actual World owner is the same. No Host+0x5F8 acquisition is attributed to the loader's capture or disk path merely because the separate update routine holds it.

This audit does not establish a universal writer discipline for World+0x10, +0x20, +0x30, +0x210 or +0x3B8, all death/reward producers, every alternate persistence caller, all scheduler or shutdown relations, or general source-pointer safety. It establishes no atomic relationship between a completed file write and a particular host-update iteration. In-memory retention, an owned state capture, bytes presented to a database wrapper, and actual durable disk completion remain distinct boundaries.

FINAL / STABLE / HELD after complete final readback. This is a new bounded contract. All earlier held files remain immutable. Exposure status: clean; standalone author policy and individually selected original Steam evidence only. Further writes stop after readback.
