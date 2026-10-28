# Loot: where every item comes from, and how its fields are chosen

Index and summary for the roadmap's "Loot" item. It lists every way Steam Cube World creates
an item for the player, points to the spec that gives the rules (new ones: `loot-shops.md`,
`loot-crafting.md`, `loot-pickup.md`), summarises how each field of an item is chosen,
records what CubeVulkan implements, and lists the open questions with the recordings that
would settle them. Routines are cited by RVA (image base 0x140000000). Item fields are those
of `drops-and-items.md` §0.1.

Vocabulary: **type** (byte +0x00, "category" in some older specs), **subtype** (+0x04,
"id"), **modifier** (+0x08, a whole rand value or hash, "seed"), **region** (+0x0c, +0x10),
**rarity** (+0x14; the stat rules use it as the item's level, `stat-rules.md` §1.2),
**material** (+0x1c), **attributes** (+0x1d.., count +0x9d: upgrade cubes placed on a weapon,
`loot-crafting.md` §4). This build of the game has no separate item level; Pyro mode's levels
are CubeVulkan's own (`progression.md`).

---

## 1. Every way an item reaches the player

### 1.1 Through a ground item (laid by 0x2a5b70, picked up per `loot-pickup.md`)

| Source | Routine | Spec |
|---|---|---|
| A creature dies: worn item 1/16, pet-slot item 1/50, generated weapon or armour 1/20, fixed item 1/8 (pet food or four set items), type-15 item 1/20 (flagged creatures), coins (hostility 1), race resources, its whole inventory | 0x2a6860 | `drops-and-items.md` §2 |
| A creature with a finished task dies (+0x128 bit 0x100) or is thanked (interaction kind 3): gear per player within 100 blocks, coins, tier resources; on thanks also its inventory | 0x2a7ca0, 0x2c3710 | `drops-and-items.md` §4, `zone-object-interaction.md` §5 |
| **Harvesting**: plants and mineral deposits are creatures (hostility 7; races 0x78–0x82 bushes, cotton, ginseng, cactus, thorn tree; 0x83–0x8b gold, iron, silver, sandstone, emerald, sapphire, ruby, diamond, ice-crystal deposits, names for orientation only). Hitting one until it dies drops 1–3 race resources (plus 1/8 subtype 37 for some plants) | 0x2a6860 §2.3, spawned by 0x3189d6 | `drops-and-items.md` §2.3, `worldgen-creatures-zone.md` §2.8 |
| **Gathering**: herbs, berries, shells, cold-water items and crops lie in the zone as ground items from world generation; taken with the use key | 0x3191b0 / 0x3171a2 | `worldgen-structures-contents.md` C5, `worldgen-creatures-zone.md` §2.7 |
| Objects that take an item (locked doors and chests, braziers, quest-camp objects whose key item — type 0x16 — a guard carries and drops with its inventory): giving the item lays their zone +0x450 contents, if any; types 0x15/0x16 also give a reward | 0x2c3710 kind 5 | `zone-object-interaction.md` §2.6, `worldgen-creatures-roles.md` §3.1 |
| Region-role items (special items, type 0x18) and lying crate rewards (type 30) placed by world generation; per-player items are taken once per character | 0x11a820, 0x316780 | `worldgen-creatures-roles.md` §2, `worldgen-structures-contents.md` C3, C4; `loot-pickup.md` §4 step 3.1 |
| The player drops an item (interaction kind 8) and picks it up again | 0x2c3710 | `zone-object-interaction.md` §7 |

### 1.2 Straight into the inventory

| Source | Routine | Spec |
|---|---|---|
| **Chests** (object 0x16): the zone's chest-content map (zone +0x450, keyed by object index) is filled by world generation — castle, ruin and crate chests (type 30 treasure, or the place's crafting book or artifact), the sunken chest (its draws, not stored). Opening one locally adapts a type-30 item to the class, gives gold for type 31 or adds the item, and marks the cell used | 0x98180 (use key) | `use-prompt.md` §3.7.3; contents: `worldgen-structures-contents.md` C3, `worldgen-creatures.md` §2.3, `lore-and-artifacts.md` §5.1, `worldgen-structures-nature.md` §9; adaptation: `player-item-adaptation.md` |
| **Shops**: weapons, armour, tier resources, general goods (pet food included), bought with gold; selling and buying back | 0xb75d0, 0x2acf90, 0x2ac6e0, 0x2acbd0, 0x2acdd0, 0x94900 | `loot-shops.md` §1–§4 |
| **Identifying** a type-15 item for gold: a random weapon or armour piece of its rarity and region | 0x965e0 | `loot-shops.md` §5 |
| **Crafting**: the known recipes and, per unlocked rank, the region's generated gear; extra yield for resources | 0xb2180, 0xb03d0, 0x8fbc0 | `loot-crafting.md` §1–§3 |
| **Conversations**: a dialog page can give an item, give coins or take an item | 0xb40d0, 0x168120 | `loot-pickup.md` §5.1, `lore-and-artifacts.md` §4.1 |
| **Taming**: the pet item (type 0x14, the race) goes to the pet slot; the old pet item to the inventory | 0xa0750, 0x2ce833 | `loot-pickup.md` §5.2, `creature-abilities-ranged.md` §14.2 |
| **New character**: the starting kit and default recipes | 0x913b0, 0x9afb0 | `frontend-characters.md` §5.5, §5.6 |
| **Missions**: completing one gives nothing by itself; its rewards are the chests, thanked creatures and carried items above | 0x131620 | `conversation-script.md`, Missions in `roadmap.md` |

---

## 2. How each field is chosen

| Source | Type / subtype | Material | Rarity | Modifier | Region | Attributes |
|---|---|---|---|---|---|---|
| Worn / pet-slot / inventory drops | as carried (`worldgen-creatures.md` §4.3, §4.5) | as carried | as carried; raised 0–4 steps (1 in 20 each) and clamped to 4 when reserved for a player who can use it | as carried (generation draws them, §4.3 H) | stamped by the drop layer for the region set: the creature's home region, or the landmark's when its block has flag 0x20 (`drops-and-items.md` §1 step 3) | as carried (generation: none) |
| Generated death gear | weapon or armour generator, class −1 (`drops-and-items.md` §3) | by generator | the creature's level byte, +1 for flags 0x200/0x40000 (if below 4), then the reservation raise | a rand | as above | none |
| Rewards | generator with the player's class | by generator | the tier min(level + 1, 4) | a rand | the reward point's region | none |
| Race resources, fixed items, coins | fixed tables (`drops-and-items.md` §2.2–§2.3) | fixed | fixed (tier stones 1–4) | 0 | coins and resources are not in the region set: none | none |
| Type-15 item | 15/0 | 0 | the creature's level (≥ 1) | a rand | stamped (type 15 is in the region set) | none |
| Chest and crate treasure (type 30) | 30, subtype rand mod 3 | 0 | 4 (castle) / steps-based (ruins) / rand mod 3 + 2 (crates) | a rand | the chest's or place's region (`worldgen-structures-contents.md` C3) | none |
| …adapted when shown or taken | subtype 1 → gold (type 31); else ring, amulet, weapon or armour for the class from X(modifier) (`player-item-adaptation.md`) | by class and X | kept | kept | kept (gold: none) | none |
| Shops' gear | generator with the player's class | by generator | 0 .. the shop tier (solved role-1 missions of the region) | a fresh rand | the shopkeeper's home region | none |
| Identified gear | generator, class −1 | by generator | the type-15 item's | the generator's | the type-15 item's | none |
| Crafted known recipes | the recipe item | the recipe's | the recipe's | the recipe's | the player's current region when crafting starts | none |
| Crafted rank gear | the class's 3 weapons, 4 armour pieces, 2 jewellery per rank | by class / rand | the rank 0–4 | a rand from the region-seeded sequence | the player's region (`loot-crafting.md` §2.2) | none |
| Starting kit | by class | by class | 0 | a rand from the account seed | the player's region | none |
| Upgrading | — | — | — | — | — | cubes placed on a weapon, up to 16 (32 for 2H) (`loot-crafting.md` §4) |
| Taming | pet item 0x14 / race | 0 | 0 | 0 | 0 | none |

**Region binding** (what the region does): an item of type 3..9 or 0x1a counts at its rarity
only in its own region, and in the eight neighbours when its modifier is a multiple of
(rarity + 1) × 10; elsewhere its level is −1 (`stat-rules.md` §1.2). The same modifier test
doubles its price (`stat-rules.md` §2.7).

**Stats** follow from type, subtype, material, rarity, modifier, region and attribute count
alone (`stat-rules.md` §1–§2); no item stores a stat.

---

## 3. Coins and items reaching the player

There is no flight. A drop is laid with a pick-up delay of 500..799 ms and drawn hopping five
blocks high (a 500 ms parabola) and tumbling while the delay runs; when it reaches 0 a landing
sound plays, and coins (and type 14) within 4 blocks of the player are picked up at once. Any
other item waits for the use key. The picker's peer then adapts the item, adds it (gold to the
counter) and announces it. Details: `loot-pickup.md` §1–§4. This also explains the recorded
arrival time of coins (0.59–0.89 s after a death).

---

## 4. What CubeVulkan implements (grep of src/ and tests/, 2026-10-27)

### 4.1 Implemented from a spec and checked

| Rules | Where | Check |
|---|---|---|
| Death drops, the drop layer, both generators, pet food, rewards (0x2a6860, 0x2a5b70, 0x2d15d0, 0x2d0680, 0x2d0fc0, 0x2a7ca0) | `src/NativeDrop.*`, called from `src/NativeHit.cpp`, `src/Spawned.*` | drops.bin, items.bin oracles (`drops-and-items.md` §7) |
| Who can use an item (0x1094d0), item level, power, price, sell price (0x109720, 0x109c50, 0x109d30, 0x10a390) | `src/NativeRules.*` | rulesoracle, playerstats |
| Class adaptation of treasure (0x10a720) | `src/NativePlayer.cpp` | unit tests |
| Interactions 5, 7, 8, 10 (objects that take items, picking up, dropping, the pet) and kind 3's reward | `src/NativeInteract.*` | unit tests |
| The use prompt: chests, automatic pick-up, "Pick up" | `src/UsePrompt.*` | spec tests |
| Inventory add and tabs for generated creatures (0x4e810, 0x5f100) | `src/worldgen/Spawns.h`, `src/UsePrompt.h` | gear.bin |
| World-generated items, chests' contents, crates, role items, herbs, resource creatures | `src/worldgen/` | the oracle zones |
| Starting kit and default recipes (0x913b0, 0x9afb0) | `src/Frontend*` | — |
| Mission completion (0x131620) | `src/Missions.*` | EngineTests |
| The drop hop and the landing sounds | `src/main.cpp` (`dropPlacement`, the loot loop) | — |

### 4.2 Present without a clean-room spec (to redo from the new specs)

* **Shops** — `src/GameShop.*` (stock by class, tier, prices, bought list). It cites no RVA and
  no spec. Re-implement from `loot-shops.md` §1–§4 and check against shop.request.
* **Crafting** — `src/GameCraft.*` (recipes, starting recipes, tabs, rank gear, extra
  pieces). Same: no RVA, no spec. Re-implement from `loot-crafting.md` §1–§3 and check against
  recipes.request and craftlists.request.
* `src/RaceKeys.inc` says it was "generated from the reconstruction's table" (race names as
  dictionary keys). The keys are game data, but the file's provenance note should be checked
  under `CLEANROOM.md`.

### 4.3 Not implemented

* **Coin "flight"**: `src/main.cpp` makes coins fly to the killer (`deathFx.coins`) and credits
  them at once. The game has no flight (§3): coins should be ordinary ground items, taken
  automatically within 4 blocks once their delay ends, and left on the ground otherwise.
* **Receiving a pick-up the game's way** (0xa5a50 notice walk): per-player items keyed by block
  (+0x88), the per-region counter for type 12 material 13 (+0xf8), the money value of type 31,
  the receipt dialog and hold-up for special items, the used-cell mark for type 30. CubeVulkan
  keeps its own bag and gear lists (`progression.md` "Loot").
* **Identify window** (0x965e0).
* **Conversation give/take** (page fields +0xd4/+0xd8 and +0x30/+0x34).
* **Taming's client side** (0xa0750: pet slot, pet id, consuming the food). The authoritative
  side is in `src/NativeAbility.cpp` (0x2ce833).
* **Upgrade cubes** (0x292650, 0x108b90, 0x10a0e0) and the weapon damage they feed.
* **Ground item update**: wall items (type 0x11), timer +0xcc.

(Selling and a sold-items list exist in `src/main.cpp`, `soldItems`, with the shop code of
§4.2; check them against `loot-shops.md` §4.2–§4.3 when redoing it.)

---

## 5. Open questions and the recordings that settle them

tools/ZoneExtractor already has `drops.request` (0x2a6860 deaths, CDR1) and
`items.request` (generators on scripted rand, CIG1). New requests, in the same style (a file
in `Extract\` holding a count, results in `Extract\oracle\`, read-only for the game):

| # | Question | Oracle |
|---|---|---|
| 1 | Shop stock, tier and the bought list (`loot-shops.md` §1–§2) | **shop.request**: per loaded shopkeeper (class 0x80–0x82, 0x9c), today and the next N days: id, home, day, tier, player class, bought list, the stock list and every rand |
| 2 | Buy-back details (`loot-shops.md` §8.2) | **watch** the controller's +0x21b0 list and the inventory while buying back |
| 3 | Identify (`loot-shops.md` §5) | hook in 0x965e0: the type-15 item, price, gold, rand values, the item added |
| 4 | The recipe table (`loot-crafting.md` §1) | **recipes.request**: 0x2aec30 on scripted items, dump result, station, ingredients |
| 5 | Crafting lists and rank gear (`loot-crafting.md` §2) | **craftlists.request**: 0xb2180 for the loaded character; tabs 0–6, region, class, solved set, rand values |
| 6 | Crafting itself, extra yield, the stack-consumption quirk, release behaviour (`loot-crafting.md` §3, §7.3–7.4) | hook 0x8fbc0 and 0xb03d0: recipe, inventory before/after, progress and speed per frame, rand values |
| 7 | Tab 6 and in-place crafting; recipe sources (books) (`loot-crafting.md` §7.1–7.2) | watch controller +0x17a8 tab 6, +0x1938, +0x1948 and player data +0x18 while using a crafting book |
| 8 | Upgrade palette and caps (`loot-crafting.md` §4, §7.5) | watch the upgrade widget's +0x1f8 palette and the weapon's attribute entries while placing cubes |
| 9 | No coin flight; the hop below ground; drawing details (`loot-pickup.md` §2, §8.1, §8.7) | **ground.request**: ground items of nearby zones every tick (position, +0xc0..+0xd8) with the player's position, plus frame captures |
| 10 | The pick-up consumer, +0x88 and +0xf8 (`loot-pickup.md` §4, §8.2–8.3) | **pickup.request**: each notice and the player's gold, inventory, +0x88, +0xf8 before and after |
| 11 | Conversation give/take operations (`loot-pickup.md` §8.4) | dump dialog pages' +0x30/+0x34/+0xd4/+0xd8 in the talk export when a page carries an item; trace the emitters |
| 12 | Taming's client side (`loot-pickup.md` §5.2) | an existing hit/watch recording of a tame: pet slot, pet id, inventory |
| 13 | Rewards in the drop oracle (`drops-and-items.md` §7.1 notes rewards are absent) | extend **drops.request** to also record 0x2a7ca0 calls (tier, position, every 0x2a5b70 call) |
| 14 | Who fills the world's pet lists (+0x360, +0x378) (`drops-and-items.md` §8.2) | already exported by items.request; tracing is enough |
| 15 | Resource creatures' identities (§1.1) | the creature tour's race list with their models; the drop oracle on a deposit kill |
