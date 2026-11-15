# Steam HUD aura strip: constants, text, widget effects and placement

Image identity: Steam Cube World x64 image `cubeworld.original.exe.unpacked.exe`,
SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244
(recomputed from the file bytes during this task), preferred base 0x140000000.
All addresses below are RVAs.

Byte-read mapping used: PE section table parsed from the file. All constants read
here lie in `.rdata` (virtual address 0x42C000, raw file offset 0x42A800), so
file offset = RVA - 0x1800. Examples: RVA 0x42E7C8 -> file 0x42CFC8;
RVA 0x441C10 -> file 0x440410; RVA 0x436390 -> file 0x434B90.

Status words: PROVED = established from bytes and/or code read in this task;
UNPROVED = not established; CALLER-STATED = taken from the task context and not
re-read here.

## a) Numeric constants

All of these are consumed as 32-bit IEEE floats (scalar single-precision loads in
the aura rebuild 0x106C20 and the HUD layout routine 0x101BD0; the placement
routine 0x334510 treats both of its vectors as floats). PROVED.

| RVA | Value (float) | Use |
|---|---|---|
| 0x42E7C8 | 120.0 | aura icon vertical offset (y) |
| 0x42E7B8 | 60.0 | money and context-item vertical offset (y) |
| 0x441BC4 | 85.0 | money widget horizontal offset (x) |
| 0x42E7D8 | 255.0 | context-item widget horizontal offset (x) |
| 0x441C10..0x441C1C | 3.0, 3.0, 3.0, 3.0 | colour multiplier vector |
| 0x441BF4 | 3,600,000.0 | "hours" threshold (ms) |
| 0x441BEC | 60,000.0 | "minutes" threshold (ms) |
| 0x441BF0 | 1,800,000.0 | hours rounding bias (ms) |
| 0x432954 | 30,000.0 | minutes rounding bias (ms) |
| 0x42E5C8 | 500.0 | seconds rounding bias (ms) |

Money/context placement location: the calls using 0x441BC4 and 0x42E7D8 with
y 0x42E7B8 are in the HUD layout routine 0x101BD0 (call sites 0x102213 and
0x102235), not in 0x103200. The money widget is the one held at interface +0x200,
the context-item widget the one at interface +0x208. Both use anchor (0, 0): the
anchor register is zeroed at 0x101D0D and not rewritten before those calls.
PROVED (field-to-meaning naming "money"/"context item" is CALLER-STATED).

Remaining-time formatting (kinds other than 0x21 and 0x22), PROVED from the
comparison order in 0x106C20:
- If remaining ms (entry +0x18) is strictly greater than 3,600,000: hours =
  truncate-to-integer(remaining + 1,800,000), then signed integer division by
  3,600,000 (truncating toward zero); suffix from 0x441B94.
- Otherwise, if strictly greater than 60,000: minutes = truncate(remaining +
  30,000) divided by 60,000; suffix from 0x435FCC.
- Otherwise (including zero and negative values): seconds = truncate(remaining +
  500) divided by 1000; suffix from 0x4366F4.
The biases are exactly half of each divisor, so positive values round to the
nearest unit. The divisions are implemented by multiply-high and shift; the
divisor values 3,600,000 / 60,000 / 1000 are as reported by the export and
consistent with the shift amounts (UNPROVED beyond that consistency).

Kinds 0x21 and 0x22: the text is first set to an empty string (UTF-16 empty at
0x42FF38), then set to the prefix at 0x441B8C followed by the decimal integer
obtained by truncating entry +0x14 toward zero. PROVED for the order and
truncation; decimal rendering by the number formatter 0x044320 and the
prepend/append roles of 0x036690 / 0x036620 are CALLER-STATED (those routines
were not read).

## b) Strings (all UTF-16LE, null-terminated)

Width/encoding PROVED: these are passed to wide-string constructors/concatenators
whose small-buffer capacity is 7 characters and whose frees size buffers as
(capacity + 1) * 2 bytes; the bytes are 16-bit code units.

| RVA | Bytes | Text |
|---|---|---|
| 0x441B8C | 78 00 20 00 00 00 | "x " (letter x, then one space) |
| 0x441B94 | 20 00 68 00 00 00 | " h" (space, h) |
| 0x435FCC | 20 00 6D 00 00 00 | " m" (space, m) |
| 0x4366F4 | 20 00 73 00 00 00 | " s" (space, s) |
| 0x436390 | 69 00 6E 00 66 00 6F 00 00 00 | "info" (built with explicit length 4) |

So a stack-type aura reads e.g. "x 5"; a timed aura reads e.g. "2 h", "15 m",
"30 s". PROVED.

## c) Widget effects inside the per-aura sequence

Order per displayed aura (PROVED from 0x106C20): clone template under the aura
container -> 0x335830 on the clone -> multiply the clone's current colour record
-> collect "info" nodes -> 0x335780 on each -> set text -> place.

- 0x335830 (make the clone's visual node private): if the clone's visual node
  (clone +0x60) exists and is shared by more than one widget (share count at
  node +0x298 greater than 1), it duplicates the node through its virtual copy,
  detaches the clone from the old node's user list and attaches it to the copy.
  No visible effect by itself; it ensures that the following colour change
  affects only this clone and not the template or other clones. PROVED.
- Colour change: the routine takes the clone's visual node, selects the record
  for the node's current state (index at node +0x134, 64-byte records starting at
  the address stored at node +0x168), and multiplies the first 32 bytes of that
  record, i.e. two consecutive 4-float vectors, component-wise by
  (3, 3, 3, 3). All four components including the fourth (presumably alpha) are
  tripled. Which visual meaning the two vectors carry (e.g. multiply/add colour,
  gradient ends) is UNPROVED.
- "info" collection (routine 0x332220, PROVED): walks the clone and all its
  descendants, depth first, and collects every node whose name equals "info"
  (the clone itself included if so named).
- 0x335780 (make a node's text component private): for each collected node, if
  its text component (node +0x58) exists and is shared by more than one widget
  (share count at component +0x88 greater than 1), duplicates it through its
  virtual copy and re-binds the node to the copy. No visible effect by itself;
  it keeps the template's "info" text from being overwritten. PROVED.
- 0x334C50 (set text), PROVED: applied to the clone, it is recursive. For each
  node, unless bit 2 of the node's flag word at +0x80 is set (in which case that
  node and its entire subtree are skipped), if the node has a text component of
  kind 3 (component +0x70 equal to 3), the string slot for the component's
  current state (index at component +0xD4, 32-byte string slots starting at the
  address stored at component +0x108) is replaced with the new text when it
  differs. It then recurses into all children (child list at node +0x48). So
  the text lands on every kind-3 text-bearing node in the clone tree (normally
  the "info" child), not on a single named child. Whether any non-"info"
  template node also carries a kind-3 text component is a property of the owner
  layout data and is UNPROVED here; if one exists and was not made private, it
  would share text with the template.

Clone creation 0x333890 (template, aura container as parent) and the meaning
of the earlier lookup/insert routines 0x0F6D10 / 0x0F73D0 were not read:
CALLER-STATED. Observed control flow, PROVED: a first pass over the aura list
ensures every aura kind has an entry in the template map at interface +0x4B8;
in the second pass entries whose template pointer is null are skipped and do
not advance the slot counter, so x positions are consecutive over displayed
auras only. The counter starts at -5 and gains 50 before each displayed aura is
cloned, giving x = 45, 95, 145, ...; the integer counter is converted to float
for placement. The container at interface +0x148 is emptied first (0x330A80,
which destroys all its children). The rebuild call at 0x1048A8 inside 0x103200
is unconditional on its path.

## d) Placement routine 0x334700 / 0x334510

PROVED from code:
- 0x334700 takes the widget plus four floats in order (a, b, x, y), forms an
  anchor point (a, b) and an offset (x, y), and forwards the widget unchanged to
  0x334510. Aura call: anchor (0, 0), offset (45 + 50*i, 120).
- 0x334510: if the widget has a parent (widget +0x40), the anchor point is
  transformed by a 4x4 matrix: the parent's visual node matrix (16 floats at
  node +0x2E0, or the identity at 0x42E630 if the parent has no visual node);
  when the parent itself has a parent, that matrix is first composed with the
  grandparent chain's matrix (0x100EE0, 0x0173E0). The transform uses row-vector
  convention with a homogeneous divide (1.0 at 0x4AC980 divided by the
  w-component). With anchor (0, 0) the result is the parent's transformed
  origin. If the widget has no parent the anchor is used as-is.
- The target position = transformed anchor + offset - the widget's own pivot
  (an (x, y) pair for the current state from the widget's visual node, index at
  +0x244, pairs at the address stored at +0x278).
- If either component differs from the stored position for the current state
  (index at node +0xAC, pairs at node +0xE0), both are written and the node's
  virtual change notification is invoked; otherwise nothing changes.
- No GUI-scale factor is applied inside 0x334700/0x334510: the offset is added
  in whatever units the parent transform space uses.
UNPROVED: whether the anchor is a fraction of the parent's size (only if the
parent matrix encodes the size as scale; not established), whether the stored
position is parent-relative or absolute, how/where the GUI scale is applied
(presumably in the parent/root transform), and which point of the child the
pivot denotes (it is data from the layout).

## e) Visibility of HUD root and aura container

Visibility primitive (PROVED): 0x0A3A20 writes a 0/1 value into the current
state slot of the widget's visibility track (widget +0x68; index +0xAC, values
at +0xE0); 0x09AF00 reads it.

- The aura container (interface +0x148) is created in 0x0F9AC0 at 0x0FB981
  with an empty name and with the HUD root (interface +0x108) passed as its
  parent. PROVED. No writer of the container's own visibility was found among
  direct calls on that field (search of all instructions loading +0x148 into the
  call-argument register). Its visibility therefore follows only its parent, if
  hiding propagates to children (propagation UNPROVED, not read).
- HUD root (interface +0x108) visibility is set every HUD update in 0x103200 at
  0x106714/0x10671B: visible exactly when none of the five interface layers at
  interface +0x00, +0x18, +0x58, +0x80 and +0xE8 is visible. PROVED. Those
  layers are created in 0x0F9AC0 (0x0F9DA0..0x0FA097); +0x00 starts visible, the
  others start hidden. By nearby button/label strings in 0x0F9AC0: +0x18 is the
  character-selection layer ("Select", "Delete Character"), +0x58 the
  character-creation layer ("Create Character", "charactername"), +0x80 the
  world-selection layer ("Delete World"), +0xE8 the world-creation layer
  ("worldName", "worldSeed", "Create World"). Identification by string
  proximity: likely, formally UNPROVED; identity of +0x00 UNPROVED (it is
  visible at construction, so presumably a start/title layer).
- Immediately after, the widget at (interface +0x268)+0x178 is given the HUD
  root's visibility. PROVED (its identity UNPROVED).
- World map: the map is not among the five layers tested; no evidence was found
  that opening the world map hides the HUD root or the aura container. UNPROVED
  either way (other writers via other registers were not exhaustively traced).
- Death: the HUD root expression contains no death/health test. PROVED for that
  expression. Separately, widgets at interface +0x1D8 and +0x1E0 get visibility
  from a test involving a window at (interface +0x300)+0x178, routine 0x1014F0,
  bit 0x400 of the local player's 16-bit field +0x128 and byte +0x68 compared
  with 0x9C; these do not involve +0x108 or +0x148. Meaning UNPROVED.
- In-game menus (inventory, options, etc.): not established. UNPROVED.

## Evidence boundaries

- Read: decompiled routines 0x106C20, 0x334700, 0x334510, 0x334C50, 0x335830,
  0x335780, 0x332220, 0x330A80, 0x0A3A20, 0x09AF00; assembly listing ranges of
  0x106C20, 0x334700, 0x101BD0 near 0x102150..0x102270, 0x103200 near
  0x1054C0..0x105520, 0x106610..0x106760 and 0x104842..0x1048B3, 0x0F9AC0 near
  0x0F9C56..0x0FA16F and 0x0FB8A1..0x0FB9D6, plus pattern searches of the
  assembly listing for the constant addresses and interface field offsets;
  strings.csv lookups for string addresses; functions.csv rows for 0x101BD0 and
  0x103200; raw bytes of the .rdata constants listed above.
- Not read: 0x333890 (clone), 0x036690, 0x036620, 0x044320, 0x0F6D10, 0x0F73D0,
  0x33CD30, 0x100EE0, 0x0173E0, the renderer (so visibility inheritance, GUI
  scale application and colour-record semantics are unproved).
- Writer search for visibility was limited to call sites that load the field
  directly into the first-argument register within four instructions of a call;
  indirect writers could exist.
- No runtime observation; all statements are static-analysis results.

## Exposure statement

Read in this task: docs/CLEANROOM_AUTHOR.md (role policy); and, under
"decompiled cube world steam": binary/cubeworld.original.exe.unpacked.exe (raw
bytes, SHA-256 and PE headers), decompiled/functions/140106c20.c, 140334700.c,
140334c50.c, 140335830.c, 140335780.c, 140334510.c, 140332220.c, 140330a80.c,
1400a3a20.c, 14009af00.c, decompiled/assembly.txt (excerpts and searches),
decompiled/strings.csv, decompiled/functions.csv (header and two rows), and a
directory listing of decompiled/ and the functions folder count. Helper scripts
written only to my scratchpad performed byte reads. No game executable was run,
no web, SDK, reconstruction or implementation material was read.

Harness-placed context before the task: the session context automatically
included the project's AGENTS.md text (project policy summary: process rules,
publication instructions; no implementation source, code or implementation
results) and a git status snapshot (branch name only). I did not open
AGENTS.md or WORKSPACE-MEMORY.md myself. The task prompt itself contained the
caller's summary of 0x106C20 behaviour. No other prohibited material was
encountered.
