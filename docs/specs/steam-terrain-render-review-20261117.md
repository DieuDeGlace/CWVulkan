# Steam terrain and final frame reference
### FINAL / STABLE / HELD

Stock installed CubeWorld.exe and cubeworld.original.exe matched SHA-256
a1efbbd41eeb8df08395c4d6511194d416cb70a14054b2a2000b9f5cdbcfa65b.
The unpacked export image matched SHA-256 7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244.
RVA citations refer to that unpacked x64 image at preferred base 0x140000000.
Findings come from original instructions, data, localization and embedded shaders. Infinite-Cubes supplied navigation leads.

## Terrain inputs and location smoothing

- The block-coordinate biome sample uses a separate noise warp on the two horizontal axes, with offsets carried at
  65,536 units per block. It samples nine centers from the surrounding three by three grid of 8,192-block cells.
  Centers are independently displaced inside their cells. The sample retains all nine biome IDs and squared distances,
  plus the nearest distance and its biome ID. The biome assignment and center placement use coordinate-derived choices.
  See RVAs 0x358e0, 0x2b9210 and 0x2b9080.
- The direct height query resolves the special location on a floor-divided 1,024-block grid, including at negative
  coordinates. Its first center-distance check uses the unwarped point and accepts distances up to 282 blocks for
  ordinary locations or 532 for large ones. A selected location then uses independent noise offsets of amplitude 32
  per axis to measure its squared distance. The influence is the square of one minus that squared distance divided
  by the radius squared, with radius 250 or 500 respectively. Influence is zero at and beyond that warped radius.
  The descriptor carries type at byte 0, the large flag at byte 4, environment at byte 8, integer radius at byte 12,
  center X and Y at bytes 16 and 20, and 32-bit floats for squared distance and influence at bytes 24 and 28.
  See RVAs 0x35ce0, 0x364e0, and the floating-point stores at 0x35e1b and 0x35e5a.
- Zone generation prepares the selected location once at 0x31a5d0 and updates distance and weight per column at
  0x31a690. The column loop at 0x2ff6f0 samples biome and location, then obtains elevation, desert weight,
  temperature, humidity, mountain strength, Firelands and undead weights, enclave weight and road strength.
  It computes water, lava and poison channels before cliffs and sand, then combines height at 0x2fff9e.
  The separate height-query wrapper at 0x35ea0 also feeds 0x2b4450. For exposed surface selection the zone loop
  queries four cardinal neighbors through that wrapper at 0x300161, 0x300178, 0x30018e and 0x3001a5.
- The height combiner applies place effects above influence 0.0001. The attraction target comes from the biome
  elevation evaluation at the location center, with a 10-block minimum in this stage. The ordinary attraction
  uses the location weight directly on the difference from the local base. Types 8, 13, 14 and 24 instead use
  cubic smoothstep on twice the influence clamped to one, also reducing the hill contribution. The ordinary
  place branch attenuates roughness with that smoother and pulls hill amplitude toward 10 when it starts above 10.
  Type 10 has a flag-gated mountain rise. Types 18 and 19 have separate radial profiles. Types 17, 21, 28 and 36
  bypass ordinary attraction. The combiner also consumes independent channel, road, cliff and sand inputs.
  See RVAs 0x2b4450 and 0x2ba200. Later shape and structure passes have additional type-specific behavior.

## Dry land climate and surface color

- The original biome-name selector maps 10 to Savannah, 12 to Deadlands and 5 to DarkWoods. Deadlands is the
  English name for the biome at issue here. See RVA 0x16b210 and the original English dictionary.
- Savannah contributes to the hot group with Jungle, Desert and Firelands. Humidity uses a separate nearest Savannah
  weight alongside the dry group of Desert and Firelands and the wet group of Jungle and Wetlands. The regional
  weight for each group falls from one in proportion to the gap between its nearest squared distance and the
  overall nearest squared distance, using 0.0004 squared as the scale and clamping at zero. In humidity, Savannah
  subtracts 0.4 times its weight, dry biomes subtract twice theirs, and wet biomes add 1.5 times theirs.
  The baseline is 0.5 plus 0.35 times broad noise, with the final humidity clamped to zero through one.
  See RVAs 0x2d9b60 and 0x2b5960. The sand calculation also consumes a separate Desert and Firelands weight
  from 0x2a44a0, along with humidity and cliff strength at 0x2d36f0.
- The undead weight takes the nearest Deadlands or DarkWoods center and the same distance-gap falloff. Active
  location type 26 adds its local influence up to one. The ordinary water-channel branch requires Firelands
  and undead weights strictly below 0.5, with further place processing in that routine. The poison channel
  uses a separate noise field activated above undead weight 0.5 and modulated by mountains, location and roads.
  The flooded surface branch requires final water-channel weight above 0.5 and terrain height above minus five.
  See RVAs 0x2d9f30, 0x2e5f80, 0x2d01a0 and the zone branch at 0x2fffba.
- At RVA 0x2d8e00, lava above 0.5 has first priority, then poison above 0.5, then the ground or cover decision.
  Sand or road weight above 0.5, a beach condition, or burial below the floors of all five heights leads to the
  ground branch. The cover branch can blend in ground color based on cardinal differences beyond a one-block
  height change. The normal surface call at 0x3002c8 receives floor of the center height, the four neighbors,
  and the climate and channel weights explicitly. RVA 0xba820 halves each float RGB channel, clamps it to
  the byte range and truncates it when writing the block record.
- For ordinary cover, the base noise color at 0x2b40b0 receives a humidity adjustment at 0x2a3e30. Relative to
  humidity 0.5, dryness raises red, while higher humidity lowers red and raises blue. Above undead weight 0.001,
  the helper blends in a separate palette weighted by that undead strength. Its green is 128. Red adds the
  first noise term and subtracts the second from 128. Blue adds the larger of the two terms to 128. Each noise
  term is shifted by one and multiplied by 30. Their frequencies are 0.01 and 0.006, with offsets 4,343 and
  983 on the second sample. The enclave and Firelands blends follow, when active. This float palette precedes
  block write, lighting and final display. The camera-side renderer separately scales its daylight value by
  one minus 0.75 times the locally sampled undead weight at RVA 0xe2e00.

## Native ground effects and presentation

- The D3D11 frame at 0xe2e00 dispatches the ground-spot pass through renderer table 0x4385e8 to 0xc43d0 after
  water. Ground spots place colored textured quads above the sampled solid voxel top, lifted 0.0035 blocks.
  One 256 by 256 mask has white RGB and a cubic radial alpha falloff from one minus normalized squared radius,
  clamped at zero before cubing. The alternate mask has its own colored radial profile. Both are generated at
  0xc3150. The pixel shader at 0x43e250 multiplies a regular 2D texture sample by interpolated vertex color.
- The later call in 0xe2e00 dispatches to 0xc4b00. Nearby plant records of type 89 feed this pass at 0xd3170.
  It draws untextured two-triangle geometry oriented by camera yaw, with white RGB and vertex alpha fading
  toward the lower edge. Its upper alpha uses distance attenuation times 100 times the C runtime daylight-saving
  flag. The two passes use source-alpha blending, less-or-equal scene depth testing with writes disabled, and
  two-sided rasterization, as set at 0xc3150. The untextured shader branch returns the interpolated vertex color.
- The initialized world shaders at 0x4387b0 and 0x43a010 declare zero texture resources. The scoped receiver evidence
  is current scene depth and color compositing, with radial mask falloff or interpolated vertex alpha. The exact
  sampler filtering state of the textured effect and the appearance across all scene types remain open.
- RVA 0x134cc0 checks D3D11 multisample quality for the configured sample count, uses that count in swap-chain
  creation, and binds a back-buffer render target with depth. RVA 0x134420 clears current color and depth, invokes
  the frame through 0xa5a50, and presents that swap chain. The inspected path establishes MSAA and current-frame
  presentation. It supplies no evidence of a temporal color or depth accumulator or a shadow-map comparison filter.

The traced contracts cover the named Steam call sites. Whole-zone parity, runtime image comparisons, other render
paths, and cross-version Alpha or Omega behavior are outside the scope of this evidence.
