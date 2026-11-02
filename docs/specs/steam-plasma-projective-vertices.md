# Steam Plasma homogeneous vertex and deformation contract

Independent native-only clarification of Plasma VS **0x47ebd0, length0xd64**,
created by **0x38fb90** and installed by **0x38e800**. It extends
[clip layers §§4–5](steam-plasma-clip-layers.md) and
[shapes and text §2.15](plasma-shapes-and-text.md#215-drawing-a-shape-matrix-and-tint).
No engine, tests, exporter, owner data or game execution was used.
This describes shader behavior, not a CPU projection or hit-test substitute.

## 1. Geometric input and ordinary path

The four input POSITION floats encode **local X, local Y, object U,
object V**. The geometric vector is reconstructed as **(X,Y,0,1)**;
the input U/V values are not geometric Z/W.

When Widget deformation is disabled, that full reconstructed vector is
the input to WORLD. There is **no homogeneous divide** before WORLD,
between WORLD and PROJECTION, or when producing the mask-coordinate pair.

WORLD produces four components. Subtract stored float32
**0.13500000536441803** independently from its X and Y results; its negative
literal has word **0xbe0a3d71** in the compiled shader at
**0x47f71c/0x47f720**. This subtraction is **not multiplied by WORLD's W**.
Preserve WORLD's Z and W, then apply PROJECTION to that full four-component
result to obtain clip position. Do not replace W with1 or commute this bias
through an arbitrary projective WORLD transform.

The later rasterizer clip-position divide is distinct from these shader
operations. The shader emits perspective-linear color and coordinate
varyings; their interpolation uses the resulting clip W in the ordinary
rasterizer path. No flat/centroid/sample override is declared. This audit
does not provide a cross-GPU arithmetic/rounding oracle or add a special
recovery rule for singular/nonfinite matrices.

## 2. Optional Widget deformation retains homogeneous components

The optional path starts from the same **(X,Y,0,1)**. Apply the Widget bind
matrix to obtain four components. The two bind X/Y values are used directly,
without division by its computed W, for the piecewise deformation:

* Compute each size difference as deformed size minus bind size, and each
  upper bind edge as bind position plus bind size.
* Below the lower bind edge, the axis fraction is0. At or beyond the upper
  edge it is1. Between the edges, subtract bind position and divide by bind
  size. The native upper/lower comparisons are strict less-than tests.
* Start each new axis at bind-transformed position plus deformed position,
  then subtract bind position. Add the fraction times that axis's size
  difference using the shader's multiply/add stage.

The bind-transformed Z/W remain intact. Apply the full inverse bind matrix
to this four-component vector, **without a homogeneous divide afterward**.
When enabled, this result is the local vector selected for WORLD and mask
coordinate generation; when disabled, select the original(X,Y,0,1).
The two divisions in the piecewise stage are by **bind width/height**,
not by homogeneous W. Native CPU point queries and inverse deformation can
have their own divisions; those are not additional stages of this VS.

Bind setup and piecewise temporary calculations precede the enable selection
in the compiled shader. Only the selected local vector supplies the later
position/mask stages. The finite valid-dimension behavior here does not
specify a new fallback for zero/negative bind sizes or invalid matrices.

## 3. Mask coordinates, object UV and color

The selected local four-component vector supplies the two mask-matrix dot
results. There is **no mask homogeneous divide** in the VS or a new mask
UV divide inserted by the PS. This is separate from the mask pixel formula's
conditional RGB/alpha unpremultiplication already described in clip§5.

Object UV always comes from the **original input U/V**, including when
Widget deformation is enabled. The object texture matrix transforms the
affine three-component UV vector(U,V,1) to its two outputs; geometric WORLD
or bind W is not substituted as its third component. Both coordinate pairs
are subsequently interpolated under the declared perspective-linear mode.

The color output remains input vertex RGBA times supplied BaseColor RGBA.
The shader does not use homogeneous W to modify that product. Plasma pixel
texture adjustment, mask modes, blend, depth and sampler inheritance remain
the existing clip contract, rather than a separate projective pixel shader.

## 4. Ordinary shape producer: descendant-to-owner mappings

The ordinary admitted resource draw **0x3913d0** uses shape node N's
**owner Widget pointer N+0x78**, not necessarily its own Widget N+0x70.
Initialization0x333820 supplies the inherited owner as described in
[scene §2.7.9](plasma-format-and-scene.md#279-native-initialization-and-button-state-sequences).
Let H be that Widget's bound node **Widget+0x178**, B its stored full bind
matrix **Widget+0xd0**, and Bi its stored full inverse bind matrix
**Widget+0x110**. Let WN/WH be the complete node world products returned by
**0x100fa0**, and IN/IH the complete inverse-world products returned by
**0x100ee0** for N/H. These queries include their respective ancestor chains.

The concrete uploaded matrices, in the established column-vector convention,
are:

* **Bind = (B · IH) · WN.** First multiply a copy of B by IH, then multiply
  that result by WN. It maps shape-node local coordinates into the owner's
  bind space even when N and H are different nodes.
* **InverseBind = (IN · WH) · Bi.** First multiply IN by WH, then multiply
  that result by the Widget's stored Bi. It maps the deformed bind-space
  four-vector back into shape-node local coordinates.

The four float32 matrix products use0x0173e0 at
**0x3916e3/0x391716/0x391750/0x391783**. Preserve these groupings; the
second matrix is constructed from those independently queried/stored inputs,
not obtained by numerically inverting the first uploaded product. No CPU
homogeneous divide occurs between these matrix products.

Bind is supplied through **0x390bd0 at0x391793** to VS buffer5, with CPU
cache **engine+0x50c**. InverseBind follows through
**0x3904a0 at0x3917a3** to VS buffer6, with CPU cache **engine+0x54c**.
The setters transpose all16 floats for the shader's row-dot layout. They
have a source-pointer-identity shortcut, not a matrix-value equality test;
this ordinary caller passes temporary completed matrices.

Admission requires a nonnull owner, a nonnull node shape whose deformable
query virtual+0x28 succeeds, and at least one difference between the
owner's bind size0x32ad60 and effective deformed size0x32adf0. The native
component comparison also treats unordered values as different. A pure
deformed-position change with equal sizes does not enable this GPU branch.
Enabled setup uploads Bind, InverseBind, enable1, then **bind position,
bind size, effective deformed position, effective deformed size**, through
0x390d40/0x390e20/0x390260/0x390340. The position/size producers are
0x32ad50/0x32ad60/0x32ad70/0x32adf0; their stored margins/constraints remain
the existing Widget geometry contract.

When this admission fails, it requests enable0 through0x390f00; it does
not replace the old bind matrices with identity. The outer resource-ready
guard of0x3913d0 precedes this setup, so an unadmitted resource draw does
not run these deformation setters.

**WORLD remains separate.** Before deformation preparation, the routine
uploads the current CPU GUI projection through0x390780 at0x39155b and the
current GUI WORLD **engine+0x290** through0x390f80 at0x39156e. This is the
draw traversal's current transform, including any reached temporary draw
adjustments; it is not rebuilt here from WN. Do not substitute the pure
tree query WN for current drawing WORLD under a surrounding cache or other
temporary transform. Shader order is descendant local → Bind → piecewise
deformation → InverseBind → current drawing WORLD → XY bias → PROJECTION.
The full homogeneous components survive each indicated stage as specified
above. CPU picking's divided coordinates are not these GPU constant inputs.

## 5. Ordinary GUI projection Z/W and layer preservation

Admitted GUI entry **0x33e170** calls engine virtual+0xa0, concrete
**0x38ea80**, before root drawing. That entry resets current CPU WORLD to
identity and builds the fullscreen CPU projection at **engine+0x2d0**.
For positive current GUI width/height converted to float32, its nonzero
column-major entries are:

| Entry | Value |
|---|---|
|0|float32 2/width|
|5|float32 2/(0−height)|
|10|**−9.999998695775503e−8**, word **0xb3d6bf93**|
|12|−1|
|13|+1|
|14|**+9.999998695775503e−8**, word **0x33d6bf93**|
|15|1|

All other entries are0. The Z coefficient and translation are installed
as literal words at **0x38eb3d/0x38eb70**. For the already WORLD-transformed
four-vector, projection's Z is its Z times entry10 plus its W times entry14;
projection's W is that input W. Thus more-negative geometric Z is farther
under this mapping. The nominal near/far geometry is **Z=+1 /Z=−10000000**
when W=1, corresponding to D3D clip depth0/1. Stored float words and GPU
dot rounding, rather than an ideal real-number endpoint assertion, govern
actual boundary results. This explicit negative-Z-forward convention is
more precise than assigning a generic left/right-handed projection label.

Layer projection wrapper **0x38ed10** saves the current full projection,
then calls **0x390050** with near parameter **−1** and far parameter
**10000000** in both reached branches. That builder sets entry10 to
float32 **1/(near−far)**, entry14 to float32 **near/(near−far)** and entry15
to1, with entries3/7/11 zero. These parameter values produce the same
two Z words listed above. The wrapper's subsequent translations have Z0
and its subsequent scales have Z1, so its reduced/offset XY projections
preserve that Z/W mapping. Layer pop0x38fac0 restores all16 saved CPU
projection floats, including Z/W; it does not reconstruct an assumed
default projection instead of the saved one.

These are CPU constant production and shader mapping facts. They do not
change the separately specified null-depth layer binding, depth-stack
translations, DSV admission, depth comparison/write rules or failure cases.
