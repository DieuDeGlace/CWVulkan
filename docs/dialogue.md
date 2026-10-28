# Dialogue and quests

## The game's own words

Every line Steam Cube World says comes from `dict_en.xml` in the owner's installation: 949 `<speech>` lines, 69 `<macro>`s and 858 `<name>`s, written in a small grammar that the game expands into sentences. `src/Speech.h` expands it the same way.

| Syntax | Meaning |
|---|---|
| `{ a \| b \| c }` | one alternative, at random |
| `#tag` | inside an alternative: only when the tag holds. The first alternative whose tags all hold wins, otherwise a random untagged one. `{ #mob_f she \| #mob_m he \| it }`, `{ east of here #east \| … \| nearby }` |
| `[ x ]` | optional |
| `*id` | another speech or macro (`*mob_name`, `*in_the_zone`, `*direction_of_here`) |
| `@fn(args)` | a value from the caller: `@mob(plural)`, `@mob_name`, `@zone(named)`, `@item()`… |
| `$creature`, `$item`, `$zone`, `$stress` | the colour a part is shown in |
| `<name id="Plains" tags="n p the">{@ #named} Plains</name>` | names, their grammar tags and plurals (`{ #plural Wizards \| Wizard }`) |

`EngineTests` expands real lines and checks that no grammar is left. For example, `objective_defeat_enemies_in_zone` with Radishlings in Lalan Village gives "Defeat the Radishlings in Lalan Village.", and `#east` picks "east of here".

## Talking to townspeople

Press **E** (the game's use key) near a townsperson. They speak one line at a time in a white bubble over their head; E skips to the next line.

- A conversation starts with `hello` and `my_name_is`.
- Then comes one topic from the game's small talk:
  - `welcome_to_location` and `how_do_you_like_our_zone`;
  - the weather of the region (`its_cold`, `its_hot`, `its_warm`, `its_pleasant`, from the game's region type there);
  - `so_you_are_a_<class>`;
  - the `random:1-9` lines (shops, "How are you?").
- It ends with `goodbye`.
- A line that needs something CubeVulkan cannot name yet is passed over for another topic.

## The game's conversations

Steam Cube World builds every townsperson's conversation as a **script**. The function is RVA 0x2d9480; its townsperson part is a 28 KB routine at 0x12a6f0. A script is a list of tokens.

**What decides it:**
- The script is seeded with a xorshift32 (shifts 13, 17, 5) from the person's id (mod 333,333) plus 13 × the world's day.
- It also depends on the person's race, class, flags and home cell, and on the player's gear.
- So it is fixed for a day, and it changes from day to day.

**Tokens:**

| Token | Meaning |
|---|---|
| `<speech id>` | A line from `dict_en.xml`. A `%` in the line starts a new line on screen. |
| `new_page` | A pause. |
| `!alert`, `!love`, `!sweat`, `!angry`... | An emote. |
| `define_location x y type` | A place: 1024-block cell (x, y) of a kind. Types 1-47: House, Farm, Inn, Crypt, Village, Tower, Castle, Graveyard, Mountain, Cave, Lake, ..., Rock (21), ..., Rocks (29), ... (`locationKind`). |
| `define_direction dx dy` | Which way it is. |
| `define_item category id modifier material rarity` | An item: its bytes +0x00, +0x04, +0x08, then the material (+0x1c) **before** the rarity (+0x14). The player's iron chest armour is `4 0 1100 1 0`. (ZoneExtractor's export labels the last two the other way round and builds its `item=` names with them swapped.) |
| `define_mob id race` | A person, the low 32 bits of their creature id. |
| `define_model kind` | A kind of creature (`@mob`); negative kinds are the game's special models. |
| `define_realm kind realm`, `define_land biome seed`, `define_ruler title`, `define_number n`, `define_specialization class spec` | A realm, a land, a ruler's title (0-25), a number and a specialisation for the next lines (`@realm`, `@land`, `@ruler`, `@number`, `@specialization`). |
| `#tag` | A grammar tag for the next line (`#inn`, `#kindness`). |
| `<speech id>?` | An answer the player may choose (`rest_inn?`, `change_specialization?`, `goodbye?`). |
| `mark_location x y` | Shows the place on the map. The game saves it with the character (characters.db). |

**What the game actually gives:**
- **Tips about treasure.** Asmore: "You won't believe what I saw at the Jakor Rocks!", then "a Hang Glider!", and the Jakor Rocks are marked on the map. Chiba: a Rare Book of Crafting on the Rokor Rock.
- **Remarks about the player's gear:** "You have a well-made Sword!", "I'm jealous".
- **Ambitions:** "My aim is to be the most powerful warrior in the world!".
- **Introductions:** "My name is Thalkor".
- **Gear-related topics:** flying, climbing, sailing and riding, depending on what the player owns.
- **Other tips:** witches who petrified a village, and demon portals.

The dictionary's `objective_*` and `solved_*` lines are not in the game's code at all. The Steam build never uses them.

**Names are the game's own.** One generator, RVA 0x611d0(seed, style), names everything:
- **People:** seed = the low 32 bits of their id, style = their race. This gave Chizy, Chiba, Zimtok, Arilon, Asmore and Thalkor, the same names the game's chat showed.
- **Places:** seed = a hash of the cell (0x2ba7c0), style from the cell and kind (0x2ba810). The kind's `<name>` gives the grammar ("on the Rokor Rock", "at the Jakor Rocks").
- **Items:** `Speech::GenerateItemName` (0x1628b0), plus the dictionary key from the game's item table (`HangGlider`).

**Exporting:**
- ZoneExtractor asks the game for the script of every townsperson that loads, once per person per day, and appends it to `Extract/talk-scripts.txt`. K writes all loaded townspeople again.
- **64 days at a time:** the day is the world's (world + 8, also the second int of `world.db`'s state). The exporter sets it to each of the next 64 days for the call and puts it back at once, on the game's thread, so each line carries `day=`. Checked: the day the game was on reproduced the script exported earlier word for word, and the next days differ (Chizy: day 135 "I want to be the best warrior", day 136 "nice to meet you... are you a good climber?").
- It adds the game's names: `speaker=`, `name=` after `define_location` and `define_mob`, and `item=` / `key=` after `define_item`.
- The F3 creature tour records the townspeople themselves, so a tour collects both. It goes around the **extraction's home** (`manifest.txt`), whichever character runs it, and skips creatures recorded before (`[Tour] Rerecord=1` records them again).

**In CubeVulkan:**
- **The game's script, made by CubeVulkan** (`src/TalkGen.h`, from `docs/specs/conversation-script.md`): a townsperson of the game's world says the script the game would build for them on the **world's day**, one line per E. It takes the person's record (id, race, class, flags, home), the player's class, gear, pet, gold, special items and missions, and the world generator's planner of the person's home region. The world's day carries on from the save and never runs behind the game's own `world.db`.
- **Exported scripts** (`src/TalkScripts.h`) speak for recorded townspeople CubeVulkan has no record of (the nearest exported day when theirs wasn't exported, logged). With `[Talk] UseExports=1`, an exported script for the person and the day also comes before the generated one.
- **Playing a script:** names come from the exporter where a script has them, else from CubeVulkan's own name generator (people, places, lands) and item names. Places they mark are saved with the character (`marked x y "place" "item"`) and logged. Answers offered (`?` tokens) are shown as `> ...` lines; an innkeeper's `rest_inn?` rests until 7:00 of the next day.
- **Without a script:** people keep small talk built from the game's lines (above).
- **Invented quests:** the "defeat the enemies in a zone" requests (described below) are CubeVulkan's. They are off unless `[Quests] CubeVulkanQuests=1`.
- **Checking it:** `EngineTests talkverify [examples]` makes every script of `Extract/talk-scripts.txt` again and reports, per branch of the builder, how many match token for token.

**Not yet:**
- **What is at the places:** the treasure itself, witches and portals, and how a tip is completed.
- **Map markers:** marked places are saved and logged, but no marker is drawn.
- **Realm and ruler names:** `@realm()` and `@ruler()` (the history of the land, artifact tales) are left empty and logged. The game's resolver uses the UTF-16 "realm" strings around 0x448910 (code 0x165f6d onward).
- **Special models:** `define_model` with a negative kind (the game's special models) has no name yet; logged.
- **Choosing an answer:** only the inn's rest is acted on; the trainer's change of specialisation is shown but not taken.

## CubeVulkan's own requests (off by default)

With `[Quests] CubeVulkanQuests=1`, some townspeople (one in three, fixed per person) ask the player to defeat the enemies in a zone, using the dictionary's otherwise unused objective lines.

1. They name the nearest kind of monster within 400 blocks and where it is: `the_location_is_full_of_creatures`, `you_must_defeat_the_enemies_in_zone`, `the_enemies_are_in_zone_direction_of_here`.
2. The quest tracker shows `objective_defeat_enemies_in_zone` with a count, up to five.
3. When the last one falls, `solved_defeat_enemies_in_zone` appears.
4. Talking to them again gives `good_work`, `thank_you` and the reward: 20 gold per creature (× level in pyro mode), plus an item rolled like loot at 3 stars.

These are saved with the character (`quest <giver> <state> <need> <done> <target> <zone>`). At most three are under way at once. Proper names here are CubeVulkan's, made in the game's style from syllables.

Testing: `--talk N` walks to the Nth townsperson nearest home and talks, logging every line. `--talk-again S` returns to them after S seconds. `--auto-attack` prefers the creatures a quest asks for.
