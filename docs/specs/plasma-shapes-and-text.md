# Plasma: shapes, tessellation and text

Behavioural specification of the shape and text half of Plasma, the interface library that
Steam Cube World links statically. Written from the decompiled executable, the binary's
constants and the shipped data. It is meant for an implementer who never sees the game's
code; it contains no code.

Companion document: `plasma-format-and-scene.md` covers the .plx chunk format, attributes
and animation, the node tree, the draw traversal, clipping layers and widgets. This document
covers what a shape **is** (its values), how it is **turned into triangles**, and how it is
**drawn**; and the whole text path (fonts, glyphs, layout, drawing).

Conventions used throughout:

* Routines are cited by RVA (image base 0x140000000). Class names are the binary's RTTI
  names. "Slot n" means the n-th entry of a class's virtual table (0-based).
* 2D coordinates: x to the right, y **down**. On screen one unit of the world matrix is one
  pixel. Text "up" is negative y.
* Matrices are 4×4, **row-vector** form: a point p becomes p·M. Rows 0 and 1 are the x and y
  axes, row 3 is the translation. "Apply X before M" means M ← X·M.
* The **live value** of an animated property is its value at the current animation state
  (companion spec, 2.6). Array properties have one array per frame; the live array is used.
* Colours are RGBA floats, nominally 0 … 1.
* `w` (smooth weight), `L` (effective subdivision level), `k = 2^L` are defined in 2.4.

---

## 0. The classes

| RTTI class | vtable | ctor / dtor | Loaded from .plx | Draws itself |
|---|---|---|---|---|
| `Shape` | 0x47e9d0 | 0x38cef0 / 0x38cfd0 | – (base) | no |
| `MeshShape` | 0x47be50 | 0x36c5d0 / 0x36da40 | no | no (slot 2 is empty) |
| `SmoothMeshShape` | 0x47acc8 | 0x3589a0 / 0x358fc0 | yes (reader 0x383e30) | yes (slot 2 = 0x35eb70) |
| `StaticMeshShape` | 0x47e8f8 | 0x38ca60 / 0x38cac0 | only by conversion, see 3.1 | yes (slot 2 = 0x38cbc0) |
| `GenericShape` | 0x47c460 | 0x37a890 / 0x37a960 | yes (reader 0x3827e0) | no |
| `CurveShape` | 0x47c318 | 0x377b40 / 0x3782f0 | no | no |
| `TextShape` | 0x47b130 | 0x367590 / 0x367a30 | yes (reader 0x384bf0) | yes (slot 2 = 0x368020) |
| `Drawing` / `D3D11Drawing` | 0x47c270 / 0x480650 | – | – | the triangle container all shapes draw through |
| `FontEngine` | 0x479bc0 | – | – | font cache and loader |
| `ScalableFont` | 0x47a1a0 | 0x34ebf0 | – | the only font class the game uses (FreeType based) |
| `Font`, `PixelFont`, `PlasmaFont` | 0x47eaf0, 0x47cdb8, 0x47cc38 | – | – | not reached for .ttf/.dat files, see 6.3 |

Shared slot meanings (shape classes): slot 1 = rebuild (argument: "force"), slot 2 = draw
(argument: the node being drawn), slot 3 = hit test of a point with a tolerance radius,
slot 4 = point inside the cached bounds, slots 6 / 7 = bounds minimum / maximum, slot 13 =
clone. TextShape slot 14 (0x368c60) recomputes its bounds.

`SmoothMeshShape` derives from `MeshShape`; MeshShape holds all the vertex arrays and the
face list, SmoothMeshShape adds subdivision, stroke and drawing. `GenericShape` and
`CurveShape` never produce triangles (3.2, 3.3).

---

## 1. SmoothMeshShape: the stored values

### 1.1 Properties

All names are prefixed `SmoothMeshShape.` in the file. "Array" means one entry per vertex.
"Animated" means the value has frames (companion spec 1.6).

| Property | Value | Default | Meaning |
|---|---|---|---|
| `name` / `wname` | string | empty | object name |
| `face` (repeated) | list of int32 vertex indices | – | one polygon per occurrence, in file order. 2-vertex faces are segments (stroke only) |
| `vertexPositions` | animated array of 2D points | – | control points |
| `vertexTexCoords` | animated array of 2D points | – | fill texture coordinates |
| `vertexColors` | animated array of RGBA | – | fill colours |
| `strokeColors` | animated array of RGBA | – | stroke colours |
| `strokeWidths` | animated array of float | – | **full** stroke width at each vertex |
| `extrusionFrontColors`, `extrusionBackColors` | animated arrays of RGBA | – | extrusion colours (3.4) |
| `vertexFlags` | array of int32 | 0 | per-vertex rule selector (1.3) |
| `vertexParameters` | array of float | 0 | parameter for the handle rules (2.3) |
| `texture`, `strokeTexture` | animated int | −1 | texture id (−1: none). Remapped at load through the file's `Texture.id` table |
| `textureTranslation`, `texturePivot` | animated 2D | (0, 0) | fill texture transform (2.12) |
| `textureRotation` | animated 3D | (0, 0, 0) | degrees about X, Y, Z |
| `textureDeformation` | animated 4×4 matrix | identity | |
| `textureOpacity`, `textureBrightness`, `textureContrast`, `textureSaturation` | animated float | 1 | fill texture adjustments (2.13) |
| `strokeTextureOpacity`, `…Brightness`, `…Contrast`, `…Saturation` | animated float | 1 | stroke texture adjustments |
| `strokeTextureStretch` | animated float | 1 | stroke texture length scale (2.12) |
| `extrusionMatrix` | animated 4×4 matrix | identity | 3.4 |
| `subdivisions` | int | 3 | clamped to 0 … 6 when set (0x360620) |
| `smoothWeight` | float | (√2 − 1) / (√2 × 0.5) ≈ 0.585786 | `w` below (ctor 0x3589a0) |
| `flags` | int | 0 | 1.2 |
| `strokeJointType` | int | 0 | 1 = round; any other value = miter join (2.10) |
| `strokeCapType` | int | 0 | 0 = butt, 1 = round, 2 = square (2.10) |
| `strokeAlignment` | int | 0 | offset of the stroke across the outline, in half-widths (2.10) |
| `strokePattern` | int | 0 | 0 = solid, non-zero = dashed (2.11) |
| `strokeDash`, `strokeGap` | float | 1, 1 | dash and gap lengths, in stroke widths |

Fixed, not stored: round-cap segment count 20 (ctor 0x3589a0); an "adaptive level" switch
that is never turned on (2.4).

Reader order of effects (0x383e30): every property is read in file order; after the object
ends, `smoothWeight` is stored, then the face list is applied (0x3605e0 → 0x371770, which
also resizes the per-vertex arrays texcoords, colours, stroke colours, stroke widths, flags
and parameters to the vertex count of the **first frame** of `vertexPositions`, new entries
zero), then `subdivisions` is applied (0x360620, clamp, topology rebuild 0x35d580, rebuild).
The level-0 topology (0x35d580) also resizes both extrusion colour arrays to the vertex count.

### 1.2 `flags`

| Bit | Meaning |
|---|---|
| 0x01 | **Outline mode.** The fill is built from the smoothed outline curve with a polygon tessellator (2.9). Surface subdivision is not run |
| 0x02 | Stroke enabled |
| 0x04 | Fill **disabled** |
| 0x08 | Open outline: the closing segment of each outline loop is dropped (strokes get caps) |
| 0x10 | Extrusion enabled |

Values found in the shipped files (counts of shapes): gui.plx 6 (145), 0 (108), 1 (102),
3 (92), 14 (32), 2 (20); icons.plx + quest-tag.plx + cursor.plx 1 (912), 0 (51), 6 (20),
3 (1). Extrusion (0x10) and dashed strokes never occur.

### 1.3 `vertexFlags`

The **low 16 bits** select the smoothing rule of a control point:

| Low 16 bits | Name used here | Effect |
|---|---|---|
| 0 | smooth | normal smoothing |
| 1 | sharp | the point never moves; edges touching it become creases; stroke joins are built (2.10) |
| 2 | linear | a boundary point that stays on the straight line between its neighbours |
| 3 | weighted corner | like linear on the first level only, with skewed midpoints (2.7), then sharp |

The **high 16 bits** select a handle rule (2.3): 1 = handle owner, 2 = interpolated point.

Shipped data: low values 0 (6640), 1 (4155), 3 (12), 2 (4) in the sampled entries; no high
bits seen.

---

## 2. SmoothMeshShape: from values to triangles

### 2.1 Order of operations of a rebuild (slot 1, 0x3622b0)

1. **Drawings.** The shape owns up to three drawings: fill (exists while bit 0x04 is clear),
   stroke (while 0x02 is set), extrusion (while 0x10 is set). Missing ones are created
   through the engine (D3D11Engine slot 12, 0x38ef90); unneeded ones are released.
2. **Dirty bits.** Attribute "changed" flags are folded into a mask: positions and stroke
   widths → bit 1; texcoords → bit 2; vertex colours, stroke colours and both extrusion
   colours → bit 4. A forced rebuild sets 0xf. Bit 1 is set unconditionally after step 4.
   The mask is handed to the drawings in step 10.
3. If an extrusion colour array's length differs from the vertex count it is resized.
4. `L` = effective level (2.4).
5. Handle adjustment on the live positions (2.3).
6. If `L` = 0: compute the texture matrix (2.12), look up both textures by id, and set the
   bounds to the box of all live positions (0x372490). Otherwise compute the texture matrix
   (0x3726a0), look up the textures, and, when the fill exists and bit 0x01 is clear, run the
   surface subdivision (2.6–2.8; 0x360670) which writes the fill drawing.
7. For every outline loop (2.2): seed the curve arrays (2.5), subdivide them `L` times, and
   if the stroke is enabled build its ribbon (2.10).
8. Bounds (2.14).
9. Mark the shape changed.
10. Build the drawings (0x364c40): copy the dirty mask into each drawing; if bit 0x01 is set
    and the mask has bit 1, also flag each drawing for outline extraction (0x3751b0); clear
    the mask; set textures and texture parameters (0x366640, 2.12–2.13); then
    * fill, `subdivisions` = 0 and bit 0x01 clear: fan fill (2.6);
    * fill and bit 0x01 set: outline fill (2.9);
    * extrusion (3.4);
    * stroke: solid (2.10) or dashed (2.11);
    * upload every drawing (2.15) and report the bounds (slot 8).

### 2.2 Topology: faces and outline loops (0x371770)

1. Store the face list.
2. Build the set of **boundary half-edges**. For each face with at least 3 vertices, for each
   consecutive pair (a → b, wrapping): if the opposite half-edge (b → a) is already in the
   set, remove it; otherwise insert (a → b). A face with exactly 2 vertices inserts both
   (a → b) and (b → a). Faces with fewer than 2 vertices are ignored.
3. Build an adjacency list: walk the faces again in order; for every consecutive pair
   (a → b) that is in the set, append b to the list of a (insertion order kept).
4. Chain the loops. While any vertex still has outgoing half-edges:
   * start vertex = the first vertex of face 0 if there is exactly one face and that vertex
     has outgoing half-edges; otherwise the smallest vertex index that has any;
   * from the current vertex take the **first** remaining outgoing half-edge, append the
     current vertex to the loop, remove the half-edge (and the vertex entry if its list is
     now empty), move to the half-edge's target; stop when the target is the start vertex or
     the current vertex has no outgoing half-edge.

The loops are the outline: the curve fill, the stroke and the extrusion follow them. A closed
polygon gives one loop in its own vertex order; a 2-vertex face gives the loop (a, b).

### 2.3 Handle rules (0x371f70)

Run on every rebuild, **modifying the live `vertexPositions` frame in place**. For each loop
(length n), with P the positions and t the vertex parameter:

1. For every loop vertex v at position i whose high 16 bits are 1:
   * forward: target T = loop[(i+3) mod n] if that vertex's high bits are 1, else
     loop[(i+2) mod n]. If T ≠ P[v]: move loop[(i+1) mod n] to P[v] + (P[T] − P[v]) ×
     t[v] / |P[T] − P[v]|;
   * backward, the same with loop[(i−3)], loop[(i−2)] and moving loop[(i−1)].
2. Then for every loop vertex v whose high 16 bits are 2: A = the first vertex after v (going
   forward, at most n−2 steps) whose high bits are not 2; B = the first vertex before v
   (going backward) whose high bits are not 2. P[v] = P[A] × (1 − t[v]) + P[B] × t[v].

The shipped data never uses these rules.

### 2.4 Levels and the smooth weight

* `w` = `smoothWeight`.
* `L` (0x35b460) = `subdivisions`. An unused adaptive variant exists: if an internal switch
  were on and `subdivisions` > 2, `L` = min(`subdivisions`, ⌊ln(2560 / C) / ln 4⌋ + 1)
  where C is the total number of face corners (constants 0x47ad78 = 2560, 0x4aca20 = 4).
  Nothing sets the switch.
* `k` = 2^L (computed as `pow(2, L)`; `L` = 2 is special-cased to 4). Every outline
  segment becomes `k` curve segments.

### 2.5 Seeding the outline curves (0x3622b0, first loop)

For each loop of n vertices: the curve has M points, M = n·k when closed, M = (n − 1)·k + 1
when open (bit 0x08). Loop vertex j is copied to curve slot j·k, taking its live position,
live colour (`vertexColors`), live texcoord and its **full** `vertexFlags` value; when the
stroke is enabled also its live stroke colour and stroke width (a width below 0.1 is stored
as 0.1, constant 0x42e528); when extrusion is enabled its two extrusion colours. All other
slots start at zero, with flag 0.

Then, for level index i = 0 … L−1 with stride s = 2^(L−i) and segment count M / s, apply the
curve subdivision step (2.7) to: positions (0x3574c0), texcoords (0x3574c0 again), colours
(0x357a80), and when stroking stroke colours (0x357a80) and widths (0x356bd0); extrusion
colours use a plain variant (0x358300, 3.4). Every call reads and rewrites the same flag array;
the result is identical whichever attribute runs first.

### 2.6 Fill, `subdivisions` = 0 (0x364c40, first part)

When the fill exists, `subdivisions` = 0 and bit 0x01 is clear:

* Vertex buffer: one vertex per shape vertex: live position, live `vertexColors` colour, live
  texcoord.
* Index buffer: for each face with n ≥ 3 vertices (v0 … v(n−1)), the fan (v0, vi, vi+1) for
  i = 1 … n−2. No other triangulation is attempted: concave polygons draw as their fan.

### 2.7 The curve subdivision step (0x3574c0; same rules in 0x357a80 and 0x356bd0)

Input: the value array, the flag array, the stride s, the segment count m, closed or open,
`w`, and the level index i. Point indices wrap modulo the array length. "Flag" means the low
16 bits.

Weighted-corner midpoint weight: c = 1 / (2·cos(π/4) + 1) ≈ 0.414214 (0x47ad6c = π/4,
0x4ac9d0 = 2, computed once, 0x140553580).

1. **Insert midpoints.** For each segment j = 0 … m−1, with a = point j·s, b = point
   (j+1)·s, the new point at j·s + s/2 gets a·α + b·(1 − α), where α = 0.5, except on the
   first level (i = 0): α = c when a's flag is 3 and b's is not, α = 1 − c when b's flag is 3
   and a's is not. Its flag becomes 2 if a or b has flag 2, or (first level only) flag 3;
   otherwise 0.
2. **Move the old points.** For old points j = 1 … m (closed) or 1 … m−1 (open), with
   neighbours L = point j·s − s/2 and R = point j·s + s/2 (both just inserted):
   * flag 1, or flag 3 after the first level: unchanged;
   * flag 0: P ← P + ((L + R)/2 − P) × w;
   * otherwise (2, or 3 on the first level): P ← (L + R)/2.
3. **Straighten linear runs.** For each segment j = 0 … m (closed) or 0 … m−1 (open): if both
   end points have flag 2 (or 3 on the first level), the midpoint is recomputed as the
   average of the (now moved) end points and its flag becomes 1.

With w = 0.5 step 2 is the cubic B-spline rule; the game's default w ≈ 0.5858 pulls points a
little further toward the neighbour average.

### 2.8 Surface subdivision (bit 0x01 clear, `L` ≥ 1)

#### 2.8.1 Topology (0x35d580, 0x38b8e0, 0x38bd80)

Built whenever faces or `subdivisions` change. Only faces with at least 3 vertices take part.

* **Level 0.** For each polygon f with corners v0 … v(n−1): one face point F_f; one edge
  point per **undirected** polygon edge (shared by both polygons that use it); and n quads
  Q(f, i) = (F_f, E(vi, vi−1), vi, E(vi, vi+1)). Quad edges are deduplicated by their end
  points; a quad edge with only one quad marks both end points **boundary**. Each polygon
  edge also knows the polygon across it, or "none".
* **Level k → k+1.** Each quad (c0, c1, c2, c3), with edges ej = (cj, cj+1), face point FQ
  and edge points Ej, splits into the 4 quads (FQ, E(j−1), cj, Ej), j = 0 … 3. New edges:
  4 inner edges FQ–E(j−1), and each old edge splits into E–(each end point). The vertex list
  of level k+1 is: all level-k vertices (the **same** objects), then the level-k edge
  points, then the level-k face points. Boundary is recomputed from the new edges.
* There are `L` levels numbered 0 … L−1. A vertex object keeps its identity across levels, so
  its position is refined in place.

#### 2.8.2 First step: polygons to level 0 (0x360670)

Ordered steps:

1. Size the fill drawing to the vertex count of level L−1 and point every vertex at its slot
   (position, colour, texcoord). Index buffer: every level L−1 quad (c0, c1, c2, c3) gives
   the triangles (c0, c1, c2) and (c2, c3, c0).
2. Reset every control vertex: accumulators, count, flags.
3. For each face and each corner i whose edge (vi, vi+1) has no polygon across it: mark vi
   and vi+1 **boundary**. (2-vertex faces mark their ends.)
4. For each control vertex that is part of the mesh: copy its live position, colour and
   texcoord; from the low 16 bits of its `vertexFlags`: 1 → sharp, 2 → linear, 3 → linear
   and weighted corner.
5. **Face points**: average of the polygon's corners (position, colour, texcoord).
6. **Edge points**, per undirected edge (a, b) with polygons f1 and maybe f2: the edge's
   flags lose "sharp", "linear" and "weighted corner".
   * Crease case (no polygon across, or a sharp, or b sharp): E = a·α + b·(1 − α) with α as
     in 2.7 using "weighted corner" (c when only a has it, 1 − c when only b has it, else
     0.5). If a or b is linear, E becomes linear.
   * Smooth case: E = ((a + b)·(1 − w) + (F_f1 + F_f2)·w) / 2.
7. **Edge midpoints into vertices**: for each polygon and each of its edges (a, b) (so an
   interior edge is visited **twice**, once per polygon): M = a·α + b·(1 − α) with the same
   α; add M to a's accumulator (count + 1) if the edge has no polygon across or a is not
   boundary; the same for b.
8. **Face points into vertices**: each polygon adds its face point to every corner that is
   not boundary (count + 1).
9. **Move control vertices** (position, colour and texcoord alike; A = accumulator / count):
   * sharp, or count = 0: unchanged;
   * not boundary: P ← 0.25·P + 0.75·A;
   * boundary, not linear: P ← (1 − w)·P + w·A;
   * boundary and linear: P ← A.
10. For each edge with no polygon across whose both ends are linear: E ← (a + b)/2 using
    the moved ends; E loses linear, becomes sharp.
11. Every weighted-corner control vertex loses linear and weighted corner and becomes sharp.
12. Run the level step (2.8.3) on levels 0 … L−2.

For an interior valence-4 vertex steps 7–9 give the classic Catmull–Clark vertex rule
(P/4 + R/2 + F/4, R = average midpoint, F = average face point).

#### 2.8.3 Level step (0x38c7d0)

Seven passes over one level; the first, third and sixth run in parallel in the original but
are independent per element.

1. Reset every vertex accumulator and count (0x3898d0).
2. **Face points** (0x388c50): FQ = average of the 4 corners.
3. **Edge points** (0x389520): flags lose sharp and linear. If the edge has two quads and
   neither end is sharp: E = ((a + b)·(1 − w) + (F1 + F2)·w) / 2. Otherwise E = (a + b)/2,
   and E becomes linear if a or b is linear.
4. **Edge midpoints into vertices** (0x388df0): M = (a + b)/2; added to each end that is not
   boundary, or to both when the edge has a missing quad. Each edge is visited **once** (not
   twice as on level 0).
5. **Face points into vertices** (0x3890f0): each quad adds FQ to its non-boundary corners.
6. **Move vertices** (0x3891b0): same four cases as 2.8.2 step 9.
7. **Linear boundary edges** (0x388fa0): an edge with a missing quad whose ends are both
   linear: E = (a + b)/2, E loses linear and becomes sharp.

Because of pass 4, an interior valence-4 vertex moves to P/4 + 3R/8 + 3F/8 on these levels,
not to the level-0 rule. This is what the game does.

### 2.9 Outline fill (bit 0x01 set; 0x364c40, second part)

Uses the subdivided curves (2.5) of **all** loops at once:

1. Means: the average position (x̄, ȳ) and average colour C̄ over every curve point.
2. Least-squares colour plane: with dx = x − x̄, dy = y − ȳ, accumulate Sxx = Σdx²,
   Syy = Σdy², Sxy = Σdx·dy, and per channel SxC = Σdx·(C − C̄), SyC = Σdy·(C − C̄).
   D = Sxx·Syy − Sxy². Gradient G = (SxC·Syy − SyC·Sxy) / D, H = (SyC·Sxx − SxC·Sxy) / D.
   (No guard: D = 0 gives non-finite colours.)
3. Evaluate C̄ + G·dx + H·dy at every curve point; per channel find lo = min(0, all values)
   and hi = max(1, all values); the vertex colour is (value − lo) / (hi − lo). Inside 0 … 1
   this is the plain plane value.
4. Vertex buffer: every curve point (position, that colour, its subdivided texcoord), loop
   after loop; each loop is a contour of vertex indices.
5. Triangulate with the GLU polygon tessellator (0x376a10): winding rule NONZERO (value
   100131, 0x47c288), all contours in one polygon, an edge-flag callback so only plain
   triangles are emitted, a vertex callback that appends the vertex index (0x376970), and a
   combine callback (0x3765a0) that appends a new vertex whose position, colour, second
   colour and texcoord are the weight-sum of the four given vertices.

### 2.10 Stroke ribbon (0x3622b0, second loop; triangles in 0x364c40)

Per outline loop, with the subdivided arrays of M points (positions p, stroke colours, widths
W), alignment a = `strokeAlignment` as a float, and h(j) = W(j)·0.5. The ribbon is a list of
vertex **pairs** (A side, B side); each pair gets the point's stroke colour twice and the
texcoords (u, 0) and (u, 1); u starts at 0 for each loop.

Visit j = 0 … M (closed: M+1 visits, j taken mod M, the first point is visited again at the
end) or 0 … M−1 (open). Record, for each visit, the index of its first pair. Then:

Zero-length fallbacks use a control point further away: backward index
b(j) = ⌊(j − k − 1 + M)/k⌋·k mod M, forward index f(j) = ⌊(j + k + 1)/k⌋·k mod M (integer
division).

* **Open start** (open and j = 0): d = p0 − p1 (if zero, d = p0 − p(f(0))).
  n = (−d.y, d.x)/|d|. Pair: A = p + n·h·(1 + a), B = p + n·h·(a − 1).
* **Open end** (open and j = M−1): d = p(j−1) − p(j) (if zero, d = p(b(j)) − p(j)). n and
  the pair as above with h(j).
* **General point**: e1 = p(j−1) − p(j) (if zero, p(b(j)) − p(j)), e2 = p(j) − p(j+1) (if
  zero, p(j) − p(f(j))). n1, n2 = the
  perpendiculars (−e.y, e.x) normalised (a zero vector stays zero). Bisector m = n1 + n2
  normalised (zero if n1 + n2 is zero).
  * If j is a control point (j mod k = 0) whose flag low 16 bits are 1 (sharp): miter scale:
    take n = n1 (or n2 if n1 is zero), m ← m / max(m·n, 0.3) (0x42e704).
  * Let m' = m·h and o = m'·a (alignment offset).
  * Non-sharp points: one pair A = p + m' + o, B = p − m' + o.
  * Sharp control points:
    * if n1·n2 ≥ 0.95 (0x47ad70): one pair as above;
    * else if `strokeJointType` = 1 (round): θ = acos(n2·n1) in degrees (×57.29578,
      0x42e768); steps S = int(|θ| / 180 × 20) (0x42e7d4, 0x42e5a0); negate θ when
      m·e2 < 0 (unnormalised e2); step angle δ = θ / S. Scale n1, n2 by h; v̄ = (n1 + n2)/2.
      Emit the pair (p + n1 + o, p − n1 + o). Then S − 1 pairs: a rotating vector r starts
      at n1 and is rotated by δ degrees each time (counter-clockwise for positive δ:
      (x, y) → (x·cos − y·sin, x·sin + y·cos)); if m·e2 ≥ 0 the pair is (p + r + o,
      p − v̄ + o), else (p + v̄ + o, p − r + o). Finally the pair (p + n2 + o, p − n2 + o).
    * else (miter): n1, n2 scaled by h, v̄ = (n1 + n2)/2; three pairs:
      (p + n1 + o, p − n1 + o); then if m·e2 ≥ 0: (p + m' + o, p − v̄ + o), else
      (p + v̄ + o, p − m' + o); then (p + n2 + o, p − n2 + o).
* After each visit: u ← u + 2·|p(j) − p(j+1)| / (W(j) + W(j+1)) (indices mod M). u is
  therefore measured in stroke widths.

**Caps** (open loops only; 0x364c40 around the call sites 0x3662a7 / 0x3663c9). The cap pairs
are written straight into the stroke drawing: the start cap before the loop's ribbon pairs,
the end cap after them.

* Round cap (`strokeCapType` 1, 0x35b6d0), 20 pairs. Using ribbon pair j (A_j, B_j) and pair
  j+1 (two vertices later), and t (0 for the start: j = first pair; 1 for the end: j =
  second-to-last pair): centre c = mix(mid_j, mid_{j+1}, t), r = mix(A_j, A_{j+1}, t) − c,
  q = (r.y, −r.x) (points outward at the start, inward at the end). For i = 0 … 19:
  φ = (i + 0.5)·(π/2) / 20.5, plus (0.5/20 + 1)·(π/2) for the end cap (0x42e740, 0x47ad74).
  Pair: c + q·cos φ + r·sin φ and c + q·cos φ − r·sin φ. Colour = mix of the two pairs'
  colours by t. Texcoords: u = mix(u_j, u_{j+1}, t) − Δ·cos φ with Δ = (u_{j+1} − u_j)·|r| /
  |mid_j − mid_{j+1}|; v = 0.5 − 0.5·sin φ and 0.5 + 0.5·sin φ (0x42e80c).
* Square cap (`strokeCapType` 2, 0x35bef0), one pair: c and r as above, c' = c + q (start) or
  c − q (end, 0x4aca4c = −1); pair c' + r, c' − r; same colour rule; u extended by ∓Δ.
* Butt (0 or anything else): nothing.

**Triangles.** For the loop's n ribbon vertices (including caps) starting at index b: T = n
(closed) or n − 2 (open) triangles; triangle t (t = 0 … T−1) uses vertices
b + (t mod n), b + ((t + 1 + (t and 1)) mod n), b + ((t + 1 + ((t+1) and 1)) mod n). That is
a triangle strip with alternating winding; closed loops wrap.

### 2.11 Dashed strokes (0x35cac0; used when `strokePattern` ≠ 0 and `strokeGap` ≠ 0)

Summary only (never used by the shipped files): the loop length λ is measured in stroke
widths as u above; the dash count is ⌊λ / (dash + gap)⌋; if zero nothing is drawn; otherwise
dash and gap are scaled so that count × (dash + gap) = λ. Each dash is an open sub-ribbon cut
from the ribbon pairs at fractional positions (0x35f050 walks a distance, 0x35c540 emits an
interpolated pair), with the loop's cap type at both ends (round caps use 10 segments, half
of 20).

### 2.12 Texture matrices (0x3726a0 / 0x372490, 0x366640)

Fill texture matrix T, built from identity in this order (each step "applied before" the
matrix built so far, row-vector form): translate by `textureTranslation`; translate by
`texturePivot`; rotate about X by `textureRotation.x`, then about Y by `.y`, then about Z by
`.z` (degrees × π/180, 0x42e520; rotation about Z maps (x, y) to (x·cos − y·sin, x·sin +
y·cos)); apply `textureDeformation`; translate by −`texturePivot`. A texcoord t becomes
t·T, i.e. ((t − pivot)·D) rotated by Z, Y, X, then + pivot + translation.

Stroke texture matrix: identity, then if `strokeTextureStretch` > 0, scale u by
1 / `strokeTextureStretch` (v unchanged).

The extrusion drawing uses the fill texture and its matrix.

### 2.13 Per-drawing parameters (0x366640)

| Drawing | Texture | Opacity, brightness, contrast, saturation | Texture matrix |
|---|---|---|---|
| fill | `texture` | `texture*` | fill matrix |
| stroke | `strokeTexture` | `strokeTexture*` | stroke matrix |
| extrusion | `texture` | `texture*` | fill matrix |

How the four numbers act is in 2.15.

### 2.14 Bounds

Box (min, max) of: the stroke ribbon pairs if the stroke is enabled; otherwise the
level L−1 quad corners if bit 0x01 is clear and L ≥ 1; otherwise all curve points; with
extrusion, also the bounds transformed by the extrusion matrix. With L = 0 the box of the live
positions (0x372490). Reported to the node through slot 8.

### 2.15 Drawing a shape: matrix and tint

**Draw order** (SmoothMeshShape slot 2, 0x35eb70): extrusion (if enabled), then fill (if
enabled), then stroke (if enabled). Each passes the engine's pass colour: extrusion
engine +0x278, fill +0x258, stroke +0x268. All three are (1, 1, 1, 1) (set in 0x339b70);
only the vector-font path (6.3) changes them.

**Vertex format** (D3D11Drawing upload, 0x391b40): 32 bytes: position x, y; texcoord u, v;
colour r, g, b, a. 32-bit triangle-list indices.

**One draw** (D3D11Drawing slot 2, 0x3913d0), node N:

1. Tint = N's live `Display.fillColor` × pass colour (component-wise). Without a node: the
   pass colour.
2. World = the engine's current matrix (the node's world matrix, companion 2.2). If the
   engine's depth-stack option is on, the world gets a Z translation of −2.0 × a running
   counter (0x42e60c), and the counter increases.
3. Mask matrix = World followed by a scale (1 / screen width, 1 / screen height).
4. Mask mode from N's display (companion 2.3.3): 0 when N's live clipping is 0.
5. Texture parameters (opacity, brightness, contrast, saturation) and the drawing's texture
   matrix; "texture enabled" = the drawing has a texture.

**Vertex shader** (DXBC at 0x47ebd0): clip position = Proj · (World · p − (0.135, 0.135))
(a −0.135 pixel offset after the world transform); colour = vertex colour × tint; mask
coordinate = mask matrix · p; texcoord = T applied to (u, v) (affine 2D). An optional widget
deformation (companion 2.7.4) runs before World.

**Pixel shader** (DXBC at 0x47f940):

1. c = interpolated colour.
2. If a texture is enabled: s = texture sample. If saturation ≠ 1:
   g = 0.3·s.r + 0.59·s.g + 0.11·s.b; s.rgb ← s.rgb + (1 − saturation)·(g − s.rgb). If
   brightness ≠ 1: s.rgb ← s.rgb × brightness. If contrast ≠ 1: s.rgb ← s.rgb + (1 −
   contrast)·(0.5 − s.rgb). s.rgb ← min(s.rgb, 1). c ← c + opacity·(c·s − c) (a mix between
   the untextured colour and colour × texture).
3. Mask modes, with m = mask sample, m.rgb divided by m.a when m.a > 0:
   * 1: c.rgb ← mix(c.rgb, m.rgb, m.a); alpha unchanged;
   * 2: c ← c × m;
   * 3: c.a ← c.a × m.a.
4. Output c.

---

## 3. Other shape kinds

### 3.1 StaticMeshShape

A frozen SmoothMeshShape. The reader converts a SmoothMeshShape into one (0x35ea00, which
takes over its three drawings) only when the file is loaded with flag 0x10 and the shape has no
animated attribute (0x369010 returns 0). No shipped load uses 0x10 (gui.plx, icons.plx and one
more file are loaded with 0x20, one with 0). Drawing (0x38cbc0): extrusion with tint
(1, 1, 1, 1), then fill with pass +0x258, then stroke with pass +0x268.

### 3.2 GenericShape

Values: `name`/`wname`, `source`/`wsource` (string), `position` (2D, not animated), `size`
(2D, not animated). It has no draw. It is a placeholder that game code reads. No shipped .plx
contains one.

### 3.3 CurveShape and MeshShape

`CurveShape` is created only by the object manager (0x33ca50) and an import path (0x3435a0);
its draw slot is empty. `MeshShape` is only used as the base of SmoothMeshShape (its own draw
slot is empty).

### 3.4 Extrusion (bit 0x10)

Summary only; never used by the shipped files. The extrusion drawing receives every curve point
(position, front colour, back colour, texcoord) loop by loop; extrusion colours are subdivided
by the plain variant 0x358300 (midpoints only); then 0x374830 tessellates the outline (2.9's
tessellator), extracts the boundary loops of the result (0x3751b0), computes each loop's signed
area, and builds the front cap, the back cap moved by the live `extrusionMatrix`, and the side
walls between them, with front and back colours.

---

## 4. TextShape: the stored values

### 4.1 Properties (reader 0x384bf0, ctor 0x367590)

| Property | Value | Default | Meaning |
|---|---|---|---|
| `name` / `wname` | string | empty | |
| `string` | animated wide string (one per frame) | empty | the text; UTF-16 code units |
| `color` | animated RGBA | (0, 0, 0, 1) | glyph colour |
| `strokeColor` | animated RGBA | (0, 0, 0, 1) | outline colour |
| `extrusionColor` | animated RGBA | (0, 0, 0, 1) | unused by TrueType fonts |
| `flags` | int | 0 | 4.2 |
| `pixelSize` | float | 0 | the size S (pixels per em) |
| `strokeRadius` | float | 0 | outline radius r (pixels); 0 = no outline |
| `spacing` | float | 0 | extra pen advance after each character |
| `lineSpacing` | float | 0 | extra distance between lines |
| `wrapWidth` | float | 0 | wrap width for flag 0x10 |
| `fontName` / `wfontName` | string | empty | font file name |

Legacy names, same fields: `fontSize` (int → S), `strokeWidth` (int × 0.5 → r, 0x4ac964),
and a `font` container with `fileName`/`wfileName` (font name), `pixelSize` (int → S),
`size` (float → S), `glowRadius` (int × 0.5 → r), `strokeWidth` (float × 0.5 → r);
`strokeGlow` and `pixelFont` are read and ignored. When the container ends, S and r are both
written (missing ones as 0). `strokeRadius` read alone replaces r.

When the file is loaded with flag 0x20, every TextShape gets the **scalable** bit (4.2). The
game loads gui.plx and icons.plx (and one more file) with 0x20.

Shipped data (gui.plx): fonts resource1.dat (155 shapes), tahoma.ttf (8), ana.ttf (5),
resource2.dat (4), verdana.ttf (1). Sizes mostly 12 (also 9, 16, 18, 25). Radius 0 (81), 2.5,
3, 2, 4. Wrap width 200 (some 400). Line spacing 0 (some 2, 3, 4). Flags 1, 0, 5, 2, 9, 6, 4,
and in other files 21, 10. Colours mostly white glyphs with opaque black outlines.

### 4.2 Flags

`TextShape.flags`:

| Bit | Meaning |
|---|---|
| 0x01 | centre each line horizontally on the origin |
| 0x02 | right-align each line on the origin |
| 0x04 | centre the block vertically on the origin |
| 0x08 | put the last line's baseline on the origin |
| 0x10 | word wrap at `wrapWidth` |

With no vertical bit the **first baseline** is at the origin (the text extends upward). With
no horizontal bit lines start at the origin.

Scalable bit (separate field, set only by load flag 0x20): chooses the scalable raster mode
(6.5). Code-made TextShapes (7.2) do not have it.

---

## 5. Fonts

### 5.1 Font files

The game ships two fonts, both plain TrueType files renamed .dat:

| File | Tables | unitsPerEm | head bbox | hhea asc / desc / gap | glyphs | name |
|---|---|---|---|---|---|---|
| resource1.dat | FFTM GDEF GPOS GSUB OS/2 cmap gasp glyf head hhea hmtx loca maxp name post | 1000 | (−125, −250, 1875, 1125) | 1125 / −250 / 62 | 232 | "resource1 Bold" |
| resource2.dat | FFTM OS/2 cmap cvt gasp glyf head hhea hmtx loca maxp name post | 1900 | (0, −400, 1700, 1500) | 1500 / −400 / 171 | 151 | "resource2 Medium" |

Both have cmaps (3,1) and (0,3) format 4 and (1,0) format 0, TrueType outlines, **no `kern`
table** (resource1's pair adjustments are in GPOS, which the game does not read), no `fpgm`
and no `prep`. head flags 0xb and 0x1f: both have bit 3 ("integer ppem") set.

Glyphs are read and rasterised by **FreeType 2**, statically linked (modules present:
truetype, type1, winfonts, psnames, pshinter, autofitter, raster1, smooth). Character codes are
the string's UTF-16 code units, looked up with FreeType's char-map lookup (default Unicode
map). Everything below is phrased in FreeType's terms; another rasteriser must reproduce
FreeType's results (including its ppem rounding for head flag bit 3, its unhinted metrics and
its stroker).

### 5.2 Loading (FontEngine 0x34d060, ScalableFont 0x352df0)

1. The cache key is the file name without any directory part (text after the last '/' or
   '\\'). A cached font is returned as is.
2. Otherwise a ScalableFont is created and loaded from the name as given (relative to the
   working directory, i.e. the game folder). If that fails, each registered font directory is
   tried as directory + "/" + key. At start-up the only directory is `c:\windows\fonts`
   (0x134cc0). If all fail, the font is discarded and nothing is cached.
3. A file whose extension (upper-cased) is "TTF" or "DAT" is opened as a FreeType face (from
   an in-memory registry entry of that name if one exists, else from disk: 0x353860). Any
   other extension goes to the vector-font loader 0x3530b0 (6.3).
4. On success the face's family, style and PostScript names are kept, and the face bbox
   divided by unitsPerEm is stored; a FreeType stroker is created.

A TextShape loads its font on rebuild (slot 1, 0x368b90) when it has none or the rebuild is
forced; if the named font cannot be loaded it uses `arial.ttf` (resolved through
`c:\windows\fonts`).

---

## 6. Text: rasterising, measuring, laying out, drawing

The engine entry is the text draw 0x34fa60 with: font, text, origin (x, y), size S, outline
radius r, spacing, line spacing, wrap width, colour, outline colour, extrusion colour, flags,
scalable bit. It returns the final pen position. TextShape calls it with origin (0, 0) between
the engine's node begin/end (slots 0x88/0x90). The measuring entry 0x352110 takes the same
values plus a start line and a mode.

### 6.1 Order of operations of one text draw (0x34fa60)

1. Read the engine's current matrix C.
2. Set the size (6.2), giving the raster sizes Sx, Sy, the raster radius, spacing and line
   spacing in raster units.
3. Replace the current matrix:
   * pixel mode (scalable bit clear): identity, with translation (int(C.tx + 0.5),
     int(C.ty + 0.5)) (truncation toward zero) — scale and rotation of C are **not** applied
     to the quads; they were given to FreeType instead (6.2);
   * scalable mode: C with row 0 scaled by S/Sx and row 1 by S/Sy, translation rounded the
     same way; the wrap width is divided by S/Sx.
4. If flag 0x10: word-wrap a copy of the text (6.6).
5. Lay out and draw (6.7).
6. Restore C.

### 6.2 Setting the size (0x354600)

**Pixel mode** (0x354740): C's 2×2 part (m00, m01; m10, m11) is kept for layout; FreeType gets
the transform xx = m00, xy = −m10, yx = −m01, yy = m11 (16.16 fixed point, ×65536,
0x42e5f8; the sign flips convert to FreeType's y-up space); char size Sx = Sy = S, set as
int(S × 64 + 0.5) in 26.6 for both axes at 0 dpi (FreeType's 72 dpi default). The raster
radius = r × min(|row 0 of C|, |row 1 of C|). Glyphs are cached per (char size, transform)
and outline glyphs per (char size, radius in 26.6, transform). Spacing and line spacing are
used as given.

**Scalable mode** (0x354cf0, 0x3541a0): with sx = |row 0 of C|, sy = |row 1 of C|:
Sx = int(sx·S·64 + 0.5)/64, Sy = int(sy·S·64 + 0.5)/64, each replaced by 1000 if larger
(0x42e5d4) and by 1 if smaller than 1; raster radius = ((Sx/S + Sy/S)/2)·r, replaced by
500 if larger (0x42e5c8); spacing × Sx/S; line spacing × Sy/S. FreeType
gets the char size (Sx, Sy) and **no** transform. Caches per (Sx, Sy) and (Sx, Sy, radius).
The stroker is set to radius int(raster radius × 64 + 0.5), round caps, round joins.

### 6.3 Vector fonts (not used)

A font loaded through 0x3530b0 (a .plx document of glyph shapes, load flags 3) draws each glyph
as a scene subtree, pushing the colour, outline colour and extrusion colour into the engine's
pass colours. The game never loads one; it is out of scope.

### 6.4 Rasterising one glyph

**Pixel mode** (0x3524f0), for character code ch and the outline switch:

1. Set the char size and transform (6.2).
2. Load the character with flags NO_HINTING | NO_BITMAP (value 10): unhinted outline.
3. Oversize guard: transform the glyph's metric box (width, height, from the slot metrics)
   with the transform; if its area exceeds 500² (0x42e5c8), scale the transform by g =
   500 / √area and reload. Otherwise g = 1.
4. Take the glyph. Outline glyph: stroke it with the stroker set to radius
   int(raster radius × 64 × g + 0.5), round cap, round join, replacing the glyph by its
   **full** stroke (both sides of every contour).
5. Render to an 8-bit antialiased bitmap (normal mode).
6. Texture: (width + 2) × (height + 2) RGBA, every texel white, alpha = coverage, with a
   1-texel transparent border on every side. Texture options: format 1; min filter =
   (texture width > 99); mag filter 0.
7. Glyph record: left = (bitmap left − 1)/g, top = (−bitmap top − 1)/g (y down),
   width = (width + 2)/g, height = (height + 2)/g, **advance = horizontal advance from the
   slot metrics / 64** (unhinted, untransformed).

**Scalable mode** (0x351b20): the same, except load flags NO_BITMAP only (value 8: hinted —
these fonts have no instructions, so this mostly rounds metrics), no transform, no oversize
guard (g = 1), both texture filters = (width > 99), and advance = the glyph object's advance
(16.16) / 65536.

The wrap measure uses the plain glyph (0x352090). Kerning (6.5) is always asked of FreeType.

### 6.5 Pen arithmetic

For character i (not a newline) with glyph record G (or none), and next character i+1:

* adv = G.advance (0 if no glyph);
* if i is not the last character: adv += kerning(ch_i, ch_{i+1}), where kerning is FreeType's
  kerning x: pixel mode in unscaled mode (font units) × Sx / unitsPerEm; scalable mode in
  default mode, in 26.6 shifted right by 6. **Both shipped fonts have no kern table, so this
  is always 0**;
* the pen x gains `spacing` (only when i is not the last character). The measure (6.7) also
  skips kerning and spacing when the next character is a newline; the draw and the wrap do
  not check that;
* pixel mode: pen.x += adv. Scalable mode: pen.x = int(pen.x + adv) (truncation).

A newline (code 10) sets pen.x to the line start (6.7) and pen.y += S_y + line spacing (the
**line advance ignores the font's ascender, descender and line gap**). A code 13 is skipped in
measuring and has no glyph advance. A code 0 ends measuring.

### 6.6 Word wrap (0x354f20)

Over the text, keeping the line width ℓ, the width at the last break candidate ℓb, and the
last break index (none):

1. A character is a break candidate if it is a space (32), a newline, a code 13, or the last
   character. At a candidate: if ℓ ≥ wrap width and there is a previous candidate, that
   previous character becomes a newline and ℓ ← ℓ − ℓb. Then ℓb ← ℓ and the candidate becomes
   the last break.
2. Then: a newline or code 13 resets ℓ and ℓb to 0; any other character adds its advance,
   kerning, spacing and truncation exactly as 6.5.

The width carried onto the new line includes the advance of the space that became the break.

### 6.7 Measuring (0x350eb0)

Inputs: text, alignment flags, start index (or −1 for the whole text), mode **pen** or
**box**. Range: from the start index to the next newline (whole text: to the end); pen mode
includes one position past the end.

* Start: min = (0, −Sy), max = (0, 0), pen = (0, 0).
* For each character, fetch its glyph (the outline glyph if the radius > 0, which is larger).
  * Pen mode: max ← componentwise max(max, pen). min never changes.
  * Box mode (characters other than 10 and 13): the glyph box shrunk by the 1-texel border,
    from (pen + (left, top) + 1) to (pen + (left + width, top + height) − 1). The first box
    replaces min and max; later boxes extend them.
  * Advance the pen (6.5; a newline goes to x = 0 and down one line).
* Alignment (flags ≠ 0): measure the **whole** text with flags 0 in the same mode, giving
  (Wmin, Wmax); then 0x01: shift x by −(Wmin.x + Wmax.x)/2; 0x02: by −Wmax.x; 0x04: shift y by
  −(Wmin.y + Wmax.y)/2; 0x08: by −Wmax.y.

The TextShape's bounds (slot 14, 0x368c60; only when it has a font **and** the scalable bit)
are the box-mode measure of the whole text with its flags, then scaled by (S/Sx, S/Sy). Slot 4
tests a point against them; slot 3 also accepts points within a radius of the transformed box
edges.

### 6.8 Layout and drawing (0x34fe40)

1. Pen-mode measure of the whole text without and with the flags; the difference of the two
   minima is the block shift (Δx, Δy). Pen start = origin + (Δx, Δy).
2. Pen-mode measure of the first line with the flags gives its width w0; the whole text's
   aligned measure gives Wmax.x. Flag 0x01: pen.x = origin.x − w0/2; flag 0x02:
   pen.x = origin.x − w0. (Each line is aligned on its own.)
3. For each character:
   * Newline: measure the next line (from the next character, pen mode, no flags) to get its
     width w; pen.x = origin.x + Δx, then corrected as in step 2 with w; pen.y += Sy + line
     spacing.
   * Otherwise:
     1. Fetch the plain glyph; if it exists draw it with the **text colour**.
     2. If the radius > 0, fetch the outline glyph and draw it with the **outline colour**,
        on top of the plain glyph.
     3. Advance the pen (6.5).
   * Drawing a glyph: position x = int(M(pen).x + left), y = int(M(pen).y + top), where M is
     the layout 2×2 matrix (pixel mode: C's 2×2; scalable mode: identity). (In scalable mode
     the rounding is int(M(pen)) + int(left), equivalent since left is whole.) Size = the
     glyph record's width and height. Drawn immediately as one textured quad covering the
     whole glyph texture (engine slot 5, 0x38f3b0) in the replaced matrix of 6.1 step 3, with
     the colour as the shader tint (the node's `Display.fillColor` is **not** applied to
     glyphs) and the mask mode of the node.
4. The returned position is the final pen.

Consequences worth keeping:

* In pixel mode glyph positions are whole pixels in screen space, and the node's scale and
  rotation are applied by FreeType to the outlines (crisp glyphs at any transform).
* In scalable mode glyphs are rasterised at the node's scale and drawn through the scaled
  matrix; pen positions are truncated to whole raster pixels.
* A single draw with a radius puts the full-width outline **over** each glyph, eating r
  pixels into it. The game avoids that by drawing outlined text twice (7.1, 7.2).

### 6.9 TextShape draw and rebuild

* Rebuild (slot 1, 0x368b90): mark changed; load the font if missing or forced (5.2).
* Draw (slot 2, 0x368020): if it has a font: engine node begin, text draw (6.1) with origin
  (0, 0), the live string, S, r, spacing, line spacing, wrap width, live colour, outline
  colour, extrusion colour, flags and the scalable bit; engine node end.
* Clone (slot 13, 0x367ea0) copies every value and rebuilds.

---

## 7. Interface texts made in code

### 7.1 The two-pass text object (draw 0x269210)

Game widgets keep a text style object with setters: font by name 0x269130 (through 5.2), size
0x2691b0, outline radius 0x2691f0, line spacing 0x2691a0, wrap width 0x269200, colour
0x269160, outline colour 0x2691c0, flags 0x269190 (spacing is a field with no setter seen).
Its draw:

1. Position = (the widget node's live **pivot** + (x, y)) × the current matrix xx scale.
   The pivot is transformation property index+0x244 / values+0x278;
   translation is the distinct index+0xac / values+0xe0 property. The distinction
   follows transformation recomputation0x3733f0 and the0x269210 loads. See
   steam-identification-ui §3.2 for the separate mouse and navigation bases.
2. If the radius ≤ 0: one text draw with colour and outline colour.
3. Otherwise **two** text draws, both in scalable mode: first with the radius, a fully
   transparent text colour and the outline colour (only the outline shows); then with radius
   0 and the text colour. The outline therefore sits behind the glyphs.
4. The returned pen is divided back by the scale and that pivot removed.

Values found:

| Where | Font | Size | Radius | Other |
|---|---|---|---|---|
| tooltip and item texts, 0xb7b90 | resource1.dat | UI scale × 12 (body), × 15 (title), × 18, × 9 | 3 (body; 0x42e580), 4 (title; 0x4aca20) | line spacing 1, flags 0x10, wrap width from the caller |
| chat log, 0x26a930 (direct draws, same two-pass pattern) | widget font | 12 (0x42e59c) | 3 | first line at y = 19, then every 18 (0x437fe0 also 18); x starts at 3; colour from the message's bytes / 255 (0x42e7d8) |
| wavy text, 0x268b80 | caller's | caller's | caller's | per-character y offset cos(index + time × 0.01) × size × 0.1 (0x42e51c, 0x42e528) |

### 7.2 Outlined TextShape pairs (0x1008a0)

Built during interface set-up (0xf9ac0). Each call creates two TextShapes on two new nodes,
font resource1.dat, line spacing 3.0, the given flags:

1. the outline shape: colour (1, 1, 1, 1), outline colour (0, 0, 0, 1), size S, radius r;
2. the plain shape: colour (1, 1, 1, 1), size S, no radius, added after (drawn on top).

Both are in pixel mode (the scalable bit is not set). Calls (call site: flags, S, r; the
nearest name strings are given as a hint):

| Call site | Flags | S | r | Hint |
|---|---|---|---|---|
| 0xfcea0 | 0 | 24 | 5 | gold |
| 0xfcf1d | 0 | 24 | 5 | context item |
| 0xfdd50 | 0 | 16 | 3 | recent lands ("land") |
| 0xfe015 | 0 | 24 | 5 | player name ("LV 1") |
| 0xfe3e1 | 2 | 12 | 3 | stamina bar |
| 0xfe471 | 2 | 28 | 6 | location |
| 0xfe4ee | 2 | 21 | 5 | quick buttons / cooldown |

---

## 8. Constants

| RVA | Value | Use |
|---|---|---|
| 0x4ac964 | 0.5 | half width, midpoints, text legacy radius |
| 0x4ac980 | 1.0 | |
| 0x4ac9d8 (double) | 2.0 | base of 2^L |
| 0x42e8e8 (double) | 4.0 | 2^L shortcut for L = 2 |
| 0x4ac9d0 | 2.0 | smooth weight default, corner weight |
| 0x47ad6c | π/4 | corner weight c = 1/(2cos(π/4)+1) |
| 0x4ac95c, 0x479770 | 0.25 (×4) | interior vertex rule |
| 0x42e710, 0x47ad80 | 0.75 (×4) | interior vertex rule |
| 0x4329b0 | (0.5 ×4) | colour midpoints |
| 0x47ad78 | 2560 | adaptive level budget (unused) |
| 0x4aca20 | 4.0 | adaptive level log base; tooltip title radius |
| 0x42e528 | 0.1 | minimum stroke width; wavy text amplitude |
| 0x42e704 | 0.3 | minimum miter cosine |
| 0x47ad70 | 0.95 | straight-join threshold |
| 0x42e768 (double) | 57.29578 | radians → degrees |
| 0x42e7d4 | 180 | |
| 0x42e5a0 | 20 | round-join steps per 180° |
| 0x42e740 (double) | π/2 | round-cap sweep |
| 0x47ad74 | π/2 | end-cap offset |
| 0x42e80c | −0.5 | cap v |
| 0x4aca4c | −1.0 | square end cap direction, texture pivot |
| 0x42e520 | π/180 | degrees → radians |
| 0x42e630 | identity 4×4 | |
| 0x42e660 | (0, 0, 0, 1) | default colours |
| 0x42e8c0 | (1, 1, 1, 1) | |
| 0x47c288 (double) | 100131 | GLU winding rule NONZERO |
| 0x42e60c | −2.0 | depth-stack step |
| (shader) | −0.135 | vertex pixel offset |
| (shader) | 0.3, 0.59, 0.11 | saturation luminance |
| 0x42e5f8 | 65536 | 16.16 |
| 0x42e510 | 1/65536 | |
| 0x42e5b4 | 64 | 26.6 |
| 0x446140 | 1/64 | |
| 0x42e5d4 | 1000 | max raster size |
| 0x42e5c8 | 500 | max raster radius; glyph oversize guard |
| (immediate, 0x3589a0) | 20 | round-cap segments |
| (immediate, 0x3589a0) | 3 | default subdivisions |
| (immediate, 0x360620) | 0 … 6 | subdivision clamp |
| (immediate, 0x3524f0 / 0x351b20) | 10 / 8 | FreeType load flags |
| (immediate) | 99 | glyph texture filter threshold |
| 0x42e59c | 12 | chat size, tooltip body factor |
| 0x42e580 | 3 | chat / tooltip radius |
| 0x42e7a4 | 15 | tooltip title factor |
| 0x437fe0, 0x437fe4 | 18, 24 | chat line pitch; tooltip spacing |
| 0x42e790 | 9 | small tooltip factor |
| 0x42e51c | 0.01 | wavy text speed |
| 0x42e7d8 | 255 | byte colour scale |
| (immediate, 0x1008a0) | 3.0 | line spacing of outlined TextShape pairs |

---

## Verification

* Virtual tables were found from the RTTI type descriptors and complete-object locators in the
  binary and listed slot by slot; constructors located by the vtable stores.
* Property names, value types and defaults were taken from the readers and constructors and
  checked against all four shipped documents (gui.plx, icons.plx, quest-tag.plx, cursor.plx),
  decoded with the documented chunk stream and seal: every SmoothMeshShape and TextShape
  property listed occurs with the stated size (for example `textureRotation` is 12 bytes, a
  3-vector; `smoothWeight` is 0.5858 in all 499 gui shapes; strokes use `strokeWidths` 3.0
  with alignment 0/1/−1). Statistics in 1.2, 1.3 and 4.1 come from that dump.
* Every constant in section 8 was read from the binary at its RVA.
* The shader formulas were read from the two DXBC blobs (0x47ebd0, 0x47f940) by disassembling
  their SHDR chunks; constant-buffer names (`baseColor`, `textureMatrix`, `maskMatrix`,
  `textureOpacity` …) confirm the mapping to the engine setters 0x3901c0, 0x390ae0, 0x390420,
  0x3908f0.
* resource1.dat and resource2.dat were parsed: TrueType, tables and metrics as in 5.1, no kern
  table, no hinting programs.
* A shipped screenshot (Steam folder, region name "DESKOR HILLS") shows white glyphs with a
  thick black outline behind them and the glyph fill intact, consistent with the two-pass
  scheme of 7.1/7.2 and the location pair (S 28, r 6).
* Load flags (0x20 for gui.plx and icons.plx) were read at the load call sites in 0xf9ac0.

## Open questions

1. **Exact FreeType behaviour.** The FreeType version is unknown; the spec relies on its
   results: whether unhinted advances (pixel mode) are fractional, how hinting without
   instructions rounds advances (scalable mode), ppem rounding for head flag bit 3, and the
   stroker's output. Pixel-exact parity needs FreeType or a careful emulation.
2. **Glyph texture filter values.** The texture options are {format 1, min = width > 99,
   mag = 0 (pixel) or width > 99 (scalable)}; the meaning of filter 0/1 belongs to the
   texture spec.
3. **Blend state** of drawings and glyph quads (straight vs premultiplied alpha) was not
   traced in the D3D11 engine.
4. **Plasma TextShapes with a radius** (most of gui.plx) are drawn by Plasma in one pass, which
   puts the outline over the glyph (6.8). It is not checked here whether the game shows those
   shapes as they are or restyles them through 7.1; a runtime check of a gui.plx caption is
   needed.
5. **Level ≥ 1 vertex weights** (2.8.3 pass 4 visits interior edges once, level 0 twice) are
   faithful to the code but unlike textbook Catmull–Clark; a render comparison of a shape with
   `subdivisions` ≥ 2 (22 + 9 shapes in icons.plx) would confirm.
6. **Dashed strokes, square caps and extrusion** are summarised only; the shipped files never
   use them.
7. **Handle rules** (2.3): the meaning of `vertexParameters` when the high flag bits are
   unused is unknown; shipped values (≈ 15 with low mantissa bits set) look like editor
   leftovers.
8. **Text measure vs draw alignment:** draws align with pen-mode bounds, bounds/hit tests with
   box-mode bounds; for centred text these differ by the glyph side bearings. Kept as found.
9. **Display.fillColor and text:** glyph quads take only the text colour as tint; whether a
   fading node also fades its TextShape therefore depends on the game animating
   `TextShape.color`.
