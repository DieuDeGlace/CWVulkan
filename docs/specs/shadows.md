# Shadows and anti-aliasing

What Cube World does about shadows and anti-aliasing, in the **Alpha** client (CubeVulkan's
default look) and the **Steam** release, and an audit of CubeVulkan's shadow maps and TAA,
which are CubeVulkan's own additions. Written from the Alpha decompile and assembly
(`disassembled alpha cube world`, addresses in `Cube.exe`), the Steam decompile (RVAs,
0x14xxxxxxx), both binaries' constants, and every shader blob of both executables disassembled
with `D3DCompiler_47`. Infinite-Cubes was not used. Conventions: `alpha-rendering.md` §0 ("AR").

---

## 1. Neither game has shadow mapping

* **No shadow map exists.** Alpha's six world shaders (AR §1.1) and Steam's world shaders (VS at
  file offset 0x436fb0; PS 0x438810, 0x4396a0, 0x439f90, 0x43ae90, 0x43b500) bind **no texture
  and no sampler** at all; the only textured shaders are the particle pair (Steam 0x43c100 /
  0x43ca50) and the interface (Plasma). No depth texture is created for lighting, no pass is
  drawn from the sun, no light matrix is uploaded.
* **No blob shadows.** Neither binary draws a dark disc or decal under creatures; the strings
  "shadow" in Steam are the armour set `shadow-chest1.cub`…`shadow-hand5.cub`, and Alpha has no
  such string.
* So there is **no technique, map size, cascade, range, bias, filter or softness to match**. A
  creature, a tree or a house casts nothing onto the ground in either game.

## 2. What darkens things instead (the vanilla "shadow")

Everything that reads as shade in Cube World is baked or per-object:

1. **Sky light, baked per block (AR §9.2).** Empty and water blocks get a light value: 255 in
   open sky above the column's first solid block, 0 below; then 16 sweeps where each takes
   85 % of its brightest neighbour (air counting at least 5). So the ground under a tree crown,
   an overhang, a roof or inside a cave is darker; this is the only "shadow" of a tree, and it
   is **straight down from the sky, not along the sun**: it does not move with the time of day
   and has no direction. Its edge is soft over a few blocks (each block 0.85 of its neighbour).
2. **Per-vertex light and occlusion (AR §9.3–§9.4).** Each corner averages the light of a
   6 × 6 window of blocks in front of its face (weights 5/6, 1/2, 1/6) into the vertex alpha, and
   the share of open blocks pulls the colour towards a darker, more saturated shade (ambient
   occlusion). Creases and corners are darker; the gradient spans about three blocks.
3. **The sun and sky term is scaled by that light (AR §5 step 6).** T = ambient + front · max(N·D, 0)
   + back · max(−N·D, 0), all × S (sunlight strength) × vertex light. A face under a canopy gets
   the sun's *colour* term reduced in proportion to its baked light, whatever the sun's angle.
   Faces turned away from the sun get only ambient and back ("form shading"), which is the
   look of self-shadowing on cubes.
4. **Creatures and models take the light where they stand (AR §11.2):** the material alpha is
   the light of the block 0.1 above the creature's position / 255, so a creature in a cave or
   under a roof is darkened as a whole. Models with flag 1 ignore it (`vegetation-wind.md` §4.5).
5. **Steam** keeps the same scheme with its own shader terms (AR §14): the wrapped front term
   max(0.7 N·D + 0.3, 0), back and ambient by a fixed-axis factor, and only sun + back scaled by
   vertex light × material alpha, so Steam's shaded areas keep their ambient (lighter caves and
   nights). Steam's mesher (its light flood and occlusion window) is not specified here.

An engine reproducing the vanilla look needs only these; they are already in `alpha-rendering.md`.

---

## 3. Anti-aliasing in the games (no TAA)

Neither game has temporal anti-aliasing, FXAA or any post pass on the world image (AR §13). The
only anti-aliasing is **hardware multisampling of the back buffer**, set from `options.cfg`
(`frontend-options.md` §1.2, items 4 and 20; neither is on the options screen):

* **Steam.** The swap chain (0x140134cc0) is created R8G8B8A8_UNORM (0x1c) with
  **SampleDesc.Count = `antiAliasingSamples`** (default **8**) and **Quality = the highest level
  the device reports** (CheckMultisampleQualityLevels(0x1c, samples) − 1); the depth buffer
  (D32_FLOAT, 0x28) is made with the same count on every resize (0x140135310). The value is taken
  from the options block at start-up (the static copy at 0x140551bf0.., samples at 0x140551c40)
  and again when the game's copy changes (0x140134420). **`antiAliasing` itself is never used
  as a switch**: it is only compared with its last value so that changing it triggers the
  swap-chain resize (0x140134420); MSAA is on with `antiAliasingSamples` samples regardless.
  `antiAliasingSamples` 1 turns it off. The game does not check the count against the device
  before use (an unsupported count makes the swap-chain creation fail).
* **Alpha.** The device is created without multisampling (0x4c8720) and reset (0x4c8940)
  whenever the window mode, size or `antiAliasing` changes (0x4c85f0 compares the game's options
  copy +0x5c..+0x5f with the last applied). The reset sets **MultiSampleType = 2 ×
  `antiAliasing`** (0 = none; 1 → 2 samples, 2 → 4, 4 → 8), MultiSampleQuality 0, back buffer
  A8R8G8B8 (0x15), depth D24S8 (0x4b), presentation immediate. Alpha's options file has no
  `antiAliasingSamples`.

So the vanilla edge quality is plain MSAA: polygon edges smoothed, interiors untouched, no
blur, no temporal ghosting, no jitter. CubeVulkan's TAA and SMAA are its own additions.

---

## 4. CubeVulkan today: shadow maps (an addition)

`src/Renderer.cpp`, `src/Renderer.h`, `shaders/lighting.glsl`, `shaders/shadow.vert`,
`shaders/alpha_scene.*`. The code says it ports the user's own MoonandShadow / SteamShadows mods.
Defaults: `[Shadows] Enabled=1` (config/CubeVulkan.ini), far map 4096² over a radius of 120
blocks, near map 2048² over 50 blocks (`Cascades=1`), D32_SFLOAT, light box depth ±420 blocks,
strength 0.9, fade from 20 % of the radius; one hardware PCF tap (LINEAR compare sampler);
receiver-plane depth bias plus a constant 0.008 blocks; the Alpha look removes only the sun's
direct term in shadow, and CubeVulkan's moon casts at night.

**Parity note.** In the Alpha look these shadows are on by default, but Alpha has none (§1).
Under the project's vanilla-parity rule they belong behind an option that is off in the vanilla
look, the way the moon is (`[Render] Moon`). When on, they should only *add* sun-directional
shade on top of §2 and never darken what the baked light already darkened twice: the Alpha look
already multiplies the sun term by the vertex light, so an area under a crown is dimmed once by
§2 and again by the map. Scaling the shadow's strength by the vertex light (or by S × light)
avoids the double darkening.

**Likely problems, by kind** (from reading; none was measured):

* **Shimmer / crawl.**
  1. The light direction changes every frame (the sun moves with the clock; at
     `MinutesPerSecond=1` about 0.25° a second), so both maps are re-rasterised from a rotated
     view each frame; `lightMatrix` snaps the *centre* to whole texels, which only stops crawl
     under translation, not rotation. Shadow edges therefore crawl continuously. Remedy: update
     the shadow direction in steps (e.g. every 0.25–0.5° or every few seconds) and cross-fade, or
     snap the direction to a fixed angular grid.
  2. Swaying leaves and plants are drawn into the maps (`shadow.vert` calls `sway`), so every
     canopy shadow ripples every frame; with one PCF tap at ~0.05 blocks per texel this reads as
     sparkle. Vanilla has neither the sway of blocks (`vegetation-wind.md` §1) nor shadows.
  3. Small props cast only within `min(radius, SceneryDistance)` of the focus
     (Renderer.cpp ~1348): their shadows pop at that ring.
  4. The maps are centred on the focus (the player), the fade on the camera (`CameraFade`):
     orbiting the camera moves the fade ring over a static map.
* **Acne.** The base bias is 0.008 blocks, scaled by 1 + 2(1 − N·L); with the far map at
  240 / 4096 ≈ 0.059 blocks per texel that is far below one texel, so the receiver-plane term
  carries all the work. Where its derivative test fails (|det| ≤ 1e-12: tiny or degenerate
  screen footprints, e.g. distant faces) it falls back to `worldTexel × min(slope, 8)`, which is
  close to zero on faces facing the sun. Flat-lit tops at a distance are the likely place for
  acne bands. A bias of half to one texel of the map in use (≈ 0.03–0.06 blocks) as the floor is
  safer for axis-aligned voxel faces.
* **Peter-panning / light leaks.** The slope-scaled part is capped at 16 texels × 1.1
  (≈ 1 block on the far map) and grows at grazing light, i.e. mornings and evenings: shadows lift
  away from the foot of walls and trunks by up to a block, leaving a lit gap where a wall meets
  the ground. Also `exposure = smoothstep(0.15, 0.30, N·L)` forces faces at N·L < 0.15 to "fully
  shadowed": vertical faces near sunrise flip between lit and shaded as the sun crosses that band.
  For cubes, a normal offset (moving the lookup along the face normal by ~0.5–1 texel) is a better
  bias than a large depth slope.
* **Cascades that do little.** Near 50 blocks / 2048 ≈ 0.049 blocks per texel vs far
  ≈ 0.059: the near map is only 20 % sharper, at the cost of a second pass and a blend band
  (0.8–1.0 of the near radius) where two slightly different results are mixed. Either make the
  near map clearly finer (e.g. 25 blocks at 2048) or drop it.
* **Softness.** One bilinear compare tap gives a hard, stair-stepped edge one texel wide. The
  vanilla shade (§2) is soft over a few blocks. If the addition is to sit with the Alpha look, a
  wider filter (a 3 × 3 or Poisson PCF over ~0.25 block, or PCSS-like widening with distance to
  the caster) matches it better than sharpness.

## 5. CubeVulkan today: TAA and SMAA (additions)

`shaders/taa.frag` (ported, it says, from the user's TemporalAA mod), `src/RendererPost.cpp`,
SMAA passes. Defaults: `[AntiAliasing] TAA=1, SMAA=1`, history 0.65, edge history 0.90, resolve
blur 0.12, jitter 0.75 px over an 8-frame Halton sequence.

**Parity note.** Vanilla is MSAA (§3). A vanilla-faithful option is multisampling the scene with
`antiAliasingSamples` (Steam default 8) and no temporal pass; TAA stays a CubeVulkan option.

**Likely problems** (from reading):

* **Ghosting on anything that moves by itself.** Reprojection uses camera motion only
  (`clipToPrevious` from the depth of the current pixel): there are no motion vectors for
  creatures, the eagle, projectiles, water, swaying leaves and plants (vertex animation), cloud
  pop-in or the shadow maps' changes. Those pixels reproject to where the surface *was*, and only
  the neighbourhood clip saves them. With edge history 0.90, edges of moving creatures and swaying
  foliage trail. Remedy: write per-object (and per-vertex for sway) motion vectors, or lower the
  history where the depth test or a velocity flag says "moving".
* **Blur.** The current frame is sampled at `(gl_FragCoord + jitter) / size` with bilinear
  filtering, i.e. between texels every frame, plus a 0.12 cross blur, plus Catmull-Rom history:
  textures-free voxel faces lose their crisp single-colour edges (vanilla MSAA keeps interiors
  exact). Sampling the current frame with `texelFetch` at the pixel and removing the jitter by
  reprojection instead keeps faces sharp.
* **Depth test in NDC.** `sameSurface` compares non-linear NDC depths with a tolerance plus 4 ×
  the depth derivatives; far away (most of NDC's range is near 1) unrelated surfaces pass, near
  the camera small moves fail. Linearising both depths before the comparison makes the test
  uniform.
* **Sky pixels always accepted** (`d ≥ 0.99999`): the sun sprite, stars and the moon move across
  the sky with the clock and the camera; accepted history smears them (stars are a few pixels).
* **Interaction with the shadow maps.** Shadows that crawl or ripple (§4) change shading under a
  static camera; TAA cannot reproject that and turns the sparkle into a slow shimmer.
* **History reset only on toggle or a jump of more than 8 blocks**: a cut on the title (Continue,
  a camera snap) of under 8 blocks keeps a stale history for a few frames.

## 6. Verification

For the user (the spec writer does not run the game):

1. **No shadows, MSAA only (Steam)**: a screenshot at noon and at 18:00 of a tree in open ground,
   1920 × 1080, `antiAliasingSamples` 8 and 1: no cast shadow at either hour; edge smoothing only
   with 8. A RenderDoc/PIX capture of one frame lists the draws and render targets: no depth-only
   light pass, swap chain sample count = the option.
2. **Alpha**: the same screenshots with `antiAliasing` 0 and 2 (2 → 4× MSAA).
3. **Sky-light shade**: a screenshot under a large crown at 09:00 and 15:00: the dark patch is the
   same (straight under the crown) at both hours.

## 7. Open questions

1. Steam's mesher (its version of AR §9.2–§9.4) was not traced; the shade under objects in the
   Steam look is assumed to come from the same flood and window.
2. What Steam does when the device rejects `antiAliasingSamples` (no fallback was seen in
   0x140134cc0).
3. Alpha's default `antiAliasing` value (its options defaults in 0x4ce6e0 were not read).
