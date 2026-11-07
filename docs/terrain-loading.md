# Terrain loading and profiling

The generator keeps 96 ready immutable zone results. Decoded terrain and resident
consumers may still own a result after its ready entry is evicted. A weak lookup
reuses those live results without adding another strong terrain keep. Expired
weak entries are removed when a result enters the ready cache. The ready limit
stays unchanged; metadata follows the ready and externally owned results.

The complete lookup key includes zone coordinates, world-settings epoch,
option epoch and original-versus-hooked generation. A flush retires both ready
and live lookups. Existing consumers retain their old immutable values, while
new requests use a new generation. Concurrent missing readers still share the
same pending generation. This cache is an engine policy, not a claim about the
original game's allocator or source-zone eviction cadence.

## Optional diagnostics

Set `CW_PROFILE_MESH` to a private CSV path before starting CubeVulkan. The parent
directory must already exist. The file is replaced on the first profiling sample and rows are
flushed under a shared writer lock. Unset the variable for ordinary gameplay;
no profiling file is opened and no profiling clock is sampled when it is unset.
Keep logs and owner-specific run settings outside published source packages.

Mesh rows separate neighborhood loading, bounds, native lighting, stored-light
publication, setup, Alpha lighting, solids, water and finishing. Generation rows
separate original Steam generation, creature refresh, optional Alpha composition
and immutable storage. `generation_wait` records a reader waiting for another
producer; `generation_reuse` records recovery of a live result after ready-cache
eviction. Cached ready hits produce no rows. The `alpha_context` field denotes
Alpha rendering for mesh rows and Alpha terrain composition for generation rows.
Generation keys in the CSV distinguish intentional settings changes from repeats.

Times are elapsed wall time within each overlapping job, excluding the CSV
writer wait. Their sums are not process CPU time or a frame-time measurement.
Use matched cold runs, record the source revision and complete argument/settings
set, and compare completed work as well as elapsed time. A bounded startup run
does not establish a fully loaded scene or native visual parity.

## Checks and limits

The public generator API regression exceeds ready capacity, then checks that
concurrent readers recover the consumer-owned object. It also checks release
without an owner, release after reuse and eviction, cache flush, world/option
epoch changes and original-versus-hooked isolation. Existing Alpha composition,
lighting, terrain publication and procedural-owner oracles remain separate checks.

Measured cold-fill results are recorded in the release status when validation
completes. Broader travel, memory pressure, native scheduling, frame performance
and original-game acceptance remain independent gates. Alpha terrain/lighting,
Omega water and the engine's enhancements retain their separate settings.
