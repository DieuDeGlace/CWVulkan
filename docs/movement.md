# Movement and the body

Measured 2026-10-22 in unmodded Steam Cube World (other mods and ReShade set aside), with a level-1 human warrior (the "Test" character). The extractor logged the player every tick it moved: its own position, velocity, flags and the game's blocks around it.

| | The game | CubeVulkan (`src/Body.h`) |
|---|---|---|
| Body width | The snapshot's `hitbox` scale is 0.96 for every recorded player. Three step-ups fired when the centre was between 0.4751 and 0.4834 from the step face: the body's edge, half of 0.96 | Square footprint ±`hitboxScale / 2` (read from the character's snapshot). Walls stop the edge; any part over a floor holds the body up |
| Steps | A one-block step is climbed at once (z +1.0013 in a single tick) when the edge reaches it | Steps up to 1.1 blocks, effectively at once |
| Walking speed | Holding a direction: 11.57 blocks/s, reached in about 11 ticks. dv/dt = 82.1 − 2.615·v fits all 11 logged ticks of three identical starts (RMS 0.0035) | The same rule, stepped in 1/64 s ticks |
| Stopping | With no direction the speed decays ×0.9204 a tick (5.31/s) | The same |
| Height above blocks | Standing, the position's z is 1.08 above the top of the highest solid block below, in the zone's block indexing. Probably an index offset of one plus a 0.08 rest height; not resolved | Unchanged |
| Body height | `physics` scale 2.16 is a candidate, not verified | Walls tested at 1.2 and 1.9 above the feet, as before |

Not yet measured: corners (square or round footprint), the body's height and ceilings, jumping, falling, slopes, swimming, climbing, gliding, sprinting, other races and sizes. Trees and other placed objects are sprites, not blocks: the block probe does not see them, and CubeVulkan does not collide with them yet.

## Recording it again

The extractor (`tools/ZoneExtractor`) has these keys:

| Key | Does | Writes to the game? |
|---|---|---|
| **9** | Toggles `Extract\collision.txt`. Every tick the player moves: position, velocity, flags, and the gaps to the nearest solid block faces, at four block levels in each direction | No |
| **7** | Holds the player's velocity at 4 blocks/s toward the nearest wall two blocks tall (within 16 blocks along ±x/±y) for 5 s | Yes (test harness) |
| **8** | Keeps the player at full health | Yes (test harness) |
| **N** | Sets the time to noon | Yes (test harness) |

Back up `Save` first and restore it afterwards; monsters killed the test character three times during the measurement before 8 was added.

## The game's creature physics (0x2bb8e0)

`src/NativeUpdate.cpp` transcribes 0x2bb8e0, the physics the world update (0x2da6c0) runs for every living creature each step, in the game's order of float operations. The game splits a frame into steps of 16 ms (whole milliseconds, at most 1000 a frame) with the rest last (0x1400f5520). The function covers:
- the top speed and the abilities' dashes;
- gravity, gliding, swimming, climbing and the knock-back (+0x4c);
- pushing creatures apart (a rand() direction when exactly on top of each other);
- the move in steps of at most 2/3 block, axis by axis, against the blocks' shell and the world's objects, with the one-block step-up (66191 dots) and the step down off ledges (+0x990 smooths both);
- fall damage and lava for the local player;
- the contact flags (+0x5c: 1 ground, 2 water, 4 wall, 0x10 partly out of water, 0x20 stuck, 0x40, 0x80/0x200/0x400/0x800/0x2000 by the block underfoot, 0x100, 0x1000, 0x4000);
- the wall normal (+0x9ac), the lean (+0x2c) and pitch (+0x28, +0x58), the stride (+0x998 strength, +0x99c phase) and footstep sounds;
- turning toward the way it moves (+0x30).

What it reads of the world comes through `nativeupdate::World`. The block lookup is `ExtractedWorld::gameBlock`, the game's 0xe9a0/0xeb90:
- a missing zone, and anything below a column's stored blocks, is grey stone (type 1);
- air above the column is type 0 above sea level and water (type 2) at or below it;
- an empty block at or below sea level is water too, unless its flag 2 is set.

**Checking it.** ZoneExtractor's `update.request N` records the next N calls in `Extract/oracle/update.bin` (CWU2). Each record holds:
- the creature before and after the call;
- the dt and the world's time;
- the creature's buffs;
- the creatures within 40 blocks and the objects near it;
- the creatures its record names;
- the blocks within 5 of its box, and whether their zones are loaded;
- every rand() the call drew (the probe swaps the rand import, 0x42cd28).

`EngineTests updateoracle <files>` replays each record through NativeUpdate and compares every byte.

2026-10-24 results:
- `update-idle.bin`: 2935 of 2935 updates exact, 2312 of them moving.
- `update-fight.bin` (F2 fight mode, a race-217 monster): 5887 of 5887 exact, 4316 moving, 187 attacking.
- The local player's updates are left out (65 + 113).
- Not covered by these recordings: water, falling and jumping, climbing, gliding, other abilities' dashes, objects (lifts, doors), and the local player's paths (glide stall, fall damage, lava, cold).

CubeVulkan runs it for the generated world's creatures (`SpawnedCreatures::simulate`). They now stand where the game's physics puts them: 32 of 32 near the start were on the ground. The 1.08-block settle is gone. They do not walk yet: walking needs the AI (the code after the physics: 0x2c5250, 0x2c3710, 0x2de4c0) to set where they want to go (+0x40).
