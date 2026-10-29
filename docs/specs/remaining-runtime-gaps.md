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
G+0x118) is false and G+0x668 (conversation mode) is false:

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

The last four rows also run in conversation mode (G+0x668 true), while the Y/B/left
shoulder/left trigger/X/A gameplay mappings are replaced by §4.4 there. The pad does not
write forward/back/left/right binding indices 5..8; movement comes from the analog pair.
No pad mapping to Walk (index 13) or Multiplayer Menu (index 20) occurs in this loop.

**B after a menu:** holding B while the UI-input predicate is true sets a suppression
latch and closes conversation mode and the visible panels enumerated by this branch
(inventory, crafting, map and associated in-game menus). The Jump state is suppressed
while that latch is set. Releasing B clears the latch; a subsequent B hold in gameplay
can jump. Thus the same B hold used to dismiss a menu does not immediately become a
jump when the menu disappears.

### 4.2 Sticks and the effective dead zone

Each XInput stick component is a signed 16-bit value, -32768..32767. Convert it to float,
multiply by **1/65536**, then add that result to itself. This gives a signed value in
[-1, 0.999969482421875]. There is no remapping of the surviving range to compensate
for a dead zone and no unit-circle normalization of diagonal input.

When gameplay input is available **or** conversation mode is active:

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

When the world-map panel (G+0x368) is visible, the same cubic input instead adds the
horizontal rate × time to map field +0x1f0 and the vertical rate × time to map field
+0x1e8, narrowing the results to float. It does not update the camera targets in that
branch.

**Corrections to the earlier broad camera wording:** this right-stick pass does not
multiply by the cameraSpeed option and does not consult invertY. Those options are used
by the separate mouse-input routine **0x97880**, which scales mouse displacement by
cameraSpeed × float 0.005 (0x42e518), reverses its vertical sign when invertY is set,
and clamps its pitch target to 0..180 there. The right-stick pass has no equivalent
pitch clamp in this input portion. This specifies target input; the existing camera
update still controls the displayed angles and smoothing.

### 4.4 UI and conversation controls

The UI-input predicate is **0x1014f0**. It reports that one of the front layers, chat,
keyboard-owning panels or in-game panels it enumerates is open; it is not merely the
chat-line flag. When true and conversation mode is false, the pad drives the following
UI behavior instead of the normal left movement stick.

* **A** becomes mouse button 0, **X** mouse button 1. **Y** additionally becomes mouse
  button 2 in conversation mode. Each is sent through **0x94900** on its down edge and
  **0x965e0** on its up edge; no repeated mouse-down is synthesized during a hold.
  The underlying GUI root's held bits are set by 0x340ec0 and cleared by 0x3410e0
  (and the corresponding other-button routines). Many widgets activate on release;
  reproducing only a down-edge callback will miss them.
* In ordinary UI mode A's down/up synthesis is allowed only after the UI-mode timer is
  **strictly greater than 200 ms**. The timer resets to zero in gameplay/conversation
  mode. X's synthesis has no such wait.
* In conversation mode left shoulder and right shoulder, on down edges, call
  **0x101860 / 0x101790**, respectively: move to the previous/next entry in the personal
  list at state+0x118, update its displayed location, and ensure map zoom index is at
  least 4. The always-on right-shoulder Normal Attack held byte of §4.1 is still set.
* In ordinary UI mode, if inventory/crafting/shop panels G+0x330, +0x340, +0x350 are not
  both visible and eligible according to their panel predicate 0x333e70, the right
  stick still feeds the normal right pair through 0xa3900. If at least one is eligible,
  raw right Y with absolute value **> 17378** calls the wheel/zoom routine **0x97cc0**:
  +1 for up, -1 for down. It fires immediately, waits **400 ms** after initial entry,
  then repeats at **100 ms** while held; falling below the threshold resets its repeat
  timer. Raw right X crossing the same threshold acts once on entry for an eligible
  inventory: negative selects the preceding item-category entry (0x280170), positive
  the following entry (0x27f6a0). This branch does not write a fresh right analog pair.
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
list; the exact candidate ranking is deliberately left unresolved here.

**Last-used-device indicator:** G+0x1958 is set to 1 by mapped pad actions. In the
gameplay/conversation analog branch it is also set when abs(raw left Y) > 7849 or
abs(raw right X) > 7849; that particular indicator test does not examine left X or
right Y. Sampled nonzero keyboard/mouse bindings set it to zero before the pad pass.
This indicator is not the stick dead-zone test.

Verification limits: the fixed mapping, radial filters, camera arithmetic and listed
edge/repeat timing are verified from exports and binary constants. No new controller
capture was run. The exact GUI point-ranking policy remains an optional follow-up;
it does not prevent implementing the complete gameplay pad mapping and camera.

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
