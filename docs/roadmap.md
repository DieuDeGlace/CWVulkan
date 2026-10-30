# Engine readiness checklist

Audited against the source and tests on 2026-10-28. The checkboxes in the first
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
  NPC bubble/emote playback remains below.
- [x] String-script answer selection, native inn/specialization actions and saved
  specialization changes. Labels retain dictionary word colors and native glue;
  owner widget/font tests cover row picking, sizing and controller points. The
  general feedback panel follows native reveal, release, voice and indicator rules.
  Answer-visible action gates preserve movement, jump and earlier ultimate input;
  focused DialogueChoicesTests cover the distinction. Broader live visual comparisons
  and NPC bubble/emote playback remain open.
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
  CPU boundary and 25 synthetic GPU checks pass, including offset70 and more than
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
  Native adaptive range/camera state, fresh cached-box comparisons, remaining
  creature/frame-transient lights, special shader controls, undefined
  consumed lamp turns and broader dungeon/theme oracles still prevent a complete
  parity claim.
- [ ] Remaining ability/AI cases. Broader recordings are needed beyond the samples
  currently supplied, including integrated pet combat and appearance comparisons.
- [ ] Finish controller GUI coverage and aim tracking. Navigation ranking, cursor
  repeats, click phases, root/system/Options/Controls points, inventory grids,
  equipment and recipe points, panel scrolling, tab rows and region filters are
  implemented from the independent spec, including answer rows. Identification points, interactive-map
  shoulder routing and hardware comparisons remain. Region-distance ties beyond
  32 choices still need an exact specification.
- [ ] NPC bubble/emote playback, numeric-script item actions and complete recipe
  sources. String-script answers and general feedback are integrated above; string
  scripts have no give/take tokens. Only numeric item-receipt feedback has a verified
  producer so far. Native NPC speech/emote timing is independently specified and
  still needs integration.
- [ ] Network feat production/payload meaning. Its display consumer is traced;
  the [producer audit](specs/steam-feat-production-audit.md) found no verified award
  source. Generating awards remains unspecified. Guest mission marks are client-local,
  with host interaction notices; no invented quest-award packet is needed.
- [ ] Remaining sound-event wiring and exact feat/chat presentation.
- [ ] Remaining native creature lights and the race0x90 death-effects flash,
  water/model/overlay shader controls,
  Alpha far-static-model ordering,
  exact logo lighting, panel opacity and the remaining object/effect visuals.
  These need reference comparisons or an independent behavioral specification.
  Hit flashes now consume local and received visual records, select current spirit
  weapons, retain the fixed hit position and use native first-frame aging/strict
  expiration. They share the scene sort/zone slots. CPU and Vulkan regressions
  cover the store, consumer bridge and zero-fade slots. The death-effects producer
  and its persistent pose-cache marker are [specified independently](specs/steam-transient-light-producers.md)
  but still need runtime integration.

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

Owners supply their own installed game, assets and local exports, and Steam checks
remain active. Prefer existing models, fonts, GUI shapes, sounds and runtime data
rather than constructing substitute game assets. No game executable, executable
dump, Steam runtime, extracted content or reference image belongs in source or release
packages. Keep implementation work behind the [clean-room process](CLEANROOM.md).

The source license choice is complete. Remaining provenance/rights considerations
are recorded in [PUBLICATION.md](../PUBLICATION.md), separate from technical readiness.
