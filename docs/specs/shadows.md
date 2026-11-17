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

The dated Steam trace confirms current-frame multisample presentation in the
reviewed route. It also identifies projected ground spots and translucent
beams composed against ordinary scene depth.
[Held Steam terrain and frame trace](steam-terrain-render-review-20261117.md).

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

`Renderer.cpp` builds two directional depth maps when shadows are active.
The default spans are 240 blocks at 4096² and 50 blocks at 2048². Both use
the light's depth range of ±420 blocks. The light direction and shadow
strength use two-game-minute steps in both looks. The map centres snap to
whole light-space texels. Casters use the same model and optional foliage
motion as the world draw while the maps are refreshed.

`shaders/lighting.glsl` samples a 3 × 3 grid of bilinear depth comparisons.
It offsets the lookup by 0.75 texel along the face normal, floors the basic
depth bias at half a texel, and caps the additional slope bias at two texels.
The near and far results blend over the last 20% of the near radius. The
Alpha and Steam shader paths attenuate the direct light while preserving
the existing baked or indirect contribution. `[Shadows] Enabled` controls
the enhancement.

On the tested Radeon, the fixed-camera scene produced a visible cast shadow
with either look. Repeated frames and a subtexel focus move had identical
pixels. This covers a fixed light and static geometry. Timing at light-step
and cascade crossings and moving foliage still needs broader motion checks.

## 5. CubeVulkan today: TAA and SMAA (additions)

`[AntiAliasing]` defaults enable both effects. The renderer uses an eight-frame
Halton sequence at a half-pixel jitter scale. The TAA defaults are 85% base
history, an 80% edge setting and 12% resolve blur. The high-edge weight uses
the greater of the base and edge settings. SMAA follows the temporal resolve. The interface
draw follows both passes.

The TAA shader reprojects camera motion from the nearest depth in a small
cross. It compares the current and previous sky classification before
blending. Geometry also has to pass the existing projected depth check.
Unavailable or rejected history returns the current filtered sample. When
TAA is disabled, that path returns the single current sample. Target changes,
toggles, title cuts and large camera jumps reset the available history.

A Vulkan regression covers a newly visible sky pixel beside a silhouette.
The corrected frame matches its history-reset reference. Moving objects still
share the camera-only reprojection, which limits what fixed-camera coverage
establishes about animated creatures, water and foliage.

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
