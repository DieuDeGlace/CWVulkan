# Steam Plasma depth state, optional counter and surface-copy distinction

Independent native-only bounded contract from GUI constructors0x339b70/
0x38e100, GUI entry0x33e170/0x38ea80, drawing0x3913d0, pipeline0x38fb90/
0x38e800, surface0x392610/0x392200/0x392280 and rectangle0x38f3b0.
No engine, tests, exporter, owner data or game execution was used.
Projection's exact Z/W coefficients remain
[projective vertices §5](steam-plasma-projective-vertices.md#5-ordinary-gui-projection-zw-and-layer-preservation).

## 1. The optional depth-stack bit starts off

This draw option is **mask0x08 of GUI engine+0x288**. Constructor write
**0x339f4e sets that unsigned32 word to7**, so mask0x08 initially is **off**.
It must not be conflated with the initially enabled masks0x01/0x02/0x04.
The reached temporary Widget-cache writes alter only mask0x01 and restore
its entry value; they preserve0x08. The bounded option-write census and
its alias/external limits remain [GUI options §§1–2](steam-plasma-options-and-shape-bounds.md).
This audit found no reached ordinary startup setter enabling0x08.

The bit's tested consequence below is a WORLD-transform adjustment and
counter increment. It does **not** select a different Plasma depth-stencil
state or enable depth testing.

## 2. Admitted shape draws and the shared counter

The raw drawing method **0x3913d0** first requires its two GPU buffers
**+0x1e8/+0x1f0 nonnull** and index count **+0x1fc nonzero**. It restores
the Plasma pipeline and saves the current CPU WORLD on its matrix stack.
Only with both a **nonnull node argument** and option mask0x08 set does
it perform the following additional reached operations:

* Read the **32-bit global at0x5535b0** as signed32 and convert it to
  float32. Multiply by float32 **−2**, word **0xc0000000 at0x42e60c**.
* Postmultiply current CPU WORLD **engine+0x290** by translation with X/Y0
  and that Z offset, through **0x02b7a0 at0x3914d5**.
* Increment the underlying32-bit global at **0x3914da**. The increment
  follows the translation and precedes projection/WORLD uploads and draw.

The conversion/read is at **0x3914b5–0x3914cd**. Integer increment wraps
the32-bit storage; the next conversion interprets its current bits as
signed32. There is no saturation or draw-local reset in this branch.
For ordinary finite counter values0,1,2 the offsets are respectively
negative zero,−2,−4. This is **per admitted raw drawing call**, not one
increment per scene node or frame. A single higher-level shape can make
separate extrusion/fill/stroke calls. Null-node draws and failed outer
resource admission do not consume the counter.

This is a full matrix post-translation, not merely a write to the Z
translation component. Under a noncanonical third WORLD basis column,
the helper can change translation X/Y/W too. The separate ordinary
projection/WORLD uploads follow at0x39155b/0x39156e; the saved CPU WORLD
is restored after drawing. The restored CPU matrix is not an automatic
restoration of every prior shader constant.

The counter is **global storage, not a field of each GUI engine**.
Its RVA lies in the image section's virtual zero-fill tail, so an
unmodified image starts it at0. A full native text direct-reference census
found the read above and that increment, with **no reset reference**.
GUI frame0x33e170, its0x38ea80 WORLD/projection reset, and layer begin/pop
do not reset this counter. The GUI frame's separate traversal/pool counters
are not evidence for resetting0x5535b0. This is bounded native static
evidence, not a proof against untraced computed aliases, bulk writes,
external/injected writers or arbitrary runtime entry state.

## 3. Depth-state binding is separate from the bound DSV

Concrete initialization **0x38fb90** creates the Plasma depth-stencil
object **engine+0x388** from native descriptor **0x480520**:

| Field | Stored value |
|---|---|
|DepthEnable|0|
|DepthWriteMask|1|
|Depth comparison|4|
|StencilEnable|0|

Pipeline restore **0x38e800** binds this object with stencil reference0.
It does so independently of mask0x08 and independently of whether the
current render target has a DSV. Disabling depth does not produce depth
writes merely because the stored write mask is1. The existing blend,
rasterizer, shader and sampler facts remain [clip layers §6](steam-plasma-clip-layers.md#6-blend-depth-raster-and-unresolved-sampler-inputs).
The ordinary rasterizer's enabled depth clipping is separate; disabled
depth testing does not imply that arbitrary clip Z/W survives rasterization.

On the normal main-target route, GUI root drawing inherits the window's
main **D32_FLOAT DSV** and its current contents. Graphics creation0x134cc0
and resize0x135310 bind that target; the outer frame0x134420 clears its
depth to1 before entering Game. There is no separate GUI-entry/per-shape
depth clear. This is not a claim that every earlier/later callback leaves
that same target bound. Existing ordinary scene/overlay and GUI WORLD
callback contracts govern their own reached changes.

For an offscreen surface, **0x392610** saves the prior color target, DSV
and viewport, then binds the surface's color target with **null DSV**, sets
its viewport and clears color transparent. It neither creates a private
depth buffer nor changes the bound depth-stencil state object. Surface end
**0x392200** restores the saved color target, DSV and viewport; it does
**not** restore a saved depth-stencil state object or depth contents.

These two independent lifetimes matter: a WORLD callback can change the
depth-stencil state while a null-DSV layer is active, and that state need
not be the one bound before the layer after its DSV is restored. A later
admitted Plasma shape or surface composition explicitly restores Plasma.
The GUI entry0x38ea80 only resets CPU matrices, while GUI exit virtual+0xa8
is a no-op in this backend. Neither is a mandatory depth-state restoration
for an otherwise empty traversal or after its last callback. No broad
WORLD-callback state equivalence is established here.

## 4. Rectangle copies do not consume the shape depth counter

Ordinary surface copy **0x392280** dispatches to concrete rectangle helper
**0x38f3b0**, which uses the engine's retained rectangle vertex/index buffers
**+0x350/+0x358** and directly requests **six indices**. This route never
calls0x3913d0. It neither tests mask0x08 nor reads/increments0x5535b0.
No replacement native counter increment appears in the reached surface,
rectangle or constant-setter sequence. Native driver statistics are not
an additional GUI counter contract.

If either rectangle buffer is null,0x38f3b0 skips its buffer/draw body;
the preceding surface-copy setup is still reached. In the valid-buffer
case the rectangle builds and submits its own full WORLD, texture matrix
and BaseColor, binds the retained buffers and draws. Surface-copy setup
restores Plasma, configures object texture and texture adjustment, selects
mask0, disables deformation and submits the current projection. Afterward
0x392280 restores its saved **CPU current WORLD**, without undoing all
those retained shader/pipeline/texture/buffer effects.

Consequently the four intermediate copies in
[blur surfaces §4](steam-plasma-blur-surfaces.md#4-four-generic-passes-and-the-actual-copy-operation)
do **not** consume the raw shape depth-stack counter, despite their draw
attempts. Their target clears/bindings, projection and other state effects
remain real reached work. The mandatory later clear of source A is not
evidence that the preceding calls were side-effect-free.

## 5. Explicit boundaries

This closes the reached default bit, counter arithmetic/admission and
direct-reference lifetime, the ordinary Plasma depth descriptor, layer
DSV versus state restoration and the surface-copy counter distinction.
It does not trace every WORLD callback, every external pipeline writer,
device reset/failure, custom dynamic class or concurrent entry. No missing
reset, private GUI depth clear, depth-enabled mask0x08 state or universal
callback restore is inferred from those open domains.
