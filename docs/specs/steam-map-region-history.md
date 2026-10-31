# Steam map region-history label and navigation

Independent native-only behavioral supplement. Sources are the Steam
executable/decompile and existing independent prose; no engine, tests,
exporter/build, SDK, reconstruction, owner assets/captures or game
execution were used. Addresses are RVAs. Interface I is the embedded
controller **Game G+0x118**. This extends
[controller handoff §2](steam-controller-handoff.md), distinguishing
history storage from flat marker hover and the map's separate camera.

## 1. Native roots and construction

| Field | Meaning |
|---|---|
|I+0x108 =G+0x220|Ordinary game-UI root, parent of this label|
|I+0x288 =G+0x3a0|Generated outlined text node, named `recentlands`|
|I+0x290 =G+0x3a8|Copied `leftbutton` subtree, older history|
|I+0x298 =G+0x3b0|Copied `rightbutton` subtree, newer history|
|I+0x550 =G+0x668|Interactive/full-map-open byte|
|I+0x54c =G+0x664|Current GUI scale|
|I+0x5c8 =G+0x6e0|Selected history **node pointer**, not pair/index|

Interface constructor **0xf8690** initializes the selected pointer to
null and map-open byte0. Construction **0xf9ac0 at0xfdd50** calls
**0x1008a0** with starter text **`land`**, node name **`recentlands`**,
parent ordinary game-UI root, flags0, font size16 and outline radius3.
`land` is text, **not an owner panel-template name**. This creates two
TextShapes using **resource1.dat**, line spacing3: root outlined white
text with opaque black outline, then a plain white text child above it.
Both are code-created **pixel-mode** TextShapes. See
[Plasma shapes/text §7.2](plasma-shapes-and-text.md).

The constructor collects those TextShapes through **0xf8590** and
sets each text flags word to **0x15**, wrap width **180**. Flags mean
horizontal centering, vertical block centering and word wrap. There
is no new rectangular background drawing or default blackwidget in
this construction.

It independently copies the owner **gui.plx** **`leftbutton`**, then
**`rightbutton`** templates through **0x3308e0**, making both children
of the outlined-text root after its plain text child. These templates
come from gui.plx loaded through0x3429a0 with flag0x20, were found
by0x332190 and detached from the template tree. Their own
drawings, widget type/settings, dimensions, transforms and keyed poses
remain owner data; this native path makes no fixed-size replacement
arrow. Unlike the nearby `button`/`button2` preparation, this path does
not add a blanket recursive passive flag to the arrow templates.

For each arrow it reads the root transform's current pivot at+0x208,
subtracts it from **left(−100,2)** or **right(100,2)**, writes that
result as the transform's current translation, and recomputes it
(0xf8d20/0xf8d30 after copy calls0xfdddb/0xfde5b). It does not call the
widget frame-size/move-to setter for these offsets. Copied owner
rotation/deformation/pivot and initialization therefore still matter.
The later common **0x333820** hierarchy initialization retains the
native owner widget/sequence contract in plasma-format-and-scene §2.7.

## 2. Placement, visibility, hover and color

Window/scale layout **0x101bd0→0x10211f** places the label through
**0x334700→0x334510**. Its requested **window point is(0,H)**,
where H is the physical window height converted to float32, and
its parent-local offset is **(150,−50)**. This is point placement,
not a widget frame-origin move. The helper converts the window point
through the parent's inverse world transform/homogeneous divide,
then adds that local offset and subtracts the node's live pivot to
obtain its translation. Under the ordinary uniform GUI scale s the
resulting anchor is **(150s,H−50s)**; do not scale H a second time.
Native float150/−50 constants are0x42e938/0x42e878.

Every reached interface update **0x103200** samples the label's
**own visibility** with0x9af00. If full-map byte is nonzero and
that own visibility was false, it assigns **first history node** to
I+0x5c8 and calls **0x1081b0** to refresh text/arrows. It then writes
label own visibility equal to the full-map byte (0x10678c/0x1067a0).
This transition does not recenter the map. It is not keyed to
flight/shrine state, a character's current region changing, or an
ancestor becoming visible. Closing/reopening through reached updates
resets selection to first; F4 hiding an ancestor alone does not.
Closing/hiding does not null the stored selected pointer; the next
own-hidden→shown initialization overwrites it with the then-first node.

When label own visibility is true, each arrow's widget subtree hover
query **0x32b0c0→0x32ae90** selects its root Display.fillColor:
**hover(0,1,1,1)**, otherwise **(1,1,1,1)**. Left is sampled/written
before right (0x1067c4..0x106860). This is a write to that root's
current Display color, not replacement of every child drawing color
or recursive TextShape glyph recoloring.

Separately, the selected-region hover rectangle in the interface
update requires **full-map byte nonzero**, **GUI pointer X<300s**,
**GUI pointer Y>H−90s**, and selected node not equal to the history
sentinel. The inequalities are strict; this block adds no X≥0 or
Y≤H condition and does not use the text's180-unit wrap bounds.
Multiplication/subtraction are float32, using the current GUI scale.
It does not test arrow hit, map drag or visible ancestors.

On success, copy the selected signed region pair to **WorldMap
(G+0x10)+0x120/+0x124**, recursively color this label's TextShapes
**(0,1,1,1)** with0x334b80, and clear the cursor text atI+0x330
(G+0x448) to empty. Otherwise write **INT32_MAX to both WorldMap
coordinates** and restore label text white. This runs even when the
label is own-hidden; its full-map predicate selects the default
branch then. It neither changes history selection nor target map center.
The successful color/clear is0x1068e9..0x10695b; default branch is
0x106962..0x1069d4. This late cursor-text clear can supersede the
earlier pointer-distance text producer in
[map rendering phases §5](steam-map-rendering-phases.md).

Ordinary Plasma ancestor visibility/flag traversal still decides
whether text/arrows actually draw or receive a global GUI event hit.
The standalone subtree queries in§5 are a separate operation. Hiding the GUI
root with F4 does not stop the ordinary interface bindings/state
updates; see steam-preview-hidden-interface-order and map rendering
phases. Label creation follows the MapOverlay creation, so it is a
later ordinary-UI child than that overlay's flat-marker draw, not a
label submitted within the3D map model pass.

## 3. Label text and arrow visibility refresh

**0x1081b0(I)** updates right/newer arrow visibility to selected-node
not first-node, then left/older visibility to selected.next not
sentinel. These are live Display visibility writes, not enable-state
changes. If selection is the sentinel, it does not replace the text
with a region0 string or an empty-history message.

For an ordinary selected node, use its stored region pair—not current
player/map cursor coordinates—to generate the proper name with
**0x611d0(seed,−1)**, seed **R(uint32(X)+57 times uint32(Y))**.
Query its kind with **0x2b9080**, choose the dictionary node through
**0x16b210**, then expand qualifier **`named`** through **0x16be20**,
with the generated name as its extra string. Write the resulting
plain text recursively through0x334c50.

This is exactly the dictionary/seed/substitution contract in
[identification UI §7](steam-identification-ui.md): kind→node table,
qualifier set `#named`, substitution key `@`, top-level grammar seed0,
and generic owner name-fragment extraction contract. Do not duplicate
or invent the owner vocabulary, seed the grammar with the region hash,
consume global crafting/drop RNG or substitute an item/NPC name.
The native calls are0x108322/0x10833b/0x108352/0x108373/0x108383.
History records save coordinates only; they do not save this string.

## 4. History storage, production phase and saved order

Personal data is **local current creature+0x1528**. At personal+0x118
is an8-byte circular doubly-linked head pointer; +0x120 is the64-bit
element count. Every allocated node is **0x18 bytes**:
next+0, previous+8, signed32 region X/Y+0x10/+0x14. Constructor
**0x7ff20→0x3c5f0** creates an empty self-linked sentinel/count0.
Sentinel payload bytes are not initialized as a valid coordinate pair.

### 4.1 Exact current-region input

The relevant late block of **0x2da6c0** runs after per-creature
**0x2de4c0** updates and queued day-reset processing, before0x2dba40.
It requires **World+0x448 local/current player nonnull**, not every
player in the world creature list, and no added HP/map/HUD-visibility
or minimum-travel-distance test.

It queries the effective block at that player's **physical fixed XYZ
+0x10/+0x18/+0x20**. Convert signed64 fixed X/Y to whole blocks
**toward zero, then narrow to signed32**. With effective-block flag
byte+5 bit0x20 clear, pass those block X/Y to **0x2ada20**. With
the bit set, floor-divide those narrowed coordinates by1024, obtain
the selected big-cell/landmark point through0x36160/0x362a0, then
query0x2ada20 at that point. This is the same state-dependent rule
as0x2ad8e0 in [region-query ABI §4](steam-region-query-abi.md), with
0x2ada20's landmark-aware query—not raw0x2ad4d0, floor(X/8192),
camera position or a saved recent label. Store its result into player
**+0x150/+0x154** (query0x2db80b, stores0x2db819/0x2db822).

The current-region fields are updated even during abilities **0x9c
and0x9d**. Only the subsequent history mutation is skipped for those
two ability bytes. No special invalid-coordinate rejection is added
in this history block.

#### 4.1.1 Big flag and landmark coordinates are not caller-pre-evened

For the bit0x20 path, the caller obtains signed cell coordinates
**cx=floor(blockX/1024), cy=floor(blockY/1024)** from the already
narrowed signed32 blocks. It calls **0x36160(World,cx,cy)** at
**0x2db7bf**. It then calls **0x362a0(World,out,cx,cy,bigByte)**
at **0x2db7d6**, passing **the same original cx/cy**, plus the low
byte returned by0x36160 as its fifth argument. It does **not** round
either cell coordinate to an even number before either call.

**0x36160** itself computes hx=floor(cx/2), hy=floor(cy/2), and
uses the existing masked hash H on wrapped32-bit
**hx+57hy+0x39a13d**. The big result is true when **unsigned
H/21474 is even**, **or** the extra sevenths test0x36210 succeeds.
For the latter it passes the original cx/cy (0x361ec..0x361f2).
**0x36210** independently computes floor(cx/2),floor(cy/2), then
v=hx+5hy; its test is **((v signed-rem7)+7) signed-rem7 equals1**.
Thus0x36210 is only this additional test, not a substitute for the
combined big predicate. Both tests give a common flag for the four
cells sharing the same even corner. This is existing terrain§3.1.

**0x362a0** first tests the **unmodified supplied cell** for exactly
(0,0), returning block point **(512,512)** regardless of bigByte.
Only afterward does its big branch floor each supplied axis/2
internally. With ex=2floor(cx/2),ey=2floor(cy/2), its big point is
**(1024ex+532+R(ex+57ey+0x7ab2c) unsigned-mod984,
1024ey+532+R(ex+57ey+0x271537) unsigned-mod984)**, with wrapped32-bit
arithmetic. With big false it uses original cx/cy, offset282 and
unsigned modulus460 instead. See terrain§3.2 for the shared point
contract. A raw neighboring cell such as(1,0) must not be changed
to(0,0) by the caller: that would incorrectly trigger the origin
special case before the helper can select its regular big point.

The caller loads the resulting block X/Y into0x2ada20 at0x2db80b;
that remains the full **landmark-aware** region query, not a direct
raw0x2ad4d0 replacement after this first point selection.

### 4.2 Revisit and node lifetime

If count is nonzero and the first node's pair equals current region,
leave the list unchanged. Otherwise scan forward from the first node
for the current pair. On the **first** match, unlink and free that
**whole node**, decrement count, then stop scanning. Insert a **new
node at the front** with the current pair using **0x2d0610**; with
no old match, simply prepend. There is no in-place splice/reuse or
fixed-length history trim. The mutation is0x2db89f..0x2db927.

Starting empty and maintained only by this producer, the list is unique
and most-recent-first. The loader below does not promise that invariant
for arbitrary saved input: a matching first entry skips all duplicate
cleanup, and a nonfirst revisit removes only its first older match.

I+0x5c8 is a borrowed **node address**, not saved coordinate storage.
Unmatched nodes retain their addresses; the matched/revisited node is
freed, and the new front node has a new allocation/lifetime. This
producer does not repair I+0x5c8. The shown-label interface block also
does not scan to repair a deleted selected pointer; it only initializes
on the own-hidden→shown condition above. No verified native fallback
selection is established for a selection whose node has been deleted
or whose personal-data list has been replaced. Do not describe it as
a value-preserving automatic reselection or invent a region0 target.

Prepending an unmatched region also does not automatically select that
new first node: the previous selected node remains at its new list
position. Its arrow visibility is not recomputed every interface
frame;0x1081b0 runs on the shown transition and explicit navigation
handlers. Therefore no automatic label/arrow refresh follows merely
from a history mutation while the label remains shown.

### 4.3 Save/load semantics and frame phase

Personal-data destructor **0x4db70** explicitly frees every history
node and its sentinel. Character loading/reset and new-character
creation replace personal state (0x147800 and0x9f7c0), so none of
those old list-node addresses is a durable cross-load selection.

Character writer **0x149760**, field28, writes **signed32 low count**,
then each8-byte X/Y pair in **next traversal order, first to last**
(0x149760 around its0x118/+0x120 member handling). Reader **0x147800**
starts with fresh personal state; for a positive signed count it reads
each pair and appends through **0xf4420**, which inserts immediately
before the sentinel. Thus a valid save's history order is preserved,
not reversed, sorted or deduplicated. Zero/negative counts add no nodes.
The UI selected-node pointer is not serialized. Existing malformed/
truncated reader behavior remains native-saves §3.1. The daily reset
**0x2c34c0** clears other named mission/shop maps but does not clear
this history list.

In the ordinary Game frame, admitted World updates **0xa7328/0xa734c**
produce history **before** early map-center follow/easing, input/use
selection, Preview selector, later map toggle and **0x103200** label
initialization/hover output. There can be multiple World substeps in
one Game frame; this is not a once-per-render label producer. The
requested world seed/name admission remains native-saves §6.
The server-world updater0xf5520 also calls0x2da6c0; the history block
still requires that World object's own+0x448 current player. No
history packet or authoritative award is produced by this block.

## 5. Arrow input, selection and recentering

Constructor0xf9ac0 at0xfdf06/0xfdf54 registers **event2, left mouse-down**, for the
named widgets `leftbutton` and `rightbutton` in the respective copied
subtrees using **0xf8090**. It enables recursive widget registration
and requires enabled widgets. The receivers are I, callbacks
**left/older0x101860**, **right/newer0x101790**. This differs from
the frontend Select button's event3/up contract. Use ordinary owner
picking/capture/passive flags, not a synthetic arrow rectangle.

For physical/controller mouse-button0 down, **0x94900 first calls
0x101020→0x340ec0**, which dispatches that GUI down callback at the
current cached pointer. Only **afterward**, if full-map is open,
0x94900 hit-tests copied left arrow, then right arrow through0x333e70,
and returns on either hit before ordinary map/item/double-click work.
These tests see visibility changed by the just-run callback. A newly
hidden boundary arrow is not protected by a cached pre-callback hit.
Their0x333e70 wrapper calls0x333f60 with the arrow as the subtree
root and cached GUI pointer/options0. That traversal checks the
supplied subtree's own visibility/flags and uses ancestor matrices
for coordinate conversion, but does not walk ancestors to reject
their hidden visibility. It is not a cached result of the preceding
global-root event pick. Thus a hidden ancestor prevents normal GUI
dispatch/drawing, without adding an ancestor-visible predicate to
these explicit post-dispatch arrow queries.
Other mouse buttons do not dispatch event2, although this post-GUI
map-arrow hit return itself has no button0-only condition. Up routes
through0x101120's ordinary release handling; it does not invoke a
second history callback.

Each handler changes the node only if its boundary allows:
left follows next unless next is sentinel; right follows previous
unless already first. **Even with no movement**, it then refreshes
text/arrows, obtains selected region's deterministic point through
**0x2b9210**, calls **0xa3390** to recenter and raises map zoom
index **G+0x1e04 to at least4**. There is no wrapping, distance-based
history sort, history deletion, travel or item consumption here.

The point is existing region-position generator
[worldgen-terrain §2.1](worldgen-terrain.md), using the selected region
pair: each block axis is signed32 wrapped **8192 times coordinate,
plus1024, plus its unsigned R remainder modulo6144**; salts0xe29e/
0x70532. It is not the region's geometric midpoint or the player's
last physical position there. **0xa3390** samples terrain height
**0x35ea0(blockX,blockY,0)**, multiplies its returned float32 height
by float32 65536, truncates to signed64 for target Z. Target X/Y are
the signed block coordinates widened and shifted left16. It writes
G+0x1e08/+0x1e10/+0x1e18 and sets manual-center+0x1e38=1, leaving
drawn+0x1e20 XYZ to ordinary update/smoothing.

**Controller routing/lifetime is exactly controller handoff §2**:
successful map-branch polls call older on left-shoulder0x0100 down
edge then newer on right-shoulder0x0200 down edge; no held repeat or
visible-arrow prerequisite. Edge bytes0x551cdd/0x551cde update only
inside that branch and are not reset by map open/close. Each invoked
edge marks last device pad. These polls precede the Game frame's
World/history and label-update stages. Mouse callbacks occur during
message handling, likewise independently of label rendering.

No history-specific keyboard-arrow/Enter/Space callback is registered
by this construction or dispatched by Game key handler0x96e70.
Generic GUI key-down0x340df0 targets only its enabled focused widget;
ordinary **Button vtable0x47b3a0 slots+0xc0/+0xc8/+0xd0** are the
no-op0xe8a20, so a Button does not turn those keys into event2.
Actual copied widget class/settings remain owner data. Mapped Zoom
In/Out controls are map zoom, not history selection. No invented
VK_LEFT/VK_RIGHT history shortcut follows from the arrow artwork.

## 6. Map toggle and the distinct zoom controls

### 6.1 Toggle and mapped zoom edges

**0x1031e0(I)** toggles only full-map byteI+0x550/G+0x668. The Game
frame calls it at **0xaafcc** on a down edge of mapped **World Map
action15, G+0x1a7f**, admitted while chat entry is closed and the
own-visible title/character-selection/creation roots G+0x118/+0x130/
+0x170 are all hidden. Previous byte0x5514fc updates only in that
admitted block. This consumer adds no System/Options/Controls or
generic focused-editor test; the actual input/binding-capture routes
remain their separate specifications. Default action15 is keyboard
scan code0x32/M, and D-pad Left0x0004 supplies the same action in
gameplay and interactive-map routing. It is not an older-history key.
Controller B's existing UI dismissal closes the map through the
shared controller contract; flight/shrine completion/close routes
remain steam-item-map-selection. The label's own shown assignment
depends on this full-map byte, including those special full-map modes.

Game constructor **0x7ea80** initializes signed zoom index
**G+0x1e04=3**. WorldMap constructor **0x2eaa00→0x2f7cc0** resizes
its layer vector at+0x170/+0x178 to **nine0x68-byte records**, not
merely nine slots of reserved capacity. Thus normal mapped zoom
uses stock layer indices **0..8**.

The early Game frame tests mapped **Zoom In21, G+0x1a85**, then
**Zoom Out22, G+0x1a86**. On their respective down edges, decrease
or increase L by1, clamp lower to0, then upper to layerCount−1
(writes0xa6edb/0xa6f31 and0xa6f68/0xa6fbe). It updates previous
bytes **0x5514f4/0x5514f5 every reached Game frame**. No held repeat,
full-map/label-visible requirement, cursor-centered translation,
manual-center assignment or drawn-center snap occurs in these two
consumers. In then Out is the simultaneous-edge order, including
their intermediate clamping. Native constructed count9 is nonempty;
these instructions do not supply a separate useful empty-vector case.

Input production is important: ordinary **0x1355a0** first samples
the23 configured controls, then **clears actions21/22 before polling
XInput**. Successful D-pad Up0x0001/Down0x0002 set them again.
Configured keyboard/mouse Zoom bindings therefore do not survive
that ordinary pass. These are not hardcoded keyboard Up/Down arrows.
Map toggle action15 and other configured controls retain their own
binding semantics; see remaining-runtime-gaps §4.1. Shoulder minimum4
is an action-time adjustment, not a persistent minimum: a fresh
Zoom In edge later in the same frame can reduce it below4.

### 6.2 Wheel map zoom and cursor-centered fixed arithmetic

Physical WM_MOUSEWHEEL0x20a is handled by **0x133c70**: take the
signed high16 wheel delta and replace a positive value with+1,
negative with−1, retaining0. It does not accumulate120-unit notches
or pass a multi-notch magnitude. Call **0x97cc0(G,signedDelta)**.

That dispatcher first sends the event to the **first** eligible panel
with its root **own-shown plus standalone0x333e70 hit**, in this order:
inventoryG+0x330, shop+0x350,
crafting+0x340, Controls+0x658, multiplayer+0x388. Their existing
row-scroll paths consume it. In fallback, mapped **Walk action13 at
G+0x1a7d** selects orbit-camera zoom, not map zoom: requested distance
decreases by float32 of the integer twice-delta and clamps0..50.
This is the rebound Walk action, not a raw test of GUI Control0x11.
See gameplay-camera §3.3 and inventory-refresh-events for those routes.

Otherwise compute **newL=clamp(oldL−delta,0,6)** and store it.
This explicit wheel ceiling6 differs from mapped zoom's stock8.
Only if L changed **and WorldMap cursor fixed X+0x100 differs from
0x7fffffff0000**, recenter as follows. There is no additional
full-map-open, cursor-Z sign or label-selection gate in this block.

Take signed64 fixed differences cursorX−targetX and cursorY−targetY.
For each axis, multiply the fixed difference by integer **32768 when
newL<oldL**, or **−65536 when newL>oldL**, retaining low64-bit native
arithmetic; divide that signed64 product by65536 **toward zero**;
add the resulting fixed displacement to that target axis. These
gains are native doubles32768 atRVA0x42e588 and−65536 at0x42e620
converted to integer before the multiply. They are not float block
deltas. The gain depends on **direction only**, even if clamping
skips more than one index from an old mapped level7/8.

Recompute target Z with0x35ea0 at target XY converted fixed→whole
blocks toward zero and narrowed to signed32, mode0. Multiply its
float height by float65536 and truncate to signed64. Then copy all
three target coordinates directly into **drawn centerG+0x1e20 XYZ**,
and set manual-center+0x1e38=1. Unlike shoulder recenter, this wheel
path snaps the drawn center. With an invalid cursor X or unchanged
index it does neither recenter nor snap/manual assignment. The zoom
index change itself still persists.

### 6.3 Rapid reached mouse-down recenter/zoom

The early common block of **0x94900**, after GUI dispatch and the
explicit map-arrow hit returns, samples **timeGetTime**. Subtract
the previous reached-down timestamp **0x5514e8 as unsigned32**,
then replace that timestamp with now. If elapsed is **strictly<200ms**,
full-map is open and **WorldMap cursor fixed Z+0x110>0**, copy cursor
XYZ to target, replace target Z with the same terrain-height sampling
rule, decrease L by1 with lower clamp0, and set manual-center1.
It does not snap drawn center here and does not require L to change.

This timestamp block itself does **not** compare mouse-button identity,
physical cursor separation, a Windows double-click setting or the
history-selected region. It has no button0-only test. Earlier returns
can prevent reaching it; the arrow callbacks/updated-visibility return
order in§5 is especially relevant. Later inventory/identification
business returns do not move this already-reached block later.

## 7. Manual center, panning and follow precision

### 7.1 Early follow/reset and drawn-center easing

The Game constructor initializes manual-center **G+0x1e38=0**, target
+0x1e08 XYZ and drawn+0x1e20 XYZ to zero. In a reached Game frame,
if full-map is closed, the early block clears manual-center and
WorldMap flight-mode byte+0x164. When manual-center is0, early map
update copies the current local player's **physical fixed XYZ** to
**both** target and drawn center. It does not follow a selected
history node or the camera eye. When manual-center is1, it retains
target XY and refreshes its terrain Z by the rule in§6.2.

The common smoothing call **0xa7f42→0x7d0b0** uses the retained
**capped double Game-frame milliseconds**, with float rate0.01
(0x42e51c), through existing easing coefficient helper0xa7a0. This
is the original Game double, not the renderer/GUI rounded float clock.
The coefficient result is float32. Multiply it by float32 65536,
truncate to signed64 integer weight. Per axis, multiply the signed64
target-minus-drawn fixed difference by that integer weight, keep
native low64 bits, divide the signed product by65536 toward zero,
then add to drawn position. This is fixed-point easing with a
quantized weight, not direct double interpolation. It runs even
after the no-manual branch made the difference zero.

### 7.2 Mouse right-held pan

The ordinary pointer handler **0x97880** first reaches interface
pointer update0x101160 when ordinary-UI predicate0x1014f0 is true,
with incoming coordinates truncated to signed32. If last-used-device
**G+0x1958 is nonzero afterward**, it returns before this mouse path.
Use current-minus-previous **GUI float coordinates at engine+0x180/
+0x184 minus+0x188/+0x18c**, not another OS delta sample.

With **full-map open and physical right-held engine+0x1a8 bit1**, form
float32 input **(dx times powf(2,float32(L)), negated-dy times that
same factor,0)**. Start an identity matrix and rotate about Z through
**float32(displayed yawG+0x1d90)** degrees using0x13050; this is the
displayed angle stored at the event, not the yaw target, map pitch
or an extra90° turn. Apply its linear3×3 to that already-scaled input.
Multiply each resulting float component by float65536, truncate each
to signed64 and add to target XYZ, then set manual-center1.
No frame-time, cameraSpeed, GUI-scale or mapScale multiplier enters
this displacement. This right-pan branch does not add a GUI-hit/
capture exclusion. Ordinary camera-look and customization rotation
remain separate branches described by gameplay-camera §3.1.

Any nonzero GUI pointer displacement in the admitted mouse UI path
sets dragged byte **G+0x1db0=1**, independently of whether right is
held. The early Game frame clears it when **engine held bits&3 are0**
(neither left nor right), and a reached ordinary down clears it at
0x94e7a. Thus it is not simply a copy of right-held; holding a button
after moving can preserve it, and an arrow's earlier return can skip
the down reset. Marker-hover admission still uses that separate byte.

### 7.3 Controller pan and late GUI cursor update

Later in **0xa5a50**, after current camera smoothing0xadcd0 and map/
panel toggle processing, the branch requires **full-map open and
last-used-device nonzero**. It first calls **0x3418d0** to update
GUI pointer to float32 **(windowWidth times the float at G+0x1e48,
(1 minus the float at G+0x1e4c) times windowHeight)**, using the current eased map-screen
fractions. This is a direct GUI pointer move, not SetCursorPos or
an ordinary left-stick navigation-point snap. Then set manual-center1
even if the movement stick is zero.

Form vector **(minus filteredLeftX at G+0x1d6c,
minus filteredLeftY at G+0x1d70,0)**.
Compute float32 **powf(2,float32(L)) times0.5**, then multiply by
**float32(cappedDoubleGameFrameMS)**. Scale the vector by this scalar
**before** rotation. Rotate an identity matrix around Z by the
**then-current displayed yaw narrowed to float**, apply its linear3×3
(0xab1ce/0xab241/0xab257), convert float components to fixed by
float65536 and trunc64 through0x6d240, and add through0x862f0
at0xab279. No new stick threshold, radial normalization, map pitch,
GUI scale or viewport size enters this pan speed. Filtered-stick
production retains its existing native dead-zone contract.

This pad branch runs **after** early drawn-center easing and after
Preview selection; it changes target now, not drawn center again.
The next reached frame follows/eases toward it. By contrast mouse
callbacks before the Game frame can affect that frame's early
easing. Right stick retains the native camera input in interactive
map mode; the ordinary-UI right-stick wheel repeater is not a new
map-wheel producer. Shoulders remain direct history actions (§5).

### 7.4 Movement closes map on the non-pad-map route

The alternative late branch is reached when **map is closed or
last-used-device is0**. It closes map and the enumerated ordinary
panels when either mapped movement actions **5/6/7/8** are held while
chat entry is closed, **or** filtered left-stick squared length from
0x2b630 is **strictly>float0.010000000707805157** (word0x3c23d70b
atRVA0x437f74). This helper returns x²+y², not its square root.
The chat-closed condition belongs only to the digital-action term.
When map is open and last device pad, this close branch is bypassed
in favor of§7.3. Keyboard movement therefore does not acquire an
invented map-pan or region-arrow meaning from these controls.

### 7.4.1 Exact movement-close writes and answer-page clearing

Once§7.4's predicate succeeds, **0xab2d4–0xab39c** performs these
writes in this order. Every Widget hide below uses0xa3a20 on that
Widget's root at+0x178: it writes the root's own live Display visibility,
not each descendant's visibility or a new disabled/input state.

|Order|Target|Action / native callsite|
|---|---|---|
|1|Game+0x668|Clear interactive full-map byte,0xab2d4|
|2|InventoryWidget Game+0x330|Hide root,0xab2ec|
|3|Shop InventoryWidget Game+0x350|Hide root,0xab301|
|4|Crafting InventoryWidget Game+0x340|Hide root,0xab316|
|5|FormulaDetailsWidget Game+0x348|Hide root,0xab32b|
|6|Game+0x420|Clear the separate menu-open byte,0xab330|
|7|SystemWidget Game+0x358|Hide root,0xab348|
|8|VoxelWidget customization Game+0x368|Hide root,0xab35d|
|9|IdentificationWidget Game+0x360|Hide root,0xab372|
|10|AdaptionWidget / Upgrade Game+0x370|Hide root,0xab387|
|11|MultiplayerWidget Game+0x388|Hide root,0xab39c|

Then query the **answer panel Game+0x378 root's own visibility** at
**0xab3af**. Only when it is shown, clear its page vector+0x1e8
through **0x8adf0 at0xab3bf**. That helper destroys each existing page
and sets vector end to begin; it does not reset the Widget's unrelated
clock/index fields. This is not merely a hide that retains visible
answers. Hidden answer pages are not cleared by this batch.

The immediately following ordinary answer-state consumer detects an
empty page vector and hides that root at **0xab3f3**. Thus answers
that were shown and cleared are hidden in the same frame. The rest of
that ordinary answer lifecycle remains dialogue §5.6.

This close batch contains no direct Ring Game+0x310 or Preview/InfoWidget
Game+0x338 hide, chat-entry Game+0x398 reset, general DialogWidget
Game+0x418 page clear, Options
Game+0x650 hide, or Controls Game+0x658 hide. Their separate routes
must not be replaced by an invented hide-every-panel policy. Except
for the explicit answer-page destruction, these writes do not perform
canonical item sorting, reset selected/cursor items, refund customization
cubes or destroy panel data. Other independently specified frame stages
can still affect those states.

## 8. Ordered integration phases and remaining boundaries

The relevant native order is: window GUI/down callbacks; ordinary
input polling/mouse pointer processing and successful pad shoulder
callbacks; then Game frame early dragged reset and mapped zoom edges;
admitted World substeps/current-region-history production; early
map follow/terrain-Z refresh/drawn-center easing; use/Preview selection
and pointer-distance text; displayed-camera smoothing; mapped map
toggle and late pad pan/movement-close;0x103200 shown-transition/
history hover outputs; current3D map submission; ordinary GUI traversal
with MapOverlay before this later-created label. See
[map rendering phases](steam-map-rendering-phases.md) for the complete
map/cache/project/draw order and hidden/full/minimap admission.

No extra map pan/zoom operation is generated by label selection
initialization, label hover highlight, word wrapping or arrow draw.
There is no verified native recovery for a freed selected node,
meaningful empty-history recenter payload, alternate keyboard-history
shortcut, or numeric replacement for the owner's arrow drawing/
binding/pose. Those limits remain explicit; no original-game or
owner-data observation was used to manufacture them.
