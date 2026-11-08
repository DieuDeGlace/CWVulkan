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

## Model samples after the light prepass

The same mesh preparation that writes stored air/water light now samples every
ordinary model record before terrain-face generation or scenery filtering.
Unnormalized float light and the sticky water bit live in an immutable model
vector. Scene preparation reads this cache until a later mesh; block edits alone
leave it intact. The scratch light cache permits zero and uses the record's
fixed position, while static objects retain their separate live gameplay query.
The captured nine-zone block lookup sets water flags without source creation.
Alpha scenery retains its own drawing-light path. See the independent
[stage contract](specs/steam-source-model-light-stage-20261107.md); the auxiliary
model-vector lifecycle and broader native-runtime acceptance remain open.

## Source-resident auxiliary light copies

Auxiliary model lights and static-light records are captured or projected from
the canonical generator once per loaded source lifetime. Ordinary records and
auxiliary models remain distinct value copies. The mesh prepass prepares both
model vectors before faces; point registration only reads the cached auxiliary
snapshot. Changing ordinary records or source files does not alias that snapshot.
A later solve can replace derived samples; unchanged samples reuse storage.
Retirement removes membership, and explicit reload starts a new auxiliary copy.
Held leases remain immutable. Lookup adds no source demand. Captured duplicate
auxiliary lists and invalid strides are rejected on load, and incomplete replay
keeps its established fallback. Broader source-state restoration and host loading
remain open. See the independent
[auxiliary lifetime](specs/steam-auxiliary-model-lifetime-20261107.md) contract.

## Generated host source worker

A generated host owns its source worker independently of the viewed client.
Each cycle copies the latest host-published physical/eagle block centres, forms
the native wanted union and retires all outside members before ranking missing
keys. The minimum admitted rectangle-distance key uses its 384-block per-centre
gate and the shared priority-only sorter. At most four selected factories run
serially; an existing hit retains its selected slot. The batch drains before a
fresh snapshot or stop. Empty demand can remove every source; idle work waits
five milliseconds and yields. Worker exit preserves remaining sources until
reader/World teardown or later policy. Alpha's finite domain remains enforced.

Host source creation now retains canonical raw bytes. Immutable source leases,
cache memo invalidation and separate host/client mutable data remain engine-owned.
The capture/standalone adapters and startup prehistory keep their stated limits.
Full selected-state retention/restoration, logical resident gates, native periodic
persistence and exact producer/worker timing remain separate work. See the
[host contract](specs/steam-host-source-cycle-20261107.md).

## Live residents and physical sources

The resident distance pass is separate from physical source demand. Byte-zero
registered actors supply centres; nonzero-category actors remain when any centre
passes the inclusive 320-block horizontal disk. Z and health do not participate.
No centres means all nonzero-category candidates leave. Selection finishes in
World registration order before owned records are erased.

Source-demand cleanup releases only empty resident buckets. A still-retained
actor can therefore outlive the physical source that supplied its template.
Owned ID-based removal tolerates vector relocation and preserves world progress;
full external alias cleanup, catalog-lease memory and World-step ordering retain
separate limits. See the
[retention contract](specs/steam-live-resident-retention-20261107.md).

## Source summary producers

The selected-source prelude clears and rebuilds coordinate counters using exact
catalog-entry membership in the World+0x20 identifier lane. It then independently
resets the scalar and counts matching entries of the source's tracked list. A
matching catalog template can contribute to multiple coordinate keys; two
matching templates can increment the same key. Repeated tracked identities count
per list entry. A template's own ID is not substituted for its catalog-entry key.
The pass does not query admitted live actors or change arrays, override flags or
edit history. See the
[summary contract](specs/steam-source-prerequisite-summaries-20261107.md).
Physical cached-summary binding and freshness, loaded/retained consumer routing
and complete update/admission order remain separate work.

## Physical source request control

Source F8 is loaded-lifetime control, separate from the saved ground/object flags.
Fresh identities start flagged; terrain COW shares that flag, but reload does not.
Replies clear only a currently loaded source. Connection reset visits all loaded
sources rather than just lazily created logical arrays, flags even empty-history
sources, and reverses their edit journals.

The viewed nonzero role predicate admits an X-major nine-by-nine loaded-only
scan around truncated/narrowed player block coordinates, or the eagle destination.
The scan does not load sources, change their recency or clear their flags. Host
reply admission separately requires a physical source and holds its lease while
copying values, independently of the host source's F8 value. See the
[F8 contract](specs/steam-source-f8-control-lifetime-20261107.md) and
[reply mapping](specs/steam-source-state-reply-mapping-20261107.md).
Connection writer/timing integration, full five-field physical ownership and
periodic persistence remain separate work.

## Retained source flags and the restore gate

Saved ground-item and static-object overrides restore their corresponding sticky
source flags even when the saved array is empty. Host replies therefore include
restored ground overrides. Once a retained entry exists, later empty collections
and zero counters remain authoritative values rather than requests to keep old
state. Recording order remains arrays, edits, count map and scalar.

`skipRestore` represents nonzero World+0x358, independently of dedicated-host
status. It suppresses restoration after generation; later unload recording has no
such internal gate. These helper semantics are covered by the
[field-lifetime contract](specs/steam-retained-source-field-lifetimes-20261107.md).
Full physical source binding, connection ordering and periodic persistence remain
separate work.

## Captures and procedural seeds

A scene capture is a screenshot of the nearby prepared view. Game and Alpha
sources generate further regions from the seed as the player travels; sampled
reference exports do not define the procedural world's boundary. A whole-world
export or all-at-once terrain bake is not required.

Capture/benchmark readiness follows required work. Because native generation
truncates fixed positions while retirement floors them, missing fringe sources
can be generated and retired repeatedly at negative fractional boundaries.
Those ordinary missing inputs outside physical retention no longer block a
prepared capture. They still enter the same native source batches. Required
sources, dirty meshes and required mesh jobs remain blocking; changed source/
view settings, physical positions and destination work receive conservative
readiness handling without extra player-provider reads. This is an owned
diagnostic policy. Original frame comparison and broader runtime acceptance
remain separate from obtaining a nonblank screenshot.

## Physical source gameplay state

Generated source identities share one mutable gameplay payload across their COW
terrain snapshots. The payload holds ground/static arrays, edit history, keyed
counts and the tracked scalar, with its own current source catalog. The owning
World stores retained values by coordinate and weakly indexes loaded payloads;
that index adds no permanent source keep. Retiring the physical cache entry ends
loaded membership even when an old immutable terrain lease survives.

Factory production happens before the guarded restoration/install boundary.
Stored array flags and repeated edits are applied before an ordinary fresh source
becomes visible. A quiescent bootstrap adapter can bind payloads after its initial
raw home read; that initialization is separate from ordinary factory parity.
Existing sources retain their live values when a retained archive is
rebound. Actual viewed, host-required and whole-map retirement record selected
fields in their specified relative order. The Alpha rendering adapter also
records its real LRU removals. Captured replay retains its logical-list adapter.

Mutable raw list views require a complete owner World scope. Scenery consumers
and copied light sources hold that scope while reading; immutable column reads
keep their existing memoized path. COW writers acquire the World scope before
the terrain/cache guards. Ordinary frame submission and source-worker joins run
outside the broad frame scope; scenery/resource preparation retains the guards
needed by its borrowed state. This consistent owned locking is an engine policy;
the original's universal writer/guard discipline and arbitrary races are unproved.

Dirty memberships weakly identify current physical instances in unsigned source
order. Publication sets sticky override flags, copies current arrays even when
empty, performs the final object pass, then clears pending memberships. Retired
instances cannot target a later coordinate replacement. Discarding invalid owned
memberships is a safety policy; native per-source erasure was not established.
The mirror keeps its existing outbound-array suppression while preserving flags.

Retained save input and current metadata have shared owned lifetimes, so the
background loader never captures an external save object or actor. Its separate
unsigned wall clock admits a capture only after more than 60,000 ms; the sampled
cycle time becomes the baseline after the wrapper returns, including gated or
failed writes. Current day/raw float time, both identifier sets and already-
retained source values are copied under the World guard and serialized outside
that acquisition. A normal exit captures after draining admitted work, before
reader teardown records still-loaded sources. These saves do not first enumerate
all loaded sources. Generated character output remains separate; captured replay
keeps its explicit archive adapter. See the [persistence contract](specs/steam-host-retained-state-persistence-20261107.md)
and [checkpoint](release-status.md) for ownership/failure tests, evidence limits
and remaining character scheduling, durability and native runtime acceptance.

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
