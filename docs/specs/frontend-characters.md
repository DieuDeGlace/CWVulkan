# Frontend: character select, character creation, deleting, and the unused world screens

Behavioural specification of Steam Cube World's character screens, written from the decompiled
executable and the binary's constants (routines by RVA, image base 0x140000000). Conventions,
the layers, the camera and the text helper are those of `frontend-title.md` (§0, §2, §3.3).
"P" is the local player's creature record, "G" the game object; offsets are for cross-checking
only. Item fields follow `drops-and-items.md` (type +0, subtype +4, modifier +8, region +0xc /
+0x10, rarity +0x14, material +0x1c); creature equipment slots are 0xa0 bytes each from P+0x278
(slot 2 the chest at +0x3b8, slot 6 the left weapon at +0x638, slot 7 the right weapon at
+0x6d8).

**Key fact:** the game edits the real player record P in place. The character on the title,
the "preview" in the creation screen and the character you then play are all P; the list of
saved characters is a separate list of copies.

---

## 1. Where characters live

### 1.1 The file

`Save/characters.db` is an SQLite database with one table, `blobs(key TEXT PRIMARY KEY, value
BLOB)` (created by 0x67070 if missing; read 0x670f0, write 0x672c0 = update if the key exists
else insert, delete 0x66fc0, rename 0x671f0). Keys:

| Key | Value |
|---|---|
| `num` | 4-byte little-endian int: the number of characters |
| `slot` | 4-byte int: the index of the character last selected or created |
| `0`, `1`, … (decimal) | one character: a zlib stream (zlib 1.2.8, 0x66c10 / 0x66d10) of the game's creature serialisation (0x149760 writes, 0x147800 reads; not specified here) |

### 1.2 In memory

At start-up (0x7ea80) `num` is read and characters 0 … num − 1 are loaded (0x9bb20) into a list
of creature records (G+0x1778). 0x9bb20 reads the blob, inflates it, deserialises it into the
given record, and makes sure its knowledge record lists the item (type 1, subtype 7) as known
(a default recipe).

Saving the current character (0xa27b0, used by Create Character, by "Start Menu" and at other
save points): P is copied (position, inventory, money, buffs, knowledge…), serialised,
compressed and stored under the key of the selected index G+0x2264. Nothing is saved while
G+0x2264 is negative.

---

## 2. Character select

The layer GUI[3] (G+0x130). The title camera keeps circling (P still rides, `frontend-title.md`
§3.1); the logo is not drawn; the main theme plays.

### 2.1 The list (built by 0x107440)

0x107440 removes the old entries and creates one **CharacterPreviewWidget** (constructor
0x26ea60, draw hook 0x26eba0) per saved character, **plus one more** whose character is none
(the "New character" entry). Entry *i* (from 0) requests size **600 × 220** and is placed at
**(300, 50 + 230 i)** in the list's space. The list is rebuilt after every create and delete.

**Fitting the screen** (0x103200, while the layer is shown and GUI engine+0x1a8 bit0
is clear): with T the list's total height (top of the first entry to bottom of the last) and H the
window height, if T < H − 20 the list is centred vertically (top = H/2 − T/2); otherwise the
list is moved, eased by the smoothing factor of `creature-upkeep.md` §4.9, so that its top is
not below 10 and its bottom not above H − 10. A gamepad scrolls it by the stick instead
(0x101160).

### 2.2 One entry (0x26eba0, every frame)

Text uses font size 15 then 12, outline 3, outline colour (0, 0, 0, 1).

* **The "New character" entry**: the words "New character" centred in the entry (alignment 5),
  at y = 30 from the top, white.
* **A character entry**:
  * Its record is quietened every frame: timers +0x12c and +0x130, flags +0x128 and +0x5c
    and the ability are cleared, and its animation clock +0x99c advances by *dt* × 0.0075.
  * **While the mouse is over the entry** (0x32b0c0) it walks and turns: its yaw (+0xb3c)
    decreases by *dt* × 0.05 degrees (50° per second), +0x998 is 0.2, its movement intent is
    eased toward (0, 8, 0) and its velocity toward zero (`creature-upkeep.md` §4.9 factor).
    **Otherwise** yaw, +0x998, intent and velocity ease back to rest.
  * The creature is drawn into the interface (0xdb630) at (100, 110) in the entry, with the
    rotation (−95°, 0°, 45°) and the entry's screen scale.
  * Text at x = 200, font size 12, left-aligned:
    * y 30: the name, colour (0.25, 1, 1, 1);
    * y 55: "LV " + level + " " + race word + " " + class word, white. Race word by race id:
      0, 1 "Elf"; 2, 3 "Human"; 4, 5 "Goblin"; 7, 8 "Lizardman"; 9, 10 "Dwarf"; 11, 12 "Orc";
      15, 16 "Undead"; other ids (13, 14 frogmen, 6) print nothing. Class word: 1 "Warrior",
      2 "Ranger", 3 "Mage", 4 "Rogue";
    * y 75: "Specialization: " + the specialisation word, colour (0.5, 0.5, 1, 1). Words by
      class and specialisation (+0x14c, +0x14d): warrior 0 "Berserker", 1 "Guardian";
      ranger 0 "Sniper", 1 "Scout"; mage 0 "Fire Mage", 1 "Water Mage"; rogue 0 "Assassin",
      1 "Ninja".

### 2.3 Selecting and the buttons (0x103200)

* The entry under the mouse becomes the **selected index** (G+0x6c0). When it changes, the
  delete button is hidden.
* With a valid selection three buttons are placed relative to the selected entry (its
  parent-relative frame x, y, width w, height h):
  * **Select / Create** (GUI[7], a "button2", requested 200 × 30): at
    ((w + x) − its current parent-relative width − 20,
    (h + y) − its current parent-relative height − 20), shown. Its caption is "Select" for a character entry and **"Create"**
    for the "New character" entry.
  * **Delete toggle** (GUI[8], a "smallbutton", requested 20 × 20): at
    ((w + x) − its current parent-relative width − 20, y + 20), shown
    for character entries only. Clicking it (0x9f600) shows the delete button.
  * **Delete Character** (GUI[9], a "button", requested 300 × 30, hidden until the toggle is clicked):
    at (x + w + 20, y), i.e. just right of the entry.
  On the "New character" entry the toggle and Delete Character are hidden.
* Without a valid selection, or while GUI engine+0x1a8 bit0 is set, the three are hidden.
* **Back** (shown on this layer) returns to the title.

### 2.3.1 Verified constructor and frame-placement distinctions

Native-only follow-up; no owner assets, captures, engine or tests were
inspected. These clarify the coordinate basis and data-owned orientation
without asserting any particular owner's template matrix values.

**Construction0xf9ac0:** GUI[3] is a newly created unnamed container node
under the GUI engine's root, initially hidden. The three buttons are its
children, not children of the selected card. The constructor obtains
owner gui.plx templates and uses the independent-copy routine0x3308e0:

| GUI entry / Game pointer | Owner template | Size request through0x32c9a0 | Release callback |
|---|---|---|---|
|7 / G+0x150|`button2`|200×30, call0xfcb60|0x9f7c0, Select/Create|
|8 / G+0x158|`smallbutton`|20×20, call0xfcbe5|0x9f600, toggle Delete|
|9 / G+0x160|`button`|300×30, call0xfcc46|0x9dea0, Delete Character|

The size receiver is each node's **widget+0x70**. All three callbacks
are registered for **event3, mouse-up**, not event2/down. GUI[7] starts
with literal `Select`; later0x103200 changes the text to `Create` for
the final null-character card.0x334c50 updates text shapes in the
copied subtree; it does not replace its drawing with a new label/button
style. GUI[8]/GUI[9] initially hide.

Before cloning,0xf9ac0 iterates the **direct children** of each
`button`, `button2` and `smallbutton` template and calls0xf9770(child,0).
The helper sets passive node-flag bit0x02 on **its passed node and all
of that node's descendants**. Therefore these constructor calls mark
every template descendant, **not the template root itself**; the root
retains whatever flags it already had. It does not alter rotation,
deformation, pivot or scale.
Preserve the copied owner drawing, text styles, node transforms, widget
bind/frame data and flags. The controller construction/placement code
does not prescribe an extra fixed Euler angle, reflection, identity
deformation or scale for GUI[7]. Owner widget initialization/enable/
disable poses remain separate and can apply their own keyed transforms;
“no explicit controller rotation” does not mean discarding those poses.
This audit did not read the owner's button2 asset, so its numerical
orientation/pivot/bind matrices are intentionally not replaced by guessed
constants.

**Requested size versus current geometry:**0x32c9a0 forwards to
0x32c6f0. That setter converts the requested parent-space size vector
through the inverse parent-relative widget matrix, stores local frame
size, applies the widget minimum/inner-bind constraints and propagates
the resulting deformation/resize. The later measured parent-relative
dimensions come from **0x32da30/0x32ad10**, through0x32d1d0: the
linear part of parent-relative frame matrix Q applied to current local
effective size. They are not the requested numbers, an occupied drawing
AABB, or a second literal200×30 rectangle. BindPos/bindSize, framePos,
minimum size, the node's live transform and ancestor/widget placement
therefore matter as specified in plasma-format-and-scene §2.7.

**Per-frame placement0x103200:** card x/y are0x32da70/0x32dab0;
card and button widths/heights are0x32da30/0x32ad10. With the selected
index valid and **GUI engine+0x1a8 bit0 clear**, native calls
**0x32b560 on each button widget**, in order GUI[7] at0x1039ee,
GUI[8] at0x103a6d, GUI[9] at0x103adb. The exact finite float32
operation order is:

- Select/Create X: card width plus card X, subtract button width, then
  subtract20. Y: card height plus card Y, subtract button height, then
  subtract20.
- Delete toggle X: card width plus card X, subtract toggle width, then
  subtract20. Y: card Y plus20.
- Delete Character X: card width plus card X, then plus20. Y: card Y.

All sizes/positions in these expressions are **parent-relative frame
queries in the same GUI[3] hierarchy**.0x32b560/0x32b250 targets the
widget's parent-relative frame origin, through Q, bind and live-node
matrices. It modifies node translation to attain that frame position;
it does not merely overwrite framePos, set a raw node translation to
those numbers, or target the node's pivot at a physical window point.
In particular this route **does not use0x334700/0x334510**. The global
window-layout0x101bd0 has no separate direct positioning call for
GUI[7]/[8]/[9] that replaces these selected-card placements.

GUI[7]/[8] are shown after those placements; GUI[9]'s ordinary placement
does not itself show it. A changed selected-card index hides GUI[9].
The final null-character entry hides GUI[8]/[9] and labels GUI[7]
`Create`. Invalid selection or the engine bit0-set branch hides all
three. The exact raw engine bit is the admission condition; it is not
a newly inferred test of the last-used-device indicator G+0x1958.

**Neighboring cards and order:**0x107440 detaches the three existing
buttons, clears old GUI[3] contents, creates saved-character cards then
the null/new card, initializes that hierarchy with0x333820, and reparents
the existing buttons back in order7,8,9. This puts them after the cards
in the root's child/draw order; it does not recreate them on each hovered
card. Each card is a copied owner `blackwidget` with CharacterPreviewWidget
0x26ea60, requested600×220 at0x107560, then parent-relative frame
position **(300,50+230×index)** through0x32b560 at0x10757e.
Subsequent list fitting/scrolling moves those card frames; button positions
use their current queried frames afterward. Preserve owner widget
initialization, including minimum size and keyed enable/disable pose
rules, instead of treating the requests as raw mesh bounds.

**Hit regions:** hovered-card selection in0x103200 uses0x333e70 on
the card's root, i.e. the scene-tree hit test0x333f60 at the cached GUI
cursor with flags0. Character-preview walking's0x32b0c0 is the separate
widget hit traversal0x32ae90. Ordinary button event dispatch also
retains the copied widget/shape hit behavior, inverse deformation and
clipping; there is no extra manual GUI[7] strict200×30 test analogous
to identification's footer halves. Do not replace these with caption
ink bounds or a new constant axis-aligned rectangle. The underlying
tree-hit rules are remaining-runtime-gaps §§4.14–4.16. Activation remains
the registered up phase, independently of card hover/placement.

### 2.3.2 Passive descendants and ordinary event picking

This follow-up distinguishes **button event dispatch** from the
character-card subtree hover query above. Interface down/up adapters
**0x101020/0x101120** route button0 into GUI engine
**0x340ec0/0x3410e0**. With no engine capture override at **+0x168**,
both pick from the **global GUI root +0x140**, using cached cursor
**+0x180/+0x184** and **0x333f60 options0**. They do not pick each
button independently and then union its descendant bounds. Later/front
siblings can win that global pick; card hover remains the separate
0x333e70 call beginning at the card subtree.
The engine word+0x1a8 bit0 is set by that left-down dispatcher and
cleared at left-up entry. The old summary's “with a gamepad” label
for this bit was incorrect; it is the native left-button-held bit,
including when controller A synthesizes those same events.

Options0 visits eligible children last-to-first before considering the
node itself. Passive node bit0x02 prevents **returning that node**; it
does not by itself prune the traversal. In the prepared button templates,
however, **every descendant is passive**, recursively, and the clone
retains those flags. Thus none of their text/decoration/child geometry
can return a hit which expands the copied root's normal pick area.
After those unsuccessful child returns the root must satisfy its own
shape/widget local-hit condition and self filters. The ordinary
shape containment, inverse deformation, display clipping, widget
frame/surface clipping and root widget flag0x100 behavior remain
remaining-runtime-gaps §4.14. Do not derive the root's hit from a union
of descendant artwork, or replace it with requested200×30 bounds.
The root was not forcibly marked passive by this constructor, but an
existing passive root flag is likewise not forcibly cleared.

Once a node is selected, dispatch uses its owning widget pointer at
node+0x78, not necessarily its own widget pointer+0x70. Left down first
invokes the enabled owner's native down virtual hook, then the
registered event2 callback if the selected node is not passive and the
callback's enabled requirement passes. Left up invokes the enabled
owner's up virtual hook, then the analogous **event3** callback. These
character buttons register event3 via0xf8410 with both **recursive
registration and enabled-required bytes1**;0xf8380 stores that latter
byte in the callback. Recursive registration can bind nested widgets,
but neither registration nor those callbacks clears the descendant
passive flags or supplies a new geometry hit. The direct callback test
also rejects a passive selected node. Generic event broadcasts through
0x33c360 prune passive nodes and their subtrees.
Right/middle up are separate dispatchers0x341780/0x3414a0 with event
IDs6/9; these constructors do not register the character-button actions
under those IDs.

The existing capture pointer is an explicit exception to fresh geometry
picking: when engine+0x168 is non-null, those down/up dispatchers use
that node instead of their first root pick. This section does not infer
which owner button flags/virtual hooks establish capture. Capture is
not evidence that a passive child enlarged the root's default hit area.
The registered Select/Create/Delete callbacks add no separate
identification-style shown-at-down snapshot or new button-specific
pressed-latch test; their up event still follows the engine's owner,
active/passive and capture rules just described.

---

## 3. Selecting an existing character (0x9f7c0, Select)

1. Hide the character-select layer.
2. The selected index becomes G+0x2264.
3. Load that character into P (0x9bb20) and store `slot` = the index.
4. Refresh the GUI lists (0x107690) and rebuild the inventory screens (0xb2180).
5. Camera: distance 8 (target and current), pitch 90, roll 0, yaw 180 (targets and current).
6. Refresh the map knowledge (0x2e0b50).

No front layer is now shown, so P stops riding and plays from where it was saved, in the
seed-123 world (§8).

> **Erratum (2026-10-28).** "Plays from where it was saved" is the effect of step 3, not of
> leaving the riding layer: 0x9bb20 resets P and reads the saved record back, so the flyover's
> position, velocity, ability 0x9d and flight fields are all replaced, and the orientation comes
> back as 0. `frontend-title.md` §6.1 states exactly what happens to P (reset, position,
> orientation, no safe-ground search, zones not waited for, missing zones solid).

---

## 4. Starting a new character (0x9f7c0, Create on the "New character" entry)

### 4.1 Resetting P

1. Hide the character-select layer; G+0x2264 = the selected index (one past the last
   character).
2. Camera: distance 8, pitch 90, roll 0, **yaw 0** (targets and current): the eye is north of
   P looking south.
3. **Show the creation layer** (GUI[0xb]).
4. Reset P's state (0x64a90: links, aggro and cooldown maps, inventory and buff lists emptied,
   stamina 1.0, several timers zeroed). Name empty, hostility 0, orientation and velocity 0,
   movement flags 0, level 1, health = its maximum (0x5fac0). Both weapon slots become
   (type 3, subtype 0, material 1) iron swords; the chest's material byte becomes 1; class 1.
5. **Start spot.** 0x2d1100 picks a start cell (below) and stores it as P's home cell
   (+0x1d0, +0x1d4). P's position becomes block (16 cx + 8, 16 cy + 32), with z = the terrain
   height there (0x35ea0) × 65536.
6. P's knowledge record is replaced by a fresh one (0x7ff20).
7. The name field is emptied.
8. The look panel is reset (0x272950) and applied (0x272d90 with preview off), §5.

P is not riding (the creation layer is not a riding layer), so it stands at the start spot and
the creation "preview" is P itself, seen by the gameplay camera.

### 4.2 The start spot (0x2d1100)

The routine draws an angle a = rand × 2π / 32767 and a radius r = rand % 100000 + 100000 (in
cells of 16 blocks), starts at (r cos a, r sin a) and walks the x coordinate forward by one,
each time asking the world for the nearest suitable cell (0x2d3b60), until a cell passes every
test: its kind (0x69c60) is 0x35; the terrain at its block (16x + 8, 16y + 40) is not below
0; the region's biome (0x2b9080) is 0, 2 or 7; the zone kind there is 5; several danger
measures of the zone are not above 0 or not above 0.4; and the ground is flat (the height
differences over 16 blocks in x and y are at most 4). The draws use the CRT `rand()` with
whatever seed it has; the start spot is not reproducible. The detailed tests belong with the
world generator and are listed as open question 3.

---

## 5. Character creation

The layer GUI[0xb] (G+0x170) contains:

* the **look panel** GUI[0xd] (300 × 330), placed at the bottom-left corner (x 0, y = window
  height − panel height), holding the **CharacterStyleWidget** GUI[0xc] (constructor 0x26fda0,
  draw hook 0x271030);
* the **name field** GUI[0xf], an "edit" with the label "Name", placed at x = half the window
  width (y −50, see open question 4);
* the **Create Character** button GUI[0xe] (a "button", 300 × 40, handler 0x9ea10), at
  x = window width − its width (y −20);
* the Back button.

### 5.1 The look state

The style widget keeps: race index R (0…7), class index C (0…3), gender S (0 male, 1 female),
face index F, haircut index K, the committed hair colour (three bytes) and a preview hair
colour (three bytes). Reset (0x272950): R = C = S = F = K = 0, committed colour
(255, 200, 50), preview colour (255, 255, 255).

### 5.2 The panel (0x271030, every frame)

Labels left-aligned at x = 15, values centred, font size 12, outline 3, white:

| Row | y | Label | Value |
|---|---|---|---|
| race | 27 | Race | Human, Elf, Dwarf, Orc, Goblin, Lizard, Undead, Frogman (R = 0…7) |
| gender | 57 | Gender | Male / Female |
| class | 87 | Class | Warrior, Ranger, Mage, Rogue (C = 0…3) |
| face | 117 | Face | "Face " + (F + 1) |
| haircut | 147 | Haircut | "Haircut " + (K + 1) |
| hair colour | 182 | (none) | "Hair color", centred |

Each row except hair colour has a left and a right arrow ("leftbutton", "rightbutton" from
`gui.plx`); an arrow is drawn in (0, 1, 1, 1) while the mouse is over it, else white.

**The hair-colour grid** starts at (13, 197): 16 columns, 17 units apart, by 7 rows, 17 apart.
Column *c* uses the base colour B*c* (RGB, alpha 1):

| c | B | c | B | c | B | c | B |
|---|---|---|---|---|---|---|---|
| 0 | 1, 0, 0 | 4 | 0.5, 1, 0 | 8 | 0, 0.5, 1 | 12 | 0.5, 0, 1 |
| 1 | 1, 0.25, 0 | 5 | 0, 1, 0 | 9 | 0, 0.25, 1 | 13 | 0.75, 0, 1 |
| 2 | 1, 0.5, 0 | 6 | 0, 1, 0.5 | 10 | 0, 0, 1 | 14 | 1, 0, 1 |
| 3 | 1, 1, 0 | 7 | 0, 1, 1 | 11 | 0.25, 0, 1 | 15 | 0.5, 0.5, 0.5 |

Row *j* (0…6): for j < 3 the colour is B × (0.2 + 0.25 j); for j = 3 it is B; for j > 3 it
is B + (1 − B) × 0.25 (j − 3) (toward white). Each cell is drawn as a black square of side 17
and the colour square of side 15 on it, one unit in from the top-left (sizes in the panel's
units, scaled to the screen like the rest of the panel).

**Hovering and choosing.** At the start of each frame the preview colour is set to the
committed one. If the mouse is inside a cell (x from the cell's left inclusive to 17 further
exclusive, same for y), the preview colour becomes that cell's colour, each channel
truncated from colour × 255. If the preview then differs from the colour P shows, the look is
applied in preview mode (0x272d90). **Clicking** in the grid (0x272470) commits the preview
colour and applies. So hovering shows the colour on P; leaving the grid restores the
committed one.

### 5.3 The arrows

Every arrow plays sound 0x58 (`menu-select.wav`, 0x1400a03f0) and then applies the look:

* race: R − 1 (below 0 → 7) / R + 1 (above 7 → 0);
* class: C − 1 (below 0 → 3) / C + 1 (above 3 → 0);
* gender: either arrow toggles S;
* face: F − 1 / F + 1; haircut: K − 1 / K + 1 (wrapped when applied, below).

There is **no randomise button** in this build, and nothing is drawn at random except the
start spot and the starting items' modifiers (§5.5).

### 5.4 Applying the look (0x272d90)

1. P's race id from R and S: Human 2 + S, Elf S, Dwarf 9 + S, Orc 11 + S, Goblin 4 + S,
   Lizard 7 + S, Undead 15 + S, Frogman 13 + S.
2. P's class = C + 1.
3. **Wrap F and K** by the race id's number of faces n*f* and haircuts n*h*: below 0 → n − 1;
   at or above n → 0.

   | race id | faces | haircuts | race id | faces | haircuts |
   |---|---|---|---|---|---|
   | 0 elf ♂ | 4 | 10 | 9 dwarf ♂ | 5 | 3 |
   | 1 elf ♀ | 6 | 11 | 10 dwarf ♀ | 5 | 6 |
   | 2 human ♂ | 6 | 15 | 11 orc ♂ | 5 | 8 |
   | 3 human ♀ | 6 | 9 | 12 orc ♀ | 5 | 4 |
   | 4 goblin ♂ | 5 | 6 | 13 frogman ♂ | 5 | 5 |
   | 5 goblin ♀ | 5 | 6 | 14 frogman ♀ | 4 | 4 |
   | 7 lizard ♂ | 2 | 6 | 15 undead ♂ | 6 | 6 |
   | 8 lizard ♀ | 5 | 6 | 16 undead ♀ | 6 | 6 |

   (These are the same counts the looks routine uses; `worldgen-creatures-tables.md` T7.)
4. **The looks record** in P's knowledge (P+0x1528): +0 = R, +4 = S (byte), +8 = F,
   +0xc = K, +0x10..+0x12 = the hair colour (red, green, blue) — the preview colour (equal to
   the committed one except while hovering the grid).
5. P's appearance is reset to the record defaults (0x4d1f0) and rebuilt by the looks routine
   0x53e80 **with this looks record**: the race comes from the record's race index and sex as
   in step 1; the head model is the race's first head + (F mod faces), the hair model the
   race's first hair + (K mod haircuts) (the bases are in T7); the colour bytes +2, +3, +4 of
   the appearance are the record's red, green, blue; every other part is the race's fixed
   data (`worldgen-creatures.md` §4.2). No `rand()` is drawn on this path.
6. **The starting kit** is given (0x913b0, §5.5), then the world is unlocked (0x2b9890).

### 5.5 The starting kit (0x913b0)

1. `srand` with the low 32 bits of P+0x968 (P's Steam account id, filled from the Steam API
   on the first frame), so the kit's random numbers depend only on the account.
2. All equipment slots emptied (0x3b6b0); the inventory emptied and given 6 tabs; tabs 0–2
   are sized 10, tabs 3 and 4 sized 30.
3. By class (P+0x14c). Every weapon here is type 3 with rarity 0; "+ inventory" items go
   through the normal add-to-inventory routine (0x4e810, `drops-and-items.md`), one call per
   item:

   | Class | Right weapon (slot 7) | Left weapon (slot 6) | Chest (slot 2) | Inventory |
   |---|---|---|---|---|
   | 1 warrior | subtype 17, material 2 | — | type 4, material 1 | 2 × (3, subtype 0, material 1); 1 × (3, 13, 1) |
   | 2 ranger | subtype 6, material 2 | — | type 4, material 27 | 1 × (3, 7, 2); 1 × (3, 8, 2) |
   | 3 mage | subtype 10, material 2 | — | type 4, material 26 | 2 × (3, 12, 11); 1 × (3, 11, 2) |
   | 4 rogue | subtype 3, material 1 | subtype 3, material 1 | type 4, material 28 | 2 × (3, 4, 1); 1 × (3, 5, 1) |

   (Item names, for orientation only: subtype 0 sword, 3 dagger, 4 fist weapon, 5 longsword,
   6 bow, 7 crossbow, 8 boomerang, 10 staff, 11 wand, 12 bracelet, 13 shield, 17 great mace;
   materials 1 iron, 2 wood, 11 gold, 26 silk, 27 linen, 28 cotton.)
4. For every class: 5 × (type 1, subtype 1), one call each.
5. Each equipped weapon or chest that is not empty gets region = P's current region
   (+0x150, +0x154) and modifier = `rand()`, in the order left weapon, right weapon, chest.
   Then every inventory item of type 3…9 or 0x1a (in tab order, slot order) gets region = P's
   region and modifier = `rand()`.
6. Health = its maximum (0x5fac0). The known-recipe list (P+0x1528+0x18) is emptied and the
   default recipes are added (0x9afb0 with 0, §5.6).

Because the kit is rebuilt on every look change, choosing a class shows its gear on P at once.

### 5.6 Default recipes (0x9afb0)

The routine adds items to P's known-recipe list (each only if P does not already know it;
0x5f440 tests, the entry has rarity byte = the routine's argument). Always: (1, 1), (1, 2),
(1, 4), (1, 10), (1, 8), (1, 11), (1, 7). With argument 0 also: (0x1c, 0…6), (0xb, 0x1a),
(0xb, 0x16); for class 4 (0xb, 9, material 0x1c); class 2 (0xb, 9, material 0x1b); class 3
(0xb, 9, material 0x1a) (asked twice; the second is a no-op); then (0xb, 10) with materials 1,
2, 11 and 12. Pairs are (type, subtype).

### 5.7 The name

* The Create Character button is shown only while the name field holds **2 to 15 characters**
  (UTF-16 code units; 0x103200). There is no other check (any characters, spaces included).
* On create the name is converted to UTF-8 (0x354c30) and copied byte by byte, with its
  terminating zero, into P+0x958 (a 16-byte field). Names whose UTF-8 form is longer than 15
  bytes overrun the field (open question 5).

### 5.8 Create Character (0x9ea10)

1. Hide the creation layer.
2. P's name = the field (§5.7); level 1.
3. P's home cell (+0x1d0, +0x1d4) is added to the knowledge's set of known cells (+0xa8).
4. **Show the help sheet** (GUI[0x4f], the HelpWidget listing the controls).
5. `num` = the old count + 1 is written; G+0x2264 = the old count; P is saved under that key
   (0xa27b0); `slot` = that index is written.
6. A copy of P (position, inventory, money, buffs, the look and recipe parts of the
   knowledge) is appended to the in-memory list; the GUI lists are refreshed (0x107440,
   0x107690) and the inventory screens rebuilt (0xb2180).

The player is now in the world at the start spot, wearing the kit.

---

## 6. Deleting a character (0x9dea0, Delete Character)

There is **no confirmation dialog**: the small toggle button (§2.3) is the only guard.

1. The selected character's record is destroyed and removed from the in-memory list (the
   following ones move up).
2. `num` = the new count is written.
3. The key of the selected index is deleted from the database.
4. For i from the selected index up to the new count: key `i + 1` is renamed to `i` (so the
   keys stay 0 … num − 1).
5. The list is rebuilt (0x107440).

`slot` is not updated; after deleting a character before it, the remembered slot points at a
different character (or past the end, in which case Continue is not offered).

---

## 7. Continue (root menu)

Only offered when `slot` names an existing character. It loads that character into P, hides
the title, sets the camera distance to 8 and refreshes the map knowledge
(`frontend-title.md` §4.2). The camera angles are not reset.

> **Erratum (2026-10-28).** "Loads that character into P" means the full reset-and-read of
> `frontend-title.md` §6.1: the player ends up standing at its saved position, motionless,
> ability 0, facing yaw 0. It does not keep the flyover's position and then fall.

### 7.1 CubeVulkan today (audit, 2026-10-28)

`src/FrontendRun.cpp`: the Continue click (~892) and Select (~905–913) only call
`decide(Result::Play, slot)`; when the screens close (~1412–1416) the native player's ability is
set from 0x9d to 0. Neither reloads the selected character into the native player record, so
that record keeps the position the flyover gave it (at least 150 blocks above the ground), its
velocity (10, 5, 0) and the rest of its riding state; with the ability cleared it falls. The
record is placed on the title by `Session::putOnStage` (~513), which is the start-up step of
`frontend-title.md` §1.6, not the Continue step. Afterwards `main.cpp` (~886–892) restores
`player.position` from CubeVulkan's own `SaveGame` only when `saveGame.world` equals the
extracted world's folder name; with the native player (`[Player] Native`),
`SpawnedCreatures::setPlayer` (`Spawned.cpp` ~1312–1329) re-places the record only if the two
differ by half a block or more and never clears its velocity, and the native step moves the
record before that, so the flyover state can win. The fix is `frontend-title.md` §6.1: on
Continue and Select rebuild the native record from the save (position, zero velocity, ability 0,
flight fields cleared, yaw 0) before the first in-game step, and treat unloaded zones as solid.

---

## 8. The world screens (present but never shown)

The interface also builds a world-select layer (GUI[0x10]: a list of WorldPreviewWidgets,
"New world" entry, Select 0x9fd10, a delete toggle and Delete World 0x9e5e0, and a
"Multiplayer Worlds…"/"Singleplayer worlds…" filter button 0x9ffa0 for names starting with
`online_`) and a world-creation layer (GUI[0x1d]: "Name" and "Seed (must be a number)" fields,
Create World 0x9f060, shown only when the seed is all digits and the name has at least 2
characters). Their handlers would store the chosen seed and name in P's knowledge
(+0x2c8, +0x2d0) and ask the world to reload with them (0x8a500).

**No code shows the world-select layer**: every instruction that reads that layer's node was
checked; the only one that shows it is Back from the world-creation layer, which is itself
reachable only from world select. The worlds database those screens use (G+0x2278) is
constructed but never opened. In this build every character therefore plays in the one
world created at start-up: **seed 123, named "world"** (`frontend-title.md` §1). An engine
does not need these screens for parity.

---

## Verification

* **The file format**: open the owner's `Save/characters.db` with SQLite: expect the
  `blobs` table and keys `num`, `slot`, `0`… ; each character value inflates with zlib.
  Comparing the inflated bytes with the in-game record needs the serialiser's spec (open
  question 1).
* **Creation**: an oracle request that, after each arrow or grid click in the creation
  screen, dumps P's race (+0x64), class (+0x14c), appearance (+0x78, 0xb0 bytes), knowledge
  +0x00…+0x13, equipment (+0x278, 11 × 0xa0) and inventory tabs, plus P+0x968. Expect §5.4
  and §5.5 exactly, including the `rand()` modifiers from `srand(low 32 bits of P+0x968)`.
* **Start spot**: dump the cell 0x2d1100 returns for several new characters (not
  reproducible, but every result must pass §4.2's tests).
* **Lists and deletion**: screenshots of the select screen with 0, 1 and 4 characters at
  1920 × 1080; `characters.db` keys before and after a delete.

## Open questions

1. **The creature serialisation** (0x149760 / 0x147800) used inside each character blob is not
   specified; an engine that wants to read the owner's `characters.db` needs it.
2. **0x9bb20's extra recipe** (type 1, subtype 7) is added on every load; whether that is a
   migration for old saves was not established.
3. **The start-spot tests** call world-generation measures (0x2b9750, 0x2d9f30, 0x2bb7e0,
   0x2a44a0, 0x2e64e0, 0x2d4730, 0x2e5f80, 0x2b92f0, 0x2d01a0) that are not all specified.
   Oracle: record many start cells and their zone data.
4. **Positions with negative y** (name field −50, Create Character −20, from the resize
   routine 0x101bd0 through 0x334510): whether negative values count from the bottom edge was
   not traced. Oracle: a screenshot of the creation screen at two window heights.
5. **Long UTF-8 names** overrun P+0x958 into P+0x968 (the account id). Whether the edit
   field limits input length (so this cannot happen) was not checked. Oracle: type a
   15-character name of non-ASCII letters and dump P+0x958…+0x970.
6. **The arrow buttons' positions** come from `gui.plx` templates placed by the style widget's
   constructor and resize hook (0x26fda0, 0x272670); they were not extracted. Oracle: a
   screenshot, or the extracted `gui.plx` with 0x272670's placement.
7. **What "+0x998" and the intent (0, 8, 0) do** in the preview walk is assumed to drive the
   walking animation; checked only by reading. Oracle: record the preview creature's pose
   while hovering an entry.
8. **Experience, money, skill points and specialisation of a new character.** §4.1 resets
   what 0x64a90 and the Select handler reset; 0x64c00 (called first by 0x64a90) was not read,
   so whether XP (+0x148), money (+0xaac), skill points and the specialisation byte carry over
   from the previous P was not established. Its stats (health, attack, armour…) follow from
   level 1, the class and the kit through `stat-rules.md`. Oracle: create a character right
   after playing a high-level one and dump P+0x140…+0x160 and +0xaa0…+0xab4.
