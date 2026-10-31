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
with rebound flag1. There is no denominator-zero guard or generic
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
Y/height to **0x32c680 at0x27b666**, rebound1. These distinctions record
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
