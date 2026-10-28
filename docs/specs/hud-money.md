# HUD money display (0xe2e00, 0xee5a0, 0xf9ac0)

Behavioural specification of the money shown at the top left of Steam Cube World's HUD: a
small turning gold coin and the amount beside it. Written from the decompiled executable,
its assembly and the binary's constants. Routines are cited by RVA (image base
0x140000000). Conventions are those of `frontend-title.md` §0 (GUI units = window pixels ÷
the GUI scale) and, for matrices, `creature-pose.md` §2a (rotX, rotY, rotZ, scale and
translate act on the right, in the model's own frame).

Names used: **R** the world renderer (0xe2e00's object), **W**, **H** the window width and
height in pixels (R+0x41c, R+0x420, integers), **g** the GUI scale (0xe2e00's 14th
argument, the `guiScale` option), **world** R+0x8, **P** the local player (world +0x448).

---

## 1. When it is drawn

In 0xe2e00, after the world, the HUD block runs when **world +0x210 ≠ 0** (a world is
loaded) and draw flag **0x20 is clear** (`frontend-title.md` §3.2). In order it:

1. calls 0xe64b0 (the rest of the 2D HUD, not covered here);
2. queues the compass (§5);
3. queues the coin's shadow, then the coin (§2);
4. queues the context item's shadow and model, when there is one (§5);
5. queues the row of special items (§5).

"Queue" means a call to the overlay model routine **0xee5a0**; the queued models are drawn
at the end of the frame in the overlay pass (§3). Nothing here depends on the amount of
money: the coin is shown even at 0.

## 2. The coin

**Model**: table entry **0xae1**, `sprites/gold-coin.cub`, 8 × 8 × 2 voxels (x, y, z). If
the table has no entry 0xae1 nothing is drawn.

### 2.1 Its numbers

* **Turn**: θ = float(trunc32(world +0x440) mod 10000) × 0.0001 × 360 (degrees; the
  products in that order, in single precision; world +0x440 is the world clock in ms, so
  it is not affected by the system clock but stops when the world clock stops). One full
  turn every 10 s of world time.
* **Angles**: (x, y, z) = (**−30**, **θ**, **0**).
* **Scale**: k = (g × 0.004) × (1024 ÷ float(H)), the division done first (constants
  0x140437f70 = 0.004, 0x14043efcc = 1024).

### 2.2 The two draws

| | Screen point (pixels) | Colour | alpha | n2 | glow | n4 | depth |
|---|---|---|---|---|---|---|---|
| shadow | (50 × g, 57 × g) | (0, 0, 0, 1) | 1 | 0 | 1 | 0 | −0.5 |
| coin | (45 × g, 52 × g) | (1, 1, 1, 1) | 1 | 0 | 1 | 0 | 0 |

(0x14042e5b0 = 50, 0x14043efb8 = 57, 0x14042e7b0 = 45, 0x14043efb4 = 52, 0x14042e80c =
−0.5.) Both use the same angles and scale. The shadow is the same coin in black, 5 × g
pixels right of and below the coin, drawn first and with depth number −0.5; the five
numbers are those of the world draws (`creature-pose-items.md` §9: alpha, the second
number, glow, the fourth number, depth).

### 2.3 The model matrix (0xee5a0)

For a call with angles (ax, ay, az), scale k and model size (sx, sy, sz):

1. M = identity, with its translation column set to **(0, 0, 1, 1)** (0x14043f170): the
   model sits one unit in front of the overlay camera;
2. rotX(ax); rotY(ay); rotZ(az); scale(k, k, k);
3. centre: translation column += M × (sx × −0.5, sy × −0.5, sz × −0.5) (each row
   ((M[r] × cx + M[4+r] × cy) + M[8+r] × cz) added to M[12+r]; the game sums x first).

So the coin first turns by θ about its own y axis (the 8-voxel height of the coin), then
the whole is tipped by −30° about the screen's horizontal axis.

The entry stores M, the colour, alpha = 1, the second number, the glow, a 0, the depth
number and the screen point. An entry whose colour alpha is ≥ 1 goes to the opaque
overlay queue (kept by model); one below 1 to the blended queue. Both coin draws are
opaque.

## 3. The overlay pass (end of 0xe2e00)

After the world and HUD are drawn, 0xe2e00 sets up and draws the overlay queues R+0x18
(the opaque one through 0xe6e60, then the blended one through 0xe7980, both with mode 1):

* **View**: identity.
* **Projection** (0xa0340 into R+0x3dc): perspective with vertical field of view
  0.87266469 rad (50°, 0x140437f98), aspect **−(W ÷ H)** (negative: the overlay camera
  mirrors x), near 0.05, far 64 × R+0x438. With f = 1 ÷ tan(25°) ≈ 2.1445: P[0] = f ÷
  aspect, P[5] = f, P[10] = far ÷ (far − near), P[11] = 1, P[14] = −(near × far) ÷ (far −
  near), all else 0 (column-major).
* **Colour** constant (1, 1, 1, 1); **fog distance** 10000 (no visible fog).
* **Lights** (renderer slot +0xc0, the call the world pass makes with its time-of-day
  lights): first colour (0.7, 0.7, 0.2, 1), second colour (0.5, 0.5, 0.5, 1), direction
  (1, 1, −1) ÷ √3, fourth colour (0.3, 0.3, 0.8, 1) (0x14043f0a0, 0x14043f0f0, 0x14043f140;
  1 ÷ sqrt(3.0) from 0x14042e580, the third component times −1). The world pass passes
  R+0x50c, R+0x51c, its sun direction and R+0x4fc in the same positions.
* **Per entry** (0xe6e60, mode 1): world matrix = the entry's M; projection = S × P, where S
  is the identity with its translation column set to (dx, dy, −0.5, 1),
  dx = 2 × (x − W × 0.5) ÷ W, dy = ((y − H × 0.5) ÷ H) × −2. Because S is applied after P,
  its shift is multiplied by w: after the divide the model's projection is moved by
  (dx, dy) in normalised device coordinates, so the model's centre lands on screen pixel
  (x, y) (y down).

**Size on screen.** A model unit at depth 1 covers f × H ÷ 2 pixels. The coin's 8 voxels
are therefore 8 × k × f × H ÷ 2 = 8 × g × 4.096 × 1.0723 ≈ **35.1 × g pixels** across
(independent of the window size), the shadow likewise. (The same reasoning puts the title
logo, `frontend-title.md` §3.4, at ≈ 12.08 × g pixels per voxel.)

**Lighting.** The coin is lit like any world model by the overlay lights above, and drawn
with glow 1, the number the world draws pass for glowing items (`creature-pose-items.md`
§8.1). What the shader does with the glow number is not specified (open question 2).

## 4. The amount

The amount is an outlined text pair (`plasma-shapes-and-text.md` §7.2, call site 0xfcea0 in
0xf9ac0): nodes named **"gold"**, font `resource1.dat`, size **24**, outline radius **5**,
colour (1, 1, 1, 1) on an outline of (0, 0, 0, 1), flags 0, initial text "100", attached to
the interface node kept at +0x108 of the interface object (the HUD layer).

* **Place** (0x101bd0, on every resize and GUI-scale change, through 0x334700): its point
  (0, 0) at **(85, 60)** GUI units (0x140441bc4 = 85, 0x14042e7b8 = 60), i.e. (85 × g,
  60 × g) pixels. The coin's centre is 40 GUI units to its left and 8 above.
* **Text** (0x103200, every frame the interface updates): the decimal form of **P+0xaac**
  (money, a signed 32-bit integer, written with the standard wide-stream integer output),
  digits only: no separators, no unit, no icon.

The node next to it, "contextitem" (call 0xfcf1d, same style), sits at (255, 60) and shows
the context item's count (§5).

## 5. Neighbours in the same block (for orientation)

* **Compass**: table entry 0xdae, `compass2.cub` (29 × 29 × 15), at (W − 60 × g, 20 × g),
  angles (−120, 0, −R+0x358), scale (g × 0.003) × (1024 ÷ H), colour (1, 1, 1, 1), numbers
  (alpha 1, second 1, glow 0, 0, depth 0).
* **Context item** (when the item at +0x1e8 of the object P+0x1528 has a category ≠ 0): its
  item model (0x173b10), scale (0.035 ÷ max(16, its largest size)) × g × (1024 ÷ H), angles
  (−120, 0, θ) with θ as in §2.1, the item's glow, drawn as a black shadow at (230 × g,
  57 × g) with depth −0.5 and then in white at (225 × g, 52 × g), as the coin.
* **Special items**: category-0x18 items in a list of P's (+0x9f0, the second entry), whose
  region equals P's, kinds other than 0xe … 0x11, laid out from x = 0xe1 (225) or 0x145
  (325) in steps of 50, as the context item.

These are not specified further here.

## 6. Verification

No recording of the overlay exists yet. Requests for the ZoneExtractor (TEST character,
`guiScale` 1, then 1.25):

1. **Overlay calls.** Hook 0xee5a0. For each call whose model pointer equals model table
   entry 0xae1 (world +0x70, vector at +0x10, entry × 8), log: x, y (arguments 3, 4), the
   three angles (argument 5), the scale (6), the colour (7), the three numbers (8, 9, 10),
   world +0x440, R+0x41c, R+0x420 and the frame's time. Expect two calls a frame, the
   values of §2, and θ advancing 36° per world second.
2. **Overlay projection.** In the same frame, after 0xa0340 is called with R+0x3dc (inside
   0xe2e00, after the HUD block), dump the 16 floats at R+0x3dc and the arguments of the
   renderer slot +0xc0 call that follows. Expect §3.
3. **Screenshots** at 1280 × 720 and 1920 × 1080, standing still, over 10 s: the coin's
   bounding box (expect ≈ 35 × g pixels wide when it faces the camera, centred at
   (45g, 52g)), the direction it turns, and the text's left edge and baseline relative to
   (85g, 60g).
4. **Money text**: set P+0xaac to 0, 7, 123456 and −1 (memory poke) and screenshot.

## Open questions

1. **Text anchoring.** Whether (85, 60) is the text's top-left, its baseline start or its
   centre depends on the Plasma text node's own origin with flags 0; not traced. Oracle 3.
2. **The glow and depth numbers in the shader.** The coin passes glow 1 and the shadow
   depth −0.5; how the world shader uses the third and fifth numbers (an emissive term?
   a depth bias toward or away from the camera?) is not specified for any draw. Oracle: the
   renderer's constant buffer for the two coin draws (request 1 plus a dump of the pixel
   shader constants), or a screenshot comparison with the numbers forced to 0.
3. **Turn direction on screen.** The angles and matrices are exact, but the overlay camera
   mirrors x (negative aspect) and the coin is nearly symmetric; which way it appears to
   turn should be confirmed by oracle 3 rather than by reasoning.
4. **Light roles.** The four light arguments are given by position; whether they are front,
   back and ambient colours (as `sky-and-clouds.md` names P7–P9) and which is which belongs
   to a lighting spec. CubeVulkan's Alpha look may light the coin its own way; the
   numbers here are the Steam game's.
5. **Interface update cadence.** 0x103200 writes the text when the interface updates; if
   that is not every frame the amount may lag a pickup by a frame. Oracle 4 with a
   pickup.
