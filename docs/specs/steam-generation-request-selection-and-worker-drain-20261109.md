# Steam generation request selection, publication and worker drain

Status: FINAL / STABLE / HELD after complete saved-artifact readback. Independent original-source contract, dated 2026-11-09. This document describes the selected native lifecycle and its evidence boundaries. It contains no implementation guidance.

## Authority and distinctions

The supported authority is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, 5,757,952 bytes, AMD64 machine 0x8664, image base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Addresses below are RVAs. Local labels G, H, W, C and R mean the ordinary Game, its embedded Host, a selected World, a World cell object, and the separate map owner. These labels describe concrete receivers and operands.

The original has several distinct collections and completion paths. Game worker 0x84F70 selects absent World cells and reaches 0x2AE0C0. Host worker 0xF2340 independently selects absent cells in its embedded World and reaches the same owner. Game worker 0x842D0 selects existing cells for renderer-side chunk work and later promotes their pending object lists. Game worker 0x83C70 selects separate map results through owner G+0x10. The routine at 0x31C540 receives an existing cell and synchronously refreshes its records; its World pointer is C+8. It is not the owner of any of these entire worker lifecycles.

No universal claim about all procedural generation callers or all writes to shared state follows from this bounded audit. The original numerical generation body 0x2FF6F0 and the large map numerical body 0x2EB7B0 are explicit internal provider boundaries. Their complete exported instruction ranges were byte-compared; this contract does not claim a numerical closure of their internal terrain/noise/resource helpers. The selected map provider entry and publication contexts are identified separately in the manifest.

## Ordinary owners, startup and thread transfer

Application context 0x1355A0 allocates 0x548 bytes for a World, reaches its constructor 0x29A130 at 0x136163, and passes that same World into the ordinary map owner, renderer and Game construction. Game constructor 0x7EA80 stores that caller World pointer at G+8, the map owner at G+0x10 and renderer at G+0x18. The Host is a distinct embedded object at G+0x6E8. Host constructor 0xF17A0 constructs its own World in place at H+8. Consequently the ordinary viewed-cell and Host-cell maps are different owners, even when their coordinates and configuration arguments happen to agree.

World construction initializes its role byte +0x358 to zero, its local-player pointer +0x448 to zero, the cell registry at +0x460, and the pointer sets at +0x190 and +0x1A0. Registry 0x29A010 starts with binary32 load-factor word 0x3F800000 and requests eight buckets. World critical sections at +0x3C8, +0x3F0 and +0x418 are initialized with spin-count argument 16. Helpers 0x2ABB70/0x2B9890 enter/leave +0x3C8; 0x2ABB80/0x2B98A0 enter/leave +0x3F0. Those particular scopes are described below; they are not one implicit lock around every World access.

During Host construction, H+0x550 is set to one and its thread representation at +0x558 is initially empty. The Host calls World configuration 0x2B5F20 with selection integer 123 and the six-byte string “ world”, then starts its own generation worker through 0xF0150. This occurs within the embedded Host construction, before the later Game constructor body initializes Game's worker flags, caller configuration copy, interface and initial local entity. The Host worker does not borrow G+8.

The Game later sets byte G+0x2068 to one, integer G+0x206C to 123 and its selected string G+0x2070 to “world”. It passes the latter two arguments to 0x2B5F20 on G+8. Character enumeration, interface setup, construction/insertion of the initial local entity, publication at World+0x448 and reached World/player rebuilds occur before the Game's chunk and generation thread-start calls near the end of 0x7EA80. The optional saved-character route also precedes those starts. This establishes static construction order, not the relative wall-clock progress of different children.

The following selected thread relationships are directly established by constructor calls, original table pointers and complete adapters.

| Owner | Start wrapper | Adapter path and worker | Stored thread representation |
| --- | --- | --- | --- |
| Game chunk route | 0x76160 | table 0x437EA8, thunk 0x87BE0, adapter 0x890C0, worker 0x842D0 | G+0x1FB8; 32-bit ID at +0x1FC0 |
| Game cell route | 0x762D0 | table 0x437EB8, thunk 0x87BF0, adapter 0x89160, worker 0x84F70 | G+0x1FA8; ID at +0x1FB0 |
| Game Host-update route | 0x75D10 | table 0x437EC8, thunk 0x87BB0, adapter 0x88EE0, worker 0x83E60 | G+0x2018; ID at +0x2020 |
| Game map route | 0x75BA0 | table 0x437ED8, thunk 0x87BA0, adapter 0x88E40, worker 0x83C70 | G+0x1FC8; ID at +0x1FD0 |
| Embedded Host cell route | 0xF0150 | table 0x43FD18, thunk 0xF3070, adapter 0xF3220, worker 0xF2340 | H+0x558; ID at +0x560 |

The Game constructor also starts the separately identified 0x83F20 and 0x83FB0 routes through their own adapters and thread fields. Their complete numerical/action work is outside this request-selection contract. The shared client callback installed through 0x7D140 is a separate raw-owner connection; it is not a generation-worker wake operation.

Allocation helper 0x7D560 creates an eight-byte payload containing the raw owner pointer. Each selected start wrapper initializes a local condition variable and mutex, locks the mutex, moves the payload into its local startup carrier, and invokes imported _Thrd_start with entry 0x3C7D0. Status 4 is converted to error 1; nonzero errors reach the original C-error throw boundary. The parent waits while the carrier's acknowledgement byte is zero, then unlocks and destroys the local synchronization objects.

The child adapter takes the payload pointer out of the carrier and clears that carrier field before locking the startup mutex. It sets the acknowledgement byte to one, signals, unlocks, and only then calls its worker with the privately held payload. It frees that payload after worker return. Entry 0x3C7D0 invokes the adapter, then reaches imported thread-exit broadcast and returns zero. The acknowledgement transfers the payload out of the stack carrier; it does not establish that the worker has reached its first sample, selected a request or completed a pass. No Game/Host reference-count ownership is created by the eight-byte payload.

## Cell identity and request records

The cell registry's key is two consecutive 32-bit coordinates. Lookups compare their full eight-byte pair. Owner 0x12700 hashes the eight stored bytes in little-endian pair order using 64-bit offset basis 0xCBF29CE484222325 and multiplier 0x100000001B3, with each byte combined before the next multiplication; the final value is masked by W+0x490. Buckets are reached through W+0x478, the sentinel through +0x468, node key through node+0x10 and cell pointer through node+0x18. Registry helpers themselves do not create an implicit World critical-section scope.

The Game and Host absent-cell vectors have twelve-byte records: first coordinate at zero, second at +4, and signed 32-bit priority at +8. They contain no World pointer, seed/configuration snapshot, mode/role byte, callback owner, stop token or result pointer. Game chunk candidates instead use a 0x18-byte stride: cell pointer at zero, signed 32-bit distance at +8, and coordinates at +0xC/+0x10. The selected stores define twenty bytes of those records; they do not establish the trailing four padding bytes. The final chunk dispatch vector contains only eight-byte coordinate pairs.

Coordinate descriptions below use the original numeric whole-coordinate unit. The selected stored entity position unit is 0x10000 per whole coordinate; no physical length label is supplied. In the ordinary representable domain, 0x84F70 and 0xF2340 first truncate a negative fractional fixed-point position toward zero, using the original conditional addition of 65535 and arithmetic shift by sixteen, then narrow to signed 32 bits. Their later division into 64-wide cells uses the negative-coordinate floor adjustment. Chunk worker 0x842D0 and its single-cell retirement helper include an additional negative-fraction correction and therefore first floor to a whole coordinate. These two conversions are distinct.

Signed 32-bit products, additions, subtractions, increments and priority comparisons retain their native widths. The simple distance and floor descriptions apply when the stated ordinary coordinate/radius arithmetic remains representable and enumeration terminates without signed-width wrap. They do not enlarge the original domain into arbitrary extreme coordinates or assign portable-language overflow behavior.

## Viewed absent-cell selection: 0x84F70

At worker entry the original samples timeGetTime three times in order. The first is a carried baseline, the second the periodic persistence baseline, and the third result is discarded. It then requests current-thread priority -1, creates a scheduler policy with concurrency limits 1 and 4 and policy key 6 set to -1, and initializes the previously observed signed setting value to one.

Every top-of-loop test reads byte G+0x2068; zero enters the exit path. The ordinary work branch additionally requires global byte RVA 0x5514C7 to be zero. A nonzero byte reaches Sleep(100) and the next stop test. This is the exact operand; it is not RVA 0x5514CF. The selected worker body does not derive that global byte from a World role or GUI page.

The ordinary branch enters W+0x3C8 on the current G+8. It reads signed 32-bit G+0x1BA0. Only a change from its previously observed value requests a new policy: zero selects thread priority -1, limits 1..4 and key-6 value -1; any nonzero value selects priority 0, limits 1..8 and key-6 value zero. The changed value is carried to the next iteration. Thus the initial value one does not itself request the nonzero 1..8 configuration: it compares equal to the initial carried value and leaves the entry configuration in effect.

Under the same guard it obtains the current entity through W+0x448 and samples its two 64-bit position components. If the signed byte at entity+0x68 equals -100, it substitutes the two signed 32-bit fields +0x1B0/+0x1B4. It reads G+0x1B9C as signed 32 bits, multiplies by 1920 at 32-bit width, divides by 100 with signed truncation and substitutes integer 64 whenever that result is less than 65. The result is the selected whole-coordinate radius. There is no independently proved input restriction to percentages 0..100 in this audit.

After releasing W+0x3C8 it samples timeGetTime again. Unsigned 32-bit elapsed strictly greater than 1000 replaces the first carried baseline with this sample; otherwise the old baseline is retained. This comparison does not gate request enumeration or dispatch. The same iteration can select work on either side of that boundary.

Enumeration visits increasing first cell coordinate outside increasing second coordinate, both with inclusive bounds obtained by flooring the sampled centre-minus-radius and centre-plus-radius into 64-wide cells. An absent node or null node value is eligible only when the double-precision sum of squared cell-index offsets from the centre cell is at most the integer squared radius formed from the signed quotient of the whole-coordinate radius by 64, increased by one. This scan contains no explicit +0x3F0 guard around its registry lookups.

Each admitted pair receives the signed 32-bit squared distance to that cell's closed 64-by-64 rectangle in whole coordinates. A centre inside both inclusive axis bounds has priority zero; a centre outside one axis uses the nearer edge's squared difference, and an outside corner uses the two squared differences added at 32-bit width. Closed upper boundaries are preserved, so neighbouring cells can tie at zero. The numerical description does not add a secondary coordinate ordering.

Sort owner 0xF0790 and its selected insertion, partition, median and heap closure compare only the signed 32-bit priority at record+8 in ascending order. For ranges up to 32 records the reached insertion path moves strictly smaller priorities; the larger partition/heap paths can reorder equal priorities. No universal stable tie order is established. The worker retains the first four records when there are more than four. This is selection priority; it is not an execution-order promise for the parallel callbacks.

An empty vector reaches Sleep(5). A nonempty vector reaches 0x7D5C0 with the raw Game owner. One record directly invokes 0x2AE0C0 on the World loaded from G+8; multiple records reach the selected parallel wrapper. The vector is discarded after the pass. Missing cells that were not retained remain candidates for a future independently sampled pass.

After that work, the worker reaches Sleep(0). Its periodic persistence test uses the unsigned 32-bit sample taken before enumeration and dispatch, not a new completion sample. It requires elapsed strictly greater than 60000, nonzero 64-bit string length at the current World+0x210, and zero selected 32-bit indexed values for each of G+0x130, +0x118 and +0x170. Each indexed value is reached through object+0x68, signed index +0xAC and integer array +0xE0. It then rereads G+0x2068; only nonzero reaches 0xA27B0 and advances the periodic baseline to the carried sample. The root offsets are operand identities here, not inferred screen names.

On the stop path the same three zero-indexed-value tests can reach 0xA27B0, without this worker's periodic name/elapsed test, before its initial policy is destroyed and it returns. Helper 0xA27B0 independently guards the viewed World and returns without a character snapshot when signed selected slot G+0x2264 is negative. Otherwise it copies the selected entity and retained World state under the World guard, releases it, serializes the entity and reaches the character-store path under Game critical section +0x1C98. It is a persistence operation, not a cell-request drain. Its backend failure and full serialization closure remain separate boundaries.

## Embedded Host absent-cell selection: 0xF2340

The Host worker is a separate loop with an eight-byte raw Host payload. At entry it samples timeGetTime twice, discarding the first result and retaining the second as its persistence baseline. Its loop is admitted by byte H+0x550 being nonzero. Unlike the selected Game cell/chunk/map loops, this complete owner contains no RVA-0x5514C7 admission test and no current-thread priority/policy reconfiguration.

It enters the embedded World H+8's +0x3C8 guard and copies position pairs for entities in H+0x48, which is that World's +0x40 list, in native forward list order. Only an entity whose +0x60 byte is zero contributes a pair. The -100 state-byte substitution uses +0x1B0/+0x1B4; other admitted entities contribute their truncated whole-coordinate positions as described above. The worker releases the World guard before using this temporary list and samples timeGetTime before selection.

For each copied position it enumerates inclusive cell bounds formed from the whole-coordinate position minus/plus 270, floored into 64-wide cells. Both absolute cell-index offsets must be less than six and their double-precision squared sum must be at most 25. The checked binary64 word at 0x43FD20 is 0x4039000000000000. Every admitted pair is inserted through 0x745E0 into a unique ordered set of two signed 32-bit integers, ordered first by the first coordinate, then by the second. Overlapping entity neighbourhoods therefore coalesce within this pass.

Before constructing the missing-cell vector, the worker calls 0x2D2670 with that needed-coordinate set. That helper retires existing cells whose coordinates are not members of the set; it does not append a generation request. The needed set is subsequently traversed in its original signed lexicographic order. A cell-registry node with a nonnull value suppresses an absent-cell request. The Host lookup scan has no independent explicit registry guard in this owner.

For each remaining pair, the worker considers all copied positions. A position inside the cell's closed whole-coordinate rectangle immediately gives score zero. Otherwise a position is considered only while each signed absolute difference from the cell origin is below 385; its score is the signed 32-bit squared distance to the nearest rectangle point. The smallest admitted score is retained. A score remaining at the initial -1 prevents appending that pair. This is a per-cell minimum over the copied position list, not a queue sorted separately for each entity.

The twelve-byte request records are sorted by the same signed priority owner 0xF0790 and reduced to at most four by the reached resize path 0xF4E60. The Host then calls 0x2AE0C0 for those records sequentially in retained vector order, always with its embedded World H+8. There is no parallel-for-each wrapper in this Host dispatch block. Empty work reaches Sleep(5), and the common continuation reaches Sleep(0).

Its periodic test compares the earlier unsigned 32-bit sample with the retained baseline. Values up to 60000 do not enter persistence; values at least 60001 can. If H+0x550 is still nonzero it copies the selected retained World state while holding the World guard, releases that guard and reaches 0x2D38E0, then advances the baseline to that earlier sample. It does not use the Game's three indexed-root gates.

When the top/bottom loop admission observes H+0x550 zero, the Host takes a final retained-state snapshot through 0xF1C90 under the World guard, releases the guard, calls 0x2D38E0 and destroys the temporary snapshot. Owner 0x2D38E0 itself requires nonzero 64-bit World string length at +0x210; it serializes the supplied retained-state snapshot and stores it with the literal key “state” through World+0x3B8. Neither this final path nor its periodic counterpart is 0x2D2930's live-cell map drain. Final retirement of the Host's remaining cells belongs to later World destruction.

## Shared cell execution and publication: 0x2AE0C0

This complete owner receives one concrete World and two 32-bit coordinates. Its first lookup of World+0x460 occurs before either publication lock. An existing nonnull cell pointer is returned immediately, with no new allocation, srand call or reached numerical generator in this owner. A missing node or null value proceeds to a new cell. The selected code does not place an in-progress reservation into the registry before generation.

The imported srand argument is the low 32-bit combination of the second coordinate multiplied by 1024, the current 32-bit World+0x244 scalar and the first coordinate. The owner then allocates 0x58518 bytes, calls concrete constructor 0x2F9400 with the World and coordinate pair, and reaches numerical provider 0x2FF6F0. No random-output sequence, imported random-state ownership or complete terrain result is supplied by this contract. A decompiler timeout for that large provider is recorded as an evidence limit rather than replaced with inferred source.

The concrete cell stores its World pointer at +8 and coordinates at +0x10/+0x14. Its constructor selects table 0x4738C0, initializes the reached owned containers, initializes cached 32-bit value +0x4F8 to -1, and constructs the render aggregate at +0x1C0 through 0xC25C0. That aggregate initially has empty current and pending pointer lists, byte +0x23C zero, and an eight-byte zero store at cell+0x440 which also defines the 32-bit counter +0x444 as zero. These are selected stores, not a whole-0x58518-byte zeroing claim, and the following numerical provider can make further changes.

Only after the numerical provider returns does 0x2AE0C0 inspect World role byte +0x358. A nonzero byte bypasses this owner's retained-state overlay. A zero byte looks for the coordinate pair in the World+0x10 retained registry using signed lexicographic comparison. If found, flag bit 0 first sets C+0xF9 and copies its selected record vector into C+0x30; flag bit 1 then sets C+0xFA and copies the next record vector into C+0x60. The reached C+0xD8 collection is copied next, and its records are applied through 0x31C1C0. The 32-bit retained scalar is then stored at C+0x490, followed by the reached collection copy into C+0x4D8. Absence of that retained entry skips these overlay operations.

This role byte and the retained entry are read at execution time after numerical generation. Neither was captured in the twelve-byte request. The World+0x244 input is likewise read when 0x2AE0C0 executes, not when a selection vector is built. These facts do not prove that any untraced writer actually changes those fields during an ordinary request.

Publication then enters World+0x3F0 followed by World+0x3C8. Helper 0x299640 finds the current coordinate node or creates/inserts one. Native store 0x2AE354 unconditionally writes the newly generated cell pointer to that node's +0x18. It then leaves +0x3C8 followed by +0x3F0 and returns that pointer.

There is no second existing-nonnull fast return under those publication locks, no comparison against the cell pointer seen by the initial lookup, and no old-cell deleting call in this publication block. Thus the entry fast path suppresses duplicates already present when that lookup runs; it is not proof of exclusive generation for two callers whose initial lookups both miss. If such callers both reach normal publication, the later assignment can replace the map value. This bounded statement does not certify that a particular schedule occurs, describe every external holder, or infer automatic retirement or safety from the registry assignment.

The selected direct-call census in the original calls metadata finds 0x2AE0C0 at the single-record bridge 0x7D5C0, two execution sites inside callback 0x858E0, and the Host sequential loop 0xF2340. This is a census of those decoded direct calls, not every indirect entry, exception route or unrecognized instruction range.

## Parallel dispatch, cancellation and carried state

For absent viewed cells, 0x7D5C0 receives begin/end of the twelve-byte vector and a pointer to the raw Game owner. For chunk work, 0x7D680 receives begin/end of the eight-byte coordinate vector. They invoke original PPL trace imports before and after dispatch. Empty ranges do no per-record work. A single viewed record directly reaches 0x2AE0C0 on G+8; a single chunk record directly reaches 0xE9530 on renderer G+0x18.

Larger ranges reach complete wrappers 0x76DA0 and 0x77120 respectively. The ordinary supplied 64-bit stride is one. Each wrapper asks the imported scheduler for its virtual-processor count, partitions signed 64-bit index ranges, and builds 0x58-byte chore objects. Its selected allocation route uses the stack for a bounded small allocation and malloc for larger storage. Allocation failure reaches the original exception boundary. It schedules all but the final initial chore through the imported structured-task collection, then calls its imported RunAndWait operation with the final chore. Chore destruction and backing-storage cleanup are ordered after that call returns.

Original thunks 0x75B80 and 0x75B90 adjust the chore receiver by 0x28 and transfer to complete callback owners 0x858E0 and 0x85DC0. Each receives start/end index fields, the stride pointer, the begin-pointer carrier and the raw-owner carrier. If a broker pointer is present, complete helper 0x88680 can reject the acquired range before execution. Its synchronized blocked-context query, signed state fields, atomic count changes and spin/yield boundaries are independent of Game byte +0x2068.

A one-element callback range invokes its corresponding operation once. Larger ranges create a local cancellation beacon, can schedule native split-range chores, and test the beacon before another per-record call. Confirmed provider cancellation can break that native callback loop. Each already reached operation is synchronous at that call site; after it returns the index advances and the callback can wait for a broker handoff to finish. The callback eventually reaches a RunAndWait call with null chore, followed by chore, cancellation-beacon and collection cleanup on the selected normal path.

The viewed callback computes its twelve-byte record address and reloads G+8 when invoking 0x2AE0C0. The chunk callback computes its eight-byte record address and reloads G+0x18 for 0xE9530. Their records do not fix a future World/renderer pointer in the vector itself. The selected owner bodies contain no Game-stop-byte test inside these per-record loops and no rollback of previously published cells or pending objects.

The native code requests work and then calls imported scheduling/wait/cancellation operations. This proves those call and cleanup orders, not the imported scheduler's exact execution order, fairness, worker count, successful completion of every record, exception propagation or quiescence. Priority-sorted records can be partitioned, split or canceled before all calls occur. The retained vectors and stack parameter carriers are cleaned only after the selected wrapper returns normally. No guarantee that all selected records publish, or that publication order equals priority order, is manufactured from an import's name.

## Existing-cell refresh and its separate accumulation drain

The complete receiver body at 0x31C540 first compares the World+8 32-bit value with C+0x4F8. Equality returns without the refresh. Otherwise it writes the current value to C+0x4F8 before deleting the old selected C+0x4E8 list contents, replacing/comparing record vectors, or making later conditional record additions. The cached value has therefore already advanced if a later helper fails; this owner is not a rollback transaction.

Changed-record paths insert the raw cell pointer through 0x73F40 into World+0x1A0 and set cell byte +0xFA to one. That set compares unsigned 64-bit pointer values and suppresses duplicate pointer keys for the supplied insertion mode. The complete body also reaches its coordinate/World-value-derived srand and conditional native record/marker providers. Their returned numerical content and transitive random draws are not interpreted here as new cell requests. This owner neither appends one of the worker coordinate vectors nor starts a thread, waits for a worker or invokes 0x2AE0C0.

Its one decoded direct caller is World update owner 0x2E17D0 at 0x2E1D7C. That complete caller is admitted only while World+0x358 is zero. It collects eligible existing entity positions, gathers existing nonnull cell pointers from the surrounding coordinate ranges, deduplicates them in an unsigned pointer-ordered temporary set and visits that set in native order. For each reached cell it calls 0x31C540 before the associated record-consumer work. This is a synchronous existing-cell route; it is independent of whether a generation worker has just selected another coordinate.

World update 0x2DA6C0 reaches 0x2E17D0 only in its zero-role-byte branch. Ordinary Game update native call sites 0xA7328 and 0xA734C pass G+8; that selected branch requires the 32-bit selection G+0x206C to equal World+0x220 and the selected Game string to compare equal to World+0x200. The preceding three indexed GUI values affect an entity flag in that context; they are not the direct branch that substitutes for this configuration equality. Ordinary Host update 0xF5520 passes H+8 at its selected World substeps. Its World guard is entered at 0xF68CD and left at 0xF69F8; Host critical section +0x5F8 also brackets that ordinary update body. The selected Game World guard calls are at 0xA6464 and 0xADB23. These identify reached ordinary scopes, not exception-safe global-lock guarantees.

The later complete accumulation drain 0x2E30B0 is reached by Game update at 0xA8498 and Host update at 0xF6947. It first traverses World+0x190, marks cell+0xF9 and copies the selected C+0x30 records into World+0xE0's record accumulation. It then traverses World+0x1A0, marks cell+0xFA and copies C+0x60 records into World+0xF0's accumulation. Next it calls 0x31D700 on cells in the latter pointer set, then clears both sets and their 64-bit sizes.

Owner 0x31D700 processes the existing cell's marker/record data through its selected coordinate helpers. This is still synchronous World update work. Neither those pointer sets nor the World+0xE0/+0xF0 accumulations are the generation worker's twelve-byte priority queue, and clearing them is not evidence that a selected worker batch completed.

## Chunk selection and pending/current publication: 0x842D0

This worker's outer admission reads Game byte +0x2068. Zero directly reaches 0x2D2930 on the current G+8, then returns. A nonzero global process byte 0x5514C7 selects Sleep(100). Ordinary work starts only while that global byte is zero.

Under World+0x3C8 it obtains the current local entity and derives the floored centre cell described above. The -100 state-byte alternative instead floors the two signed 32-bit +0x1B0/+0x1B4 coordinates into 64-wide cells. The signed setting G+0x1BA0 is compared with a carried initial value one; changes request the same zero/nonzero priority and 1..4/1..8 policy settings as the viewed cell worker. This worker does not contain the viewed cell worker's unconditional entry priority/policy setup. A setting that remains one therefore does not itself trigger a policy request here. The guard is then released.

The selected signed tuning calculation multiplies G+0x1B9C by 30 at 32-bit width and divides by 100 with signed truncation. A result at most three is replaced with three. Before gathering chunk candidates, the worker calls 0x2D2410 eight times with that radius plus one. It does not stop those eight calls after a false return. These are retirement attempts, not new-cell generation.

Candidate enumeration is an inclusive square of cell indices around the sampled centre. Each pair reaches Sleep(0), then a World+0x3F0-guarded lookup. A nonnull cell pointer is copied into the 0x18-byte candidate record, with signed 32-bit squared cell-index distance and its coordinate pair. After the guard is released this record is a raw pointer, not a reference-counted handle. Owner 0x7BB20 and its selected insertion/partition/heap/median closure sort only signed record+8 in ascending order. They do not establish a stable equal-distance order.

The budget radius is read again from G+0x1B9C using that same signed calculation. While the World guard is held, the original widens the selected radius to signed 64 bits, squares it, multiplies by three and shifts left by fifteen. It converts that signed 64-bit result to binary32, multiplies twice in sequence by binary32 word 0x3F4CCCCD at 0x42E530, then performs signed 64-bit truncating conversion. Native 0x849F8..0x84A05 proves the signed conversion and the two separate multiplications; the decompiler's unsigned type is not adopted. For finite in-range results, conversion truncates toward zero. Floating exception modes and NaN/out-of-range conversion behavior are not broadened into an independent valid-input guarantee.

The cumulative signed 64-bit amount starts at zero. For each sorted candidate, an amount at most the budget admits the outer branch; a nonzero signed 32-bit C+0x444 also admits it even when the amount exceeds the budget. Inside, C+0x444 must be zero or byte C+0x23C must be nonzero. The selected renderer neighbourhood predicate 0xCE3E0 must return a nonzero low byte, and the signed selected-pair count must be below eight, before the coordinate pair is appended. Once the outer branch is admitted, the cumulative amount increases by sign-extended C+0x444 whether or not the inner checks appended work. The loop does not terminate merely because the amount exceeds the budget or eight pairs have already been selected.

Predicate 0xCE3E0 obtains its World through renderer+8 and holds World+0x3F0 while visiting the selected 3-by-3 neighbourhood, first-coordinate offsets -1 through +1 outside second-coordinate offsets -1 through +1. Every pair must have a present nonnull cell value. The first miss unlocks and returns zero; complete success unlocks and returns one. This is registry presence admission, not proof that every neighbour's numerical/render work is complete.

Dispatch 0x7D680 supplies the raw Game owner to the single-record or parallel chunk route. Complete renderer owner 0xE9530 obtains its World from renderer+8, enters World+0x3F0, looks up the requested cell, and builds its local cached neighbourhood through 0xCBFC0 before releasing that guard. A missing/null requested cell enters temporary-buffer cleanup and returns without publishing a new pending list.

For a present cell, that complete body performs its selected numerical and resource-building work using the neighbourhood and original helpers, with explicit Sleep(0) calls within its loops. Its full body has no direct Game-stop-byte or World-role-byte admission check. Those facts do not define cancellation or exceptions inside its uninspected helper/provider closure, nor do they specify the generated geometry.

On its normal publication path it creates resource objects through the backend reached by World+0x70 and selected virtual calls. It copies the prior pending list's raw pointers to a temporary list through 0x7DC20 before entering World+0x3C8. Under that guard it stores the selected 64-bit bounds/centre fields and binary32 bound scalar, clears the pending list nodes through 0x3C9C0, clears the 32-bit counter C+0x444, and appends newly created 0x60-byte objects to C+0x1D8 in the produced layer order. Pending size C+0x1E0 is 64 bits. The counter increases with the selected 32-bit object counts; the directly reached contribution adds object+0x50, twice object+0x48, and object+0x44 at native integer widths. It then clears byte C+0x23C and releases the World guard.

Only afterward does that owner delete the old pending objects through their deleting virtuals, free the copied list nodes/sentinel and clean up temporary geometry/resource arrays. It does not directly replace the current list C+0x1C8 in this block. The list copy helpers copy pointer values, not the underlying resource objects; their assignment/clear operations free excess list nodes without deleting those pointed objects.

After the dispatch wrapper returns, worker 0x842D0 makes two separate passes through its selected coordinate list in stored order. The first begins each pair with Sleep(0), looks up the current registry value again, and proceeds only for a nonnull cell with nonzero 64-bit pending size C+0x1E0. It traverses pending objects in forward list order. For each object it calls nonnull resource fields at +0x10, +8, +0x18 and +0x28, in that order, through virtual slot +0x10. These are exact selected fields; this pass does not invoke that slot on every resource field indiscriminately.

The second pass looks up each coordinate again and applies the same nonnull/pending-size test. It copies the current list C+0x1C8 to a temporary pointer list before entering World+0x3C8, copies/assigns pending-list pointer values into the current list through 0x7CC20 while guarded, then releases the guard. After Sleep(0), it deletes the previously current objects, clears pending list nodes and resets C+0x1E0 to zero, and frees the temporary nodes/sentinel. Publishing current pointer values is therefore ordered before deleting the old current objects. This is per-cell work in a second pass, not one atomic all-cell publication transaction.

A normal numerical result with an empty pending list does not pass these nonzero-size tests. This worker therefore supplies no unconditional “empty pending result clears current list” rule. The source also supplies no universal lifetime guarantee for raw candidate pointers, copied list pointers or unlocked registry lookups during another owner's retirement. The selected lookup, lock, promotion and deletion orders are the supported facts.

The concrete 0x60-byte object constructor 0xC2650 selects table 0x4382B0 and defines its seven resource pointer fields as null. Its deleting target 0xC2860 reaches destructor 0xC2770, which visits nonnull pointers at +8, +0x10, +0x18, +0x20, +0x28, +0x30 and +0x38 in that order through their deleting virtuals; it frees the object when deleting flag bit 0 is set. Concrete identities and semantics of those externally returned resource receivers remain provider boundaries.

## Separate map-result worker and settings

Map owner R is G+0x10, constructed by 0x2EAA00 on the same ordinary World used by G+8. It stores that World at R+8 and its GUI argument at R+0x10, initializes its ordered registry at R+0x188, initializes critical sections +0x1A0/+0x1C8 with spin-count argument 16, and normally resizes the 0x68-byte group vector at +0x170 to nine entries. These map-result objects and registry nodes are distinct from the World's 0x58518-byte cells and +0x460 registry.

Complete worker 0x83C70 samples a timeGetTime baseline at entry. Its stop byte is Game+0x2068 and its ordinary-mode gate is the same RVA-0x5514C7 zero test. Under World+0x3C8 it copies the three 64-bit fields G+0x1E20/+0x1E28/+0x1E30 and signed 32-bit G+0x1E04, then releases the guard. A new time sample with unsigned elapsed strictly greater than 5000 reaches map retirement owner 0x2F3270 and advances that baseline. It subsequently invokes 0x2F7F50 on the copied fields. Ordinary iterations then request Sleep(10); the nonordinary branch requests Sleep(100). On exit it calls 0x2F3880 on R and returns. Neither sleep is a request-specific condition-variable wait.

Selection owner 0x2F7F50 requires the signed mode nonnegative and below the current signed group count computed using the 0x68-byte stride, and nonzero 64-bit string length at World+0x210. The mode is part of the map key; it is not the World role byte. The normal constructor supplies nine groups, so the ordinary unchanged vector admits modes zero through eight. Alternate writers of that vector or the Game scalar are not exhaustively traced here.

Spacing is obtained from original powf using checked binary32 base 2.0 and the mode converted to binary32, followed by multiplication by checked binary32 4.0 and signed 32-bit conversion. A result below one is replaced with one. The raw image holds signed 32-bit values 16, 8 and 3 at RVAs 0x5469A0, 0x5469A4 and 0x5469A8. The selector multiplies the first of those live operands by spacing to obtain its grid width and uses the second as the enumeration half-width. Their untraced later writers remain state dependencies; the raw initial values are not a universal immutable-runtime promise.

It truncates the copied first/second fixed-point coordinates to whole coordinates and applies its selected negative floor division by grid width. Enumeration visits increasing first index outside increasing second index, from centre minus the live half-width through centre plus that value with the upper limit excluded. Each key contains three signed 32-bit values: first index, second index and mode. Registry lookup/insertion owner 0x2EA520 and its comparison closure use signed lexicographic ordering of those three values.

New registry-node constructor 0x2E6950 defines its result pointer at node+0x30 as zero. The selector creates/finds each key before testing that pointer; a null result appends a twelve-byte record containing two indices and signed 32-bit squared index distance. Thus this family can have registry placeholders before dispatch. Null here is not an in-progress marker: the next selector still admits it.

Sort 0x2EA0D0 and its complete selected comparison/heap/partition closure order these records by signed +8 ascending, with no secondary key or universal tie stability. The selected resize retains at most eight records. A nonempty selection builds a parameter carrier containing raw R plus pointers to the local 32-bit grid width, spacing, auxiliary sample parameter and mode. When positive spacing is below 32, that auxiliary parameter is the smaller of two and the signed quotient of 32 by spacing; otherwise it is one. These selected local values remain in this caller until its dispatch wrapper returns. Unlike the viewed-cell record, this route explicitly carries these settings by pointers to its live stack carrier.

Wrapper 0x2EA7E0 uses the original PPL trace/import route, calls numerical map owner 0x2EB7B0 directly for a single record, or reaches complete wrapper 0x2E8CE0 for more. Thunk 0x2E8680 selects complete callback 0x2F21F0. Its range-broker, cancellation-beacon, split-chore and normal RunAndWait/cleanup boundaries are the separately verified map versions of the mechanisms described above. The map worker does not itself request the viewed worker's priority/policy setup.

The selected entry of map numerical owner 0x2EB7B0 allocates a 0x90-byte result through 0x2EAB60, stores a 32-bit start-clock sample at result+0x10 and obtains its primary resource through 0x1739A0 using World+0x70. Its large internal numerical body is explicitly unclosed here. On the selected normal publication continuation it takes another time sample, writes the unsigned 32-bit elapsed difference to result+0x88, enters R+0x1A0, finds/creates the three-integer key through 0x2EA520 and unconditionally stores the new result pointer at node+0x30, then unlocks and cleans up its temporary buffer. No existing-result comparison or old-result deleting call occurs in that selected publication block. The low-byte success returned by selector 0x2F7F50 after normal wrapper return is not a per-result proof that every selected callback completed or published.

Map retirement owner 0x2F3270 first gathers null-result nodes for removal and nonnull-result nodes for sorting. Its selected sort 0x2EA250 and complete comparator closure order nonnull entries by the signed 32-bit field at result+0xC in descending order. This is a different operand and direction from request distance. Its purpose and full writer census are not inferred from that offset; in particular no wall-clock-recency contract is supplied.

In the ordinary valid-count domain it retains the first 1000 nonnull entries. For later entries it reaches Sleep(0), clears node+0x30 under R+0x1A0, releases that guard, destroys/frees the detached result, and adds the node to the removal list. Every node in that list is then erased while holding R+0x1A0, with Sleep(0) after unlock. This complete owner does not hold that section for its initial traversal and sorting.

Map exit drain 0x2F3880 constructs an independent tree-node copy of R+0x188 through 0x2E6750 and its recursive copy closure. The copied values include the same raw result pointers; no result-object clone occurs. It then holds R+0x1A0 while clearing the original tree, unlocks, destroys/frees each nonnull result reached in the copy and finally frees the copied nodes/sentinel. The tree copy precedes the clear guard. This is the map worker's own exit drain, distinct from the chunk worker's World-cell drain and the Host's final retained-state save. A universal copy/retirement synchronization or alias-safety property is not proved by those particular scopes.

## Cell retirement and shutdown ownership

Single-cell retirement 0x2D2410 first holds World+0x3C8 while sampling the current local entity's ordinary position and deriving a floored centre cell, then releases that guard. This helper does not use the worker's -100 alternative coordinates. It next holds World+0x3F0 while scanning nonnull cell values in the registry's native list order. The initial distance threshold is the signed 32-bit square of its supplied radius, converted to binary64. Ordinary deltas are signed 32 bits and their squared distance is accumulated in binary64. A delta whose selected signed absolute value exceeds twice the radius substitutes the signed 32-bit four-times-radius-squared result converted to binary64.

Only a distance strictly greater than the current best replaces the chosen candidate. Equality preserves the earlier encountered candidate; it does not replace it by a coordinate tie-break. A selected cell reaches 0x2D3E20 while the helper additionally holds World+0x3C8, then its registry node is erased by 0x2ABD20. The +0x3C8 guard is released, then +0x3F0. Only after both unlocks does the concrete cell deleting virtual run. The return is one for a selected nonnull cell, otherwise zero. This ordering differs from the Host's needed-set retirement order.

Host helper 0x2D2670 builds a temporary list of registry-node iterators for cells absent from the supplied signed-coordinate set while holding World+0x3F0, then releases that section. Its scan directly dereferences each selected node's cell coordinates; a general null-payload safety promise is not inferred from this valid-cell path. For each saved iterator it reads the cell pointer, enters +0x3F0 then +0x3C8, erases the registry node first, and calls 0x2D3E20 afterward. It leaves +0x3C8 then +0x3F0 and invokes the cell deleting virtual outside both locks. Copying raw iterators across the unlocked interval is not proof of their lifetime under every alternate caller.

Complete exit drain 0x2D2930, reached by the chunk worker when it observes Game stop, enters World+0x3F0, copies the registry's node/key/value structure into an independent local map, clears the original registry nodes and size and requests rehash to eight buckets. The map-copy closure copies the same raw cell pointers; it does not clone the cells. After leaving +0x3F0 it visits nonnull copied values. For each, it enters +0x3C8, calls 0x2D3E20, unlocks, and invokes that cell's deleting virtual. It then destroys the local map's nodes and backing storage. Clearing original membership precedes the per-cell retention/deletion loop. No World role/name test or request-persistence callback is added by this drain.

Retention owner 0x2D3E20 receives a concrete World and cell. A nonzero C+0xF9 first sets retained-entry flag bit 0 and copies C+0x30; a nonzero C+0xFA sets flag bit 1 and copies C+0x60. It next copies the selected C+0xD8 collection if nonempty or if the retained coordinate entry already exists. The analogous C+0x4D8 collection is considered next under its own nonempty-or-existing-entry condition. Finally, a nonzero signed 32-bit C+0x490 or an existing retained entry admits the retained scalar store. The corresponding empty/zero paths can therefore preserve absence of a new retained entry, while an existing entry can be updated with an empty collection or zero scalar. Each existing-entry test is made at that point in this owner's order; it is not an entry-time snapshot. This owner itself neither erases the live cell registry node nor initiates a worker, and it contains no role/name admission test around those stores.

Concrete cell table 0x4738C0 selects deleting owner 0x2FA3B0, which reaches complete destructor 0x2F9940 and frees the object when deleting flag bit 0 is set. That destructor tears down the reached cell-owned records, arrays, containers and render aggregate. The render aggregate destructor 0xC2680 visits the current object list first and the pending object list second through each nonnull object's deleting virtual, then frees pending list nodes/sentinel and current list nodes/sentinel. This concrete target, not an assumed universal destructor rule, establishes the selected current/pending ownership cleanup. Effects inside other unexpanded record/resource destructors remain boundaries. No universal World pointer-set/reference cleanup or safety of every outstanding raw iterator is inferred.

Game destruction 0x818D0 first considers the object at G+0x21F0 through its deleting virtual, then calls separate socket/thread disconnect 0x911E0. That disconnect clears its own connection field and, on its admitted active branch, clears byte G+0x21E8, closes a nonzero socket, reaches conditional receive/send joins and writes the viewed World role byte zero. It does not directly write Game+0x2068. These are separate connection/thread operations, not the cell worker's stop signal.

Only afterward, native 0x8191C writes Game+0x2068 to zero. The reached ordinary destructor order then joins the music route, reaches the selected backend virtual +0x20 call, joins the AI route, joins the viewed-cell generation route at G+0x1FA8, and joins the chunk route at G+0x1FB8. Conditional joins of the Host-update route, map route, receive route and send route follow, in that order. Socket and Game critical-section teardown follow those reached joins. Embedded Host destruction 0xF1F20 occurs later in the Game's member teardown.

Join helper 0x9AF20 tests the signed 32-bit thread ID at representation+8 and reaches original C++ error 1 for an empty representation. It tests for joining the current imported thread ID and reaches error 5, copies the selected sixteen-byte representation for imported _Thrd_join, reaches error 2 for nonzero join status, and on normal return clears the ID and eight-byte handle. Conditional joins in the destructor use their corresponding nonzero-ID admission before the same self/join-error boundaries. Remaining nonzero IDs at the selected member-destruction checks reach terminate. Imported join/error behavior and exceptional partial teardown are not silently replaced with successful cleanup.

The ordinary Game stop store does not clear Host byte H+0x550. Host generation can therefore remain independently admitted until its own destructor is reached; exact interleavings are not proved. Host destruction first writes H+0x550 zero, joins the thread representation H+0x558, tears down its selected Host-owned resources/containers and reaches World destructor 0x29B290 on H+8. The Host worker's own exit has already reached its retained-state save on its selected normal return. It did not itself perform 0x2D2930's World map drain.

Complete World destructor 0x29B290 deletes the selected live entity-list objects, then visits remaining nonnull cell-registry values, calls 0x2D3E20 and invokes each concrete cell deleting virtual. Only afterward does it delete World critical sections +0x3C8, +0x3F0 and +0x418 and destroy the remaining selected registry/collection storage. It contains no worker join of its own. The parent's particular join order and the worker's particular drain are separate ownership facts; no universal external-World lifetime protocol is inferred.

The stop byte is tested at worker loop boundaries. The selected cell/chunk/map dispatch owners do not poll it inside every numerical/provider operation. They make the described normal callback/wait/publication/cleanup calls before another loop test. Clearing the byte is not itself a source-proved cancellation request for imported task collections, an interruption of Sleep, or a transaction abort that undoes already completed publication.

## Source-only conditional sequences

These independently chosen sequences assume valid stable receivers, ordinary representable coordinates/counts, normal allocator/provider returns where stated, and only the specified changes. They are control-flow references, not runtime observations, scheduling guarantees or suggested implementation steps.

* Let the viewed worker sample whole centre (32,32), radius 64 and only missing pairs (0,0) and (1,0) within the admitted range; let all other candidates have nonnull values. Their distance priorities are 0 and 1024. Both survive the four-record selection, ordered by priority in the vector. The multiple-record path still uses the parallel wrapper, so the contract does not order their starts or publications.
* Let a selected coordinate already acquire a nonnull cell before its callback's 0x2AE0C0 entry lookup. That callback returns the then-current cell without this owner's new allocation, srand or numerical-provider call. If two independent callbacks instead both miss initially and both later complete normally, each can reach the unconditional map-value assignment; the selected publication block supplies no old-cell deleting call for the later replacement.
* Let a normal chunk provider return an empty pending list while a current object list is nonempty. With pending size zero, both later worker promotion passes skip that coordinate. No “empty clears current” effect follows from these particular tests.
* Let a chunk candidate have zero counter C+0x444 and the current cumulative signed amount equal the computed budget. The outer comparison admits it and the remaining dirty/neighbour/count checks decide appending. A strictly larger amount suppresses that zero-counter candidate. A nonzero counter can admit the outer branch even with the larger amount, and its signed value contributes to the cumulative amount when that branch is reached.
* Let the Host's copied eligible-position list be empty and all live cell entries be valid nonnull objects. Its needed-coordinate set is empty. The reached 0x2D2670 path selects every such entry for its erase/retain/delete sequence, after which the Host has no missing request from that empty set and reaches the empty-work sleep. This is independent of the viewed World's local entity and request vector.
* Let an existing cell's cached 32-bit value +0x4F8 equal World+8. The reached 0x31C540 call returns before its refresh. Change only the World value for a later admitted call: the cache is written before the selected refresh and changed-record accumulation. This is not a request to the cell worker, and the later World accumulation drain has its own call sites.
* Let a map selector create a key whose result pointer is null, and let that callback be skipped by provider cancellation before numerical execution. The placeholder remains null in this selected closure, so another independently reached selector can admit it again. Completed results from other callbacks are not rolled back by that missing one.
* Let Game stop be cleared after a selected batch has been formed. The described callback owners contain no direct check of that byte per item. Their imported cancellation/provider behavior remains independent; after their selected normal returns the outer worker reaches its next stop path. The viewed-cell worker's selected exit can save a character, the chunk worker clears/retains/deletes World cells through 0x2D2930, and the map worker drains its separate registry through 0x2F3880. These are different operations.

## Coverage, evidence limits and exposure

Only the standalone author policy at C:/Users/DieuDeGlace/Documents/Cubeworld-Vulkanengine-Steam/docs/CLEANROOM_AUTHOR.md and filename-selected original-side evidence were read. Original metadata is confined to decompiled/functions.csv, decompiled/function_ranges.csv and decompiled/calls.csv beneath the original reference root. Original C exports are the individually selected decompiled/functions/140 plus six-digit lowercase RVA plus .c files represented below. Native instructions are from the selected original decompiled/assembly.txt, compared against the PE-mapped raw bytes of the identified original image.

Complete lifecycle/selection bodies were expanded for the Game/Host constructors and destructors, selected start/join adapters, viewed/Host/chunk/map worker loops, request selectors and sort comparison closures, parallel wrapper/callback/broker owners, shared cell execution/publication, selected list/map copy-clear-retirement owners, complete cell refresh and its reached World caller/drain, and the selected concrete cell/render-aggregate/resource-container lifetimes. Complete renderer chunk owner 0xE9530 was read for admission, publication and lifetime boundaries; its transitive geometry/font/graphics/numerical helper closure is not independently derived here.

Host update 0xF5520 was expanded as a complete owning caller, with its packet/provider operations left as boundaries for this contract. Application 0x1355A0 received complete byte comparison and selected ordinary-construction context inspection. Game update 0xA5A50 has a timed-out C export; its complete recognized native ranges were compared, and the cited configuration/substep/accumulation/guard contexts were independently read. Neither that entire Game update's unrelated systems nor every alternative entry to its later blocks is described.

Numerical provider 0x2FF6F0 has a timed-out C export; all its recognized original instruction bytes were compared, but its internal numerical/control/helper closure is not presented as independently reconstructed. Large map provider 0x2EB7B0 likewise has complete owner-level byte comparison but only the cited entry and normal publication contexts are semantically relied upon here. No universal numerical result, random sequence, noise/table initialization, imported PRNG state sharing, resource allocation success, GPU completion or failure/exception cleanup is invented for these boundaries.

The selected worker bodies prove the zero test at global RVA 0x5514C7. The bounded direct-operand scan of the original exported instructions finds the three identified reader owners 0x83C70, 0x842D0 and 0x84F70 and no explicit same-operand writer. This does not exclude wider overlapping stores, pointer-based writers, imported initialization or unrecognized instructions. That RVA is in the original image's unbacked tail of .data, so a raw-file byte is not supplied as an observed live process value. Byte 0x5514CF is not substituted for this operand.

The concretely reached Game/Host stop writers are the one-byte constructor/destructor stores described above. There is no universal alias/writer census for those object offsets, World+0x358, tuning globals/scalars, resource-provider state or registry payloads. In particular the settings/World guards shown here do not prove that every other writer uses the same lock. The original has unlocked scans, copies before guard entry, raw pointers across unlocks and different simultaneous lock orders in different selected paths. Their existence is recorded without inferring either universal race freedom or a particular failing schedule.

Imported thread, critical-section, clock, sleep, scheduler, cancellation, allocation, exception, database and resource operations remain provider boundaries. Unsigned 32-bit elapsed comparisons are modulo 2 to the power 32 and use the exact strict boundaries stated above; they do not establish real-time deadlines, monotonic physical time over arbitrary wraps, fairness or maximum stop latency. Floating rounding modes, exceptional values and out-of-range conversions are not generalized beyond the selected ordinary finite arithmetic descriptions.

Exposure statement: clean original-only exposure. No active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, project history, replacement engine/exporter/extractor, SDK/dependency/reconstruction source, tests/results, owner outputs/data/captures, packages or earlier held handoff was read or modified. No additional agent was spawned and no implementation/runtime feedback was requested or received. The new artifact was created exclusively and fully read back before the reported FINAL / STABLE / HELD boundary. Earlier held files remain unread and unchanged. All writes to this artifact stop at that boundary.

## Complete owner-level byte manifest

The owner-level comparison covers 143 entries, 62,260 exported instructions and 265,342 instruction bytes, with zero mismatching bytes. Ten separately decoded thunks add 62 bytes and thirteen instructions, also with zero mismatches; they are not included again in the owner totals.

The ranges below are start-inclusive/end-exclusive RVAs. Discontiguous recognized ranges remain separate; omitted failure traps/padding and unsupported gaps are not assigned invented instructions. Complete byte matching is distinguished from the semantic/provider limits above.

| Owner/file RVA | Original instruction ranges | Compared bytes | Instructions |
| --- | --- | ---: | ---: |
| 0x07EA80 | 0x07EA80–0x7F35E; 0x07F35F–0x7F36E; 0x07F36F–0x7F37E; 0x07F37F–0x7F38B; 0x07F38C–0x7F9E0 | 3932 | 806 |
| 0x31C540 | 0x31C540–0x31D428 | 3816 | 1003 |
| 0x07D140 | 0x07D140–0x7D1C6 | 134 | 33 |
| 0x07D560 | 0x07D560–0x7D5B8 | 88 | 25 |
| 0x076160 | 0x076160–0x762C5 | 357 | 92 |
| 0x0762D0 | 0x0762D0–0x76435 | 357 | 92 |
| 0x075D10 | 0x075D10–0x75E75 | 357 | 92 |
| 0x075BA0 | 0x075BA0–0x75D05 | 357 | 92 |
| 0x075E80 | 0x075E80–0x75FE5 | 357 | 92 |
| 0x075FF0 | 0x075FF0–0x76155 | 357 | 92 |
| 0x2E17D0 | 0x2E17D0–0x2E2E76 | 5798 | 1498 |
| 0x2E0B50 | 0x2E0B50–0x2E0DA8 | 600 | 158 |
| 0x03C7D0 | 0x03C7D0–0x3C7E6 | 22 | 7 |
| 0x086630 | 0x086630–0x86664 | 52 | 16 |
| 0x0818D0 | 0x0818D0–0x82268 | 2456 | 592 |
| 0x088E40 | 0x088E40–0x88ED6 | 150 | 45 |
| 0x088EE0 | 0x088EE0–0x88F76 | 150 | 45 |
| 0x088F80 | 0x088F80–0x89016 | 150 | 45 |
| 0x089020 | 0x089020–0x890B6 | 150 | 45 |
| 0x0890C0 | 0x0890C0–0x89156 | 150 | 45 |
| 0x089160 | 0x089160–0x891F6 | 150 | 45 |
| 0x09AF20 | 0x09AF20–0x9AF93 | 115 | 31 |
| 0x0842D0 | 0x0842D0–0x84F66 | 3222 | 856 |
| 0x084F70 | 0x084F70–0x8576A | 2042 | 562 |
| 0x07D680 | 0x07D680–0x7D71D | 157 | 39 |
| 0x2D2410 | 0x2D2410–0x2D2666 | 598 | 173 |
| 0x2D2930 | 0x2D2930–0x2D2A7C; 0x2D2A7D–0x2D2A9A; 0x2D2A9B–0x2D2AAA; 0x2D2AAB–0x2D2ABA; 0x2D2ABB–0x2D2AC7; 0x2D2AC8–0x2D2B33 | 510 | 129 |
| 0x2ABB70 | 0x2ABB70–0x2ABB7E | 14 | 2 |
| 0x2ABB80 | 0x2ABB80–0x2ABB8E | 14 | 2 |
| 0x2B9890 | 0x2B9890–0x2B989E | 14 | 2 |
| 0x2B98A0 | 0x2B98A0–0x2B98AE | 14 | 2 |
| 0x07D5C0 | 0x07D5C0–0x7D679 | 185 | 45 |
| 0x0A1E10 | 0x0A1E10–0xA1ECE | 190 | 55 |
| 0x0F0790 | 0x0F0790–0xF0902 | 370 | 102 |
| 0x0A2630 | 0x0A2630–0xA271A | 234 | 65 |
| 0x2AE0C0 | 0x2AE0C0–0x2AE391 | 721 | 192 |
| 0x076DA0 | 0x076DA0–0x770E4; 0x0770E5–0x7711A | 889 | 214 |
| 0x077120 | 0x077120–0x77464; 0x077465–0x7749A | 889 | 214 |
| 0x0F0070 | 0x0F0070–0xF0145 | 213 | 59 |
| 0x0F02C0 | 0x0F02C0–0xF044A | 394 | 103 |
| 0x0F06B0 | 0x0F06B0–0xF0788 | 216 | 61 |
| 0x0F0450 | 0x0F0450–0xF06A8 | 600 | 171 |
| 0x07BB20 | 0x07BB20–0x7BC95 | 373 | 102 |
| 0x0CE3E0 | 0x0CE3E0–0xCE573 | 403 | 105 |
| 0x0E9530 | 0x0E9530–0xEC326; 0x0EC330–0xECD9B | 14433 | 3044 |
| 0x2FF6F0 | 0x2FF6F0–0x305FCB; 0x30604D–0x30653C; 0x306587–0x306D80; 0x306DDE–0x307B76; 0x307B80–0x308ECD; 0x308ED0–0x308EDB; 0x308EE0–0x314A7C; 0x315F59–0x3162BD; 0x3162C0–0x3173A2; 0x3173D7–0x31742E; 0x317487–0x317FD1; 0x318036–0x319041 | 98881 | 22519 |
| 0x2F9400 | 0x2F9400–0x2F978D | 909 | 194 |
| 0x0858E0 | 0x0858E0–0x85CB2; 0x085CB3–0x85DC0 | 1247 | 282 |
| 0x085DC0 | 0x085DC0–0x8618F; 0x086190–0x862A0 | 1247 | 282 |
| 0x0F2340 | 0x0F2340–0xF2F62 | 3106 | 829 |
| 0x2DA6C0 | 0x2DA6C0–0x2DBA37 | 4983 | 1216 |
| 0x2D3E20 | 0x2D3E20–0x2D4286 | 1126 | 329 |
| 0x299640 | 0x299640–0x29971C | 220 | 61 |
| 0x07A770 | 0x07A770–0x7A8B0 | 320 | 85 |
| 0x0EF370 | 0x0EF370–0xEF46F | 255 | 69 |
| 0x0758D0 | 0x0758D0–0x759E5 | 277 | 78 |
| 0x079CB0 | 0x079CB0–0x79FB3 | 771 | 234 |
| 0x076970 | 0x076970–0x76A42 | 210 | 59 |
| 0x07B460 | 0x07B460–0x7B57B | 283 | 78 |
| 0x0F17A0 | 0x0F17A0–0xF1995; 0x0F1996–0xF19A5; 0x0F19A6–0xF19B5; 0x0F19B6–0xF19C2; 0x0F19C3–0xF1A41 | 669 | 160 |
| 0x0F3220 | 0x0F3220–0xF32B6 | 150 | 45 |
| 0x2D2670 | 0x2D2670–0x2D2925 | 693 | 186 |
| 0x2D38E0 | 0x2D38E0–0x2D39B4; 0x2D39B5–0x2D39C4; 0x2D39C5–0x2D39D4; 0x2D39D5–0x2D39E1; 0x2D39E2–0x2D3A10 | 300 | 68 |
| 0x0F1F20 | 0x0F1F20–0xF20FE; 0x0F20FF–0xF2105; 0x0F2106–0xF210C; 0x0F210D–0xF2113; 0x0F2114–0xF2339 | 1045 | 258 |
| 0x0F4E60 | 0x0F4E60–0xF4F4A | 234 | 65 |
| 0x088680 | 0x088680–0x88791 | 273 | 86 |
| 0x076C20 | 0x076C20–0x76C8D | 109 | 31 |
| 0x0F0150 | 0x0F0150–0xF02B5 | 357 | 92 |
| 0x29A130 | 0x29A130–0x29AF46 | 3606 | 826 |
| 0x29B290 | 0x29B290–0x29B39E; 0x29B39F–0x29B3BB; 0x29B3BC–0x29B3CB; 0x29B3CC–0x29B3DB; 0x29B3DC–0x29B3E8; 0x29B3E9–0x29B443; 0x29B444–0x29B460; 0x29B461–0x29B470; 0x29B471–0x29B480; 0x29B481–0x29B48D; 0x29B48E–0x29B4E8; 0x29B4E9–0x29B505; 0x29B506–0x29B515; 0x29B516–0x29B525; 0x29B526–0x29B532; 0x29B533–0x29B58D; 0x29B58E–0x29B5AA; 0x29B5AB–0x29B5BA; 0x29B5BB–0x29B5CA; 0x29B5CB–0x29B5D7; 0x29B5D8–0x29B632; 0x29B633–0x29B64F; 0x29B650–0x29B65F; 0x29B660–0x29B66F; 0x29B670–0x29B67C; 0x29B67D–0x29B6D7; 0x29B6D8–0x29B6F4; 0x29B6F5–0x29B704; 0x29B705–0x29B714; 0x29B715–0x29B721; 0x29B722–0x29B7A3; 0x29B7A4–0x29B7C0; 0x29B7C1–0x29B7D0; 0x29B7D1–0x29B7E0; 0x29B7E1–0x29B7ED; 0x29B7EE–0x29B838; 0x29B839–0x29B854; 0x29B855–0x29B864; 0x29B865–0x29B874; 0x29B875–0x29B881; 0x29B882–0x29BAE3 | 2091 | 514 |
| 0x073F40 | 0x073F40–0x74097 | 343 | 99 |
| 0x0745E0 | 0x0745E0–0x74812 | 562 | 173 |
| 0x29C310 | 0x29C310–0x29C33C; 0x29C33D–0x29C363 | 82 | 21 |
| 0x07A600 | 0x07A600–0x7A762 | 354 | 103 |
| 0x0705A0 | 0x0705A0–0x70776 | 470 | 142 |
| 0x2E1750 | 0x2E1750–0x2E17C7 | 119 | 27 |
| 0x2E30B0 | 0x2E30B0–0x2E3275; 0x2E3280–0x2E3503 | 1096 | 295 |
| 0x0A27B0 | 0x0A27B0–0xA2ADD; 0x0A2ADE–0xA2AED; 0x0A2AEE–0xA2AFD; 0x0A2AFE–0xA2B0A; 0x0A2B0B–0xA2BD7 | 1059 | 240 |
| 0x31D700 | 0x31D700–0x31DBAB | 1195 | 328 |
| 0x2ABD20 | 0x2ABD20–0x2ABE2B | 267 | 71 |
| 0x2FA3B0 | 0x2FA3B0–0x2FA3E4 | 52 | 16 |
| 0x07DC20 | 0x07DC20–0x7DC7A | 90 | 27 |
| 0x07CC20 | 0x07CC20–0x7CD67 | 327 | 92 |
| 0x03C9C0 | 0x03C9C0–0x3CA0B | 75 | 22 |
| 0x29C840 | 0x29C840–0x29C8F7 | 183 | 52 |
| 0x012700 | 0x012700–0x127EC | 236 | 64 |
| 0x29A010 | 0x29A010–0x29A085 | 117 | 32 |
| 0x2F9940 | 0x2F9940–0x2F9A5E; 0x2F9A5F–0x2F9A7B; 0x2F9A7C–0x2F9A8B; 0x2F9A8C–0x2F9A9B; 0x2F9A9C–0x2F9AA8; 0x2F9AA9–0x2F9EEB | 1446 | 350 |
| 0x074820 | 0x074820–0x748CB | 171 | 45 |
| 0x07AD00 | 0x07AD00–0x7AD28 | 40 | 10 |
| 0x299D40 | 0x299D40–0x299E0D | 205 | 50 |
| 0x07D8B0 | 0x07D8B0–0x7D924 | 116 | 32 |
| 0x0C2680 | 0x0C2680–0xC276F | 239 | 69 |
| 0x0C2650 | 0x0C2650–0xC267F | 47 | 13 |
| 0x06E4D0 | 0x06E4D0–0x6E501 | 49 | 14 |
| 0x1355A0 | 0x1355A0–0x137E1C | 10364 | 2217 |
| 0x0A5A50 | 0x0A5A50–0xA74D8; 0x0A7E6C–0xAA9FA; 0x0AAC40–0xABDE2; 0x0ABE34–0xABECA; 0x0ABF82–0xADBD8 | 29860 | 6106 |
| 0x0F5520 | 0x0F5520–0xF56BB; 0x0F56BC–0xF56C2; 0x0F56C3–0xF56C9; 0x0F56CA–0xF56D0; 0x0F56D1–0xF5829; 0x0F582A–0xF5830; 0x0F5831–0xF5837; 0x0F5838–0xF583E; 0x0F583F–0xF5A61; 0x0F5A62–0xF5A68; 0x0F5A69–0xF5A6F; 0x0F5A70–0xF5A76; 0x0F5A77–0xF6A52 | 5414 | 1284 |
| 0x083E60 | 0x083E60–0x83F15 | 181 | 48 |
| 0x0911E0 | 0x0911E0–0x913A5 | 453 | 92 |
| 0x0C2860 | 0x0C2860–0xC2894 | 52 | 16 |
| 0x0C2820 | 0x0C2820–0xC2854 | 52 | 16 |
| 0x0C2770 | 0x0C2770–0xC2814 | 164 | 52 |
| 0x083C70 | 0x083C70–0x83D86 | 278 | 65 |
| 0x2F7F50 | 0x2F7F50–0x2F8369 | 1049 | 283 |
| 0x2F3270 | 0x2F3270–0x2F366B; 0x2F366C–0x2F3688; 0x2F3689–0x2F3698; 0x2F3699–0x2F36A8; 0x2F36A9–0x2F36B5; 0x2F36B6–0x2F36F9 | 1156 | 306 |
| 0x2F3880 | 0x2F3880–0x2F39D2 | 338 | 85 |
| 0x2EA7E0 | 0x2EA7E0–0x2EA88A | 170 | 41 |
| 0x2EA0D0 | 0x2EA0D0–0x2EA242 | 370 | 102 |
| 0x2EA520 | 0x2EA520–0x2EA5F7 | 215 | 68 |
| 0x2EA250 | 0x2EA250–0x2EA362 | 274 | 78 |
| 0x2E6750 | 0x2E6750–0x2E67A4 | 84 | 23 |
| 0x2F3780 | 0x2F3780–0x2F37FE | 126 | 35 |
| 0x2EB7B0 | 0x2EB7B0–0x2F21EC | 27196 | 6715 |
| 0x2E8CE0 | 0x2E8CE0–0x2E9024; 0x2E9025–0x2E905A | 889 | 214 |
| 0x2F8F60 | 0x2F8F60–0x2F8FDE | 126 | 41 |
| 0x2E85A0 | 0x2E85A0–0x2E867A | 218 | 60 |
| 0x2E94E0 | 0x2E94E0–0x2E96DA | 506 | 154 |
| 0x2E8980 | 0x2E8980–0x2E8AB0 | 304 | 83 |
| 0x2E9EB0 | 0x2E9EB0–0x2E9F46 | 150 | 43 |
| 0x2EB4C0 | 0x2EB4C0–0x2EB64F | 399 | 117 |
| 0x2EAA00 | 0x2EAA00–0x2EAB52 | 338 | 68 |
| 0x2EAB60 | 0x2EAB60–0x2EAC77 | 279 | 77 |
| 0x2F21F0 | 0x2F21F0–0x2F25AF; 0x2F25B0–0x2F26C0 | 1231 | 277 |
| 0x2E84E0 | 0x2E84E0–0x2E8594 | 180 | 53 |
| 0x2E87D0 | 0x2E87D0–0x2E8977 | 423 | 116 |
| 0x2E9DA0 | 0x2E9DA0–0x2E9EA7 | 263 | 74 |
| 0x2E9280 | 0x2E9280–0x2E94D9 | 601 | 194 |
| 0x2E9840 | 0x2E9840–0x2E9986 | 326 | 95 |
| 0x2E9990 | 0x2E9990–0x2E9A80 | 240 | 71 |
| 0x2E6D00 | 0x2E6D00–0x2E6DFF | 255 | 69 |
| 0x2E6E00 | 0x2E6E00–0x2E6ED6 | 214 | 60 |
| 0x2E6950 | 0x2E6950–0x2E69A6 | 86 | 22 |
| 0x2E8B30 | 0x2E8B30–0x2E8BB3 | 131 | 39 |
| 0x2E8BC0 | 0x2E8BC0–0x2E8C3F | 127 | 37 |
| 0x2E6A80 | 0x2E6A80–0x2E6B2B | 171 | 51 |
| 0x2E6B30 | 0x2E6B30–0x2E6BCE | 158 | 43 |
| 0x2E67B0 | 0x2E67B0–0x2E67F1 | 65 | 17 |
| 0x0C25C0 | 0x0C25C0–0xC264D | 141 | 37 |
| 0x2F7DE0 | 0x2F7DE0–0x2F7ECA | 234 | 65 |
| 0x2F7CC0 | 0x2F7CC0–0x2F7DD2 | 274 | 75 |
| 0x295D70 | 0x295D70–0x295DA1 | 49 | 14 |
| 0x06DD20 | 0x06DD20–0x6DD94 | 116 | 39 |


## Additional native ranges and data correspondence

The independently compared complete thunks are listed separately because their decoded instructions are not assigned to one of the named owner entries above.

| Complete range | Bytes | Instructions | Direct transfer |
| --- | ---: | ---: | --- |
| 0x75B80–0x75B89 | 9 | 2 | Receiver adjustment, then 0x858E0 |
| 0x75B90–0x75B99 | 9 | 2 | Receiver adjustment, then 0x85DC0 |
| 0x87BA0–0x87BA5 | 5 | 1 | 0x88E40 |
| 0x87BB0–0x87BB5 | 5 | 1 | 0x88EE0 |
| 0x87BC0–0x87BC5 | 5 | 1 | 0x88F80 |
| 0x87BD0–0x87BD5 | 5 | 1 | 0x89020 |
| 0x87BE0–0x87BE5 | 5 | 1 | 0x890C0 |
| 0x87BF0–0x87BF5 | 5 | 1 | 0x89160 |
| 0xF3070–0xF3075 | 5 | 1 | 0xF3220 |
| 0x2E8680–0x2E8689 | 9 | 2 | Receiver adjustment, then 0x2F21F0 |

Original table data checked against the image includes the selected thread-carrier entries at 0x437EA8, 0x437EB8, 0x437EC8, 0x437ED8, 0x437EE8, 0x437EF8 and 0x43FD18; Game deleting entry 0x436068; concrete cell deleting entry 0x4738C0; and resource-container/aggregate deleting entries 0x4382B0 and 0x4382C0. Only the selected valid entries are used; adjacent metadata is not promoted to additional virtual slots.

Checked raw numeric data includes 32-bit integer 64 at 0x432E84, binary32 word 0x3F4CCCCD at 0x42E530, binary64 word 0x4039000000000000 at 0x43FD20, binary32 words 0x40000000 and 0x40800000 at 0x4AC9D0 and 0x4ACA20, and 32-bit words 16, 8 and 3 at 0x5469A0/+4/+8. Other offsets, widths, event/call identities, loop/size limits, comparison directions, instruction conversions and lock/store order cited in this contract are in the byte-matched native owner ranges.

Full saved-artifact readback and its exact byte length/SHA256 are reported externally at handoff. No earlier held artifact participates in this document's evidence. FINAL / STABLE / HELD ends all writes to this artifact.
