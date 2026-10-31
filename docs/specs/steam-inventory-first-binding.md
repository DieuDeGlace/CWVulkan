# Steam InventoryWidget first binding and character-load order

Independent native-only addendum to steam-inventory-refresh-events1–3/7.
Sources: Game constructor0x7ea80, Interface constructor0xf9ac0,
update0x103200, character loaders0x9bb20/0x147800, Select0x9f7c0,
Create0x9ea10, label builder0x107690 and Game frame0xa5a50. Native
assembly confirms omitted arguments/order. No engine, tests, exporter/
build, SDK, owner assets or captures were read; no game was executed.

## 1. Startup really has an unbound Widget stage

The constructor order is important:

1. **0x7ea80** initializes the world and reads saved-character records
   into separate preview creatures/list Game+0x1778. The calls of0x9bb20
   for that list do not make those creatures World+0x448.
2. Call **Interface constructor0xf9ac0 at0x7f4d4**. It creates the
   three InventoryWidgets. Each0x27c950 constructor initializes its
   collection+0x1e8 to null. Construction, requested sizing and the root
   initialization **0x333820 at0x100820** happen in this stage. Any
   inventory layout refresh reached there sees that null binding;
   refresh0x280a40 and thumb0x280c80 keep their documented null returns.
  0xf9ac0 does not assign the three collection pointers.
3. Only **after0xf9ac0 returns**, construct the actual live local
   creature through **0x4ce80 at0x7f517**, initialize its default state,
   add it to the world through0x29d0d0 at0x7f64e and store it in
   **World+0x448 at0x7f657**. This is the persistent local P, distinct
   from the saved-preview list. The frontend is already constructed.
4. If the saved last-slot entry is present, load that character into
   this P via **0x9bb20 at0x7f71a**, refresh knowledge, ensure default
   recipe proposals, rebuild crafting labels0x107690, then call
   crafting rebuild **0xb2180 at0x7f74d**. This still precedes the first
   ordinary Interface update and hence the first collection binding.

The early crafting rebuild can populate Game's derived recipe collection
but its final crafting0x280a40/0x280c80 pair sees the still-null Widget
binding. It does not silently bind just because a local creature now
exists. No last-slot entry omits that optional early load/rebuild; it
does not change the following binding producer.

## 2. The first actual binding is an ordinary Interface frame

The native direct call index has one ordinary producer invocation of
**0x103200**, from Game frame **0xa5a50 at0xab5ce**. It is reached while
front screens are shown as well as during gameplay; it is not gated by
inventory/crafting/shop visibility or waiting until a save starts play.

Within that Interface update, the exact writes are:

| Store RVA | Target collection+0x1e8 |
|---|---|
|0x104925|Inventory G+0x330 → current World+0x448 creature's embedded inventory **P+0x9f0**|
|0x104941|Crafting G+0x340 → persistent Game+0x17a8|
|0x10495d|Shop G+0x350 → persistent Game+0x1870|

They are pointer assignments to the collection objects, not copies of
their inner vector buffers. They add **no refresh, thumb call, sort,
row-vector resize, A write, selection reset or first-bind callback**.
Initially null remains a real native state until these stores occur.
Thereafter each reached update assigns these same collection-object
addresses again, unless the current local creature itself has changed.

Normally the displayed title/character screens have already had ordinary
Interface updates before the player enters gameplay. Their bindings
therefore already refer to the persistent P and Game collections; they
are not first created by opening inventory after choosing a character.
This normal frontend ordering is not permission to make an earlier
constructor/refresh/open helper auto-bind on demand.

## 3. Loading into P preserves the bound collection object's address

**0x9bb20** receives its destination creature as argument3. Native
**0x9bb59** retains that pointer; **0x9bcd7–0x9bcda** passes it as
argument2 to reader0x147800. Loading replaces destination state/tabs/
subobjects in place; it does not replace World+0x448 with a saved-preview
creature. The embedded P+0x9f0 collection-object address therefore
survives ordinary selected-save loading even though its inner vectors
and slot-buffer addresses can change. Raw save ordering remains
slot-lifecycle1 and frontend-title6.1.

Existing-character **Select0x9f7c0** loads current P at0x9f856, rebuilds
class-specific crafting labels0x107690, then crafting0xb2180 at0x9f93a.
The label builder ends in **0x280440 on G+0x340**, not0x103200; it does
not bind the collections. Starting the New entry instead resets the
same P for creation; final **Create0x9ea10** saves the
record, rebuilds cards/labels, then calls0xb2180 at0x9efb9. None of
these routes allocates a replacement live P for a panel to bind.

These character transitions have no new universal inventory0x280a40
pair merely because a save's inner collection changed. If a bound
crafting rebuild is admitted, its own refresh still has the established
local canonical sort side effect. Do not merge label creation, derived
collection rebuild and Widget refresh into one operation.

## 4. First open and same-frame order

In an ordinary admitted Game frame, keyboard/action inventory toggle
**0xaaff5** and crafting toggle **0xab07f** precede the Interface update
**0xab5ce**. Thus a reached open uses the binding already stored from a
previous update; the later bind is not moved ahead of it. The normal
toggle block requires title, character-select and creation roots hidden
and chat editing off, as in refresh-events3. By a regular transition
from already drawn frontend screens, those previous updates have run.

If an earlier setup/nonstandard call reaches an open before the first
0x104925/41/5d stores, the helpers still see null and their refresh/thumb
returns are real. The subsequent first assignment supplies no deferred
refresh or catch-up. This audit does not assert that arbitrary manually
forced early visibility is a normal player-accessible startup route;
it specifies the actual event order rather than repairing such a call.

For the normal first shown inventory after loading, its opening helper
copies current player region then runs its explicit refresh/thumb pair.
Crafting open supplies its rebuild and shown-branch pair exactly as
refresh-events3. Keep initial null setup, frontend-frame binding, in-place
save load and eventual explicit open refresh as distinct stages. Broader
unused world-select UI/multiplayer replacement lifecycles are not newly
proved by this bounded startup/character contract.
