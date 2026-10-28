# Creature poses (the game's 0x17ab0)

The game poses and draws every creature in one function, 0x17ab0 (78 KB; Ghidra's decompiler times out on it). From the creature record it:

1. picks each body part's sprite: worn items through 0x173b10, else the appearance's model ids (creature+0x90..0x9e) in the world's model list;
2. works out target values for the hands, arms, body, head and feet from what the creature is doing: its animation state (animation state+0xc, a jump table of 102 entries at 0x2af94; +8 is the state's timer in ms), its ability (+0x68) and time (+0x6c), the stride (+0x998, +0x99c), flags and race;
3. eases the pose fields of the animation state (creature+0xc38, +0x20..+0xd0) toward those targets, frame-rate independently (0x14000a7a0: a step of rate·0.02 per hundredth of the frame time);
4. builds the 33 part matrices (+0x138 body-with-head, +0x178 hands, +0x1b8 body, +0x1f8 head, +0x238 hair, +0x278/+0x2b8 the arm parts, +0x2f8/+0x338 weapons, +0x378/+0x3b8 hands, +0x3f8 shoulders, +0x438 neck, +0x478/+0x4b8 wings, +0x4f8/+0x538 feet, +0x578 tail), the weapons' 16-point trails (+0x5bc..+0x8bb) and draws the parts (0xee2f0; 0xee990 mirrored; 0xeec50 a weapon's effect).

`src/NativePose.cpp` is its transcription, in the game's order of float operations, working on a game-layout creature record. Every case of the jump table is transcribed (states 0xc, 0xd, 0xe, 0x25, 0x46 and 0x53 go straight to the common code). The cases share register values with the code after the switch; the `Entry` enum stands for the eight places they rejoin it (0x19e63..0x19e9d), and `standUp` and `bothHands` for tails several cases jump into. The rules it calls are inputs (`nativepose::Call`), and `src/NativeRules.cpp` computes them as the game does:
- the ability's three phases and their sum (0x4ead0, 0x660d0, 0x5e9f0, 0x4f220), divided by the attack speed (0x66460);
- the attack speed: the berserker's and two other specs' combo (0x5f9e0 caps it by weapon), each worn item's speed (0x10a490 over its power 0x109c50 and effective rarity 0x109720), buff 0xc, and three appearance flags;
- an item's use time (0x108fa0);
- whether a worn item is usable (0x50640: any creature but a player, else the class check 0x1094d0);
- 0x5e9b0 (a two-handed weapon in either hand) is in NativePose.

Buffs are asked through a callback (the game keeps them in a list at +0x980). The item models (0x173b10) come from the caller.

`EngineTests rulesoracle <files...>` checks the rules against the values the game gave in the recordings. On 2026-10-24 all 13000 CWA3 records (abilities 0, 2, 3, 12, 55, 64, 76, 95, 130, 148, 149) matched for every rule, including 117000 usable checks.

`cosf`, `sinf`, `asinf` and `powf` are called from ucrtbase.dll (`src/NativeCrt.cpp`), as the game does: the static CRT's versions differ in the last bit for a few angles.

## Checking it

ZoneExtractor's animation oracle: write a number N to `<game>\Extract\anim.request` while in a world, and the next N calls of 0x17ab0 go to `Extract\oracle\anim.bin` (CWA3): each with the whole creature record as the call finds it, its arguments, the rules' values and the models it can ask for. A creature's next record holds what the call made. `EngineTests animoracle <state or -1> <files...>` runs our pose on every record and compares the pose fields, matrices and trails with the next record, bit for bit.

The probe reads `timeGetTime()` last, just before the game's call; the pose reads it late (races 0x11d, 0x25..0x28, 0xef..0xf1, 0x125 and 0x126 beat with it), so the test also accepts the next millisecond.

2026-10-24, the TEST warrior as berserker and guardian, idle, attacking and in a fight with a monster (race 294) and its neighbours (`Extract\oracle\anim3-*.bin`, 12976 calls; the earlier CWA2 files in `oracle\cwa2` lack 0x5e9f0, which the trails need):

| State | Calls | Pose fields, matrices and trails |
|---|---|---|
| 0 (idle, walking, running) | 8886 | all exact |
| 1 (a charged blow) | 766 | all exact |
| 6 (weapon at the ready) | 178 | all exact |
| 0xf, 0x10 (sword swings) | 914 | all exact |
| 0x1d (a spinning swing) | 469 | all exact |
| 0x2d (the monster's attack) | 1032 | all exact |
| 0x41 (shield bash) | 121 | all exact |
| 0x54 | 139 | all exact |
| 0x57 | 314 | all exact |
| 0x59 | 157 | all exact |

5 of the calls needed the clock's next millisecond.

Not yet checked against the game: the other states (other classes' and creatures' moves). ## The generated world's creatures

With `[World] Source=Game` (or `--set World.Source=Game`), the spawn points the world generator (`src/worldgen/`) makes become creatures the way the game makes them, and are drawn by the pose above (`src/Spawned.cpp`):

- **Appearance (0x53e80, `src/worldgen/Looks.cpp`):** colours, part models, sizes, offsets and flags.
  - Each race's fixed values are the game's, read from `Extract/oracle/looks.bin`. What depends on `rand()` follows the game's rules draw for draw.
  - This includes the flags already set on the spawn point, which can grow the size (0x20000 and 0x200 ×1.5, 0x40000 ×7, held to 8 wide).
  - `EngineTests looksoracle` matches all 7704 game-made appearances byte for byte, with the same number of draws.
  - `EngineTests allspawns` matches the appearance of every spawn point the game dumped (struct padding aside).
- **Creature record (0x2e17d0, `src/worldgen/Residents.cpp`):** the constructor's defaults (0x4ce80), then the spawn point's place, home, race, level, class, appearance, worn items, abilities, region and full health (0x5fac0, `nativerules::maxHealth`).
  - Max health matches every unhurt creature in the recordings.
  - A creature with flag 0x4000000 turns hostile and grows ×1.1, unless a player there has state flag 0x800 (none recorded has).
- **Standing:** the game's own physics (0x2bb8e0, `src/NativeUpdate.cpp`, docs/movement.md) runs for every creature, so it stands where the game puts it. The AI is not ported yet, so creatures do not walk.
- **Hair:** drawn with the appearance's colour and without its marker voxels (the model loading), recognised by the appearance's hair model id.
- **Worn items' models (0x173b10, `src/NativeItems.cpp`):** the model ids come from the game's own lookup.
  - Its table is `Extract/oracle/item-models.bin`: ZoneExtractor's `itemmodels.request` asks the game for every category, id, material, rarity and modifier that can matter. The current extractor writes CWI3: ids 0–0x12f plus 0x130 for every higher id, materials 0–63 and 0x80–0x8f, the modifier % 660 (the remainders 2, 3, 4, 5, 6, 10, 11) and category 1 id 1 by raw rarity (docs/specs/item-models.md §4). The first extraction (CWI2: ids 0–127, materials 0–63, modifier % 330, 394 distinct tables for 241,101 keys) is still read.
  - `EngineTests rulesoracle` matches all 117,000 recorded item slots.
  - Not covered: category 0x17 (models the game makes on demand), and category 0x16 id 0x3f, whose model goes by the id at +0x18.

Not yet: state 0x61's sparks (a list at +0x8e0), and the creatures' AI, which decides where they go.
