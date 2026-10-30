# Gear in Steam Cube World, as the game itself reports it

Everything here comes from the user's own game, through ZoneExtractor:

- **Ids:** the game's own item ids (category, id, material).
- **Models:** each item paired with the sprite the game draws for it, taken from what creatures were seen wearing (creature tour, F3, `creatures.txt` "equip=").
- **Class rules:** from the game's class check (`0x1094d0`), asked for every category, id and material (F8, `Extract/character/class-rules.txt`). In this install that function is hooked by `CubeModLoader.fip`, which lets installed mods change the answer. The rules below are therefore what the game says with the user's mods loaded.
- **Stats:** what an item adds, measured on the player by equipping carried items in turn (F8 gear survey, `gear.txt`).

Classes are numbered as the game numbers them: 1 Warrior, 2 Ranger, 3 Mage, 4 Rogue.

## Active inventory storage

Native-save characters use `InventorySlots` as their item/equipment authority.
The survey remains a source of measurements/model names for compatibility views;
its flat list no longer reconstructs a native character's saved inventory.
Empty slots and saved extra tabs remain intact until an explicit native refresh.
Refresh sorts/compacts the actual tabs; identification keeps its tab/index binding
and resolves the current record at that pair after a refresh.

Direct left/right and raw-Shift equipment choices follow the independent
[slot specification](specs/steam-inventory-slot-lifecycle.md), including return order,
full item attributes and the native category26 chest/head distinction. Shop transactions
follow [their own action/refresh order](specs/loot-shops.md). The data implementation
uses checked indices across allocation; allocator-dependent native dangling-pointer
outcomes are not treated as a portable behavior to reproduce. Exact auxiliary UI
appearance and broader gameplay comparisons remain on the roadmap.

## Item categories

| Category | Slot | Notes |
|---|---|---|
| 1 | - | Consumables (potions) |
| 2 | - | Formulas |
| 3 | Weapon (right or left hand) | Type by id, below |
| 4 | Chest | Class by material |
| 5 | Hands | Class by material |
| 6 | Feet | Class by material |
| 7 | Shoulder | Class by material |
| 8 | Neck | Amulets (gold 11, silver 12); every class |
| 9 | Ring | Every class (not drawn on the body) |
| 11 | - | Crafting materials |

The game's armor function only gives armor for categories 4-7, and its HP function only gives HP for categories 3-7.

## Weapons (category 3): type by id, class by type

| Id | Type (from the game's sprite) | Class |
|---|---|---|
| 0 | sword | Warrior |
| 1 | axe | Warrior |
| 2 | mace | Warrior |
| 3 | dagger | Rogue |
| 4 | fist | Rogue |
| 5 | longsword | Rogue |
| 6 | bow | Ranger |
| 7 | crossbow | Ranger |
| 8 | boomerang (seen on goblins) | Ranger |
| 10 | staff | Mage |
| 11 | wand | Mage |
| 12 | bracelet | Mage |
| 13 | shield | Warrior |
| 14 | (not seen worn yet) | Ranger |
| 15 | greatsword | Warrior |
| 16 | greataxe | Warrior |
| 17 | greatmace | Warrior |
| 9, 18+ | (not seen worn) | every class |

For each weapon type, a few materials come back from the class check as usable by every class or by none. The iron fist is one of them: by this check, no class may use it. These are probably type and material pairs the game never generates, but that is not confirmed.

## Armor (categories 4-7): class by material

| Class | Materials (id: name where a creature wore it) |
|---|---|
| Warrior | 1 iron, 4 steel, 29 bronze, plus 5, 11, 19, 23 |
| Ranger | 27 linen (worn by the recorded ranger), 31 felt and 33 mail (inferred, see below), plus 20 (probably parrot) |
| Mage | 26 silk, 32 flannell, 35 filament, plus 24 |
| Rogue | 28 cotton, 30 wool, 34 rivet |

Material 6 is NPC clothing (innkeeper chest, `body1`-`body5`). An empty chest slot draws the bare `body4`.

## Models seen

The full list (56 item models: 21 types, 16 materials) is printed in CubeVulkan's log at start-up (`item category ...`). Sprite names follow `<material>-<type><n>[-random<k>]`, for example `iron-greatmace4-random4` or `cotton-chest2-random2`.

## Not done yet

- Ranger materials 31 and 33 are inferred, not seen: the game ships exactly three more 5-sprite armor sets than the other classes use (linen, felt, mail), and every class takes its three sets in the same tier order (warrior 1 iron / 29 bronze / 4 steel, rogue 28 cotton / 30 wool / 34 rivet, mage 26 silk / 32 flannell / 35 filament). So linen is 27, as the recorded ranger wears, felt 31 and mail 33. `ItemCatalog::addInferredRangerArmor` adds them (marked inferred) so they can drop. Material 20 follows the Alpha numbering shifted by one (Alpha 19 parrot), and `parrot-chest` exists; unconfirmed.
- Weapon id 14: no sprite in the game files fits it (Alpha's quiver has none either), so it is most likely unused in Steam Cube World.
- Stats for item types the player doesn't carry: the game removes equipped items the player doesn't own before drawing them, so they cannot be measured by equipping test items.
