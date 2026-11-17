# Steam authority resident refresh and random stream

Final source-side contract for the original Steam image `binary/cubeworld.original.exe.unpacked.exe`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`, preferred base `0x140000000`. Addresses below are image RVAs. Offsets are bytes from the named native object. The evidence is static.

## Selected zones and the day gate

The World tick at `0x2DA6C0` advances the clock via `0x2DA728` before the authority resident call at `0x2DA8DF`. The caller checks that byte `World+0x358` is zero at `0x2DA8CE`. The resident phase `0x2E17D0` also checks that byte.

The resident phase takes its proximity subjects from `World+0x40` where actor byte `+0x60` is clear. For each, it considers all zone cells between the horizontal position minus and plus 256 blocks on both axes, with inclusive endpoints and floor conversion to 64-block zone coordinates. It uses the loaded-zone registry at `World+0x460` and inserts only non-null zone pointers into an ordered set at `0x2E1C0C`. Duplicates collapse and the actual visit order is ascending unsigned Zone pointer, as shown by the comparator in `0x073F40`. For each selected pointer, the call at `0x2E1D7C` runs the daily routine `0x31C540` synchronously, then the same pass walks that Zone's descriptor tree at `Zone+0x460` in signed 64-bit descriptor ID order. The next selected zone follows after that zone's descriptor pass and any zone-level follow-through. A selected zone with no descriptors still reaches the daily call.

Each Zone constructor at `0x2F9400` stores 32-bit `0xFFFFFFFF` at `Zone+0x4F8` via `0x2F9732`. On the ordinary registry miss path, construction is called at `0x2AE16A`, before the generation call at `0x2AE179`, saved-zone overlays and publication. The identified saved-zone overlays restore change flags, object data, resident-related data and a zone counter. The remembered day stays at its constructor value through this traced path. The identified ordinary writes to this Zone field are initialization at `0x2F9732` and refresh at `0x31C596`. A registry hit preserves the live instance and its stamp. The separately stored world-state day can already have been restored before a new Zone is first selected.

The live day is the 32-bit field `World+0x08`, reached through the World pointer held at `Zone+0x08`. World initialization `0x2B5F20` writes day 1 at `0x2B66F7`. The optional saved `state` load at `0x2B98B0` passes `World+0x08` at `0x2B999D` and can replace the day at `0x148F43`. The ordinary clock increments the day at `0x2E2FF5` for each crossing of its 86,400,000-unit threshold. Zone creation uses `World+0x244` for a separate generation seed at `0x2AE13D`, with the Zone coordinates and a factor of 1024 on Y. Generation on a thread can therefore contribute to its incoming stream before the resident phase.

At `0x31C589` the live day is compared for equality with `Zone+0x4F8`. Equality takes `0x31C590` straight to the epilogue at `0x31D3F8`. The daily call then consumes zero draws, performs zero seeds and leaves the stamp untouched. A mismatch stores the sampled day at `0x31C596` before clearing the prior daily descriptor list at `Zone+0x4E8`, removing those descriptors from `Zone+0x460`, and copying the base object array at `Zone+0x78` to the live one at `Zone+0x60` with change detection. Thus a freshly constructed stamp of all ones also takes the equality path if the sampled World day is all ones. The mismatch path proceeds to the seed even when no spawn results.

## Daily seed

Use the signed 32-bit Zone X and Y stored at `Zone+0x10` and `Zone+0x14`. Add X, 57 times Y and `0x4373E1` as a 32-bit word. Mix that word by exclusive-or with its own 13-bit left shift, retaining 32 bits. Multiply the square of the mixed word by `0xEC4D`, add `0x0131071F`, multiply by the mixed word once more, and add `0xD208DD0D`. Each arithmetic step retains the low 32 bits. Clear bit 31 of the last result and divide the resulting nonnegative integer by `0x53E2` (21,474), rounding down. At `0x31C724` the routine rereads `World+0x08`. The seed is this latest day minus the quotient plus 100,000, retaining the low 32 bits. The call at `0x31C72F` supplies that word to UCRT `srand`. The stored stamp comes from the entry sample and the seed from the later sample.

The daily routine reaches the UCRT Utility API set import slots at image RVAs `0x42CD38` for `srand` and `0x42CD28` for `rand`. The former changes the UCRT random state of the calling native thread. The latter advances that same state, including from the nested helper calls below and the authority altitude call. The daily routine returns with this state in the CRT, with no stream object, returned RNG value or state restore. `0x31C72F` is the sole CRT reseed in the resolved daily and intervening resident paths described here.

## Daily draws for stored positions

The first population uses the 24-byte position span at `Zone+0x500` through the end pointer at `Zone+0x508` (exclusive). The constructor initializes an empty span. Ordinary generation can append points here via `0x317379` when the preceding `0x2AF7B0` call returns 2. On a stale day an empty span bypasses `0x31C77C` and its entire population loop, then proceeds to the additional encounter gate.

On a nonempty span of N entries, `0x31C77C` draws once. The record count is the larger of two and the remainder after dividing that draw by the quantity N plus one. Each successful loop iteration uses the corresponding position in entry order, starting at zero, and adds a descriptor to the Zone tree and daily list. A one-entry span still requests two records, so the second coordinate read is beyond its logical end. The per-record accounting assumes a valid readable position for each requested record.

Each of these records consumes the following calls in this order.

1. `0x31C7CF` selects the value at descriptor `+0x24` by remainder modulo 5.
2. The species picker called at `0x31C7F2` with input all ones uses `0x14133B` to select a list member, then `0x142142` to select a variant of that member. The caller at `0x31C7F9` makes another variant choice via `0x142142`. The ordered default base list is `00, 02, 04, 07, 09, 0B, 0D, 0F, 30, 33, 4F, 2D, 2B, E6, 34, 4C, BF, 34, 50, CA` in hexadecimal.
3. `0x31C80B` draws the initial byte at descriptor `+0x34` as the remainder modulo 4 plus one.
4. The appearance call at `0x31C832` and equipment initialization at `0x31C89A` advance the stream as described below. They run on each new descriptor. Position copying and insertion at `0x31C8D1` follow them.

Variant lookup `0x13F0D0` supplies a nonempty ordered list for the daily pickers. The multi-member groups are the pairs `00/01`, `02/03`, `04/05`, `07/08`, `09/0A`, `0B/0C`, `0D/0E`, `0F/10`, `16/17`, `4D/4E`, `53/54`, `A3/A4`, `E3/E4`, `EC/ED`, `104/105`, and the groups `28, 26, 27, 25` and `CA, CB, D0`, in hexadecimal. A member selects from its whole group. Other daily inputs take their singleton list. The draw at `0x142142` happens even for a singleton. Each repeated variant choice advances the stream again.

## Additional daily encounter

After the first population or its empty bypass, the routine rereads the live day for the gate at `0x31C934` through `0x31C973`. The signed 32-bit sum of X, three times Y, and that day must be divisible by five. When it is, `0x31C979` draws once and the encounter continues only on remainder zero modulo five.

On passage, six subsequent draws at `0x31C99C`, `0x31C9BB`, `0x31C9E0`, `0x31CA03`, `0x31CA43` and `0x31CA84` supply, in order, the provisional count (one plus the remainder modulo 4), the value for descriptor `+0x24` (modulo 5), a discarded result, a boolean (remainder zero modulo 4), and the two placement-cell offsets (each modulo 4). Each offset is added to four times the matching Zone coordinate for ordinary in-range coordinates. The location-based query at `0x31CAC1` and `0x31CAD1` determines a species-picker input. The call at `0x31CAD8` uses `0x14133B` and then `0x142142` even if placement will fail. This species picker has the same draw count for all inputs. Its ordered list changes with the input value. Input 6 or 11 replaces `30, 33` in the default list with three repetitions of `31, D8, A0`. Input 5 or 12 replaces `4F` with `128, A5, 61`. Input 9 or 10 inserts six entries of `53` after `4F`.

The tests at `0x31CAF1` and `0x31CB0E` look for static placement positions at the chosen cell. The key is two signed cell coordinates and a third integer equal to 5 when the type-5 pool is nonempty, or 1 otherwise. When the chosen pool is empty, the encounter exits at `0x31CB15`. The preceding nine draws remain consumed, counting from `0x31C979`. If there are positions, `0x31CB1B` draws again. Remainder zero modulo 4 forces one record, species `0x130` and byte `+0x34` of `0x9C`. With a nonzero remainder the classification call at `0x31CBBF` may return 5, which retains the preliminary species and clears the scenery boolean with no additional species draw. On the other classification branch, the call at `0x31CC4D` draws modulo 4 after obtaining a regional value through `0x31CC42`.

When `0x31CC4D` has remainder zero, the deterministic selector `0x10CD60` supplies a category from 1 through 8. Its result uses bits from the regional value plus `0x18365`, after left-13 and logical-right-17 exclusive-or shifts. The category is its low three bits plus one. Categories 2 through 6 select descriptor group 1, and the other categories select group 3. The corresponding reachable picker draws are as follows.

| Category | Call from refresh | Additional UCRT draws before the descriptor loop |
| --- | --- | --- |
| 1 or 2 | `0x31CCEA` or `0x31CD00` to `0x141D70` | `0x141DC8`, `0x141F4C`, `0x141F92` in order. The first selects whether species 9 joins 2 in the candidate list, the second selects a base member, and the third picks a variant. |
| 3 | `0x31CCAB` to `0x13FDA0` | `0x13FDFB`, then `0x13FE40`, for base `0x104` and its variant. |
| 4 | `0x31CCC7` to `0x13FEB0` | `0x13FF0B`, then `0x13FF50`, for base `0xE3` and its variant. |
| 5 | `0x31CCB9` to `0x142760` | `0x142BDB`, then `0x142142`. The ordered base choices are `61, E6, 0F, A5, 11, 10D` in hexadecimal. |
| 6 | Inline species `0xEC` | Zero. |
| 7 | Inline retained preliminary species | Zero. |
| 8 | `0x31CD13` to `0x140490` | `0x140886`, then `0x142142`. The ordered base choices are `02, 09, 00, 0D, 07` in hexadecimal. |

A nonzero remainder from `0x31CC4D` takes one draw at `0x31CD1E` and supplies it to the deterministic species selector `0x10EB10`. That call's regional index comes from the unsigned xorshift mix of the regional value plus `0x1337C`, modulo 11. Indices 0 through 7 select the ordered pair bases `00, 02, 04, 07, 09, 0B, 0D, 0F`, incremented by the low bit after the supplied draw's left-13 and right-17 exclusive-or shifts. Indices 8, 9 and 10 yield `0x30`, `0x33` and `0x4C`. The associated regional category and flags come through `0x10CEE0`, `0x10D310` and `0x10D2E0`. `0x10D310` selects group 1 when true and group 3 when false. The three selectors use arithmetic, tables and initialization synchronization, consuming zero UCRT random draws on the resolved path.

For each produced encounter descriptor, `0x31CDDF` invokes the variant draw at `0x142142`, `0x31CE2C` runs appearance, `0x31CE90` runs equipment initialization and `0x31CEAE` then uses the placement helper `0x31BD90`, before tree/list insertion. The daily descriptor constructor clears the 32-bit mode at `+0x8BC` as part of an eight-byte zero store at `+0x8B8`. All the scoped daily writes to `+0x8B8` store only 32 bits. The two daily population paths leave these new descriptors with mode zero at `+0x8BC`. Their appearance byte at `+0x34` is a different field.

For each placement helper call, the one possible draw is at `0x31BF92`. It takes the result modulo the count of a live 24-byte position vector at `Zone+0x110`, indexed by the chosen cell and pool type, returns the selected position and removes it from that vector in order. If the live vector is empty the helper copies the matching static pool from `Zone+0x100` first. With no usable positions it uses the zone-center fallback with zero draws. There is also a zero-draw shortcut for a type-3 lookup with an object of type `0x75`, before consulting the placement vector. The daily encounter calls supply type 1 or 5.

When the scenery boolean survives, the extra tail first uses placement via `0x31D064`, followed by one draw at `0x31D080` for the initial scenery variation modulo 4, even when its position falls outside the Zone. `0x31D195` then draws the number of further records modulo 4. For each of those, `0x31D1FE` draws its type selector modulo 8, with an extra draw at `0x31D268` only on result seven. The latter draw occurs even for species that force the final type to `0x85`. Placement at `0x31D2C7` follows, then `0x31D2E3` draws variation modulo 4 before the bounds gate. The object-list rebuild at `0x31D357` and final comparison/dirty-list operations consume zero direct or resolved nested draws. Returning after the chance or placement gates leaves all previously made draws consumed.

## Appearance and equipment draws

The two daily appearance calls both pass a null optional appearance input to `0x053E80`. Each consumes five draws at `0x053F21`, `0x053F3B`, `0x053F5A`, `0x053F88`, `0x053FAE` in that order. The first three supply bytes modulo 256 and the last two supply values modulo 100. Following the species dispatch at `0x0540BE`, each matching row gives its additional call sequence. The species keys are hexadecimal. Every other key consumes zero additional appearance draws.

| Species key | Ordered additional RVAs |
| --- | --- |
| `06` | `0x0544D8`, `0x0544F7`, `0x054516`, `0x054542` |
| `2E` | `0x0568E6`, `0x056905`, `0x056924` |
| `3A` | `0x05A9CB` |
| `53`, `54`, `76` | respectively `0x055AE5`, `0x055B0A`, `0x05563E` |
| `77` | `0x05C850`, `0x05C87E`, `0x05C8AC`, `0x05C8DD`, `0x05C90B`, `0x05C93B`, `0x05C9DA`, `0x05CA21` |
| `7C`, `8F`, `91` or `92`, `9F`, `A1` | respectively `0x05B3C8`, `0x05B940`, `0x056CB7`, `0x05C2A7`, `0x05C691` |
| `CA`, `CB` | respectively `0x055BC3` or `0x055BEA`, followed by `0x055C14` |
| `CC`, `CD`, `CE`, `CF`, `D0`, `D4` | respectively `0x05D691`, `0x05D6F0`, `0x05D74F`, `0x05D78F`, `0x0551FA`, `0x05D2AA` |
| `E3`, `E4`, `E7`, `E9`, `121` | respectively `0x055B4C`, `0x055B64`, `0x05B49C`, `0x05C022`, `0x05B500` |

The equipment call `0x0511E0` uses the resulting species at descriptor `+0x20`, group at `+0x1C`, appearance byte at `+0x34` and appearance flags at `+0x58`. Its first draw at `0x0512A8` occurs if classifier `0x065750` gives category 3. The category-3 species IDs, in hexadecimal, are `00` through `10`, `12`, `2B` through `2E`, `30, 31, 33, 34, 36`, `4C` through `51`, `53, 54, 5F, 94, 95, A7, BF, C7, CA, CB, D0, D8, E3, E4, E6, EC, ED, FC, FD, FE, 100, 103, 104, 105, 128, 130, 131`.

The middle draws use the following precedence. The flags are those at descriptor `+0x58` after appearance.

- Appearance byte `0x8A` has one draw, at `0x052244`, then jumps to the common tail. Bytes `0x80` through `0x87`, `0x8B` and `0x9B` jump to the tail directly. Species `0x12` with byte zero, or species `0xFC, 0x103, 0x100, 0x106, 0xFD, 0xFF, 0xFE`, also jump to the tail at this stage.
- Appearance byte `0x8F` has one draw at `0x0514FE` when flag `0x20` is clear, then goes to the tail. Bytes `0x90` and `0x88` go directly to the tail. The subsequent group-7/8, flags `0x18`, or species `0x76` shortcuts go directly to the tail. The same holds for the next species tests `0x75, 0x6C, 0x72, 0x6E, 0x2E, 0x73, 0x6D, 0x51, 0x52, 0x2F, 0x6F, 0x70, 0x71`.
- Species `0x2B, 0x2D, 0xA7, 0xC7, 0xE6` that reach the next stage have one draw at `0x0521D5`, then go to the tail. Otherwise an initial zero appearance byte draws at `0x051885` to choose byte 1 through 4, after which species `0x61` forces byte 4 and species `0xA5` forces byte 1 before the following dispatch.
- Byte 1 draws at `0x0518EB`, then shares the byte-`0x8D` branch at `0x051906`. On remainder zero modulo 3, `0x051906` is followed by `0x0519F5`. On remainder one it is followed by the pair `0x05198A`, `0x0519BD`. On remainder two it is followed by `0x051941`.
- Byte 2 consumes `0x051A07`, `0x051A29`. Byte 3 consumes `0x051A57`, `0x051A72`. Byte 4 consumes `0x051B0E`, `0x051B30`. Byte `0x8E` draws at `0x051B9A` after taking its own dispatch. On remainder zero modulo 4 it adds `0x051C6C`, on remainder one it adds `0x051C0A` and `0x051C3A`, and on results two or three it adds zero.
- Each path through that dispatch, including its default, then draws the seven outfit values at `0x05201F`, `0x05203E`, `0x05205D`, `0x05207C`, `0x05209B`, `0x0520C3`, `0x0520EB` when flag `0x20` is clear. Earlier shortcuts bypass these seven.

In the common tail, flag bit 9 causes a draw at `0x052709` and causes one more at `0x05272C` if the first chooses item `0x65` (index zero in its list of nine). The daily records use group 1 or 3, which take **both** nine-draw batches, at `0x0528CF` through `0x05292F` and `0x052975` through `0x0529D5`. The first batch is replaced by deterministic mixes for group 6. The second batch happens even when it overwrites the fields just supplied by the first. All of these draw advances must be retained even when only the later altitude matters.

## New mode-0x8A residents

The actor ID comes from descriptor `+0x38`. At `0x2E1DC8` through `0x2E1E2E`, an equal-key entry with a non-null attached actor in the tree at `World+0x450` suppresses creation and any altitude draw. The full 32-bit descriptor mode `0x8A` additionally clears a local one-shot flag on this existing-actor path. If that test permits further consideration, an equal ID in the tree at `World+0x20` also suppresses creation and any altitude draw, regardless of the attached pointer there. This second check is at `0x2E1E33` through `0x2E1E71`.

A new actor requires a proximity subject at most 256 blocks away in three dimensions, using the descriptor's original X/Y/Z and the actor list gathered before the Zone loop. The first matching subject continues the admission at `0x2E1EA6`. Additional descriptor gates are as follows.

- If signed `+0x920` is nonzero, the activity total must be at least that value. The sum spans X and Y from the descriptor's Zone minus eight to plus eight, inclusive. It takes `Zone+0x490` from an already loaded zone or the corresponding saved-record counter from `World+0x10` when a loaded pointer is absent. This test ends at `0x2E2079`.
- The optional coordinate test uses descriptor tree `+0x928` and count `+0x930` at `0x2E2089`. With a nonempty tree it starts a flag set at `0x2E2097`. A missing referenced live zone rejects immediately at `0x2E213A` or `0x2E214C`. It searches that zone's `+0x4A8` index for a valid object entry in the `+0x60` vector with a nonzero byte at object `+0x28` whose horizontal coordinates fall in the requested 16-block cell. A matching active object clears the flag at `0x2E2276`. An exhausted object search with the flag still set continues admission via `0x2E2282`. Once cleared, the flag stays clear across coordinate entries and reaching the coordinate-tree sentinel rejects at `0x2E22D1`. For a normal nonempty tree this amounts to requiring the first referenced zone to be loaded and the first queried cell to lack a matching active object.
- A nonzero selector at descriptor `+0x948` requires a matching actor in `World+0x40` with byte `+0x14C` equal to `0xA1` and unsigned byte `+0x19C` equal to the descriptor selector. The decision is at `0x2E234F`.
- For the optional list at descriptor `+0x938` with count `+0x940`, each required ID must be present in `World+0x20`. A missing one rejects at `0x2E23BB`.

These rejections occur before creation or the altitude draw. After allocation and copying the descriptor position and other fields, the pass resolves derived location and actor data at `0x2E2651` and `0x2E266A`. It reads the low byte of descriptor mode `+0x8BC` at `0x2E2677` and stores it at actor `+0x68`. For byte `0x8A`, `0x2E2686` consumes **one** value from the calling thread's UCRT stream. It takes that value's remainder modulo 50, adds 80 and multiplies by 65,536. At `0x2E26B1` this is added to the actor's existing signed 64-bit Z at `+0x20`. X and Y stay at their copied coordinates, and the copied base Z at actor `+0x1C8` remains the descriptor's original Z. The addition ranges from 80 to 129 blocks in 16.16 position units. A different low mode byte goes past this branch with zero altitude draws.

The one-shot flag is evaluated after the Z addition at `0x2E26B5`. Every later admitted descriptor with the same byte mode also draws. Follow-through at `0x2E28D5` uses an independent deterministic mix of the live World day for a zone-level effect. It consumes zero UCRT draws. At `0x2E283A` the new actor is appended to `World+0x40` and entered in the `World+0x450` tree by `0x29D0D0`.

## Carry-forward and observation vectors

On one native thread, a stale daily pass replaces the entering stream with the day-and-zone seed, then advances through each actual draw in execution order. A same-day pass preserves the stream entering that call. Each admitted mode-`0x8A` actor draws from the state left by the most recent seed plus every actual intervening draw. Consequently a second stale zone reseeds after the first zone's resident loop, and a second same-day zone can use the next output after the first zone's last admitted actor. Across separate ticks there can be other work on the calling thread. Between native threads the UCRT states are separate. The native routines here have no handoff of a random state across threads. The host loop call at `0x083EC6` leads to the World tick sites `0x0F6917` and `0x0F692D` synchronously, including when the first site is repeated for multiple simulation steps.

These vectors assume the ordinary UCRT recurrence used to calculate sample outputs, with 32-bit state multiplied by 214,013 and advanced by 2,531,011 before each output of bits 16 through 30. The original executable establishes the imports and call order. The numbers below are arithmetic predictions, with no observed runtime baseline. Take each candidate's base Z before the draw, with all admission conditions met unless stated otherwise.

| Situation | Expected seed and draw sequence |
| --- | --- |
| Fresh A at X 0, Y 0, day 1, empty span | Quotient 82,056, seed 17,945. The additional gate fails before a draw. The first and second admitted altitude draws are 25,871 and 15,424, giving 101 and 104 blocks. |
| A as above with one admitted actor, then B at X 1, Y 0 on the same thread with stamp already 1 | A takes 25,871. B reaches `0x31C590` and consumes 15,424 at its first altitude call, giving 104 blocks. The same holds if B has a nonempty stored-position span. If B is stale instead, it seeds 95,324 and its first altitude is 16,414, giving 94 blocks. |
| Fresh zone at X 4, Y 0, day 1, empty span | Seed 44,836. The additional gate reaches `0x31C979`, which consumes 15,382 and rejects on its remainder of 2. The first altitude draw is 4,292, giving 122 blocks. |
| Fresh zone at X -40, Y 3, day 1, with empty span and no positions in the chosen encounter pool | Seed 37,124. The initial chance draw is 22,965 and passes. The first nine draws end after the two nested species draws of 18,820 and 28,520. The placement test rejects and the first altitude draw is 18,865, giving 95 blocks. |
| Fresh A at X 0, Y 0, day 1, with two usable stored positions | Seed 17,945 and count 2. The selected final species are `0x2D` and `0x50`, with initial appearance bytes 1 and 4. The first record consumes 30 draws and the second 31, after the count draw. The first altitude uses draw 63, value 2,752, giving 82 blocks. |
| Fresh A with empty span and a previously queued mode-`0x8A` ID ahead of a new one | The first ID bypasses `0x2E2686`. The admitted new ID uses draw one, giving 101 blocks. The same draw ordering follows when any earlier ID fails an admission gate. |

For comparisons the distinguishing sequence is clock call `0x2DA728`, resident call `0x2DA8DF`, Zone selection `0x2E1D70` and refresh `0x2E1D7C`, then gate/write `0x31C589`/`0x31C596`, reseed `0x31C72F` if stale, ordered direct and nested draw RVAs, and finally altitude `0x2E2686` and Z write `0x2E26B1`. Logically equivalent ordering evidence includes the same native thread identity, actual Zone and World pointers, pre-refresh stamp, live day at both reads, seed argument, and the raw draw and position before the addition. When comparing consecutive zones, their pointer visit order is the join key for the two sequences.

## Coverage and open boundaries

The game-side daily and resident bodies use direct calls, resolved import slots, and internal indexed dispatch tables. Appearance paths are counted from the actual species-table targets in the image. The additional encounter still depends on live region classification, position pools, flags, and object histories. Those inputs determine which of the resolved conditional paths above will run. The complete region-classification and generation rules feeding the selectors and position pools remain unexpanded. Earlier unrelated activity on the same OS thread, and later inter-tick draws, can only be accounted for with a wider observation.

### Indirect targets

The shared static-initialization helper has an indirect wait at `0x392ED7` and an indirect wake tail at `0x392E6F`. In the original the supplying initializer at `0x392BF0` resolves the Windows condition-variable APIs and also supports an event fallback. This indirect platform dispatch serves synchronization for `0x10CEE0` and the relevant `0x31D470` helper branch. The eventual OS virtual addresses remain runtime-dependent. The separately imported UCRT implementation and actual thread identity at an observation point also remain runtime-dependent. Allocation failure, exceptional container limits, corrupted container state, and external changes to import targets are outside the resolved ordinary paths.
