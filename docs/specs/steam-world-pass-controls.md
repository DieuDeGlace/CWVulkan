# Steam world, model and overlay shader controls

Independent native pass/input audit:0xe2e00,0xe5070,0xe6e60,0xe7980,
0xe85e0,0xee2f0,0xee5a0,0xc3dc0/0xc3150 and shader setters. Owner
blackwidget data below was read directly from the original gui.plx in the
native-analysis source set, not an exporter/capture. No engine, tests,
exporter or build files were inspected, and no game was run.

Use steam-world-shader-output.md for the WORLD formulas and
steam-gui-item-icons.md for direct0xddbc0 icon transforms. This document
specifies which constants actually reach those shaders. A pipeline-state
bind does not reset material/effect buffers to presumed defaults.

## 1. Frame-wide world constants and inheritance

Scene setup at **0xe3974–0xe3a54** selects ordinary state0, sets **White0,
Opacity1, Shininess0, Glow0, DepthOffset0**, and submits frame material
**renderer+0x530**, normally white or the verified underwater(0.5,0.5,1,1).
Directional palette and ambient come from steam-scene-lighting §2. The
frame also uploads its smoothed Sky1/Sky2/Fog/Sun RGBA and projected SunPos
through0xc5a70/0xc5a80. These pixel inputs are not recomputed from each
subsequent model's local view matrix.

The world **fog range F is renderer float+0x4a4**, the adaptive maximum
range, not+0x49c and not the ordinary-model fade-end radius. When the
camera's effective block type is water2, pass **min(F,150)** instead,
with stored150 at0x42e938. Setter0xc6bb0 supplies1/F when F>0, otherwise0,
as already specified. At0xe3a50 the gradient-angle input is the first
component of the submitted camera-angle vector, i.e. displayed pitch.
Setter0xc6b10 computes float **(pitch−80)/(−30)**. Do not recompute that
gradient from a GUI icon's identity view.

The meaningful main ordering in0xe2e00 is:

1. Normal scene constants and sky preparation.
2.0xd3170 prepares living actors, ordinary scenery descriptors and effects;
 0xce5f0 collects/binds the current light tables.
3. Terrain0xe5070, then opaque ordinary descriptor list **renderer+0x138**,
 opaque second list **+0x1d0**, and its faded consumer0xe7980.
4. Batched effect consumers0xe6270 and, when enabled,0xe6840.
5. Re-submit the normal directional palette/ambient, then **water0xe85e0**.
6. Faded ordinary-list consumer0xe7980, then remaining clouds/effects and
 the later screen-overlay preparation/submissions.
7. Screen overlays, then GUI traversal through0xc4af0→0x33e170.

The detailed opaque/faded state sequence and model flag8 timing remain
worldgen-parity-gaps §§9.5–9.6. In particular the second descriptor list
is not implicitly an after-water pass.0xe5070 may temporarily replace
lighting for an auxiliary face category, then restores the supplied normal
palette; it does not replace the frame Sky/Fog/SunPos buffers with icon data.

## 2. Ordinary queued model controls

An ordinary opaque list node consumed at **0xe7328–0xe7381** contains:

| Offset from list node | Meaning |
|---|---|
| +0x10 | float4x4 model matrix |
| +0x50 | submitted material RGBA |
| +0x60 | separate pixel Opacity |
| +0x64 | Glow |
| +0x68 | Shininess |
| +0x6c | White |
| +0x70 | shader DepthOffset |
| +0x74/+0x78 | overlay screen center X/Y when requested |

Both normal and opposite-cull opaque trees explicitly submit **all six
material/effect inputs per draw**, not only material and opacity. Matrices
are followed by material, Shininess, Glow, Opacity, White, DepthOffset,
then voxel draw. Ordinary world mode uses the already specified transformed
header-center point-light zone. Screen-overlay mode clears point tables
and does not bind world points for each model.

Faded-vector records are120 bytes, with sprite pointer+0, matrix+8,
material+0x48, Opacity+0x58, Glow+0x5c, Shininess+0x60, White+0x64,
DepthOffset+0x68 and screen center+0x6c/+0x70. Their final colored draws
also submit all these values. The first two traversal passes select
states3/4 and draw depth-only using the matching cull orientation; then
colored states6/7 use alpha blending. These depth-only iterations update
model/projection/DepthOffset but need not write every color constant,
because their color write mask is0. The later color iterations do.

Thus WORLD **material alpha** is the lighting-factor input from the vertex
contract, **Opacity** is the separate final pixel alpha, and **Glow/White**
are their independently specified shader operations. Do not merge them
into a single per-model opacity or infer Glow from a CPU vertex flag.
This also applies to generic creature overhead models and screen overlays.

## 3. Water controls and retained values

**0xe85e0** shares the ordinary WORLD **vertex shader**, input layout and
constant bindings. State5 is a no-color depth prepass with that ordinary
shader pair; state9 then replaces only the pixel shader with native water
PS **RVA0x43b790**, length0xef4, through0xc5ff0. Shader creation0xc60d0
and state selector0xc3dc0 establish this distinction. The water pixel
formula remains the separate water-rendering specification.

Before its two zone/group traversals set **White0, Glow0, material from
renderer+0x530, zero point-light tables**, and separate Opacity from
**renderer float+0x4a8**. The depth pass has no color writes; the color
pass binds each water-containing zone's+0x240/+0x340 point tables. Both
pass traversals use the same ordinary group model translations. At exit
explicitly restore separate **Opacity1**.

No water call here writes **DepthOffset, Shininess, F, gradient translation,
Sky/Fog/Sun RGBA or SunPos**. DepthOffset therefore retains the preceding
draw's value; normal terrain/model values are usually0, but that is not a
water-specific reset. The water PS does not acquire the WORLD PS's
Shininess ripple merely because the scalar buffer is retained.
Immediately before water,0xe2e00 restores the normal directional palette
and ambient; the water material itself is explicitly restored by0xe85e0.

Water Opacity+0x4a8 is initialized to1 by0xcbd10 and updated by0xecda0:
stored float advances by float **s times(1−old)+old**, with
**s=0xa7a0(frameMS,0.05)**. No arbitrary camera-depth/scene-rain opacity
replacement is established by this producer. The water PS's own opacity
formula is distinct from this submitted scalar.

## 4. Screen overlays and exact logo lighting

Overlay setup **0xe4d90–0xe4f22** installs identity view and builds its
own perspective projection through0xa0340. Its FOV argument is literal
**float0.8726646900177002**, i.e.50° in radians, at0x437f98; aspect is
negative float(W)/float(H), near literal float0.05, far float(signed32
renderer zone-window count+0x438 shifted left6).0xa0340 computes cotangent
from half that float FOV and the usual native positive-view-Z, W=Z,
depth0..1 projection. This is distinct from direct item icon0xddbc0's45°.

An overlay descriptor shifts that projection by its pixel center's NDC
X/Y and **NDC Z−0.5**, on the left. The model itself starts at Z1,
appends X/Y/Z Euler rotations and one submitted uniform scale, then
centers the full sprite header dimensions. There is no item-icon header
divisor or automatic1024/H scale inside0xee5a0: those belong to callers.
The corresponding projection shift is in0xe6e60/0xe7980 with mode bit1.

At setup set **Opacity1, White0, DepthOffset0, Shininess0, Glow0, white
material**, fog range **10000**, and clear points. Fixed lighting is:
front **(0.7,0.7,0.2,1)**, back **(0.5,0.5,0.5,1)**,
ambient **(0.3,0.3,0.8,1)**. Prepare float a=1/sqrtf(3), vector
**(a,a,−a)**;0xc6ce0 normalizes that vector again. These are the constants
at0x43f0a0/0x43f0f0/0x43f140. They are not the current day/night lights.

**Sky/Fog/Sun RGBA, SunPos and fog-gradient translation are retained from
the current frame.** Overlay matrix changes do not reproject SunPos or
reset it to a hidden sun. The finite10000 fog range still allows a small
contribution from those inputs. Native0xc4b00/0xc5300 effects change their
own draw pipeline/matrices but do not replace these shared palette/gradient
inputs before the overlay setup. Each overlay descriptor then explicitly
sets its own material/effects as in §2.

Logo0xc31 receives **white material, Opacity1, Glow0, Shininess0, White0,
DepthOffset0**. Thus it is normally directionally shaded by this fixed
palette, with the ordinary WORLD saturation/grade; no fullbright override,
logo-specific contrast curve or extra UI tint is established here.
Its source/position/rotation remain frontend-title §3.4, with exact
arithmetic clarification: unsigned timeGetTime remainder modulo10000 is
taken **before** float conversion; multiply float remainder by stored
float0.0001, add it to itself, widen to double and multiply double pi,
narrow to float for cosf. Screen Y separately multiplies that cosine by
float0.01, then float5, then adds0.25 and multipliesH. Rotation X is20
minus float(cosine times5). Caller uniform scale is successively GUI scale
times float0.011, times float(1024/H). These stages should not be collapsed
into a differently rounded phase/scale expression.

For other0xee5a0 overlays, submitted material alpha≥1 selects the opaque
tree with that material and Opacity1. Material alpha<1 selects the faded
vector, with **material RGB unchanged/material alpha1**, and the original
material alpha supplied separately as pixel Opacity. Glow/Shininess come
from the producer's separate inputs, White is0 and DepthOffset is its
separate producer input. This routing is not a reason to skip WORLD
lighting or grading for a translucent overlay.

## 5. What direct GUI item icons inherit

The main0xddbc0 pass explicitly supplies its own material/effects, points
and fixed direction proportional to(1,0.4,−1), as in its shared spec.
The GUI traversal occurs **after** §4, so its retained Sky/Fog/SunPos/
gradient are the same scene inputs described above, even though its own
projection/view are local. It sets F=10000 itself. Do not replace these
inherited values with arbitrary black sky/hidden-sun constants merely
because the icon is in a panel.

Its optional black pass precedes the main-pass White/Glow/Shininess/points
and lighting writes; these are whatever prior draw left. Normal overlay
preparation begins at0, but prior overlay descriptors or GUI callbacks can
change them.0xddbc0's main pass and caller GUI-context restoration do not
amount to a general save/restore of all WORLD constant-buffer values.

## 6. Owner blackwidget fill and border opacity

Native widget constructors clone the owner blackwidget and do not supply
a replacement constant such as0.75 or0.9 for its fill. For the original
gui.plx in the native-analysis source set, SHA256
**3e8fc1c334df173ef90ae8b2d28ea816835a1dada1f89051f6d50b0d8142e7f7**,
the first pre-order blackwidget is **node136**, with shape48/display50,
transformation50/widget1, and child **node137**, shape49/display49/
transformation49/no widget. Preserve the actual owner's loaded scene;
these indexes/file offsets identify this source file, not a universal
replacement template for other builds or changed owner data.

The root shape48's twelve live vertex RGBA entries are all
**(0,0,0,0.6689655184745789)**, alpha word **0x3f2b4153**. Its first
vertex-color frame is at file offset **0x1d56c**,192 bytes(12×16).
Display50 live fill RGBA at **0x1aa220** is white. Shape flags at
**0x1df88** are2: fill enabled and stroke enabled, surface subdivision0.
Its live stroke colors are opaque black and all twelve stroke widths3.

The child shape49's flags at **0x1eccc** are6: **fill disabled**, stroke
enabled; all twelve live stroke colors are opaque white and widths2.
Its stored vertex fill colors, some purple, do **not** produce another
translucent interior. Display49 live fill at **0x1aa130** is white.
The two shapes share the same stored twelve-point outline. Their native
stroke-alignment integers are1 and−1 respectively. Border width/alignment,
deformation, visibility and mask behavior remain the loaded template data
and Plasma drawing contract, not a newly styled rectangular border.

For an untextured interior, native shape draw0x3913d0 multiplies its own
display's fill RGBA by the current fill tint, then the shape's interpolated
vertex RGBA in the ordinary Plasma pipeline. There is **no parent tint
inheritance**. Thus with default white pass tint the interior's source alpha
is exactly the owner float above; this is distinct from WORLD pixel
Opacity and WORLD material alpha. The GUI shader does not apply the
WORLD fog/grade to that panel. Keep the owner's data-driven shape rather
than hardcoding this numeric alpha into every GUI panel.
