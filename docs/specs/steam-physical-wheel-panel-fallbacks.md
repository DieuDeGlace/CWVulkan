# Steam physical-wheel Controls and Multiplayer fallbacks

Stable bounded native-only supplement to steam-map-region-history §6.2.
Sources:0x133c70,0x97cc0,0x275a70,0x276ea0/0x276f10/0x276f40,
0x279db0,0x27b360/0x27b470/0x27b490 and0xb33f0, with binary constants.
No engine, tests, exporter, build, SDK, owner assets/captures or
reconstruction were inspected; no game was executed.

## 1. Dispatcher precedence and admission

The physical window-wheel path **0x133c70→0x97cc0** reduces the signed
wheel delta to **−1,0,+1**; it does not pass the magnitude or accumulate
fractional wheel notches. Inside0x97cc0, test these Widget roots in order:

1. Inventory Game+0x330.
2. Shop Game+0x350.
3. Crafting grid Game+0x340.
4. **Controls Game+0x658**, hit call **0x97e16**.
5. **Multiplayer Game+0x388**, hit call **0x97e71**.

Each candidate requires its root's **own live visibility** and a true
**0x333e70** subtree hit at the cached GUI pointer. This is the established
standalone hit query, with options0 and the ordinary deformation/outline
rules; it is not a guessed panel rectangle or a newly sampled wheel-message
screen position. It does not independently walk all ancestors' visibility.
The first shown-and-hit candidate consumes the branch, including delta0.
An earlier shown panel that is not hit does not consume it. Multiple shown
panels retain this precedence, regardless of their later GUI draw order.

There is no extra mapped-Walk, map-open, focused-editor, or Controls
binding-wait test on the Controls/Multiplayer branches inside this
dispatcher. Those branches precede the existing fallback that chooses
camera orbit versus map zoom. Therefore a reached Controls/Multiplayer
hit prevents that fallback, even when map is open. Normal window-message
admission remains separate from these reached-call facts.

Negative delta calls the down/next-row handler; positive calls up/previous:
**Controls0x97e2a→0x276ea0 /0x97e41→0x276f10**;
**Multiplayer0x97e85→0x27b360 /0x97e9c→0x27b470**.
Each selected handler changes at most one row, irrespective of any
direct caller's larger delta magnitude. Delta0 calls neither handler.

This does **not** add controller right-Y wheel eligibility for these two
panels. Successful-poll0x1355a0 retains its independently specified
three-inventory-grid wheel admission. Do not extend that pad repeater
merely because the later physical dispatcher has more fallback branches.

## 2. Controls row offset and clamp

Controls constructor **0x275a70** initializes signed32 first-row offset
**widget+0x2d8=0**, hover index+0x2dc=−1 and waiting index+0x2e0=−1.
The row offset addresses the fixed **23 binding rows**, not a filtered
list of assigned keys. These scroll handlers do not reorder bindings,
set a different waiting row, or immediately recompute the draw-produced
hover index. Normal draw0x276200 recomputes hover separately.

For down **0x276ea0**, obtain the current local effective frame height
H through **0x329e00→0x32a110**. Subtract float32 **150**, then float32
**35**, truncate that float32 result toward zero to signed32, and divide
that integer by **30** toward zero. Call that quotient q.
Increase the signed32 offset by1. If the increased offset is **at least
23−q**, replace it with **22−q**. Then tail-call **0x276f40** to update
the thumb. This is not a common max(0,last-row) clamp: the down handler
does not add a lower clamp or a special fully-fitting-list reset.

For up **0x276f10**, decrease signed32 offset by1, clamp a negative result
to0, then tail-call0x276f40. It does not recompute an upper bound. Both
paths update the thumb even if clamping leaves the old offset unchanged.
These are32-bit offset operations, not float scroll positions.

The geometry constants are **150.0f at0x42e938**, **35.0f at0x441bc0**,
and30/23/22 are integer constants in the code. H is not a physical-pixel
height multiplied by GUI scale. Do not substitute draw's separate
visible-row calculation or the count of nonempty bindings for q.

## 3. Multiplayer friend-list offset and clamp

Constructor **0x279db0** initializes signed32 first-row offset
**widget+0x2e8=0**, hover index+0x2ec=−1 and action-side byte+0x2f0=0.
The stored friend vector is **begin+0x1f8, end+0x200**, with **40-byte
ID/name records**. Its current length N is the pointer difference divided
by40. It includes valid stored friends whose current game fails the
checkGame interaction predicate; do not compact to eligible friends.
See steam-chat-feat-presentation §6 for that separate predicate.

For down **0x27b360**, read local effective height H, subtract float32
**150**, then float32 **60**, truncate to signed32, and divide by integer
30 toward zero to obtain q. The native comparison treats N as unsigned64
and converts q by signed32→signed64→unsigned64. If **N is at most that
converted q**, set offset0 and **return without calling the thumb updater**.

Otherwise increase signed32 offset by1. Compare its sign-extended value
as unsigned64 against **N−sign-extended(q)−1**, using native64-bit
unsigned arithmetic. If the offset is at least that maximum, store the
low signed32 result **N−q−1**. Then clamp a negative signed32 offset
to0 and tail-call **0x27b490**. For ordinary nonnegative q and finite
native friend lists, this is one row down, capped atN−q−1; the early
fitting-list return is still distinct and must not gain a thumb refresh.

For up **0x27b470**, decrease signed32 offset by1, clamp a negative result
to0, and always tail-call0x27b490. It performs no list-fit test or upper
clamp. Neither handler clears the current draw-produced hover/action-side
fields, invokes friend-list enumeration, toggles checkGame, invites a
friend, or changes an invitation. Ordinary draw0x27a520 refreshes its
hover fields later.

The60.0f constant is **0x42e7b8**. As with Controls, this uses local
effective frame height and integer rows, not physical-pixel wheel travel.

## 4. What the requested refresh actually updates

The scroll helpers call **thumb geometry updates**, not collection
refreshes. Neither0x97cc0 nor these four scroll methods/thumb helpers
issues a sound request. A later independent pointer/button action must
not be represented as an extra wheel scroll sound.

For Controls **0x276f40**, return immediately if thumb-node pointer
**widget+0x2d0** is null. Otherwise recompute D by the float32
**(H−150)−35**, then trunc32; q is signed D/30 toward zero.
The thumb height is signed integer **q times D, divided by23 toward
zero**, with the product evaluated in32 bits. Its Y is signed integer
**35 plus ((D−thumbHeight) times rowOffset)/(22−q)**, using32-bit
product and signed integer division toward zero before float conversion.
Pass X as float32 **current effective panel width−28**, preserve the
thumb Widget's current effective width, convert the integer Y/height
to float32, and set that rectangle through **0x32c680 at0x277027**
with final size-propagation flag1. There is no denominator-zero guard or generic
small-height/full-fit clamp in this helper.

For Multiplayer **0x27b490**, return if thumb-node pointer
**widget+0x2e0** is null. Recompute signed D by trunc32 of float32
**(H−150)−60**, and signed q=D/30. Compare N against the sign-extended
**signed32 q+1** converted to unsigned64. When **N is at most that
value**, request X=panelWidth−28, Y=60, preserved thumb effective width,
and height=float32(D), through0x32c680 at0x27b575. This branch itself
does not alter rowOffset.

Otherwise compute thumb height from **q times D** with a32-bit product,
sign-extend the product, reinterpret it as unsigned64, and divide
unsigned by N. The stored height used afterward is the quotient's
low32 bits, converted as signed32 to float32. For Y, form32-bit
**(D−that integer height) times rowOffset**, sign-extend/reinterpret
unsigned64, divide unsigned by **N−sign-extended(q)−1**, and add60 in
64 bits. Convert that unsigned64 position to float32 using the native
signed conversion plus float2^64 correction when its high bit is set.
The normal nonnegative case is60 plus the integer track displacement.
Set X=panelWidth−28, preserve thumb effective width, and pass the new
Y/height to **0x32c680 at0x27b666**, size-propagation1. These distinctions record
the actual signedness; they do not introduce a replacement safe clamp.

Data **0x46e620 is28.0f**. The frame setters operate on the thumb Widget
at its node+0x70 and retain ordinary binding/minimum-size behavior; the
requested rectangle is not a pixel-space scissor rectangle.

As an arithmetic reference, supplied Controls H600 gives D415, q13,
maximum down offset9 and requested thumb height234; at offset9 its
requested Y is216. Supplied Multiplayer H600 with N30 gives D390,
q13, maximum down offset16 and requested thumb height169; at offset16
its requested Y is281. These are formula evaluations, not runtime captures.

Friend-list rebuild **0xb33f0** also calls0x27b490 after rebuilding
the vector, and size/layout hook **0x27b230** calls it. That rebuild
does not reset/clamp+0x2e8 before the thumb call. Controls layout hook
**0x276cd0** similarly calls0x276f40. Do not silently turn a thumb
refresh into a row-offset normalization or a fresh Steam query.

## 5. Scope

This supplement closes the two physical-wheel fallback consumers and
their directly requested refreshes. It does not replace owner thumb/
button artwork, the wider Multiplayer presentation, ordinary binding
capture semantics, or the earlier Inventory/Shop/Crafting wheel rules.
No additional pad action, key binding or list compaction is inferred.

## 6. Controls opening, Defaults/OK/Cancel and refresh lifetime

### 6.1 First construction versus reopening the same Widget

Controls constructor **0x275a70** initializes the binding array through
**0x41dd0 at0x275b3d** and writes signed32 first row **+0x2d8=0 at
0x275b63**.0x41dd0 zeroes23 eight-byte binding records and populates
the defaults through0x43760. This array **+0x1f0..+0x2a7** is separate
from the Game's active bindings and from the later row/hover/wait fields.
The constructor does not read controls.cfg into this local array.

The ordinary System menu's Controls choice **0x292300, index1**, first
hides System's root, then tail-calls **Interface0x102ed0 at0x29235f**.
The complete toggle body **0x102ed0–0x102f02** only changes Controls
root own visibility, **Interface+0x540 = Game+0x658**, from hidden to
shown or vice versa. It performs **no row-offset reset, active-binding
copy, data replacement, thumb refresh, or root-size request**.
Thus reopening this same Widget retains its first row. A fresh Widget
construction is a separate zero-initialization event.

### 6.2 The three footer buttons

Constructor0x275a70 registers all three for **event3, mouse release**.
Their effects on the retained row are:

|Action / native callback|Binding/visibility effect|First row and thumb|
|---|---|---|
|Defaults,0x276a30|Construct a fresh184-byte default binding block through0x41dd0 and copy it into widget+0x1f0..+0x2a7; keep the panel shown|Retain+0x2d8; no0x276f40 call|
|OK,0x276c80|Pass local bindings to Game0x89b80, which replaces active bindings and writes controls.cfg through0x438e0; then hide Controls root|Retain+0x2d8; no0x276f40 call|
|Cancel,0x276a00–0x276a20|Only hide Controls root|Retain+0x2d8 and local binding edits; no0x276f40 call|

The Defaults copy ends before the row field. Cancel does not restore the
previous local array, and the verified opening route does not overwrite
it from active Game bindings. Consequently uncommitted local edits can
still be present on reopening, while Cancel itself did not apply them
to the Game. Do not add a modern rollback/reload policy to this path.
The independent binding-wait clear in ordinary input/release handling
remains its existing contract; it is not a first-row reset.

### 6.3 Thumb updates independent of row-step input

The verified direct caller of **0x276f40** outside up/down is the
Controls **size/layout hook0x276cd0**, which calls it at **0x276df7**.
Controls' native vtable0x46e468 assigns that hook to **slot+0x50**.
The hook arranges the footer controls, updates the thumb using the
existing first row, then places up/down controls. It does not zero or
otherwise normalize+0x2d8. The updater still returns when its thumb
node+0x2d0 is null, as§4 specifies.

There is an initial refresh before first display: after constructing
Controls, **Interface0xf9ac0 at0xfca81** requests **500×600**, size-propagation1,
through0x32c9a0. That setter calls0x32c6f0, whose final own virtual+0x50
dispatch reaches0x276cd0, and therefore the thumb update. The interface
then hides the Controls root. This is a size/setup event, not an implicit
first-open refresh.

Later reached size requests through0x32c6f0 likewise invoke that own
layout hook, **even if the requested size does not produce a new effective
size**; the final hook dispatch is outside its size-delta branch. It has
no root-shown condition. A hidden Widget can therefore have its thumb
recomputed by actual size/layout work. Mere visibility toggle and the
examined Defaults/OK/Cancel bodies do not perform that work.

Ordinary draw **0x276200** does not call0x276f40 and does not write back
its clamped visible first/end indices as a new stored first row. It is
not a per-frame thumb refresh or automatic row normalization. Binding
assignment **0x2769a0** changes binding records/hover state, not the row
offset or thumb. Thumb drag **0x276af0** has its own held-pointer path
that assigns+0x2d8 and directly repositions the thumb; it does not call
0x276f40 to recompute thumb height. Its full pointer geometry is outside
that opening/footer subsection; the verified drag contract is now§7.

## 7. Controls thumb drag0x276af0

### 7.1 Registration, held state and capture routing

Constructor **0x275a70** makes an independent copy of the supplied owner
thumb node, stores it at **Controls+0x2d0**, and registers callback
**0x276af0 for event12** through0x2757b0. It matches the own Widget name
**scrollbutton** in that copied subtree, with recursive Widget targets
through0x2758b0/0x32ac80 and enabled-required1. The callback's target
object is the containing ControlsWidget, not the particular hit child.
The constructor preserves copied Widget dynamic types/settings; it
does not install a new hardcoded slider class or capture policy.

The callback itself has exactly one initial input gate: **GUI engine
pointer Controls+8, held-button word+0x1a8 bit0 must be set**. This is
the GUI's logical left-held state, including any earlier synthesized
button input; it is not a fresh physical-button query. If clear, return
without geometry or row changes. It does not test its own capture node,
a pressed-origin flag, minimum movement, pointer containment, current
hover index, binding waiting index, panel visibility or elapsed time.

Ordinary dispatch **0x3418d0** first saves old GUI XY into+0x188/+0x18c,
then stores new float XY at+0x180/+0x184. It routes to **captured node
engine+0x168 when nonnull**, otherwise hit-tests the GUI root with flags0.
After owner enter/leave processing, the target's enabled virtual move
hook+a8 runs **before registered event12**. That virtual hook can have
its own data-driven behavior; do not move this custom callback ahead of
it. Registered dispatch requires the hit/captured node's passive bit0x02
clear and the owner Widget enabled for this enabled-required callback.
With capture, the dispatcher does not require a new root/ancestor hit.

The verified ordinary **Button** route is: left down0x340ec0 sets the
held bit, resolves capture-or-hit and invokes the enabled owner's virtual
+0x58; Button **0x36a140** calls **0x33c410** to capture its own root node.
The setter simply stores that node at engine+0x168. This makes later
movement continue outside that root's hit region. On left release,
**0x3410e0 clears held bit0 before the release virtual**; Button
**0x36a240→0x3493b0** clears capture and invokes0x3418d0 with unchanged
XY. A reached thumb callback during that release-induced update sees
held0 and makes no final drag step.

This Button behavior applies to the actual copied Button owner, including
a captured descendant carrying the registered callback. Native registration
does not by itself prove an owner's serialized dynamic Widget type from
the word scrollbutton. Read that type/settings from the owner's scene;
the native constructor does not override them. Independently of capture,
a reached matching move callback while left is already held can drag:
there is no requirement that the press started on this thumb. Conversely
a capture routing to another owner without this callback does not trigger
it merely because the cursor crosses the visible thumb.

### 7.2 The three geometry queries have different contracts

Read panel local effective height **H through0x329e00**. This is the
same local-height input used by the row-step handlers. Separately obtain
the thumb Widget at **(Controls+0x2d0)->+0x70**:

* Thumb height comes from **0x32ad10→0x32d1d0**, then trunc32 toward
  zero. This is the existing Q-transformed effective-size vector's Y,
  not0x329e00 on the thumb, an absolute value, or an occupied-artwork AABB.
  Preserve the parent-relative/root-cancelled Q contract in
  plasma-format-and-scene §2.7.2; it does not apply a generic inverse
  parent-world matrix.
* Thumb position comes from **0x329f50**, call0x276b6a. This is distinct
  from0x32c340. Starting at its node's parent, find the nearest ancestor
  with an own Widget. Compose the thumb's local node matrix with its
  bind inverse **Widget+0x110**, precomposing intervening ancestors
  without Widgets, as0x329e40. Project the thumb's stored frame position
  **+0x88/+0x8c** through that matrix with a homogeneous reciprocal.
  Separately project the ancestor Widget's frame position through that
  ancestor's own bind inverse, also with a homogeneous reciprocal, then
  subtract the two XY results. The ancestor's node/world matrix is not
  appended here. If no such Widget ancestor exists,0x329f50 instead
  returns the ordinary world-frame origin0x32cf50. Do not substitute
  a raw translation channel or generic screen rectangle origin.
* Pointer displacement comes from **0x32b0d0 on the ControlsWidget**,
  call0x276b7a. Subtract old from new GUI XY in float32. Invert that
  panel's full frame matrix **0x32ce40**, and apply its XY linear part
  to the screen delta as a direction, with Z0. There is **no translation
  or homogeneous divide on the direction**, time factor, or separately
  guessed GUI-scale division. The frame matrix includes the actual
  inherited node transforms, bind inverse and frame-position translation.

Use these actual helpers/bases together. The delta is not the absolute
pointer Y, and the thumb position/height are not interchangeable local
or screen queries simply because the stock pose may look axis-aligned.

### 7.3 Float order, clamps, row write and continuous thumb movement

Subtract float32 **150** from H, then float32 **35**, and truncate that
float result to signed32 D. Let signed q be **D divided by30 toward zero**.
Let signed travel K be **D minus trunc32(queried thumb height)**, in32-bit
arithmetic. Candidate Y is float32 **pointer displacement Y plus queried
thumb position Y**. For finite inputs, clamp candidate first upward to
float35, then downward to **float32(signed32 K+35)**. The upper endpoint
is formed by the integer addition before conversion. Call the resulting
float Y. This order is retained even when K is zero or negative.

Only when **K>0**, replace row+0x2d8 by trunc32 of the float32 result
obtained in this order: convert signed **22−q** to float; subtract
float35 from Y; multiply those float values; divide by float32(K).
Every operation has its float32 result. If K<=0, skip this assignment
and start the following clamps with the previously stored row.

Then clamp a negative stored row to0; afterward, if row is **at least
23−q**, store **22−q**. There is no additional final lower clamp after
that upper write. Do not replace this with a different all-sizes clamp.

After the row work, **0x276c3d** queries0x329f50 again. Preserve that
current X and move the thumb to **(that X,Y)** through
**0x32b560 at0x276c57**, final flag1. That is the ordinary frame-placement
solver0x32b250, not a raw translation-column write. It does not call
0x276f40, change the thumb's requested size, or snap Y back to the
integer-row thumb formula. The continuous Y can lie between two
row-quantized scrollbar positions.

There is no nonzero-delta gate. A stationary held move event still
performs the queries, ratio/truncation, clamps and placement. As an
arithmetic example with supplied H600, thumb queried height234, current
Y55 and delta0: D415, q13, K181, and the ratio9×20/181 truncates to
row0 even if the old stored row was1; the thumb remains atY55. This
is an evaluation of the contract, not a runtime capture. Do not add
a no-coordinate-change early return or nearest-row rounding.

### 7.4 Input/draw timing

The handler is an event12 move callback, **not a draw-time update**.
For the normal input loop, the verified pointer poll is
**0x1355a0→0x97880 at0x13683e→0x101160→0x3418d0**. It runs after
queued window messages, before that iteration's XInput work and ordinary
frame driver0x134420. WM_MOUSEMOVE does not independently invoke it.
Unchanged reached XY still dispatches the move callback. Foreground/
enabled admission and binding-capture waiting can skip that normal poll;
see steam-character-list-input-and-frame-init §7 for the exact cadence.
Other reached0x3418d0 dispatches retain their own caller phase.

Consequently the drag's row/placement writes precede the later admitted
Controls draw, which reads the new row. Drawing does not repair or
recompute the thumb. A subsequent independent size/layout event can
recompute it through§6.3, but that is not an automatic part of dragging.
No list rebuild, binding change, sound, time-based velocity or page-jump
track click is introduced by this callback.

## 8. Startup controls.cfg versus the local Controls editor array

The verified Game constructor **0x7ea80** has this order:

1. **0x7ed12→0x41dd0** initializes the **active Game bindings at+0x1bdc**
   with defaults.
2. **0x7f267→0x42b30**, with that active array address, reads
   controls.cfg. The reader's recognized binding writes address its
   supplied array; it is not given a ControlsWidget pointer.
3. **0x7f4d4→0xf9ac0** constructs the interface, including Controls
   **0xfca5e→0x275a70**. Its own separate editor array **+0x1f0..+0x2a7**
   is initialized by0x275b3d→0x41dd0 with defaults, after the active
   configuration load has already happened.
4. After interface construction, **0x7f4ef→0x89b80** receives the Game
   and **the same active Game+0x1bdc array** as source. Its writes go
   back to Game+0x1bdc..+0x1c93, then it tail-calls the controls.cfg
   writer **0x438e0 at0x89bfd**. This is an active-array self-apply/save,
   not a copy into widget+0x1f0.0x438e0 serializes the supplied binding
   array and does not synchronize an editor Widget.

Thus the identified startup path **does not assign loaded active values
to the local Controls editor after construction**. Loaded active bindings
and the initially default local editor are distinct native states. The
earlier opening/Cancel retention facts are not authorization for an
implicit active→local synchronization on first open.

The verified local-editor writers in this bounded audit are:

|Writer|Local array effect|
|---|---|
|Constructor0x275b3d→0x41dd0/0x43760|Initialize all23 records to defaults/unassigned values|
|Defaults0x276a30|Replace all184 bytes with a freshly initialized default block|
|Captured binding0x2769a0|For a waiting index whose unsigned value is<23, clear any matching kind/code pairs across23 rows, then write the captured eight-byte pair into the selected local row; clear hover index+0x2dc|

OK reads this local array to replace active bindings through0x89b80;
Cancel/open/ordinary draw/scroll/drag do not replace the local array.
These facts classify the verified startup and editor routes, not every
conceivable indirect memory writer or an unexecuted runtime capture.

## 9. Setter flag clarification: no Widget replacement

The earlier shorthand **“rebound1” means size-propagation1 where attached
to0x32c680/0x32c9a0**. It does not mean reconstructing/rebinding a Widget
or replacing a ScrollButton with an ordinary Button. Exact setter
semantics are now explicit in [Plasma §2.7.3](plasma-format-and-scene.md).

For the reached **Controls thumb update0x276f40→0x32c680 at0x277027**,
the existing thumb node's own Widget pointer is the receiver. The native
rectangle helper first moves that same Widget with position flag0, then
sizes it with the caller's trailing size-propagation flag1. The final byte
does **not** also become the position call's byte. The saved Widget pointer
is reused for both calls, with no allocation, constructor, clone or
dynamic-class/handler replacement between them; native instructions
0x32c680–0x32c6d1 settle the abbreviated decompile's argument ambiguity.

The **Controls layout0x276cd0** positions its existing footer/arrow Widgets
through0x32b560 with **position flag1**. That flag retains the live
transformation's changed indication after moving; it is not the size
setter's descendant-propagation flag. The constructor's500×600 request
uses0x32c9a0 and therefore its flag1 has the **size** meaning.

Size flag0 still updates the requested frame size and executes the
receiver's own virtual+0x50 layout hook. Size flag1 additionally permits
delivery of resize placement to different own Widgets reached at the
propagation frontier. These callbacks may perform their native layouts,
including0x276cd0 and its thumb update, but the setters do not discard
the dynamic class, owned sequence strings or registered input callbacks.
