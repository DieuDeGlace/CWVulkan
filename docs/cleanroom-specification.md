# CubeVulkan clean-room specification

CubeVulkan is a replacement engine for Cube World that aims for 1:1 behaviour with the supported
Steam release. This document is organised in four parts:

- **Part A** lists the interoperability facts any implementation may reuse.
- **Part B** is the functional specification the engine must meet.
- **Part C** lists the material those facts come from.
- **Part D** describes the tests that confirm the behaviour matches the original.

An appendix summarises the process. The authoritative process texts are [CLEANROOM.md](CLEANROOM.md)
and [CLEANROOM_AUTHOR.md](CLEANROOM_AUTHOR.md); where this document and those files differ, the
files win. These rules preserve source separation; they are not a statement of legal clearance.

For both playable Frogman sexes, customization retains the selected primary RGB.
The normal haircut draw multiplies its authored colours by white. Face and
haircut choices select independent models. Exact red, green and blue attachment
markers on the numbered haircuts are hidden. Near-primary paint remains visible
where the model is exposed. The two-tone option is an engine extension.
On the world map, the face and haircut retain their catalog geometry for the
outline and central draw. Hidden haircut cells still occlude adjacent paint.
[Steam Frogman appearance](specs/steam-frogman-primary-appearance-20261121.md). [Map portrait geometry](specs/steam-map-creature-head-markers-20261121.md).

Bounced sunlight is an optional engine enhancement. It samples a 512 by 512
sun-facing terrain map with twelve taps. The camera fade ends at 50 blocks.
The stock cloud grid stays anchored as the day advances. Camera submersion
caps its fog range at 150 blocks, and the pass has its own draw gate.
[Steam cloud motion and fog](specs/steam-cloud-movement-20261121.md).

With the optional moving layer, cell lookup subtracts displacement from
camera X. Each staggered column determines its own row using floor division.
The cache refreshes after more than two seconds or a changed seed. Opacity,
culling and rendering use the displaced anchor. Each cell samples climate
and height at its original coordinates.
[Add-on cloud behavior](specs/owned-addon-cloud-movement-20261121.md).

The optional Revised water look uses the frame's local position together
with two signed, wrapped phases computed from the world origin and elapsed
seconds in double precision. This keeps wave travel continuous when the
view rebases. Its five slopes, depth attenuation, sun glint and 48-block
submerged palette belong to the add-on. A shorter configured underwater
range still applies.
[Surface appearance](specs/owned-addon-water-later-look-20261121.md).
[Wave offsets](specs/owned-addon-water-wave-offsets-20261121.md).

The optional paired character creator fits two measured style panels
between the left margin and the character preview. Their hit tests
invert the same transform used by their artwork. A valid tips swatch
enables and commits its colour. Hover previews apply while dual tone
is enabled. At the outer edge of the palette, right and bottom are
excluded.
[Panel layout](specs/owned-addon-dual-hair-layout-20261121.md).
[Tips selection](specs/owned-addon-hair-mode-20261121.md).

## Part A. Interoperability facts

These are the facts the engine needs to read the player's files, talk to other copies of the game
and behave the same. They are functional facts rather than creative expression, so the
specifications state them as plain values, each cited by RVA in its governing contract.

### A1. What may be copied

| Stated directly in specs and code | Never copied |
|---|---|
| File-format layouts: chunk tags, field order, widths, encodings | Code, pseudocode or decompiler output, or its structure and expression |
| Key-derivation formulas and the keys needed to open the player's own files | Names a decompiler or a third-party reconstruction invented |
| Network packet ids, byte layouts, versions and channel rules | Game data tables (items, sounds, models, art, music, text): loaded or extracted from the player's installation at run time |
| Constants, thresholds, timings, divisors and magic numbers | SDK headers or implementation |
| Record offsets, enumerations and identifiers that saves or packets carry | |
| The order of decisions and random draws, where the result must match | |
| Class and resource names carried by the game's own files or RTTI | |

A value that a save file, a packet or a matching result depends on is a fact and may be written
down. Anything that is the game's content, or the way its programmers chose to write it, stays
out. Where a table is game content (for example the 125-entry sound list), the specification
describes how to find it, and the extractor reads it from the owner's installation.

### A2. File formats

| Format | Key facts | Spec |
|---|---|---|
| Plasma `.plx` (gui, icons, cursor, quest-tag) | One flat stream of tagged chunks (i32 tag, i32 size, payload). Tag names are defined inline the first time they are used, through tag id 0, and the name table is global to the file. A `PlasmaGraphics` header chunk holds version 1, followed by a `Seal` chunk. Strings are i32-length-prefixed; wide strings are UTF-16LE. Matrices are 16 floats, column-major. The objects (Transformation, Display, shape kinds, widgets) are linked into a node tree with per-attribute animation sequences. | [plasma-format-and-scene](specs/plasma-format-and-scene.md), [plasma-shapes-and-text](specs/plasma-shapes-and-text.md) |
| Plasma seal | Only tag names are obfuscated. The key is a 64-bit hash: start at `0x0003FFFFFFFFFFE5`, then `h = h×31 + byte` over the phrase. The game opens its files with the `PlasmaXGraphics` key. Decryption rotates by `low32(K) mod n` and subtracts the key bytes. | plasma-format-and-scene §1.4 |
| Saves (`Save/characters.db`, `Save/world.db`) | SQLite 3 with one table, `blobs(key TEXT PRIMARY KEY, value BLOB)`, and no explicit transactions. characters.db holds the keys `num`, `slot` and `0…num−1`; each character entry is a zlib-compressed blob (character version 11, world-state version 44). Items are 0xA0 bytes each. world.db holds zone state records. | [native-saves](specs/native-saves.md), [frontend-characters](specs/frontend-characters.md) |
| Sounds | Loose RIFF files in `sounds\`. The loader requires form type `WAVE` and takes the `fmt ` and `data` chunks. 125 numbered sounds are preloaded, so each handle is its sound number plus 1. | [sound](specs/sound.md) |
| Models and data | Voxel `.cub` models and the game's data databases are read from the owner's installation. Their formats are documented for reading only; nothing is redistributed. | [extraction](extraction.md), [item-models](specs/item-models.md) |

### A3. Network protocol

The game uses Steam peer-to-peer messages, all sent reliably (send type 2). The host sends to
clients on channel 0, and clients send to the host on channel 1. Each packet starts with a u32 id
and has no length, checksum or sequence number. Values are little-endian. A short packet never
fails: any missing fields keep their defaults.

| Id | Packet | Direction | Body |
|---|---|---|---|
| 0 | Invitation | host → client | u32 100001 (protocol version) |
| 1 · 2 | Join · Leave | client → host · either | empty |
| 3 | Creature update | either | i32 L, then a zlib stream of the record delta |
| 4 · 6 | Your id · End of updates | host → client | i64 creature id · empty |
| 7 | World update | host → client | i32 L, then zlib of 15 lists |
| 8 | Time | host → client | i32 day, i32 time of day in ms |
| 9–13 | Interaction, damage, sound, buff change, effect | client → host | fixed records of 200, 72, 48, 48 and 128 bytes |
| 14 | Chat | both | i32 n plus UTF-16 units; from the host, prefixed by the speaker's i64 id |
| 15 · 24 | Zone request · Version mismatch | client → host | i64 zone key · u32 100001 |

Compression is zlib's one-call compress at the default level. Any valid zlib stream decodes, so a
sender need not reproduce the game's exact bytes. The full layouts, roles and timings are in
[netcode-packets](specs/netcode-packets.md), [netcode-transport](specs/netcode-transport.md),
[netcode-roles](specs/netcode-roles.md) and [netcode-compatibility](specs/netcode-compatibility.md).

### A4. Constants and magic numbers (sample)

| Value | Meaning | Spec |
|---|---|---|
| `0xFB3A260B5EC7CD5C` | Plasma document key for `PlasmaXGraphics` | plasma-format-and-scene |
| `100001` | Network protocol version | netcode-packets |
| `11` · `44` | Character blob version · world-state version | native-saves |
| `0x1F400` | Save inflate chunk size, in bytes | native-saves |
| `300000` ms · `400` | Death-marker lifetime · clear distance squared (20 blocks) | steam-local-death-tombstone-and-marker-review-20261114 |
| `600` / `1200` ms | Map "NEW" note blink: on for 600 ms of every 1200 ms | map-markers |
| `4` · `8` | World-map drawn radius: corner map · full map | steam-corner-map-screen-size-20261114 |
| (85, 60) · (255, 60) | HUD money counter · context-item anchor offsets | steam-hud-area-objectives-20261115 |
| 45 + 50·i, 120, ×3 | HUD aura strip: x, y and template scale | steam-hud-aura-strip-constants-20261115 |
| `2.0` | Maximum sound frequency ratio | sound |

## Part B. Functional specification

### B1. Subsystem requirements

The contracts listed in the third column are the authority for each requirement.

| Area | The engine must | Governing specs | Status |
|---|---|---|---|
| World generation | Produce the same terrain, structures, residents and creatures as Steam from the same seed and zone, in the same random-draw order | worldgen-terrain\*, worldgen-structures\*, worldgen-creatures\* | Gaps listed in worldgen-parity-gaps |
| Creatures and combat | Run AI, abilities, hits, damage, buffs, upkeep and poses with Steam's rules and timings | creature-\*, stat-rules, [host random streams](#b3-host-random-streams) | Authority resident and pet handoffs checked in the engine. The full Steam host sequence remains open. |
| Player control | Map mouse, keyboard and controller input to movement, camera, skills and use actions | player-control, use-prompt, skills-and-interactions, [Ring stick routing](specs/steam-ring-stick-routing-20261117.md) | Controlled Ring cases pass in the licensed engine. Original-client and physical controller checks remain open. |
| Items and loot | Generate, name, drop, pick up, craft, buy and sell items | drops-and-items, loot-\*, lore-and-artifacts, item-models | Ported |
| Interface | Draw the title, character, options, HUD, inventory, map and dialogue screens from the player's Plasma files | plasma-\*, frontend-\*, [Stock graphical entry](specs/steam-stock-graphical-entry-routes-20261117.md), hud-money, conversation-script, steam-hud-\* | Ported |
| World map | Build the map model and draw the corner and full maps with markers, labels, missions, history and the player | worldmap-model, worldmap-rendering, map-markers | Recorded match |
| Rendering | Reproduce the sky, clouds, lighting, shadows, water, vegetation wind and particles | sky-and-clouds, shadows, water-rendering, vegetation-wind | Waterfalls and rain not yet recorded |
| Sound and music | Play the same sounds, ambient emitters and music for the same events and places | sound, steam-\*-music-\* | Ported |
| Saves | Read and write characters.db and world.db so Steam and CubeVulkan open each other's saves | native-saves | Ported |
| Multiplayer | Host and join sessions with the Steam protocol and roles | netcode-\* | Two-account test pending |

Open questions are tracked in [remaining-runtime-gaps](specs/remaining-runtime-gaps.md) and
[worldgen-parity-gaps](specs/worldgen-parity-gaps.md). An unproved point stays open; it is never
filled in by guessing.

For ordinary ground pickups in multiplayer, the host validates the slot and
broadcasts the picker's ID and item. The receiving player's client applies the
award. A shared drop's duplicate check belongs to the recipient. A licensed
UDP gold pickup survived character reload. Original Steam save timing and
two-account play remain open. [Guest pickups](specs/steam-remote-guest-item-pickup-20261118.md)

A personal ground pickup records the signed X, Y and Z of its containing
block. The character save stores the full triples in signed lexicographic
order, including distinct heights in one column. A separate horizontal
16-block cell controls visibility of collected treasure. The adjacent saved
XYZ set has independent membership. The local receiver applies ordinary bag
items, regional platinum and Mana Cubes. A newly pressed Use action prepends one manual request. Walk-over pickup
checks the pending queue before appending. The manual capacity check needs
an existing tab and can credit one gold or one auxiliary-counter unit for
specific item categories during prompt and press evaluation. Regional
special-item ownership uses the player's region and ordinary bag tabs.
[Steam ground pickup input](specs/steam-ground-manual-capacity-20261118.md). [Capacity, regional ownership and sound](specs/steam-ground-manual-details-20261118.md). [Steam guest inventory settlement](specs/steam-guest-nongold-settlement-20261118.md). [Steam personal pickup history](specs/steam-personal-pickup-position-history-20261118.md).

### B2. Extensions and preserved looks

Features the user asked for that Steam does not have are labelled as project choices, never as
parity claims:

| Extension | Provenance |
|---|---|
| Alpha terrain and lighting profile | Reuses the sister AlphaVulkan project's own clean-room GPL implementation under its independent contracts |
| Directional shadow maps and TAA/SMAA | CubeVulkan's optional lighting and image passes. The traced Steam path presents current-frame MSAA. [Rendering settings](specs/shadows.md) |
| Dual-tone hair controls | Built on CubeVulkan's own tinting and profile APIs |
| World list pages | An optional extension beside the native world selection |

Every checkpoint preserves these looks: Alpha relief and lighting, the dry purple Undead Lands,
distinct biome palettes, and the gentler Savannah and Omega water.

Recent review checked two landmark cores, their map approaches, open-land
relief and representative Undead Lands and Savannah columns. Its original
Steam evidence covers the scoped location, channel, palette and frame paths.
[Terrain measurements](alpha-generation.md), [held Steam trace](specs/steam-terrain-render-review-20261117.md).

The archived wooded mound is a type-60 cell structure at (9992, -10760).
Its stored crown and the archived volcano match across the native and
Alpha providers on seeds 0 and 123. The same informal description could
refer to another location in a player's world.
[Steam place naming](specs/steam-forest-mound-identity-20261121.md).

### B3. Host random streams

Each Steam execution thread owns its CRT stream. Host simulation, host services
and zone generation, and AI run on distinct workers. The public world seed and
the derived generation seed govern content preparation. The empty first
authority tick has zero resolved game-side draws or seeds under the documented
empty-queue, actor, zone and projectile conditions. The incoming numeric state
and which operation runs first in a populated tick depend on the runtime and
admitted work. [Host stream ownership](specs/steam-host-crt-stream.md),
[bounded startup trace](specs/steam-host-startup-crt-route-20261117.md).

Steam can also enter the shared world step from the frame thread on its own
World instance. The mode byte governs access to resident refresh. The live
comparison identifies the simulation worker by the outer return chain.
[Held CRT return sites](specs/steam-host-live-return-sites-20261117.md).

Two licensed original runs recorded 279 daily reseeds and 2,206 subsequent
draws on the identified simulation workers. The game imports resolved to
`ucrtbase.dll`. Area generation appeared on separate workers in these licensed sessions. The built
CubeVulkan CRT object replayed every recorded authority draw exactly.
Each capture covers the observed window following its first recorded seed.

In the ordinary authority resident phase, selected loaded zones are visited in
Zone-address order. Each zone's live refresh stamp is checked before its
residents. A changed day reseeds the simulation stream. Any refresh draws and
an admitted mode-0x8A resident's altitude draw advance that same stream.
When the stamps match, the refresh leaves the incoming stream unchanged.
A zone loaded for the first time starts with a stamp of minus one. A
replacement instance starts with its own stamp. [Authority resident and daily
refresh contract](specs/steam-host-resident-refresh-stream-20261117.md).

CubeVulkan tracks that stamp on the live source. The first authority visit
restores the base static objects and adopts the refresh's final random state.
A changed day repeats both steps. Source preparation can supply a prepared
catalog for the sampled day, and same-day visits keep live object changes and
the current stream. Twelve diagnostic refreshes at day 137 in the supported
Steam game produced 242 recorded draws across four
nonempty traces and eight empty traces. The engine matched the complete draw
lists. A separate engine fixture checks the handoff, same-day continuation,
rejected and existing residents, day changes and replacement sources. It also
checks the saved object overlay, base restoration and same-day object edits. The
native refresh diagnostic invokes the refresh directly.

Between residents and pets, the host removes distant actors and processes
dead actors. The resolved removal path uses no CRT draws. Death effects can
draw for block-sweep sound and each eligible blast candidate. Nested hit and
item work uses the same stream synchronously. None of these resolved paths
reseeds it. The combined World-step fixture checks a resident refresh, a blast
deflected using the next two draws, and the following pet setup.
[Held authority prelude contract](specs/steam-host-pre-pet-prelude-stream-20261117.md).

A stocked Bomb reaches the destructive actor path through Use Quick Item.
The Bomb loses its health over about four seconds. A neutral Water Bubble
expires after about nine seconds of authority simulation. Its specific
upkeep and ordinary idle resource drain both reduce its lifetime. The
specific upkeep tests the threshold before the second drain. The next authority area pass admits either kind at
zero death clock. The independent contract identifies the separate block and
target draw sites and the conditions for observing them. A stock Barrel
encounter remains unconfirmed.
[Held stock death-area routes](specs/steam-stock-death-area-capture-route-20261117.md).

The engine World fixture uses a Bomb item, advances the fuse and checks that
the next step emits the first area burst, changes breakable terrain and
queues the rolled terrain sound. A later step keeps the events without
emitting another first-visit burst.

The original Steam authority sampled the Water Mage's Water Bubbles action
18 times during play. The creation calls returned in power, size and sound
order for 16 bubbles in two groups of eight. The first creation in each group
preceded the next shared area-target roll by 8,985 and 9,015 ms respectively.
The same recording contains 56 target rolls
from the area pass shared by Water Bubbles, Bombs and Barrels. It records
no source actor ID per roll, so individual bubble lifetimes, damage and
healing remain unverified in the original. CubeVulkan reproduced all 1,163
recorded post-seed authority draws across 78 seeds. The recorder retained
1,523 events and reported zero overflow losses.
Each new bubble takes spell power from both equipped weapon slots. Power,
size and spawn-sound pitch scale each draw by multiplication followed by
division in binary32. The integrated World fixture drives the ultimate
input, observes eight births and checks the first neutral expiry and area
on consecutive authority steps over breakable terrain.
[Creation and caster power](specs/steam-water-bubble-caster-power-20261118.md).
[Full lifetime and area timing](specs/steam-water-bubble-lifetime-20261118.md).

At first death, each admitted target draws once. A roll divisible by four
doubles the stored creation strength before multiplication by the remaining
resource. The caster's relation to the target selects ordinary combat or
healing. A friendly recipient can generate two consecutive full negative
amounts even when the first application reaches maximum HP. The authority
applies the heal once to an eligible NPC. A dedicated host leaves player HP
to the recipient. The receiving guest applies each incoming record to its
local player separately and caps each write. A spell's rolled magnitude,
HP change and displayed number can therefore differ. [Steam Water Bubble target outcomes](specs/steam-water-bubble-target-outcomes-20261118.md).

A Bomb has no owner and is its own hit source, with strength 1 doubled on a critical
roll. Its hits on the local player's pet pass through ordinary defence like any other
target's, and nothing protects a pet specifically. A stock level-1 pet's defence exceeds
2, so it takes no damage, while a player with no defence loses the full amount. Bubble
heals on a pet are applied once, by the authority.
[Steam Bomb and bubble against a pet](specs/steam-bomb-pet-hit-application-20261121.md).

A successful guard takes precedence over shields and emits one block record.
Otherwise a positive combined shield pool produces an absorb record, followed
by an ordinary hit only when damage exceeds the original sum. Guest receipt
first applies incoming statuses, then visits each hit. Each current type-6
shield loses the full absorb-record amount. The recipient sends its resulting
shields in list order as packet 12. It ignores its own status echoes from
packet 7. [Steam guard and shield receipt](specs/steam-bubble-defense-receipt-20261118.md).

Ordinary damage arrives at the resolver with its upstream roll already
applied. Physical category zero selects armor and other categories select
resistance. The attacker's signed combo divided by its positive threshold
is capped above at one. Defense uses the remaining fraction and damage
is floored at zero before later equipment and status effects. Eligible
authority HP settles in single precision with a lower bound of zero.
Production, immediate HP settlement and later guest state are separate
observations. [Steam ordinary hit and HP contract](specs/steam-ordinary-hit-hp-comparison-20261118.md).

A direct swing passes its move number. Projectile contact instead passes
ability zero and the projectile's category. Its damage was stored at
launch and can double on a critical roll against this target. Two
source-less physical hazards also pass zero for ability and category.
The actor's action at contact alone cannot identify the producing throw.
Resolved hit messages carry the signed amount, IDs and outcome. [Steam hit producer contract](specs/steam-direct-hit-roll-provenance-20261118.md).

The day in a terrain edit records when the change occurred. A daily reset
clears archived zones, including entries from the current day. Loaded
zones retain their live histories. An unload after reset can archive those
older edits again. An ordinary save snapshots the archive as it stands.
Loading a valid state restores the stored day and its ordered history.
[Steam zone history retention](specs/steam-zone-history-retention-20261118.md).

On guest receipt, zero material and zero flags remove a block only
inside the stored column span, leaving ordinary air above sea level
or water at and below sea level. The guest retains the transmitted
six-byte value in its resident edit history even when the physical
block differs. Restoring the local saved history uses the general
setter, which can preserve colored air or extend the column downward.
A missing zone receives no edit or history entry. [Steam guest block changes and saved replay](specs/steam-guest-terrain-replacement-20261118.md).

The authority sends hit records to every peer. Pulses and sounds use a
256-block spherical range around the peer, and sounds skip their owner.
Non-player visibility uses 270 blocks. Threat from healing first updates
opponents already tracking the recipient, including on overheal. It can
then spread to eligible actors within eight blocks of each such opponent.
Each range comparison subtracts the signed fixed positions with 64-bit
wrap, evaluates squared displacement in double precision and compares
against a radius squared in single precision. Equality passes. [Steam authority spatial range](specs/steam-authority-spatial-range-20261118.md).
The original Bomb's block record retains signed absolute XYZ, all six old
block bytes, the six-byte empty replacement and the four-byte day. Zone
histories retain duplicates. Loading applies replacements in forward order.
Undo uses the same terrain setter in reverse order. At or below sea level
that setter converts ordinary empty space to water.
[Original Steam terrain persistence contract](specs/steam-bomb-block-persistence-20261118.md).

CubeVulkan read a copy of an original Steam world and wrote its complete
version-44 state payload byte for byte. It contained 935 terrain edits across
seven zones, including 496 repeated records. Each retained its original block,
replacement and day. The save does not identify the action that made an edit. A live original Bomb terrain roll remains open.

When the host creates a pet or changes its equipped race, pet setup seeds the
same thread's stream from the pet's unsigned race. Appearance and gear leave
their final state for subsequent host work. The pet phase keeps the current
stream on a same-race item change. CubeVulkan also continues the authority stream after
the same setup called by taming. [Held pet stream contract](specs/steam-host-pet-setup-stream-20261117.md),
[native taming call](specs/creature-abilities-ranged.md#142-taming-0x7e-0x2ce833).

The engine fixture checks Collie creation, a same-race item edit, a Terrier
race change and a fresh Terrier that inherits the flag for two extra gear
draws. The controlled World step consumes one movement draw before each
comparison. A private setup trace matched the held static vectors directly.
The independent reset
trace verifies that a race change zeroes class and specialization before setup.
Its explicit store supersedes the earlier class-preservation statement in the
pet stream contract. [Held pet reset contract](specs/steam-pet-transient-class-reset-20261117.md).
The first pet maintenance scan reads the byte length of the World's narrow
name. It skips the scan at zero length. The graphical selected character
loads its pet equipment from the version 11 save. The embedded authority
receives that equipment through an accepted peer update. The graphical
mode and the authority owner repair, link and action checks affect whether
the pass can create a pet.
[Held pet admission roles](specs/steam-live-pet-admission-roles-20261117.md).

In the first follow-up, both live player views carried the copied Collie
equipment for all 78 equipped samples. The graphical World was in mode one
and both sampled pet links stayed zero. The later diagnostic recorded 58
equipped samples with authority mode zero, a six-byte World name, an absent
ownership match and a zero pet link. Action `0x9d` was present in all 58.
This eagle-flight action satisfies an explicit creation blocker. It is used
on the title and character-select screens.
[Held title flight behavior](specs/frontend-title.md#31-the-player-on-the-eagle-every-frame-in-0xa5a50-before-the-world-step).

CubeVulkan supplies the configured World name gate independently of the
clock mode. The authority World fixture checks the unnamed scan, eagle
flight and pet creation after the action clears. In an ordinary original
Steam gameplay session, the linked Collie appeared after landing.
All six recorded complete Collie setup sequences matched the held and engine
expectations, including the final equipment draw 19749 and following draw
16204. The compiled engine CRT replay also matched all 2,050 captured
post-seed authority draws in that session.

The stock client starts in its graphical frontend for the traced launch
operands. Use Continue on the visible title menu when the saved slot is valid,
or select a copied character through Start Game. Both paths converge on the
ordinary pointer release. XInput can move or snap the same menu pointer and
send its primary-button edges. Continue loads the remembered slot and closes
the title. [Stock graphical entry](specs/steam-stock-graphical-entry-routes-20261117.md).

## Part C. Referenced material

### C1. Evidence sources

| Source | What it is | Author | Implementer |
|---|---|---|---|
| Steam binary | Unpacked x64 image, SHA-256 `7AF18CB2…F6D244`, base `0x140000000`; facts are cited by RVA | Yes | No |
| Decompiled export | Per-function decompilation, assembly listing, and the function, call and string tables of that image | Yes | No |
| Alpha decompile | The source for Alpha-era behaviour | Yes | No |
| Infinite-Cubes | Third-party reconstruction, authorised on 2026-11-07 for isolated research | Leads only | No |
| Held contracts | `docs/specs/*.md`, copied unchanged after intake | Only if checked suitable | Yes |
| Owner data | Values exported from the player's own installation, kept local | No | Yes |
| Steam recordings | Screenshots and notes from playing the owned game, kept private | As prose in a brief | Yes |
| CubeVulkan source | The engine, extractor, tests and tools | No | Yes |

A finding from the reconstruction becomes a Steam fact only after it is confirmed against the
original Steam evidence.

### C2. Specification library

The [specification library](specs/) groups Steam and Alpha contracts alongside
cross-cutting subsystem references. Check each contract's stated evidence scope
and author provenance before using it. [CLEANROOM.md](CLEANROOM.md) records which
contracts governed each checkpoint and any observed limits.

### C3. Third-party and provenance

- zlib and SQLite formats are public standards. CubeVulkan uses its own copies of both, under their
  own licences.
- The licence provenance of the CWSDK is audited in [sdk-license-provenance](sdk-license-provenance.md).
  The extractor builds and passes without SDK material, and every checkpoint checks this.
- The sister project's GPL code is reused only for the Alpha profile.
- A 2026-10-27 audit removed reconstruction-invented names from code and comments. It also
  re-specified and reimplemented the modules that had been written while reading the decompile.

## Part D. Conformance tests

Tests compare behaviour, never code.

### D1. Test layers

| Layer | What it compares | Examples (EngineTests groups) |
|---|---|---|
| Rule checks | One contract's values and order, on synthetic inputs | hudauras, deathmarker, worldmap, notices, combattext, shrines, soundrules, saveformat |
| Recorded-game oracles | Engine results against values recorded from the running Steam game: creature updates, AI ticks, hits, ability calls and effect steps | aioracle, hitoracle, abilityoracle, updateoracle, effectoracle, statsoracle, droporacle, itemoracle, rulesoracle, animoracle |
| Generation oracles | Generated zones, structures and creatures against zones exported from the game | wgterrain, wgstructures, wgcreatures, zonediff, objdiff, rolesoracle, gearoracle, looksoracle |
| Extractor captures | Values read from a live game | cameracapture, inventorycapture, itemcapture, appearancecapture, creatureposecapture |
| Image checks | Vulkan renders against expected images | steamshader, guiitems, maprender, nativewater, sharedwater, alphascenery |
| Runtime probes | Changed behavior in the full engine under the applicable profiles | Focused private runs with inputs and scope recorded |
| Steam recordings | An engine capture beside a recording with the same save, time of day and settings | private, one set per session |

**Before-failure rule.** For behavior corrections, keep the case that fails
against a stub or the old behavior and passes after the change. Retain the
failing log with the checkpoint.

### D2. Checkpoint gate

A checkpoint is published only when all of the following pass on the exact binaries being frozen:

- **Builds:** all five targets build.
- **CPU suite:** the full required owner-data suite passes, with no skips.
- **Images:** Run the shader, camera, GUI item, map, native water, shared water
  and Alpha scenery groups. Record the counts for the exact checkpoint.
- **SDK independence:** the extractor builds and passes without SDK material.
- **Focused groups:** the feature's own groups pass.
- **Runtime probes:** Exercise the affected live paths with disposable profiles
  and record the scope and save hashes. Track original-client, physical hardware
  and broader release acceptance separately from engine-only runs.

## Appendix. Process

- **Roles.** The work is split between two roles kept in separate contexts:
  - The *source-side author* reads only [CLEANROOM_AUTHOR.md](CLEANROOM_AUTHOR.md) and the assigned
    original evidence. The author delivers one code-free contract, reads it back, and reports it as
    FINAL / STABLE / HELD.
  - The *implementer* reads held contracts in full, CubeVulkan's own source, owner data and Steam
    recordings. The implementer never reads original or decompiled code, SDK source, reconstructions
    or unheld drafts.
  - Verification feedback never goes back to an author; a gap is answered by a new author given a
    neutral question.
- **Lifecycle.** Each change moves through these steps in order:
  1. Trigger.
  2. Brief.
  3. Contract.
  4. Intake: hash the contract, read the whole of it, copy it unchanged into `docs/specs`, and write
     a receipt.
  5. Implementation.
  6. Before-failure tests.
  7. Validation.
  8. Freeze and publish.
- **Contracts** state inputs, outputs, ownership, decision and random-draw order, and constants with
  RVAs, offsets and widths. Each point is marked PROVED or UNPROVED. A contract never contains
  transcribed code, pseudocode or decompiler names.
- **Exposure-stop rule.** A role exposed to prohibited material stops and reports the exposure
  without repeating the material. Further work on that topic needs a new author context. A later
  audit does not erase an exposure.
- **Publication.** Only source is published, through a worktree based on `origin/main`, and remote
  history is never force-pushed. The source and runtime archives are audited, and each push is
  confirmed independently. Owner assets, captures, recordings, saves, extracted data and build
  products are never published.
- **Release gates.**
  - A final provenance and parity audit.
  - Human acceptance: controller play, multiplayer between two Steam accounts, a real first-run
    install and long sessions.
