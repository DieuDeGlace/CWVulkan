# Source review checkpoint — 2026-11-03

This is a replacement-engine source package for review. It is not a finished-game
release or a claim of pixel/behavioural parity. Older dated development notes may
describe prototypes that have since been replaced; the current README and this
checkpoint describe the intended setup and remaining limits.

## Current handoff items

The newest checkpoints appear first. The latest validation result below qualifies
older successful runs; none of these bounded changes establishes complete parity.

- The existing interface-frame capture action now has a fixed-path, one-shot
  `Extract/ui.request` alternative to F10. It polls at most four times per second,
  waits for an active capture batch, and queues six frames without requiring a
  loaded character. Live title-menu validation on the existing modded owner
  installation saved all six 1920x1080 frames; two overlapping requests correctly
  waited and saved twelve more. All eighteen BMPs decoded, had nonuniform content
  and distinct hashes; the first was visually checked as the actual title menu.
  The callback guard and 332 owner-resource comparisons pass, and the game closed
  normally with exit code 0. These images remain local, with no original asset in
  the source package. This verifies capture plumbing, not unmodified visual parity
  or loaded-world/recording/clean-machine acceptance. No SDK layout or new native
  implementation was consulted for this trigger.

- Live validation of the guarded extractor now passes on the existing owner
  installation. The verified rebuilt DLL was installed while Cube World was
  closed, with the prior DLL retained in a checked backup. Startup refreshed the
  sprite exports; a title-menu request captured two existing full-key procedural
  resources with zero lookup/write failures and byte-identical output. The owner
  resource check then compared 332 volumes with zero mismatches. Cube World closed
  through its normal window command and returned exit code 0. No character was
  loaded for this check and no other mod was changed. Direct Windows window
  capture timed out; a fresh Steam screenshot verified the title menu instead.
  The F10 automation attempt produced no capture and is not counted as passing.
  This closes the observed startup/resource/normal-close smoke check, not loaded-
  world recording, every quit branch, clean-machine recovery or SDK release rights.

- The extractor now checks its callback argument against the supported image's
  published native controller and reads the quit byte through a guarded copy
  before any typed controller/World access. The independent native trace proves
  normal context identity but leaves post-PostQuitMessage RAX unproved; those
  calls are rejected even if the register happens to retain the controller.
  Pure SDK-independent guard tests cover poisoned/null/unreadable pointers,
  address overflow, quit handling and title admission without requiring a World.
  All five targets build and the full CPU/owner-data suite passes; final focused
  checks also pass after formatting. The rebuilt DLL was not installed into the
  running original game, so live startup/quit and capture acceptance remain open.
  The DLL still has its existing SDK dependency; no full replacement, permission
  grant or loader/object-lifetime guarantee is inferred from this guard.

- Alpha base-height roughness gating now preserves the independently verified
  float32 multiplication-chain order. Synthetic scalar inputs expose differences
  from the generic easing expression; the shared easing helper is unchanged.
  Two independently derived raw-H words and their explicit partial-world state
  also pass. These are not generated-surface or fully prepared-region oracles.
  Alpha generation identity is now v2: existing v1 profile folders are retained,
  while v2 saves/caches and peer agreement use the separate corrected identity.
  The actual landscape-store one-byte-budget test confirms oversized results keep
  valid caller leases and identical geometry without being retained in ready cache.
  All five targets build, the Alpha suite and full CPU/owner-data suite pass, and
  every image suite passes. A fresh full-distance v2 Alpha-look capture also passes
  in25.836 seconds wall; the corresponding v2 Steam-look capture also exits0 in
  131.453 seconds wall. Both retain the existing internal observation logic and
  external180-second guard, not a promised120-second whole-process runtime. Their
  sampled working/private peaks are1206/1620 and1636/2534 MiB. Broader generation,
  timing variability and runtime acceptance remain open.

- Full-distance Alpha-look captures now also pass on this owner's installation:
  the default-seed forest view completes in26.554 seconds wall, and seed123 at a
  second location completes in40.674 seconds. Both use fresh processes, no-save
  mode and the existing capture deadline. The corresponding sampled whole-process
  working/private peaks are approximately1067/1617 and986/1425 MiB. Both new images
  were inspected locally and remain excluded from source/runtime packages. These
  are bounded streaming/rendering checks with imported owner settings, not native
  pixel parity, travel testing or a fresh-machine acceptance result.

- Alpha cold streaming now retains up to64 immutable landscape parents with a
  384 MiB budget for their estimated allocated ready-stage payloads. A private
  count-only65-second run previously generated563 parents for49 coordinates;
  the64-entry diagnostic completed56 generations for56 coordinates with no
  repeats. The production bounded-cache full default Steam-distance capture now completes: exit0,
  113.808 seconds wall, preserving setting50/radius15 and the existing deadline.
  Its sampled whole-process working/private peaks were approximately1612/2542 MiB;
  caller leases, in-flight work, allocator overhead and other engine data are not
  covered by the ready-cache budget. Weighted LRU/exact-budget/oversized/retry
  checks, the full CPU/owner-data suite and all image suites pass. No generation
  math, asset data, view distance or source-role lifetime rules were changed.
  This closes that observed capture failure, not wider cold/travel/low-memory or
  multi-GPU acceptance. Private diagnostic logging was removed before packaging.

- Two additional independent Alpha references now cover a natural positive-fire
  cell and an actually mixed fire/nonfire boundary: eight literal float words,
  with exact cell and absent-location prerequisites. The Alpha suite passes;
  these conditional arithmetic fixtures are separate from runtime/oasis acceptance.

- Independent Alpha climate references now check twelve literal float words across
  land, a mixed land/sea boundary and sea under the native seed123 nine-cell state.
  Removing any one required cell also checks the specified defaults, including a
  cell whose complete-window weight was zero. The full CPU/owner-data suite passes;
  no generation formula was changed. These are conditional arithmetic references,
  with the separate region/location object absent and the zero-fire branch pinned;
  they do not establish positive-fire, oasis or full native-world runtime parity.

- Host source demand now follows the independently specified per-player union:
  a 270-block box clipped to the radius-5 disk, with phase-dependent 77/78/79
  single-player candidates. Ability 0x9c selects the raw block destination;
  physical player positions remain the resident-admission input. Logical host
  unloads and instance-death release follow this wanted set even when another
  reader retains departure terrain. Mirror-client behavior is unchanged. All five
  targets build and the full CPU/owner-data suite passes, including native phase,
  negative/narrowed coordinates, overlapping-player union and cached-terrain travel
  regressions. Background cache/job timing and broader packet-request producers
  remain qualified; no exact concurrent native cache lifetime is asserted.

- Eagle travel now binds generated-world source/mesh preparation to the existing
  actor's signed block destination only during ability0x9c. Physical retention,
  title flight0x9d and arrival gliding0x78 remain separate; destination fields are
  not implicitly cleared or rejected by a new sentinel/age rule. Current terrain
  remains available while the destination view is prepared, under the replacement
  cache policy. The adaptive camera scan also uses the verified narrowed-block
  producer, including wrapped negative reconstruction. Four independent native
  displacement word pairs are now regression references; the displacement routine
  itself is unchanged. All five targets build. Full CPU/owner-data and all image
  suites pass for the anchor integration; the final camera change additionally
  passes focused range and both camera-style image checks. Live travel/network
  cache-lifetime acceptance and the full-distance performance gate remain open.

- Generated Steam-look worlds now use the independently specified client source
  window, full neighboring-zone admission, signed distance-key ordering and a
  separate mesh-preparation budget. Installed and pending costs contribute, and
  deferred edits keep their dirty state. Partial captured-world replay preserves
  its existing missing-neighbor behavior and passes object/reward regression checks.
  Alpha immutable cell/region reads now use bounded thread-local caches with
  distinct world lifetimes; misses and late publication retain synchronization.
  No generation arithmetic, geometry bytes or owner assets were replaced.
  All five targets build and the full CPU/owner-data suite passes, followed by
  92 Vulkan,2 camera,102 GUI and17 map image checks plus shared water and Alpha
  scenery checks. The full-distance capture still exceeds its120-second observation
  limit: the latest run reached502 prepared zones/15 pending at128.851 seconds wall,
  versus280/15 at140.348 before the immutable-read change. This is diagnostic progress,
  not an FPS result or completed cold-streaming acceptance. Lazy backend cache roles,
  ability-specific source anchors and broader native-runtime comparisons remain open.

- Independently derived Alpha Rk references now check seed123's original offset
  draw positions, its lattice-noise word and six raw RGB float-word results with
  synthetic cached climate. The existing generation math matches all six exactly;
  no replacement color formula was introduced. The Alpha terrain suite passes.
  These are bounded arithmetic references, not captured world/biome samples.
  The fresh-process full Steam-look radius15 rerun still fails its settlement
  deadline (135.017 seconds wall time); that performance gate remains open.

- Alpha first-run readiness now requires the owner model table and validates the
  two palm and seventeen plant models used by startup. The simulated fresh-install,
  incomplete-table and damaged-model cases pass, as do existing extraction/recovery
  tests. All five targets build; the read-only check of the owner's installation
  passes with Steam ownership verified. Steam mode retains its previous asset
  requirements. This is not fresh-machine or live interrupted-extraction acceptance.
  The independent loader ABI census is documented without SDK implementation;
  no SDK-free adapter has been implemented and the permission gate remains open.

- The published a00d377 source snapshot builds all five targets in an independent
  output directory using only its project files, cached pinned dependency sources
  and the documented separately supplied SDK headers for the extractor. Its full
  CPU/owner-data suite passes. The extractor is correctly optional without that
  SDK path. This establishes source completeness on this development machine,
  not a fresh-machine setup or clearance of the SDK's unresolved permission scope.
  The bounded font registry audit still does not identify a named-byte producer;
  no face bytes, registration behavior or glyph coverage were invented.

- Alpha's light solver stops at a proven unchanged working state, retaining the
  exact synchronous attenuation and maximum16-sweep reach. New golden tunnel and
  isolated-air cases guard propagation limits and the neighbour floor. All five
  targets and the full CPU/owner/Vulkan suite pass. A three-case private benchmark
  retained full-grid hashes in every run; enclosed-case median time improved7.38x,
  with no measured glow-wave slowdown. This is a CPU-kernel result, not whole-game
  FPS or a closure of wider cold-streaming acceptance.

- Alpha clouds now use the independently verified noise hash additive, fixing
  an older spec typo, with an exact native-derived float-word regression vector.
  Their altitude preserves M-before-H query order, separate float stages, two
  scalar fixed64 conversions, post-conversion base clamp and full64 addition.
  Tests catch a one-fixed-unit rounding difference and accidental32-bit narrowing.
  Full-header owner-model centering was already correct. Steam cloud logic and
  both terrain profiles remain separate. Five targets and the full CPU/owner/GPU
  suite pass; these are contract checks, not a new native runtime capture.

- Alpha environment lighting now follows the independently traced mesh-time
  origin sample: exact fixed-coordinate division, retained scalar, dark solid
  origins and a complete OWN_LIGHT material override. It preserves cloud handling,
  underwater RGB and separate distance fading. Steam rendering and the creature
  standing-light policy retain their existing paths. The hybrid's lighting-volume
  and64-block cache policy still differ from a complete original Alpha cache.

  Terrain faces reuse exact corner light through two bounded Z planes. CPU checks
  cover cache hits, eviction, reversed/negative Z and XY fallback; GPU checks cover
  own-light at night/zero cached light and retained far fading. Five targets build
  without warnings; the full CPU/owner/Vulkan suite and final CPU run pass. A live
  no-save foliage capture exits0 and was visually reviewed. The private64-tile CPU
  sampler benchmark was3.66–3.70x faster across three equal-checksum runs; this is
  not a gameplay FPS or full cold-load result. Save/extractor hashes are unchanged.
  Broader native cache lifecycle, runtime comparisons and release gates stay open.

- Alpha live edits now sample source rock for newly stored downward gaps, retaining
  original source Z through translated columns. Missing input fails atomically;
  the target block and Steam setter path retain their existing byte contracts.
  Implicit buried faces now follow the independent first-block/rock blend, with
  separate float contributions preserved through corner shading and final packing.
  Air/water/empty references use pure rock. This does not change collision storage
  or invent implicit block lighting.

  All five targets build without warnings. Focused gap/translation/branch/precision
  checks and the full CPU/owner/Vulkan suite pass (92 shader,2 camera,102 GUI,17 map,
  shared-water and Alpha-scenery checks). A private no-save live cliff capture exits0
  and was visually reviewed; it is not a native pixel-parity oracle. Save/extractor
  hashes remain unchanged. Wider runtime, cold-streaming and release acceptance
  gates remain open.

- Alpha natural ground cover now continues the retained landscape stream through
  an immutable vegetation stage. It replaces tagged Steam plants outside protected
  construction tiles and resolves all17 natural models through the owner's actual
  environment registry. No model or registry output is bundled. Steam town models
  preserve their exact order, position, scale, tint and flags.

  The bridge uses full-header voxel scale/pivot, half-block XY centering, exact
  lily fixed-Z offset and branch-specific flags from the independent plant-consumer
  contract. Its active-list phase follows the reached one-based convention; original
  Alpha cache population and remaining wind-sign limits are still unclosed. Alpha
  ground-item/static decisions retain stream order but their records are not used;
  Steam gameplay items and town lights remain in use. This is cosmetic and leaves
  landscape geometry/profile identity unchanged.

  All five targets, focused integration checks and the full CPU/owner/Vulkan suite
  pass. A private live capture selected from generated plant records shows actual
  owner-model foliage on Alpha terrain; startup/nature captures exit0. The new owned
  GPL import audit confirms unchanged plant arithmetic/draw order with only output
  and storage adaptation. Save/extractor hashes remain unchanged. Broader native
  runtime/biome comparisons, wider live-edit coverage, cold streaming and release
  acceptance remain open.

- Concurrent canonical zone requests now share one generation result or failure.
  Cache flushes retire old generations without changing terrain identity. The
  decoded-world LRU keeps up to96 unpinned zones in addition to explicitly kept
  player neighborhoods; a121-zone keep can no longer evict every farther source
  immediately. In-flight snapshots can retain additional data until their readers
  finish. These are engine cache policies; generation arithmetic is unchanged.

  Concurrent-pointer, geometry/identity,121-zone retention and bounded eviction
  regressions pass. All five targets and the full CPU/owner/Vulkan suite pass
  (92 shader,2 camera,102 GUI,17 map, shared water/scenery). Full Steam-option
  streaming still exceeded the capture deadline; the cache corrections do not
  close the broader performance gate. A separate native-only plant-consumer
  contract now resolves the next ground-cover placement/scale/flag questions;
  its bridge is not implemented in this checkpoint.

- Alpha terrain is now the playable default (`World.Source=Alpha`); Steam
  generation remains separate as `Game`, with captured `Extracted` and the older
  approximate `Generated` modes still explicit choices. The Alpha landscape runs
  relief, ordered boulders and voxel trees while retaining conservative Steam
  town/dungeon tiles. It reanchors residents/scenery/refresh lists and uses the
  canonical collision, water and gameplay-light readers.

  Terrain-profile databases isolate worlds and copy characters once without old
  geographic state. Compatible-engine multiplayer checks generation identity
  before exchanging Alpha world data; local host startup is not gated by a remote
  handshake. Direct Alpha climate drives the palette/HUD, absolute Alpha cloud
  coordinates are kept separate from renderer rebasing, and the Steam-style map
  receives a close-voxel/distant-height overview. Full-RGB metadata prevents Alpha
  terrain from being brightened twice while preserving Steam construction colors.

  All five targets build warning-free. Full CPU/owner checks and92 shader,2 camera,
  102 GUI,17 map, shared-water and Alpha-scenery Vulkan checks pass. No-save startup
  captures pass at128 and400 blocks in Alpha lighting. Separate Steam generation
  starts correctly, and Alpha terrain renders in a bounded Steam-lighting capture.
  Full Steam-option streaming exceeded the capture deadline; cold fill remains an
  open performance issue. Steam's existing save-resume identity is preserved, and
  original-save protection also applies before that directory exists. Exact
  whole-tile and map construction preservation, source/cache separation, spawn and
  collision agreement, profile isolation and handshake ordering checks pass.
  The private save fixture and extractor hashes are unchanged. No game content is
  included. Native full-Alpha random-stream parity, Alpha small ground cover,
  broader transitions/seeds, live-edit material behavior, two-account multiplayer
  and manual release gates remain open; this is not a final release claim.
  See [Alpha generation](alpha-generation.md) for the hybrid policies and limits.

- Ordinary affine scene-graph clipping now renders children into a retained
  fullscreen RGBA8 pool and composes them through the parent's actual shape.
  Ordered vertex/callback cuts preserve empty geometry and own-hook placement;
  a clip node's own callback follows child composition even with DRAW_FIRST.
  Nested targets have null depth for WORLD callbacks and restore parent depth.
  All three mask modes preserve their distinct alpha/RGB rules, including
  conditional layer-RGB unpremultiplication and the separate position/UV bias.
  Ordinary owner button shine is enabled through these masks.

  Pool images are reused on the ordered graphics queue, with explicit barriers,
  guarded allocation, atlas descriptor refresh and safe window-target teardown.
  Invalid command streams leave the resolved scene intact. Existing nearest
  object-atlas sampling and an explicit linear/clamp mask baseline remain engine
  choices; the native inherited sampler state is not an all-frame oracle.
  Widget/node caches, blur, projective/flattened-W GUI and static legacy HUD mask
  approximation remain separate gaps.

  Eleven new Vulkan cases cover mask modes, nonrectangular geometry, unlayered
  control, nested composition, reused full clears, null-depth/restoration, atlas
  replacement and uncaptured frames in flight. Owner-HUD callback/cut checks pass.
  All five targets build warning-free; full CPU/owner and89 shader,2 camera,
  42 GUI,17 map/shared-water/Alpha-scenery checks pass. Five no-save captures exit0
  and are reviewed, with the save database unchanged. These bounded checks do not
  certify complete native UI or rendering parity.

- Plasma overlay vertices now retain float RGBA through upload, using the native
  32-byte position/UV/color layout while preserving existing API color units.
  Native text/shape emission no longer truncates these colors to bytes. Ordinary
  overlay blending now retains destination alpha with ONE/INV_SRC_ALPHA; debug
  readback can preserve actual framebuffer alpha, while normal captures stay opaque.
  Display fill affects its own shape rather than descendants, and glyphs use their
  text/outline colors independently of their own display fill. The legacy mask
  approximation remains separate. Recording-only panel meshes still explicitly
  convert to their byte-color compatibility format.

  Actual owner-HUD emission at GUI scales1/1.5 catches both old tint errors and
  verifies visible glyphs. Vulkan cases cover sub-byte color, a rounded control
  and source-over alpha. Previous tint/blend behavior fails those regressions;
  the correction passes. All five targets build warning-free; full CPU/owner and
  89 shader,2 camera,31 GUI,17 map/shared-water/Alpha-scenery checks pass. Five
  no-save captures exit0 and are reviewed, with the save database unchanged.
  The independently specified ordinary clip pool is the next implementation;
  real layer composition, cache/blur policy and broader UI parity remain open.

- Shared body/burst particle draws now build Steam WORLD matrices using the
  current installed scene's fixed negative-look-at offset before float conversion.
  Collection follows camera preparation, avoiding a preceding-frame origin.
  A small ParticleScene adapter keeps the source-home transform/Z shift for Alpha
  separate from native unshifted Z and the fixed producer sphere. Alpha avoids
  computing an unused native matrix. Simulation, producer order, material/style
  choices and the actual effect draw-pass order retain their existing contracts.

  CPU cases check fixed fractions, unequal header dimensions, source-home Z and
  the Alpha path. An actual Vulkan coverage case preserves a quarter block at
  block2^24 and matches the near scene; its old home-rounding negative control
  loses that offset. All five targets build warning-free. Full CPU/owner and89
  shader,2 camera,28 GUI,17 map/shared-water/Alpha-scenery checks pass. Five no-save
  captures exit0 and are reviewed, with the save database unchanged. This closes
  the specified particle-coordinate binding, not all producer/CRT/visual parity.

- Ordinary WORLD now composes its independent float matrices on the GPU before
  evaluating local vertices. Terrain/model, native water depth/color and direct
  GUI paths share that vertex stage, as established by the independent binding
  audit. View position and view normal use the composed WV; point lighting keeps
  its separate WORLD position/normal. DepthOffset changes the composite clip-Z
  coefficient before the local dot. The optional engine leaf-deformation path
  remains separate, and the native-menu early return is preserved.

  Three synthetic actual-Vulkan regressions fail with the previous shader and
  pass after: WORLD translation cancellation, raw GUI translation cancellation
  and GUI depth-coefficient placement. Visible baseline/depth controls prevent
  empty geometry from satisfying the comparisons. All five targets build without
  warnings. Full CPU/owner and88 shader,2 camera,28 GUI,17 map/shared-water/
  Alpha-scenery checks pass; five no-save captures exit0 and are reviewed, with
  the save database unchanged. These large-coordinate sensitivity fixtures are
  not captured native rounding or complete visual-parity oracles. Cross-family
  input/state lifetimes and special shader paths retain their separate scope.

- Map models now retain independent WORLD and VIEW inputs. The frame supplies
  the map view once; the vertex stage composes WV/WVP on the GPU and keeps the
  WORLD lighting normal separate from its view-normal varying. DepthOffset is
  applied to the composite clip coefficient. Exact fixed-axis literals and the
  independently derived default light-vector words replace rounded substitutes.
  Typed Glow, Shininess, material alpha, White and separate Opacity reach their
  stated stages. GPU dot/MAD/reciprocal-square-root rounding remains a bounded
  comparison limit, rather than a CPU or captured-frame numeric oracle.

  Five new Vulkan cases cover view-independent lighting, actual WORLD-basis
  changes, material alpha versus opacity, Glow and view-normal Shininess. All
  five targets build warning-free; full CPU/owner and87 shader,2 camera,24 GUI,
  17 map/shared-water/Alpha-scenery checks pass. Five no-save captures exit0,
  all reviewed, with the save database unchanged. The unused CPU fogAmount and
  pixelColour approximations and their obsolete checks were removed after a
  caller audit; coin-preview color helpers remain in use. Ordinary scene/water/
  direct-GUI WORLD vertex composition is the next bounded shared-shader check.

- Map opaque resources now group by stable engine-owned unsigned identity keys
  and retain descriptor FIFO. Faded vectors keep append order. Opaque buffer
  readiness precedes descriptor submission; faded submissions precede their draw
  readiness check. Unavailable buffers are never bound or drawn. Valid zero-count
  headers are not classified as unready merely from their counts. The map preset
  and fog setup still run for an admitted empty/undrawable context.
  Vulkan cases cover resource-group FIFO and unavailable opaque/faded resources.
  All five targets build warning-free; full CPU/owner and87 shader,2 camera,24 GUI,
  12 map/shared-water/Alpha-scenery checks pass. Five no-save runs exit0, all
  captures reviewed, with the save database unchanged. Native runtime addresses
  and aliases remain separate inputs, without a catalog-to-native-heap claim;
  marker-specific inputs and wider native visual comparisons remain open.

  Local cleanup removed ten obsolete, hash-verified smoke executable copies,
  reclaiming146 MiB. Their screenshots, logs and verified versioned packages
  remain available; the current build and owner files were preserved.

- Steam map region pixels now share the full WORLD pixel implementation with
  scene/GUI rendering. They use the base-two fog exponent and both Fog/SkySun
  blends, retaining the actual raw frame palette, sun position and gradient when
  map matrices change. Frame/color/pixel helpers are factored into shared shader
  includes with explicit build dependencies; the map binds the frame descriptor.
  The Alpha appearance option retains its compatibility coloring, and the CPU
  preview helper is labelled as an approximation rather than a native pixel oracle.
  Marker-specific producers and broader map comparisons remain separate work.

  Three synthetic map cases failed before and pass after: full-distance SkySun,
  a partial-fog numeric reference and retained sun/gradient with a different map
  lens. All five targets build warning-free; full CPU/owner and87 shader,2 camera,
  24 GUI,9 map/shared-water/Alpha-scenery checks pass. Five no-save runs exit0,
  all captures reviewed, with the save database unchanged. No native shader bytes
  or owner assets are included in source/runtime packages. Resource grouping and
  unavailable-buffer handling remain the next bounded map-consumer checks.

- WORLD fog now consumes a retained float reciprocal binding instead of
  recomputing it from every draw's current scene range. Nonpositive/NaN distances
  preserve the prior binding; positive infinity legitimately supplies zero scale.
  Range reset does not reset it. Native water shares that scalar, while immediate
  GUI models use their reached10000 setup without an underwater re-clamp.
  Late overlay, admitted map, item/raw-resource and nonnull card callbacks retain
  their specified ordering. Explicit map admission survives empty geometry, and
  an empty card still submits its setup/palette; null item resources skip setup.
  GPU-readiness failure does not erase earlier setters. Ring composition does not
  incorrectly become another native fog write after the map.

  CPU boundary/cache checks and seven Vulkan cases cover these distinctions;
  the initial world/water cases failed before and pass after. All five targets
  build warning-free; full CPU/owner and87 shader,2 camera,24 GUI,6 map,
  shared-water/Alpha-scenery checks pass. Five no-save captures exit0, all reviewed,
  with the save database unchanged. Initial native GPU contents remain unproved:
  a zero-scale pre-binding fallback is explicit engine policy. These checks do
  not replace native visual comparisons or final release acceptance.

- Ordinary particles and water now retain the preceding WORLD clip depth offset.
  Water depth/color phases have explicit state, its depth phase leaves points zero,
  and only color draws bind zone tables. The water prologue clears points even for
  an empty list; particles retain their material interpretation while submitting
  their own shininess. Two new Vulkan cases failed before the correction and pass
  afterward; a third covers the shared Omega-inspired water path. All five targets
  build warning-free, the full CPU/owner suite and80 shader,2 camera,24 GUI,6 map,
  shared-water/Alpha-scenery checks pass. Five no-save captures exit0 and were
  reviewed, with the save database unchanged. These remain stability/regression
  checks, not original-game visual acceptance.

  The independent point-table lifetime audit now separates CPU collection from
  uploads, identifies late overlay/GUI clears and establishes that construction
  and the early sky path supply no valid numeric initial oracle. The engine's
  empty no-binding fallback is explicit policy, not a native zero claim. Changing
  origin does not imply rebasing a retained GPU table or refreshing a zone key.
  Broader effect callers and native visual comparisons remain open.

- Interaction targets and mission object checks now consume current loaded object
  vectors, including moved/resized footprints, canonical indices, current state,
  full required items and unsigned counts. Generation-time placement recovery and
  native-path optimistic object toggles no longer override received/authority state.
  Chest receipt preserves the full current reward item through inventory and hold-up.
  Rewards remain a separate map, not an inference from the required item.

  The extractor now emits a project-owned reward-map chunk, including explicit
  empty maps, using a bounded guarded reader of the independently specified native
  layout. The engine shares immutable captured maps across source snapshots and
  distinguishes known absence from omitted metadata without hot-path file reads.
  Old captures need refreshing; DataCheck reports missing/invalid maps. Synthetic
  native-memory tests cover signed order, sentinel/black-node distinction, full item
  bytes, invalid/count/cycle cases and visibly changing observations. They establish
  no atomic native snapshot or particular synchronization lock.

  All five targets build warning-free; full CPU/owner and77 shader,2 camera,24 GUI,
  6 map/shared-water/Alpha-scenery checks pass. Five no-save runs exit0, all captures
  viewed, with the save database unchanged. A real owner-data subset passes and
  correctly warns about its older omitted map. The updated extractor is installed
  locally with the previous DLL backed up. Fresh native-map capture acceptance is
  pending: computer control fails during initialization with missing kernel assets,
  including after reset/retry. The source/runtime packages contain no owner data.

- Generated static-object records now preserve the signed64 user/link ID and
  unsigned required-item count rather than losing both during conversion to the
  live format. The default count remains1; explicit0 and255 remain unchanged.
  Save/restore integration coverage follows these fields from generation through
  recorded zone state back into a live list. All five targets build warning-free;
  full CPU/owner and77 shader,2 camera,24 GUI,6 map/shared-water/Alpha-scenery
  checks pass. A no-save gameplay run exits0 with its capture reviewed and the
  save database unchanged. Current-record interaction adapters and captured chest
  reward-map completeness remain separate work; a required item/count is never
  evidence of a chest's reward contents.

- Queued WORLD consumers now group opaque draws by stable engine-owned resource
  identity, preserve FIFO within each resource and keep winding families/context
  order separate. Faded vectors retain append order. CPU header/identity metadata
  survives unavailable GPU buffers: opaque resources fail before per-instance
  setters, while faded color records still submit controls and selected point
  tables before readiness can suppress their draw. Depth-only iterations leave
  the retained color/point cache unchanged. Explicit controls also respect the
  second-context selector rather than forcing every faded model after water.
  CPU unsigned-key/FIFO and four Vulkan ordering/readiness cases pass; all five
  targets build warning-free, with full CPU/owner and77 shader,2 camera,24 GUI,
  6 map/shared-water/Alpha-scenery checks passing. Five final no-save captures
  exit0, all viewed, and the save database remains unchanged. Temporary profiling
  code was removed. Capture observation failure now returns an error instead of
  success without an image. Native heap-address order cannot be inferred from a
  catalog or equated to the engine's allocation order; exact cross-run/native
  address inputs, the incoming no-point-binding baseline and visual acceptance
  remain open. A separate live-object supplement now specifies required-item
  counts/users and the distinction between object records and chest reward maps.

- Owned ground gear now submits the specified eight owner-extracted `cube.cub`
  draws, using exact-zero delay, adapted category, personal-data and owner gates.
  The per-frame list uses the raw item position, regional RGBA colors, unsigned
  clock/443 stepping, separate pulse/orbit/rise phases, local rotations and full
  allocation-header centering. It has no additional per-cube CPU admission or
  synthesized geometry. The immediate state0 pass follows second-context fades
  and precedes burst particles/water, explicitly setting White0/Glow1 while
  retaining the preceding WORLD opacity, shininess, depth offset and point tables.
  Focused CPU tests and three Vulkan comparisons cover these boundaries. Strict
  floating-point compilation preserves this module's staged arithmetic. All five
  targets build warning-free; full CPU/owner checks and73 shader,2 camera,24 GUI,
  6 map/shared-water/Alpha-scenery image checks pass. Five no-save gameplay/menu
  runs exit0, all frames viewed, with the save database unchanged. A new independent
  queued-state supplement distinguishes opaque grouping/readiness from faded
  setters; full resource grouping, failed-resource inheritance, the no-earlier-point-
  binding baseline, other effect families and native visual acceptance remain open.

- Captured zones now seed their gameplay item/object lists with the owner's exact
  CWR1 records, before the existing saved-state overlay. Missing or empty captured
  lists remain empty rather than being regenerated. Loaded static-object rendering
  follows authoritative replacements, including moved, resized, turned or removed
  objects; an empty received list removes previous geometry. Placement identity
  excludes clocks, interaction state and padding, so live door state can animate
  without rebuilding placement. Synthetic integration coverage checks whole-record
  preservation, source/overlay order, replacement geometry, live clocks and removal.
  All five targets build warning-free; full CPU/owner checks and70 shader,2 camera,
  24 GUI,6 map/shared-water/Alpha-scenery image checks pass. Five no-save captures
  exit0, all viewed, and the save database remains unchanged. This closes the
  captured-list and live-placement gaps for these paths; other content producers
  and native visual acceptance remain open. A separate stable clean-room supplement
  now specifies the owned-ground eight-cube consumer; its implementation is next.

- Steam ground rendering now consumes the live native-format zone lists in physical
  player-centered loaded-zone order. It checks exact timer0, class-adapted resources,
  the raw-position sphere and original-record validity, then uses the specified
  fixed hop, style transforms, two-axis tumble, channel0 material, RGBA pulse and
  selected-tuple addition. Pixel fade retains the un-hopped point and model range;
  normal winding, shininess and separate item opacity reach the descriptor consumer.
  Weapon spirits retain the supplied material and spectral-color policy. Stable
  zone/index tags interleave ground/base/spirits before that zone's static objects
  prior to queue consumption. Native interaction boxes share the placement math,
  and missing native resources no longer become invented bobbing pouch draws.
  Alpha/legacy drawing retains its separate existing path. Synthetic CPU cases and
  two Vulkan cases cover hop/sphere separation, centering, pulse/selection, spectral
  alpha, raw-position admission and faded ground/static ordering. All five targets
  build warning-free; full CPU/owner checks and70 shader,2 camera,24 GUI,6 map/
  shared-water/Alpha-scenery image checks pass. Five no-save gameplay/menu captures
  exit0, all viewed, with the save database unchanged. The owned-ground cube effect's
  downstream appearance, exact resource-pointer ordering, alternate callers, live
  list completeness and native visual acceptance remain open. Current indoor/blue
  gameplay and sky-only menu backgrounds remain stability fixtures, not native
  visual acceptance.

- Ordinary Steam static objects now come from loaded data zones independently of
  terrain GPU publication. Lookup never loads an absent zone; the physical player's
  inclusive seven-zone square supplies absolute XY/record ordering. Cached records
  refresh on zone lifetime changes and explicit invalidation, keep live door clocks,
  and do not duplicate the mesh-published object copies. Only small lifetime tokens
  are retained, so this cache does not pin obsolete terrain snapshots. Valid captured
  objects at fixed position(0,0,0) are no longer discarded. A synthetic headless
  World/Extracted/Renderer integration test covers these boundaries, source reset,
  same-lifetime refresh and missing-zone lookup. All five targets build without
  warnings; full CPU/owner checks and68 shader,2 camera,24 GUI,6 map/shared-water/
  Alpha-scenery image checks pass. Five no-save gameplay/menu runs exit0, all captures
  viewed, with the save database unchanged. This closes terrain-publication gating
  for stable source records; complete live object-list replacement, ground-item
  interleaving, special extra geometry and native visual acceptance remain open.

  Local workspace maintenance also removed122 obsolete generated source-staging
  trees (about1.154GiB) and regenerable Python bytecode. Recent snapshots, the current
  build/dependency setup, owner data, saves and reference captures were preserved.
  No standalone obsolete Makefiles outside dependencies were identified.
  A further62 obsolete smoke-test executable copies (about0.865GiB) were removed,
  retaining their captures/logs and the recent tested binaries. Total local cleanup
  reclaimed about2GiB; current build products and packaged checkpoints remain.

- Ordinary Steam creature poses now receive the prepared scene's frame material,
  effective channel0 physical-position light, ordered buff tint factors and separate
  raw-drawn-position fade/visibility opacity. The descriptor bridge retains RGBA,
  all five scalar controls and the producer's explicit normal/opposite winding.
  The WORLD shader consumes already combined material once, including underwater
  frames, supports negative Glow luminance, and applies clip DepthOffset. Faded
  descriptors use normal/opposite depth then normal/opposite color passes.
  The105 independently verified playable-hair resources retain raw RGB and hide
  only marker-owned faces, preserving raw neighbor and corner-occlusion occupancy.
  Owner models remain required; optional dual-tone hair and Alpha retain their
  separate policies. Full CPU/owner checks and68 shader,2 camera,24 GUI,6 map,
  shared-water/Alpha-scenery image checks pass. Five no-save gameplay/menu runs
  exit0, with all captures viewed and the save database unchanged. The final
  catalog-table readability cleanup was rebuilt and its focused checks rerun.
  These checks cover the specified finite inputs and available resources;
  complete catalog/factory policy, unavailable-resource retained state, exact
  resource-group ordering, alternate callers and native visual acceptance remain
  open. The blue/indoor gameplay fixture and sky-only menu background are not
  accepted as a finished native appearance.

- Ordinary Steam static objects now use their own admission path, independently of
  the environment model's cached terrain-box gate. Available scene records retain
  zone/record order and use the player's physical-coordinate inclusive seven-zone
  square, the independently specified half-height sphere and the model-range
  distance/fade limit without an extra64 blocks. Scenery and animated door/window
  transforms retain exact fixed-position anchors before float conversion; Alpha
  and shadow transforms keep their existing coordinate policy. Synthetic boundary,
  overflow, plane and fade checks pass, including a Vulkan object whose containing
  zone fails the environment-mesh gate. Full CPU/owner checks and57 shader,2 camera,
  24 GUI,6 map/shared-water/Alpha-scenery image checks pass. This closes the renderer
  gate for available scene records; supplying objects from every loaded zone before
  terrain-mesh publication, ground-item ordering and broader native visual
  acceptance remain open. Five hidden no-save Steam/Alpha gameplay, title and
  idle/hover character-selection runs exit0, with all captures viewed and the
  character database unchanged. Their visible blue/indoor scene and sky-only menu
  background are stability coverage, not native visual acceptance.

- Normal world pose admission now uses the current scene's camera, planes and actor
  range, in registration order, before any state/clock/spark/cache updates. The
  independently corrected sphere includes max(body height,0.6*max(width,depth));
  race12e adjustments and low64 center arithmetic remain separate. Actor data and
  sphere math are shared with speech/emotes, and all body parts carry the admitted
  actor sphere instead of generic per-part range gates. Renderer scene preparation
  is reused once by the GPU frame; earlier death/use queries retain the prior CPU
  scene until that boundary. Exact fixed camera coordinates and camera-relative XY
  plane inputs avoid a float-derived distance/origin substitution. Gameplay keeps
  its original descriptor queue position; the title-world caller uses the same
  policy and now performs its previously missing ordinary drawn/link phase.
  Direct widget/character-preview calls retain their separate policy.
  All five targets build without warnings. Focused real headless draw/order/cache,
  shared sphere and particle tests pass, as do full CPU/owner and48 shader,2 camera,
  20 GUI,6 map/shared-water/Alpha-scenery image checks. Current-scene replacement and
  one-time range preparation have Vulkan integration checks. Steam/Alpha no-save
  combat, title and character-selection runs exit0 with all four frames viewed and
  the character database unchanged. These are bounded stability checks; the blue
  gameplay scene, empty-sky frontend underlay and missing owner hair are not accepted
  as native pixel parity. Pose/body matrices still use the engine's source-home
  frame; their native cached-coordinate lifetime, factory/startup ordering, optional
  preview counts and broader shadow/scenery/viewport acceptance remain open.

- World work and local body/burst/scene-pose randomness now use the independently
  specified CRT32-bit transition and15-bit return. Pose sparks no longer use a
  static unrelated Mersenne Twister. Ordinary expansion and its once-frame update
  now precede scene pose, so spark initialization continues earlier World/body/
  burst draws. AI keeps its distinct engine state and unchanged CRT transition.
  The helper chooses no seed or thread; engine role states retain their earlier
  numeric initial choices, explicitly unverified as native entry/first-frame data.
  A known1037-draw initialization subphase and overflow test check the arithmetic;
  actual body, burst and state61 pose consumers check continued draws, zero-draw
  updates and separate execution-role state. First-frame state, all reached
  crafting/stock/daily/pet resets, exact pose admissions/visitation and remaining
  same-thread consumers are not closed by this result. No owner data is packaged.
  All five targets build; focused particles/body and full CPU/owner checks pass,
  with48 shader,2 camera,20 GUI,6 map and shared-water/Alpha-scenery image results.
  Both no-save combat styles exit0 with viewed frames and unchanged character data.
  Short hidden combat FPS samples vary substantially and are not a controlled
  performance comparison; extended profiling and the existing scene/image gates
  remain open.

- Combined sessions now own a separate host terrain reader, following the native
  allocation/local-byte-delivery ownership contract. Source metadata is copied;
  loaded identities, columns, model/light caches, history, queues and worker
  lifetimes are not inherited from the client. Host saved-zone callbacks, travel
  events and item placement use the authority reader; the client retains its own.
  A dedicated host keeps its supplied reader. Synthetic roof/sea tests exercise
  source metadata, fresh base loading, client solve versus host light, independent
  physical/history changes, copied protocol-notice application, client history undo
  and source reload without resetting the host lifetime. Generated rebasing and
  dedicated reuse have coverage. No host raw-light reset or guessed solver schedule
  is introduced; full host scheduling and original-peer/native image acceptance
  remain separate gates. The audited ordinary mesh worker is client-World-bound.
  Owner captures may already contain solved light bytes; forks preserve source data,
  without asserting that those snapshots reproduce every native host initial value.
  All five targets build without warnings. Focused storage/network/cache and full
  CPU/owner suites pass, alongside48 shader,2 camera,20 GUI,6 map and shared-water/
  Alpha-scenery image checks. Both no-save combat runs exit0 with viewed frames and
  unchanged character data. These short runs are stability checks, not performance
  benchmarks or acceptance of the known blue/cavity scene and missing owner hair.

- Ordinary consumers0/3/4/6/7/8 now use the independent style contract alongside
  existing1/2/5 and body records. Their gravity/angular differences, extra style4
  pass types, descending6 full-XYZ contacts and deferred retirement are implemented.
  Style3 stays still while aging; styles7/8 shrink geometry without changing the
  stored culling radius or rejecting lifetime equality. Style7 loads the owner's
  note.cub with its own full header, sets Glow/Shininess1 and resets shininess for
  following draws. Other glowing styles keep shininess0; ordinary lit types retain
  the common stored-block accessor. No new CRT draws or replacement geometry were
  introduced. Synthetic motion/header/retirement and GPU ripple/reset tests cover
  these bounded consumers. The first GPU expectation omitted the camera gradient
  translation; an independent shader-contract calculation corrected that test
  reference without changing rendering. Native duplicate-erasure recovery, unknown
  style numbers, broader producers and native CRT/pixel acceptance remain open.
  All five targets build without warnings; focused particle/body and full CPU/owner
  checks pass, with48 shader,2 camera,20 GUI,6 map and shared-water/Alpha-scenery
  image results. Steam and Alpha no-save combat runs both exit0 with viewed frames
  and the character database unchanged. Those blue/cavity scenes and missing owner
  hair remain unaccepted native visual comparisons.

- Completed mesh-light solves now write target-zone stored air/water red back
  before model samples and terrain/GPU installation, in both rendering styles.
  Other block bytes, halo records, loaded-zone identity, history and dirty queues
  remain intact. Physical writes are immediately readable; dirty requests preserve
  old red until another solve. Upkeep and lit particles use the same corrected
  stored-column accessor, including the native signed fixed-coordinate narrowing.
  Engine commits use serialized immutable snapshots rather than reproducing native
  per-record concurrent writes. Lazy object metadata now follows that same write
  scope, preventing it from being mutated while another writer copies the Zone.
  Actual roofed-room CPU mesh tests verify stored red, the native spread floor21,
  auxiliary/effective sea rules, unchanged halo/identity/queues and physical edits
  before remeshing. The full CPU/owner suite and all Vulkan image suites pass;
  final focused fixture/cleanup checks pass. No-save Steam and Alpha runs both exit0
  with viewed frames and unchanged character data. The first45-second observation
  window was too short; retained-handle runs allowed full startup/cleanup to finish.
  Images remain stability evidence, not native pixel acceptance. Native partial-write
  concurrency and broader visual/peer comparisons remain outside this bounded result.
  The existing combined-session setup still shares one terrain reader between host
  and client; independent World cache/light ownership is the next integration task.
  The independent main-thread CRT contract is available for later stream work; exact
  first-frame seeds and all reached reset/consumer integration remain open.

- Native body fragments now share the ordinary particle list, update, owner cube
  draw and1000-entry tail cap. Prepared sprite IDs and the exact ten cached part
  matrices are retained by value; death visuals no longer recompute a corpse pose
  or fragment an arbitrary draw list. Raw owner CUB data is read without requiring a
  ready source GPU mesh, and policy0 surface/marker checks precede the X/Y/Z sampling.
  Producer draws, quantized homogeneous positions/center velocities, tint, negative
  delay, retained reflected basis and randomized temporary/head insertion follow the
  independent body contract. Body and burst cap/order, the existing relative falling
  sample, cached pose slots and matrix/header behavior have focused regressions.
  Received death notifications now collect frame-local actor-instance candidates;
  strict-negative local-player damage collects one too, while exact-zero subtraction
  does not. Deduplication, prepared/retained-renderer admission and unsigned identity
  order are shared by body production and its following race90 light. Births run
  after received events and before control/revival/current-frame pose preparation.
  Progression/persistent deaths remain separate; later frame notifications may
  collect the same corpse again. Recorded-only DeathEffects remains compatibility code.
  The independent light query correction now reads auxiliary byte3, not flags byte5;
  tests deliberately distinguish those bytes. Stored-red publication/upkeep binding
  is the next integration from its newly written contract.
  All five targets build without warnings. Focused particle/body and full CPU/owner
  suites pass, along with46 shader,2 camera,20 GUI,6 map and shared-water/Alpha-scenery
  image checks. A final main-phase build and no-save combat run exit0 with a viewed
  frame and unchanged character database. This is stability evidence; native body
  pixel/CRT acceptance and the blue scene/missing owner hair remain open. Remaining
  ordinary-style consumer prose is now available; its implementation follows.

- Particle admission now uses native renderer+0x49c, the shorter ordinary-model
  radius, rather than the wider adaptive+0x4a4 scene/fog radius. A Vulkan regression
  reproduced the prior erroneous admission between those radii; both admitted and
  cutoff cases pass after correction, alongside all45 Steam shader checks. All five
  targets build. The preceding particle CPU/other image results remain the baseline;
  this correction changes only the particle gate. The independent
  [body-particle contract](specs/steam-body-voxel-particles.md) now specifies the
  next shared-list integration, including retained prepared parts and head insertion.

- Ordinary particle bursts now enter a local live list before outgoing events
  clear. Every style consumes its shared expansion draws; supported styles1/2/5
  implement their extra color/age draws, fixed-coordinate motion, once-frame
  integer-ms update, collision queries and strict lifetime retirement. The1000-entry
  cap discards the newest tail after expansion. Zero-time style5 queries and the
  collision-off double angle advance have focused regressions.
  Geometry loads the owner's cube.cub with full-header centering; no substitute
  model or owner bytes enter packages. The before-water draw uses an opaque,
  depth-writing LEQUAL/no-cull pipeline, source-size sphere admission and inherited
  WORLD point tables/opacity. A128-byte push layout supplies per-instance material
  RGBA, with identity defaults for existing draws. Style5 uses the documented stored
  block channel0 accessor, including upward type10/12/19 traversal and the light floor.
  All five targets build without warnings; the full CPU/owner-data suite,43 shader,
  2 camera,20 GUI,6 map and shared-water/Alpha-scenery image checks pass. New GPU
  cases check zero material alpha without blending, reversed faces and RGB material.
  A no-save combat run exits0 with a frame and unchanged character database; it is
  a stability check, not native particle-image acceptance. The blue scene and
  missing owner hair remain visually unaccepted. Other particle styles retain only
  shared expansion state; body fragments still have a separate simulation/cap.
  Shared body-list integration, full native CRT streams and completed native
  block-light publication remain open, alongside wider visual/peer acceptance.

- Destructive race0x90 first visits now emit debris, apply guarded terrain removal,
  append day-stamped notices/sound, clean unsupported ordinary models and then visit
  live area targets in the specified order. Zero-time repeats retain their native
  history behavior without breaking blocks twice. Model cleanup selects loaded zones
  from independently clamped bounds, retains water support, erases semantic fields
  backward and preserves destination padding, cached light and sticky water state.
  Auxiliary light/emitter lists remain unchanged. Renderer model refresh is separate
  from terrain notices; immutable snapshots and stale-job checks prevent a completed
  older mesh job from reinstalling deleted models or annotating reindexed entries.
  Pure producer and actual facade/column/model regressions pass. All five targets
  build, the full CPU/owner-data suite and Vulkan shader/camera/GUI/map/water/Alpha
  image checks pass. A no-save combat run exits0 with a captured frame and unchanged
  character database. The blue-lit scene/missing owner hair remain unaccepted;
  native peer, material recapture, publication latency and full CRT parity stay open.
  The independent [particle contract](specs/steam-particle-bursts.md) now specifies
  ordinary styles1/2/5 and binds their geometry to owner cube.cub; its consumer is
  the next implementation task, rather than a completed runtime feature.

- Physical terrain writes, history and mesh requests now have separate runtime
  paths. Abilities and active effects use the guarded in-column removal helper;
  full writes/restoration share the specified setter with generated terrain.
  Batches publish one immutable copy per edited zone while same-thread physics
  sees current edits, including nested resets. Background-reader tests cover
  publication and memo invalidation. Loaded-zone history appends no longer replay
  terrain or create missing-zone archives; every World substep records the entire
  outstanding notice list, retaining native repeated entries.
  Received packets concatenate per list before consumption; zone-change processing
  runs once per admitted client frame even with no received lists. A two-packet
  loopback regression reproduced the prior interleaved ordering before correction.
  Notice-driven mesh requests use deduplicated XY block samples at -13/0/+13,
  loaded zones only. Exact native mesh-publication timing remains an acceptance gap.
  The full setter now preserves requested cells, canonical sea substitution and raw
  upward gaps, and colors only downward interior gaps from cached materials.
  Generated zones retain those inputs while releasing their larger temporary fields.
  The extractor writes optional CWM1 material sidecars; readers validate exact size,
  coordinates and finite values. Legacy block-only exports remain readable but
  cannot supply missing downward-fill materials; DataCheck reports refresh needs.
  All five targets build; focused snapshot/adapter/setter/network tests and the full
  CPU/owner-data suite pass. A no-save loopback combat run exits0, captures a frame
  and leaves the character database hash unchanged; the blue-lit scene/missing hair
  remain visually unaccepted. Fresh native material-capture comparison, model support
  cleanup and the first destructive race0x90 visit remain open.

- Received packet7 world events now remain queued while the client world advances
  and smooths drawn positions, then are consumed before player actions. Creature
  and clock reception still precedes that World pass. This corrects the old early
  application of blocks, buffs, hits and effects against the independently verified
  callsite order; the session/client API comments now state the same cut points.
  All five targets build without warnings and the full CPU/owner-data suite passes.
  A no-save loopback combat run exits0 and captures a frame; the character database
  hash stays unchanged. Its blue-lit scene and missing owner hair remain unaccepted.
  Original-game peer and wider gameplay acceptance remain separate open checks.

- Independent terrain contracts now identify ordinary-model support cleanup,
  in-column physical removal, authority/received history appends and the later
  asynchronous mesh request. The [block lifecycle contract](specs/steam-block-change-lifecycle.md)
  records received empty-to-water handling and retained-notice history duplication;
  these facts are not a claim that all terrain/cache wiring is implemented. Full
  saved-history setter behavior, auxiliary-list membership refresh and exact mesh
  publication latency remain outside that bounded trace.

- Destructive-source deferral now applies only to its first-clock-zero terrain
  branch. A nonzero-clock race0x90 corpse advances and retires normally without
  repeating that branch. New pure and vector-backed facade regressions reproduced
  the previous retained-corpse failure before the correction. All five targets
  build without warnings; focused and full CPU/owner-data suites pass afterward.
  The first explosion's terrain/scenery implementation remains open.

- Non-terrain dead-source areas (races 0x119/0x11d) now execute before their
  death-clock advance. They visit the live registration list with the specified
  double-horizontal/float-vertical bounds, small-distance push, critical draw and
  remembered-target ordering. Blast hits use type1; hostile water hits use the
  resolved owner and type2. Friendly water queues two identical negative-damage
  notices before applying healing once through the shared native hit facade.
  First-burst initialized fields enter the common particle queue before nested
  hit output. Native producer alpha/padding are unspecified; the engine uses
  defined storage without asserting a deterministic native value. The independent
  consumer trace establishes opaque material submission for these styles, rather
  than a native outgoing alpha1. Tests cover wrapping, boundaries, late rejection,
  zero-time repeat, callback births, same-pass deaths and actual facade damage/heal.
  All five targets build without warnings; focused and full CPU/owner-data checks
  pass. A no-save combat run exits0 with the character database unchanged and a
  captured frame. The destructive 0x90 first visit, shared terrain/scenery mutations,
  local burst expansion/rendering and broader visual/CRT parity remain open.

- Ordinary nonplayer corpse lifecycle now runs after the object pass. Authority
  advances the float death clock and retires eligible actors at 1,000 ms in
  registration order, clearing their aggro/damage references before deferred
  vector removal and borrowed-view rebuilding. Owned kind6 actors instead revive
  at 20,000 ms when their owner's HP is nonnegative; physical position and health
  are restored while the specified clock, dead ID and other retained fields stay.
  Missing/negative-health owners do not trigger ordinary corpse removal. Existing
  timed/instanced enemy respawn policy remains in place. Synthetic cases and real
  facade tests cover multiple removals, a subsequent substep and owned revival.
  All five targets build without warnings; focused tests and the full CPU/owner-data
  suite pass. Two no-save combat runs exit0 with the character database unchanged;
  the stationary run produces a frame, while moving capture reaches its settle
  timeout without one. The blue-lit scene and missing owner hair remain unaccepted.
  This lifecycle checkpoint deferred all three area-source races; the newer area
  checkpoint above adds two. Destructive terrain and complete frame ordering stay open.

- The specified static-object trap tail now runs in World substeps, including
  zero-time calls. Collection clears state bits 0x2002 on every registered actor;
  kind0 players select only already-loaded zones, retained and visited in unsigned
  stable-identity order. Each object's clock and personal reset precede its own
  automatic state cycle and hit callbacks. Types 0x11/0x12/0x13 use the verified
  strict ranges, float clock buckets, maxHP fractions, push and sound/RNG order.
  The shared hit facade owns actual damage and notices; direct trap sounds follow
  nested hit outputs and are delivered once locally while retained for distribution.
  Physics also receives the current World clock, restoring timed lava damage.
  Synthetic integration checks cover actual local health/events, a legacy callback
  with player ID 77, rejected hits, callback births, zero-time repeat, float rounding,
  clients and overflow coordinates. All five targets and the full CPU/owner-data
  suite pass; a no-save gameplay run exits0 with the character fixture unchanged.
  The inherited blue movement scene and missing owner hair remain visually unaccepted.

- Personal-state cells now share the verified signed32 block narrowing before
  floor-to-cell conversion across object state, lighting, lore, treasure receipts
  and affected ability lookups. A regression first reproduced six failing object
  cases; boundary checks include negative exact values and int64 minimum. Loaded
  zone identities survive copy-on-write block edits/undo, preserving visit order
  while the block lookup memo still invalidates its replaced storage.
  The independent [trap contract](specs/steam-world-traps.md) is implemented within
  that stated scope. The new [dead-area contract](specs/steam-world-death-area.md)
  documents the separate area pass; only its ordinary lifecycle is wired. Other
  object families, full prelude ordering and the complete native CRT stream remain
  open, as do the broader visual and gameplay release gates.

- A new isolated original-game capture resolves the two region-reference failures.
  Its provenance reports the supported build and zero other mod DLLs. All 3,823
  sampled points across 25 terrain fields, 4,847 block samples and 60,000 cell-object
  samples match without generator or expected-value changes. The refreshed 1,596
  zone records include 17 nonempty lighting zones: 8 model and 11 static light records
  match, with zero incomplete neighborhoods. The full CPU/owner-data suite now
  passes; all five targets build. This sampled coverage does not establish every
  region, dungeon, runtime path or pixel comparison.
  The journaled capture restored Save, Mods and ReShade's dxgi.dll; all 58 original
  file hashes match and the journal is closed. The old EngineTests access violation
  came from the already-corrected native-player test setup; its focused rerun passes.
  Extractor startup no longer scans/exports executable function bytes, and hook
  errors identify their location without printing prologue bytes. The engine
  behavior and asset/data capture paths were not changed by that diagnostic cleanup.

- Loaded-only terrain block lookups now distinguish reader lifetimes and invalidate
  their memo when zones are loaded, evicted or replaced. Weak references avoid
  retaining unloaded terrain. Source resets join the reader's prefetch worker before
  clearing its cache, and concurrent loads preserve one resident identity per key.
  Synthetic checks cover reopening while an old neighbor is pinned, same-storage
  reader reconstruction, LRU eviction and release of the old terrain. The fixture
  deliberately uses a non-home neighbor because opening eagerly loads the home zone.
  All five targets and the focused checks pass. Full CPU/owner-data validation has
  only the two known modded region-reference failures. A no-save movement run exits0
  with the character save unchanged; its blue scene remains unaccepted. Separate
  stationary captures show the house/terrain under Steam Native/Omega water and
  Alpha Native water, without establishing pixel parity or a rendering fix.

- The [host CRT audit](specs/steam-host-crt-stream.md) now separates constructing,
  zone-generation and simulation threads, and records the known refresh/pet seed
  producers and continuation rules. The fresh simulation thread's numeric state
  remains unverified; neither the public world seed nor a per-zone seed is a valid
  substitute. The legacy resident altitude RNG integration remains open.

- Resident zone selection now uses the native fixed/narrowed inclusive bounds,
  snapshots loaded runtime Zone identities, retains their storage through the pass,
  and visits unique nonnull identities in unsigned address order. Each selected zone
  completes archive restoration, daily refresh, resident admission and kind9 request
  production before the next zone refreshes. Metadata is initialized independently
  of whether an actor vector already exists for the zone. Unit checks distinguish
  unsigned pointer order from coordinate/encounter order and deduplicate aliases;
  owner-data checks retain the same loaded identity through daily metadata refresh.
  This matches the ordering rule for the engine's runtime allocations. Exact external
  replay still needs the reference zone sequence and incoming CRT state; the legacy
  altitude RNG remains an explicit gap. All five targets and the new checks pass;
  full CPU/owner-data validation retains the two known modded region-reference failures.
  A no-save gameplay run exits0 with the fixture save unchanged. The inherited blue
  scene and missing owner hair model remain unaccepted; native visual comparison
  and clean generator references are still required.

- The specified resident kind9 producer now tracks existing full-word mode0x8a
  separately from newly copied low-byte modes. Per-zone requests create day-derived
  fixed candidates, apply inclusive200-block XY suppression against current live
  kind9 effects, and append independent live/outgoing records. The ordinary effect
  step advances only the live copy later in that World step. Tests cover native
  numeric references, wrapped centers, request ordering, expired-effect suppression,
  exact fields and copy lifetime. The semantic initializer preserves native-unwritten
  damage/padding in supplied defined storage; engine-owned fresh records use zeroed
  storage without claiming those bytes have a native zero value. No new art or
  replacement kind9 renderer is introduced. Zone processing order, CRT state and
  broader render consumers remain separate limits.
  All five targets build; focused kind9 and full CPU/owner-data checks pass apart
  from the two known modded generator-reference assertions. A no-save gameplay run
  exits0 with the fixture save unchanged. No native visual-acceptance claim is made.

- Resident collection now follows registered kind0 membership, including dead players,
  and snapshots both region-permission sets before zone work. Live positions are read
  through stable actor resolution during admission; streaming stand-ins cannot supply
  a player. Existing nonnull registered IDs suppress duplicates independent of HP or
  generated-ID bookkeeping, and a birth immediately joins that admission set. The
  final petrification pass includes local, network, dead and new registered actors.
  Resident transfer retains native16-block home cells with the specified coordinate
  narrowing, truncates copied position toward zero for region lookup, zero-extends
  style32 into64 bits, and preserves low64 altitude addition. The shared fixed-block
  conversion also drives terrain rays. Asset-free tests cover collection/permissions,
  moving storage, admission ordering, inclusive range edges, extreme coordinate
  narrowing, field-copy widths and constructor/postprocessing order. Owner-data
  checks retain their respawn assertions with explicit registered player fixtures.
  Zone-pointer processing order, incoming/persistent CRT state and the remaining
  zone-effect/prerequisite producers are still separate integration gaps.
  All five targets build, the full CPU/owner-data suite has only the two known
  modded region-reference assertions, and both camera Vulkan styles pass. A no-save
  movement/combat run exits0 with the fixture save unchanged. The blue scene and
  missing owner hair model remain unaccepted; this is not native visual parity.

- Authoritative resident creation, visited daily refresh and admission now require
  that terrain zone to be loaded. The collector no longer creates creatures for an
  absent terrain world or refreshes retained resident metadata through an unloaded
  zone. This uses the existing independent remaining-runtime-gaps §6.1 contract;
  complete zone visit ordering, RNG producers and prelude integration remain open.
  The unload/reload respawn fixture explicitly loads terrain and retains its single
  comparison zone as a separate view while the host's resident zone unloads. Its
  respawn assertions remain intact. All five targets build; full CPU/owner-data
  checks have only the two known modded generator-reference assertions.
  A no-save movement/combat run exits0 with the fixture save unchanged. The blue
  scene and missing owner hair model remain unaccepted; this is not visual parity.

- Host batches now consume an admitted zero-time residual and run both drawn-state
  passes once with the full capped interval, followed by zone output collection.
  AI collection uses retained World registration order instead of placing the local
  player first. The independent [daily-reset contract](specs/steam-world-daily-reset.md)
  puts the authority notice reset after creature bodies and before active effects.
  Pending births join the reset, and all actor views and ID/pointer caches are rebuilt
  after deletion. Kind0 guests, loaded-zone state, effects and queued events survive.
  A queued zero notice remains eligible in every later substep until host distribution
  clears the outgoing group. Configured respawn deadlines also survive a death caught
  by the reset before the adapter's next death scan. Focused checks cover tied AI
  choices, zero/33-ms host intervals, reset/effect order, repeated notices, compacted
  guest storage, queued kill credits and the mirror's authority boundary. Broader
  time/resident preludes, lifecycle producers and native-game comparisons remain open.
  All five targets build; full CPU/owner-data validation retains only the two known
  modded generator-reference assertions. Both camera Vulkan styles pass. No-save
  movement/combat and dialogue-request runs stop normally with the save unchanged.
  The dialogue request produces no speech bubbles, so it is not native conversation
  acceptance. The inherited blue scene and missing owner hair model remain open.

- World body updates now use retained creature registration order. A separate
  full-list pass clears state0x10 before body callbacks, including dead entries and
  called zero-time steps. Physics/ability/interaction retain one HP admission, then
  upkeep runs without the former outer loaded-terrain skip. Synchronous births
  extend the indexed body traversal after the completed clear pass.
  Once-per-frame drawn state now has two full registered passes before local input:
  native quantized XYZ plus the second Z correction, angle/look smoothing and raw
  timer/step side effects; then ordered carried/seat/spin/look/focus/pet placement.
  Linked poses retain distinct raw/drawn fields, independent fixed conversions,
  live relationship gates and the specified sole smoothing-control reset.
  Mesh drawing no longer recopies raw state after input. Shared native angle math
  corrects the widened-float threshold in control, abilities and physics too.
  Packet receivers preserve omitted fields and accept wire timestamps directly;
  the invented200-ms prediction cap and countdown are removed. Derived math/row/
  gate tests and actual-facade pass-order tests pass. Native packet/interpolation
  regressions replace the former engine-extension prediction expectations. All
  five targets build; the full CPU suite has only the two known modded region
  oracle assertions, and both camera styles plus20 GUI Vulkan cases pass. Read-only
  combat/movement and dialogue runs exit normally with the fixture save unchanged.
  The inherited blue scene and missing owner hair model are still unaccepted.
  Complete
  time/resident preludes, AI collection order, reset/roster-rebuild lifetimes and
  original-game/network comparisons remain open.

- Native gameplay camera positioning now retains fixed eye/look-at state. Its
  candidate uses physical coordinates, separate step/posture truncation, living
  mount offsets and boat height. Native same-cell/corner probes, distinct ordinary
  XY/Z smoothing, ability-specific anchors and the fifty-block safety snap replace
  the fixed head offset. Eight visibility guards and sequential shortening rays
  replace the stepped wall check; shortened current distance persists separately
  from requested zoom. Camera rays use type-only obstruction, including exact
  zero-maximum zone lookup without a block read. Audio and next-frame targeting
  retain the fixed eye/anchor directly. Native inverse rotation drives world,
  clip and speech views even at coincident eye/anchor or a vertical camera pole.
  Posture/probe/rate/rounding/guard regressions pass. Both Vulkan styles pass
  coincident/pole/projection/culling tests, and all40 world/20 GUI/6 map image cases,
  shared water and Alpha scenery pass. All five targets build; the full CPU run
  has only the same two modded generator-reference assertions. Read-only combat
  and dialogue runs exit normally with the fixture save unchanged. The blue
  fixture appearance and missing owner hair model remain unresolved/unaccepted.
  Full double-degree angle storage/producers, native startup/reset transfer,
  FOV/render-origin matrix fidelity and live GUI admission remain open.

- Target-candidate visibility now queries the completed renderer epoch, including
  the outer height-aware sphere and separate step-adjusted inner width/range.
  Candidate float keys use the independently specified insertion/partition/heap
  ordering rather than an always-stable sort. The shared admitted loop retains
  strong-charge selection independently of ordinary selected ID and aim depth.
  Its score truncates each low64 squared axis before summing; the first hostile
  kind seeds it and later kinds replace only on a strictly lower score. The live
  path no longer makes a second line-sphere/nearby charge search. Ability0x23's
  hostile exclusion is strict below duration. Derived regressions cover tall
  candidates, distinct frustum epochs/radii/points, quantization, neutral seeding,
  ties and the duration boundary. Full camera state, GUI focus and native/device
  comparisons remain open. Targeting now visits the engine's retained creature
  registration order, including ordinary outer admission of the local player
  before inner skip-self. Updates/storage moves keep their membership position;
  removal/recreation and a reloaded local player join the tail. Resident, runtime,
  network and newly added creature paths record registration without copying raw
  records. This removes zone-grouped/local-first ordering from candidate input.
  Full native list-mutation producer coverage and other World consumers' visit
  order remain separate validation/integration work. All five targets build;
  control/target/registration regressions pass. Both Vulkan lighting styles pass
  retained culling epoch/range/lifetime checks and camera projection checks.
  The full CPU run retains exactly the same two modded generator-reference
  failures, with no new failures. Read-only combat/movement and conversation runs
  exit normally with the fixture save unchanged. Inherited fixture appearance
  remains unaccepted; these checks do not establish native visual parity.

- Creature targeting now runs once before early control. Selected IDs, the
  projected-box flag and independent minimum aim depth survive into the later
  object/item/fallback pass. Cached aim metadata is evaluated after movement at
  its admitted early stage; assistance follows ultimate processing, before owner
  look. Live speaker override and manual-stick slowdown precede camera smoothing.
  The sixty-block aim ray follows the independently specified fixed traversal,
  loaded-zone lookup order, crossing budget and flag0x40 obstruction rule. All
  four target LOS calls use fixed endpoints and a distinct type-only predicate,
  double range rejection, float normalization and full-length-minus-epsilon
  comparison. Creature step offsets and object half-heights truncate before low64
  coordinate addition; ground LOS retains its raw tuple. Target/control/ray tests
  cover separate phases, depth/rank ties, high-coordinate low bits, strict range,
  flags, zero crossings, missing zones and nearby fallback without extra LOS.
  Legacy radian storage, startup/reset/FOV integration, live GUI focus and
  original-game comparisons remain open. Camera positioning and its eight-corner
  terrain caller now use the independent fixed contract described above. All five targets
  build; focused tests and the full CPU suite add no new failures. The latter
  retains exactly the two modded generator-oracle assertions described below.
  Read-only movement/attack/dodge and conversation runs both exit normally with
  the fixture save unchanged. The inherited blue fixture scene and missing
  owner orc-hair-m08 model remain unresolved; these runs are integration checks,
  not native visual acceptance.

- Local control now has explicit early movement/ultimate, owner-look, pet and late
  attack stages. The canonical live path runs Use before pet, Quick Item before
  late attacks, and samples ordinary UI again at that late gate. Chat skips the
  remaining early actions and edge stores. Movement/lamp/pet/ultimate use retained
  logical inputs rather than a broad panel-visibility mask; Ring and pad/full-map
  still have their separate movement gate. Full focused-Widget plumbing is not
  inferred from visible panels and remains part of shared GUI integration.
  The facade now supplies region-owned special-item checks and direct sound hooks,
  resolves the local player in control lookup, and forwards ultimate buff notices
  to outgoing World events. Device-free staged/actual-facade tests pass, including
  post-Use pet timing, strict mount boundaries, owner look and buff/callback output.
  All five targets build; a read-only movement/attack/dodge run exits normally with
  the fixture unchanged. The full CPU run still has only the two reference failures
  below. Full camera state and live GUI admission
  remain open; these changes do not complete controller/GUI parity.

- The rendered/client World now completes its substeps and one full-frame drawn
  position update before creature targeting and local input. New input abilities
  are not immediately simulated again in the same frame; control precedes camera
  smoothing. World meshes collect once after current object/block updates. All
  five targets build. A hidden read-only run exercised movement/combat and exited
  normally; still captures against the previous build retained the same scene,
  and the fixture save hash stayed unchanged in all three runs. The fixture's
  blue environment is present in the baseline too; this is not a native visual
  parity certificate. Earlier/later input cut points and live GUI admission remain
  open under the expanded independent frame-order specification.

- Movement controls now sample retained displayed angles rather than mouse/pad
  targets. Their unshaken left/up/forward basis and the frontend forward vector
  share native float radians/trig, including pitched/rolled up vectors. Focused
  angle/pole/orthogonality checks, all five builds and both Vulkan camera styles
  pass; the full CPU run still has only the two modded-oracle failures below.
  Gameplay's legacy radian storage, complete frame/target order and full camera
  positioning remain open. The independent ordinary-frame supplement now traces
  World-before-input and separate early/late action gates; it is a specification
  for remaining integration, not a claim that the whole loop is already matched.

- Camera/audio timing now follows the independent ordinary-frame/host distinction.
  Frontend and gameplay orbit distance use rate 0.004 independently of angle
  smoothness; camera easing retains the capped double wall interval. Ordinary
  world simulation splits that interval into float 16-ms steps and a narrowed tail,
  including the zero tail, with no carried whole-ms balance. The live loop no
  longer supplies a separate 100-ms clock. Shake reads the client world's float
  elapsed simulation clock after those substeps. Explosion notices use physical
  player squared distance, strict maximum accumulation and same-frame rate 0.005
  decay, even when playback is muted/inaudible. Native cosine shake translates
  only the render/clip/speech view, leaving stored eye and targeting unshaken.
  A regression reproduced the fractional-time error before its correction.
  Independent float references, actual device-free Audio consumption, fractional/
  long-frame/lifetime tests and two Vulkan projection cases pass. All five targets
  build; 40 world/20 GUI/6 map Vulkan cases, shared water and Alpha scenery pass too.
  The full CPU/owner run completes with exactly two known generator-reference
  failures: an owner modded launch overwrote the region-pair oracle. An isolated
  original-game capture is still required; engine generation and assertions have
  not been changed to match that modded data. Double-degree angle state, full
  startup/reset/render-origin/FOV transfer and remaining GUI integration stay open.

- The shared GUI foundation has independently specified pointer routing, capture/
  focus/auxiliary lifetimes, button state/groups, cached owners and frame placement.
  Numeric, reentrant routing and owner-tree regressions pass. This dispatcher is
  not yet connected to the complete live GUI. Retained cursor images now borrow
  the canonical transient stack at the admitted panel's callback tail; no new
  drag/grab/save producer is inferred. Controls local defaults/edits remain
  separate from active loaded bindings and survive reopening. Full capture and
  Controls thumb-drag integration remain release work.

- Map history, wheel/shoulder controls, hovered/forced land state and border depth
  now have bounded CPU/owner and six Vulkan checks, including inherited world
  occlusion. Procedural artifact resources have a shared full-key cache and atomic
  extractor request/status protocol. 332 owner native-lookup samples, captured
  with the owner's mods active, match generated RGB/semantic metadata; they are
  not an exhaustive or vanilla-world parity claim. Local captures and all
  original assets remain excluded from source/runtime packages.

- Current map matrices now precede GUI drawing, and marker collection/projection
  runs inside the admitted Overlay callback after earlier game-UI widgets.
  F4 suppresses the 3D map too; half radius uses retained Control message state,
  independent of full-map mode. Terrain picks precede later marker cursor snaps;
  pointer text samples retained cursor data earlier using its float32 norm.
  Eased screen centres truncate to pixels before projection. Numeric/owner-tree
  regressions, all five targets and the full CPU/owner suite, 40 world/20 GUI Vulkan,
  water and Alpha checks pass. A fresh hidden owner-data capture completed with
  the test save unchanged. Earlier input/centre/toggle phase fidelity, hovered-land
  highlighting and procedural artifact resources remain open.

- Native map information now uses the independent map-location/lore audit.
  Overlay drawing publishes semantic item records and independent flight/lore
  outputs; hidden draws retain them. Earlier coordinates stay copied while the
  item and target-kind flag remain live. Flight pricing/distance/time and static
  styled lore words use their specified arithmetic, spacing, fixed origins and
  late geometry. Rejected lore suppresses the item branch. Owner-tree and numeric
  regressions pass, along with all five targets and the full CPU/owner suite, 40 world/
  20 GUI Vulkan, water and Alpha checks. The expanded 789-snapshot strict audit
  still reports the existing street-lamp turn differences in three fixture sets;
  this is a development checkpoint, not a final parity release. Complete map
  collection/camera/capture timing and earlier inventory sampling remain open.
  A read-only owner-data capture confirms artifact tooltip text; category23
  procedural voxel resources are not yet supplied to its icon renderer.

- Ground and Ring information are connected. Ring keeps the canonical key sampled
  before later toggle/repeat work; ground fallback resolves existing zone/index
  data fresh at draw without loading zones or applying new validity filters.
  Reserved-origin metadata remains separate when Ring supplies the displayed item.
  Native bottom placement uses the prior admitted widget height. Target production
  now rejects quantity zero while preserving negative nonzero quantities, distinct
  from later record resolution. Packet-backed cleared-slot, target-priority and
  owner-placement regressions pass, as do all five targets and the full CPU/owner,
  40 world/20 GUI Vulkan, water and Alpha checks. Private read-only Ring captures
  exercise both GUI scales; the final executable capture leaves its fixture unchanged.
  Full earlier UI/comparison sampling remains open; the bounded map-output
  implementation is described above.

- Information now reads its borrowed original during the later draw callback,
  after earlier palette updates, rather than freezing its text/image beforehand.
  Hidden GUI ancestors skip widget hooks and late size writes while own panel
  state, placement and bindings survive. Map-marker labels follow the game-UI
  windows and precede information. Owner-tree tests cover live palette reads,
  hidden-state retention and resume; all five targets and the full CPU/owner,
  40 world/20 GUI Vulkan, water and Alpha checks pass. F4 repeats are forwarded and
  visibility changes precede binding-wait gates. A const existing-zone ground
  resolver is ready for source wiring. Earlier sampling, map/ground/Ring bindings,
  full GUI layers and synchronous message/focus timing remain open.

- Native item details now use an independent text plan for the specified category
  rows, stars/region/realm text, artifact/rank/pet effects, healing/demon/consumable
  text, exact float rounding and separately rounded comparison deltas. Equipped
  comparisons preserve original address identity, candidate/Shift order, smaller
  fonts and two late resizes. Inline suffixes follow the real glyph pen; fractional
  fonts share the owner atlas. Tests reproduced missing glyphs and stale standalone
  GUI scale before correcting both. All five targets, the full CPU/owner suite,
  40 world/20 GUI Vulkan cases, water and Alpha checks pass. A private read-only
  gameplay captures at two GUI scales display the selected weapon and its original
  equipped comparison, with unchanged fixture hashes and matching validated binaries.
  Earlier selector/source sampling, live map/ground/Ring bindings and broader
  GUI/input/camera and system checks remain release gates. No owner assets ship.

- Native item-preview selection now positions from stored geometry before its
  later draw. The owner background/children precede icon/text and the text-derived
  resize; a hidden draw retains the size, and growth/shrinking affects the next
  background. Four owner-tree lifecycle regressions fail against the previous
  implementation and pass with this correction. All five targets, the full
  CPU/owner suite, 40 world/20 GUI Vulkan cases, water and Alpha checks pass.
  Ring source selection also retains
  its independent ground-origin flag, rather than forcing it to describe the
  selected canonical item. The later formatter/comparison implementation is described
  above; live source producers and timing still prevent complete PreviewWidget parity.

- Native-save customization now uses the actual owner editor root and shared
  WORLD models: equipped thumbnails, raw base/attributes/candidate and material
  palette, with native picking, cached clicks, rotation and bench/movement gates.
  Ordinary item downs rejoin the editor before shared health/chat/Ring completion.
  Full-coordinate pickup/removal and byte placement retain their distinct rules.
  Palette information follows native source priority and the hidden crafting
  anchor; text and image use a separate class-adapted copy. Gameplay wheel events
  are cleared before each poll, preventing idle-frame replay. Focused regressions,
  all five targets, the full CPU/owner suite, 40 world/20 GUI Vulkan cases, water and
  Alpha checks pass. Private read-only owner-resource captures exercise the pane
  and palette tooltip; they are not original-game pixel comparisons. Broader
  cached inventory/input state, GUI capture/map controls, preview source timing
  and map/ground bindings, controller aim and system checks remain release gates.

- Physical inventory wheel messages now retain their event pointer and sign,
  selecting one owner-tree hit in inventory/shop/crafting priority. Shop and recipe
  grids can receive wheel input while the bag is visible elsewhere. Opposing
  messages remain separate; fractional or large deltas do not accumulate into
  different row counts. Idle input passes leave the stored row alone. Synthetic
  priority/sign cases cover this routing. Cached widget range/thumb state, precise
  refresh callbacks, first binding and broader native input comparisons remain open.
  All five targets build; the full CPU/owner suite,40 world and20 GUI Vulkan cases,
  shared water and Alpha ordering pass.

- Alpha shared water now draws its sorted fading-scenery queue after the sky and
  before the refraction snapshot. Previously, transparent scenery could write
  depth before its sky background and leave dark silhouettes. Native water keeps
  its after-water queue, and CPU fade values and depth writes are unchanged.
  New Vulkan regressions failed before the change and pass afterward: dry partial
  fades match between water modes, and beyond-endpoint fades retain the sky.
  Native admission/cache traversal and broader original-game comparisons remain open.

- Character creation now includes optional dual-tone hair controls: second-color
  preview/selection, enable, transition start and blend. The native palette and
  runtime-loaded hair models are reused without a mod DLL. Per-character engine
  notes retain the style through local selection, play and map rendering, leaving
  the native appearance record unchanged. Synthetic tests cover gradient/skin
  handling, hover/commit, bounds, cache identity and metadata round trips. Hidden
  read-only creation captures exercise Steam and Alpha lighting. Remote-player
  synchronization of this extra appearance remains open; no owner assets ship.
  All five targets build; the full CPU/owner suite,40 world and20 GUI Vulkan cases,
  shared water and Alpha ordering pass. The captures verify the new controls and
  hair; broader background/camera and original-game visual comparisons remain open.

- Customization's native edit projection, object-space pointer ray and voxel/attribute
  traversal now have independent engine implementations and synthetic regressions.
  Checks include full header centering, scale independence, stored lens words,
  exact negative-integer cells, marker exclusion and wrapped attribute coordinates.
  The GUI renderer accepts the raw edit matrices/material with its specified WORLD
  palette and retained state. All five targets, the full CPU/owner suite,40 world
  and20 GUI Vulkan cases, shared water and Alpha ordering pass. The editor root,
  presentation and event continuation are now integrated above; these backend
  checks alone do not establish native pixel parity.

- Native customization palettes now read every canonical tab directly: the selected
  weapon's exact base-material item precedes four spirit-material aggregates.
  Signed counts wrap and may remain negative; aggregation retains its distinct
  full-equality and field-only rules. Clicks consume the preceding draw's palette;
  the later draw rebuilds its entries. Synthetic regressions cover ordering,
  extra tabs, inactive attributes, heterogeneous spirits, cancellation and overflow.
  All five targets, the full CPU/owner suite,40 world/16 GUI Vulkan cases and
  water/Alpha checks pass. The later owner-root integration is described above;
  these palette checks alone do not establish complete customization parity.

- Newly loaded inventory models now share one pending atlas rebuild before drawing,
  instead of rebuilding and uploading the font/icon atlas for every new item.
  Explicit layout updates also clear the pending work. A late-resource owner-tree
  regression, all five targets, the full CPU/owner suite,40 world/16 GUI Vulkan
  cases and water/Alpha checks pass. A hidden read-only recipe capture from the
  optimized binary renders the result/ingredient normally and leaves the save
  unchanged. This is a resource scheduling change, with the same item draw rules.

- Formula detail results and ingredients now use original-record WORLD images;
  hovered item information uses a class-adapted copy. Their separate callback,
  pivot/frame geometry, rotation, dark-pass and depth rules follow the independent
  specification. Missing result resources skip the detail hook without advancing
  its local angle, and hidden draws do not accumulate catch-up. GUI timing now
  narrows each capped elapsed step to float then signed milliseconds and wraps
  its accumulated total across title/game frames. Ingredient hover/navigation
  share the same geometry. The full CPU/owner suite, all five targets,40 world
  and16 GUI Vulkan cases, shared water and Alpha ordering pass. Other icon callers,
  GUI layers and persistent widget refresh state remain open. Hidden, read-only
  native-save captures at GUI scale1 render the recipe result/ingredient and hover
  preview, stop normally and leave the fixture unchanged. Capture executables
  match the validated build; these are not original-game pixel comparisons.

- Inventory, shop and recipe cells now render their original records through the
  shared WORLD item helper during the owner's InventoryWidget callback, after its
  child frames. Physical inventory slots are retained rather than packed; holes,
  per-viewport region cutoffs and full-tab scrollbar counts remain distinct.
  Screen snapping truncates toward zero, hover uses half-open rectangles, and
  icons retain native signed scale, category sizing, GUI spin and spirit data.
  Labels use native class/color and mode-specific quantity admission. Resource
  loads are shared within a session. Formula details, hovered previews and other
  callers still need conversion; full GUI layers and refresh timing remain open.
  All five targets build; the full CPU/owner suite, 40 world and 16 GUI Vulkan
  cases, shared water and Alpha ordering pass. A hidden, read-only native-save
  capture renders the owner grid model and stops normally with the fixture
  unchanged. These checks do not establish complete native visual parity.

- Ring Menu now uses canonical category1 slots, retained signed selection and native
  neutral/repeat/animation state. Selected Use precedes independent Quick Item; the later
  toggle and selection update preserve same-frame ordering and pre-repeat labels.
  Bombs and portal stones use the existing shared item helper, including borrowed shrine
  selection without portal depletion. The owner selector artwork, Display-based arrow
  feedback and selected Quick Item3D SpriteWidget are connected. Ring items use their
  distinct queued50-degree WORLD path, full-header dimensions, world-clock spin and
  retained depth before Plasma, without direct-widget black/spirit passes. Combined UI
  open/close sounds are sampled once after visibility changes. CPU/owner-tree checks,
  all five targets,40 world and16 GUI Vulkan image cases, shared water and Alpha ordering
  pass. Original-game/hardware comparisons and other widget callers remain open.
  Location-banner aging now uses capped UI elapsed time rather than simulation substeps.
  Read-only native-save Ring captures at GUI scales1/0.4096 render three owner items;
  the final capture executable matches the validated build and the fixture save is unchanged.

- Native equipment slots now render original item records through the shared3D WORLD
  helper at each SpriteWidget's owner-tree callback. The copied binding, full-header
  sizing, signed GUI spin, dark/main passes and spirit attributes remain intact;
  child labels/frame art follow the item. Hover does not enlarge this caller.
  Record-less legacy sessions retain compatibility images. Inventory grids,
  crafting and other item-preview callers still need conversion; selected Quick Item
  is covered by the later Ring Menu integration above.
  Headless owner-tree checks cover record preservation, draw order, empty/missing
  resources and visibility. All five targets, the full CPU/owner-data suite,
  40 world and13 GUI Vulkan image checks, shared water and Alpha ordering pass.

- Character-menu controls now initialize and animate the owner's ordinary Button sequences
  instead of showing the saved gray idle pose. Select uses live effective frames and20 physical
  pixels of padding. Card binding is transferred from the copied template; horizontal centering,
  ordered vertical fitting, filtered left-drag and momentum follow the independent specification.
  Pointer polling and frame decay retain separate clocks. CPU state/owner-data checks cover
  these paths. Cached GUI masking/shine and preview creature/world rendering remain open;
  these changes are not a claim of complete menu parity.

- Identification now renders the original selected item through the shared 3D WORLD
  shader path, using owner-extracted CUB geometry and full header dimensions. Native
  projection, rotation, main/dark passes, shininess and weapon spirit cubes are
  integrated with immediate UI ordering and retained depth. CPU checks and 13 actual
  Vulkan image checks pass, including TAA/SMAA, shadows and both lighting presets.
  Read-only native-save captures at GUI scales1/1.5 render the selected owner sprite
  and stop normally with the fixture save unchanged.
  Other widget callers still need conversion from bitmap previews; full inherited
  callback state, offscreen GUI layers and original-game visual comparison remain open.

- Shrine-choice map mode now follows item and creature use, restricts hover to shrine
  targets and accepts the preceding actual draw's target on eligible left release.
  It checks known cells with negative-coordinate floor division, starts the delayed
  native return ability, and preserves item quantity. Deferred mode/source cleanup,
  float world-clock pulses and GUI-scale label sizing are integrated. CPU/owner-data
  checks pass; full original-game/controller comparison remains a release gate.
- Character Select/Create and delete-control placement now uses the owner's effective widget
  frames after resizing, rather than treating the requested sizes as final dimensions. This fixes
  Select extending below its character card. Owner-shape picking replaces the old rectangular
  checks. Owner-data regressions cover UI scales 1 and 1.5; broader menu visual parity remains open.
- Native-save gameplay now uses canonical inventory tabs, equipment and counters.
  Screens derive live slot views and the save bridge writes those actual records.
  Direct equipment/use/drop, pickups, quick items, crafting, cube upgrades, shops,
  identification, pet food and daily reset use their specified distinct operations.
  Shop regressions check transaction data, separate
  sale nodes, whole-node buyback attempts and ordered refresh callbacks. Health
  adjustment stores its rescale before the second maximum-health callback/cap.
  All five build targets, focused inventory/save/loot-window checks and the full
  CPU/owner-data suite pass. A hidden read-only native-character run rendered
  the inventory and stopped normally. Native identification uses the owner's
  blackwidget/itemframe tree, native text/footer positions, live selection and
  release gates. Transformed edge/navigation tests and the CPU/owner suite pass;
  read-only captures at GUI scales1/1.5 exercise the owner artwork and typography.
  The remaining shared3D icon callers still use bitmap compatibility previews; full
  map-panel/capture behavior and extended owner validation remain open. Ring Menu
  and selected Quick Item are covered by the later integration above.
- Native adaptive scene range is integrated: actual unmerged face costs/readiness,
  option-derived window, native equal-key sort, budget threshold, easing and player
  state feed distinct terrain/model/actor/light ranges. Pending cache metadata is
  separate from active GPU geometry and is reused coherently for culling and ground
  lights. Missing legacy cost evidence is not replaced by a guessed triangle count.
  All12 sampled clean native min/max/center/radius and weighted costs match; the full CPU/owner-data suite and
  40 actual Vulkan image checks pass. Native radius15 also completed a hidden no-save
  settled-world capture (sampled private peak1,687MiB). Camera, inherited shader/pass
  inputs, readiness-opacity consumption and wider native visuals remain open.

- NPC speech/emotes now use owner templates and models, with per-speaker playback,
  repeat-Use advancement, exact capped clocks, cached projection and native emote
  pose/depth/culling rules. Lore and receipt scripts append to the general panel;
  final-page lore activation preserves its distinct sound rule. Category24 item
  descriptions receive the item's region context and verified description phrase.
  The full CPU/owner-data suite, 33 actual Vulkan image checks and a hidden no-save
  live speech capture pass. `tools/validate_engine.ps1` runs the full suite followed
  by actual `steamshader` rendering; `steamvoxel` is a CPU data test. These checks
  do not establish complete native visual parity. The other implementation/system
  checks below still need work.
- Saved recipe records now reach the crafting UI through the native progress bridge.
  Missing startup defaults use full item identity; unlisted records remain preserved,
  and crafted results retain formula and all128 upgrade bytes. Full CPU/owner-data
  tests pass, including lifecycle/default-merge and attribute/padding regressions.
  This batch changes no shader path; Vulkan image checks remain the33 passing cases
  from the preceding dialogue checkpoint. No book receipt or decoder-only opcode
  was turned into an invented recipe award.

- Live lighting now reads personal used-cell state, kind3/4 projectiles and
  glowing ground items from gameplay. It preserves native ownership, timer,
  quantity, coordinate conversion and fade rules. Model and terrain light ranges
  are separate. Hit flashes now use consumed local/received hit records, current
  spirit weapons and native fixed positions, first-frame aging and expiration.
  CPU regressions and 28 Vulkan cases cover these sources, including zero-fade
  slots, personal state updates, projectile filtering/removal and owned/delayed/
  picked-up items. The race0x90 death flash is also integrated, using the persistent
  pose-preparation marker, drawn position, cached previous renderer visibility,
  fixed 100-block range and native candidate ordering/800-ms lifetime. Death births
  precede hit-display flashes. Further CPU/GPU checks cover these gates and cached
  visibility across pending origin changes. Live player/NPC records and buffs now
  supply carried/toggled lamps, race/state effects, casting and equipment spirit
  glow, with native anchor/rotation/truncation and source ordering. The casting
  sphere range has its separate native zone-window calculation and 256 cap.
  Native mesh-budget/adaptive range inputs are now connected as described above.
  Camera state, shader/pass inheritance and broader native visual comparisons remain
  open. Source-branch tests do not prove complete frame-input parity.

- Isolated reference capture now runs automatically after game-session
  initialization and refreshes cached light snapshots. The real-install run
  captured both auxiliary lists in 1,590 zones; all 8 model and 9 static lights
  match the engine reader, including mesh-time light samples. All 3,823 terrain
  reference points across 25 fields and 4,847 block samples match. The two region
  failures came from the earlier BigRegions-modified reference. The full suite
  passes, and original Save, Mods and ReShade were restored byte-for-byte.
  These sampled comparisons do not establish full visual parity.

- Pyro regional difficulty now uses the first entrant's level while a region has
  occupants. The host shares it in multiplayer; single player uses the same path.
  The last departure/disconnect clears it, so re-entry chooses a fresh level.
  Permanent discovery difficulty locks are ignored. Existing biome/id variation
  and gear-level records remain. Real two-session UDP tests cover publication,
  retained occupancy, last departure and refreshed re-entry.
- Steam model/beam visibility now uses the mesh job's native column/face prepass,
  published-geometry readiness, five-plane containing-zone test, stored center,
  exact early fixed-distance arithmetic and raw six-plane model-sphere test.
  Native CPU coordinates retain unshifted game Z. Missing neighbor data retains a
  conservative fallback. The camera lens/distance settings remain configurable;
  adaptive inputs are integrated above; native camera-state matching remains open.
  A versioned owner-export record captures coherent bounds/counts/triangle cost
  for future comparisons. Synthetic/file-format regressions and no-save Vulkan
  captures pass. The later12-cache oracle above matches fresh native boxes/costs;
  no complete pixel match is claimed.
- Native Steam front/back/ambient RGBA targets and state smoothing now follow
  the separate author's completed specification. Steam world RGB uses the native
  targets, with optional moon additions kept separate. Tests cover ordered regional/
  structure blends, effect13, low/high day interpolation, constructor values and
  exact native coefficient words. The frame input uses the verified1000-ms wall-time
  cap before float conversion; rendering reads the current client clock/buff state.
  Day/night no-save GPU captures pass. Native ambient alpha is retained in CPU state,
  but its shader saturation/composition stage is still pending. Point registration,
  sky/fog special overrides and remaining shader/color behavior still need work.
- Runtime follow-up: incoming local-player hit rules, native music smoothing,
  gameplay XInput mapping/sticks/camera, page-scoped dialogue marks and daily world/
  character reset behavior now follow the independent runtime-gap specification.
  The complete EngineTests run passed, including the new focused regressions and
  owner-data comparisons. String-script answer activation, inn payments, persisted
  specialization changes and the general feedback panel are now integrated, with
  native word colors, row picking, navigation points and distinct UI action gates.
  Focused tests include the owner's fonts/templates at unit and 1.5 GUI scale.
  Broader GUI controller coverage and the other roadmap
  gaps remain open. The subsequent equipped-pet update pass
  also builds and passes the full suite, with focused creation, naming, health,
  inheritance, authority, travel and detachment regressions.
- Follow-up timing: music receives double-precision elapsed frame time through the
  audio API with the native 1000-ms cap. The title root activates on mouse release,
  correcting the older spec's description of the 0x965e0 input callback.
- Enemy lifecycle: configurable Timed/Instance/Permanent modes. Timed defaults to
  300 seconds; Instance resets on spawn-zone unload for all players. Temporary
  deaths do not enter permanent saves, and mirrors handle repeated deaths.
  Synthetic and owner-data generated-world integration checks cover these paths.

- Water: implemented the written Steam surface curve, colour/depth passes, full-height
  exposed water faces, wet-ground films and underwater tint. Alpha dynamic models
  follow water. Default-on lilies use owner assets as independent render decoration.
  See [water progress](water-progress.md) for checks and remaining lighting/order gaps.
- HP above maximum: fixed native-player initialization and equipment updates at the
  record boundary. The regression preserves wounds between frames. Updated GPU
  captures show 136/136 where the earlier capture showed 169/136.
- Statue slab: compared 81 columns against owner-exported blocks. All 477 stored
  blocks within the slab height range match; the generator was left unchanged.
- Character selection: reuses the owner's rounded `gui.plx` frame, blended behind
  previews so it does not tint them. The owner's alpha is retained without the earlier extra
  0.6 multiplier; a full composition match and the logo overlay's native light are not established.
- Packaging: a separate allowlisted tree and ZIP include required source tables,
  specifications, config templates, project art and dependency notices. Game data,
  screenshots, recordings, saves, SDK checkouts, private handoffs and binaries are
  excluded. Local research is preserved outside the upload tree.
- Source license: the project owner selected GPL version 3 for original engine and
  extractor source (`GPL-3.0-only`). The full text is in the root LICENSE; existing
  third-party licenses and notices remain separate.

## Validation

The development build and full EngineTests suite passed, including synthetic water,
lily and HP checks, owner-data comparisons and the setup wizard's simulated-install
tests. Steam and Alpha Vulkan captures completed on an RX 6600 with saving disabled;
the updated character-select screen was inspected. These cover the tested cases.
The staged source package built without compiler warnings in a separate CMake output
directory using cached dependency sources, and its full EngineTests run passed.
It contains no dependency build products or game exports. This is a source-completeness
check on the development machine, not a fresh-machine installation test.

The latest isolated original-game recapture refreshed generation and lighting
references. Its journaled restoration was verified against the original Save, Mods
and ReShade DLL file hashes. Captures remain private and excluded from the source archive.

## Work still open

- Alpha hybrid: wider seed/biome/domain comparisons, construction-transition and
  forest-edge review, live edits, cold streaming and original cache/input lifetime.
  The implemented default and bounded numeric fixes are documented in
  [Alpha generation](alpha-generation.md); they do not establish full native parity.
- Water: Steam vertex occlusion/point-light parity, Alpha near/far cache/input
  correspondence, and wider waterfall, underwater, wet-ground, dusk/night and
  material comparisons. The independently specified Alpha threshold arithmetic
  and staged draw order have CPU/GPU checks.
- Frontend: wider owner-window visual comparisons and the optional recording-only
  preview compatibility path. Normal saved cards, creation and Options already use
  live owner blackwidget copies, authored fill/Display values and ordinary Plasma
  blending; the independent panel audit confirms that no universal opacity constant
  belongs here. Logo placement, queued projection and fixed WORLD lighting likewise
  have independent specifications and CPU/GPU checks. Full native screenshots and
  unmeasured font/cache/owner hierarchy cases remain separate acceptance work.
- Gameplay edges: sound coverage and feat/chat presentation, ability/AI cases,
  integrated pet combat/appearance comparisons, controller GUI/aim tracking,
  newly observed item-action producers and wider gameplay comparisons,
  and network feat production. See the [audited roadmap](roadmap.md). Recorded
  comparisons do not prove complete gameplay parity.
- Human/system checks: two Steam accounts and an original-game peer, a real first-run
  wizard session, a fresh-machine install and extended save/load gameplay.
- Publication: complete provenance/rights review, including the unresolved original
  CWSDK revision/permission scope for the SDK-dependent extractor. The retained
  Gijs MIT notice matches CubeMegaMod's project notice; it does not establish that
  separate SDK coverage. See [SDK provenance](sdk-license-provenance.md).
  The original-source license choice is complete.
  See [PUBLICATION.md](../PUBLICATION.md). Existing third-party licenses remain separate.

Do not upload the whole development checkout. Review the fresh
`build/CubeVulkan-upload-<timestamp>/` tree produced by `tools/package_source.ps1`,
then upload its contents after the publication decisions above.
