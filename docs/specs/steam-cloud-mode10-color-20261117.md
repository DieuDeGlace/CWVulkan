# Steam mode-10 cloud color

## Supported image and binding

The installed x64 Steam CubeWorld.exe has SHA-256 `a1efbbd41eeb8df08395c4d6511194d416cb70a14054b2a2000b9f5cdbcfa65b`. The matching unpacked analysis image has SHA-256 `7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244` and preferred image base `0x140000000`.

The cloud draw at `0xe5730` selects mode 10 at `0xe57e0`. The D3D selector at `0xc3dc0` calls `0xc5e50`, which binds the shared vertex shader from blob RVA `0x4387b0` and the cloud pixel shader from blob RVA `0x43cd00`. The objects are created at `0xc60d0`, and their constant buffers are bound at `0xc68c0`. Shader instruction offsets below are relative to the start of the respective instruction streams at image RVAs `0x438e08` and `0x43d0fc`.

**Fog-weight refinement for the held view-distance contract.** The pixel shader's two complement operations include a square of the intermediate complement. For t equal to the saturated squared-distance term, the final fog weight is 1 minus the square of (1 minus t cubed). The exact operations appear at pixel instruction offsets `0x5d8–0x644`.

## Vertex inputs and position

The mode uses the two packed `R8G8B8A8_UNORM` elements of the original eight-byte vertex. Position is the first four bytes and color is the next four. The model packer at `0xc3a10` writes low bytes of the integer position components. The fourth position byte encodes the face normal. Values 0, 1, 2, 3, and 4 select +X, −X, +Y, −Y, and +Z respectively. The remaining values select −Z. The shader compares against the normalized forms of those byte codes at vertex offsets `0x114–0x224`.

For the matrix description, treat each float4 read from a constant buffer as a row. Vertex constant buffers 0, 1, and 2 contain the projection, view, and world matrices, respectively. The original setters transpose from the native memory layout at `0xc7240`, `0xc78b0`, and `0xc7ab0`. Multiply normalized position XYZ by 255 and take homogeneous W as 1. The shader composes view times world, then projection times that combined matrix. Each four-term composition starts with the Y term and adds the X, Z, and W terms in that order. Before the clip dot products, it subtracts the scalar in vertex constant buffer 8 from the W coefficient of the combined clip Z row. The view-space position for fog is the first three row products with the view-times-world matrix. The depth-offset subtraction affects clip Z. It uses the shared depth state at `0xc6a80`. The frame sets zero at `0xe39f4` and earlier render-list draws can supply another value. Mode 10 consumes the last value in that shared constant buffer.

The shader passes clip XYZW as both position and texture coordinate 0, and view-space XYZ as texture coordinate 3. It also emits the normalized view-times-world face normal and the light direction transformed by the first three linear rows of view times world. The pixel shader reads only lit RGB, clip X/Y/W, and view-space XYZ from the interpolated varyings. The latter input is declared and used at pixel offsets `0x70` and `0x57c`. Interpolation for those inputs is the shader's ordinary perspective-correct linear interpolation.

## Original model color and vertex lighting

Exposed face corners are baked by `0x170700` using the exposure from `0x171db0`. The model light calculation at `0x170530` raises exposure to 1.2, adds model RGB additions to the voxel RGB in normalized-byte units, and scales that RGB by the exponentiated exposure. It forms luminance with weights (0.30, 0.59, 0.11). When luminance exceeds one, it divides the RGB by luminance and uses one for gray. It mixes gray toward the scaled RGB with weight 3 minus twice the exponentiated exposure and clamps RGB. The alpha at this step is 2. Model byte `+0x9a` selects full RGBA when set and half RGBA when clear. Packing truncates each RGBA component multiplied by 255 and retains the low byte. The full alpha thus becomes 254 and the halved alpha becomes 255. A fully exposed bright cloud corner with original recolor (100, 130, 180) packs to (100, 130, 180, 254). A fully exposed dark example with source voxel (90, 140, 190), additions (10, 0, 20), and half RGBA packs to (50, 70, 105, 255). These are synthetic corner examples evaluated with the original arithmetic.

Cloud setup sends material RGBA (1, 1, 1, 1) to vertex constant buffer 3, clears point lights, and sets the white override and glow (vertex constant buffers 7 and 9) to zero. The relevant state writers are `0xc6f20`, `0xce580` through `0xc7010`, `0xc7a20`, and `0xc6c50`, reached at `0xe57e3–0xe5821). The frame's light state at `0xe3cc8` supplies front RGB from renderer `+0x50c`, back RGB from `+0x51c`, and direction to vertex constant buffer 5 via `0xc6ce0`. The same call chain supplies ambient RGBA from renderer `+0x4fc` to vertex constant buffer 6 via `0xc6820`.

1. Use the first three linear rows of the world transform on the face normal and normalize the result with reciprocal square root. The front shape is the greater of zero and 0.3 plus 0.7 times the dot of that normal with the supplied light direction. The back and ambient shape is 0.5 plus 0.5 times the absolute dot of the normal with the fixed direction (0.597022295, 0.398014873, 0.696526051), whose exact float bits are (3f18d674, 3ecbc89b, 3f324f88). Multiply back RGB by the second shape, add front RGB times the front shape, and clamp that result to zero through one.
2. Multiply the resulting front/back RGB by packed vertex alpha times material alpha. Add ambient RGB times the back and ambient shape. Point lighting, if admitted by the shared vertex shader, also enters at this point. Clamp this final lighting sum to zero through one per channel.
3. Double the packed vertex RGB before calculating its luminance. For this and the pixel luminance operations below, the common weights are (0.299, 0.587, 0.114). The vertex color saturation weight is 1 plus half the difference between ambient alpha and lit green, with ambient alpha first. Clamp that weight to one through two. Mix the doubled RGB's luminance toward doubled RGB with that weight and clamp each resulting channel to zero through two. Multiply lighting by material RGB first, then multiply by this shaped vertex RGB.
4. The shared shader uses pure doubled-color luminance in place of the shaped vertex RGB if glow is negative. For positive glow it mixes the resulting lit material color toward the doubled RGB times material RGB with glow as weight. It then mixes the result toward white by the white override and clamps to zero through one. On cloud draws the zero values for both overrides leave the normal lighting and saturation calculation active. The vertex shader passes the packed color's normalized alpha separately from its lit RGB.

The common point-light buffer has ten position/radius vectors followed by ten color vectors. `0xc7010` uploads the first ten entries of each supplied array. The shader admits a slot only for radius strictly above zero. For an admitted slot, use the displacement from the light to the world-space vertex. The radial weight is the square of one minus the lesser of one and squared displacement divided by squared radius. The directional weight is minus four times the dot of the normalized displacement with the world face normal, clamped to 0.3 through one. Multiply radial weight, directional weight, and light RGB, then add contributions in increasing slot order. The cloud clear supplies zero radii to every slot, so its vertices receive the ambient and front/back terms and original baked face color. Vertex offsets `0x818–0x10c4` cover this arithmetic.

## Pixel constants supplied by the frame

| Pixel constant buffer | Values used by mode 10 | Original producer |
| --- | --- | --- |
| 0 | RGB of SkyColor1, SkyColor2, FogColor, and SunColor at byte offsets 0, 16, 32, and 48 | Frame dispatch at `0xe3a12` through `0xc7440`, from renderer `+0x4ac/+0x4bc/+0x4cc/+0x4dc` |
| 1 | FogScale in X | Frame dispatch at `0xe3a40` through `0xc6bb0`, positive reciprocal of renderer range `+0x4a4` in ordinary media |
| 2 | FogGradientTranslation in X | Camera pitch in degrees converted to float from game `+0x1d80` via `0x295d00`, then (pitch minus 80) divided by −30 at `0xc6b10` |
| 3 | SunPos, with projected X, shifted Y, viewport width over height, and depth admission | Frame dispatch at `0xe3aac` through `0xc7650` |
| 4 | Draw opacity in X | `0xc6790`, set by the cloud distance passes |

The ordinary Sun direction used by the frame at `0xe38b4–0xe3961` comes from world float `+0xc`. Double the world value in single precision, multiply by double-precision pi and divide by 86,400,000 in double precision, then convert the phase to float. Form (sine of phase, −0.4 times sine of half phase, negative cosine of phase) and normalize it. To supply SunPos, scale the direction by 100, apply the view's linear part, and project with homogeneous one. The CPU takes reciprocal clip W and multiplies the clip components by it. SunPos Y subtracts (pitch minus 80) divided by 30 from projected Y. The depth flag is 1 for projected Z in the inclusive interval zero through one, and 0 otherwise. Camera medium class 2 supplies direction (0, 0, −1) for this projection and passes the lesser of 150 and the live range into the fog setter.

## Mode-10 pixel arithmetic

Here clamp means saturation to the inclusive interval zero through one. Each cubic grade below forms the square first, doubles square times the original value, and subtracts that quantity from three times the square. The shader uses multiply-add for the weighted sums and final subtraction.

1. Divide interpolated clip X and Y by interpolated clip W. Add FogGradientTranslation to the resulting Y. Use this shifted Y for both the gradient and the Sun's vertical displacement. Clamp 0.4 times shifted Y plus 0.2, apply the cubic grade to this weight, and use it to mix SkyColor1 toward SkyColor2.
2. Form horizon RGB by multiplying FogColor by 0.6 and adding SkyColor1 times 0.4. Clamp 2 times shifted Y plus 1.5 and grade this weight with the same cubic. Mix horizon RGB toward the broad gradient RGB using the graded weight. For each color mix, form the destination-minus-source difference before weighting and adding to the source.
3. Subtract projected fragment X from SunPos X and multiply the difference by SunPos Z (the aspect ratio). Subtract shifted fragment Y from SunPos Y. Square and sum the two displacements, multiply by four, and cap at one. Subtract from one and multiply by SunPos W. Square this result, then subtract its square from one. Use that last weight to mix SunColor toward the directional sky RGB from step 2. Thus the Sun contribution has a fourth-power radial fade within the admitted projected disk. This produces the directional fog target. Pixel offsets `0x90–0x42c` cover the sky and Sun operations.
4. For each interpolated lit RGB channel, compute the reciprocal of the reciprocal square root. Take luminance of the resulting three channels. The weight for the next color mix is 1 plus 0.25 times (1 minus that luminance). Form luminance times (1 minus weight) first and add weight times each of the root RGB channels. The result can exceed one before the later fog clamp. Pixel offsets `0x454–0x55c` cover this treatment of lit color.
5. Dot the interpolated view-space XYZ with itself, square FogScale separately, multiply the two quantities, and clamp to obtain t. Form t squared, then the intermediate 1 minus t squared times t. The fog weight is 1 minus that intermediate squared. Use it to mix the processed vertex color toward the directional fog target, saturating the resulting RGB. Pixel offsets `0x57c–0x644` cover this blend.
6. Take luminance of the saturated fog blend and multiply it by 0.35. Leave red at its fog-blend value. For green and blue add the shared 0.35 luminance term to 0.65 times the channel's fog-blend value. Apply the cubic grade separately to those three resulting channels. Write the float in pixel constant buffer 4 unchanged as output alpha. Pixel offsets `0x668–0x764` cover the final grade and alpha.

The shader luminance weights have float bits (3e991687, 3f1645a2, 3de978d5). The other noninteger shader literals used here are 0.2 (3e4ccccd), 0.25 (3e800000), 0.3 (3e99999a), 0.35 (3eb33333), 0.4 (3ecccccd), 0.5 (3f000000), 0.6 (3f19999a), 0.65 (3f266666), 0.7 (3f333333), and 1.5 (3fc00000). The mode-10 draw uses source-alpha RGB blending with a depth-writing less-or-equal test. A queued zero-alpha draw uses the same shader RGB and writes depth.

## Other embedded pixel shaders

The frame's general scene binder at `0xc5de0` uses `0x43a010`. It has a specularity term and extra distance shaping, and its input RGB treatment differs from the cloud shader's reciprocal-of-reciprocal-square-root and luminance expansion. The mode-9 binder at `0xc5ff0` uses `0x43b790`, whose alpha is multiplied by a distance term and whose inputs to fog differ. The mode-8 binder at `0xc5f80` uses `0x43aea0`, which grades sky color and supplies alpha one. The blob at `0x43c690` uses a separate sky-color mix followed by channel scale and offset.

## Worked reference vectors

These are shader input and output values before render-target conversion or blending. Decimal inputs are read as binary32. The numbers were calculated from the embedded instruction streams and independently from the described arithmetic. The reference rounds each operation to binary32, with one rounding for each multiply-add and dot product and a rounded reciprocal-square-root result. Separate rounded multiplication before addition changes some displayed low bits. Hardware-specific rounding and rasterization are outside these static references.

All three cases use SkyColor1 (0.34, 0.44, 0.68), SkyColor2 (0.10, 0.27, 0.56), FogColor (0.66, 0.57, 0.48), SunColor (1, 0.72, 0.39), and pitch shift 0.125. Cases A and B use SunPos (0.27, −0.21, 1.6, 1). Case C uses (0.27, −0.75, 1.6, 0), which disables a Sun contribution even though the projected point is within its disk.

Case A includes the vertex stage. The position bytes are (8, 16, 9, 0), and the color bytes are the fully exposed bright example (100, 130, 180, 254). The uploaded world matrix rows are (0, −2, 0, 58), (2, 0, 0, −47), (0, 0, 2, 84), and (0, 0, 0, 1). The view rows are (1, 0, 0, 10), (0, 0, 1, −128), (0, −1, 0, 72), and (0, 0, 0, 1). The projection rows are (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, −1), and (0, 0, 1, 0). Material is all ones. The point lights, white override, depth offset, and glow are zero. Use front RGB (0.72, 0.55, 0.42), back RGB (0.16, 0.21, 0.30), direction (0, 0.60, 0.80), and ambient RGBA (0.24, 0.30, 0.41, 1).

Case A's world point is (26, −31, 102), world face normal is (0, 1, 0), and clip and view vectors appear below. The front and back/ambient factors are 0.72000003 and 0.69900745. Its clamped lighting is (0.79553145, 0.75036526, 0.79668707), the vertex saturation weight is 1.12481737, and the lit RGB is (0.60312909, 0.76748043, 1). The pixel evaluation uses those full binary32 vertex results. Cases B and C supply the pixel varyings directly.

| Case | View-space XYZ | Clip XYZW | Lit RGB input | FogScale bits | Draw alpha |
| --- | --- | --- | --- | --- | ---: |
| A | (36, −26, 103) | (36, −26, 102, 103) | Vertex result above | 3bde9bd4 | 0.35 |
| B | (−380, −410, 620) | (−380, −410, 619, 620) | (0.80, 0.38, 0.17) | 3a8f377f | 1 |
| C | (240, −1000, 1100) | (240, −1000, 1099, 1100) | (0.27, 0.61, 0.39) | 3a8f377f | 1 |

| Case | Processed vertex RGB | Directional fog target | Squared-distance t | Fog weight | Mode-10 output RGBA |
| --- | --- | --- | ---: | ---: | --- |
| A | (0.77368897, 0.87660402, 1.00486827) | (0.78398389, 0.62704629, 0.48058259) | 0.58063018 | 0.35317981 | (0.87332976, 0.88477075, 0.90435278, 0.35000000) |
| B | (0.91208071, 0.61159813, 0.39094740) | (0.45675641, 0.48743230, 0.60702723) | 0.83202893 | 0.82021588 | (0.55780953, 0.52265334, 0.57932454, 1) |
| C | (0.50659043, 0.78858495, 0.61973417) | (0.53200001, 0.51800001, 0.56000000) | 1 | 1 | (0.54793447, 0.53169239, 0.57243395, 1) |

Case A with draw alpha zero has the same RGB and output alpha zero. The output RGB float bits for A, B, and C in the stated arithmetic are (3f5f928a, 3f628056, 3f6783aa), (3f0ecc9b, 3f05cc9c, 3f144e9d), and (3f0c456f, 3f081cfe, 3f128b08), respectively.

The frame's broader environment-color generation, the packaged voxel occupancy, and the GPU's exact output for a specific native render target are outside this follow-up.
