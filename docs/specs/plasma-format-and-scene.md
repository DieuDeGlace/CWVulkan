# Plasma: the .plx document format and the scene graph

Behavioural specification of the interface library "Plasma" as statically linked into Steam
Cube World, written from the decompiled executable and the binary's constants, and checked
against the shipped `gui.plx`, `icons.plx`, `cursor.plx` and `quest-tag.plx`. Routines are
cited by RVA (image base 0x140000000). RTTI class names (`Node@plasma` and so on) are used
as class names. Byte offsets into in-memory objects are given only where they help a reader
cross-check a cited routine; the implementer does not need to reproduce the layout.

Conventions used throughout:

* All integers in the file are little-endian. "int" means signed 32-bit, "float" means IEEE
  754 32-bit.
* Matrices are 4x4 floats stored column-major: element index `c*4 + r` holds row `r`,
  column `c`. Points are column vectors, so a matrix with translation `(tx, ty)` stores
  them in elements 12 and 13. "A·B" means the ordinary product (apply B first, then A).
  Transforming a 2D point `(x, y)` always means `(x, y, 0, 1)` followed by division of x and
  y by the resulting w.
* Times are integer milliseconds (the interface clock advances by the frame's elapsed
  milliseconds, 0x349cf0; the text caret blinks on `clock / 500`, 0x38f0c0).
* Angles in the file are degrees.

Shape geometry, text layout, textures and the GPU side of drawing are specified elsewhere.
This document lists shape properties only so that a reader knows how to walk past them.

---

## Part 1. The .plx file format

The reader is the loader object built by 0x3802b0 and run by 0x3809a0. The entry point that
opens a file is 0x3429a0 (see 1.9).

### 1.1 Primitive values

| Kind | Encoding | Reader |
|---|---|---|
| int | 4 bytes | 0x37fe70 |
| float | 4 bytes | 0x37fea0 |
| byte string | int `n`, then `n` bytes. If `n` is 0 the string is empty. The value is the bytes up to the first zero byte (a string may carry trailing zeros). | 0x38dd30 |
| wide string | int `n` (count of 16-bit units), then `2n` bytes of UTF-16LE. Truncated at the first zero unit. | 0x38deb0 |
| vec2 / vec3 / vec4 | 2 / 3 / 4 floats | |
| matrix | 16 floats, column-major | |

Wherever the game reads a byte string as a name or text, it widens it to a wide string
(0x349c30). A property that exists in a narrow form (`X.name`) and a wide form (`X.wname`)
sets the same field; the shipped files use the wide forms.

### 1.2 The chunk stream

A .plx file is one flat sequence of **tagged chunks** with no file header of its own. Every
chunk is:

| Bytes | Meaning |
|---|---|
| 4 | tag id (int) |
| 4 | payload size `S` in bytes (int), counted from the byte after this field |
| S | payload |

Tag ids are small integers that stand for **tag names** such as `Node` or
`Transformation.translation`. The name table is not stored up front. Instead, the first
time a writer uses a name it emits a **definition** in front of the chunk:

1. Where a tag id is expected, the reader reads an int (0x38d8a0).
2. If it is 0, a definition follows: an int size (covers the rest of the definition), an
   int **new id**, then the **name** (see 1.4 for how the name bytes are read). The reader
   records `new id → name` in one table shared by the whole file, then reads the next int,
   which is the actual tag id of the chunk that follows.
3. The tag id is looked up in the table. An id never defined yields the empty name (which
   matches no property and is therefore skipped).

Only one definition can precede a tag; two definitions in a row are never written. The
table is global and never cleared. A definition can occur at any nesting depth, at the
first point where that name is used. **A reader must therefore descend into every chunk the
game descends into**: a definition sitting inside a chunk that is skipped wholesale would be
lost and later tags with that id would read as empty. Part 1.6 marks every container.

Chunk handling primitives (the reader keeps a stack of chunk end offsets):

* *enter* (0x38d7c0): read the size, push `position + size`.
* *at end / leave* (0x38dcb0): if the stream position equals the innermost end (or the
  stream has failed), pop it and report "ended"; otherwise report "not ended" and pop
  nothing. Every container is read as: enter; loop { if ended, stop; read a tag; handle
  it }.
* *skip* (0x38e010): read the size and seek past the payload. Unknown or unwanted tags at
  any level are skipped this way.

A leaf property is read as: enter, read the value, leave. An implementer should simply
seek to the chunk end after reading a leaf.

### 1.3 Header and version

The first chunks of every shipped file are:

| Offset | Bytes | Meaning |
|---|---|---|
| 0x00 | `00 00 00 00` | definition follows |
| 0x04 | int 22 | definition size |
| 0x08 | int 1 | id 1 |
| 0x0c | int 14, `PlasmaGraphics` | name (always plain) |
| 0x1e | int 1, int 4, int `v` | chunk `PlasmaGraphics`, payload = version int |
| 0x2a | definition of id 2 = `Seal` (plain) | |
| 0x3e | chunk `Seal` (1.4) | |

* If the loader flag 0x40 is set, 0x3809a0 first peeks: it skips 16 bytes, reads 14 bytes
  and requires them to equal `PlasmaGraphics`, else it throws `InvalidFileFormatException`
  ("… has invalid format."). The game does not set this flag.
* In the main loop a `PlasmaGraphics` chunk is read as one int; a version **greater than
  1** throws `InvalidVersionException` ("… has invalid version."). All shipped files have
  version 1.

### 1.4 Name obfuscation: the seal

Tag **names** in definitions may be obfuscated. Nothing else in the file is (strings,
numbers and pixels are plain).

**Key derivation** (the same formula at 0x3809a0 and 0x0f9ac0): a 64-bit key is a hash of a
phrase. Start with `h = 0x0003FFFFFFFFFFE5`; for every byte `b` of the phrase in order,
`h = h × 31 + b`, wrapping modulo 2^64. Values:

| Phrase | Key |
|---|---|
| `PlasmaXGraphics` (string at 0x440550, used by the game) | 0xFB3A260B5EC7CD5C |
| `PlasmaGraphics` (string at 0x47e1f8, built into the reader) | 0x62F15BACC28BC932 |

**The seal chunk** (`Seal`): payload = int `n`, then `n` obfuscated bytes. The reader builds a
list of candidate keys in this order: the keys the caller passed in (the game passes one:
the `PlasmaXGraphics` key, built in 0x0f9ac0 and handed to every `.plx` load), then 0, then
the `PlasmaGraphics` key. It decrypts the seal bytes with each candidate in turn (below) and
takes the first key whose result is exactly the 14 bytes `PlasmaGraphics`. That key becomes
the document key. If none matches, `InvalidDemoLicenseException` is thrown ("… was created
with a different demo license.", built in 0x380180). Key 0 decrypts to the input unchanged,
so an unprotected file has a plain seal and plain names.

**Decryption** (0x38d6a0) of `n` bytes `in[0..n-1]` with key `K`:

1. Let `k[0..7]` be the 8 bytes of `K`, little-endian (byte 0 is the lowest).
2. Let `s` = (low 32 bits of `K`, unsigned) mod `n`.
3. For `i` from 0 to `n-1`: let `j = (s + i) mod n`; set `out[j] = (in[i] − k[j mod 8]) mod 256`.
4. The result is `out` (no truncation at zero bytes).

So the writer rotated the plaintext left by `s` and added the key bytes, indexed by the
plaintext position.

**After the seal** every definition whose name is read while a non-zero document key is
set is read as int `n` + `n` bytes, decrypted as above. While the key is 0 (before the
seal, or for an unprotected file), the name is a plain byte string (1.1). All four shipped
files use the `PlasmaXGraphics` key.

Worked example (gui.plx, offset 0x58): definition size 15, id 3, `n` = 7, bytes
`80 98 9F B0 32 3F D2`. With the game key, `s = 0x5EC7CD5C mod 7 = 4`; byte 0 lands in
position 4 and loses `k[4] = 0x0B`: 0x80 − 0x0B = 0x75 `u`. The whole name decrypts to
`Texture`.

### 1.5 The top-level object stream

After the seal, 0x3809a0 reads top-level chunks until the end of the file and dispatches on
the tag name:

| Tag | Handled by | Counter it advances |
|---|---|---|
| `PlasmaGraphics` | version check (1.3) | |
| `Seal` | 1.4 | |
| `Texture` | 0x385740 | texture id map (by the texture's own `Texture.id`) |
| `TextShape` | 0x384bf0 | shape index |
| `SmoothMeshShape` | 0x383e30 | shape index |
| `GenericShape` | 0x3827e0 | shape index |
| `Transformation` | 0x385f50 | transformation index |
| `Display` | 0x382130 | display index |
| `Widget` | 0x386400 | widget index |
| `Button` | 0x381f50 | widget index |
| `ListWidget` | 0x382da0 | widget index |
| `Edit` | 0x382680 | widget index |
| `ScrollButton` | 0x3839b0 | widget index |
| `ScrollSlider` | 0x383bf0 | widget index |
| `PopUpButton` | 0x3837d0 | widget index |
| `Node` | 0x382f00 | node index |
| `pageWidth`, `pageHeight`, `dpi` | float each | (page info; ignored when loader flag 8 is set) |
| `unit` | int | (page info) |
| `pageColor` | vec4 | (page info) |
| anything else | skipped | |

Each counter starts at 0 and gives the k-th object of that family the index k. All three
shape kinds share one counter; all seven widget kinds share one counter. Every index map
also holds the entry −1 → "none". Other chunks refer to objects only by these indexes, so
the shipped files write all textures, then shapes, transformations, displays, widgets, and
finally nodes. Child links refer to later nodes, so linking is deferred (1.8).

Loader flags (an int handed to the load call):

| Bit | Effect |
|---|---|
| 0x04 | every created node gets node flag 0x8 (2.3.4) |
| 0x08 | page info chunks are skipped |
| 0x10 | SmoothMeshShape post-processing (shape spec) |
| 0x20 | every TextShape gets text flag bit 0 (text spec) |
| 0x40 | header peek (1.3) |

The game loads `gui.plx`, `icons.plx` and `quest-tag.plx` with flags 0x20, and
`cursor.plx` with 0 (0x0f9ac0).

### 1.6 Attributes: frames and sequences

Every animatable property is an **attribute**. On disk an attribute is a container chunk
holding, in any order:

* `Attribute.frame` (any number, leaf): one value of the attribute's type. The i-th
  `Attribute.frame` chunk inside the container sets **frame slot i**.
* `Attribute.sequence` (any number, container), holding:
  * `Attribute.sequence.name` (byte string) or `Attribute.sequence.wname` (wide string):
    the sequence name. It must come before the keys; it creates (or replaces) the named
    sequence on this attribute (0x34ad80).
  * `Attribute.sequence.key` (any number, container), holding leaves
    `Attribute.sequence.key.frame` (int, a frame slot index),
    `Attribute.sequence.key.time` (int, milliseconds) and
    `Attribute.sequence.key.smoothness` (float). Missing fields default to 0. When the key
    container ends, the key is inserted into the sequence before the first existing key
    whose time is greater than or equal to its own (0x36b3d0), so keys end up sorted by time.
* anything else: skipped.

Attribute readers by value type (all share the structure above; 0x3819c0 reads a
sequence):

| Value type | Reader | Bytes per frame |
|---|---|---|
| int (discrete) | 0x37fcb0 | 4 |
| float | 0x37f3f0 | 4 |
| vec2 | 0x37f770 | 8 |
| vec3 | 0x37f930 | 12 |
| vec4 | 0x37faf0 | 16 |
| matrix | 0x37f5b0 | 64 |
| wide string (discrete) | inline in 0x384bf0 | one wide string |

**Array attributes** (shape vertex data, readers 0x37ec70 float, 0x37eef0 vec2, 0x37f170
vec4) have the same container structure, with `ArrayAttribute.size` (int: resize every frame
to this many elements) and `ArrayAttribute.frame` (one frame: element count × element size
bytes) instead of `Attribute.frame`, plus `Attribute.sequence`.

**Frame slots.** Every attribute is constructed with exactly two frame slots (0x34a420 plus
each class constructor, e.g. 0x372bd0, 0x3736c0). Slot 0 is the **live value**: it is what
the object uses when drawing, what setters write and what animation writes into. Slot 1 is a
**scratch slot** used by `play` (2.6.3). Slots 2 and up are the **stored poses** that
sequence keys point at. Reading the i-th `Attribute.frame` overwrites slot i if it exists;
otherwise a new slot is appended first (initialised as a copy of the live value, 0x36ed60)
and then overwritten. The shipped files always write slots 0 and 1; when an attribute has
sequences, slots 2+ follow. Slot 1 often holds stale or uninitialised data (for example
NaNs in deformation matrices) and must not be interpreted.

### 1.7 Object properties

Property chunks are named `Kind.property`. Unknown property tags inside an object are
skipped. "attr" = attribute container (1.6), "leaf" = a single value.

#### 1.7.1 Node (0x382f00)

| Property | Form | Value |
|---|---|---|
| `Node.name` / `Node.wname` | leaf, byte / wide string | name (used by lookups) |
| `Node.shape` | leaf int | shape index, −1 = none |
| `Node.transformation` | leaf int | transformation index, −1 = none |
| `Node.display` | leaf int | display index, −1 = none |
| `Node.widget` | leaf int | widget index, −1 = none |
| `Node.child` | leaf int, repeated | a child's node index; order = child order |
| `Node.flags` | leaf int | node flags (2.3.4) |
| `Node.variable` | leaf: two byte strings | a key/value pair attached to the node (see Open questions on the order) |

When the Node chunk ends, the node is created by the object manager (0x33cd30) with the
referenced transformation, shape and display. If the transformation or display index is −1
(or absent), a fresh default Transformation / Display is created for the node. The widget
(if any) is attached (0x335150), flags are stored, variables are added (0x335010, which
ignores a key already present), and if loader flag 0x04 is set, flag 0x8 is ORed in.

**The first Node chunk of a file is special**: it is not created. It stands for the
*target node* passed to the load call (1.9). Everything it carries except its `Node.child`
list is read and discarded, including its transformation and display references. Its
children are attached to the target node (1.8).

Objects are **shared by reference**: several nodes may name the same shape, transformation
or display index and then hold the same object (gui.plx does this for shapes, up to 5
nodes per shape, and for two transformation/display pairs).

#### 1.7.2 Transformation (0x385f50; object 0x372bd0)

| Property | Form | Type | Default (slot 0) |
|---|---|---|---|
| `Transformation.name` / `.wname` | leaf | string | empty |
| `Transformation.translation` | attr | vec2 | (0, 0) |
| `Transformation.rotation` | attr | vec3, degrees about x, y, z | (0, 0, 0) |
| `Transformation.pivot` | attr | vec2 | (0, 0) |
| `Transformation.deformation` | attr | matrix | identity |

When the chunk ends the local matrix is recomputed (2.2.1).

#### 1.7.3 Display (0x382130; object 0x3736c0)

| Property | Form | Type | Default |
|---|---|---|---|
| `Display.name` / `.wname` | leaf | string | empty |
| `Display.visibility` | attr | int (discrete) | 1 |
| `Display.clipping` | attr | int (discrete) | 0 |
| `Display.fillColor` | attr | vec4 RGBA | (1, 1, 1, 1) |
| `Display.strokeColor` | attr | vec4 RGBA | (1, 1, 1, 1) |
| `Display.blurRadius` | attr | float | 0 |
| `Display.flags` | leaf | int (not animatable) | 0 |

#### 1.7.4 Widgets (0x386400 and the kind readers; shared property reader 0x386560)

Every widget kind is a flat object; all its properties are leaves. The kind readers first
check their own extra property and otherwise hand the tag to the shared reader 0x386560,
which skips what it does not know.

| Property | Type | Meaning (2.7) |
|---|---|---|
| `Widget.name` / `.wname` | string | widget name |
| `Widget.caption` | wide string | caption text (set through the caption setter 0x32d120) |
| `Widget.innerBindPos` | vec2 | inner (stretchable) rectangle origin, widget space |
| `Widget.innerBindSize` | vec2 | inner rectangle size |
| `Widget.bindPos` | vec2 | design-time frame origin |
| `Widget.bindSize` | vec2 | design-time frame size |
| `Widget.framePos` | vec2 | current frame origin |
| `Widget.frameSize` | vec2 | current frame size |
| `Widget.bindMatrix` | matrix | node-local → widget space (2.7.1); its inverse is computed at load |
| `Widget.horizontalAlignment` | int | 0 left, 1 right, 2 centre |
| `Widget.verticalAlignment` | int | 0 top, 1 bottom, 2 centre |
| `Widget.flags` | int | widget flags (2.7.6) |

Extra properties by kind:

| Kind | Extra property |
|---|---|
| `Button`, `PopUpButton` | `Button.type` (int) |
| `ScrollButton` | `ScrollButton.direction` (int), `Button.type` (int) |
| `ScrollSlider` | `ScrollSlider.direction` (int, 1 = horizontal, otherwise vertical, 0x366b60), `Button.type` (int) |
| `Widget`, `ListWidget`, `Edit` | none |

#### 1.7.5 Shapes and textures (reference only)

Listed so that a reader can walk past them correctly. Value semantics belong to the shape
and text specifications.

`TextShape` (0x384bf0): leaves `name`, `wname`, `flags` (int), `pixelSize` (float),
`strokeRadius` (float), `spacing` (float), `lineSpacing` (float), `wrapWidth` (float),
`fontSize` (int), `strokeWidth` (int), `fontName` (byte string), `wfontName` (wide string).
Containers: `TextShape.string` (a discrete wide-string attribute: its `Attribute.frame`
chunks each hold one wide string; it also takes `Attribute.sequence`),
`TextShape.color`, `TextShape.strokeColor`, `TextShape.extrusionColor` (vec4 attributes),
and `TextShape.font`, a container of leaves `fileName`, `wfileName`, `pixelSize` (int),
`glowRadius` (int), `size` (float), `strokeWidth` (float), `strokeGlow`, `pixelFont` (int),
all prefixed `TextShape.font.`.

`SmoothMeshShape` (0x383e30), all names prefixed `SmoothMeshShape.`:

| Containers | Kind |
|---|---|
| `vertexPositions`, `vertexTexCoords` | vec2 array attributes |
| `vertexColors`, `strokeColors`, `extrusionFrontColors`, `extrusionBackColors` | vec4 array attributes |
| `strokeWidths` | float array attribute |
| `texture`, `strokeTexture` | int attributes (values are `Texture.id`s, remapped at load) |
| `textureTranslation`, `texturePivot` | vec2 attributes |
| `textureRotation` | vec3 attribute |
| `textureDeformation`, `extrusionMatrix` | matrix attributes |
| `textureOpacity`, `textureBrightness`, `textureContrast`, `textureSaturation`, `strokeTextureOpacity`, `strokeTextureBrightness`, `strokeTextureContrast`, `strokeTextureSaturation`, `strokeTextureStretch` | float attributes |

Leaves: `name`, `wname`, `face`, `vertexFlags`, `vertexParameters`, `subdivisions` (int),
`smoothWeight` (float), `flags` (int), `strokeJointType`, `strokeCapType`,
`strokeAlignment`, `strokePattern` (ints), `strokeDash`, `strokeGap` (floats).

`GenericShape` (0x3827e0): all leaves: `name`, `wname`, `source`, `wsource`, `position`
(vec2), `size` (vec2).

`Texture` (0x385740): all leaves: `name`, `wname`, `id`, `format.pixelFormat`,
`format.minFilter`, `format.maxFilter`, `format.horizontalWrap`, `format.verticalWrap`,
`width`, `height`, `pixels`, `compressedPixels`, all prefixed `Texture.`.

### 1.8 Linking after the stream

When the top-level stream ends (end of 0x3809a0):

1. Visit the node index map in ascending index order (index −1 maps to nothing and is
   skipped). For each node, take the list of child indexes recorded from its `Node.child`
   chunks and append each referenced node to it, in recorded order (0x32f640). Index 0 is
   the target node, so the file root's children are appended to the target node **after**
   any children it already had.
2. Appending a node that already has a parent first removes it from the old parent.

In the shipped files every child index is greater than its parent's, so the result is a
tree. The game loads `gui.plx` and then `quest-tag.plx` into the same container node, so the
container's children are gui's 141 top-level nodes followed by quest-tag's.

### 1.9 Loading entry point

0x3429a0 (engine, file name, target node, flags, page-info receiver, key list): opens the
file, upper-cases the extension, and for `PLG`, `PLD` and `PLX` runs the chunk reader
above (`PLA` goes to a different, older reader, 0x3448a0, not covered). If no page-info
receiver is given, the engine's own is used.

---

## Part 2. The scene graph

### 2.1 Nodes and the tree

A node (`Node@plasma`, constructor 0x32ee60) has:

* a name (wide string);
* a parent (or none) and an **ordered list of children**; `addChild` (0x32f640) appends at
  the end, detaching the child from its previous parent; `setParent` (0x3347f0) does the
  same from the child's side;
* optional references to a **shape**, a **transformation**, a **display** and a **widget**.
  Transformations, displays and shapes keep a list of the nodes using them; setting a new
  one (0x334d30, 0x334760, 0x334850) unregisters the node from the old object and deletes
  the old object when this node was its last user;
* an **owner widget**: the node's own widget if it has one, else the parent's owner widget,
  else none. It is refreshed during every update traversal (2.6.6) and by 0x333820;
* integer node flags (2.3.4);
* a list of (key, value) string variables.

Children are drawn in list order (later children on top) and hit-tested in reverse order.

### 2.2 Matrices

#### 2.2.1 Local matrix of a transformation (0x3733f0)

Using the live values (slot 0) of translation `T = (tx, ty)`, pivot `P = (px, py)`,
rotation `R = (rx, ry, rz)` in degrees, and deformation matrix `D`:

1. Start from the identity `M`.
2. `M = M · Translate(tx, ty)`.
3. `M = M · Translate(px, py)`.
4. `M = M · RotX(rx)`, then `M = M · RotY(ry)`, then `M = M · RotZ(rz)`. Each angle is
   multiplied by 0.017453292 (π/180, constant at 0x42e520) and the standard right-handed
   rotation matrices for column vectors are used (0x2b360, 0x2b470, 0x13050).
5. `M = M · D` (0x173e0).
6. `M = M · Translate(−px, −py)` (the −1 is the constant at 0x4aca4c).
7. Flatten z: set the z output row to (0, 0, 1, 0), that is elements 2, 6 and 14 to 0 and
   element 10 to 1. The perspective row (elements 3, 7, 15) is kept, so a deformation
   matrix may encode a 2D projective map.
8. Store `M` as the **local matrix** and its inverse (0x0dfc30) as the **inverse local
   matrix**. Mark the transformation as changed.

So `local = Translate(T) · Translate(P) · Rx · Ry · Rz · D · Translate(−P)`, then flattened:
the node rotates and deforms about its pivot and is then moved by the translation. This
recomputation happens whenever any of the four attributes changes (animation, setters, end
of the chunk).

A node without a transformation uses the identity.

#### 2.2.2 World matrix

`world(node) = world(parent) · local(node)`, with the world of a parentless node equal to
its local (0x100fa0). The inverse world is computed the same way from inverse locals:
`invWorld(node) = invLocal(node) · invWorld(parent)` (0x100ee0). Bounds (0x332300) and
drawing (2.3.1) accumulate the same product down the tree.

### 2.3 Display, drawing and flags

#### 2.3.1 Draw traversal (0x330b20)

Drawing a node with a set of traversal options `F` (0x01: skip the node's own shape when
its bounds are off-screen, 0x3339c0; 0x04: no clipping, and stop at the first clip or blur
child; 0x08: no blur on this node; 0x10: no clipping and no children). Children are always
visited with 0x08 removed.

1. If the node's live `Display.visibility` is 0, or the node has flag 0x4, draw nothing
   (neither the node nor its subtree).
2. Push the matrix stack and multiply the current matrix by the node's local matrix
   (engine 0x38eb90).
3. Decide:
   * **clip** = live `Display.clipping` ≠ 0 and `F` has neither 0x04 nor 0x10;
   * **blur** = the engine's blur option is on, `Display.flags` bit 0 is set, the live
     `Display.blurRadius` > 0, and `F` has no 0x08.
4. Ordinary node (neither clip nor blur): draw the node's shape, then, unless `F` has
   0x10, draw each child in order. If `F` has 0x04, the children loop stops at the first
   visible, non-disabled child that is itself a clip or blur node.
5. Clip node: push an offscreen layer (0x33c110) and clear it over the node's bounds
   (0x33c920, bounds from 0x332fd0, which for a clip node cover only its own shape); draw
   all children into that layer; pop the layer (0x33e1e0), which leaves it bound as the
   current mask source; then draw the node's own shape. The shape's draw samples the layer
   according to the **mask mode** (2.3.3). The effect: the children are visible only where
   the clip node's shape is.
6. Blur node: its subtree is rendered to an offscreen target sized to the blurred bounds
   and composited with a blur whose radius scales with the matrix (render spec). Bounds of a
   blur node grow by `0.5 × (|column 0| + |column 1|) × blurRadius` (0x332300, 0x332fd0).
7. Widgets with widget flag 0x40, and nodes with node flag 0x400, render their subtree
   into a cached surface that is redrawn only when something changed (0x32d890; render
   spec).
8. Pop the matrix.

#### 2.3.2 Colour

There is **no tint inheritance down the tree**. When a shape is drawn (0x3913d0), its
colour multiplier is the component-wise product of

* the **owning node's own** live `Display.fillColor`, and
* the engine's current tint for that pass (fill, stroke or extrusion), which is (1, 1, 1, 1)
  except while text glyphs are drawn (0x34fe40 sets it per glyph).

`Display.strokeColor` is loaded, animated and copied but is not read by the drawing code
(see Open questions). To fade or tint a whole subtree, the game must set every node's fill
colour (or animate it).

#### 2.3.3 Mask mode (0x3913d0, 0x38ec30)

For a node whose live clipping value is non-zero, the shape is drawn with mask mode:

| `Display.flags` | Mode |
|---|---|
| bit 1 (0x2) set | 2 |
| bit 1 clear, bit 2 (0x4) set | 3 |
| both clear | 1 |

Other nodes draw with mode 0. The mode is a shader constant (0x390420). All five clip
displays in gui.plx have flags 2. The exact pixel rule per mode is in the pixel shader (see
Open questions).

#### 2.3.4 Node flags

| Bit | Meaning |
|---|---|
| 0x0001 | excluded from hit tests that pass option 1 (0x333f60) |
| 0x0002 | non-interactive: skipped by the widget visitors (0x33c360) and by hit tests unless option 2 is passed; excluded from bounds when option 4 is passed (0x32f7d0) |
| 0x0004 | disabled: skipped by draw, update, hit test, bounds, text setters, play-state queries and copies |
| 0x0008 | set by loader flag 0x04; excluded from hit tests that pass option 4 |
| 0x0080 | auto-fit: after an update in which the subtree changed, the node's shape is refitted to the union of its children's bounds (0x335920) |
| 0x0400 | render the subtree through a cached surface |
| 0x1000 | excluded from hit tests |
| 0x2000 | keep animating while invisible; inherited by the subtree during the update (2.6.6) |

gui.plx uses 0x2000 (its root), 0x0001 (two `faction` nodes) and 0x0800 (one node), the
last with no meaning found in Plasma.

#### 2.3.5 Hit testing (0x333f60)

Point picking from a node with options: a node is skipped if invisible, flagged 0x4 or
0x1000. If the node's own widget renders into its own surface and the engine's
widget-clipping option is on, the point must lie in the widget's frame rectangle (2.7.2) unless widget flag
0x100 is set. The point is brought into the node's local space with the inverse world
matrix (with w division), then un-deformed through the owner widget when the shape is
deformable (2.7.4). The node is "hit" if its shape contains the point, or its widget reports
a hit, or the widget has flag 0x100. Children are tested first, last child first, unless
the node is a clip node that was not hit. The first hit found wins; the node itself
qualifies after its children if it was hit and passes the option filters of 2.3.4.

### 2.4 Lookup and text

* **Find by name** (0x332190): depth-first pre-order from a start node (the node itself
  first, then each child subtree in order); exact, case-sensitive comparison of wide names;
  returns the first match. Disabled nodes are **not** skipped. gui.plx has duplicate names
  (two top-level `hpbar` and `mpbar`); the first in pre-order wins.
* **Set text** (0x3348e0) with a name, a wide string and a mode: walk the subtree,
  skipping flag-0x4 nodes. For a node whose name matches:
  * mode 0: if its shape is a TextShape and the shape's live string differs, set the live
    frame (slot 0) of `TextShape.string` and ask the shape to rebuild (its update with
    argument 0);
  * mode 1: run 0x334c50 on it, which sets the live string of every TextShape in the
    matched node's whole subtree (skipping flag-0x4 nodes) without the rebuild call.
  A matched node's descendants are not searched for further name matches; non-matching
  nodes are searched recursively.
* **Set text colour** (0x334a20, 0x334b80) works the same way on the live frame of
  `TextShape.color`.

### 2.5 Instancing a document node

Typical use: find a template node by name in the loaded document container, then copy it
under a parent. Two copies exist:

* **Shallow instance** (0x333890, the most used): create a new node under the parent with
  the same name, the **same** transformation, shape and display objects (shared), copy the
  flags, the variables and the node's second key/value list; let the source widget clone
  itself onto the new node (widget class slot 0x140); recurse into each child that is not
  flagged 0x4. Because the objects are shared, moving, recolouring or retexting one
  instance changes all of them until the game gives an instance its own object.
* **Deep copy** (0x3308e0): as above, but the shape, transformation and display are each
  duplicated (shape slot 0x68, transformation 0x373310, display slot 0x10), including all
  frames and sequences. The new node's owner widget is its own cloned widget or else the
  parent's owner widget; when it has one and a shape, the shape is asked to rebuild.

### 2.6 Animation

#### 2.6.1 Objects involved

Shapes, transformations and displays are **keyables** (`Keyable@plasma`): each holds its
list of attributes (registered in construction order, 0x368fa0), a "playing" flag, a
"changed" flag, an "updated this pass" flag and the name of the last played sequence.

Each attribute holds its frame slots (1.6), a map from sequence name to **sequence** (a
list of keys {frame slot, time, smoothness} kept sorted by time, plus a loop field that is
0 for everything loaded), and its **active key list** with its own **period** (int, 0 = not
looping), its **clock** (int ms), the last frame applied by a jump (initially −1), a
cached segment index (initially 0) and a "changed" flag.

#### 2.6.2 Interpolation (attribute class slot 0x30)

Given frame slots `Pp` (previous key), `A`, `B` (the segment's keys), `Pn` (next key), the
smoothness `sA` of key A and `sB` of key B, and `u ∈ [0, 1]`:

* Continuous types (float 0x36ff20; vec2/vec3/vec4 and matrix per component, e.g. 0x370010
  for matrices):
  * `d = (B − A) × 0.25`
  * `tA = sA × 0.25 × (B − Pp) + (1 − sA) × d`
  * `tB = sB × 0.25 × (Pn − A) + (1 − sB) × d`
  * live value = `A·(1−u)³ + (A + tA)·3u(1−u)² + (B − tB)·3u²(1−u) + B·u³`
  This is a cubic Bézier from A to B whose handles sit a quarter of the segment along the
  chord when smoothness is 0 and a quarter of the neighbour chords (Catmull-Rom-like) when
  smoothness is 1. Note that smoothness 0 does **not** give exact linear motion; it gives a
  mild ease in and out.
* Discrete types (int 0x342050, wide string): live value = A while `u < 1`, else B.

The constants: 0.25 at 0x4ac95c (four-wide at 0x479770 for matrices), 1.0 at 0x4ac980
(four-wide at 0x42e8c0), 3.0 at 0x42e580.

#### 2.6.3 Play (0x369030 per keyable; 0x3342a0 per subtree; 0x34af80 per attribute)

`play(node, name, period)` visits the node's shape, transformation and display, then
recurses into **all** children (no flag checks). For each keyable it stores the name, and
for every attribute that has a sequence of that name it starts it and sets the keyable's
playing flag. Starting a sequence on an attribute (only if the sequence has at least one
key; otherwise nothing changes):

1. Clear the active key list. Set its period to `period`, the clock to 0, the cached
   segment to 0 and the last applied frame to −1.
2. Copy each key of the sequence into the active list (sorted insert by time), except that
   when `period ≥ 1`, keys whose time is ≥ `period` are left out.
3. If the first active key's time is not 0: copy the live value into frame slot 1, then
   insert a key {slot 1, time 0, smoothness of the first key} at the front. The animation
   therefore starts from wherever the value is now.

`stop` (0x369150 per keyable, 0x335170 per subtree, skipping flag-0x4 children) clears the
playing flag and each attribute's active key list.

`is playing` (0x333ed0): true if the node's shape, transformation or display has its
playing flag set, or any child not flagged 0x4 is playing (recursively).

`duration(node, name)` (0x3351f0): the largest time of the **last key** of the named
sequence over every attribute of the node's shape, transformation and display and,
recursively, its children not flagged 0x4; 0 if none.

#### 2.6.4 Evaluate an attribute at time `t` (0x34b5e0)

With `n` active keys, period `L`, and `key(i)` meaning index `i` wrapped modulo `n` when
`L > 0` and clamped to `[0, n−1]` otherwise (0x34ab00):

1. `n = 0`: nothing; report "no change".
2. `n = 1`: if the last applied frame differs from the key's slot, copy that slot to the
   live value (class slot 0x18), remember it, report "changed"; otherwise "no change".
3. `n ≥ 2` and `t <` time of key 0: same jump as step 2, to key 0's slot.
4. `n ≥ 2`, `L = 0` and `t ≥` time of key `n−1`: same jump to key `n−1`'s slot (so the
   end pose is applied once, then "no change").
5. Otherwise interpolate. Set the last applied frame to −1. The segment count is `n` when
   `L > 0` (the last segment wraps to key 0), else `n − 1`. Reset the cached segment to 0
   if it is out of range. Starting at the cached segment and wrapping around, for each
   segment `i`:
   * `A = key(i)`, `B = key(i+1)`. If either slot is negative or not an existing slot,
     report "no change".
   * Segment times `ta = A.time`, `tb = B.time`. When `L > 0`, add `base = (t div L) × L`
     to both, and on the last segment (`i = count − 1`) add `L` to `tb` as well.
   * If `ta ≤ t ≤ tb`: `u = (t − ta) / (tb − ta)` in floating point; `Pp = key(i−1)`,
     `Pn = key(i+2)`; if their slots are valid, interpolate (2.6.2) into the live value,
     remember `i` as the cached segment, report "changed"; else "no change". Stop.
   If no segment contains `t`, report "no change".

#### 2.6.5 Clocks

* **advance(attribute, dt)** (0x34bb50): if the active list is non-empty, add `dt` to the
  clock and evaluate at the new clock; if that reports "changed", done. Otherwise (or if
  the list was empty) clear the active list: the animation has finished. A non-looping
  animation therefore plays, applies its last pose once, and stops on the following frame.
* **set time(attribute, t)** (0x34bb20): clock = `t`, then evaluate. Does not clear.
* **advance(keyable, dt)** (0x369190): only if playing; advance every attribute; if any
  changed, the keyable recomputes itself (for a transformation, 2.2.1) and reports
  changed; if none changed, its playing flag is cleared.
* **set time(keyable, t)** (0x3690c0): set time on every attribute regardless of the
  playing flag; recompute if any changed.

The widget enable/disable routine (0x32a8d0, 0x32b830) shows the intended use: play
`widget:enable` or `widget:disable` on the widget's node, compute its duration, set every
keyable in the subtree to that time (0x334ec0, which runs the same traversal as below with
set-time instead of advance), then stop the subtree. That snaps to the end pose instantly.
Buttons similarly play `button:enter` / `button:leave` (seen in gui.plx).

#### 2.6.6 Per-frame update traversal (0x349cf0 → 0x3358e0 → 0x335920)

Each interface frame the engine adds the elapsed milliseconds `dt` to its clock and runs on
its root node:

1. Clear every keyable's "updated this pass" flag in the tree (0x334dc0, skipping
   flag-0x4 subtrees).
2. Visit the node (starting with force = false):
   1. Skip the node and its subtree if it has flag 0x4. Clear its "changed" flag.
   2. Continue only if the node's live visibility is non-zero or force is true. Let
      force′ = force or (node flag 0x2000).
   3. Refresh the owner widget (2.1).
   4. Advance the shape, transformation and display by `dt`, each only if not yet updated
      in this pass (shared objects advance once per frame), and mark them updated.
   5. Run the widget's per-frame hook (widget class slot 0xd8).
   6. The node is "changed" if its shape, transformation or display changed this frame,
      if its widget is dirty, or if its widget is the engine's active widget (0x342090).
   7. Visit each child with force′; a changed child marks this node changed.
   8. If the node has flag 0x80, changed, and a shape: compute the union of the children's
      bounds in this node's space (each child's bounds with its local matrix, 0x332300);
      if any child contributed, fit the shape to that box (shape slot 0x90) and rebuild it.
3. Clear the "changed" flags of the changed part of the tree (0x334e30).

### 2.7 Widgets

#### 2.7.1 Fields and spaces

A widget (`Widget@plasma`, constructor 0x328ff0) belongs to exactly one node. Its
geometry lives in **widget space**. The bind matrix `B` maps the node's local space to
widget space; its inverse `B⁻¹` (computed at load, 0x386560) maps back.

| Field | Default | Source |
|---|---|---|
| innerBindPos `ip`, innerBindSize `is` | (0,0), (1,1) | file |
| bindPos `bp`, bindSize `bs` | (0,0), (1,1) | file |
| framePos `fp`, frameSize `fs` | (0,0), (1,1) | file / setters |
| minimum size `ms` | (0,0) | set to `bs − is` when the widget initialises (0x32b830) |
| `B`, `B⁻¹` | identity | file |
| alignment h, v; flags | 0 | file |

When it initialises (widget class slot 0x38, 0x32b830) a widget records the window size,
sets `ms = bs − is`, and snaps its node to the `widget:enable` or `widget:disable` end pose
(2.6.5).

#### 2.7.2 Derived geometry

* **Effective frame size** (0x32a110): per component
  `eff = bs + max(fs + is − bs, max(0, ms + is − bs)) − is`, which with `ms = bs − is`
  is `max(fs, bs − is)`. Width alone: 0x32a1d0.
* **Deformed inner rectangle**: position `Dp = fp + ip − bp` (0x32ad70); size
  `Ds = max(fs + is − bs, max(0, ms + is − bs))` (0x32adf0). The inner region never
  shrinks below zero; everything outside it keeps its design size.
* **Frame matrix** (0x32ce40): `F = world(node) · B⁻¹ · Translate(fp)`. Maps frame space
  (origin at the frame's top-left) to world space. The world frame rectangle is
  `F·(0,0)` (0x32cf50) with size equal to the linear part of `F` applied to `eff`
  (0x32d010).
* **Frame-to-screen matrix** (0x32cae0): `invLocal(engine root) · F` with the world taken
  below the engine root; the frame origin in that space is 0x32cc50.
* **Parent-relative matrix `Q`** (0x32c0c0): find the nearest ancestor (starting with the
  node's parent) that has a widget. If there is one, `Q = Translate(−o) · frame-to-screen`
  where `o` is that ancestor widget's frame origin from 0x32cc50. If there is none,
  `Q = world(parent) · frame-to-screen`.
* **Position** (0x32c340) = `Q·(0,0)`: the frame's top-left relative to the parent
  widget's frame top-left. x alone 0x32da70, y alone 0x32dab0.
* **Size** (0x32d1d0) = linear part of `Q` applied to `eff`. x alone 0x32da30, y alone
  0x32ad10.

#### 2.7.3 Setters

* **Set position** (0x32b250, two-float form 0x32b560): if the requested position differs
  from the current one, convert both the requested point and the origin through the
  inverse of `Q · B · invLocal(node)` and add the difference to the live translation of the
  node's transformation, then recompute it. The node's transformation therefore carries the
  move; the widget's own fields do not change.
* **Set size** (0x32c6f0, two-float form 0x32c9a0): remember the old `Ds`; set
  `fs` = inverse linear part of `Q` applied to the requested size; compute the new `Ds`;
  let the size delta be the linear part of `Q` applied to (new `Ds` − old `Ds`). If the
  node is not flagged 0x4 and the delta is non-zero, mark the widget dirty and propagate the
  delta through the node's descendants (0x32c560): descendants without a widget, or with
  this widget, pass it on to their children; the first descendant widget on each branch
  receives it through its placement routine (2.7.5) when propagation is enabled. Finally
  the widget's resize hook runs (slot 0x50).

#### 2.7.4 Deformation (CPU form 0x32a310; GPU form uses the same quantities in 0x3913d0)

Shapes that report themselves deformable (SmoothMeshShape and GenericShape yes, TextShape
no; shape slot 0x28) are stretched by their **owner widget** like a nine-slice. To deform a
point `p` given in the local space of node `N`:

1. If `Ds = is` and `Dp = ip`, `p` is unchanged.
2. `q = B · invWorld(widget node) · world(N) · p` (into widget space; divide by w).
3. Per axis: `t = 0` if `q < ip`; `t = 1` if `q ≥ ip + is`; else `t = (q − ip) / is`.
4. `q′ = q + (Dp − ip) + t × (Ds − is)`.
5. `p′ = invWorld(N) · world(widget node) · B⁻¹ · q′` (divide by w).

Points left of / above the inner rectangle move with its near edge, points beyond it move
with its far edge, points inside are spread linearly. The inverse mapping (for hit
testing) is the Deformer's second slot, 0x32d2e0. The GPU path (0x3913d0) enables
deformation only when `Ds` differs from `is` (a pure move of the inner rectangle is not
deformed there) and passes `B·invWorld(widget node)`, its inverse, `ip`, `is`, `Dp`, `Ds`
as shader constants.

#### 2.7.5 Placement on parent resize (0x32bd70, widget class slot 0x48)

Called with an offset `o` and a size delta `d` (in the parent's space). Nothing happens for
a widget whose node has no parent. Let `pos` be the current position (2.7.2):

1. Horizontal alignment 1 (right) and widget flag 0x4 clear: set position
   (`pos.x + o.x + d.x`, `pos.y`). Alignment 2 (centre): set position
   (`pos.x + 0.5 × d.x`, `pos.y`). Alignment 0: no move.
2. Vertical alignment 1 (bottom) and widget flag 0x8 clear: set position
   (`pos.x`, `pos.y + o.y + d.y`). Alignment 2: (`pos.x`, `pos.y + 0.5 × d.y`).
3. Stretch: if widget flag 0x4 or 0x8 is set, take the current size (linear part of `Q`
   applied to `fs`), add `d.x` when 0x4 is set and `d.y` when 0x8 is set, and set size
   (2.7.3, with propagation).
4. Update attached scroll sliders (2.7.7) and re-clamp the scroll (0x3298b0).

Top-level widgets (no widget ancestor) are driven by the window: when the window size
changes, slot 0x40 (0x32bfd0) calls the placement with `o = (0, 0)` and
`d = (new width − old width, new height − old height)` and records the new size.

#### 2.7.6 Widget flags

| Bit | Meaning |
|---|---|
| 0x004 | stretch horizontally with the parent; disables right alignment |
| 0x008 | stretch vertically with the parent; disables bottom alignment |
| 0x020 | consulted by the press handling (0x340ec0) |
| 0x040 | render content into an own surface the size of the frame (0x32d890) |
| 0x100 | the whole frame counts as a hit; no frame clipping of hits |
| 0x200 | widget draw hook runs before the children instead of after (0x330b20) |

#### 2.7.7 Scrolling

A scrollable list is a content widget inside a viewport widget (its nearest widget
ancestor). The scroll position is the content's **position** relative to the viewport
(2.7.2), which is always ≤ 0. After moves and resizes, 0x3298b0 clamps it per axis: if the
content is larger than the viewport, the position is kept between `viewport − content` and
0; if not, it is 0. A scroll slider (0x366b60) attached to the content sets its thumb
length along its axis to `track length / max(1, content / viewport)` and its value to
`−position / (content − viewport)` when the content is larger, else 0.

#### 2.7.8 Widget hit testing (0x32ae90)

A node participates if it is not flagged 0x4, is visible, and has no widget or this widget.
The pointer is taken into the node's space with the inverse world matrix, un-deformed by the
widget's Deformer, and tested against the node's shape and then the widget. Children are
tested last-first, but only when the node is not a clip node.

---

## Part 3. Constants

| RVA | Value | Use |
|---|---|---|
| 0x440550 | `PlasmaXGraphics` | the game's key phrase |
| 0x47e1f8 | `PlasmaGraphics` | header name, seal plaintext, reader's built-in key phrase |
| 0x47e208 | `Seal` | seal tag |
| (code, 0x3809a0, 0x0f9ac0) | 0x0003FFFFFFFFFFE5, ×31 | key hash seed and multiplier |
| (code, 0x3809a0) | 1 | highest accepted version |
| 0x42e520 | 0.017453292 | degrees to radians |
| 0x4aca4c | −1.0 | pivot undo; negating the parent frame origin |
| 0x4ac964 | 0.5 | centring; blur bounds factor; pixel rounding |
| 0x4ac980 | 1.0 | interpolation, w division |
| 0x4ac95c | 0.25 | Bézier handle factor (float) |
| 0x479770 | (0.25, 0.25, 0.25, 0.25) | Bézier handle factor (matrix) |
| 0x42e8c0 | (1, 1, 1, 1) | `1 − s` (matrix); default fill and stroke colours |
| 0x42e580 | 3.0 | Bernstein weights |
| 0x42e630 | 4x4 identity | default matrix |
| 0x4acb30 | sign-bit mask | negation in the slider value |

Default attribute values: translation (0, 0), rotation (0, 0, 0), pivot (0, 0),
deformation identity, visibility 1, clipping 0, colours (1, 1, 1, 1), blur 0. Widget
defaults as in 2.7.1.

## Part 4. Order of operations (summary)

Load: read chunks → definitions decoded with the current key → seal fixes the key → objects
built in stream order (textures, shapes, transformations, displays, widgets, nodes), each
transformation's matrix computed at the end of its chunk → file root mapped to the target
node → child links appended in node-index order.

Frame: interface clock += dt → clear per-pass flags → update traversal (advance animations,
refresh owner widgets, recompute changed transformations, auto-fit) → clear changed flags →
draw traversal from the root (matrix push, clip/blur/cache decisions, own shape, children)
→ hit testing as input arrives.

---

## Verification

A throwaway parser written from Part 1 (kept outside the repository) reads all four shipped
files to the last byte, with every tag id resolving to a defined name.

| File | Size | Key | Tag names | Textures | Shapes (Text / Mesh) | Transformations | Displays | Widgets | Nodes |
|---|---|---|---|---|---|---|---|---|---|
| gui.plx | 2,005,316 | PlasmaXGraphics | 115 | 4 | 659 (160 / 499) | 677 | 677 | 144 (38 Widget, 105 Button, 1 Edit) | 685 |
| icons.plx | 3,734,983 | PlasmaXGraphics | 82 | 0 | 939 (14 / 925) | 1047 | 1047 | 0 | 1119 |
| cursor.plx | 11,609 | PlasmaXGraphics | 82 | 0 | 3 (2 / 1) | 4 | 4 | 0 | 4 |
| quest-tag.plx | 261,415 | PlasmaXGraphics | 88 | 0 | 92 (34 / 58) | 93 | 93 | 0 | 93 |

* Page info: gui.plx 760 × 530, dpi 72, unit 0, colour (0.2135, 0.7034, 0, 1); icons.plx
  1000 × 1000, dpi 144; cursor and quest-tag 1000 × 1000, dpi 72, colour (1, 1, 1, 0).
* Several definitions occur only inside SmoothMeshShape containers (ids 20, 21, 29 in
  gui.plx: `ArrayAttribute.size`, `ArrayAttribute.frame`, `Attribute.frame`). A first
  attempt that skipped mesh internals lost them and read every later `Attribute.frame` as
  an empty name, which is how the "descend into all containers" rule was confirmed.
* gui.plx: node 0 is the file root (flags 0x2000, 141 children, a transformation with
  translation (−3083.2, −93.5) and scale 6.65 that the loader discards). Every child index
  is greater than its parent's. Node flags in use: 0 (681), 0x2000, 0x0001 (×2), 0x0800.
  Display flags: 0 (672) and 2 (5, exactly the five clip displays: `location`, `edit/clip`,
  `button/clip`, `button2/clip`, `combopoints`). Seven displays are invisible in slot 0.
  Widget flags: 0 (142) and 4 (2); alignments (0, 0) except three `text` widgets with
  (2, 2). All `Button.type` values are 0.
* Every attribute in every file has frame slots 0 and 1; slot 1 of deformation matrices
  holds garbage (NaNs, 1e28-scale values), confirming it is a scratch slot.
* **hpbar** (gui.plx node 27, the first `hpbar` in pre-order; a second one is node 546):
  translation (566.25, 882.5), pivot (−56.25, −37.5), mesh shape. Children in order:
  `background` (translation (−7.25, −25.0), pivot (−50, −12.5), mesh), `bar` (same
  placement, mesh), `Shape` (52.75, −26.25, mesh), `frame` (−111.43, 79.16, pivot
  (49.18, −116.66), mesh) with child `Shape` (mesh), `info` (TextShape "HP") with child
  `info` (TextShape "HP"), and two `text` nodes (TextShape "20000/2000o", translation
  (33.06, −21.0)).
* **mpbar** (node 16): translation (761.23, 881.95), same child structure as hpbar plus a
  `chargebar` after `bar`; its `info` text is "MP".
* **staminabar** (node 328): translation (176.25, 1012.5), pivot (3.75, −2.5); children
  `background`, `bar` (pivot (−45.0, −13.5)), `frame` with children `Shape` and three
  `Mesh` nodes at translations (−40.83, −1126.66), (−80.83, …), (−0.83, …) and pivot
  (180, 1013.86).
* Sequences: gui.plx uses `button:enter`, `button:leave`, `hit`, `location`; e.g.
  `button/clip/Shape` translation has slots (−195, −270), (−550, −450), (−195, −270),
  (−195, −130) and `button:enter` keys {slot 2, 0 ms}, {slot 3, 500 ms}; `combopoints/Text`
  has `hit` = {slot 2, 67 ms}, {slot 3, 867 ms} (first key not at 0, so play inserts the
  slot-1 key). quest-tag.plx uses `shine`, `show`, `show2`; a translation `shine` has keys
  at 0, 367, 1500, 1867 and 1900 ms over slots 2 to 6. Some sequences are written with a
  name and no keys (playing them changes nothing). All smoothness values seen are 0.
* Worked decryption in 1.4 reproduces `Texture`; the seal of every file decrypts to
  `PlasmaGraphics` only with the `PlasmaXGraphics` key.

## Open questions

1. **Node.variable order.** The two strings are read in stream order and passed to the
   pair constructor (0x338a90) in reversed argument order; with MSVC's right-to-left
   evaluation this most likely makes the **second** string the key and the first the value,
   but no shipped file contains a variable to confirm it.
2. **Mask modes 1–3.** The CPU side only selects a mode number (2.3.3); what each does to
   the layer and the shape's coverage (plain mask, inverted mask, or mask without drawing
   the shape) lives in the D3D11 pixel shader embedded in the binary and was not
   disassembled.
3. **Display.strokeColor** is never read by the drawing code found here (only loaded,
   animated and copied). Whether the stroke pass uses it indirectly (through the mesh) is
   for the shape spec to confirm.
4. **Node flags 0x0001 and 0x0800** appear in gui.plx. 0x0001 only filters hit tests that
   ask for it; no Plasma routine tests 0x0800.
5. **Button.type and ScrollButton.direction** values and their behaviour were not traced
   (all shipped buttons use type 0).
6. **Cached widget surfaces** (widget flag 0x40, node flag 0x400): the surface holds three
   more widget fields (a scale defaulting to 1, an offset pair defaulting to 0, an integer
   divisor defaulting to 1) used when compositing; their setters were not found.
7. **Blur rendering** (radius scaling, pass count) is only outlined (2.3.1 step 6).
8. **The `PLA` reader** (0x3448a0) for the older format was not examined.
9. **The no-widget-ancestor case of `Q`** (2.7.2) multiplies the parent's world matrix with
   a matrix that already contains the node's world (confirmed in the assembly at
   0x32c140), so it equals the expected parent-relative matrix only when the parent's world
   matrix is the identity. Reproduce it as written; whether the game ever hits a case where
   it matters was not checked.
