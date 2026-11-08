# Terrain loading and profiling

The generator keeps 96 ready immutable zone results. Decoded terrain and resident
consumers may still own a result after its ready entry is evicted. A weak lookup
reuses those live results without promoting them into the ready LRU. This leaves
ready capacity for unowned neighboring inputs and adds no permanent terrain keep. Expired
weak entries are removed when a result enters the ready cache. The ready limit
stays unchanged; metadata follows the ready and externally owned results.

The complete lookup key includes zone coordinates, world-settings epoch,
option epoch and original-versus-hooked generation. A flush retires both ready
and live lookups. Existing consumers retain their old immutable values, while
new requests use a new generation. Concurrent missing readers still share the
same pending generation. This cache is an engine policy, not a claim about the
original game's allocator or source-zone eviction cadence.

## Native client source streaming

Generated sources in Steam rendering use a separate client loader. It selects at
most four missing coordinates from the native block-window/circle intersection,
ranks squared block distance to closed zone rectangles, and waits for the whole
source batch before selecting another. Persistent worker concurrency remains an
engine policy. Alpha rendering and partial-capture replay keep their adapters.
Alpha terrain can still supply the generated ground beneath Steam rendering.

Native mesh selection requires nine already loaded neighbors. Preparation captures
their immutable pointers under one cache guard.
Neither retaining a loaded zone nor preparing a mesh pads the generation region.
After the previous mesh batch completes, each cycle makes eight source-retirement
calls. Each samples the physical center again and removes at most one strictly
outside the inclusive mesh-radius-plus-one circle, using the native far-distance
clamp. Eagle destination demand remains separate from physical retention.

The loaded map uses the independent eight-byte hash, bucket segments, insertion,
binary32 load threshold, growth and erase rules. Read hits preserve traversal.
Actual serialized installation history controls ties; parallel source jobs do not
promise a prescribed publication order. Startup compatibility entries are replayed
oldest first before native mode; arbitrary earlier native history is not inferred.
Host and client mutable sources remain separate. A dedicated-server renderer
leaves source ownership with its host and does not start this client loader.

Retirement invalidates loaded membership before releasing storage outside the
cache guard. Held snapshots keep their own storage, while subsequent missing
block queries return the solid fallback. Client shutdown joins both work groups
and clears loaded membership. Existing local block journals replay in order after
fresh generation; an emptied journal does not revive older changes. Full native
retained-array/collection/scalar handling, connection-dependent restore gates,
host source-loader cadence and complete state restoration remain separate work.

Independent contracts cover [retention](specs/steam-terrain-source-retention-20261107.md),
[query admission](specs/steam-client-source-query-admission-20261107.md),
[generation ordering](specs/steam-source-generation-batch-order-20261107.md),
[map traversal](specs/steam-source-map-traversal-20261107.md) and
[retired state](specs/steam-source-retired-state-20261107.md).

## Source creation and the later light prepass

Native client source creation copies the canonical generator result without an
extra neighborhood lighting solve. Stored byte zero can initially be a color
component: the independent paths include zero-filled gaps, converted water 127
and direct 255 records. Query fallbacks and retained edits have separate rules.
The mesh prepass later solves lighting from resident neighbors and replaces only
stored byte zero for types 0/2. Other bytes and generator inputs stay unchanged.
No universal absence of transitive feature-helper lighting or writes is claimed.

The public API boundary test compares 44,485 stored records across three separated
Steam zones, applying effective-query water normalization. The prior wrapper
changed 20,303 air/water records before a mesh existed; the native path now matches
the canonical bytes. Isolated production tracing falls from 27 raw generations to
exactly the three requested coordinates. Compatibility Alpha rendering, capture
adapters and the current host reader retain their existing eager-lighting policy.
The [initial-byte contract](specs/steam-source-initial-block-light-20261107.md)
records stage distinctions and limits; no blanket air/water initialization is used.

## Optional diagnostics

Set `CW_PROFILE_MESH` to a private CSV path before starting CubeVulkan. The parent
directory must already exist. The file is replaced on the first profiling sample;
rows are flushed under a shared writer lock. Unset the variable for ordinary gameplay;
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
without an owner, release after reuse and eviction, ready working-set preservation,
cache flush, world/option epochs and original-versus-hooked isolation. Existing Alpha composition,
lighting, terrain publication and procedural-owner oracles remain separate checks.

Measured cold-fill results are recorded in the release status when validation
completes. Broader travel, memory pressure, native scheduling, frame performance
and original-game acceptance remain independent gates. Alpha terrain/lighting,
Omega water and the engine's enhancements retain their separate settings.
