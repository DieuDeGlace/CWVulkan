# Lore, tales and artifacts

Behavioural specification of how Steam Cube World handles examining lore objects, the tales
they tell, the lore a character collects per realm, the artifacts the region planner hides,
how lore reveals them, and what carried artifacts do. Written from the decompiled executable
and the binary's constants. Routines are cited by RVA (image base 0x140000000). Offsets are
byte offsets into the record named.

This document builds on `conversation-script.md` (same folder). Its conventions are used
unchanged and are only summarised here:

* **X(n)** — the 32-bit xorshift step (conversation-script §2.1). "odd(n)" means X(n) is odd.
  Tests of the low 1-5 bits may skip the last shift; the result is the same.
* **N(n), R(n)** — the integer noise and R(n) = 100000 − floor(N(n) / 21474)
  (conversation-script §2.4; CubeVulkan `gamegen::hash`, `gamegen::rnd`). R is never
  negative, so "R(...) mod m" is an ordinary remainder. Test vectors: X(1) = 270369,
  X(12345) = 3336926330, R(0) = 35908.
* **floor division** — every block → cell → place conversion below rounds toward minus
  infinity.
* **realmKind(R), rulerRace(R), rulerTitle(R), nationModel(R, n), magic(R), goodTrait(n),
  badTrait(n)** — conversation-script §5.4 helpers (0x10cee0, 0x10f320, 0x10ec60, 0x10eb10,
  0x10e6f0, 0x10d000, 0x10bbd0). Note that goodTrait and badTrait take a **number**, not a
  realm: goodTrait(n) = good-trait table [X(n + 0x20b3) mod 10], badTrait(n) = bad-trait
  table [X(n + 0x140e) mod 9]. The realm stories pass n = realm + 0x1e2e; the tombstone tale
  passes a counter (§3.5).
* **year(R)** (0x10ead0) = (X(R + 0x13309) mod 9000) + 100. It is the same number the land
  history prints.
* **faction(R)** = (X(R + 0x18365) AND 7) + 1, the argument of every `define_faction` below.

Three cell sizes appear:

| Name | Size | From a block coordinate b |
|---|---|---|
| **cell** (object cell) | 16 blocks | floor(b / 16) |
| **place** (place cell) | 1024 blocks | floor(b / 1024) |
| **region** | 8192 blocks | `gamegen::regionAt` (0x2ada20) |

The place of a cell c is floor(16·c / 1024), i.e. the place that contains the cell's
corner block.

---

## 1. Records and fields used

### 1.1 Player data (creature +0x1528, a pointer; "mission data" in conversation-script)

| Offset | Type | Meaning |
|---|---|---|
| +0x38 | set of (x, y) | places discovered (a talk `mark_location` also adds here) |
| +0xa8 | set of (x, y) | **used cells**: 16-block cells the character has used. Opened reward chests and read lore cells both go here (CubeVulkan's `SaveGame::openedChests` is this set for chests). |
| +0xb8 | set of (x, y) | places **known** (marked on the map) |
| +0xc8, +0xd8 | sets | solved / finished missions (conversation-script §1.2) |
| +0xe8 | map (x, y) → f32 | "NEW" timers on the map, in ms |
| **+0x128** | map u32 → set of (x, y) | **lore by realm**: realm → the 16-block cells of the lore buildings read in that realm. Ordered by realm (unsigned); the set's element count is what the game reads. |

All sets are ordered (x first, then y, signed).

### 1.2 Creature record fields

| Offset | Type | Use here |
|---|---|---|
| +0x5c | u32 | movement flags: bit 0x02 in water; bit 0x10 must be clear for the diving drain (its meaning was not traced) |
| +0x60 | byte | hostility kind: 0 player, 3 townsperson, 8 object-creature |
| +0x64 | i32 | race |
| +0x68 | byte | current ability (0x79 = 121: sailing a boat) |
| +0x80 | u32 | appearance flags (0x400000 petrified, 0x40200 used by climbing/swimming) |
| +0x84 | f32 | size factor |
| +0x128 | u16 | toggles; bit 0x0001 = lamp lit |
| +0x144 | i32 | **level** |
| +0x194 | f32 | ground-speed factor (multiplies the move speed) |
| +0x198 | f32 | **light radius** |
| +0x9a0 | f32 | stamina, 0..1 |
| +0x9f0 | vector | inventory: tabs of 0x18 bytes, each a vector of 0xa4-byte stacks (i32 count, then the 0xa0-byte item) |
| **+0xb5c .. +0xb74** | 7 × i32 | **artifact skill counters**, index 0..6 (§5.2) |

### 1.3 Item record

As in conversation-script §1.2: +0x00 category (byte), +0x04 id, +0x08 modifier (u32),
+0x0c/+0x10 region x/y, +0x14 rarity (byte), +0x18 the "formula" field (for an artifact: its
**realm**), +0x1c material (byte), spirits, +0x9d spirit count.

### 1.4 Static object record (0xe0 bytes; zone list at zone+0x60)

| Offset | Meaning |
|---|---|
| +0x00 | type (i32) |
| +0x08, +0x10, +0x18 | position x, y, z (i64, 1/65536 block) |
| +0x28 | "active" byte |
| +0x30 | owner creature id (i64) |
| +0x38 | the item it takes (0xa0-byte item) |
| +0xd8 | how many of that item it takes (byte) |

The use prompt works on one targeted static object, held by the game controller as zone x
(+0x1e7c), zone y (+0x1e80) and index in that zone's list (+0x1e84); 0x2c3150 resolves it.

---

## 2. Examining (the use prompt, 0x98180)

0x98180 runs every frame. It shows one prompt (label, category "info") and, on the frame the
use key goes down, performs it. The checks run in order; the first that applies wins:

1. Player states (falling, climbing, revive, throw) — not part of this spec.
2. The targeted **creature**, if any (§2.4).
3. The targeted **static object**: first its item (§2.1), then its type (§2.2).

### 2.1 An object with an item it takes (checked first)

Applies when the object's item category (+0x38) is not 0 **and** its active byte (+0x28)
is not 0. The type does not matter and no Examine is offered while this applies.

1. carried = the player carries at least +0xd8 of the item (0x5e780, items compared as in
   0x4df60).
2. Prompt text: the item's name (0x1628b0). If the count (+0xd8) is 2 or more it is
   "*count* x *name*" (the separator is the wide string " x " at 0x437c70).
3. **Not carried:** the prompt is drawn in red (1.0, 0.3, 0.3, 1.0; 0x4380a0). Using it does
   nothing.
4. **Carried**, on use:
   1. If the object type is not 9: remove that many of the item from the player (0x648b0).
   2. Play sound 0x3e ("drop-item.wav") at the listener, volume and pitch 1 (0xa03f0).
   3. Queue a pending interaction of kind 5 on the player (+0xab8) carrying the object's zone
      x, zone y and index. (Its handling — the object turning inactive — is outside this
      spec.)

A spent object (active byte 0, e.g. a lit brazier after its flame was given) or one with no
item falls through to §2.2.

### 2.2 Static object types that offer "Examine"

Switch on the type (+0x00):

| Type | Prompt | What it is (where placed) |
|---|---|---|
| 0x76 (118) | Examine | lore object of cell objects 13, 69, 70 (1 in 4) |
| 0x77..0x7c (119..124) | Examine | lore objects of cell objects 13, 69, 70 (0x77 + X(style + 0xda5) mod 6) |
| 0x7d (125) | Examine | lore object of cell object 55 |
| 0x7e (126) | Examine | the role-22 keepers' brazier (per CubeVulkan's generator, 0x11a820); it takes an item first, §2.1 |
| 0x82 (130) | Examine | lore object of cell object 37 (the tombstone object) |
| 0 | none | special item 4 is always "owned" (0x5e890 returns true for 4), so the routine returns with no prompt. Its "gift_of_the_goddess" branch is unreachable. |

The placement column is from the zone generator 0x2ff6f0 (around 0x316d5d: lore building →
0x7d for cell object 55, 0x82 for 37, otherwise 0x76 or 0x77..0x7c). All eight types behave
identically on use: §2.3. There is no distance or owner check in this branch; distance is part
of choosing the targeted object.

### 2.3 On use: finding the lore cell and telling the tale

Let (x, y) = the **cell** of the object's position: floor(+0x08 / 65536 / 16),
floor(+0x10 / 65536 / 16) (routine 0x137e0, floor at both steps).

**Lore building test** (0x2b42e0): a cell is a lore building when its cell object
(`gamegen::cellObject`, 0x69c60) is one of **13, 37, 55, 69, 70**. (The binary tests
cellObject − 13 < 58 against the mask 0x300040001000001.)

Steps:

1. r = 0 if (x, y) itself is a lore building, otherwise r = 1.
2. For dx = −r .. r (outer loop), for dy = −r .. r (inner loop), with the cell
   C = (x + dx, y + dy):
   1. If C is not a lore building, go to the next dy.
   2. Build the tale of C (§3) and show it in the dialog window (0x1694e0) with seed
      R(C.x + 57·C.y) and an empty speaker name. The pages are appended to the dialog's page
      list (dialog = controller+0x418, list at dialog+0x1e8, pages of 0x1b0 bytes).
   3. If the page list is now not empty, attach C to the **last** page's lore list
      (page+0x190) and stop.
   4. Otherwise (the tale was empty) leave the inner loop and continue with the next dx.
3. After an inner loop ends without a match, the routine also stops if the page list is not
   empty; otherwise it continues with the next dx.

So for an object outside a lore building the order is (−1,−1), (−1,0), (−1,1), (0,−1),
(0,0), (0,1), (1,−1), (1,0), (1,1), and the first lore building wins. Nothing is recorded yet:
lore is credited when the last page is shown (§4.1).

### 2.4 Creature prompts that also say "Examine" (not lore)

For completeness; they record no lore.

* A townsperson (hostility 3) with appearance flag 0x400000 (petrified), within the talk
  distance (0x5ec30, squared): on use the dialog shows `define_model` *race*,
  `he_is_petrified`, seed = creature id mod 100000.
* A hostility-8 creature within (size factor +0x84 × 0.5 + 5.0) blocks (0x4ac964, 0x42e584):
  on use a pending interaction of kind 3 with the creature's id is queued (0x98040). Its
  effect was not traced.

---

## 3. The tale of a lore cell (0x2d4290)

Input: a lore cell C = (cx, cy). Output: a token script (the same token language as
conversation-script §7). Let n = cx + 57·cy (32-bit wrapping) and s = R(n).

1. P = the place of C: px = floor(16·cx / 1024), py = floor(16·cy / 1024).
2. **realm** = realmOf(P) (§4.3; 0x2bb060 with bigCell(P)).
3. If cellObject(C) = 37: the **tombstone tale** (§3.5) with realm and seed s. Done.
4. Otherwise, if R(n + 0x9b084) is odd, try the **artifact tale**:
   1. (rx, ry) = the region of the block point (16·cx + 8, 16·cy + 8).
   2. rx += (R(n + 0x5a278) mod 3) − 1; ry += (R(n + 0x7d1b4) mod 3) − 1.
   3. Build the region planner of (rx, ry) (0x115df0, §5.1).
   4. Pick T (0x129e10): the planner's artifact-place list (role 14 cells in role-map order,
      conversation-script §1.5 +0x48), keeping only the cells with realmOf(cell) = realm. If
      none: go to step 5. Otherwise T = kept[X(R(n + 0x8ef9b)) mod count].
   5. item = the planner's artifact of T (0x119660).
   6. kind' = placeKind(T, bigCell(T)); if bigStructure(T.x, T.y, kind', bigCell(T))
      (0x2aa270) is not 0, kind' = that big-structure kind.
   7. The script is the artifact tale (§3.4) with realm, item, T, kind', seed s. Done.
5. Otherwise the **realm story** (§3.3) with realm, seed s, biome = `gamegen::biome` of the
   region of (16·cx + 8, 16·cy + 8) (no offset this time) and land seed
   L = R(region.x + 57·region.y).

The script depends only on the cell (and on world generation), so the same cell always tells
the same tale. The map's lore panel (§4.5) builds it again from the cell.

### 3.1 Commands the tales use

The speech builder 0x1694e0 reads these (each argument is its own token, printed as a signed
32-bit decimal):

| Command | Arguments | Sets |
|---|---|---|
| `define_realm` | realm kind, realm | the realm (names, ruler) |
| `define_ruler` | title index | the ruler's title |
| `define_mob` | id, race | person slot "mob" |
| `define_mob2` | id, race | person slot "mob2" |
| `define_model` | creature kind | "mob" as a creature kind |
| `define_model2` | creature kind | "mob2" as a creature kind |
| `define_land` | biome, land seed | the land ("land") |
| `define_faction` | 1..8 | the faction |
| `define_number` | n | @number |
| `define_number2` | n | @number2 |
| `define_name` | a text token | @name (not a number: the next token is the text itself) |
| `define_item` | category, id, modifier, material, rarity | the item (material **before** rarity, conversation-script §7.1) |
| `define_location` | x, y, kind | the place ("zone") |
| `mark_location` | x, y | attaches the place to the current page (§4.2) |
| `new_page`, `#tag`, ids | | as in conversation-script §7.1 |

CubeVulkan's `TalkScripts` plays only `define_location`, `define_direction`, `define_item`,
`define_mob` and `mark_location` today; the tales need the rest.

### 3.2 Tag tables

| Helper | RVA | Index | Tags in index order |
|---|---|---|---|
| placeTag(R) | 0x10be80 | X(R + 0x334e) mod 9 | #island #village #ocean #mountains #snowlands #deadlands #farm #city #palace |
| sourceTag(R) | 0x1126d0 | X(R + 0x13031) mod 21 | #ancient_inscription #old_inscription #magic_inscription #stone_tablet #inscribed_tablet #ancient_scroll #old_scroll #magic_scroll #ancient_book #magic_book #old_book #witch #wise_man #wise_woman #sorcerer #sorceress #wizard #ghost #spirit #djinn #demon |
| terrainTag(R) | 0x10eda0 | X(R + 0x154e) mod 7 | #island #mountains #ocean #snowlands #deadlands #forest #desert |
| buildingTag(R) | 0x10f010 | X(R + 0x392fa) mod 11 | #temple #pyramid #palace #grotto #cave #citadel #castle #cathedral #fortress #tower #volcano |
| magic(R) | 0x10e6f0 | X(R + 0x392fa) mod 15 | conversation-script §6.4 |

**capitalName(R)** (0x10c130) is the text A + B + C with
A = first [X(R + 0x18043) mod 29], B = middle [X(R + 0x138c) mod 10],
C = last [X(R + 0x4f4e) mod 9]:

* first (0x442300..0x442370): Ka, Cha, Ca, Cy, Ko, Go, Ga, Gro, Te, Tu, Lu, A, E, Y, I, Vy, Wa,
  No, Le, Ti, Ba, So, Sa, Si, Xa, Xy, Xi, Zu, Za
* middle (0x442374..0x442398): mer, ra, gu, ga, mor, nor, lor, gor, nar, far
* last (0x44239c..0x4423c0): zan, a, o, ia, an, e, une, unia, nia

A **fake land** (used by realm-story case 1 and 2) is `define_land`, X(R + 0x14ca) mod 14,
X(R + 0x18198) (the full value, printed signed).

"perished(R)" = odd(R + 0xbd8b9) (the same test as the land history's perished civilisation).

### 3.3 Realm story (0x10f450)

Inputs: realm R, seed s, biome B, land seed L.

Header, always:
1. `define_realm`, realmKind(R), R
2. `define_ruler`, rulerTitle(R)

Let **ruler** mean `define_mob`, X(R + 0x8144) (signed), rulerRace(R).

Then by **X(s) mod 7**:

**1 — the founder's journey**
1. ruler; placeTag(R); `npc_lived_in_a_location`; `new_page`.
2. By X(R + 0x230b) mod 3:
   * 1: fake land; `later_he_moved_to_the_land`
   * 2: fake land; `one_day_he_explored_the_land`
   * 0: `define_faction`, faction(R); fake land; `during_the_faction_war_he_had_to_flee_to_the_land`
3. By X(R + 0x4d1) mod 5:
   * 1: `there_he_explored_ancient_ruins_and_dungeons`
   * 2: magic(R); `there_he_learned_about_realm_magic`
   * 3: `after_some_years_he_founded_the_realm`
   * 4: `there_are_many_legends_about_his_adventures`
   * 0: `define_faction`, faction(R); `there_he_fought_against_the_faction`

**2 — the source of magic**
1. ruler; fake land; sourceTag(R); magic(R).
2. By X(s + 1) mod 6:
   * 1: `npc_used_his_realm_magic_skills_to_explore_dangerous_dungeons`
   * 2: `define_faction`, faction(R); `npc_used_his_realm_magic_skills_to_fight_the_faction`
   * 3: `using_realm_magic_npc_was_able_to_increase_his_power`
   * 4: `define_land`, B, L (the real land); `npc_used_his_realm_magic_skills_to_explore_the_land`
   * 5: `npc_spent_a_lot_of_time_exploring_the_land`
   * 0: `during_his_time_in_land_npc_discovered_wisdom`

**3 — the residents**
* If odd(s + 1):
  * If perished(R) **and** X(s + 2) is even: ruler; then if X(R + 0xe6cb) is even
    `the_residents_loved_their_ruler`, goodTrait(R + 0x1e2e), else
    `the_residents_feared_their_ruler`, badTrait(R + 0x1e2e); then
    `he_was_known_for_his_trait`.
  * Otherwise: ruler; `praise_the_ruler`.
* Otherwise: magic(R); `some_residents_used_realm_magic` if perished(R), else
  `some_residents_use_realm_magic`.

**4 — the capital**
1. `define_name`, capitalName(R); `define_faction`, faction(R).
2. By X(s + 1) AND 3:
   * 1: `the_capital_of_the_realm_was_called`
   * 2: `archeologists_are_looking_for_the_ruins_of_the_capital_of_the_realm`; then, unless
     X(s + 2) = 0 (the binary tests t·32 = t on the pre-final value, true only for 0):
     `new_page`, `until_now_their_efforts_were_useless`
   * 3: `define_number`, (X(R + 0xc355) mod 101) + year(R);
     `the_capital_of_the_realm_was_built_in_year`
   * 0: by R mod 8: 1 `the_capital_of_the_realm_sunk_in_the_ocean`;
     2 `the_capital_of_the_realm_was_destroyed_by_a_meteor`;
     3 `..._by_an_earthquake`; 4 `..._by_a_volcanic_eruption`; 5 `..._by_a_titan`;
     6 `..._by_a_megasaurian`; 7 `..._by_a_demon`;
     0 `the_capital_of_the_realm_was_destroyed_during_a_war_against_the_faction`
     (the "..." ids all begin `the_capital_of_the_realm_was_destroyed`).

**5 — the land**
1. `define_land`, B, L; `define_number`, year(R).
2. By X(s + 1) mod 3:
   * 1: `the_land_was_populated_by_residents_in_year`
   * 2: `define_faction`, faction(R);
     `long_ago_there_was_a_war_in_the_land_between_the_realm_and_the_faction`
   * 0: `the_land_was_inhabited_by_residents` if perished(R), else
     `the_land_is_inhabited_by_residents_since_year`

**6 — the ruler's seat**
ruler; buildingTag(R); terrainTag(R); then `the_ruler_was_buried_in` if perished(R), else
`the_ruler_lives_in`.

**0 — the rulers**
* **perished(R):**
  * If odd(s + 1):
    1. ruler; `once_there_was_a_a_named_ruler` if X(s + 2) is even, else `define_number`,
       year(R), `in_the_year_there_was_a_named_ruler`.
    2. By X(s + 3) mod 3: 1 trait(R), `he_was_known_for_his_trait`; 2 magic(R),
       `he_was_a_master_of_realm_magic`; 0 `he_founded_the_realm`.
  * Otherwise:
    1. ruler; `once_there_was_a_realm` if X(s + 2) is even, else `define_number`, year(R),
       `in_the_year_there_was_a_realm`.
    2. `new_page`.
    3. If odd(s + 3): `the_realm_was_ruled_by`.
    4. Otherwise `the_residents_had_a_ruler`, then by X(s + 4) mod 3:
       * 1: trait(R), `he_was_known_for_his_trait`, **then also** magic(R),
         `he_was_a_master_of_realm_magic` (the binary falls through; verified in the
         assembly at 0x14011113a..0x1401111d3);
       * 2: magic(R), `he_was_a_master_of_realm_magic`;
       * 0: ruler, `his_name_was`.
* **Not perished:**
  1. If odd(s + 1): ruler; `the_realm_is_ruled_by`; let u = s + 2.
  2. Otherwise: `define_number`, year(R); `the_realm_exists_since_the_year`; then if
     odd(s + 2): `new_page`, ruler, `it_is_ruled_by`; in both cases u = s + 3.
  3. If X(u) is odd: magic(R), `he_is_a_master_of_realm_magic`.

Here **trait(R)** = goodTrait(R + 0x1e2e) if X(R + 0xe6cb) is even, else badTrait(R + 0x1e2e).

### 3.4 Artifact tale (0x10b6f0)

Inputs: realm R, item, place T, kind', seed s.

1. `define_realm`, realmKind(R), R
2. `define_item`, category, id, modifier, material (+0x1c), rarity (+0x14) of the item
3. By X(s) mod 3: 1 `the_item_was_a_mighty_artifact`; 2 `there_is_a_mighty_artifact`;
   0 `there_is_a_legend_about_an_artifact`
4. `define_location`, T.x, T.y, kind'
5. `it_is_said_to_be_located_in_the_location` if kind' < 40, else
   `it_is_said_to_be_located_in_the_dungeon` (40+ are the castle/dungeon big structures)
6. `mark_location`, T.x, T.y

### 3.5 Tombstone tale (0x111560)

Inputs: realm R, seed s. Used when the lore cell's object is 37.

1. `define_realm`, realmKind(R), R
2. `define_mob`, X(s) (signed), nationModel(R, s)
3. age = (X(s + 2) mod 500) + 18
4. By X(s + 3) mod 3 (u is the counter used next):
   * 1: `here_rests_resident`; u = s + 4
   * 0: `here_rests_hero_of_the_realm`; u = s + 4
   * 2: born = (X(R + 0x13309) mod 9000) + 100 + (X(s + 4) mod 1000) — that is year(R) plus
     up to 999 years; emit `define_number`, born, `define_number2`, born + age,
     `npc_life_span`; u = s + 5
5. `new_page`
6. Cause of death by X(u) mod 11 (sub-draws use u + 1):
   * 1: `define_faction`, faction(R); `he_was_killed_by_a_faction_member`
   * 2: `define_mob2`, X(u + 1) (signed), killerRace(R, u + 1); `he_was_killed_by_npc`
   * 3: `define_model2`, monster(u + 1); `he_was_killed_by_a_monster`
   * 4: `define_model2`, poisoner(u + 1); `he_was_poisoned_by_a_creature`
   * 5: `he_lived_a_full_life`
   * 6: goodTrait(u + 1); `he_was_known_for_his_trait`
   * 7: `define_number`, age; `he_lived_for_years`
   * 8: magic(R); `he_was_a_master_of_realm_magic`
   * 9: `there_are_many_legends_about_his_adventures`
   * 0, 10: `define_faction`, faction(R); `he_died_in_the_war_against_the_faction`

Helpers:

* **killerRace(R, m)** (0x10cd80), by X(R + 0x157c) AND 15, with b = X(m) AND 1:
  1→2+b, 2→4+b, 3→7+b, 4→9+b, 5→11+b, 6→13+b, 7→15+b, 8→48, 9→51, 10→76, 11→199, 12→43,
  13→230, 14→227+b, 15→167, 0→b.
* **monster(m)** (0x10d330) = list [X(m) mod 47], list in order: 96, 17, 165, 41, 269, 230,
  173, 166, 276, 269, 97, 40, 94, 226, 85, 73, 229, 110, 225, 226, 226, 45, 43, 167, 108,
  109, 114, 116, 115, 119, 262, 117, 118, 111, 112, 81, 102, 105, 295, 293, 104, 69, 70, 71,
  72, −99992, −99993.
* **poisoner(m)** (0x10e5e0) = list [X(m) mod 11]: 230, 97, 45, 167, 43, 53, −99997, −99993,
  −99992, −99995, −99994.

### 3.6 Dictionary ids used by the tales

Realm story: `npc_lived_in_a_location`, `later_he_moved_to_the_land`,
`one_day_he_explored_the_land`, `during_the_faction_war_he_had_to_flee_to_the_land`,
`there_he_explored_ancient_ruins_and_dungeons`, `there_he_learned_about_realm_magic`,
`after_some_years_he_founded_the_realm`, `there_are_many_legends_about_his_adventures`,
`there_he_fought_against_the_faction`,
`npc_used_his_realm_magic_skills_to_explore_dangerous_dungeons`,
`npc_used_his_realm_magic_skills_to_fight_the_faction`,
`using_realm_magic_npc_was_able_to_increase_his_power`,
`npc_used_his_realm_magic_skills_to_explore_the_land`,
`npc_spent_a_lot_of_time_exploring_the_land`, `during_his_time_in_land_npc_discovered_wisdom`,
`the_residents_loved_their_ruler`, `the_residents_feared_their_ruler`,
`he_was_known_for_his_trait`, `praise_the_ruler`, `some_residents_used_realm_magic`,
`some_residents_use_realm_magic`, `the_capital_of_the_realm_was_called`,
`archeologists_are_looking_for_the_ruins_of_the_capital_of_the_realm`,
`until_now_their_efforts_were_useless`, `the_capital_of_the_realm_was_built_in_year`,
`the_capital_of_the_realm_sunk_in_the_ocean`,
`the_capital_of_the_realm_was_destroyed_by_a_meteor`,
`the_capital_of_the_realm_was_destroyed_by_an_earthquake`,
`the_capital_of_the_realm_was_destroyed_by_a_volcanic_eruption`,
`the_capital_of_the_realm_was_destroyed_by_a_titan`,
`the_capital_of_the_realm_was_destroyed_by_a_megasaurian`,
`the_capital_of_the_realm_was_destroyed_by_a_demon`,
`the_capital_of_the_realm_was_destroyed_during_a_war_against_the_faction`,
`the_land_was_populated_by_residents_in_year`,
`long_ago_there_was_a_war_in_the_land_between_the_realm_and_the_faction`,
`the_land_was_inhabited_by_residents`, `the_land_is_inhabited_by_residents_since_year`,
`the_ruler_was_buried_in`, `the_ruler_lives_in`, `once_there_was_a_a_named_ruler` (sic),
`in_the_year_there_was_a_named_ruler`, `he_was_a_master_of_realm_magic`,
`he_founded_the_realm`, `once_there_was_a_realm`, `in_the_year_there_was_a_realm`,
`the_realm_was_ruled_by`, `the_residents_had_a_ruler`, `his_name_was`,
`the_realm_is_ruled_by`, `the_realm_exists_since_the_year`, `it_is_ruled_by`,
`he_is_a_master_of_realm_magic`.

Artifact tale: `the_item_was_a_mighty_artifact`, `there_is_a_mighty_artifact`,
`there_is_a_legend_about_an_artifact`, `it_is_said_to_be_located_in_the_location`,
`it_is_said_to_be_located_in_the_dungeon`.

Tombstone tale: `here_rests_resident`, `here_rests_hero_of_the_realm`, `npc_life_span`,
`he_was_killed_by_a_faction_member`, `he_was_killed_by_npc`, `he_was_killed_by_a_monster`,
`he_was_poisoned_by_a_creature`, `he_lived_a_full_life`, `he_was_known_for_his_trait`,
`he_lived_for_years`, `he_was_a_master_of_realm_magic`,
`there_are_many_legends_about_his_adventures`, `he_died_in_the_war_against_the_faction`.

Elsewhere in this spec: `realmname` (§4.1, §4.5), `he_is_petrified` (§2.4).
Tags: §3.2 plus the trait and magic tables of conversation-script §6.4-6.6.

---

## 4. Lore

### 4.1 Crediting lore when the page is shown (0xb40d0)

0xb40d0 updates the dialog window every frame. When a page becomes current (its page timer,
dialog+0x200, is exactly 0 for the page index at dialog+0x204), the page's effects run once,
in this order: the item it gives (page +0xd4 count, +0xd8 item), the item it takes (+0x30,
+0x34), **its lore list (+0x190)**, then its marked places (+0x1a0, §4.2). The same routine
does the same for the speech bubbles over creatures (controller+0x400).

For each cell C in the page's lore list:

1. Insert C into the used-cell set (+0x1528 → +0xa8). This happens even if C is not a lore
   building.
2. If C is not a lore building (§2.3), stop here.
3. P = the place of C (floor(16·C / 1024) per axis); realm = realmOf(P) (§4.3).
4. Look up lore[realm] at +0x128 (creating an empty set if absent). If C is already in it,
   nothing more happens.
5. Otherwise insert C, and let k = the set's new size.
6. Add one chat line (0x26bf10 on controller+0x398), in two coloured parts:
   1. the realm's name: the dictionary entry `realmname` rendered with the realm defined as
      (realmKind(realm), realm) (0x16c3e0), in colour (1.0, 0.8, 0.5) (0x438110);
   2. the text " lore increased to " + decimal(10·k) + "%." and a line break (wide strings at
      0x437e40 and 0x437e38), in colour (0.6, 1.0, 1.0) (0x4381e0).

   Colours are converted to bytes by × 255 (0x42e7d8). With three cells read the line reads
   "*realm name* lore increased to 30%." The percentage is not capped; 11 cells give 110%.
7. Play sound 0x20 ("quest-progress.wav") at the listener (controller+0x1dc0), volume 1.

Because Examine attaches the cell to the **last** page (§2.3), the lore is credited when the
player has paged to the end of the tale.

### 4.2 Marked places on the same pages

For each place M in a page's marked list (from `mark_location`), if M is not known
(0x5f4c0: not in +0xb8 and not in +0xc8):

1. insert M into +0xb8 (known) and +0x38 (discovered);
2. set the NEW timer +0xe8[M] = 180000 ms (0x482fc800);
3. play sound 0x20 at the listener.

The artifact tale ends with `mark_location` T (§3.4), so reading such a tale marks the
artifact's place directly.

### 4.3 The realm of a place (0x2bb060; 0x9bf20 computes the big flag itself)

Input: place (px, py) and big = bigCell(px, py) (0x36160).

1. If big, round px and py down to even (floor(v / 2) × 2).
2. bx = floor(px / 16), by = floor(py / 16) (the 16 × 16-place block).
3. sel = R(px + 57·py + 0x4f7cff) mod 30.
4. realm = R(bx + 57·by + K) where K depends on sel:

| sel | K |
|---|---|
| 1 | 0x1e44621 |
| 2 | 0x1e44623 |
| 3 | 0x1e445e9 |
| 4 | 0x1e4465b |
| 5 | 0x1e445e8 |
| 6 | 0x1e445ea |
| 7 | 0x1e4465a |
| 8 | 0x1e4465c |
| 0, 9..29 | 0x1e44622 |

So each block of 16 × 16 places has nine possible realms; a place picks one by its own noise,
22 times in 30 the block's main one. CubeVulkan: `gamegen::placeRealm(x, y)`.

### 4.4 Persistence (characters.db)

The lore map is **not saved**. Each character is a blob in `Save/characters.db` (sqlite,
table `blobs(key, value)`; loader 0x9bb20 → 0x147800). The mission-data part of the blob
stores the sets in this order: +0x38, +0x48, +0x58, +0x88, +0x98, **+0xa8**, +0xb8, +0xc8,
+0xd8, then the +0xe8 timers. Each set is an i32 count followed by that many (i32 x, i32 y)
pairs.

After loading a character, and at two other points (0x965e0, 0x9f7c0), 0x2e0b50 **rebuilds**
the lore map: it clears +0x128, then for every cell in the used-cell set (+0xa8) that is a lore
building, inserts it into lore[realmOf(place of the cell)]. An implementation may save the lore
map directly instead, but the observable result must equal this rebuild; in particular a lore
cell counts once, and every read lore cell is also a used cell (it shows on the map, §4.5).

### 4.5 Lore on the map

* **Lore building markers.** The map region model (0x2eb7b0) adds a place marker of plot type
  14 at the centre (16·cx + 8, 16·cy + 8) of every lore-building cell (and type 15 for the
  crate cells of 0x2b4410). The map (0x2829c0) draws markers of types 12, 14 and 15 only
  when their cell is in the used-cell set (+0xa8). So a lore building appears on the map once
  read.
* **Lore panel.** Hovering such a marker selects its cell; the information panel
  (0x28c290 → 0x28e640) then shows, for that lore cell: the realm's name (`realmname`, as in
  §4.1) in colour (1.0, 0.8, 0.5); "Lore " + decimal(10·k) + "%" with k = size of
  lore[realm], in grey (0.75, 0.75, 0.75) below 100 and green (0.6, 1.0, 0.25) at 100 or more;
  then the cell's tale (§3) rendered with seed R(cx + 57·cy).

---

## 5. Artifacts

### 5.1 How the region planner places artifacts (0x115df0 → 0x11d7b0)

The planner of region (rx, ry) is described in conversation-script §1.5. Its random counter
starts at **c = R(rx + 0x46d7c2 + 57·ry)** and is consumed by the steps below in this order.

1. **Pool.** For every place (x, y) with x and y each from 8(rx − 1) to 8(rx + 2) and
   8(ry − 1) to 8(ry + 2) inclusive (floor of the region edge / 1024), except (0, 0):
   kind = placeKind(x, y, bigCell(x, y)); skip if 0; round to even if big; keep it if the
   region of its landmark point (`cellPoint`, then `regionAt`) is (rx, ry). The pool is an
   ordered set of (kind, x, y), compared as signed ints in that order. No draws.
2. **Kinds 7, 16, 17, 24**, in pool order, each removed from the pool:
   kind 7 → role 21, level X(c) mod 3 + 2, c += 1; kind 16 → role 22, level (X(c) AND 3) + 1,
   c += 1; kind 17 → role 23, level (X(c) AND 3) + 1, c += 1; kind 24 → role 17, level 4, no
   draw.
3. **Witch.** Let towns = the kind-5 places (pool order). Only if there are at least 2 towns
   and at least one kind-22 place: T = kind22[X(c) mod n22]; remove; role 19; level
   X(c + 1) mod min(towns, 5); special town = towns[X(c + 2) mod towns]; c += 3.
4. **Demon portal.** If there is a kind-37 place: T = kind37[X(c) mod n37]; remove; role 20;
   level X(c + 1) mod 5; flag +0x88 = 1; c += 2.
5. **Artifact places.** Collect, in pool order, every remaining place whose kind is 8 or
   whose bigStructure(x, y, kind, bigCell) (0x2aa270) is not 0. For each, in that order:
   remove it; role 14; level 4; make its artifact (below) with the counter.
6. **Fallback** (only when step 5 found none). Build a candidate list, taking the first of
   these that is not empty (pool order):
   1. kind 24 places (always empty: step 2 took them);
   2. kinds 18, 23, 28, 30, 31, 36 (mask 0x10d0840000);
   3. kinds 7, 9, 12, 15, 18, 20, 23, 25, 28, 30, 31 (mask 0xd2949280);
   4. every kind other than 5.

   If one is found: T = list[X(c) mod count]; c += 1; remove T; role 14; level 4; make its
   artifact with the counter.

**Making an artifact (0x11d7b0)** for place (x, y) with counter c:

| Field | Value |
|---|---|
| category (+0x00) | 23 |
| id (+0x04) | X(c) mod 7 — the look: 0 Vase, 1 Amulet, 2 Ring, 3 Stone, 4 Gem, 5 Crown, 6 Scepter (name keys ArtifactVase ... ArtifactScepter, 0x1510f0) |
| modifier (+0x08) | X(c + 1), the full 32-bit value |
| region (+0x0c, +0x10) | the **place** (x, y), not the region |
| rarity (+0x14) | 0 |
| +0x18 | the realm: realmOf(x, y) with bigCell(x, y) (§4.3) |
| material, spirits | 0 |

Then c += 2. The item is stored in the planner's artifact map (+0x20) under (x, y) and
returned by 0x119660 (an empty item for any other place). The place's role treasure
(0x131540) for role 14 is this item, which is how the castle's reward chest gets it.

Worked example: region (0, 0) has c = R(0x46d7c2) = 5986; if no step before 5 drew, its
first artifact has id X(5986) mod 7 = 3 (Stone) and modifier X(5987) = 1600304309.

### 5.2 Carried artifacts: level and skill counters (0x66850)

Every world update (0x2da6c0), for the local player only:

1. Set the seven counters +0xb5c, +0xb60, +0xb64, +0xb68, +0xb6c, +0xb70, +0xb74 to 0.
2. Set the level (+0x144) to 1.
3. If the inventory has more than five tabs, go through the stacks of **tab 5** (the tab
   0x5f100 assigns to category 23). For each stack whose item category is 23:
   1. level += 1 (once per stack, whatever its count);
   2. i = modifier (+0x08) mod 7, unsigned (0x108ef0); counter[i] += 1.

The skill an artifact gives therefore comes from its modifier, not from its id (look).

### 5.3 Skill values

Let f(n) = 1 − 1 / (n / 20 + 1), i.e. n / (n + 20) (constants 1.0 at 0x4ac980, 20.0 at
0x42e5a0). For the player (hostility 0) n is the counter; for any other creature
n = 10 × level.

| i | Counter | Character-sheet label | Routine | Value | Used by |
|---|---|---|---|---|---|
| 0 | +0xb5c | Climbing Speed | 0x50b60 | 1.0 if appearance flags AND 0x40200; else (f(n) + 1) × 0.5 | sheet only (see Open questions) |
| 1 | +0xb60 | Swimming Speed | 0x663f0 | 1.0 if flags AND 0x40200; else (f(n) + 1) × 0.5 | ground-speed factor in water |
| 2 | +0xb64 | Diving Skill | 0x51190 | player: (1 − f(n)) × 0.1 = 0.1 / (n / 20 + 1); others: 0 | stamina drain under water |
| 3 | +0xb68 | Riding Speed | 0x652d0 | f(n) × 1.4 + 1.1 | sheet only |
| 4 | +0xb6c | Hang Gliding Speed | 0x5e3d0 | f(n) × 1.9 + 1.1 | sheet only |
| 5 | +0xb70 | Sailing Speed | 0x65330 | f(n) × 1.9 + 1.1 | ground-speed factor while sailing |
| 6 | +0xb74 | Light Radius | 0x5f6e0 | f(n) × 12 + 8 | light radius (+0x198) |

Constants: 0.5 at 0x4ac964, 0.1 at 0x42e528, 1.4 at 0x4328bc, 1.9 at 0x4328d4, 1.1 at 0x42e538,
12 at 0x42e59c, 8 at 0x4aca30. At n = 0 the values are 0.5, 0.5, 0.1, 1.1, 1.1, 1.1 and 8.

**Where they act:**

* **Creature physics 0x2bb8e0** (runs these only for the local player, or for a non-player
  when the world is not a remote client, world+0x358 = 0):
  1. +0x194 = 1.0.
  2. If the ability (+0x68) is 0x79 (sailing): if in water (+0x5c bit 0x02), +0x194 = sailing
     value.
  3. Otherwise, if in water and neither flags +0x80 bit 9 nor bit 18 is set: +0x194 =
     swimming value. (The other ground cases — 0x3eb33333, 0x3e800000 ... — are not part of
     this spec; CubeVulkan `NativeUpdate` has them.)
  4. Later multipliers: × 0.8 (0x42e530) when +0x990 exceeds the threshold, × 1.1 when
     +0x5c has 0x2000.
  5. +0x198 = light-radius value.

  The move speed is multiplied by +0x194.
* **Stamina 0x2de4c0**: for the local player with (+0x5c AND 0x12) = 0x02 (under water),
  stamina (+0x9a0) −= diving value × dt × 0.001 (dt in ms; 0x42e514). So the diving value is
  stamina per second (the sheet's "stam/s"). When stamina reaches 0 or less and the player is
  alive, every time the world clock (world+0x440) crosses a multiple of 500 ms the player loses
  5 % of maximum health (0.05 at 0x438200, maximum health 0x5fac0) with sound 0x2f.
* **Renderer 0xce5f0**: a living creature carrying weapon id 20 in either hand (+0x63c or
  +0x6dc), or with the lamp toggle (+0x128 bit 0x0001), gets a point light of radius +0x198 at
  its head.

**Character sheet** (0x273380): for i = 0..6 the label above, then " +" and the counter if it
is not 0, then the value: rounded value × 100 followed by "%" for i = 0, 1, 3, 4, 5; value ×
100 then " stam/s" for diving; the value then " m" for light radius.

### 5.4 Artifacts revealed by lore (map, 0x2829c0)

The map draws each place marker of the loaded map regions (list 1 of 0x2eb7b0: kind +0x10,
x/y/z in blocks +0x14/+0x18/+0x1c, role +0x20, level +0x24, the place's artifact item +0x28,
so its realm is at +0x40). For each marker that is in front of the camera and on screen
(projected x and y within −1..1):

1. M = the place of (x, y), rounded to even if bigCell(M).
2. known = M is known (0x5f4c0).
3. If the role is 14 and not known: look up lore[realm] where realm is the marker artifact's
   +0x18 (creating an empty entry if absent). If it holds **10 or more** cells:
   1. insert M into the known set (+0xb8) — not into +0x38;
   2. set the NEW timer +0xe8[M] = **18000 ms** (0x468ca000), one tenth of a talk mark's;
   3. treat the marker as known for this frame.

   No sound plays. The test runs only while the map shows the marker, so the reveal happens
   the first time the player looks at that part of the map after reaching ten.
4. A known role-14 marker uses the artifact icon (map texture +0x570).

The NEW timers decrease by dt each world update and are removed at 0 or below (0x2da6c0).
While an entry exists and the place's mission is not completed (0x5e430), the marker's label
gets "NEW" (0x46f578), shown when (clock / 600 ms) is odd, i.e. blinking every 600 ms.

### 5.5 Artifact preview on the map

A marker entry built for a role-14 place carries a pointer to its artifact item (when the
category is not 0) and the flag "known" in its byte +0x82. When the cursor hovers such a known
marker (the nearest under the cursor), the map copies the artifact item into its hovered-item
slot (map+0x1b0), from which the ordinary PreviewWidget item branch shows it.
Exact payload provenance, coordinate sampling, precedence and the distinct lore/flight
text are now specified in [steam-preview-map-location-lore.md](steam-preview-map-location-lore.md).
Lore-building markers (plot type14, §4.5) carry no item; their hovered16-block cell
instead selects the validated static lore branch. The source/selection/draw lifetime
contracts are also in [steam-preview-source-lifetimes.md](steam-preview-source-lifetimes.md)
and [steam-preview-hidden-interface-order.md](steam-preview-hidden-interface-order.md).

---

## 6. CubeVulkan correspondences (API only)

| Need | CubeVulkan |
|---|---|
| R(n) | `gamegen::rnd` |
| cell object (0x69c60) | `gamegen::cellObject(cx, cy)` |
| bigCell, placeKind, cellPoint, regionAt, biome | `gamegen::` of the same names |
| realm of a place (0x9bf20) | `gamegen::placeRealm(x, y)` |
| planner, artifact map, artifact-place list | `RegionRoles` / `gamegen::regionRolesOf`, `gamegen::placeArtifact` |
| lore by realm, used cells, known/marked places, NEW timers | `SaveGame::lore`, `openedChests`, `marked`, `missionTimers` |
| item category 23 names | `ItemNames.cpp` (category 22/23 keys) |

---

## Open questions

1. **Climbing, riding and gliding** (counters 0, 3, 4): their routines are called only by the
   character sheet; no other reader of +0xb5c, +0xb68, +0xb6c or caller of 0x50b60, 0x652d0,
   0x5e3d0 was found in the executable (all register-based accesses were searched). In this
   build they appear to have no gameplay effect. Confirm in play before porting an effect.
2. **Hostility-8 Examine** (§2.4) queues interaction kind 3; its handling was not traced.
3. **Chat line**: 0x26bf10 → 0x26b670 was not traced. The realm name and the " lore increased
   to N%." part are two coloured appends; the trailing line break suggests they form one line.
4. **Map hover tooltip — resolved by later native audit**:
   [steam-preview-map-location-lore.md](steam-preview-map-location-lore.md) gives
   the semantic item copy, coordinate producers/snapshots, flight/lore/item precedence
   and exact flight/static-lore presentation. Ordinary item/comparison drawing is
   [steam-voxel-widget-information-preview.md](steam-voxel-widget-information-preview.md) §§7–8.
5. **Target selection**: how the controller picks the targeted static object (+0x1e7c..+0x1e84),
   including its distance, was not traced.
6. **Dialog already open**: if the dialog page list is not empty before Examine, the new pages
   are appended and the search stops after the first column (§2.3 step 3). Whether Examine can
   run while a dialog is open was not checked.
7. **placeKind's big argument** in the planner pool (0x2b99d0) is taken from
   conversation-script; the decompiler hides the fourth argument.
8. **Weapon id 20 light**: the renderer tests only the item id at +0x63c / +0x6dc, not the
   category; whether non-weapon items with id 20 can sit in those slots was not checked.
9. **Lore without a tale**: §2.3 credits nothing if every tale is empty; no lore cell was found
   whose tale is empty (all three tale kinds always emit tokens), so this path is theoretical.
10. The tags `#stone_tablet` / `#inscribed_tablet` and models stone-tablet1..6 (0xaf8..0xafd)
    and inscribed-tablet (0xaff) suggest the lore objects' looks, but the static-object type →
    model mapping was not traced.
