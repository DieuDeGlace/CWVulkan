# Engine readiness checklist

Audited against the source and tests on 2026-10-30. The checkboxes in the first
section mean implemented and exercised in the stated checks; they do not certify
complete Steam or pixel parity. Older progress notes are in Git history and the
individual feature documents.

## Implemented

- [x] Clean-room generator: terrain, structures, placements, items, residents and
  encounters in `src/worldgen/`, from the written specifications. Owner-exported
  terrain/structure/creature comparisons run in EngineTests when available.
- [x] Creature controller, physics, AI, upkeep, abilities, hits and poses in the
  `Native*` modules. Recorded comparisons cover the supplied samples; edge cases
  and the gaps below remain.
- [x] Item generation and death drops, ground pick-up and coin hop, shops and
  crafting. `GameShop`, `GameCraft`, `GroundItems` and `NativeDrop` implement the
  written rules. Coins are picked up near the player rather than flying to them.
- [x] Identify and upgrade windows, including spirit cubes: `IdentifyWindow`,
  `UpgradeWidget`, the main-loop handlers and `LootWindowTests`. These were stale
  unchecked items in the previous checklist.
- [x] Generated conversations, regional mission checks, lore/artifact data, map
  markers and interactions. `TalkGen`, `TalkScripts`, `Missions`, `Lore`, `MapMarks`
  and `UsePrompt` are wired into play. Remaining script/network gaps are below.
- [x] Native character and world save formats, read-only import, zone changes and
  Continue restoring the saved position. `NativeSaves`, `NativeProgress`,
  `ZoneStates` and SaveTests cover the supported format paths.
- [x] Eagle title flyover, character select/create/delete, options, binding editor
  and the system menu. `Frontend`, `FrontendRun`, `GameOptions` and FrontendTests
  cover their implemented rules. Exact final rendering still needs comparison.
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
  implemented from the independent spec, including answer rows. Identification points, interactive-map
  shoulder routing, aim integration and hardware comparisons remain. Large region
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
- [ ] Native gameplay camera state, water/model/overlay shader controls,
  Alpha far-static-model ordering,
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
