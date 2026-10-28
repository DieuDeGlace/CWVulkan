# The game's own save files (characters.db, world.db)

Behavioural specification of how Steam Cube World stores its characters and its world, so that
CubeVulkan can read and write the player's existing saves 1:1, the way OpenMW uses Morrowind's
saves. Written from the decompiled executable (RVAs, image base 0x140000000), its assembly and
the binary's constants. No real save file was opened to write it: every layout below comes from
the code that writes and reads the files. `saves.md` describes what CubeVulkan does today; this
spec corrects and completes its "Steam save format" section (see §9).

Vocabulary used here and defined elsewhere:
* **creature record** (0x15f8 bytes) and its fields: `creature-upkeep.md` §1.1, `stat-rules.md`;
* **item record** (0xa0 bytes): `drops-and-items.md` §0.1;
* **player data** (the structure at creature +0x1528, called chardata in `map-markers.md` §0.4);
* **zone** (64 × 64 blocks), **static object** and **ground item** records (0xe0 bytes each):
  `worldgen-creatures.md` §0.2, `use-prompt.md` §0.3 and §0.4.

All integers are little-endian. "i32" is a signed 32-bit integer, "pair" is two i32 (x first,
then y) stored as 8 bytes, "i64" a signed 64-bit integer. World positions are i64 in 1/65536
of a block.

---

## 1. The files

### 1.1 Where they are

* At start-up the main routine 0x1355a0 prints "create save dir" and calls `_mkdir("Save")`,
  relative to the working directory (the install folder). Failure is ignored.
* The game opens exactly two files, both by relative path:
  * `Save/characters.db`, opened by the game object's constructor 0x7ea80;
  * `Save/world.db`, opened by the world set-up 0x2b5f20, only when the world is not in
    server mode (world +0x358 is 0) and its name (world +0x200) is not empty.
* Both are SQLite 3 databases opened read-write with create (open flags 6, 0x1f0bb0 →
  0x210230). The game links SQLite statically.

There is code for a third database (game +0x2278) holding a list of worlds ("Create World",
"Delete World" and a world picker, 0x9f060, 0x9e5e0 and the handler ending at 0x9fdc6). Nothing
in this build ever opens it (the only open routine 0x67070 is called for the two files above),
so all its reads and writes fail quietly. "Delete World" also deletes `Save\world_<name>.db`
and `Save\map_<name>.db`, files the Steam build never creates. A port can ignore all of this.

### 1.2 The table and its access routines

Right after opening, 0x67070 runs `CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);` and
ignores the result (it fails harmlessly when the table exists). Every save record is one row:
a text key and a binary value. (The game's data files use the same table shape, but their
values are obfuscated by 0x66f10; the save files' values are not.)

| Routine | Statement | Behaviour |
|---|---|---|
| 0x670f0 read | `SELECT value FROM blobs WHERE key = ?` | Copies the value into a byte buffer. False if there is no such row or the statement cannot be prepared. |
| 0x672e0 write (via 0x672c0) | `SELECT 1 FROM blobs WHERE key = ?`, then `UPDATE blobs SET value=? WHERE key=?` if a row came back, else `INSERT INTO blobs(key, value) VALUES(?, ?)` | An empty buffer is bound as a null pointer of length 0. If the insert cannot be prepared it prints "DATABASE WRITE ERROR". |
| 0x66fc0 delete | `DELETE FROM blobs WHERE key = ?` | |
| 0x671f0 rename | `UPDATE blobs SET key = ? WHERE key = ?` | |

Each routine re-prepares and retries while finalising returns 17 (SQLITE_SCHEMA). There are no
explicit transactions, so every statement commits on its own (SQLite's default rollback
journal: a `characters.db-journal` / `world.db-journal` file exists briefly next to the file
during each write). Writes to characters.db are made while holding the game's lock at game
+0x1c98.

---

## 2. characters.db

### 2.1 Keys

| Key | Value |
|---|---|
| `num` | 4 bytes: i32, the number of characters. |
| `slot` | 4 bytes: i32, the index of the character last selected or created. |
| `0`, `1`, … `num−1` | One character each: the character blob of §3, zlib-compressed (§2.2). |

Character keys are the index printed in decimal by a string stream (0xa5870), so `10` follows
`9`; there is no padding. Indices are dense: deleting one renames the keys above it (§4.4).

### 2.2 Compression

* Writing (0x66c10): zlib 1.2.8 `compress2` with level −1 (the default, 6) via 0x322280 →
  0x3222a0, into a buffer of trunc32(length × 1.1) + 12 bytes (constant 0x432c90 = 1.1). The
  stream therefore starts `78 9c`. No length prefix is stored.
* Reading (0x66d10 → 0x66dc0): zlib `inflate` in chunks of 0x1f400 bytes until the stream
  ends. The return code is not checked: a damaged stream yields whatever was inflated (possibly
  nothing), and the blob reader (§3.1) then fills the missing fields with defaults.

---

## 3. The character blob (version 11)

Written by 0x149760 and read by 0x147800. Both work through a cursor made by 0x147750, which
holds two version numbers: the character version **11** and the world-state version **44**.
The writer always writes version 11. Items are copied as their full 0xa0 bytes, padding
included (the padding holds whatever was in memory; readers ignore it, a writer should write
zeros).

### 3.1 Reading rules (0x147800)

* Each field is read with a bounds check. If fewer bytes remain than the field needs, the
  cursor jumps to the end, the field keeps its previous value and the reading carries on, so
  every later field also fails. A truncated blob therefore loads as a valid character whose
  missing fields are defaults. There is no error and no checksum.
* Quirk: when the **count** of one of the lists in §3.3 fails to read, the loop runs with a
  stale value left in a local variable (not zero), inserting default entries. A port should
  stop at the end of the data instead; no well-formed blob triggers this.
* The version (first field) is never rejected. It only gates two optional parts: the block of
  sets (§3.3 items 17–20) is present when version > 9, and a legacy list (§3.4) when version < 4.

### 3.2 Before the fields: the character is reset

The reader first resets the target creature (0x64a90), gives it fresh player data (0x7ff20,
0x300 bytes), clears its equipment block (+0x278), resets the inventory to six empty tabs
(0x64f30 with 6), and overwrites the creature's main data block from +0x10 with a default
creature's (0x4d300 / 0x4d600 → 0x3b7a0). It sets hostility (+0x60) to 0, the player.
Everything not listed in §3.3 keeps these defaults.

### 3.3 The fields, in file order

| # | Bytes | Field | Notes |
|---|---|---|---|
| 1 | 4 | i32 version | 11 when written by this build. |
| 2 | 24 | position x, y, z (+0x10..+0x20), 3 × i64 | |
| 3 | 12 | orientation (+0x28..+0x30), 3 × f32 | Read, then set to 0 by the reader. |
| 4 | 4 | health (+0x180), f32 | |
| 5 | 4 | level (+0x144), i32 | |
| 6 | 4 | experience (+0x148), i32 | |
| 7 | 1 | class (+0x14c) | 1 warrior, 2 ranger, 3 mage, 4 rogue. |
| 8 | 1 | specialisation (+0x14d) | 0 or 1. |
| 9 | 4 | +0x9a4, f32 | Default 1.0 (0x4ce80). The physics raises a pet owner's +0x9a4 while the pet is wet (`creature-physics.md`); meaning not established. |
| 10 | 4 | +0x9a8, f32 | Default 0. Meaning not established. |
| 11 | 11 × 160 | equipment: the eleven item slots from +0x278, 0xa0 apart (+0x278, +0x318 neck, +0x3b8 chest, +0x458 feet, +0x4f8 hands, +0x598 shoulder, +0x638 left weapon, +0x6d8 right weapon, +0x778 and +0x818 rings, +0x8b8 pet) | Raw item records (0x1496d0 / 0x14b0c0). The first slot's use was not traced. |
| 12 | 4 + n | name (+0x958): i32 length n, then n bytes, no terminator | The writer measures the name with strlen; the reader appends a 0. The game's buffer holds 16 bytes (`strcpy_s(..., 0x10, ...)` in 0x7ea80); longer names would overflow it. |
| 13 | 20 | appearance choices: player data +0x00..+0x13 (§3.5) | |
| 14 | 4 + … | inventory (+0x9f0): i32 tab count T; then per tab an i32 stack count n and n stacks of 0xa4 bytes (i32 count, then the item) | The game keeps six tabs. |
| 15 | 4 | gold (+0xaac), i32 | See §3.6. |
| 16 | 4 | +0xab0, i32 | Copied from a spawn point's +0x8b0 for other creatures; meaning not established. |
| 17 | 4 + n × 160 | known recipes: player data +0x18 list; i32 count, then items | The crafting menus are built from this list (0xb2180, which sorts it into tabs by item type). |
| 18 | sets | player data +0x38, +0x48, +0x58: each an i32 count, then that many pairs | +0x38 discovered zones, +0x58 discovered lands (`map-markers.md` §0.4); +0x48 not established. |
| 19 | maps | player data +0x88, +0x98: each an i32 count, then that many 12-byte entries (pair key, 4-byte value) | Meanings not established. +0x88 is emptied by the world reset (§5.6). |
| 20 | sets, map | player data +0xa8 (**used cells**: opened reward chests, read lore, …), +0xb8, +0xc8, +0xd8 (the mission sets): i32 count + pairs each; then +0xe8 (mission note timers): i32 count + 12-byte entries (pair, f32 ms) | `lore-and-artifacts.md` §1.1, `map-markers.md` §0.4. Items 18–20 are present only when version > 9. |
| 21 | 4 | world seed (player data +0x2c8), i32 | See §6. |
| 22 | 4 + n | world name (player data +0x2d0): i32 length, then the bytes | |
| 23 | 4 + 8n | known flight points: player data +0x28, i32 count + pairs (block x, y) | |
| 24 | 8 | home cell (+0x1d0, +0x1d4), pair | The 16-block cell a new character started in (§4.2). |
| 25 | 4 + 24n | buffs (+0x980): i32 count, then 0x18-byte buff entries (type byte +0, strength f32 +4, duration f32 +8, source id i64 +0x10) | The whole entry is copied, padding included. |
| 26 | 4 + 12n | player data +0xf8: i32 count, then (pair key, i32 value) | Keyed by region; 0x60ec0 reads the value for the character's current region (+0x150). Meaning not established. |
| 27 | 4 + 16n | star lists: player data +0x108 | Written as one flat list: an i32 total count, then 16-byte entries (i32 not established, block x, y, z as i32), map key order then list order. The reader regroups them by (x ÷ 4096, y ÷ 4096) with **truncation toward zero** (C division), see open question 6. |
| 28 | 4 + 8n | player data +0x118: i32 count, then pairs | Regions listed by name on the map screen (0x1081b0 names them with the region-name generator); exact meaning open. |
| 29 | … | items bought per shop: player data +0x138 | i32 count of shops; per shop an i64 key, an i32 item count and that many items. The shop's stock routine (e.g. 0x2acbd0) subtracts these from its daily stock (0x2a0c80). Emptied by the world reset (§5.6). |

All sets and maps are written in their sorted order (keys compared as signed integers, x first),
and read back by inserting each entry; a duplicate key in a file keeps the first one.

Not saved, so they take their defaults on load: the item held on the mouse cursor (+0xa08 count,
+0xa0c item), mana (+0x188), stamina, block power, cooldowns and abilities, the lore map
(player data +0x128, rebuilt from the used cells by 0x2e0b50, `lore-and-artifacts.md` §4.4),
the death position and tombstone timer (player data +0x28c..+0x2a0).

### 3.4 The legacy list (version < 4 only)

Between item 20's position and item 21: an i32 count, then per entry 4 skipped bytes, an i32
length and that many bytes of text, then 24 skipped bytes. Everything is discarded.

### 3.5 Appearance choices and the race

The 20 bytes of item 13 are the only appearance data in the file:

| Offset | Content |
|---|---|
| +0x00 | i32 people choice, 0..7 |
| +0x04 | byte; bit 0 is the sex (0 male, 1 female) |
| +0x05..+0x07 | padding |
| +0x08 | i32 face choice |
| +0x0c | i32 hair choice |
| +0x10, +0x11, +0x12 | hair colour bytes |
| +0x13 | padding |

After the inventory is read, 0x147800 rebuilds the whole look with 0x53e80(&race (+0x64), the
look block (+0x78), the player data, 0). For a player it sets the race from the choice and the
sex bit s: choice 0 → race 2 + s, 1 → 0 + s, 2 → 9 + s, 3 → 11 + s, 4 → 4 + s, 5 → 7 + s,
6 → 15 + s, 7 → 13 + s. The face and hair choices are reduced modulo the race's number of
models (signed remainder) to pick the head and hair models; the colour bytes are copied into the
look. Those per-race counts and model ids belong to the looks routine (`worldgen-creatures-tables.md`
T7) and are not repeated here. A choice outside 0..7 leaves the race unchanged.

### 3.6 Gold

After reading, if the gold, sign-extended to 64 bits and compared **unsigned**, is greater than
10,000,000,000, it is set to 1,410,065,408 (0x540be400, the low 32 bits of 10^10). A positive
i32 can never pass the test, so in effect a negative gold value loads as 1,410,065,408.

### 3.7 After the blob (0x9bb20)

The caller loads the blob under the game lock (0x1c98) and the world lock (0x2abb70 / 0x2b9890),
inflates it, runs 0x147800, and then makes sure the recipe list contains the item {type 1,
subtype 7, everything else 0} (compared with 0x4df60); if not, it appends it at the end.

---

## 4. Character lifecycle

### 4.1 Start-up (0x7ea80)

1. Preload the 125 sounds, then print "load characters" and open `Save/characters.db`.
2. Read `num`. If it has at least 4 bytes, for i = 0 .. num−1: make a creature (0x4ce80), load
   blob `i` into it (0x9bb20; a missing row leaves the default creature), and append it to the
   character list at game +0x1778. Missing `num` means no characters.
3. Build the interface; create the local player creature with the placeholder name "Wollay".
4. Read `slot`. If it exists: game +0x2264 = slot; load blob `slot` into the local player; then
   the map, interface and crafting lists are refreshed (0x2e0b50, 0x9afb0, 0x107690, 0xb2180).
   Without `slot` the constructor does not set game +0x2264 (its starting value was not traced).

### 4.2 Selecting (0x9f7c0, the "Select" button)

game +0x2264 = the list cursor (+0x6c0).
* If the cursor is outside the list (the new-character entry): reset the local player to a level
  1 warrior with default gear, a new home cell from 0x2d1100 (a random place) at +0x1d0, position
  (cell x × 16 + 8, (cell y + 2) × 16) blocks with z on the ground (0x35ea0), fresh player data,
  and open the creation screen.
* Otherwise: load blob `cursor` into the local player (0x9bb20), write `slot` = cursor, and enter
  the game.

### 4.3 Creating (0x9ea10, "Create Character")

1. Copy the typed name (the "edit" field) into +0x958; level = 1; insert the home cell (+0x1d0)
   into the used-cell set (player data +0xa8).
2. Write `num` = old count + 1.
3. game +0x2264 = old count; save the character (§4.5) under key `old count`.
4. Write `slot` = old count; append a copy of the character to the list.

### 4.4 Deleting (0x9dea0, "Delete Character")

With i = the cursor and n = the count after removal:
1. Remove entry i from the list; write `num` = n.
2. Delete key `i`.
3. For j = i .. n (inclusive): rename key `j+1` to `j`. (The last step renames a key that does
   not exist and changes nothing.)
4. `slot` is not changed. If it pointed at or past the deleted entry it now names a different
   character or none.

### 4.5 Saving (0xa27b0)

If game +0x2264 is negative, nothing is saved. Otherwise, holding the world lock:
1. Copy the local player into a scratch creature: the main block from +0x10 (0x3b7a0), +0x9a4,
   +0x9a8, a deep copy of the player data (0x7fbb0), the inventory, the cursor item, gold,
   +0xab0 and the buff list.
2. Release the world lock, serialise the copy (§3), compress (§2.2), and write it to key
   `<game +0x2264>` under the game lock.

When it happens:
* every time more than 60,000 ms have passed since the last save (`60000 < now − last`, with
  timeGetTime), checked by the game-side thread 0x84f70, when the world has a name (world +0x210
  ≠ 0) and none of the three menu screens (game +0x118 character list, +0x130, +0x170 creation)
  is showing;
* when that thread ends (quitting), with the same menu-screen condition;
* when returning to the character list (0x9fdf0), which then reloads every listed character
  from the file;
* when a character is created (§4.3).

---

## 5. world.db

### 5.1 Key and format

One row, key `state`, stored **uncompressed**. Written by 0x2d38e0 with 0x14ab50, read by
0x2b98b0 with 0x148e90. The writer only runs when the world has a name (world +0x210 ≠ 0).

| # | Bytes | Field |
|---|---|---|
| 1 | 4 | i32 version, 44 |
| 2 | 4 | i32 day (world +0x08). A new world starts at 1. |
| 3 | 4 | f32 time of day in ms (world +0x0c), 0..86,400,000. A new world starts at 36,000,000 (10:00, 0x4c095440). |
| 4 | 4 + 8n | **dead creatures** (world +0x20, logged as "numDeadCreatures"): i32 count, then i64 creature ids, sorted as signed numbers |
| 5 | 4 + 8n | **dropped rewards** (world +0x30, logged as "numDroppedRewards"): i32 count, then i64 ids, sorted |
| 6 | 4 + … | **zone states** (world +0x10, logged as "numZoneStates"): i32 count, then the records of §5.2 in key order |

### 5.2 Zone state record

| Bytes | Field |
|---|---|
| 8 | zone key: pair (zone x, zone y), zones of 64 blocks |
| 1 | flags: bit 0 = the ground items are saved, bit 1 = the static objects are saved |
| 4 + 224n | ground items: i32 count, then 0xe0-byte ground item records (raw, 0x1476a0) |
| 4 + 224n | static objects: i32 count, then 0xe0-byte static object records |
| 4 + 28n | block changes: i32 count, then 28-byte entries (§5.3), list order |
| 4 + 12n | place counters: i32 count, then (pair key, i32 count) entries (§5.3) |
| 4 | i32 dead tracked creatures of the zone (zone +0x490) |

When the reader meets a ground item it starts from defaults before copying, but every byte is
then overwritten by the copy; a port can treat the records as raw.

### 5.3 What the zone state holds

0x2d3e20 records a zone's state into the world's map when the zone is unloaded (by 0x2d2670 in
the streaming thread, and by 0x2d2410 / 0x2d2930), creating the record on first use (0x299560):

* if the zone's ground items were changed (zone +0xf9): set bit 0 and store a copy of the ground
  item list (zone +0x30);
* if the zone's static objects were changed (zone +0xfa): set bit 1 and store the static object
  list (zone +0x60). These records carry each object's "active" byte (+0x28): a chest opened, a
  door open or shut, an item given (`use-prompt.md` §0.3);
* the block change list (zone +0xd8) when it is not empty or a record already exists. Each
  28-byte entry is the block (x, y, z) as three i32, then the block as it was (6 bytes at +0x0c),
  then the block as it is now (6 bytes at +0x12), then 4 bytes not used here. Loading re-applies
  the "now" block (0x31c1c0 with +0x12); the undo path 0x2da090 ("undo N changes") puts the
  "was" block back;
* the place counters (zone +0x4d8, a map from 16-block place cell to a count) when not empty or a
  record exists. The world update 0x2e3510 adds these counts over nearby zones and compares the
  total with a requirement (zone +0x4c8) to switch a static object (0x2d3a10);
* the zone's count of dead tracked creatures (zone +0x490) when not 0 or a record exists
  (`worldgen-creatures.md` §0.2).

When a zone is created (0x2ae0c0: generated with srand(zone y × 1024 + world +0x244 + zone x)
and 0x2ff6f0), a stored record is applied on top: bit 0 replaces the generated ground items,
bit 1 replaces the generated static objects (and marks them changed again), then the block
changes are copied and re-applied in order, the dead count is set and the place counters
replaced. Nothing is applied in server mode.

The day of the zone's last daily refresh (zone +0x4f8) is **not** stored, so a reloaded zone
always runs its daily refresh again (`worldgen-creatures-zone.md` §4).

### 5.4 Load (0x148e90)

1. Read the version, then day and time **before** checking anything.
2. If the version is not exactly 44, stop: day and time are kept from the file, the rest stays
   as the new world had it.
3. Otherwise clear and refill the dead set, the dropped-reward set and the zone map.

### 5.5 When the world is saved

The zone streaming thread 0xf2340, under the world lock, takes copies of the day, the time and
the three maps (0xf1c90-style copies) and writes `state`:
* whenever more than 60,000 ms have passed since its last save (`now − last ≥ 60001`) while the
  game runs;
* once more when the thread ends (quitting).

Zone records only enter the world's map when zones unload, so a zone that stays loaded through
the save keeps its newest changes in memory only (open question 3).

### 5.6 The world reset (0x2c34c0)

When the world update 0x2da6c0 finds, outside server mode, an entry of kind 0 in the list at
world +0x160, it removes every non-player creature (0x2d2eb0) and then 0x2c34c0 empties the
dead-creature set (world +0x20, 0x3d6c0) and the whole zone-state map (world +0x10, 0x8a830),
removes every item of type 0x16 from the local player's inventory, and empties player data
+0x88, +0xd8 (finished missions) and +0x138 (items bought from shops). Every zone loaded afterwards therefore
comes back as generated (monsters alive, chests closed, no dropped items). The dropped-reward set
(world +0x30) is kept. What adds that entry was not traced (open question 9).

---

## 6. The world a character plays

* The game object's constructor sets the requested world to seed **123** (game +0x206c = 0x7b)
  named **"world"** (game +0x2070) and builds the world with them (0x2b5f20, which also calls
  srand(seed) and draws the world's noise offsets).
* 0x8a500 sets both the requested seed/name and the character's own copy (player data +0x2c8,
  +0x2d0). Its only callers are the world-list handlers of §1.1, which have no worlds to offer in
  this build. A character made by character creation therefore stores an empty name and, as its
  seed, whatever the player data constructor 0x7ff20 left at +0x2c8 (it does not initialise that
  word); loading a character never changes the world.
* The world update only ticks while the requested seed and name equal the world's (0xa5a50 at
  0xa72e1); otherwise 0xe8a30 is called. In practice every Steam character shares the single
  seed-123 world and the single `world.db`.

---

## 7. Steam Cloud

The executable makes no Steam remote-storage calls (its only Steam imports are init, shutdown,
callbacks and the user handle). Cloud saving is Steam Auto-Cloud, configured on Steam's side to
sync `Save/characters.db` and `Save/world.db` (see `saves.md`). Consequences:
* Steam uploads the files after the game exits and downloads them before it starts. Anything
  that writes them must do so while Steam considers the game running, or before Steam's
  post-exit sync, or the change is overwritten or conflicts.
* The journal files are not synced; a crash mid-write leaves a hot journal that SQLite rolls
  back on the next open.

---

## 8. Notes for CubeVulkan

* To read 1:1: open read-only, read `num`, `slot` and each key, inflate, and parse §3 in order
  with the §3.1 rules; read `state` with §5.4.
* To write 1:1: write version 11 / 44 exactly as §3 and §5 lay out, items and records as their
  full byte images, sets and maps in sorted order, `num` and `slot` as 4-byte blobs, the
  character compressed with zlib level 6, the state uncompressed. Each write is one statement;
  use the same UPDATE-or-INSERT so other rows survive.
* Writing the player's real saves must stay opt-in (CubeVulkan's current rule is read-only).

## 9. Corrections to `saves.md`

* The 11 equipment items start at offset 0x3e of the inflated blob (after version, position,
  orientation, health, level, experience, class, specialisation, +0x9a4, +0x9a8); 0xde is the
  second slot (neck). The name length at 0x71e agrees with this.
* world.db +0x4 is the day and +0x8 the time of day (f32 ms); the dead-creature list follows.
* After the inventory come gold, +0xab0 and the recipe list, then (version > 9) the place sets;
  the "discovered map cells" are those sets.

---

## Open questions

Each names a way to check it. "Test character" means a throw-away character made for the
purpose; the user's real saves are never needed.

1. **A sample blob.** No real file was inspected. Oracle: create a test character, play a minute,
   quit, copy `characters.db` and `world.db` aside, and decode them with a reader written from
   this spec; every field must parse to the end with no bytes left over. Then compare with the
   ZoneExtractor F5 snapshot taken just before quitting (position, level, experience, class,
   gold, equipment, inventory, buffs).
2. **Meanings** of +0x9a4, +0x9a8, +0xab0, the first equipment slot (+0x278), player data +0x48,
   +0x88, +0x98, +0xf8, +0x118 and the first i32 of a star entry. Oracle: a ZoneExtractor dump of
   the local player's creature record and player data before and after a known action (feeding
   a pet, discovering a region, completing a mission, buying, visiting a new region).
3. **Loaded zones at save time.** Whether a zone that is still loaded when `state` is written
   ever reaches the file before quitting (the exit path copies the map without unloading zones
   first, unless 0x2d2930 runs earlier in shutdown). Oracle: open a chest, stay next to it,
   quit; then decode `state` and see whether that zone's record is present with bit 1.
4. **Health on load.** Health (+0x180) is saved and restored as is; whether a character saved at
   0 health loads dead. Oracle: test character, die, quit during the death screen, reload.
5. **Who fills the block change list** (zone +0xd8) in Steam play, if anything does. Oracle:
   ZoneExtractor dump of zone +0xd8/+0xe0 after explosions and other block-breaking abilities.
6. **Star key truncation.** The reader groups star entries by x ÷ 4096 truncated toward zero; if
   the game inserts them with floor division, stars at negative coordinates move to another
   group after a reload. Oracle: dump player data +0x108 before quitting and after reloading a
   character with stars west or south of the origin.
7. **The recipe {1, 7}.** Which item that is and why every load adds it. Oracle: decode a fresh
   character's recipe list.
8. **The world list.** Confirm on the user's install that the world picker and "Create World"
   are never shown and that no `Save/world_*.db` exists, i.e. that every character uses seed
   123. Oracle: decode a test character's player data +0x2c8/+0x2d0 (expected: an arbitrary or
   zero seed and an empty name).
9. **The world reset** trigger (an entry of kind 0 at world +0x160). Oracle: ZoneExtractor
   watch on that list across a play session, especially around map changes and re-joins.
10. **Cloud timing.** When Steam syncs relative to the process exit. Oracle: Steam's own cloud
    log (`logs/cloud_log.txt` in the Steam folder) during a test character's session.
