# Steam character-list drag, inertia and fresh card frames

Independent native-only follow-up to frontend-characters2.3.3–2.3.4.
Section5 corrects the earlier omission of the final Widget-clone callback
in0x333890: the card's fresh unit defaults are temporary, not its attached
binding when the source root has a Widget.
Sources are0xf8690,0x101020,0x101120,0x101160,0x103200,0x97cc0,
0x107440,0x333890,0x328ff0,0x334410 and associated native constants.
No replacement code, tests, exporter/build output, SDK, owner assets or
captures were inspected, and no game was executed.

## 1. Scroll field and clocks

The character-list scroll accumulator is **float32 at G+0x168**, equivalently
interface object **G+0x118, field+0x50**. It is not GUI engine+0x168: that
same numeric offset in the GUI engine denotes a captured node. Interface
constructor**0xf8690** initializes the scroll accumulator to float0.

The frame consumer0x103200 treats this field numerically as screen pixels
per second: its final list pass adds **field times stored float0.001,
then times float frame-ms** to each current queried card Y. Multiplications
are distinct float32 operations. The input's generation is a native drag
heuristic below, not a conventional pointer-distance divided by time
velocity estimate. Positive values move card origins down. The existing
fitting/centering order and screen-pixel query basis remain
frontend-characters2.3.3.

There are **two distinct time inputs**.0x103200 receives the ordinary float
frame-ms argument. Reached GUI pointer polling0x101160 maintains a separate unsigned32
timeGetTime sample at**0x551770**, initialized by a separate timeGetTime
call immediately before the first normal sample. It subtracts the previous
sample in32 bits and interprets the result as **signed32 input-poll milliseconds**.
It stores the new sample on every reached call, including calls without
an admitted character drag. No render-frame dt replacement, poll-interval cap,
negative-interval clamp or guaranteed zero first interval appears here.

## 2. Mouse drag producer

The ordinary-UI pointer route**0x97880→0x101160** first calls GUI movement
**0x3418d0**.0x101160 receives signed32 XY and converts them to floats for
that GUI call. It consequently updates engine current cursor XY+0x180/+0x184
and previous XY+0x188/+0x18c before reading drag deltas.

Interface float **+0x548 (G+0x660)** is a separate filtered cursor Y.
Constructor0xf8690 initializes it to0. Mouse-down entry**0x101020** stores
the current GUI cursor Y there before dispatching the selected physical
button's down event. It does this for the handled mouse-button entries,
not by clearing the scroll accumulator or resetting the poll timestamp.
Mouse-up**0x101120** dispatches
the corresponding GUI up event and contains no scroll-velocity reset.

On each0x101160 call, obtain the integration coefficient from**0xa7a0**
using input-poll milliseconds converted **directly from signed32 to double**,
and rate float **0.01**, word**0x3c23d70a** at**0x42e51c**. Update filtered
Y by subtracting old filtered Y from current raw GUI Y, multiplying by that
float coefficient, then adding old filtered Y, with float rounding after
each operation. Keep the new-minus-old filtered Y as the direct drag shift.
This filtering happens before the character/world-list admission tests.

Character dragging requires **GUI[3]'s own live Display.visibility nonzero**
and **GUI engine+0x1a8 bit0 set**, the shared left-button-held bit. It does
not additionally require that the original press hit a card, that a card
is currently hovered, or that capture is null. The character branch takes
precedence over the separate old world-selection-list branch.

For each character card, immediately move its current queried Y by that
**filtered** shift. Recenter X exactly as the frame consumer does:
float(signed window width/2) minus queried card width times float0.5.
The frame setter is0x32b560 with changed/update byte1; no drag-time list
clamp or substitute container translation is inserted. Native character
loop calls are**0x1012d3/0x1012f1/0x10130b**.

After the card loop, including when the vector is empty, update the scroll
accumulator from the **raw** current-minus-previous GUI Y. Convert signed32
input-poll milliseconds to float, multiply raw delta Y by that float interval,
**add the result to itself**, then add the old accumulator and store float.
These operations are visible at**0x10131c–0x10134c**. They do not divide by
poll time, substitute the filtered drag shift, multiply by GUI scale or
clamp to a speed limit. Preserve this arithmetic even though it is not a
standard velocity estimate.

The filtered-Y state and timestamp still advance on non-drag polls. The
world-selection branch uses its separate float+0xe0 accumulator, not this
character field. Do not combine those two lists' state.

## 3. Frame decay, release and resets

While GUI[3] is shown,0x103200 applies the inertial movement only in the
existing nonempty-vector, left-button-not-held list loop. Thus a held
button uses direct mouse-move dragging rather than also applying velocity
in that frame loop. Upper/lower fitting also waits for the button release.

Later in the same **shown GUI[3]** branch, after control/caption handling,
**0x103c18–0x103c3d** obtains an0xa7a0 coefficient from the float frame-ms
argument promoted to double and rate float **0.005**, word**0x3ba3d70a**
at**0x42e518**. Compute float0 minus the old accumulator, multiply by the
returned float coefficient, add the old accumulator, store float. This
decay still occurs with the left button held or an empty card vector;
its placement is outside the card-movement admission. It occurs after
that frame's admitted inertial displacement, not before it. There is no
explicit epsilon cutoff, speed clamp, sign change or per-frame reset here.

When GUI[3] is hidden, the alternative branch at**0x103c45** writes exact
float0 instead. The basic0xa3a20 visibility setter does not itself clear
this field; the next reached0x103200 hidden branch does. Neither mouse
press nor release, window-layout0x101bd0, or list rebuild0x107440 introduces
another reset of this field in the inspected paths. A rebuilt shown list
therefore retains the current accumulator until the usual frame behavior
changes it. The filtered-Y value is a different state and is not cleared
by this character-hidden reset.

## 4. Wheel, keyboard and controller boundary

The native wheel dispatcher **0x97cc0** has no character-list branch and
does not assign G+0x168. Its verified targets are eligible inventory/shop/
crafting/other list panels and its map-zoom fallback. Do not turn the
character momentum value into a guessed mouse-wheel row/notch mapping.

The independently traced ordinary-UI XInput route, remaining-runtime-gaps
§4, supplies left-stick cursor navigation and A's ordinary mouse down/up
synthesis. Right-stick wheel repetition requires eligible inventory,
crafting or shop panels; GUI[3] alone is not an eligible character-list
wheel target. That verified path does not specify a direct stick assignment
to this accumulator. Controller-generated ordinary pointer movement while
the shared left-held bit is set can reach the **same mouse-drag route**;
it is not a separate analog list-speed formula. No direct keyboard producer
for this field was established in these native frontend paths. Preserve
the verified drag/input routes instead of inventing arrow, wheel or stick
bindings from the existence of a scroll field.

## 5. Fresh CharacterPreviewWidget and transferred template binding

Builder**0x107440** creates each card with shallow instance**0x333890** of
the blackwidget node hierarchy under GUI[3]. This recreates nodes, initially
sharing drawing/transformation/display objects. New node constructor
**0x32ee60** initially makes own-widget+0x70 and owner-widget+0x78 null,
but that is **not the completed instance's Widget state**. After recursively
copying eligible children, flags and node variable lists,0x333890 checks
the source node's own Widget and invokes its **virtual clone slot+0x140**
with the new node. The call is explicit at**0x333990–0x33399f**. Each child
instance goes through the same process; children with node flag0x04 are
excluded. Sharing the artwork does not suppress Widget cloning.

For an ordinary Widget the callback is**0x32a2c0→0x329210**, confirmed by
vtable**0x477988, slot+0x140**. Its copy constructor preserves the source
bind/frame/minimum geometry, bind matrix and inverse, flags/alignment,
caption and enabled setting, and attaches the copied Widget as the new
node's own Widget. Other source dynamic Widget types use their own clone
callback. The owner's template supplies that dynamic type and numeric
geometry; no owner file was inspected for this audit.

The new CharacterPreviewWidget wrapper is then constructed by
**0x26ea60→0x268a70→0x328ff0**. Before attachment, the base constructor
temporarily establishes these defaults:

| Geometry/state | Native default |
|---|---|
|inner bind position+0x68/+0x6c|(0,0)|
|inner bind size+0x70/+0x74|(1,1)|
|bind position+0x78/+0x7c|(0,0)|
|bind size+0x80/+0x84|(1,1)|
|frame position+0x88/+0x8c|(0,0)|
|frame size+0x90/+0x94|(1,1)|
|minimum size+0x98/+0x9c|(0,0)|
|bind matrix+0xd0, bind inverse+0x110|identity|
|widget flags+0x150, horizontal/vertical alignment+0x154/+0x158|0,0,0|
|enabled byte+0x1a0|1|

Base constructor attachment goes through**0x334410**. When the receiving
root has the Widget just cloned by0x333890, attachment **does transfer its
binding** via**0x329c70**, then removes/deletes that old wrapper through
0x33e0c0 and attaches the new one through0x32d190. The transfer takes the
following source values before deletion:

|Source getter/value|Meaning used by the replacement|
|---|---|
|0x32ab60, old+0x88/+0x8c|old stored frame position becomes both new bind position and initial frame position|
|0x32ab80, old+0x90/+0x94|old **stored frame size** becomes both new bind size and initial frame size|
|0x32ad70|old frame position plus old inner-bind position minus old bind position becomes new inner-bind position|
|0x32adb0|old stored frame size plus old inner-bind size minus old bind size becomes new inner-bind size|
|old+0xd0 matrix|copied to new bind matrix; its inverse is recomputed by0xdfc30|

The new minimum is new bind size minus new inner-bind size, with separate
float32 subtraction. These source size getters are **raw stored fields**:
0x32ab80 is neither effective-size getter0x32a110 nor transformed query
0x32d1d0. Likewise0x32adb0 does not apply the nonnegative/minimum clamp of
0x32adf0. Even if the source raw frame size is below its effective minimum,
the transfer retains that raw size and derives the inner size from it;
normal effective-size constraints subsequently determine the result. The
old source minimum itself is not copied by0x329c70: the new minimum is
derived from the new bind and inner sizes even when it differs from the
source minimum. A minimum-constrained source is therefore not a license
to replace either raw getter with its effective visible dimensions.
Do not substitute a queried effective rectangle in this reset-binding step.

This transfer writes geometry, minimum and bind matrices, not all source
Widget settings. The new wrapper retains its freshly constructed flags0,
alignments0 and enabled1, rather than adopting the old wrapper's dynamic
class or enabled/button policy. If a source root genuinely has no own
Widget,0x333890 has no root Widget to clone and0x334410 has no old binding
to transfer; only that case retains the unit defaults. There is no fallback
drawing-AABB fit. The native template data, not an invented numeric bind,
determines which case applies.

The custom-widget constructor makes shared root transformation/display
unique through**0x335830/0x3356d0** when their reference counts require it.
This preserves their contents while separating subsequent per-card changes;
this is separate from the preceding binding transfer. Attachment replaces
only the receiving **root's** old own Widget. Descendant own Widgets cloned
by0x333890 are retained; it neither deletes them nor flattens their binding
into the card. Initialization refreshes owner-widget pointers according to
the existing nearest-own-Widget rule.

For each entry, the builder then requests **600×220** through0x32c9a0,
stores the saved-character pointer (or null for the final New card), and
requests frame origin **(300,50+230 times index)** through0x32b560. These
are frame setters under the current query matrices, not raw node/shape
size assignments. The requested size goes through inverse Q and constraints
as frontend-characters2.3.3 describes; no second GUI-scale multiplication
is added. The first size request therefore acts on the transferred binding
when present, not on unconditional unit geometry.

After all entries,0x333820 initializes the card hierarchy while the three
existing controls are detached. CharacterPreviewWidget's virtual init
resolves to **generic0x32b830**, not the Button override. It records the
window dimensions, sets minimum to bindSize minus innerBindSize, and updates
caption when present. Enabled generic initialization does not play
widget:enable or button:leave. The controls
are then reparented7,8,9. This preserves the separate native paths for
new card geometry versus copied Button settings and evaluated Button poses.

### 5.1 No additional lazy binding in queries or drawing

The frame/size query chain**0x32c0c0/0x32c340/0x32d1d0**, scalar wrappers
0x32da70/0x32dab0/0x32da30/0x32ad10 and matrix helpers
0x32cae0/0x32cd10/0x32ce40/0x32cf50/0x32d010 calculate outputs from
existing node matrices and Widget geometry. They do not import a template
Widget, fit artwork bounds or replace bind matrix/inner/bind fields. Thus
the important geometry change is the **earlier attachment transfer**, not
a lazy query before600×220. Set-size0x32c6f0 changes stored frame size and
performs its documented constraints/descendant resize propagation; its
CharacterPreviewWidget resize slot+0x50 is the no-op0xe8a20 in vtable
0x46d6e0. It does not perform another binding reset.

The character constructor's font selection**0x269130** only stores the
resolved font at+0x1a8. Drawing entry**0x26eba0** changes its text-style
fields through0x2691b0/0x2691f0 and the other custom text setters; those
are not replacements for the attachment binding above. No owner drawing
coordinates or numeric template bind have been inferred here. Preserve
the cloned drawing and live node transforms together with the transferred
Widget binding; a raw-art-versus-unit-bind mismatch cannot justify dropping
that verified transfer.

## 6. Hidden-node force: advance versus absolute seek

These are distinct native admission orders, relevant to the initialization
poses referenced above. The table assumes node+0x80 bit0x04 is clear,
Display.visibility0, own node+0x80 bit**0x2000 set**, and incoming parent
force0:

| Native traversal | Hidden node admitted? | Descendants visited? |
|---|---|---|
|ordinary advance0x335920|No|No; its own force bit is never propagated|
|absolute seek0x334ef0|Yes|Yes, with force1|

Advance first requires the node's current visibility or incoming force.
Only inside that admitted branch does it combine the current node's0x2000
with incoming force for its children. Thus the current bit can force
hidden descendants of an already admitted node, not admit its own hidden
node. Absolute seek combines own0x2000 with incoming force **before**
testing visibility, and uses that combined value for both its own admission
and descendants. Both traversals reject the excluded bit0x04 regardless
of force. Their ordinary wrappers0x3358e0 and0x334ec0 start with incoming
force0. Named sequence start0x3342a0 is a separate recursive operation,
not either of these property-evaluation admission tests.

## 7. Pointer-poll cadence, admission and stationary calls

In the inspected stock Steam image, the direct-call index has one caller
of0x97880: **0x1355a0, call0x13683e**. That routine's outer application
loop drains its message queue first, using PeekMessage/GetMessage,
TranslateMessage and DispatchMessage. Its admitted normal input branch
then polls keyboard/mouse input, applies the mapped control states and
calls0x97880 **once**, before XInputGetState and the iteration's controller
button/cursor work. There is no coordinate-change predicate around this
call. The loop subsequently calls its ordinary frame driver0x134420 and
repeats; this is an input-loop poll clock, not a WM_MOUSEMOVE clock and
not a substitution of the frame driver's elapsed time.

Admission to that normal branch requires native enabled/ready byte
**0x536ce8 nonzero**, main window**0x551ac8** being the foreground window,
and binding-capture predicate**0x9ae50 false**. The last predicate tests
whether Controls widget**G+0x658, signed32 field+0x2e0** is nonnegative;
a nonnegative waiting index skips this normal pointer/XInput branch.
The inactive/disabled route sleeps16ms instead. The semantic owner of
0x536ce8 is not independently named here. An unreached pointer poll does
not advance0x101160's timestamp, so a later reached interval can include
the skipped time; no cap was found in that helper.

Within the normal branch, ordinary-UI predicate**0x1014f0** selects
GetCursorPos absolute coordinates. When fullscreen-state DWORD
**0x551bf0 is0**, ScreenToClient converts them; the alternative uses the
cursor coordinates without that conversion. With ordinary UI false, the
caller uses relative mouse input instead. Instructions
**0x136813–0x13683e** convert signed32 XY to float for0x97880. That helper
rechecks0x1014f0: only its true branch calls0x101160 at**0x978e3**, with
signed32 coordinates converted from those floats. Gameplay-relative polls
therefore do not advance the GUI helper's timestamp.

Window procedure**0x133c70**, registered by0x1355a0, handles
**WM_MOUSEMOVE0x200 by returning1** without calling either helper. Its
mouse-button down/up and wheel branches use the separate established
Game handlers. Message-driven button callbacks can therefore precede the
iteration's pointer poll; they do not imply an extra movement poll per
message. XInput's synthetic mouse down/up also calls those handlers, not
0x97880. Its later SetCursorPos navigation has no second0x97880 call in
that iteration; its position is observed by a subsequent admitted pointer
poll. A generated WM_MOUSEMOVE does not bypass this ordering.

On a **reached call with unchanged XY**, GUI helper**0x3418d0** still
copies old current XY to previous XY, stores the supplied current XY and
runs normal hit/capture/hover/move processing.0x101160 still samples and
stores timeGetTime and updates the filtered Y before checking drag
admission. If the filter is still behind a stationary raw Y, its filtered
shift can remain nonzero, and a held admitted drag continues moving the
cards toward it. Raw delta Y is0, so the admitted momentum update adds0;
it does not clear the accumulator. Its separate shown/hidden frame decay
in section3 remains in force. If the filter already equals raw Y, its
direct shift is0; the timestamp and processing are still reached.
