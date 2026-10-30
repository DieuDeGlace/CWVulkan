# Steam NPC speech bubbles and emote rendering

Independent clean-room behavioral facts from the original native executable's
decompile and assembly. No engine, tests, exporter, owner captures or generated
build files were inspected, no assets were copied, and no game was executed.
All addresses are native RVAs. Page timing, repeat Use, emote selection and
lifetime remain in `remaining-runtime-gaps.md` §5.7 rather than being repeated.

## 1. Visible answers hide both crosshairs

Interface update **0x103200** separately controls the normal **crosshair**
shape at **G+0x2f0** and **zoomcrosshair** at **G+0x2f8**. Both require the
ordinary UI predicate **0x1014f0 to be false**. Thus the **visible answer
panel G+0x378 hides both**, independent of hovered row, pointer position or
whether the pointer hits a choice. Nonempty answers hidden behind a speaker
bubble do not satisfy the answer-visibility part of that predicate.

Both crosshair checks also explicitly require the general DialogWidget
**G+0x418** to be hidden, despite that general widget not being part of
0x1014f0. Their other native aim/ability requirements remain in place:
normal/zoom selection uses player **+0x128 bit 0x400**, and the normal
crosshair also excludes ability **0x9c**. The ordinary speaker-bubble vector
**G+0x400** is not itself a crosshair-hiding predicate. These branches are
the consecutive crosshair visibility updates in 0x103200, following its
other HUD visibility updates.

### 1.1 Complete predicate and frame order

For each crosshair separately, 0x103200 first queries general-widget root
visibility through **0x9af00**. If hidden, it evaluates **0x1014f0**; only
if that is false does it inspect the player's aim flag. Normal crosshair
requires **bit0x400 clear**, then **ability byte+0x68 unequal to0x9c**.
Zoom crosshair requires **bit0x400 set**, without that ability exclusion.
Both roots are explicitly assigned their resulting visibility each update;
this is not a toggle or a state retained until another input event.

For completeness, 0x1014f0's true conditions, in native short-circuit
order, are the following. Widget conditions mean their current primitive
visibility property is nonzero, rather than an empty/nonempty contents
test, pointer hit, navigation focus, or existence of a widget object.

| Order | State making the predicate true |
|---|---|
| 1–5 | Visible front layers **G+0x118**, **+0x130**, **+0x170**, **+0x198**, **+0x200**: title, character select/create, world select/create |
| 6 | Nonzero full-map-open byte **G+0x668**, including an ordinary map opened by the map binding |
| 7–10 | Visible widget roots at widget+0x178 for **G+0x330**, **+0x340**, **+0x388**, **+0x350**, in that order |
| 11 | Nonzero menu-open byte **G+0x420** |
| 12–14 | Visible widget roots for **G+0x370**, **+0x368**, **+0x360**, in that order; the last is the identification panel |
| 15 | Nonzero chat-active byte **(G+0x398)+0x1d8** |
| 16–19 | Visible widget roots for **G+0x378** answers, **+0x358** system menu, **+0x658**, **+0x650**, in that order |

The menu byte and identification root are independent enumerated tests;
neither is inferred from speech activity. G+0x668 is the normal interactive
map byte, not a Talk flag or a flight/shrine-only state. In this table the
less frequently used panels retain their native G offsets so that an
untraced semantic name does not silently broaden the predicate.

Frame **0xa5a50** calls speech/bubble update at **0xaa1bc**, performs the
movement-dismiss path around **0xab283**, then calls interface update
**0x103200 at0xab5ce**. The consecutive crosshair assignments occur in that
interface call, before scene render at **0xad9d4**. Thus they consume the
answer root's visibility at that point in the current frame; there is no
extra crosshair hide simply because an NPC speech root remains retained.

## 2. Bubble construction, template and text

Attach helper **0xa3170** finds an existing bubble by **speaker creature ID**
at widget+0x210, or creates a new **0x300-byte** widget through **0x28fd60**
at **0xa3238**. It copies the owner's **speech** template **G+0x430** under
the game UI root **G+0x220**, using **0x333890**. This is the speech template,
separate from the blackwidget template used for the general/answer panels.
It passes **null next/end indicator templates** for an ordinary NPC bubble.

The common constructor uses **resource1.dat, font size 12, outline 3**,
and milliseconds-per-character 40. Bubble attachment does not set a new
numeric width/height: the copied speech template supplies its frame,
shape, decorations and appearance. The placement pass below changes its
position/visibility, not its size. Do not add distance-dependent scaling
or fit-to-page resizing to this path.

Ordinary pages append to this bubble's existing page vector at
**+0x1e8/+0x1f0/+0x1f8**. Answer-ID pages instead go to the separate answer
panel. Attachment sets the bubble's full speaker ID+0x210 and recipient
ID+0x218; null recipient becomes ID zero. It initializes the bubble's
signed64 smoothed XYZ **+0x2c0/+0x2c8/+0x2d0** from the speaker's **drawn
position +0xb10/+0xb18/+0xb20**. Reusing a bubble also performs these writes.

The shared word draw **0x28ffe0 / 0x2904b0** supplies the same precise
colored-word, prefix, margins, gaps, wrap, shake and font-ink measurement
rules as `steam-dialogue-panel-routing.md` §§5–7/10. Use the bubble's copied
template frame width in those rules. There is no answer-row hover color or
choice navigation registration in this shared speech draw. Its null
continuation templates mean there is no general-dialogue next/end icon to
draw on an NPC bubble.

## 3. World anchor smoothing and projection

The retained-bubble pass of **0xb40d0**, **0xb4b30–0xb4df9**, resolves the
speaker in **World+0x450**. With a live nonnull entry it updates each cached
signed64 fixed coordinate before projecting. If that lookup fails, this
pass skips its anchor/position/visibility work; it does not install a new
fallback actor or delete the bubble here. Normal page exhaustion remains
the separate lifetime/removal path from §5.7.

The smoothing input is this update's **double frame milliseconds**, passed
to **0xa7a0** at **0xb4ba4** with coefficient **float32 0.01** from
**0x42e51c**. The helper's exact easing arithmetic is already specified in
`steam-scene-lighting.md` §5. Let the returned float be A. Multiply it by
float32 **65536** and truncate to signed64 to obtain its fixed fraction.
For each axis, take target minus current cache as signed64, multiply by
that fraction with the native 64-bit product, divide the product by 65536
**toward zero**, and add it to the current cache. The assembly uses the
negative-product add-65535/right-shift form for this toward-zero division.
There is no float interpolation followed by a final coordinate conversion.

The target X/Y are the speaker's drawn +0xb10/+0xb18. Target Z is drawn
**+0xb20 plus trunc64(float32(−speaker float+0x990 ×65536))**. Negation
precedes the float32 multiplication. This body offset therefore eases
with the cached Z rather than being added only after projection.

Construct the projection point from the newly updated cache:

- X fixed is cache X plus **G+0x1df0**.
- Y fixed is cache Y plus **G+0x1df8**.
- Z fixed is cache Z plus the signed64 truncation of float32
  **(speaker body-Z float+0x8c ×0.5 +1) ×65536**.

Convert each signed64 sum directly to float32, then multiply by exact
float32 **1/65536**, constant **0x42e510**. Transform by cached matrix
**G+0x1f24** using **0x2b680**, including that helper's homogeneous divide.
The root is hidden if the resulting view Z is **not strictly positive**;
this includes equality and unordered comparisons. With positive Z it is
initially made visible, then transformed through **G+0x1f64**, again by
0x2b680, for projection.

For projected x/y, native screen coordinates are **(x×width×0.5+width×0.5,
y×(−height)×0.5+height×0.5)**, with float32 operations in the order stated.
Width/height are signed32 client dimensions at GUI engine **+0x1c4/+0x1c8**.
Clamp screen Y **first upward to G+0x664 ×180**, then downward to window
height. G+0x664 is the GUI scale; 180 is **0x42e7d4**. Screen X is not clamped.
There is no projected X/Y frustum rectangle, far-distance, wall-occlusion,
speaker-on-screen or terrain-depth test in this bubble placement pass.

Position the bubble root through **0x334700**, call **0xb4df2**, with that
screen point and the fixed local offset **(−125,−170)** from
**0x43803c / 0x438044**. The placement helper **0x334510** converts the
screen reference through the parent transform, adds the local offset and
compensates the primitive's frame origin. Thus the offsets remain GUI
units; do not subtract them as unscaled physical pixels at arbitrary GUI
scale. This is literal template placement, rather than a newly measured
half-width/height centering rule.

The scene renderer writes these cached view/projection outputs later in
the frame: **0xa5a50** calls bubble update at **0xaa1bc**, then scene render
**0xe2e00** at **0xad9d4**, passing G+0x1f24/G+0x1f64 as outputs. The bubble
update consequently consumes the matrices already cached at that point,
normally from the preceding completed scene render. Do not silently
replace that ordering with a fresh same-frame camera projection when
claiming exact native scheduling.

## 4. Bubble visibility during emotes

After the projection step, the same retained-bubble pass checks the live
speaker's emote model pointer **creature+0xc50**. A nonnull pointer forces
the text root hidden at **0xb50a8**, even when the anchor projects in front
of the camera. This is independent of recipient ID or whether the emote
model itself will pass the 3D renderer's visibility gate. It is not a
depth-based text/model overlap test.

The text is an ordinary GUI subtree using the existing GUI appearance and
clip/mask rules, rather than an extra world voxel quad. Its lifetime and
clock suppression while an emote is active remain in §5.7. Hiding a root
does not remove it from the bubble vector used to decide whether pending
answers may appear.

## 5. Emote model resources and animation

The dedicated consumer is **0xd3170**, **0xd83a1–0xd86de**. A nonnull
creature+0xc50 supplies the selected model, overriding the adjacent generic
class/quest/buff overhead-model selection. Resource registration in
**0x177f50 at0x1e0558–0x1e081d** supplies these catalog entries:

| Catalog index | Native asset name |
|---|---|
| 0xdec | emote-question.cub |
| 0xded | emote-alert.cub |
| 0xdee | angry.cub |
| 0xdef | angry2.cub |
| 0xdf0 | heart.cub |
| 0xdf1 | emote-sweat.cub |

These are owner-provided CUB models, not generated quads, font glyphs or
reconstructed images. Selection/lifetime and the speech-emote page marker
remain specified in remaining-runtime-gaps §5.7. The render clock here is
the signed32 age in milliseconds at creature+0xc48.

Let S be the geometric scale multiplier, B an additional height in blocks,
and theta an additional local-Y rotation in degrees. At entry they are
**1,0,0**. All divisions and ordinary products described as float below
are **float32**, with rounding after the indicated operation.

For age **less than200**, first divide float(age) by200. Convert that
float quotient to double and multiply by the native double pi constant
at **0x42e758**. This double phase is used twice: B becomes one half of
**sinf(float(phase))**; S becomes
**sinf(float(phase times double(float32(0.6))))**, divided by
**sinf(float32 constant1.8849556446075439)**. The multiplier's stored double
value is **0.6000000238418579** at0x43ef88; the denominator argument is at
0x43ef90. This entrance has a slight geometric overshoot; it is not a
linear fade. At age200 the entrance branch is skipped and S=1,B=0.

For age **greater than1800**, S is replaced by
**1 minus float(age−1800)/200**. At1800 it remains1, and at2000 it is0.
The existing update removes the active emote only after age exceeds2000.
There is no alpha fade corresponding to either scale branch.

The sweat model **0xdf1** additionally adds **0.5 minus float(age) times
float32(0.0005)** to B, after the entrance bounce. That constant is at
**0x43ef2c**. It produces a gradual downward displacement, independently
of geometric scale.

For either angry model pointer, **0xdee or0xdef**, theta becomes
**45 plus10 times sinf(float(age) times float32(0.01))**. The submitted
resource alternates between0xdee and0xdef in consecutive **250-ms**
intervals: for ordinary nonnegative age, even intervals use0xdee and odd
intervals0xdef. The native signed quotient/remainder sequence is at
**0xd8509–0xd8529**. This changes the submitted model, not the retained
creature+0xc50 pointer. Other listed emotes leave theta zero.

There is a separate nearby heart fallback when +0xc50 is null and buff
lookup **0x5e3a0(creature,type7)** succeeds: it selects0xdf0, with
S=0.5+the buff's float strength+0x4, B=0 and theta=0. That is not an
active emote and does not suppress the text through the +0xc50 test.
The active-emote branch's forced-white material assignment is skipped
for that fallback; it retains the adjacent generic overhead selection's
existing material.

## 6. Exact emote transform and draw controls

Transform construction is **0xd8576–0xd869e**. It starts from identity.
Its anchor uses the actor's **physical signed64 fixed position
+0x10/+0x18/+0x20**, unlike the smoothed drawn position used by the text
bubble and the actor visibility test.

The head-height offset is formed in float32, in order: body Z extent
**+0x8c times0.5**, plus1, minus float+0x990, plus B. Multiply that
completed height by65536 and truncate to signed64. Add it to physical Z;
add renderer origin offsets **+0x428/+0x430** to physical X/Y. Convert the
resulting fixed coordinates to floating block units through **0x6d6b0**:
convert each completed signed64 sum to float32, then multiply that float
by the exact float32 **1/65536**. Install that translation. This path does not reuse the text bubble's
cache or its screen clamp.

The model is then rotated about world Z by **actor float yaw+0x30**,
followed by the extra **local-Y theta** rotation. It is **not camera
billboarded**. Uniform XYZ scale is, in order, float32 **0.8 divided by
the model header's X dimension+0x88**, multiplied by S. Finally its local
origin is translated by **negative one half of each complete header
dimension X/Y/Z at+0x88/+0x8c/+0x90**. These are full sprite dimensions,
not occupied-vertex bounds. At S=1 the nominal width is0.8 blocks; the
other extents preserve the CUB dimension ratio, centered at the anchor.
No actor-width multiplier or additional distance scale is applied.
Each header dimension is read as signed32 and converted to float32
before the division or negative-half multiplication described here.

For an active emote, the material RGBA is explicitly **(1,1,1,1)** at
**0xd83b5–0xd83de**. Submission **0xee2f0 at0xd86de** goes to ordinary
voxel queue **renderer+0x138**, with **Opacity=1, Glow=1, Shininess=0,
White=0, DepthOffset=0**. This mapping follows descriptor consumption
**0xe7338–0xe7381** and the D3D renderer vtable at0x4385e8, rather than
inferring Glow from the model name. Normal world shader fog and final
grading remain applicable; Glow1 is not an always-white GUI shader.

Opacity1 selects the opaque ordinary-model queue. Its ordinary consumer
**0xe6e60**, with draw state **0xc3dc0/c3150 state0**, uses depth test
**LESS_EQUAL**, enabled depth writes, disabled alpha blending, normal
depth clipping and the ordinary front-cull state. It precedes the water
pass as documented in worldgen-parity-gaps §9.6. Thus an emote is an
occludable world voxel model, not an always-on-top icon. The age animation
does not alter descriptor opacity or introduce a distance-alpha fade.

## 7. Shared actor visibility and bounded unknowns

The icon is inside the living-actor route of **0xd3170**. The actor must
have **HP float+0x180 greater than zero**. Scene flags bit0x01 suppress
the local-player actor; scene flags bit0x80 restrict the actor loop to
the local player. These are renderer call flags, not new NPC-dialogue
conditions. The shared actor admission branches are at
**0xd48ac–0xd48df**.

Before preparing the body and reaching the overhead-model branch, the
renderer calls **0xe08c0 at0xd5e1d**. A failed test skips the entire icon
branch. For ordinary races the center is drawn fixed XYZ
**+0xb10/+0xb18/+0xb20**. For its Z correction, negate float+0x990 first,
multiply by65536 in float32, truncate toward zero to signed64 and add that
result to drawn Z. Its sphere radius in blocks is the
larger of **body Z+0x8c** and **0.6 times max(body X+0x84,body Y+0x88)**.
The distance limit argument is **renderer float+0x4a0**. Use the existing
0xe08c0 signed-fixed XYZ early-distance gate and six-plane sphere test
from steam-scene-lighting §1; there is no separate high-icon sphere here.

Race **0x12e** is an explicit exception: its test center Z is drawn Z
plus the truncation of **float32(float+0x17c times0.5 times65536)**,
and its sphere radius is the ordinary radius plus
**float32(float+0x17c times1.2)**. All these multiplications/additions
are float32 before the fixed conversion. This exception does not change
the physical-position emote transform above.

The high model can consequently be clipped when its actor sphere fails,
and a text bubble can be suppressed by +0xc50 even when the icon is not
drawn. There is no extra text-bubble distance fade, no individual emote
screen-edge clamp, and no separate terrain-occlusion query in its actor
admission branch; geometry depth testing handles occlusion after drawing.

The exact GPU depth-state setup of the **generic GUI speech-template
pass** has not been separately traced in this audit. The verified text
placement has no terrain-depth test and uses the standard GUI subtree;
the explicit opaque world depth state above applies to the emote model,
not to that GUI template. Exact dimensions/decorations of the shipped
speech template have not been measured here: they are owner GUI data,
retained by the clone, not supplied numerically by the native attachment
or placement functions. No original-game oracle was run for this work.
