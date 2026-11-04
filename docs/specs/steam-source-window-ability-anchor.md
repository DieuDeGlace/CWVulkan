# Steam eagle-flight anchor used by source and mesh workers

Independent native-only contract. Addresses are Steam executable RVAs.
Evidence is original native code/constants and the linked independent
prose, without engine, tests, exporter, SDK implementation, owner data,
captures or game execution. This is a bounded producer/consumer contract,
not a census of every possible write to a creature.

## 1. Fields, units and active meaning

For a creature P, the relevant fields are:

| Offset | Representation | Reached meaning |
|---|---|---|
|+0x68|byte|Current ability; 0x9c selects eagle travel and 0x9d the separate rising-eagle route|
|+0x6c|float32 milliseconds|Ability clock|
|+0x1b0/+0x1b4|two signed32 integers|While ability is0x9c, destination X/Y in **blocks**|
|+0x1b8/+0x1c0/+0x1c8|three signed64 integers|Stored departure XYZ, in fixed units of1/65536 block|

The destination pair is not a zone index, a16-block cell, a map-screen
coordinate, a world seed or an inventory-slot association. The local
travel consumer at0x2cdfd9 converts each signed32 destination component
to fixed block position before constructing the flight endpoint. Its
native reads/conversions are0x2ce00a–0x2ce043. The pair contains no
destination altitude; the flight consumer derives altitude from terrain.

The meaning is conditional on ability0x9c. Other creatures use these same
fields for other purposes: the resident copy at0x2e17d0 transfers spawn
fields+0x8f8/+0x8fc into them, and the map's NPC/mission readers can treat
such a pair as a zone key. This note does not assign block-destination
semantics to every occurrence of these offsets. See
[map-markers.md](map-markers.md) §2 and
[creature-abilities-ranged.md](creature-abilities-ranged.md) §13.6.

## 2. Ordinary map-click producer

The reached normal producer is the mouse-down handler0x94900, specifically
0x94ba6–0x94daf. After its earlier routing/returns, this branch requires:

- Button argument0, the ordinary left-button down route.
- Game+0x1db0 equal to0.
- Interactive-map flag Game+0x668 nonzero.
- Map renderer Game+0x10's signed32 target X at+0x150 different from
  0x7fffffff. This local guard checks X; it is not a second independent
  validity check of both coordinates.

The target pair at map+0x150/+0x154 is supplied by the marker hover/pick
consumer, not recomputed from the current click by this branch.
[map-markers.md](map-markers.md) §1.4 specifies that producer, including
the associated byte map+0x158. Earlier handler gates remain applicable;
this local branch is not a guarantee that every map click reaches it.

At0x94beb,0x5f310 receives the local creature and the target pair. Native
argument preparation explicitly supplies map+0x150; the abbreviated
decompile's one-argument call is incomplete. The helper checks the
creature's CharacterData+0x28 known-flight-point set. A missing
CharacterData returns false. A present set returns true precisely for
the supplied pair's membership.

If membership is false **and** map+0x158 is0, the click follows the price
path:0x2adce0 supplies the integer price, insufficient gold produces the
failure feedback, and sufficient gold is subtracted before inserting the
pair in the known-point set. That successful purchase branch does not
itself set ability0x9c or write the flight anchor.

Otherwise, the flight-start branch performs these writes in order:

1. Copy map+0x150/+0x154 into local creature+0x1b0/+0x1b4 as signed32
   block components, without cell/zone scaling. The stores are0x94d49
   and0x94d55.
2. Copy the creature's current physical fixed XYZ+0x10/+0x18/+0x20 into
   the departure triple+0x1b8/+0x1c0/+0x1c8 at0x94d6a–0x94d80.
3. Set ability+0x68 to0x9c at0x94d92, then clock+0x6c to float zero
   at0x94da1.
4. Clear Game+0x668 and Game+0x420.

This successful branch replaces any prior destination/departure values.
There is no association with a canonical inventory slot in these writes.
The reached condition is the membership/marker-byte distinction above;
it must not be shortened to “every target click purchases and flies.”

## 3. Flight update, arrival and retained values

The ability branch at0x2cdfd9 first requires P to be exactly World+0x448,
the local creature pointer. Other creatures do not run this branch's
trajectory merely because their replicated ability byte is0x9c.
The complete existing trajectory, duration and fixed-position arithmetic
is in [creature-abilities-ranged.md](creature-abilities-ranged.md) §13.6.

During this reached local travel, the destination and stored departure
fields are inputs; they are not progressively rewritten to the current
physical flight position. The actor's physical/drawn position changes
while the source workers can already use the destination pair.

On arrival,0x2ce196 onward first clears the ability and installs the
destination fixed X/Y in physical position. It then selects the glider
item from inventory tab1 or the documented fallback, sets ability0x78,
and resets the clock to0 at0x2ce209. The destination and departure fields
are **not cleared by this arrival branch**. The worker substitution ends
because the ability ceases to be0x9c, not because the pair becomes empty.
The glider item selection does not turn the destination into an item or
slot reference.

This does not establish every interruption, death, ability replacement or
recovery caller. It establishes the ordinary successful start/travel/
arrival lifetime without inventing a universal coordinate-clear event.

## 4. Other directly reached writes and initial/reset state

### 4.1 Construction, copy and character load

Creature construction0x4ce80 calls main-data constructor0x4d600 with
the address creature+0x10. Stores0x4d706/0x4d710 therefore initialize
creature+0x1b0/+0x1b4 to **0x7fffffff each**. The ability and its clock
are initialized to0. The data constructor does not assign the departure
triple at creature+0x1b8..+0x1c8. It provides no guarantee that fresh
enclosing storage for that triple was initially zero.

Main-data copy0x3b7a0 copies the destination components and all three
departure signed64 components. Its relevant stores are0x3ba03–0x3ba38,
relative to the main-data pointer at creature+0x10. This is a value copy,
not a retained pointer to a map marker.

The ordinary character reader0x147800 has a more specific reset result.
It zeroes its scratch storage covering these fields, constructs default
equipment/data there, calls0x4d600 at0x147978, and copies that result into
the creature with0x3b7a0 at0x147984. Thus this reached load initializes the
destination to **(INT32_MAX,INT32_MAX)**, departure to **(0,0,0)** and
ability/clock to0. The saved character fields do not restore an active
flight destination, departure or ability. The persistent known-point set
is separate and is loaded into CharacterData+0x28. See
[native-saves.md](native-saves.md) §§3.2–3.3.

The older frontend shorthand that flight fields are “cleared” on reload
does not mean the destination pair is(0,0). The sentinel pair and the
zero departure above are the exact reached reader results. Nor does the
reader's prezeroing establish a universal fresh-allocation default.

### 4.2 Title/character-selection rising eagle

In the ordinary frame0xa5a50, while the title or character-selection
layer supplies the established riding condition, ability is set to0x9d.
The frame writes the current physical X/Y divided by65536 toward zero,
narrowed to signed32, into the pair at0xa6561/0xa657b. It copies current
fixed XYZ into the departure triple at0xa6591–0xa65a7. These fields then
follow the title flyover each frame; they are not a selected destination.
When the riding condition ends, the reached0x9d exit clears that ability
without itself clearing the pair/triple. See
[frontend-title.md](frontend-title.md) §§3.1,6.1.

Since the worker substitution checks0x9c exactly, title ability0x9d
does not activate it, despite these writes to the same fields.

### 4.3 Additional global-gated frame producer

A separate reached frame branch begins at0xa9c4f when global byte
RVA0x5514cf is nonzero. Its destination-start subbranch additionally
requires ability not already0x9c and at least one low contact bit at
creature+0x5c masked by3. It sets ability0x9c and clock0, obtains an
integer pair from0x2d1100 at0xa9cd5, and passes it through0x83b80 with
factor16 at0xa9ce9. That helper multiplies each signed32 component with
a32-bit result. The multiplied block pair is stored at0xa9cf0/0xa9cf9;
the current physical fixed XYZ is copied into the departure triple.

This establishes another producer, not a second ordinary marker-click
command. The upstream writer/default/user trigger of global0x5514cf and
the complete random/search behavior of0x2d1100 are outside this bounded
trace. They must not be supplied with an invented default or command.

## 5. Exact worker substitutions and excluded interpretations

| Consumer | Reached substitution |
|---|---|
| Client source worker0x84f70,0x85115–0x85122 | When local ability is0x9c, replace the physical-derived signed32 block X/Y with the destination pair before block-radius bounds and source-zone circle calculation |
| Client mesh coordinator0x842d0,0x8442e–0x84494 | When local ability is0x9c, replace the previously computed physical zone center with mathematical floor of destination block X/Y divided by64 |
| Host source collection0xf2340 | For each collected kind0 creature, ability0x9c selects destination signed32 block XY instead of physical fixed XY truncated/narrowed to blocks, before its±270-block wanted-set bounds |

The destination is already integer blocks: these substitution paths do
not apply a fractional fixed-coordinate correction to it. They preserve
signed negative block coordinates and floor them to64-block zones where
required. Source radius, mesh radius/budget, loaded-neighbor checks and
host wanted-set rules are unchanged by the choice of anchor. See
[steam-world-zone-window-demand.md](steam-world-zone-window-demand.md)
§§2–7 for their separate windows and widths.

The reached substitution checks have no additional minimum flight age,
far-flight-distance threshold or sentinel-pair rejection. In particular,
the scene's separate ability/time-dependent range behavior is not a gate
on these worker substitutions. A malformed record with ability0x9c and
sentinel coordinates is not automatically repaired into a physical anchor
by these branches.

Client eviction0x2d2410 still derives its center from the physical local
position and has no matching destination substitution. The ordinary
renderer's adaptive camera window likewise uses its installed fixed
final-eye tuple, not a direct read of the destination pair. These centers
must remain distinct from the chosen source/mesh anchor.

## 6. Network transfer and limits

The ordinary creature delta contains ability as bit9, clock as bit10,
the raw8-byte destination pair as bit39, and the raw24-byte departure
triple as bit41. Delta reader0x138e00 reads the pair into main-data+0x1a0
and the departure into main-data+0x1a8 when those respective bits are set.
It does not convert either coordinate space. Main-data starts at
creature+0x10. See [netcode-packets.md](netcode-packets.md) §3.

The client packet3 receiver0x3e450 ignores a received record with the
local player's own ID; other records are updated through this reader.
The host receiver0xf4490 applies a connected guest's correctly identified
creature delta, including these fields. This is a proven route by which
a host kind0 record can carry the client's flight ability/destination;
it does not require the host to invent a destination or execute every
creature's local-only flight trajectory. These are raw field transfers,
not independent proof of destination validation or an atomic cross-thread
snapshot of all flight fields.

This note does not exhaust all aliased/bulk copies, every ability
cancellation, NPC mission-pair producer, global-gated random search or
exceptional startup/reset path. It establishes the ordinary actor state,
known start/arrival/load lifetime and directly reached worker/network
binding. Untraced paths do not justify a replacement sentinel, implicit
coordinate clear or new generation fallback.
