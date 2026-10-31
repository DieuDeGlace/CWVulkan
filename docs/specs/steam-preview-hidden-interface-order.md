# Steam preview sources under F4 and draw-stage map updates

Independent native-only supplement to
[preview source lifetimes](steam-preview-source-lifetimes.md),
[preview selection/draw](steam-voxel-widget-information-preview.md),
[map markers](map-markers.md) and
[Ring frame order](steam-ring-selector-rendering.md#73-ordered-frame-stages).
Sources are Steam native instructions/decompile at0x1355a0,0x133c70,
0x96e70,0xa5a50,0x103200,0xf9ac0,0x2829c0,0xb6b00,0xc4af0,
0x33e170 and0x330b20. No implementation, tests, exporter/build,
SDK, owner assets/captures or game execution were used.

## 1. F4 changes global live visibility, not individual panel state

Physical **F4 / VK0x73** is handled by Game key-down **0x96e70**.
It obtains **GUI engine G+0x20**, then that engine's **scene root at
engine+0x140**: loads at **0x96fc3/0x96fc7**, shared toggle0x96efd.
It changes the root's current keyed Display.visibility integer to1
when its old value is0, otherwise0. The live value is selected through
node+0x68, that Display's current index+0xac and value array+0xe0.

This is neither a separate Game interface-hidden byte nor a write to
the game-UI child G+0x220. It does not change any child node's own
visibility, map mode+0x668, map drag+0x1db0, map embedded item,
preview source/comparison pointers or stored Widget dimensions.
Normal later writers of those child states remain independent.

This key switch precedes0x96e70's chat/focus/binding-wait tests.
Window handler **0x133c70**, WM_KEYDOWN0x100, first dispatches the
GUI engine key handler0x340df0, then calls0x96e70 with low8 VK bits.
The F4 case toggles on each reached key-down call; it is not a later
Game-frame action-edge stage. This route does not filter repeated
WM_KEYDOWN using the message's repeat flag.

Visibility helper **0x9af00** only tests the queried node's own live
Display.visibility. It does **not** recursively test ancestors or the
node's separate draw-disable flag0x4. Therefore a child can still be
reported shown by this helper while F4 has hidden its global ancestor.
The common visibility setter0xa3a20 likewise writes that one live
value; it adds no subtree reset or sibling reordering.

## 2. Root hierarchy and draw admission

Constructor/interface setup **0xf9ac0** establishes these relationships:

| Root | Parent and own visibility policy |
|---|---|
|Game UI G+0x220|Child of GUI scene root; Interface update decides its own shown value from the five front roots below.|
|VoxelWidget G+0x368, node+0x178|Its blackwidget clone is beneath G+0x220; construction0x1005e4 initially hides it. Its ordinary opening/closing changes its own value.|
|MapOverlay G+0x380, node+0x178|A separately created direct child of GUI scene root, construction0xfdcd5. Its late Interface assignment copies G+0x220's own shown value.|
|PreviewWidget G+0x3c0, node+0x178|Owner `preview` node is reparented directly under GUI scene root before constructor0xffebe. Preview selector controls its own shown value.|

Factory0x33cd30 attaches its fifth parent argument through0x32f640;
attachment appends to the parent's child list. Preview's explicit
reparent uses0x3347f0. In the constructor-established ordinary child
order, the **whole game-UI subtree, including an admitted VoxelWidget,
paints before MapOverlay; MapOverlay paints before PreviewWidget**.
These need not be adjacent siblings. Root/selector/interface routines
traced for this supplement do not move those nodes to a new sibling
position each frame.

The late Interface update **0x103200** assigns game-UI visibility at
**0x10671b**. It shows G+0x220 exactly when the own shown queries of
all five roots **G+0x118,+0x130,+0x170,+0x198,+0x200** are false:
title, character select/create and world select/create. It then queries
G+0x220 at0x106735 and copies that result to MapOverlay's root at
**0x106740**. These are own-node queries, so F4's ancestor is not
part of either assignment. This list is not a substitute for the
different ordinary-UI predicate or every menu's visibility.

Late interface drawing dispatches renderer virtual slot0x10 to
**0xc4af0**. It loads the GUI engine from renderer+0x540 and tail-calls
**0x33e170 with traversal flags0**. That engine entry visits its root
through **0x330b20 at0x33e1b5**. At the very beginning of traversal,
live visibility0 or node flag0x4 returns before drawing the node,
visiting children or executing Widget draw hooks. A hidden ancestor
therefore prevents every descendant hook, including MapOverlay0x2829c0,
VoxelWidget0x292990 and PreviewWidget0x28c290.

F4's root rejection occurs before cached-surface and ordinary traversal
choices. Clip/blur/cache details otherwise retain the shared Plasma
contract. Own shown state is not evidence that a hidden-ancestor hook
ran. A skipped hook performs none of its draw-produced hover writes,
item resets, local draw-clock updates or late size writes. Other
Game/Interface updates still have their own normal execution rules.

## 3. Selection and collection bindings continue while F4 hides drawing

The ordinary Game-frame call **0xb6b00 at0xa9d22** is not enclosed
by a GUI scene-root visibility test. The selector also has no F4-root
admission test inside it. It still resolves ground/Ring/UI sources,
copies map coordinate scalars, computes comparisons, changes Preview's
own shown value and positions it on the reached normal frame path.
Its shown tests retain0x9af00's own-node meaning. MapOverlay's item
priority itself still has no Overlay-visible test, as specified in
the source-lifetime document.

The later **0x103200 at0xab5ce** also continues independently of F4.
Its normal collection assignments at **0x104925/0x104941/0x10495d**
still bind Inventory to current P+0x9f0, Crafting to G+0x17a8, and
Shop to G+0x1870. See steam-inventory-first-binding §2 for their exact
no-refresh semantics. Neither global-root hiding nor the individual
panels being hidden turns this into an unbind or refresh operation.

Thus hidden drawing does not clear draw-produced MapOverlay/Voxel
hover state through a synthetic frame reset. Preview can select an
older retained draw-produced source while its own rendering is also
suppressed. Its earlier placement still uses the stored effective
size; skipped Preview drawing does not perform §7's later resizes.

## 4. Closed full map and dragging still reach the minimap reset

Full interactive-map state is **G+0x668**, distinct from Overlay's own
visibility. Ordinary map toggle0x1031e0 changes that byte only.
The own MapOverlay draw **0x2829c0** has no initial F4, map-closed or
drag early return. When its traversal admits it, collection/sorting
are followed by the embedded-item semantic reset **0x285b41** and
Overlay+0x250/+0x254 reset to INT32_MAX. Its earlier draw entry also
resets map renderer G+0x10's+0x150/+0x154 output pair to INT32_MAX.

Only **after** the embedded item/Overlay-cell reset does it require
**G+0x1db0==0 and G+0x668!=0** to perform marker hover selection.
Restricted item-map byte+0x669 further filters candidates. Eligible
item markers can then semantically copy the original marker item into
Overlay+0x1b0 at **0x285e31**, as in source-lifetimes §1.

Consequently ordinary visible gameplay with full map closed still
draws the minimap overlay and reaches the reset. Reached dragging
also reaches the reset but skips hover copying. They leave the reset
empty item/cell outputs. F4, a hidden Overlay root or another hidden
ancestor instead prevents entry and therefore prevents this reset.
An actual skipped draw is not equivalent to a reached closed-map draw.

## 5. Ordered frame stages and the live borrowed-item consequence

The application loop **0x1355a0** drains queued window messages before
ordinary input polling/frame work: PeekMessage0x13646b, dispatch
**0x1364b1**, repeated peek0x1364e2. Its later admitted frame call is
**0x134420 at0x137b8e**. Accordingly a queued F4 key-down handled by
this drain has already changed root visibility before that frame's
selector. F4 is not inserted after selection as a second frame toggle.

The relevant order inside the ordinary reached Game/render path is:

1. Ordinary use0x8ae50 at0xa9b2c, subject to its existing admission.
2. Preview selection **0xb6b00 at0xa9d22**. Draw-produced map outputs
   read here are from their most recent reached producing draw.
3. Later panel-toggle block: ordinary map toggle **0xaafcc**, Ring
   toggle0xab0be..0xab0e7, other panel toggles; then the existing
   movement-close stages. Their individual gates remain Ring §7.3
   and item-map-selection, not a new F4 policy.
4. Interface update **0xab5ce**, including collection binding, map's
   Ring close and final game-UI/MapOverlay own visibility assignments.
5. Scene render **0xe2e00 at0xad9d4**, ending with late GUI traversal
   via0xc4af0/0x33e170. In the admitted constructor-established root
   order, Voxel's game-UI subtree precedes MapOverlay's reset/hover
   copy, and that precedes Preview's background/children and hook.

There is **no second selector after the current Overlay draw**.
However, selecting Overlay+0x1b0 stores a **borrowed pointer to that
same member**, not an item snapshot. A later admitted Overlay draw
can reset/replace the pointed-to item before Preview's hook reads and
adapts it. Comparison selection and placement are already computed;
the copied coordinate scalars Preview+0x218..+0x224 do not become
references and do not change with that later reset. Native timing can
therefore combine earlier scalar/comparison decisions with the later
contents of the borrowed item member.

If the selector did not choose that member because it was empty, a
new item copied during the current Overlay draw does not retroactively
change the selected pointer. Conversely, if it did choose it, a later
reset does not clear the already stored pointer or recompute comparison.
The next normal selector performs its normal priority test again.
This is a storage/order fact, not a promise about invented category0
text or a reason to add another hover/select pass.

## 6. Bounded checkpoint

F4 state, draw rejection, continued selector/bind execution, minimap
reset admission and the ordered borrowed-member behavior above are
verified. This does not audit every unrelated writer of GUI visibility,
all owner-defined cache/animation settings or exceptional destruction
during callbacks. Existing source-lifetime and ordinary Plasma contracts
retain those broader ownership/rendering boundaries.
