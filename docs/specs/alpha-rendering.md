# Alpha rendering: how Cube World Alpha lights and shades the world

Behavioural specification of the lighting, sky, fog and shading of the Cube World **Alpha**
client (`Cube.exe`, SHA-256 `84a7a132…30bf`), written from the Alpha decompile
(`disassembled alpha cube world/decompiled/client`), its assembly, the binary's constants and
the Direct3D 9 shader bytecode embedded in the executable. CubeVulkan's default look is to
follow this document; the Steam look stays an option (differences in the "Steam differences"
section).

Alpha routines are cited by their **address** in `Cube.exe` (image base 0x400000; the RVA is
the address minus 0x400000), which is how the Alpha corpus names them. Steam routines and
blobs, where cited for contrast, are named explicitly as Steam. Byte offsets into in-memory
records (the game object "G", the world "W" = G+0x2e4) are given only so that a reader can
cross-check; an implementation does not need the layouts.

The shaders are compiled Direct3D 9 bytecode (vs_3_0 / ps_3_0) stored in `.rdata`. They were
read by disassembling the blobs with the system's `D3DCompiler_47` disassembler. The HLSL
parameter names (`lightFrontColor`, `fogScale`, …) are the names the compiler kept in each
blob's constant table; they are the game's own data and are used here as names.

---

## 0. Conventions

* Colours are RGBA floats in 0..1 unless stated. "lerp(a, b, t)" is a + (b − a)·t.
  "sat(x)" clamps to [0, 1]. "smooth(x)" is 3x² − 2x³ (the shaders compute it as
  x²·3 − x²·x·2 on an already saturated x).
* World axes: +Z is up. Distances are blocks. World positions are 64-bit integers in
  1/65536 of a block; the renderer converts camera-relative positions to float blocks by
  multiplying by 1.5258789e-05 (= 1/65536, constant at 0x6fcd94).
* The day is 86,400,000 ms (float constant 0x702ac4, double 0x702b60). The time of day *t*
  is the integer at G+0x800440 (= W+0x80015c), milliseconds.
* Shader arithmetic is GPU float; CPU arithmetic is SSE single precision unless stated.
  Bit-exactness against the original GPU is not a goal; the formulas and their order are.
* The palette and light maths below are evaluated once per frame on the CPU and handed to
  the shaders as constants.

---

## 1. Where things are

### 1.1 The shader set and its object

The world shader object (RTTI `CubeShader@cube`, constructor 0x447c20, created by
0x447e10 during start-up from 0x4c8720, stored at 0x76b1bc and at G+0x800580) owns one
vertex shader and five pixel shaders, and one vertex declaration:

| Blob address (file offset) | Kind | Role in this spec | Bound by |
|---|---|---|---|
| 0x6fd928 (0x2fc928) | vs_3_0, 240 slots | **world vertex shader**, used by every world draw | all binders |
| 0x6feb50 (0x2fdb50) | ps_3_0, 50 slots | **world pixel shader**: terrain, creatures, items, models | 0x447d10 |
| 0x6fefa0 (0x2fdfa0) | ps_3_0, 19 slots | **sky pixel shader**: the screen-filling sky gradient | 0x447d90 |
| 0x6ff7c8 (0x2fe7c8) | ps_3_0, 24 slots | **cloud pixel shader**: fog-to-sky only, used for cloud models | 0x447d50 |
| 0x6ff1d0 (0x2fe1d0) | ps_3_0, 42 slots | **water pixel shader** | 0x447dd0 |
| 0x6ff5c0 (0x2fe5c0) | ps_3_0, 16 slots | created but **never bound** (no binder exists) | — |

Vertex declaration (0x447e10): stream 0, offset 0, `UBYTE4` as POSITION; offset 4,
`D3DCOLOR` as COLOR. So a world vertex is 8 bytes: x, y, z and a face index (0..5) as
unsigned bytes, then a BGRA colour that the shader reads as RGBA in 0..1.

Constant setters (each takes values on the CPU and uploads them):

| Routine | Constant (register) | What it uploads |
|---|---|---|
| 0x4482a0 | worldMatrix c27–29, worldViewMatrix c24–26, worldViewProjMatrix c20–23 | from world, view, projection |
| 0x448010 | cameraPosition c36 | camera position (blocks, render-origin relative) |
| 0x448170 | lightFrontColor c34, lightBackColor c35, ambientColor c38, lightDirection c33 | the direction is normalised here (1/√(x²+y²+z²), 0x448205) |
| 0x448280 | materialColor c32 | as given |
| 0x448070 | darknessColor c37 | as given |
| 0x449090 | white c39 | scalar |
| 0x448f10 | pointLightPositions c0–9, pointLightColors c10–19 | 10 positions (xyz, radius in w) and 10 colours (xyz, w = 0) from 16-entry tables |
| 0x448100 | fogScale c30 | 1 / distance, only if distance > 0 |
| 0x448090 | fogGradientTranslation c31 | (angle − 80) / (−30) (constants 0x6fce50 = 80, 0x745e9c = −30) |
| 0x449040 | skyColor1 ps c0, skyColor2 ps c1, fogColor ps c2 | three colours |
| 0x447fb0 | alpha ps c3 | scalar |
| 0x448fe0 | shininess ps c4 | scalar |

The vertex shader declares `pointLightPositions[16]` / `pointLightColors[16]` but only
registers 0–9 / 10–19 are allocated and used: **10 point lights**.

### 1.2 Other shaders in the executable (not world lighting)

The interface library (Plasma) has its own set, created by 0x68a350: a widget vertex shader
(0x720558), a pass-through vertex shader (0x7212c0), the widget pixel shader with
brightness/contrast/saturation and masks (0x720e28), a plain textured pixel shader
(0x721420), box down-samplers of 2×2, 3×3 and 4×4 taps (0x721530, 0x721778, 0x721a98) and
a 9-tap blur in x (0x722270, ps_3_0) and in y (0x721f00, ps_2_0). The blur weights are 0.18
at the centre and 0.15, 0.12, 0.09, 0.05 at offsets ±1..±4 × `blurScale` (sum 1.0). They
serve Plasma blur nodes (0x68cad0); **they are not applied to the 3D view**. Nothing in the
frame sets a gamma ramp. There is no post-processing of the world image (§13).

### 1.3 The frame (0x4ac260), in order

0x4ac260 renders one world frame. In order:

1. Device state: depth test on, depth function less-or-equal (0x4ac345..0x4ac36d).
2. **Sunlight strength S** (§2.2) is computed (0x4ac3e1..0x4ac44a).
3. Point lights are collected and assigned to chunks (§10).
4. Underwater test: the block at the camera position (0x42f860) has type 2 (water) →
   *underwater* (0x4ad6e2..0x4ad6f1).
5. Frame constants (0x4ad684..0x4ada3d): material colour, white = 0, alpha = 1, the palette
   (0x59d640, §3), sun direction (§2.3), camera position, lights, darkness = (0,0,0,1),
   shininess = 0, sky colours, fogScale, fogGradientTranslation; all 10 point lights
   cleared.
6. **Sky gradient**: one screen-covering quad drawn with the sky pixel shader (0x4adeb1,
   0x4aded6), unless G+0x8006e6 is set.
7. Map mode (G+0x8006e4 set) takes a separate path and returns (§11.5).
8. **Stars** (§8.3), when S < 0.5 and not underwater (0x4ae641..0x4aea7d).
9. **Sun sprite** (§8.4) (0x4aea83..0x4aed61).
10. World pass state: alpha blending on with source-alpha / inverse-source-alpha
    (render states 0x1b = 1, 0x13 = 5, 0x14 = 6 at 0x4aedc0..0x4aee12).
11. **Cloud models** (§11.4) with the cloud pixel shader (0x4b1489..0x4b1a65).
12. **Terrain** chunk meshes with the world pixel shader, culling clockwise (0x4b1a6b..),
    then particle effects.
13. **Water** chunk meshes with the water pixel shader, culling off, depth write on
    (0x4ba13e..0x4ba445), then back to clockwise culling and the world pixel shader.
14. Static models and items (0x4bd160, 0x4be760), then creatures (§11, from 0x4ba5d0).
15. Interface drawing.

---

## 2. Clock, sunlight and the sun direction

Two different daylight curves are used. Let **x = 2t / 86,400,000 − 1** (−1 at midnight,
0 at noon, +1 at the next midnight).

### 2.1 Palette daylight L (0x59d640, at its start)

**L = (1 − |x|³)⁴**, computed as: x in float; |x| through a double; pow(|x|, 3.0)
(double 0x6fcb98); 1 − that in float; pow(…, 4.0) (double 0x702b50). L drives the palette
(§3). Sample values: noon 1; 09:00/15:00 (|x| = 0.25) 0.9390; 06:00/18:00 (|x| = 0.5)
0.5862; 03:00/21:00 (|x| = 0.75) 0.1117; midnight 0.

### 2.2 Sunlight strength S (0x4ac3e1)

**S = (1 − x⁴)⁵**: x in float, pow(x, 4.0) (0x702b50), 1 − that, pow(…, 5.0)
(0x702b58). Sample values: noon 1; 09:00 0.9806; 06:00 0.7242; 03:00 0.1494;
01:00 (|x| = 0.9167) 0.0022; midnight 0.

S is used as:

* the **alpha of the material colour** of every world draw, which the vertex shader
  multiplies into the whole sun/sky lighting term (§5). At midnight that term is exactly 0:
  Alpha nights are black apart from point lights, the sky and stars;
* the multiplier **(1 − S)** of torch and street-light colours (§10.1);
* the sun sprite colour (§8.4) and the star switch (S < 0.5, §8.3).

### 2.3 Sun direction (0x4ad7a6..0x4ad822)

Angle **a = (t · 2 · π) / 86,400,000** (t to float, ×2.0 in float, then ×π and ÷86,400,000
in double, back to float). The light direction is **D = (0, −sin a, −cos a)**, normalised
by the setter. It points *from the surface towards the light*: at noon D = (0, 0, 1)
(overhead), at 06:00 D = (0, −1, 0), at 18:00 (0, 1, 0), at midnight (0, 0, −1).

The same D is used day and night. **There is no moon**: no moon direction, colour, disc or
texture exists in the client. At night the "front" colour is near black and the "back"
colour lights the faces turned away from D, i.e. upward faces at midnight (§3.6).

---

## 3. The palette (0x59d640)

Called once per frame with the camera position (G+0x140, a 64-bit world position) and six
output colours. It returns:

| Output | Becomes | Notes |
|---|---|---|
| P1 | skyColor1 (ps c0) | horizon side of the sky gradient |
| P2 | skyColor2 (ps c1) | zenith side of the gradient |
| P3 | fogColor (ps c2) | distance haze |
| P4 | lightFrontColor ("sun colour") | |
| P5 | lightBackColor; also ambient for the sky draw | |
| P6 | ambientColor for terrain, creatures, water | depends on zone warmth only |

All six have alpha 1, except the alpha of P4 and P5, which is never written in some paths
(an uninitialised stack value); the shaders use only their RGB.

### 3.1 Inputs

* **L** (§2.1) and the **underwater** flag (block at the camera of type 2; recomputed inside
  0x59d640 through 0x42f860).
* Four **zone factors** in 0..1 at the camera's block column (camera position ÷ 65536),
  each a distance-weighted blend over the region's zone cells within one cell (cells are
  16384 units, ±0x4000 around the point, with a noise-distorted sample position):
  * **Warmth** *Wm* = 0x5c4dd0: blends the float at zone-cell +0x10. Low values select the
    cold ambient colour (§3.7).
  * **Moisture** *Mo* = 0x5c4800: blends the float at zone-cell +0xc. High values select the
    green "wet" fog, low values the pale "dry" sky.
  * **Sea share** *Se* = 0x5f0720: share of nearby cells whose int at +0x18 is negative
    (below sea level).
  * **Fire share** *Fi* = 0x5ef040: share of nearby cells whose byte at +0x8 equals 1.

  The names are this spec's, chosen from how the palette uses them; the region generator's
  own specification is the authority on what the cell fields mean.

### 3.2 Day keys and the biome modifiers (applied in this order)

Day keys before any modifier:

| | R | G | B |
|---|---|---|---|
| Day sky1 | 0.1 | 0.6 | 1.0 |
| Day sky2 | 0.01 | 0.05 | 1.0 |
| Day fog | 0.01 | 0.1 | 1.0 |

1. **Hot and dry.** If Wm > 0.6 and Mo < 0.4: w = (1 − Mo/0.4) · ((Wm − 0.6)/0.4);
   day fog = (0.01 + 0.79w, 0.1 + 0.6w, 1 − 0.2w) (constants 0x71db28 = 0.79,
   0x71db18 = 0.599999964, 0x71db80 = −0.199999988; alpha + 0·w).
2. **Warm and wet.** If Wm > 0.3 and Mo > 0.6: m = min((Wm − 0.3)/0.7 · 4, 1);
   w = (1 − Se) · ((Mo − 0.6) · m / 0.4). Then
   day fog → lerp(day fog, (0, 0.7, 0.5), w) (alpha → lerp(alpha, 1, w));
   day sky1 = (0.1 + 0.4w, 0.6 + 0.299999952w, 1);
   day sky2 = (0.01 − 0.01w, 0.05 + 0.55w, 1 − 0.100000024w).
   (When this branch is not taken, sky2's blue stays 1.)
3. **Dry.** If Mo < 0.4: u = 1 − Mo/0.4; day sky1 → lerp(sky1, (0.6, 0.8, 1), u)
   per channel (R towards 0.6, G towards 0.8, B towards 1); day sky2 → lerp(sky2,
   (0.1, 0.4, 1), u).
4. **Fire.** If Fi > 0: with k = Fi every key of §3.2 and §3.3 is blended towards its fire
   value as lerp(key, fire, k). Fire day keys: sky1 (1, 0, 0), sky2 (0.4, 0.2, 0.2),
   fog (0.3, 0, 0).

### 3.3 Twilight and night keys

| Key (palette daylight) | sky1 | sky2 | fog |
|---|---|---|---|
| Night (L = 0) | (0.05, 0, 0.1) | (0, 0, 0.1) | (0, 0, 0.1) |
| Dusk A (L = 0.4) | (1, 0.3, 0.3) | (0.5, 0, 0.5) | (0.5, 0.2, 0.6) |
| Dusk B (L = 0.6) | (1, 0.9, 0.5) | (0.2, 0.1, 0.8) | (1, 0.9, 0.8) |
| Day (L ≥ 0.75) | §3.2 | §3.2 | §3.2 |

Fire targets for the twilight keys (applied by modifier 4, k = Fi): Dusk A sky1
(0.8, 0.1, 0), sky2 (0.2, 0.2, 0.2), fog (0.3, 0.1, 0); Dusk B sky1 (0.8, 0.1, 0), sky2
(0.2, 0.2, 0.2), fog (0.3, 0, 0); the night fog → (0.3, 0.2, 0.1). The night sky1 and sky2
have no fire variant. All alphas are 1.

### 3.4 Interpolation over L (all three colours alike)

* L > 0.75: the day keys.
* 0.6 < L ≤ 0.75: s = (L − 0.6)/0.15 (constant 0x6fcdb8); s·day + (1 − s)·duskB.
* 0.4 < L ≤ 0.6: r = (L − 0.4)/0.2; r·duskB + (1 − r)·duskA.
* L ≤ 0.4: r = L/0.4; r·duskA + (1 − r)·night.

Each blend is written as two products and a sum (a·k1 + b·k2), not as a lerp.

### 3.5 Underwater override

If underwater, after §3.4: **sky1 = sky2 = (0.1·L, 0.15·L, L, 1)** (0.1 and 0.15 are doubles
at 0x71a048 and 0x71db50; the products are made in double and rounded). The fog colour is
*not* changed. The frame also tints the material colour (§4).

### 3.6 Sun (front) and back colours (P4, P5)

Keys over L, interpolated in two halves:

| | L = 0 | L = 0.5 | L = 1 |
|---|---|---|---|
| Front (sun) | (0.01, 0.02, 0.01) | (1.0, 0.9, 0.6) | (1.1, 1.1, 0.9) |
| Back | (0, 0.02, 0.03) | (0.6, 0.5, 1.0) | (0.5, 0.6, 0.8) |
| Fire front | (1, 0.8, 0.1) | (0.8, 0.8, 0) | (1, 0.8, 0.1) |
| Fire back | (0.7, 0.3, 0.3) | (0.8, 0.4, 0.4) | (1, 0.5, 0.5) |

With fire share k each key is lerp(normal, fire, k). Then: if L ≤ 0.5, a = 2L, value =
(1 − a)·key(0) + a·key(0.5); else a = (L − 0.5)·2, value = a·key(1) + (1 − a)·key(0.5).
No underwater variant.

### 3.7 Ambient (P6)

Independent of time: (0.6, 0.6, 0.4, 1). If Wm < 0.2: t = Wm/0.2 and ambient =
((1 − t)·0.3 + t·0.6, (1 − t)·0.4 + t·0.6, (1 − t)·0.6 + t·0.4, 1), i.e. cold zones move
towards (0.3, 0.4, 0.6). (The routine first writes (0.4, 0.4, 0.4, 1) and immediately
overwrites it.)

Because the shader multiplies ambient by S (§5), ambient also fades out at night.

### 3.8 Worked values (temperate zone: Wm 0.5, Mo 0.5, Se 0, Fi 0)

| | t = 12:00 (L 1, S 1) | 06:00 (L 0.5862, S 0.7242) | 00:00 (L 0, S 0) |
|---|---|---|---|
| sky1 | (0.1, 0.6, 1) | lerp(duskA, duskB, 0.931) = (1, 0.8586, 0.4862) | (0.05, 0, 0.1) |
| sky2 | (0.01, 0.05, 1) | (0.2207, 0.0931, 0.7793) | (0, 0, 0.1) |
| fog | (0.01, 0.1, 1) | (0.9655, 0.8517, 0.7862) | (0, 0, 0.1) |
| front | (1.1, 1.1, 0.9) | (1.0172, 0.9345, 0.6517) | (0.01, 0.02, 0.01) |
| back | (0.5, 0.6, 0.8) | (0.5828, 0.5172, 0.9655) | (0, 0.02, 0.03) |
| ambient | (0.6, 0.6, 0.4) | same | same |

(Values rounded to 4 places; an implementation should reproduce them from the formulas.)

---

## 4. Constants per pass

| Pass | Pixel shader | material | front / back / ambient | darkness | fogScale | other |
|---|---|---|---|---|---|---|
| Frame set-up and sky | sky | (1, 1, 1, S); underwater (0.4, 0.5, 1, S) | P4 / P5 / P5 | (0,0,0,1) | 1/F | white 0, alpha 1, shininess 0 |
| Clouds | cloud | (1, 1, 1, S) | P4 / P4 / P6 | ((sky1 + sky2)·0.5, alpha S) | 1/(1.5·F) | |
| Terrain (per chunk) | world | the frame material | P4 / P5 / P6 | (0,0,0,1) | 1/F | the chunk's point lights, alpha 1 |
| Water (per chunk) | water | the frame material | as terrain | (0,0,0,1) | 1/F | alpha 1 |
| Creatures | world | §11.2 | as terrain | (0,0,0,1) | 1/F | white = flash value, shininess for some items |

F is the fog distance (§7.1). The sky and cloud passes are drawn before terrain, so the
terrain is drawn over them.

---

## 5. The world vertex shader (0x6fd928)

Inputs: byte position (x, y, z, f) and colour C (RGBA). Constants as in §1.1.

1. **Position**: P = (x, y, z, 1). Clip position = worldViewProj · P.
2. **Screen coordinate for the fog gradient** (per vertex, then interpolated): with clip
   (cx, cy, cz, cw), out.x = cx / cw, out.y = cy / cw + fogGradientTranslation. The division
   is per vertex, not per pixel.
3. **World position** Pw = world · P (3 rows). **To-camera vector** V = cameraPosition − Pw.
4. **Fog distances**: out.z = (V·V)·fogScale², out.w = (Vx² + Vy²)·fogScale² (the
   horizontal part). Both are squared and scaled, never square-rooted.
5. **Face normal** from f: 0 → +X, 1 → −X, 2 → +Y, 3 → −Y, 4 → +Z, any other value → −Z.
   N = normalize(world₃ₓ₃ · faceNormal).
6. **Sun and sky term**: T = ambient + max(N·D, 0)·front + max(N·(−D), 0)·back, with D =
   lightDirection. Then **T ← T · (material.a · C.a)**. C.a is the baked block light and
   ambient occlusion of §9; material.a is S (§2.2).
7. **Point lights**, for i = 0..9 in order: enabled only if radius rᵢ = posᵢ.w > 0.
   d = Pw − posᵢ.xyz; q = d·d; facing = clamp(−4 · (N · d/√q), 0.3, 1);
   fall = (1 − min(q / rᵢ², 1))²; T ← T + facing · fall · colourᵢ. (Lights are not scaled
   by C.a or material.a.)
8. **Clamp**: T ← clamp(T, 0, 1.2) per channel.
9. **Colour**: K = T ⊙ C.rgb ⊙ material.rgb; K ← darkness.rgb ⊙ (1 − T) + K;
   K ← K + white · (1 − K). Output colour = (K, C.a): **the output alpha is the vertex's
   light value, not the material alpha.**
10. Other outputs: 2·normalize(worldView₃ₓ₃ · faceNormal) (used by the shimmer, §6.2),
    lightDirection (used by water), V (used by water).

Consequences worth keeping in mind: top faces at noon in the open get T = min(ambient +
front, 1.2) = (1.2, 1.2, 1.2) and so are drawn 20 % brighter than their colour; vertical
faces at noon get only ambient (0.6, 0.6, 0.4), a warm shade; nothing exceeds 1.2× its
colour.

---

## 6. The pixel shaders

All world pixel shaders that read the sky share one **sky gradient** function of the
interpolated screen value Y (= out.y of §5.2):

* g1 = smooth(sat(0.6·Y)); top = lerp(sky1, sky2, g1).
* g2 = smooth(sat(2·Y + 1)); bottom = 0.2·sky1 + 0.8·fog.
* **sky(Y) = lerp(bottom, top, g2)**. (RGB; where alpha is carried it follows the same
  maths on the colours' alpha.)

### 6.1 Sky (0x6fefa0)

Output = (sky(Y), 1). Only Y is used.

### 6.2 World (0x6feb50): terrain, creatures, items, models

Inputs: lit colour K (§5), the fog values (out.z = 3D, out.w = horizontal), Y, the doubled
view normal n.

1. sky colour s = sky(Y).
2. **Sky blend** by horizontal distance: h = sat((out.w − 0.6)·2.5); f1 = smooth(h)².
3. **Haze** by 3D distance: v = sat(1.5·out.z − 0.025); f2 = 0.75 · (1 − (1 − v)²).
4. c = lerp(K, fog, f2); c = lerp(c, s, f1).
5. **Shimmer** (only when shininess > 0): angle θ = 10·Y + 5·X + n.y + n.x (X = out.x),
   reduced to (−π, π]; c ← c · (1 + 0.3·sin θ). The amount of shininess is ignored; only its
   sign matters.
6. Output = (c, alpha constant).

With fogScale = 1/F: the haze reaches its full 75 % at 3D distance ≈ 0.827·F; the sky blend
starts at horizontal distance 0.775·F and is complete at F.

### 6.3 Cloud (0x6ff7c8)

f = sat(out.z) (3D); g = 1 − (1 − f⁵)²; output = lerp(K, sky(Y), g) on all four channels
(colour alpha → sky alpha). Used with fogScale = 1/(1.5·F).

### 6.4 Water (0x6ff1d0)

Inputs: K.b and K.a (the lit blue channel and the vertex light value), Y, out.z, the light
direction D and V (to camera, unnormalised, in blocks).

1. s = sky(Y).
2. **Glint**: H = normalize( normalize(D + (0, 0, 2)) + normalize(V) );
   g = sat(H.z)¹⁰ · K.a; glint colour = g · sky1.
3. **Body**: a = sat(Y + 1); body = (0, 0.6 − 0.5·a, 1). Colour = K.b · body + glint.
4. Haze: c = lerp(colour, fog, sat(out.z)).
5. Sky: c = lerp(c, s, sat(2·out.z − 1)).
6. **Opacity** = 0.8 + 0.0001·|V|² (written without clamp; the target clamps to 1, reached
   at about 44.7 blocks). Blending is source-alpha / inverse-source-alpha.

Water is lit only through its blue channel: the mesher gives water vertices a blue of 1
(§9.5), so K.b is the light the vertex received.

### 6.5 The unused pixel shader (0x6ff5c0)

Never bound in Alpha. For reference: d = sat(out.z); base = lerp(K.rgb·(0.25, 0.25, 1),
sky2, min(1.25·d, 1)); far = lerp((0.8, 1, 1), lerp(sky1, sky2, sat(Y)), sat(Y + 1));
result = lerp(base, far, d³) when d > 0, else base; alpha 1. (Steam ships and uses a shader
built on the same constants, §14.)

---

## 7. Fog

### 7.1 Fog distance F (G+0x1d4)

Updated each simulation step in 0x488ee0 (0x489034..0x489316):

1. Start with m = R², where R = G+0x1d0 is the render radius: R = (window size in chunks)
   · 0.5 · 32 (0x45ab13..0x45ab36; the window is 40 chunks by default, 0x459f39, so
   R = 640 blocks).
2. For every chunk slot in the window that is missing or not yet ready (0x4890cf,
   0x4890f4), take the horizontal distance² from the camera to that chunk's corner (in
   blocks) and keep the minimum in m.
3. Target = √m − 22.4 (0x702a94).
4. Smoothing weight w = 1 − 0.999ⁿ with n the step's elapsed milliseconds (0x4ac150 with
   0.001, evaluated in double by repeated (1 − v)·0.001 + v). **F ← F·(1 − w) + target·w.**
5. If F < 0.0001 the following update steps are skipped (0x48930b).

So fog closes in on the nearest unloaded chunk and opens out to the render radius as the
world loads. The frame uses fogScale = 1/F (§4); clouds use 1/(1.5·F).

### 7.2 The gradient shift

fogGradientTranslation = (A − 80) / (−30), where A is the camera's vertical angle in
degrees (G+0x1a4; initialised to 120 at 0x45aadf), or 90 in map mode (G+0x8006e4). It
shifts the sky gradient (and the sky colour used by fog) up or down the screen with the
camera tilt.

### 7.3 Fog curves

See §6.2 (world: 75 % haze towards the fog colour by 3D distance, then full blend to the
sky colour by horizontal distance), §6.3 (clouds), §6.4 (water). There is no separate fog
for the sky itself.

### 7.4 Underwater

Underwater there is no separate fog distance or fog curve. The changes are: sky1 and sky2
become (0.1L, 0.15L, L) (§3.5), the material becomes (0.4, 0.5, 1, S) for terrain, water
and creatures (0x4ad6fc..0x4ad726), and stars are not drawn. The distant haze still uses the
normal fog colour.

---

## 8. The sky

### 8.1 The gradient quad

A two-triangle fan of world vertices (packed by 0x466650 as in §9.6, normal +Z) is drawn
with the world vertex shader, matrices built for it at 0x4adb10..0x4adea6, and the sky pixel
shader. Its only varying is Y (§5.2); the colour is sky(Y) (§6). It is drawn first, without
depth writes mattering, under everything else.

### 8.2 No sun disc in the shader, no moon

The sky pixel shader has no sun term. The sun is a separate sprite (§8.4). There is no moon.

### 8.3 Stars

* Generated once (0x459c40 at 0x4636a8..0x463859): **3000** entries, each from four CRT
  `rand()` draws in this order: azimuth θ = rand·2π/32767 (double), elevation
  φ = rand·(π/2)/32767 (double), then size = rand·0.1/32767 + 0.01 (float); the position is
  (cos θ · 100cos φ, sin θ · 100cos φ, 100 sin φ) with cos/sin of float arguments; then a
  fifth draw: if rand % 10 == 0 the size is doubled.
* Drawn when **S < 0.5** and the camera is not underwater, with the fixed-function pipeline
  (FVF position + diffuse, no texture, no shader): each star direction is transformed by the
  sky camera (§8.4) and drawn as a four-triangle fan: centre vertex opaque white
  (0xFFFFFFFF), four corner vertices fully transparent white (0x00FFFFFF) at (±1, ±1)·size
  around the projected centre. Stars do not fade with S: they switch on at S = 0.5
  (|x| = 0.5998: before about 04:48 and after about 19:12, by §2.2).

### 8.4 The sun sprite

* **Sky camera** (0x4ae3ba..0x4ae624): the game's view matrix (G+0x22c) with its
  translation recomputed so the camera sits at the origin; a perspective projection with
  near 0.1, far 2000, aspect = width/height, vertical field of view 50° (0.872664 rad), or
  30° (0.523599 rad) while the local player's flag 0x400 at +0x124 is set.
* **Texture** (built once at 0x462400..0x46252f, 256×256 A8R8G8B8, G+0x8006e0): for texel
  (i, j), r = √((i − 127.5)² + (j − 127.5)²); q = clamp(1.05 − 1.05·0.0078125·r, 0, 1)⁴;
  texel = (R 255, G 255, B 100 + 155q, A 255q) (bytes truncated).
* **Quad**: centre = sky view · (100·D); corners at centre ± (40, 40) (four corners, UV 0..1),
  vertex colour **(R 255, G min(50 + 250·S, 255), B trunc(200·S²) & 0xFF, A 255)**,
  modulated with the texture, alpha-blended. It is drawn whenever the frame draws the sky,
  also at night (when D points below the horizon the sprite is below the horizon too).

A second procedural texture (G+0x8006dc, white with alpha 128·max(1 − r²/16384, 0)³) is
made alongside it; it is used by billboard effects (0x4b8c59), not by the sky.

---

## 9. Block shading baked into chunk meshes

Chunks are 32 × 32 columns. The mesher is 0x49d910. It emits two vertex lists per chunk,
solid and water, split vertically into sub-meshes of 250 blocks (0xfa).

### 9.1 Which faces are emitted

A face of a non-empty block is emitted unless the neighbour on that side is non-empty
**and** (this block is water, or the neighbour is not water). So: faces towards air always;
faces of solid blocks towards water; never water–water or solid–solid.

### 9.2 Light values in empty and water blocks (0x59a0e0)

Empty (type 0) and water (type 2) blocks carry a light value 0..255 in their first colour
byte. It is computed by a fixed-iteration flood:

1. For every column of the region (top to bottom): empty/water blocks above the first
   non-empty, non-water block are **sky-lit**: working value 255. Those below: working
   value 0.
2. Repeat **16** times: for every empty/water block whose working value is not 255, take the
   largest of its six neighbours' values, where a neighbour of type 13 counts 255, an
   empty/water neighbour counts max(its working value, 5) and any other block counts 0;
   store (largest × 85) / 100 (integer). After each sweep the stored values become the
   working values.
3. Finally the working value becomes the block's light value (only inside the region proper,
   not its border).

It runs for the whole zone when a zone is generated (0x5e4850 → 0x5ede84), and when a chunk
is meshed at a zone edge or flagged (chunk +0x74), over the chunk plus a 16-block border
(4 if flagged).

### 9.3 Per-vertex light and occlusion (0x4c1510)

For a vertex at integer corner (x, y, z) of a face with normal n:

* Sample a box of blocks in front of the face: along the normal axis only the layer just
  outside the face (offset 0 if n > 0, −1 if n < 0); along each other axis the six offsets
  −3..+2 around the corner.
* For each block: weight w = (3 − m)/3, where m = the largest of |o + 0.5| over the three
  offsets (so 5/6, 1/2 or 1/6). Its light value v: type 13 → 255; empty or water → max(its
  light value, 5); anything else → 5, and it counts as **closed**.
* light = Σ v·w / (Σw · 255); open = Σ w·[not closed] / Σw.

Vertex alpha = **light**.

### 9.4 Per-vertex colour

From the block colour c (bytes / 255): c′ = clamp(c, 0, 1) per channel;
lum′ = (0.59·g′·0.1 + 0.3·r′·0.1 + 0.11·b′·0.1) · (−9) (constants 0x702b98, 0x6fcdc0,
0x702b94, 0x6fce94); shade = clamp(c′·0.1·10 + lum′, 0, 1) per channel (≈ c′ − 0.9·lum);
**vertex rgb = c · open + (1 − open) · shade**.

So occlusion pulls a colour towards a darker, more saturated version of itself, while
**sky light is carried separately in the alpha** and applied by the vertex shader.

### 9.5 Special block colours

* **Water** blocks: colour = (1 − b)·(0.1, 0.2, 1, 1) + b·(0.3, 0.4, 1, 1), b = the block's
  third colour byte / 255 (for water this byte holds the propagated light, §9.2); then §9.3
  and §9.4 as for any block.
* Blocks below a column's stored range are filled from a sampled colour (0x5f9620) blended
  with the column's reference block by k = min((bottom − z)·0.1, 1) (in 0x49d910); this is content colour, not lighting.

### 9.6 Packing (0x466650)

Position bytes x, y (0..32 within the chunk), z relative to the sub-mesh base, face index
from the normal (+X 0, −X 1, +Y 2, −Y 3, +Z 4, −Z 5). Colour bytes: trunc(channel·255) & 0xFF
for R, G, B and A, packed as a D3DCOLOR.

---

## 10. Point lights

### 10.1 Sources (0x4b0850..0x4b0e3c)

Collected each frame from objects in the chunks around the camera:

| Source | Colour | Radius | Where | Visible test |
|---|---|---|---|---|
| Static object kind 50 (0x32) | (1.2, 1.0, 0.8) · (1 − S) | 20 | object position + (0, 0, its height) | in view with a 20-block margin (0x47f760) |
| Static object kinds 51, 65 (0x33 torch, 0x41 campfire) | (1.2, 0.8, 0.4) · (1 − S) | 20 | same | 32-block margin |
| Item lying on the ground (owner field +0x140 == 0) | by item type (0x4c71c0): type 11 subtype 19 (0.1, 0.3, 0.3); type 18 subtype 0 (0.6, 0.6, 0.2); type 18 subtype 1 (0, 0.6, 0.2); others none | 16 | item position | only if colour² > 0 and in view |

Static light colours are (1.2, 1.0, 0.8) and (1.2, 0.8, 0.4) scaled by (1 − S) through
0x4252f0; items are not scaled. The visibility test 0x47f760 also requires the source to be
within F + 22.4 blocks of the camera (F the fog distance, §7.1) and inside the six view
planes widened by the margin.

### 10.2 Assignment

Each light is added to every chunk whose 32-block cell overlaps the light's position ± its
radius, up to **16 per chunk** (count at chunk +0x238, positions at +0x78, colours at
+0x178). A frame-wide list keeps the first 16 lights (0x4b12ea). The shader uses the first
10 of whichever list is bound: the chunk's for its terrain and water, the frame list for
creatures and models.

### 10.3 Falloff

As §5 step 7: fall = (1 − min(dist²/r², 1))², facing = clamp(−4·N·L̂, 0.3, 1) where L̂ is
the unit vector from the light to the vertex. Lights are evaluated per vertex.

---

## 11. Models and creatures

### 11.1 Model meshes and their occlusion (0x4e7870, 0x4eb8d0)

Voxel models (.cub sprites) are meshed into the same vertex format. For each emitted vertex:
occlusion factor w = (number of empty voxels) / (number sampled) over the box in front of
the face: the layer outside the face along the normal, offsets −3..+2 along the other two
axes (empty = colour (0, 0, 0)). **Vertex rgb = w·voxel + (1 − w)·shade**, where shade is
the model's shade colour (bytes +0x5c..+0x5e of the sprite record; set to black by the
constructor 0x4e6a20). Vertex alpha = 1 (models carry no baked sky light).

### 11.2 Creatures (0x4ba5d0..0x4ba7e3, drawn by 0x4128f0)

Material colour, componentwise products in this order:

1. (1, 1, 1, l), with l = the light value of the block 0.1 above the creature position
   (0x4718b0: type 13 → 255; empty/water → max(value, 5); otherwise 0), divided by 255;
2. × (0.5, 0.5, 1.5, 1) if the creature's int at +0x134 is positive;
3. × (1, 0.5, 0.5, 1) for each entry of its effect list whose first byte is 1;
4. × the frame material ((1, 1, 1, S) or the underwater tint);
5. if flag 0x200 at +0x7e: averaged (×0.5 after adding) with a highlight colour (0x4460f0).

So a creature's sun/sky term is scaled by S × the light where it stands, like terrain.
Body parts that use the creature's hair colour multiply that colour into the material
(0x4128f0).

* **Hit flash**: `white` = the creature's float at +0x1184 (§5 step 9).
* **Translucency**: if the float at +0x1190 is positive the creature is drawn twice:
  depth only (colour writes off, render state 0xa8 = 0), then with colour writes on and
  alpha = 1 − 0.75 × that value.
* **Shimmer**: items whose material byte (+0xd) is 1, 11, 12 or 22 are drawn with
  shininess 1 (0x4c7be0), others 0.

### 11.3 Other models

Static objects and items use the world shaders with the frame lights and material (1, 1,
1, S) unless their drawer sets otherwise.

### 11.4 Clouds (0x4b1534..0x4b1a65)

Only where the camera's moisture factor > 0.01 and fire share < 0.01: a 13 × 13 grid of
cells around the camera (offsets −6..+6), each cell drawing a cloud model (assets
`cloud01.cub`, `cloud02.cub`) when a noise test against the local moisture passes; position,
scale and rotation from noise. Shading: §4 cloud row and §6.3. With darkness = the sky
average and T usually < 1, clouds take the sky's colour in proportion to how unlit they are.

### 11.5 Map mode and the character preview

* Map mode (G+0x8006e4): material (1, 1, 1, 1), front (1, 1, 1), back (0.2, 0.3, 0.4),
  direction normalize(0, 0.5, −1), ambient (0.4, 0.4, 0.4) (0x4ae048..0x4ae163).
* The character preview in the interface: front (1, 1, 1), back (0.2, 0.3, 0.4), direction
  normalize(0.5, 0.4, −0.6), ambient (0.4, 0.4, 0.4).

---

## 12. Water, summarised

Colour: vertex colour (0.1, 0.2, 1) to (0.3, 0.4, 1) by its light (§9.5), lit by the vertex
shader; the pixel shader keeps only the lit blue and builds (0, 0.6 − 0.5a, 1)·blue, a glint
of sky1 coloured specular power 10 around a half-vector biased upwards by (0, 0, 2), haze
by 3D distance, sky by 3D distance. Transparency 0.8 near, opaque by ~45 blocks. No
reflection or refraction texture is used; the "reflection" is the sky-coloured glint and
the sky blend. Faces between water and air only (water–water faces are not emitted), drawn
after all opaque geometry, without culling, with depth writes on.

---

## 13. Post effects and gamma

None for the world: the frame renders straight into the back buffer. No gamma ramp, no
bloom, no colour grading. The only blur and down-sampling shaders belong to the interface
library (§1.2). Multisampling depends on the `antiAliasing` option.

---

## 14. Steam differences

Steam (`cubeworld.original.exe.unpacked.exe`, D3D11 `DXBC` blobs at file offsets 0x436fb0
world VS, 0x438810 world PS, 0x4396a0 sky PS, 0x439f90 water PS, 0x43ae90 and 0x43b500
further PS) differs as follows. The Steam palette routine itself was not traced for this
spec.

* **Time of day.** Steam adds a sun colour and a sun position to the sky constants and draws
  the sun **inside the sky shader** as a radial glow; no separate sun sprite. No moon in
  either.
* **Sky gradient.** Steam: g1 = smooth(sat(0.4Y + 0.2)), bottom = 0.4·sky1 + 0.6·fog,
  g2 = smooth(sat(2Y + 1.5)); Alpha: 0.6Y, 0.2/0.8, 2Y + 1.
* **Final grading (Steam only).** Steam ends sky and world pixel shaders with: G and B pulled
  35 % towards luminance (0.299, 0.587, 0.114), then a contrast curve 3c² − 2c³. Alpha has
  no grading.
* **Fog.** Steam computes fog in the pixel shader with a power curve of distance whose
  exponent depends on fogScale (a 2^(−403.95·fogScale) term), smoothed and then squared
  twice; Alpha uses the squared distances of §6.2 (75 % haze plus sky blend).
* **Per-face light.** Steam: front term = max(0.7·N·D + 0.3, 0) (wrapped); the back and
  ambient terms use a fixed axis, h = 0.5 + 0.5·|N·(0.597, 0.398, 0.697)|, back·h and
  ambient·h; only sun + back is scaled by vertex light × material alpha (ambient is not);
  the sum is saturated to 1. Alpha: plain max(N·D, 0) and max(−N·D, 0), everything scaled
  by S × vertex light, clamp to 1.2.
* **Vertex colour.** Steam doubles the vertex colour (stored at half intensity) and boosts
  saturation by up to ×2 in shade (k = clamp(1 + 0.5·(ambient.w − light.g), 1, 2)); it has
  a `glow` and a `depthOffset` constant; no `darknessColor`. Steam's face index is stored as
  index/255.
* **Darkness at night.** Steam's ambient is not scaled by vertex light, so Steam nights are
  not black; Alpha's whole sun/sky term goes to 0 at midnight.
* **Water.** Steam opacity = min(0.75 + 2.5e-5·dist², 1) × an alpha constant; Alpha 0.8 +
  1e-4·dist².
* **Unused shader.** Alpha's never-bound pixel shader (§6.5) corresponds in constants to
  Steam's 0x43ae90, which Steam uses.
* **Point lights.** Same 10-light falloff and facing formula in both.
* **Block occlusion.** Not compared here (Steam's mesher is specified elsewhere, if at all).

---

## 15. Checking

There are **no Alpha screenshots or captured frames** in the Alpha folder: `evidence/` holds
analysis metadata (identities, job metrics, schemas), and `resources/extracted/` holds the
game's assets (models such as `cloud01.cub`, `cloud02.cub`, interface images, sounds). No
recorded constant values exist either. An implementer can still check:

1. **The shader maths against the blobs.** Disassembling the six blobs of §1.1 (e.g. with
   `D3DDisassemble`) gives the exact instruction order; the user's own mod
   `AlphaCWShadows/build/tools/world-shaders.txt` already holds a disassembly of the world
   shaders' constant tables for comparison.
2. **The palette tables** by evaluating §2–§3 at the sample times of §3.8 and at the key
   boundaries (L = 0.4, 0.6, 0.75; S = 0.5), and for each modifier at factor values 0 and 1.
3. **Light propagation** on hand-made columns: an open column (255 down to the first solid
   block), a one-block overhang (255·85/100 = 216 beside sky-lit air after one sweep), and a
   sealed cave (stored values settle at 5·85/100 = 4, read back as 5; open question 7).
4. **Visual expectations** known from Alpha: very dark nights lit by torches; warm yellow
   sides at noon; deep-blue distant haze; red skies over fire zones; a large soft yellow sun
   sprite; white point stars only at night.
5. Captures from a live Alpha session (not made for this spec) would be the real oracle:
   frame constants can be read from the shader object's registers via the user's existing
   D3D9 mod hooks (AlphaCWShadows already reads the native front/back/ambient colours and
   sun direction).

---

## 16. Open questions

1. **Zone factors.** The meaning of zone-cell fields +0xc, +0x10, +0x18 and byte +0x8 is
   inferred from the palette only (moisture, warmth, below-sea, fire). The region
   generator's specification should confirm them, and the exact blend weights of 0x5c4dd0,
   0x5c4800, 0x5f0720 and 0x5ef040 (distance falloff constant at 0x71dcdc) are not written
   out here.
2. **The time field.** Whether G+0x800440 is always within [0, 86,400,000) (the formulas
   assume it is) was not traced.
3. **Uninitialised alpha** of the front/back colours (§3) is harmless for the shaders but
   means recorded constants may hold garbage there.
4. **Star projection.** The star and sun quads use the sky camera of §8.4; whether the star
   corner offsets are in view space (tiny) or in post-projection units (screen-sized dots
   independent of the field of view) was not settled from the assembly at 0x4ae6b0..0x4ae9b4.
5. **Sun sprite size**: same question (corners ±40 at distance 100 in view space would
   make a sprite about 44° across, mostly transparent given the q⁴ falloff).
6. **Block type 13** counts as full light (255) both in propagation and at vertices; which
   block that is (lava? a glowing block?) was not identified.
7. **Integer rounding of the 0.85 falloff**: (v·85)/100 with unsigned integers; the
   minimum value 5 for air means even sealed air is never fully dark. Exact cave values
   should be checked against a capture.
8. **Light-emitting items**: item type 11 subtype 19 and type 18 subtypes 0/1 were not
   identified by name.
9. **Cloud placement** (cell size, which model, height, rotation, the noise test at
   0x4b1600..0x4b1a42) is only summarised.
10. **Creature tints**: what +0x134 > 0, effect-list entries of kind 1 and the 0x200 flag
    (with colour from 0x4460f0) represent was not traced.
11. **Model shade colour**: whether any model sets a non-black shade (+0x5c..+0x5e) was not
    found.
12. **Render radius**: whether the `renderDistance` option changes the 40-chunk window
    (G+0x2dc) and hence R and the fog distance was not traced.
13. **Steam's palette routine** (its time-of-day keys and sun position) was not traced;
    §14 compares shaders only.
