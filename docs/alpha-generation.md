# Alpha terrain generation

## Steam land and biome-color continuity, 2026-11-09

The hybrid preserves the Steam land floor beneath named land biomes, including
Undead Lands, while keeping higher Alpha relief. Savannahs keep their gentler
native geometry and map relief. A smooth256-block regional halo joins the
policies; exact construction cores and the rounded landmark approach are retained.
The independent raw Alpha ocean field no longer replaces those land floors.

Composed soil samples use the native column's stored palette at the corresponding
depth. Purple Undead Lands, snow, sand and other biome ground colors therefore
follow Steam membership. Map land colors use the native map palette. Alpha voxel
geometry and forests remain outside native savannahs and construction cores;
raw source colors and random streams are unchanged. The composed blocks use Steam
half-intensity encoding, including quantized Alpha foliage, and ordinary retained
native inputs fill downward edits. Raw full-RGB source/provider paths remain
separate and tested. Alpha lighting and Omega water remain enabled.

Actual baseline/candidate probes reduce sampled Deadlands and savannah flooding
to zero; both savannah cases retain native terrain and overview heights. All five
builds, full required-owner CPU without skips, every Vulkan group and SDK
independence pass. The bounded three-process median changes from2.899 to2.876
seconds; no gameplay speedup is claimed. The composition identity advances to
compose-v3. Existing compose-v1/v2 folders are retained without geographic migration.
Final current-binary Native/Alpha scenes and purple-land/savannah views complete
and are inspected. Creation, read-only and reopen checks preserve canonical and
previous-compose-v2 bytes. Earlier readiness failures remain retained; startup
timings remain variable. Height-only cover reads and per-compose XY reuse preserve
exact H words and composed terrain/model fingerprints. Broader biome edges, native
pixels, reliable startup, travel and final acceptance remain open.

## Fixed-water plant follow-up, 2026-11-09

Water lilies now retain their original water-surface Z when the hybrid moves
solid ground. Composition omits a lily if the resulting floor or scenery covers
its former water surface; grounded plants still follow the solid translation.
The exact lily fixed-point placement and both species are retained. No
vegetation draws, raw Alpha terrain or compose-v2 geometry/profile identity
change. Four reproduced bridge/pool failures now pass, including raised and
lowered floors and the separate grounded-reed behavior.

A separately source-qualified Steam height correction preserves the order of
two binary32 multiplications in the small-peak admission scalar. It changes no
global relief amplitude or mode policy. All recorded height pipeline values
remain exact, but wider native/provider closure is still open. All five targets,
full required-owner CPU without skips, all Vulkan groups and SDK independence
pass. Current live Alpha/Help scenes, landmark/material/water review and new
profile/save acceptance still await Steam reconnection.

## Landmark transition checkpoint, 2026-11-08

The user's flat-platform report was reproduced in the default Alpha landscape
seed 1570745101 with Steam layout input 0. At a forest mound's construction/coast
boundary, neighboring solid columns previously jumped 282 blocks, from -97 to
185. The revised transition gives 184 and 185, a one-block step. The protected
mound peak remains 281 and the sampled volcano peak remains 269; exact retained
core column bytes, core map heights and construction masks stay unchanged.
This changes the hybrid blend, not native Steam generation or the raw Alpha
height formulas. It does not apply an arbitrary global relief multiplier.

Read-only cross-sections sampled every32 blocks reduce maximum overview steps
from256 to96 at the volcano and192 to64 at the forest mound. Those are bounded
map samples, not a guarantee that every slope has a one-block step. All sampled
raw Alpha height values are unchanged. Close map composition blends the solid
height separately from the fixed liquid surface; distant views still use the
existing H-based approximation.

All five targets, focused continuity/water/metadata/core tests, full
required-owner CPU without skips, every Vulkan group and SDK independence pass.
The existing native cell comparison retains zero mismatches across60,000 cells.
Three alternating CPU probe runs per version measured medians of1.687 and1.860
seconds, about10.3 percent more time for the wider composition. This probe
includes generation, map work and two composed zones; it is not gameplay FPS or
cold-start acceptance.

Live native, Alpha/Omega and landmark-specific views, new-profile first use and
save acceptance remain pending because Steam reports the account offline.
The first native capture timed out without an image and is excluded; no terrain
or performance cause is inferred. Ownership checks and the150-second deadline
remain intact. Current test results do not close broad visual, world, travel,
hardware or multiplayer acceptance.

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
hybrid retains up to 64 ready landscape parents with a 384 MiB estimated allocated
ready-payload budget. Caller-held leases, in-flight work, allocator overhead and
World metadata remain outside that budget. Leases keep const voxel storage alive
through eviction without retaining a mutable generator pointer.

`SteamBridge` converts four-byte Alpha materials into six-byte engine blocks with
explicit type and flag rules. It transposes XY indexing, preserves original RGB,
and records full-color column metadata separately from native block flags. Alpha
lighting can consume those full RGB values directly. The current hybrid then
selects native biome terrain palettes and half-intensity encoding; its geometry
and the immutable full-RGB source remain separate. The raw Steam lighting adapter quantizes
full RGB to the Steam vertex representation. Dry air, leaves and glowing material
have named conversions; unknown materials fail rather than sharing a coincidental
numeric type. Glow13 becomes Steam light20 with full emission as a hybrid policy.

`worldgen::AlphaTerrain` preserves every native column in conservative Steam
construction footprints, including rooms, openings, layouts and sizes. Outside
those cores a256-block smooth transition shifts solid Alpha terrain toward Steam
ground. Distance from the nearest footprint uses a rounded Euclidean halo; box
lookup covers that whole halo on either side of a publication-tile boundary.
Duplicate footprints are removed before sampling. The blend continues through
sea level, while sea and raised-pool water levels remain fixed in world Z. Dry
air and immutable source metadata are retained. Composed palette bytes are selected
separately as described above. This intentionally keeps
some surrounding Steam terrain without the former abrupt coastal pad edge.
Broader forest edges, material seams and steep transitions still need live review.

Models, lights, emitters, objects, ground items, spawn/shop points and daily-refresh
working lists are reanchored in Z; XY and model scale are retained. Object block
marking runs again at the resulting positions. Composition is transactional. The
canonical generator, collision, water, lighting, local host and mirror readers all
consume these composed columns; H alone never places the player.

Current hybrid downward edits use retained native biome color inputs. Explicit
full-RGB Alpha columns still fill gaps from source rock at the original Alpha Z,
including translated columns. The caller's target bytes stay intact. Missing
material input fails without a partial edit; native Steam setters are unchanged.

For explicit full-RGB Alpha columns, implicit buried mesh faces blend the first
stored solid block toward procedural rock over ten blocks. Air/water/empty references use pure rock. The independently
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
regions. Current composed land uses the native biome map palette; water remains
separate. Distant overview and
transition bands are project policies, not a native Alpha map parity claim.
Steam region names, quest plans and difficulty-region identities remain in use.

## Saves and multiplayer

An Alpha profile identity includes landscape/composition versions, Alpha seed,
Steam layout seed and exact scale bits. Its own `Save/<profile>/characters.db`
and `world.db` keep deaths, terrain edits and progress separate from Steam saves.
Alpha composition keeps its Steam layout input 0. Landmark transitions previously
advanced compose-v1 to compose-v2; land/biome corrections now select compose-v3.
Existing compose-v1/v2 folders remain untouched, with no geographic migration.
The landscape generation version remains v2. Compatible engine peers
must agree on the full new identity.
Normal Game-generated Steam sessions use the separately verified native startup
input 123. Keeping these explicit avoids silently changing existing Alpha geography;
the Alpha composition choice is an engine extension policy rather than a claim
that input 0 equals native Steam configuration.

On first writable use only, characters may be copied from the engine's canonical
save or configured read-only import. Identity, appearance, equipment, level and
recipes carry over; coordinates, discoveries, missions, flight points and other
geographic state start fresh. Subsequent progression is independent in each
profile. The native height-gate precision correction advances the landscape
identity to v2. Existing v1 folders are retained; the corrected generator selects
its own v2 profile and compatible peers must agree on that identity. This does not
migrate an existing v1 world's geographic state. Existing profile characters are never recopied. `--no-save` creates no
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

Parallel source requests exposed repeated landscape generation: a65-second private
run completed563 parent generations for49 coordinates. The64-parent diagnostic
completed56 for56, with no repeats. Runtime Alpha composition now retains up to 64
parents with a 384 MiB estimated allocated ready-payload budget. Vector capacities,
including column blocks and nested records, supply the weights; caller leases,
in-flight results, allocator overhead and other World data remain separate. An
oversized result reaches callers without displacing useful cached entries. Generic
surface/relief users preserve their count-only policy. No terrain formula, RNG draw
or immutable stage contents change. Exact-budget, LRU, oversize/retry and owner-data
checks pass, followed by all image suites. One full default-distance Steam-look
capture now settles; that is not an FPS ratio or broader runtime certification.

Independent direct climate references also pin seed 123's nine-cell state and
check twelve float words at land, a mixed land/sea boundary and sea. Missing-cell
checks retain the native defaults even when the absent cell would have zero
weight. The separate region/location object is absent, so these are not oasis
or prepared-world samples; fire follows the specified no-fire branch. The
independent derivation accounts for the stored shortened-pi constant and bounds
nonlattice displacement before integer conversion. It does not claim unproved
individual noise/displacement words or native runtime captures. A separate natural
positive-fire window adds pure and mixed fire/nonfire four-field references; its
stored-angle enclosures also preserve the final integer sample points.

Separate native-instruction references verify crossed-axis region displacement
float words at four lattice inputs, seed 123 noise offsets and six raw
rock-color float-word results with pinned synthetic cached climate. They are not
native world captures; the existing generation math passes unchanged.

Independent raw-H references pin seed 123 offsets, nine cells, a missing link
neighbor and absent region/location/Zone state. Both literal returned words match;
they are conditional raw-height checks, not final generated surfaces. Synthetic
roughness inputs additionally verify the native two-chain smooth/square staging.
The H path now uses its own small helper to retain that rounding order, leaving
other easing consumers unchanged. Its potentially changed terrain receives the
separate v2 generation/profile identity described above. Full CPU/owner-data and
all image checks pass; the v2 full-distance Alpha-look capture completes in25.836
seconds wall. The corresponding v2 Steam-look capture exits0 in131.453 seconds
wall with the same observation logic and view distance. Whole-process wall time
includes startup/shutdown and is not identical to the loop's observation clock.
Wider locations/prepared regions/nonlattice inputs and timing variability remain open.

Formula vectors, worked tree output, synthetic palm rotations, finite-domain and
cache-lifetime checks validate the independent contracts. Composition tests check
whole construction tiles, source isolation, spawn/collision agreement, reader forks
and save-profile separation. Network tests check pending-player IDs, rejected
updates and acknowledgement ordering. These checks do not establish full native
runtime parity; broader seeds, seams, transitions, live terrain edits, weather,
map views and long gameplay still need owner-runtime comparisons. Default Alpha
lighting captures passed at128 and400 blocks, and a bounded Steam-lighting view
rendered correctly. The previously failing full default Steam-distance capture
now completes in113.808 seconds wall with the bounded parent-cache continuation
below. Wider cold-load/travel, constrained-memory and multi-GPU acceptance remains
open; Steam view-distance settings and the observation deadline were not weakened.
Fresh full-distance Alpha-look captures also pass:26.554 seconds for the default
seed's forest view and40.674 seconds for seed 123 at another location. Their
sampled whole-process working/private peaks were1067/1617 and986/1425 MiB. Owner
settings were imported, both images were inspected locally, and no imagery is
packaged. These are bounded observations, not native-image or travel certification.

TerrainVariance remains a separately MIT-licensed Steam height/column translation
mod. Its shared coordinate/rule checks do not make its wrapper a full Alpha voxel
generator. No GPL generation implementation is silently copied into that mod.
Further compatible height propagation remains separate work.

No original executable code, assets, sprites, terrain exports, saves or captures
are included in the source or development packages. Steam account/license checks
remain mandatory. This architecture and the clean-room process do not themselves
establish legal clearance; see PUBLICATION.md.

## Bounded travel checks, 2026-11-03

Two no-save Alpha-look runs completed 35 simulation seconds with a test character:
a 512-block X flight using the default seed, and a negative 512-block X/Y flight
using seed 123. Both crossed parent boundaries, landed, produced inspected captures
and exited normally. Wall times were 53.12/53.60 seconds; sampled working peaks were
1027.9/1134.4 MiB and private peaks 1308.0/1421.7 MiB. The original test save stayed
byte-identical. The second run encountered ordinary hostile creatures; undying was
enabled, and this is not combat acceptance. These are two configured streaming
checks, not exhaustive forest/biome/transition coverage, a long soak, native runtime
parity or a whole-process memory cap. Reference images stay local.

A further default-seed Alpha-look run requested a 4,096-block X flight and 145
simulation seconds. It landed, finished the observation and exited 0 in 163.79
seconds wall, with sampled working/private peaks 1406.7/1698.5 MiB. The final
terrain/character scene was inspected; the original test save hash was unchanged.
This is one longer cache/streaming observation, not an exhaustive soak or a
verification of every intervening biome, seam or native scheduling detail.
