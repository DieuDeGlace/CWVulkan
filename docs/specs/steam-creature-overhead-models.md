# Steam generic creature overhead models

Independent native specification from0xd3170, specifically0xd8278–0xd86de,
and0xe0630. No engine, test, exporter, build or owner-capture inspection;
no original-game execution. This completes the generic selection adjacent
to [active emotes](steam-npc-speech-rendering.md) §§5–7.

## 1. Admission and selection order

This is inside the living-actor render path, after body admission. Use the
existing HP>0, scene-flag/local-player, actor-sphere and renderer+0x4a0 range
contract in steam-npc-speech-rendering §7. There is no separate quest-icon
distance fade or billboard overlay admission here. Begin with **no selected
model, white material RGBA, scale multiplier1, extra height0 and local-Y
angle0**. Then perform these decisions in order:

| Decision | Effect |
|---|---|
| Creature unsigned class byte+0x14c is128,129 or130 | Select owner catalog0xc3d |
| Class155 | Select0xdaf |
| Class131 | Select0xc3c |
| Class139 | Select0xdaf |
| Actor ID belongs to WorldMap collection+0x138, tested by0xe0630 | Replace model by0xdc2; material is(1,0.25,0,1) if actor ID equals WorldMap+0x148, otherwise white |
| Active emote pointer creature+0xc50 nonnull | Replace model by that pointer and explicitly replace material by white; apply the already specified emote animation |
| Otherwise, buff lookup0x5e3a0(creature,7) nonnull | Replace model by owner heart0xdf0; multiplier is float **0.5 plus the buff's strength float at+0x4**; retain the material selected above |

The membership object is **WorldMap**, reached at renderer+0x10, also
Game+0x10; it is not World+0x138. Its ordered map has signed64 creature
ID keys and saved creature pointers.0xe0630 tests membership by the actor's
ID+8; it does not test whether the saved pointer is currently visible,
whether this creature is a quest giver, or whether a quest is complete.
WorldMap selected ID+0x148 is signed64. Selection color's green0.25 is
verified at0xd8185/0xd8270 and0xd8366, not an inferred rarity color.

The collection has concrete existing producers: completed player ability
0x96 in0x98180 copies its target through0x2f4290 and selects its ID;
completed0x97 removes through0x2f7ef0 and selects the existing list cursor's
remaining entry or0. See use-prompt §3.3. These facts identify the data
source without inventing a quest state or reinterpreting the catalog's
appearance.0x2f4290 preserves a creature copy and relevant supplemental
fields, rather than storing a new mission-marker record.

Buff7 lookup returns the first corresponding current buff entry through
the existing buff-list helper. There is no strength clamp in this draw
branch. A successful buff lookup replaces even an already selected class
or collection marker. A nonnull active emote prevents entering that buff
fallback. A missing selected catalog model means no draw; the routine does
not synthesize a heart or retain an earlier selected class icon after a
failed replacement lookup. Generic markers do not create an emote timer
or make the speech path's+0xc50 suppression test true.

## 2. Shared placement, dimensions and material controls

The transform after final selection is **the exact transform already in
steam-npc-speech-rendering §6**, not a second screen-space marker routine:

- Physical signed64 actor XYZ+0x10/+0x18/+0x20, renderer origin+0x428/+0x430.
- Height in float32, in order: **body Z+0x8c times0.5, plus1, minus
  actor+0x990, plus extra height**. Multiply by65536, truncate signed64,
  add to physical Z, then convert completed fixed sums to float block units.
- Actor yaw around world Z, then the extra local-Y angle. Generic class,
  collection and buff-heart branches have extra angle0; no camera facing.
- Uniform scale **float32(0.8 /float(model header X+0x88)) times multiplier**,
  then translate by negative half of each complete signed32 header dimension.
  Full CUB dimensions, not occupied bounds or actor width, determine shape.

For a plain generic marker, multiplier1 gives nominal width0.8 blocks.
For buff7 heart it is **0.8 times(0.5+strength)** blocks, preserving model
aspect. The active-emote bounce/scale is separate, as already specified.
Extra height remains0 for a plain class/collection/heart marker.

All these selections submit through0xee2f0 at0xd86de to ordinary scene
model queue renderer+0x138 with **pixel Opacity1, Glow1, Shininess0,
White0, DepthOffset0** and the material above. Glow1 is the already
specified WORLD vertex-light bypass, followed by the ordinary pixel/fog/
grade stages; it does not change the model's depth state to always visible.
Opaque depth-tested/depth-writing state0 is used before water. A buff heart
on the selected collection member therefore retains orange material; an
active emote on the same actor explicitly returns to white.

## 3. Resource boundary

Catalog IDs0xc3c/0xc3d/0xdaf/0xdc2/0xdf0 are original owner voxel sprites,
selected through0x17190.0xdf0 is independently registered as **heart.cub**
in0x177f50, as in the emote resource table. This investigation has not
independently named the other four assets; their IDs and selection rules
are verified, and no replacement icon, glyph or geometry is authorized by
an assumed English role name. Use the owner's existing sprite catalog.
