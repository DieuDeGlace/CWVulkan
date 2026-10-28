# Replacement-engine readiness checklist

This combines dated development checkpoints. Some older prototype descriptions below have been superseded by the clean-room implementations. See the [2026-10-28 source review checkpoint](release-status.md) for current handoff progress and remaining release checks. A working implementation is not evidence of complete Steam parity. The target remains a Vulkan replacement engine using the owner's installed game data and local extraction tools.

## Latest Claude checklist: death particles

- [x] Locate and record the native fragment container. A bounded list reader replaces the unsuccessful heap scan.
- [x] Record a native death and measure velocity, gravity, delayed starts, spin and lifetime.
- [x] Apply those measured rules in `DeathEffects`, keeping exposed voxel size and original part basis.
- [x] Build and run regression tests against native movement samples; run a Vulkan render smoke check.
- [x] Restore the mods and ReShade set aside for measurement, and preserve the new test character.
- [x] Verify the native matrix convention and rotation composition; correct spin to `Rz * Ry * Rx * originalPartBasis`, with an independent numeric corner regression.
- [ ] Compare lighting and final rendering frame for frame. Current captures do not establish pixel parity.

## Gameplay work before visual polish

- [ ] **Combat rules:**
  - Done (2026-10-22, [combat.md](combat.md)), read from the game and checked hit by hit:
    - the damage formula, critical hits, ability multipliers and the 0.75–1.25 roll;
    - subtractive armor/resistance, and combos that make armor stop counting.
  - Still open:
    - hit timing and ranges, projectiles and knockback (effects, mana shield, lifesteal, charge, stun, poison ticks and the player's blocking are done; monster guarding is not);
    - healing, cancel transitions, and which abilities monsters choose;
    - mana/stamina costs and refill behaviour, which still need wider measurement.
- [ ] **Loot:** every source is specified (`docs/specs/loot-overview.md`). Done from the specs: death drops, rewards and both item generators (`NativeDrop`); shops, their daily stock, the shop tier, buying, selling, buy-back and identifying (`GameShop`, `loot-shops.md`); the recipe table, the crafting lists with the region's rank gear, the craft button's timing, the extra yield and upgrade cubes (`GameCraft`, `loot-crafting.md`); ground items, the hop in place, automatic pick-up of coins within 4 blocks, receiving a pick-up (per-player items +0x88, region counters +0xf8, the receipt dialog and hold-up) and taming's client side (`GroundItems`, `loot-pickup.md`). Coins no longer fly to the player. Still open: an identify window of its own (talking to the identifier identifies the first unidentified item), the upgrade widget, dialog pages that give or take items (no script emits them yet), recipe sources, and checking shops and crafting against recordings (`shop.request`, `recipes.request`, `craftlists.request`). See [progression.md](progression.md).
- [ ] **Creature AI:**
  - Done (2026-10-22, measured in the game, [creatures.md](creatures.md)):
    - detection, chase speed, attack reach and back-to-back swings;
    - give-up distance and the walk home.
  - Still open:
    - more trials for the ranges, fleeing and companions;
    - monster skills and ranged attackers;
    - whether the game ever removes a creature from its dead list (persistence itself is done, 2026-10-22);
    - carrying every kind of drop (resources, food, pets, specials), drawing projectiles, the unported ability branches, cell spots, and the creature update that uses seats and doors.
  - Done (2026-10-25): the melee abilities (0x2c5250's tail and its 0x2c5cf0 branch) in `src/NativeAbility.cpp`. They match 13,780 of 13,786 recorded calls, and the other 6 ask for exactly the game's hits. Monsters' hits now land on the CubeVulkan player through Combat.h's target side of 0x29eb60.
  - Done (2026-10-25): the hit on records (0x29eb60 with 0x29dcf0) in `src/NativeHit.cpp`. It matches 600 of 600 recorded hits byte for byte, and creatures' hits on each other now land.
  - Done (2026-10-25): effect objects (0x2dba40) in `src/NativeEffects.cpp`, plus the shot, arrow and charging branches that create them. They match 400 of 400 arrow steps and 297 of 300 healing-area steps, and 4918 and 4911 ability calls. The player now kills generated creatures through NativeHit (XP, coins, bursts).
  - Done (2026-10-25, [creatures.md](creatures.md) "The game's AI, ported"): the native AI (0x13830 and its behaviours, path search and combat brain) in `src/NativeAI.cpp`. It matches 1091 of 1100 recorded game ticks byte for byte; the rest are network noise. Spawned creatures run it, and the player is one of the AI's creatures.
  - Done (2026-10-25): the creature upkeep (0x2de4c0) in `src/NativeUpkeep.cpp`. It matches 10800 of 10800 recorded calls, and spawned creatures run it, so their abilities end.
  - Optional, off by default: road travel for packs (`[Creatures] RoadTravel=1`).
- [ ] **NPCs and quests:**
  - Done (2026-10-22, [dialogue.md](dialogue.md) "The game's conversations"):
    - the game's own conversation scripts, exported from the game, with its names for people, places and items;
    - played in CubeVulkan, with marked places saved per character;
    - the invented kill quests are off by default (the game never gives them).
  - Still open:
    - scripts are exported for 64 days and played by the world's day (2026-10-22); they also change with the player's gear;
    - capture coverage: the tour now covers the extraction's home and exports scripts as it goes;
    - the places and their items in the world (treasure, witches, portals);
    - map markers, and the other script commands (define_realm/land/ruler/number/model/specialization: names not exported yet);
    - the player's answer choices (tokens ending in ?), e.g. trainers changing specialization.
  - Done (2026-10-27, `src/Missions.cpp`, CODEX-HANDOFF):
    - the region's missions on the map (kind icons, level colours, the solved mark, a blinking "NEW");
    - their completion (0x131620): residents, chests and braziers, mission creatures, carried items;
    - the witch, demon-portal, magic-barrier, flame and mana-pump effects;
    - objects that take an item (the brazier's flame);
    - residents made only once their conditions hold (0x2e17d0): arena stages, camp leaders, tower spiders;
    - activating creatures (0x2c3710 kind 3) and the freed residents' rewards (0x2a7ca0);
    - mission creatures and the tombstone on the map.
  - Still open: the artifacts (planner +0x20) and their lore, the brazier and object transition visuals, the marker pulse, other players on the map.
- [ ] **Progression and world interactions:** vanilla artifacts, region-bound gear behavior, dungeons, treasure, shops, crafting, travel and interactive objects need their native data/rules and working gameplay paths. Optional mod progression must remain distinguishable from vanilla behavior.
- [ ] **Movement and character states:**
  - Done (2026-10-22, [movement.md](movement.md)):
    - A body footprint of ±hitbox/2 (0.48), measured from the game's step-ups. It replaces the centre-point floor/wall checks, so corners no longer clip.
    - The game's walking acceleration, top speed (11.57) and stopping decay.
  - Still to validate: body height and ceilings, square vs round corners, collision with placed objects (trees are sprites, not blocks), slopes, jumping/falling, swimming, climbing, gliding, riding, boats, death and revival.
- [ ] **Equipment and animation coverage:** current recorded classes/subclasses are a starting set. Other weapon variants/grips, transitions, skills and effects need owner-supplied captures and validation.
  - Done (2026-10-24, [animation.md](animation.md)):
    - The game's own pose code (0x17ab0, every state), its ability and item rules, appearance (0x53e80), creature records (0x2e17d0) and worn-item models (0x173b10).
    - All checked bit for bit against recordings of the game.
    - The generated world's spawn points now appear as posed, dressed creatures.
  - Done (2026-10-24, [movement.md](movement.md)): the creatures' physics (0x2bb8e0), exact on 8822 recorded updates; the generated world's creatures run it.
  - Still open: the creatures' AI, state 0x61's sparks, and recordings of more states (other classes and creatures).

## Engine and data work

- [x] **World generation** ([worldgen.md](worldgen.md)): Cube World's generator 0x2ff6f0 reimplemented and checked against the game's own output (`src/worldgen/`, clean-room from `docs/specs/worldgen-*.md`).
  - Done (2026-10-24): terrain, structures, buildings, castles, furniture, static objects, models, items, spawn points, region roles and every reachable encounter; the daily zone refresh (0x31c540). Every block, object, item and spawn point of 518 zones the game generated for the oracle matches, as do all 7615 extracted vanilla zones. Streamed by `[World] Source=Game`, with the 1.5x `[World] Scale` option (default 1, vanilla).
  - Touch-ups: zones loaded in the engine are not refreshed again when the day changes in play; shoppers in the saved world are gone after play (not traced).
- [ ] **Durable state:** validate the full set of game/quest/item/dead-creature state across save/load, class changes and world transitions. The current separate Vulkan progress save is not complete native save compatibility.
- [ ] **Multiplayer:** the game's own protocol and roles ([multiplayer.md](multiplayer.md), docs/specs/netcode-*.md).
  - Done (2026-10-28):
    - A wire layer checked byte for byte against 172,964 recorded packets.
    - The host/guest split: a host world, and a client mirror connected to it by loopback in single player.
    - Interest at 270 blocks, events at 256 blocks, zone requests, the clock and the sleep reset.
    - The J multiplayer menu: friends, invite and uninvite, join and leave.
    - Steam (friends only, reliable channels 0/1), and UDP by address (CubeVulkan only).
    - A dedicated server (`--server [port|steam]`) that keeps world.db.
    - Tested with hidden multi-process runs on one PC.
  - Still open:
    - A real two-account Steam session, with an original-game player.
    - The spec's open questions: the feat payload, the pick-up consumer, quests for a guest, and the arithmetic of a monster's hit on the local player.
    - A chat entry line.
- [ ] **Owner setup:** `CubeVulkanSetup` locates the game and installs/removes the extractor, while the engine checks Steam and the license.
  - Done (2026-10-28, [extraction.md](extraction.md) "First-run wizard"): the first-run wizard. It finds the game through `libraryfolders.vdf`, checks the supported build, CubeModLoader, Steam and the licence, and reports what is extracted and missing in plain words. One button runs the extraction safely: Save copied, Mods and ReShade set aside, the extractor installed alone, requests written, the game started through Steam with in-game steps shown, the extractor's log and files watched, and everything put back once the game has gone. A journal rolls an interrupted run back on the next start. Headless `--setup-check`, `--setup-extract [--dry-run]` and `--setup-restore`; unit tests in a fake game folder (`src/OwnerSetup.*`, `setup/Wizard.cpp`, `tests/OwnerSetupTests.cpp`).
  - Still open: a real first run on the owner's machine with the wizard (not yet done); the optional captures that stay manual (F5/F4 character recordings, F3 creature tour, F8 gear survey, F10 HUD frame); a tested fresh-machine package.
- [ ] **Frontend:** character creation, selection and options work as prototypes. Verify behavior and layouts against native references; menu flyover, transitions and complete input/settings behavior remain unfinished.
- [ ] **Release validation:** test complete gameplay sessions, restart/save recovery, multiple classes/equipment sets, supported game builds and multiple GPUs. Audit the source/release allowlists and dependency notices. Source licensing/provenance review remains tracked in `PUBLICATION.md`.

## Completed reliability steps toward those items

- [x] Provide Steam-gated quick/deep owner-data reports in Setup and `CubeVulkanDataCheck`; report missing captures, corrupt exports and estimated object sizes without generating replacements. See [extraction.md](extraction.md).
- [x] Validate the supported x64 executable before extractor installation; require the loader, a valid extractor DLL and a closed game, and stage replacement before overwriting the old DLL.
- [x] Reject malformed terrain/manifest data, require an actual home zone, clear world caches on reload, and respect explicit sprite IDs in sparse mapping tables.
- [x] Make local progress replacement atomic; preserve prior state on malformed loads, failed replacement, failed Cloud reads and invalid Cloud bundles. Regress empty appearance fields, floating-point round-trips and preserving other character sections.
- [x] Make the streetlight example mod opt-in so default builds do not add its example objects to the world.

These bounded checks do not complete native save compatibility, full-world coverage, fresh-machine setup, or the gameplay items above.

## Extraction boundary

Prefer reading existing models, textures, fonts, sounds, records and decoded runtime data from each owner's installation. Keep those exports local and outside repository/release source packages. No game executable, executable dump, Steam runtime, asset recording or reference screenshot belongs in the source archive.

Extraction does not by itself supply a replacement implementation of every runtime system. When data or behavior is unknown, keep that gap explicit and collect evidence rather than treating an invented rule as native behavior. Only after the core items above are verified would the remaining work be mainly visual polish.
