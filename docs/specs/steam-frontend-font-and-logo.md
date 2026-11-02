# Steam frontend immediate fonts and queued title logo

Stable bounded native-only audit of immediate Widget text construction,
font selection/loading and the reached title-logo submission/consumer.
Sources:0x268a70,0x269130–0x269210,0x2910c0/0x291210,0x26ea60/0x26eba0,
0x34c410/0x34c940/0x34d060,0x352df0/0x353860/0x353c80,
0x34fe40/0x351b20, Game0xad9d4, scene0xe2e00,0xee5a0 and0xe6e60.
No engine, tests, exporter, owner files/captures or game execution was used.
This supplies no font artwork, inferred glyph coverage or image-based logo size.

## 1. Which face immediate text actually selects

Base Widget constructor **0x268a70** initializes its font pointer
**+0x1a8 to null**. It does not select a default font solely by being a
BaseWidget. The relevant style helpers do not change this pointer:

| Helper | Stored setting |
|---|---|
|0x269160|Text RGBA at+0x1c4..+0x1d0|
|0x269190|Alignment flags at+0x1e4|
|0x2691b0|Font size at+0x1b0|
|0x2691f0|Outline width at+0x1b4|

Font-name selection is **0x269130**: obtain the GUI engine's FontEngine
at **engine+0x48**, call0x34d060 with the requested name and store its
returned pointer at Widget+0x1a8. Verified frontend constructors are:

| Immediate text owner | Construction/drawing | Requested name |
|---|---|---|
|Title root menu|0x2910c0 →0x269130 at0x2911ba; draw0x291210|resource1.dat|
|Saved-character card, including its null/New character entry|0x26ea60; draw0x26eba0|resource1.dat|
|Character-creation appearance controls|0x26fda0|resource1.dat|

These requests establish a font name and resolver route, not proof of the
actual runtime face bytes under arbitrary cache/registry history. There
is no reached automatic substitution from resource1.dat to resource2.dat
in these immediate callers.

If the selected pointer is null, **0x269210** returns pen(0,0) and emits
no text. Its ordinary successful two-pass outline/fill behavior remains
[shapes and text §7.1](plasma-shapes-and-text.md#71-the-two-pass-text-object-draw-0x269210).
The existing geometry/pivot/scalable-font contract still applies; choosing
font size or outline is not another face selection or case conversion.

## 2. Effective source, cache, directories and unresolved memory producer

FontEngine constructor **0x34c410** creates an empty named-byte map
**+0x10**, empty cached-font map **+0x20**, and directory list **+0x38**.
The named map's sentinel allocator is0x34c700. Graphics startup0x134cc0
registers **c:\windows\fonts** through0x34c940. This registration appends
a directory; it does not register resource1.dat/resource2.dat bytes.

Resolver **0x34d060** strips the directory portion for its **cache key**.
An already cached basename returns its existing font pointer before a
new load or directory search. Otherwise it creates a font and first tries
the requested name/path. If that fails, it visits registered directories
in list order and tries directory + slash + basename. If every attempt
fails, it discards the new object and erases its cache entry.

Loader **0x352df0** uppercases the **filename extension** to recognize
TTF/DAT. For those face formats, a supplied named-byte map is searched
using the requested name. A matching nonempty entry takes precedence over
disk. The consumed entry fields are the wide-string key at **node+0x20**
and byte-vector **begin+0x40/end+0x48**. The memory-face call
**0x353c80 →0x397720** receives the begin pointer and a32-bit length formed
from end-minus-begin. A missing/empty entry takes the file-face route
**0x353860 →0x397550**. Registered-directory retries pass no named map.
Other extensions use the separately specified vector-font route.

The first requested resource1.dat can therefore come from an exact named
memory entry or a file; later same-basename requests can reuse the already
loaded pointer. Merely finding a269130 filename literal does not establish
the effective byte source or a later cache reload.

**Unresolved producer:** this bounded startup/font/PXL investigation did
not establish a reached insertion of resource1.dat or resource2.dat byte
spans into that FontEngine map. The superficially similar
0x344290 →0x33cca0 path belongs to another GUI resource-object map, not this
named font-byte registry. No global absence, alias-free lifetime or
registration/replacement policy is inferred. Establishing effective
runtime byte provenance still requires the actual registration caller,
or registry/cache state before the first load. No owner file or runtime
observation was used to fill this limit.

## 3. Capitalization and HUD/template distinctions

The title draw **0x291210** supplies mixed-case strings, including
“Start Game” and “Options”. The card's0x26eba0 likewise supplies mixed-case
text and saved names. The reached helper0x269210 and font draw0x34fe40 do
not uppercase those display strings. The glyph loop passes each unchanged
UTF-16 code unit to lookup; scalable glyph0x351b20 passes that code unit
unchanged to0x396c00. The extension-only uppercase operation in0x352df0
does not transform display text.

Thus the old frontend-title§0.1 assertion “capital letters only, so
mixed-case strings appear in capitals” is **not established by this native
code path**. Actual glyph availability, cmap aliases and uppercase-looking
glyph outlines depend on the effective font data. None is replaced here
by an assumed uppercase transformation or an inferred visual appearance.

Program-created in-game text can request the same resource: examples
include identification0x277060, general feedback0x28ebb0 and Preview
0x28c0a0 through the immediate helper; chat0x26a2e0 selects resource1.dat
through the same resolver for its separate direct text path. The explicit
TextShape pairs0x1008a0 and transient labels0xb6240 also name resource1.dat.
This is not proof that every owner template label uses that face.

Template-backed TextShapes load their own data-supplied font name. Their
rebuild **0x368b90** separately retries **arial.ttf** when the requested
font fails. That fallback does not occur in0x269130/0x269210. A Button's
data-owned child text is not converted into an immediate BaseWidget font
merely because both appear on a character-selection screen. Template font
contents/selection beyond the verified native assignments remain data inputs.

## 4. Logo inputs use scene pixels and the global GUI scale

Game scene call **0xad9d4** supplies signed32 window dimensions from
**Game+0x1d60/+0x1d64** and float GUI scale **Game+0x664**; the latter is
loaded at0xad970 into scene argument14. Scene/overlay width and height
are renderer **+0x41c/+0x420**. These are the window/viewport dimension
inputs, **not divided by GUI scale** and not a Widget's logical frame.
This statement does not add an untraced OS DPI-virtualization contract.

With title-logo flag0x40, **0xe3fe2–0xe4129** selects catalog slot0xc31,
whose existing owner-file identity is **sprites/cubeworld.cub**. Its
screen center is **W×0.5**, and **H times (0.25 plus cosine times0.01
times5)**, preserving the float multiplication order. The cosine phase
uses the unsigned timeGetTime remainder before float conversion; its
precise widening/pi/cosf sequence is already specified in
[WORLD controls §4](steam-world-pass-controls.md#4-screen-overlays-and-exact-logo-lighting).

The caller computes uniform scale by multiplying **g** by float
**0.010999999940395355**, word **0x3c343958 at0x43ef38**, then multiplying
by the separately computed float **1024/H** (1024 at0x43efcc). g is the
same global GUI-scale input used elsewhere by the interface. It is not
BaseWidget's font size, outline, copied frame size or current text style;
the logo does not inherit a second Widget/root scale at the queued draw.

## 5. Full-allocation model transform and queued projection

Logo submission **0xe4129 →0xee5a0** uses white RGBA, Glow0, Shininess0
and DepthOffset0. Let c be the supplied cosine and s the scale above.
In the existing column-vector convention, its model matrix is the product
of translation(0,0,1), X rotation(20−5c degrees), Y rotation180 degrees,
Z rotation0 degrees, uniform scale s, and local translation by the
negative half allocation dimensions, **in that matrix order**.

The helper appends X, then Y, then Z rotations through0x2b360/0x2b470/
0x13050, then scale0x2b580. Centering uses signed32 full header dimensions
**resource+0x88/+0x8c/+0x90**, converted to float and multiplied by−0.5,
then transformed through the rotated/scaled basis. It is not an occupied
voxel/vertex AABB center or unscaled subtraction from the final translation.
For a point, centering/scale act before the rotations, and the Z rotation
acts before Y before X; the older title wording about rotations being
applied to points in append order was misleading.

The ordinary CUB packed vertex decoder recovers voxel-corner integers by
multiplying normalized position bytes by255. Those are the local units
consumed by this matrix. There is no extra0.1/0.01 voxel-to-block factor
or65536 fixed-coordinate conversion in this queued logo path. The actual
shape/allocation/occupied extent remains the owner's model data.

Material alpha1 selects the normal opaque resource tree in screen queue
**renderer+0x18**. The verified queued draw contract is the same
[Ring rendering §§3–4](steam-ring-selector-rendering.md#3-queued-transform-and-descriptor0xee5a0):
consume through **0xe6e60(mode1)** after identity VIEW and perspective
**50 degrees**, float radians **0.8726646900177002**, with aspect
**−float(W)/float(H)**, near0.05 and far float(zone-window count shifted6).
This is positive view Z and clip W=view Z, depth0..1, not the direct GUI
item icon's45-degree camera or the Plasma orthographic projection.

Mode1 applies a homogeneous projection offset per descriptor: subtract
float(W)×0.5 from screen X, divide by float(W), then add that quotient to
itself; subtract float(H)×0.5 from screen Y, divide by float(H), then
multiply by−2. The NDC depth translation is **−0.5**. The offset matrix is
on the left of the base projection, translating homogeneous clip coordinates
after that base projection; it is not a model Z change
and is separate from the descriptor's shader DepthOffset0. Screen centers
are not multiplied by GUI scale again.

Consequently voxel-to-pixel magnification follows the actual transformed
view depth and this perspective. A center-plane scale calculation is not
a constant pixel width for every rotated voxel or a measured logo box.
Readiness/resource-group order and inherited target/depth/palette controls
remain the existing queued contracts; the logo has no text substitute or
invented model when its catalog resource is unavailable. These screen
overlays are consumed before Plasma root traversal, outside a Button's
font/frame/clip callback path.
