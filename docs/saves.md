# Characters and saves

## Recorded characters

`tools/ZoneExtractor` records each character and specialization (subclass) played in Steam Cube World in its own folder:

```
<game>\Extract\characters\<name>-<specialization>\    e.g. warrior-guardian
    snapshot.txt    F5: appearance, stats, sprites, equipment, part matrices
    anim-*.cwa      F4: animation recordings (start/stop)
    gear.txt        F8: gear survey (also writes the game-wide files below)
<game>\Extract\class-rules.txt      the game's class check, the same for every character
<game>\Extract\item-functions.txt
```

F7 switches the loaded character to the class's other specialization, which is what a town's guild receptionist does. The game then plays that subclass's own skills. Press F7 again before leaving the world, because the game saves the specialization.

Recorded so far, all at level 1:

| Class | Specialization | Folder | Skill ids recorded |
|---|---|---|---|
| Warrior | Berserker | `warrior-berserker` | 58, 60, 95, 55 |
| Warrior | Guardian | `warrior-guardian` | 58, 60, 95, 149 |
| Ranger | Sniper | `ranger-sniper` | 23, 26, 56, 80 |
| Ranger | Scout | `ranger-scout` | 23, 26, 56, 22 |
| Mage | Fire | `mage-fire` | 31, 32, 83, 51, 47 |
| Mage | Water | `mage-water` | 33 (basic), 34 (special), 83, 51, 144 |
| Rogue | Assassin | `dieudeglace-assassin` | 20, 19, 160, 80 |
| Rogue | Ninja | `dieudeglace-ninja` | 20, 19 |

Each class starts with its own weapon: warrior greatmace, ranger bow, mage staff, rogue daggers. The surveys also measured the weapons each one carries; the warrior carries swords and a shield.

Ninja: 18 is Intercept (L), and 108 is Shuriken, which K becomes when the rogue has MP. X in the extractor fills MP for recording, because the game only gives MP for hitting enemies. Ninjutsu (R) is a 10-second buff with no animation of its own, and Sprint (Shift) is a speed change.

Moves that did not fire in a take are still missing:
- the warrior's and rangers' moving dodge (move + P), which needs two keys at once;
- the Berserker's and Sniper's ultimate;
The Water mage's basic and special were subsequently captured together with the read-only recorder; its earlier take contained only the end of ability 34 and had incorrectly identified it as the basic attack.

### Controls in CubeVulkan

`src/ClassMoves.h` holds the per-specialization ability ids from the table above. The engine uses the game's default controls:

| Control | Move |
|---|---|
| Left mouse | basic attack (the rogue alternates two hits) |
| Right mouse | special; the ranger holds a bow charge and releases into ability 56 |
| Middle mouse | dodge |
| Shift | class skill (when the class has none, Shift runs fast as before) |
| P | tap skill |
| R | ultimate |

Moves missing from a character's recording are skipped, and the log's `Moves:` line lists what each character has.

Mouse movement now turns the camera during gameplay, leaving right mouse available to begin its attack on press. Menus and inventory release the cursor. `[Input] MouseLook=0` restores the earlier right-drag camera and tap-special controls.

### Animation fidelity corrections

Native matrices are normalized by the entity's facing in **degrees**, rather than by the animated root's angle. This preserves torso wind-up and attack turns. CWA2's entity position supplies the height reference, retaining root bobbing and lunges. Consecutive uses of one ability split when the native ability clock resets. Idle/walk clips exclude active attacks. Rotation interpolation preserves scale and mirrored feet; ability samples use the game's phase clock, and Cyclone also retains its recorded entity turn.

Basic attacks and the measured casts use the supported Steam build's three-phase duration rules, including separate integer truncation after each haste division. Ranger 23 is about 0.54 seconds with the recorded haste, rather than the 0.95 seconds between recorded clicks; Water 33 is about 1.49 seconds and Water 34 about 2.88. The fixed middle phase of mage casts is not divided by haste. `src/NativeAttackTiming.h` contains the measured rules. Clicks queue the next swing and holding left mouse repeats full swings, replacing the earlier universal 0.45-second interruption.

`tools/RecordCharacter.py` can capture an already-loaded matching character without installing the mod or restarting. It uses read-only process access, verifies the installed executable path and supported build, and writes CWA2 only to that character's existing extraction folder. It changes neither native character data nor native saves. Run it without `--seconds` to check readiness; with `--seconds 120` it records for two minutes. The game must keep the character visible so its graphics poses are updated. This does not replace a snapshot or gear survey.

Full parity is still limited to the captured weapons/poses. Alternate weapon animations, all charged/cancel transitions and spell/projectile effects are not comprehensively captured or visually verified. Damage is approximate. Mana/stamina are simulated with measured and provisional rules listed in [progression.md](progression.md).

- Attacks and specials hit monsters within the class's reach: 2.6 blocks for melee, 18 for the ranger and 12 for the mage, in front of the character.
- `--auto-attack` now steps up to the nearest monster, so any class's combat can be tested.

To record another class:
1. Create the character in Steam Cube World.
2. Load into the world.
3. Press F5 for the snapshot.
4. Press F4, walk, jump and use the class's attacks, dodge and skills, then press F4 again.
5. Press F8 for the gear survey.

## Picking a character in CubeVulkan

- The title's character select lists the characters of `Save/characters.db` (see below). Each one moves with
  the newest recording of its class and specialization, or else of its class, and wears its own look and gear.
- `--play` skips the title and plays a recording:
  - `--character NAME`, or `[Game] Character=NAME` in `CubeVulkan.ini`.
  - NAME can be the character's name, its folder (`warrior-guardian`), a class (`warrior`) or a specialization
    (`guardian`). A class or specialization picks the newest matching recording.
  - Without either, CubeVulkan plays the most recently recorded character.
  - Its progress is the character of `characters.db` with the same name and class, made at its first save.
- `--list-characters` logs every character and quits.

## CubeVulkan's progress, in the game's own format

CubeVulkan saves in Steam Cube World's own format (`docs/specs/native-saves.md`, `src/NativeSaves.*`), in
its own `Save` folder beside the engine:

- `Save/characters.db`: `num`, `slot` and one zlib stream per character (the character blob, version 11);
- `Save/world.db`: `state`, the world (version 44): the day, the time of day, the dead creatures, the rewards
  given and the zones' states.

**What a character keeps.** `src/NativeProgress.*` turns a character into the progress gameplay uses and back.
A character keeps:

- its position (world units, the record's place a block above the feet);
- its health, level, experience, class, specialization and gold;
- the 11 worn items and the six inventory tabs. The records already saved are kept whole, so attributes
  survive.
- its name and looks;
- the recipes;
- the cells it used, the places and lands it discovered, the finished and solved missions and their timers;
- its flight points and map stars.

Fields CubeVulkan does not model are kept as they were read.

**CubeVulkan's own notes.** CubeVulkan's own quests, the places townspeople told the character about, and the
zone items and objects it took or used have no place in the game's format. They are kept as text in the same
table, under `cubevulkan:<index>`. The game reads only `num`, `slot` and the numbered keys. These notes move
with their character when one before it is deleted.

**What the game does not save.** As the game does on load, the facing is not restored, and the death
position and tombstone are not saved.

**When it saves.** It saves every 60 s, when the engine closes, on Start Menu, and when a character is
created.

**The world.** `world.db`'s day and time drive the world's clock and the generator's world day. Its dead
creatures stay dead.

**The zones' states** (`src/ZoneStates.*`, native-saves.md §5.3). With the game's creatures, each zone near the
player keeps the game's lists: its static objects (the AI's and the use key's, with each object's active byte),
its ground items, its block changes, its place counters and its count of dead tracked creatures. When a zone is
made, its stored state from `world.db` goes on top as 0x2ae0c0 puts it: the saved objects and ground items
replace the generated ones, the block changes are put back in order and the counts are set. When the zone
leaves memory (more than five zones from the player) its state is recorded (0x2d3e20), and `world.db` keeps it
at the next save. So a chest opened (its active byte cleared, `Open` no longer offered, whichever character
comes), an item given to an object, a door left open, a ground item taken and a block the abilities broke stay
that way. When the engine closes, the zones still in memory are recorded too (the spec's open question 3 leaves
open whether the game does). Not kept: items that creatures dropped (CubeVulkan's own loot, not the zone's
list) and the place counters, which nothing in CubeVulkan changes yet (they are kept as read).

**Moving the old saves.** On the first normal run, CubeVulkan's old text saves move into `characters.db`, once:
`Saves/<name>-<class>.sav` and the `cubevulkan.sav` Steam Cloud bundle, the newer copy of each character.
Their kills join `world.db`'s dead. The old files are left where they are. A position in another extracted
world is dropped.

**Not written.** `--no-save`, `--capture` and `--benchmark` runs open the saves read-only and write nothing.
`--save-folder DIR` uses another folder (testing).

### Steam Cube World's own save is never written

The game's `Save/characters.db` and `Save/world.db` are the owner's real saves, which Steam Auto-Cloud syncs.
CubeVulkan never writes them:

- `[Saves] ImportFrom=<the game's Save folder>` in `CubeVulkan.ini` copies the two files into CubeVulkan's
  `Save` folder. Each source is opened with `SQLITE_OPEN_READONLY` and copied whole by SQLite's backup.
- It only does so while CubeVulkan has no file of that name yet; delete CubeVulkan's own to import again.
- There is no import by default.
- If CubeVulkan's `Save` folder is the game's own (the engine placed in the game's folder), nothing is saved
  at all.

CubeVulkan no longer writes its progress to Steam Cloud. Steam Cloud is only read, once, for the old bundle.

## The game's save format

See `docs/specs/native-saves.md`, which corrects the earlier partial decoding:

- The equipment starts at 0x3e of the inflated character, and the name's length is at 0x71e.
- `world.db` holds the version, then the day, then the time of day (f32 ms), then the dead creatures, the
  rewards and the zone states.

`tests/SaveTests.cpp` checks the format against the spec:

- the character and world blobs read and written back byte for byte;
- a truncated blob loading with defaults;
- negative gold loading as 1,410,065,408;
- the recipe {1, 7} added on load;
- zlib (`78 9c`);
- the four table statements;
- deleting a character renumbering the keys above it, with `slot` left alone;
- the read-only import.

Every save the tests use is a test character made by the writer, in a temporary folder.
