# Creatures

How monsters behave in the supported Steam build, and what CubeVulkan reproduces.

**How it was measured**
- On 2026-10-22, in the vanilla game: no mods, only ZoneExtractor, with the TEST warrior.
- The extractor's **AI trace** (key 0) writes every creature within 120 blocks to `Extract\ai-trace.txt`, 10 times a second:
  - position, velocity, ability and its time;
  - health, combo and effects.
- Its **approach test** (= with the trace on) works on the nearest monster:
  1. It puts the player 50 blocks away, then 1 block closer every 1.5 s.
  2. When the monster closes in, the player stays put to see whether it attacks.
  3. After 12 s of fighting, the player backs off 2 blocks every 1.5 s.

## Measured

| What | Value | Basis |
|---|---|---|
| Notices the player | 27–28 blocks | 1 trial (level 1, race 71): closing in at 27, nothing at 28 and beyond |
| Chase speed | 11.5 blocks/s | both trials. The same as the player's top walking speed (11.57) |
| Starts an attack | within 3.1 blocks | 20 swings at 1.8–3.1 blocks |
| Stands off | about 2 blocks | 1.77–2.07 while swinging |
| Swing | ability 76, about 1.02–1.05 s; back to back while in reach (about 1.1 s apart) | 241 swings by race 71, 88 by race 294. The first three of a fight came about 0.6 s apart |
| Gives up | 40–44 blocks | 2 trials: chasing at 38.1 and 41.7, gone at 40.1 and 43.7 |
| After giving up | walks away at about 5.8 blocks/s | both trials. Taken to be walking home, which isn't confirmed |
| Wandering | median 3.3 blocks/s, mostly 1.4–5.3 | 32,000 samples while idle. CubeVulkan keeps each creature's recorded walking speed |

Other abilities seen:
- **122:** idle poses lasting minutes (sitting or sleeping).
- **130:** about 3.9 s.
- **Race 88 (hostility 7):** abilities 1, 2, 3, 12 and 64.

CubeVulkan does not play these yet.

## In CubeVulkan (`src/Actors.cpp`)

Monsters chase from 27.5 blocks at 11.5 blocks/s and attack within 3.1. Each swing lasts its recorded move (1.05 s without one), and the next starts at once. They hold about 2 blocks off, give up beyond 42 and walk home at 5.8 blocks/s. Their hits follow [combat.md](combat.md); they stop while stunned. `EngineTests` `monsterBehaviour` checks this on the owner's recordings.

## Death is permanent

The game keeps a set of the creatures that have died, in `Save\world.db` (layout in [saves.md](saves.md)). It is saved every 60 s and shared by every character in that world.

A test on 2026-10-22, with the vanilla game and the TEST warrior:
- **Before:** 98 ids in the set.
- **The fight:** the warrior attacked a pack of four level-0 monsters (race 27) and killed three of them. Monster 9122 from an earlier fight also died.
- **After:** the next save held exactly those four new ids, 102 in all. The pack-mate that was never touched was not added.
- **Removals:** the game never took an id back out, and nothing suggests a creature in the set ever returns. Whether one is ever removed (after a long time, or by some event) is not known.

The ids are the creatures' own (the same ones the extractor records). They look like packed zone coordinates plus an index: for example `0x7ffffd40 7fffffc2` for the pack, whose members end in c0–c3.

**In CubeVulkan:**
- Recorded creatures whose ids are in the game's set, or in CubeVulkan's own kill list for that world, start out dead.
- A creature killed in CubeVulkan stays dead; the old one-minute respawn is gone.
- CubeVulkan never writes the game's `world.db`. Its kills go in its own save, as `killed` lines in every character's section.
- On loading, the kills of every character in the same world are joined, as the game shares one world state.
- In multiplayer, the host's kill message now names the creature, so clients remember it too.

## Not established

- **More trials for the ranges:** more monsters, levels and races. The ranges come from one or two trials of level 1 monsters.
- **Whether line of sight, height or level matter.** The player was placed 2 blocks above the monster.
- **Giving up:** whether it is measured from the player or from the monster's home.
- **Fleeing at low health, and companions.**
- **Removal from the dead list:** whether the game ever brings a creature back (after a long time, or with world events). Not seen.
- **Monster skills and their choice:** abilities other than the basic swing, ranged attackers, and casters.
- **Hostility 7 creatures** and their abilities.
- **The native code for any of this.** These are measurements, not decoded rules.

## Where the game's AI runs (found 2026-10-25)

Even in single player, creatures are run by the game's built-in server. It keeps its own world: world+0x358 is clear there, and set in the client's world. The client copies each creature's state from the server's entity updates (0x138e00 unpacks them).

The physics (0x2bb8e0, docs/movement.md) runs in both worlds. The AI runs only on the server, on a thread of its own:
- **0x13830**, the AI loop: for each creature it calls 0x14e00 and copies the result's wanted direction into the creature (+0x40, from its table entry's +0x4c).
- **0x14e00**, one creature: the checks, then the first behaviour that applies among 0x11df0, 0x13530, 0xb330, 0xbac0 (abilities: it clears +0x40 while one runs), 0x12400, 0x103f0, 0x10b90 and 0xdff0.
- Together these are about 30 KB of code, plus helpers.

How it was found: ZoneExtractor's `watch.request` ("<field hex> <seconds> [1 = the server's world]"). It puts a hardware write breakpoint on that field of the nearest non-player creature, on every thread, and lists the writing instructions in `Extract/watch.txt`. With F2 fight mode and the player running away, the monster's +0x40 was written at 0x149ff (394 times) and 0xbbee (95 times).

## The game's AI, ported (2026-10-25)

`src/NativeAI.cpp` is a transcription of the AI thread: 0x13830 and everything it calls. It runs on game-layout creature records, plus the containers the game keeps beside them (`nativeai::Creature`):
- buffs (+0x980) and interactions (+0xab8);
- the aggro (+0xb88) and damage (+0xb98) maps;
- special moves (+0xbd0), inventory (+0x9f0) and group (+0xc08).

The AI's per-creature record is the game's map node (0x178 bytes), with its search containers (`nativeai::Entry`).

**A tick (0x13830), about every 40 ms with the ms since the last:**
1. Each creature the server runs (not players, unless flagged +0x128 bit 9) has its place, size and flags copied into its AI record, then takes its turn (0x14e00).
2. Each AI record with an open search moves on toward its goal (below).
3. Each creature gets its record's way to go (+0x40). A record that is climbing sets ability 0x7a. Waiting interactions are handed over, and an object within 5 blocks is handed to the creature to use.
4. Records of creatures gone from the world are dropped.

**A turn (0x14e00):**
- Mounts (owner of race 0x106/0x107/0x10e) are ridden (0x77).
- Townsfolk with +0x80 bit 13 call out (world+0x170) when they first see a player within 20 blocks, at most every 4 s.
- Then the first behaviour that applies:
  - **0x11df0** (+0xbcc 0x400): rides along with its owner.
  - **0x13530** (objects, hostility 7/8, +0xbcc 0x200): watches the nearest monster within 5 blocks.
  - **0xb330** (+0xbcc 0x4000): guards its owner, staying within 30 blocks while it sees it and healing it (0x1d, buff 0x20).
  - **0xbac0**, fighting. Potions below a quarter of health. The target is the taunt's source, else the angriest (+0xb88), else the nearest hostile it sees within 20 (anger spreads to allies within 8). It closes in with a path search and gives up after 15 s apart. In reach it picks its race's or weapon's move: hits (0x60460), charges (0x60720), the bow's draw, leaps (0x35), dodge rolls (600 ms) and special moves (+0xbd0, every ~10 s).
  - **0x12400**: friendlies turn to a player within 10 blocks.
  - **0x103f0**: a group member (+0xc08) follows the group's first living member, 2 blocks ahead of it, once more than 8 blocks away.
  - **0x10b90**, idling. It walks back home (+0x1b8) when more than 32 blocks away, or 4 with +0xbcc bit 1. Every few seconds it picks one of what +0xbcc allows: sit at a free object within 64 blocks (0x20), stand (4), sleep (8), stroll (0x10), dance (0x100), or walk to a spot in a cell nearby (0x1000, the zone's spots, 0x31c0e0).
  - **0xdff0** (+0xbcc 0x80): companions follow their owner.
- Doors and gates (kinds 3-5, 0x69) that a moving creature touches are handed to it to open.
- Townsfolk sleep (+0x128 bit 0) from 20:00 to 7:00 (world+0xc, the time of day).

**Going somewhere:**
- **0xa950** checks a straight walk. It steps a 2D grid walk up to 50 blocks: up single blocks, down drops, level with the goal when swimming. Then it needs a clear line to the goal (0x121a0) or to be farther than the radius.
- When the straight walk fails, **0x15640** searches blocks: A* over (x, y, z) in the game's order. Steps cost 20 flat and 34 diagonal, or 10 and 14 when standing on something; down is free and up costs 14 or 28. The best is the step nearest the goal.
- A tick allows 50 to 500 steps of search (+0x70), more after failing, fewer after walking straight. 0xedb0 rebuilds the path, and the creature heads for the farthest path block it can walk to straight, at 80 blocks a second.

**Checked against the game:** ZoneExtractor's `ai.request` ("<ticks> <every>") records whole AI ticks into `Extract/oracle/ai.bin` (CAI2). A recorded tick holds both of the world's locks, so the game's other threads wait. The record holds every creature and AI record before and after, every block looked up (0xeb90), every rand() and timeGetTime(), and every cell spot asked for.
- `EngineTests aioracle <tick|-1> <files>` replays each tick.
- On 2026-10-25, 1091 of 1100 ticks came out byte for byte the same (idle 400/400, fight 392/400, town 299/300).
- The other 9 are ticks in which the network moved the player's own record while the tick ran (the test lists them).

**In CubeVulkan:**
- `SpawnedCreatures::think` runs a tick every 40 ms of play, before the physics.
- Each creature has the game's id: ((zy + 0x1ffffff) << 32 | (zx + 0x1ffffff)) << 6, plus its order in the zone (0x119530). The travellers' camp group (+0x910 → +0xc08) refers to those ids.
- Skills, inventory and a companion's owner come from the spawn point.

- The CubeVulkan player is a creature in the AI's world (id 1, hostility 0), so monsters near it turn on it.

**Upkeep (0x2de4c0), ported in `src/NativeUpkeep.cpp`:**
- This is the last of a creature's steps in the world's update. It runs after the physics (0x2bb8e0), the abilities (0x2c5250) and the step after them (0x2c3710), and it runs for dead creatures too.
- What it does:
  - runs the ability clock (+0x6c) and ends abilities;
  - runs down buffs (poison hits every 400 ms, healing every second);
  - counts down the stun and other timers;
  - places riders, held creatures and pets riding along;
  - updates mana (+0x188), sneaking (+0x18c), stamina (+0x9a0) and the dodge (+0x12c).
- **Oracle:** `EngineTests upkeeporacle Extract/oracle/upkeep-{idle,fight}.bin` matches all 10800 recorded calls byte for byte, in the record and in the creature's buffs, aggro and damage maps, hit sets and the world lists it adds to. The local player's 200 calls are skipped.
- **Not covered by these recordings:** poison, healing, sneaking (the light 0xda830 isn't ported), riding, and the dead-list and zone-link parts. ZoneExtractor now records the dead list and zone links too, but it hasn't been re-recorded.
- **In CubeVulkan:** `SpawnedCreatures::simulate` runs it after each creature's physics, so monsters' abilities now end and new ones start. The sounds, damage numbers and poison hits it reports are dropped for now.

**Abilities (0x2c5250), ported in `src/NativeAbility.cpp`:**
- **Why it's from assembly:** Ghidra times out on this function, so it is ported from capstone disassembly.
- **Shape:** it's a 160-case jump table on the ability (+0x68), indexed by ability − 2, with tables at 0x2d00dc and 0x2d0044.
  - Abilities 0 and 1, and 38 others, go straight to the shared tail (0x2cfe95). The tail drops the combo after 4 s without a hit and runs down skill cooldowns (+0xb78).
  - 53 abilities go to the melee branch (0x2c5cf0). Its code finds:
    - where the swing reaches (yaw matrix 0x13050, reach scaled by width);
    - the strike window, from the ability lengths 0x4ead0 and 0x660d0;
    - repeated strikes every `period` ms;
    - blocks with flag bit 0 inside the reach, which it breaks;
    - sounds, particles, mana spending and charge (+0xba8), and crediting dodgers (+0x9c8 vs +0x9b8);
    - the hit search: reach and height, sight 0x2b5d90, hostility 0x50550, dodging (+0x12c).
  - The hit search works out damage through `nativerules::attackPower` (0x4fa70 with 0x4fd40), `spellPower` (0x65c70) and `critChance` (0x50d30). Those rest on the item stats 0x10ad90, 0x1090f0, 0x108d50 and 0x10a1e0.
  - The other 36 branches aren't ported yet; those abilities run only the tail.
- **Oracle:** `EngineTests abilityoracle Extract/oracle/ability-{fight,swing}.bin` replays the recordings.
  - 13,780 of 13,786 calls match byte for byte. That covers the record, buffs, sides, the sounds, particles, numbers and buff notes it adds, and its rand draws.
  - The other 6 apply hits, and every one asks for exactly the game's hit: target, damage, crit, flag, charge, place, push, type and ability.
  - The recordings only contain abilities 0, 1 and 0x4c.
  - The oracle masks a sound's padding at +0x24, which the game leaves as stack garbage.
- **In CubeVulkan:** `simulate` runs it after each living creature's physics.
  - A hit on the player goes to `setPlayerHit`. That applies the target's side of 0x29eb60 as Combat.h has it for CubeVulkan's player: blocking, the mana shield, armor against the attacker's combo, Bulwark, and the stun chance.
  - In a run at (508,-200), the five race-88 monsters landed 47 hits (454 damage) on a player standing still.

**The hit (0x29eb60) and its damage step (0x29dcf0), ported in `src/NativeHit.cpp`:**
- **0x29eb60 decides how a hit lands:**
  - an untouchable target: flag 0x80 on +0x128, flag 0x20 on +0x5c, or ability 0x7d;
  - a block, which drains +0x184, by the block power (0x50370) and the guard 0x5ed30;
  - the mana shield (buff 6);
  - armor or resistance, with the attacker's combo going through it;
  - the attacker's weapon runes (bytes 0x80–0x83 at item +0x20) and its frenzy (buff 2), healing it;
  - poison (ability 0x12, buff 4 at 1/5 of the hit for 3 s);
  - Bulwark (buff 1);
  - the struck sound by weapon id, a critical's cry by race, and the stun roll.
- **The damage number (world+0x90):** 0x29eb60 writes one, and the server applies it through 0x29dcf0:
  - sneaking is lost;
  - the last hitter (+0x9e0), the aggro and damage maps, and their spread (0x2c3200);
  - knockback (+0x4c), dizziness (+0x134) and stun (+0x130);
  - the mana shield's drain, heals and their aggro;
  - a death: the death sound, kill credits (world+0x120), a mount's death, and every creature forgetting the dead.
  - What a death drops (0x2a6860) is the world's `died` hook.
- **Oracle:** `EngineTests hitoracle Extract/oracle/hit-fight.bin` matches 600 of 600 recorded hits byte for byte.
  - It compares the result, both records with their buffs and sides, every other creature's head, +0x9e0 and aggro, every list entry added, the kills and the rand draws.
  - The hits cover the TEST warrior's swings (abilities 2 and 3, client world), monsters hitting the player and each other (0x4c, server world), and two kills.
  - The two kills' loot rolls are the drop's, not part of this port.
  - The oracle masks stack-garbage padding in damage numbers (+0x15..+0x17, +0x47) and aggro notes (+0x14).
- **How it was recorded:** ZoneExtractor's `hit.request` ("<count>") writes `Extract\oracle\hit.bin` (CHT1).
  - The run used a game session with the TEST warrior.
  - Attack was bound to K (`the attack button 37 0`), with controls.cfg restored afterwards.
  - The extractor's F2 fight mode, HP guard (8) and weakest-monster hop (F) were used.
- **In CubeVulkan:** creatures' hits on other creatures, and poison ticks, go through it. A kill is counted in the log, but its drop isn't made yet.
- **Found along the way:** with a shield in the left hand, the game's block power uses the right hand's weapon damage. Combat.h's `blockFactor` uses the shield's power instead, so it's off there.

**The player against generated creatures:**
- `SpawnedCreatures::targets` and `strike` let CubeVulkan's player attack hit the generated creatures through NativeHit. The player's record is the attacker, and the damage comes from CubeVulkan's player model.
- `takeDeaths` reports each death once. A kill by the player gives coins (level + 1) and XP through `creditKill`. The body bursts into the voxels it was drawn with (`takeBursts`) and is no longer drawn.
- With `--auto-attack` at (508,-200), the player killed race-88 monsters this way.

**Effect objects (0x2dba40), ported in `src/NativeEffects.cpp`, with their creation in NativeAbility:**
- **Layout:** made by 0x29a0b0, 0x80 bytes: owner, position, target (+0x20), velocity (+0x44), damage, radius, size, strength, flag (+0x64), kind (+0x68), spec (+0x6c), age, lifetime and burn (+0x78).
- **Kinds:**
  - 0–3 and 7: arrows and bolts that fall;
  - 4 and 10: homing on a target;
  - 5: an orb that returns to its owner;
  - 6: a lasting area that poisons, freezes or heals;
  - 8: drifts;
  - 9: hovers over the ground.
- **Each step:** an effect moves in sub-steps and hits the first creature it touches (box, sight, dodge), or every one for areas, through 0x29eb60. It gives its owner mana, rolls criticals, and stops at walls (the block, then a ray back). A wall hit breaks breakable blocks within 4 blocks. It bursts with particles, a sound and a blast (5 blocks, 10 for homing), and a water bolt turns into a healing area.
- **Shots that create them:**
  - 0x2cf5fa: staffs and wands, abilities 0x26–0x30 and 0x7b;
  - 0x2ceb05: arrows, abilities 0x17, 0x18, 0x1b, 0x6c and 0xa0;
  - 0x2cad1d: the charging hum of 0x19, 0x1a, 0x1c, 0x25, 0x3c, 0x40, 0x41 and 0x93.
- **Oracles:**
  - `EngineTests effectoracle Extract/oracle/effects-bow.bin` matches 400 of 400 arrow steps. Each one's hits run through NativeHit and match too.
  - `effects-mage.bin` (a water mage's healing area) matches 297 of 300. Its 3 heal steps need creature records the first probe didn't keep. The probe now saves creatures within 40 blocks whole, but hasn't been re-recorded.
  - `abilityoracle Extract/oracle/ability-{bow,mage}.bin` (CWU4: the update probe now also records the effect objects created) matches 4918 of 4918 and 4911 of 4911.
  - The oracles mask stack-garbage padding in effect objects (+0x65, +0x6d, +0x7c).
- **Probe:** ZoneExtractor's `effects.request` ("<count>") writes `Extract\oracle\effects.bin` (CFX1). Each record holds the effects, the creatures, the blocks around each effect, the lists gained, the hits with their results, and the draws.
- **Finding ranged monsters:** `EngineTests rangedscan <zx> <zy> <r>`. Bow users (race 104, class 0x95) were at block (−662, −965) and mages (race 68) at (237, −1754).
- **In CubeVulkan:** `simulate` runs the effects after every creature's step, and their hits on the player go through `setPlayerHit`.
- **Not done yet:** the shots aren't drawn.

**Monster skills (more branches of 0x2c5250, ported in `src/NativeAbility.cpp`):**
- **Choosing what to port.** `EngineTests skillscan <zx> <zy> <r>` counts the special moves (+0xbd0) of the monsters a region spawns. Around zone (0, −10) the unported ones were 0x82 (166 monsters, races 69–71, 87, 88), 0x88 (54, race 287), 0x62 (10) and 0x73 (3).
- **Using an item and the healing skill (0x2c5337: abilities 0x54, 0x55, 0x82).**
  - A potion (item category 0x1c, at +0x1d8) gives its buff (0x16–0x1c, 600 s) as its use starts. Id 5 also ends buff 0x1d.
  - Food (category 1) and the skill 0x82 heal every 200 ms: 200 / use time of the maximum health, halved for non-players, with a heal number.
  - 0x82 adds a burst and a sound at 50 ms, and a sound every 400 ms.
- **The beam (0x2c97c1: 0x88).** It ends without the mana for a pulse (1/32). Every twentieth of its length it pays and strikes six points along the aim, out to where a ray stops (at most 5 blocks). Each point hits whom it reaches (radius i + 1): (1.25 − rand/2) × spell power × its share of the hold, push 8.
- **Area casts (0x2c9ffc: 0x62, 0x63).** As the cast ends, an effect object of kind 6 (spec 1 or 2) lasts 5 s: radius 8, damage 2 × spell power.
- **Starts of 0x70, 0x71, 0x73.** 0x29d3c0 is called with 0xc, 1 and 6 rather than the ability, so no buff comes. Only a zero cooldown under that key is set, and a sound plays.
- **Moves the AI picks (from 0x14e00's attacks):**
  - **0x68–0x6b (0x2cb299), a ray spell.** Class 0x97 and staff users whose weapon id is 0xb cast it. It goes from the shot point along the aim, up to 200 blocks.
    - Where it meets a block, the breakable blocks within 4 blocks break.
    - Then it hits, block by block, each creature it passes through.
    - 0x69 and 0x6a pay mana and strike every 200 ms. A fire mage leaves burning ground (kind 6).
    - With no one hit, a spark shows where it ended and the combo is lost.
  - **0x1d (0x2cae0d), healing an ally (+0x1a8).** The charge grows; every 200 ms there is a burst and a tenth of the ally's health comes back.
  - **0x38 (0x2cf1ea), a bow's barrage.** Every quarter of the hold an arrow flies at speed 150.
  - **0x4a (0x2cc920).** A growl.
  - The alpha of 0x1d's and the ray's bursts is a stack slot the game never sets; the port uses 1.
  - Not ported: 0x65/0x66 (class 0x99 summons a creature) and 0x7e (taming: a creature answering pet food becomes a pet). Both add or change creatures in the world.
- **Oracles.**
  - `abilityoracle Extract/oracle/ability-heal.bin`: 1500/1500 (race 71's 0x82).
  - `ability-spells.bin` (race 230: 0x1d, 0x68, 0x69): 1978 of 2000 the same. The other 22 need blocks beyond the recorded cube, because the ray reaches 200 blocks. The masked fields are the ally's health in 0x1d's heal number and the never-set alphas.
  - `ability-beam.bin`: 2845 calls the same, and 153 of 155 pulses ask for the game's hits. The 2 others differ in one critical flag; the target's state (the player, recorded by its head only) decides it.
  - 0x62, 0x63, 0x70, 0x71, 0x73, 0x54 and 0x55 have not been seen in a recording yet.
- **Recording.**
  - `update.request` takes an optional third word, the abilities to record ("3000 2c5250 82,88").
  - Each recorded call now ends with each hit's rand() draws and result. The oracle passes over those draws, so calls that land several hits can be checked.
  - The oracles mask the padding of damage numbers (+0x15, +0x47) and bursts (+0x44..+0x47).

**Drops on a death (0x2a6860), ported in `src/NativeDrop.cpp` with 0x2a5b70 and the item generators:**
- **What drops.** Nothing comes from players, hostility 3 or appearance flag 0x800. Otherwise:
  - one worn item, 1 in 16 (always with flag 0x200);
  - the pet, 1 in 50;
  - for hostility 1 (1 in 20, always with flags 0x40200) and 7 (1 in 20), a generated weapon (0x2d15d0) or armour (0x2d0680);
  - 1 in 8, a special (0x2d0fc0: category 0x15 from the world's lists) or a fixed item;
  - pet food (flag 8 or 0x10, 1 in 20);
  - for hostility 1, coins: ((level − 1) × 0.1 + 1) × (1 + rand), ×10 with flag 0x200, ×20 with 0x40000;
  - 1 to 3 of the race's resources, then for hostility 1 and 7 all it carries.
  - Each lands within a block of the body.
  - The armour generator draws a random cloth material, then overwrites it with the set's base material, for every set but the first. The port keeps this game behaviour.
- **Placing (0x2a5b70).**
  - Gear takes the region of where its creature was made, or of its place cell's landmark when the block it died in has flag 0x20.
  - The drop goes to a player who can use it: one who angered a hostility-1 creature, or one within 256 blocks. If there is none, any player. If there are no players, nothing drops.
  - That player's gear rarity rises 1 in 20, up to four times.
  - The drop falls to the first solid block of a loaded zone.
- **Oracles.**
  - `EngineTests droporacle Extract/oracle/drops-fight.bin`: 30 of 30 recorded deaths, 76 calls and 76 drops, byte for byte. ZoneExtractor's `drops.request` writes CDR1. Kills were made with F (weakest monster), V (0.01 HP) and attacks in fight mode; Z's 0 HP never reaches 0x2a6860.
  - `EngineTests itemoracle Extract/oracle/items.bin`: 2000 of 2000 each for weapons, armour and specials, on scripted rand(). `items.request` writes CIG1 with the world's special lists (+0x360, +0x378).
- **In CubeVulkan:** `SpawnedCreatures` runs 0x2a6860 for every death and `takeDrops` hands them over.
  - The player's kills give the game's number of coins.
  - Gear that the survey stats cover drops in a pouch.
  - Other kinds are logged as "not carried yet": resources, food, pets, specials, and gear without survey stats.

Not yet:
- **Carrying every drop:** resources, pet food, pets and specials need inventory support. Gear stats should come from NativeRules (0x10ad90 and the rest) instead of the survey.
- **The other ability branches** (about 25, such as leaps, spins, pets and spells) run only the tail.
- **Drawing effect objects:** the arrows, bolts and areas themselves.
- **Knockback:** the push the player takes.
- **Breaking blocks:** there's no world API for it.
- **Ability output:** the sounds and particles the abilities report are dropped.
- **The player's dodge** (+0x12c on the player's record) isn't set, so monsters' swings don't miss a dodging player.
- **Cell spots (0x31c0e0)** aren't given, so townsfolk who pick 0x1000 only wait.
- **Interactions** handed to creatures are taken up by `src/NativeInteract.cpp` (0x2c3710, run after the ability step for living creatures, on the server).
  - Only zone objects (kind 5) are ported. A free seat is taken: its user (+0x30) is the creature, which stops. Doors, gates and hatches swing (0x2d3a10: +0x28, swing time +0x2c, a sound by kind).
  - Not ported: the other kinds, locked objects and chests, and the innkeeper's night.
  - `interact.request` (CIN1) records such calls for `EngineTests interactoracle`. No call came up in 15 minutes near townsfolk, so this part is unverified. CubeVulkan logs how many objects creatures took up.

## Road travel ([Creatures] RoadTravel, CubeVulkan's own)

Off by default, and not the game's: with `RoadTravel=1`, groups of wild creatures and travellers walk the roads from place to place.
- **The game's AI does all the walking.** Packs become groups (wild creatures of the same hostility made one after another within 8 blocks), so they follow their first living member (0x103f0).
- **The leader heads home** when more than 4 blocks from it (+0xbcc bit 1 set), and its home is moved along the road: the generator's `roads` (`worldgen::columnValues`), a waypoint every 12 blocks, the straightest turn first.
- **At the edge of the loaded area,** or after 20 s without reaching a waypoint, the pack moves on or turns back.
