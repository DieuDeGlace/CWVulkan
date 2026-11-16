# Steam HUD quick buttons: use-key (E) button visibility rule

## Image identity

- Image: cubeworld.original.exe.unpacked.exe (Steam release, x64), SHA-256
  7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (re-hashed
  during this task), preferred base 0x140000000. All addresses below are RVAs.
- PE section mapping used for raw reads: .text RVA 0x1000 (raw 0x400),
  .rdata RVA 0x42C000 size 0x103316 (raw 0x42A800), .data RVA 0x530000 (raw 0x52DC00).
  Constants quoted below were read from .rdata through this mapping.

## Terms used

- "HUD controller": the object whose per-frame HUD update is routine 0xA5A50
  (called from 0x134558 inside routine 0x134420).
- "Quick-button list": a contiguous array at controller +0x228 (begin pointer) /
  +0x230 (end pointer), element stride 0x20 bytes; the first 8 bytes of each element
  are the button's GUI node. Element 0 is at list+0x00, element 1 (the use button) at
  list+0x20, element 2 at list+0x40, element 3 at list+0x60. Each button node owns
  named text children "name" (key cap, wide string RVA 0x4363C8), "info" (RVA 0x436390)
  and "count" (RVA 0x4363E8); texts and text colours are addressed through the parent
  node by child name (text write 0x3348E0, colour write 0x334A20, text read 0x333690).
- "Visibility property": a GUI node keeps a property block pointer at node +0x68; that
  block holds an index at +0xAC into a 32-bit value array whose pointer is at +0xE0.
  Routine 0xA3A20 stores 1 or 0 there (setter, input boolean) and routine 0x9AF00 reports
  whether it is non-zero (getter). The same property is what the HUD and the
  interaction code test as "panel shown"; it is the visibility flag the lead calls
  Display.visibility. PROVED (the identification as "visibility" rests on its use as the
  shown/hidden gate for panels throughout 0xA5A50; it is the only flag touched).

## a) Which routine writes the use button's visibility

1. PROVED. The only per-frame visibility write for the quick buttons is a loop inside
   the HUD update 0xA5A50 at 0xA9D27-0xA9DBC. It walks every element of the
   quick-button list (begin to end, stride 0x20) and calls the visibility setter 0xA3A20
   on each element's own node (the parent of its "name", "info", "count" and icon
   children). The use button is the element at list+0x20 and is therefore covered.
2. PROVED. No write of visibility, alpha or an enabled flag to the use button's children
   (key cap, info text, count text, icon) was found in 0xA5A50, 0x8AE50 or 0x98180; the
   children are only given texts, text colours and (for the icon) an item. Hiding the
   parent hides everything.
3. UNPROVED. Whether any other routine outside 0xA5A50 / 0x8AE50 / 0x98180 also writes the
   use button's visibility was not exhaustively checked (other callers of 0xA3A20 exist
   in 0xF9AC0 and 0x103200 and were not traced; none of them was seen addressing the
   controller's +0x228 list).

## b) Exact condition, order of evaluation, frame-to-frame state

Per call of 0xA5A50 the use button goes through these steps, in this order:

1. Reset (PROVED), at 0xA6C81-0xA6E91, on the straight-line path of 0xA5A50:
   - use button "count" text set to the empty string (RVA 0x42FF38, a zero wide
     character) at 0xA6D08;
   - the use-button icon item, held by the item widget whose pointer is at controller
     +0x3E8 (item stored at widget +0x1F8), is overwritten with a zero-initialised (empty)
     item at 0xA6D7A;
   - use button "info" text set to empty at 0xA6E06;
   - use button "info" colour set to opaque white (1, 1, 1, 1; RVA 0x42E8C0) at 0xA6E84.
2. Prompt selection (PROVED that it can only run here, and only under a gate). At
   0xA9AC4-0xA9B2C the interaction routine 0x8AE50 is called only when all of the
   following hold, tested in this order:
   - the pointer at (controller +0x8) +0x210 is non-null;
   - the local player creature's state byte (creature +0x68, creature reached through
     (controller +0x8) +0x448) is neither 0x9C nor 0x9D;
   - the panels whose nodes are at controller +0x118, +0x130 and +0x170 are all hidden
     (visibility getter 0x9AF00).
   If the gate fails, nothing refills the use button this frame and it ends hidden
   (step 3).
   Inside 0x8AE50, routine 0x98180 is called at 0x8DF69. 0x98180 is a prioritised chain:
   the first matching case writes one verb into the use button's "info" text and returns
   1; if nothing matches it returns 0. Its own early-outs (return 0, no text) are: byte
   controller +0x1A7A non-zero, byte controller +0x1A7D non-zero, or the panel node at
   (controller +0x418) +0x178 visible. The verbs it can write (wide strings) include
   "Fall" 0x437BA8 (player state 0x7A), "Climb" 0x437BB8, "Revive" 0x437BC8, "Throw"
   0x437BD8, "Pick up" 0x437260, "Examine" 0x437BE8, "Talk" 0x437C18, "Pet" 0x437C28,
   "Interact" 0x437C30, "Open" 0x437C48, "Use" 0x437C58, "Sleep" 0x437C60, "Close"
   0x437C78, "Teleport" 0x437C90 and station names ("Furnace" 0x437CD0, "Anvil"
   0x437CE0, "Spinning Wheel" 0x437CF0, "Loom" 0x437D10, "Saw" 0x437D20, "Workbench"
   0x437D28, "Customization Bench" 0x437D40, "Campfire" 0x437D68). PROVED that these are
   all written to the "info" child of list element +0x20.
   - "Revive" case (0x9868E-0x98716), PROVED: chosen when the local player's health
     float (creature +0x180) is not greater than 0.0. It is reached after the Fall,
     Climb and two state-0x96 / 0x97 timer cases, which do not set text unless their own
     conditions hold. This matches the observed "E REVIVE while dead".
   - "Talk" case at 0x98C93, PROVED as a text write; the full target-classification
     conditions for "friendly townsperson" were not transcribed here (UNPROVED in detail).
   - The use-key press (rising edge of byte controller +0x1A7B against the previous-frame
     copy kept in global RVA 0x5515B4) only selects whether the chosen case also performs
     its action; the text is written whether or not the key is pressed. PROVED.
   When 0x98180 returns 0 and the panel at controller +0x310 is hidden, 0x8AE50 has two
   fallbacks that also fill the use button (PROVED):
   - a nearby ground item (controller +0x1E70 not 0x7FFFFFFF): the icon item at
     controller +0x3E8 is set to that item and "info" becomes "Reserved" 0x437238 with
     colour (1, 0.1, 0.1, 1) RVA 0x438060, or "Pick up", or "Full" 0x437250 with colour
     (1, 0.3, 0.3, 1) RVA 0x4380A0;
   - otherwise, if the panel at (controller +0x418) +0x178 is hidden and a carried item
     exists at ((player) +0x1528) +0x148, the icon is set to that item and "info"
     becomes "Cancel" 0x437350 or that item's display name.
3. Visibility decision (PROVED), loop at 0xA9D27-0xA9DBC, evaluated after step 2 in the
   same call: for each quick button, its "info" text is read through 0x333690 and the
   button is set visible exactly when that text has a non-zero character count (length
   field at +0x10 of the returned wide string) AND byte controller +0x668 is zero;
   otherwise it is set hidden. The info-length test is evaluated first, then +0x668.
   Nothing else enters the decision: not the icon item, not the "count" text, not the
   key cap, not death state directly (death matters only because it makes 0x98180 write
   "Revive").
4. Frame-to-frame state (PROVED for this path): none beyond the text itself. Because the
   use button is cleared in step 1 and refilled in step 2 of the same call, its
   visibility is a pure function of the current frame's prompt selection. No fade timer,
   alpha ramp or named show/hide sequence is involved in the loop; the visibility value
   is written as an immediate 1 or 0 each frame. Whether the GUI layer animates a
   visibility change on its own: UNPROVED (no such call was seen in the loop).
5. Controller byte +0x668 (PROVED as a gate; meaning UNPROVED): set to 1 at 0x98D5A
   (when the use action is taken on a target of type 0x9A), at 0x996A5 and at 0xB78FE
   (both write 1 to +0x668 and +0x669); cleared at 0xAB2D4 (when any of bytes
   +0x1A75..+0x1A78 is set and (controller +0x398) +0x1D8 is zero, or a vector at
   +0x1D6C exceeds 0.01, RVA 0x437F74), and at 0x96D85 / 0x9FE94. While it is non-zero
   every quick button is hidden regardless of text, and a camera path at 0xAB15D runs.
   The player-facing name of this mode is not established.

## c) Item (Q) and skill (R) buttons

- PROVED: they share the same loop and the same rule (non-empty "info" and +0x668 zero).
  They look "always visible" because their info text is normally non-empty.
- Element at list+0x00 (item button; identified by content: it shows the selected usable
  item): its key cap "name" is set at 0xA5BB0 / 0xA6071. Its "count" and "info" are set
  to empty at 0xAC3F3-0xAC496, then, if the usable-item list built by 0x93E60 is not
  empty, the selected item (index controller +0x20C0, wrapped into range) becomes the
  icon (item widget pointer at controller +0x3E0, item at +0x1F8; 0xAC556), "count" gets
  the stack count when the item is countable (0x5F0E0), and "info" gets the item's
  display name (0xAC6F3). These writes happen AFTER the visibility loop in 0xA5A50, so the
  item button's visibility in a given call is decided from the text left by the previous
  call: it is hidden one call after the usable-item list becomes empty and shown one call
  after it becomes non-empty. PROVED (ordering by address within 0xA5A50; both blocks are
  on its main path).
- Element at list+0x40 (skill button; identified by its cooldown text): "count" set empty
  at 0xA6869. If 0x656E0 (queried on the player creature) returns non-zero, "info" becomes
  a name produced through 0x16BE20 (style "default" 0x4363F8) at 0xA699F, and, when the
  cooldown float found at entry +0x20 through 0x7BCA0 is non-zero, "info" is replaced by
  the whole seconds count (milliseconds plus 999.0, RVA 0x4ACA40, divided by 1000) followed
  by " s" (ASCII, RVA 0x436408) at 0xA6AB6. If 0x656E0 returns zero, "info" is set empty
  at 0xA6B64 and the skill button is hidden by the loop. These writes precede the loop,
  so the skill button follows the current call. PROVED.
- Element at list+0x60: its "info" is set empty every call at 0xA6C0A, so it is always
  hidden by the loop. PROVED (what it represents: UNPROVED).
- Key-cap letters, PROVED as written: in one branch the three caps are fixed glyphs
  "X" (0x430EB8, colour 0.4, 0.5, 0.9, 1 at 0x438160) for list+0x00, "A" (0x430E64, colour
  0.4, 0.8, 0.3, 1 at 0x4380B0) for the use button, "Y" (0x430E1C, colour 0.9, 0.9, 0.4, 1
  at 0x4380E0) for list+0x40; in the other branch they are key names converted (0x4CA10)
  from bindings stored at controller +0x1BFC (list+0x00), +0x1C34 (use button) and +0x1BF4
  (list+0x40). That these bindings default to Q / E / R and that the glyph branch is the
  gamepad mode: UNPROVED (consistent with the observation).

## d) Other HUD effects tied to the same decision

- PROVED: immediately after the loop, at 0xA9DC1-0xA9DD3, the node at controller +0x2C0 is
  set visible exactly when byte +0x668 is zero. It shares the +0x668 gate, not the use
  button's text condition.
- PROVED: no HUD element was found that reads the use button's visibility or its info
  text to change itself; the button's visibility has no side effects in 0xA5A50.
- The use button's icon (controller +0x3E8) and info colour change with the chosen
  fallback (ground item / carried item) as listed in b); the "count" text of the use
  button is only ever cleared in 0xA5A50 (no non-empty write to it was found there).
  PROVED for 0xA5A50 / 0x8AE50 / 0x98180.
- The usable-item popup at controller +0x310 ("You don't have any usable items." RVA
  0x436900) is a separate panel; its visibility suppresses the 0x8AE50 fallbacks but is
  not written by the quick-button loop. PROVED.

## Evidence boundaries

- Evidence is static: assembly listing, call table, strings table and raw .rdata bytes of
  the identified image. No game executable was run; nothing here is runtime-verified.
- The full case conditions of the 0x98180 prompt chain (which creature classes yield
  "Talk", ranges, facing) were not transcribed; only the text targets, the Revive health
  test, the Fall state test and the entry early-outs are stated.
- Whether 0x8AE50 always reaches its call to 0x98180 on every path was not proved; the
  call at 0x8DF69 was examined on the path leading to it.
- Meaning of controller +0x668, +0x2C0, +0x118 / +0x130 / +0x170, (controller +0x8)
  +0x210 and player states 0x9C / 0x9D: UNPROVED (raw offsets and values only).
- That 0xA5A50 runs once per rendered frame: UNPROVED (it is called from 0x134420 at
  0x134558; the caller's frequency was not traced).
- Other callers of the visibility setter in 0xF9AC0 and 0x103200 were not traced.

## Exposure statement

- Read in full: docs/CLEANROOM_AUTHOR.md (role policy).
- Read from original Steam evidence only: directory listing of
  "decompiled cube world steam" and its decompiled and binary folders (file names and
  sizes); decompiled/assembly.txt (targeted searches and excerpts around RVAs 0x8DF05-
  0x8E991, 0x98180-0x98D71, 0x9AD42, 0x9AF00, 0xA3A20, 0xA5B37-0xA6F40, 0xA9A50-0xA9E51,
  0xAB053-0xAB51F, 0xAC2A4-0xACADF, plus whole-file searches for the info / count /
  Revive / Talk string references and for +0x228 and +0x668 operands);
  decompiled/calls.csv (searches for callers of 0x333690, 0xA3A20, 0x98180, 0xA5A50,
  0x8AE50, 0xB0680 and the call sequence of 0xA5A50); decompiled/strings.csv (searches and
  the rows near 0x436340-0x437E68); raw bytes and SHA-256 of
  binary/cubeworld.original.exe.unpacked.exe (PE headers and .rdata constants).
- Not read: decompiled/functions/*.c, functions.csv, function_ranges.csv, any other file
  in the project tree, SDKs, reconstructions or the web.
- Harness-placed context before the task: the project's AGENTS.md text (combined project
  policy) and a git status snapshot (branch names, no file contents) were placed in this
  agent's context by the harness before the assignment. They contained no implementation
  source, tests, logs or engine details, and nothing from them was used in this document.
  WORKSPACE-MEMORY.md was not read.
- No reconstruction (Infinite-Cubes) material was consulted.
