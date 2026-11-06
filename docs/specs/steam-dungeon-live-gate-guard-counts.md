# Steam dungeon live gate guard counts

This contract covers the unpacked original Steam AMD64 image `cubeworld.original.exe.unpacked.exe`, preferred base `0x140000000`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`. Addresses are image RVAs. The scope is the type-8 and type-10 cell-object branch of `0x2e3510`, the two maps that supply its counts, and the reached state transition.

## Admission and refresh order

`0x2da6c0` calls the update at `0x2da73c`. Within `0x2e3510`, the first relevant collection walk uses the live list at `World+0x40`. A node's Creature pointer is at `+0x10`. The admission at `0x2e3c2e` tests that Creature's byte at `+0x60` for zero. An admitted pointer is appended to a temporary Creature list and also contributes to the selection of Zones to refresh. This is a separate selection from the later check for a live creature ID belonging to a spawn.

The admitted Creature's horizontal fixed-point coordinates at `+0x10` and `+0x18` are converted to their containing 64-block Zone coordinates, rounding down on each axis. The selection at `0x2e3d2a–0x2e3dd7` visits the seven by seven Zone-coordinate square centered there, inclusive of the three Zones on each side. It looks up each candidate through the live Zone index at `World+0x460`. Only a found entry with a nonnull Zone pointer joins the temporary selection. Insertion at `0x2e3dba` through `0x73f40` deduplicates by Zone pointer value. Subsequent traversal of this selection is by ascending unsigned pointer value. The order therefore depends on the live addresses.

For every selected Zone, the first pass clears its guard-count map through `0x509e0` at `0x2e4473`. It walks the Zone's outer spawn map at `Zone+0x460` in ascending signed spawn ID order. For each spawn with a nonempty guard tree, it tests the outer node's ID against the World creature tree at `World+0x20`. Exact membership of that ID admits the spawn to the count pass. This test does not use the Creature admission byte or measure the matched creature's distance from a gate. The count pass walks that spawn's guard tree at `Spawn+0x900` and adds one at `0x2e451a` for each distinct guard coordinate. The update inserts a missing coordinate through `0x1473e0` before incrementing its value. Thus a count represents matching live IDs with that membership, and a spawn can contribute to more than one coordinate. An unselected loaded Zone is outside this clear-and-rebuild pass.

The relevant object processing runs in a second pass over the same selected Zones, after all of those count maps have been rebuilt. Each selected Zone supplies its live object vector from `+0x60` and `+0x68`, traversed in stored order at stride `0xe0`. The object pass adds the update's binary32 time argument to record `+0x2c` before the gate decision. Rebuilding the maps does not itself test the World mode byte that later gates the state decision.

## Coordinate keys and threshold

The gate branch at `0x2e5255–0x2e527e` requires byte `World+0x358` to be zero, the object's 32-bit type to be exactly 8 or 10, and the object's state byte at `+0x28` to be nonzero. A zero state skips both tests described below. Other code in the update can handle other object types.

The gate key is signed X followed by signed Y, each in 16-block cells. The object stores signed 64-bit fixed-point X and Y with 16 fractional bits at `+0x08` and `+0x10`. The key uses the containing cell on each axis, rounding down through block and cell conversion, including for negative coordinates. Z is absent from the key. The gate uses this same pair in every count map it consults.

| Map owner and offsets | Meaning |
| --- | --- |
| `Zone+0x4c8` and `+0x4d0` | Head pointer and 64-bit size of the threshold map for that Zone's objects. |
| `Zone+0x4d8` and `+0x4e0` | Head pointer and 64-bit size of the live guard-count map. |
| Retained World Zone entry `+0x70` and `+0x78` | Head pointer and 64-bit size of the copied guard-count map for that Zone coordinate. |

These maps use signed 32-bit X and Y at node `+0x1c` and `+0x20`, with a 32-bit value at `+0x24`. `0x93730` performs the exact signed-pair lookup and returns the head on a miss. At `0x2e5352`, the threshold lookup uses the object owner's `Zone+0x4c8`. A missing entry exits this gate branch before either the count test or the proximity test. A present zero value proceeds as a threshold of zero.

The threshold comes from the Zone map. Several original generation writes are established within `0x2ff6f0`. The kind-8 cell-ID-`0x2e` branch assigns the result of `0x2c30b0` at `0x3050fb`, keyed by that 16-block cell. The helper can return 1 or one of 3 through 6. It also supplies the loop count in the kind-8 gate-side spawn producer at `0x2faa2f`. The branch through `0x30b90c–0x30b9d2` with a negative current layer index increments the threshold for descriptor byte values 2, 4 and 6 in an aligned eight by eight cell square. The nonnegative kind-8 branch at `0x30b9dc–0x30bae3` scans the corresponding two-by-two span of 1024-block regions using 16-block cells. For each cell admitted through `0x25eb30`, it examines the levels bounded by `0x25f5f0` and increments for each level whose `0x2a1d10` result is `0x0d`. The following access at `0x30bb07` can create a zero-valued key when no qualifying level increment occurred. Those are scoped writers of the value used by the runtime check. The full selection of structure paths and every possible threshold writer are outside this contract.

## Nearby sum and retained Zone counts

For an admitted gate with a threshold entry, `0x2e536a–0x2e53a6` forms the neighborhood independently of the Creature-centered refresh selection. On each horizontal axis it subtracts 1024 world blocks from the object's fixed-point position for the low endpoint and adds 1024 for the high endpoint. In fixed-point storage the offset is `0x4000000`. Each endpoint is converted by `0x16e50` to its containing 64-block Zone coordinate with floor semantics. The sum at `0x2e53ae–0x2e54a3` traverses that rectangle inclusively, ascending X outside and ascending Y inside. At ordinary positions the two endpoints are 32 Zone intervals apart on each axis, yielding a 33 by 33 rectangle. The search uses no Z coordinate, circular distance test or early exit when the threshold is reached.

For each Zone coordinate, `0x16e00` first looks up its pointer in the live World Zone index.

- A nonnull live Zone supplies its `+0x4d8` map. A missing gate key in this map contributes zero. Even an empty live map takes precedence over retained data.
- A null live result leads to lookup through `0x286af0` in the retained Zone tree at `World+0x10`. The outer key is the signed Zone X/Y pair. A found retained entry supplies the map at its node-relative `+0x70`. A missing retained entry or missing inner gate key contributes zero.

The accumulator starts at zero and adds each found dword in 32-bit arithmetic. The final comparison at `0x2e54a3` treats both the sum and the threshold as signed 32-bit values. A sum equal to or greater than the threshold requests state zero. A sum below threshold continues to the separate proximity and descriptor test. A loaded Zone that was outside the current refresh selection can still be consulted by the wider gate rectangle and contributes whatever its live map holds.

The retained map is a value copy. `0x2d3e20` checks the live guard-count size at `0x2d4030`. A nonempty live map allows lookup or creation of the retained Zone entry and replacement of its map. An empty live map is copied if the matching retained outer entry already exists. This count-map branch alone does not create a retained entry for a previously absent coordinate with an empty live map. At `0x2d4141–0x2d4151` it clears the destination then copies the live map through `0x6f000`. The key and count dwords are copied by value through `0x6e240`. The eviction path through `0x2d2410` calls this retention routine at `0x2d25fa` before removing the live Zone entry and destroying the Zone. On the matching restore branch of `0x2ae0c0`, the retained map is copied back into the new Zone at `0x2ae304–0x2ae315` after generation and before live publication. A later selected refresh clears those restored counts before recounting. These edges establish in-memory retention and replacement. They do not establish how long a retained entry survives other operations or what survives process restart.

## Below-threshold test

If the sum is below threshold, the branch at `0x2e54ee` queries `0x2a8290` for the gate's 16-block X/Y key and reads the result's unsigned byte at `+0x03`. The native fifth argument at `0x2e55ac` is byte 1. This path then walks the earlier temporary list of admitted Creatures in its stored order.

A Creature qualifies only when all of the following tests pass.

- Squared three-dimensional distance from the gate is strictly below 256. The three signed 64-bit fixed-point differences are converted to binary32 and scaled by the image constant `0x42e510` (one divided by 65536). The update separately squares the components and adds X and Y before Z. The binary32 comparison constant at `0x42e5c4` is 256. Exact equality does not pass.
- The block read through `0xeab0` at the Creature's fixed-point position has bit `0x20` set in its returned byte at `+0x05`.
- The same `0x2a8290` query at the Creature's containing 16-block X/Y cell, again with fifth argument 1, yields an unsigned byte at `+0x03` strictly greater than the gate's byte.

A qualifying Creature requests state zero through `0x2d3a10` at `0x2e573d`. The iteration advances through the rest of the list. Later matching calls after a successful transition encounter the already-zero state in the setter. With no qualifying Creature the branch leaves the gate state as it was. The contract uses the actual returned descriptor byte. Its full upstream taxonomy and the circumstances that produce each value are outside this audit.

## State transition and subsequent consumption

Both paths call `0x2d3a10` with a requested state byte of zero. At `0x2e54b6` this follows the successful count comparison and bypasses the proximity test. The setter returns immediately if the byte already equals the request. Otherwise it writes record `+0x28` and changes the binary32 timer at `+0x2c` to 1000 minus its current value, clamping a negative result to zero. The binary32 constant at `0x42e5d4` is 1000. On these transitions the setter also appends a positional sound event to the World list at `+0xa0`, with code `0x36` for type 8 and `0x39` for type 10.

When the state byte changes, the caller inserts the object's owning Zone pointer into `World+0x1a0` through `0x73f40` at `0x2e54e4` or `0x2e5765`. The observed consumer `0x2e30b0` marks each such Zone at `+0xfa`, queues a copy of the live object vector with the Zone's coordinates, runs its spatial rebuild `0x31d700`, and clears the pointer set. For both types, the spatial rebuild excludes the zero-state object from the applicable footprint processing. The renderer at `0xe0ac0` also distinguishes the two states and uses the timer in the transition. This branch only requests the transition to zero. It establishes no inverse transition when counts later decrease or when nearby creatures move away.

The ordinary update and retention paths above establish sequence and data ownership within the inspected calls. The audit does not establish a universal lock or cross-thread observation interval, a complete lifetime proof for the dirty-Zone pointer set, or a census of other gate-state writers.
