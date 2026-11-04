# Alpha terrain generation

`World.Source=Alpha` selects the project hybrid: independently implemented Alpha
landscape and climate with Steam towns and dungeons preserved. Steam generation
remains available separately as `World.Source=Game`; extracted owner zones use
`World.Source=Extracted`. `Generated` and `Terrain.Style=Alpha` are older approximate
terrain options, not substitutes for the Alpha generator. Restart after changing
source, seed or scale. Alpha is the shipped default after the runtime checks below.

## Implementation and provenance

The `alphagen` core reuses the sister AlphaVulkan project's original GPL-3.0-only
clean-room code by DieuDeGlace. Its independent `alpha-worldgen-*.md` contracts
cover random streams, regions, locations, terrain, formations and trees. The
implementer has not read native code or third-party reconstructions. The separate
native-only author supplied `alpha-terrain-generation-boundaries.md` and
`alpha-sky-terrain-inputs.md`, followed by bounded plant-consumer, material,
environment-light and cloud supplements in `docs/specs/`. The implementer used
their prose only.

Height H, the initial voxel surface S and a completed native Alpha zone are
separate products. `SurfaceStage` retains S, the padded height grid, ravine seeds
and random state. `ReliefStage` continues through rock formations, road cuts,
terraces and pools. `LandscapeStage` runs ordered boulder passes and the voxel-tree
grid. Mutable working storage moves into immutable publication; intermediate
stages do not duplicate full voxel zones.

Alpha towns, dungeons, camps and type10 structure calls are deliberately skipped.
The subsequent random stream therefore belongs to this hybrid profile, and is not
a claim of native full-Alpha generation equality. `VegetationStage` continues a copy
of the landscape stream over its immutable columns. Natural Alpha grass, flowers,
reeds, underwater plants and raised-lake lilies replace tagged Steam ground cover
outside construction cores. Other Steam scenery and residents are reanchored to
the composed ground. Alpha ground-item and street-light decisions retain their
ordered draws, but their records do not enter the hybrid runtime: Steam gameplay
items and town lights remain in use. Extra ruin additions, including Sphinx, are
deferred at the owner's request.

Trees include the giant-tree continuation and owner-model palms. Palm stamping
uses only the independent mode0 placement contract: ordered model-dependent
height draws, quarter-turn rotations, internal dry/keep air and protected-solid
exclusion. Both valid `sprites/palm-leaf.cub` and
`sprites/palm-leaf-diagonal.cub` must be supplied by the owner's Steam install.
The engine creates no replacement artwork and packages neither file.

Ground-cover kinds resolve by name through the owner's `Extract/sprite-groups.txt`
environment section, with all17 required natural model files checked locally.
Alpha kind numbers are never assumed to equal Steam registry indices. The engine
reuses the owner's sprites; missing entries/files are reported without substitution.
`Model::groundCover` is project provenance and is never packed into a native flag.

The independent plant-consumer supplement supplies the placement bridge: XY at
column centers, one uniform scale in blocks per model voxel, full-header XY pivot
with base Z, and lily Z exactly `zt*65536-6553`. Reed flags overwrite to4; ordinary
nonsway flags retain2 and other admitted sway requests produce6, including the
branch-dependent kind4 distinction. The engine uses its active tile-model list's
one-based order for sway, including skipped entries. The original Alpha cache's
extent/population and remaining wind-sign details are unclosed; no full-zone or
stable-ID phase equivalence is claimed. Small-plant integration is cosmetic and
does not change the landscape geometry/profile identity.

## Coordinates, storage and composition

The original MIT coordinate helper is shared with TerrainVariance, with its
existing notice. Steam block (0,0) maps to absolute Alpha home (8396928,8396928).
Mapping happens in integer coordinates before noise evaluation. A Steam64 tile
may cross four Alpha256 parents. Finite-world coverage is clipped, never wrapped;
incomplete playable tiles are unavailable.

The stores coalesce concurrent parent requests, generate outside the cache lock,
propagate failures to waiters and permit retries. Ready entries are bounded;
caller-held leases and in-flight work can retain additional storage. Canonical
Steam64 requests also coalesce by source/settings/options identity, sharing failures
with waiters and allowing retry. A flush retires older pending generations. The
world reader budgets96 unpinned entries separately from explicit player keeps, so
the native121-zone neighborhood cannot starve the streaming cache. The runtime
hybrid retains up to16 ready landscape parents. Leases keep const voxel storage
alive through eviction without retaining a mutable generator pointer.

`SteamBridge` converts four-byte Alpha materials into six-byte engine blocks with
explicit type and flag rules. It transposes XY indexing, preserves original RGB,
and records full-color column metadata separately from native block flags. Alpha
lighting consumes those full RGB values directly; retained Steam construction
columns keep their half-intensity encoding. The Steam lighting adapter quantizes
full RGB to the Steam vertex representation. Dry air, leaves and glowing material
have named conversions; unknown materials fail rather than sharing a coincidental
numeric type. Glow13 becomes Steam light20 with full emission as a hybrid policy.

`worldgen::AlphaTerrain` preserves every native column in conservative Steam
construction footprints, including rooms, openings, layouts and sizes. Outside
those cores a32-block band shifts whole Alpha columns toward Steam ground; the
Alpha sea plane is kept fixed. This intentionally retains some surrounding Steam
terrain. Forest edges and steep transitions still need wider visual review.

Models, lights, emitters, objects, ground items, spawn/shop points and daily-refresh
working lists are reanchored in Z; XY and model scale are retained. Object block
marking runs again at the resulting positions. Composition is transactional. The
canonical generator, collision, water, lighting, local host and mirror readers all
consume these composed columns; H alone never places the player.

Alpha downward edits fill newly stored gaps with the source rock palette, sampled
at the original Alpha Z even inside translated transition columns. The caller's
target block bytes stay intact. Missing material input fails without a partial
column edit; Steam setters retain their existing material path.

Implicit buried mesh faces blend the first stored solid block toward procedural
rock over ten blocks. Air/water/empty references use pure rock. The independently
specified scalar float stages remain unquantized through corner shading, with
byte packing only at the vertex. This changes face color, not collision storage
or block light. The Steam-look adapter keeps its explicit vertex-format policy.

Alpha environment models retain their mesh-time origin-light sample. Native
fixed division happens before the hybrid's local coordinate conversion; source
Alpha XY is restored before that division. A solid origin reads zero instead of
the creature-style raised sample. OWN_LIGHT replaces both the cached factor and
frame sunlight with1; ordinary model fading and underwater RGB remain separate.
The hybrid still uses its lighting volumes and64-block publication tiles, so this
does not establish the original Alpha World-light/cache lifecycle in full.

Adjacent Alpha faces reuse exact corner-light values through two bounded Z
planes. The immutable-volume sampler preserves the existing float values and
vertex packing. A private64-tile flat-scene CPU benchmark showed3.66–3.70x faster
corner sampling across three runs with identical checksums. That is a focused
CPU result, not a gameplay FPS or cold-streaming acceptance claim.

The Alpha light flood also skips remaining synchronous sweeps once every working
value is unchanged: subsequent sweeps would repeat that fixed point. Its original
six-neighbour arithmetic, minimum-neighbour rule and sixteen-step propagation
limit stay intact. Golden tunnel/isolated-air cases and full-grid benchmark hashes
cover those distinctions. An enclosed synthetic scene was7.38x faster by median;
outdoor and glow-wave cases retained their hashes without a measured slowdown.
This is a focused CPU result, not a claim about whole-game frame rate.

## Sky and map

Alpha terrain supplies direct World warmth, moisture, sea and fire blends to the
Alpha palette and HUD weather values. Sea share is the weighted negative-elevation
region indicator, not current water occupancy. H and the mountain field feed
cloud height. Cloud hashes and the intentionally cell-indexed moisture query use
absolute Alpha coordinates; renderer coordinates are rebased separately. The
independent sky supplement distinguishes these queries from loaded-column caches
and leaves unmeasured native scheduling/fixed-altitude details open.

Alpha cloud noise now uses the independently rechecked additive3523796237
(0xD208DD0D), correcting an older specification's transcription error. The
verified zero-lattice float word is0xBE9046E8. Altitude queries M then H, keeps
separate float stages, converts the base and noise rise to fixed64 independently,
clamps the converted base, then adds full64 Z. Owner-model header centering was
already consistent and needs no replacement art. These sky corrections leave
terrain geometry/profile identity and the Steam cloud algorithm unchanged;
original cache/preparation and render-origin lifetime still need broader review.

The Steam-style map keeps its UI, marker XY, region identities and protected
construction geometry. Close views sample actual landscape voxels, including tree
crowns; distant views use an H-based overview to avoid generating entire off-screen
regions. Map colors are adapted to the Steam vertex format. Distant overview and
transition bands are project policies, not a native Alpha map parity claim.
Steam region names, quest plans and difficulty-region identities remain in use.

## Saves and multiplayer

An Alpha profile identity includes landscape/composition versions, Alpha seed,
Steam layout seed and exact scale bits. Its own `Save/<profile>/characters.db`
and `world.db` keep deaths, terrain edits and progress separate from Steam saves.
On first writable use only, characters may be copied from the engine's canonical
save or configured read-only import. Identity, appearance, equipment, level and
recipes carry over; coordinates, discoveries, missions, flight points and other
geographic state start fresh. Subsequent progression is independent in each
profile. Existing profile characters are never recopied. `--no-save` creates no
profile or imported database. The original game's Save directory is rejected.

Alpha multiplayer requires compatible CubeVulkan peers with matching mode, seeds,
scale and generation versions before exchanging world data. Original Steam peers
continue to use Steam generation. The local player shares the host's immutable
profile and bypasses the remote-peer agreement wait during slow initial generation.

## Verification and remaining limits

First-run setup follows the selected generation mode. Alpha needs no captured
terrain, but it requires the owner environment model table and validates the two
palm and seventeen plant `.cub` files before reporting ready. Damaged or missing
original models must be repaired through Steam. Tests cover a fresh installation,
incomplete tables, invalid models and the separate Steam generation requirements.

Generated worlds in Steam-look mode now separate the native client source footprint from its
candidate square and geometry budget. It checks all nine source neighbors before
new mesh work, counts installed and pending native costs, and retains deferred
edits. Partial captured-world patches retain their existing missing-neighbor replay
fallback. Source coordinates truncate before narrowing; mesh centers use the separate
negative-fraction correction. Terrain bytes, placement and view-distance settings
are unchanged by this scheduling work.

The replacement backend materializes immutable sources lazily. Its cached-source
fallback uses the native steady retention boundary; it does not replay the original
four-job source worker, eight-job preparation cadence or incremental eviction. Its
shared reader/cache roles and host demand lifetimes still need broader comparison.
Ordinary eagle travel now selects its raw block destination for preparation,
keeps physical retention separate, and stops that substitution on arrival. The
title eagle state does not select a destination. During travel the replacement
keeps both current and destination terrain windows; exact native worker cadence
and transient eviction order are separate from this cache policy. Arithmetic footprint checks are not that acceptance.

Published Alpha cells and regions use bounded per-thread read caches. A distinct
world-lifetime identity prevents pointers from carrying over when an object address
is reused; missing and not-yet-published entries are retried through the synchronized
lookup. Region initialization still publishes once. No climate arithmetic, seed
stream or voxel bytes change. Publication, concurrent-reader and lifetime tests
cover this optimization.

Separate native-instruction references verify crossed-axis region displacement
float words at four lattice inputs, seed123 noise offsets and six raw
rock-color float-word results with pinned synthetic cached climate. They are not
native world captures; the existing generation math passes unchanged.

Formula vectors, worked tree output, synthetic palm rotations, finite-domain and
cache-lifetime checks validate the independent contracts. Composition tests check
whole construction tiles, source isolation, spawn/collision agreement, reader forks
and save-profile separation. Network tests check pending-player IDs, rejected
updates and acknowledgement ordering. These checks do not establish full native
runtime parity; broader seeds, seams, transitions, live terrain edits, weather,
map views and long gameplay still need owner-runtime comparisons. Default Alpha
lighting captures passed at128 and400 blocks, and a bounded Steam-lighting view
rendered correctly. Full Steam-option streaming exceeded the capture deadline;
cold-load performance remains open, without weakening the Steam mode's settings.

TerrainVariance remains a separately MIT-licensed Steam height/column translation
mod. Its shared coordinate/rule checks do not make its wrapper a full Alpha voxel
generator. No GPL generation implementation is silently copied into that mod.
Further compatible height propagation remains separate work.

No original executable code, assets, sprites, terrain exports, saves or captures
are included in the source or development packages. Steam account/license checks
remain mandatory. This architecture and the clean-room process do not themselves
establish legal clearance; see PUBLICATION.md.
