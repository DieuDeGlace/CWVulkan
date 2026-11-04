# Engine readiness checklist

Audited against the source and tests on 2026-11-02; the Alpha section was updated
against its implementation and checks on 2026-11-03. The checkboxes in the first
section mean implemented and exercised in the stated checks; they do not certify
complete Steam or pixel parity. Older progress notes are in Git history and the
individual feature documents.

## Implemented

- [x] Align climbing upkeep's special-item ownership with the independently verified
  signed32 quantity accumulator. Actual upkeep distinguishes wrapped-overflow
  unavailability from a positive signed total before/after the correction. Restore
  ordinary unequip's0x5b completion sound after its refresh. All five targets and
  CPU/owner checks pass; broader widget/crafting and native audio acceptance remain.

- [x] Replace the first-carried-vehicle stand-in with the independently specified
  World-step derived item. Exact nearby-object priority, regional full-item equality,
  signed32 quantities, cancellation-before-duration and boat/glider flag/block
  checks now drive the retained cache before ordinary creature traversal. Use
  consumes the synthetic zero-region value; direct inventory use keeps its original
  regional value. Shared semantic assignment preserves held/copy item padding.
  Fresh-character reset initializes both derived CharacterData caches. Pure and
  actual zero-time World/Use regressions, all five targets and CPU/owner checks pass.
  Unknown missing-zone fallback flags defer new travel selections as explicit
  replacement policy; wider native runtime/alternate-writer acceptance stays open.

- [x] Automatic abilities 0x96/0x97 now update a controller-owned creature-copy
  collection at the effect threshold, after prompt suppression/Climb and before
  Revive. Copies retain constructor/prior values outside the specified semantic
  fields; duplicate IDs update the same owned object. Signed-ID ordering, wrapped
  selection, reset/sound/selection order, missing targets and raw zero-ID lookup
  have direct and actual-prompt regressions. All five targets and CPU/owner checks
  pass. The owner spans world/menu sessions; save/transition mutation rules, other
  collection consumers and original-game runtime acceptance remain unverified.

- [x] Independently verify proper-name selector dispatch, 32-bit hash inputs and
  unsigned index reductions; share the existing R implementation and retain raw
  selector bits across table aliases. Derived index/extreme-selector fixtures pass.
  Replace fragment instruction scanning with guarded literal-data reads from the
  complete 380-entry RVA/length manifest. Legacy output matches every length and
  repeated-address alias; source metadata contains no vocabulary. All five targets
  and CPU/owner checks pass. New extractor live execution, wider original-name
  output and caller/grammar acceptance remain separate.


- [x] Full item naming starts from a shared complete semantic copy, preserving all
  attribute/count/padding bytes and converting the four special gear stand-ins,
  including recursive formulas. Unlisted material bytes use the generic Material
  key. Independent material/quality/unused-tally facts are documented; synthetic
  regressions reproduce five normalization and three fallback failures before
  their respective fixes and pass afterward, with all five targets/full CPU-owner
  checks. This does not certify family-specific proper-name data, every dictionary
  failure path or original-game visual/name-output parity.


- [x] Steam map-region pixel stages share the full WORLD color/fog implementation,
  including base-two exponent, Fog/SkySun fourth-power blend and inherited raw
  sky/sun/gradient inputs. Synthetic far/partial/sun cases pass alongside the full
  world/GUI suites. Engine identity grouping/FIFO and unavailable-buffer checks
  also pass. WORLD/VIEW upload separation, distinct lighting/view normals and
  typed material/effect controls now have17 map image cases. Native address/alias
  inputs, marker-specific producers and wider native visual comparisons remain
  separate work.
- [x] Optional engine-owned dual-tone hair controls in character creation only.
  Hover/commit, second color, start/blend, local model/map rendering and profile
  metadata are tested. Native appearance bytes remain unchanged. Remote peers
  do not yet receive the extra style; this extension does not establish UI parity.
- [x] Clean-room generator: terrain, structures, placements, items, residents and
  encounters in `src/worldgen/`, from the written specifications. Owner-exported
  terrain/structure/creature comparisons run in EngineTests when available.
- [x] Creature controller, physics, AI, upkeep, abilities, hits and poses in the
  `Native*` modules. Recorded comparisons cover the supplied samples; edge cases
  and the gaps below remain.
- [x] Item generation and death drops, ground pick-up and coin hop, shops and
  crafting. `GameShop`, `GameCraft`, `GroundItems` and `NativeDrop` implement the
  written rules. Coins are picked up near the player rather than flying to them.
- [x] Identification and cube-editing backends: `IdentifyWindow`, `UpgradeWidget`,
  the main-loop handlers and `LootWindowTests` exercise their implemented rules.
  Native-save customization now uses the owner editor root and raw WORLD models;
  remaining cached GUI/input comparisons are detailed below. Recording-only
  sessions retain their compatibility picture.
- [x] Generated conversations, regional mission checks, lore/artifact data, map
  markers and interactions. `TalkGen`, `TalkScripts`, `Missions`, `Lore`, `MapMarks`
  and `UsePrompt` are wired into play. Remaining script/network gaps are below.
- [x] Native character and world save formats, read-only import, zone changes and
  Continue restoring the saved position. `NativeSaves`, `NativeProgress`,
  `ZoneStates` and SaveTests cover the supported format paths.
- [x] Eagle title flyover, character select/create/delete, options, binding editor
  and the system menu. `Frontend`, `FrontendRun`, `GameOptions` and FrontendTests
  cover their implemented rules. Select/Create and delete controls use live owner widget
  frames, physical-pixel padding and copied-shape picking. Ordinary Button release/idle/hover
  sequences retain per-instance values. Character cards preserve transferred template binding,
  center with native integer fitting and use filtered left-drag/momentum with separate clocks.
  Scale0.4096/1/1.5 and synthetic state regressions cover these paths. Native saved-character
  cards now pose their own persistent records and render owner sprites at the live widget
  callback, independently of class recordings. Integer GUI hover/walk updates, fractional
  renderer pose steps, zero-origin cache lifetime,50-degree projection, fixed WORLD palette,
  material/effect values and opposite winding have CPU/GPU checks. Main-target captures
  show the native portrait. Float overlay RGBA, destination-alpha blending,
  independent own-shape fill and glyph tint now have actual owner-HUD and Vulkan
  regressions. Ordinary affine scene-graph clip layers now use a retained RGBA8
  pool, distinct mask modes, null-depth child WORLD callbacks and restored parent
  targets; the owner button shine is enabled.102 local GUI image cases (86
  asset-free,16 with the owner's atlas) and owner callback
  checks pass. Normal selection cards, creation and Options copy the owner's
  blackwidget art and live Display values, with white non-text tint and no shared
  fixed-alpha rectangle; the [independent panel audit](specs/steam-frontend-panel-backgrounds.md)
  confirms those inputs. Logo placement/projection and fixed WORLD shading also
  have independent contracts and real CUB GPU cases in both looks. Native visual
  comparison and the recording-only preview fallback remain separate work.
  The legacy placed HUD now uses the same real clip layers and
  samples shape, Transform and Display sequences through the existing evaluator,
  preserving independent placement/fill overrides and disabled-node admission.
  Owner location masks and synthetic movement/visibility/clipping channels pass
  at two GUI scales; the former shape-only sampling fails those checks.
  Ordinary geometry now retains homogeneous XYZW through the pixel bias,
  native GUI depth projection and perspective interpolation; drawing deformation
  retains bind/inverse-bind W separately from divided CPU picking. Synthetic
  owner/descendant geometry and Vulkan color/mask/depth checks pass, while the
  flattened shader fails four image assertions. The concrete blur path's cleared
  final source and post-composite own hooks now use offscreen child traversal;
  authored child visibility is preserved. Small/large-radius, nested-clip and
  retained-mask GPU checks pass; omitting the final clear fails two checks.
  The current clamp-sampler renderer omits pixel-dead intermediate/final copies;
  this does not replay inherited native pipeline state or certify altered samplers.
  Default Plasma depth testing is disabled and optional depth-stack mask0x08 starts
  off, per the independent depth contract. General Widget/node caches, broader blur/callback
  state comparisons, optional depth-stack behavior and frontend pixel parity remain open.
  The title logo now uses the independently specified finite50-degree queued projection,
  full-allocation centering, exact clock/scale staging and normal WORLD overlay lighting.
  It precedes every Plasma cut and retains scene depth/sky rather than using baked shades.
  Shared logo/item CUB meshes now select packed RGB/V shading explicitly. Six asset-free
  Vulkan cases cover color, before-Plasma order and retained depth in both world styles;
  frontend CPU checks cover perspective and effect/palette reset. Immediate text's verified
  request remains resource1.dat; cached/named-memory font-byte provenance and unmodified
  visual comparisons remain open, so no resource2 substitution is inferred from modded captures.
  The former catch-all font1 mapping now uses a file-backed TTF/DAT basename cache,
  requested-path/directory precedence and TextShape-only Arial fallback. Inline
  document callbacks retain startup order and complete-shape request timing. Owner-face
  tests cover first-face retention, directory priority, failed-request retry, fallback
  isolation and parser timing; no font bytes enter the test source. Named-memory
  sources and nested vector-font documents remain outside this bounded implementation.
  The independent cache-startup supplement also establishes the ordinary engine-byte
  constructor/teardown gates. The concrete cached ChatWidget now has a retained RGBA8
  target, rounded live frame queries, null-depth redraw and ordinary surface-copy
  composition. Explicit append/key/Enter-close notifications and interface editing
  updates control repaint; stable inactive text skips its drawing callback. Surface
  creation/resize retains its separate next-step dirty indication. Synthetic Vulkan
  cases cover quantized alpha, cross-frame reuse, moved copies and depth attachment;
  actual owner-HUD cases cover initial/extra repaint, callback suppression and input
  invalidation. General node caching and arbitrary cache/clip nesting remain open.
- [x] Host/client worlds, single-player loopback, UDP, friends/invites, Steam
  transport and dedicated server. The recorded wire comparisons and local
  multi-process runs pass for the checked cases; Steam peer validation is pending.
- [x] Chat entry and send/receive handling: `ChatLine` and its main-loop
  consumers. The old "chat entry line missing" item was stale.
- [x] Owner setup wizard, data validation, journal/restore and interrupted-run
  tests against a simulated installation. The real-install checks remain below.
- [x] Alpha/Steam sky styles, clouds, stars, plant sway, map rendering, HUD coin,
  sound-table import and native GUI/font loading from owner data. The owner-backed
  coin and its black shadow now use the queued WORLD shader, full CUB header
  centering,50-degree lens and configured zone-window far plane. World-clock
  rotation, resolved Glow0/Shininess1, separate shader depth offsets and placement
  are preserved;
  CPU-baked colors and per-turn mesh variants are removed. Mesh handles are
  scoped to the play session rather than retained across renderer recreation.
  Ten asset-free image cases cover both appearance styles, source-mesh admission,
  the offset shadow,
  Plasma ordering and retained scene depth. The adjacent compass now loads the
  owner's compass2 CUB, uses displayed float yaw without wrapping, native physical
  placement/scale and Glow1/Shininess0. Six additional image cases cover both styles.
  A positive-Shininess coin case also caught/fixed the missing common camera gradient
  in Alpha-mode native WORLD callbacks. Coin rotation now uses the canonical
  float simulation clock shared by Ring/map producers. Wider native HUD comparisons remain.
- [x] Context/special-item HUD model producers from independent
  [context lifecycle](specs/steam-hud-context-item-lifecycle.md) and
  [overlay](specs/steam-hud-model-overlays.md) contracts. Each admitted World
  substep, including zero time, derives the last eligible nearby record in the
  loaded3×3 walk, then checks the category-selected inventory tab. Semantic copies,
  signed quantity overflow/rejection and the count>1 label preserve that snapshot.
  The canonical tab1 special row keeps physical order/duplicates, region/subtype
  filters and integer positions. Owner-backed model pairs use full allocations,
  native scale/clock, typed effects and the shared queued WORLD path. Synthetic
  lifecycle/selection checks, actual World zero-step integration, eight row image
  cases and actual HUD resource/count emission pass. Full native pixel comparisons,
  original-resource identity/alias comparisons and general cached text lifetimes remain open.
- [x] Shared root HUD screen-queue grouping by engine resource identity, FIFO for
  equal resources, before Plasma callbacks. Interior layer/cache cuts and later
  widget/faded streams retain their explicit order. CPU boundaries and four
  overlapping-model GPU cases pass in both appearance styles. Renderer overlay
  vectors now accept ownership from callers that move their prepared streams.
  Native pointer-number/alias correspondence still requires wider comparison.
  Gold display now distinguishes absent data from signed values; runtime negative
  amounts render normally, with the native save-reader quirk unchanged. Actual
  owner-HUD glyph emission checks positive/negative/absent values.
- [x] Water surface/mesh/depth changes and render-only lilies; HP initialization
  capped at the native maximum; statue slab compared with exported blocks.
  See [water progress](water-progress.md) for the remaining rendering limits.
- [x] Configurable enemy deaths: `Timed` (300 seconds by default), `Instance`
  (until the original spawn zone unloads for all players), and `Permanent`.
  Temporary deaths are excluded from permanent saves. Tests cover actual generated
  enemy return, old death recovery and repeated deaths in client mirrors.
- [x] Optional first-entrant regional difficulty in Pyro mode. Host-authoritative
  active baselines refresh after the last occupant leaves; no permanent discovery
  lock. Actual two-session UDP tests cover synchronization and refreshed re-entry.
- [x] Source-only GitHub packaging, file hashes, dependency notices and GPL-3.0-only
  for original code. Game content, recordings, native saves and binaries are excluded.
- [x] Incoming local-player hit arithmetic: blocking, mana shields, healing and
  lethal boundaries, from the independent runtime-gap spec and PlayerHitTests.
- [x] Native music fade coefficients and frame smoothing, including ducking,
  ordinary recovery and urgent fades. SoundRules tests cover these branches.
- [x] Gameplay XInput mappings, radial stick filtering, cubic camera input and the
  B-after-menu jump latch. FrontendTests cover thresholds and the context gates.
- [x] Dialogue page grouping and per-page mission marks. DialoguePageTests cover
  deferred breaks, dictionary breaks, percentages, choice separation and emotes.
  Native playback and owner-template checks are described below.
- [x] String-script answer selection, native inn/specialization actions and saved
  specialization changes. Labels retain dictionary word colors and native glue;
  owner widget/font tests cover row picking, sizing and controller points. The
  general feedback panel follows native reveal, release, voice and indicator rules.
  Answer-visible action gates preserve movement, jump and earlier ultimate input;
  focused DialogueChoicesTests cover the distinction. Broader live visual comparisons
  remain open.
- [x] Authoritative day rollover, loaded-zone refresh and daily character data
  reset. Ground/block lists remain loaded; elapsed respawn timers survive resets.
  Synthetic and generated-world tests cover the clock boundary and timer behavior;
  shopper/static-object comparisons still need broader owner recordings.
- [x] Equipped-pet reconciliation, creation, race/name changes, health fractions,
  owner equipment/buff inheritance and detach rules in `NativePet`. PetTests cover
  dead-link repair, mirror authority, travel gates and the 3000-ms glide boundary.

## Genuine implementation gaps

- [ ] Strict Steam world/dungeon parity is a release requirement: region/theme
  generation, dungeon/building layouts and object placement/transforms must match
  the owner's reference inputs. Implemented generation is not a certification of
  every region or seed. The isolated 2026-10-31 recapture matches all 3,823 sampled
  points across 25 terrain fields, 4,847 block samples and 60,000 cell objects. It
  resolves the two modified-reference failures without changing generation; the
  full CPU/owner-data suite passes. Its 17 nonempty zones match 8 model and 11 static
  light records with complete neighborhoods. These samples do not close the broader
  placement/theme and runtime comparisons below.
  The expanded 2026-10-29 audit covers 789 zone snapshots
  across four overlapping seed-123 sets; it still finds object-record differences.
  `WC_STRICT_RECORDS=1 EngineTests wgcreatures 10000` now fails on object keys,
  model counts/positions or item keys/angles that differ, and on absent zone fixtures.
  Defined model fields and list order now match all 50,116 records in those snapshots:
  type, position, size/angle, tint and flags. Only independently verified unused,
  unwritten marker-size fields are excluded and reported separately. The loader now
  draws only live model/object lists, retains full captured turn bytes, and derives
  object scale from type extents and owner voxel dimensions. Captured door/window
  pivots and sliding extents have corner regressions. The runtime continuation now
  advances object clocks in the native player-zone/substep pass, reads live poses,
  renders the generated 0x59 quad, bakes model sunlight from current neighboring
  terrain, maps native flag 1 glow, and orders model fades around water. Castle
  room choices/floor increments now use their documented unsigned remainders.
  Full structure/light-list checks pass on all 789 snapshots; fresh-generation RNG
  checks are distinct from partial daily-refresh traces. Mesh-prepass bounds,
  containing-zone/model gates and coherent cache-oracle capture are implemented.
  Native directional palette targets/RGBA easing have numeric checks, including
  regional/structure/effect13 blends and capped wall timing. Steam sky/fog/sun now
  retain full RGBA, with constructor values, stateful first updates and corrected
  ordinary/interior transition rates from the independent sky-state supplement.
  Place-27 alpha, selected far-flight keys and post-water effect13 are connected.
  CPU checks cover alpha, strict flight thresholds and retained frame state.
  Directional lighting also survives clock rollback and scene-range reset, with
  actual rendered-image regressions after a positive-duration transition.
  Steam voxel inputs now use weighted front-layer occlusion/light, encoded CUB
  colors and native face corners/diagonals. Terrain reuses one retained light solve
  for mesh corners and models; lava has its own opaque palette group. Water films,
  the optimized sea quad and post-solve air-byte tests have fixture regressions.
  The full suite and Vulkan smoke pass; the 300-zone terrain-only light oracle
  matches all 482,880 eligible columns and 1,541,581 compared blocks.
  Ordinary Steam geometry now lights/saturates/clamps per vertex before raster
  interpolation, with separate ambient alpha, native sun direction and material
  sunlight factors. The world pixel path retains saturation for Glow, reconstructs
  fog/sky from raw palettes and interpolated clip/view coordinates, and grades
  only after the native two-stage fog blend. Synthetic Vulkan checks match the
  three derived RGB references and full-fog endpoints; day/night and Alpha
  smoke captures pass. GPU world clip planes now match the native CPU gate.
  Auxiliary model/static-object light registration now follows the source gates,
  flicker, camera ranking and per-zone16/10 limits. Vulkan uses per-frame storage
  batches, so a global64-light cap cannot discard another zone's lights. Sprite
  bindings use full CUB dimensions, posed matrices and native coordinate rounding;
  loaded neighboring zones participate before they have meshes. Source-reader,
  CPU boundary and 28 synthetic GPU checks pass, including offset70 and more than
  64 candidates. The isolated recapture refreshed1,590 zones with both auxiliary
  light lists and coherent cache metadata. The engine reader and neighboring-zone
  light solve match all 8 model and 9 static lights in 15 nonempty zones.
  Both region-oracle failures are resolved using that vanilla reference: all
  3,823 points across 25 terrain fields and 4,847 block samples match. The earlier
  reference had BigRegions Scale2 enabled. Generation code and expected values
  were unchanged; the full owner-data suite passes. Snapshot-only characters
  are reported as lacking attack recordings; supplied broken clips still fail.
  Personal object lights now use the local character's live used-cell state,
  including native signed32 block narrowing before cell flooring. Live kind3/4
  projectile lights and glowing ground items are also connected. Ground sources
  preserve ownership, zero-timer/quantity gates, strict cached-center admission,
  Z+3 emission, native fade and pickup changes. Model and terrain light ranges
  remain separate. Synthetic Vulkan checks cover state changes without remeshing,
  projectile kinds/removal and item ownership/delay/pickup; packet-backed CPU
  checks verify the live-item adapter. These do not establish native pixel parity.
  Adaptive ranges, pending cache publication and the listed live-light branches
  are integrated below. All12 sampled clean native cache bounds/centers/radii and
  weighted costs match. Native camera/pass/shader state, undefined consumed lamp
  turns and broader dungeon/theme oracles still prevent a complete parity claim.
- [ ] Remaining ability/AI cases. Broader recordings are needed beyond the samples
  currently supplied, including integrated pet combat and appearance comparisons.
  Target candidates now retain engine World registration order across record updates
  and storage moves, including the local player at its actual registration point;
  removal/recreation appends at the tail. Full native list-mutation producer coverage
  and remaining World consumers' visit ordering still need validation/integration.
  The independent [World visitation](specs/steam-world-creature-visitation.md)
  contract now drives registered body ordering and separate global state clearing,
  including zero-time/dead entries and synchronous body-pass births. Native
  [interpolation](specs/steam-creature-drawn-interpolation.md) and
  [linked placement](specs/steam-creature-linked-placement.md) run as two complete
  drawn passes before input, without late visible-draw snaps. Row, threshold,
  packet and actual-facade ordering regressions pass. AI collection now shares
  registered order; host interpolation runs once after its complete batch. The
  [daily-reset contract](specs/steam-world-daily-reset.md) drives reset-before-effects,
  repeated notice handling and fresh actor views after deletion. Focused tests cover
  retained guests, compacted storage, outgoing events and configured respawn policy.
  Resident collection now snapshots kind0 permissions before refresh, reads current
  player positions, and suppresses IDs from all registered actors. Its final flag
  reconciliation includes local/network/dead/new entries; copied field widths and
  coordinate conversions have dedicated boundary checks. Runtime zone-pointer
  order keeps each zone's refresh/admission/effect phases together, with retained
  identities across metadata refresh. Loaded-only block queries now track cache
  membership and reader lifetimes without retaining unloaded terrain. Full
  time/resident preludes, incoming CRT-state integration, broader mutation producers
  and native comparisons remain. The [host CRT audit](specs/steam-host-crt-stream.md)
  separates generator and simulation threads; the latter's initial numeric state is
  still unverified.
  The [object-trap prelude](specs/steam-world-traps.md) now runs per substep with
  common state clearing, loaded-zone identity order, per-object state/hit/sound
  ordering and native coordinate boundaries. Stable identities persist across
  block-storage copies; synthetic facade and full CPU checks pass. The separate
  [death-area/retirement pass](specs/steam-world-death-area.md) now integrates
  ordinary corpse retirement, owned-kind6 revival, water/blast areas and destructive
  terrain/model work. Bounded facade/lifecycle checks pass; broader prelude and
  native comparisons above remain open.
  The specified resident kind9 request/producer and independent live/output lifetimes
  now have native numeric and lifecycle checks; its broader rendering remains open.
- [ ] Finish controller GUI coverage and aim tracking. Navigation ranking, cursor
  repeats, click phases, root/system/Options/Controls points, inventory grids,
  equipment and recipe points, panel scrolling, tab rows and region filters are
  implemented from the independent spec, including answer rows and identification footer points.
  The shared pointer core now preserves cached coordinates, capture, independent
  virtual/registered gates, nested release and global visitor order. Native Widget
  ownership/enabled state, Button copy/group/press semantics and resize propagation
  have synthetic regressions; scrollbar sizing retains its loaded dynamic class.
  Binding that dispatcher to the complete live GUI hierarchy, remaining dynamic
  hooks, full camera/focus integration and hardware comparisons remain. Creature selection now runs once before early control;
  separate minimum depth, post-movement aim, early assistance, later speaker
  override and manual-stick slowdown have bounded stage regressions. Target
  candidate culling now uses the retained renderer, height-aware outer radius and
  separate inner radius/range. Strong charge uses that same filtered/sorted loop,
  with separately quantized integer scoring and retained ID. Registration order
  includes self before inner skip-self; full native lifecycle ordering remains open. Large region
  lists now follow the verified key-only insertion/partition/heap order, including
  signed32 distance wrapping. Tests cover the independently derived33-choice tie
  fixture, heap ties and finite-key ordering without inventing a stable tie-break.
- [x] NPC bubbles and emotes: per-speaker retained pages, native timeout/repeat-Use
  behavior, fixed-point smoothing/cached projection, owner speech frames and all five
  owner emote models. Emotes use the actor's culling sphere and ordinary world depth.
  Lore/receipt scripts append to the general panel; lore credit occurs on final-page
  activation with the distinct general/speaker sound gates. Region-bound special
  item descriptions and hold-up admission use the verified receipt rules. Full
  CPU/owner-data tests and 33 actual Vulkan image cases pass; a hidden no-save live
  frame confirms the speech template and colored prefixes. These are bounded checks.
- [x] Saved/default recipe sources: raw known records pass through native progress,
  including unlisted recipes. Startup appends only missing full-identity defaults;
  load repair ignores padding and inactive attribute bytes. Crafting results retain
  formula and all128 attribute bytes, while equality tests only active attributes.
  Inventory ingredient matching uses that same full identity. The CPU/owner-data
  suite passes with saved-list, default-merge, padding and result-retention regressions.
  Regional rank menus still use the verified solved-place predicate, without adding
  generated gear or crafting-book receipts to the saved recipe list. No new award
  producer was invented; numeric stream boundaries remain documented below.
- [ ] Network feat production/payload meaning. Its display consumer is traced;
  the [producer audit](specs/steam-feat-production-audit.md) found no verified award
  source. Generating awards remains unspecified. Guest mission marks are client-local,
  with host interaction notices; no invented quest-award packet is needed.
- [x] Native chat history/editor and entered commands: plain 400x250 owner-font
  widget at bottom right; colored word boxes, strict wrapping, retained rows,
  UTF-16 selection, global-clock caret, mouse dismissal and ordered callbacks.
  Incoming entries consume one per frame and resolve the sender's current name
  at that visit. New owner glyphs join the atlas rather than replacing received
  text with ASCII question marks. Commands use the verified six-token dispatch,
  strict motion clock and narrow pet-name writes. `/check` toggles the actual
  Steam friend-game eligibility/color/action gate, independently of ownership
  and protocol checks. Focused regressions and the full CPU/owner suite pass;
  all 40 existing Vulkan image cases pass. A hidden/no-save Alpha frame confirms
  chat placement/font/color and selected render style, not native pixel parity.
- [x] Addressed feat presentation: purple chat receipt followed by the captured
  physical-point label, native float projection order, physical 30px pixel/unhinted
  font with 3.75px colored outline, linear 3000ms fade/drift and old-age draw before
  strict expiry. The textFX layer precedes lifebars, ordinary UI and cursor; hiding
  the interface does not pause its age. Projection/lifetime cases and the full
  CPU/owner suite pass, as do all 40 existing Vulkan image cases. Owner-font captures
  at GUI scales 1/1.5 keep the same 112x19 colored-glyph bounds; those UI-only early
  frames are not settled-world lighting or native-game pixel-parity evidence.
  No feat producer, award or descriptor meaning was invented.
- [ ] Remaining sound-event wiring. The static-object trap tail now uses the shared
  hit and outgoing queues with state-change/nested-hit/direct-impact ordering.
  The native World clock now reaches physics for timed lava damage. Actual-facade
  health/event, callback-route and no-replay checks pass with the full CPU suite.
  Ordinary nonplayer corpse retirement and owned-kind6 revival now run between
  objects and pets, with deferred removal/view rebuilding and retained respawn
  policy. Lifecycle thresholds, ownership exclusions and actual vector-backed
  facade updates pass focused and full CPU checks. The non-terrain 0x119/0x11d
  first-visit areas now produce burst records and shared hits/healing, retaining
  critical/remembered-target ordering and two healing notices with one application.
  Actual health and event-queue checks pass. Nonzero-clock destructive corpses now
  retire without revisiting their terrain branch (regressions reproduce the old
  retained-corpse failure). Destructive 0x90 first visits now apply guarded terrain
  removal, debris/notices/sound, unsupported-model cleanup and shared area hits in
  the specified order. Ordinary model vectors retain semantic light/flags and their
  immutable loaded snapshots; renderer-only refresh and stale-job guards preserve
  removals without changing auxiliary light/emitter membership. Physical/full writes,
  batched publication, loaded-only/repeated history and nine-block-sample mesh
  requests are integrated. Packet7 lists concatenate across arrivals, empty-frame
  zone-change processing remains, and event consumption follows the client World
  and drawn-position passes. Cached generated materials and owner CWM1 sidecars
  support downward fills. Focused and full CPU/owner-data checks, Vulkan image
  suites and a no-save combat run pass. Fresh material capture, original-game peer
  acceptance, exact publication latency and broader visual/CRT comparisons remain.
  Local burst expansion now preserves all shared draw groups and cap ordering;
  styles1/2/5 update/collide/retire and render owner cube.cub from the independent
  [contract](specs/steam-particle-bursts.md). Zero-time queries, double angle steps,
  full-header matrices and opaque/no-cull material rendering have CPU/GPU checks.
  Particle admission uses native+0x49c; distinct shorter/wider-range GPU fixtures
  reproduced and corrected an initial+0x4a4 binding mistake.
  Native body fragments now use the same live cap/update and owner cube pass;
  cached ten-part bindings, surface-policy sampling, producer draw/deque ordering,
  negative age, retained matrices and mixed body/burst capacity have regressions.
  Frame-local received/strict-negative player death candidates replace the old
  persistent-death-driven fragment trigger; birth runs before current-frame pose.
  The corrected stored-block accessor reads auxiliary byte3, independently of flags.
  Static-object coverage now shares the generator/live-zone marking pass. It retains
  tracked coordinates, clears only existing0x40, respects overlap order and uses
  effective air/water plus the native narrowed fixed-coordinate bounds. Received
  object lists mark immediately; pending publication copies their records before
  re-marking on both host and mirror paths. Synthetic checks cover persistent keys,
  overlap bits, empty-column sea water, missing zones and repeated publication.
  A separate full-generation owner check preserves fresh versus daily-refreshed
  fixture phases. All26,418,885 block comparisons and3,231,744 column comparisons
  match across789 zone fixtures in the four supplied sets. The older structure-only
  check's84 differing blocks omit two creature-pass objects; they are not full-pipeline failures.
  Broader zone/theme coverage, pending-set producers and adjacent-zone ownership
  still need acceptance work; the recorded-fixture result is not global generation parity.
  Completed target-zone air/water red now publishes before geometry in both styles,
  without history/dirty/halo changes. Upkeep and particles share the corrected
  accessor; real CPU-mesh roof/sea/physical-edit cases and full CPU/GPU checks pass.
  Safe engine writer serialization retains immutable snapshots and lazy metadata.
  Combined sessions now fork independent host terrain readers with the same source
  configuration. Client solves, physical/history edits and loaded lifetimes do not
  mutate host storage; copied notices still update the client. Dedicated hosts reuse
  their supplied reader. The audited ordinary mesh worker is client-bound;
  indirect host-light writers remain outside that trace.
  Ordinary consumers0/3/4/6/7/8 now follow the independent style supplement:
  gravity/angle/pass-set differences, descending6 contact retirement, still3,
  owner note.cub7, separate geometry shrinking and per-draw shininess reset.
  World work, body/burst births and pose sparks now share the verified CRT transition
  in their engine execution role. Burst expansion/update precedes scene pose; AI
  retains a separate state. Arithmetic and actual state61 spark-continuation cases
  pass. Normal world posing now uses a current fixed-camera/range snapshot,
  registration order and pre-pose flags/health/sphere gates; body parts share that
  sphere. The independently corrected radius includes the body-height minimum.
  Headless real-draw and Vulkan snapshot/reuse checks pass, and the title underlay
  now runs the ordinary drawn/link phase. The independently traced coordinate
  contract distinguishes final-eye distance from negative-look-at plane/pose origins.
  Actor caches now retain native origin-relative matrices; only queued mesh transforms
  convert to the engine's source-home frame. Body births subtract the separate Game
  origin left by the preceding camera update, with no invented culled-cache repair.
  Part light binding uses the native full-header center before that mesh conversion.
  Large-coordinate, retained-cache/body-birth and explicit GPU binding checks pass.
  Zone boxes, scenery spheres/fades, adaptive ranges and retained speech now share
  the exact native CPU view/projection. Source-home metadata is converted to absolute
  XY once; native game Z stays unshifted. Beam distances retain a fixed eye too.
  Speech publication keeps its earlier cache boundary. Ordinary WORLD uploads
  its float matrices separately and composes WV/WVP on the GPU before local dots,
  including immediate GUI models and native water's shared vertex stage. DepthOffset
  modifies the composite clip-Z coefficient. WORLD and GUI large-translation plus
  retained-depth regressions fail before and pass after; full CPU/owner and Vulkan
  checks pass. Cross-GPU last-bit rounding and separate family input lifetimes
  remain bounded comparisons. Posed actors now retain their origin-relative matrices through GPU
  submission, with the native view/lens and matching fixed-origin point tables.
  Source-home transforms remain available to shadows and Alpha rendering. Asset-free
  GPU checks preserve a quarter block at world block16,777,216 and its attached light.
  Terrain now emits local corners in lowest-exposed-relative250-block bands, keeps
  source-block top250 corners and sparse bases, and submits fixed zone/band anchors.
  Source scan order, physical corner colors, face costs and native zone admission
  have CPU/GPU checks; retired band resources follow zone lifetime. Ordinary
  static/environment adapters also retain fixed anchors and local native matrices,
  separately from source-home shadow/Alpha transforms. Shared body/burst particles
  now also bind the current installed scene's fixed origin before float conversion;
  collection follows camera preparation. CPU Z/fraction/Alpha cases and actual
  Vulkan quarter-block coverage pass, including an old home-rounding negative
  control. Broader original visual and producer/CRT acceptance remains open.
  The independent [runtime initialization audit](specs/steam-crt-runtime-initialization.md)
  establishes a version-qualified fresh context default1 and startup/reset gates;
  it does not establish native first-frame state or change the engine's initial seed.
  A further [saved-appearance audit](specs/steam-playable-appearance-crt.md) proves zero
  draws/seeds for all16 valid saved playable routes; null startup avatar creation
  still consumes its five documented draws.
  Initial/factory ordering, reached reseeds,
  optional preview calls and broader same-thread consumer integration remain open.
  Unknown styles and independently untraced producers remain open; full native CRT and
  broader visual/peer acceptance remain open. Other object
  families and listening comparisons also remain. The independently traced
  [sound addenda](specs/steam-sound-event-addenda.md) resolves additional consumers;
  their actual runtime wiring and broader listening checks still remain.
- [x] Saved inventory loading preserves the stored tab count, including zero or
  fewer than six tabs, holes, negative quantities and complete item bytes. Empty
  quantity-zero slots contribute neither gameplay entries nor stale raw-item
  payloads. Regressions reproduce the previous tab-count change, phantom items
  and a stale empty pet slot replacing the live pet's name; focused save-format
  checks and the full CPU/owner-data suite pass.
- [x] Native-save gameplay uses canonical inventory tabs/equipment/counters as its
  authority. Bag/gear screens derive views with live tab/slot indices; saving writes
  those actual records, rather than rebuilding six flat categories. Pickups, direct
  equip/use/drop, quick items, recipes, cube upgrades, shops, identification, pet food
  and daily reset retain their distinct insertion/removal rules. Native slot and
  equipped sales have separate sources; buyback attempts its whole selected node.
  Explicit direct/nested refresh order is covered by transaction regressions. Legacy
  recording sessions retain their compatibility path. Full original-game comparison
  and extended gameplay/save-load checks remain below.
- [x] Identification's owner blackwidget/itemframe hierarchy, requested350x285 root,
  effective-frame placement, native captions/details/cost runs, strict footer halves
  and two-stage controller point truncation. Mouse footer picking retains the
  verified frame-origin/inverse-node hybrid, rather than inverting the full frame.
  Native release/Goodbye behavior retains selection and does not identify on down.
  Explicit opening/toggle refreshes preserve actual sort counts and selected region.
  Synthetic affine/edge/gate checks and the full CPU/owner-data suite cover these paths.
- [x] Shrine-choice map continuation: native item/creature openings, restricted flag-A
  hover, actual-overlay cached targets, known-cell left-release admission and deferred
  mode/source cleanup. The borrowed item address is never dereferenced or consumed.
  Accepted cells start ability0x75; its effect boundary keeps native signed32 coordinate
  arithmetic and mount placement. Shrine text uses the float world-update clock,
  separately from GUI bobbing. State, negative-coordinate, timing and owner-data tests
  pass. Broader original-game/controller and map-panel event comparisons remain.
- [ ] Native VoxelWidget customization editor: the independent
  [edit-view specification](specs/steam-weapon-customization-edit-view.md) now
  describes its owner root, camera, raw model, picking and input. Native palette
  production is corrected and tested, including negative/wrapped totals and
  retained draw-to-click state. Native model projection, object-space ray/voxel
  picking and the raw WORLD render pass are implemented and tested. The actual
  owner root, headings/footer, thumbnail/palette calls, right-drag/pad rotation,
  bench/movement gates and shared native down continuation are integrated.
  Full signed pickup coordinates, byte placement and full-coordinate removal
  retain their native asymmetry. Palette tooltips use original-source priority
  and the hidden crafting anchor, with a separate adapted text/image copy.
  Cached inventory refresh/hover state, broader GUI capture/map subtrees, full
  comparison/map/ground preview bindings and controller aim still need work.
  AdaptionWidget is a separate native panel; no unverified gold/item upgrade or
  opening action is inferred from its traced price-only callback.
- [ ] Ordinary WORLD-shader 3D GUI item icons: identification and equipment SpriteWidgets
  now use the shared helper, including full item records and per-widget callback ordering.
  The selected quick-item SpriteWidget also uses the original record. Inventory, shop
  and recipe grids now submit original records at their owner callbacks, preserve
  physical holes and region cutoffs, and use native cell snapping, sizing and colors.
  Formula results/ingredients and class-adapted information previews now use their
  distinct owner callbacks, depth, scales and clocks. Recipe spin advances only on
  resource-admitted draws; ingredients share native hover/navigation geometry.
  Customization now submits its thumbnails, raw base/attributes/candidate and
  palette through that renderer. The retained-item cursor now samples the canonical
  transient stack at each admitted InventoryWidget callback, after children and
  grid/text work. It retains nonzero negative counts, category0, the cached float
  pointer, fixed size and signed GUI-clock spin. Missing resources or hidden
  panels leave the stack intact; no drag/grab/save producer is inferred. Other
  callers and broader native visual comparisons still need conversion/validation.
  Category 23 procedural artifacts now use a full-key resource cache shared by
  the GUI and world model paths. The extractor can publish completed native
  volumes through an atomic request/status protocol; all eight independently
  specified shape families provide a fallback. Full allocations and native mode 1
  inputs are retained, with a separate Alpha appearance conversion. Codec,
  malformed-input, cache, generator and world-loader checks pass, as do the full
  CPU/owner and 40 world/20 GUI Vulkan suites. A completed 332-key native owner
  capture (with the owner's mods active) matches every generated RGB byte and semantic metadata field, including
  high-bit modifiers and multiple branches of all families. Exhaustive key coverage
  and broader original-game visual comparisons remain separate validation work.
  Ordinary information text now has its specified category rows, effects, float
  rounding and inline deltas. Equipped comparison uses original address identity,
  native candidate/Shift order, smaller fonts and fresh late resizes. Fractional
  comparison glyphs and standalone scaling have owner-tree regressions; earlier
  full earlier UI/comparison sampling remains open. Native map outputs now retain
  the full semantic payload in live storage, copy earlier flight/lore coordinates
  and publish only during an admitted overlay draw. Flight/lore/item precedence,
  exact flight quotes and static styled lore text follow the independent map audit.
  Owner-tree checks cover hidden/reset state, fixed body origins and late panel
  sizing; the full CPU/owner and 40 world/20 GUI Vulkan suites pass. Complete map
  collection/capture timing and camera state remain separate gaps.
  Ring keys now precede later toggle/repeat work; ground fallback resolves only
  existing zone/index data, retaining independent origin metadata and native
  previous-size placement. Packet-backed cleared-slot and owner-geometry tests pass.
  Information now reads borrowed records in its later draw callback; a hidden GUI
  ancestor suppresses draw-produced hover/pick/size changes without clearing own
  panel state. The game-UI/map/preview layer order follows the independent audit.
  The GUI clock now narrows each capped step before wrapping accumulation across
  title/game frames, rather than deriving spin from absolute wall time. CPU/owner
  checks and all40 world/16 GUI Vulkan image cases pass, plus water/Alpha checks.
  Current 3D map projection now precedes GUI, with flat collection at the actual
  Overlay callback. F4 suppresses both map passes; Control's retained message bit
  controls half radius, while pointer text samples earlier retained cursor data
  using its specified float32 norm. Screen-centre pixel truncation and owner
  callback-order tests pass with the full validation suite. Earlier camera/input
  phases, remaining map-panel controls, GUI capture/poses
  and layout-callback timing still
  need implementation/comparison.
- [x] Ring Menu selector: canonical category1 candidates, retained signed32 selection,
  neutral/repeat state, native label-before-selection timing, filtered input gates,
  selected Use/Quick Item and common down-tail close. The owner's selector subtree,
  per-node arrow feedback, separate queued3D WORLD projection/palette/depth and
  world-clock spin are integrated. Missing resources keep their original ring positions;
  no direct-widget shadow or spirit pass is added. Combined UI sound sampling and
  selected quick-button SpriteWidget use their specified paths. CPU/owner-tree tests,
  40 world and16 GUI Vulkan image cases, shared water and Alpha ordering pass.
  Original-game input/visual and hardware controller comparisons remain release gates.
- [x] Complete-record inventory slot backend and starter-kit consumer: six native
  kit lengths, equal-stack-before-hole insertion, separate counted fields, page
  versus creature removal, direct UI clearing and native key-only refresh followed
  by compaction. Refresh retains original tab membership, nonzero negative counts,
  unsigned subtype comparisons and the verified empty-slot predicate. Stat queries
  are explicit local-player inputs. Starter-kit contents and random draws remain
  covered by the existing frontend tests. Native-save gameplay now uses this storage,
  with its UI/owner-validation gaps explicitly retained above.
  Native semantic assignment updates every meaningful item field while retaining
  destination padding; selected operation inputs remain valid while their slots
  are cleared or vectors grow. Dedicated regressions cover both distinctions.
- [x] Complete item records survive gameplay/save views: received items, bag entries,
  carried/equipped gear and crafting comparisons use a shared conversion that
  retains uninterpreted bytes while writing current metadata/attributes. Removing
  one of two similar pets or weapons preserves the actual remaining name/rarity;
  formulas and original rarity bytes survive conversion. Bag pickups merge only
  native stackable types with complete semantic equality. Full-byte round-trip,
  metadata-edit and distinct-item save regressions pass, as does the CPU/owner suite.
- [x] Living-creature light source branches: carried/toggled lamps, effect/race/state
  lights, casting and four equipment spirit-glow anchors. The renderer reads live
  player/NPC records and buffs in world traversal order, including actors without
  submitted geometry; dead actors emit none. CPU cases cover raw/displayed yaw,
  fixed truncations, physical/drawn anchors, timing and range distinctions. GPU
  cases cover live/dead/restored actors without rebuilding geometry. Adaptive range
  inputs are integrated below; wider original-game visual comparisons remain.
- [x] Native adaptive scene range: current option-derived zone window, mesh readiness,
  native unmerged weighted triangle costs, tie-sensitive budgets, asymmetric easing,
  exceptional block/effect/flight gates and distinct scene/model/actor ranges. New
  bounds/cost metadata can be published while older GPU geometry remains active;
  culling and ground-light admission read a coherent frame snapshot. Legacy meshes
  without native cost data retain an explicit unavailable result. All12 sampled clean
  owner min/max/center/radius and weighted mesh costs match. Full CPU/owner-data and40 actual Vulkan image checks pass,
  including readiness loss/recovery and new cached bounds with old geometry. A hidden
  no-save settled-world capture with native radius15 passed; sampled peak private
  memory was1,687MiB. This is bounded evidence, not a whole-game/performance guarantee.
- [x] Shared Omega-inspired water and underwater appearance, independent of the
  Alpha/Steam lighting selection. Use the owner's surface/reflection and submerged
  haze references, retaining a native appearance option for parity checks. This is
  a visual rendering change; world geometry and generation stay unchanged. Actual
  Vulkan fixtures cover transparent shallow terrain, reflected geometry, blue haze,
  teal foreground tones, unchanged HUD colors and resized render targets in both
  lighting modes. Wider scene comparisons remain open below.
- [x] Alpha static-scenery distance staging: separate model/object range inputs,
  fixed XYZ keys, exact near equality, one mixed descending far queue after native
  water and before creatures, retained depth, and pixel-opacity blending. CPU
  boundary/sort checks and actual Vulkan near/far/dry controls pass. Shared water
  retains the fades: near scenery precedes the sky, and its one sorted far queue
  follows the sky before the refraction snapshot. Partial/zero-opacity image
  regressions prevent dark sky silhouettes; native after-water ordering and CPU
  opacity arithmetic are unchanged.
  Full prior admission/cache traversal and fixed gameplay-camera parity remain open.
- [x] Shop purchase/sale sounds use the player's physical fixed position; sales
  retain their specified 0.75 volume. Sound IDs and timing follow the independent
  shop and sound specifications.
- [x] Native map region history and bounded controls: ordered personal save data,
  per-world-step current-region sampling, separate borrowed-entry lifetime, owner
  arrows and bottom-left outlined label, callback-before-hit behavior and retained
  map-only shoulder edges. Nine mapped zoom layers, the separate wheel ceiling,
  fixed recenter/pan/easing and the independent information ZOOM value are integrated.
  Hovered/forced land state retains its 3D/Overlay phases; generated border points
  use the owner's cube model, full-header centering, native float pulse and normal
  depth-only/blended passes. The ordinary map retains normal-family winding and
  inherits world depth rather than clearing it. CPU/owner and six synthetic Vulkan
  map image cases cover these changes, including reversed winding and near-world
  occlusion. Controls/Multiplayer physical-wheel fallback now uses the
  cached GUI pointer after the three inventory grids, native one-row clamps,
  owner scroll-thumb rebinding and separate fit/refresh branches. It does not
  extend controller wheel eligibility. Controls reopening retains its local edits
  and row rather than reloading active bindings. The editor's initial defaults
  remain separate from loaded active bindings. Finite thumb-drag arithmetic is
  covered; its owner hierarchy/frame queries and shared capture routing still
  need integration. The movement-close batch hides its stated
  panel roots and clears only shown answer pages, preserving unrelated data and
  Options/Controls. Focused arithmetic checks cover those bounded rules. Broader
  early input/camera/GUI phase fidelity and native interaction/visual comparisons
  remain release gates.
- [ ] Native gameplay camera state, water/model/overlay shader controls,
  remaining panel opacity and object/effect visuals.
  Independent camera, UI/icon, shader/pass, overhead-model and Alpha-ordering specs
  are now available. Their remaining consumers and native visual comparisons still
  need implementation/validation. Scene range and Steam sky easing now use the
  verified float-rounded scene interval, distinct from the double camera interval.
  Movement now samples retained displayed yaw/pitch rather than current input
  targets. Controls and frontend forward vectors share the float camera-to-world
  basis, including its full up column, native radians constant and system CRT trig.
  Pole, tilted/rolled and double-to-float regressions pass. Legacy gameplay angle
  storage now shares the frontend's double-degree orbit state. Native controls,
  camera positioning, view matrices and the compass consume those retained degrees
  directly. Mouse scaling rounds to float before the double target update, with
  the native sensitivity/invert/clamp rules; pad and assistance retain double
  targets. Sub-float displacement/smoothing and no-shortest-wrap tests cover this.
  The independent [mouse-delivery audit](specs/steam-camera-mouse-delivery.md)
  confirms a polled handler before XInput, including stationary relative input.
  Its mouse-only pitch clamp now runs on every admitted poll rather than only
  coordinate changes. Captured look requests available raw motion, retaining
  device-count units instead of desktop-accelerated cursor movement. Unsupported
  raw input remains an explicit cursor-motion compatibility path; exact shared
  GUI capture/mode admission and physical-device comparisons remain open.
  Mouse map/preview/look handlers now retain the last-device selection after the
  mapped physical pass and before the current XInput work. Later controller
  callbacks update the live selection without rewriting that earlier snapshot.
  Focus/capture admission, root picking and UI-transition scheduling still need
  the wider runtime integration. The independent
  [GUI root/lifetime audit](specs/steam-gui-root-construction-lifetime.md) establishes
  a detached document source, one live native root and a retained Interface on Play.
  Frontend and gameplay now share the same engine Hud and reuse completed resources
  for the same install; entry clears only transient preview draw submissions.
  Explicit reloads separately reset the whole Hud value, avoiding stale template,
  geometry and font bindings; failed reloads cannot report the old interface ready.
  Scene-owned detached trees, identity-preserving reparenting and explicit
  node/Widget deletion now provide ordered cleanup for a bound Input. Production
  probes, icons, speech and inventory cells use the scene mutation operations.
  Frontend templates, speech and gameplay panels now share one gui.plx scene and
  document geometry cache. Semantic root order/active visibility, cached hit-only
  templates, full global picking and generated-card subtree rebuilding still need
  the broader runtime work; the game does not yet use the scene-bound input owner
  throughout its interface. Cursor units remain in a separate scene.
  Frontend Play now transfers both displayed/target angles and both distances;
  Continue retains angles and resets distances to8, while Select resets90/0/180.
  Saved-angle compatibility cannot overwrite this entry state or snap it to targets.
  The canonical frontend now also uses the shared physical-posture/fixed-anchor
  camera and eight-corner terrain obstruction, with native view rotation/fixed
  coordinates. Its completed eye/look-at state survives Play; the prior negative
  look-at render origin is restored separately from the eye. The independent
  [matrix-lifetime audit](specs/steam-camera-matrix-lifetime.md) establishes retained
  transition matrices and complete late CPU rebuilds, including zero elapsed time.
  Projection publication now follows that CPU phase independently of presentation;
  early speech/feat consumers keep a value snapshot of the preceding camera/origin.
  No numeric native pre-first-camera defaults are established; the engine keeps an
  explicit absent projection until its first CPU build. Direct-launch startup,
  exact event delivery, recovery and live comparisons still need parity work.
  The newly documented
  [ordinary frame order](specs/steam-gameplay-frame-order.md) also establishes
  split early/late input phases. The live client now completes World stepping and
  one full-frame drawn-position pass before creature targeting/local control, and
  does not simulate a newly selected input ability again in that frame. Control
  completes before camera smoothing. The canonical path now interleaves early
  movement/ultimate and owner-look with Use, then pet, Quick Item and late attacks.
  Stage tokens preserve skipped edge histories, and the late ordinary-UI gate is
  sampled fresh. Ownership/sound hooks, local lookup and ultimate buff forwarding
  have actual-facade regressions. Creature targeting now retains one early
  selection/depth/box result through the later object/item pass. Assistance follows
  ultimate processing and manual-stick slowdown follows live speaker override.
  Aim metadata is evaluated after movement rather than before the admitted stage.
  The60-block aim ray now follows fixed native stepping, missing-zone/budget order
  and flag0x40 rules. Target LOS has exact fixed endpoints, a double range check,
  independently normalized float direction and native epsilon comparison; it
  ignores0x40, unlike the aim ray. Derived tests cover boundaries, fixed low bits,
  mode differences and fallbacks without extra LOS. Native candidate float-key
  ordering now reuses the verified insertion/partition/heap algorithm. The charge
  score truncates each squared fixed axis before summing and remains distinct
  from selected ID, minimum depth and later speaker override. Shared focused-
  Widget admission and full camera-state integration remain open.
  Orbit-distance smoothing now uses its separate float 0.004 coefficient in
  both frontend and gameplay; the camera uses capped wall time rather than the
  simulation's 100-ms limit. Explosion notices use physical-player squared
  distance, strict maximum accumulation and same-frame 0.005 decay. Cosine shake
  translates the render view, CPU clip view and speech projection while eye/aim
  remain unshaken. Focused float references and two Vulkan projection cases cover
  these bounded consumers. Physical fixed anchors, posture/mount/step probes,
  native XY/Z/ability smoothing and the50-block snap now drive gameplay camera
  position. Eight type-only eye guards and sequential corner rays shorten current
  distance without replacing requested zoom; final solid fallback uses raw P.
  Zero-limit rays retain the initial zone lookup and skip block reads. World/clip/
  speech views use a native inverse rotation, including zero distance and poles.
  Derived numeric/probe tests, both-style Vulkan camera cases and the full image
  regression set pass. Exact mouse/capture producers, direct-launch startup,
  full GPU render-origin fidelity and input/GUI integration remain open.
  The native process lens now starts at its exact50-degree float radians and
  eases toward30/50 using the imported creature toggle and distinct native rates.
  Frontend and gameplay share that CPU state without resetting it on selection;
  canonical CPU/GPU projections and targeting rays consume the completed radians
  directly in both appearance styles. Generic Graphics.FieldOfView remains a
  compatibility-camera fallback. No unproved local scope key/gesture is added;
  hardware/entry comparisons and untraced recovery remain validation work.
  Shake reads the client world's float simulation clock after its substeps.
  Ordinary world frames
  now consume the retained capped double interval, 16-ms steps and a narrowed
  float residual without whole-ms carry; the final zero step is not skipped.
  The host's integer tick remains a separate caller. Fractional/long-frame and
  clock-lifetime regressions cover this distinction.
  Stock water opacity initializes to 1 and retains 1 through range reset; the native
  audit found no zero/fade-reset writer. The previous zero initializer was corrected.
  Physical flagged-block colour now feeds the sky/fog/sun transitions; it clears
  each frame even when legacy mesh metadata cannot establish a native scene range.
  Native numerical/visual comparisons and the remaining shader controls stay open.
  Shared Omega water and Alpha appearance remain explicit project extensions.
  Hit flashes now consume local and received visual records, select current spirit
  weapons, retain the fixed hit position and use native first-frame aging/strict
  expiration. They share the scene sort/zone slots. CPU and Vulkan regressions
  cover the store, consumer bridge and zero-fade slots. The race0x90 death flash
  also follows the [independent producer specification](specs/steam-transient-light-producers.md):
  persistent pose-preparation marker, drawn position, previous renderer sphere/range
  test, unsigned actor-identity ordering and 800-ms strict expiration. Death births
  precede hit-display births; the common age pass precedes scene collection.

The independently authored [runtime-gap specification](specs/remaining-runtime-gaps.md)
resolves several questions in older specs. Remaining checkboxes are intentional.

## Extractor callback admission, 2026-11-03

- [x] Replace stored terrain export's SDK identity/column access with the independent
  primitive Zone view and an original CWZ1 codec. Capture enumeration/leases also
  use the independent views. Synthetic malformed/change/budget cases and exact
  six-byte/65535-run-boundary checks pass; the actual engine accepts the fixture.
  All five targets and full CPU/owner suite pass. New native integration is not
  yet live-validated; no atomic snapshot or completed-light-phase guarantee is
  inferred from mesh presence or metadata rechecks. Other SDK views remain open.


- [x] Replace SDK GenericMod inheritance with an independently authored explicit
  callback record covering all 30 reached positions and 29 priority bytes. Raw-host
  tests verify initialization, tick routing, neutral interception and unchanged
  caller-owned mutable state. All five targets, required DLL exports and full
  CPU/owner checks pass. Project policy uses tick priority 0 and disables unused
  events; no SDK default was inferred. A new supported-game integration smoke and
  all other SDK dependency/provenance gates remain open.


- [x] Validate the loader tick value against the independently traced published
  controller and reject quit/unreadable/overflow paths before typed access. Pure
  memory-fixture admission tests and the full CPU/owner-data suite pass. All five
  targets build. The existing-owner title startup/resource-refresh/normal-close
  smoke check passes: two byte-identical exports, 332 owner volumes matching and
  game exit code 0. Loaded-world/recording/clean-install coverage and the SDK rights/
  full replacement gates remain open; title identity does not imply readiness.

- [x] Provide a bounded request-file alternative to extractor F10, including the
  title menu, so short automated keypresses are unnecessary. Live checks produced
  six frames and two queued batches totaling twelve more; all eighteen decoded,
  and the game exited normally. Original reference frames stay private. Broader
  loaded-world/recording and vanilla visual acceptance remain separate.

- [x] Replace primary sprite-table metadata access with an independently specified,
  guarded catalog reader. Corrupt-span, unreadable-memory, overflow and changed-
  catalog/range fixtures pass, as does the full CPU/owner-data suite. Live startup
  preserves all 3,652 rows and groups byte-for-byte and closes normally. Remaining
  SDK-dependent views and the complete independent adapter are still open.
- [x] Validate a loaded-world request capture: 1,101 fresh records, both light lists,
  and all 20 model/7 static lights in 18 nonempty zones match with no incomplete
  neighborhoods. Existing mods were retained; vanilla comparison baselines were
  restored exactly. This does not close animation recording or vanilla visual parity.

- [x] Read primary sprite vertex-buffer identities through an independent guarded
  ordinary-backend view; reject unknown types/changed links and avoid native bind
  calls. Focused fixtures, full CPU/owner checks and live 3,230-buffer capture pass.
  This is one further data view, not full SDK independence.
- [x] Separate modified zone/reference outputs and support a terrain-only pinned
  native reference. The same four isolated loaded/lit zones match 106,380 blocks
  and all region types exactly; full CPU/owner checks pass with that reference.
  Historical mixed exports are retained and their provenance is not inferred.

- [x] Independently implement bounded active terrain-group traversal and shared
  vertex-buffer views with the documented lock pair held through each capture.
  Linked/cyclic/corrupt/changed-publication fixtures and CPU/owner checks pass.
  The later loaded-zone checkpoint supplies a bounded original-game title smoke;
  other World views still depend on the SDK.

- [x] Replace the remaining SDK loaded-zone iterator in terrain-buffer capture
  with the independently traced element-sequence view. Synthetic noncontiguous,
  null-entry, corrupt/cyclic, changed-container and overflow cases pass. Actual
  Win32 contention/unwinding checks verify ordered nonblocking lock leases. The
  full CPU/owner suite passes. An extractor-only original title session also
  identified 3,230 sprite/1,172 terrain buffers and decoded six distinct frames.
  Broader lifetime, modified-session and gameplay acceptance remain separate.

## Host source-window continuation, 2026-11-03

- [x] Replace host square source demand with the independently verified per-player
  270-block box/radius-5 union, including eagle destinations. Keep physical resident
  admission separate and unload logical residents outside the wanted union even
  if a reader retains their source terrain. Native 77/78/79 phase, multiplayer
  overlap, negative/narrowed coordinates and owner-data instance-death travel
  regressions pass, together with the full CPU/owner-data suite. Exact concurrent
  cache/job scheduling and all packet-request demand paths remain unqualified.

## Requested Alpha terrain default and compatible mod support

- [x] Separate Alpha random, region, location and initial surface core, reusing the
  sister engine's clean-room code. Formula/surface checks and existing CPU tests pass;
  the initial surface remains a distinct intermediate product. Shared parent-stage
  caching, owning column leases and effective block/water reads are checked and
  consumed by the playable composition below.
- [x] Continue initial terrain through rock mounds, layered mesas, roads, terraces
  and pools at the explicit pre-dungeon boundary. Retain its random stream and
  boulder seeds; cache immutable relief parents and adapt their columns to Steam
  indexing. Protected-block, terrace, cache retry and lease-lifetime checks pass.
- [x] Voxel-tree builders and giant-tree relief locations: preserve moss, roots,
  crowns, dry-air exclusion and random draws. Palms load and stamp the owner's
  cardinal/diagonal leaf models with explicit rotation and placement rules.
- [x] Later boulders and the natural vegetation continuation, with retained random
  state, immutable parent caching, owner-model resolution and native plant placement
  rules. Formula/import/integration checks and live foliage captures pass.
- [ ] Broader labeled native height/surface/biome comparisons and forest-edge,
  transition, live-edit and cold-streaming acceptance. Contract tests are not those
  full runtime comparisons. Two additional 35-simulation-second Alpha-look flights
  using default/123 seeds cross positive/negative parent boundaries and land with
  normal exits; the original test save is unchanged. They add two bounded travel
  cases and do not close the broader acceptance item. A further 4,096-block flight
  lands and completes 145 simulation seconds with exit 0 in 163.79 seconds wall; the
  inspected final scene and unchanged test save add one longer streaming case.
- [x] Read-only initial-surface column adapter: explicit material/flag translation,
  four-parent crops, native indexing and finite-domain coverage. All 4096 columns
  in an asymmetric fixture retain their geometry and RGB. The runtime composition
  consumes this adapter while retaining Steam construction columns.
- [x] Bridge Alpha terrain/visual climate to Steam's retained town and dungeon
  layouts, with coordinate conversion, collision, water, lighting and object/spawn
  reanchoring. Protected Steam columns/models retain their tested geometry and
  placement. Gameplay region identities and difficulty remain Steam's.
- [x] Separate Alpha/Steam generation selection and versioned caches/save profiles,
  finite admission and multiplayer agreement. The working Alpha source is now the
  shipped default; original Steam generation remains Source=Game. Local and remote
  handshake cases pass; real two-account acceptance remains below.
- [x] Alpha material, environment-light and cloud precision corrections from separate
  native-only specifications. Byte/float/fixed vectors and full CPU/GPU checks pass.
  Bounded corner-light reuse and converged floods retain their checked outputs;
  microbenchmarks are not whole-game performance acceptance.
- [x] Steam-look preparation distinguishes the generated-source footprint, full
  neighbor admission and native cost budget from the draw window. Captured patches
  retain replay compatibility. Ordinary eagle source/mesh anchors and narrowed
  camera-zone conversion follow independent contracts; current/destination cache
  retention remains replacement policy. Bounded immutable Alpha reads preserve publication
  and world lifetimes. Twelve additional independent direct climate words cover
  land, mixed land/sea and sea, with missing-cell defaults; two more references cover
  natural positive-fire and mixed fire/nonfire. Formulas stay unchanged. These
  conditional arithmetic checks do not establish oasis or native-runtime parity.
  Alpha ready-parent retention now has64 entries and a 384 MiB estimated payload
  budget, eliminating observed repeat generation in the diagnostic. The previously
  failing full default-distance capture completes in113.808 seconds wall. Full
  CPU/GPU checks pass. Full-distance Alpha-look captures also complete for the
  default seed and seed 123 at another location; broader cold/travel/memory/cache-role
  acceptance remains open.
- [x] Preserve native float32 roughness-gate operation chains in raw Alpha H,
  using separate scalar fixtures and conditional base-height words. Landscape
  identity v2 keeps changed geometry/profile/peer state separate from v1. Full
  CPU/GPU checks and fresh full-distance v2 Alpha/Steam-look captures pass; these
  are not final-surface, whole-region or broader runtime parity claims.
- [ ] Carry compatible height changes into TerrainVariance in SteamCubeWorldMods,
  preserving its shoreline/water fixes and respecting its separate source license.
- [ ] Add verified Alpha-only location variants alongside Steam layouts. The first
  investigated item is the Sphinx desert-ruin type, whose model exists in the owner's
  Steam install; its actual role/placement needs the Alpha structure contract.
  Extra ruin structures are deferred at the user's request (2026-11-03) and are
  not a final-release gate.

See [Alpha generation](alpha-generation.md) and the
[independent parity boundary](specs/alpha-terrain-generation-boundaries.md).

## Validation needed before a final release

- [ ] Close the implementation gaps above, excluding explicitly deferred extensions,
  with focused checks for each change.
- [ ] Two-account Steam session and an original Steam-game peer: join/leave,
  interactions, combat, loot, mission state, chat and reconnects.
- [ ] Real first-run setup and interrupted extraction/recovery on a game install,
  then repeat on a fresh machine without development tools or prior exports.
- [ ] Extended gameplay/save-load sessions across classes, gear changes, death,
  respawn modes, travel and world transitions. Unit saves are not this whole check.
- [ ] Wider rendering comparisons: waterfalls, wet-ground film, underwater,
  day/night, buildings, effects and multiple GPUs.
- [ ] Verify the Windows package and bundled extractor against the supported game
  build, its separate loader dependency, license notices and matching source.
  Resolve the exact SDK revision and applicable permission before final
  SDK-dependent extractor redistribution; the [local-notice audit](sdk-license-provenance.md)
  does not establish SDK-wide license coverage.
  See [Windows packaging](release-packaging.md).

The installable development package is a candidate for those checks. It is not a
finished 1:1 build. A final GitHub release should follow completion of these gates.

## Content and source boundaries

The independent dialogue/item source audit identified no producer of numeric
8/9/10 grant/take streams and no book-to-known-recipe insertion. String scripts
have no give/take commands. The existing accepted-input helpers do not justify
inventing gameplay awards. Any newly observed producer needs a separate spec
and native evidence before integration.

Owners supply their own installed game, assets and local exports, and Steam checks
remain active. Prefer existing models, fonts, GUI shapes, sounds and runtime data
rather than constructing substitute game assets. No game executable, executable
dump, Steam runtime, extracted content or reference image belongs in source or release
packages. Keep implementation work behind the [clean-room process](CLEANROOM.md).

The source license choice is complete. Remaining provenance/rights considerations
are recorded in [PUBLICATION.md](../PUBLICATION.md), separate from technical readiness.
