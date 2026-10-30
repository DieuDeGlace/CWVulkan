# Steam world shader output, interpolation and fog

Independent clean-room behavioral specification from the owner's compiled
Steam shader blobs and native constant setters. No engine, test or exporter
sources were read, no original game was executed, and no native shader source
or instruction stream is reproduced. Addresses are RVAs. This completes the
output side of `steam-voxel-vertex-inputs.md` and
`worldgen-parity-gaps.md` §9.3; palette inputs come from
`steam-scene-lighting.md` and `sky-and-clouds.md`.

The ordinary world pair is **vertex shader0x4387b0**, length0x1860, and
**pixel shader0x43a010**, length0xe88. Creation is **0xc60d0** and binding is
**0xc5de0**, including the constant-buffer bindings in **0xc68c0**. The pixel
shader has no texture/sampler input, texture lookup, discard or explicit
depth output. Its result is one RGBA render-target output.

## 1. Vertex outputs and raster interpolation

Input decoding, CPU-packed RGB/V and per-vertex lighting remain as specified
in the earlier documents. Let the reconstructed local position be transformed
by world, view and projection to obtain native homogeneous clip coordinates.
The vertex stage subtracts the scalar **DepthOffset from clip Z**, leaving
clip X/Y/W unchanged. This is a clip-coordinate offset, not a world-Z change
or an offset applied after the perspective divide.
The direct scalar setter is **0xc59c0 → 0xc6a80**, vertex constant buffer8;
it supplies the value without a new clamp.

| Output semantic | Vertex value | Ordinary world pixel use |
| --- | --- | --- |
| SV_POSITION | Adjusted homogeneous clip XYZW | Rasterization/depth; no explicit pixel-stage read |
| COLOR0 RGB | Per-vertex lit/Glow-selected RGB after White and clamp | Interpolated base color |
| COLOR0 alpha | Input normalized vertex V | Not read by this pixel shader |
| TEXCOORD0 XYZW | A duplicate of adjusted homogeneous clip XYZW | Interpolated X,Y,W, then division in the pixel stage |
| TEXCOORD1 XYZ | Normal transformed by the world-view upper3×3 and normalized at the vertex | Only interpolated X/Y, for the optional shininess ripple |
| TEXCOORD2 XYZ | Light direction transformed by the world-view upper3×3 | Not read by this pixel shader |
| TEXCOORD3 XYZ | Position transformed by world-view, before projection | Interpolated view-space position for distance fog |

The normal transforms use the corresponding upper3×3 matrices followed by
normalization; this shader does not construct an inverse-transpose normal
matrix. Its world normal used for vertex lighting is separately transformed
by the world upper3×3 and normalized. The pixel stage does not redo the
directional/point lighting or renormalize the interpolated normal.

All consumed user varyings use **perspective-correct linear interpolation**,
at ordinary pixel frequency. They have no flat, noperspective, centroid or
sample qualifier. This is the native compiled declaration's linear mode;
the distinction from noperspective is documented by
[Microsoft's shader interpolation reference](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dcl-input).

In the pixel stage divide the **interpolated** TEXCOORD0 X/Y by its
**interpolated W** to obtain native screen coordinates X and screenY. The
vertex stage does not first divide that varying by W. In particular, merely
perspective-interpolating per-vertex NDC values is a different operation.
The procedural screen coordinates follow native projected +Y-up orientation,
independently of an API-specific viewport/matrix Y flip.

The pixel fog distance is computed from the **interpolated view-space XYZ**.
It is not an interpolation of per-vertex distances, squared distances or
already computed fog factors.

### 1.1. Conditional comparison to a right-handed look-at basis

The camera convention already verified in `steam-scene-lighting.md` §1 is
view-forward **positive Z**, native projection aspect **−width/height**, and
clip W equal to view Z; the explicit negative aspect is at **0xaf097**.
For a conventional right-handed look-at basis built from the **same eye,
target and rolled up vector**, native view coordinates are
**(−rightHandedX, rightHandedY, −rightHandedZ)**. This is a comparison of
matching camera bases, not a claim about an independently chosen engine's
camera or roll conventions.

The same sign relationship applies to transformed normals: negate the
right-handed view normal's X and Z, retain Y. In particular the ordinary
pixel ripple's native normalViewX must change sign while normalViewY stays
the same. Squared view-space distance is unchanged. Native projection's
negative X scale cancels the view-X reversal for matching projected X;
do not introduce another screen-X reversal on that account. This does not
alter world-space N or D used by the vertex lighting calculation.

## 2. White, Glow and pixel opacity are distinct stages

After the vertex light/palette computation and Glow selection, let the vertex
RGB be C. The vertex stage outputs **sat(C + White×(1−C))**, componentwise.
Here sat clamps to0..1. Thus **White0 still clamps the vertex RGB**, while
White1 makes its vertex RGB white. White is not applied after interpolation,
after fog or in the pixel shader. The White setter is
**0xc5aa0 → 0xc7a20**, bound as vertex constant buffer7.

Positive Glow g selects the already specified interpolation toward
P×materialRGB. A flag1 ordinary model supplies g1. Negative Glow uses P's
luminance as the palette before normal lighting/material multiplication.
These branches still pass through the vertex White clamp and the **same
ordinary world pixel shader** below. Glow does not skip pixel saturation,
fog, sky/sun blending or final grading. The Glow setter is
**0xc59f0 → 0xc6c50**, vertex constant buffer9.

The pixel output's alpha is **the separate scalar Alpha constant**, with no
dependence on interpolated V, material alpha, White, Glow, fog or final RGB.
There is no multiplication by COLOR0 alpha and no shader-side alpha clamp.
RGB is not premultiplied by this separate Alpha constant in the shader.
Its setter **0xc59a0 → 0xc6790** uploads the supplied float unchanged to
pixel constant buffer4. Normal frame setup supplies1; faded draws can supply
their separate draw opacity. Render-target conversion and blend/depth state
are subsequent operations, not additional stages of this shader formula.

## 3. Pixel base color and the optional shininess ripple

Let K be interpolated vertex RGB after §2. Define
**Y=screenY+fogGradientTranslation**. Only if the scalar **Shininess>0**,
multiply K by **1 + 0.3×cos(5X + 10Y + 2×(normalViewX+normalViewY))**.
The normal components are the interpolated TEXCOORD1 X/Y; no pixel normal
normalization is performed. The positive scalar's magnitude is not used as
an amplitude or exponent. Zero and negative values leave K unchanged.
The multiplier is a screen/normal-space ripple, with no time input in this
shader; it is not a specular half-vector power calculation.

The setter is **0xc5a60 → 0xc73b0**, pixel constant buffer5. Normal frame
setup supplies0. The complete list of other per-draw producers of positive
Shininess is outside this trace; the shader behavior for a supplied value
is verified.

Next compute K's luminance with coefficients **(0.299,0.587,0.114)**.
Define gain **u=1+0.25×(1−luminance)**. Replace each channel by
**u×channel+(1−u)×luminance**. This pixel saturation step is distinct from
the earlier shade-dependent vertex palette saturation. Its gain and result
are not clamped at this point; shininess may already have raised a channel
above1. There is no analogous second multiplication by material RGB here.

## 4. Ungraded sky target including the sun halo

This target is reconstructed inside the world pixel shader from the **raw
frame palette**, not sampled from an already graded sky image. Let smooth(t)
mean **3t²−2t³**, and lerp(a,b,t) mean **a+t×(b−a)**. Sky1, Sky2, Fog and Sun
below refer to RGB; their RGBA fourth components are not used here.

The upper gradient is a blend from Sky1 to Sky2 by
**smooth(sat(0.4Y+0.2))**. The bottom color is **0.4×Sky1+0.6×Fog**.
Blend bottom toward upper by **smooth(sat(2Y+1.5))**.

For the supplied SunPos=(sunX,sunY,aspect,visible), form horizontal offset
**(sunX−X)×aspect** and vertical offset **sunY−Y**. Let q be the smaller of
**4×(horizontal²+vertical²)** and1. The sun contribution weight is
**((1−q)×visible)⁴**. Blend the gradient toward Sun RGB by that weight.
Call the resulting ungraded target S. The world pixel shader does not clamp
this target separately before the subsequent fog blend; §5 supplies its
final clamp. Do not apply final grading to S before blending it with geometry.

**0xc5a70 → 0xc7440** uploads Sky1/Sky2/Fog/Sun as the four float4 values of
pixel constant buffer0. **0xc5a80 → 0xc7650** supplies pixel buffer3 SunPos:
project100 times the supplied sun direction through the view's rotational
part and projection, divide by projected W, subtract **(cameraAngle−80)/30**
from projected Y, set aspect to float width/height, and set visible to1
when projected depth is within0..1 inclusive, otherwise0.
The existing frame setup uses its downward sun direction override when the
camera is underwater; palette/time inputs remain specified separately.

## 5. Exact ordinary world fog curve

In the normal world scene, view-space distances and the supplied fog range
are in rendered block units, not fixed-point65536ths.

Let f be the supplied **fogScale**, and let d² be the dot product of the
interpolated view-space XYZ with itself. Form **Q=d²×(f×f)**. The exponent
is **E=1−0.75×exp2(−403.9546203613281×f)**. The negative coefficient is
exactly float word **0xc3c9fa31** in the world pixel blob.

The native shader evaluates the power through base2 operations: log2(Q),
multiplication by E, then exp2, and limits the result to at most1. Call that
value t; for the normal positive finite fog range it is **min(Q^E,1)**.
Then form **s=smooth(t)**. Blend the §3 color toward **Fog by s**, then blend
that result toward the ungraded sky-with-sun target S by **s⁴**. Only after
the second blend clamp RGB componentwise0..1.

This is a full3D view-space distance curve, not a separate horizontal fog
curve. There is no fixed75% haze cap:0.75 occurs in E, not as the final fog
blend opacity. At d equal to the positive fog range F=1/f, t and s reach1
and the pre-grade result is sat(S). Geometry Glow1 and White1 retain these
same fog operations.

**0xc59e0 → 0xc6bb0** accepts a distance F, rather than an already inverted
scale. For F>0 it computes float32 **1/F** and uploads pixel constant buffer1;
for nonpositive F it leaves the previous scale unchanged. Normal0xe2e00
supplies renderer+0x4a4, except that with the effective camera block type2 it
uses **min(renderer+0x4a4,150)**. This changes F, not the shader's fog color
formula or interpolation mode.

**0xc59d0 → 0xc6b10** accepts camera angle and computes
**(angle−80)/(−30)** in float32 for pixel buffer2 fogGradientTranslation.
The SunPos Y correction in §4 is consistent with that same sign. World
fog is reconstructed from ungraded palette RGB; using the final displayed
sky RGB here and grading the result again applies the grading twice.

## 6. Final grading and output

From the clamped §5 RGB, compute luminance with **(0.299,0.587,0.114)**.
Keep red unchanged; replace green and blue by
**0.65×their channel+0.35×that luminance**. Compute this luminance once
from the pre-grade RGB, before changing either green or blue.

Apply **3c²−2c³** independently to each resulting channel. The compiled
arithmetic forms c², multiplies c by c², doubles that cubic, and subtracts
it from3 times the square. There is no further explicit shader saturation,
exposure multiplication, gamma power, tone map, or texture/color-table lookup
after this contrast operation. Output alpha remains §2's separate constant.
The target-format boundary is now traced in §10; it is a different pipeline
stage, not an extra formula inside this pixel shader.

Useful float32 literal words are: luminance0.299/0.587/0.114 =
**0x3e991687/0x3f1645a2/0x3de978d5**; grading0.65/0.35 =
**0x3f266666/0x3eb33333**; fog0.75 = **0x3f400000**;
pixel saturation0.25 = **0x3e800000**. Sky coefficients0.4/0.2/0.6 are
**0x3ecccccd/0x3e4ccccd/0x3f19999a**. These describe shader float operands;
GPU multiply-add, log2/exp2 and raster-interpolation precision should not
be conflated with the separately verified CPU scalar arithmetic in the
vertex-input specification.

## 7. Title background, overlay models and GUI scope

The native **title background is the ordinary live world**, rendered through
0xe2e00 with its ordinary sky/fog/light inputs and this world shader for world
geometry. Frontend flags0x20 and0x40 hide the HUD and enable the logo; they do
not select a menu-specific world pixel formula. This is also established in
`frontend-title.md` §§3.1–3.2. A normal geometry Glow value does not identify
a menu shader or a second grading pipeline.

The3D logo and other model overlays enter a distinct overlay queue through
**0xee5a0** and are drawn near the end of0xe2e00 by **0xe6e60/0xe7980** with
overlay matrices, using the **same ordinary world shader pair**. At this
overlay setup the frame clears Glow/White/Shininess/depth offset to0, sets
opacity1 and default material(1,1,1,1), clears point lights, and supplies
fog range **10000**, from0x42e5e8. Do not replace that finite range by an
assumed shader fog-disable branch.

Its overlay lights differ from the world: front RGBA **(0.7,0.7,0.2,1)**
at0x43f0a0, back **(0.5,0.5,0.5,1)** at0x43f0f0, ambient
**(0.3,0.3,0.8,1)** at0x43f140, and normalized direction proportional to
**(1,1,−1)**. Per-draw overlay descriptors can restore their own material and
effect values. The title logo's supplied material is white RGBA and its
extra effect values are0; its placement/pose is specified separately in
`frontend-title.md` §3.4. These overlay inputs do not replace the background
world's own front/back/ambient palette.

Actual Plasma text/widgets use their own GUI shader path, rather than this
ordinary world pixel contract. Their GUI grading, masks, compositing and
font pipeline are outside this trace. Water, clouds, the sky-only pass and
the separate0x59 light-shaft primitive also keep their own pixel shaders;
sharing some constants does not make their output formulas identical.

## 8. Verification boundary and remaining inputs

The ordinary world shader contract above is fully traced for supplied vertex
and constant inputs. No original-game execution or new runtime capture was
performed. A small oracle should preserve vertex-output RGB/clip/view XYZ,
interpolated values at a labeled fragment, raw palette RGBA, SunPos, fogScale,
fog translation, White/Glow/Shininess/Alpha, and the successive unclamped and
clamped RGB stages. Include variable clip-W triangles, a positive Shininess,
White1, Glow1, zero/near/F/far distance, and a sun-halo-aligned fragment.

Still untraced here are the complete positive-Shininess producer list, the
Plasma GUI's complete separate shaders, and external display transforms.
Section10 closes the native world/backbuffer format and presentation path.
Cross-GPU exact final floating bits for
interpolation/transcendentals are not asserted. Those limits do not change
the verified stage order, constants, interpolation mode or separate opacity.

## 9. Independent finite numeric references for GPU integration

These are **derived arithmetic references**, not owner/native GPU captures.
Inputs and shader constants were decoded to float32; the equations were then
evaluated independently in binary64 and the values below rounded for display.
They are suitable for stage/curve/clamp checks, not claims about exact GPU
floating words. Small shader floating differences should not be confused
with the CPU byte-encoding cases specified separately.

### 9.1. Supplied inputs and vertex results

Use **decoded P=(102/255,204/255,50/255)**. Because P is already the shader's
doubled palette, the corresponding ordinary packed RGB bytes are
**(51,102,25)**, not(102,204,50). Their decoded float32 P is approximately
**(0.4000000059604645,0.800000011920929,0.19607843458652496)**.
Use vertex V byte128, material RGBA **(1,1,1,0.5)**, world N=(0,0,1),
world D=(0,0,−1), front=(0.7,0.7,0.3), back=(0.5,0.5,0.5), ambient
RGBA=(0.3,0.3,0.7,1), and zero point lights. All decimal inputs denote
their float32 values. Pixel Shininess is0 and separate Pixel Alpha is1.

The front directional factor is0, so its supplied RGB makes no contribution
in this example. The shared fixed-axis factor h is **0.8482630253**;
V×materialAlpha is **0.2509804070**. The clamped front/back sum is
**(0.4241315126,0.4241315126,0.4241315126)**. The resulting clamped lighting
RGB L is **(0.3609276174,0.3609276174,0.7002328073)**.

P's luminance is **0.6115529582**, vertex saturation gain is **1.3195361913**,
and boosted palette before lighting is
**(0.3324011813,0.8602156657,0.0633192877)**. The three effect cases give:

| Case | Vertex RGB after Glow selection and White clamp |
| --- | --- |
| A: White0, Glow0 | (0.1199727664,0.3104755906,0.0443382426) |
| B: White0, Glow1 | (0.4000000060,0.8000000119,0.1960784346) |
| C: White1, Glow0 | (1,1,1) |

For the pixel reference, supply those RGB values constantly across the
triangle, screen X=0/screenY=0, fog translation0, and fragment view position
(0,0,2). This avoids a varying-color interpolation dependency in the color
test; interpolation itself remains §1's separate contract.

### 9.2. Pixel saturation and grading with fog omitted as a diagnostic

First ignore the tiny finite fog only for the following diagnostic table.
The extra pixel saturation gains are approximately **1.1942061021** for A,
**1.0971117604** for B, and1 for C.

| Case | RGB after pixel saturation | RGB after green/blue grading, before contrast | Final RGB with fog omitted |
| --- | --- | --- | --- |
| A | (0.0999301479,0.3274297831,0.0096069381) | (0.0999301479,0.2909408069,0.0843559652) | (0.0279622916,0.2046853864,0.0201472445) |
| B | (0.3794557263,0.8183004371,0.1557309722) | (0.3794557263,0.7459387962,0.3152686598) | (0.3226868284,0.8391565396,0.2355111505) |
| C | (1,1,1) | Approximately(1,1,1) | Approximately(1,1,1) |

With nearest conversion to a **linear UNORM8 target**, the reference RGB
bytes are respectively **(7,52,5)**, **(82,214,60)** and **(255,255,255)**.
Output alpha is255 for all three because Pixel Alpha is1, despite the vertex
V byte128 and material alpha0.5. These bytes assume this explicitly named
target conversion. Section10 independently confirms the native UNORM target;
external display transforms are not part of these byte references.

### 9.3. Actual finite near-fog reference and a full-fog endpoint

For distance2 and fog range **1000000**, the setter's float32 fogScale is
**9.999999974752427e−7**. Derived Q is **3.9999999798e−12**, E is
**0.2502099706**, t is **0.0014064418**, smooth factor s is
**5.9286714942e−6**, and sky factor s⁴ is **1.2354624425e−21**.
The fog is small, but it is not identically zero.

For a fully specified finite test, set raw Sky1, Sky2, Fog and Sun RGB all0,
and SunPos.visible0. The sky target is0. The actual reference results are:

| Case | RGB after finite fog/sky clamp | Final RGB after grading/contrast |
| --- | --- | --- |
| A | (0.0999295555,0.3274278419,0.0096068811) | (0.0279619719,0.2046832514,0.0201470127) |
| B | (0.3794534767,0.8182955856,0.1557300489) | (0.3226836500,0.8391515109,0.2355087295) |
| C | (0.9999940713,0.9999940713,0.9999940713) | Approximately(0.9999999999,0.9999999999,0.9999999999) |

These retain the same nearest linear UNORM8 RGB bytes as §9.2. Arbitrary
other raw palette RGB bounded0..1 changes the tiny finite near-fog result;
it cannot be called the exact black-palette result. For these cases its
effect relative to the no-fog result is below about **9e−6 per final RGB
channel** in the stated mathematical reference, before GPU roundoff.

As a full-fog endpoint, instead use **view distance64 and range64**, still
at screen center with the same black raw palette. Here Q=t=s=s⁴=1 and
the reference output is **RGB(0,0,0), alpha1 for A, B and C**. Thus both
Glow1 and White1 still become the fog/sky result at full range. A White
operation placed after fog would fail this endpoint.

## 10. Native render target, MSAA and presentation boundary

Sources: graphics creation **0x134cc0**, resize/rebind **0x135310**, outer
frame/presentation **0x134420**, original game-frame render call **0xa5a50**,
renderer **0xe2e00**, world state creation/binding **0xc3150/0xc3dc0**, GUI
draw bridge **0xc4af0 → 0x33e170**, and GUI texture target begin/restore
**0x392610/0x392200**. These facts are from the owner's **original unpacked
native executable**. No mod-loader, injected graphics DLL or external effect
implementation was inspected or included.

### 10.1. Backbuffer and depth formats

0x134cc0 creates the hardware D3D11 device, then an original DXGI swapchain
with these fields:

| Field | Native value |
| --- | --- |
| Buffer format | **28, DXGI_FORMAT_R8G8B8A8_UNORM** |
| Buffer count | 1 |
| Buffer usage | 0x20, render-target output |
| Swap effect | **0, DXGI_SWAP_EFFECT_DISCARD**, the original bitblt model |
| Initial Windowed field | True; subsequent mode application can enter fullscreen |
| Flags | 2, allow mode switch |
| Sample count | Configured antiAliasingSamples, see §10.2 |
| Sample quality | Reported quality-level count minus1 |

It obtains backbuffer0 and creates the **render-target view explicitly with
format28**, dimension **6, Texture2DMS**. The separate depth texture has
**format40, DXGI_FORMAT_D32_FLOAT**, one mip, one array element, matching
sample count/quality and depth-stencil binding; its depth view uses format40
and dimension5, Texture2DMS. The routine binds this backbuffer RTV and depth
view directly to the output merger. The relevant RTV creation is
**0x134fec**, with initial output-merger binding at **0x1350d4**.

This is a typed **UNORM** resource and UNORM view, not a typeless resource
with an sRGB RTV. Format29, R8G8B8A8_UNORM_SRGB, is not selected here. The
distinction and numeric format IDs are documented in
[Microsoft's DXGI format reference](https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format).
Consequently the native output merger does **not** insert sRGB encoding
through this view. The already graded shader RGB numbers go directly to
normalized8-bit target values, subject to the draw's blend and coverage.
No additional shader gamma power is needed to reproduce this application
boundary. This does not claim that the artistic palette is physically linear
light or describe the monitor's transfer function.

On resize,0x135310 unbinds/releases the old backbuffer view, calls
**ResizeBuffers with format0/UNKNOWN**, preserving the swapchain format,
obtains buffer0 again and creates its new RTV with a **null/default view
descriptor**. It inherits the same typed UNORM format. It recreates the
matching D32_FLOAT depth target and binds both, with viewport depth0..1.
Changing window/fullscreen mode does not select an sRGB view in this path.

### 10.2. Actual sample count, quality and shader frequency

The options constructor **0x13a9e0** supplies **antiAliasingSamples=8**,
overridden by the existing0x13aa50 options reader when present. Graphics
creation reads that count directly from global **0x551c40**. Its separate
legacy **antiAliasing** on/off option defaults0, but **is not tested by
this swapchain sample-count selection**. Do not infer a single-sample target
merely from that option being0.

Creation calls **CheckMultisampleQualityLevels(format28, configuredCount)**,
then chooses **quality=returnedCount−1** at **0x134e89**. It does not request
the special standard-pattern quality constant, and there is no lower-count
search/fallback in this creation routine. The same selected sample count
and quality are supplied to the color swapchain and depth texture. A failed
graphics initialization is reported by the original startup path, not
silently changed here into a different sample count.

The ordinary world rasterizer descriptors built by0xc3150 use
**MultisampleEnable=false** and **AntialiasedLineEnable=false**. Their
depth clipping is enabled. Their blend descriptors have
**AlphaToCoverageEnable=false** and **IndependentBlendEnable=false**;
0xc3dc0 binds the full sample mask **0xffffffff**. Ordinary opaque state0
has blending disabled and RGBA writes enabled; other world passes retain
their separately specified depth/blend controls.

The false rasterizer MultisampleEnable field must be interpreted with the
selected D3D feature level. For **10.1 and higher**, it does not disable
triangle MSAA; for9.x/10.0 its effect differs. This is an API rule documented
in [Microsoft's D3D11 rasterizer descriptor reference](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc).
The native device call does not request an explicit feature-level list or
retain the selected feature level through its optional output argument.
No particular owner's actual runtime feature level or sample positions were
recorded in this audit.

The compiled ordinary world pixel shader uses pixel-frequency, default
linear interpolation, without sample/centroid qualifiers or sample-index
inputs. MSAA coverage/depth can therefore differ per sample while the pixel
shader is not explicitly converted to a per-sample shader. Exact sample
positions/quality are device-dependent inputs; an edge oracle should capture
the actual feature level, sample count, quality and coverage rather than
assuming one universal8-sample pattern. Fully covered constant-color
interior pixels remain suitable for §9's byte checks.

### 10.3. Frame order and absence of a general scene color postpass

The original outer frame0x134420 clears the global backbuffer RTV to white
and depth to1, then calls the original game frame0xa5a50. That game frame
calls0xe2e00 for both title and ordinary gameplay rendering. The normal
world passes operate on the bound swapchain backbuffer; this is not an HDR
offscreen scene whose result is later run through another tone-map shader.

At the end of0xe2e00, after its3D overlay model draws, **0xc4af0** invokes
the GUI engine through0x33e170. GUI geometry/compositing is distinct from
world fog/grading. Native GUI component caching can use offscreen targets:
**0x392610** saves the previous render target/depth and viewport, binds its
own target and clears it transparent; **0x392200** restores the saved target,
depth and viewport. This is not a copy of the whole world into a compulsory
scene-wide color filter. Individual visible UI layers can of course cover
or blend over world pixels.

After the game-frame render returns, the traced outer path performs mode/
resize bookkeeping and frame pacing, then calls **Present(vsync?1:0,0)**
at **0x13474e**. It does not draw a final full-screen tone/gamma/color shader,
and it does not explicitly resolve a separate multisampled scene texture
into a single-sample display texture on this path. The original multisampled
DISCARD swapchain is presented through DXGI; this differs from a modern
flip swapchain with an application-owned MSAA resolve texture. See the
[DXGI swap-effect documentation](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect)
for the API presentation-model distinction.

Thus the application-side world color contract ends with its own shader
grading, UNORM writes/blending/coverage, GUI overlays and native DXGI
presentation. An injected mod-loader/ReShade/vendor postprocess is **not**
part of this original path and must not be inferred from a modified capture
as a missing native tone/color pass. Driver/OS composition, display
calibration/HDR remapping, and any injected effects remain external to this
specification; no original-game or injected-runtime capture was performed.
