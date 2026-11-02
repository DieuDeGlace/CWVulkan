# Steam Plasma Widget and node surface caches

Independent native-only bounded contract from the completed trace of
0x32d890,0x330b20, constructors/copies/destructors, frame update and concrete
surface composition. No engine, tests, exporter, owner captures or game
execution was used. This covers valid ordinary finite cache inputs; **blur
is separate**. Surface format, null-depth target handling and Plasma pixel
composition remain [ordinary clip layers §§2/4–7](steam-plasma-clip-layers.md).
Existing Widget geometry remains
[Plasma §2.7.2](plasma-format-and-scene.md#272-derived-geometry).

## 1. Selection and retained ownership

After node visibility/disabled admission and matrix push, **0x330b20**
first considers an own Widget cache. It requires both **GUI engine+0x288
bit0x02** and **own Widget+0x150 bit0x40**. This is not a cache selected
merely because a Widget exists. It calls **0x32d890 at0x330bef** and uses
the Widget's surface pointer **+0x58** when available.

Native GUI startup initializes the engine option word to7. Its bounded
direct writers and temporary bit0x01 restoration are
[GUI options §§1–2](steam-plasma-options-and-shape-bounds.md).

If no Widget surface is selected, **node+0x80 bit0x400** can select the
node's separate surface **+0x88**. That node branch does not require the
Widget-cache engine-option bit. If its surface is absent, it creates one
through the engine's surface factory and sets **node changed byte+0x84=1**.
The Widget and node surfaces are separate retained objects, not entries in
the depth-indexed fullscreen clip pool.

The base Widget constructor **0x328ff0** initializes:

|Widget field|Initial value|
|---|---|
|+0x40 float scale|1|
|+0x44/+0x48 float offsets|0,0|
|+0x4c signed32 divisor|1|
|+0x58 surface pointer|null|
|+0x15c dirty byte|0|

Base copy **0x329210** copies those four scalar scale/offset/divisor fields,
but initializes the destination surface pointer to null and dirty byte to0.
It does not share or copy the source's cached image. Node constructor
**0x32ee60** initializes changed+0x84=0 and surface+0x88=null. The existing
shallow/deep node-copy paths construct new nodes; sharing their shape,
Transformation or Display does not copy a retained node surface.

Widget destructor **0x329500** destroys its retained surface through
0x33dfa0. Node destructor **0x32f0c0** destroys its nonnull+0x88 surface
only when **the byte at(node's engine+0x08) equals0**. The ownership meaning
and other cleanup routes for that engine byte remain unproved; it must
not be silently replaced by unconditional node-surface destruction.

0x32d890 contains a branch destroying Widget+0x58 when flag0x40 is clear.
However, its sole resolved direct caller above already gates that flag on.
This trace therefore does not establish automatic disposal immediately on
clearing the flag. The draw path likewise does not destroy an unused node
surface merely because node flag0x400 is cleared.

## 2. Widget surface preparation and exact geometry queries

When 0x32d890 creates Widget+0x58, it writes **surface byte+0x08=0** and
sets **Widget+0x15c=1**. The byte write is recorded without inventing a
broader semantic name for it. On every reached call with a surface, the
helper requests size from **0x32d010**, independently for width and height:
add float32 **0.5**, then **truncate toward zero to signed32**. Surface
resize0x392730 sets/recreates the physical dimensions as already specified;
when it reports a change, 0x32d890 marks Widget+0x15c again.

These are not AABB dimensions. **0x32d010** calculates the constrained
effective frame size, then applies the linear XY terms of the frame matrix
**F from0x32ce40**. The existing geometry contract gives F from the node's
world transform, Widget bind inverse and framePos. **0x32cf50** obtains
that frame matrix's origin with its homogeneous reciprocal. Neither query
is the nearest-parent-relative Q geometry helper, and the size query does
not take absolute values to construct an enclosing rotated rectangle.

0x32d890 also compares the queried frame origin with its pair at
**Widget+0x60/+0x64**. On a difference it performs another size query and
stores the current origin. That comparison/store alone does not write the
dirty byte. It is distinct from a physical surface resize or a changed
Transformation already marking its node.

For redraw setup, 0x330b20 requeries size and origin. It independently
rounds each through **float32 add0.5 → signed32 truncation → float32
conversion**, then supplies the rounded origin/size as the surface target
rectangle. There is no extra two-pixel allocation padding for this Widget
route. Negative values do not use floor(x+0.5); nonfinite/unrepresentable
conversion and failed allocations remain outside the ordinary scope.

The size/origin queries are **live and repeated**, including final copy
after the callbacks. No single snapshot is retained through the entire
redraw. A callback's later geometry mutation can therefore affect final
copy arguments without retroactively reallocating the surface selected
earlier in that draw.

## 3. Widget redraw projection and concrete copy wrapper

The Widget target is bound through the same surface target/projection
wrapper as the clip contract. The redraw path then reads the current GUI
projection and calls **0x1028b0 at0x330db5**, scaling its X and Y basis
by **Widget float+0x40**. It adds the offsets **+0x44/+0x48** through
that scaled basis to the projection translation and installs the result.
The operation is not a replacement WORLD matrix or an inferred inverse
frame transform.

If the scale is ordered less than1, redraw temporarily clears **engine
+0x288 bit0x01**. The entry value of that bit is saved, and the final
Widget copy restores that saved value. No new interpretation of this bit
or global per-frame toggle is assigned here.

Final Widget composition again queries and rounds current origin/size.
The generic traversal prepares source offsets by multiplying each stored
offset by scale, then dividing by the float conversion of signed divisor;
it prepares copy scale as scale divided by that same converted divisor,
and also passes the raw signed32 divisor. These are **prepared arguments**,
not proof that the concrete backend consumes them.

The call at **0x33175a** uses surface virtual **+0x38**. Concrete vtable
**0x480718** maps it to **0x3925c0**, which discards the supplied source/
scale/divisor arguments and invokes ordinary **0x392280** with **source
offset(0,0) and copy scale1**. Destination origin/size still pass through.
Consequently the generic final-copy scale/offset/divisor arithmetic must
not be applied a second time by interpreting virtual names. Widget scale
and offsets already affected redraw projection; the divisor's prepared
final arguments are not consumed by this concrete wrapper.

The reached copy uses the ordinary surface texture/color/pipeline behavior
in clip layers§7. That includes the surface's own RGBA and actual allocated
texture dimensions; a changed final requested size need not equal those
earlier allocated dimensions.

## 4. Node surface bounds and negative-scale copy

For the node-flag0x400 route, **0x332ba0** supplies screen-space minimum
and maximum bounds. On an admitted redraw, each surface allocation extent
is obtained by **float32(max−min), float32 add0.5, signed32 truncation,
then integer add2**. The redraw rectangle uses independently rounded
minimum and extent, with the same add0.5/truncation, but **excludes the
extra2** in its requested rectangle size.

Final composition requeries the bounds rather than reusing an immutable
redraw snapshot. Its destination origin and extent are again rounded with
add0.5/truncate. Source origin is0. The call at **0x331861** reaches
surface virtual **+0x28 →0x392280** directly with literal copy scale
**−1.0**, word0xbf800000 at **0x4aca4c**.

For a negative copy scale,0x392280 replaces it with **float(textureWidth)
divided by the raw requested destination width**. It sets fractional
translation corrections and centering offsets to0. The resulting **same
width-derived float scale is used for both axes' UV extents**, each then
divided by its corresponding actual texture dimension. It does not derive
another ratio from textureHeight/requestedHeight. Destination size remains
the requested rectangle; the scale controls source sampling.

Thus the two-pixel physical allocation padding does not authorize symmetric
per-axis UV reconstruction. Positive finite extents are the bounded normal
case here. Zero/negative extents, division by zero, invalid casts and
failure recovery are not assigned replacement values.

## 5. Redraw predicate, callback suppression and reset timing

For a selected nonnull surface, **0x330b20** redraws when **node+0x84
is nonzero OR own Widget+0x15c is nonzero**. With both clear, it skips
the node's own shape, child traversal and both own-Widget draw callbacks,
then performs the retained surface copy. Visibility/disabled admission
still occurs before that decision. Hiding a node does not itself destroy
the retained image; a general forced redraw merely on showing is unproved.

On a redraw, the cache target surrounds the reached normal shape/child/
Widget-hook work, including any separately admitted clip branch. An own
hook outside its own clip mask can still draw into the surrounding cache
target. Callback position and clip composition remain the clip contract;
cache reuse does not rerun a WORLD hook merely because it is time-dependent.

The reached ordinary GUI step **0x349cf0 →0x3358e0** orders:

1. **0x334dc0** resets attached keyable evaluation bytes+0x4a along its
   admitted traversal.
2. **0x335920** first resets non-disabled node changed+0x84 to0. For
   visible/forced nodes, changed shape/Transformation/Display bytes+0x49,
   own Widget+0x15c, a match with the engine's active Widget from0x342090,
   or a changed admitted child can set node+0x84 again.
3. **0x334e30** visits only non-disabled nodes whose node+0x84 is nonzero.
   It clears attached shape/Transformation/Display+0x49 and own
   Widget+0x15c, then recursively applies that admission. It **does not
   clear node+0x84**, which remains available to drawing.

Draw0x330b20 does not itself clear those two dirty bytes after a redraw.
The base Widget post-cache virtual+0x30 is a no-op. A custom override
retains its own behavior. Surface creation/resize or callback mutation
during draw can therefore mark Widget+0x15c **after the ordinary step's
clear pass**, causing another redraw on the following update/draw. Node
surface allocation marks node+0x84 instead; its next advance can reset
that byte under the ordinary rules.

There is no blanket elapsed-time/per-frame cache invalidation in this
consumer. Reached dirty producers include effective-delta resize
**0x32c6f0**, its descendant path **0x32c560**, focus change **0x340ec0**,
the keyable/active-Widget paths above, and surface allocation/resize.
Existing notification/placement distinctions remain
[Plasma scene](plasma-format-and-scene.md), especially§2.6.6 and§2.7.3;
calling alignment-only placement is not itself a proved dirty write.

## 6. Explicit limits

Broader producers/setters of the scale/offset/divisor fields, all cache
flag changes and reset writers, the node engine ownership-byte meaning,
failed/invalid resource behavior and concurrent mutation remain unclosed.
This is not a census of every custom Widget override or invalidation caller.
It preserves directly reached copy/destruction gates without inventing
automatic recovery or replacing live queries with a geometry snapshot.

Blur's bounded traversal, fullscreen allocation/reduced projection, repeated
copies and final rebind clear/source selection are specified separately in
[blur surfaces §§1–6](steam-plasma-blur-surfaces.md). Cache policy does not
establish a Gaussian filter. Sampler provenance and the separately cited conditional
NULL-sampler API defaults remain clip layers§6.1, rather than a new
cache-specific sampler assumption.
