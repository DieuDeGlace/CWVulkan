# Steam GUI pointer, capture and registered-event routing

Stable bounded native-only integration contract. Sources:0x339b70,
0x340ec0,0x3410e0,0x3418d0,0x33c410,0x3493b0,0x33c360,
0x342560,0x369270/0x3694d0/0x369c70,0x36a140/0x36a240,
0x101020/0x101120/0x101160,0x133c70 and0x1355a0, with native binary vtables.
No engine, tests, exporter, build, SDK, owner data/captures or reconstruction
were inspected; no game was executed. RVAs refer to the Steam build.
The enumerated target events are left down/up2/3 and pointer move12;
other mouse-button and keyboard event families retain their own handlers.

## 1. State objects and initialization

Let **E** be the GUI engine, Game+0x20; **N** a scene node; and **W**
a Widget. These are different bases. In particular E+0x168 is not the
Game's character-list scroll field, and E+0x188 is not W's event map.

Base GUI engine constructor **0x339b70**, reached by D3D GUI backend
constructor0x38e100, initializes these retained fields:

|E offset|Meaning / initial value|
|---|---|
|+0x140|Global input/draw root, newly created named root node|
|+0x160|Previous movement-dispatch hit/captured node, null|
|+0x168|Explicit captured **node**, null|
|+0x170|Focused **Widget**, null|
|+0x178|Auxiliary shown/popup **Widget** managed by0x342270, null|
|+0x180/+0x184|Current float32 GUI pointer XY,0/0|
|+0x188/+0x18c|Previous float32 GUI pointer XY,0/0|
|+0x190/+0x194|Signed32 GUI step-ms / accumulated clock,0/0|
|+0x1a8|Held-button32-bit word,0; logical left is bit0|
|+0x1b0|Qualifying left-down owner marker, null; distinct from capture/focus|

The constructor initializes option word+0x288 to7. Existing scene/hit
math and option-mask semantics remain
[Plasma §§2.3.5,2.7.2/2.7.8](plasma-format-and-scene.md) and
[exact panel hit rules §4.11](remaining-runtime-gaps.md). The initial cached pointer
is not a query of the current OS cursor.

Node **+0x70 is its own Widget**; **+0x78 is its resolved owner Widget**,
which can be inherited from an ancestor. Targeted virtual and registered
pointer dispatch use+0x78. Global Widget visitors use own Widgets+0x70.
Do not replace both relations with the nearest rectangle handler.

## 2. Picking versus dispatch admission

The three audited left-down/up/move functions choose a target node from
**E+0x168 if nonnull**, otherwise **0x333f60(E+0x140,current XY,options0)**.
They do not use an independently maintained per-panel hover rectangle.
The shared query **0x33f1e0** exposes this same capture-or-fresh-root-pick
result. It is not a read of the retained movement hit E+0x160.
Button events use already cached XY and do not sample message coordinates
or the OS cursor first. Down/up do not themselves replace current/previous
XY or store a new movement-hit+0x160.

For no-capture picking, retain the native scene traversal: visibility and
flag filters, reverse child-list order, native clip admission, inverse
transform/deformation and actual shape/Widget hit rules. Link the geometry
contracts above rather than approximating them with drawn AABBs. A child
can win outside its parent's own drawing when no native clip gate excludes
it. Passive node bit0x02 filters that node's hit eligibility after child
search; it is not the same subtree rule as the global visitor below.

This is one scene pick followed by dispatch. It does not search again for
another underlying node merely because the first node has no callback or
its owner is disabled. The targeted virtual hook is invoked only when
owner W exists and **enabled byte W+0x1a0 is nonzero**.

Registered dispatch separately requires the selected node's
**passive bit0x02 clear**, owner W nonnull, and a nonnull callback object
for the requested event in **W+0x188**. Invoke it when W is enabled **or
the callback object's enabled-required byte+8 is0**. Thus a disabled
Widget can have an explicitly allowed registered callback while its
ordinary virtual hook is skipped. Do not merge these two admission tests.
0x32a210 supplies the optional event lookup;0x342560 uses the same
registered-event gate for explicitly requested node events.
The lookup is on that resolved owner only; it does not bubble to another
ancestor Widget when no callback exists. The per-event entry holds one
optional callback object. Recursive registration installs callbacks on
multiple Widgets beforehand; it is not runtime event bubbling.

With capture, no fresh scene-root hit, ancestor visibility walk,
inside-frame check, node visibility check, or node flag0x04/0x1000 test
is inserted by this target-selection path. Registered dispatch still has
its passive/owner/enabled-required gates; the virtual hook still has its
owner-enabled gate. Its passive condition is not an additional explicit
gate on the virtual hook. Root-hidden/captured and root-hidden/uncaptured
therefore differ. Visibility assignment alone is not capture cancellation.

## 3. Pointer move0x3418d0

First copy current float XY to previous XY, then store supplied new float
XY. This happens even when the values are unchanged. Resolve the target
using§2; keep that selected node for this dispatch.

Compare **owner Widget pointers**, not node pointers, between previous
movement node E+0x160 and the selected node. If they differ, perform:

1. Old enabled owner virtual **+0xb8 leave**.
2. Registered **event14** on the node then stored in E+0x160, under its
   independent callback gates. This field is reloaded after the leave virtual.
3. New enabled owner virtual **+0xb0 enter**.
4. New node registered **event13**, under its independent callback gates.

The old/new owner pointers used for these virtual hooks are obtained
before this transition sequence. The new enter and move virtuals use
that selected owner's saved pointer, with its enabled byte tested at
invocation. Registered phases reload the corresponding node's owner/
callback rather than reusing that virtual-owner pointer.

Then, for a nonnull selected node, call its enabled owner's virtual
**+0xa8 move**, followed by its registered **event12**. Finally store
the selected node into **E+0x160**. Changing between descendants with
the same owner does not emit leave/enter, but still emits that owner's
move hook and selected node's registered move event. There is no
nonzero-delta gate, elapsed-time test or render-frame coalescing here.

After this targeted phase, the function performs the global own-Widget
visitor stage in§6. Callback-driven changes do not cause this outer
move to choose a fresh target halfway through. Exact reentrancy/deletion
limits are§8; this is not authority to cache a separate stale owner map.
0x3418d0 does not advance the GUI clock or Interface0x101160's separate
filtered-pointer timestamp. An internal capture-clear refresh is not
an extra call to that Interface filtering/timer layer.

## 4. Left down0x340ec0 / event2

Set **E held bit0 before selecting the target or running callbacks**.
There is no prior-held-edge requirement suppressing a repeated reached down.
Resolve the capture-or-root target from cached XY. Before its targeted
down hook, the following distinct bookkeeping occurs:

* If its owner exists, is enabled, and **W flags+0x150 mask0x20 is set**,
  store that owner in **E+0x1b0** and run the conditional global virtual
  **+0xe0** visitor. Otherwise store null in E+0x1b0. This marker is not
  E's captured node.
* If auxiliary Widget E+0x178 exists and the selected node lies outside
  its tree according to0x3308c0,0x342270 closes that auxiliary Widget.
* Compare the selected node's **own Widget+0x70**, not owner+0x78, with
  focused Widget E+0x170. A missing node or differing own Widget marks
  the old focus Widget dirty at+0x15c, requests its root registered
  **event0x13, decimal19**, and clears E+0x170. This focus-loss event
  is different from decimal13 pointer-enter.

For a nonnull target, call enabled owner virtual **+0x58 left down**,
then the original selected node's registered **event2**. The virtual
can establish capture; event2 is not redirected to a newly captured
node by a second pick. The registered phase reads the selected node's
then-current owner/callback after the virtual returns.

Afterward run the global own-Widget virtual **+0xe8** visitor. No generic
registered release/click event is synthesized here. These functions do
not return an event-consumed flag to stop all later Game business.

## 5. Left release0x3410e0 / event3 and nested refresh

Clear **E held bit0 first**, even if it was already clear. Then select
the capture-or-root target and keep this original release node. Close
the auxiliary Widget when E+0x178 and down marker+0x1b0 are both nonnull.
For a nonnull target, invoke enabled owner virtual **+0x60 left release**,
then registered **event3 on the original release node** using its
then-current owner and callback gates. There is no general matching-down,
same-hover-node, inside-rectangle or Button-held-byte predicate around
this registered event. Individual Game/Widget business callbacks can
still have their independently specified own gates.

For a normal Button, the release virtual makes this phase **reentrant**:

1. **0x36a240 first calls0x3493b0**.
2. **0x3493b0** unconditionally writes **E+0x168=null**, then immediately
   calls **0x3418d0 with the current XY unchanged**. This is a full nested
   move dispatch, with previous=current and delta0, fresh root picking,
   possible leave/enter/move callbacks and global visitor. It is not just
   clearing a pointer or waiting for the next physical movement.
3. After that nested dispatch returns, the Button clears its held byte
   **+0x2b8** and starts its release/release-checked sequence.
4. The outer release then reaches its original target's registered event3.

Consequently a captured Button can receive its registered release even
outside its current hit region: nested hover now reflects the pointer's
fresh root pick, but the outer event3 still targets the original captured
node. Its original node may be the Button root whereas the preceding
movement hit was a descendant; apply passive gating to the node actually
used for that event. Do not substitute a new hit's callback or add a
universal release-inside requirement. A nested Controls-thumb move sees
the already-cleared engine held bit and does not apply a final drag step.

After registered event3,0x3410e0 reloads capture or obtains a fresh root
pick, compares that owner with a fresh root pick's owner, and if different
calls that enabled owner's leave virtual and registered event14. This
post-release comparison does not replace the earlier event3 target.
Finally it runs global virtual **+0xf0** visitation.

The release dispatcher itself **does not unconditionally clear capture**.
That clear comes from the dispatched Button release or another explicit
caller. A different dynamic class, or a disabled owner whose release
virtual is skipped, must not gain an invented generic clear/recovery.
The down-owner marker E+0x1b0 is not generically zeroed by this release.

## 6. Global visitors, root/ancestor admission and Game layering

After move, global thunk **0x33aab8** calls each admitted own Widget's
virtual **+0x130**. The down conditional thunk0x33aac4 calls+0xe0;
ordinary down thunk0x33aad0 calls+0xe8; release thunk0x33aadc calls+0xf0.
These are **virtual broadcasts**, not registered events12/2/3 repeated
for every Widget.
Ordinary Button vtable0x47b3a0 maps these four global slots
+0xe0/+0xe8/+0xf0/+0x130 to no-op0xe8a20. Its targeted hooks remain
distinct; another dynamic class can supply global overrides.

The root must have own live visibility and neither node flag0x04 nor
passive0x02 before its own Widget/children are visited. Recursive
**0x33c360** applies those same conditions at each node, visits its
own Widget+0x70, then children in forward list order. A passive node
therefore prunes this visitor's subtree, unlike its individual-hit
filter role. The visitor does not independently gate a Widget on
enabled+0x1a0; the receiving virtual can have its own behavior. Hit-only
flag0x1000 is not a filter in this visitor.

These root/ancestor visitor gates are separate from captured-target
dispatch, which can already have happened before the global stage is
skipped. Do not use one broad root-visible boolean to erase all capture,
pointer-history, hover-transition and registered-callback distinctions.

Interface **0x101020** forwards left down to0x340ec0; **0x101120**
forwards left up to0x3410e0. The former also seeds its separate filtered-Y
field from cached GUI Y; that does not update E's pointer. Their Game
callers0x94900/0x965e0 continue their independently specified manual
inventory/map/dialogue/customization routes after GUI dispatch unless
their own branch returns. Registered Buttons and manual Game callbacks
are not interchangeable activation policies. See
steam-voxel-widget-down-event-routing and remaining-runtime-gaps §4.10
for the relevant control/capture route; the per-widget activation-phase
table and common-tail precedence remain those existing documents' scoped
contracts. A visibility change in a registered callback does not itself
trigger a new movement/hover calculation afterward.

## 7. Button identity and owned settings

The actual dynamic class comes from the owner's loaded Widget and its
clone operation, not its node name or a rectangle's appearance.
**Button0x369270, vtable0x47b3a0** initializes enabled/base settings,
Button.type+0x2bc and held/check bytes+0x2b8/+0x2b9=0. Dynamic cloning
through0x369c20→0x3694d0 copies base settings, all eight sequence names
and Button.type, creates a fresh group list and clears held/check.
Other classes, including ScrollButton/ScrollSlider and custom wrappers,
retain their own dynamic hooks rather than becoming plain Buttons.
The targeted move hook and precise group/check ordering are expanded
in§11; normal Button move is the inherited0x32bb80 handler, not no-op.

Button down **0x36a140** captures **its own root node+0x178** through
0x33c410, which only stores E+0x168 and does not itself refresh the pointer
or call an OS capture/warp operation. It sets held1 and its native
bookkeeping fields, then starts the
appropriate press sequence. Type0 uses ordinary press without changing
check state. Nonzero types choose press/press-checked from the current
check byte; type2 first clears checked members in its rebuilt group,
and remains checked when already checked, whereas other nonzero types
toggle. Button release0x36a240 has the ordering in§5 and chooses ordinary
release for type0 or unchecked, release-checked otherwise.

Owner sequence names, initialization poses, enabled setter behavior and
pointer enter/leave are [Plasma §2.7.9](plasma-format-and-scene.md). Initialization
seeks/stops the documented poses; ordinary press/release/enter/leave
start the owned timed sequence and are not an immediate end-pose or
guessed transform assignment. Pointer enter/leave select checked variants
only for nonzero Button.type with check byte set. Base Widget flags,
enabled state, binding and drawing remain owned inputs. The registration
helpers do not force a new dynamic class.

## 8. Cadence and bounded lifetime facts

The direct-call audit of the normal application loop finds one
**0x97880 call at0x13683e inside0x1355a0**, after window-message draining,
before XInput/controller synthetic work and frame driver0x134420.
Its normal branch requires **0x536ce8 nonzero**, main window0x551ac8
foreground, and **0x9ae50 false**.0x9ae50 is true when Controls
waiting-index+0x2e0 is nonnegative, so a reached normal poll requires
that waiting index negative. Waiting suppresses this normal pointer/
XInput branch. This is the live predicate after queued message handling,
not a pre-message waiting snapshot. The inactive route sleeps16ms. The semantic owner name
of0x536ce8 remains unverified.

Ordinary-UI predicate0x1014f0 selects absolute GetCursorPos input;
fullscreen-state DWORD0x551bf0=0 adds ScreenToClient, while its alternative
uses the cursor coordinates without that conversion. Signed32 XY become
floats for0x97880, whose UI branch truncates them to signed32 for
Interface0x101160, then GUI0x3418d0. Gameplay-relative polls do not call
that GUI route. There is no coordinate-change condition on the reached
poll. WM_MOUSEMOVE0x200 returns1 in0x133c70 without an extra0x97880 call.

Physical button messages can therefore run against the preceding cached
pointer before that iteration's poll. Synthetic pad down/up use the same
Game/GUI handlers. Later SetCursorPos navigation does not itself replace
E's cached XY or run a second normal poll; a subsequent admitted poll
observes it. Other explicit0x3418d0 calls, including capture clear and
the mapped-map cursor update, retain their own phases. Full cadence and
filter timing are [pointer-poll cadence §7](steam-character-list-input-and-frame-init.md); no frame
delta is substituted for this event ordering.

**Window capture is a separate layer.** In0x133c70, physical down messages
**0x201/0x204/0x207** call **SetCapture(window) before Game0x94900**
with button IDs0/1/2. Their up messages **0x202/0x205/0x208** call
**ReleaseCapture before Game0x965e0**, again IDs0/1/2. Each listed up
performs that OS call without an all-other-buttons-released test. This
is not E+0x168 or Button's internal0x33c410/0x3493b0 operation. Direct
synthetic Game-handler calls do not pass through that window-message
wrapper. The inspected window procedure handles focus messages7/8 by
returning1 without a GUI held/capture reset; skipping the normal
foreground-gated poll does not itself synthesize GUI releases. This
does not predict later messages produced by the OS or invent recovery
for capture loss outside the verified handlers.

Known lifetime cleanup is explicit: **0x33dda0**, deleting a node owned
by E, clears capture if it is that node and runs an unchanged-pointer
update before proceeding, then clears E+0x160 if it still equals that
node, and recursively removes children. Own-Widget deletion **0x33e0c0**
clears matching focus, auxiliary-Widget and down-owner references.
These are native deletion paths, not proof that arbitrary callback-driven
deletion/reparenting is safe. The dispatchers retain raw selected nodes
over virtual/registered callbacks; no stable-ID recovery or general
liveness/re-pick layer was verified. Disabled/hidden capture, unknown
dynamic-class overrides and unsupported callback mutations must not gain
invented recovery behavior.

## 9. Exact down/up owner-read boundaries

These observations distinguish a retained **target node** from a retained
**owner pointer**. They come from0x340ec0/0x3410e0's native field loads.
Neither function freezes one owner pointer at initial capture/root pick
and reuses it through all earlier bookkeeping/callback stages.

For **left down**:

* Keep the initially selected node through marker/global-visitor,
  auxiliary-close and focus work. Capture changes during those stages
  do not make this outer down choose a different node.
* The first owner+0x78 read is for flags/enabled admission and the
  E+0x1b0 marker. If it qualifies, that marker stores this owner before
  the global+0xe0 visit. The marker is not automatically replaced if
  that visit changes the selected node's owner.
* Auxiliary containment/close uses that same selected node. The close
  operation described in§10 has no callbacks of its own.
* The focus comparison then reads the selected node's **own+0x70** and
  current E+0x170. Its focus-loss registered event19 can run callback
  code; afterward this down still writes E+0x170=null. A focus callback
  assigning another focus value does not bypass that final clear.
* **After all of those stages**, reload selected node+0x78 for the
  enabled-owner virtual+0x58 call. This is not the earlier marker owner.
* After the virtual returns, check the selected node's passive bit and
  **reload node+0x78 again** for registered event2 and its callback/
  enabled-required checks. A capture established by the virtual is not
  a replacement target; an owner change on the retained node is observed.

For **left up**, retain the initially selected release node after clearing
held bit0. E+0x178/E+0x1b0 auxiliary-close work occurs **before the owner
pointer is loaded for virtual+0x60**. Reload node+0x78 after that virtual
for registered event3. Thus the Button's nested unchanged-pointer update
can change hover/capture and invoke callbacks before the event3 owner
lookup, while event3 still belongs to the original release **node**.

The later post-release leave comparison obtains a new capture-or-root
node, then a fresh root-only hit. It saves their owner pointers for that
comparison and leave virtual; registered event14 then reloads the selected
post-release node's owner after the virtual. This is separate from the
earlier release target. Raw-node deletion during these stages remains
bounded by§8, not repaired by an invented pick or owner snapshot policy.

## 10. Auxiliary close0x342270 and containment direction

The close operation used here is **0x342270(E,newWidget=null,anchor=null)**.
If E+0x178 held an old Widget, first set its root node's **own live
Display.visibility to0**, a 32-bit zero to that attribute's current slot.
Then store **E+0x178=null** and return. If
there was no old Widget, the null store still occurs. This is a direct
live visibility write, not enabled-setter or sequence dispatch.

This null/newWidget branch emits **no registered close, leave, focus or
other event**, and no virtual close hook. It does not clear capture,
focus, E+0x1b0, callback maps or Widget data; does not delete/detach the
root; and does not reposition/resize it. The nonnull replacement/opening
branch of the same helper has separate reparenting/placement behavior
and is not implied by this close-only contract.

Down's test calls **0x3308c0(auxiliaryRoot,selectedTargetNode)**.
The helper walks from the **second** node upward via parent+0x40 and
returns true if it encounters the first node, **including equality**.
It returns false immediately for a null second node. Therefore:

* Selecting the auxiliary root or a descendant keeps it open at this test.
* Selecting its ancestor, another branch, or no node closes it.
* The argument is the saved capture-or-root target, not a fresh root pick
  substituted for a captured node and not an arbitrary rectangle match.

The helper does not inspect visibility, flags or Widget enabled state.
Left up instead closes solely when both E+0x178 and E+0x1b0 are nonnull;
it does not repeat that containment test. These writes explain why the
targeted owner lookup still runs on a node inside a root just hidden by
the close operation: targeted dispatch does not add a new visibility gate.

## 11. Button move, nonzero press and stored groups

### 11.1 Normal Button targeted move is inherited Widget behavior

Vtable **0x47b3a0 slot+0xa8 is0x32bb80**, the common Widget pointer-move
handler. It is not Button's no-op global+0x130. With no applicable owned
flags/state it makes no geometry change, but the virtual still precedes
registered event12 as§3 specifies.

Its first branch requires **Widget.flags+0x150 mask0x80**, logical left
held bit0, and either resize-edge byte **W+0x180/+0x181 nonzero**.
It reads Q-relative frame position/size through0x32c340/0x32d1d0.
Per axis, edge−1 adds the0x32b0d0 direction delta to position and subtracts
it from size; edge+1 adds it to size. Other edge-byte values make neither
of those per-axis adjustments. It sets position through0x32b250 with
flag0, then size through0x32c6f0 with flag1, including that setter's
layout propagation. This all occurs before the registered move callback.

The second branch, reached after the first, requires flags mask1 or2
and **GUI held mask4**. It obtains current pointer XY through0x32c230,
subtracts the stored pointer anchor **W+0xc0/+0xc4**, and zeros X when
flag1 is clear or Y when flag2 is clear. Add the remaining deltas to
stored translation anchor **W+0xc8/+0xcc**, write the node's live
translation attribute, and update its transform. Then call0x3298b0's
parent constraint work and dispatch root registered **event0x11,
decimal17**, before returning to the outer registered event12 phase.

0x32c230 uses current GUI float XY transformed through the **immediate
parent node's inverse world matrix0x100ee0**, including homogeneous
division; with no parent it returns current XY directly. It is not the
frame-direction query0x32b0d0. General frame setters/Q math stay in
Plasma §2.7.2. This subsection identifies the exact move hook and gates;
resize-edge/translation-anchor production and the full0x3298b0 constraint
algorithm are not newly specified here. Do not invent those state values
from the word Button or force movement for ordinary flags0.

### 11.2 Exact nonzero-type press order0x36a140

After capture, the native press writes signed32 **W+0x2c0=−500**, signed32
**+0x2c4=150**, and **held byte+0x2b8=1**. These are verified writes;
this subsection does not invent a repeat consumer for them. Type0 starts
its stored ordinary press sequence and returns without check-byte change.

For a nonzero type, the remaining order is:

1. **Type2 only:** traverse its already stored group list in forward order.
   For each member with nonzero check byte+0x2b9, first store that byte0,
   then start that member's **stored ordinary leave sequence+0x208** on
   its root, period0. This is not the member's virtual leave hook and
   does not emit its registered event14. No enabled/visibility/type
   revalidation is added during this stored-list traversal.
2. Read the pressed Button's current check byte. Start its stored
   **ordinary press+0x1a8 if zero**, or **press-checked+0x228 if nonzero**,
   period0. This start precedes the following own check-byte write.
3. After that start returns, read check state again. If zero, store1.
   If nonzero and current type is not2, store0. If nonzero and type2,
   **do not write its own check byte at all**. In particular this is
   not an unconditional checked=true store for an already checked type2.

Every checked peer is thus cleared before its leave sequence; the pressed
Button's own press sequence starts before its own toggle. Its own entry
is excluded by the verified group builder below. Sequence start is the
owned timed operation0x3342a0, not an immediate seek/stop or synthetic
action callback. Registered event2 follows the entire virtual press.

### 11.3 Group membership is a hierarchy query at initialization

**0x369c70** calls base initialization, frees/empties the previous group
list **W+0x2a8, count+0x2b0**, then rebuilds it before its documented
Button state poses. This happens independently of whether the Button is
currently enabled. It is not a same-name group or a new runtime group ID.

**0x32c090** finds the nearest ancestor node with an **own Widget**, from
the Button root's parent upward, without visibility/enable filtering.
If the Button root has **node.flags+0x80 mask0x10**, take that ancestor's
nearest Widget ancestor instead. With no required ancestor, membership
stays empty.

On that chosen ancestor, **0x32ac80→0x32aba0** collects a Widget frontier:
start at its root, walk children in forward list order through nodes
without own Widgets, append the first encountered own Widget on each
branch, and stop descending that branch at that Widget. The starting
root is not appended. **Node flag0x04 excludes its subtree**; live
visibility, passive0x02 and enabled bytes are not collector filters.

For each frontier candidate, append it if its **Widget kind+0x50 is1**,
it is not the pressed Button itself, and **Button.type+0x2bc is2**.
For an initiating Button whose root mask0x10 is set, first collect each
frontier candidate's own frontier and append qualifying type2 members
from that extra level, then consider that candidate itself. Preserve
that append order. The builder does not add a separate deduplication step.

These are borrowed Widget pointers stored until the next initialization,
not a fresh hierarchy search on each press. Press consults their check
bytes without rechecking the original kind/type filters. Actual owned
hierarchy, mask0x10 and Button.type values determine the membership;
no owner group names, counts or missing data are inferred here. Mutation/
deletion of borrowed members retains§8's raw-pointer limits.

## 12. Internal Button kind versus serialized class and sequence names

### 12.1 Four supported dynamic classes share kind1

The **signed32 Widget.kind at+0x50 is not the serialized dynamic class
name**, nor the object's index in the file's common Widget index map.
Among the seven supported tagged Widget classes in Plasma §1.5, these
four use the Button base layout and retain internal kind1:

| Serialized class tag | Tagged reader | Concrete constructor | Native vtable |
|---|---|---|---|
| `Button` | 0x381f50 | 0x369270 | 0x47b3a0 |
| `ScrollButton` | 0x3839b0 | 0x37ad70 | 0x47c560 |
| `ScrollSlider` | 0x383bf0 | 0x366ac0 | 0x47ae08 |
| `PopUpButton` | 0x3837d0 | 0x37b450 | 0x47c8d8 |

The last three constructors call the Button constructor, which writes
kind1, then install their own virtual tables and additional state without
replacing that kind. Their clone paths0x37ae10/0x366d30/0x37b4f0 call the
Button copy constructor0x3694d0, which also explicitly writes kind1,
before reinstalling the concrete virtual table. Their dynamic input and
initialization overrides are therefore retained despite the shared kind.

Consequently§11.3's group filter includes any of these four classes
whose stored **Button.type is2** and which passes the other hierarchy/
self filters. Restricting that native kind1 test to the exact serialized
name `Button` would exclude native candidates, including ScrollButton.
Conversely, shared kind does not replace virtual dispatch with ordinary
Button hooks; for example ScrollSlider's move hook is0x366ed0 rather
than0x32bb80. This table does not assign internal kinds to the other
three tagged classes or to unexamined game-specific wrappers.

### 12.2 Constructor names are separate from serialized animation data

All four concrete constructors above obtain the same **eight UTF-16
sequence-name strings** from Button0x369270. Their exact names and field
offsets are already listed in [Plasma §2.7.9](plasma-format-and-scene.md).
The derived constructors do not replace them with ScrollButton,
ScrollSlider or PopUpButton name prefixes.

For the **supported tagged file loader**, none of these eight string
fields is loaded from a Button-specific sequence-name property. The
complete Button/PopUpButton readers accept **Button.type** plus the
shared properties; ScrollButton/ScrollSlider additionally accept their
own **direction** property. All otherwise delegate to shared0x386560,
whose accepted properties are Plasma §1.7.4's Widget leaves. That reader
has no eight-name assignment, no `Widget.name`-based prefix substitution,
and returns unrecognized tags to the caller's skip path0x38e010.
Thus an ordinary instance produced by these readers keeps the native
constructor names. Do not introduce unsupported sequence-name tags as
though this loader recognized them.

The **animation definitions** which those names select remain owner
file data: the attributes' serialized sequences/frames in Plasma
§§1.7.1/2.6. Reading such an animation does not rewrite the Button's
stored name fields. Its class tag, own Widget name/caption, stored Button
sequence-name strings and animation definitions are distinct data.

Cloning copies the eight actual source strings through0x3694d0;
it does not reconstruct them from the dynamic class tag. This preserves
any source instance's current names, while the held/check bytes and
group list receive the fresh state already described in§7.

This establishes the supported tagged **read/clone** contract. An
authoring writer's handling of renamed in-memory fields, and arbitrary
programmatic name mutation outside these examined construction/read/
clone paths, remain untraced. No custom owner names or missing owner
group values are supplied by this supplement.
