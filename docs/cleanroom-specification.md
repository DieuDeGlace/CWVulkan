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
| Creatures and combat | Run AI, abilities, hits, damage, buffs, upkeep and poses with Steam's rules and timings | creature-\*, stat-rules | Ported |
| Player control | Map mouse, keyboard and controller input to movement, camera, skills and use actions | player-control, use-prompt, skills-and-interactions | Controller untested |
| Items and loot | Generate, name, drop, pick up, craft, buy and sell items | drops-and-items, loot-\*, lore-and-artifacts, item-models | Ported |
| Interface | Draw the title, character, options, HUD, inventory, map and dialogue screens from the player's Plasma files | plasma-\*, frontend-\*, hud-money, conversation-script, steam-hud-\* | Ported |
| World map | Build the map model and draw the corner and full maps with markers, labels, missions, history and the player | worldmap-model, worldmap-rendering, map-markers | Recorded match |
| Rendering | Reproduce the sky, clouds, lighting, shadows, water, vegetation wind and particles | sky-and-clouds, shadows, water-rendering, vegetation-wind | Waterfalls and rain not yet recorded |
| Sound and music | Play the same sounds, ambient emitters and music for the same events and places | sound, steam-\*-music-\* | Ported |
| Saves | Read and write characters.db and world.db so Steam and CubeVulkan open each other's saves | native-saves | Ported |
| Multiplayer | Host and join sessions with the Steam protocol and roles | netcode-\* | Two-account test pending |

Open questions are tracked in [remaining-runtime-gaps](specs/remaining-runtime-gaps.md) and
[worldgen-parity-gaps](specs/worldgen-parity-gaps.md). An unproved point stays open; it is never
filled in by guessing.

### B2. Extensions and preserved looks

Features the user asked for that Steam does not have are labelled as project choices, never as
parity claims:

| Extension | Provenance |
|---|---|
| Alpha terrain and lighting profile | Reuses the sister AlphaVulkan project's own clean-room GPL implementation under its independent contracts |
| Dual-tone hair controls | Built on CubeVulkan's own tinting and profile APIs |
| World list pages | An optional extension beside the native world selection |

Every checkpoint preserves these looks: Alpha relief and lighting, the dry purple Undead Lands,
distinct biome palettes, and the gentler Savannah and Omega water.

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

`docs/specs` holds 355 contracts: 272 bounded Steam contracts (`steam-*`), 23 Alpha contracts
(`alpha-*`) and 60 subsystem references. Each bounded contract answers one question, carries a date
in its filename, and ends with an evidence-boundaries section and the author's exposure statement.
[CLEANROOM.md](CLEANROOM.md) records which contracts governed each checkpoint.

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
| Runtime probes | The full engine under the Native and Alpha profiles | 40 runs per checkpoint |
| Steam recordings | An engine capture beside a recording with the same save, time of day and settings | private, one set per session |

**Before-failure rule.** A new check must fail against a stub or the old behaviour, and pass after
the change. The failing log is kept with the checkpoint.

### D2. Checkpoint gate

A checkpoint is published only when all of the following pass on the exact binaries being frozen:

- **Builds:** all five targets build.
- **CPU suite:** the full required owner-data suite passes, with no skips.
- **Images:** 92 shader, 2 camera, 106 GUI item, 17 map and 14 native water images, plus the shared
  water and Alpha scenery groups.
- **SDK independence:** the extractor builds and passes without SDK material.
- **Focused groups:** the feature's own groups pass.
- **Runtime probes:** 40 probe runs pass, each leaving the private saves unchanged.

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
