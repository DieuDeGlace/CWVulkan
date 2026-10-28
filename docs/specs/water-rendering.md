# Water: the surface, under water, and surface plants

Behavioural specification of how Cube World draws water: which blocks are water, how their
faces are meshed and lit, the water pixel shaders, transparency and draw order, what changes
when the camera is under water, and the plants that sit on the surface. The **Alpha** client
comes first (CubeVulkan's default look), then the **Steam** release (the optional style), then
what CubeVulkan does today.

Sources: the Alpha decompile and assembly (`disassembled alpha cube world`, routines cited by
their address in `Cube.exe`), the Steam decompile (`decompiled cube world steam`, routines cited
as 0x14xxxxxxx), constants read from both binaries, and the shader blobs disassembled with the
system's `D3DCompiler_47` (Alpha D3D9 blobs, Steam D3D11 `DXBC` blobs). Infinite-Cubes was not
used.

This spec **extends `alpha-rendering.md` (AR) and `sky-and-clouds.md` (SC)** and does not
repeat them. AR already gives the Alpha water pixel shader (AR §6.4), the water vertex colour
(AR §9.5), the face rule (AR §9.1), the light flood (AR §9.2–§9.3), the underwater palette and
material (AR §3.5, §7.4) and a summary (AR §12). SC gives the Steam palette, its underwater
override and the Steam sun (SC §B.3–§B.5). Conventions are AR §0: "sat" clamps to 0..1,
"smooth(x)" is 3x² − 2x³, "lerp(a, b, t)" is a + (b − a)·t; Y is the screen-height value of the
sky gradient (clip y / w plus the gradient shift, −1 at the bottom of the screen, AR §5.2).

---

## 1. Which blocks are water (both games)

* A block is water when its **type is 2**. Type is the low five bits of the fourth byte in
  Alpha (byte 3 & 0x1f) and the fifth byte in Steam (byte 4).
* **Sea.** Air at or below z = 0 reads as sea water unless the block carries the "dry" flag
  (Steam: flag 2, `worldgen-terrain-zone.md` §5.2–§5.3; Alpha: flag 0x40, read in the mesher
  0x49d910 at 0x49ec4b, which substitutes the static block at 0x76b054, type byte 0x82 = type 2
  with flag 0x80, set by 0x6f9b70). Above a column's stored top, z ≤ 0 is sea and z > 0 air.
* **River water** is stored by the generator (`worldgen-terrain-zone.md` §5.4 step 1; stored
  as (127, 127, 127, glow 0, type 2, flags 0), its red byte later replaced by the light pass).
  Structures can add water too (mason liquids, `worldgen-structures-mason.md`).
* **The renderer treats sea and river water identically**, except for Steam's whole-zone sea
  quad (§B.1) and Alpha's lily placement, which needs a stored water block (§A.8). Neither game
  has a per-region water colour: the stored colour bytes of a water block are never drawn
  (Alpha uses only its third byte as a light value, AR §9.5; Steam replaces the colour with a
  constant, §B.1). Water looks different by region only through the sky palette (sky1, sky2,
  fog), which the water shaders read.
* **Wet ground** (type 3, the river bed and other wet ground) is a solid block in both games
  (solid = type AND 0xfd is not 0), and in both its top face, when the block above is air,
  also gets a **film of water**: an extra quad in the water list (§A.1, §B.1).
* For lighting, water is an **open** block like air (AR §9.2; `worldgen-terrain-zone.md` §6):
  sky light passes through it undiminished. Neither game darkens water with depth.

---

## A. Alpha (the default look)

### A.1 Meshing (0x49d910)

Chunks are 32 × 32 columns, split in sub-meshes of 250 blocks; the mesher keeps a solid list
and a **water list** per sub-mesh (index lists at 0x76b0a8 and 0x76b0b4).

* **Faces** (AR §9.1): a face of a non-empty block is emitted unless the neighbour is
  non-empty and (this block is water, or the neighbour is not water). So a water block emits a
  face towards every **air** neighbour, in all six directions: the top surface, the sides where
  water meets air (at a waterfall edge, a dam, the edge of a lake held by nothing) and the
  bottom where water hangs over air. Solid faces towards water go to the **solid** list (the
  bed and the banks under water are ordinary opaque terrain). Water–water faces never exist.
* **Geometry**: full block faces at integer corners. The top surface lies exactly on the
  block's top (z + 1); **no lowering, no slope, no merging**: one quad (two triangles) per face.
* **Normals**: each water face keeps its own face index (+X, −X, +Y, −Y, +Z, −Z), so side faces
  are lit as sides by the vertex shader.
* **Wet-ground film** (the +Z face code, type test at 0x4a01f9): when the block is type 3 and
  the block above is type 0 (air; water above does not count), besides its normal solid top
  face a second top quad is added to the **water list** at the same height (z + 1), +Z normal,
  colour (0, 0, 1, 1), lit by 0x4c1510 like any vertex. Because the water pass runs after the
  terrain with depth test "less or equal" (AR §1.3), the film is drawn over the ground's top
  face as a translucent sheet of water.

### A.2 Water vertex colour and light

* **Colour** (AR §9.5): (1 − b)·(0.1, 0.2, 1) + b·(0.3, 0.4, 1) with b = the block's third
  colour byte / 255, then the occlusion shading of AR §9.4. The blue channel is 1 before
  occlusion, so on an open surface the shader's blue input is the lighting alone; under
  overhangs and against walls occlusion lowers it (AR §9.4: shade ≈ c − 0.9·luminance).
* **Light** (vertex alpha, AR §9.3): sampled in front of the face like any block; an open
  water surface under the sky gets 1.

### A.3 The pass and its state (frame 0x4ac260, 0x4ba13e..0x4ba45a)

After the terrain chunks (AR §1.3 step 13):

1. the alpha constant is set to 1 (0x447fb0); the water pixel shader is bound (0x447dd0,
   blob 0x6ff1d0);
2. render states: depth test on (state 7 = 1), **depth writes on** (state 0xe = 1), **culling
   off** (state 0x16 = 1); blending stays source-alpha / inverse-source-alpha (AR §1.3 step 10);
   depth function less-or-equal (AR §1.3 step 1);
3. the frame material (AR §4: (1, 1, 1, S), or the underwater tint of §A.7) is uploaded;
4. for every chunk in the chunk map's order (no sorting by distance), for every sub-mesh with
   water: the world matrix (a translation by the chunk's x, y and the sub-mesh's base z), the
   chunk's point lights (0x448f10, AR §10) and one indexed triangle-list draw;
5. afterwards depth writes stay on, culling returns to clockwise (state 0x16 = 2) and the world
   pixel shader is bound again (0x447d10).

### A.4 The vertex shader

The ordinary world vertex shader (AR §5): water vertices are lit by sun, sky, ambient and point
lights exactly like terrain, multiplied by the material. Nothing moves: the shader has no time
input and the water mesh is static, so **there are no waves and no vertex motion**.

### A.5 The pixel shader (blob 0x6ff1d0, AR §6.4)

Restated in full so the water can be implemented from this page:

* Inputs: the lit colour's **blue** K.b and its **alpha** (the vertex light) K.a; Y; the
  scaled squared distance q = |V|²·fogScale² (fogScale = 1/F, AR §7.1); the light direction D
  (towards the sun, AR §2.3); V, the vector from the surface to the camera, in blocks.
* **Body**: a = sat(Y + 1); body = (0, 0.6 − 0.5·a, 1); colour = K.b·body. At the bottom of
  the screen (Y = −1) the water is (0, 0.6, 1)·K.b, a light azure; from the middle of the
  screen up (Y ≥ 0) it is (0, 0.1, 1)·K.b, a deep blue. The gradient is in **screen space**,
  shifted with the camera tilt (AR §7.2), not by depth or distance.
* **Glint**: H = normalize(normalize(D + (0, 0, 2)) + normalize(V)); g = sat(H.z)¹⁰ · K.a;
  colour += g·sky1. The "normal" is world up for every face (side faces glint as if they were
  level), the light direction is bent upwards by (0, 0, 2) so the sheen exists even with a low
  or set sun, and the highlight is a broad, soft patch (power 10) coloured like the horizon side
  of the sky. It is not a texture and has no sparkle; it fades with the vertex light.
* **Haze**: colour = lerp(colour, fog, sat(q)) — full fog colour at distance F.
* **Sky**: colour = lerp(colour, sky(Y), sat(2q − 1)) — from 0.71·F to F the water turns into
  the sky gradient (AR §6).
* **Opacity** = 0.8 + 0.0001·|V|², unclamped in the shader (the target clamps it): 80 % at
  the camera, opaque from about 44.7 blocks.

### A.6 What the order means

* Water is drawn after the terrain and **before** the creatures and the far static models.
  Near static models are drawn with the terrain (0x4b2801, before the water); models beyond a
  distance threshold are queued in a distance-keyed map and drawn after the water (0x4ba4fe,
  0x4ba54a); creatures follow (from 0x4ba5d0). Because the water writes depth, **anything drawn
  after it and lying below the surface is hidden when seen from above** (it fails the depth
  test against the surface); only what is drawn before (the terrain, the bed, near models) shows
  through the 20 % transparency. This is a consequence of the order and states above; a capture
  should confirm it (§F).
* With depth writes on and no sorting, overlapping water faces (a side face behind a top face,
  or two chunks' surfaces) blend in chunk order: a nearer face drawn first hides a farther one
  drawn later; a farther face drawn first is blended under the nearer. Usually invisible because
  water faces rarely overlap on screen.
* No reflection or refraction texture exists; nothing is rendered twice. The "reflection" is
  only the sky-coloured glint and the fade into sky(Y).

### A.7 Under water

* **The test** (0x4ad6e2..0x4ad6f1): the block at the camera position (G+0x140, the camera
  itself, not the player) through the block lookup 0x42f860 has type 2. The palette repeats the
  same test (0x59d715).
* **Sky** (AR §3.5): sky1 = sky2 = (0.1·L, 0.15·L, L) with the palette daylight L: a dim blue
  that goes black at night. The sky quad, the fog-to-sky blend of terrain and water, and the
  glint colour all use it.
* **Fog**: the **fog colour is unchanged** (the region's day/dusk/night fog, e.g. day
  (0.01, 0.1, 1)) and the **fog distance F is unchanged** (AR §7.4). There is no separate
  underwater fog curve: what makes the water feel dense is the blue haze plus the dim sky.
* **Tint**: the material becomes **(0.4, 0.5, 1, S)** (0x4ad6fc..0x4ad726) for terrain, water
  and creatures; it multiplies the lit colour, so everything turns blue.
* **Light**: no other change. Sun, sky and point lights are as above water; the sun's term is
  not reduced with depth (the light flood treats water as open, §1).
* **Stars** are not drawn (AR §8.3); the sun sprite still is (it is not tested).
* **The surface from below**: culling is off, so the surface's top faces are seen from below
  with the same shader (V now points down: the glint is weak, the body colour and opacity as
  above).
* **No caustics, no screen overlay, no distortion, no colour grading**: nothing else in the
  frame or the interface reads the underwater flag.

### A.8 Lily pads and other surface plants (zone generator 0x5e4850, 0x5ec0e7..0x5ec2f1)

Alpha **places water lilies**. In the generator's plant pass over each column's stored blocks,
for a block of **type 2 whose block above is air**, with z the height of that air block:

1. skip unless the warmth at the column is above **0.2** (0x5c8350: the column record's float
   +8 when the column has one, else the warmth factor 0x5c4dd0, AR §3.1) — no lilies in cold
   zones;
2. skip unless z > 0;
3. skip unless |N(0.05·x + 24234, 0.05·y + 53565)| > **0.7**, N being Alpha's 2-D noise 0x4c0ef0,
   x and y the block column (constants 0x6fcdb4, 0x71df78, 0x71df80, 0x6fd570) — so lilies
   come in **patches** where the noise is strong;
4. skip unless rand() mod 10 = 0 (CRT `rand`) — one column in ten inside a patch;
5. place a zone model at (x + 0.5, y + 0.5, z + 0.1) blocks (0.5 at 0x745de8, 0.1 at
   0x71a048), scale **0.09** (the same as reeds), rotation rand()·360/32767 degrees (0x6fce6c,
   0x6fd338), type **0x1f + (rand() mod 2)**, flags 0 (it does not sway).

Types 0x1f and 0x20 map to `water-lily01.cub` and `water-lily02.cub` (model ids 0x991 and
0x992; table filled in 0x459c40 at 0x4628a4..0x4628dc). The pads are drawn like any static
model (AR §11.3), 0.1 above the surface. The random draws are taken in the order listed and are
part of the generator's single `rand()` sequence.

Other plants near water in Alpha: **reeds** (type 0x16, swaying) on river banks, and
**underwater-plant**, **alga** (sway) and **coral** (still) under water
(`vegetation-wind.md` §3.3). None of them is on the surface.

---

## B. Steam (the optional style)

### B.1 Meshing (0x1400e9530)

Render chunks are whole zones (64 × 64 columns), sub-meshes of 250 blocks; each sub-mesh has a
solid list, a lava list and a **water list**.

* **Faces**: the same rule as Alpha (§A.1): water emits faces towards air in all six
  directions, solid blocks emit faces towards water into the solid list.
* **Every water face carries the +Z normal** (the face-direction argument is (0, 0, 1) for all
  six faces of a water block), so the vertex shader lights
  side faces as if they were level surfaces and the glint treats them alike.
* **Colour**: every water vertex takes the constant **(0.25, 0.5, 0.5, 1)** (0x14043f0e0)
  instead of the block's colour, then the vertex light routine 0x1400e8f70 (occlusion sampled
  in the layer above the face, because the normal is +Z; the light byte of the open blocks
  there as the vertex alpha; occluded colours darkened and saturated). The world vertex shader
  doubles it to (0.5, 1, 1), so the blue is 1 on an open surface.
* **Whole-zone sea quad** (0x1400e9dc7..0x1400e9f4c): when every column of the zone has water
  at z = 0 and the block at z = 1 above it is fully lit (light byte 255), the zone's sea surface
  is **one 64 × 64 quad** at height 1, +Z normal, colour (0.25, 0.5, 0.5, 1), full light; the
  per-block meshing then skips water at z = 0. Partial sea zones (coasts) are meshed per block.
  Visually the same as per-block quads.
* **Wet-ground film** (type test at 0x1400eb9fc): type 3 with air above gets an extra top quad in the
  water list at z + 1, +Z normal, the water colour (0.25, 0.5, 0.5, 1), through the vertex light
  routine — as in Alpha.
* No lowering of the surface, no waves, no merging.

### B.2 The pass (0x1400e85e0) and its states

Called once per frame (0x1400e2e00, after the terrain lists 0x1400e6e60, the batched models
0x1400e6270 and the step 0x1400e6840), it walks the renderer's chunk list twice:

1. **Depth pre-pass**: state slot 5 (0x1400c3dc0): depth test on, writes on, "less or equal"
   (descriptor 0x140438740 = 1, 1, 4), culling **off** (raster state with cull none), and a
   blend state whose colour write mask is 0 — **no colour**. The frame material, glow 0 and
   white 0 are set, the alpha constant is the renderer's float +0x4a8 (it starts at 1 and is
   eased towards 1 every frame by 0x1400ecda0, so it is 1 in practice). Every water sub-mesh is
   drawn with a translation.
2. **Colour pass**: state slot 9: the same depth state and culling off, **alpha blending**
   (source alpha, inverse source alpha; alpha channel one/zero), the **water pixel shader**
   (renderer +0x28, `DXBC` at file offset 0x439f90, bound by 0x1400c5ff0), the light constants,
   and per chunk its point lights (vtable +0xd0 with the chunk's light tables). Every water
   sub-mesh is drawn again.
3. The alpha constant is set back to 1.

Because the pre-pass has already written the nearest water depth everywhere, the colour pass
only shades the **nearest water surface** of each pixel (farther water faces fail the
less-or-equal test): Steam water never blends with other water, whatever the order.

**Order in the frame** (0x1400e2e00): sky quad; creatures and models (0x1400d3170); terrain;
batched models; **water**; a second set of batched draws (0x1400e7980 on renderer +0x27×8,
state slots 3/4 then 6/7); clouds; stars (SC §B.2). Creatures and models are drawn **before**
the water, so submerged ones show through it.

### B.3 The vertex shader

The ordinary Steam world vertex shader (`DXBC` 0x436fb0, AR §14): with the +Z normal of §B.1
every water vertex gets the level-surface lighting, the front term max(0.7·N·D + 0.3, 0), the
back and ambient terms through the fixed-axis factor, the point lights, the colour doubling and
the shade saturation, times the material. Outputs used by the water shader: the lit colour, the
clip position, the view-space normal, the view-space light direction and the view-space
position. No time input: **no waves or vertex motion**.

### B.4 The pixel shader (`DXBC` file offset 0x439f90)

Constant buffers: Sky (sky1, sky2, fog, sunColor), FogDistance (fogScale), FogAngle (the
gradient shift), SunPos, Alpha. Inputs used: the lit colour's **blue only** (K.b), the clip
position, the view-space normal N, light direction D and position P.

1. **Screen values**: X = clip.x / clip.w; Y = clip.y / clip.w + gradient shift.
2. **Sky colour with the sun**: the Steam gradient (g1 = smooth(sat(0.4Y + 0.2)), top =
   lerp(sky1, sky2, g1), bottom = 0.4·sky1 + 0.6·fog, g2 = smooth(sat(2Y + 1.5)), sky =
   lerp(bottom, top, g2)), then the sun glow of SC §B.5: with dx = (sunPos.x − X)·sunPos.z,
   dy = sunPos.y − Y, q = (1 − min(4(dx² + dy²), 1))·sunPos.w, k = 1 − q⁴; skySun =
   lerp(sunColor, sky, k).
3. **Body**: b = 0.8·smooth(sat(Y + 1)); body = lerp(sky1, sky2, b). Like Alpha, a screen-space
   gradient: the bottom of the screen shows sky1 (day (0, 0.6, 1)), from the middle up it is
   80 % of the way to sky2 (day (0, 0, 0.8)): a deep blue. Unlike Alpha, the water takes the
   **palette's colours**, so it follows the region, dusk and night.
4. **Sheen**: R = normalize(2(D·N)N − D), the light reflected about the normal; s = max(R ·
   (−normalize(P)), 0) — a Phong term with **exponent 1**, very broad; sheen = s·0.25·sky1.
5. colour = sat(K.b·body + sheen).
6. **Fog**: d² = |P|²; f = d²·fogScale²; e = 1 − 0.75·2^(−403.95462·fogScale); h = min(f^e, 1);
   s = smooth(h); colour = lerp(colour, fog, s); then colour = sat(lerp(colour, skySun, s³)).
   (The same curve as the Steam world shader, AR §14.)
7. **Opacity** = min(0.75 + min(0.000025·d², 1), 1) × alpha: 75 % at the camera, opaque from
   100 blocks.
8. **Grading**: lum = 0.299r + 0.587g + 0.114b; g and b move 35 % towards lum (g = 0.65g +
   0.35·lum, b likewise; r unchanged); then each channel c ← 3c² − 2c³.

### B.5 Under water

* **The test** (0x1400e2e00, through the block lookup 0x14000eab0): the block at the camera position (renderer +0x338,
  +0x340, +0x348) has type 2. The palette (0x1402b0400) makes the same test.
* **Palette** (SC §B.3): after interpolation, **sky1 and fog become sky2**: the whole sky, the
  fog and the water body take the upper sky colour — by day (0, 0, 0.8), a deep blue; region
  modifiers (forest, lava, kind 27) change it as they change sky2.
* **Fog distance**: unchanged (0x1400ecda0 shortens it only indoors or with buff 0x13).
* **Tint**: the frame material becomes **(0.5, 0.5, 1, 1)** (0x1400e2e00 at 0x1400e3399;
  default (1, 1, 1, 1)); it multiplies terrain, water and models.
* **Sun**: the sun position constant is computed from (0, 0, −1) instead of the sun direction
  (SC §B.5), so no sun glow is seen; stars are not drawn (the night star call is skipped).
* **Light**: no other change (the light directions and colours are the palette's as usual).
* **No caustics, no overlay, no distortion.** The grading of §B.4 step 8 applies as always.

### B.6 Lily pads and other surface plants

Steam **loads** `water-lily01.cub` and `water-lily02.cub` (model ids 0xd23 and 0xd24; zone
model types **0x2c and 0x2d**, table filled in 0x140177f50 at 0x1401e2d3c..) but **no Steam
generator routine places them**: none of the callers of the zone-model add routine 0x140069b30
(0x1403191b0, the plant pass, 0x1402ff6f0, 0x1402fb7d0, 0x140068510, 0x140068790, 0x140068b00,
0x14006a9a0, 0x1402626e0, 0x140263210, 0x140263ea0, 0x140264670) writes type 0x2c or 0x2d.
(Room object kind 0x2c in 0x1402fc270 is a static object, a different id space.) So vanilla
Steam has **no lily pads**; the assets are leftovers of Alpha's §A.8. A zone dump with the
extraction tool would confirm it (§F).

Plants in Steam's plant pass (`vegetation-wind.md` §3.2): **reeds** (type 0x23) by rivers;
**alga**, **underwater-plant** (sway) and **coral** (still) under water at z ≤ −5. None on the
surface.

---

## C. What neither game has

* no waves, no vertex motion, no animated normals, no time input in any water shader;
* no reflection or refraction pass, no screen-space reflection, no environment texture;
* no caustics on the bed, no light shafts, no underwater particles drawn by the renderer;
* no depth-based colour or opacity: water does not get darker with depth (the light passes
  through it); its colour varies only with screen height (the body gradient), the vertex light,
  occlusion, distance (haze, sky, opacity) and the palette;
* no foam or shoreline effect; no separate underwater fog distance or curve;
* no region-specific water colour (only the palette's influence).

---

## D. The user's reference screenshots

All three are **Steam** (Steam interface, Steam hours), very likely with the user's mods
(an Alpha-lookalike lighting mod, SteamShadows, WaterLilies) and ReShade. Compared with
vanilla Steam (§B) and vanilla Alpha (§A):

**`water-surface-reference.webp`** (a saturated blue river, see-through near the bank, white
glints):
* *Vanilla*: the saturated blue — Steam's body is sky1 → sky2 by screen height, made more
  saturated by the grading's contrast curve; the river turning a deeper blue towards the top of
  the picture; the near water letting the bank show through (opacity 75 % near, Alpha 80 %; the
  submerged bank walls are solid faces towards water, drawn before the water); a translucent
  sheet over exposed wet ground at the waterline (the type-3 film, §A.1/§B.1) can also read as
  "see-through shallows".
* *Not vanilla*: the **sharp white streaks** of glint. Steam's sheen is exponent 1 (a broad
  wash, coloured sky1 at 25 %), Alpha's is a soft power-10 patch coloured sky1; neither makes
  small bright white highlights. The soft cast shadows (SteamShadows), the lily pads and
  flowers (WaterLilies), and any bloom/sharpening (ReShade) are additions too.

**`water-lilies-reference.webp`** (deep blue, calm, darker with depth):
* *Vanilla*: the calm flat surface (no waves in either game) and the deep blue that darkens
  towards the far end — but that darkening is the **screen-height gradient** (and distance haze
  and opacity), not depth. Reeds on the bank are vanilla.
* *Not vanilla*: lily pads with pink flowers (Alpha has pads without flowers, §A.8; Steam has
  none, §B.6). "Darker with depth" as such would be CubeVulkan's own addition.

**`water-underwater-reference.webp`** (deep blue fog with distance, teal-tinted floor):
* *Vanilla*: the deep blue fog — Steam's fog colour becomes sky2 under water, day (0, 0, 0.8);
  the distance at which it closes in is the normal fog distance. The bluish tint of the floor
  and the creatures comes from the (0.5, 0.5, 1) material (Alpha (0.4, 0.5, 1)). The swaying
  alga and the coral are vanilla plants.
* *Partly vanilla*: the **teal** cast of the floor. The material pushes towards blue-violet,
  not green; teal needs the floor colour's green kept against the red — plausible from sand
  colours times (0.5, 0.5, 1) plus the grading, but the strength seen suggests a colour
  adjustment (ReShade or the lighting mod). Treat a stronger teal as not vanilla.
* *Not vanilla*: the plant shadows on the floor (SteamShadows); vanilla has no caustics, so
  their absence matches.

---

## E. CubeVulkan today (`src/`, `shaders/`)

**Underwater test and palettes — follow the game:**

* `World::waterAt` (World.cpp ~119) / `ExtractedWorld::waterAt` (Extracted.cpp ~733): type 2,
  or air at z ≤ 0 without the dry flag — the game's rule (§1). Called with `camera.eye`
  (Renderer.cpp ~1152, ~1184), as the games test the camera.
* Alpha: `alpha::palette(…, underwater)` sets sky1 = sky2 = (0.1L, 0.15L, L) and keeps the fog
  (AlphaLook.cpp ~107); `alpha::material` gives (0.4, 0.5, 1, S) (~138); stars off
  (`starsVisible`). As §A.7.
* Steam: `steamsky::palette(…, underwater)` sets sky1 = fog = sky2 (SteamSky.cpp ~130); the sun
  glow from (0, 0, −1) (Renderer.cpp ~1193). As §B.5.

**Alpha look — mostly the game's, with differences:**

* `shaders/alpha_water.frag` reproduces §A.5 (body, glint with the (0, 0, 2) bias and power 10
  times the vertex light, haze, sky, opacity 0.8 + 0.0001·d²). **Invented**: the blue input is
  `mix(vShaded.b, vLit.b, sunShare(…))`, CubeVulkan's shadow maps darkening the water; vanilla
  has no shadows.
* Pipeline (Renderer.cpp ~533): blended, culling off, depth writes on — as §A.3; but the depth
  compare is "less", where the game uses "less or equal" (matters for a coplanar film, §A.1).
* Water vertex colour `alphalight::waterColour` (AlphaLight.cpp ~94) with top-face corner
  shading — as AR §9.5.
* **Meshing differs** (Extracted.cpp ~952–996; Terrain.cpp ~765 for the old terrain): only
  the **top** of each water stack, **lowered by 0.1 block** (sea at z = 0.9, rivers at top −
  0.1). **Invented**: the lowering. **Missing**: side and bottom faces towards air, the
  wet-ground film. Also **type 3 is treated as not solid** (`solidType`, Extracted.cpp ~21), so
  wet ground (river beds) is not meshed at all, where both games draw it as solid ground with a
  film on top when exposed.
* **Order differs**: CubeVulkan draws every opaque item (creatures and models included) before
  the water; Alpha draws creatures and far models after it, hidden under the surface (§A.6).

**Steam look — invented:**

* `shaders/water.frag` is CubeVulkan's own: time-animated ripple normals (`f.eye.w`), a
  Fresnel term, a sharp sun glint (power 220, warm white), a per-region water colour
  (`regionWater`, Terrain.cpp ~796, from CubeVulkan's palette), opacity from a "shallow" depth
  byte (0.55–0.82 plus Fresnel), and the shared `applyFog`/`vanillaGrade`. None of it is §B.4:
  vanilla uses lerp(sky1, sky2, …) × the lit blue, an exponent-1 sheen of 0.25·sky1, the Steam
  fog curve with the sun glow, 0.75 → 1 opacity by distance, then the grading.
* Pipeline (Renderer.cpp ~526): blended, culling off, **no depth writes and no pre-pass**;
  vanilla writes depth in a pre-pass and shades only the nearest water surface (§B.2).
* The water colour (0.25, 0.5, 0.5) and the +Z normal on all water faces (§B.1) are not used;
  the whole-zone sea quad is equivalent to CubeVulkan's per-column sea quads.
* **Missing**: the underwater material (0.5, 0.5, 1, 1) — no tint is applied in the Steam look
  under water.

**Lily pads — CubeVulkan's own:**

* Only the old terrain (`Terrain.cpp` ~703–736, `[WaterLilies]` keys PatchDensity,
  FlowerPercent, MaxPadsPerChunk, MaximumDepth, on by default) places `water-lily01/02` at the
  sea surface − 0.12, with a swaying `lily` flower model on top for some (that model is not in
  the game's model list; it must come from the user's files). `World.cpp` ~459 stops pads from
  casting shadows. The game-faithful generator (`src/worldgen/`) and extracted zones get **no
  lilies**.
* For the planned default-on option on the faithful generator: Steam (whose generator
  CubeVulkan reproduces) places none, so any lily is CubeVulkan's addition. Alpha's rule (§A.8:
  warm columns, |noise| > 0.7 patches, one in ten, pad 0.1 above the surface at a block centre,
  scale 0.09, random yaw, two pad models, no flowers, no sway) is a vanilla model to follow, but
  it must use its own random stream (not the generator's `rand()` sequence, which the oracle
  zones check) and run after generation so it does not disturb any draw. Flowers on pads, depth
  limits and density settings would be CubeVulkan's own.

---

## F. Oracles the user can record

The game is not to be run by the spec writer. For the user:

1. **Screenshots** (Steam and, if at hand, Alpha), level camera at noon, dusk and midnight:
   looking down at a river from 5 blocks above; along a river to the horizon; at a waterfall or
   a water edge against air (side faces); at exposed wet ground (the film); and from under water
   looking at the floor and up at the surface. Compare the body gradient against screen height,
   the glint shape and colour, and the opacity near and far.
2. **Alpha submerged creatures**: a fish or the player's pet under water seen from above — the
   order of §A.6 predicts they are not visible below the surface (Steam: visible).
3. **Steam lilies**: a ZoneExtractor dump of zones with rivers and lakes, listing zone-model
   types; §B.6 predicts no type 0x2c/0x2d.
4. **Constants**: the frame material under water (Steam (0.5, 0.5, 1, 1), Alpha (0.4, 0.5, 1,
   S)) and the water alpha constant (Steam renderer +0x4a8, expected 1) through the existing
   hooks.

---

## G. Open questions

1. Alpha's far-model threshold (the squared distance compared at 0x4b2767) and the key of the
   map drawn after the water were not traced; only that near models come before the water and
   far ones after.
2. Alpha's noise 0x4c0ef0 (used by the lily patches) is not specified; whether it equals
   Steam's 2-D noise 0x14002e0c0 was not checked.
3. The Steam sea-quad test (§B.1) was read from the decompile; which z the "light byte 255"
   check reads (z = 1 relative to the zone's sub-mesh) should be confirmed against a mesh dump.
4. Steam's second batched list drawn right after the water (0x1400e7980 on renderer +0x27×8,
   slots 3/4/6/7) was not identified; if it holds translucent models, they are drawn over the
   water with depth against the water's pre-pass.
