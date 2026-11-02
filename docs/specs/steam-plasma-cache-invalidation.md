# Steam ordinary Plasma cache invalidation and GUI change lifetime

Independent native-only bounded contract for0x349cf0 →0x3358e0 and its
three traversals, plus the directly reached setters below. Existing specs
are unchanged. No engine/tests/exporter, owner data/captures, reconstruction
or game execution was used. Geometry, retained surfaces and drawing remain
[surface caches §§1–5](steam-plasma-surface-caches.md) and
[Plasma scene §§2.2/2.6/2.7](plasma-format-and-scene.md).

## 1. Separate fields and ordinary entry

|Receiver/field|Meaning in these consumers|
|---|---|
|Shape, Transformation or Display+0x48|Playing byte|
|Same shared keyable+0x49|Changed indication read by node advance|
|Same shared keyable+0x4a|Already advanced/evaluated in this traversal|
|Node+0x84|Changed indication retained for drawing|
|Own Widget+0x15c|Widget dirty indication|
|Node+0x70 /+0x78|Own Widget / refreshed inherited-or-own owner|

Ordinary engine entry **0x349cf0** receives signed32 step milliseconds,
adds them to engine signed32 clock+0x194 and stores the step at+0x190.
It clears engine32-bit field+0x20 and invokes0x3358e0 on root+0x140 with the same
integer step. This entry has no positive-step test or elapsed-time cache
redraw rule: a reached zero-step call still performs the traversals.

**0x3358e0** orders reset0x334dc0, advance0x335920 with incoming force0,
then clear0x334e30. It retains advance's returned low-byte result across
the clear and returns it. Clearing is not folded into each individual
node's advance; all admitted users of shared keyables get the advance
pass before the final clear traversal begins.

## 2. Reset and ordinary advance admission/order

**Reset0x334dc0** excludes node flag0x04 and its subtree. Otherwise it
clears+0x4a of attached shape+0x58, Transformation+0x60 and Display+0x68,
then visits children forward in stored child-list order. It does not
test visibility, own/incoming force, Widget enabled state or node+0x84.
It does not clear keyable+0x49, Widget+0x15c or node+0x84.

**Advance0x335920** first tests node flag0x04. A flagged node returns0
without clearing its existing+0x84 or visiting descendants. For an
unflagged node it immediately sets **node+0x84=0**, then samples current
live Display.visibility. Zero visibility and zero incoming force reject
the node, return0 and skip its descendants. Its own flag0x2000 cannot
rescue that hidden node: only after this admission is the force passed
to children chosen as incoming force OR own flag0x2000. That choice
precedes owner refresh, animation and the own-Widget hook; a later hook's
flag mutation does not recompute the saved child-force value in this call.

For an admitted node the order is:

1. Refresh owner+0x78 from own Widget+0x70, else parent's owner+0x78,
   else null for a parentless node.
2. Advance shape, Transformation and Display in that order, each only
   when nonnull and its+0x4a is0, through0x369190 with the integer step.
3. Invoke the **own** Widget's virtual+0xd8 when own Widget is nonnull.
4. Set node+0x84 to1 if any attached keyable+0x49 is nonzero, or own
   Widget+0x15c is nonzero, or the active-Widget comparison below matches.
5. Advance children forward in child-list order with the propagated
   force. Each child's returned nonzero low byte marks the parent+0x84=1.
6. The existing flag0x80/changed/nonnull-shape branch can fit/rebuild the
   shape from admitted child bounds, after child advance. Its geometry
   remains the scene§2.6.6 contract. Return the current node+0x84.

There is no Widget+0x1a0 enabled test in this traversal. Node flag0x04
is a separate subtree exclusion. Visibility is sampled **before** the
node's animation and own-Widget hook; the node is not readmitted or
rejected by a second visibility sample after those calls.

**Active predicate:**0x342090 returns engine pointer+0x170. At0x335a72/
0x335a80 the advance tests that getter for nonnull, then compares a
second getter result to this node's **own Widget+0x70**. A match marks
node+0x84 even with all dirty bytes0. This is not a comparison with
inherited owner+0x78, pointer hover, captured node or every active-looking
control, and it has no additional enabled/pressed predicate here.

Absolute seek0x334ef0 differs: it incorporates own0x2000 **before** its
visibility test. A hidden unforced node with own0x2000 is admitted by
that seek but rejected by ordinary advance. Existing seek/stop behavior
is unchanged; this difference must not be replaced with one shared gate.

## 3. Shared evaluation and final clearing

**0x369190** sets keyable+0x4a=1 on every reached call, even when not
playing. With playing+0x48 nonzero it advances the registered attributes
in their list order. If any reports change it invokes keyable virtual
+0x08 with argument0. Otherwise it clears playing+0x48. It does not
generically set+0x49 merely because an attribute changed, and does not
clear a preexisting+0x49 on the no-change path.

Crucially, **0x335920 ignores0x369190's returned change boolean**. It
later reads keyable+0x49. The virtual update implementation therefore
matters: Transformation update0x3733f0 always sets+0x49=1 after rebuilding;
TextShape update0x368b90 sets+0x49=1 on entry; **Display vtable0x47c170
slot+0x08 is no-op0xe8a20**. The reached Display update does not itself
translate an attribute-change result into the dirty byte. No replacement
generic property-comparison invalidation is established by this path.

**Clear0x334e30** excludes flag0x04 and, independently, any node whose
+0x84 is0. For an admitted node it clears attached shape/Transformation/
Display+0x49 and own Widget+0x15c, then recursively applies the same
admission to each child. It does not check live visibility or force.
It does **not** clear node+0x84 or keyable+0x4a.

Consequences of those literal admissions:

- A hidden unforced node has its own+0x84 cleared by advance, but its
  skipped descendants keep their existing node+0x84. Their keyable+0x4a
  was nevertheless reset if reset could reach them through non-0x04 nodes.
- A flag0x04 node retains its old node+0x84, but returns0 to its parent;
  its stale byte alone does not propagate change in this advance.
- Clear does not visit below a zero-changed parent. Pending keyable/
  Widget indications in such a skipped branch can survive. Shared objects
  still have one shared byte: another admitted user can cause that byte
  to be cleared in the final pass. There is no per-node private copy.
- Descendant change propagates upward through admitted advance returns.
  Parent change does not set every descendant's+0x84 or Transformation+0x49.

## 4. Directly verified text/color and Display writes

Named helpers below exclude node flag0x04 but have no visibility gate.
An exact matching node stops further name-search descent; a nonmatching
node searches children forward. The nonzero mode applies its subtree
helper to the matched node and descendants instead.

|Reached operation|Writes/rebuild and equal-value behavior|
|---|---|
|Named text0x3348e0, mode0, own shape type3|Compare live UTF-16 string contents. Only a difference assigns the live string and invokes shape update+0x08 with argument0. For TextShape that reaches0x368b90 and sets shape+0x49=1. Equal contents do not rebuild or mark these dirty fields.|
|Named text0x3348e0, nonzero mode →0x334c50|For each admitted type3 shape, assign only differing string contents. This helper has **no rebuild** and no direct keyable+0x49, node+0x84 or Widget+0x15c write. An equal call also leaves those fields untouched.|
|Named TextShape color0x334a20, mode0|Compare four live RGBA floats. A difference copies all four; equality skips copying. Neither case rebuilds or directly writes any of those dirty fields.|
|Nonzero color mode →subtree0x334b80|Same four-component comparison/copy for each admitted type3 shape, followed by child traversal. No rebuild/dirty write. Floating unordered comparison also takes the copy path; signed zeros compare equal.|
|Own visibility0xa3a20|Unconditionally stores integer0/1 from the supplied boolean into the current live Display.visibility frame, including an equal call. No rebuild or direct keyable/node/Widget dirty write.|

TextShape update0x368b90 sets its changed byte even when its existing
font pointer is retained or an equal-value caller explicitly invokes that
update. The named-text helper's earlier equality guard is distinct from
that unconditional update-byte behavior. Recursive string/color changes
alone therefore do not guarantee an already retained cache is redrawn;
other actual dirty producers may still admit it.

The general vec4 live-value writer0xf88f0 copies all four components
unconditionally, including equal values, and has no change/rebuild
notification of its own. It receives an attribute object, not a node or
Widget. This does not assign every caller of that writer to Display or
TextShape, or establish a census of raw attribute writers.

## 5. Direct transform, geometry and focus producers

Transformation recompute **0x3733f0** always writes+0x49=1, even for an
equal resulting matrix. Geometry math stays in scene§2.2.1/2.7.3.

- Widget position **0x32b250** first compares the requested/query XY.
  Equality returns without recompute or byte replacement. A difference
  saves the old Transformation+0x49, changes translation and recomputes.
  Nonzero position flag retains recomputed1; flag0 restores the exact
  saved byte, including a saved1. The move itself is not undone.
- Node point placement **0x334510** compares its computed target local
  translation to the live pair. Only a differing component assigns the
  pair and calls Transformation update+0x08. This helper does not have
  Widget-position flag0's restoration step.
- Widget size **0x32c6f0** sets Widget+0x15c only on the previously
  specified nonzero effective-delta, admitted starting-node/own-Widget
  route. Descendant walker0x32c560 can mark the **initiating** Widget;
  it does not mark all visited descendants. An equal/effectively zero
  delta does not make that direct dirty write, but the receiver's own
  resize hook+0x50 still runs. Trailing propagation flag controls
  delivery to a different own Widget, not unconditional dirtying.
- Rectangle **0x32c680** calls position with flag0, then size with its
  supplied flag. XY wrapper0x32c9a0 only packages size for0x32c6f0.
  Their calls do not construct a new Widget or imply a rectangle-wide
  dirty reset. Alignment placement0x32bd70 has no independent+0x15c write.
- Focus/active assignment **0x349a90** returns immediately when its
  nonnull old engine+0x170 equals the requested Widget. On a change it
  marks the old Widget+0x15c=1 before registered event19, assigns+0x170,
  then attempts the new Widget's event18 under its dispatch conditions.
  It has no direct new-Widget dirty write; the advance's own-Widget
  comparison supplies the separate active redraw indication. The reached
  pointer-down focus-loss path0x340ec0 likewise marks the old Widget
  before event19, then clears+0x170.

These are the explicit directly reached setters, not a complete setter
inventory. In particular enabled0x32a8d0 differs from Display visibility:
it writes Widget enabled state and plays/seeks/stops the relevant named
sequence even for equal calls, under the existing scene§2.7.9 contract.
Its resulting keyable notifications follow the reached sequence/update
classes and admission, not an invented generic enabled-change dirty write.

## 6. Drawing retention, finite cases and limits

The existing selected-surface redraw predicate remains **node+0x84 OR
own Widget+0x15c** after ordinary draw admission. A cache hit bypasses
shape, descendants and own hooks. Ordinary draw does not clear those
fields after redraw. Surface preparation can set Widget+0x15c during
draw, after the step's clear; cache target creation can set node+0x84.
Those writes retain the distinct next-step treatment above.

For a visible otherwise unchanged cached node: a differing named mode0
text update sets shape+0x49; the next advance sets node/ancestor+0x84;
clear consumes the shared+0x49 while preserving node+0x84 for redraw.
On the following update, absent another producer or active match, the
node resets to0 and the retained image can be reused. An equal named
mode0 string call alone does not trigger this sequence. A mode1 string
change or direct color/visibility write alone does not supply the missing
dirty indication; no image/content snapshot comparison is specified.

This contract does not close every raw attribute, Display.flags, node
flag, cache-option/scalar, shape topology, attachment/reparenting or
derived-Widget hook writer. It does not promise hidden changes survive
through other users of shared objects, automatic invalidation on showing,
all-Widget per-frame redraw, or arbitrary external/concurrent mutation
recovery. Owner sequence content and custom hook behavior remain inputs.
Only this new supplement was written; existing specifications stay unchanged.
