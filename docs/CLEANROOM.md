# Clean-room process

CubeVulkan is to Cube World what OpenMW is to Morrowind: its own engine, written from a
description of how the game behaves, that reads the content of the owner's own copy. To keep
source provenance separate it follows a clean-room process: the people (or agents) who read the game's code don't write
CubeVulkan's code, and those who write the code don't read the game's code.

## How a feature is ported

1. **Specification.** One agent reads the decompiled game (`decompiled cube world steam`, by
   RVA) and the binary for its constants. It writes a plain-language behavioural spec in
   `docs/specs/`: inputs, outputs, the order of decisions and random draws, and constants, ids
   and record offsets (facts). It contains no code, no transcribed pseudocode, no decompiler
   variable names, and nothing from third-party reconstructions (Infinite-Cubes, its Steam
   `release` and its Alpha `alpha` folders alike). Alpha specs come from the Alpha decompile
   (`disassembled alpha cube world`). Routines are cited by RVA.

   The spec writer may look at the reconstructions to cross-check a reading: a misread
   offset, a stuck routine, or a guess about what something is for. It takes nothing from
   them. Whatever the cross-check suggests is confirmed in the decompile and written in the
   spec's own words, citing the decompile, with none of the reconstruction's names, comments
   or structure. They have no license, so all rights stay with their authors.
2. **Implementation.** A second agent reads only the spec and CubeVulkan's own sources. It
   never opens the decompile, the binary's code or Infinite-Cubes. It writes the code with
   CubeVulkan's structure and names, readable on its own, commented in plain words.
3. **Verification.** The result is checked against the game's own behaviour: its exported
   data (`Extract/`), recordings and captures. It is never checked by comparing against code.

Similarities with the game are expected where behaviour must match (constants, the order of
random draws, data layouts). What must not happen is lifting code or its expression.

## Engine-specific appearance extension

The optional dual-tone hair controls are a user-requested engine feature, not a
port of native behavior. Their implementation uses CubeVulkan's existing tinting,
frontend, configuration and profile-note APIs. Initial values may be read from the
owner's INI data; no mod implementation or native source is consulted. Hair models
and palette artwork are loaded from the owner's installation at runtime and stay
out of source and runtime packages. The native character appearance format is
unchanged, and the extra style currently applies only to the local player.

## Audit, 2026-10-27

- Comments and docs no longer use names that a third-party reconstruction invented for the
  game's methods, members and constants (about 300 lines). They now describe the behaviour in
  plain words, citing RVAs where known.
- Class names that the game's own binary carries (its RTTI: `Game`, `World`, `Creature`,
  `Zone`, `MapOverlayWidget`, `InventoryWidget`, `ScalableFont`, ...) and outside APIs remain.
- Code identifiers borrowed from the reconstruction were renamed (`mapLabelScale`,
  `mapViewCentre`, `boughtFlights`, `boughtFromVendor`, `nameHash`, `plx::keyOf`,
  `setTextColour`). The game's own item keys (`WaterFlask`, ...) are its data and stay.

### To redo clean-room

These were written while reading the Infinite-Cubes reconstruction. They are to be specified
from the decompile and reimplemented from the spec, then their notes here removed:

(None left: the last, `src/RaceKeys.inc`, is under "Done so far" below.)

Everything ported from here on follows the process above.

### Also to redo: modules transcribed straight from the decompile

These were written by one hand while reading the decompile, often close to its shape (raw
float bit patterns, offset-named helpers). They don't use the third-party reconstruction, but
they aren't clean-room either, and many aren't readable the way the project wants. Each gets
a spec from the decompile and a fresh implementation from that spec, verified against the
same oracles that check them today:
- the recorded creature updates, AI ticks, hits, ability calls and effect steps;
- the oracle zones extracted from the game.

(None left: the last two, `NativePlayer`'s `transformed()` and `NativeMenu`, are below.)

Done so far with the full process:
- Shared GUI routing follows `steam-gui-pointer-capture-routing.md`: cached and
  previous pointers, held bits, capture-or-root selection, independent virtual/
  registered gates, focus/auxiliary bookkeeping, nested release moves and ordered
  global visitors. Synthetic traces cover owner changes and reentrant movement.
  Widget initialization/copy, dirty-byte identity, Button groups and owned sequence
  starts follow the independently traced scene lifecycle. Layout preserves Widget
  identity and resize-flag propagation, with separate own hooks and disabled-subtree
  admission. The dispatcher is a tested core; full live binding, other dynamic-class
  hooks and native frame/capture comparisons remain integration work.
- Map history, fixed map controls and region borders follow
  `steam-map-region-history.md` and `steam-map-border-and-information-hud.md`.
  Personal ordered history and borrowed-entry lifetime are separate from discovery
  sets; native resources provide arrow and border geometry. Fixed-coordinate pan,
  wheel recenter, quantized easing and the distinct zoom limits use the independent
  arithmetic contract. Border descriptors retain generated points, native float
  clock/opacity and separate depth/color passes. The independent retained-item/
  pass-family supplement confirms normal-only map queues and inherited world
  depth. Actual Vulkan checks distinguish both winding directions and reproduce
  the former private-clear occlusion bug before verifying its correction.
  Hidden-root dispatch and standalone
  hit queries remain distinct. Physical-wheel Controls/Multiplayer fallbacks use
  `steam-physical-wheel-panel-fallbacks.md` for priority, signed/unsigned row
  arithmetic and separate thumb refreshes; they add no controller-wheel eligibility.
  The movement-close batch follows region-history §7.4.1, preserving Options,
  Controls, panel data and unrelated answer fields. Wider input/GUI phase gaps are
  recorded in the roadmap.
- The retained item image follows `steam-retained-item-and-map-pass-family.md`:
  each admitted inventory/shop/crafting callback samples the canonical transient
  player stack after its children and text. Pointer coordinates, literal size,
  signed-clock rotation and shared helper state follow the independent contract.
  No positive cursor-origin action or new save field is manufactured. Controls'
  local-edit retention and separate startup defaults follow the wheel supplement's
  independently traced lifecycle. Its finite drag arithmetic is implemented from
  the supplied query outputs; actual owner geometry/capture routing remains a
  separate integration boundary, rather than a guessed rectangle.
- Procedural artifact resources follow `steam-procedural-item-resources.md`:
  independent generators implement the seven named families and default sphere,
  preserving full ID/modifier keys, allocation dimensions and native color mode.
  The extractor copies completed native resources on the game thread to a separate
  owner-data format; captured RGB and game data stay outside publication. GUI and
  dropped-item consumers prefer those owner records. Synthetic codec, cache,
  geometry and rendering checks pass. A separate 332-key owner capture, taken with
  the owner's mods active, matches independent generation across all RGB bytes and
  semantic metadata. The private
  captures are excluded from publication; synthetic fixtures do not supply that
  native reference evidence.
- Map draw phases follow `steam-map-rendering-phases.md`: current 3D map work
  precedes GUI, and flat markers collect at the admitted Overlay callback rather
  than using the preceding camera's screen positions. F4 suppresses 3D and GUI
  map draws; retained Control message state selects half radius independently
  of full-map mode. Pointer text samples retained cursor data earlier, with its
  distinct float32 distance norm. Pixel-centre, owner callback-order and hidden
  collection regressions cover these changes. Broader input/camera phase fidelity
  and hovered-land highlighting remain separate work.
- Map information follows `steam-preview-map-location-lore.md`: complete semantic
  item payloads stay live in the overlay member, while flight/lore coordinates
  are copied before the later draw. Reached minimap/drag draws reset outputs;
  hidden ancestors retain them. Flight quotes use current physical coordinates,
  known points and the live target flag. Static lore keeps fixed headers, styled
  words, native punctuation gaps and page spacing without activating page effects.
  Owner-tree tests cover draw order, previous backgrounds and late 115/145/124/103
  heights. Full GUI collection/capture and earlier inventory sampling remain open.
- Ground/Ring information bindings follow `steam-preview-source-lifetimes.md`,
  `steam-ring-selector-rendering.md` and preview sections4/5/7. Ring sampling keeps
  the earlier canonical key; draw resolves current ground storage without loading
  zones or rechecking source validity. Ground metadata remains independent when
  Ring supplies the item; only the adapter's temporary item value is copied, never
  retained as an original-source snapshot. Native bottom placement uses the earlier
  stored size. Packet-backed post-pickup and owner-geometry cases cover these paths.
- Live information reads, hidden-ancestor draw admission and the bounded
  game-UI/MapOverlay/Preview order follow `steam-preview-hidden-interface-order.md`.
  Information formatting occurs after earlier widget callbacks instead of freezing
  their mutable member beforehand. Owner-tree tests exercise palette mutation,
  hidden-state retention and resumed reads. A const ground-key resolver reads only
  existing zone state and index bounds; it does not invoke lazy generation or add
  source-validity filters. Complete source sampling and key/focus timing remain open.
- Item-detail text and equipped comparisons follow
  `steam-voxel-widget-information-preview.md` sections4/7/8. A separate engine text
  plan preserves row admission, owner dictionary callbacks, original equipped
  records, artifact/rank/effect literals, exact float rounding and separately
  rounded deltas. Glyph output reports its actual pen for inline suffix placement;
  comparison fonts retain unscaled outline widths in the shared atlas. Synthetic
  record tests and owner glyph/lifecycle tests exercise these bounded consumers.
  Native selector timing and live map/ground/Ring bindings remain separate work.
- Native information-panel size lifetime follows
  `steam-voxel-widget-information-preview.md` section7: earlier selection uses
  stored geometry, root/children precede the own callback, and later text-derived
  resizes do not redraw that background. Owner-tree regressions cover initial,
  hidden, growth and shrink transitions. `steam-preview-source-lifetimes.md`
  independently clarifies that Ring can retain a distinct ground-origin record;
  source selection preserves that flag without inventing a copied stack or a
  quantity formatter. Full formatter rows/deltas and live source wiring remain open.
- Native customization root, draw plan and event continuation follow
  `steam-voxel-widget-presentation-input.md`,
  `steam-weapon-customization-edit-view.md` and
  `steam-voxel-widget-down-event-routing.md`. The implementation reads only
  those independent descriptions and owner runtime resources. Full-coordinate
  pickup/removal and byte placement are tested separately; no native code or
  geometry is included in either package. Palette information-source precedence
  follows `steam-voxel-widget-information-preview.md`, preserving the original
  record and using a separate class-adapted copy for text/image. Full cached GUI
  lifecycle, comparison/map/ground bindings and controller aim remain separate.
- Inventory wheel target priority and per-message sign normalization follow
  `steam-inventory-refresh-events.md` section4.2. Input retains the event pointer
  and reuses the engine's existing owner-tree hit test; no game input code is read.
  Synthetic cases cover overlap priority, no-hit and signed fractional/large input.
  Cached range/thumb state and the remaining refresh lifecycle stay separate.
- Alpha static-scenery pass ordering remains based on
  `alpha-far-static-ordering.md`. Its new normalized-target/fade clarification
  confirms that CPU opacity must not gain a new endpoint clamp or discard.
  The shared-water extension now blends its deferred scenery over the sky before
  its refraction snapshot; the native-water path and retained depth are preserved.
  Synthetic Vulkan controls reproduce the prior shared-water error and its fix.
- Native edit projection/ray/voxel traversal and raw WORLD draw support from
  `steam-weapon-customization-edit-view.md`. Full allocation, negative aspect,
  stored lens constants, signed cell/byte semantics and raw shader state are
  independently implemented. Synthetic CPU and Vulkan cases verify these bounded
  paths; the root/event integration is described above. No owner geometry is shipped.
- Canonical customization palette production from
  `steam-weapon-customization-palette.md`: selected-material semantic equality,
  four field-only spirit totals, signed32 wrapping and retained click input.
  One engine-owned pass accumulates the five totals without copying game code.
  Editor presentation is described above; no artwork or owner asset is added.
- Recipe result/ingredient and information-preview WORLD calls from
  `steam-gui-item-icons.md` §§11/12/16. Class adaptation affects the information
  icon's copied record, preserving the source. Resource-gated recipe rotation,
  callback order, strict result hover, ingredient rounding/navigation and GUI
  step/total precision follow the independent facts. Original records also reach
  the existing naming helper without discarding wrapped item or spirit data.
  Synthetic math/state and local owner-tree checks cover these bounded paths.
- Inventory, shop and recipe grid WORLD item calls from `steam-gui-item-icons.md`
  §9 and `steam-inventory-grid-layout.md`. Native inventory views retain physical
  slots, including holes, and original records; region drawing stops independently
  of whole-tab scrollbar counting. Cell rounding, hover scaling, names and colors
  follow the independent facts. Only locally loaded owner geometry reaches the
  renderer; no original asset is added to source or runtime packages. Persistent
  widget refresh/callback timing and broader original-game comparisons remain open.
- Ring Menu input, canonical candidate identity, selected Use/Quick Item, combined UI
  sound edges and queued screen-overlay rendering, from `steam-inventory-slot-lifecycle.md`
  §9 and `steam-ring-selector-rendering.md`. Original item records, owner selector/fonts
  and runtime CUB resources remain external. Synthetic state/math, local owner-tree checks
  and actual Vulkan image cases cover the bounded implementation; original-game/hardware
  comparisons remain open. Earlier use-prompt inventory terminology was corrected by the
  separate spec author before the implementation changed that fallback.
- equipment SpriteWidget item calls from `steam-gui-item-icons.md` §10. Original
  equipment records and owner CUB geometry reach the shared WORLD helper inside
  each copied widget's draw callback; legacy record-less previews remain separate.
  Synthetic placement and local owner-tree checks cover sizing, callback order,
  attributes, visibility and missing resources. No owner assets are distributed.
- character-menu Button initialization/hover animation, raw custom-widget binding transfer,
  physical-pixel control placement and filtered drag/inertia, from `frontend-characters.md`,
  `plasma-format-and-scene.md` and `steam-character-list-input-and-frame-init.md`.
  Instance animation does not modify owner documents. Synthetic state/clock/geometry checks
  and local owner-data comparisons cover these bounded paths; GUI layers and broader visual
  comparisons remain open. No original game asset is part of the publication package.
- shrine-choice map opening, borrowed source lifetime, render-produced target and left-release
  acceptance, from `steam-item-map-selection.md` and `map-markers.md`. The existing native
  return ability keeps its effect boundary and signed32 cell arithmetic. World elapsed time,
  GUI bob time and calendar time remain distinct; no source-item depletion was invented.
- character-selection control placement using effective owner widget frames, preserving
  template transforms and picking through the copied shapes, from `frontend-characters.md`
  §2.3.1. Size requests are not substituted for measured geometry. Owner-data checks cover
  GUI scales 1 and 1.5; remaining frontend animation/rendering gaps stay explicit.
- identification owner-tree layout, independent footer mouse/navigation adapters,
  caption/details/cost typography and release/Goodbye behavior, from
  `steam-identification-ui.md` and `steam-controller-handoff.md`. The implementation
  clones owner data at runtime; cached bitmap icon previews remain explicitly
  separate from the unfinished shared3D GUI item renderer.
- canonical native-save gameplay inventory/equipment/counters, direct item actions,
  live identification indices and shop transactions, from
  `steam-inventory-slot-lifecycle.md`, `steam-identification-ui.md`, `loot-crafting.md`
  and `loot-shops.md`. Independent tests cover operation order, semantic assignment,
  signed counts, distinct removal callers, refresh callbacks and raw save retention.
  No native code or owner asset is included; auxiliary UI presentation remains open.
- received-feat projection, lifetime and physical-pixel owner-font display, from
  `steam-chat-feat-presentation.md` sections4.1-4.4. Synthetic geometry/timing and
  live owner-font captures check this display without inferring awards. Scene/sky
  clock narrowing and stock water-opacity lifecycle follow their independent specs.
- the stackable-type mask correction, from `steam-inventory-slot-lifecycle.md`:
  type22 merges, type18 stays separate, and an equal stack wins over a prior free
  slot. These helpers do not replace the remaining canonical player-slot work.
- chat history/editor, entered commands and friend-game UI eligibility, from
  `steam-chat-feat-presentation.md`. Owner fonts/templates stay external; synthetic
  editor/wrap/queue/command records and a live owner-font frame check the bounded
  implementation. No new feat producer/award or ownership bypass was added.
- adaptive mesh-readiness ranges, native face-cost metadata and key-only tie ordering,
  from `steam-scene-lighting.md` §§2.6/6 and `steam-controller-handoff.md` §4. Pending
  metadata is independently published from worker jobs without asserting new geometry
  is active. Owner cache costs and CPU/Vulkan regressions verify the supported paths.
- saved/default recipe lifecycle and full crafting-result item retention, from
  `steam-dialogue-item-actions.md` §4 and `zone-object-interaction.md` §0.7.
  Synthetic bridge/default/equality cases and the full owner-data suite verify
  preserved records; no new recipe grant or numeric award producer is inferred.
- per-speaker NPC speech/emote lifecycle, literal emote-word pages, cached projection,
  owner-template placement, world emote transforms/depth/culling and general lore/
  receipt routing, from `steam-npc-speech-rendering.md` and
  `steam-dialogue-item-actions.md`. Tests use synthetic state, owner templates and
  Vulkan captures; no native code or replacement assets are included.
- live static-object clocks/pose synchronization, generated light shafts, native
  mesh-time model sunlight/glow/fade queues and castle unsigned-choice arithmetic,
  from `worldgen-parity-gaps.md` §§2.1,4.4,9–10. Verification separates fresh
  generation, partial refresh traces and mesh-time light inputs. No original
  source code or replacement game assets are included.
- static-object visual sizing, captured door/window transforms and crop/furnishing
  model metadata, from `worldgen-parity-gaps.md`. Owner voxel dimensions are read
  from local model headers; no replacement assets or reference source code are included.
- controller GUI events, ranking and repeat timing, inventory region/tab state,
  navigation registration, shape picking and transformed bounds, from the independent
  `remaining-runtime-gaps.md` sections 4.1–4.17. Implementations and synthetic checks
  use only that specification and the engine's sources; owner assets remain external.
- the special skill and interaction kinds 1/11/12/13;
- the conversation builder (`src/TalkGen.*`);
- map markers, lore and artifacts (`src/MapMarks.*`, `src/Lore.*`);
- the .plx reader and the Plasma scene, shapes and text (`src/Plx.*`, `src/Plasma*`), with
  FreeType for glyphs.
- the use key: targets, prompts and actions (`src/UsePrompt.*`).
- the creature upkeep (`src/NativeUpkeep.cpp`), all 10,800 recorded calls matching.
- the hit routine (`src/NativeHit.cpp`), all 600 recorded hits matching.
- drops, rewards and item generation (`src/NativeDrop.*`), the drop, item and gear oracles matching.
- the effect objects (`src/NativeEffects.cpp`), 697/700 steps as before; the 3 failures are a recorder gap.
- the stat rules (`src/NativeRules.cpp`), playerstats and rulesoracle matching.
- the creature physics (`src/NativeUpdate.cpp`), all 8822 recorded updates matching.
- the creature AI (`src/NativeAI.cpp`), 1776/1800 recorded ticks; the rest are recording artefacts.
- the creature abilities (`src/NativeAbility.cpp`), the ability oracle as before. (Slip on record: the implementer saw the old file's first ~30 lines, includes and a byte accessor, plus one asinf line, before deleting it; the new code was written from the specs.)
- the creature pose (`src/NativePose.cpp`, `NativePoseTables.inc`), every recorded pose bit for bit.
- the player control (`src/NativeControl.cpp`), the control and attack oracles matching.
- the zone-object interactions, kinds 2, 3, 5, 7, 8 and 10, and the open/shut routine (`src/NativeInteract.cpp`), from unit tests (no recording exists).
- the item models (`src/NativeItems.cpp`), all 117,000 recorded model slots matching.
- the world generator (`src/worldgen/`, replacing `GameGen.cpp`, `GameGenMap.inc` and
  `GameCreatures.cpp`, deleted unread): terrain, structures, creatures, region roles, looks and
  gear, every caller moved to it. The oracle zones match draw for draw (gen_seed_123, gen2, genb:
  every .rtrace whole), with every spawn point but those whose race comes from the three picker
  lists not yet extracted, and genr's shopper that reads past the game's list.
- shops, identifying and crafting (`src/GameShop.*`, `src/GameCraft.*`, deleted unread) and ground items and
  pick-up (`src/GroundItems.*`, replacing CubeVulkan's invented coin flight), from `loot-shops.md`,
  `loot-crafting.md` and `loot-pickup.md`; checked by unit tests of the seeds, prices, the craft timing,
  the hop and pick-up timing (no shop, crafting or pick-up recording exists yet).
- the world map's region model and its markers (`src/worldgen/MapModel.*`, 0x2eb7b0), written
  fresh from `worldmap-model.md` in place of CubeVulkan's own stand-in; checked by unit tests of
  what the spec states (sizes, units, marker lists and their order around a town), since no
  recording of the game's map exists yet (`EngineTests mapdump` prints what one would be
  compared with).
- the creature looks (`NativeLooks.cpp`, now `worldgen/Looks.*`) and the creature record, residents
  and pets (`NativeCreature.cpp`, now `worldgen/Residents.*`), deleted unread; looks.bin 7704/7704.
- the class-adapted treasure item, `transformed()` in `src/NativePlayer.cpp` (0x10a720), its old body deleted
  unread and written from `player-item-adaptation.md`; checked by unit tests of every rule over 3000 modifiers
  and each class (no recording of the routine exists yet).
- the race-number-to-dictionary-key table (`src/RaceKeys.inc`), deleted and written again from
  `race-keys.md`, whose facts come from the game's own table (0x1401510f0); checked by unit tests of the
  table and that every key names an entry of the install's `dict_en.xml` (all but Bullterrier and Vase,
  which the game's dictionary lacks). (Slip on record: the implementer saw the old file's first five
  lines, its two-line header and three lines of entries, before deleting it.)
- the game's save files (`src/NativeSaves.*`, from `native-saves.md`) and the front screens (`src/Frontend*.cpp`,
  `src/HudFrontend.cpp`, from `frontend-title.md`, `frontend-characters.md`, `frontend-options.md`).
  `src/NativeMenu.*` (the CWM1 replay and its third-party `shade()`) is retired with `tools/MenuProbe.cpp`,
  `tools/MenuCapture.cpp` and `tools/ConvertMenuCapture.py`: the title is now the ordinary world. Nothing of
  `shade()` was kept; the logo's overlay lighting is a stand-in until a spec covers it (`docs/menu.md`).
