# Steam HUD top-left "sprite with x N" element: behavioral contract

## Image identity

- Supported image: Steam Cube World x64 executable, unpacked image
  SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244,
  preferred base 0x140000000. All addresses below are RVAs into that image.
- Evidence: the decompiled export of that image (assembly listing, per-function
  decompilation, function/string/call tables). No executable was run.

## Summary of the finding

The element under the money counter is not drawn by a mission/POI counter and
is not drawn from the mission-marker sprite handle "manapump". It is the HUD
**aura (status-effect) strip**: one cloned icon widget per entry in the local
player's active-aura list, rebuilt every HUD update by the routine at RVA
0x106C20. For two specific aura kinds (0x21 and 0x22) the text under the icon
is a fixed short prefix followed by an integer taken from the aura entry; this
is the "x 1" seen in the recording. For every other aura kind the text is a
remaining-duration readout instead.

The aura kinds 0x21 and 0x22 have their icons registered by name as
"aura:ancientpower" (kind 0x21) and "aura:manaabsorption" (kind 0x22). Which of
the two the recorded purple icon is cannot be decided from code alone (see
Evidence boundaries); the recorded "x 1" text form proves it is one of these two
kinds, because no other kind receives a prefixed count.

The observed correlation with a mana-pump point of interest (mission kind 23)
is therefore indirect: the HUD element shows an aura carried by the player, and
whatever grants that aura (not traced here) is what links it to the mana pump.

## Negative results that redirected the search

- The mission-marker template "manapump" (found by name at load in 0x0F9AC0 and
  stored at interface +0x368 = game +0x480) is read by a single code site in the
  whole image: the map-marker routine 0x2829C0 (mission-kind switch, kind 0x17
  selects it, around RVA 0x284033 and decompiled case 0x17). No other read of
  game +0x480 or interface +0x368 off an interface/game base exists; no indexed
  read of the contiguous template block (interface +0x348..+0x368) exists.
- The only HUD-level code that draws a sprite widget immediately (0x330B20) or
  draws text directly (0x34D360) is 0x2829C0. Everything else on the HUD is a
  retained GUI widget whose text/position is set by code and which the GUI
  framework draws.
- No literal wide string beginning with "x" exists in the string table; the
  prefix is a short (two-character class) constant that the string export does
  not list (see Evidence boundaries).

## Ownership and data sources

- HUD interface object = game object +0x118. The interface keeps a back-pointer
  to the game at interface +0x5C0; the local player is reached through game +0x8
  then +0x448.
- Aura icon table: an ordered map at interface +0x4B8 keyed by aura kind
  (integer), each value node holding an icon template widget pointer at node
  +0x28. It is filled in the loader 0x0F9AC0 by looking up named children of
  the interface sprite library (interface +0x328): kinds and names, in load
  order: 1 bulwark, 2 warfrenzy, 3 camouflage, 4 poisoned, 6 manashield,
  9 instantfire, 10 instantshoot, 11 dodge, 12 swiftness, 13 ninjutsu,
  15 shadowclone, 16 flamerush, 17 regeneration, 19 spiritworld, 22 lifeflask,
  23 attackflask, 24 armorflask, 25 resistanceflask, 27 coldresistance,
  26 heatresistance, 28 plagueresistance, 29 frozen, 33 (0x21) ancientpower,
  34 (0x22) manaabsorption (each name prefixed "aura:"). Kinds not listed have no
  icon.
- Aura container widget: interface +0x148, created empty in 0x0F9AC0 as a child
  of the HUD root (interface +0x108). It holds only the per-frame icon clones.
- Source list: the local player's aura list at player +0x980 (linked list,
  iterated head to tail). Per entry: kind = unsigned byte at entry +0x10;
  integer-valued payload = 32-bit float at entry +0x14; remaining duration in
  milliseconds = 32-bit float at entry +0x18.

## Per-update behavior of 0x106C20

Called once per HUD update from the HUD update routine 0x103200 (call at RVA
0x1048A8), which itself is called from the game frame routine 0x0A5A50 (call at
RVA 0x0AB5CE). Within 0x103200 the call is not inside a conditional branch.

1. All children of the aura container (interface +0x148) are removed (call to
   0x330A80 at RVA 0x106C8B). The strip is therefore rebuilt from scratch every
   update; there is no persistent per-aura widget and no animation state kept
   between updates by this routine.
2. First pass over the player's aura list: for every entry whose kind has no
   node in the icon table, an empty node (no icon) is inserted. This has no
   visible effect.
3. Second pass over the same list, in list order. A running horizontal value
   starts at -5 (constant 0xFFFFFFFB set at RVA 0x106D76).
   For each entry:
   - The icon table node for the entry's kind is looked up. If its icon
     template pointer is null the entry is skipped entirely: no widget, and the
     running value is not advanced (test at RVA 0x106E53).
   - Otherwise the running value is increased by 50 (0x32, RVA 0x106E5C) before
     use, so the first shown entry uses 45, the second 95, the third 145, and so
     on: a fixed 50-unit pitch with no wrapping or row limit in this routine.
   - The template is cloned into the aura container (0x333890).
   - A post-clone step (0x335830) runs on the clone; its effect was not traced.
   - The clone's colour matrix (slot selected by the clone's state index) has
     its first two 16-byte rows multiplied component-wise by a constant 4-float
     vector at RVA 0x441C10 (value not readable from the export).
   - Children of the clone whose name equals a 4-character constant at RVA
     0x436390 are collected and a per-child operation (0x335780) is applied to
     each; effect not traced.
   - Text is set on the clone (GUI set-text 0x334C50), chosen by kind:
     - Kind 0x21 or 0x22 (test "kind minus 0x21 is at most 1", RVA 0x106F86):
       the text is first cleared to the empty string, then set to
       PREFIX followed by the decimal integer obtained by truncating the float
       at entry +0x14 toward zero (RVA 0x1071CC). PREFIX is the wide-string
       constant at RVA 0x441B8C, prepended (0x036690). Its storage slot is 8
       bytes (next constant at 0x441B94), i.e. at most three UTF-16 characters;
       the recording shows it renders as "x " (lower-case x, one space). There
       is no special case for value 1 or 0: a value of 1 shows "x 1", 0 would
       show "x 0", negative values would show a minus sign.
     - Any other kind: remaining-time readout from the float at entry +0x18
       (milliseconds), number followed by a SUFFIX constant (0x036620):
       - if time > constant at RVA 0x441BF4: hours =
         integer(time + constant at RVA 0x441BF0) divided by 3 600 000
         (integer division, truncation), suffix constant at RVA 0x441B94;
       - else if time > constant at RVA 0x441BEC: minutes =
         integer(time + constant at RVA 0x432954) divided by 60 000, suffix
         constant at RVA 0x435FCC;
       - else: seconds = integer(time + constant at RVA 0x42E5C8) divided by
         1000, suffix constant at RVA 0x4366F4.
       The float-to-integer step truncates toward zero before the division.
       Threshold and bias values and the three suffix strings are not readable
       from the export.
   - Placement: the clone is positioned with the GUI placement call 0x334700
     (RVA 0x1072DC) using anchor (0, 0) (top-left of its parent), horizontal
     offset = the running value (45, 95, 145, ...) and vertical offset = the
     constant at RVA 0x42E7C8.
4. If the player has no auras, or none of its auras has an icon, the container
   stays empty and nothing is drawn. Hiding is therefore implied by an empty
   aura list; there is no count-zero rule.

## Relation to the money counter (same placement convention)

In the HUD layout routine 0x101BD0 the same placement call positions the money
text widget (interface +0x200, created in 0x0F9AC0 by the text-widget factory
0x1008A0 with integer style parameters 0x18 and 5, name "gold") with anchor (0, 0), horizontal offset constant
at RVA 0x441BC4 and vertical offset constant at RVA 0x42E7B8 (call at RVA
0x102213), and the context-item text widget (interface +0x208, "contextitem")
with horizontal offset constant RVA 0x42E7D8 and the same vertical constant
(call following RVA 0x102224). The caller's established values for those are
money (85, 60) and context item (255, 60); this author did not read the float
bytes. Under that convention the first aura icon sits at horizontal offset 45
(left of the money anchor by 40 units) and at the vertical offset held at RVA
0x42E7C8; that the recording shows it "directly below the money counter" is
consistent with that constant being larger than the money row's, but the value
is unread.

## Visual properties

- Icon art, icon size, text font, text size, text colour and the text's
  position relative to the icon (the recording shows it centred under the icon)
  all come from the aura icon template in the interface sprite library resource,
  not from code. The only code-side visual change is the colour-matrix multiply
  by the 4-float vector at RVA 0x441C10.
- No animation, pulsing, fading or sorting is applied by 0x106C20.
- GUI-scale handling: none in 0x106C20; any scaling is inside the GUI placement
  call 0x334700 or the framework (not traced).

## State gates

- 0x106C20 runs on every call of the HUD update 0x103200, unconditionally within
  that routine. The container is a child of the HUD root (interface +0x108), so
  it is shown or hidden with the HUD root. Whether the HUD root is hidden while
  the map, menus or death screens are open was not traced in this task.
- No map-open, death or menu test exists inside 0x106C20 itself.

## Order and multiple entries

- Entries appear left to right in player aura-list order (+0x980 list, head
  first), one per aura entry that has an icon, at a 50-unit horizontal pitch.
  Two entries of the same kind would produce two icons (no de-duplication in
  this routine).
- The "x N" form is specific to kinds 0x21 and 0x22; other kinds show a
  duration. Different kinds are not grouped.

## Evidence boundaries

- Proved from code: the drawing routine (0x106C20), its per-update rebuild, the
  aura source list and entry offsets, the kind-to-icon-name table and kind
  numbers, the 0x21/0x22 prefixed-integer rule, the duration branches and
  divisors, the -5 start and +50 pitch, the anchor (0, 0), and which constant
  addresses hold the vertical offset, colour vector, prefix and suffixes.
- Not proved / unread: the actual bytes of the prefix (0x441B8C; "x " is taken
  from the recording), the three duration suffixes, the vertical offset
  (0x42E7C8), the colour vector (0x441C10), the duration thresholds and biases;
  the exact parameter semantics of the placement call 0x334700 (inferred as
  anchorX, anchorY, offsetX, offsetY from its use with the money widget, whose
  values come from the caller's lead, not from this author's reading); the
  effects of 0x335830 and of 0x335780 on the 4-character-named children; the
  icon art and text styling inside the resource layout; GUI-scale handling.
- Not proved: whether the recorded icon is kind 0x21 ("aura:ancientpower") or
  kind 0x22 ("aura:manaabsorption"); what game logic adds these auras to the
  player list, what the float at entry +0x14 means for them (a stack or charge
  count is suggested by the "x" prefix only), and how this ties to the mana-pump
  POI (mission kind 23). Those need a separate trace of aura creation.
- Not proved: HUD-root visibility rules (map open, menus, death).
- The earlier lead that the "manapump" handle is a raw sprite handle is refined
  here: 0x0F9AC0 obtains it by named-child lookup (0x332190) in the interface
  sprite library, i.e. it is a template widget; only 0x2829C0 uses it.
- No Steam runtime behaviour was observed by this author; the only behavioural
  input is the caller's description of the recording.

## Exposure statement

- Before the task, the harness placed in this context the project's AGENTS.md
  text (role and workflow policy for the combined project, no implementation
  details) and a git branch/status snapshot. No replacement-engine source,
  tests, logs, specifications or workspace memory were read.
- Read in full: docs/CLEANROOM_AUTHOR.md (role policy).
- Original Steam evidence only, under "decompiled cube world steam":
  decompiled\assembly.txt (searched and read in ranges), decompiled\strings.csv
  (searched, read first lines), decompiled\functions.csv (searched),
  decompiled\calls.csv (searched); decompiled\functions files read or searched:
  140103200.c, 140101bd0.c, 1400f9ac0.c, 1402829c0.c, 140273240.c,
  140273380.c (searched only), 1401008a0.c, 140106c20.c; pattern searches over
  functions\1400*.c, 1401*.c, 1402*.c and 1402[6-9]*.c, and 14010*.c. The binary
  directory listing was viewed; no binary bytes were read. Two of this author's
  own search outputs were saved by the harness to its tool-results folder and
  re-searched there.
- No web, SDK or reconstruction source was accessed. No executable was run.
