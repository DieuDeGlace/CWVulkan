# Remaining runtime behaviors

Clean-room behavioral facts from the owner's Steam executable and native exports. Image base
is 0x140000000; addresses below are RVAs. No third-party reconstruction was consulted.
This document describes behavior, not an implementation. Sections explicitly marked verified
are usable independently; unresolved questions are listed separately.

## 1. Verified: received hits on the local player

Source: the damage-number consumer in frame routine **0xa5a50**, with helpers 0x50370,
0x5f8d0, 0x5ed30, 0x5fac0 and the copy operation 0x16fb0. This resolves
`netcode-compatibility.md` question 8. Use the 72-byte damage-number record in
`creature-hit.md` §0.3. The arithmetic below is single-precision float; stun durations
arrive as signed integer milliseconds. The damage in this record has already passed
through the hit producer's defenses; do not apply armor again.

The frame consumes the received damage list at G+0x1970 in list order. It first looks up
the record's target id. No target means no application. A port that holds its local player
outside its general creature map must still resolve that id to the local player. The
following rules apply when that resolved target is the local player P.

### 1.1 Blocked outcome, record kind +0x45 = 1

* Copy the record's push vector (+0x38, +0x3c, +0x40) directly into P's **ordinary velocity**
  (+0x34, +0x38, +0x3c). It is not added, strength-compared, or copied into the separate
  knock-back velocity at +0x4c. This happens even when damage is zero or negative.
* If damage is positive, P's hit flash (+0x994) becomes 1.0.
* The block cost is damage divided by P's block value (0x50370, already specified by
  `stat-rules.md` §5.3 / `creature-hit.md` §5.5). Abilities 0x48, 0x49 and 0x82 replace
  that cost with zero; ability 0x5f then multiplies its cost by 0.5.
* Potential mana gain is the larger of `(1.0 - cost) × 0.5` and zero, in that order.
  Obtain the mana factor (0x5f8d0): 1.0 unless the first buff of type 0x22 exists, in
  which case it is `1.0 - strength × 0.1`.
* Only when P is guarding (0x5ed30; `stat-rules.md` §8.3), add
  mana-factor × potential-gain to mana (+0x188). Also add the same quantity to secondary
  mana (+0x158), except during ability 0x5f.
* Cap mana at 1.0, then lower secondary mana to mana if secondary mana exceeds it.
  Subtract cost from block power (+0x184), then floor block power at -1.0.
* This branch does not change health, stealth, or stun and does not apply the record's
  second timer (+0x1c).

### 1.2 Mana-shield outcome, kind 5

Visit **every** buff of type 6 in P's buff list. Subtract the received damage from its
strength (+4 in the 24-byte buff). When the resulting strength is <= 0, set both strength
and duration (+8) to zero. For each matching buff append a buff-change notice to the
client world's +0x150 list: source id and target id are both P's id, the buff is the
resulting 24-byte buff, and its flag is the constructor default zero. This pass changes
no health, mana, stealth, velocity, or stun. It does not require positive damage or
positive health.

### 1.3 Other outcomes

Only if P's health is positive and kind is **not 6**, perform these operations:

* For positive damage, reduce stealth (+0x18c) by 0.25, floor it at zero, and set hit
  flash (+0x994) to 1.0.
* For a positive record stun duration (+0x18), convert it to float and raise P's stun
  (+0x130) to the larger of its old value and that duration. No velocity is zeroed here.
* Subtract damage from health (+0x180). Thus negative damage heals. If the result is
  **strictly below zero**, set health to zero, append sound 0x1b at P's position with
  pitch 1.0 and owner id P, and collect P for this frame's death-effects pass. An exact
  result of zero does not take this particular sound/death-collection branch.
* Cap the resulting health at P's maximum (0x5fac0; `stat-rules.md` §5.1).

The consumer does not test ordinary outcomes for kind 0 exclusively: kinds 2, 3, 4 and
other values use this same branch, except kind 6 which skips it. The received ordinary
push, critical flag, spirit flag and second timer are not applied to P in this branch.

Before the local-player branch, a resolved target whose health is already <= 0 and whose
race is not 0x11d is separately collected for death effects. For a non-local target, positive
damage made by someone other than the local player sets only its hit flash to 1.0 here;
the host's creature-state updates supply its resulting health.

### 1.4 Constants and verification limits

Binary constants confirmed from the owner's unpacked executable: 1.0 at 0x4ac980; 0.5
at 0x4ac964; -1.0 at 0x4aca4c; 0.25 at 0x4ac95c; float 0.1 at 0x42e528. These facts
are verified by native exports and binary data, not by a new runtime capture. Useful
behavioral cases for an implementer are: blocked zero damage still overwrites velocity;
shield exhaustion clears both strength and duration and emits a notice; kind 6 leaves
health untouched; exact-zero health versus an overshooting lethal hit; and healing capped
at maximum health.

## 2. Verified: feat display; payload semantics remain unresolved

Packet 7 list 11 is a 40-byte record beginning with its recipient creature id. It is
copied into the client's incoming G+0x1a10 list and then appended to world+0x130 by
0xa5a50. The same frame visits the world list and acts only on records whose recipient
is the local player's id. It creates floating text at the player's position, a 3000 ms
lifetime, size 30, vertical parameter -100, and text **FEAT!**; appends that text to
G+0x20b0; prints the same text in the chat area; and plays sound 0x1f. The text helper
**0x935a0 ignores the entire payload** after the recipient id and always returns FEAT!.
The feat consumer does not award an item, XP, coins, or quest completion.

The record copy operations **0x36a90 / 0x37080** establish these occupied fields, without
establishing their meaning: i64 +0, bytes +8 and +9, four-byte values +0xc and +0x10,
eight-byte value +0x18, and four-byte values +0x20 and +0x24. Gaps +0xa..+0xb and
+0x14..+0x17 are not copied by these operations. Preserve the raw 40 bytes on the wire;
do not infer a mission structure or reinterpret the trailing 32 bytes as a string.

Unresolved: no gameplay producer was identified in this pass. The traced construction
paths cover received records, list copies, initialization and host distribution. This
does **not** establish that a mission completion emits a feat. A recording or further
producer trace is still needed to name the payload fields or specify when to originate
one. A receiver can implement the display completely without that producer.

### 2.1 Follow-up producer audit and concrete limit

A second pass traced the feat list's construction paths through native call
references as well as decompiled bodies. Its record-copy constructor **0x36a90**
is called only by the list-range copy **0x38540**; the other copy constructor
**0x37080** is called by incoming 40-byte list decoder **0x3a130** and range copy
**0xefd00**. Those range copies serve world-event collection/distribution and
incoming-list transfer. The common 56-byte node allocator **0x3c4d0** is otherwise
used for empty-list initialization, event-list initialization and those copies.
No gameplay append that constructs a new feat record from mission completion was
identified. No stored 64-bit function pointer to either record constructor was
found in the owner's unpacked image.

The 32-byte descriptor accepted by numeric dialogue opcode 11 (§5.4) has the same
occupied field widths as the feat's trailing descriptor. This establishes a layout
correspondence only. Its own source is an eight-i32 operand reader **0x15ce50**;
it supplies no names for the fields. The traced numeric producer in §5.5 never emits
opcode 11, so it cannot establish what values or gameplay meaning a real feat would
have. The feat display helper still ignores all those fields.

**Concrete blocker:** available native paths preserve/receive/display an existing
descriptor but provide no identified gameplay construction or semantic consumer
for one. These sources therefore do not support inventing a feat kind, location,
quest id or reward interpretation. Dormant/unfinished feat production is consistent
with the static evidence, but is an inference, not a demonstrated runtime fact.
Originating feats remains unresolved; preserve arbitrary received payloads and use
the verified receiver behavior.

## 3. Verified: talking and receiving a quest as a guest

Sources: **0x98180** (local use/talk), **0x98040** (queue talk interaction), **0x2d9480**
(build script), **0x1694e0** (compile script into pages), **0xa3170** (attach pages to
speaker), **0xb40d0** (consume pages), **0x2c3710** kind 3 (host-side interaction).
`conversation-script.md` specifies the actual script and the player's known/completed
tests. The game performs the ordinary dialogue and its personal rewards on the talking
player's client. It does not wait for the host to send a quest-given packet.

### 3.1 Two effects of one ordinary talk

After the local use path accepts an ordinary friendly NPC in talk range, it builds the
script from the client's world and NPC record (0x2d9480; the planner and 0x12a6f0 described
in `conversation-script.md`). It compiles the script locally with 0x1694e0 and attaches
the resulting pages to that NPC with 0xa3170, passing the **local player as listener**.
If that speaker already has a dialogue widget, use advances its presentation instead
of generating a fresh script. Special merchant, crafting and flight screens have their
own branches; these statements concern ordinary scripted talk.

The client additionally queues an interaction in its own player's interaction list
(P+0xab8) through 0x98040. It is the ordinary 0xc8-byte interaction layout: kind +0xc4
is 3, target id +0xb0 is the NPC, default object index is -1, and the item is empty.
The host processes that interaction through 0x2c3710 kind 3. Its changes include NPC
state (rescues, arena stages and reward drops as applicable); those return in ordinary
creature/zone/item events. It does not serialize the dialogue pages or the player's
personal known-mission set as part of kind 3.

### 3.2 A mark is applied on the page that carries it

The `mark_location x y` token carries **signed i32 place-cell coordinates**, each cell
1024 blocks wide. In 0x1694e0 it appends that pair to the **most recently created
dialogue page's list at page+0x1a0**; if no page exists it first creates one. Compiling
the token does not directly change the player's persistent quest state. Pages are
0x1b0-byte records. Their relevant fields are:

| Page offset | Meaning |
|---|---|
| +0x30 | number of the item to remove, whose item record starts at +0x34 |
| +0xd4 | number of the item to give, whose item record starts at +0xd8 |
| +0x190 | list of discovered location pairs (also used for lore) |
| +0x1a0 | list of mission-location pairs to mark |

The page consumer 0xb40d0 performs a page's actions when its presentation clock is
**exactly zero** and the current page index is valid. Thus later pages' quest marks
wait until the player reaches those pages. For each pair in +0x1a0, only if mission-known
test 0x5f4c0 is false, it inserts the pair into both personal-state sets **+0xb8 (known)**
and **+0x38 (map marks)** beneath the data pointer P+0x1528. It sets that location's
map-highlight countdown in the map at personal-state+0xe8 to **180000.0 ms**, and plays
sound **0x20** at the listener/camera position G+0x1dc0. The known test also considers the solved
set at personal-state+0xc8; consequently an already known or solved location is not
marked/sounded a second time by this operation. These mutations are local, including
for a guest, and are not sent to every party member.

The same page consumer independently processes the current page of the general
dialogue panel G+0x418, with the same +0x1a0 known/mark/highlight/sound behavior. Speaker
dialogue widgets are kept in G's +0x400 vector. A port can use its own UI storage but
needs to retain the per-page timing of side effects.

### 3.3 Page rewards are also local

For a speaker page at clock zero, remove the page's requested item when its +0x30
count is nonzero (0xa2210). For a nonzero give count at +0xd4, first test whether the
local player may receive that item (0x50670); on success add that count to the local
inventory (0x4e810), display the received-item feedback (0x9d6f0), and set G+0x2050 to
60000 ms. These operations address the local player directly, with no host-player
selection and no broadcast reward. The general dialogue panel has the same local
give/remove actions, but its remove action is within its nonzero-give-count branch.
This page consumer does not itself set the solved/finished mission sets.

For distinguishing this from host-created reward drops: the host interaction can create
ground items, which reach the recipient through the separate pick-up notice and zone
item paths. Receiving a dialogue page's item and receiving a ground item are distinct
events. The feat list in §2 is not the carrier of either the quest mark or the page's
inventory reward.

Limits: this section verifies where the local client consumes scripted side effects.
It relies on `conversation-script.md` for which script appears and does not newly
specify dictionary expansion into page reward fields, all special-NPC screens, or the
complete solved/finished mission update lifecycle.

## 4. Verified: XInput controller input

**Mode correction:** **G+0x668 is the interactive/full world-map-open byte**. It is
true for an ordinary opened world map as well as flight/shrine selection maps. It is
independent of NPC/object speech and lore/item-receipt dialogue. Earlier versions of
this spec incorrectly called it “conversation mode”; every affected rule in §4 now
uses “interactive map mode” instead. Engine-facing mapping and source evidence are
in §4.14. In particular do not drive this byte from a Talk-active/from-object flag.

Sources: **0x1355a0**, including its assembly, plus **0xa36c0**, **0xa38c0**,
**0xa3900** and the camera-input portion of **0xa5a50**. This completes the gameplay
mapping left open by `frontend-options.md` question 5 and `player-control.md` question 8.
The main loop polls **XInput user index 0 only**. Polling is within the foreground-window
input branch and is skipped while the controls screen waits to capture a new binding.
These fixed mappings are independent of `controls.cfg`.

### 4.1 Held buttons

The pad adds held states after the configured keyboard/mouse states have been sampled.
Each listed button sets the associated byte G+0x1a70+index to 1 for that frame. It does
not create an additional pulse each frame; existing consumers implement press-edge or
hold behavior as described by `player-control.md` and the frontend specs. Before polling
the pad, Zoom In and Zoom Out (indices 21/22) are explicitly cleared even if a configured
keyboard/mouse binding had set them.

These mappings run in both gameplay and UI mode:

| XInput control | Button mask / threshold | Binding index and meaning |
|---|---|---|
| D-pad Up | 0x0001 | 21, Zoom In |
| D-pad Down | 0x0002 | 22, Zoom Out |
| Back | 0x0020 | **both** 18 Crafting Menu and 17 Inventory |
| Start | 0x0010 | 19, System Menu |
| Right shoulder | 0x0200 | 0, Normal Attack |
| Right trigger | byte > **128**, not >= | 1, Special Attack |

The following additional states run when the game's UI-input predicate (0x1014f0 on
G+0x118) is false and G+0x668 (interactive map mode) is false:

| XInput control | Mask / threshold | Binding index and meaning |
|---|---|---|
| Y | 0x8000 | 3, Ultimate Skill |
| B | 0x2000 | 9, Jump, with the suppression rule below |
| Left shoulder | 0x0100 | 10, Dodge |
| Left trigger | byte > 128 | 2, Class Skill |
| X | 0x4000 | 4, Quick Item |
| A | 0x1000 | 11, Use/Interact |
| D-pad Right | 0x0008 | 12, Lamp |
| Left-stick press | 0x0040 | 14, Call Pet / Ride |
| Right-stick press | 0x0080 | 16, Ring Menu |
| D-pad Left | 0x0004 | 15, World Map |

The last four rows also run in interactive map mode (G+0x668 true), while the Y/B/left
shoulder/left trigger/X/A gameplay mappings are replaced by §4.4 there. The pad does not
write forward/back/left/right binding indices 5..8; movement comes from the analog pair.
No pad mapping to Walk (index 13) or Multiplayer Menu (index 20) occurs in this loop.

**B after a menu:** holding B while the UI-input predicate is true sets a suppression
latch and closes interactive map mode and the visible panels enumerated by this branch
(inventory, crafting, map and associated in-game menus). The Jump state is suppressed
while that latch is set. Releasing B clears the latch; a subsequent B hold in gameplay
can jump. Thus the same B hold used to dismiss a menu does not immediately become a
jump when the menu disappears.

### 4.2 Sticks and the effective dead zone

Each XInput stick component is a signed 16-bit value, -32768..32767. Convert it to float,
multiply by **1/65536**, then add that result to itself. This gives a signed value in
[-1, 0.999969482421875]. There is no remapping of the surviving range to compensate
for a dead zone and no unit-circle normalization of diagonal input.

When gameplay input is available **or** interactive map mode is active:

* Left X and Y go to G+0x1d6c and +0x1d70 through 0xa38c0; positive X is right and
  positive Y is forward, as consumed by `player-control.md`.
* Right X and Y go to G+0x1d74 and +0x1d78 through 0xa3900.
* Each setter computes x²+y² in float. If that sum is **strictly below** the float
  **0.04000000283122063** (binary value 0x3d23d70b, RVA **0x437f7c**), it zeroes both
  components. Otherwise both survive unchanged. This is a circular dead zone with
  radius approximately 0.2, shared by both sticks. Equality survives.

The main loop also contains per-component raw-stick zeroing at ±7848 (left) and ±8688
(right). These assignments occur **after** both normalized pairs were written, and are
not sent back through the setters. Consequently these per-axis thresholds do not define
the effective gameplay movement/camera dead zones. The setters' circular test above does.

On a failed XInput poll the loop calls both setters with zero pairs. In ordinary UI
mode the left movement pair is zeroed while the left stick navigates the cursor. Right
stick behavior in that mode is described in §4.4.

### 4.3 Right-stick camera arithmetic

The frame's right-stick pass in **0xa5a50** uses the stored right-stick components. Its
base rate is float **0.4 degrees per millisecond** (RVA **0x42e52c**), and its response
is the cube of each component (native powf with exponent float 3.0, **0x42e580**).
The elapsed time used here is the frame's double-precision milliseconds capped above
at **1000.0 ms** (double at **0x437fd8**).

Normally subtract `(rightX cubed × rate)` times elapsed milliseconds from the double
yaw target at G+0x1da8, and subtract the corresponding rightY quantity from the double
pitch target at G+0x1d98. The cube and its product with rate are single-precision; widen
that product to double before multiplying by elapsed time and updating a target. Thus
right-stick right turns yaw toward decreasing angles, and stick up lowers pitch (looks
up in the game's pitch convention). The subsequent camera smoothing is the existing
rule in `frontend-title.md` §3.3.

If aim tracking G+0x1e88 is active and its target G+0x1e58 resolves to a living creature,
let d be the Euclidean distance in blocks from camera eye G+0x1dc0 to the target's
drawn position +0xb10. Convert each fixed-position difference to float, multiply by
1/65536, and take the square root of the sum of squares (0x6d6b0 / 0x9baf0).
Multiply the rate by the larger of **1.0 - d / 50.0** and float **0.3** before using
it. Sources of 50.0 and 0.3: **0x42e5b0**, **0x42e704**. This is the extra scaling
seen in this pass.

When the **weapon-customization panel G+0x368** is visible, the same cubic input
instead adds horizontal rate × time to that widget's +0x1f0 and vertical rate × time
to +0x1e8, narrowing the results to float. These are its model-view angles. It does
not update the camera targets in that branch. The previous label “world-map panel”
for G+0x368 was incorrect: **0xf9ac0** constructs it with **0x292380**, and its draw
**0x292990** identifies the Weapon Customization screen. The actual world-map object
is **G+0x10**, and its interactive-open state is G+0x668 (§4.14).

**Corrections to the earlier broad camera wording:** this right-stick pass does not
multiply by the cameraSpeed option and does not consult invertY. Those options are used
by the separate mouse-input routine **0x97880**, which scales mouse displacement by
cameraSpeed × float 0.005 (0x42e518), reverses its vertical sign when invertY is set,
and clamps its pitch target to 0..180 there. The right-stick pass has no equivalent
pitch clamp in this input portion. This specifies target input; the existing camera
update still controls the displayed angles and smoothing.

### 4.4 UI and interactive-map controls

The UI-input predicate is **0x1014f0**. It reports that one of the front layers, chat,
keyboard-owning panels or in-game panels it enumerates is open; it is not merely the
chat-line flag. When true and interactive map mode is false, the pad drives the following
UI behavior instead of the normal left movement stick.

* **A** becomes mouse button 0, **X** mouse button 1. **Y** additionally becomes mouse
  button 2 in interactive map mode. Each is sent through **0x94900** on its down edge and
  **0x965e0** on its up edge; no repeated mouse-down is synthesized during a hold.
  The underlying GUI root's held bits are set by 0x340ec0 and cleared by 0x3410e0
  (and the corresponding other-button routines). Many widgets activate on release;
  reproducing only a down-edge callback will miss them.
* In ordinary UI mode A's down/up synthesis is allowed only after the UI-mode timer is
  **strictly greater than 200 ms**. The timer resets to zero in gameplay/map
  mode. X's synthesis has no such wait.
  The previous-A and previous-X bits are updated from the actual pad state at the
  end of **every successful poll**, including gameplay and polls within the 200 ms
  gate. They are not updated only when a mouse event is emitted. Therefore an A
  press made before the timer passes 200 ms is consumed as an edge: continuing to
  hold it beyond 200 ms emits **no delayed mouse-down**. Releasing it after the gate
  opens still emits mouse-up, even if that press's down event was suppressed. An
  A hold carried into UI mode from gameplay likewise does not become a new down
  edge on entry or when the timer expires. Release and press again to get a down
  edge. The mouse-up comparison has the same strict >200 ms gate as mouse-down.
* In interactive map mode left shoulder and right shoulder, on down edges, call
  **0x101860 / 0x101790**, respectively: move to the previous/next named-region entry in
  **player character data +0x118**, update its displayed location, and ensure map zoom index is at
  least 4. The always-on right-shoulder Normal Attack held byte of §4.1 is still set.
* In ordinary UI mode, if inventory/crafting/shop panels G+0x330, +0x340, +0x350 are not
  both visible and eligible according to their panel predicate 0x333e70, the right
  stick still feeds the normal right pair through 0xa3900. If at least one is eligible,
  raw right Y with absolute value **> 17378** calls the wheel/zoom routine **0x97cc0**:
  +1 for up, -1 for down. It fires immediately, waits **400 ms** after initial entry,
  then repeats at **100 ms** while held; falling below the threshold resets its repeat
  timer. Raw right X crossing the same threshold acts once on entry for an eligible
  inventory: negative selects the preceding **item-region filter** (0x280170), positive
  the following region (0x27f6a0; exact model in §4.9). This branch does not write a fresh right analog pair.
  The scroll repeat timer is reduced by the successful-poll elapsed milliseconds
  before this test and floored at zero. At zero, emit **at most one scroll action
  in that poll**, then assign a fresh timer of 400 ms if the previous right-Y
  threshold bit was false, or 100 ms if it was true. **Discard overshoot** rather
  than carrying it into the newly assigned timer or emitting catch-up actions.
  Equality with zero permits an action. Right-X/Y threshold bits are updated after
  this branch. Moving directly from strong up to strong down keeps the threshold
  bit true: it does not reset the initial 400 ms delay or create an extra direction
  edge. The next repeated action uses the current sign when the timer reaches zero.
* In those same three eligible panels, the left-trigger crossing above 128 selects the
  previous tab (0x2802f0) and right-trigger crossing above 128 the next tab (0x27f830).
  These are down-edge operations and do not repeat during a hold.

**Left-stick cursor navigation:** raw left X or Y with absolute value **>= 15699**
activates navigation. Below that on both axes resets the repeat timer. If the frame's
GUI navigation-point list G+0x6c8 is empty, offset the cursor each active frame by
raw-X × (1/65536) × 20 horizontally and minus raw-Y × (1/65536) × 20 vertically,
then truncate the resulting pixel coordinates toward zero and set the Windows cursor.
This cursor movement is per frame, not multiplied by elapsed time. With navigation
points available, snap to a candidate in the stick's dominant direction, play sound
0x58 when the selected point actually changes, wait **250 ms** after initial entry,
and then repeat at **100 ms**. Candidate points within squared pixel distance 24 of
the current cursor are excluded. This establishes the thresholds, timing and source
list; the candidate ranking is specified in §4.5 below.

**Last-used-device indicator:** G+0x1958 is set to 1 by mapped pad actions. In the
gameplay/map analog branch it is also set when abs(raw left Y) > 7849 or
abs(raw right X) > 7849; that particular indicator test does not examine left X or
right Y. Sampled nonzero keyboard/mouse bindings set it to zero before the pad pass.
This indicator is not the stick dead-zone test.

### 4.5 Exact GUI navigation-point ranking

Source **0x1355a0**, the ordinary-UI left-stick branch, including its first scan and
fallback at **0x1370f2**. G+0x6c8 is a list of candidate **client-window pixel points**
(i32 x/y pairs), count at +0x6d0. Each scan visits the list in its existing order.
Take the current cursor from the GUI root's float +0x180/+0x184, truncating each
coordinate toward zero to i32. Use this one original cursor point for both scans.
Candidate offsets dx/dy are candidate minus that cursor, in pixels; screen y grows
downward. Exclude every candidate with **dx²+dy² <= 24**.

Compare the **absolute magnitudes of normalized left X and left Y**, retaining the
strict comparisons below. Horizontal input is dominant only when abs(X)>abs(Y),
vertical only when abs(Y)>abs(X). The normalized values here are the UI's unfiltered
raw-component conversions (raw × 1/65536 × 20); the positive common scale does not
change the ordinary dominance comparison.

**First scan:** for horizontal dominance require dx to have the sign of X and
abs(float(dy)/float(dx)) <= **1.0**. For vertical dominance require dy to have the
**opposite sign** to Y and abs(float(dx)/float(dy)) <= 1.0. Thus the preferred cone
extends 45 degrees to either side of the dominant axis. For each accepted candidate,
its score is **forward pixel distance + 100 × perpendicular pixel distance**, where
both distances are absolute values (horizontal: abs(dx)+100×abs(dy); vertical:
abs(dy)+100×abs(dx)). Choose the smallest score. Equal scores retain the candidate
encountered first; selection replacement uses strict less-than.

**Fallback scan:** only if the first scan selected nothing, rescan the same point
list from the start. Require the same forward sign on the dominant axis but omit
the perpendicular/forward ratio test. Score is now **forward distance + 10 ×
perpendicular distance**, again smallest first and first encountered on a tie.
There is no wrap to a candidate behind the cursor when none is forward.

**Exact diagonal exception:** when abs(X)==abs(Y), neither dominance branch runs.
The native first scan applies neither sign nor cone restriction and gives every
candidate beyond the exclusion radius the same score **101000**. It therefore picks
the **first such point in list order**, regardless of its direction or distance.
Do not choose a diagonal axis by an invented tie-break rule. The fallback's analogous
default score is 11000, although with consistent point-list state a diagonal first
scan already finds any point that the fallback could consider.

On success snap to the selected point and play sound **0x58**. If both scans find
nothing, leave the cursor at its original truncated point and emit no navigation
sound. The repeat timer is still assigned after an unsuccessful scan: **250 ms**
on the first active navigation scan since recentering, **100 ms** afterwards. The
first-active latch clears after that scan regardless of success. Like scroll, this
timer subtracts elapsed milliseconds, floors at zero, emits at most one navigation
scan per poll, and **discards repeat overshoot**. Changing stick direction while
remaining outside the active threshold does not by itself restart the first delay.

When candidates exist but the repeat timer has not reached zero, the UI still sets
the cursor to its original truncated pixel coordinates in this branch; it does not
perform the empty-list continuous movement. When the point list is empty, continuous
cursor motion follows §4.4 and does not itself clear the first-navigation latch.
Recentering both raw axes below 15699 sets that latch and repeat timer back to their
initial states.

The cursor operation at the end of this branch is **ClientToScreen followed by
SetCursorPos only**. It does not write the GUI root's float cursor fields or dispatch
another mouse-move update. The A/X synthesized events occur **after** that OS cursor
operation in the native loop, but still hit-test using the unchanged GUI cursor
floats from before navigation. Consequently an A release in the same poll must not
activate the newly snapped point merely because SetCursorPos just moved there.
The new OS position becomes GUI input through the ordinary subsequent mouse-input
processing; navigation does not itself update the GUI hit-test cursor synchronously.

The input timers above use elapsed **integer milliseconds between successful
XInput polls** (timeGetTime subtraction), not a fixed frame duration. The timestamp
is not refreshed on a failed poll; the next successful poll includes that interval.
The ordinary-UI >200 ms timer advances on successful polls in that mode and is reset
in the gameplay/map branch, as specified above.
On the **first successful poll**, a guarded static initialization first samples
timeGetTime into the previous-poll timestamp; the loop then samples timeGetTime
again for the current poll and subtracts. Thus first-poll elapsed time is normally
zero or a few milliseconds, not time since program start. The cached timestamp
changes only on successful polls. Subtraction is DWORD wrap subtraction; its result
is then used as a signed 32-bit millisecond count by the timer operations. No frame-dt
or maximum-elapsed clamp is applied to this successful-poll timing. Previous A/X,
scroll threshold and trigger bits also remain unchanged across failed polls.

Verification limits: the fixed mapping, radial filters, camera arithmetic, exact
candidate ranking, and listed edge/repeat timing are verified from exports and
binary constants. No new controller capture was run.

### 4.6 Navigation-point registration and the title root menu

The list is explicitly rebuilt per frame: **0xa5a50** empties G+0x6c8 at frame start
with **0x3c9c0**. Interface update/drawing then appends points through **0xa1990** or
the equivalent direct node append **0x6ece0**. Several GUI routines take the interface
object at **G+0x118**; their list offset +0x5b0 is therefore the **same G+0x6c8 list**,
not a separate navigation list.

The registration helper **0xf97e0** takes that interface object, a widget, and an
integer x/y inside the widget. It applies the widget/node placement and transform,
including its frame transform, divides by the resulting homogeneous coordinate,
truncates the resulting client-window x/y toward zero, and appends that pixel point.
It does not itself deduplicate or sort the list. Source call sites include the
start menu **0x291210**, system menu **0x291e00**, answer-choice panel **0x28ee60**
and other manually drawn widgets. Options **0x288c50** also appends centers obtained
from the bounds of its child widgets; controls **0x276200** and inventory-related
widgets append their own points. The equipment-slot update in **0x103200** appends
centers to this same list while its panel is shown.

**The manual title root menu does register points.** Its draw routine **0x291210**
calls 0xf97e0 in this order, at x = integer truncation of half the root widget width:

| Entry | Local navigation y | Registration condition |
|---|---|---|
| Start Game | 25 | always when this widget is drawn |
| Continue | 75 | only when a selected saved character exists |
| Options | c-5 | always; c=130 with Continue, otherwise c=80 |
| Exit | c+45 | always, with the same c |

These are widget-local points before 0xf97e0's placement/GUI transform. The root's
appearance and hover bands remain those in `frontend-title.md` §4.1. Once its visible
frame has been drawn, a title root menu therefore supplies at least **three** points
and uses the snapping/ranking path, not continuous empty-list cursor motion. A first
poll before any completed point-producing frame, or a screen that actually registers
no points, can still use the empty-list fallback. Do not infer an empty list merely
because the root is a manually drawn menu rather than ordinary button widgets.

The system menu likewise registers its four points at local half-width and y values
**25,55,85,115**, in entry order. Registration order matters for equal navigation
scores and the exact-diagonal case in §4.5.

### 4.7 Controller state storage and panel routing

Verified from **0x1355a0** and the owner's image initialization data:

| State | RVA | Storage and initial value |
|---|---|---|
| Ordinary-UI A gate accumulator | 0x551cc0 | signed i32 milliseconds, initially 0 |
| Previous successful-poll timestamp | 0x551cc4 | DWORD, initialized on first successful poll as in §4.5 |
| Previous right-X / right-Y over-threshold states | 0x551ccc / 0x551ccd | one byte each, initially 0 |
| Scroll countdown | 0x551cd0 | signed i32 milliseconds, initially 0 |
| Previous left / right trigger over-128 states | 0x551cd4 / 0x551cd5 | one byte each, initially 0 |
| Navigation countdown | 0x551cd8 | signed i32 milliseconds, initially 0 |
| First-navigation latch | 0x537288 | one byte, initially **1** |
| Previous map-mode Y / left shoulder / right shoulder | 0x551cdc / 0x551cdd / 0x551cde | one byte each, initially 0 |

The accumulator and countdowns are **integers**, not floats. Their arithmetic and
comparisons use 32-bit signed values; no saturation/clamping of the growing A-gate
accumulator occurs. Scroll/navigation subtraction is floored at zero as already
specified. Navigation subtraction occurs on **every successful ordinary-UI poll**,
including when the point list is empty. Empty-list movement leaves the first latch
unchanged, but does not freeze the countdown.

The right-X/Y threshold bytes and trigger bytes update on **every successful
ordinary-UI poll**, including when none of the inventory/crafting/shop panels is
eligible. Accordingly a trigger hold begun over an ineligible part of the UI is
consumed; moving into an eligible panel during that hold creates no new tab edge.
Likewise a right-X hold begun while ineligible does not acquire a new region-filter edge
when eligibility changes. The scroll countdown is explicitly zeroed when all three
panels are ineligible, and is also zeroed whenever right Y is not above its threshold.
A continued right-Y hold can therefore scroll immediately when a panel becomes
eligible, using **100 ms**, rather than 400 ms, if its threshold byte was already set.

Outside ordinary UI mode (gameplay or interactive map mode), none of those right-X/Y bytes,
trigger bytes, scroll/navigation countdowns or first-navigation latch is updated or
reset. The A-gate accumulator alone is reset there. Failed polls also leave all those
states untouched. Centering the left stick while outside ordinary UI does not rearm
the first-navigation latch; releasing a trigger there does not clear its saved edge.
On re-entry the previous values resume, with the next successful poll's elapsed time
applied normally. This differs from the A/X bits, which update in all successful polls.
The saved **Y and map-mode shoulder bits update only in the interactive-map branch**;
ordinary UI and gameplay leave them untouched. In particular Y is not updated globally
like A/X. A release outside interactive map mode does not clear its saved map-mode bit.

**Panel eligibility is cursor-sensitive.** The checks combine the panel root's
nonzero current visibility state with **0x333e70**, which obtains the GUI root's
current float cursor +0x180/+0x184 and asks **0x333f60** whether that panel's node tree
has a hit at that point. Thus a merely open panel need not be eligible: the cursor
must hit its GUI tree, respecting its node visibility, hit-test flags, transforms and
clipping. This is evaluated before the left-stick cursor movement in this poll.

For right-Y wheel actions **0x97cc0** chooses **one** eligible target in this priority:
**inventory G+0x330, shop G+0x350, crafting G+0x340**. Positive Y moves the chosen
panel's row offset backward by one via **0x280270**; negative Y moves it forward by
one via **0x27f7a0**. Those routines clamp at the available rows and refresh the
selection/UI; an actual row change emits sound **0x59**. These are scroll rows,
not category changes. The wheel routine has further fallbacks, but the pad's wheel
entry condition requires at least one of these three eligible panels.

Right-X region-filter changes test **inventory only**, even when shop or crafting made
the wheel branch eligible. The region handlers wrap through its stored region
pairs; they do not change the other panels' filters. Trigger tab changes instead
test all three panels **independently**, in order **inventory, crafting, shop**, and
call the corresponding previous/next-tab handler on every eligible one. There is no
first-panel-wins rule for triggers. If both trigger edges occur in one poll, all
left-trigger previous-tab actions occur before all right-trigger next-tab actions.

### 4.8 Frontend mouse-button event phases

The GUI dispatch in **0x340ec0** associates left-button **down** with event id **2**;
**0x3410e0** associates left-button **up** with event id **3**. These are the paths
reached through 0x94900 / 0x965e0. “Press event” in an older frontend spec must not be
read as universally meaning down. Verified registration/consumer phases are:

| Control | Activation phase | Native source |
|---|---|---|
| System menu Options / Controls / Start Menu / Exit Game | **up** | 0x291e00 registers handler 0x292300 for event 3 |
| Character-select Select/Create | **up** | 0xf9ac0 registers 0x9f7c0 for event 3 |
| Character-select delete toggle and Delete Character | **up** | 0xf9ac0, handlers 0x9f600 / 0x9dea0, event 3 |
| Shared frontend Back button | **down** | 0xf9ac0 registers 0x9ddb0 for event 2 |
| Creation Create Character button | **up** | 0xf9ac0 registers 0x9ea10 for event 3 |
| Creation style previous/next arrows | **down** | 0x26fda0 binds its arrow handlers for event 2 |
| Creation hair-colour grid: commit hovered preview colour | **down** | 0x26fda0 binds 0x272470 for event 2; it copies preview RGB to committed RGB and applies the look |
| Options value arrows | **down** | 0x287050 binds left/right handlers for event 2 |
| Options Apply / OK / Cancel | **up** | 0x287050 binds 0x28b140 / 0x28b350 / 0x276a00 for event 3 |
| Controls Defaults / OK / Cancel | **up** | 0x275a70 binds 0x276a30 / 0x276c80 / 0x276a00 for event 3 |
| Controls binding row: enter key-capture waiting state | **up** | 0x965e0 calls 0x276f30 using hovered row +0x2dc when Controls is visible and button id is 0 |

The normal GUI release dispatch does not require a matching prior synthesized down
edge before it delivers event 3 to the current hit/captured target. Therefore the
suppressed-down A case in §4.4 can still activate a release-bound button when A is
released after 200 ms over that button. Keep actual down/up routing and the control's
own phase; do not impose a universal down phase or invent an extra paired-down rule.

### 4.9 Inventory region filter and tab handlers

**Correction of terminology:** the i32 pair at InventoryWidget **+0x1f0/+0x1f4**
means **item region x/y**, in the same region-coordinate units as item +0xc/+0x10 and
player +0x150/+0x154. It is not an item-type/category selector. The six inventory tabs
remain **Equipment, Specials, Items, Ingredients, Pets, Artifacts**, indices 0..5
(**0x27d2b0**). The widget's selected tab is signed i32 +0x2b4, initially 0.

**Region choices, 0x2806a0:** for the ordinary inventory widget (mode +0x280 == 0),
rebuild its choice vector +0x1f8 from the region pair of **each nonzero-count slot in
the currently selected player inventory tab**, and always include the player's current
region. Duplicate pairs collapse. The intermediate set is signed lexicographic x/y
order (**0x745e0**). A subsequent native sort (**0x27c420**, comparison visible in
**0x27b780**) orders by ascending **Chebyshev distance to the player's region**, i.e.
the greater of absolute x difference and absolute y difference. There is **no second
comparison key** for equal distance. Up to 32 choices the insertion-sort path preserves
their intermediate lexicographic order on ties. For more than 32 choices the native
sort is unstable; this trace does not specify the permutation of distance ties.
The vector is rebuilt during the normal widget refresh **0x280a40**.

Opening the inventory through **0x102fe0** resets the active region pair to the
player's current region and refreshes it. Changing tabs or rebuilding choices does
not itself reset that pair. Region-filtering of the displayed/countable entries
applies only to ordinary inventory **tabs 0 and 1**: retain slots whose region equals
the active pair. The other inventory tabs, shop and crafting do not use this filter
for their displayed slot count. The mouse arrow visibility in 0x27d2b0 is limited to
inventory tabs below 2; the pad's right-X edge test does **not** impose that tab test.

**Right X, 0x280170 / 0x27f6a0:** find the active pair in the stored choice vector and
move to the previous/next entry, wrapping at the ends. If there are no choices or no
matching current pair, leave the pair unchanged. Each handler still emits sound
**0x59** and refreshes the row limits/scrollbar. The next-region handler explicitly
refreshes/sorts the player's inventory before the normal refresh; the previous handler
reaches that refresh through 0x280a40. Neither handler changes selected tab, resets its
stored row offset, or directly assigns a different selected-slot pair.

The sort invoked by refresh (**0xb3ed0**, comparators **0x85770 / 0x83d90**) brings
matching-region nonempty slots to the front of inventory tabs 0/1 before the display
stops at the first other-region item. Thus a simple type-category change is not an
equivalent substitute for this filter.

**Trigger tabs, 0x2802f0 / 0x27f830:** decrement/increment the selected tab and wrap
through the widget's own **tab-label count +0x268**. This is the tab count of that
particular widget, not always six. Refresh row limits and its existing per-tab signed
row offsets (vector +0x288). Select **(-1,-1)** when the new tab's backing slot vector
is empty or its row-offset entry is unavailable; otherwise select **(new tab,
rowOffset × columnCount)** in +0x278/+0x27c. Column count is integer truncation of
**(widget width - 10)/(cell width + 5)**. Refresh the selection frame and scrollbar,
emit sound **0x59**, then call **0x9de90**. That last helper does work only for the
shop widget: it rebuilds its current tab's goods, including the buyback-list case;
the existing shop-stock rules remain those of `loot-shops.md`.

Row scrolling via **0x27f7a0 / 0x280270** increments/decrements only the current tab's
stored row offset within 0..(availableRowOffsetCount-1). An actual change updates the
selection frame and scrollbar and sounds 0x59; a boundary attempt does not. Row-limit
refresh may clamp an old row offset downward when fewer rows remain; it does not
unconditionally reset every tab to row zero. Region selection, selected tab, row offset
and selected slot are separate state, with the transitions above.

### 4.10 Controls capture includes the next physical left-button down

Sources **0x1355a0**, **0x2769a0**, **0x2761f0 / 0x276f30**, **0x965e0**. Waiting-row
+0x2e0 is signed i32, initially -1. A physical left-button **release over a hovered
binding row** first clears any old waiting row, then assigns the hovered row to the
waiting row. Its selecting down is already past. The next fresh physical left-button
down is therefore a valid binding input; there is **no blanket exclusion of mouse
button 0** while waiting.

Actual capture uses sampled **DirectInput rising states**: current button/key byte
nonzero, saved previous byte zero. It is not performed from the left-button-up GUI
callback. Mouse indices **0,1,2,3** are all accepted, with the binding stored as
**(button index, device kind 1)**. All rising mouse buttons are visited in ascending
index order, so if several qualify the later index can overwrite the earlier binding.
**0x2769a0** first clears any other binding equal to the new pair to (0,0), writes
the pair into the waiting row, and clears hovered-row +0x2dc to -1. It does not itself
clear waiting-row +0x2e0 for a mouse capture. The normal mouse-up handler clears the
waiting row on **every mouse-button up**, before its separate left-row activation test.
While waiting, the draw routine **0x276200** does not set a hovered binding row, so
this finishing release normally does not rearm the row just captured.
That draw routine also clears waiting when the last-used-device indicator G+0x1958
is nonzero, and only assigns row hover while that indicator says mouse/keyboard.

Keyboard capture uses the 0..255 scan-code array, also on a sampled rising state.
Scan codes **1,0x1c,0x9c** (Escape, Enter, keypad Enter) finish waiting without assigning
a binding. Other scan codes store **(scan code, device kind 0)**. The keyboard capture
path clears waiting after the first new key. Existing held inputs do not become new
captures merely because waiting has just begun; saved prior sampled states are retained.
The GUI visibility/layout rules and applying the local binding copy with Controls OK
remain those of `frontend-options.md`.

While a waiting row is active, the main loop takes this capture branch instead of
its normal binding and XInput branch. It does **not call XInputGetState there**, update
pad previous-edge bytes, or refresh the successful-XInput timestamp. Resuming the next
successful pad poll therefore includes that interval in the timestamp subtraction.
No gamepad button is translated into a keyboard/mouse binding by this capture path.

### 4.11 Exact panel-tree hit eligibility

The existing foundation is **`plasma-format-and-scene.md` §2.3.4–§2.3.5** (node
flags and point picking), **§2.7.2** (frame geometry) and **§2.7.4** (inverse widget
deformation). The following details were rechecked directly against **0x333e70 /
0x333f60**, including the latter's assembly. They apply to the panel eligibility
checks in §4.7 with **hit-test options = 0**.

Eligibility asks for any hit **starting at that panel's node subtree**, using the
GUI root's current float cursor in client pixels. It does not start at the whole
screen root and then compare the topmost global hit with the panel. Consequently
another panel drawing on top does not by itself make this panel ineligible; overlapping
panel trees can each qualify independently.

* A node whose current `Display.visibility` is zero, or whose node flags include
  **0x0004 or 0x1000**, rejects its **whole subtree**.
* Node **0x0002** prevents returning that node itself for options 0. Its descendants
  can still supply a hit. Node bits **0x0001 and 0x0008 do not exclude** a node for
  options 0; they matter only when caller options 1 or 4 are supplied, respectively.
* A node with its own widget having an allocated own surface (widget +0x58 nonzero),
  with GUI engine option **+0x288 bit 0x2** enabled, first applies frame clipping unless
  widget flag **0x100** is set. Get frame origin from **0x32cf50**, transformed frame
  size from **0x32d010**; reject x below origin, y below origin, x **above** origin+size,
  or y **above** origin+size. All four exact frame edges survive this preliminary
  check. Use those two returned components directly, not a replacement rotated-corner
  bounding box or an invented clipping margin.
* For geometric testing, transform the original client point by that node's **inverse
  world matrix**, including homogeneous w division. If it has an owner widget and a
  deformable shape, apply that owner's inverse nine-slice deformation **0x32d2e0**
  before testing the shape. Ordinary shape-free nodes are not automatically rectangles.
* A node's local hit is true if its shape contains the transformed point, its own
  widget's hit callback accepts the original node-local point, or its own widget flag
  **0x100** is set. In this last case the native hit is **unconditional** at that node:
  the flag also bypassed the preliminary frame clipping. The older “whole frame counts
  as a hit” wording must not be implemented as an extra rectangle test here. The
  ordinary Widget/game GUI widget callback reports false; visible shape geometry is
  normally what makes their panel backgrounds hit.
* A nonzero current `Display.clipping` permits searching descendants only when the
  node's own local hit is true. A non-clip node searches descendants even if its own
  geometry was missed. Descendants are visited **last child first**, recursively;
  the first returned hit wins. Only after none returns does the node itself qualify,
  requiring its local hit and the node's self filters above.

**Shape containment is not generally the bounds rectangle.** For shipped GUI
SmoothMeshShape, **0x35fd00** tests the available fill drawing unless shape flag
0x4 disables fill, then stroke drawing if flag 0x2 enables it, then extrusion drawing
if flag 0x10 enables it. Any accepted drawing suffices. Drawing containment
**0x375880** first rejects outside its cached min/max box (edges inclusive), then
uses **even/odd crossings across all its closed outline loops**. A crossing counts
only where **low edge y < pointer y <= high edge y**, and where the edge intersection
is **strictly left of pointer x**. Horizontal edges do not count. Odd total is inside;
there is no extra epsilon or universal “point on edge is inside” rule. An outline
vertex index at least the drawing's vertex count rejects the drawing. A plain axis-aligned rectangle therefore
normally excludes its exact top/left edges and includes its bottom/right edges under
this parity test, despite the preliminary bounds test accepting all edges.

TextShape's containment at **0x368430** instead accepts its cached min/max rectangle
inclusively. GenericShape returns false for containment, though it reports deformable.
These follow-up facts refine the broad “slot 4 = cached bounds” summary in
`plasma-shapes-and-text.md` §0: SmoothMeshShape's slot delegates to actual drawing
outlines and cannot be replaced with the node's union bounds.

The engine constructor **0x339b70** initializes +0x288 to **7**, so the widget-frame
clipping bit **0x2 is enabled by default** (bit index 1, not mask 0x4). The game uses
the derived engine constructor **0x38e100**, which calls that base constructor.

The loops consumed by drawing containment are produced by **0x3751b0** from that
**final drawing's triangle index buffer**, after the drawing has been built. Opposite
directed edges cancel by **vertex indices**; the remaining boundary edges are linked
into contour lists. On an ordinary consistently wound manifold drawing these are its
free triangle edges. This is not a test against the original unsmoothed curve, nor
does extraction weld distinct vertices merely because their positions coincide.
The ray-crossing test closes each extracted list. With a choice of outgoing edges,
the extraction consumes them in its stored order; it is not a geometric angular sort.

The shipped GUI SmoothMeshShape flag values listed in `plasma-shapes-and-text.md`
§1.2 are **0,1,2,3,6,14** (decimal). None has extrusion mask **0x10**, consistent with
that spec's §3.4 finding that shipped files do not use extrusion. Decimal 14 is 0x0e,
not 0x14. Supporting the verified fill/stroke geometry covers these shipped GUI shapes;
the extrusion test above describes the native generic path when such a drawing exists.

### 4.12 Registration for the remaining requested screens

Registration is **explicit**. Native ordinary Button/Edit nodes do not automatically
add navigation points merely because they are visible or interactive. The shared list
contains all points actually appended by the frame's update/draw routines, in that
order; it is not restricted to one modal/eligible panel and has no deduplication.

**Character selection/creation:** the character-preview draw **0x26eba0**, creation
style draw **0x271030**, and the corresponding parts of interface update **0x103200**
do not register navigation points. This includes the character entry rectangles,
Select/Create/delete controls, creation arrows, hair-colour cells, name edit, Create
Character and shared Back button. Therefore a character screen shown by itself uses
the **empty-list continuous cursor fallback** of §4.4. If another visible widget
registers points, those are still in the shared list and can change that outcome.
This is a different native registration situation from the title root in §4.6.

**Options, 0x288c50:** before drawing its text, visit the Options root node's **direct
child list from first to last**. For each child, obtain its transformed geometry
min/max bounds with **0x332ad0** and append the i32 truncation of **(minimum+maximum)/2**
for each axis. The registration loop has no additional per-child visibility,
interactivity or hit-test filter and does not recurse itself. Bounds collection can
include that child's visible descendants according to the existing scene bounds rules.
Use the actual instantiated child list rather than every text label or clickable row.
**0x287050** creates the 15 left/right arrow pairs in row order, left then right, and
then Apply, OK, Cancel, preserving that construction order in the child list; any
children inherited from the `blackwidget` template retain their existing list order.

**Controls, 0x276200:** registers exactly the centers of the transformed bounds
(0x332ad0), in order **Defaults (+0x2b8), OK (+0x2a8), Cancel (+0x2b0)**. It does not
register binding rows, row text, scrollbar, or scroll arrows. These points are appended
even while a binding waits. Row capture has the separate mouse behavior in §4.10.

**Inventory/crafting-list/shop grids, shared 0x27d2b0:** with a valid selected tab and
backing list, register the icon point of **every viewport cell in row-major order**,
including cells without an item. This loop's end is viewport capacity, not the number
of nonzero slots or the count surviving the inventory region filter. It adds no
separate tab-label, region-arrow or scrollbar point. The relevant signed fields are
cell width +0x2a8, cell height +0x2ac and current tab's row offset from +0x288.

For exact placement let W/H be the widget's effective width/height, O its client frame
origin (0x32cf50), and S the x scale M[0,0] of its node's drawing matrix. Columns are
integer truncation of **(W-10)/(cellWidth+5)**; visible rows are truncation of
**(H-40)/(cellHeight+5)**. A viewport cell at zero-based column c, row r has icon
top-left at the per-coordinate integer truncations of
**Ox + S×(10+c×(cellWidth+5))**, **Oy + S×(40+r×(cellHeight+5))**.
Its navigation point adds **S×trunc(cellHeight/2)** to **both** of those truncated
coordinates, and truncates again. In particular a wide vendor cell's x point is at
the item icon, not the center of the whole wide slot. Source constants 10 and 40:
**0x42e598 / 0x42e5ac**. Invalid/null backing data produces no grid points.

**Equipped slots, 0x103200:** when inventory G+0x330 is visible, append all ten slot
frame centers, even empty slots, from the interface's stored equipment-widget vector.
Their order corresponds to creature equipment slots **6,7,8,9,1,5,2,4,3,10** (item
records at creature +0x278+slot×0xa0). For each widget use **frame origin 0x32cf50 plus
half transformed frame size 0x32d010**, then truncate both components to i32. This
is frame geometry, rather than the shape union bounds used by Options/Controls.

**Craft recipe detail G+0x348, 0x26c2d0:** if its selected result item cannot resolve,
the draw returns before adding points. Otherwise append every ingredient icon center
in ingredient-list order, then the craft-button frame center when its button exists.
Ingredient availability or a missing required station does not suppress these points;
the craft button still gets its point when unavailable. There is no result-model point.

For its exact ingredient placement, let O/S be the detail widget's client frame origin
and node x scale, H its effective height, and Wp its parent-relative width (0x32da30).
The native column count is truncation of **(Wp-10)/45**, even though horizontal icon
spacing is **70**. Ingredient i's icon top-left is truncated from
**Ox+S×(20+70×(i mod columns))** and
**Oy+S×(H-130+10+45×trunc(i/columns))**. Its navigation point then adds **20×S** on each
axis and truncates again. These are centers of the drawn 40-unit icons. The craft
button point uses its own frame origin plus half its transformed frame size, truncating
once at the end. Sources: **0x26c2d0**, constants **0x42e5a0=20**, **0x42e7b0=45**,
**0x42e7cc=130**.

### 4.13 Exact pad writes to the last-used-device indicator

Source **0x1355a0**, writes to byte **G+0x1958**. In addition to the keyboard/mouse
reset and gameplay/map analog thresholds already in §4.4, its UI-related
writes of **1 = pad** are precisely:

| Situation | Sets pad indicator |
|---|---|
| D-pad Up/Down, Back, Start, right shoulder, or right trigger >128 | **Yes on every successful held poll**, before UI/gameplay routing; these are the always-on mappings of §4.1 |
| Ordinary-UI left stick at its navigation threshold | **Yes every active poll**, even with a running countdown, no usable candidate, an empty point list, or no resulting pixel movement |
| Ordinary-UI A down/up actually passes the >200 ms gate and calls its mouse handler | **Yes**, immediately after the handler returns |
| Ordinary-UI A edge suppressed by the gate, or unchanged A hold | **No write from that A test** |
| Ordinary-UI X synthesized down/up | **Yes**, immediately after the handler returns; an unchanged hold does not write |
| Interactive-map A/X/Y synthesized down/up | **Yes**, immediately after the handler returns; unchanged holds do not write |
| Interactive-map left/right shoulder down-edge list navigation | **Yes**; right shoulder also has the always-on held write above |
| Ordinary-UI right-Y wheel action | **No explicit write** |
| Ordinary-UI right-X region-filter action | **No explicit write** |
| Ordinary-UI trigger tab handler | **No explicit write by the tab handler**; the right trigger already caused its always-on write, the left trigger did not |
| Ordinary-UI right analog pair forwarded because no panel is eligible | **No analog-indicator write in that UI branch** |
| B closing panels/interactive map through the UI dismissal branch | **No explicit write by dismissal** |

“No write” preserves the existing indicator, rather than setting it to mouse. Other
simultaneous controls can still set it as shown. Neither a failed poll nor the
Controls capture branch writes pad state here. In particular a pad A release can
enter a Controls row through the mouse-up callback, then mark pad immediately after
that callback; Controls' next draw clears that wait as in §4.10. Updating the device
indicator only on navigation movement, or marking every right-stick/tab operation
uniformly, changes this native behavior.

### 4.14 G+0x668 denotes the full interactive map, independent of Talk

**Explicit engine-facing mapping:** map the engine's **full `mapOpen` state** to
**G+0x668**. A normal full map opened with the map binding/menu is sufficient; flight
or shrine mode is **not** required. The always-visible corner minimap does not set
this state. Keep **flight selection** separate (WorldMap at G+0x10, its byte +0x164),
and **shrine-choice mode** separate (G+0x669). An NPC/object Talk-active or from-object
flag is not a substitute for any of those map states.

The assignment trace is decisive:

| Source | Verified effect on map state |
|---|---|
| Interface constructor 0xf8690 | initializes interface +0x550/+0x551, hence G+0x668/+0x669, to zero |
| Ordinary map toggle 0x1031e0 | toggles interface +0x550 = **G+0x668**; no flight/shrine condition |
| 0xa5a50, WorldMap binding index 15 rising edge | calls 0x1031e0 through the interface at G+0x118 |
| Interface setup 0xf9ac0 | binds the `worldmap-menu` button to that same 0x1031e0 toggle |
| Flight-master use in 0x98180 (NPC role 0x9a) | sets G+0x668 to 1 and WorldMap +0x164 to 1 |
| Shrine creature use in 0x98180 (race 0x10c), or kind-13 use in 0xb7740 | sets **both** G+0x668 and G+0x669 to 1 |
| Interface update 0x103200 with G+0x668 clear | clears G+0x669; the frame also clears WorldMap flight mode when the full map is closed |
| B UI dismissal in 0x1355a0, Start Menu 0x9fdf0 | clears G+0x668 |
| A successful flight launch in 0x94900, or accepted shrine-target action in 0x965e0 | clears G+0x668 as the corresponding travel starts |

The consumers agree with these assignments: the map overlay **0x2829c0** performs
interactive marker hovering only while G+0x668 is set and the map is not dragged;
the world-map frame changes corner/full placement and size using this byte
(`worldmap-rendering.md` §4.5). The mouse input **0x97880** uses it for map dragging.
Player control **0x9c050** stops its ordinary movement pass when this byte and the pad
indicator are both nonzero. The analog/control-state writes in §4 therefore do not
mean the player should keep walking while a pad is operating the full map.

The shoulder functions **0x101860 / 0x101790** operate on the current UI list node
at interface +0x5c8 in the player's character-data list **+0x118**. They update the
displayed **region name** (0x1081b0), obtain that region's generated block position
(0x2b9210), set the map's target centre (0xa3390), and raise zoom to at least 4. These
are map-region navigation operations. They are not dialogue-page or conversation-
choice navigation. `native-saves.md` field 28 describes this serialized region list;
its broader gameplay provenance remains separate from the verified map use.

**What makes 0x1014f0 true:** it explicitly includes this full-map byte, so an
ordinary open map is a UI-input state even without flight/shrine selection. It does
**not** test the ordinary NPC/object speaker-bubble vector **G+0x400**, or the general
lore/item-receipt DialogWidget **G+0x418**. Either can display speech without making
0x1014f0 true by itself. The separate **answer-choice panel G+0x378 is tested by its
visibility**, so that panel can cause ordinary UI routing when the map is closed.

Normal NPC speech construction **0xa3170** creates/reuses speaker widgets, attaches
compiled pages and speaker/recipient ids, and does not set G+0x668. The ordinary
speaker path in **0x98180** likewise proceeds through 0xa3170 without setting the map
flag; the special flight-master branch is an explicit separate case. Lore and item
receipt pages use their own general dialogue panel. Thus route pad input from the
actual UI predicate and full-map state, not from “any Talk is active,” nor from
“Talk came from an object.” Existing dialogue advancement remains governed by the
speech/page consumers in §§3 and 5.

### 4.15 Row-scroll storage, clamping, and selected item

Sources: **0x27f7a0** (next row), **0x280270** (previous row), **0x280a40**
(refresh), **0x280e30** (selection frame), and **0x280c80** (scrollbar).
Each inventory/list/shop widget owns a vector of **signed i32 row offsets** at
**+0x288 / +0x290 / +0x298**, one entry per backing tab. This is persistent
**per-widget, per-tab** state. In contrast, the selected item pair **+0x278 / +0x27c**
is a single widget-wide **(tab, backing-slot index)** pair, not a selected item per
tab. The available-row-offset count **+0x2a4** is one signed i32 for the current tab.

Both scroll handlers first refresh the widget. With positive column count, refresh
resizes the row-offset vector to the backing tab count: existing entries survive,
new entries are zero. For a valid current tab with a completely empty backing slot
vector, set available-row-offset count to **0** and that tab's row offset to **0**.
Otherwise let C be truncation of **(effective width−10)/(cell width+5)**, and R be
truncation of **(effective height−40)/(cell height+5)**. For ordinary inventory tabs
0/1, N counts only nonzero-count slots matching the selected region. For other tabs
and for crafting/shop, N is the backing slot-vector length. The available offset
count is **max(1, truncate((N−1)/C) − R + 2)**. Clamp only the current tab's offset
downward to **count−1** if it exceeds that value. Other tabs' stored offsets are not
clamped until refreshed as current. A missing/invalid current tab sets the count
to **1**; a nonpositive C leaves the existing offset vector/count untouched.

After that refresh, next-row increments the current stored offset only below
**count−1**; previous-row decrements only above **0**. An actual increment/decrement
emits sound **0x59**, updates the selection frame, and updates the scrollbar.
The scroll itself **does not assign +0x278 or +0x27c**, select the first newly shown
slot, or preserve a selected viewport-cell position by changing its backing index.
The previously selected backing item remains selected; its selection frame is
hidden if its index lies outside the newly visible row range. The selection-frame
helper used here has its adopt-hover argument **false**. Its visibility test uses
the stored backing index against the viewport range; it does not separately test
that the item is nonempty or that the stored selected-tab field equals the current
tab. A boundary attempt causes
no scroll sound or selection-frame/scrollbar call, although its initial refresh
can already have sorted backing entries or silently clamped an obsolete offset.

### 4.16 The bounds used by Options and Controls navigation

Sources: **0x332ad0 / 0x332300**, **0x35f570 / 0x375750** (SmoothMeshShape),
**0x368c60** (TextShape), and the owner's shipped **gui.plx**. This is distinct
from the frame rectangles of §4.12 and the hit-test geometry of §4.11.

0x332ad0 starts with the complete ancestor world matrix and includes the queried
node's own local matrix. It unions that node's own shape and the eligible child
subtrees. Descendant matrices accumulate in child-list order, and transformed
points undergo homogeneous division. The resulting minimum/maximum are in the
GUI world's coordinate system, with the GUI root's scale/translation included.
There is no renderer's −0.135-pixel adjustment here.

**The queried starting node is not filtered by its own visibility or node flags.**
Its own shape can therefore contribute even if that starting node is hidden,
disabled, or non-interactive. This matters because the Options loop calls the
helper on every direct child without filtering. During recursion, a child is
skipped with its whole subtree when its current visibility is zero or its node
flags contain **0x4**. Node **0x2** and the hit-test flag **0x1000** are not bounds
filters. Fill alpha and hit-test eligibility are not filters either.

A node whose current **Display.clipping is nonzero** contributes only its own
shape: bounds recursion stops at that node, rather than intersecting its children's
boxes with its clipping outline. Otherwise union the eligible children with the
own-shape bounds. This wrapper enables blur expansion. At each node with display
flags bit 0 set and positive blur radius, expand all four bounds edges by
**0.5 × (length of transformed x axis + length of transformed y axis) × radius**.
This bounds test does not additionally require the GUI engine's blur rendering
option to be enabled.

For **SmoothMeshShape**, bounds come from **every vertex of its active Drawing
position arrays**, transformed by the accumulated matrix. Union fill when shape
flag **0x4 is clear**, stroke when flag **0x2 is set**, and extrusion when flag
**0x10 is set**, provided those drawings exist. This is the final drawing geometry,
not the widget frame, the original mesh's control-point box, or only its outline
loops. **This bounds path does not apply widget nine-slice deformation.** The
Drawing vertices are measured before the separate GPU deformation used for
drawing, even when a widget's frame size differs from its bind size. Node movement
and resizing-induced child placement still affect their accumulated matrices.

For **TextShape**, 0x368c60 refreshes its cached local bounds for the supplied matrix
only when a font exists and its scalable-font mode is enabled; otherwise the cache
is retained. Then transform the four corners of that cached rectangle and
take their axis-aligned union. This uses the current whole string, font, flags,
alignment and scalable-font box-mode measurement from
`plasma-shapes-and-text.md` §6.7, including its **S/Sx, S/Sy** scaling. It does not
substitute the drawn pen-mode bounds or the widget frame. Text is not nine-slice
deformed. Other shapes without a specialized transformed-bounds method similarly
use their local min/max rectangle's four transformed corners.

The Options root is a copied **blackwidget**. In the shipped gui.plx, template
node **136** has exactly one direct child: node **137**, named **`Shape`**, with a
SmoothMeshShape. It is retained on instancing and is **the first Options navigation
point**, before the 30 arrow widgets and Apply/OK/Cancel. It is a decorative shape,
but the registration loop does not exclude it. Thus the usual instantiated Options
root registers **34 points**, in order: inherited Shape, each of the 15 arrow pairs
left then right, Apply, OK, Cancel. Per-child hidden states do not remove those
point registrations or suppress their starting-node own-shape bounds. Text labels
drawn manually by Options are not extra direct children or extra points.

Controls instead asks for the bounds of its three named button roots only, in the
Defaults/OK/Cancel order already specified. Each button's own shape and eligible
text/other descendants affect that one union; the inherited blackwidget decoration
does not receive its own Controls point. If the queried node has no shape and no
bounds-contributing child, the bounds helper leaves caller outputs untouched; these callers
do not check its empty result or define a fallback to a frame center.

### 4.17 Overall registration order across panels

Sources: **0xa5a50**, **0x103200**, **0xf9ac0**, **0x32f640 / 0x3347f0**,
and **0x330b20**. The frame first clears the shared list. Its interface update
0x103200 registers the ten equipment-slot centers **before any of the draw-time
registrations**, whenever inventory is shown. Their internal order remains §4.12.

Draw-time registrations follow the actual ordered GUI tree. Drawing visits sibling
nodes from first to last. A widget draw hook runs after its children, unless its
widget flags include **0x200**, in which case it runs before them. A hidden or
node-flag-0x4 ancestor suppresses the whole draw subtree and its registrations.
There is no global active-panel choice, modal filter, point sorting or deduplication.

For the tree assembled by 0xf9ac0, the order of the requested groups, **omitting
groups whose draw/registration conditions fail**, is:

| Order | Registration group |
|---|---|
| 1 | Equipment-slot centers from interface update, if inventory is visible |
| 2 | Crafting-list grid **G+0x340**, viewport cells in row-major order |
| 3 | Crafting recipe detail **G+0x348**, ingredients followed by craft-button point |
| 4 | Shop grid **G+0x350**, viewport cells in row-major order |
| 5 | Identification panel **G+0x360**, its two manual points, if shown |
| 6 | Answer panel **G+0x378**, answer-row points in answer order |
| 7 | Inventory grid **G+0x330**, viewport cells in row-major order |
| 8 | System menu **G+0x358**, its four manual entries in their draw order |
| 9 | Title root Start menu, its conditional manual entries from §4.6 |
| 10 | Options, its direct-child centers in §4.16 order |
| 11 | Controls, Defaults, OK, Cancel |

The seemingly late inventory grid is intentional. Construction initially attaches
inventory before crafting, but near the end of 0xf9ac0 **0x3347f0 explicitly detaches
and appends the inventory root to the end of the game UI root's child list**.
The system menu is then constructed after it. The game UI root itself was appended
to the engine root before the title/frontend roots; Options is appended before
Controls. Opening/closing these requested panels through their ordinary visibility
handlers does not reorder their nodes. If another operation actually reparents a
node, draw-time registration follows the resulting child list rather than a fixed
panel-priority table.

The shared list therefore can contain, for example, equipment points first and
shop points before inventory points when those panels coexist. This registration
order is independent of the inventory-first right-stick wheel eligibility chain
and the independent trigger tab handlers in §4.7.

For completeness, the two Identification points in this ordering come from
**0x277250** and are always registered by that draw, including when no item resolves.
With effective panel width W and height H, their widget-local integer coordinates
are **(truncate(W/3), truncate(H−20))**, then
**(truncate(2W/3), truncate(H−20))**. They correspond to **Identify**, then **Goodbye!**,
and pass through 0xf97e0's placement transform.

## 5. Verified: dialogue page grouping and placement of page effects

Sources: **0x1694e0** (string-token compiler), **0x167c80 / 0x166cb0** (dictionary
expansion), **0x16c880** (append displayed words), **0x15c380** (empty page),
**0x98180** (Examine's final lore attachment), and **0x168120** (the separate numeric
token compiler). These rules refine §3 for a UI that currently flattens dialogue.

### 5.1 String-script tokens

Pages begin with empty displayed-word lists, empty action lists, zero item counts, and
an empty answer-choice id string at page+0x10. Treat script metadata and the displayed
words separately. Compiling `define_item`, `define_location`, `define_mob`, the other
definition tokens, and grammar tags does not itself display a page or grant an item.

* **`new_page`** sets a pending page-break flag. It does not immediately create a page.
  Multiple such tokens coalesce. Definitions, tags and a following `mark_location` do
  not consume that flag. In particular, a mark after `new_page` still attaches to the
  existing last page if there is one; it does not move to an as-yet uncreated page.
* **`mark_location x y`** appends to the last page as specified in §3.2, creating an
  empty page only if the whole page list is empty.
* **A speech/dictionary id** creates a fresh empty page first if the pending-break flag
  is set, then clears that flag. Otherwise the dictionary renderer uses the last page,
  creating one if none exists. Before rendering an ordinary dictionary speech it also
  creates a new page when the last page already has **more than 10 displayed words**.
  Exactly 10 words does not trigger this particular split. This test occurs at the
  speech call boundary, not after every displayed word.
* **An id ending in `?`** creates a fresh page regardless of that pending flag and stores
  the id as the page's answer-choice id at +0x10. Its dictionary expansion fills that
  page. When pages are attached to a speaker (0xa3170), pages with nonempty choice ids
  go to the choice/dialogue panel G+0x378; ordinary pages go to the speaker's bubble.
* **An emote token beginning `!`** creates a fresh page if none exists or the last page
  already has any displayed words; otherwise it uses the existing empty page. It
  appends the emote and sets the pending-break flag, so the next speech begins another
  page. Its emote page remains in the page list.

Consequently consecutive short speech ids can share a page. A `new_page` cannot be
modeled merely as a newline, and the metadata effects must remain attached to the
specific compiled page even when that page is empty or contains an emote.

### 5.2 Dictionary expansion: `%`, `%%`, and word splitting

The recursive native dictionary renderer **0x166cb0** sees expanded grammar nodes.
A node whose entire text is the single character **`%`** creates a new empty page
**immediately**, including during expansion of one speech id. This differs from the
script's deferred `new_page` flag. A node whose entire text is **`%%`** emits one
literal `%` into the current page's displayed words. The strings are confirmed in
binary data at **0x4378e8** (`%`) and **0x44d0d0** (`%%`). Other text is appended to
the current page through 0x16c880, which divides it at whitespace into displayed-word
entries carrying their formatting. Grammar/macros and substitutions can expand to
multiple such entries; they do not each imply a new page. Nested `%` nodes retain their
immediate-break behavior.

Thus `mark_location` following a speech containing dictionary `%` attaches to that
speech's **last expansion page**, and a mark before the speech remains on the earlier
page. Do not hoist all marks to conversation start or end.

### 5.3 Where lore cells are attached

The displayed lore list at page+0x190 is not an implicit result of `define_location`
or every speech mentioning a place. **Examine 0x98180** identifies the lore building's
**16-block cell**, generates and compiles its entire tale (0x2d4290 → 0x1694e0), then
appends that cell pair to the **last resulting page's +0x190 list**. This is after all
deferred script breaks and immediate dictionary `%` expansions have been processed.
Its source cell must not be confused with a `mark_location` pair, which names a
1024-block place cell. The source of this explicit lore attachment is also specified
by `lore-and-artifacts.md` §2.3.

The consumer 0xb40d0 credits a page's +0x190 cells at page activation (clock zero),
before processing its +0x1a0 mission marks. It first inserts the 16-block cell into
the personal used-cell set +0xa8. For a lore building it then adds it, once, to the
appropriate realm's set under +0x128 and reports ten percentage points per recorded
cell. All details, including coordinate conversion and sound, are already specified
in `lore-and-artifacts.md` §4.1. With several `%` pages, Examine's lore is credited
only when the player reaches the final page.

### 5.4 Item actions: separate numeric script format

The string-token compiler **0x1694e0** has **no `give_item` or `take_item` token**.
Its `define_item` only sets dictionary context. No item-grant action is implicitly
created by expanding ordinary dictionary text. The native compiler that writes page
item-action fields here is **0x168120**, which consumes a different stream: signed
32-bit **numeric opcodes and operands**, rather than the string scripts returned by
0x12a6f0. Verified numeric opcodes:

| Opcode | Operands and page effect |
|---|---|
| 5 | item description: define the dictionary's current item; no inventory action |
| 8 | item description: put that item at last page+0xd8 and set give count +0xd4 to **1** |
| 9 | i32 count: set last page's give item type to **12**, material to **11**, and give count to that operand (money) |
| 10 | item description followed by i32 count: put the item at last page+0x34 and remove count +0x30 to that count |
| 11 | eight i32 operands decoded as a 32-byte descriptor by 0x15ce50: append it to the last page's descriptor list +0x178; its gameplay meaning is unresolved |
| 12 | i32 place x, i32 place y, i32 place kind: define the location and append x/y to the last page's **mission mark list +0x1a0** |
| 0x3d | deferred page break, analogous to string `new_page` |

For opcodes 8..12 a last page is created when none exists. Their effects otherwise
attach to the existing last page. Numeric item descriptions are decoded by **0x15cf40**;
the five i32 operands are category (low byte), subtype, modifier, material (low byte),
and rarity (low byte), in that order; other item fields begin at zero. The numeric compiler's item
actions are recorded during compilation and consumed later by §3.3; merely setting
dictionary context with opcode 5 or string `define_item` never grants an item.

Implementation boundary: support the verified string-script grouping and its marks/lore
without inventing string item-action tokens. A numeric script can supply page item
actions only when its native producer and item operands are actually represented.

### 5.5 Identified numeric-stream producer

The native call-reference export has **one direct call to 0x168120**, from
**0xb40d0 at 0xb5283**. No stored 64-bit pointer to 0x168120 was found in the owner's
unpacked image. At that call site the current speaker page has already given its
inventory reward through the page consumer. Only when its give count is **1** and
its give item is category **24**, it creates a numeric feedback stream consisting of:

* opcode **5**, followed by that item's five operands (category, subtype, modifier,
  material, rarity), serialized by **0x838a0**;
* speech id **0x85**, which **0x165640** maps to dictionary id
  **`you_received_the_item`**.

It compiles that feedback into the general dialogue panel G+0x418 with an empty
speaker name and a seed of **rand()%10000** (one CRT rand draw). If the local player
is idle or its ability has finished, the same branch also starts ability **0x7c**,
sets its clock to zero, and copies the received item into the player's item-in-use
record +0x1d8.

This stream defines an item for display; it contains **no opcode 8, 9, 10, 11 or 12**
and does not award the item a second time. No actual producer of numeric give/take
actions or descriptors was identified by this follow-up. Implementing the above
feedback is supported; wiring a decoder for the other opcodes into an invented
quest/reward path is not supported by the available native evidence. Their decoder
semantics in §5.4 remain facts about accepted input, rather than proof that ordinary
Steam gameplay originates those actions.

### 5.6 Answer-choice panel: visibility, layout, selection, and actions

Sources: **0xa3170** (attach dialogue), **0x28ebb0** (panel constructor),
**0x28ed90 / 0x28f6e0** (size), **0x28ee60** (draw, hover, navigation points),
**0x101bd0** (position), **0xa5a50** (visibility), **0x94840** (selected id),
**0x965e0** (release), **0x89cb0** (choice actions), and **0x2c3710** (host use).
This also corrects the earlier description of 0x28ee60 as a multiplayer widget:
it draws the answer-choice panel **G+0x378**.

The panel's pages are a separate vector at widget **+0x1e8 / +0x1f0 / +0x1f8**,
with the same **0x1b0-byte** page records as §5.1. Each record is a separate answer;
its narrow id string at page **+0x10** identifies its action. Panel **+0x200** is the
speaker's i64 creature id, and **+0x2f8** is the signed hovered row index, initially
and at the start of every draw **−1**. Attaching dialogue addressed to the local
player first empties the existing answer list. New answer pages are appended in
script order, and each sets the panel's speaker id. Ordinary pages are appended to
the existing bubble for that speaker, or to a newly created bubble.

An empty answer list hides the panel. With answers present, the frame hides it
while **any bubble in G+0x400 has recipient id +0x218 equal to the local player's
id**; otherwise it shows it and updates its size. This is independent of the
bubble's screen visibility, and answers are not selected automatically when the
preceding speech finishes. The usual movement-dismiss path in 0xa5a50 empties
the answer list only if its panel is currently visible.

The root uses the copied **blackwidget** shape, initially hidden, under the game
UI root. Font is **resource1.dat, 12**, outline **3**. Layout values below are GUI coordinate
units, before the GUI engine's overall scale. On interface placement, the panel is
horizontally centered; its bottom is **150** above the window bottom. More exactly,
0x101bd0 positions it at the parent's transformed reference point
**((window width − panel width)/2, window height)** plus offset
**(0, −150 − panel height)**. The constructor's temporary size is **300 × 200**.

For sizing, measure each choice's displayed words with the panel font. The row
measurement helper returns maximum reached line width and **32 + 18 per wrap**.
Panel width is **50 plus the maximum measured row width**, and height is **20 plus
the sum of all measured row heights**. The measurement uses the panel's current
width for wrapping, then the size update installs the new dimensions. Do not
replace this with a fixed width or a one-pass fit to the window.

Words have a shared horizontal word cursor, initially zero. Before each word
except the first, add **12**, reduced to **3** when that word is one of the
single-character punctuation tokens **period, hyphen, comma, colon, semicolon,
exclamation mark, question mark, apostrophe**. The space is suppressed after a
hyphen or apostrophe. Word advance is its truncated measured width, with an extra
**4** for an apostrophe. A nonpunctuation word wraps before drawing if the cursor
plus its advance is **strictly greater than current panel width − 24**; wrapping
resets the horizontal cursor to zero. A punctuation word does not trigger wrapping.
Text is drawn at local **x = cursor + 20**, **y = current line top + 25**, with each
word's stored color. Drawing advances the line top on wrap by
**truncate(12 × 1.350000023841858)**, which is **16**, and adds **32** after the last
line to get the next row top. The sizing helper instead advances by **18** per wrap;
this native difference also affects the hover intervals.

Row tops start at zero and follow the drawing advances. A row is hovered when the
panel root's subtree hit test succeeds and its local mouse y is in the half-open
interval from the row's drawing top to that top plus its **measured** row height.
There is no additional test against that row's individual text width. A hovered
row's words become **RGBA (0, 1, 1, 1)**; other words retain their stored colors.
Overlapping measured intervals can let a later row replace the hovered index.
For each row, drawing registers a navigation point through 0xf97e0 at local
**x = truncate(panel width/2)** and **y = truncate((row drawing top + next drawing
top)/2 + 8)**. These are real point registrations; ordinary text lines elsewhere
do not acquire this behavior automatically.

Left **mouse-up**, button index **0**, activates choices in 0x965e0 **after** normal
GUI release dispatch. It tests panel visibility, then retrieves the id of the
last drawn hovered row through 0x94840. An invalid index yields an empty id. There
is no matching-down requirement and no fresh mouse hit test here. It resolves
the panel's stored speaker id in the live creature map and calls the choice
handler even if that resolution fails. **Every such release empties the complete
answer list first**, including a release outside the rows, an empty id, an unknown
id, an unavailable speaker, or an action whose further checks fail.

With a live speaker, 0x89cb0 recognizes exactly these two byte-for-byte ids:

| Choice id | Action |
|---|---|
| `change_specialization?` | Immediately replace the local player's specialization byte +0x14d with **1 minus its old value**. Compile `define_specialization`, the player's class byte +0x14c, the new specialization, and `your_specialization_is_now` into the general DialogWidget **G+0x418**, using the speaker id's low 32 bits as the dictionary seed and an empty name. No interaction request or charge originates here. |
| `rest_inn?` | Require local coins **greater than 9** and world time **at least 64,800,000 ms or at most 21,600,000 ms**, including both endpoints. Subtract **10 coins immediately**, then append a **kind 5** interaction to the local player's **+0xab8** list, targeted at the speaker id. Failure of either check simply consumes the answers. |

Other ids, including `goodbye?`, do only the initial answer-list clearing. Receipt
or display of `rest_inn?` does not perform the action. Choice availability is not a
command parser: the translated displayed label is never used to choose an action.

The inn request is the ordinary **0xc8-byte interaction** from
`zone-object-interaction.md` §0.1: empty/default item, zone **(−1, −1)** at
**+0xa0 / +0xa4**, object index **−1** at **+0xa8**, speaker id at **+0xb0**, kind
byte **5** at **+0xc4**, and **+0xc6 = 1**. This identifies the previously unresolved
innkeeper-targeting producer in that older spec. Use the established local-player
interaction transport/host consumer, including for a guest; no invented text
message or immediate local day reset is involved.

At authoritative consumption, 0x2c3710 ends sneaking as usual for kind 5, resolves
that target, and checks **class 0x84** and the same inclusive night interval. It
does **not** recheck money, deduct a second fee, or perform a distance test in the
inn branch. If valid, request sound **0x1f** at the user, pitch **1**, volume **0.75**;
append sleep notice **i32 0** to world **+0x160**; increment the day and set time to
**25,200,000 ms (07:00)**. The ensuing world reset/guest mirror follows §6. If the
target or time fails the host test, ordinary zone-object handling is attempted;
this request's invalid zone/index normally makes it do nothing. A local fee is
not refunded by this consumer.

Offer generation is also explicit, in **0x2d9480**, rather than a translated-label
heuristic. For an innkeeper, the script begins with `define_number 10`, `hello`,
`new_page`. In the inclusive night interval, coins below 10 append
`you_dont_have_enough_gold_to_rest_here`; otherwise append `how_can_i_help_you`,
`rest_inn?`, `goodbye?`, in that order. By day append
`you_can_rest_here_between_six_and_six`. The specialization helper has class byte
**0x9b**; it offers `change_specialization?` after defining the local player's class
and **opposite** specialization, followed by `goodbye?`. Generation of either
offer does not perform its selected action.

### 5.7 Speech, emotes, and general-dialogue page timing

Sources: **0x28fd60** (speech widget), **0x290430** (page duration),
**0x2904b0 / 0x28ffe0** (typewriter display), **0x98180** (repeat Use on a speaker),
and **0xb40d0** (page and emote update). The frame calls 0xb40d0 **before** the
answer-panel visibility decision in §5.6.

Speech widgets have a **f32 elapsed clock at +0x200**, **i32 page index at +0x204**,
**f32 last displayed character count at +0x208**, speaker id at **+0x210**, recipient
id at **+0x218**, and **i32 milliseconds per character at +0x2fc**, initially **40**.
For a valid current page, its nominal duration **D** is 40 times the sum of each
displayed word's UTF-16 code-unit length **plus one**. An invalid page has duration
zero. The extra unit is included for every word, including the last. Drawing uses
the truncated clock/40 character budget and can show only a prefix of the current
word. It is not a whole-word reveal.

At the beginning of each update, a speaker page advances when its clock is
**strictly greater than max(truncate(1.75 × D), D + 1000 ms)**. This checks once per
bubble, advances one page, resets clock and last displayed count to zero, and
discards excess elapsed time. An exhausted bubble is removed and its GUI root is
destroyed. For an ordinary live NPC speaker, removal queues an ordinary
**kind 4** interaction on the local player's +0xab8 list with that speaker's id as
target; it does not itself activate a pending answer. If the speaker has vanished,
there is no such request. The separate embedded-item case for a bubble whose
speaker is the local player uses kind 2 instead.

Using the same NPC again through 0x98180 acts on its existing bubble: if clock is
**below D**, set it to **D**, revealing the rest. At or above D, add
**100,000 ms** to the clock. The latter causes the next normal update to expire
the page; Use does not directly increment the index or select an answer. A single
Use on partially revealed text therefore completes that page before a subsequent
Use can skip its remaining wait.

The second part of 0xb40d0 visits the retained bubbles and applies their current
page effects when **clock equals zero**, in the order already described in §3 and
§5.3. A normal timeout advances before this visit, so the newly reached page's
effects can run **in that same update**, before the elapsed-clock increment.

An emote is recognized from the **first displayed word** of a current speaker
page. The five accepted words select these resource-model indices:

| Word | Model index |
|---|---|
| `!alert` | **0xded** |
| `!question` | **0xdec** |
| `!love` | **0xdf0** |
| `!angry` | **0xdee** |
| `!sweat` | **0xdf1** |

Recognition immediately installs the model on the speaker at **+0xc50**, resets
its signed **i32 emote timer +0xc48** to zero, and hides the text bubble while an
emote model is active. Page effects are then handled under the same clock-zero
guard, after which a recognized emote **immediately increments the page index and
resets the clock to zero**. It does not wait for D or its ordinary speech timeout.
Only one emote page is recognized per bubble in this visit; consecutive emote
pages can replace the preceding model on successive updates.
If that emote was the final page, the next update removes the exhausted bubble
even while the model is still active. Pending answers can consequently appear
before the emote's two-second model lifetime ends.

After processing the bubbles, the update visits live creatures with an emote
model. A zero timer emits sound **0x3c** at that creature. Add this update's elapsed
milliseconds to the integer timer, truncating the result to an integer. Clear
both timer and model only when the result is **strictly greater than 2000 ms**.
A missing model resource supplies a null model and therefore no active-model
wait. At the end of the update, each retained bubble adds elapsed milliseconds to
its f32 clock if its speaker has no active model; otherwise its clock is set to
**zero**. Thus a following speech page's clock stays zero during the active emote.
The page-effect guard is literally clock zero, rather than a separate native
once-only activation flag; while an emote holds that clock at zero, the following
page is revisited under that guard on successive updates.

The general DialogWidget **G+0x418** has a different advance policy from the NPC
bubbles. Its current page effects run under clock-zero before increasing its
elapsed clock. Until clock is **strictly greater than D + 100 ms**, its stored
previous Primary-Attack and Use bits are both forced to zero. After that threshold,
a **release** of either action whose previous bit is set advances one page and
resets clock/display count. Both previous bits are sampled only in this mature
branch. A press released entirely before the threshold is ignored; a button held
through the threshold becomes eligible for a later release. Advancement emits
sound **0x7b** when another page remains or **0x7a** when the dialogue ends. On
exhaustion the page list is cleared and the widget is hidden. The next page's
effects run on the following update, rather than being moved to dialogue start
or end. This release gate does not select entries in the separate answer panel.

## 6. Verified: day change, loaded-zone refresh, and live residents

Sources: **0x2da6c0** (world step), **0x2e2e80** (time), **0x2e17d0** (near-player
spawn pass), **0x31c540** (zone refresh), **0x2d2eb0** (live reset), and **0x2c34c0**
(world/player daily reset). This supplements `worldgen-creatures-zone.md` §4 and
`native-saves.md` §5.6.

### 6.1 Refresh is visited by the authoritative near-player spawn pass

During an authoritative world step, **0x2e17d0** builds a set of already loaded zones
around every creature whose hostility byte +0x60 is **0** (players). For each player,
the zone-coordinate bounds cover player X/Y minus/plus **256 blocks**, inclusive,
with floor conversion to 64-block zones. It looks zones up in the loaded-zone map;
this particular collection does not itself load a missing zone. Deduplicate zones
shared by players. For each collected zone, call **0x31c540 before walking that
zone's current spawn-point map**.

The refresh does nothing when zone+0x4f8 already equals world day +8. Otherwise it
stores the new day, removes and frees the previous daily refresh spawn points listed
at zone+0x4e8 from the zone's spawn map +0x460, clears that list, restores the base
object list, and generates the new daily points/objects under the existing rules of
`worldgen-creatures-zone.md` §4. There is no direct walk of the live-creature list,
no direct kill of a shopper, and no live-creature removal inside **0x31c540**.

After refreshing the zone, the spawn pass checks each current spawn id against both
the world's live-id map and dead-id set before creating its creature. Existing live
ids are not duplicated or replaced just because that spawn's daily source changed.
Do not add a live-shopper purge to the zone refresh itself. Loaded zones outside the
near-player set retain their older refresh day until they are visited by this pass.
Mirror worlds do not execute this authoritative spawn/refresh pass.

### 6.2 Normal midnight also resets live non-players

The world step calls its time routine **0x2e2e80 before the near-player spawn pass**.
Time advances and, while the time-of-day value is **strictly greater than 86400000 ms**,
the world day increases by one and that value is subtracted. On each such day change,
for an authoritative world, the routine calls **0x2d2eb0**, then **0x2c34c0**, and
appends a sleep/reset notice with i32 value 0 to world+0x160.

**0x2d2eb0 removes every live creature whose hostility +0x60 is not 0.** This includes
friendly NPCs, shoppers, monsters and pets. It retains all player records, rebuilds
the live-id map from those retained players, and empties world+0x50. It is a removal
and destructor pass, not a damaging hit: it does not award drops/kill rewards here.
Then 0x2c34c0 clears the dead-id and mission-progress structures under its existing
daily-reset rules. The later near-player pass can therefore spawn the new day's
residents without finding the previous day's shoppers still live.

The bed/inn reset notice is also handled during the authoritative world step by the
same pair of live-reset/daily-reset calls. The notice pass is **after** the creature
AI/abilities/interaction/upkeep traversal. In the literal midnight step this means
the pair runs once from 0x2e2e80, and then again from that step's queued reset notice;
non-players that were recreated between those calls are removed again. On the next
world substep the ordinary spawn/pet passes repopulate them. For an inn interaction
that increases day during the creature traversal, the zone refresh occurs on the
following spawn pass, after the notice reset. Thus normal midnight or sleep has broader
live-entity effects than calling 0x31c540 after manually changing a day integer. The
latter by itself only changes zone generation state. Networking must preserve this
distinction when applying a day/reset event to a mirror.

Behavioral checks available from these facts: a bare zone refresh must not kill a
live shopper; a full midnight/sleep reset removes all non-player records first;
nearby zones refresh before their spawn scan; distant loaded zones refresh when
visited; retained players survive, and their pets may be recreated by the pet pass
described next.

### 6.3 Loaded zone contents and received day packets

The reset pair **0x2d2eb0 / 0x2c34c0 does not unload or rebuild loaded zones**. The
first routine operates on creature lists/id maps. The second clears the archived
zone-state map **W+0x10** and the dead-id set, plus the stated player daily data; it
does **not** visit the loaded-zone map W+0x460. Consequently loaded zones' live ground
items, block-change logs and modified block columns survive these calls. Do not add
a blanket clearing of these live lists to emulate this reset. Because later unload
captures live zone state, a still-loaded zone can subsequently repopulate the archive
that was just emptied.

There is a narrower static-object change from the **later zone daily refresh**:
0x31c540 restores its base object list at zone+0x78 and then adds the new day's daily
objects. That happens only when that zone is next visited by §6.1. It is distinct from
clearing its ground items or block-change log. A loaded zone outside that visit set
keeps its current objects until visited. A zone loaded afresh after the archive was
cleared uses generated state, as `native-saves.md` says; that statement does not mean
already loaded zones were all regenerated at reset time.

For a **received time packet 8**, dispatcher **0x3e450** compares incoming day to
the mirror world's day. On a difference it calls **0x2c34c0 only**, then assigns
the received day and time. It does **not call 0x2d2eb0**. The mirror's world step also
skips the authoritative reset-notice pass. Removal of host non-players therefore
arrives through ordinary creature omission/removal updates; a guest must not delete
all non-players merely because it received a new day. This is a refinement of the
networking wording in §6.2, not an instruction to run the authority's whole reset
on a mirror.

## 7. Verified: the player pet update pass

Source **0x2e0db0**, called every world step by **0x2da6c0**, after authoritative
resident spawning and before individual creature AI/abilities/upkeep. The pass has two
parts: reconcile equipped pet items with live pets, then maintain creatures linked
to players. It also runs on mirror worlds, but creation, item-change processing and
the second part require authority as described below. All health arithmetic is float.

Relevant fields beyond the existing creature specs:

| Offset | Meaning in this pass |
|---|---|
| W+0x210 | active-world object; must be nonnull for the first part |
| W+0x358 | mirror flag; zero means authoritative |
| C+0x8b8 | equipped 0xa0-byte pet item; category 20 identifies a pet |
| C+0x8bc | pet item subtype, used as pet race |
| C+0x8d0 | pet item's i32 field +0x18; forced to at least 1 for the local player |
| C+0x955 | pet item's byte +0x9d; used as its embedded name length |
| C+0x8d8 + 4×i | first byte of name character i |
| C+0x190 | float compared against the cached pet value; its wider meaning is unresolved |
| C+0x1544 | cached copy of +0x190 |
| C+0x1548 | cached copy of the equipped pet item |
| C+0x154c | cached pet subtype/race |
| C+0x1530 | float copied into a newly created pet; default 200.0 |
| C+0x9d8 | linked pet id |
| C+0x1a0 | owner id |

### 7.1 Resolve the currently linked pet

When W+0x210 is nonnull, visit all live creatures in world-list order. For a player
(hostility +0x60 = 0) with equipped pet category 20:

* If its +0x9d8 is zero or does not resolve in the live-id map, clear it and search the
  world list for the first creature whose owner +0x1a0 equals this player's id. If
  found, use that creature's id. This search does not require positive pet health.
* When W is a mirror, stop this player's reconciliation here. The authority supplies
  pet creation and appearance changes in its ordinary creature updates.
* When W is authoritative, let the item-change flag begin false. If the linked pet
  exists and has health <= 0, clear the **player's hits-dealt map +0xb98**. The pet's
  existing id does not immediately become missing just because its health is zero.

### 7.2 Existing pet: item/cache changes and level

For an existing pet on the authority, consider it changed if either the player's
+0x190 float differs from cached +0x1544, or equipped item +0x8b8 differs from cached
+0x1548 under the game's full item comparison 0x4df60. For a change:

1. Set the item-change flag true. If pet subtype differs from cached subtype, reset
   the pet's transient state with 0x64a90 (§7.6).
2. Set pet race to the equipped subtype. When embedded name length is **< 16**, copy
   exactly that many bytes, each taken from player+0x8d8+4×i, into pet+0x958 and append
   a zero terminator. For a length >=16 leave its existing name alone.
3. Set the pet's owner id to the player's id. If subtype differed from cached subtype,
   restore constructor-default appearance, then call the existing pet setup **0x2b67b0**
   (`worldgen-creatures.md` §7.4): reseed rand by race, generate looks and gear, set
   owner and AI flags 0x80, and update the owner's linked id. Changes within the same
   subtype do not reroll appearance/gear in this branch.

Then, whether or not it changed: if the owner has buff 0x13 and the pet does not, copy
the owner's first buff 0x13 to the pet with give-and-notify 0x29d2f0, refresh flag 1.
The resulting **buff-change notice's header source id and target id are both the
pet's id**, because 0x29d2f0 always uses its recipient for both header fields. The
copied buff's own embedded source id (+0x10 within its 24-byte buff record) is left
unchanged from the owner's buff. Do not replace that embedded source with the pet id
or use the owner id as the notice header source. Source: **0x29d2f0** plus its call
from **0x2e0db0**.
Take pet max health before changing its level; copy the player's level (+0x144) into
the pet; take its new max health; set pet health to `(newMaximum / oldMaximum) × oldHealth`.
This particular level adjustment has no positive-oldMaximum guard in the native pass.

After processing an existing pet or a missing pet, on the authority always copy the
player's +0x190 into cached +0x1544 and the equipped pet item into cached +0x1548.
These caches are updated before the new-pet creation decision.

### 7.3 Creating a missing pet

Create one only on the authority, with no linked pet id, the item-change flag false,
and player ability **not in {0x78, 0x79, 0x98, 0x9c, 0x9d}**. In ordinary missing-pet
cases no existing pet set the change flag, so an equipped pet is created in this step.

* Choose a new id by starting at **INT64_MIN+1000**, lowering to any smaller id already
  in the world list, then subtracting one (same policy as temporary bomb creatures).
* Construct the default 0x15f8-byte creature and append/register it in the world.
  Apply reset 0x64a90, then set owner id, hostility **6**, +0x1530 from the player,
  and race from equipped pet subtype.
* Copy all three player position coordinates. Set health to the new pet's maximum
  **at this point**, and copy the player's people byte +0x970. Copy the embedded name
  only when length <16, by the same rule as above.
* If the player's appearance flags contain 0x200, OR that bit into the pet's flags.
* Run **0x2b67b0** to generate looks/gear, set owner and AI flags, and set the player's
  linked pet id. This routine does not itself set health or level. Do not pre-copy
  player level here: the creation path leaves the reset level **1**, and the existing
  pet level adjustment runs on a subsequent pass.

The native creation branch has an additional hostility-1 flag-copy branch, but its
enclosing player test requires hostility 0, so it is unreachable on this path.

### 7.4 Unequipping and the local item's field floor

For every creature that is **not** a player with an equipped category-20 pet, look up
its linked id +0x9d8. If the linked creature exists and has any buff of type **7**,
leave both the link and linked creature unchanged. Otherwise clear the owner's link;
if that linked creature exists, set its health to zero. This branch is within the
active-world gate but is **not** gated by authority in 0x2e0db0.

After the first part (even if the active-world gate was false), if the local player
W+0x448 exists and its i32 pet-item field +0x8d0 is <1, set it to **1**. This does not
test the equipped category.

### 7.5 Linked-creature equipment and detachment

Only on the authority, visit all world creatures C. Skip C when owner id +0x1a0 is
zero or C's appearance flag **0x800** is set. Resolve its owner O and continue only
when O exists and is a player (hostility 0). Copy O's current region (+0x150/+0x154)
into C first.

Keep C attached only when all of these hold: O's linked pet id equals C's id; O's
ability is not **0x79**; and O is either not gliding (0x5ee70) or O+0xbb4 is <=
**3000.0 ms** (0x42e5dc). Otherwise apply reset 0x64a90 to C, set its health to zero,
hostility to **3**, ability clock +0x6c to **5000.0**, and owner id to zero. This pass
does not directly remove it from the live list.

For a kept attached C, record its maximum health before the following item changes:

* For each owner's weapon (A at +0x638, B at +0x6d8): if its category is zero, set only
  C's corresponding weapon rarity byte to zero. Otherwise copy its rarity and both
  region coordinates into C's corresponding weapon fields. Do not copy its subtype,
  modifier or whole item.
* For the owner's chest at +0x3b8: copy rarity, or zero C's rarity when its category
  is zero; copy **both region coordinates in either case**.
* For each of those three owner items, it counts as region-adapting when category is
  3..9 or 0x1a, modifier is divisible by `(rarity+1)×10`, and its effective rarity
  (0x109720 against the owner's current region) equals its stored rarity. For such
  an item overwrite C's corresponding item region with C's newly copied current
  region. 0x109720's region/effective-rarity rules are the existing stat/item rules.
* Compute C's new maximum health. Only if the recorded old maximum was >0, preserve
  its health fraction by setting health to `(newMaximum / oldMaximum) × health`.

This final part applies to any qualifying owner-linked creature, not merely to one
that was created by §7.3. It intentionally drops a stale owned companion when O's
linked id points elsewhere.

### 7.6 State cleared by 0x64a90

This is a reset of transient combat/equipment state; it preserves creature id,
position, race, hostility and current health. Sources **0x64a90 / 0x64c00**, where the
second helper addresses the embedded state beginning at C+0x10. Verified reset fields:

* Velocity/intent/knockback C+0x34..+0x57 become zero; ability +0x68 and its clock
  +0x6c become zero; combo +0x70 and the following four bytes become zero.
* Stun +0x130 becomes -3000.0; +0x134..+0x143 become zero; state flags +0x128
  become zero; +0x19c's first two bytes become zero.
* All equipment/carried item bytes C+0x278 through +0x957 become zero; appearance
  +0x78 is restored to the constructor defaults in `worldgen-creatures.md` §7.3.
* Level +0x144 becomes 1; +0x148 becomes zero; class/spec +0x14c/+0x14d become zero;
  +0x158 and +0x15c..+0x173 become zero. The reset does not overwrite region +0x150.
* Block/mana/stealth and +0x190 (the 16 bytes +0x184..+0x193) become zero; +0x194
  becomes 1.0; +0x198 becomes 10.0. Owner +0x1a0 and the following eight bytes zero.
  Pair +0x1b0/+0x1b4 and pair +0x1d0/+0x1d4 become (-1,-1).
* Name +0x958..+0x967, +0x968..+0x977, +0x978..+0x97f become zero. Linked id
  +0x9d8, last-hit id +0x9e0, +0xad8 and cached pet float +0x1544 become zero.
* Clear interaction list +0xab8, aggro +0xb88, hits-dealt +0xb98, +0xb78 map,
  inventory tabs +0x9f0, buff list +0x980 and +0xc20 vector; zero +0xc18.
* Zero +0xbc0 (8 bytes), +0xbc8 (4), +0x994 (8), +0xb58 (4), +0x99c (4),
  +0xbb0 (8), +0x9e8 (4), +0xba8 (4), and byte +0xbb8. Stamina +0x9a0 becomes 1.0.

The cached item +0x1548 is not cleared by this helper. Constructor defaults and pet
setup still apply in the order stated above; resetting is not equivalent to replacing
the entire record and losing its position/id/health.

Useful regression cases: equipped pet recreates after daily live reset; existing dead
pet does not duplicate; same-race name edit does not reroll gear; changed race does;
new pet starts level 1 then scales health on next pass; unequip preserves a buff-7
linked creature; mirrors do not spawn pets; another owned creature with a stale id
detaches; early gliding keeps a pet but gliding beyond 3000 ms detaches it.

## 8. Verified: music fade rates and the frame's sound-request consumer

Sources: **0xa5a50**, especially assembly **0xac269..0xac2f0**, **0x16ee0**,
**0xa7a0**, **0x83fb0**, and **0x83640**. This resolves `sound.md` question 3 and
the queue-lifetime portion of question 1.

### 8.1 Fade targets and rates

Keep the desired-track, switch-delay and five-minute rules of `sound.md` §3.4. When
music is playing, choose the target and smoothing coefficient **in this priority**:

| Condition | Target volume | Smoothing coefficient k (f32) | Binary source |
|---|---|---|---|
| Instrument duck countdown G+0x204c >0 | 0.015 | **0.005** | target 0x437f78; k 0x42e518 |
| No duck and elapsed E <= switch threshold T | musicVolume × 0.01 | **0.001** | k 0x42e514 |
| No duck, E>T, boss wanted or success countdown G+0x2050 >0 | 0 | **0.005** | k 0x42e518 |
| No duck, E>T, every other case | 0 | **0.0004** | k 0x437f68 |

The exact stored floats are 0.004999999888241291, 0.0010000000474974513 and
0.00039999998989515007, respectively. The assembly's two zero-target branches differ
in k even though the simpler decompile loses their extra arguments.

Call the existing native smoothing-factor rule **0xa7a0** (`creature-upkeep.md` §4.9)
with the frame's elapsed **double milliseconds**, capped above at 1000.0 as in the
rest of this frame, and the selected float k. Then update volume G+0x2048 in float:
the difference target-minus-current is multiplied by that float factor, then added
to current. Do not substitute a fixed-duration linear crossfade or a per-frame
constant multiplier. The target is independent of the rate: returning from duck to
the ordinary volume uses k=0.001, while ducking itself uses k=0.005. When not playing,
volume is assigned musicVolume×0.01 immediately, under the existing idle branch.

The smoothing factor is not an exponential call. For completeness, its existing
definition uses n = trunc32(elapsed×100), a double accumulator initially zero, and
n progressively increasing fractions 1/n through n/n. At each fraction, add
(fraction-minus-accumulator) × double(k) × 0.02 to the accumulator, retaining the
native arithmetic grouping from the existing smoothing spec; finally narrow to
float. For n<=0 it returns zero.

The music thread's **strictly below 0.01** stop threshold is unchanged (`sound.md`
§3.2). Fade down to that threshold, stop the old streamer, then the desired path
can start through the established track-selection/idle logic. There are never two
music tracks crossfading in the traced frame/thread path.

### 8.2 Sound-request lifetime and parameters

In **0xa5a50**, after applying received world events and stepping the client world,
the frame walks the world's entire sound-request list **W+0xa0** in its list order.
For every 48-byte record it calls **0xa0420** with sound id +0x18, position +0x00,
volume +0x20 and pitch +0x1c, and the final flag 1. This is the ordinary positional
attenuation and instrument-duck entry point of `sound.md` §2. The frame does not
poll only newly appended indices or retain a played cursor across frames.

Incoming packet sounds were appended to this same world list earlier in the frame;
local simulation-generated sound requests already in it use the same consumer. At
frame end **0x83640** replaces the whole world event-list block beginning W+0x90
with a fresh empty block, including the sound list at relative +0x10. This happens
after event distribution/network sends. Thus queued world sounds do not persist
and replay every frame. Interface sounds sent directly through **0xa03f0 / 0xa0420**
are played immediately and are not added to W+0xa0 by those helpers.

The sound consumer additionally examines ids **0x54 and 0x55** for camera feedback.
It computes squared distance d² in blocks between sound position and local-player
position, sets a candidate to max(1-d²/2500, 0), and raises G+0x1e00 to that
candidate if larger. This is separate from sound attenuation, which uses distance
to the camera/listener and a 64-block limit. The **2500.0** constant (50 blocks squared)
is at **0x437ffc**; squared-length helper is **0x2b650**.

These facts cover queue forwarding and fade behavior. They do not establish new
producers for sound ids that `sound.md` lists as never found, nor replace the existing
per-action sound specifications with guessed events.
