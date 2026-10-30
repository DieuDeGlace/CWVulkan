# Steam category1/subtype13 map-selection continuation

Independent behavioral prose from the native Steam executable, decompile,
assembly and constants. Addresses are RVAs relative to0x140000000.
No engine, tests, exporter, build files, owner captures, SDK or
reconstructions were inspected, and no original game was executed.

This closes the map continuation left open in
steam-inventory-slot-lifecycle §7.6. It distinguishes the actual queued
interaction from the item subtype and does not infer an item cost,
travel packet or cursor operation from a name.

## 1. Opening selection and the retained slot pointer

The admitted item-use producer **0xb7740**, for category1/subtype13,
sets the two bytes **Game+0x668/+0x669 to1**, appends the already specified
**kind1** item interaction to the local creature's interaction list, then
stores the supplied live canonical slot pointer in **Game+0x21c0** at
**0xb79d4**. It does not decrement that slot, copy the item to the
retained cursor, or copy it to active-item field+0x1d8 in this branch.
Its stun and source-slot admission remain slot-lifecycle §7.6.
The item producer itself requests no direct sound for this subtype.

The number13 here is the **item subtype**, not interaction kind13.
The shared interaction dispatcher **0x2c3710**, kind1, acts on a
category1 item only when its subtype is9, creating the bomb creature.
The category1/subtype13 kind1 request has no action in that branch.
It still follows the normal interaction delivery/list-lifetime contract;
do not fabricate an additional host action or subtype13 award from the
otherwise retained request.

Game+0x668 is full interactive-map state; +0x669 selects the restricted
marker mode called shrine-choice in map-markers. These are separate
bytes. Ordinary map toggle **0x1031e0** changes only+0x668. The creature
use path **0x98180**, race0x10c within its admitted use range, also sets
both bytes1 without requiring or assigning this item-slot pointer.
Thus the marker mode and its acceptance are not dependent on possessing
Game+0x21c0.

Constructor **0x7ea80 at0x7f0d7** initializes+0x21c0 to null. After
interface update **0x103200 at0xab5ce**, the main frame **0xa5a50** tests
the then-current+0x669 at0xab5d3 and clears+0x21c0 at0xab5dd when
that mode byte is0. Interface update itself clears+0x669 at0x1033ca
whenever+0x668 is clear. Closing the map therefore has an ordered
flag-cleanup/pointer-cleanup continuation; the close event does not
itself perform a source-slot decrement or item return.

The identified direct Game-field accesses to+0x21c0 are precisely the
constructor initialization, producer store and conditional frame clear
above. A binary displacement/address-taking audit found **no verified
Game+0x21c0 read that dereferences the saved slot, validates it or consumes
its quantity**. The same displacement at0x175f6b belongs to0x173b10's
item-to-model catalog-vector lookup, not a Game-field consumer; the
other same-displacement matches are local stack/unwind storage.

This is a bounded negative result, not a proof against every indirect
alias. It is sufficient to reject an invented “consume saved slot on
confirmation/cancel” action. The pointer is not an owning reference,
index pair, item-identity reservation or copied stack. Ordinary canonical
insertion, sorting and compaction can move/change its contents or
invalidate its storage while it is retained. No rebind/remap is verified.
A positive missing consumption path would require the first native read/
dereference of the current Game+0x21c0 and its call stack, not merely
observing its non-null store.

## 2. Rendered marker selection is the confirmation input

MapOverlayWidget draw **0x2829c0** resets both WorldMap signed32 target
pairs **+0x150/+0x154** and **+0x15c/+0x160** to INT32_MAX/INT32_MAX at
the start of each actual overlay draw; writes are0x282aa7/0x282ae7.
These are render-produced hover results, not a destination chosen directly
by the item producer. A skipped overlay draw does not execute this reset.

While full-map+0x668 is set and dragged flag Game+0x1db0 is clear,
the existing marker hover pass runs. With+0x669 set it considers only
records whose **flag A** is nonzero. The full hit rectangle, label-width
expansion, strict distance comparisons and sorted traversal are
map-markers §1.7. A winning flag-A record writes its block XY into
WorldMap+0x15c/+0x160, at0x285da7. Current mode also supplies the
already specified pulsing color/scale of those records in §1.8 there.
It does not construct a new independent inventory-derived destination
list or filter these records by the initiating item's own region.

The acceptance consumer **mouse-up0x965e0** uses that stored target pair;
it does not rerun marker picking from the release coordinates. Require:

1. Released button is **0, left**. This is after the ordinary interface
   up dispatch and the preceding identification branch/gates.
2. Game+0x668 is still nonzero and Game+0x1db0 is0.
3. WorldMap target X at+0x15c is not INT32_MAX. The explicit sentinel
   test is on X; normal draw writes/reset both components together.
4. The signed32 target block XY, each divided by16 with **floor division
   including negatives**, is present in the local character's ordered
   cell set at **personal+0xa8**.

No additional+0x669, stored-slot-pointer, item-category/count, price,
stun, ability-finished or confirmation-dialog check appears in this
acceptance branch. In particular an ordinary full map can also expose
this flag-A target/acceptance; +0x669 principally restricts hover/drawing.
If a condition fails, the branch makes no accepted-destination change
and does not clear the map or saved item pointer on its own.

On acceptance, **0x96cf2–0x96d85** sets local ability byte+0x68 to
**0x75**, ability age+0x6c to0, stores the two floored cell coordinates
at player+0x1d0/+0x1d4, then clears Game+0x668. No general DialogWidget
or second Yes/No confirmation is created. No item is consumed, cleared,
returned, or assigned to active-item/cursor storage by this branch.
No direct sound call occurs in it. The ordinary later mouse-up handlers
can still run after the map action.

The reached normal mouse-down path0x94900 clears Game+0x1db0 at0x94e7a.
Under ordinary-UI
mouse movement0x97880, after interface position handling and with the
pad indicator Game+0x1958 clear, any nonzero GUI cursor delta sets that flag at
0x9793b; right-held additionally pans the map. The flag is therefore not
limited to actual right-button panning. A movement-disqualified release
does not silently confirm the previous target. Controller synthesis
retains the existing cursor-snapshot and down/up sequencing contract;
there is no controller-only business action that bypasses these checks.

## 3. Cancel/close routes and item quantity

Concrete close routes include ordinary map toggle **0x1031e0** when
open (WorldMap action15 or its GUI map button), controller B UI dismissal
**0x1355a0 at0x137913**, and menu-return **0x9fdf0**, which explicitly
clears+0x668. These routes do not clear/decrement the initiating canonical
slot or put an item back from the cursor. The mode and pointer then clear
in the interface/frame sequence in §1 when those updates observe the
closed map. A close/reopen before that sequence is not an additional
verified pointer reset event.

Right/middle up are not the left-up target acceptance in §2. Right
movement has the separately specified pan route; middle down has its
existing map marking/recenter behavior in0x94900, rather than an item
confirmation or implicit cancellation. This audit does not assign an
unverified Escape-key confirmation/cancel behavior.

Across the verified producer, mode drawing, accepted release, normal
close/cancel, flag cleanup and local ability branch below, **no eventual
source quantity decrement or item-category/subtype clearing was found**.
There is no verified consume-on-success/refund-on-cancel distinction to
implement for this saved pointer. Other independently admitted inventory
events retain their own effects; the pointer does not freeze a tab slot.

Full-map+0x668 contributes to ordinary-UI predicate0x1014f0. Its sampled
frame sound remains the shared edge in steam-sound-event-addenda §4.1:
predicate0x1014f0 OR G+0x310 shown, false→true requests0x56 and
true→false requests0x57, with the direct call at0xad7e6. No change in
that overall predicate produces no edge sound. Opening marker mode while
another qualifying UI is already sampled open does not add an item-specific
open sound; closing the map while such a UI remains shown does not force
a close sound. This is not a new unconditional sound on each map toggle.

## 4. Ability0x75 continuation and direct event boundary

The exact ability consumer is **0x2c5250, branch0x2c5886**, also
independently specified in creature-abilities-ranged §13.4. It runs this
position change **only for the creature equal to World+0x448**, the
local creature. There is no authoritative-world-only flag test in this
branch: its explicit ownership test is that pointer equality.

Let t be current float32 ability age before the step and dt the ability
step's float32 elapsed milliseconds. T is the integer effect-part
duration0x4ead0(C,−1), converted to float32; for ability0x75 it is
**3000 ms**. Trigger only when **t≤T and T<float32(t+dt)**. This is the
effect boundary, not immediate mouse-up or the total-duration timer.
Ability time advances later through the normal upkeep order.

At that boundary take signed32 cell hx/hy from player+0x1d0/+0x1d4.
Set fixed X to **((hx×16)+6)×65536**, Y to
**((hy×16)+8)×65536**, and initially Z to0. Query integer cell height
**0x2d87f0(World,hx,hy,0)** and set fixed Z to its signed result
times65536. If the creature ID at player+0x9d8 resolves, copy the
player's resulting fixed XYZ to that creature as well. This is a
specified position write with the native cell offsets6/8, not an
arbitrary point at the clicked marker or an inferred flight path.

The16×cell plus6/8 arithmetic is performed in the native32-bit integer
width, then sign-extended to signed64 before the16-bit left shift.
The integer height is likewise sign-extended before that shift. Normal
admitted coordinates fit these intermediates.

That branch reads neither the initiating item nor Game+0x21c0, does not
decrement any inventory quantity, and does not directly append an item
grant/drop or sound request. Generic ability/upkeep effects and sampled
GUI open/close sounds keep their existing independent contracts; no
item-specific teleport sound is inferred from asset names. The cell
height function remains its native procedural-height input here; this
focused pass does not newly rederive its building/style internals.

## 5. Verification boundary

Mode production, render-produced targets, left-up admission, cell-key
conversion, actor fields and delayed position writes, close cleanup,
pointer lifetime and the identified absence of item depletion are
verified from native code/instructions. No game was run. A useful bounded
oracle would label item-use enqueue, map mode/target draw, button/drag
release state, used-cell membership, accepted destination, pointer
store/clear, canonical slot before/after, and the ability effect boundary
separately. An untraced indirect saved-pointer consumer remains explicitly
unverified; no extra consumption or authority packet is supplied for it.
