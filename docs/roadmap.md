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
  Emote rendering and interactive answer selection remain below.
- [x] Authoritative day rollover, loaded-zone refresh and daily character data
  reset. Ground/block lists remain loaded; elapsed respawn timers survive resets.
  Synthetic and generated-world tests cover the clock boundary and timer behavior;
  shopper/static-object comparisons still need broader owner recordings.

## Genuine implementation gaps

- [ ] Remaining ability/AI cases and the native player's pet update pass. Broader
  recordings are needed beyond the samples currently supplied.
- [ ] Controller GUI navigation/click synthesis, tab/category switching, title-menu
  routing and aim-tracking integration. Exact navigation-point ranking is unresolved.
- [ ] Interactive conversation choices, emote rendering, numeric-script item actions
  and complete recipe sources. String scripts do not have give/take tokens; the
  independent runtime-gap spec describes a separate numeric format.
- [ ] Network feat production/payload meaning. Its display consumer is traced;
  generating awards remains unspecified. Guest mission marks are client-local,
  with host interaction notices; no invented quest-award packet is needed.
- [ ] Remaining sound-event wiring and exact feat/chat presentation.
- [ ] Water vertex occlusion/point lights, Alpha far-static-model ordering,
  exact logo lighting, panel opacity and the remaining object/effect visuals.
  These need reference comparisons or an independent behavioral specification.

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
