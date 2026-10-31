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
