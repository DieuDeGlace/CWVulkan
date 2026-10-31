# Shared surface and underwater style target

Water appearance is to be selected independently of `[Render] Style`, so switching
between Alpha and Steam lighting retains the same chosen water treatment. The
requested Omega-inspired surface is clear blue water with visible shallow geometry,
crisp reflections of the voxel banks and small highlights. The underwater target
uses deep blue distance haze, teal foreground tones and strong scene silhouettes.
These are visual targets for original CubeVulkan shaders, not a port of Omega code.

The owner's local `water lillies.PNG` and `underwater shader.PNG` are the references.
They remain outside the repository and distribution packages. The older Alpha-water
pictures do not supersede this choice.

## Shared style implementation — 2026-10-30

`[Water] Style=Omega` is the default independently of the Alpha/Steam lighting
selection. `Style=Native` retains the two original water shaders and their draw
behavior for parity comparisons. Changing this setting requires a restart.

Alpha's partially faded scenery is blended after the sky and before the shared
color/depth snapshot. Near scenery stays in its earlier pass. This prevents a
transparent model's depth from blocking an undrawn sky background, while keeping
the scenery available to refraction. Native water retains its separate after-water
far queue. The same mixed sort, CPU opacity and depth writes are preserved; Vulkan
regressions cover partial fades and zero effective alpha without adding a discard.

- The shared surface reads immutable color/depth snapshots of the opaque world.
  Shallow terrain remains visible through depth-dependent blue absorption and
  scattering. A small normal ripple and broad sunlight highlights leave the voxel
  surface geometry intact.
- Screen-space reflection rays sample visible banks, models and actors, with a sky
  fallback outside the available scene. Reflection steps are configurable from 8
  to 48 (default 24); reflections can be disabled without changing the world.
- Submerged views use the same cobalt haze and teal scene treatment in either
  lighting mode. The shared color treatment acts on world fragments; interface
  art, fonts and the map overlay retain their own colors. The default underwater
  visibility limit is 80 blocks, configurable from 8 to 256.
- Opaque actors are drawn before the shared water so refraction can include them.
  Native Alpha ordering remains available under the Native water option. Its
  separately specified static-scenery split now keeps near records before water
  and sorts one mixed far model/object queue after water, before creatures. Both
  paths preserve the scenery fade; shared refraction needs its opaque background
  prepared first.
- Snapshots are copied outside dynamic rendering with explicit image barriers.
  Color/depth targets are recreated with the window; frame uniforms follow their
  own completion fences. No live render attachment is sampled by the water pass.

The implementation is original CubeVulkan code guided by the two owner-supplied
images. No Omega source, textures or shader binaries are included. The photos stay
outside source packages. Meshes, object placement, generation and random streams
are unchanged.

Verification includes actual Vulkan image checks with Alpha, Steam legacy vertices
and Steam native packed vertices: changed submerged terrain remains visible through
the surface; adding a red bank changes its reflected image; disabling reflections
changes the result; green foreground terrain receives a teal cast; distant water
stays blue; a white HUD fixture stays white; and a resize recreates usable targets.
`tools/validate_engine.ps1` runs these alongside the native shader regressions.

This is a reference-guided implementation, not a certified pixel match. Reflections
cannot show off-screen geometry, and intersecting non-opaque models still need wider
comparisons. Waterfalls, wet-ground films, dusk/night and underwater scene coverage
remain on the rendering checklist.

---

# Native-option water baseline — 2026-10-28

Implementation uses the existing behavioural specification, `specs/water-rendering.md`, and CubeVulkan sources. No decompile, native shader blob, or third-party reconstruction was opened during this pass. The specification's section E describes the earlier implementation; the changes below supersede those particular findings.

Completed:

- Steam water uses the screen-height palette gradient, broad reflected-light sheen, specified fog curve/sun glow, distance opacity and final grading. Animated ripple normals, Fresnel/depth colour and sharp white glints were removed.
- Steam water has a colour-masked depth pre-pass, then a blended pass with less-or-equal depth testing. Alpha water also uses less-or-equal, permitting coplanar wet-ground film.
- Extracted/generated zone meshing emits exposed water faces in all six directions, at full block height. Wet ground is solid and gets water film when exposed to air. Sea water follows the existing dry-air rule. Steam water vertices use the doubled native colour and +Z normals; Alpha faces retain their individual normals and baked light.
- Steam's underwater material tint is applied to the ordinary surface shader. Existing underwater palette handling remains.
- Default-on render-side lilies use the owner's stock models, a separate stateless hash, warm shallow fresh-water checks and existing scenery lighting. Generator code, blocks and random streams are unchanged. Imported settings and the WaterLilies section control density, depth and the pad limit.
- Alpha's creatures/dynamic models draw after the depth-writing water; static scenery retains its existing position in the draw order.

Verification:

- Warning-free final build and full `EngineTests` run: all passed, including existing world-generation comparisons.
- New synthetic mesh regressions check a flat sea at its exact height, six exposed faces of suspended water, wet-ground solidity and film, and style-specific normals/colour.
- Lily regressions check determinism, disabled density, limits, rejected locations, model names, scale and height. The maximum-HP regression checks that initialization caps legacy health while subsequent frames preserve wounds.
- RX6600 Vulkan captures in isolated executable/configuration folders: `build/codex-water-2026-10-28/steam/ocean.png` and `alpha/ocean.png`. Both completed normally with saves disabled; images inspected. These are smoke checks, not reference-match certification.

Still needed:

- The independently specified Alpha static-model/object split, mixed far queue and
  fades are implemented with CPU threshold and GPU retained-depth regressions.
  Complete prior admission/cache traversal and native fixed-camera state still need work.
- Complete Steam vertex occlusion/point-light parity; this pass uses CubeVulkan's existing shared light/shadow implementation.
- Wider reference comparisons at waterfalls, wet-ground films, dusk/night and underwater, including non-opaque model materials.

The HP-above-maximum issue was traced to legacy startup health supplied before the native record's maximum was established; the client mirror skips the host's upkeep cap. Initialization now respects native maximum health after level/equipment, and equipment changes cap excess health. Later frames retain wounds. Both updated isolated warrior captures show 136/136 instead of 169/136 (`steam/hp-water.png`, `alpha/hp-water.png` in the same private capture folder).

The Telgor statue comparison is complete: 81 columns around the owner's goddess2 placement were compared against the extracted zone. All 477 stored blocks within z=19..25 match, including the seven-block slab above the statue. No generator change was made. The private comparison report is `build/codex-water-2026-10-28/statue-columns.txt`.
