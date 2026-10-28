# How the world map is drawn: mesh, lighting, camera (minimap and full map)

Behavioural specification of how Steam Cube World turns the map's region models into pixels:
the corner minimap and the full map screen. Written from the decompiled executable, its
assembly, the binary's constants and the shader bytecode embedded in it (disassembled with
the system's own shader disassembler). Routines are cited by RVA (image base 0x140000000);
shaders by the RVA of their bytecode blob.

What each region model contains (its voxel bytes, haze, overhang halving) is
`worldmap-model.md`; this file starts where that one ends ("finished for drawing", §4 there).
The flat icons and labels drawn over the map are `map-markers.md` Part 1, and the 3D creature
and tombstone markers are `map-markers.md` Part 2. The smoothing factor 0xa7a0 is
`creature-upkeep.md` §4.9.

**The main finding.** The game has no separate map look. The map regions are meshed by the
same routine as every voxel model (0x170700), with a per-corner occlusion term that both
darkens *and* saturates; the map pass then draws them through the same vertex and pixel shader
as the world's models, with a light rig of its own (§3). Four things make the Steam map look
solid: voxel colours enter the shader at **double** brightness (§2.1, §3.2); the face light
wraps (0.7 × N·L + 0.3) and is added to a second, fixed, unsigned "back" term (§3.2); the
ambient is blue, (0.25, 0.3, 0.7), not grey (§3.1); and the pixel shader ends with a
saturation lift, a partial desaturation of green and blue and an S-curve (§3.3). Section 3.4
gives the resulting light per face.

The minimap and the full map are **one draw** (0x2f44a0 through 0xe64b0). Opening the map
only moves the draw's screen centre and grows its scale, both eased (§4.5).

---

## 0. Conventions

* Matrices are 4 × 4 floats stored row by row, used with **row vectors**: a point p becomes
  p · M, and the translation sits in the last row. "M′ = A · M" means A acts on the point
  first. The matrix helpers are: translate 0x2b7a0, scale 0x2b580, turn about z 0x13050,
  turn about x 0x2b360, product 0x173e0. Turning about z by θ degrees takes (x, y) to
  (x cos θ − y sin θ, x sin θ + y cos θ); turning about x by θ takes (y, z) to
  (y cos θ − z sin θ, y sin θ + z cos θ). Degrees become radians through 0x42e520 (π/180).
* Colours are floats 0…1 unless written as bytes. "lum(c)" is named where it is used, since
  two different weightings occur (0.3/0.59/0.11 in the mesher, 0.299/0.587/0.114 in the
  shaders).
* "Map units": one voxel of a region model is one map unit, in all three axes. A map unit is
  u blocks (`worldmap-model.md` §1).
* sat(x) clamps to 0…1. smooth(x) = 3x² − 2x³.

---

## 1. Where it happens

| Step | Routine |
|---|---|
| Region model built, voxels filled | 0x2eb7b0 (`worldmap-model.md`) |
| Region model meshed | 0x170700 with border 3; per-corner colour 0x170530; corner occlusion 0x171db0; vertex packing 0xc3a10 |
| Map pass (sets light, fog, matrices, draws the queues) | 0xe64b0, called from the frame render 0xe2e00 |
| Map content queued (camera, regions, cubes, markers) | 0x2f44a0 (class `WorldMap`) |
| Queue a model instance | 0xee2f0 |
| Draw the opaque / blended queues | 0xe6e60 / 0xe7980 |
| Shader pair, raster and blend state by mode | 0xc3dc0 (renderer vtable at 0x4385e8, slot +0x08) |
| Screen centre and scale easing | game update 0xa5a50, around 0x1400ab4bd … 0x1400ab5b6 |

The frame render skips the map pass when world +0x210 is null or bit 0x20 of its flags is set
(0xa5a50 sets that bit when certain full-screen windows are open, 0x1400ad78c … 0x1400ad846).

---

## 2. The mesh (0x170700)

### 2.1 The model's settings for map regions

A map region's model is made by 0x1739a0 with its flag argument 1, which the constructor
0x16e6b0 stores at model +0x9a. The constructor also sets: +0x99 = 1, +0x9c = 3 (the
occlusion reach N), +0xa0 … +0xa2 = 0 (a colour offset, here none). Consequences:

* **No halving.** Models loaded from .cub files keep +0x9a = 0, and their corner colours are
  multiplied by 0.5 at the end of 0x170530; the vertex shader doubles them back (§3.2). Map
  regions skip the halving, so their colours reach the shader at **twice** their voxel
  brightness before lighting. This is deliberate and is a large part of the bright Steam map.
* With +0x99 set, the pure colours (255, 0, 0), (0, 255, 0), (0, 0, 255) are ordinary solid
  voxels (in .cub models they mark attachment points and are not drawn, 0x16f230).
* A voxel is **empty** when its three bytes are all 0.

### 2.2 Which faces

The drawn voxels are i = 3 … sizeX − 4, j = 3 … sizeY − 4 (the region's own 16 × 16 columns;
the 3-column border of `worldmap-model.md` §2 is never drawn but is read by the occlusion
term), and every k = 0 … sizeZ − 1. Order: i outer, then j, then k. A non-empty voxel emits
up to six faces, in this order, each only when its test holds:

| Face | Normal (index) | Emitted when | Corners, in order |
|---|---|---|---|
| west | (−1, 0, 0) (1) | i is the first drawn column, or (i − 1, j, k) is empty | (i, j, k), (i, j, k+1), (i, j+1, k+1), (i, j+1, k) |
| east | (1, 0, 0) (0) | i is the last drawn column, or (i + 1, j, k) is empty | (i+1, j, k), (i+1, j+1, k), (i+1, j+1, k+1), (i+1, j, k+1) |
| south | (0, −1, 0) (3) | j is the first drawn row, or (i, j − 1, k) is empty | (i, j, k), (i+1, j, k), (i+1, j, k+1), (i, j, k+1) |
| north | (0, 1, 0) (2) | j is the last drawn row, or (i, j + 1, k) is empty | (i, j+1, k), (i, j+1, k+1), (i+1, j+1, k+1), (i+1, j+1, k) |
| bottom | (0, 0, −1) (5) | (i, j, k − 1) is empty or outside (k = 0) | (i, j, k), (i, j+1, k), (i+1, j+1, k), (i+1, j, k) |
| top | (0, 0, 1) (4) | (i, j, k + 1) is empty or outside | (i, j, k+1), (i+1, j, k+1), (i+1, j+1, k+1), (i, j+1, k+1) |

So every region is a closed block: along its four edges it always shows its side walls down
to k = 0 (a cross-section of the terrain), and it always has a floor. Between two drawn
neighbouring regions those walls sit back to back.

There is no merging of faces and no sharing of corners: each face computes its own four
corners.

### 2.3 Vertex record and triangles

Each corner is 8 bytes (0xc3a10): x, y, z (low byte of the integer corner position), the
normal index of the table above, then r, g, b, a as bytes, each the low 8 bits of
trunc(value × 255) (0x42e7d8 = 255). The alpha value is 2.0 for every corner (§2.5), so the
alpha byte is 510's low byte, **254**. Both four-byte groups are read by the shader as
unsigned normalised bytes (input layout in 0xc60d0: POSITION and COLOR, both format 28,
R8G8B8A8 unorm).

Each face makes two triangles, corners (0, 1, 2) and (0, 2, 3) of its own four; the diagonal
is always corner 0 to corner 2. Indices are 16-bit when the model has fewer than 65 536
corners, else 32-bit. Model +0x80 gets the corner count and +0x84 the triangle count.

### 2.4 Corner occlusion (0x171db0)

For a corner at lattice point (x, y, z) of a face with normal n, with N = 3 (model +0x9c):

* Along each axis the sampled voxel offsets d are: −N … N − 1 when n is 0 on that axis; only
  0 when n is +1; only −1 when n is −1. So the window is the 6 × 6 square of voxels around the
  corner, in the single layer just in front of the face.
* Each voxel (x + dx, y + dy, z + dz) gets the weight w = (N − m) / N, where m is the largest of
  |dx + 0.5|, |dy + 0.5|, |dz + 0.5|. The inner 2 × 2 weigh 5/6, the next ring 1/2, the outer
  ring 1/6 (total 12 2/3 for a face).
* open = (sum of w over empty voxels, voxels outside the model counting as empty) ÷ (sum of all
  w). The result is **ao = max(0.5, open)** (0x4ac964 = 0.5). (It is 0 only if the total
  were 0, which cannot happen with N = 3.)

Because the window reads the undrawn 3-column border, occlusion is continuous across region
edges.

### 2.5 Corner colour (0x170530)

Given the voxel's bytes as c = byte / 255 per channel, and ao from §2.4:

1. a = ao^1.2 (powf; 0x42e718 = 1.2). With ao in 0.5 … 1, a is 0.435 … 1.
2. c = (c + offset) × a, offset = model +0xa0 … +0xa2 / 255 (zero for the map).
3. L = 0.3 r + 0.59 g + 0.11 b (0x42e704, 0x43ef64, 0x43ef48). If L > 1, divide r, g, b by L
   and set L = 1.
4. s = 3 − 2a (0x42e580 = 3, 0x4ac9d0 = 2). s is 1 for an open corner and up to 2.13 for the
   most hidden one.
5. Each channel becomes sat((1 − s) × L + s × channel): pushed away from its grey by s.
6. Alpha 2.0. (When model +0x9a is 0, all four are then halved; not for the map, §2.1.)

So occluded corners are both **darker** (a) and **more saturated** (s). Corners are
interpolated across each face, which gives the soft dark creases at the foot of every step,
cliff and wall, and the dark undersides. The overhang halving of `worldmap-model.md` §4 is
already in the bytes and stacks with this.

---

## 3. The shaders and their constants

The map uses the world's model shaders: vertex shader blob 0x4387b0 and pixel shader blob
0x43a010 (mode 0 of 0xc3dc0; the blended modes 3, 4, 6, 7 use the same pair). The vertex
shader reads ten constant blocks and the pixel shader six, bound by 0xc68c0:

| VS block | Contents | PS block | Contents |
|---|---|---|---|
| b0 | projection | b0 | sky: skyColor1, skyColor2, fogColor, sunColor |
| b1 | view | b1 | fogScale |
| b2 | world (the instance) | b2 | fogGradientTranslation |
| b3 | material colour | b3 | sun position |
| b4 | 10 point lights: position.xyz + radius, colour | b4 | alpha |
| b5 | light: front colour, back colour, direction | b5 | shininess |
| b6 | ambient colour (with w) | | |
| b7 | white (flash amount) | | |
| b8 | depth offset | | |
| b9 | glow | | |

### 3.1 What the map pass sets (0xe64b0)

In this order, after 0x2f44a0 has queued the map content:

* material colour (1, 1, 1, 1) (0x42e8c0);
* light (renderer slot +0xc0, 0xc6ce0): **front colour (0.75, 0.7, 0.3, 1)** (0x43f0c0),
  **back colour (0.5, 0.5, 0.5, 1)** (0x43f0f0), **direction (0.2, 0.5, 1.0) normalised**
  (0.2 = 0x42e700, 0.5 = 0x4ac964, the length √1.29 from 0x43ef80; the backend normalises
  again), and **ambient (0.25, 0.3, 0.7, 1.0)** (0x43f130). The ambient's w = 1 is used by the
  vertex shader (§3.2 step 6);
* all ten point lights cleared to zero (0xce580), so none apply;
* fog distance 20 000 (0x42e5ec), stored as fogScale = 1 / 20 000 (0xc6bb0);
* view = map +0x58, projection = map +0x18 (§4);
* a "target ring" call (slot +0x48) that the D3D11 renderer ignores (its slot is an empty
  routine, 0xe8a20);
* the opaque queue, then the blended queue, of the map's own queue (renderer +0x268).

The sky colours, fogGradientTranslation and sun position keep whatever the world pass set this
frame; with this fog distance they have almost no effect (§3.3).

Per region instance (0x2f44a0 → 0xee2f0 → 0xe6e60): colour (1, 1, 1, 1), **alpha = the
region's fade** (§4.3), glow 0, shininess 0, white 0, **depth offset 0.5**.

### 3.2 Vertex shader (0x4387b0)

Inputs: position bytes p (× 255 back to integers, w = 1), normal index (the fourth position
byte), colour bytes C (r, g, b, a).

1. The normal comes from the index (table in §2.2). nW = the normal through the world matrix's
   3 × 3, normalised. For a map region the world matrix is a translation (§4.3), so nW is the
   face normal in map space: **the light is fixed to the terrain and turns with it** when the
   camera turns.
2. Clip position = p through world, view, projection; then clip z is reduced by the depth
   offset (b8).
3. **Direct term** d = max(0, 0.7 × (nW · D) + 0.3), D the light direction (0.7 and 0.3 are
   shader literals).
4. **Fixed back term** h = 0.5 × |nW · (0.597022, 0.398015, 0.696526)| + 0.5. This direction is
   a literal in the shader, independent of the light constants.
5. Lsun = sat(h × back + d × front) per channel. lit = h × ambient.rgb + Lsun × (C.a ×
   material.a). Point lights would be added here (none for the map). light = sat(lit).
6. Colour: c2 = 2 × C.rgb (**the doubling**). k = clamp((ambient.w − light.g) × 0.5 + 1, 1, 2):
   the less green light a face gets, the more its colour is pushed from grey, up to ×2.
   g = 0.299 c2.r + 0.587 c2.g + 0.114 c2.b; c3 = clamp(k × c2 + (1 − k) × g, 0, 2).
   (When glow < 0, c3 = g, a grey; not used by the map.)
7. out = light × material.rgb × c3. (When glow > 0, out moves toward c2 × material.rgb by glow;
   not used by the map.)
8. Output colour = sat(out + white × (1 − out)); output alpha = C.a (254/255, unused by the
   pixel shader below). The view-space position and normal and the light direction in view
   space are passed on for the pixel shader.

### 3.3 Pixel shader (0x43a010)

1. c = the interpolated vertex colour. (When shininess > 0 it is multiplied by 1 + 0.3 ×
   cos(…) of screen position and normal, a sparkle; shininess is 0 for the map.)
2. **Saturation lift**: g = 0.299 r + 0.587 g + 0.114 b; t = 1 + 0.25 × (1 − g); c = t × c +
   (1 − t) × g.
3. **Fog**: q = (|view position|² × fogScale²) ^ (1 − 0.75 × exp(−403.95462 × fogScale)),
   at most 1; f = smooth(q). c moves toward fogColor by f, then toward a sky gradient colour by
   f⁴ (the gradient mixes skyColor1/2 by screen height and a sun glow). With fogScale =
   1/20 000 and the camera 300 units away, f ≈ 0.03 and f⁴ ≈ 10⁻⁶: about a 3 % tint toward the
   world's current fog colour, and no sky colour.
4. sat.
5. **Grade**: g = 0.299 r + 0.587 g + 0.114 b (of the fogged colour); green becomes
   0.65 g + 0.35 g_lum and blue 0.65 b + 0.35 g_lum, red is kept.
6. **S-curve**: each channel becomes smooth(channel) = 3x² − 2x³.
7. Output alpha = the alpha block (the instance's alpha, so the region fade).

There is no gamma step in either shader; step 6 is the only tone curve (see open question 5).

### 3.4 What a face receives

With the map rig, before the colour terms (light after step 5 of §3.2; k from step 6):

| Face | d | h | light (r, g, b) | k |
|---|---|---|---|---|
| top (+z) | 0.916 | 0.848 | (1, 1, 1) | 1.00 |
| north (+y) | 0.608 | 0.699 | (0.98, 0.98, 1) | 1.01 |
| east (+x) | 0.423 | 0.799 | (0.91, 0.93, 1) | 1.03 |
| west (−x) | 0.177 | 0.799 | (0.73, 0.76, 1) | 1.12 |
| bottom (−z) | 0 | 0.848 | (0.63, 0.68, 1) | 1.16 |
| south (−y) | 0 | 0.699 | (0.52, 0.56, 0.84) | 1.22 |

(+x is east, +y north, +z up in map space.) Tops are fully lit and, since colours were
doubled, often clip to full brightness in their dominant channel; sides facing away from the
light are cooler (blue kept at 1 by the ambient) and more saturated; south faces are about
half as bright as tops. Corner occlusion (§2.5) and the S-curve (§3.3) then deepen creases and
contrast.

---

## 4. Camera and placement (0x2f44a0, 0xe64b0, 0xa5a50)

### 4.1 Arguments

0xe64b0 calls 0x2f44a0 with: the map centre (three i64, dots; controller +0x1e20), the screen
centre in pixels (W × fx, H × fy; §4.5), the window size W, H (renderer +0x41c, +0x420), the
angle triple (pitch, 0, yaw) (§4.2), the zoom level L (controller +0x1e04, clamped to the
map's level table), **viewScale** = options `mapScale` (+0x1bd0) × mapScaleNow (+0x1e50), the
renderer +0x330 value as an int, a "not shrine mode" flag (not controller +0x668) and the
**half-radius flag** (§4.6). Nothing is drawn when H < 0.0001 (0x4ac954).

### 4.2 Projection (map +0x18) and view (map +0x58)

**Projection.** Perspective with a vertical field of view of 45° (half-angle 0x43ef54 = π/8,
f = 1 / tan(π/8) ≈ 2.4142): row 0 = (−f ÷ (W / H), 0, 0, 0) — note the sign, the x axis is
mirrored; row 1 = (0, f, 0, 0); row 2 = (0, 0, 1.0001, 1); row 3 = (0, 0, −0.10001, 0) (the bit
patterns 0x3f800347 and 0xbdccd20b: near 0.1, far 1000). It is then followed by a clip-space
shift S whose last row is (sx, sy, −0.1, 1), with sx = 2 (cx − W/2) / W and sy =
2 (cy − H/2) / H for the pixel centre (cx, cy) of §4.1 (0x42e528 = 0.1). Stored is projection ·
S: after the perspective divide the whole map is moved by (sx, sy) in normalised device
coordinates and its depth by −0.1. (D3D's y points up, so cy measured this way counts from the
bottom.)

**View.** Starting from identity, each helper acting first on the point: turn about z by the
yaw, turn about x by the pitch, (a turn about z by 0), scale by k on all axes, translate by
(0, 0, 300) (0x42e7e0). So a map point is turned by yaw, tilted by pitch, scaled, and pushed
300 units in front of the camera, which looks along +z.

* **k = 0.8 × viewScale**: viewScale × 0.6 × 80 ÷ 120 × 16 ÷ 8 (0x42e708, 0x42e5b8, 0x42e7c8,
  0x4aca34, and the region radius 8 of 0x5469a4, used here even when the drawn radius is
  halved).
* **yaw** = −(the world camera's third angle, renderer +0x358), so the map turns with the
  **camera**, not with the player's facing.
* **pitch** = min(−(the world camera's first angle, renderer +0x350), −120) (0x42e894 =
  −120). With this view, pitch −180 looks straight down at the map and −120 looks down at 30°
  below the horizontal; the map is never flatter than that and follows a steeper camera.

The renderer's three angles are copied each frame by 0xe2e00 from its fourth argument, which
the game update builds from the camera state at controller +0x1d80 (0x295d00).

### 4.3 Placing a region

For each region rx = cx − R … cx + R − 1 (outer), ry likewise (inner), R the drawn radius
(§4.6), cx and cy the centre's block position ÷ (16 u) truncated toward zero, and only if its
model exists and is non-empty:

* translation (map units) = ((16 u rx − 3 u) − centre.x, (16 u ry − 3 u) − centre.y,
  (lowest/u) × u − centre.z) in blocks, divided by u (fixed point × 1/65536, 0x42e510). The
  world matrix is this translation alone, so voxel (i, j, k) of §2 lands at map units (i, j, k)
  plus it.
* **fade** = (now − the region's build time, record +0x10) × 0.005 (0x42e518, per ms), 0 if
  negative, and 1 once above 0.99 (0x473358). A region thus fades in over 200 ms. While its
  fade is below 1 it is queued as a blended instance (0xee2f0 sends opacity < 1 to the blended
  list), drawn with the blended state and the pixel shader's alpha = fade.
* The region record's +0x0c is stamped with the map's frame counter (map +0x198, increased once
  per draw), which the map's 5-second sweep (0x2f3270, from 0x83c70) uses to drop regions not
  drawn lately. Map +0x168 accumulates the drawn regions' build times.

### 4.4 The centre

When not panning (controller +0x1e38 clear) the target centre (+0x1e08) and the drawn centre
(+0x1e20) are both set to the local player's position every update (0x1400a7e9f), so the map is
centred on the player at the player's height. While the full map is dragged, the target follows
the drag with its height taken from the terrain (0x35ea0 at the target), and the drawn centre
eases toward it with 0xa7a0 (dt, 0.01) (0x7d0b0).

### 4.5 Screen placement, size, and opening the map

Every update (0x1400ab4bd …), with s = options `mapScale` (default 1.0, range 1.1⁻³ … 1.1³,
F5 / F6; `frontend-options.md`):

| | target screen centre (fx, fy) | mapScaleNow eases toward |
|---|---|---|
| **minimap** (controller +0x668 clear) | (1 − 0.3 × (H / W) × s, 1 − 0.2 × s) | s |
| **full map** (+0x668 set) | (0.5, 0.6) | 1.8 (0x4328d0) |

Both are eased each update by the factor 0xa7a0 (dt, 0.01) (0x7d050 for the pair, 0x16ee0 for
the scale), so opening and closing the map glides the same 3D map between the corner and the
middle while it grows or shrinks. fx, fy are fractions of W and H, with fy counted from the
**bottom** (§4.2): the minimap's centre sits 0.2 s H below the top edge and 0.3 s H left of
the right edge. (When W ≤ 0 only the scale is eased.) The 0.3 is a register value traced along
the main path (open question 3).

Derived size, for checking: at the centre one map unit spans 2.4142 × 0.8 × viewScale / 300 ≈
0.00644 viewScale in device y, i.e. ≈ 0.0032 viewScale × H pixels. The shown square's half
width, 16 R map units, is therefore ≈ 0.41 viewScale × H pixels for R = 8 (before tilt and
perspective). The minimap is large and its far part runs off the right edge; the full map at
viewScale 1.8 is larger than the screen and is panned.

### 4.6 Radius

R = 8 regions (0x5469a4); halved to 4 when the 13th argument is set. The game update sets it
(bit 0x10 of the render flags, 0x1400ad7eb) when the id 0x11 is present in the set kept at
(controller +0x20) +0x198 (0x9aeb0); the other callers of 0x9aeb0 ask for 0x10, which suggests
the set holds held keys and 0x11 is the Control key (open question 2). When the flag is clear,
0x2f44a0 also does the mouse pick (§6).

The zoom level L is shared by both views (controller +0x1e04).

---

## 5. Water

There is no separate water surface, no transparency and no animation on the map. Water columns
are ordinary opaque voxels whose colour is fixed by `worldmap-model.md` §3.3 (blue, greener
when shallow and near forests, blended toward land colours at large units), meshed and lit like
everything else. Shininess is 0 for the map, so the pixel shader's sparkle never applies. The
only transparency on the map is the 200 ms fade-in of a new region and the blended cubes of §6.

---

## 6. Other 3D content of the map pass

In 0x2f44a0, in this order (`map-markers.md` §2.1 gives the overall order):

1. **Regions** (§4.3).
2. **Mouse pick** (only when the half-radius flag is clear): the cursor's ray, unprojected
   through the map's matrices, is cast into each region's model (0x16f6e0) up to 5000
   (0x42e5e4); a hit sets the map's cursor position (+0x100 … +0x110, dots) and the hovered
   land (+0x118, the region id 0x2ada20 at the hit). A forced value at +0x120 overrides it.
3. **Castle cells** of each fully faded-in region (record +0x68), only when bit 0x20 is set
   in byte +5 of the block at the player's position (0xeab0): model 0xa46 of the world's model
   table, at the cell's position snapped down to the unit, scaled uniformly by 16 / u (C
   division), coloured by the cell's kind byte — 9: (0.2, 0.2, 0.2) (0x43f090), 0xb: (1, 0.3,
   0.2) (0x473390), 0xa: (0.2, 0.5, 1) (0x4733a0), 0xc: (0.3, 0.5, 0.1) (0x473380), else white;
   kinds 9 … 0xc are additionally scaled (1, 1, 0.5). Only cells found in the local player's
   cell map (player +0x1528, +0x78, keyed by the cell) are drawn; that entry's byte +0x24 ×
   0.75 / 255 (0x42e710 = 0.75) is the cube's opacity (so the cubes are blended), white 0,
   depth offset 1.5 (0x42e548).
4. **Hovered land border** (when +0x118 is set): for each region in view, for each border point
   of the hovered land (`worldmap-model.md` §3.1), model 0xa46 at that block position, scaled by
   2 / viewScale, centred on its own size (× −0.5), white, opacity (cos(world clock × 0.006) +
   1) × 0.5 (a pulse, so drawn blended), depth offset 1.6 (0x4328c0).
5. **Creature and player markers, tombstone**: `map-markers.md` §2.2 … §2.5. The player is not
   an arrow: it is its own head (or chest) and hair model, turned by its facing, centred on its
   position, with 26 black copies offset by one voxel as an outline. Terrain has no outline.
6. The flight route's dotted line (not covered).

The flat icons and labels (places, missions, lands, flight points, names) are drawn afterwards
by the overlay 0x2829c0 through the same map matrices (`map-markers.md` §0.7, Part 1).

---

## 7. Minimap and full map compared

| | Minimap | Full map |
|---|---|---|
| Draw | 0x2f44a0 | the same call |
| Screen centre | eases to (1 − 0.3 s H/W, 1 − 0.2 s) | eases to (0.5, 0.6) |
| viewScale | s × mapScaleNow → s² (s = 1 by default) | s × 1.8 |
| Centre position | the player | the player, or the dragged target |
| Zoom level | shared | shared (changed by the wheel in the map, 0x94900) |
| Radius | 8 (4 with the half-radius flag) | the same rule |
| Mouse pick, hovered land | when the half-radius flag is clear | the same |
| Light, shaders, fog | identical | identical |
| Camera angles | follow the world camera | the same |

---

## 8. Constants

| RVA | Value | Use |
|---|---|---|
| 0x42e7d8 | 255.0 | byte ↔ float |
| 0x4ac964 | 0.5 | occlusion floor; light direction y |
| 0x42e718 | 1.2 | occlusion exponent |
| 0x42e704, 0x43ef64, 0x43ef48 | 0.3, 0.59, 0.11 | mesher grey weights |
| 0x42e580, 0x4ac9d0 | 3.0, 2.0 | saturation factor 3 − 2a |
| 0x4329b0 | (0.5, 0.5, 0.5, 0.5) | halving (not for map regions) |
| 0x43f0c0 | (0.75, 0.7, 0.3, 1) | light front colour |
| 0x43f0f0 | (0.5, 0.5, 0.5, 1) | light back colour |
| 0x42e700, 0x4ac964, 1.0; 0x43ef80 | 0.2, 0.5, 1.0; 1.29 | light direction and its squared length |
| 0x43f130 | (0.25, 0.3, 0.7, 1) | ambient |
| 0x42e5ec | 20 000 | map fog distance |
| shader literals | 0.7, 0.3; (0.597022, 0.398015, 0.696526); 0.5 | wrap; back direction; back weight |
| shader literals | 0.25; 0.65, 0.35; 3, 2; −403.95462, 0.75 | PS lift; grade; S-curve; fog exponent |
| 0x43ef54 | π/8 | half vertical field of view |
| 0x3f800347, 0xbdccd20b | 1.0001, −0.10001 | depth terms (near 0.1, far 1000) |
| 0x42e528 | 0.1 | device depth shift |
| 0x42e7e0 | 300.0 | camera distance |
| 0x42e708, 0x42e5b8, 0x42e7c8, 0x4aca34, 0x5469a4 | 0.6, 80, 120, 16, 8 | k = 0.8 × viewScale |
| 0x42e894 | −120.0 | flattest pitch |
| 0x42e518, 0x473358 | 0.005, 0.99 | region fade rate (per ms), snap |
| 0x42e51c | 0.01 | easing rate |
| 0x4328d0 | 1.8 | full-map scale target |
| 0x4ac954 | 0.0001 | minimum height |
| 0x42e548, 0x4328c0 | 1.5, 1.6 | depth offsets of castle and border cubes |
| 0x43ef34 | 0.006 | border cube pulse rate |
| 0x42e5e4 | 5000 | pick distance |

---

## 9. Oracles

None exists in `Extract/`. To record (for the user; ZoneExtractor additions or a frame
capture, not engine code):

* **Mesh dump.** Hook 0x170700 for map regions (model +0x9a = 1) and, at the point where the
  corner list is handed to the renderer (model +0x60, slot +0x08), save the 8-byte corners and
  the indices, together with the model's voxel bytes and sizes. Checks §2 exactly: faces, order,
  occlusion, colours. (A smaller variant: log the inputs and output of each 0x170530 call.)
* **Constant dump.** During one map pass (between 0xe64b0's entry and its 0xe6e60 call) log the
  values given to the backend setters 0xc6ce0 (light + ambient), 0xc6f20 (material), 0xc6bb0
  (fog), 0xc78b0 / 0xc7240 (view, projection), and per instance 0xc6790 (alpha), 0xc73b0
  (shininess), 0xc7a20 (white), 0xc6a80 (depth offset), 0xc6c50 (glow); plus the arguments of
  0x2f44a0 (screen centre, W, H, angle triple, L, viewScale, flags) and the matrices at map
  +0x18 and +0x58 after it returns.
* **Frame capture.** A RenderDoc capture of one Steam frame with the minimap, and one with the
  full map open, gives all of the above and the final pixels.
* **Screenshots.** Minimap and full map at known camera angles (looking north, east, south,
  west; low and high camera), with `mapScale` at 1.0 and at 1.331 (F6 three times), and while
  holding Control, over a town, a coast and a mountain. Compared with CubeVulkan's output at the
  same seed, position and angles.

---

## 10. Open questions

1. **Camera angle convention.** The map uses −(renderer +0x350) for its pitch (capped at −120)
   and −(renderer +0x358) for its yaw. How these relate to CubeVulkan's camera (zero direction,
   sign) is not established here. Oracle: the constant dump's angle triple and map +0x58 next to
   screenshots at the four headings.
2. **Half-radius flag.** It is set when id 0x11 is in the set at (controller +0x20) +0x198. The
   other uses of that test ask for 0x10, which reads like Shift and Control key codes. If so,
   holding Control halves the drawn radius and turns off the mouse pick. Oracle: screenshot
   while holding Control.
3. **Minimap offset 0.3.** The 0.3 in fx = 1 − 0.3 (H/W) s comes from a register loaded with
   0x42e704 (0.3) on the main path through 0xa5a50; another path loads it from the stack.
   Oracle: the screen centre argument in the constant dump, or a screenshot.
4. **Queue matrix.** 0xee2f0 multiplies each instance by its queue's current matrix (queue
   +0x50 … +0x8c); for the map queue it is taken to be identity, so the world matrix is the
   translation of §4.3. Oracle: the world matrix in the frame capture.
5. **Back-buffer format and gamma.** The shaders write their result with no gamma step. Whether
   the swap chain is an sRGB format (which would add one) was not traced. Oracle: frame capture
   (resource formats), or compare a screenshot's flat top colour against §3.
6. **Depth.** The map is drawn with a −0.1 device depth shift plus the per-instance depth
   offset, into the depth state of mode 0. Whether the depth buffer is cleared before the map,
   and so whether near world geometry can cover the minimap, was not traced. Oracle: frame
   capture, or a screenshot with the camera against a wall.
7. **Full-map background.** Whether the world keeps being drawn behind the full map, and any
   dimming, belong to the frame render and interface, not traced here. Oracle: screenshot.
8. **Castle cells** (§6 step 3): the gate (bit 0x20 of the block's byte +5), the meaning of the
   player's cell map and its byte +0x24, and the kind colours are as found; their meaning is
   inferred. Oracle: screenshot inside a castle or dungeon with the map open.
9. **Fog colours.** The 3 % fog tint uses the world's current fog colour, so the map shifts
   very slightly with the time of day. Oracle: two screenshots at noon and night.
