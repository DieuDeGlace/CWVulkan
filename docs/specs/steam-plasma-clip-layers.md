# Steam Plasma ordinary clip layers

Independent native-only contract for the ordinary **clipping, without blur or
Widget/node caching**, route. Sources: traversal0x330b20, layer0x33c110/
0x33e1e0, concrete surface0x3920e0/0x392730/0x392610/0x392200,
shape0x3913d0, surface copy0x392280, pipeline0x38e800/0x38fb90, and the
embedded Plasma VS0x47ebd0/PS0x47f940. No engine, tests, exporter, owner
captures or game execution was used. Existing node math/deformation remains
[Plasma scene §2](plasma-format-and-scene.md#part-2-the-scene-graph).

## 1. Admission and the depth-indexed pool

Traversal0x330b20 rejects live Display.visibility0 and disabled node flag4
before drawing the subtree. Ordinary clipping requires live clipping!=0
and neither traversal flag **4 nor0x10**. This section assumes the blur
branch and cache branches are not selected; their algorithms are not
inferred from the ordinary clip route.

The GUI engine owns a persistent pointer vector **begin+0x50/end+0x58/
capacity+0x60**, signed32 layer index **+0x70**, and current mask-surface
pointer **+0x78**. Ordinary frame traversal0x33e170 sets the index to0.
Push **0x33c110** increments it first, so the first ordinary push uses
index1. If that index is beyond the existing vector, it creates enough
surface objects through virtual+0x68, concrete factory **0x38efd0**, to
make the index available. New entries are resized using the engine's
signed32 **width+0x1c4/height+0x1c8**. They are fullscreen surfaces, not
textures sized to the clip node's shape bounds.

Already existing pool entries are reused on this push path without
another size request. They are not destroyed on pop or at every frame.
The separate window-size assignment0x349940 does not itself iterate this
pool. This bounded trace does not establish every possible external
pool-reset/recreation path following a resize; it does not silently add
a per-push resize of retained entries.

Engine destruction **0x33a440** destroys every retained pool surface and
then releases the pointer vector. Concrete surface destructor **0x392130**
releases its retained GPU views/saved target references. Balanced ordinary
push/pop and valid resource lifetimes are assumed; recovery from failed
allocation, unbalanced nesting or concurrent destruction is not specified.

## 2. Surface storage, binding and restoration

Factory0x38efd0 allocates a **0x78-byte** concrete object; constructor
**0x3920e0** stores the engine pointer at+0x38 and initializes dimensions
and view pointers to0. Surface resize **0x392730** recreates storage when
its render-target view is missing or either requested signed32 dimension
differs from **surface+0x30/+0x34**. It releases old render-target and
shader-resource views before creating the new ones.

The texture descriptor has **format28, R8G8B8A8_UNORM**, one mip, one array
slice, **one sample**, quality0, usage0, bind flags **0x28** (render target
and shader resource), CPU-access0 and miscellaneous0. No private depth
texture is created. The resize helper has no power-of-two rounding or
positive-dimension clamp; failure behavior for invalid dimensions is not
established. Surface+0x40 is its render-target view, +0x48 its sampled view.

Layer entry through **0x38ed10 → surface virtual+0x10 →0x392610** saves
the preceding **one color target, depth target and one viewport** in the
surface. It binds its own color target with **null depth**, viewport
**(0,0,surface width,surface height,0,1)**, and clears the **entire color
target to transparent black**. The GUI wrapper separately saves its
projection state and installs the projection for the requested layer
rectangle. Ordinary clip push requests origin0 and current GUI width/height.
The current node/child WORLD transforms continue to locate their geometry.

Pop **0x33e1e0 →0x38fac0 →0x392200** restores the saved color/depth targets
and viewport, releases the temporary saved target references, and restores
the wrapper's prior GUI projection. The engine then retains the just-popped
pool surface in **+0x78 as the mask source**, and decrements its layer
index. It does not immediately free that surface or replace it with an
ancestor's previous mask pointer. Another reached traversal decision can
subsequently clear or replace that mask source.

These are target/projection operations, not restoration of arbitrary WORLD
shader material/effect constants. A WORLD callback reached while the layer
is bound uses the current offscreen color target and null depth; its own
shader/pipeline contract remains separate.

## 3. Bounds-clear no-op and callback order

Generic traversal calls **0x33c920** after pushing the layer. That helper
queries the clip bounds and invokes the surface's virtual+0x40 bounds
operation. In the concrete surface vtable **0x480718**, both **+0x40 and
+0x48 point to the no-op0xe8a20**. Thus this backend does **not** perform
a partial bounds clear there. Its actual clear is the full transparent
clear in0x392610. Bounds do not become a WORLD scissor rectangle.

For an ordinary clip node, children are traversed in their existing list
order into the offscreen surface. After the pop, the node's own shape is
drawn into the restored parent target, sampling that child image according
to§5. A clip node without drawable own shape geometry does not thereby
gain a fallback full-frame mask draw.

Both own-Widget callback placements occur **after that clip child-layer
pop and own-shape mask drawing**: Widget flag0x200 selects the earlier
callback location; otherwise the later location is used. The flag's usual
before-children distinction applies to the ordinary non-clip child loop,
not to moving a clip node's hook before the already-rendered layer children.
Consequently an own WORLD hook is not masked by its own clip shape. A
descendant hook emitted while an ancestor layer is bound can participate
in that ancestor's later composition. Ancestor visibility, clipping/cache
admission and each hook's own behavior remain separate.

## 4. Geometry color and texture operands

Plasma shader construction **0x38fb90** creates VS **0x47ebd0, length0xd64**,
and PS **0x47f940, length0x980**. Restore **0x38e800** binds them, their
layout and constant buffers. The VS outputs perspective-linear color and
two coordinate pairs; no flat/centroid/sample interpolation override is
declared.

Let **C** denote interpolated vertex RGBA after the Plasma VS multiplies
the mesh's vertex RGBA by the supplied BaseColor RGBA. For an ordinary
shape, **0x3913d0** supplies the owning node's own live Display.fillColor
times that draw's engine tint. This is not inherited subtree tint. Text
can have its independently selected glyph tint; its ordinary shape/color
contract still determines C.

PS texture resources are **t0 mask surface, t1 object texture**, sampled
with **s0 and s1**, respectively. Object-texture enable is PS buffer0;
mask mode is signed integer PS buffer1; object texture parameters are PS
buffer2 in order **opacity, brightness, contrast, saturation**. The latter
parameter names describe texture adjustments, not sampler filtering.

When object texturing is disabled, base RGBA **B=C**. When enabled, sample
the object texture at the object UV and process RGB in this order:

* If saturation differs from1, move RGB about its luminance using
  **0.3/0.59/0.11**, with the supplied saturation as gain.
* If brightness differs from1, multiply RGB by brightness.
* If contrast differs from1, move RGB about0.5 using contrast as gain.
* Limit RGB to at most1; no matching lower clamp occurs here. Sampled
  alpha is not changed by these RGB adjustments.
* Form B by interpolating **C toward C multiplied componentwise by that
  adjusted sample**, using texture opacity, for **all four components**.

Texture opacity is therefore not merely a multiplier of the final alpha.
The reached helpers/setters are **0x3908f0** (object enable), **0x390ae0**
(parameters) and **0x390420** (mask mode).

## 5. Exact mask pixel modes and coordinates

With nonzero mask mode, sample t0 to obtain RGBA **M**. If **M.alpha>0**,
divide its RGB by that alpha to obtain **U**; otherwise retain the sampled
RGB as U. Keep M.alpha unchanged. This conditional unpremultiplication is
performed regardless of which draw family produced the child pixels.

|Mask mode|Final shader RGB|Final shader alpha|
|---|---|---|
|0|B.rgb|B.alpha|
|1|Blend B.rgb toward U by M.alpha|B.alpha|
|2|B.rgb multiplied by U|B.alpha multiplied by M.alpha|
|3|B.rgb|B.alpha multiplied by M.alpha|

Mode0 does not sample the mask. The compiled default branch for any other
nonzero mode uses mode3's result; the ordinary CPU selector supplies only
0–3. These modes add no discard and no final explicit shader clamp.
In particular mode1 preserves own-shape alpha, mode2 also multiplies RGB,
and mode3 uses layer alpha without its RGB. They are not interchangeable
generic alpha clips.

For a clipping shape, Display.flags bit **0x02** selects mode2; otherwise
bit **0x04** selects mode3; otherwise mode1. An ordinary non-clipping
shape selects mode0. The popped layer is bound as mask resource t0 by
surface binding **0x3921e0**.

Shape0x3913d0 builds mask coordinates from the current WORLD transform
and the reciprocals of GUI width/height; **0x390610** submits the mask
matrix. The VS evaluates that matrix using the deformed local position.
For ordinary affine geometry this is current WORLD X divided by GUI
width and WORLD Y divided by GUI height. The mask coordinates have **no
homogeneous divide**. Object UV is separate: original vertex Z/W holds
its two texture coordinates, transformed by the object texture matrix.

The Plasma VS applies **-0.13500000536441803**, word **0xbe0a3d71** at
**0x47f71c/0x47f720**, to transformed position X/Y before projection.
It does not subtract this bias from the separately calculated mask UV.
Existing Widget deformation happens before these position/mask operations;
the clip contract does not replace it with a frame rectangle.

## 6. Blend, depth, raster and unresolved sampler inputs

Pipeline construction **0x38fb90**, bound by0x38e800, establishes:

* Alpha-to-coverage off; independent blending off; target0 blending on.
  RGB uses **SRC_ALPHA / INV_SRC_ALPHA, ADD**. Alpha uses **ONE /
  INV_SRC_ALPHA, ADD**. All RGBA write bits are enabled and the sample
  mask is0xffffffff. The ordinary shader result is not premultiplied again
  before these blend factors by an extra shader step.
* Depth testing disabled. The stored depth descriptor has write mask1 and
  comparison4, but disabling depth does not create depth writes. Stencil
  is disabled. Offscreen binding still has null depth as§2 specifies.
* Solid fill, **CullMode1=NONE**, depth clipping enabled, scissor disabled,
  multisample rasterization enabled, antialiased-line mode disabled.

The reached pipeline, surface bind0x3921e0, object bind0x391f20 and
surface-composite paths do **not set sampler slots0/1**. The compiled
sample instructions use the samplers present in those slots. This audit
does not prove their initial numeric filter/address descriptors or all
external sampler writers. **Filter/address behavior remains an explicit
inherited-state input**, not an invented linear/clamp or repeat policy
derived from the shader declaration's default mode. No sampler-state
runtime oracle was obtained.

### 6.1 Bounded native provenance versus the public NULL-sampler default

Native startup **0x134cc0** obtains a newly created immediate context from
**D3D11CreateDevice**, then supplies it to the concrete graphics/renderer
construction path, including **0x38e100**. The reached Plasma initialization
**0x38fb90**, pipeline restore **0x38e800**, surface binding **0x3921e0**,
object texture upload/binding **0x391f90/0x391f20**, and ordinary surface
composition **0x392280** contain no PS sampler-slot0/1 assignment or sampler
descriptor creation. Their texture bindings assign shader-resource views,
not sampler state.

Nearby calls with the same numeric virtual displacement are not evidence
of sampler writes: **0x134f53/0x1350ac/0x1354d0** are device depth-stencil-
view creation calls, and **0x134695** operates on the swap chain's fullscreen
state. Receiver identity matters, not only the displacement.

The exported native call metadata census found **79 constant-displacement
+0x50 sites and18 +0xb8 sites**. Its inspected reached graphics/startup
subset contained no PSSetSamplers/CreateSamplerState writer. Other sites
include Game, Widget and runtime virtual methods; this bounded audit did
not exhaustively classify every receiver, indirect/computed target or
external writer. It therefore establishes **no local override on the
listed reached paths**, not an explicit initial NULL assignment and not
a proof of NULL bindings throughout every frame.

Separately, Microsoft's public **PSSetSamplers** contract defines the
behavior **when a sampler binding is NULL**:

|Property|Conditional NULL-sampler value|
|---|---|
|Filter|MIN_MAG_MIP_LINEAR|
|Address U/V/W|CLAMP for each axis|
|Mip LOD bias|0|
|Maximum anisotropy|1|
|Comparison function|NEVER|
|Border RGBA|All components1|
|Minimum / maximum LOD|−FLT_MAX /+FLT_MAX|

These are API-defined default values, rather than a sampler descriptor
found in the native constructors. The authority is
[Microsoft PSSetSamplers documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-pssetsamplers).
This conditional API rule does **not** convert absence of a reached local
writer into proof of the native binding at an arbitrary draw. No all-frame
native sampler-state oracle was obtained; unclassified/external writers
and the exact native initial binding observation remain open. The shader's
sample declaration alone does not settle those provenance questions.

## 7. Ordinary surface rectangle composition

Surface virtual+0x28 **0x392280** takes destination XY, requested width/
height, source XY and a scale argument. This section bounds its ordinary
**scale1** use; negative-scale cache policy and blur remain outside scope.
Negative requested width/height individually select the surface's full
width/height for the destination dimension.

It saves the GUI current WORLD matrix, temporarily uses identity, restores
the Plasma pipeline, enables object texture, selects **mask0**, sets all
four object texture parameters to **1**, disables Widget deformation,
and uses the GUI's current projection. The surface's sampled view+0x48
is bound to **t1**. Its per-copy color is surface RGBA at **+0x0c..+0x18**,
initialized to **(1,1,1,1)** by0x3dec10.

For scale1, destination XY/size define the screen rectangle. UV origin is
source XY divided componentwise by surface dimensions; UV extent is the
chosen width/height divided by those dimensions. Rectangle helper
**0x38f3b0** supplies the corresponding WORLD/texture matrices and draws
its existing six-index quad. The shader's ordinary position bias still
applies. The helper restores the prior GUI current matrix afterward;
it does not invent an ancestor clip mask, private depth or separate tint
inheritance. GPU constant/state restoration beyond the reached operations
is not implied by restoring that CPU current matrix.

## 8. Explicit remaining boundaries

Widget/node surface cache policy, its scale/offset/divisor fields and all
invalidation writers remain separate. The separately completed bounded
[blur-surface contract §§1–6](steam-plasma-blur-surfaces.md) specifies fullscreen
allocation/reduced projection, four concrete copies, final rebind clear and
source selection. It preserves sampler/shape/callback limits rather than
inferring a Gaussian or universal pixel appearance from virtual-method names.

The subsequently completed bounded cache consumer/lifecycle contract is
[surface caches §§1–6](steam-plasma-surface-caches.md). It retains its own
explicit setter/ownership/reset/failure limits; the blur contract is separate.

Sampler provenance in§6, external resize/reset routes for retained pool
entries, invalid/failed resource handling and exact GPU sample/interpolation
rounding remain unclosed. No owner art, geometry substitute or runtime
capture is supplied by this contract.
