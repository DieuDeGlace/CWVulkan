# Steam character-selection card: immediate 3D preview

Stable bounded native-only contract, 2026-11-01. Sources: card builder
**0x107440**, constructor **0x26ea60**, draw **0x26eba0**, immediate helper
**0xdb630**, pose **0x17ab0**, descriptor consumer **0xe6e60**, normal scene
**0xe2e00** and the reached Plasma draw/binding routines. No game execution,
implementation, SDK, owner assets or captures were inspected.

This is the saved-character list card. The creation screen's live player
and scene's separate optional preview at0xe4bfb retain their own contracts.
Card construction, binding transfer and placement remain in
[frontend characters §2](frontend-characters.md) and
[character-list input/frame initialization](steam-character-list-input-and-frame-init.md).

## 1. Record and callback ownership

Builder0x107440 visits the saved-character pointer array at **Game+0x1778**
in increasing index order, followed by one extra null entry. It creates a
blackwidget instance and a new card wrapper, then stores the corresponding
creature pointer directly at **widget+0x1e8**. The Game pointer is stored at
**widget+0x1f0**. This is a borrowed reference to the cached full character
record, not a per-frame copy, newly generated avatar or lookup by creature
ID. The normal local player is a separate record.

Constructor0x26ea60 initializes +0x1e8 to null and loads the existing
`resource1.dat` font resource through0x269130. Its destructor route
**0x291dc0 → 0x329500** destroys the Widget, not the borrowed creature.
The list rebuild/removal lifecycle remains the frontend contract; no
independent retained snapshot is introduced by this drawing helper.

Draw0x26eba0 is the card's own Widget callback. The null entry draws
the existing New character text and does not call the pose or3D helper.
For a nonnull record, the sole resolved direct caller of0xdb630 is
**0x26f22e**. It supplies **Game+0x18's renderer**, the cached creature's
animation object at **creature+0xc38**, and the inputs in §§3–4 below.

## 2. Admission, clocks and persistent record mutation

### 2.1 Draw admission and two intervals

The card is reached through ordinary Plasma traversal0x330b20. Hidden
ancestors, live Display.visibility0 or node flag0x04 can prevent its draw.
Cache reuse can also skip the callback. There is no additional HP>0,
World registration, distance, sphere/frustum or card/window-intersection
test around the nonnull card's0x26f22e call. Hardware clipping remains.

The card-side updates use the GUI engine's **signed32 last-step milliseconds
+0x190**, converted to float for arithmetic or double for0xa7a0. Its producer
is the current float Interface step truncated toward zero once, after the
ordinary Game elapsed cap/narrowing. See
[GUI clock production §16.1](steam-gui-item-icons.md#161-producer-of-recipe-detail-step0x190).
Skipped card draws do not accumulate a separate hidden-time catch-up.

The pose receives a different input: **renderer float+0x330**, loaded at
**0xdb7ac**. Normal scene0xe2e00 stores its float frame-ms argument there
before GUI rendering. It retains fractional milliseconds, unlike GUI+0x190.
Thus for a scene step16.75, card-side updates use integer16 while pose uses
float16.75. Neither is World+0x440 or the world calendar.

### 2.2 Updates before pose

On each nonnull callback, write zero to the creature's32-bit fields
**+0x12c / +0x130 / +0x5c**, its **16-bit +0x128**, and **byte +0x68**.
Advance float **+0x99c** by float(step) times stored float **0.0075**
(word0x3bf5c28f at0x42e6e4), with float multiply then float add.

Walking/turning uses **0x32b0c0**, the direct card-subtree pointer test
through0x32ae90. It is distinct from the character-selection hover query
in0x103200; it is not a World ray, button-held test or current creature
target. It uses the GUI's cached pointer and the node's hit geometry.

All these card easing calls use **0xa7a0(double(step), float0.01)**,
with float0.01 word0x3c23d70a at0x42e51c, rather than adopting another
caller's coefficient. The helper's double accumulation/float return is
[creature upkeep §4.9](creature-upkeep.md#49-smoothing-factor-0xa7a0-all-in-double).

While the pointer test is true, subtract float(step) times float **0.05**
(0x3d4ccccd at0x42e524) from drawn yaw **+0xb3c**, without a separate
wrap/clamp here. Set **+0x998 to0.2**. Ease float vector **+0x40/+0x44/+0x48**
toward **(0,8,0)**, and **+0x34/+0x38/+0x3c** toward **(0,0,0)**.
Otherwise ease yaw toward0 with angle helper0x17840 and the same coefficient,
ease +0x998 toward0, and ease both vectors toward0. Scalar/vector updates
are float target-minus-old, multiply by the returned float coefficient,
then add old. The yaw helper retains its own angular behavior.

These mutations and the pose's cached fields persist in the borrowed
record. Clearing the listed creature fields does not reset the entire
animation object or perform normal world renderer0xd3170's ability-to-pose
state/clock assignment. In particular this caller does not first copy
ability clock+0x6c into animation+0x08 or overwrite animation state+0x0c.

## 3. Screen anchor, scale and model base

The callback builds **M** from the node parent's world matrix through
0x100fa0, or identity when there is no parent, followed by the card node's
current Transformation matrix **+0x2a0** through0x173e0. Its selected pivot
**Q=(qx,qy)** is read from that Transformation's float2 channel array
**+0x278**, indexed by signed32 **+0x244**. No extra Widget bind inverse
or raw frame-origin subtraction is applied at this callsite. The native
binding/layout process has already determined the node's current matrix.

For clarity, the following offsets refer to float components in M's64-byte
matrix. Compute a float reciprocal of
**(M+0x0c)×qx + (M+0x1c)×qy + (M+0x3c)**. Multiply it into the X numerator
**(M+0x00)×qx + (M+0x10)×qy + (M+0x30)** and Y numerator
**(M+0x04)×qx + (M+0x14)×qy + (M+0x34)**. Products/sums and the reciprocal
are float; each displayed numerator/denominator adds its two products
before the translation component.

The submitted screen center is that projected pivot, plus **M+0x00 times100**
in X and **M+0x00 times110** in Y. Each offset multiply precedes its final
addition. The submitted uniform scale is exactly **M+0x00**. It is not a
basis length, determinant, absolute scale or queried effective card width.
GUI scale is already present in M. The Euler vector is **(−95°,0°,45°)**.

Inside0xdb630, a fresh local draw context is initialized by0xeddd0. Its base
matrix begins as **translation(0,0,17)** (constant0x43f1a0), then appends
X, Y, Z rotations in that order through **0x2b360 / 0x2b470 / 0x13050**.
If submitted scale differs from1, its first three matrix columns, all four
components each, are multiplied by that float; translation is retained.
The context stores the submitted screen center separately at **+0x90/+0x94**.

Part submissions through0xee2f0/0xee990 combine that context base with
each pose-produced part matrix, and copy the context's screen center into
their descriptors. They do not add the screen pixels to world/model XYZ.
There is no actor-AABB fit to600×220, preview-specific allocation centering
or size derived from the card height. Body/part centering and scale remain
the ordinary [pose](creature-pose.md) and held-item contracts.

## 4. Exact pose call and temporary origin

The0x17ab0 call at **0xdb7c0** has these inputs:

| Input | Card value |
| --- | --- |
| Animation object | Borrowed creature+0xc38 |
| Output context | Fresh local context from §3 |
| Resource/model table | Renderer.World+0x70 |
| dt | Current renderer float+0x330 |
| Tint and alpha | White RGBA(1,1,1,1), separate alpha1 |
| camX / camY | Pointers to signed64 zero values |
| Hide mask | 0 |
| Ridden-creature pointer | Null |
| Shader depth input | Float0 |
| Body uniform-scale argument | Float1; GUI scale is in the context base |

Immediately before this call,0xdb630 saves the three signed64 drawn
coordinates **creature+0xb10/+0xb18/+0xb20** and writes zero to them.
Immediately afterward it restores those three saved values. Physical
coordinates are not moved. This is not normal world's final-eye/look-at
origin policy. Pose/cache writes other than those restored drawn coordinates
remain: no whole-creature or cached-matrix rollback follows the call.

The normal scene actor gate and optional scene-preview path are not called
by this helper. It supplies no independent class/quest/buff overhead pass.
The ordinary pose/held-resource rules still determine the emitted parts.

## 5. Projection actually consumed by the draw

0xdb630 first uploads identity model/view and a locally constructed **45°**
perspective: half-angle float0.39269909262657166 at0x43ef54, negative
float(W)/float(H) aspect, positive view Z, near0.1/far1000. Its depth words
are **1.000100016593933** and **−0.10001000016927719**. W/H are renderer's
signed32 full-window fields **+0x41c/+0x420**.

That upload is not the final per-part projection. At **0xdba18** the helper
calls **0xe6e60 with mode1**. For each ready descriptor, this consumer
replaces projection using **renderer+0x3dc**, at matrix combinations
**0xe7315 / 0xe7821**. The initial upload through
**0xc5a30 → 0xc6fc0 → 0xc7240** changes shader state, not that renderer cache.

On the ordinary **scene→GUI** route,0xe2e00 has just replaced +0x3dc with
the **50° screen-overlay projection**, through0xa0340 before its queued
overlay draws at0xe4f10/0xe4f22 and GUI traversal. Its near is0.05 and far
is float of signed32(renderer+0x438 shifted left6); its aspect is negative
float(W)/float(H). This cached projection therefore supplies the actual
card descriptors on that reached route. See
[queued overlay projection §4](steam-ring-selector-rendering.md#4-queued-draw-projection-state-and-ordering).
An independently called helper would use whatever +0x3dc it actually retains.

Mode1 applies a homogeneous screen shift on the left of that cached
projection. X is **(screenX−float(W)×0.5)/float(W)** then added to itself.
Y is **(screenY−float(H)×0.5)/float(H)** times float−2. Depth is **−0.5**
in NDC: clip Z is shifted by −0.5 times clip W. Positive view Z remains
clip W and the native depth interval is0..1. The per-part model matrix
retains the context's17-unit translation; the depth shift does not subtract
0.5 from model/view Z.

## 6. Fixed shader inputs and immediate consumption

After pose preparation,0xdb630 sets **Opacity1, White0, DepthOffset0,
Shininess0, Glow0, white material and fog range10000**, and0xce580 binds
zero point tables. It supplies **front(1,1,1,1)**, **back(0.2,0.2,0.2,1)**
and **ambient(0.4,0.4,0.4,1)**. For direction, float a is
**1/sqrtf(float1.2)**; submit float **(a×0.2,a×0.4,a×−1)**. Setter0xc6ce0
then performs its existing float normalization again. The stored constants
are at0x42e718,0x42e700,0x42e52c and0x4aca4c.

It also submits **white RGBA for all four Sky1/Sky2/Fog/Sun colors** through
0xc5a70→0xc7440. It does not submit a new SunPos or fog-gradient angle;
those inputs retain their earlier values. This is ordinary WORLD shading,
with its existing color/lighting/grade/fog operations, not an unlit image.

The immediate0xe6e60 mode1 call consumes the local context's opaque normal
and opposite-winding resource trees. It does not use global screen queue
renderer+0x18. Resources group in unsigned pointer-key order; descriptors
within one resource group retain submission order. Ready admission is
**0x16eba0**; absent/unready vertex or index buffers omit that group.
The consumer sets each descriptor's own material, effects, opacity and
model/projection as in [WORLD controls §2](steam-world-pass-controls.md#2-ordinary-queued-model-controls).
Mode bit2 is absent, so it does not select world-zone point tables.

Its normal state0 and conditional opposite state1 use ordinary WORLD shaders,
LESS_EQUAL depth testing/writing and disabled alpha blending, with the
verified front-cull winding families. There is **no faded consumer0xe7980**
between this draw and context destruction **0xcc540 at0xdba22**. Any queued
faded records are therefore not drawn by this helper. There is no additional
per-card render target, viewport, scissor, depth clear or depth-disabled pass.

### 6.1 Exact pose-descriptor classification

Both **0xee2f0** and **0xee990** classify on their **separate float scalar
alpha, argument5**, following the material-RGBA pointer. For ordinary finite
input the complete routing is:

| Scalar alpha |0xee2f0 destination|0xee990 destination|
| --- | --- | --- |
| At most0 | No descriptor | No descriptor |
| At least1 | Normal opaque resource tree, context+0x00 | Opposite opaque resource tree, context+0x10 |
| Strictly between0 and1 | First faded vector, context+0x20 | Second faded vector, context+0x38 |

The zero comparison/exit is at **0xee328 / 0xee335** and
**0xee9c8 / 0xee9d5**; the comparison against float1 and opaque selection
are at **0xee34a / 0xee376** and **0xee9ea / 0xeea16**. There is no clamp
of values above1. Positive fractional values keep their supplied alpha
when appended through0xee230; they are not promoted to opaque.

The four submitted **material RGBA components are copied unchanged in both
routes**. Material.a is not the classification input and is not forced to1
in these helpers' faded route. The separate scalar alpha is stored as the
descriptor's pixel Opacity. For example, scalar alpha1 with material.a0.25
enters the selected opaque tree; scalar alpha0.5 with material.a1 enters
its faded vector, which this card helper never consumes.

Neither enqueue routine reads the sprite's record, voxel alpha, geometry,
material category or sprite flags to choose opaque versus faded. The sprite
pointer is the opaque grouping key or the faded record's resource pointer.
Normal versus opposite winding is selected by calling0xee2f0 versus0xee990;
it is not inferred from a sprite flag or the model matrix here. Resource
readiness is checked later by0xe6e60 as specified above.

The card's pose call supplies alpha1. A reached part submission forwarding
that value takes its helper's opaque route even if its material.a differs.
Any part producer submitting a smaller positive scalar instead takes the
unconsumed faded route. This subsection specifies enqueue admission, without
asserting a single alpha for every separate effect/part producer.

**Pose-facing argument order**, identical for0xee2f0 and0xee990: arguments
1–4 are context, sprite pointer, model-matrix pointer and material-RGBA
pointer. The following five float inputs map directly as follows:

| Position after material | Absolute argument | Consumer meaning | Opaque list-node offset | Faded-record offset |
| --- | --- | --- | --- | --- |
| First | 5 | Opacity; scalar used by the classification above | +0x60 | +0x58 |
| Second | 6 | Glow | +0x64 | +0x5c |
| Third | 7 | Shininess | +0x68 | +0x60 |
| Fourth | 8 | White | +0x6c | +0x64 |
| Fifth | 9 | DepthOffset | +0x70 | +0x68 |

These inputs retain this field mapping on both winding routes and
both queue classifications; material.a remains the fourth component of
the separately copied RGBA. In the earlier pose descriptions' five-number
notation, the number called **x9** occupies the **Glow** slot, and the
third number sometimes called a weapon's glow occupies **Shininess**.
Those producer labels do not reorder the descriptor's shader inputs.
The stored-field consumption is0xe6e60 at0xe7328–0xe7381 and its opposite
branch, as specified in WORLD controls §2.

## 7. GUI order, clipping and bounded cache behavior

The new card wrapper retains Widget flags0 through its reached constructor;
neither0x268a70 nor0x26ea60 enables own surface-cache flag0x40 or the
before-children draw flag0x200. Ordinary0x330b20 consequently draws its
shape/children before the own callback. The callback updates the creature,
draws3D immediately, then invokes **GUI virtual+0x80 → 0x38e800** before
submitting the card's name/level/specialization text. That routine rebinds
Plasma's GPU pipeline/buffers/states. It does not undo the creature's pose
updates or restore the previous WORLD color/effect buffers.

The immediate draw retains the target/depth/viewport current at callback
entry. On the normal main-target route this loads the frame's existing
depth; no card-private clear occurs. Plasma ancestor clip/blur/cache
composition remains [scene §2.3.1](plasma-format-and-scene.md#231-draw-traversal-0x330b20)
and [GUI targets §8.2](steam-gui-item-icons.md#82-viewport-node-admission-and-plasma-clipping).
WORLD does not consume Plasma mask constants or inherit a slot scissor.
The own callback is after the node's own clip-child composition; that
does not itself mask the callback by its own blackwidget shape.

The cloned node retains its data-owned node flags, and ancestors can have
their own surface policy. Such a reached surface can contain the emitted
draw and later composite it, or cache reuse can skip the callback. This
audit does not assert numeric owner node flags, invent a new card cache,
or complete every alternative offscreen-cache coordinate path. The precise
main-target callback, state restoration and conditional composition facts
above are the bounded contract.
