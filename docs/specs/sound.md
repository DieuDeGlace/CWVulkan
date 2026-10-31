# Sound and music

Follow-up evidence: `remaining-runtime-gaps.md` §8 resolves the music-fade
coefficients (old question 3) and sound-queue lifetime. Use that independently
authored clarification together with this specification.

Further native evidence: **steam-sound-event-addenda.md** resolves old
questions5/12 and the area block-breaking pitch base, corrects the map-list
sound site's earlier pickup/salvage label, and specifies sampled UI open/
close edges. **steam-chat-feat-presentation.md** supplies the received-feat
consumer's actual UI without inferring an award from its sound filename.

Behavioural specification of Steam Cube World's sound system: the sound list, how a sound is
placed and mixed, the music choice and its fades, the ambient sounds of the world, and which
events play which sound. Written from the decompiled executable (RVAs, image base 0x140000000),
its assembly and the binary's constants. The game update 0xa5a50 did not decompile in the main
export; it is cited from its longer-budget retry and its assembly. The last section compares
this with what CubeVulkan does today.

Vocabulary from other specs: the **sound request** record and the world lists (`creature-hit.md`
§0.3, `creature-upkeep.md` §2.2), creature fields (`creature-upkeep.md` §1.1), static objects
and zones (`worldgen-creatures.md` §0.2). "Game" is the client object (its constructor is
0x7ea80), "world" the world object at game +0x08, "the player" the local player creature at
world +0x448. Times are milliseconds; distances are blocks unless a step says world units
(1/65536 block).

---

## 1. The audio engine

### 1.1 Output

0x34270 initialises COM, creates XAudio2 (XAudio2Create, imported from XAUDIO2_8.DLL) and one
mastering voice with default channels and rate (stream category 6, game effects). It then makes
two **music streamers** (0x1e0370 bytes each, volume 1.0) at engine +0x48 and +0x50. The
engine object lives at game +0x28; its methods (vtable 0x42eda0) are:

| Slot | RVA | Use |
|---|---|---|
| +0x08 | 0x33920 | per-frame clean-up of finished sound voices (§1.4) |
| +0x10 | 0x34910 | play music (main track, second track, loop flag) |
| +0x18 | 0x35290 | per-frame feeding of both streamers |
| +0x20 | 0x34f90 | stop both streamers |
| +0x28 | 0x343a0 | is the main streamer playing |
| +0x30 | 0x34aa0 | set the two streamers' volumes |
| +0x38 | 0x343c0 | play a sound (§1.3) |
| +0x40 | 0x34b10 | load a sound by file name, return its handle (§1.2) |

There is no 3D audio library: X3DAudio is not used, and all sounds are voiced in mono or stereo
as their files are, straight into the mastering voice.

### 1.2 The sound list

Sounds are numbered 0..0x7c (125 sounds). 0xa3a40(game, n) maps number n to a file name under
`Sounds\` with one switch case per number and asks the engine to load it. The constructor
preloads all 125 in order ("preload sounds", 0x7ea80), so a number's engine handle is n + 1.
Several numbers share a file (for example 0x14 and 1, 0x15 and 0x55 and 8, 0x3b and 0x2b,
0x41/0x42 and 0x3f/0x40). The list itself is game data: CubeVulkan should take it from the
user's install with the extraction tool (it is the switch in 0xa3a40), not copy it into the
repository. File names appear below only where an event needs one to be recognisable.

Loading (0x34b10) caches handles by file name. When the engine has a data database (engine +0x58)
the file is the blob under that name, de-obfuscated by 0x66f10; otherwise the file is read from
disk with CreateFileW. Steam ships the sounds as loose files in `sounds\`. The loader walks the
RIFF chunks, requires the form type `WAVE`, copies the `fmt ` chunk as the wave format and the
`data` chunk as the samples, and marks the buffer end-of-stream (flag 0x40). Anything else is
not loaded (handle 0, which plays nothing).

### 1.3 Playing a sound (0x343c0)

Arguments: handle, volume v, (an unused float), frequency ratio p.
1. Nothing happens if XAudio2 is missing, the mastering voice cannot be made, or **v ≤ 0**.
2. Look the handle up (an unknown handle gets an empty entry and plays nothing).
3. Create a source voice with that format, flags 0 and **maximum frequency ratio 2.0**
   (0x4ac9d0); set volume v; submit the buffer; start; set volume v again; set the frequency
   ratio to p. XAudio2 clamps p to at most 2.0, so every requested pitch above 2.0 plays at
   2.0 (open question 2).
4. Append the voice to the engine's active list (engine +0x18).

A sound always plays from its start to its end. There is no looping sound effect, no per-sound
stop, and no fading of sound effects.

### 1.4 How many play at once

There is no limit: every request makes its own voice. 0x33920, run once per frame from the game
update, asks each active voice for its state and destroys the ones with no buffers queued.

---

## 2. Placing, attenuation, volume and pitch (0xa0420)

0xa0420(game, number n, position P (3 × i64), volume v, pitch p) is the single way sounds reach
the engine:

1. The **listener** L is game +0x1dc0..+0x1dd0: the camera's eye, written by 0xadcd0 as the
   camera focus minus the view direction times the camera distance.
2. d² = |P − L|² in blocks (each axis × 1/65536, in floats). If **d² > 4096** (0x438000), the
   sound is dropped (beyond 64 blocks).
3. Attenuation a = 1.0 − d² × (1/4096) (0x437f64), so it falls with the square of the distance
   from 1 at the listener to 0 at 64 blocks.
4. A pan value is computed: P in clip space through the view-projection matrix (game +0x1ee0..),
   x / w × −0.1, clamped to [−1, 1]. It is passed to the engine, which ignores it. **There is
   no stereo panning.**
5. If n is one of 0x6d..0x77 (the instrument sounds), the music duck timer game +0x204c becomes
   at least 6000 (§3.5).
6. Final volume = soundVolume × v × a × 0.01, where soundVolume is the integer option
   `soundVolume` (game +0x1ba4, from `options.cfg`, written by 0x13b1d0). Pitch is p unchanged.
7. Play handle 0xa3a40(game, n) (§1.3).

**Interface sounds** use 0xa03f0(game, n) = 0xa0420 at the listener with v = 1 and p = 1, i.e.
soundVolume × 0.01 at normal pitch. A few interface sounds call 0xa0420 at the listener directly
with the same values.

### 2.1 Playing the world's requests

Every frame the game update walks the world's sound-request list (world +0xa0) and, for each
request in list order, calls 0xa0420 with its position (+0x00), number (+0x18), volume (+0x20)
and pitch (+0x1c) (0xa5a50 at 0xaa570). This confirms the field order the other specs
assumed: +0x1c is the pitch and +0x20 the volume. The owner (+0x28) is not used here.

**Corrected/superseded arithmetic:** after attempting number0x54 (`Explosion.wav`) or0x55,
camera shake (game+0x1e00) is raised only for a strictly larger candidate
**max(0,1−d²/2500)**. Here d² is float **squared** distance in blocks from the sound to the
player's physical fixed position, not linear distance or distance from the listener eye.
The former D/2500 and “always about1” wording was incorrect. The shake candidate reaches0
at50 blocks; sound attenuation independently uses64 blocks from the eye. Playback failure,
volume and audibility return do not gate the shake update. Camera decay then occurs in the
same frame with its independent0.005 coefficient before view translation. Exact fixed/float
conversion, operation order, finite references and cosine phases are authoritative in
[gameplay camera §§2/4/7.2](steam-gameplay-camera.md), native0xaa594–0xaa5f3/0xadcd0.

---

## 3. Music

### 3.1 Files and the streamer

Music is Ogg Vorbis read from `Music\<name>` (the path is relative to the install). A streamer
(0x34600) stops what it was playing, opens the file with the Vorbis library (0x31f460), and
plays it as 16-bit PCM at the file's rate and channels through its own source voice, with three
buffers of 0xa0000 bytes (640 KiB) used in turn. Each buffer is cleared and filled completely;
the last one of a track is padded with silence. Per frame (0x35030), when fewer than two
buffers are queued it fills the next one. With the loop flag set, reaching the end of the file
seeks back to the start (0x31f4e0) and keeps filling the same buffer, so the loop has no gap
beyond the padding of nothing. Without it the last buffer is marked end-of-stream and the voice
stops after it.

The music thread (0x83fb0) always plays with the loop flag on and never uses the second
streamer (it passes an empty second name), so in practice one looping track plays at a time.

### 3.2 The music thread (0x83fb0)

Every 16 ms, until the game stops:
1. Under the game's lock +0x1d38, copy the requested path (game +0x2028) and the music volume
   (game +0x2048).
2. If the volume is ≥ 0.01 (0x42e51c), set the main streamer's volume to it; otherwise **stop**
   both streamers.
3. If the requested path differs from the one it last started, start it (looping), set the
   volume again, and remember the path.
4. Feed the streamers (0x35290).

So a track changes only through silence: the old track is faded down by the game update until
the volume drops under 0.01, the thread stops it, and the new path is started at whatever volume
is then requested.

### 3.3 Which track (in the game update)

Each frame the game update computes a desired track. With the player's block position (x, y),
its zone Z = (x ÷ 1024, y ÷ 1024) floored (0x9beb0), the region's biome B (0x2b9080 of the
player's region +0x150/+0x154) and the place kind K of that zone (0x2b99d0; when the player is
inside a building, 0x2aa270 instead; outside, K counts only while the place weight (0x9bee0:
(1 − d²/r²)², r = 250, or 500 for a double zone, d² as returned by 0x364e0) is at least 0.5, else K = 0):

1. **Title**: if Z = (0, 0) and K ≠ 0: `maintheme.ogg`; not "place music" (see §3.4).
2. **Shrine**: else, if the column record of the player's (x, y) (0xeab0) has bit 0x20 in its
   byte +5: `shrine.ogg`; not place music.
3. Else, by biome B: 3 `woodlands`, 4 `jungle`, 5 and 12 `deadlands`, 6 and 11 `winter`,
   7 `mountains`, 9 `desert`, 10 `savannah`, 13 `cult of doom`; anything else `greenlands`.
   Then, by the ground height at the player's column (0x35ea0): below 10 → `beach`, below 0 →
   `ocean`. Then, by the place kind K: 4 and 9 `unholy pact`; 5 → `unholy pact` when the
   weight from 0x2d9f30 for that column is above 0.5, else `village` or `village2` by the low
   bit of the place's hash 0x2ff5c0(zone); 8 `order of the light`; 10 `mountains`; 13, 0x14 and
   0x20 `druids of mana`; 0x11 `steel empire`; 0x12 and 0x25 `cult of doom`; 0x19 `woodlands`;
   0x1a `deadlands`; 0x1b `enchanted-forest`. These choices are place music.
4. **Spirit world**: if the player has buff 0x13 (0x5e3a0), `deadlands` (and the zone name
   shows "SpiritWorld"). If that changes the track, T (§3.4) is set to 100 here; §3.4 may
   replace it in the same frame.
5. **Boss**: if any creature of hostility 1 has the player in its target map (+0xb88) and is
   a boss by 0x5eee0 or 0x5eda0: `boss.ogg`; not place music.
6. **Success**: while the success timer game +0x2050 is above 0 (it counts down by the frame's
   ms, floored at 0): `success.ogg`. If that changes the track, the threshold becomes 500.
7. **Menus**: if the character list (game +0x118), the creation screen (+0x170) or the screen
   at +0x130 is showing: `maintheme.ogg`, not place music; threshold 500 if that changes it.

Each later rule overrides the earlier. All names carry `.ogg`; the request is `Music\` + name.

The success timer is set to 60,000 when the mission of the player's place becomes completed
(0xa5a50 at 0xa7471); when the player obtains a special item (type 0x18; the player shows it,
abilities 0x78, 0x79 or 0x7c: 0xa5a50 at 0xaad54, 0x98180 at 0x9a205); and when a talk gives a
reward item (0xb40d0 at 0xb51d9 and 0xb591d).

The install also holds `city`, `dungeon`, `home`, `night`, `tribe` and `wood`; this build never
asks for them. Music does not depend on the time of day.

### 3.4 When to change, and the five-minute rule

State kept between frames: the current track C (a static string at 0x551508), the time E the
current track has played (0x551504, ms), the switch threshold T (0x533934) and a "first frame
of a difference" flag F (0x533938).

* **Not playing** (engine +0x28 says the main streamer is idle): if the desired track D differs
  from C, set C = D and request `Music\D`. In every case: T = 300,000 (10,000 while the success
  timer runs), music volume = musicVolume × 0.01 immediately (no easing), E = 0, F = true.
* **Playing**: E += the frame's ms.
  * If D equals C: T = 300,000 when place music is playing and the option `musicLoop` (game
    +0x1bd4) is 0; otherwise T = E + 300,000 (never reached). F = true.
  * If D differs from C and F is true: T = E + 500 if the boss or success track is wanted;
    else T = 30,000 if E < 25,000; else T = E + 5,000. F = false (T then stays fixed).
  * Target volume: if the duck timer game +0x204c is above 0, **0.015** (0x437f78); else if
    E ≤ T, musicVolume × 0.01; else **0** (fade out). The music volume game +0x2048 eases
    toward the target with 0x16ee0 (open question 3).

In words: a new track waits at least 30 s into the old one (5 s more if it has already played
25 s, half a second for boss and success), then the old one fades out and the new one starts.
With `musicLoop` off, place music fades to silence after five minutes and stays silent until
the place changes. musicVolume is the integer option `musicVolume` (game +0x1ba8); changing it
in the options sets the volume at once (0x89c10).

### 3.5 Ducking for instruments

The duck timer game +0x204c counts down by the frame's ms (floored at 0). Playing any of the
sounds 0x6d..0x77 (flutes, harps, fiddle, whistles, bells, lute, nightingale) raises it to at
least 6000, so the music drops to 0.015 for six seconds after the last note.

---

## 4. Ambient sounds (0xaf110)

There are no ambient loops. Ambient sounds are single sounds played by **emitters**: 0x40-byte
records kept by each zone (zone +0xa8 .. +0xb0; type u32 at +0, position 3 × i64 at +8) that
world generation places: copies of the zone's leaf (0x4d), fruit (0x4e..0x50) and type-0x1f
models, made without rand() at the end of the plant pass (`worldgen-structures-contents.md`
§C2.1). Each frame, 0xaf110 visits the 3 × 3 zones around the player (or
around the flight destination +0x1b0/+0x1b4 while the player is flying, ability 0x9c), skipping
zones not loaded or whose +0x234 is not below 4096, and goes through their emitters in order
with a 1-based index i.

Every emitter test uses a **staggered tick**: with the world clock c (world +0x440, ms) and the
frame's ms dt, an emitter at block (bx, by) ticks on a period of N ms when
trunc((c + o) / N) changes between c and c + dt, where o = (bx mod 1000) + ((by + 0x14d) mod
1000) for N = 1000, and the same with mod 5000 for N = 5000 (the two terms are taken from x and
from y + 0x14d0000 world units). Emitters thus tick at different moments. "rand" is the CRT
rand shared with the game.

With t = the time of day (world +0x0c, ms), night is t ≤ 21,600,000 (6:00) or t ≥ 75,600,000
(21:00). Pitches use rand × k / 32767 + b.

| Emitter type | When | Chance | Sound | Volume | Pitch |
|---|---|---|---|---|---|
| 0x1f (water) | every 500 ms tick (offset uses x × 13 and y × 13 + 0x14d0000) | 1 in 4 | 0x68 + rand % 3 (`river1..3`) | 0.1 | rand × 0.4 / 32767 + 0.8 |
| 0x4e, day | 1000 ms tick | 1 in 10 | 0x61 (`bird1`) | 0.1 | rand × 0.2 / 32767 + 0.9 |
| 0x4f, day | 1000 ms tick | 1 in 10 | 0x62 (`bird2`) | 0.1 | same |
| 0x50, day | 1000 ms tick | 1 in 10 | 0x63 (`bird3`) | 0.1 | same |
| 0x4e, night | 1000 ms tick | 1 in 10 | 0x66 + rand % 2 (`owl1/2`) | 0.1 | rand × 0.2 / 32767 + 0.5 |
| 4..8, any other type by day, or not 0x4e by night: only when 12:00 < t < 24:00 (43,200,000 < t < 86,400,000) and i is a multiple of 20 | 5000 ms tick | 1 in 70 | 4..8 → 0x64 (`cricket1`); 0, 1, 11, 12 → 0x65 (`cricket2`); other types none | 0.05 | rand × 0.2 / 32767 + 0.9 |
| 0x4d | 1000 ms tick | 1 in 16 | no sound (a particle effect) | | |

"1 in N" is one rand whose value mod N is 0 (for 4 and 16 the remainder is taken as for signed
numbers; rand is never negative, so it is plain mod). The order of rand draws per sound is:
the chance, then the pitch, then (river, owl) the variant. Each sound plays at the emitter's
position through 0xa0420, so it is attenuated and cut at 64 blocks like any other.

Wind is not an ambient: it is the gliding sound of §5.1.

---

## 5. Which events play which sound

Two groups: sounds the **client** plays directly through 0xa0420 / 0xa03f0, and **sound requests**
that the simulation appends to world +0xa0 and §2.1 plays. Requests are fully specified in the
specs that own their routines; this section lists where to find them.

### 5.1 Client sounds (game side)

| Number | Event | Call site |
|---|---|---|
| 0x78 `wind` | gliding (player ability 0x9c) once its clock passes 500 ms, on every 500 ms boundary of the ability clock: two rands r1, r2; pitch = (r1 + r1) / 32767 + 1.0 (1..3, so 1..2 after the cap), volume = r2 × 0.5 / 32767 + 0.5 | 0xa5a50 at 0xa7130 |
| 0x1d `level-up` | the "LEVEL UP!" banner | 0xa5a50 at 0xad60a |
| 0x1f `quest-complete` | the mission-complete banner | 0xa5a50 at 0xa8a8c |
| 0x21 `new-quest` | a new mission marker ("shine") | 0xa1320 |
| 0x20 `quest-progress` | talk progress and lore increase (three places) | 0xb40d0 at 0xb55e7, 0xb56e7, 0xb5efb |
| 0x7b `speech-next` | next line of a conversation | 0xb40d0 at 0xb6079 |
| 0x3c `drop` | an item given in conversation lands | 0xb40d0 at 0xb5781 |
| 0x34 / 0x35 `human-voice01/02` | talking to people (use key, shops, conversations) | 0x8ae50 (×2), 0x8fbc0, 0x94900 (×2 + 1 interface), 0xb7740, 0x28ffe0 (0x35) |
| 0x7c `tamers-whistle` | taming a pet | 0x8ae50 at 0x8ecb9 |
| 0x5c `craft` / 0x5d `craft-proc` | crafting an item / crafting into a special result | 0x8fbc0 |
| 0x3d `drop-coin` | paying or being paid in shops | 0x94900 (×3) |
| 0x5b `menu-drop-item` | moving items in the shop and inventory | 0x94900 (×2 + 1 interface) |
| 0x3c, 0x3b + k | shop and pick-up results | 0x94900 at 0x96011, 0x9617d |
| 0x30 + k (`pickup`, `disenchant2`) | the use prompt's pick-up and salvage actions | 0x98180 at 0x98561 |
| 0x31 `disenchant2` | salvaging | 0x965e0 |
| 0x3c, 0x3e (interface) | dropping and receiving items | 0x98180 at 0x98660, 0x99bc8 |
| 0x56 / 0x57 `menu-open2/close2` | an in-game window (0x9ae70 or the panel at game +0x310) opens or closes; played once per change | 0xa5a50 at 0xad7e6 |
| 0x58 `menu-select` | buttons (the callbacks 0x2723d0..0x272620, 0x280e30, 0x294c70, 0x294ca0, the main menu 0x1355a0) and each step of a held +/− spinner, at most every 200 ms | as listed; 0xa5a50 at 0xacc4e, 0xacc92 |
| 0x59 `menu-tab` | changing tab | 0x27f6a0, 0x27f7a0, 0x27f830, 0x27f980, 0x280170, 0x280270, 0x2802f0 |
| ambient | §4 | 0xaf110 |

Numbers marked "+ k" add a register value whose source was not traced (open question 5).

### 5.2 Sound requests (world side)

All are appended to world +0xa0 and played by §2.1.

* **Hits** (0x29eb60, `creature-hit.md` §2.10–§2.11): the impact sound by the attacker's weapon
  (1..10, 0x0b), magic hits 0x2a/0x2d, blocks 0x18, mana shield 0x5e, the critical groan by the
  target's race (0x3f..0x53), the stun sound, and the death sound 0x1b (0x2d for race 0x11d).
* **Abilities** (0x2c5250, `creature-abilities.md` §4, `creature-abilities-ranged.md`): swings
  0x0f, two-handed swings 0x11, charge hums 0x27 and 0x3a, casts and their hits (0x29, 0x2c,
  0x54), shield 0x10, rolls and dashes 0x1a/0x0e, arrows 0x16, block breaking 0x1a, skills (0x32,
  0x60, 0x5f, 0x2f, 0x30, 0x2e, 0x73, 0x6e, 0x75, 0x79 …), each with its pitch rule.
* **Footsteps and water** (0x2bb8e0, `creature-physics.md` §5.6, §6.2–§6.4): on ground, when
  the stride phase crosses a multiple of π, |v_xy| > 5, not dodging and out of water: if the block
  under the feet has type 3, 0x24 + rand % 3 (`step-water`, `step-water2/3`) at pitch 1.0,
  otherwise 0x23 (`step2`) at pitch rand × 0.2 / 32767 + 1.0; volume 1.0, only for the
  creature the local machine owns. There is one dry footstep sound for every ground type. Also:
  landing 0x23, falls and lava 0x17 and 0x2a, splashes 0x22, swimming 0x24..0x26 at volume 0.75,
  dust 1.
* **Upkeep** (0x2de4c0, `creature-upkeep.md` §2.2): roll 0x1a, water bubble 0x2d, drowning 0x2f,
  rising 0x3c.
* **Effects** (0x2dba40, `effect-objects.md`): projectiles and their impacts.
* **Interactions** (0x2c3710 and the open/shut routine 0x2d3a10, `zone-object-interaction.md`,
  `skills-and-interactions.md`): doors and gates 0x36..0x39, sleeping 0x1f at volume 0.75,
  pick-up 0x30, drop 0x3c, the gate opener 0x70, heroic shout 0x6b.
* **Ground items and traps** (world update 0x2e3510): when a ground item's pick-up delay (+0xc8)
  runs out, outside server mode, 0x3d `drop-coin` for coins (type 12) or 0x3e `drop-item`
  otherwise, pitch rand × 0.1 / 32767 + 1.0 (0x42e528; 0x2e49eb); if it is reserved for the local
  player and its type is 3..9 or 0x1a, also 0x5d at pitch 1 (0x2e4a9e). A trap object that hits a
  creature (object type not 0x11) plays 1 at the creature (0x2e5d59), pitch rand × c / 32767 +
  1.0 with c in XMM12, loaded from 0x42e704 earlier in the routine (open question 12).
* **Area blow** (0x2a45e0, called by the world tick): once per call that broke blocks in its
  9 × 9 × 9 cell area, number 1 at the centre, pitch rand × 0.4 / 32767 + a base held in XMM7
  (0x2a4ccc..0x2a4cf2; the base was not resolved).
* **The player dying from a client-side hit** (0xa5a50 at 0xa8f4d): when a hit applied on the
  client takes the player's health below 0, a request 0x1b at the player, pitch and volume 1.0,
  owner the player.

---

## 6. Constants

| Address | Value | Use |
|---|---|---|
| 0x438000 | 4096.0 | hearing range² (64 blocks) |
| 0x437f64 | 1/4096 | attenuation per block² |
| 0x42e7f8 | −0.1 | pan scale (unused) |
| 0x4ac9d0 | 2.0 | maximum frequency ratio |
| 0x42e51c | 0.01 | volume option scale; music stop threshold |
| 0x437f78 | 0.015 | ducked music volume |
| 0x437ffc | 2500.0 | explosion shake squared-distance denominator;50-block radius |
| 0x42e598 | 10.0 | beach height |
| 0x4ac964 | 0.5 | place weight threshold; wind volume base and scale |
| 0x438014 / 0x438024 | 21,600,000 / 75,600,000 | night bounds |
| 0x438018 / 0x438028 | 43,200,000 / 86,400,000 | cricket window |
| 0x42e524 / 0x42e528 | 0.05 / 0.1 | cricket / other ambient volume |

---

## 7. CubeVulkan today, and where it differs

`src/Audio.*` uses miniaudio: `music(name)` crossfades looping `music/<name>.ogg` over 2.5 s,
and `sound(name, volume)` plays `sounds/<name>.wav` once at volume × soundVolume. `main.cpp`
chooses music with `musicFor` and plays birds and crickets on a timer; a few interface events
play sounds by name. The native modules (`NativeAbility`, `NativeHit`, `NativeUpdate`,
`NativeUpkeep`, `NativeEffects`, `NativeInteract`) already build the game's 0x30-byte sound
requests. Differences:

1. **World sound requests are never played.** `Spawned.cpp` clears every module's `sounds`
   list ("not played or sent yet"), so hits, swings, footsteps, spells, doors, deaths and every
   other §5.2 sound are silent. Playing them through §2 is the largest gap.
2. **No position, distance or pitch.** `Audio::sound` has no position, no 64-block cut-off, no
   1 − d²/4096 attenuation from the camera's eye, and no pitch. The game has no panning either,
   so mono-centred output is correct; pitch above 2.0 must be clamped to 2.0.
3. **Sounds by name, not by number.** The game addresses the 125-entry list (§1.2); CubeVulkan
   would need the list from extraction to play requests.
4. **Music choice.** `musicFor` picks `night` from 20:00 to 5:00, `wood` for dark woods,
   `ocean`/`beach` from water depth and sea level, and maps biomes its own way. The game never
   plays `night` or `wood`, ignores the time of day, uses ground height (below 10 beach, below 0
   ocean), and has place music (villages, faction places, shrines, enchanted forest), boss,
   success, spirit-world and title rules (§3.3).
5. **Music changes.** CubeVulkan crossfades over 2.5 s at once. The game waits (at least 30 s
   into a track, 5 s more after 25 s, 0.5 s for boss and success), fades the old track out,
   starts the new one only after silence, and never overlaps two tracks (§3.4).
6. **The `musicLoop` option** (fade to silence after five minutes of place music) and the
   **instrument ducking** (§3.5) are missing.
7. **Ambient sounds.** CubeVulkan plays `bird1/2` by day and `cricket1/2` at night every 6–14 s
   at 0.35 from the player. The game plays them from world-generated emitters in the 3 × 3 zones
   around the player, with per-type schedules, rivers and owls, crickets only from 12:00 to
   24:00 and only on every twentieth emitter, volumes 0.1 and 0.05, and pitch variation (§4).
   Rivers (`river1..3`) and owls are never played by CubeVulkan.
8. **Wind while gliding.** CubeVulkan plays `wind` on the 500 ms boundary with volume 0.5..1, as
   the game does, but without the pitch (game: 1..3, heard as 1..2 after the cap), the
   distance attenuation (it is at the player, so the camera distance applies) or the volume
   option.
9. **Interface sounds.** CubeVulkan plays `quest-progress`, `human-voice01`, `drop-coin`,
   `craft`, `craft-proc`, `menu-tab`, `menu-select` and `drop-item` at its own events; their
   triggers should be checked against §5.1, and `menu-open2/close2`, `level-up`,
   `quest-complete`, `new-quest`, `speech-next`, `tamers-whistle` and `disenchant2` added.
10. **Explosion shake** from sounds 0x54/0x55 (§2.1) is not modelled.
11. **Volume options.** The game uses the integer options `soundVolume` and `musicVolume` × 0.01
    and stops music under 0.01; CubeVulkan's defaults are 0.7 and 0.5.

---

## Open questions

Each names an oracle the user could record. ZoneExtractor (`tools/`) can dump game memory at
run time; "record" means a dump per frame for a short session.

1. **Play order and timing of requests.** Whether the world list (+0xa0) is played every frame
   in full and then cleared, or only the new entries. Oracle: ZoneExtractor dump of world +0xa0
   (count and entries) each frame for a few seconds of combat, compared with the calls into
   0xa0420 (hook the function and log number, volume, pitch, position).
2. **The 2.0 pitch cap.** Requests with pitch above 2 (e.g. 0x0f at 2.0 + rand, the wind) play
   at 2.0 if XAudio2 clamps as documented. Oracle: hook 0x343c0 and log p, and record the
   game's audio output for one such sound.
3. **Music easing rate.** 0x16ee0 moves the volume by a factor from 0xa7a0 whose arguments
   (registers XMM2/XMM3 at 0xac2e2) were not resolved. Oracle: ZoneExtractor dump of game
   +0x2048 every frame across a region change, giving the fade curve directly.
4. **Wind draws.** The pitch's rand is drawn before the volume's (assembly at 0xa70e0..0xa7130
   as decompiled); confirm the order, which matters if this rand is shared with the
   simulation. Oracle: hook 0xa0420 while gliding and log volume and pitch with the rand
   state.
5. **Computed numbers** "0x3b + k" (0x94900 at 0x9617d) and "0x30 + k" (0x98180 at 0x98561):
   which k occur. Oracle: hook 0xa0420 during shop use, pick-ups and salvaging.
6. **Emitters.** Answered in part: `worldgen-structures-contents.md` §C2.1 (end of the plant
   pass 0x3191b0, no draws) fills zone +0xa8 with the zone's models of type 0x4d..0x50 and
   0x1f, so in vanilla the list holds only leaves (0x4d) and fruit (0x4e..0x50). No generator
   code was found placing a type-0x1f model, and none occurs in 31,940 extracted zones, so the
   river sound and the cricket branch (types 0..12) look unreachable
   (`worldgen-structures.md` open question 11). Still open: the meaning of zone +0x234 (a float
   that must be below 4096). Oracle: `.rec` files from a ZoneExtractor that records +0xa8
   (the existing `Extract/seed_123` ones predate it) compared with the §C2.1 rule.
7. **Place kind 5 and 0x2d9f30.** What the column weight for kinds 5 and 12 measures (it decides
   between `unholy pact` and the village tracks). Oracle: log the desired track and the place
   kind while walking into and around several towns.
8. **Boss tests** 0x5eee0 / 0x5eda0 (which creatures start `boss.ogg`). Oracle: ZoneExtractor
   log of the desired music name (0x551508) and the attacking creatures' race and flags during
   fights with dungeon bosses and ordinary monsters.
9. **The data-database path** for sounds (engine +0x58): whether anything sets it in Steam.
   Oracle: dump engine +0x58 after start-up (expected null, loose files).
10. **Client-side death sound** (0xa5a50 at 0xa8f4d): which hits reach this path (it applies a
    hit record to the local player outside the world tick). Oracle: hook 0xa18b0 while the
    player dies in single player and in a hosted game.
11. **Sounds not found** at any call site: 0x1e `skill-up`, 0x33 `swirl`, 0x5a
    `menu-grab-item`, 0x6c `machine`, 0x7a `speech-end`, and the instrument sounds 0x6d..0x77
    (expected from playing an instrument item). Oracle: hook 0xa0420 and log every number over
    a long session that levels a skill, grabs items, ends talks and plays an instrument.
12. **Trap pitch** factor: XMM12 is reloaded inside 0x2e3510 (0x2e46fc from 0x42e704, 0x2e477f
    from a local); which value is live at the trap call. Oracle: hook 0xa18b0 and log pitch
    when a trap hits.
