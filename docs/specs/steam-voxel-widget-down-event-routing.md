# Steam VoxelWidget: left-down admission and shared completion

Independent native-only supplement to
[weapon-customization edit-view sections2/9](steam-weapon-customization-edit-view.md)
and [presentation/input](steam-voxel-widget-presentation-input.md).
The audited handler is **0x94900**, with the actual raw edit call at
**0x9651a→0x292650**. Sources are the Steam executable/decompile and
independent prose; no engine, tests, exporter/build, SDK, owner assets or
captures were inspected, and no game was executed. All addresses are RVAs.
G denotes Game. This describes ordinary successful event execution, not
allocation/list-length exception handling.

## 1. GUI down callbacks precede Game's item/editor decisions

Physical **WM_LBUTTONDOWN0x201** reaches **0x133c70**: first SetCapture
for the application window, then **0x94900 with Game and button0**. That message
branch does not test title, System, Options, Controls, chat editing or a
GUI-captured-node result before calling the Game handler. Native right
and middle messages use the same handler with buttons1/2. Controller
synthesis also calls it, subject to its separately specified routing,
binding-capture and edge/time admission; this document does not bypass
those caller gates or turn a logical Normal Attack binding into a GUI click.

At **0x94953**, before the following Game business decisions, the handler
calls **0x101020** with Interface G+0x118 and the button. For left down,
that helper records current GUI cursor Y in Interface+0x548 and invokes
GUI **0x340ec0**. This sets GUI engine **+0x1a8 bit0** before resolving
the captured node or tree hit, press/drag bookkeeping, focus changes,
Widget down callbacks and registered down events.

These GUI operations run synchronously **before** the handler's
identification snapshot, map checks, inventory resolver, crafting-preview
updates and VoxelWidget check. A Button, text editor, another Widget or a
captured node receiving the GUI event does **not** supply a handled/consumed
result that makes0x94900 return.0x101020's left case returns from the
interface helper, after which the Game handler continues. The physical
application-window capture and GUI engine's captured-node pointer are
different states; neither is an automatic VoxelWidget-tail veto.

The GUI down dispatcher can itself decline a callback because of the
ordinary visible/enabled/hit/event conditions. That does not terminate
the caller's Game-side continuation either. A callback's actual mutations
are observed by subsequent Game tests; do not use a pre-callback panel,
focus or item-state snapshot to choose their branches.

## 2. Actual early map returns, and absent broad modal guards

Immediately after GUI down processing, **0x94958–0x9497c** records whether
Identification G+0x360's root is currently shown into byte **0x5514d7**.
That byte is for the separately specified later release admission. It is
not a return or consumed flag for this down event.

The normal button0 handler has two explicit early map-hit returns before
its ordinary item/crafting/editor continuation:

* Require full-map byte **G+0x668!=0**. Test the designated GUI subtree
  **G+0x3a8** through **0x333e70 at0x94993**. A true result returns via
  **0x9499a→0x965ac**.
* If the first misses, test **G+0x3b0** through **0x333e70 at0x949a7**.
  A true result returns via **0x949ae→0x965ac**.

Helper0x333e70 copies the current GUI cursor floats from engine
+0x180/+0x184 and runs **0x333f60 on the supplied subtree with flags0**.
It reports whether that subtree query found a node. This is not the
global0x33f1e0 capture-or-hit query and not a test that the whole full map
is shown or hit. The ordinary owner shape/visibility/clipping hit rules
remain inputs to the two subtree queries. A full map with both misses
does **not** itself exclude the rest of this handler.

The subsequent map target/flight action and map multi-click behavior
remain their separate established contracts. Their ordinary success,
failure or no-action branches continue. The reached map-drag reset is
**0x94e7a**; the two earlier subtree-hit returns skip it. Some map
business can clear map/menu state or change the player's action before
item/editor processing, but it does not add a general map-visible return.

There is **no additional title/root-menu, character-selection/creation,
System G+0x358, Options G+0x650, Controls G+0x658, chat-active, GUI-focus,
global hit/capture, or ordinary-UI0x1014f0 exclusion** of the reached
button0 continuation in0x94900. Their own upstream input routes and
GUI/release actions remain separate. In particular, merely drawing an
overlapping menu or receiving a Button down callback does not make this
Game-side event exclusive. This does not assert that every combination
of those roots is normally opened together.

The remaining button distinction is explicit: middle button2 takes its
drop route and then joins **0x9651f**, bypassing crafting and VoxelWidget;
right button1 can perform item/equipment/shop work but skips both at
**0x9622b–0x9622e**. The editor continuation below is for **button0**.

## 3. Ordinary item routes rejoin, rather than consume the down event

For button0/1 the canonical-slot resolver **0x94600 at0x95013** runs
first. It requires Inventory G+0x330 shown, its current frame hit through
0x32b0c0, a current valid hover tab/slot pair and source quantity!=0.
It uses the stored grid hover pair, not a fresh cell pick in this event.
The exact canonical transactions remain slot-lifecycle6/7. Their
precedence and continuation relevant to the editor are:

|Resolved route|Decision/precedence|Continuation toward crafting/editor|
|---|---|---|
|Canonical slot + Identification shown|Identification is tested first at0x95033. Copy the live tab/slot pair into its +0x1e8/+0x1ec at0x95060–0x95074.|Join0x9622b at0x95076; no Identify transaction or return here.|
|Canonical slot + Shop shown|After Identification is excluded, Shop is tested at0x9507b. Its sale branch wins over Upgrade and ordinary equip/use; forbidden source tabs1/5 request their error sound.|Both sale and rejection reach0x9622b.|
|Canonical slot + Upgrade G+0x370 shown|After Shop is excluded, the test begins0x95263. Categories3..9 or26 bind that live item pointer into Upgrade+0x1e8; other categories request sound0x34.|Pointer binding joins at0x952ec; rejection through0x95911 also joins0x9622b. This is not the separate G+0x368 editor selection.|
|Ordinary canonical equip/use|With those three shown-panel interceptions excluded, the class/usability test is0x50640 at0x95315. Failed usability requests0x35; a now-zero source skips the item action. Weapon/armor/accessory equips, direct quick-item use and categories without a direct action retain their documented rules.|Success, rejection and no item-specific effect all reach0x9622b. There is no successful-equip return before the editor.|
|No canonical slot: equipment under pointer|Resolve through0x94260 at0x95c1d. It requires Inventory shown and tests its equipment SpriteWidget subtrees. A nonempty result takes equipment sale when Shop is shown; otherwise Upgrade binding/rejection when Upgrade is shown; otherwise ordinary unequip/return.|The ordinary exits are0x95d21,0x95d72 or0x95dcd to0x9622b. Identification does not intercept this equipment fallback as it intercepts a canonical slot.|
|Neither canonical nor nonempty equipment: shop offer|Resolve through0x94750 at0x95dd5. Require Shop shown, its frame hit and current valid offer hover pair. No offer simply joins. A regular purchase or buyback follows its documented source/quantity/currency/carry rules.|No offer, rejected purchase, successful purchase and buyback completion all reach0x9622b; their normal sound/refresh/finalization is not an event-consumed return.|

Thus an earlier item action can change the live equipped weapon,
canonical vectors, visibility or game state **before** the editor checks
the currently selected equipped address. It does not preserve an old
weapon pointer for the later edit. Conversely a visible panel with no
matching item route simply leaves that business stage without excluding
the editor. Existing per-action refresh/sort requirements still occur at
their own earlier call sites; this is not a new generic refresh policy.

## 4. Crafting preview is also before the VoxelWidget decision

The shared join at **0x9622b** admits only button0 to this part. If
Crafting G+0x340 is shown, call **0x280e30 with copy-hover true at0x96260**.
The caller does not add a current crafting-frame/global-hit/capture test
around that call. The helper has its ordinary collection/hover gates;
an absent hover can leave the prior selected pair intact and return
from that helper without terminating0x94900.

The caller then checks the resulting selected tab/slot against the
current crafting collection at **G+0x17a8**. Invalid bounds clear the
formula-detail result at **(G+0x348)+0x1f8** and write its associated pair
**+0x2b8/+0x2bc** to−1. Valid bounds call **0x2aec30 at0x9634b** with
the selected recipe item and its tab6 flag. Recipe failure likewise
clears the detail result and pair; success copies the result through
**0x834f0 at0x96366** and stores the selected pair. Temporary results
are cleaned up normally. This is the recipe-preview update, not a new
craft transaction or cursor grab.

Crafting hidden, no hover, invalid selected pair, recipe failure and
recipe success all continue to the same editor check at **0x964a5**.
There is no successful-crafting-selection early return.

## 5. Exact editor choice at the reached common left-down tail

At **0x964a5–0x964c9**, require the VoxelWidget root's current shown
property to be nonzero. At **0x964cb–0x964d3**, require **0x294cc0** to
resolve a current nonempty selected equipped record. These tests happen
after the preceding GUI callbacks/item/crafting work, not on entry.

Then use the stored draw-produced fields in priority order:

1. **Hovered palette index+0x2d8>=0**: invoke **0x294c70 at0x964f3**.
2. Otherwise **hovered equipped thumbnail index+0x2c4>=0**: invoke
   **0x294ca0 at0x96503**.
3. Otherwise resolve the selected equipped record again through0x294cc0
   at0x9650a and invoke **0x292650 at0x9651a**.

The existing edit-view contract specifies those helpers and the cached
pick/carry state. This tail adds no fresh ray pick, shape/rectangle hit,
captured-node exclusion, ordinary-UI predicate, current physical pointer
movement, or matching-down/release latch. The raw helper's own no-hit /
GUI-null branch remains separate. A callback changing visibility or an
earlier item action emptying the selected equipment can make the explicit
editor tests fail; that is different from treating the GUI event as consumed.

## 6. Post-editor health fraction, chat and selector completion

Before the item routes, **0x5fac0 at0x94e8c** saves the current local
player's **float32 maximum health**, after GUI callbacks and the earlier
map handling. After crafting/editor processing, **0x9651f** is the common
completion also reached when the editor is hidden, has no selected
weapon, or took its palette/thumbnail helper instead of raw editing.

At **0x9652a**, query the current maximum again. Perform **float32 division
of new maximum by saved old maximum**, then **float32 multiplication by
the player's current float32 health+0x180**, and store the result at
**0x9653a–0x96546**. This preserves health fraction for ordinary positive
finite maxima; it does not add the maximum-health delta, fill health to
maximum, or merely refresh a visual widget. Query maximum a second time
at **0x96559** and cap the stored health to that value if the maximum is
smaller, at **0x96569–0x96579**. There is no lower-zero clamp or special
old-maximum-zero branch in this handler. The same completion follows a
cube edit even if that edit changed maximum-health-affecting attributes.

Then clear ChatWidget **(G+0x398)+0x1d8** at **0x96588** and hide
**Ring selector G+0x310** at **0x965a8**. This is not the ordinary
InventoryWidget G+0x330. The completion does not clear the VoxelWidget's
carry/palette selection, newly select a canonical slot, update a tooltip
or rebuild the formula-detail result again after the edit.

There is **no inventory refresh/thumb call, canonical sort, crafting
rebuild or inventory-cursor putback added after the VoxelWidget helper**.
Cube inventory consumption remains the unsorted first-match route in
the palette contract. The earlier crafting-preview stage above remains
before editing; later per-frame/draw detail production remains separate.
The two early map-subtree returns in section2 skip this common health /
chat / Ring completion as well as skipping the editor.

## 7. Application, pointer-poll and frame timing

The physical button message uses the cached GUI cursor fields. Its
0x133c70 branch does not import WM_LBUTTONDOWN's coordinate argument or
invoke0x97880 before0x94900. In the ordinary application loop **0x1355a0**,
queued messages are dispatched first. The admitted normal input route
then polls devices and calls pointer processing **0x97880 at0x13683e**,
before successful XInput processing and its possible synthesized Game
down/up calls. See character-list-input-and-frame-init section7 for
the pointer-poll admission. Thus physical down dispatch and synthetic
controller down dispatch are not interchangeable placements of a new
per-frame pointer poll.

After those input stages, **0x134420** calls the Game frame **0xa5a50**.
The frame's movement-close and subsequent interface update **0x103200
at0xab5ce** are later than a normal down callback in that application
iteration. Rendering0x292990 supplies the thumbnail/palette hover and
raw picked tuple used by a subsequent click;0x94900 does not force a
synchronous new draw. Preserve the actual current fields and callback
ordering rather than generating a new pick from the button message or
retroactively applying a later frame's visibility/input decision.

The above is a stable bounded audit of this down continuation and its
normal callers. It does not replace the separate frontend/identification/
answer/map **release** contracts, controller synthesis admission, or the
ordinary native owner-tree hit rules.
