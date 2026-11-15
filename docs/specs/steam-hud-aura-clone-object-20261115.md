# Steam HUD aura clone: identity of the per-node object at +0x60

Image identity: Steam Cube World x64, unpacked image
`cubeworld.original.exe.unpacked.exe`, SHA-256
7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (hash
recomputed during this task), preferred base 0x140000000. All addresses below are
RVAs relative to that base.

Role: isolated source-side specification author. Content is code-free behavioural
prose derived from the original image and its decompiled export only.

## Summary

- The object at scene node +0x60 is the node's Plasma **Transformation** object
  (translation, deformation, rotation, pivot, plus a cached local matrix and its
  inverse). It is not the Display object; the Display lives at node +0x68 and the
  Shape at node +0x58. PROVED.
- Multiplying the first two 16-byte rows of the current deformation record by 3
  scales the node's geometry by 3 along its local x and y axes, about its pivot,
  before rotation and translation. It does not touch colour. PROVED (matrix
  algebra in the recompute routine). The renderer's own vertex application was not
  inspected.
- 0x106C20 sends no change notification after the multiply. The cached matrices
  are rebuilt only if a later placement call through 0x334700 → 0x334510 finds
  that the node's translation actually changes. That routine skips the rebuild when
  the new translation equals the stored one. PROVED for code inside 0x106C20 and
  0x334510. Whether some other per-frame path rebuilds it is UNPROVED.
- The object scaled is the same object that 0x335830 has just made private
  (copy-on-write) for the clone. PROVED.

## a) What node +0x60 is

### Node construction

- The scene-node constructor 0x32EE60 (node object size 0xC8, allocated by the
  node factory 0x33CD30) stores three scene-object pointers:
  - node +0x58: the 4th argument (Shape). The node registers itself in that
    object's user set at object +0x80.
  - node +0x60: the 3rd argument (Transformation). The node registers itself in
    that object's user set at object +0x290.
  - node +0x68: the 5th argument (Display). The node registers itself in that
    object's user set at object +0x320.
- The factory 0x33CD30 handles a null transformation argument by allocating
  0x328 bytes and constructing them with 0x372BD0. It handles a null display
  argument by allocating 0x338 bytes and constructing them with 0x3736C0. It then
  allocates the node and passes the transformation as the node's +0x60 argument
  and the display as the node's +0x68 argument.
- Template cloning 0x333890 calls the factory with the template's own +0x60,
  +0x58 and +0x68 pointers. The clone therefore starts by **sharing** the
  template's Transformation, Shape and Display. It then recurses over the
  template's children and copies the template's flags at +0x80 and further
  fields.

### Plasma document loader

- The element dispatcher 0x3809A0 matches element names. "Transformation"
  (string at 0x47E248) is parsed by 0x385F50, which creates the object through
  0x33DB70 → 0x372BD0. The result is stored in a loader table at loader +0x70,
  keyed by a running index. "Display" (string 0x47E258) is parsed by 0x382130,
  which creates the object through 0x33CAA0 → 0x3736C0 and stores it in loader
  table +0x80.
- The node element reader 0x382F00 resolves three node references:
  - "Node.shape" (0x47E318) through loader table +0x60
  - "Node.transformation" (0x47E328) through loader table +0x70
  - "Node.display" (0x47E340) through loader table +0x80
  It passes them to the node factory 0x33CD30 as the transformation, shape and
  display arguments respectively. So a loaded node's +0x60 is the Transformation
  that its "Node.transformation" field names.

### Transformation constructor and vtable

- Constructor 0x372BD0 runs the common scene-object base constructor 0x368D90
  and installs vtable 0x47C0F0. The vtable has three code slots:
  - slot 0 0x373270: destructor
  - slot +0x08 0x3733F0: recompute matrices
  - slot +0x10 0x373310: clone
- The constructor builds four named attribute sub-objects. Each is an embedded
  attribute record with a common base built by 0x34A420. Within each record, the
  current state index is the 32-bit value at record +0x3C and the per-state value
  array pointer is at record +0x70. Each attribute is registered on the owner's
  attribute list at owner +0x38 by 0x368FA0.

  | Attribute name (string RVA) | Record at | State index at | Value array at | Element size | Initial state count, default |
  |---|---|---|---|---|---|
  | "translation" (0x47C108) | +0x70 | +0xAC | +0xE0 | 8 bytes (2 floats) | 2 states, zero |
  | "deformation" (0x47C118) | +0xF8 | +0x134 | +0x168 | 64 bytes (4×4 floats) | 2 states; current record then set to identity |
  | "rotation" (0x47C128) | +0x180 | +0x1BC | +0x1F0 | 12 bytes (3 floats, degrees) | 2 states, zero |
  | "pivot" (0x47C134) | +0x208 | +0x244 | +0x278 | 8 bytes (2 floats) | 2 states, zero |

  Translation and pivot share attribute vtable 0x47BD50. Deformation uses
  0x47BDD0 and rotation uses 0x47BD90.
- Further fields:
  - +0x290: set of nodes using this Transformation; its count is at +0x298.
  - +0x2A0: 4×4 local matrix, initialised to identity.
  - +0x2E0: 4×4 inverse of the local matrix, initialised to identity.
  - +0x320: link into the document's transformation registry. The registry count
    is at document +0xE8, and an object is removed from the registry by 0x33E040.

### Recompute routine (vtable +0x08, 0x3733F0)

This routine treats a 4×4 matrix as four 16-byte rows (row 0 at +0x00, row 3 at
+0x30). Composing a factor A onto the matrix M replaces each row i of M with the
sum over k of A[i][k] times row k of M. For example, composing a translation adds
x times row 0 plus y times row 1 to row 3.

Starting from identity in +0x2A0, the routine composes these factors in order:

1. Translation by the current translation value.
2. Translation by the current pivot value.
3. Three rotations by the current rotation triple, each converted from degrees
   with the factor 0.01745329 (RVA 0x42E520). The rotations go through 0x2B360,
   0x2B470 and 0x13050, one per component.
4. The current 64-byte deformation record (state index +0x134 into the array at
   +0x168), composed by the general 4×4 product 0x173E0.
5. Translation by the pivot times −1 (constant at 0x4ACA4C).

After that it flattens the z column of the result: column 2 of rows 0, 1 and 3
becomes 0 and row 2 column 2 becomes 1. It copies the 64 bytes to +0x2E0 and
inverts +0x2E0 in place with 0x0DFC30, an augmented-elimination 4×4 inverse that
returns success. Finally it sets the byte at object +0x49 to 1, which appears to
be a "matrix changed" flag (the base constructor clears the word at +0x48).

Answer for a): **+0x2E0 is the inverse of the (z-flattened) local matrix**. The
local matrix itself is at +0x2A0.

Clone (vtable +0x10, 0x373310): creates a fresh Transformation in the same
document through 0x33DB70. It copies all four attributes, including their value
arrays (+0xE0, +0x168, +0x1F0, +0x278), then calls the new object's recompute
slot +0x08.

Status for a): PROVED. The node at +0x60 holds the Transformation. Per-state
arrays: +0x134/+0x168 is deformation, +0xAC/+0xE0 is translation and
+0x244/+0x278 is pivot. +0x2E0 is the inverse local matrix.

## b) Visual effect of the ×3 and whether matrices are rebuilt

### Effect

0x106C20 multiplies rows 0 and 1 of the current deformation record
component-wise by (3, 3, 3, 3). The vector is at RVA 0x441C10; the values were
read from the image as 3, 3, 3, 3. Under the composition rule above, the final
local matrix maps a point by:

1. subtracting the pivot,
2. applying the deformation,
3. applying the rotations,
4. adding back the pivot and then the translation.

Row 0 of the deformation is the image of the local x input axis and row 1 is the
image of the local y input axis. Tripling those rows triples the contribution of
the point's local x and y coordinates. The result is a uniform 3× enlargement in
x and y of the node's geometry about its pivot, applied before rotation and
translation. The z column and w column of those rows are also tripled, but the
recompute flattens the z column afterwards, and the w column is zero in an
identity or affine deformation. Row 3 of the deformation (its own offset) is not
scaled. This is a geometric scale, not a colour change: the Display object (node
+0x68) is not touched.

Status: PROVED for the matrix algebra in 0x3733F0 and 0x173E0. I did not inspect
the renderer's final point-times-matrix routine. The row-as-axis reading follows
from how translation is composed in 0x3733F0, so it holds unless the renderer
applies the matrix in transposed form, which this task did not investigate.

### Recompute after the multiply

- In 0x106C20, the instructions between the multiply (around RVA 0x106E92 to
  0x106EA4) and the end of the loop iteration make no call to the
  Transformation's slot +0x08 or to any attribute-change routine. The calls that
  follow are:
  - a child lookup by a short name (0x332220)
  - making each found child's Shape private (0x335780, node +0x58)
  - text setting on the clone (0x334C50)
  - placement through 0x334700
  None of the routines in this list, other than placement, reads or writes the
  clone's Transformation in the code examined. PROVED that 0x106C20 itself sends
  no notification.
- The clone routine 0x373310 recomputes the new Transformation's matrices. That
  happens inside 0x335830, before the multiply, so the cached +0x2A0/+0x2E0
  reflect the unscaled deformation at that point.
- Placement 0x334700 forwards to 0x334510. 0x334510:
  1. Takes a target point. In this caller it is (0, 0), and the reference offset
     is (running x offset, 120.0). The constant 120.0 is at 0x42E7C8. The x offset
     starts at −5 and grows by 50 before each aura, giving 45, 95, …
  2. If the node has a parent (node +0x40), maps the target point through the
     parent Transformation's inverse matrix +0x2E0, combined with a matrix from
     the grandparent chain (0x100EE0, not examined), and divides by w.
  3. Adds the reference offset and subtracts the node's own current pivot (+0x244
     index into +0x278) to get a new translation.
  4. Compares the new translation with the stored current translation (+0xAC
     index into +0xE0). Only if either component differs does it write the new
     translation and call the node Transformation's slot +0x08 (0x3733F0). That
     rebuild also folds in the tripled deformation.
- So the tripled deformation reaches the cached local and inverse matrices in
  this routine only when placement changes the translation. A fresh clone keeps
  the template's translation, so the rebuild happens exactly when the computed
  position differs from the template's stored position. PROVED as a code
  property. Whether those values differ at runtime, and whether any other
  per-frame or per-draw path recomputes Transformations unconditionally, is
  UNPROVED.

## c) Same object as the one 0x335830 makes private

0x335830 reads node +0x60. If it is non-null and its user count at +0x298 is
greater than 1, it:

1. clones the object through its vtable slot +0x10 (0x373310 for a
   Transformation),
2. removes the node from the old object's user set at +0x290. If the count is
   exactly 1, it releases the old object through the document's
   transformation-release routine 0x33E040 instead.
3. inserts the node into the new object's user set at +0x290,
4. stores the new object back into node +0x60.

0x106C20 reloads node +0x60 from the clone immediately after 0x335830 returns
(RVA 0x106E7C) and applies the ×3 to that object. The object scaled is therefore
the clone's private Transformation copy, and the template's shared Transformation
is left unmodified. If the count was 1 or less, no copy is made and the existing
object is scaled; in this flow the template shares it, so the count is at least
2.

Status: PROVED.

## Evidence boundaries

- Static evidence only. No game executable was run and no runtime state was
  observed.
- Not examined:
  - the renderer's application of +0x2A0 to vertices
  - the grandparent-chain matrix routine 0x100EE0
  - the rotation helpers' exact axis assignment (0x2B360, 0x2B470, 0x13050)
  - the Display constructor 0x3736C0 beyond its role and size
  - whether any per-frame update calls slot +0x08 unconditionally
- The meaning of the byte at +0x49 as a "matrix changed" flag is inferred from
  where it is written; its readers were not traced.
- The identification of 0x0DFC30 as a 4×4 inverse is based on its
  augmented-elimination structure, its write-back to the input matrix and its
  boolean return. Its singular-matrix behaviour was not characterised.
- Float constants read from the image: 0x441C10 = (3, 3, 3, 3);
  0x4ACA4C = −1; 0x42E520 = 0.01745329; 0x42E7C8 = 120; 0x4AC980 = 1.

## Exposure statement

Read in this task:

- `C:\Users\DieuDeGlace\Documents\Cubeworld-Vulkanengine-Steam\docs\CLEANROOM_AUTHOR.md`
  (role policy, read in full)
- `C:\Users\DieuDeGlace\Documents\decompiled cube world steam\decompiled\assembly.txt`:
  targeted searches and line ranges around 0x106C20, 0x333890, 0x334510/0x334700,
  0x335780/0x335830, 0x33DFC0/0x33E040, 0x33CD30, 0x32EE60, 0x368D90/0x368FA0,
  0x372BD0, 0x373310, 0x3733F0, 0x3809A0, 0x382F00, 0x173E0, 0x2B360 and
  0x0DFC30, plus searches for references to string RVAs and for callers of
  0x33CD30, 0x32EE60, 0x372BD0, 0x3736C0, 0x33DB70 and 0x33CAA0.
- `C:\Users\DieuDeGlace\Documents\decompiled cube world steam\decompiled\strings.csv`:
  searches and a short range near RVA 0x47C108 to 0x47E3A8.
- `C:\Users\DieuDeGlace\Documents\decompiled cube world steam\binary\cubeworld.original.exe.unpacked.exe`:
  raw byte reads of the PE header, the vtables at 0x47C0F0, 0x47BDD0, 0x47BD50
  and 0x47BD90, and the float constants listed above, plus a SHA-256 of the file.

Not read: no other file under the CubeVulkan repository, no `decompiled\functions\*.c`
file, no `functions.csv`, `calls.csv` or `function_ranges.csv`, no
reconstruction source, no SDKs and no web.

Harness context: before the task began, the harness placed into this agent's
context the text of the repository's `AGENTS.md` (project-wide coordination
instructions) and a git status snapshot naming the current branch. I did not open
either file myself and used none of their content in this document. AGENTS.md is
a combined policy and memory pointer, not implementation source. No replacement
engine source, tests, logs, captures or implementation history was exposed.
