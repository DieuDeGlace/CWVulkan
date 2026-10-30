# Progression, loot and the gameplay loop

## Two modes, switchable

Set the mode with `[Progression] Mode=pyro|vanilla` in `CubeVulkan.ini`, or with **Options → PROGRESSION** (also in the pause menu, where it takes effect at once). With no setting, CubeVulkan follows the owner's game: `pyro` when a `PyroProgression*.dll` is installed in its `Mods` folder, otherwise `vanilla`.

| | vanilla | pyro |
|---|---|---|
| XP bar | hidden, as in Steam Cube World | fills, with "XP/needed" (the mod's purple) |
| Levels | none: gear and artifacts are the progression | XP from kills; each level adds HP, armor, attack and crit |
| Enemy plates | no levels (unless `[Interface] ShowLevels=1`) | the enemy's level |
| Enemy strength | as recorded | follows the player's level by region type |
| Dropped gear | rarity sets its strength | level and rarity set its strength; gear above your level cannot be equipped |

The rules are pure functions in `src/Progression.h` and are checked by `EngineTests`.

## Pyro mode: PyroProgression v1.4, "relative" variant

This is the variant installed in the owner's game. Its formulas are re-implemented in CubeVulkan; the mod's source is in `Documents/ChatGPT/CubeWorldBackups/PyroProgression-relative-20261009`.

### XP and levels

- XP needed for the next level is `50 + 4 × level`. The mod puts this in place of the game's own function at RVA `0x5fa80`, and the game showed 0/54 at level 1.
- XP per kill is the creature's stars + 1. Against a weaker enemy it is multiplied by `((enemy + 1) / player)³`; against an equal or stronger one by `(1 + diff/6) · 1.05^diff`, with the difference capped at 20 levels. Bosses give ×10, named bosses ×5 and mini-bosses ×2.
- A level up subtracts the XP needed, restores full health, shows a purple "LEVEL UP!" and re-levels the enemies.
- Each level beyond the recorded one adds +1 HP, +0.2 armor, +0.2 attack and +0.0001 crit.

### Enemies

- Difficulty regions and their biome tiers remain. An enemy's level uses the active regional baseline (or the player's level in relative mode):

  | Region types | Enemy level |
  |---|---|
  | Easy: plains, hills, woodlands, ocean, wetlands, mountains | 90–98% of the baseline |
  | Moderate: dark woods, snow forest, savannah, snowlands | 108–116% |
  | Hard: jungle, desert, deadlands, lava | 125–133% |

- Each creature also varies by ±1 level.
- A region's roll comes from the world seed and the 1024-block region cell, so it stays the same across restarts.
- Recorded creature stats were taken beside a level-1 player. They are scaled by `1 + 1000·log2((L+1001)/1000)` between that level and the new one.
- **The game's own creatures** (`src/PyroCreatures.*`), as the mod hooks them:
  - Level: the mod's `RelativeCreatureLevel`. A player or pet keeps its own level (+0x144). Any other creature uses its region (+0x150, or 0x2ada20 of its block), that region's biome (0x2b9080), the roll from seed 123 and the region, and its id.
  - Stats: the mod's `OnCreature{HP,Armor,Resistance,AttackPower,SpellPower}Calculated` hooks, through `nativerules::addCallbacks`. Each stat is multiplied by `0.05 · (1 + k) · (1 + 1000·log2((L+1001)/1000))`, with k = 2 for health, 3 for attack and spell power, and 0.01 for armor and resistance.
  - A level up keeps every creature's share of its health (`SpawnedCreatures::keepHealthShares`).
  - Kills give XP by that level.
  - Life bars read "LV n" for every creature (the mod's display hook at 0xb1966; "K"/"M" past a thousand and a million).
  - OverworldEvents takes its enemies' levels from it (PyroProgressionCreatureLevel).
  - CubeVulkan's optional shared regional difficulty (`[Progression] RegionScaling=first-player`, the default in Pyro mode):
    - The first player entering a region sets its level baseline. Existing biome/seed/id variation still produces weaker and stronger enemies around it.
    - Other players share that baseline. Leveling in place, or the first player leaving while others remain, does not retune it.
    - When the last player leaves or disconnects, the baseline is discarded. The next entrant chooses a fresh level. Unoccupied/unloaded regions retain no discovery lock or persistent difficulty history.
    - The host decides arrival order and broadcasts its active baselines to negotiated CubeVulkan clients. Single player uses the same host path. Original Steam clients receive no extension messages.
    - `RegionScaling=relative` retains player-relative scaling. Legacy `LockDiscoveredRegions` and imported `R` discovery entries no longer affect enemy difficulty. Existing `G` gear entries remain supported.
    - This is a requested gameplay option, not a claim of vanilla Steam parity. Vanilla mode keeps its original progression rules.
  - Gear levels (the mod's `RelativeItemLevel`; `G <key> <level>` lines in the same state files):
    - A piece of gear (categories 3–9) keeps the first level it is given. The key mixes the same identity with category, id, modifier and material, so identical items share a level.
    - On the first session, gear the character owns gets the mod's old level: `1 + 5·region distance + rand(modifier) % 5`.
    - A drop (the mod's hook on the game's drop, 0x2a5b70) gets `lootLevel(player, enemy, key)`.
    - `EngineTests pyrogear <file> <name> <character dir>` finds the owner's recorded rogue's 3 gear items in the game's file, which confirms the key.

### Gear

- A dropped item's level is the player's level minus 0–2. A stronger enemy has a one-in-three chance of dropping an item above the player's level.
- Gear the character already owned uses the mod's level for owned gear: `1 + 5·distance + rand(modifier) % 5`.

## Vanilla mode

There is no XP or levelling. Recorded combat numbers are used as they are.

The recordings were made with PyroProgression installed, so they carry its level-1 adjustments. Vanilla's own stat formulas have not been measured, so they are not guessed at. Recording characters and creatures with the mod disabled would give exact vanilla numbers.

Steam's **artifact** system (artifacts raise climbing, swimming, gliding, sailing, riding and lamp radius) is **not implemented yet**. It needs dungeons and treasure, which CubeVulkan does not have yet.

## Deaths and coins (recorded from the game)

Recorded 2026-10-21 in unmodded Steam Cube World (all other mods and ReShade set aside), with the extractor's death probe in fight mode. Nine deaths: 0-, 1- and 2-star monsters (races 47, 98 and 100).

| | The game | CubeVulkan (`src/DeathEffects.h`) |
|---|---|---|
| The body | Gone the moment HP reaches 0; each sprite emits a random half of its exposed voxels | Uses the owner's posed sprites, exposed voxels, original voxel size and part transform |
| The creature | Held in ability 1, then removed 969–1031 ms later (2-star golems: 422–859 ms) | Not drawn once dead |
| Fragments | Delayed starts, outward velocity from each part's centre, gravity and point collisions; disappear at age 5000 ms | Uses the measured rules below; the earlier speed, gravity and shrinking estimates have been removed |
| Gold | stars + 1 coins of 1 gold, which fly to the player and count 0.59–0.89 s after the death | Same, with the game's `gold-coin` sprite. Pyro mode keeps its amount (the enemy's level), split between the coins |
| No player hit | A death without a player's hit (HP forced to 0) pays nothing | Kills only come from players |

Body-particle rules were subsequently checked in the owner's supported Steam build (PE timestamp `5d93ab0d`) and against a read-only recording of a test-character death. The creation, update and draw routines use the same body particles for creatures and the player. The independent implementation is in `src/DeathEffects.cpp`:

- Only solid voxels with an empty neighbour on at least one of six sides are candidates. Each is kept with probability one half. A fragment keeps its sprite colour and original basis; sampling does not enlarge it.
- Each sprite uses its own transformed centre. Initial velocity is twice the displacement from that centre, in blocks per second. There is no additional upward kick. Positions are quantized to 1/65536 block.
- Initial age is `-(rand() % 1000)` ms. A particle is visible but stationary while its updated age is negative. Each spin component lies between -50 and +50 degrees per second.
- Once active, velocity Z decreases by 30 blocks/s², then the particle moves with that updated velocity. Movement is split into at most ten substeps; each axis is quantized separately. Solid-block contact reverts that axis step and clears that velocity component. Z contact clears all velocity and spin, leaving the particle at its preceding point.
- Body particles disappear when age exceeds 5000 ms, without shrinking or fading. The initial delay makes their total time since emission approximately five to six seconds. Native update time is integer milliseconds.
- The native list retains at most 1000 particles across particle types. The engine currently applies that shared limit to the body fragments it implements.
- Native draw applies `translation * rotationZ * rotationY * rotationX * originalPartBasis`. The engine now applies spin in this order, preserving the original part's scale and reflection. A numeric corner regression distinguishes it from the previous local-axis spin.

The recording contains 96,638 samples. Among 5082 selected falling pairs under 100 ms, median inferred gravity was 30.000001 blocks/s²; 5012 were within 0.001 of 30. Racing reads and collision transitions are excluded from the regression pair used by `EngineTests`. Tests also check delayed starts, wall/floor collisions, exposed-voxel selection, original size, reflected transforms, rotation composition and lifetime. Vulkan smoke captures confirm rendering, not pixel equality with Steam. The random generator/sequence and lighting still need a frame-for-frame comparison. Coin count and arrival observations above are unchanged; the coin flight arc remains an approximation.

For future captures, `tools/RecordDeathParticles.py --pid PID --game "D:/.../Cube World" --seconds 120` reads only the decoded body-particle list. Load a test character first; either a creature death or test-character death supplies samples. It checks the exact installed executable path and supported build and opens the process for query/read only. It sends no input, injects nothing and writes no native saves. Omit `--seconds` for a state check. Output stays under the owner's private `Extract/death/particles-*.jsonl` directory. The extractor's fight-mode recorder also reads that bounded list; the unsuccessful multi-gigabyte heap scan has been removed.

Earlier raw data remains in `Extract\death\` (`d_<race>_<id>.cwa`: the dying body's pose every tick; `death-log.txt`; `drops-log.txt`) and `Extract\death\frame-*.bmp` (the initial 2026-10-21 set is in `frames-2026-10-21`). These exports and reference images are excluded from the source archive.

Probe keys (fight mode, F2):
- **N**: noon, set on the local server's clock.
- **H**: 100 blocks ahead, or to `[Hunt] X, Y`.
- **J**: turn the camera 90°.
- **F**: go to the weakest monster.
- **V**: hold the target at 0.01 HP for 6 s, so the next hit kills.
- **Z**: set the target's HP to 0.
- **U**: take the target's weapons.
- **L**: player level from `[Record] Level`.

N, V, Z, U and L change the running game, so back up `Save` first and restore it afterwards.

## Loot (both modes)

Gold comes as coins (above). Gear comes in a pouch, drawn with the game's `pouch1` sprite. Walking over the pouch picks it up.

- **Gold counter:** at the top left, with the game's `gold-coin` sprite.
- **Worn gear:** selected from the creature's recorded equipment, preserving its type, material and rarity. The new extractor also records its modifier; old captures without one retain an explicitly reported modifier estimate. Names, models and icons use the owner's existing sprites/catalogue. Item stats still use a scaled gear-survey reference, and unsupported slots are skipped.
- **Player stats (vanilla mode):** the game's own rules on a player record (`src/NativePlayer.cpp`): maximum health, attack, armor, resistance and critical chance. They match the live game exactly (`EngineTests playerstats`). An item counts fully only in its own region, so stats change when the player crosses regions. Pyro mode keeps its own progression.
- **The bag:** drops that are not gear (resources, food, pets, specials) stack in a bag. It is shown in the inventory and saved; using its items is not ported yet.
- **Generated gear:** for the game's generated creatures ([World] Source=Game), drops are the game's own: 0x2a6860 with its generators, ported in `src/NativeDrop.cpp` and verified byte for byte (see creatures.md). The recorded actors still use the prototype (catalogue type/material, stars ±1 rarity). Stats still come from the survey for both.

### Item drop decisions checked in the supported Steam build

The drop-decision routine is at RVA `0x2a6860`; `0x2a5b70` places an already chosen item. The following subset is independently expressed in `src/NativeLoot.h`:

| Condition | Decision |
|---|---|
| Hostility 0 or 3; or appearance flag `0x800` | No item drops through this routine |
| Eligible worn items exist | `rand()%16 == 0`, or appearance flag `0x200`, selects one |
| Candidate order | Left weapon, right weapon, chest, shoulder, hands, feet, neck |
| Candidate exclusions | Empty slots, weapon IDs 22/23, chest ID 1 |
| Pet slot is nonempty | `rand()%50 == 0` requests its item; pet inventory/stat support is still missing |
| Hostility 1 | `rand()%20 == 0` or appearance flags `0x40200` requests generated gear |
| Hostility 7 | `rand()%20 == 0` requests generated gear |

The tests exhaust all 32,768 native random values and verify suppression/override flags, exclusions and slot order. Modulo bias is retained: `%20 == 0` accepts 1,639 values and `%50 == 0` accepts 656. The engine uses an independent random sequence, so matching individual kills is not claimed.

New F3 manifests include `flags2=<hex>` and `equip=category:id:material:rarity:modifier,...`. Four-field equipment exports remain readable; absent flags and modifiers remain marked unknown. Existing exports are not rewritten. The engine log and deep data report identify legacy captures that still use clear-flags/modifier estimates. Do not mistake those estimates for measured zero flags.

This subset replaces the old universal 30% chance. Race-specific resources, carried inventory drops, a third generator, native generated item contents/region binding/spirit data, and complete item stats remain unfinished. Worn items whose models or survey references are unavailable are skipped instead of inventing replacement assets. Native visual drop/pickup effects also need further comparison. Enemy death persistence is now configurable: timed respawns default to 300 seconds, instance deaths last until the original spawn zone unloads for every player, and permanent deaths remain available. These respawn policies are an explicit project option; the recorded native world kept a growing dead-creature list.

## Saving

Level, XP, gold and found items are saved with each character, locally and in Steam Cloud (see `saves.md`).

Found items are stored by their game ids, level and slot; their stats are rebuilt from the survey reference whenever they load, so switching modes re-scales them.

## Gameplay loop now

1. Pick a character on the title screen.
2. Fight monsters with the class's own moves.
3. Earn XP and level up (pyro).
4. Collect gold and gear pouches, and equip items in the inventory (B), subject to class and level rules.
5. Die and respawn at home with full health; XP, gold and items are kept.

Dead monsters stay dead, as in the game ([creatures.md](creatures.md)). Progress is saved every minute and on exit.

### Still to do

- Stamina refill and held-skill costs (measure in game).
- Artifacts (vanilla).
- Quests.
- Shops for spending gold.
- Recorded drop tables.
- Crafting.

## Mana and stamina

Measured with the extractor's fight mode (F2), which logs MP, stamina and charge changes with the ability that caused them (`Extract/combat-log-*.txt`, ranger and water mage, 2026-10-21):

| Rule | Value |
|---|---|
| Landed basic hit (warrior, ranger, rogue) | +0.2 MP |
| MP drain (warrior, ranger, rogue) | −0.05 MP per second |
| Mage regeneration | +0.1 MP per second; hits add nothing |
| Ranger salvo release (56) | spends all MP |
| Water mage special (34) | 0.3 MP |

From the game's on-screen skill list:

| Move | Cost |
|---|---|
| Dodge | 25 stamina |
| Smash, Retreat, Poison Vial | 100 stamina |
| Teleport | 100 mana |
| Ambush, Intercept | all MP (needs some) |

Not measured yet:
- Stamina refill (assumed 4 s from empty).
- The held skills (Cyclone, Sneak, Float, Sprint), charged 25 stamina when they start.
- Fire and other classes' specials, which use the water and ranger values.

A skill that cannot be paid for is refused (logged as "Not enough mana/stamina").

The MP bar shows mana out of 100. A small blue stamina bar appears under the character while stamina is below full, as in the game.

The XP and MP bars were captured empty, so their fill colours are derived from each bar's own text colour (purple and cyan) until frames with them filled are captured.
