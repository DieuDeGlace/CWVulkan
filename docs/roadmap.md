# Engine readiness checklist

Audited against the source and tests on 2026-10-31. The checkboxes in the first
section mean implemented and exercised in the stated checks; they do not certify
complete Steam or pixel parity. Older progress notes are in Git history and the
individual feature documents.

## Implemented

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
  Scale0.4096/1/1.5 and synthetic state regressions cover these paths. Cached GUI mask layers,
  preview creatures and full frontend pixel parity remain open.
- [x] Host/client worlds, single-player loopback, UDP, friends/invites, Steam
  transport and dedicated server. The recorded wire comparisons and local
  multi-process runs pass for the checked cases; Steam peer validation is pending.
- [x] Chat entry and send/receive handling: `ChatLine` and its main-loop
  consumers. The old "chat entry line missing" item was stale.
- [x] Owner setup wizard, data validation, journal/restore and interrupted-run
  tests against a simulated installation. The real-install checks remain below.
- [x] Alpha/Steam sky styles, clouds, stars, plant sway, map rendering, HUD coin,
  sound-table import and native GUI/font loading from owner data.
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
  every region or seed. The expanded 2026-10-29 audit covers 789 zone snapshots
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
  Native scene palette targets/RGBA easing now have numeric checks and Steam RGB
  integration, including regional/structure/effect13 blends and capped wall timing.
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
- [ ] Finish controller GUI coverage and aim tracking. Navigation ranking, cursor
  repeats, click phases, root/system/Options/Controls points, inventory grids,
  equipment and recipe points, panel scrolling, tab rows and region filters are
  implemented from the independent spec, including answer rows and identification footer points.
  Interactive-map shoulder routing, aim integration and hardware comparisons remain. Large region
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
- [ ] Remaining sound-event wiring. The independently traced
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
  palette through that renderer. Retained-cursor and other callers still need conversion.
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
  extend controller wheel eligibility. The movement-close batch hides its stated
  panel roots and clears only shown answer pages, preserving unrelated data and
  Options/Controls. Focused arithmetic checks cover those bounded rules. Broader
  early input/camera/GUI phase fidelity and native interaction/visual comparisons
  remain release gates.
- [ ] Native gameplay camera state, water/model/overlay shader controls,
  exact logo lighting, panel opacity and the remaining object/effect visuals.
  Independent camera, UI/icon, shader/pass, overhead-model and Alpha-ordering specs
  are now available. Their remaining consumers and native visual comparisons still
  need implementation/validation. Scene range and Steam sky easing now use the
  verified float-rounded scene interval, distinct from the double camera interval.
  Stock water opacity initializes to 1 and retains 1 through range reset; the native
  audit found no zero/fade-reset writer. The previous zero initializer was corrected.
  Flagged-block color and the remaining shader controls still need their consumers.
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

## Validation needed before a final release

- [ ] Close the implementation gaps above, with focused checks for each change.
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
