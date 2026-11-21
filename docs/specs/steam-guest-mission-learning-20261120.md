# Steam guest mission learning, Talk and completion (behavioural contract)

Author role: isolated source-side specification author (docs/CLEANROOM_AUTHOR.md).
Date: 2026-11-20. Status: FINAL / STABLE / HELD.

## Supported image

- Analysed image: `cubeworld.original.exe.unpacked.exe`, SHA-256
  `7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`,
  image base 0x140000000. All RVAs below are relative to that base.
- Packed shipping file `cubeworld.original.exe`, SHA-256
  `a1efbbd41eeb8df08395c4d6511194d416cb70a14054b2a2000b9f5cdbcfa65b`.
- Evidence: static analysis listings of the unpacked image only. Nothing here was
  observed at run time. Infinite-Cubes (release) was used only to find routines;
  every statement below marked "confirmed" was checked against the original
  listings. Statements marked "LEAD" were not confirmed.

## Terms and layout used

Offsets are from the start of a creature object unless stated otherwise. The
replicated entity block starts at creature +0x10.

| Item | Offset | Width | Notes |
|---|---|---|---|
| creature id | +0x08 | 8 | signed 64-bit |
| hostility kind | +0x60 | 1 | 0 player, 1 enemy, 3 friendly NPC, 4 neutral, 7 attackable object, 8 interactive object |
| current ability | +0x68 | 1 | 156 and 157 are flight and menu-view states |
| appearance block | +0x78 | 176 | appearance flags are the 32-bit value at +0x80 |
| appearance flag "captive" | +0x80 | bit 0x1000 | |
| appearance flag "mission resident" | +0x80 | bit 0x2000 | |
| appearance flag "usable" | +0x80 | bit 0x200000 | |
| appearance flag "examine/petrified" | +0x80 | bit 0x400000 | |
| binary toggles | +0x128 | 2 | bit 0x100 = "mission complete" toggle |
| class | +0x14C | 1 | 157 = shop supplier; 128..130 and 156 = vendors; 131 = analyser; 154 = flight master; 161 = arena champion; 132 = innkeeper |
| interaction state | +0x19C | 1 | 255 = done |
| mission zone key | +0x1B0 (x), +0x1B4 (y) | 4 + 4 | signed 32-bit each; "none" = 0x7FFFFFFF in both |
| mission origin | +0x1B8 | 24 | three 64-bit fixed-point coordinates (1/65536 block) |
| interaction queue | +0xAB8 (list head), +0xAC0 (count) | | list of interaction records |
| player-data pointer | +0x1528 | 8 | null for every creature except a character loaded as a local player |

Character (player-data) sets, offsets from the player-data object:

| Set | Offset | Key |
|---|---|---|
| discovered places | +0x38 | pair of 32-bit place coordinates |
| discovered markers (lore cells) | +0xA8 | pair of 32-bit cell coordinates |
| marked (known) missions | +0xB8 | pair of 32-bit place coordinates |
| solved missions | +0xC8 | pair of 32-bit place coordinates |
| finished missions | +0xD8 | pair of 32-bit place coordinates |
| new-mission note timers | +0xE8 | map from place pair to a 32-bit float (value at map node +0x24) |
| lore by realm | +0x128 | map from realm key to a set of cells (set size read at node +0x30) |

World object: creature list +0x40; talking-creature id list +0x170; guest flag byte
+0x358 (1 = this world is a guest mirror); local player creature pointer +0x448.
Game object: crosshair (aimed) creature id +0x1E58.

"Knows mission" test (RVA 0x5F4C0): given a creature and a place pair, returns
false if the creature has no player data; otherwise true when the pair is in the
marked set (+0xB8) or in the solved set (+0xC8). Confirmed.

## 1. Writers of the marked-mission set (+0xB8)

A search of the listings for that set offset finds five routines that write +0xB8:
three gameplay writers and two non-gameplay ones. Routines that copy a whole
player-data object may also copy the set; they were not enumerated.

### 1a. Aiming at a creature (crosshair) - RVA 0x8AE50

- Trigger: every game frame in which the per-frame player-control step runs
  (called from the game update, RVA 0xA5960 area, call site 0x1400A9B2C). The
  step runs only while a world is loaded, the local creature's current ability is
  neither 156 nor 157, and the start menu, character-selection and
  character-creation screens are hidden. There is no host/guest test on this
  path.
- Target choice (same routine): the nearest creature under the crosshair becomes
  the aimed creature only if its hostility is neither 1 (enemy) nor 7
  (attackable object).
- Write gate, all required, evaluated on the aimed creature:
  1. the creature still exists in the local world;
  2. hostility is not 0 (players are never sources);
  3. hostility is not 3, or class is 157 (shop supplier), or the captive
     appearance bit 0x1000 is set;
  4. mission zone key x (+0x1B0) is not 0x7FFFFFFF;
  5. "knows mission" for (key x, key y) on the local player is false.
- Effects, in this order: insert (x, y) into marked missions (+0xB8); insert
  (x, y) into discovered places (+0x38); set the note timer for (x, y) (+0xE8)
  to 180000.0 (milliseconds). No sound is played by this writer.
- The key is used exactly as stored on the creature; no place normalisation is
  applied here.

### 1b. Dialog pages that carry marked locations - RVA 0xB40D0

- This routine advances speech once per frame (called from the game update at
  call site 0x1400AA1BC). It has two consumers: speech bubbles (one per speaking
  creature) and the main speech panel. Each dialog page record is 0x1B0 bytes;
  the page's marked-location list is at page +0x1A0, its lore-cell list at page
  +0x190.
- When a page becomes current (its read time is still zero), for each marked
  location (x, y) on the page for which "knows mission" is false on the local
  player: insert into +0xB8, insert into +0x38, set timer +0xE8 to 180000.0, and
  play sound 0x20 (quest progress) at the camera position. This happens in both
  the bubble consumer and the panel consumer.
- Lore cells on the same page go to discovered markers (+0xA8) and, where the
  cell holds lore, to lore-by-realm with the chat line " lore increased to N%"
  (N = 10 x set size). That path does not touch +0xB8.
- There is no host/guest test on this path. Pages come from dialogs generated
  locally (see section 3).

### 1c. Map overlay lore completion - RVA 0x2829C0

- While drawing mission markers on the map, for a mission entry of kind 14
  (artifact reward) whose place is not yet known: if the lore-by-realm set keyed
  by that mission's realm key holds at least 10 entries, insert the place into
  +0xB8 and set its timer (+0xE8) to 18000.0. It does not insert into discovered
  places and plays no sound. Large places are normalised to even coordinates
  before the test. No host/guest test.

### 1d. Non-gameplay writers

- Character load (RVA 0x147800) fills +0xB8 (and +0xC8, +0xD8) from the saved
  character. RVA 0x149760 only reads these sets (LEAD: it is the save path).
- Player-data teardown (RVA 0x4DB70) empties the set.
- No network message handler among the found writers writes +0xB8. Every gameplay
  writer targets only the local player (world +0x448).

## 2. Does 1a run for a guest on host-owned (mirrored) creatures?

- Yes. Writer 1a has no role gate (the guest flag at world +0x358 is not read on
  that path) and it operates on any creature present in the guest's world list,
  which includes every mirrored host creature.
- Every field the gate reads is replicated to the guest in creature updates
  (message kind 3). Decoder RVA 0x138E00; host encoder RVA 0x139620. The update
  is a 64-bit change mask followed by the changed fields in bit order; fields
  are copied straight into the mirror's entity block (creature +0x10 onward):

| Gate field | Mask bit | Bytes | Destination |
|---|---|---|---|
| hostility | 7 | 1 | +0x60 |
| appearance block (contains flags at +0x80) | 13 | 176 | +0x78 |
| binary toggles | 14 | 2 | +0x128 |
| class | 23 | 1 | +0x14C |
| interaction state | 35 | 1 | +0x19C |
| mission zone key (x, y) | 39 | 8 | +0x1B0 |
| mission origin | 41 | 24 | +0x1B8 |

- The host builds one kind-3 message per creature per connection: message kind,
  payload length, then a compressed payload holding the creature id followed by
  the mask and the fields. A field is included when it differs from what that
  connection last received, or always (full update) when the creature has not yet
  been sent to that connection (host routine RVA 0xF5520). The guest skips
  updates for its own creature id, creates a mirror for an unknown id
  (constructor RVA 0x4CE80) and then applies the fields (guest handler RVA
  0x3E450).
- So the guest's mirror holds the host's mission zone key. Before any update
  carrying bit 39 arrives, a newly constructed mirror holds (0x7FFFFFFF,
  0x7FFFFFFF), i.e. "none", which makes writer 1a do nothing. With a full first
  update that window does not persist once the creature has been sent.
- Mirrors never have player data (+0x1528 is null after construction), so no
  writer marks anything inside a mirror. Each client writes only its own local
  character.

## 3. Guest uses Talk on a host-owned quest giver

Interaction prompt and use: RVA 0x98180.

- Prompt: offered when the aimed creature has hostility 3 and lies within its
  interaction range. If appearance bit 0x400000 is set, the prompt is "Examine"
  instead and only shows a local text. Otherwise the prompt is "Talk".
- On use, by class: vendors (class 128..130 and 156) open the local shop panel;
  class 131 opens the local enchanting panel; class 154 opens the local
  flight-master map. These three return without queuing anything.
- Other classes: if a speech bubble for that speaker already exists, the use only
  speeds it along (locally). Otherwise:
  1. The guest builds the creature's talk script from its own mirrored copy and
     local world (talk-script builder RVA 0x2D9480), generates dialog pages with
     seed = creature id modulo 100000, and shows them as speech from that creature
     to the local player (RVA 0xA3170). Pages with marked locations then mark
     missions in the guest's own character via writer 1b.
  2. In all cases it queues an interaction (RVA 0x98040) on the guest's own
     creature: kind 3 ("activate"), target creature id, position (-1, -1, -1),
     empty item.
- Interaction record layout (0xC8 bytes): item at +0x00 (160 bytes), position as
  three 32-bit values at +0xA0, target creature id at +0xB0 (8), an 8-byte zero
  at +0xB8, a 32-bit zero at +0xC0, kind byte at +0xC4, a 16-bit value at +0xC6
  (1 by default).
- Sending (guest client tick, RVA 0x3F4F0): about every 33 ms the guest sends its
  own creature update (kind 3), then removes the oldest queued interaction (only
  one per tick) and sends it as message kind 9 carrying that record.
- Host receipt (RVA 0xF4490, kind 9): the record is appended to the interaction
  queue of the creature bound to that connection (the guest's creature in the
  host world).
- Host processing (RVA 0x2C3710, called per live creature from the world step RVA
  0x2DA6C0). It returns at once when the world is a guest (world +0x358 set), so
  only the host processes queues. For kind 3, by target:
  - race 245, or usable bit 0x200000: interaction state := 1 (race 302 also gets
    ability 153);
  - class 161 (arena champion) with state below 255: if the state is even,
    increment it and print "Arena Master: N" to the console;
  - otherwise, if mission-complete toggle 0x100 is set and the state is 0:
    state := 255, clear the toggle, and if the target id is not already in the
    rewarded set, record it and drop rewards (count min(level + 1, 4), at a
    randomly offset position near the creature) plus the creature's inventory
    as world items.
  - For a plain quest giver with none of these conditions, nothing changes.
- The host does not build a talk script, start dialog, or mark any mission in
  response to a guest's Talk, in the guest's character or in its own. The host has
  no player data for the guest's creature.
- Sent back: nothing dedicated. Changes to interaction state and toggles reach the
  guest only through ordinary creature updates (bits 35 and 14). Dropped rewards
  reach it as world-item state. No dialog, mission-given or notice message exists
  for this exchange.
- Spontaneous talk, unrelated to a Talk press: for every id in the world's
  talking-creature list (world +0x170), the game update (RVA 0xA5960 area, call
  sites 0x1400A97B9 / 0x1400A985D / 0x1400A9875) builds the talk script, shows it
  as speech with no listener (seed = id modulo 100000), and queues a kind-3
  interaction with that creature. On a guest this queued interaction is then sent
  to the host as above. Confirmed: the server-update bundle the guest receives
  (message kind 7, guest handler RVA 0x3E450) carries about fifteen lists into the
  game object (+0x1970 to +0x1A50). LEAD: one of these is the host's
  talking-creature list, merged into world +0x170; the host adds a mission-resident
  friendly NPC when a player first comes into its sight, with a cooldown. The exact
  list mapping and the host trigger were not confirmed.

## 4. Completing or solving a mission as a guest

- Writer: the mission task check (RVA 0x131620), run unconditionally at the end of
  every world step (wrapper RVA 0x2E1750, called from RVA 0x2DA6C0) whenever a local
  creature exists. It is skipped while the local creature's ability is 156. It writes
  only the local player's finished (+0xD8) and solved (+0xC8) sets. This applies on
  host and guest alike, so each side records missions for its own character only.
- First rule, confirmed: for each creature in the local world (mirrors included)
  with the mission-resident bit 0x2000, the mission-complete toggle 0x100 set and a
  non-zero interaction state, within range of the local player: take the place
  pair from the creature's mission origin (+0x1B8, converted from fixed point to
  place coordinates, then normalised to even coordinates for large places) and
  look up the mission kind there. Kind 15 adds the pair to finished only; kind 1
  adds it to finished and solved. Other kinds do nothing here.
- Further rules in the same routine (also writing only the local character's
  +0xD8 and +0xC8) cover chests and braziers near the player, cleared mission
  creatures in zones near the player, and held items or special items. Confirmed
  that they target only the local character; the per-rule details are LEAD
  (reconstruction) and were not individually re-derived.
- Inputs for a guest come from replicated mirror state (toggle bit 14, interaction
  state bit 35, origin bit 41), local world/zone state and the guest's own
  inventory. No packet carries the solved or finished sets, and no message tells
  the guest that a mission is solved.
- Day rollover: when the guest receives message kind 8 (day and time) with a day
  number different from its own, it runs the new-day routine (RVA 0x2C34C0). That
  routine clears the local finished set (+0xD8) and other daily state. On the host
  the same routine runs from the world step when the day advances.

## 5. Explicit limits

- Static evidence only; none of the paths was exercised at run time, and nothing
  was checked against owner captures.
- The content of the talk script (RVA 0x2D9480) was not audited. It is not proved
  whether it depends on host-only state or on the mirror's zone key, or which
  dialog commands fill a page's marked-location list. Whether a guest's locally
  generated quest-giver dialog matches the host's is therefore unproved.
- The crosshair candidate scan before the gate (distance and visibility rules) was
  not audited beyond the hostility exclusions stated above.
- The new-mission note timer is set to 180000.0 or 18000.0. Its countdown and
  removal (LEAD: reduced each world step and removed when it runs out; the map shows
  "NEW" while it exists and the mission is not completed) were not confirmed.
- The guest world flag (+0x358) is confirmed to be set when joining a host (RVA
  0x3D820) and cleared by the local start/disconnect path (RVA 0x911E0). Its meaning
  as "guest" rests on those writers and on its use as an early exit in host-only
  processing.
- The world-loaded condition for the control step is read as "world name length
  non-zero" (world +0x210). That reading is LEAD.
- In the first completion rule, the gate (toggle set and state non-zero) and the
  host activate rule (toggle set and state zero, then the toggle is cleared) are
  different conditions. Which runtime sequence satisfies the completion rule on a
  guest was not established.
- The kind-7 list-to-field mapping, and how dropped reward items replicate, were
  not traced.
- Host creature-selection rules per connection (which creatures are sent) were not
  audited; only the delta/full rule is stated.
