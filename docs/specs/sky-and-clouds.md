# Sky, sun, stars and clouds

Behavioural specification of how Cube World draws its sky: the gradient by time of day and
region, the sun, the stars and the voxel clouds. The **Alpha** client comes first, because its
look is CubeVulkan's default; the **Steam** release follows as the optional style. Neither game
has a moon or any weather (no rain, snow or storms: neither binary carries such strings or
code); "weather" in both is only the region the camera is in.

Sources: the Alpha decompile and assembly (`disassembled alpha cube world`, routines cited by
their address in `Cube.exe`), the Steam decompile (`decompiled cube world steam`, routines cited
by RVA, addresses as 0x14xxxxxxx), the constants read from both binaries, and the Steam D3D11
shader blobs disassembled with the system's `D3DCompiler_47`. Infinite-Cubes was not used.

This spec **extends `alpha-rendering.md`** and does not repeat it. Where that document already
covers a point it is cited as "AR §n". In particular AR covers: the Alpha clock and the two
daylight curves (AR §2), the Alpha palette with its zone modifiers (AR §3), the constants per
pass (AR §4), the sky gradient function and the sky and cloud pixel shaders (AR §6, §6.1, §6.3),
the fog distance and gradient shift (AR §7), the sky quad, stars and sun sprite (AR §8), and the
Steam shader differences in outline (AR §14). Two of AR's open questions (§16 items 4, 5 and 9)
are settled here (§A.3, §A.4, §A.5), and one AR statement is corrected (§A.5: Alpha draws only
`cloud02.cub`).

Conventions are AR §0. "smooth(x)" is 3x² − 2x³; "sat" clamps to 0..1; "lerp(a, b, t)" is
a + (b − a)·t. Blocks are world units; fixed-point positions are 1/65536 of a block.

---

## A. Alpha (the default look)

### A.1 Order of the sky draws in a frame

From the frame routine 0x4ac260 (AR §1.3), the sky-related draws in order:

1. the gradient quad (AR §8.1), unless the game flag G+0x8006e6 is set;
2. the stars, when the sunlight strength S < 0.5 and the camera is not under water (§A.3);
3. the sun sprite, always (§A.4);
4. alpha blending switched on (source alpha / inverse source alpha);
5. the clouds (§A.5), again unless G+0x8006e6 is set;
6. then terrain, water, models and creatures (AR §1.3 steps 12–14).

Everything sky-related is drawn before the terrain, so the world covers it. What G+0x8006e6
means was not traced (it skips both the sky and the clouds); map mode takes its own path
before step 2 (AR §11.5).

### A.2 Gradient, palette and time of day

Fully covered by AR: palette daylight L and sunlight S (AR §2.1, §2.2), the sky1/sky2/fog keys
for night, two dusk keys and day, with the four region modifiers (hot and dry, warm and wet,
dry, fire) and the underwater override (AR §3.2–§3.5, worked values AR §3.8), the gradient
function sky(Y) and its tilt shift (AR §6, §7.2). Nothing is smoothed over time in Alpha: the
palette is evaluated fresh each frame.

### A.3 Stars: placement settled

Generation and the switch-on rule are AR §8.3 (3000 stars, five `rand()` draws each, drawn
while S < 0.5 and not under water, no fading). The projection (AR §16 item 4) is now settled
from 0x4ae641..0x4aea7d:

* Each star's stored direction (100 blocks out) is multiplied by the **sky view matrix**
  (AR §8.4: the game's view with its translation recomputed so the camera is at the origin),
  dividing by the fourth component, which is 1 for that matrix. The result is a **view-space**
  point, not a screen point.
* The five vertices of the fan (centre, then the corners (−1, −1), (1, −1), (1, 1), (−1, 1),
  then the first corner again to close it: four triangles) are that point plus size × corner,
  added to the view-space x and y. So a star is a small square facing the camera, `size` view
  units in half-width at a distance of about 100.
* The device then draws them with world and view transforms set to the same identity-like
  matrix (0x4ae5ed..0x4ae624, transform states 0x100 and 2) and the projection set to the sky
  lens of AR §8.4 (state 3). Vertex format: position plus diffuse colour, 16 bytes; drawn one
  star at a time as a triangle fan of 4 primitives.

Consequence: a star's half-width on screen is size × (the lens's focal scale) / distance, i.e.
about size × 2.14 / 100 of the half-screen height at the default 50° lens (and narrower at the
30° lens). Sizes are 0.01..0.11, doubled for one star in ten, so stars are a few pixels at most;
there is no minimum size. Stars behind the camera get a negative depth and are clipped.

### A.4 Sun sprite: size settled

AR §8.4 holds the sprite texture, colour and quad. AR §16 item 5 is settled the same way as the
stars: the centre is 100 × D transformed by the sky view matrix (0x4248a0, with the divide by
the fourth component), and the four corners are ±40 added to view-space x and y (0x4aeae0..
0x4aed61). So the quad spans 80 view units at distance 100, about 2·atan(0.4) ≈ 43.6° across,
most of it transparent because of the q⁴ falloff of the texture. Vertex format: position,
diffuse, one texture coordinate (24 bytes), two triangles. The sprite is drawn whether or not
the sun is above the horizon; below it, it is under the horizon line and the world covers it.

There is no moon sprite, no moon texture and no moon light in Alpha (AR §2.3, §8.2).

### A.5 Clouds (0x4b1534..0x4b1a65)

**Model.** Only `cloud02.cub` is used. The loader (0x4ee230, at 0x51bdc6) puts it in model
slot 0xa08 of the model table; `cloud01.cub` is not referenced by the client at all (it exists
among the assets only; AR §11.4 named both, which is corrected here). The file is 24 × 24 × 16
voxels, every filled voxel the same colour (242, 246, 255). Each frame, before anything else,
the frame routine sets that model's shade colour (AR §11.1, bytes +0x5c..+0x5e) to
**(0, 100, 200)** (0x4ac2d0..0x4ac30f), so the occlusion shading of the cloud mixes towards a
mid blue in its creases instead of black.

**Gate.** Clouds are considered only if, at the block column of the local player
(the creature at G+0x8006d0, position +0x10, divided by 65536):

* the moisture factor Mo (AR §3.1, 0x5c4800) is above 0.01, and
* the fire share Fi (0x5ef040) is below 0.01.

**Grid.** The cells are 200 blocks. The centre cell is the camera position (G+0x140) in blocks
divided by 200 with C integer division (towards zero; note this is not a floor for negative
coordinates). For each cell offset i = −6..+6 (outer loop) and j = −6..+6 (inner loop):
cell cx = centre x + i, cy = centre y + j (13 × 13 = 169 cells).

For each cell, in this order:

1. **Coverage noise.** n1 = N(cx, cy), where N is the Alpha value noise 0x4c0ef0 (below);
   p = (n1 + 1) × 0.5.
2. **First moisture test.** m1 = the moisture factor 0x5c4800 evaluated **at block column
   (cx, cy)**, i.e. at the cell numbers themselves, not at the cell's position. (This is how
   the binary calls it; it samples somewhere near the world origin, so for normal play m1 is a
   near-constant. Keep it as the game does.) Skip the cell unless p < 0.8 × m1.
3. **Position.** x = 200·cx blocks; y = 200·cy + 100 × (cx mod 2) blocks, where "mod" is C's
   remainder (−1 for odd negative cx): odd columns are staggered by half a cell. z starts at 0.
4. **Second moisture test.** m2 = 0x5c4800 at block column (x, y). Skip the cell if m2 < 0.3.
5. **Height.** k = 0x5effa0(x, y, 0) (a 0..1 field of the Alpha terrain generator that rises in
   mountains) and h = 0x5c5e20(x, y, 0) (the Alpha generator's ground height at that column).
   z = h + 100 × k blocks; if z < 0 it becomes 0. Then z += 170 + 50 × N(17·cx, 19·cy).
   So a cloud's base is 120..220 blocks above the local terrain level, 100 more over mountains.
6. **Placement**, composed so that the last step listed applies to the model first:
   translate to (x, y, z) relative to the render origin (G+0x1d8, G+0x1e0; z relative to 0);
   scale uniformly by s = 7 + 2 × N(15·cx, 23·cy) (so 5..9 blocks per voxel); rotate about the
   vertical axis by (cx + cy) × 90 degrees; translate by (−½ × model x size, −½ × model y
   size, 0) (the model record's +0x44 and +0x48), so the model is centred over its base point.
   A cloud is therefore 120..216 blocks wide and 80..144 blocks tall.
7. Draw the model with the cloud shading (below).

Nothing in the placement depends on time: **Alpha's clouds do not move.** As the camera
crosses a 200-block cell boundary, a whole row of 13 cells is replaced at once (13 cells either
side = 1300 blocks, normally beyond the fog).

**The noise N(u, v)** (0x4c0ef0) is a lattice value noise with cosine interpolation on
doubles: the lattice point (a, b) gets the integer hash k = a + 57·b, k = (k << 13) xor k, value
= 1 − ((k·(k·k·60493 + 19990303) + 3524844813) and 0x7fffffff) / 1073741824 (32-bit
wrap-around arithmetic, constants at 0x745df8 = 1.0, 0x702b20 = 2⁻³⁰); the four corners around
(u, v) (integer parts by truncation towards zero) are blended with weights
(1 − cos(π·f)) / 2 on the fractional parts f, x first. The result is in about −1..1 and is
rounded to float.

**Shading.** AR §4 cloud row and AR §6.3: material (1, 1, 1, S); front and back colours both the
sun colour P4; ambient P6; darkness = ((sky1 + sky2) × 0.5, alpha S) (0x4a8f40 with 0.5, then the
alpha set to S); fogScale = 1/(1.5·F) (0x6fcde0 = 1.5); the cloud pixel shader 0x6ff7c8, which
blends the lit colour to the sky colour sky(Y) by 3D distance. Clouds are opaque (models carry
vertex alpha 1 and the sky colours alpha 1). Depth testing and writing are on as for the rest
of the world pass, and clouds are drawn before terrain.

Constants used: 0x6fcda0 = 0.01; 0x745dc0 = 1; 0x745da0 = 0.5; 0x6fcdc8 = 0.8;
0x6fcdc0 = 0.3; 0x6fd0b4 = 100; 0x6fcd7c = 50; 0x6fd598 = 170; 0x745e10 = 2; 0x6fce28 = 7;
0x6fce8c = −0.5; 0x6fcda4 = π/180; 0x6fce7c = 65536.

---

## B. Steam (the optional style)

### B.1 What differs, in short

| | Alpha | Steam |
|---|---|---|
| Palette | AR §3, 3 key colours, 4 times of day | §B.3: 4 colours (adds a sun colour), keys at L 0.2/0.6/0.75, more region modifiers |
| Smoothing over time | none | fog, sun colour and lights eased each frame (§B.4) |
| Sun | sprite (AR §8.4) | glow inside the sky shader (§B.5) |
| Sun path | D = (0, −sin a, −cos a) | D = normalise(sin a, −0.4·sin(a/2), −cos a) (§B.5) |
| Stars | 3000 fan sprites, on at S < 0.5 | about 1250 small `cube.cub` models on a dome, fading in (§B.6) |
| Clouds | `cloud02`, 13×13 cells, before terrain | `cloud02` recoloured or `cloud-dark`, cells to the view distance, after terrain, fading near the camera (§B.7) |
| Grading | none | luminance mix and contrast curve on sky, clouds and world (AR §14) |
| Moon, weather | none | none |

### B.2 Order of the sky draws (frame 0x1400e2e00)

1. The palette (0x1402b0400, §B.3) is evaluated for the camera position, then eased into the
   renderer's stored colours (§B.4).
2. The sun direction D is computed (§B.5) and the frame constants set: lights, sky colours
   (vtable +0xa0, the `Sky` constant buffer: skyColor1, skyColor2, fogColor, sunColor; setter
   0x1400c7440), the sun position (vtable +0xa8, setter 0x1400c7650, §B.5).
3. The **sky quad** (vtable +0x20 → 0x1400c5120): a screen-filling quad with the sky pixel
   shader (shader slot 8, the DXBC at file offset 0x4396a0), unless bit 0x80 of the frame's
   flags is set. (Vtable +0x28, called next at night when S′ < 0.5 and not under water, is an
   empty routine in this renderer.)
4. The world: creatures and models (0x1400d3170), terrain and water chunk lists (0x1400e6e60,
   0x1400e7980), batched models (0x1400e6270), and so on.
5. Water again (0x1400e7980), then, unless bit 0x80 of the frame's flags is set:
6. **Clouds** (0x1400e5730, §B.7);
7. **Stars** (in the frame routine, 0x1400e3d6e..0x1400e3fb2, §B.6), when S′ < 0.4 or during the
   eagle's far flight (0x2b8050; `creature-abilities-ranged.md` §13.6).

So, unlike Alpha, Steam draws its clouds and stars **after** the terrain, depth-tested against
it.

### B.3 The Steam palette (0x1402b0400)

Inputs: the world, the camera position (renderer +0x338, +0x340: x, y in fixed point; z after),
seven output colours P3..P9. Time: x = 2·t / 86,400,000 − 1, with t the float time of day at
world+0xc (86,400,000 at 0x140438028). **L = (1 − |x|³)⁴**, as in Alpha (exponents 3 at
0x14042e580 and 4 at 0x1404aca20).

The colours named here are the ones the renderer consumes: **sky1 = P3**, **sky2 = P4**,
**fog = P5**, **sun colour = P6**; P7, P8, P9 are the front, back and ambient light colours,
which belong to a lighting spec and are summarised only at the end.

**Region values** at the camera's block column (the world generator's routines, named as in
`worldgen-terrain-height.md` and `worldgen-structures.md`): the neighbourhood (0x358e0) and
the place there (0x35ce0: kind, weight); warmth W (0x2d9b60), dryness D (0x2b5960), ground
height H (0x35ea0, mode 0), lava weight Lv (0x2b9750), forest weight Fo (0x2d9f30, the region
kinds {5, 12}), and the place weight Pw, which is the place's weight when the place kind is 27
and 0 otherwise. (D acts here as moisture does in Alpha: high values give the green haze and,
in §B.7, more clouds. The name is the worldgen spec's.)

**Key colours before any modifier** (all alphas 1; the .rdata address of each vector given so
an implementer can check them):

| Key | sky1 | sky2 | fog |
|---|---|---|---|
| Day | (0, 0.6, 1) 0x140472980 | (0, 0, 0.8) 0x1404728f0 | (0, 0.1, 1) 0x140472940 |
| Dusk B (L 0.6) | (1, 0.95, 0.5) 0x1404728b0 | (0.3, 0.5, 1) 0x140472970 | (0.7, 0.6, 0.3) 0x140472820 |
| Dusk A (L 0.2) | (1, 0.5, 0.1) 0x140472720 | (0.5, 0.2, 0.8) 0x140472910 | (1, 0.5, 0.2) 0x14042ffa0 |
| Night | (0, 0.03, 0.15) 0x140472740 | (0, 0, 0) | (0, 0.1, 0.1) 0x140472700 |

Sun colour keys: day (1.5, 1.3, 0.4) 0x140472870; dusk (1.5, 0.85, 0.25) 0x1404727e0;
night (0, 0, 0) (alpha 0 too).

**Modifiers, in this order:**

1. **Near sea level.** If H < 20: k = max(H / 20, 0); D ← D·k; W ← W·k.
2. **Moist.** If D > 0.5: u = 2·(D − 0.5); day sky1 += (0, 0, −0.2)·u; day sky2 +=
   (0, 0.5, −0.3)·u; day fog += (0, 0.5, −0.35)·u (vectors 0x1404725f0, 0x140472600,
   0x140472610).
3. **Hot and dry.** If W > 0.8 and D < 0.4: k = min((1 − D/0.4) × ((W − 0.8)/0.2) × 4, 1);
   day sky1 → lerp(·, (0.6, 0.8, 1), k); day sky2 → lerp(·, (0.1, 0.4, 1), k); day fog →
   lerp(·, (0.8, 0.5, 0.7), k).
4. **Lava** (Lv > 0.001): every key below is moved to lerp(key, target, Lv). Targets: day sky1
   (1, 0, 0); both dusk sky1 (0.8, 0.1, 0); night sky1 (0.06, 0, 0.03); day sky2 (0.4, 0.2,
   0.2); both dusk sky2 (0.2, 0.2, 0.2); day fog and dusk B fog (0.3, 0, 0); dusk A fog
   (0.3, 0.1, 0); night fog (0.1, 0, 0). Night sky2 has no lava target.
5. **Forest region** (Fo > 0.001): lerp towards, by Fo: day sky1 (0.2, 0.6, 0.4); dusk B sky1
   (0.1, 0.6, 0.3); dusk A sky1 (0.3, 0.1, 0.4); day sky2 (0, 0.1, 0.2); dusk B sky2
   (0, 0.05, 0.1); dusk A sky2 (0.2, 0.2, 0.2); day fog and dusk B fog (0.4, 0.1, 0.8); dusk A
   fog (0.4, 0.2, 0.5); night fog (0.1, 0.15, 0.2).
6. **Place kind 27** (Pw > 0.001): lerp towards, by Pw: day sky1 (1, 0.6, 1); dusk B sky1
   (0.8, 0.2, 1); day sky2 (0, 0.6, 1); dusk B sky2 (0, 0.5, 1); dusk A sky2 (0.1, 0.5, 0.8);
   day fog and dusk B fog (0.6, 0.3, 1). Dusk A sky1, dusk A fog and the night keys are not
   changed.
7. **Far flight** (0x2b8050 true): the fog keys for day, dusk B and night take the value of the
   matching sky1 key, and the day sky1 becomes black (0, 0, 0). (Read from 0x1402b1dc3 onward;
   worth confirming with an oracle, §C.)

**Interpolation over L** (0x1402b1dc3..):

* L > 0.75: the day keys; sun colour (1.5, 1.3, 0.4).
* 0.6 < L ≤ 0.75: t = (L − 0.6)/0.15; each colour = lerp(dusk B, day, t); sun colour =
  lerp(dusk, day sun, t).
* 0.2 < L ≤ 0.6: t = (L − 0.2)/0.4; each colour = lerp(dusk A, dusk B, t); sun colour = the
  dusk sun.
* L ≤ 0.2: f = max(1 − (1 − L/0.2)·4, 0) (so f is 0 until L = 0.15 and 1 at L = 0.2); each
  colour = lerp(night, dusk A, f); sun colour = dusk sun × f.

Each blend is computed as a·(1 − t) + b·t on all four channels.

**After interpolation:**

* **Under water** (the block at the camera has type 2, 0x14000eab0): sky1 and fog are set to
  sky2.
* **Buff 0x13** on the local player (0x14005e3a0; the buff that using item kind 0x18, subtype
  8 gives for 60 s, `creature-abilities-ranged.md` §13): with k = min((60000 − the buff's
  float +8) / 3000, 1): fog and sky1 → lerp(·, (0.9, 1, 1, 1), k); sky2 → lerp(·, (0, 0, 0, 1),
  k); the lights move too (front and back towards (0.05, 0.1, 1, 1), ambient towards
  (0.01, 0.05, 0.2, 1)).

**Lights (summary only).** P7, P8 and P9 come from further key tables (0x1404727d0,
0x14043f0f0, 0x1404726e0, 0x140472750, 0x1404728e0, 0x14043f130, 0x1404726f0 and their
lava/forest/kind-27 targets) blended over L with a split at L = 0.5 (above it by
1 − (1 − 2(L − 0.5))⁴), plus an interior variant chosen by block flag 0x20 and the big-structure
kind 0x2aa270 (0x2b or 0x2c). P9's alpha is L. These are for a Steam lighting spec.

### B.4 Easing and the interior override (frame 0x1400e2e00)

The renderer keeps its own copies and eases them each frame with the smoothing factor
0xa7a0(dt, c) (`creature-physics.md` conventions): front, back and ambient light with
c = 0.001 (0x14042e514); fog with c = 0.0005 (0x14043ef2c); sun colour with c = 0.002
(0x14042e6d0). sky1 and sky2 are taken as computed, without easing.

Exception: 0x1400ecda0 (the same routine that works out the view distance at renderer +0x4a4)
sets an **interior colour** at renderer +0x4ec when the block at the local player has the
flags 0x20 and 0x02 in its byte 5; the colour is the interior's colour from 0x2aa140, bytes
divided by 255. While it is non-zero, sky1, sky2 and fog are eased (c = 0.0005) to it and the
sun colour is eased to zero: indoors the "sky" is the room's colour.

### B.5 The Steam sun

**Direction** (0x1400e38b4..0x1400e3961): a = (2t·π) / 86,400,000 (double, then float);
D = normalise(sin a, −0.4·sin(a/2), −cos a) (−0.4 at 0x14043efe4). At midnight D = (0, 0, −1);
at 06:00 the sun is in +x; at noon it is overhead tilted towards −y; and because sin(a/2) ≥ 0
over the whole day the path always leans towards −y. (Alpha's sun rises in −y and has no lean;
AR §2.3.) Under water the sun position is computed from (0, 0, −1) instead.

**Sun position constant** (`SunPos`, setter 0x1400c7650): P = 100·D (0x14042e5bc) rotated by the
view matrix's 3 × 3 part (no translation) and projected by the projection matrix with the
divide by w. sunPos = (screen x, screen y − (A − 80)/30, width/height, visible), with A the
camera's vertical angle (as AR §7.2, constants 80 at 0x14042e5b8 and 30 at 0x14042e5a8), and
visible = 1 when the projected depth is within 0..1, else 0.

**Sky pixel shader** (DXBC at file offset 0x4396a0, constant buffers Sky, FogAngle, SunPos):

1. Y = screen y + fogGradientTranslation; X = screen x.
2. top = lerp(sky1, sky2, smooth(sat(0.4Y + 0.2))); bottom = 0.4·sky1 + 0.6·fog;
   c = lerp(bottom, top, smooth(sat(2Y + 1.5))) (AR §14).
3. **Sun glow**: dx = (sunPos.x − X)·sunPos.z, dy = sunPos.y − Y; q = min(4·(dx² + dy²), 1);
   g = ((1 − q)·sunPos.w)²; c = sat(lerp(sunColor, c, 1 − g²)). The glow is pure sun colour at
   the centre and fades out at a radius of 0.5 screen half-heights.
4. Grading: l = 0.35 × luminance(c) (weights 0.299, 0.587, 0.114); green and blue become
   0.65·channel + l (red unchanged); output = 3c² − 2c³ per channel; alpha 1.

There is no sun model or sprite in Steam and no moon of any kind.

### B.6 The Steam stars

**Visibility value** S′ (frame, at the start): S′ = min(max(2·(1 − x²)⁴, 0.05), 1) (renderer
+0x52c), then multiplied by (1 − 0.75 × Fo) with Fo the forest weight at the camera. Stars are
drawn when S′ < 0.4 or during far flight. Their opacity is 1 in far flight, otherwise
min((1 − S′/0.4) × 4, 1) (vtable +0x70, the alpha constant), so they fade in over S′ 0.4..0.3.

**Placement** (0x1400e3dd8..0x1400e3fb2): the model is slot 0xa46, `cube.cub` (loaded at
0x14019c48b), drawn once per star. For ring r = 0, 1, … while 57·r < 2850 (r = 0..49):
n = trunc(50 − 7·√r) stars in the ring. For j = 0..n − 1:

1. k = 57·r + j; h = the integer hash of §A.5's noise applied to k ((k << 13) xor k, then the
   same cubic with 60493, 19990303, 3524844813, masked to 31 bits); v = h divided by 21474
   (integer, so 0..100000); b = 100000 − v.
2. If b is odd there is no star. Otherwise b ← b / 2.
3. Matrix, composed so the last listed applies first: rotate about the vertical axis by
   360·j/n + 13·r degrees; tilt about the x axis by 90·r/50 degrees; translate along y by
   64·R − 10 blocks, where R is the renderer's integer at +0x438 (the frame's 11th argument;
   the same 64-block unit drives the view distance); scale uniformly by 1 + (b mod 1000)/500.
4. Draw `cube.cub` with that world matrix.

So about half of the 1275 ring places hold a star: a dome of cubes 1..3 blocks big at radius
64·R − 10, dense at the horizon ring and thinning to one star near the zenith. Before the stars
the frame sets the far plane to 100000 (vtable +0x80). The view used for the pass carries no
camera translation in these matrices; whether the dome is centred on the camera through the
view matrix was not traced (§D).

### B.7 The Steam clouds (0x1400e5730)

**Models** (loader 0x140177f50, 0x1401df06c..0x1401df2fd): slot 0xdb8 is `cloud02.cub`; at load
every filled voxel is recoloured to **(100, 130, 180)** and the model's "full colour" flag
(+0x9a) is set, so its vertex colours are not halved (0x140170530; Steam otherwise stores model
colours at half intensity, AR §14), then its mesh is rebuilt. Slot 0xdb9 is `cloud-dark.cub`,
whose shade colour (+0xa0..+0xa2) is set to (10, 0, 20).

**Pass set-up:** shader slot 10 (by its constants the DXBC at 0x43b500, see below), no glow,
point lights cleared (0x1400ce580), material (1, 1, 1, 1), the camera's view and projection,
alpha 1.

**Grid.** Cells are 200 blocks, the centre cell being the camera block position (renderer
+0x338, +0x340) divided by 200 towards zero, as in Alpha. With V the view distance (renderer
+0x4a4), i and j run from −2 − trunc(V/200) to 2 + trunc(V/200) (i outer, j inner).
Cell (cx, cy): **x = 200·cx, y = 200·cy + 100·(cx mod 2)** exactly as in Alpha. For each cell:

1. **Coverage.** d = the dryness D (0x2b5960) of the neighbourhood at (x, y);
   n = 0x2e0c0(x, y) (the generator's simplex noise on doubles, `creature-physics.md` §7.7; here
   at the cell's block position). Skip unless (n + 1)·0.5 ≤ d·0.5.
2. **Height.** g = the ground height 0x35ea0(x, y, mode 1) × 65536, truncated, and raised to 0
   if negative; z = g + trunc((200 + 30 × N(17·cx, 19·cy)) × 65536), with N the same
   0x2e0c0 noise on (17·cx, 19·cy). So the base is 170..230 blocks above sea-level-clamped
   ground (Alpha: 120..220 above ground, +100 over mountains).
3. **Cull.** 0x1400e08c0 with the point, radius 250 (0x14043293c) and the distance V + 200;
   skip if it fails.
4. **Model.** f = the forest weight 0x2d9f30 at the place there (0x35ce0). f ≤ 0.3 →
   `cloud02` (slot 0xdb8), else `cloud-dark` (slot 0xdb9). Dark clouds hang over forest regions.
5. **Matrix**, as Alpha: translate to (x, y, z) relative to the render origin; scale by
   s = 7 + 2·N(15·cx, 23·cy); rotate about the vertical axis by (cx + cy)·90°; translate by
   (−½ model x size, −½ model y size, 0) (model record +0x88, +0x8c).
6. **Near fade.** dist = distance from the camera to (x, y, z) in blocks. If dist ≥ 180
   (dist² ≥ 32400) draw now, opaque. Otherwise a = clamp((dist − 80) × 0.01, 0, 1): a < 0.01 →
   not drawn; a ≥ 1 → drawn now; else the cloud is kept in a list with its matrix and a.
7. After the grid: alpha is set to 0 and every listed cloud drawn once (writing depth only, in
   effect), then each listed cloud is drawn again with alpha a. The previous lights are
   restored at the end (vtable +0xc0).

Nothing depends on time here either: **Steam's clouds do not move** (they only pop in by cell
and fade near the camera).

**Cloud pixel shader** (DXBC 0x43b500; identified by its alpha constant and its sky and sun
terms, the slot-to-blob table was not traced): sky colour with sun glow as §B.5 steps 1–3
without the final clamp; the lit vertex colour K is square-rooted per channel, then saturated
up by w = 1 + 0.25·(1 − lum) (K′ = w·K + (1 − w)·lum); fog f = sat(|V|²·fogScale²),
g = 1 − (1 − f³)²; c = sat(lerp(K′, sky, g)); then the §B.5 grading; alpha = the alpha constant.

---

## C. Oracles the user can record

The game must not be run by the spec writer; these are for the user, with the existing
in-game `tools/ZoneExtractor` DLL (which already writes oracles on request files) or plain
screenshots.

1. **Screenshots by hour** (Steam, and Alpha if an Alpha install is at hand), level camera
   (A = 90) and the default lens, at 00:00, 03:00, 04:48, 05:30, 06:00, 07:00, 09:00, 12:00,
   15:00, 18:00, 19:12, 21:00, in: a temperate plain; a hot desert (hot-and-dry modifier); a
   lava region; a forest region (kinds 5/12: dark clouds, purple haze); a place of kind 27;
   under water; inside a dungeon (interior colour); during an eagle far flight; with buff 0x13.
   Compare the pixel colour at fixed screen rows (top, middle, the horizon) against sky(Y).
2. **Palette dump** (new ZoneExtractor request, e.g. `sky.request`): for a list of times and
   positions, call or hook 0x1402b0400 and write P3..P9; and once per frame the eased copies
   at renderer +0x4ac..+0x51c, +0x52c and the sunPos constant. This checks §B.3–§B.6 number
   for number.
3. **Cloud dump**: hook the model draw 0x14016ec60 while 0x1400e5730 runs and write, per draw,
   the model slot, the world matrix and the alpha constant, with the camera position. Checks
   §B.7's cells, heights, scales, rotations and fades. The same hook in the star loop
   (0x1400e3d8b..0x1400e3fb2) gives every star matrix for §B.6.
4. **ZoneExtractor zone dump** of the zones under a cloud dump, so the ground height, dryness
   and forest weight used by the cloud tests can be checked against CubeVulkan's generator.
5. For Alpha, the user's D3D9 hooks (AlphaCWShadows) can read the frame constants (AR §15);
   recording the world matrices of draws of model slot 0xa08 gives the Alpha cloud oracle.

---

## D. Open questions

1. G+0x8006e6 (Alpha) skips both the sky quad and the clouds; what sets it was not traced.
2. Alpha 0x5effa0 and 0x5c5e20 are named here from their use (a 0..1 mountain-like field and
   the ground height); the Alpha terrain generator has no spec yet to confirm them.
3. The far-flight substitution (§B.3 step 7) was read from vectorised code; an oracle should
   confirm which keys change.
4. Steam star dome: R (renderer +0x438) and whether the pass's view is camera-centred.
5. The shader slot → blob mapping in Steam (slot 8 = 0x4396a0 is certain from the
   `SunPos`-only buffers; slot 10 = 0x43b500 is inferred).
6. The view distance V (renderer +0x4a4, from 0x1400ecda0) is Steam's analogue of Alpha's F
   (AR §7.1); its exact rule was not specified here.

---

## E. CubeVulkan today (`src/`, `shaders/`)

**Alpha look — drawn and following the specs:**

* the palette, daylight curves, sun direction, fog distance, material and gradient shift
  (`src/AlphaLook.*`, `shaders/alpha.glsl`, `shaders/alpha_sky.frag`), from AR;
* the 3000 stars from the C `rand()` sequence and the sun sprite (`AlphaLook.cpp` `stars()`,
  `shaders/alpha_sprite.*`, drawn from `Renderer.cpp` around "The stars at night").

**Alpha look — differs from the game:**

* **Star size**: `alpha_sprite.vert` adds the corners after projection, size/clip.w in clip
  units, with a minimum of one pixel. The game adds them in view space before the lens
  (§A.3), so its stars are about 2.14× larger vertically (and 2.14/aspect horizontally) than
  CubeVulkan's, and have no minimum.
* **Order**: CubeVulkan draws the sky, stars and sun after the opaque world at the far plane;
  the game draws them first. The picture is the same except for anything translucent drawn
  in between; not a fidelity issue today.
* **Zone factors**: `World::zoneFactors` maps the Steam generator's warmth, dryness and lava
  values onto Alpha's four factors (and a nine-sample sea share); Alpha's own zone-cell blends
  (AR §3.1) are not reproduced. Invented approximation.
* **Moon (invented)**: `src/AlphaMoon.*` and the moon sprite (mode 2 of `alpha_sprite.vert`)
  are CubeVulkan's own addition, switchable with `[Render] Moon=0`. Neither game has one.
* **Clouds: missing.** Nothing draws clouds (the only "cloud" in `src/` is
  `MoonRules.h`'s `cloud()` colour helper, unused for drawing, and Steam Cloud saves). §A.5 is
  to be implemented: the model slot for `cloud02.cub` from the owner's install, the shade
  colour, the 13 × 13 staggered grid, the noise N, the moisture tests and the cloud pass.

**Steam look — almost entirely invented:**

* `src/DayNight.h` (from the user's MoonandShadow mod, `MoonRules.h`) and `Palette.h`
  (colours measured from reference screenshots) replace the Steam palette; `shaders/sky.frag`
  draws a view-direction gradient, a sun disc, a moon disc and hashed point stars. None of this
  is Steam's §B.3–§B.6.
* The one faithful piece is the title-screen path of `sky.frag` (when `menuSky` is filled):
  its gradient and sun glow match §B.5's shader, and `vanillaGrade` the grading.
* Missing: the Steam palette with its modifiers and easing, the Steam sun path and `SunPos`,
  the cube-model star dome, and both cloud models with the near fade.

## Errata (from the clean-room implementation)

- §B.6: n = trunc(50 − 7√r) for r = 0..49 gives 855 places (about 427 stars), not 1275. 1275 is 50 + 49 + … + 1. To recheck in the decompile.
- §A.5: the hash constant 3524844813 (0xD218DD0D) differs in one bit from the oracle-checked world hash's 0xD208DD0D. §B.6 says the stars use "the same" hash, so one of the two is likely a transcription slip. To recheck in the binary.
- §B.7: the Steam cloud pass's fog scale and the cull routine 0x1400e08c0 need full specs; the implementation uses labelled stand-ins.
