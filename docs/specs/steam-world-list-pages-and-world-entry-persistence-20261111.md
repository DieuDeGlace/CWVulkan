# Steam world list pages and world entry persistence

Status: FINAL / STABLE / HELD (source-side behavioural contract, code-free).
Date: 2026-11-11.
Scope: the supported Steam image only (identity in section 12). Everything
below describes the original program's observable behaviour. Nothing here is
implementation advice.

Companion contract (read-only context): "Steam pending source selection and
world reconfiguration" (2026-11-10). Its shorthand is reused here:

- G = the Game object (0x2288 bytes).
- W = the live World (pointer at G+0x08).
- I = the frontend/interface object embedded in the Game at G+0x118. I keeps a
  back pointer to G at I+0x5C0. Because I = G+0x118, an interface field at
  I+n is the same memory as G+(n+0x118); both spellings appear below where the
  original code uses both.
- "Node" = an element of the interface library's scene tree. Every node has a
  widget/transform object at node+0x70 and a property block at node+0x68. The
  node's visibility flag is the dword at index [node+0x68 -> +0xAC] of the
  array at [node+0x68 -> +0xE0]; "show" stores 1 there, "hide" stores 0. The
  tiny routine at RVA 0xA3A20 performs exactly this store (non-zero byte
  argument becomes 1). Visibility tests read the same dword (RVA 0x9AF00).
- "Text object" = the 32-byte narrow string (inline capacity 15). "Wide text"
  = the 32-byte UTF-16 string object (inline capacity 7).
- All offsets hexadecimal; dword = 32 bits, qword = 64 bits; coordinates are
  interface pixels, x to the right, y downward.

---------------------------------------------------------------------------

## 0. Headline findings (read first)

H1. Both world pages are constructed but are statically unreachable from the
title / character flow in the supported image.

- The world selection page (node at I+0x80) is made visible by exactly one
  code path: the Back handler (RVA 0x9DDB0) when the world creation page is
  visible.
- The world creation page (node at I+0xE8) is made visible by exactly one
  code path: the "select world" handler (RVA 0x9FD10) when the current index
  is not below the entry count. That handler is bound to a button that lives
  on the world selection page.
- Every other store to either page's visibility flag writes 0 (construction,
  the two handlers above, the "create world" handler).
- The character selection handler (RVA 0x9F7C0) neither shows nor hides
  either world page. A successful character selection goes straight to
  gameplay; a failed one shows the character creation page.
- Evidence: listing scans of every load of the two page pointers (G+0x198 /
  I+0x80 and G+0x200 / I+0xE8) that is followed by a visibility store, plus
  manual decoding of the Back handler, which the analysis listing left
  undecoded. See limit L9 for what this cannot exclude.

H2. The world-list store is never opened and never read.

- The Game's second key/value store object lives at G+0x2278 (its database
  handle is the qword at G+0x2280). The Game constructor builds it with a
  null handle (helper RVA 0x66970) and the Game destructor releases it (RVA
  0x669A0). The only store-open routine (RVA 0x67070) has exactly two callers
  in the whole image: the Game constructor opening "Save/characters.db" into
  the first store at G+0x2268, and the World seeding routine opening
  "Save/world.db" into the World's store. No path opens G+0x2278 and no path
  copies a handle into G+0x2280.
- No routine reads a blob from G+0x2278. The world list (G+0x1790..G+0x17A0)
  is therefore empty at every start-up; it is filled only by the "create
  world" handler during the session.
- All writes aimed at G+0x2278 (the "num" key and one key per entry) are
  issued against a store whose handle is null. What the database library does
  with a null handle was not closed (limit L1); the game ignores the result
  either way.

H3. Entries can be created and deleted through the pages; they cannot be
renamed. No random or generated seed exists: both edit fields start empty.

---------------------------------------------------------------------------

## 1. Objects, fields and identifiers

### 1.1 Page nodes (all created in RVA 0xF9AC0, all children of the GUI root)

| Field | Page | Initial visibility |
| --- | --- | --- |
| I+0x00 (G+0x118) | title / start page | shown (1) |
| I+0x18 (G+0x130) | character selection page | hidden (0) |
| I+0x58 (G+0x170) | character creation page | hidden (0) |
| I+0x80 (G+0x198) | world selection page | hidden (0) |
| I+0xE8 (G+0x200) | world creation page | hidden (0) |

Each page is created, in the order of the table, through the node factory at
RVA 0x33CD30 with its default (empty) drawable and widget objects, an empty
name and the GUI root
(the node at gui+0x140, gui = the object at G+0x20) as parent; the initial
visibility value is stored immediately after each creation.

### 1.2 World-selection sub-object (I+0x80 .. I+0xE7)

Initialised by the routine at RVA 0xF87D0 (called from the interface
constructor at RVA 0xF8690, itself called by the Game constructor):

| Field | Meaning |
| --- | --- |
| I+0x80 | page node (filled by 0xF9AC0) |
| I+0x88 / I+0x90 / I+0x98 | sequence of card nodes: begin / end / capacity |
| I+0xA0 | ordered map: card node pointer -> list index (signed dword at map element +0x28) |
| I+0xB0 | ordered map: list index -> card node pointer (qword at map element +0x28) |
| I+0xC8 | "Select" button node |
| I+0xD0 | small button node (reveals Delete World) |
| I+0xD8 | "Delete World" button node |
| I+0xE0 | float: drag-scroll momentum, initialised 0.0 |
| I+0xE4 | byte: list mode, 0 = single-player worlds, 1 = multiplayer ("online_") worlds; initialised 0 |

### 1.3 World-creation widgets

| Field | Widget |
| --- | --- |
| I+0xE8 | page node |
| I+0xF0 | "Create World" button node |
| I+0xF8 | seed label node, name "worldSeed" |
| I+0x100 | name label node, name "worldName" |
| I+0xC0 | "Multiplayer Worlds..." toggle button node (child of the GUI root, not of a page) |
| I+0x340 | shared "Back" button node (child of the GUI root) |

### 1.4 Game fields

| Field | Width | Meaning |
| --- | --- | --- |
| G+0x1770 | signed dword | current world index; set to 0 by the Game constructor (RVA 0x7EB4F) |
| G+0x1790 / G+0x1798 / G+0x17A0 | qwords | sequence of world-entry pointers: begin / end / capacity; all zero after construction |
| G+0x1D60 / G+0x1D64 | signed dwords | interface width / height in pixels (stored by RVA 0xA2720 before every layout pass) |
| G+0x2278 | 0x10-byte store object | world-list key/value store (never opened, H2) |

### 1.5 World entry (0x38-byte heap object)

Created only by the "create world" handler (RVA 0x9F060; the type table is
stored at RVA 0x9F302, the only constructor-side reference to it). Layout:

| Offset | Width | Content | Initial value |
| --- | --- | --- | --- |
| +0x00 | qword | type table pointer (RVA 0x436008; first entry = destructor RVA 0x86670) | set |
| +0x08 | qword | optional preview object pointer (0xA8-byte voxel model) | 0 (null) |
| +0x10 | 32 bytes | name, narrow text object (length at +0x20, capacity at +0x28) | empty |
| +0x30 | signed dword | seed | NOT initialised before the seed extraction (U2) |
| +0x34 | signed dword | "explored" figure, shown as "Explored: <value> km²" | 0 |

- No code in the image ever writes +0x34 other than this zero initialisation;
  it is only read (by the persistence routine and the card drawing).
- The preview pointer is written only by the persistence routine (section
  5.2 step 2) and released by the entry destructor and by that same step.
- Destructor (RVA 0x86670): if +0x08 is non-null, destroy the preview through
  its first table entry with flag 1; destroy the name; free the entry when the
  flag bit 0 is set.

### 1.6 Preview object (referenced by entry+0x08)

A 0xA8-byte voxel model built by the copy constructor at RVA 0x16E790 from
the source model found at [[G+0x10] + 0x170] + 0xD0. Fields used here: three
signed dword dimensions at +0x88 (X), +0x8C (Y), +0x90 (Z) and a colour
buffer pointer at +0x68 holding X*Y*Z cells of 3 bytes each. The identity of
the object at G+0x10 and the voxel model internals are not closed (limit L6).

---------------------------------------------------------------------------

## 2. Construction in RVA 0xF9AC0 (interface build)

### 2.1 Prototypes

Widget prototypes are looked up by name in the loaded "gui.plx" scene tree
(lookup routine RVA 0x332190) and detached from it (RVA 0x3347F0): "edit",
"button", "button2", "smallbutton", and later "blackwidget" (stored at
I+0x310). A clone of a prototype is made with RVA 0x3308E0 (prototype,
parent). Appearance, hover/press feedback, fonts and caret behaviour of
these prototypes come from gui.plx and the interface library (limit L2).

### 2.2 Event registration

Registration helper RVA 0xF8410. Arguments: the clone's widget (node+0x70),
an event kind, the receiver G (read from I+0x5C0), the handler RVA, and two
flag bytes both set to 1. Handlers receive G as their only argument.

| Order | Node | Prototype | Parent | Size (w x h) | Caption | Kind | Handler RVA | Visible after build |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | I+0x340 Back | button | GUI root | 200 x 40 | "Back" | 2 | 0x9DDB0 | per frame (2.5) |
| 2 | I+0x38 char Select | button2 | I+0x18 | 200 x 30 | "Select" | 3 | 0x9F7C0 | per frame |
| 3 | I+0x40 char small button | smallbutton | I+0x18 | 20 x 20 | none | 3 | 0x9F600 | hidden |
| 4 | I+0x48 "Delete Character" | button | I+0x18 | 300 x 30 | "Delete Character" | 3 | 0x9DEA0 | hidden |
| 5 | I+0x70 "Create Character" | button | I+0x58 | 300 x 40 | "Create Character" | 3 | 0x9EA10 | per frame |
| 6 | I+0xC8 world Select | button2 | I+0x80 | 200 x 30 | "Select" | 3 | 0x9FD10 | per frame (3.4) |
| 7 | I+0xD0 world small button | smallbutton | I+0x80 | 20 x 20 | none | 3 | 0x9F630 | hidden |
| 8 | I+0xD8 "Delete World" | button | I+0x80 | 300 x 30 | "Delete World" | 3 | 0x9E5E0 | hidden |
| 9 | I+0xF0 "Create World" | button | I+0xE8 | 300 x 40 | "Create World" | 3 | 0x9F060 | per frame (4.2) |
| 10 | I+0xC0 multiplayer toggle | button | GUI root | 300 x 40, initial position (40, 40) | "Multiplayer Worlds..." | 3 | 0x9FFA0 | per frame (2.5) |

- "Size" is the value given to the widget size setter (RVA 0x32C9A0); the
  position setter is RVA 0x32B560.
- Captions are set with the text setter RVA 0x334C50 (it changes the text of
  the node's text drawable and of every descendant's). All captions are
  literal English UTF-16 strings in the image. None of them goes through the
  dictionary (dict_*.xml): there are no dictionary keys behind any text on
  these pages.
- Event kinds 2 and 3: the reconstruction lead names them "pointer pressed"
  and "pointer released". This was not confirmed in the original dispatcher
  (limit L2).
- Creation order of the world-related groups inside 0xF9AC0: pages (1.1) ->
  prototypes -> Back -> character-select buttons -> character-creation
  button -> world-select buttons (rows 6-8) -> world-creation labels and edit
  fields (2.3) -> "Create World" -> character-creation name label -> the
  multiplayer toggle -> ... -> at the very end the character card rebuild
  (RVA 0x107440) and then the world card rebuild (RVA 0x108450, section 3.2),
  called at RVA 0x100830.

### 2.3 World-creation labels and edit fields

Built in this order (name group first, then seed group):

1. A text drawable is created (RVA 0x33DA70, 0x338-byte object) and set up:
   primary colour at drawable+0x120 = (1, 1, 1, 1); size field at
   drawable+0x2B8 = 12.0; secondary colour at drawable+0x1A8 = (0, 0, 0, 1);
   the two float fields at drawable+0x2BC and drawable+0x2C4 = 3.0; text at
   drawable+0x98 = the caption; the dword at drawable+0x2F8 = 1; font file
   at drawable+0x2C8 = "resource1.dat"; then its refresh entry (table slot 1)
   is called with 1.
2. A node named "worldName" (seed group: "worldSeed") is created with that
   drawable, parent = the creation page I+0xE8; the node pointer is stored at
   I+0x100 (seed group: I+0xF8).
3. A second text drawable is created with: primary colour (1, 1, 1, 1); the
   size field copied from the first; drawable+0x2C4 copied from the first; the
   text copied from the first; drawable+0x2F8 = 1; font "resource1.dat"; then
   refreshed. It gets no secondary colour and no drawable+0x2BC store. It is
   attached as a child node of the first label node and carries the same
   name ("worldName" / "worldSeed"). The visual effect of the two stacked
   drawables (most likely an outlined caption) is a provider detail (L2).
4. An "edit" prototype clone is made as a child of the first label node. Its
   position (RVA 0x32B560) is: x = V.x - 0.5 * (edit width), y = V.y + 10,
   where V is the current two-component value of the property block at
   label-drawable+0x70 (array at drawable+0xE0, index at drawable+0xAC) and
   the edit width is the widget width getter (RVA 0x32DA30). The meaning of V
   (text extent or anchor) is not closed (L2).
5. The text of the descendant named "edit" (string at RVA 0x437B38) of the
   label node is set to the empty string (RVA 0x3348E0 with flag 1, which
   uses the caption setter).

Captions: name group "Name"; seed group "Seed (must be a number)".

### 2.4 Layout pass (RVA 0x101BD0; run on every resize through RVA 0xA2720
and once from RVA 0x96E70)

Inputs: interface width Wd and height Ht (integers, then floats). The
placement helper RVA 0x334700 receives a node, an anchor point (x, y) and an
offset pair (ox, oy); the results are consistent with "top-left = anchor +
offset" for every case below, but the helper's internals (RVA 0x334510) were
not closed (L2).

| Node | Anchor | Offset |
| --- | --- | --- |
| Back (I+0x340) | (0, 0) | (20, 20) |
| Multiplayer toggle (I+0xC0) | (Wd, 0) | (-(toggle width) - 20, 20) |
| Name label (I+0x100) | (Wd/2, Ht) | (0, -50) |
| Seed label (I+0xF8) | (Wd/2, Ht) | (0, -100) |
| "Create World" (I+0xF0) | (Wd - its width, Ht - its height) | (-20, -20) |

(Wd/2 is an integer halving rounded toward zero.) The world-select buttons
and the cards are not placed here; they are placed every frame (3.3, 3.4).
No scale factor is applied anywhere on these pages: all sizes are absolute
pixels and only positions follow the window size.

### 2.5 Per-frame visibility rules shared by all frontend pages (RVA 0x103200)

Every frame, regardless of which page is visible:

- The multiplayer toggle (I+0xC0) is shown exactly when the world selection
  page is visible, hidden otherwise.
- The Back button (I+0x340) is shown exactly when at least one of: character
  selection, character creation, world selection, world creation is visible.

---------------------------------------------------------------------------

## 3. World selection page

### 3.1 Card list concept

The world list is a vertical list of cards, one per shown entry, plus one
trailing "New world" card. There is no scroll bar, no paging and no
keyboard navigation; scrolling is by dragging with the primary pointer
button (3.3).

### 3.2 Card rebuild (RVA 0x108450, complete owner)

Called at the end of construction, after "create world", after
"delete world", and after the multiplayer toggle. Input: I. Order:

1. Detach the Select, small and Delete World buttons (I+0xC8, I+0xD0, I+0xD8)
   from their parent.
2. Destroy all remaining children of the page node I+0x80 (RVA 0x330A80).
3. Empty the card sequence (end = begin), then empty both maps (I+0xA0,
   I+0xB0).
4. Let n = the entry count ((G+0x1798 - G+0x1790) / 8). For i = 0 .. n
   inclusive (n+1 iterations; n is re-read each iteration), with a running
   card counter k starting at 0:
   a. For i < n: entry = list[i]. The entry is an "online" entry when its
      name is at least 7 bytes long and its first 7 bytes are exactly
      "online_". A card is made for it only when that online flag equals the
      mode byte at I+0xE4; otherwise this i is skipped (no card, k unchanged).
   b. For i = n: entry = null; a card is always made, in both modes.
   c. Card creation: a 0x200-byte card widget is constructed (RVA 0x294D20)
      on a fresh instance of the "blackwidget" template parented to I+0x80
      (RVA 0x333890). The card stores the entry pointer at card+0x1E8 (null
      for the New world card), G at card+0x1F0, a rotation float at
      card+0x1F8 = 0.0, and selects the font file "resource1.dat".
   d. Size 600 x 220; position (300, 50 + 230*k). k increases by 1 per card
      made.
   e. Append the card's node to the sequence I+0x88; record node -> i in
      I+0xA0 and i -> node in I+0xB0. The index recorded is the list index i
      (so skipped entries leave gaps), and the New world card's index is n.
5. Finalise the page's widgets (RVA 0x333820) and re-attach the three buttons
   to I+0x80 in the order Select, small, Delete World (so they draw above the
   cards).

### 3.3 Per-frame card placement and scrolling

Executed in RVA 0x103200 while the page I+0x80 is visible (window
0x103C49..0x10457B). Let dt be the frame-time argument of 0x103200, Wd/Ht
the interface size, and "smooth(dt, r)" the frame-rate-independent blend
factor returned by RVA 0xA7A0 for rate r (its formula is not closed, L3).
"y" and "height" of a card are the widget getters RVA 0x32DAB0 and 0x32AD10;
"x" and "width" are RVA 0x32DA70 and 0x32DA30.

When the primary pointer button is NOT held (bit 0 of the byte at gui+0x1A8
clear) and at least one card exists:

1. Bounds. total = (last.y + last.height) - first.y, truncated to integer.
   If total < Ht - 20: top = Ht/2 - total/2 and bottom = Ht/2 + total/2 -
   last.height (integer halvings). Otherwise top = 10 and bottom =
   (Ht - 10) - last.height.
2. If first.y > top: delta = smooth(dt, 0.01) * (top - first.y); every card
   is moved to x = Wd/2 - 0.5 * width, y = y + delta.
3. Then, re-reading positions: if first.y < top AND last.y < bottom:
   delta = smooth(dt, 0.01) * (bottom - last.y); every card moved likewise.
4. Momentum pass for every card in order: x = Wd/2 - 0.5 * width,
   y = y + (I+0xE0) * 0.001 * dt. Immediately after placing a card, if the
   pointer lies over that card's node (hit test RVA 0x333E70 at the pointer
   position gui+0x180 / gui+0x184), its index from I+0xA0 becomes the
   current index G+0x1770; if that differs from the previous current index,
   the Delete World button is hidden first. When the pointer is over no
   card, G+0x1770 keeps its previous value (selection is "sticky").
5. Momentum decay (every frame while the page is visible, held or not):
   I+0xE0 = I+0xE0 + smooth(dt, 0.005) * (0 - I+0xE0). When the page is not
   visible, I+0xE0 is set to 0.

Dragging (pointer-move routine RVA 0x101160, complete owner): a smoothed
pointer y is kept at I+0x548 (blend rate 0.01 over the milliseconds elapsed
since the previous call, measured with the multimedia timer). While the world
page is visible, the character page is not, and the primary button is held:
every card is moved by the change of that smoothed y (x re-centred as
above), and I+0xE0 grows by 2 * elapsed_ms * (pointer y - previous pointer y)
(gui+0x184 minus gui+0x18C). While the button is held, step 1-4 of the frame
routine are skipped, so the hovered index cannot change during a drag.

### 3.4 Current card, Select / small / Delete buttons (per frame)

After 3.3, still in RVA 0x103200:

- If the button is not held and the map I+0xB0 has a card for the current
  index G+0x1770: with C = that card,
  - Select (I+0xC8) is placed at x = C.x + C.width - Select.width - 20,
    y = C.y + C.height - Select.height - 20, and shown;
  - the small button (I+0xD0) is placed at x = C.x + C.width - small.width -
    20, y = C.y + 20, and shown;
  - Delete World (I+0xD8) is placed at x = C.x + C.width + 20, y = C.y; its
    visibility is not changed here.
- Otherwise (button held, or no card for the index, including index -1): all
  three are hidden.
- Then: if G+0x1770 < n (signed comparison against the entry count), the
  Select caption is set to "Select"; otherwise Delete World and the small
  button are hidden and the Select caption is set to "Create".

So the current/selected row is simply the last card the pointer passed over;
there is no separate highlight drawn by the game for it beyond the buttons
appearing on that card and the hover rotation of its preview (3.5). Press
feedback of the buttons is the prototypes' own (L2).

The small button handler (RVA 0x9F630, 6 instructions) only shows the
Delete World button (G+0x1F0 = I+0xD8). Deletion therefore takes two
clicks: small button, then Delete World. The character page mirrors this
with RVA 0x9F600 showing "Delete Character" (G+0x160 = I+0x48).

### 3.5 What each card draws (RVA 0x294EB0, card draw routine)

Text state fields of the card widget (meanings by usage; exact rendering is
the base text-widget's, L2): size at +0x1B0, a secondary width at +0x1B4,
primary RGBA at +0x1C4..+0x1D0, secondary RGBA at +0x1D4..+0x1E0, alignment
code at +0x1E4. The text-draw call takes the text, an x and a y relative to
the card.

At the start of every draw: size 15.0, secondary width 3.0, primary colour
(1, 1, 1, 1), secondary colour (0, 0, 0, 1).

New world card (entry null): one text "New world", alignment code 5, at
x = truncate(0.5 * card width), y = 30. Nothing else.

Entry card:

1. Preview (only when entry+0x08 is non-null): the rotation float card+0x1F8
   is updated: while the pointer is over the card (RVA 0x32B0C0) it decreases
   by 0.05 per unit of the global frame time (dword at engine+0x190);
   otherwise it eases back toward 0 through the angle-blend helper RVA
   0x17840 with factor smooth(frame time, 0.01). The voxel model is then
   drawn through the renderer at G+0x18 (RVA 0xDF0B0) centred on its own
   dimensions (each dimension times -0.5), with a fixed tilt of -2.094395 rad
   (-120 degrees) and a yaw of (rotation + 45) degrees, scaled by 0.008 times
   the card's horizontal scale, in a perspective projection of half-angle
   0.3926991 rad (22.5 degrees) placed at the card's on-screen location. The
   remaining matrix details are not closed (L4).
2. Name: alignment 0, colour (0.25, 1, 1, 1), at (200, 30). The text is the
   entry name converted from narrow to wide (RVA 0x349C30).
3. "Explored: " followed by the dword entry+0x34 converted to float and
   written with the default wide stream float format, followed by " km²"
   (U+00B2). Size 12.0, colour (1, 1, 1, 1), at (200, 55).
4. "Seed: " followed by the signed decimal seed entry+0x30. Size 12.0,
   colour (0.5, 0.5, 1, 1), at (200, 80).

Because +0x34 is never written after creation, every entry shows
"Explored: 0 km²".

### 3.6 Empty list

With no entries the page holds exactly one card, "New world", at (300, 50),
600 x 220, vertically centred by 3.3 when it fits. Hovering it sets the
current index to 0 = n, so Select reads "Create" and the small and Delete
buttons stay hidden. Clicking it opens the creation page (6.2).

### 3.7 Multiplayer toggle (RVA 0x9FFA0, complete owner)

Flips the mode byte G+0x1FC (= I+0xE4). The toggle caption becomes
"Singleplayer worlds..." when the new mode is 1 and "Multiplayer worlds..."
(lower-case w) when it is 0; note the construction caption is
"Multiplayer Worlds..." (upper-case W). Then the cards are rebuilt (3.2).
In mode 1 only entries whose names start with "online_" get cards; in mode 0
only the others; the New world card exists in both.

---------------------------------------------------------------------------

## 4. World creation page

### 4.1 Widgets

The two labels with their edit fields (2.3), the "Create World" button
(2.2 row 9) and the shared Back and (hidden, since the selection page is not
visible) multiplayer buttons. Placement: 2.4. There is no Cancel button;
Back is the only way out other than Create World.

### 4.2 Edit field contents and limits

- Default contents: both empty (2.3 step 5). No code fills either field;
  in particular no seed is generated, randomised or suggested. The fields are
  not cleared after a creation, so a second visit shows the last text typed.
- Accepted characters, maximum length, caret, selection and focus order are
  properties of the gui.plx "edit" prototype and the interface library; the
  game sets none of them (L2). No focus call was found in the construction
  routine or in the handler that shows the page.
- Validation is done only by the visibility of the Create World button,
  re-evaluated every frame while the page is visible (RVA 0x103200, window
  0x10466D..0x104842). Let name = the wide text of the descendant "edit" of
  I+0x100 and seed = the wide text of the descendant "edit" of I+0xF8 (text
  getter RVA 0x333690). Create World is shown exactly when:
  - every UTF-16 unit of seed lies in "0".."9", AND
  - seed is not empty, AND
  - name has at least 2 units.
  Otherwise it is hidden. There is no upper bound in this rule, no minus
  sign is allowed, and the name may contain any characters the edit widget
  accepts. (For comparison, "Create Character" is shown when the character
  name length is 2..15.)

### 4.3 Confirm ("create world" handler RVA 0x9F060, complete owner)

Input: G. Order of effects:

1. Hide the creation page (G+0x200).
2. Camera fields: G+0x1DB8 = 8.0 and G+0x1DB4 = 8.0 (floats);
   G+0x1D98 = 90.0, G+0x1DA0 = 0.0, G+0x1DA8 = 180.0 (doubles); then
   G+0x1D80 = 90.0, G+0x1D88 = G+0x1DA0, G+0x1D90 = G+0x1DA8.
3. Write "num" to the world store G+0x2278: a 4-byte blob holding the signed
   dword (current entry count + 1). This happens before the entry exists.
4. Read the name edit text, then the seed edit text (as wide text).
5. Allocate and initialise a new entry (1.5).
6. Extract a signed 32-bit decimal integer from the seed text with the
   standard wide string-stream extraction into entry+0x30 (U2 for failure
   and overflow results; with 4.2 only digit strings reach here, but values
   above 2147483647 are possible).
7. Convert the name from wide to narrow (RVA 0x354C30, rule not closed, L5)
   and assign it to entry+0x10.
8. Append the entry pointer to G+0x1790 (growing the sequence when needed).
9. Set G+0x1770 = (new count - 1), the new entry's index.
10. Persist the entry: RVA 0xA2BE0 with (G, that index, entry, lock flag 1)
    (section 5.2).
11. Record the selection: RVA 0x8A500 with the entry's seed and the address
    of its name (companion contract section 4: stores into the character
    block D and into G+0x206C / G+0x2070, purges non-local creatures).
12. Rebuild the cards (RVA 0x108450 on I).

No page is shown afterwards: the creation page is hidden in step 1 and the
selection page was already hidden by the handler that opened the creation
page (6.2). With no frontend page visible the game is in gameplay. What
gameplay looks like when the recorded pair differs from the applied World
pair is the companion contract's U1.

### 4.4 Back from the creation page

See 6.1: hide the creation page, show the world selection page. The fields
keep their text.

---------------------------------------------------------------------------

## 5. Persistence

### 5.1 Store

- Object at G+0x2278 (table pointer at +0x00, database handle qword at +0x08
  = G+0x2280). Built with a null handle by RVA 0x66970 in the Game
  constructor (RVA 0x7F1F9). Released in the Game destructor (RVA 0x81CE7)
  by RVA 0x669A0, which prints "release db", closes the handle only if it is
  non-null, then prints a second short line.
- Never opened (H2). If it were opened, it would hold one table
  "blobs(key TEXT PRIMARY KEY, value BLOB)".
- Write helper RVA 0x672C0 / 0x672E0 (complete owners): prepare
  "SELECT 1 FROM blobs WHERE key = ?" on the handle; if preparing fails, return
  without writing or printing. Otherwise bind the key, step, finalise; if a
  row exists, run "UPDATE blobs SET value=? WHERE key=?" (value first, key
  second), retrying the whole prepare/bind/step/finalise while finalise
  returns result code 17 (0x11); else run "INSERT INTO blobs(key, value) VALUES(?, ?)"
  with the same retry, and print "DATABASE WRITE ERROR" if preparing the
  INSERT fails. An empty value is bound as a null pointer with length 0.
  Every caller in this contract ignores the helper's result.
- Keys are narrow text bound with length -1 (terminated text).

### 5.2 Entry persistence (RVA 0xA2BE0, complete owner, 344 instructions,
range 0xA2BE0..0xA316D)

Inputs: G; index i (signed dword); entry pointer; lock flag (byte).
Both callers pass flag 1. Order:

1. If the flag is set, enter the World lock (W+0x3C8, RVA 0x2ABB70).
2. Preview refresh: if the entry name equals the applied World name W+0x200
   (same length and bytes, RVA 0x6DD20), AND the entry seed equals the
   applied World seed W+0x220, AND the source model pointer at
   [[G+0x10] + 0x170] + 0xD0 is non-null: destroy the old preview at
   entry+0x08 if present (first table entry, flag 1), allocate 0xA8 bytes,
   copy-construct a preview from the source model (RVA 0x16E790), and store
   it at entry+0x08 (null if the allocation failed). Otherwise entry+0x08 is
   left as it was.
3. Build the value blob in a growable byte buffer, all integers little-endian:
   - offset 0: unsigned dword L = the low 32 bits of the name's byte length;
   - offset 4: the L name bytes (narrow, no terminator);
   - offset 4+L: signed dword seed (entry+0x30);
   - then print "wrote " followed by the seed as a signed decimal and a line
     end to standard output;
   - offset 8+L: signed dword entry+0x34;
   - offset 12+L: three signed dwords X, Y, Z = the preview dimensions
     (+0x88, +0x8C, +0x90) when entry+0x08 is non-null, else 0, 0, 0;
   - only when X > 0 and Y > 0 and Z > 0: let N = X*Y*Z*3 bytes (the cell
     count is a 32-bit product, the times-3 is 64-bit); copy N bytes from the
     preview colour buffer (+0x68); compress them with zlib 1.2.8 "compress"
     semantics at the default level (wrapper RVA 0x322280, level -1) into a
     buffer of capacity truncate(N_low32 * 1.1) + 12 bytes; shrink the buffer
     to the length the compressor reports; append unsigned dword C = that
     length at offset 24+L and the C compressed bytes after it. The
     compressor's return code is not checked.
4. If the flag is set, leave the World lock (RVA 0x2B9890). The lock is NOT
   held for the key formatting or the store write.
5. Key = the index i written as a signed decimal with a narrow string stream
   in its default (classic) format, e.g. "0", "1", "12".
6. Write (key, blob) to the store G+0x2278 (5.1).

### 5.3 The "num" key

- Key text "num" (3 bytes, at RVA 0x436070), value = 4-byte signed dword.
- Written by "create world" (count + 1, before the entry is appended) and by
  "delete world" (count after removal). Never read from G+0x2278. (The same
  key text is also used, separately, as the character count in the character
  store G+0x2268; that use is outside this contract.)

### 5.4 Loading at start-up

None in the supported image (H2). The entry sequence starts empty, G+0x1770
starts at 0, and the only way to obtain entries is 4.3 during the same
session. Consequently store order, value parsing and error handling of a
world-list load do not exist to be described.

### 5.5 Deletion ("delete world" handler RVA 0x9E5E0, complete owner,
254 instructions, range 0x9E5E0..0x9EA06)

Guard: does nothing unless 0 <= G+0x1770 < entry count. Otherwise, in order:

1. Build the narrow paths "Save\world_" + entry name + ".db" and
   "Save\map_" + entry name + ".db" (backslash separator; ".db" at RVA
   0x437B44).
2. Delete the first file, then the second, with the operating system's
   delete-file call; failures are ignored.
3. Destroy the entry (destructor, flag 1), close the gap in the sequence by
   moving the later pointers down one slot, shrink the end by one pointer.
4. Set G+0x1770 = -1.
5. Write "num" = new count.
6. Re-persist every remaining entry: for j = 0 .. count-1, RVA 0xA2BE0 with
   (G, j, list[j], flag 1). The now unused key for the old last index is not
   removed (the store's delete helper RVA 0x66FC0 is used only by character
   deletion).
7. Rebuild the cards (3.2).

There is no confirmation dialog beyond the two-click reveal (3.4). The files
named in step 1 are not the files the World seeding routine uses (that one
always uses "Save/world.db", companion contract section 2); whether they
exist at all is a runtime question (U4).

### 5.6 Renaming

No path renames an entry. The store's key-rename helper (RVA 0x671F0) is
called only from character deletion (RVA 0x9E30D). Entry names are written
only by 4.3 step 7.

---------------------------------------------------------------------------

## 6. Transitions

### 6.1 Back button (RVA 0x9DDB0, complete owner, 41 instructions, range
0x9DDB0..0x9DE8E, decoded from the image bytes because the analysis listing
did not decode it)

Event kind 2 on the Back widget. Exactly one branch runs, tested in this
order:

1. World creation page visible: hide it, show the world selection page.
2. Else world selection page visible: hide it, show the character selection
   page.
3. Else character creation page visible: hide it, show the character
   selection page.
4. Else character selection page visible: hide it, show the title page
   (I+0x00).
5. Else nothing.

No other store is made (no camera fields, no list or index change, no card
rebuild). No keyboard binding to this routine was found (its only reference
is the registration in 0xF9AC0).

### 6.2 Select / Create button on the world page (RVA 0x9FD10, complete owner,
37 instructions, range 0x9FD10..0x9FDEB)

Order:

1. Hide the world selection page (G+0x198).
2. The same camera-field stores as 4.3 step 2 (in the order G+0x1DB8, then
   G+0x1DB4, G+0x1DA8, G+0x1D98, G+0x1DA0, G+0x1D80, G+0x1D88, G+0x1D90).
3. If the signed G+0x1770 is less than the entry count: tail-jump to the
   selection recorder RVA 0x8A500 with list[G+0x1770]'s seed (+0x30) and
   name address (+0x10). No page is shown: gameplay follows.
4. Otherwise: show the world creation page and return.

Guard: only the upper bound is tested. In practice the button is hidden
whenever the index has no card (3.4), which covers the -1 left by deletion,
but the handler itself would read list[-1] if invoked with a negative index.

### 6.3 Character select (RVA 0x9F7C0) and the world pages

The character Select handler hides the character selection page, enters the
World lock, copies the character index to G+0x2264 and either loads that
character (then writes the "slot" key, rebuilds the crafting-tab interface
parts (RVA 0x107690) and sets the camera fields, never touching the world
pages) or, for an invalid index, shows the
character creation page. It does not show the world selection page.

### 6.4 Summary graph

- Title -> character selection: outside this contract (start page widget).
- Character selection -> gameplay (valid character) or character creation.
- Character selection -> title: Back.
- Character creation -> character selection: Back.
- World selection -> gameplay: Select on an entry card (6.2 step 3).
- World selection -> world creation: Select ("Create") on the New world card.
- World selection -> character selection: Back.
- World creation -> gameplay: Create World (4.3).
- World creation -> world selection: Back.
- Nothing leads into world selection except Back from world creation (H1).

---------------------------------------------------------------------------

## 7. Order of stores, at a glance

| Action | Visibility stores | Data stores | Then |
| --- | --- | --- | --- |
| Select entry | hide I+0x80 | camera fields; recorder 0x8A500 (D, G+0x206C, G+0x2070, creature purge) | gameplay |
| Select New world | hide I+0x80; show I+0xE8 | camera fields | creation page |
| Create World | hide I+0xE8 | camera fields; "num"; new entry; G+0x1770; persist; recorder | card rebuild; gameplay |
| Small button | show I+0xD8 | none | - |
| Delete World | none | delete two files; remove entry; G+0x1770 = -1; "num"; re-persist all | card rebuild |
| Multiplayer toggle | none | flip I+0xE4; caption | card rebuild |
| Back | one hide, one show (6.1) | none | - |

---------------------------------------------------------------------------

## 8. Explicit limits

- L1. Database library (prepare RVA 0x1F1E10, bind, step, finalise, open
  RVA 0x1F0BB0, close RVA 0x1EB6C0): identified by use and statement text
  only. Behaviour when given a null handle (the world store's permanent
  state) was not read.
- L2. Interface library: event dispatch (RVA 0xF8410, meaning of kinds 2 and
  3), node factory, text drawable rendering (font metrics, outline, the
  fields at +0x2B8/+0x2BC/+0x2C4/+0x2F8), widget getters and setters beyond
  "size", "position", "x/y/width/height", the placement helper RVA 0x334510,
  hit testing, the "edit" prototype (input filtering, length, caret, focus),
  the visual content of gui.plx prototypes ("button", "button2",
  "smallbutton", "edit", "blackwidget"), and hover/press feedback.
- L3. The blend factor helper RVA 0xA7A0 (its exact formula and units of
  dt) and the angle blend RVA 0x17840.
- L4. Card 3-D preview projection (RVA 0x294EB0 middle part) and the
  renderer call RVA 0xDF0B0.
- L5. Wide-to-narrow name conversion RVA 0x354C30 and narrow-to-wide RVA
  0x349C30.
- L6. The object at G+0x10 and the source model at its +0x170 -> +0xD0; the
  voxel model class (RVA 0x16E790 and helpers).
- L7. The interface coordinate system is taken from usage (G+0x1D60/G+0x1D64
  as width/height); DPI or scaling inside the library was not examined.
- L8. The standard-library behaviours (wide stream integer extraction,
  float formatting of "Explored").
- L9. H1 rests on the analysis listing plus manual decoding of the undecoded
  handlers found near the frontend code. Generic interface-library writes to
  a node's visibility property (for example from gui.plx animation states or
  library-side input handling) were not enumerated, and undecoded byte
  ranges elsewhere were not decoded by hand.

---------------------------------------------------------------------------

## 9. UNRESOLVED

- U1. Runtime effect of writes to the never-opened world store (null-handle
  database calls): silently failing, an error, or a fault (L1).
- U2. The seed value stored for digit strings that overflow a signed 32-bit
  integer, and for extraction failure in general (entry+0x30 is not
  initialised before extraction; the result follows the runtime library).
- U3. Whether any mechanism outside the decoded code paths (L9) makes either
  world page visible in practice; owner-side runtime verification needed.
- U4. Whether "Save\world_<name>.db" and "Save\map_<name>.db" ever exist in
  the supported image (no writer of those paths was searched for in this
  contract).
- U5. The meaning of the label property value V used to place the edit
  fields (2.3 step 4), and the exact on-screen result of the placement
  helper offsets (2.4).
- U6. Edit field character set, maximum length, focus order and caret
  behaviour (L2).
- U7. Visual appearance of cards, buttons and their hover/press states;
  font metrics of "resource1.dat" (L2).
- U8. The meaning of event kinds 2 and 3 (reconstruction lead: pressed /
  released), not confirmed in the original.
- U9. The identity and lifetime of the preview source model (L6) and the 3-D
  preview drawing details (L4).
- U10. Every limit L1 to L9.

---------------------------------------------------------------------------

## 10. Relation to the companion contract

- Confirms its section 5.1 A/B sequences and adds: "num" is written before
  the entry exists (count + 1); the persistence routine's lock is released
  before the store write; the select handler's camera stores are the same as
  the create handler's.
- Closes its limit L7 (complete owner read, section 5.2).
- Corrects nothing in it. Adds H1 and H2, which mean the two handlers it
  describes are not reachable from the character flow in the supported image
  (subject to U3).

---------------------------------------------------------------------------

## 11. Reconstruction lead usage

The 2026-11-07 authorised reconstruction (release tree) was consulted only
after the original evidence for each point: interface construction lines for
the world pages, the Back handler, the pointer-move routine, and a name
search for world-page visibility. Every fact stated above was confirmed in
the original evidence, except U8 (event-kind names), which is marked as a
lead. Mismatches between the reconstruction and the original evidence: NONE
found for the facts stated. The reconstruction, like the original, contains no
code path that shows the world selection page other than Back from the
creation page. No reconstruction names, expressions or structure are
reproduced here.

---------------------------------------------------------------------------

## 12. Evidence manifest

Supported image:

| Item | Value |
| --- | --- |
| Retail file | cubeworld.exe, Steam release, 64-bit |
| Retail size | 5,939,072 bytes |
| Retail SHA-256 | A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B |
| Installed copy | identical SHA-256 (checked 2026-11-11, read-only) |
| Analysis image | unpacked derivative, 5,757,952 bytes |
| Analysis SHA-256 | 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 |
| Preferred base | 0x140000000 |

All RVAs refer to the analysis image. Evidence was static only: the
instruction listing, the per-routine decompiled listings (read as evidence,
nothing transcribed), the call-reference data, and read-only byte inspection
of the analysis image. No executable was run.

Complete owners (every instruction read; counts from the listing unless
stated):

| RVA range | Role | Instructions |
| --- | --- | --- |
| 0xA2BE0..0xA316D | world entry persistence | 344 |
| 0x9F060..0x9F5FE | create-world handler | 313 |
| 0x9E5E0..0x9EA06 | delete-world handler | 254 |
| 0x9FD10..0x9FDEB | select-world handler | 37 |
| 0x9DDB0..0x9DE8E | Back handler (hand-decoded) | 41 |
| 0x9F600..0x9F620 | show Delete Character (hand-checked; listing misaligned by one byte) | 6 |
| 0x9F630..0x9F650 | show Delete World | 6 |
| 0x9FFA0..0xA007E | multiplayer toggle | 52 |
| 0x108450..0x108917 | world card rebuild | 298 |
| 0x101160..0x1014DF | pointer move / drag scroll | 187 |
| 0x294D20..0x294E63 | card widget constructor | 76 |
| 0x294EB0..0x295CBD | card draw (texts fully; 3-D part by structure, L4) | 679 |
| 0x86670..0x866C5 | entry destructor | 25 |
| 0x16E790..0x16E8BE | preview copy constructor (calls not followed) | 68 |
| 0x66970, 0x669A0, 0x67070, 0x670F0, 0x672C0, 0x672E0 | store construct / release / open / read / write | 5 + 31 + 34 + 68 + 8 + 115 |
| 0x322280, 0x3222A0 | zlib compress wrapper | 5 + 44 |
| 0xF8690..0xF87C9, 0xF87D0..0xF884D | interface constructor, world-selection sub-object constructor | 61 + 30 |
| small helpers 0xA3A20, 0x9AF00, 0xF8D20, 0x32C9A0, 0x32B560, 0x32DA30, 0x32DA70, 0x32DAB0, 0x32AD10, 0x32A1D0, 0x334700, 0x333E70, 0x334C50, 0x3348E0, 0x332190, 0x3347F0, 0x33CD30, 0x33DA70, 0x269160, 0x269190, 0x2691B0, 0x2691C0, 0x2691F0, 0x32B0C0, 0x66FC0 (use only) | visibility, getters/setters, lookup, factories | about 330 |

Totals for complete owners: about 40 routines, about 3,120 instructions.

Bounded partial windows:

| RVA window | Owner | Instructions (approx.) |
| --- | --- | --- |
| 0xFA000..0xFA100, 0xFCA03..0xFDC60, 0x100820..0x100840 | interface construction RVA 0xF9AC0 (5,496 total) | 820 |
| 0x103C43..0x1048F9 plus the per-frame visibility block at its start | frontend frame routine RVA 0x103200 (3,103 total) | 720 |
| 0x101BD0..0x101E46, 0x101F90..0x1020F7 | layout pass RVA 0x101BD0 (665 total) | 200 |
| whole decompiled view, calls list, 0x9F900..0x9F950, 0x9FC00..0x9FCE5 | character select RVA 0x9F7C0 (276 total) | 120 |
| 0x7EB4F..0x7EB79, 0x7F105..0x7F340, 0x7F3C0..0x7F500 area | Game constructor | 170 |
| 0x96F50..0x96F90, 0xA2720..0xA2734 | resize callers | 20 |
| 0x107440 (decompiled view), 0x107690 (decompiled view) | character card rebuild, post-load interface refresh | structure only |

Whole-image checks:

- Raw scan of every section of the analysis image for rel32 call/jump
  targets and absolute 64-bit pointers to 0x67070, 0x670F0, 0x672C0, 0x672E0,
  0x671F0, 0x66FC0, 0xA2BE0, 0x9F060, 0x9FD10, 0x9E5E0, 0x9F630, 0x103200,
  0x108450, 0x101160, 0x101BD0, 0x86670, 0x294D20, 0x16E790.
- Raw scan for RIP-relative address loads of 0x436070 ("num"), 0x436008
  (entry table), 0x470CF0 (card table), 0x470E60 / 0x470E78 (card strings)
  and of every frontend handler RVA (each handler has exactly one reference,
  its registration in 0xF9AC0).
- Listing scans for every access to displacements 0x2278 / 0x2280 (all
  hits other than construction, destruction and the three writers are stack
  slots or unrelated objects), to G+0x1770 / G+0x1790 / G+0x1798 /
  G+0x17A0 and their interface-relative forms I+0x1658 / I+0x1678 / I+0x1680
  (none), to I+0xE0 / I+0xE4 / G+0x1FC writes, and for visibility stores on
  the pointers at G+0x198, I+0x80, G+0x200, I+0xE8.
- Callers of the database open entry RVA 0x1F0BB0 (only RVA 0x67070).
- String scan of the analysis image for the blob SQL statements.

Mismatches between reconstruction leads and original evidence: none for the
stated facts (section 11).

Prohibited-material exposure: none.
