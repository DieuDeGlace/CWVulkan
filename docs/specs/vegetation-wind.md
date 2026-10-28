# Vegetation wind: which plants sway, how, and the clouds

Behavioural specification of how Cube World moves its vegetation, in the **Alpha** client
(CubeVulkan's default look) and the **Steam** release. Written from the Alpha decompile and
assembly (`disassembled alpha cube world`, routines by address in `Cube.exe`), the Steam
decompile (`decompiled cube world steam`, routines by RVA, 0x14xxxxxxx), the constants read from
both binaries, and every shader blob of both executables disassembled with the system's
`D3DCompiler_47`. Infinite-Cubes was not used.

Conventions are those of `alpha-rendering.md` §0 ("AR"). Model records are the 0x40-byte zone
model records of `worldgen-structures.md` §4.2 (type +0x00, position +0x08..+0x18, size +0x20,
yaw +0x24 in degrees, tint +0x2c..+0x34, **flags +0x38**). "H" is a model's height in voxels
(its .cub z size: Alpha model +0x4c, Steam model +0x90). Times are milliseconds.

---

## 1. Summary

* **No shader moves anything.** Neither game's world vertex shader has a time or wind
  constant: Alpha's vs_3_0 at 0x6fd928 (constants AR §1.1) and Steam's vs_4_0 at file offset
  0x436fb0 (cbuffers Proj, View, World, Material, PointLights, Light, Ambient, White,
  DepthOffset, Glow) only transform and light. The other Steam blobs (0x438810, 0x4396a0,
  0x439f90, 0x43ae90, 0x43b500, 0x43c100/0x43ca50 particles, 0x47d3d0/0x47e140 interface) have no
  clock either.
* **Terrain blocks never move.** Tree crowns are blocks (Alpha leaf material 8; Steam's leaf
  block types) meshed into the chunk meshes, drawn with the static world shader. **Vanilla tree
  leaves do not sway in either game.**
* **Plant models sway**, on the CPU, by a time-varying shear added to each model's world matrix
  when its record has **flag 4** (§3, §4). Grass tufts carry flag 4; ordinary flowers do not.
* The sway does **not** use the glider wind `0x2e65e0` (`creature-physics.md` §7.7, "wind(x, y)")
  or any field of the world: its only inputs are the world clock and a per-model phase. There is
  no wind direction, no gusting by region, no weather.
* Steam adds a second, hinged swing for models with flags 0x10 / 0x20 (§4.4); no placement of
  them was found.
* **Clouds do not move** in either game (§6).

---

## 2. The clock

Both games keep a millisecond counter of world time that the world step advances by the step's
milliseconds (unlike the time of day, which runs ×10, or ×100 while sleeping):

* **Alpha**: W+0x8000bc (= G+0x8003a0), a 32-bit integer; 0x60c510 adds the step's
  milliseconds (its first argument) at 0x60c678, just before the time of day W+0x80015c gets
  step × 10 (× 100 asleep).
* **Steam**: world +0x440, a **float**; 0x1402e2e80 adds the step (param 2, ms) every step, next
  to the time of day +0xc (× 10, × 100 asleep). Being a float, it loses milliseconds after about
  4.7 hours of play (2²⁴ ms) and steps in 2, 4, 8 ms after that; the sway then becomes visibly
  coarser. That is the game's behaviour; an engine may keep the counter in double precision.

Call this value **t** below. It starts at 0 when the world is created and is not saved.

---

## 3. Which models sway

### 3.1 The rule

A zone model sways **iff bit 2 (value 4) of its record's flags +0x38 is set** (Alpha 0x4bd160
tests it at 0x4bd4fd; Steam 0x1400d3170 at 0x1400d3b36). Nothing looks at the model's name or
type. Bit 0 (value 1) is the light flag (it also makes the model ignore the light where it
stands, §4.5); bit 1 (value 2) is not read by either draw routine.

The type → model tables are the games' own (Alpha: G+0x800718, filled in 0x459c40 from
0x46254e; Steam: model manager +0x38, 100 entries, filled in the loader 0x140177f50 from
0x1401e2d3c). The names below come from those tables.

### 3.2 Steam: the plant pass (0x3191b0, `worldgen-structures-contents.md` C2)

| Placement (C2 step) | Type → model | Flags | Sways |
|---|---|---|---|
| 2, hanging | 0x48 liana | 2 | no |
| 4, river reed | 0x23 reed | 4 | **yes** |
| 6, block type 12 | 9/10 grass-dark1/2, 13/14 dark-plant1/2 | 6 | **yes** |
| 9, under water | 0x11 alga, 0x0f underwater-plant | 6 | **yes** |
| 9, under water | 0x10 coral | 2 | no |
| 10, lava | 0x13 lava-grass | 6 | **yes** |
| 10, lava | 0x14 thorn-plant | 2 | no |
| 13, forest | 9/10 grass-dark1/2 | 6 | **yes** |
| 13, forest | 13/14 dark-plant1/2 | 2 | no |
| 14, hot and wet | 7/8 grass-dry1/2, 11/12 savannah-plant1/2 | 6 | **yes** |
| 14, hot | 4/5/6 grass, grass2, grass3 | 6 | **yes** |
| 14, hot | 0x15 echinacea2, 0x16 leaf | 6 | **yes** |
| 15, elsewhere | 0/1/2 flowers, flowers2, flowers3 | 2 | no |
| 15, elsewhere | 4/5/6 grass, grass2, grass3; 0x16 leaf | 6 | **yes** |
| 15, river | 0x23 reed | 4 | **yes** |

(Type 0x16 is `leaf.cub`, a ground plant, not a tree leaf. Types 0x4c–0x50, the tree's light,
leaf-particle and fruit markers, have no model in the table and are not drawn.) Other
placements (signs, lanterns, banners, farm plants) set the flags their own specs give; a
renderer applies §3.1 to whatever record it is handed.

### 3.3 Alpha: the zone generator (0x5e4850)

Alpha's generator writes the same flag on the same kinds of plant, with Alpha's own type
numbers (0 flowers2, 1 flowers, 2 grass, 3 grass2, 4 grass3, 5 underwater-plant, 6 coral, 7 alga,
9 lava-grass, 0xa thorn-plant, 0xb echinacea2, 0xc leaf, 0x16 reed):

* reed (0x16): flags = 4 (0x5ec0d3);
* underwater-plant (5) and alga (7): flags |= 4; coral (6) not (0x5ec54c);
* lava-grass (9): flags |= 4; thorn-plant (0xa) not (0x5ec7d1);
* grass, grass2 (2, 3), echinacea2 (0xb), leaf (0xc): flags |= 4 (0x5ecaa4..0x5ecafc); in the
  branch ending at 0x5ed0e1 also grass3 (4);
* the flowers (0, 1) never.

So in both games: **grass sways, flowers stand still, tree crowns stand still.**

---

## 4. The sway transform

The model's world matrix is built from operations that each act on the model **before** the
ones already applied (each is pre-multiplied). Read in the order a vertex goes through them, a
zone model's vertex is: centred (−½ x size, −½ y size, 0) → turned by the yaw about the
vertical axis → **[Steam: hinged swing, §4.4]** → **swayed (§4.1)** → scaled by the record's
size → translated to its position. The sway therefore acts in world-aligned axes after the yaw,
in voxel units of the model.

### 4.1 The shear

For a vertex at height z voxels above the model's base (z in 0..H):

* Δy = 0.1 · a(t, i) · z / H blocks,
* Δx = −0.1 · b(t, i) · z / H blocks,

with

* a(t, i) = cos(0.004 · t + 30 · i + 84) + cos(0.0027 · t + 93),
* b(t, i) = 0.8 · (cos(0.005 · t + 30 · i) + cos(0.003 · t)),

arguments in radians, t in milliseconds, i the model's phase index (§4.2, §4.3). The base
stays put, the top moves linearly with height (a shear, not a bend): up to ±0.2 blocks along y
and ±0.16 blocks along x **whatever the plant's size**, because the coefficient is divided by
the model's height in blocks (H × size) and multiplied back by the size scale. The periods are
1.57 s and 2.33 s (a), 1.26 s and 2.09 s (b); the second term of each axis is the same for every
model (a shared sway), the first is shifted per model. Z is unchanged.

The signs above follow from the rotations the games compose (−90°, shear, +90° about one
horizontal axis, then the same about the other); a mirrored sign is not visible and not tested.

### 4.2 Alpha (0x4bd160)

When flag 4 is set (0x4bd507..0x4be40f): k = 0.1 / (H × size) (0x6fcb7c = 0.1); the matrix is
rotated by −π/2 (double 0x702ad0), sheared by a × k (cosines of t × 0.004 (0x702a30) + 30 n + 84
(0x702b70) and t × 0.0027 (0x702b14) + 93 (0x702b74)), rotated back by +π/2 (0x702a70),
rotated the other way, sheared by b × k (t × 0.005 (0x6fcd9c) + 30 n; t × 0.003 (0x702b18);
× 0.8 (0x6fcdc8)), and rotated back. **The phase index is the integer n the frame routine
passes as the drawer's sixth argument** (30 n is formed as (n << 4 − n) × 2 at 0x4bd64e): for
the call at 0x4b2801 the value at [esp+0x14] of 0x4ac260 (the result of 0x4ec400 for the object
being drawn), for the call at 0x4ba4fe the entry's +0xc field. Its meaning is open (§9); any
integer that stays the same for a model from frame to frame gives the right look, since it only
offsets the phase. t is the integer clock of §2 converted to float.

### 4.3 Steam (0x1400d3170)

Steam computes the shear once per frame for **12 phases** and lets models share them:

* At the start of the routine (0x1400d3234..0x1400d34c5) twelve 4 × 4 matrices are rebuilt
  (0x1400cb870 with 12); matrix j = 0..11 uses the phase 30 j (R15 += 0x1e): rotate about the
  y axis by −90° (0x14042e88c), shear with a(t, j) (0.004 at 0x140437f70, 84 at 0x14043efc0,
  0.0027 at 0x14043ef30, 93 at 0x14043efc4), rotate +90° (0x14042e7c0); rotate about the x axis
  by −90°, shear with b(t, j) (0.005 at 0x14042e518, 0.003 at 0x14042e6dc, × 0.8 at 0x14042e530),
  rotate +90°.
* Per zone the models of the zone list (+0x18) are visited in list order with a counter n that
  starts at 1 for the first record (it counts every record, the light records of type 0x59
  included). A flag-4 model uses **matrix n mod 12**, applied between a z-scale by
  q = 0.1 / (H × size) (0x14042e528) and its inverse (0x1400d3b40..0x1400d3bb3), which gives
  exactly §4.1.

So Steam's plants fall into 12 phase groups by their position in the zone's list; Alpha's
phases come from n directly. The look is the same.

### 4.4 Steam only: the hinged swing (flags 0x10, 0x20)

After the sway (0x1400d3bc0..0x1400d3d0f), with h = n xor (n << 13); h ^= h >> 17; h ^= h << 5
(unsigned 32-bit), h ← h mod 1000:

* flag 0x10: the model swings about the y axis by 30° × cos(0.004 · t + h) (30.0 at
  0x14042e5a8), pivoting at its top (moved down by H voxels, turned, moved back);
* flag 0x20: the same about the x axis.

These hang from their top (chains, lanterns, banners). No placement in the specified generator
sets 0x10 or 0x20 (open question 2). Alpha has no such code.

### 4.5 What else the drawers do (not wind)

The model's material alpha is 1 when flag 1 is set, else (record +0x28 / 255) × the light where
it stands (Alpha 0x4be6b6, Steam 0x1400d3d9b); Steam's flag 8 queues the model in the second
model list (renderer +0x1d0) instead of the first (+0x138). The yaw is record +0x24 in degrees
about the vertical axis; the model is centred on half its x and y size (Alpha −0.5 at 0x6fce8c).

---

## 5. Amplitude and range

* Amplitude: §4.1, fixed; no option, no distance fade. Every model that is drawn sways; models
  are drawn within the game's model distance (Steam renderer +0x49c, plus 64) and the view.
* Speed: fixed by the clock; it does not scale with game speed or the time-of-day rate.
* The sway of a model is the same in every frame for the same t; nothing is random per frame.

---

## 6. Clouds

Vanilla clouds **do not move**. `sky-and-clouds.md` §A.5 (Alpha 0x4b1534..0x4b1a65) and §B.7
(Steam 0x1400e5730) place each cloud from its 200-block cell number and noise only; neither
routine reads the clock (no access to W+0x8000bc / world +0x440 or the time of day in either).
They only appear and disappear by cell as the camera moves, and Steam fades them near the
camera. Cloud drift seen in the user's game comes from the user's own **AlphaCWWorldMotion**
mod ("slow cloud drift and gentle voxel-leaf sway", `CubeWorldAlphaMods/AlphaCWWorldMotion`),
which also adds the leaf sway; it is a mod, not vanilla. Under `mods-feel-native` it would be a
CubeVulkan option, off in the vanilla look.

---

## 7. CubeVulkan today (`src/`, `shaders/`)

**What is the game's:** nothing of the motion. CubeVulkan's sway is the WorldMotion mod's idea
re-expressed in its own shaders, applied to more than the mod did, and the game's own model sway
(§4) is not implemented.

* `shaders/common.glsl` `sway()` (called from `scene.vert`, `alpha_scene.vert` and
  `shadow.vert`): offset = (sin φ, cos(0.83 φ + 1.3)) × amplitude × gust × bend² × reach,
  φ = clock × speed + 0.37 x + 0.29 y, gust = 0.65 + 0.35 sin(0.23 clock + …), bend =
  local z / model height, reach fading out at `[Motion] LeafRange` (96 blocks). **Invented**: the
  formula, the gust, the squared bend (the game shears linearly), the distance fade, the
  position-based phase (the game's phase is per model, never per position) and the amplitude
  (`leafAmplitude` 0.16 blocks at the top vs the game's 0.1 × (two cosines), up to 0.2 / 0.16).
* The clock is `f.eye.w = fmod(time, 100000)` (Renderer.cpp, "u.eye"), in seconds, times
  `[Motion] WindSpeedPercent`; it jumps every 100,000 s. The game's is the world's millisecond
  clock (§2).
* **Leaf blocks sway** (`Extracted.cpp` ~884 sets SWAY on block types LEAVES_TYPE and
  SNOW_LEAVES_TYPE; `Terrain.cpp` ~361 on the old terrain's crowns). **Invented** (the mod's);
  vanilla never moves blocks.
* **Which models sway** is decided by name, `plantName()` in `Extracted.cpp` (~38: grass,
  flower, plant, reed, alga, coral, tendril, wheat, corn, lily, leaves, tulip, sunflower,
  echinacea, liana), for both the generated and the extracted zones (~586, ~660), and for the old
  terrain's own scenery (`Terrain.cpp` ~621, ~735, SWAY by default). **Not the game's rule**: the
  game reads flag 4 of the record (§3.1), which `worldgen::Model::flags` and the extracted
  records already carry. The name list sways flowers, coral, liana and others that the game
  keeps still.
* Config: `[Motion] Enabled`, `Leaves`, `LeafAmplitudePercent`, `LeafRange`,
  `WindSpeedPercent` (Renderer.cpp ~163); F8 toggles (`main.cpp` ~2939). All CubeVulkan's.
* The shadow pass applies the same sway (`shadow.vert`), and `sphereOf` pads culling spheres by
  the amplitude: consistent for CubeVulkan's own shadows (see `shadows.md`).

**Grass not swaying.** By reading, a grass model with the SWAY flag should move with the rest;
the cause was not settled without running. Likely candidates, to check in that order: (1) the
grass seen is the ground's grass blocks (type 4 surface), which nothing moves, rather than the
tufts; (2) the tufts' `Scenery.flags` are lost between `ExtractedWorld::props*` and the draw
(`World.cpp` ~377–393 copies them; the `[Motion] Leaves=0` switch turns off **all** sway, models
included); (3) `pc.params.z` (`models[m].height`, the mesh's max z) does not match the mesh's
local z range, so `bend` stays near 0 for small models. Implementing §3–§4 as specified removes
the question: sway the model matrix of every record with flag 4, on the CPU or in the vertex
shader, by §4.1.

**To bring CubeVulkan to vanilla:**

1. Per model instance with flag 4: the shear of §4.1 with the model's height H (voxels) and size,
   applied after the yaw and before the size scale; phase index = the record's 1-based position
   in its zone's model list mod 12 (Steam) or that position (Alpha; any stable per-model integer
   gives the same look). Flags come from the record, not the name.
2. Clock: the world's millisecond counter of §2 (CubeVulkan's world step), not wall time.
3. No sway of terrain blocks and no distance fade in the vanilla look. Keep the WorldMotion-style
   leaf sway and cloud drift as named options (default off), per `mods-feel-native`.
4. Steam look only: the hinged swing of §4.4 for flags 0x10 / 0x20.

---

## 8. Verification

The game is not to be run by the spec writer; these are for the user.

1. **Model matrix oracle (Steam)**, a new ZoneExtractor request (e.g. `sway.request`): for 300
   frames, at the end of 0x1400d3170 write world +0x440, the twelve matrices (the vector filled
   by 0x1400cb870, 12 × 64 bytes) and, per model drawn through 0x1400ee2f0 into renderer +0x138
   / +0x1d0, the record's type, flags, list position n and the final matrix. Expect §4.1 to the
   float, matrix n mod 12, and identical matrices for flag-4 models in the same group.
2. **Screenshots** (either game): a close, level view of a grass field next to flowers and a tree,
   two frames 0.4 s apart: the grass tops move by up to ~0.2 block, the flowers and the crown do not.
3. **Alpha**: with the AlphaCWShadows D3D9 hooks, record the world matrix of each draw made from
   0x4bd160 for a grass model over 120 frames, with G+0x8003a0; compare with §4.1 and recover the
   phase n of each.

## 9. Open questions

1. **Alpha's phase index** (0x4bd160's sixth argument): what 0x4ec400 returns at 0x4b229b and
   what the second list's +0xc holds were not traced. Oracle: request 3 above.
2. **Steam flags 0x10 / 0x20**: no placement was found that sets them. Oracle: a zone dump
   listing models whose flags have 0x10 or 0x20 (ZoneExtractor's model records).
3. **Signs of Δx, Δy** (§4.1): derived from the rotation helpers 0x14002b470 (about y) and
   0x14002b360 (about x); not checked against a recording.
