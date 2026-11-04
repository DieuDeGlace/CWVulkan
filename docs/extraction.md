# Checking an owner's extracted data

## First-run wizard

`CubeVulkanSetup` opens **Get ready to play** by itself when something needed is missing (or an earlier run was interrupted); the button of that name opens it at any time. It has three parts and a Play button:

1. **The game:** the Cube World folder (from `CubeVulkan.ini`, else every Steam library listed in `libraryfolders.vdf`), the executable's build (PE timestamp `0x5d93ab0d` is the one the extractor supports), CubeModLoader, and Steam online with a Cube World licence, checked as the engine does.
2. **What is extracted:** each part the engine reads, present or missing, and what is lost without it: game files (always required), captured land (not required with `[World] Source=Alpha`, `Game` or `Generated`), the model list (required by the default Alpha mode), sound list (`Extract\sounds.txt`), creature tables (`Extract\oracle\`: spawn table, races, pickers, looks, gear), item models (`item-models.bin`) and special items (`items.bin`). DataCheck's other findings follow as notes.
For Alpha, setup also parses the two original palm models and seventeen plant models, and checks that the owner-exported environment table identifies every required plant. Missing or damaged original `.cub` files require Steam file repair; extraction does not create replacements. Setup uses the same case-sensitive generation names as engine startup.

3. **Extract missing data**, one button. With the game closed, the setup:
   - copies `Save` to `<game>\CubeVulkanSetup\run-<time>\Save` and checks the copy;
   - moves the owner's `Mods` folder and `dxgi.dll` (ReShade) into the same folder, and installs `ZoneExtractor.dll` alone in a fresh `Mods` (CubeModLoader, already in the game folder, loads it);
   - writes the requests the extractor answers by itself once a world is loaded (`sounds.request`, `oracle.request`, `itemmodels.request`, `items.request`; an existing `Extract\oracle` is copied into the run folder first);
   - starts Cube World through Steam and says what to do: Start Game, pick or create any character, and press **F11** once for the sweep around home if land is missing. It watches `Mods\ZoneExtractor.log` and the output files and ticks each part off;
   - when everything is collected, asks the owner to quit (or closes the game's window with **Close the game**), and once the game has gone, for whatever reason (quit, crash, **Stop and put back**), puts everything back: the run's `Mods` (with the extractor's log) is kept in the run folder, the owner's `Mods` and `dxgi.dll` return, and `Save` is put back exactly (the session's Save is kept as `Save-after-run`). Unanswered requests are removed; what was extracted stays.

Each step is written to `<game>\CubeVulkanSetup\journal.txt` before it is taken. If the setup is closed, the PC restarts or anything else interrupts a run, the next start of the setup (or `--setup-restore`) puts the folder back from the journal; each step looks at the folder as it is, so this is safe at any point. The journal then moves into its run folder. Steam Cloud may notice that Save changed back after the session and ask which copy to keep; keep the local files.

Headless, for scripts and tests (`CubeVulkanDataCheck` is the console program; `CubeVulkanSetup` accepts the same):

```bat
build\CubeVulkanDataCheck.exe --setup-check [--game PATH]
build\CubeVulkanDataCheck.exe --setup-extract --dry-run [--game PATH]
build\CubeVulkanDataCheck.exe --setup-extract [--game PATH]
build\CubeVulkanDataCheck.exe --setup-restore [--game PATH]
```

`--setup-check` exits 0 when ready to play, 1 when data is missing, 2 when the game or Steam cannot be checked. `--setup-extract --dry-run` prints what a run would back up, set aside, install and request, changing nothing (0: a run could start, 1: nothing to extract, 2: blocked, with the reason). Without `--dry-run` it runs the extraction from the console. `EngineTests ownersetup` runs the setup's unit tests alone (they are also in the default run).

## Data check

`CubeVulkanSetup` provides **Check data...** for a quick report. It reads the installation selected in Setup. It requires Steam online with a Cube World license, as the engine does. It does not install anything, generate missing models, or write game files or saves.

The console companion supports a deeper audit without opening a Vulkan window:

```bat
build\CubeVulkanDataCheck.exe --help
build\CubeVulkanDataCheck.exe --deep --report build\owner-data-report.txt
build\CubeVulkanDataCheck.exe --game "D:\SteamLibrary\steamapps\common\Cube World" --world "D:\SteamLibrary\steamapps\common\Cube World\Extract\seed_123"
```

Without `--game`, it uses `CubeVulkan.ini` beside the executable and the normal Steam-library search. `--world` selects a particular extracted seed folder; otherwise the configured extraction or newest seed folder is used. `--report` creates a new UTF-8 text file and refuses to overwrite an existing file. Its parent directory must already exist. The report also appears in the console.

Exit codes are 0 for no blocking data errors, 1 for data errors, and 2 for invalid arguments, unavailable Steam/license verification, or a report-write failure. Warnings may still accompany exit 0. Steam is checked again after the scan.

## What is checked

Quick mode checks the executable's x64 PE header and supported extractor timestamp, original sprite/font presence, the manifest and actual home terrain, placement-file coverage, player appearances and animation loading, mapped class moves, gear surveys, the native title recording and HUD export presence. Other terrain and sprite payloads are only indexed in quick mode; the report says so.

`--deep` additionally parses every original `.cub`, validates all terrain run-length records and placement containers, checks referenced placement models, lists object types still using estimated sizes, and loads creature appearances and poses. Long terrain scans print progress. Estimated sizes are reported from the same reader used by the engine, so measurements loaded from `sprite-scales.txt` take precedence over fallbacks.

“Ready to test” means basic data is available. It does not establish complete world coverage, complete classes/equipment, native gameplay behavior, or visual parity. Nonempty fonts and HUD images are presence checks, not full resource validation. Unsupported executable builds cannot be used to install this extractor even when an existing export remains readable.

## Addressing gaps

- Missing or damaged original models/fonts: repair the owner's Steam installation.
- Missing terrain or placements: revisit the area with the extractor, or run its F11 sweep. Keep the matching `manifest.txt` with the zones. This captures generated/visited areas, not an entire infinite world.
- Missing player appearance or moves: use F5 for a snapshot and F4 for animation recording. Missing moves are skipped; replacement animation is not generated.
- Missing initialized creature appearances/poses: use the F3 tour while the original game draws those creatures.
- Missing creature loot flags/modifiers: future F3 captures with the updated extractor record appearance flags and a fifth equipment field for each modifier. Old four-field captures stay readable and are reported as incomplete; the checker does not backfill guessed metadata.
- Estimated object sizes: collect native transforms for the listed types. A fallback size is not evidence of native size.
- Missing native title scene: follow [menu.md](menu.md). `--play` can test gameplay directly.
- Missing HUD exports: capture a frame with extractor F10 and process it with `HudExtract`, as described in the README.
  Alternatively, with the supported game and extractor running, create an empty
  `Extract/ui.request` file. The extractor consumes it and queues the same six
  frames, 300 ms apart, under `Extract/ui`; this also works at the title menu
  without a loaded character. Requests wait for an active capture batch to finish.
  Check `Mods/ZoneExtractor.log` for six saved frames; a consumed request alone is
  not proof that the game presented or that frame saving succeeded. The request
  accepts no output path and changes no gameplay or save state. Keep these original-game
  images local; they do not belong in source or release packages.

Close the original game before installing or updating the extractor. Setup checks that the supported game is closed, checks its x64 loader/DLL prerequisites, stages the new DLL, and only then replaces the old extractor. Existing exports can be checked repeatedly without reinstalling or running the game.

All exports stay local to the owner. The source archive includes the readers and extraction tools, while excluding game content, recordings, screenshots, native executables and Steam DLLs. See [PUBLICATION.md](../PUBLICATION.md) for the separate source and rights review still required before publication.


## Refreshing zone and light snapshots

The updated extractor records the separate auxiliary model and static-object light
lists, including invisible light markers. Older zone exports can omit the static
light list even when their terrain and visible placements are usable.

Close the original game before replacing its extractor DLL. After restarting with
the updated DLL and loading a character, press **F5** once. It saves the character
snapshot and queues a refresh of loaded zone snapshots, including previously
cached exports. Allow the writer a few seconds to finish. F4 remains the animation
recording toggle. The engine reads these records directly; it does not create
replacement lamp assets or infer the missing auxiliary list from sprite names.

For vanilla parity comparisons, use the wizard's isolated extraction workflow.
Captures made with gameplay/world-generation mods enabled describe that modified
run. In particular, a BigRegions Scale2 region query is not a vanilla Scale1 oracle.
Keep capture provenance separate instead of changing expected values to hide the
difference. Native light-layout fixtures and synthetic GPU checks can be run with
`EngineTests steampoints`, `EngineTests steamterrain` and `EngineTests steamshader`.


## Isolated reference recapture

`CubeVulkanSetup.exe --setup-capture --game "<Cube World folder>"` prepares a
vanilla reference session even when the normal completeness check finds nothing
missing. Add `--dry-run` to inspect the preparation without changing files.
The command uses the same journaled Save backup and temporary Mods/ReShade
isolation as the first-run wizard. Existing exports remain in place.

Launch the game and choose Yes for the extractor-only mod session. The queued
`scene-reference.request` runs on a normal game tick after session initialization:
it writes terrain reference values and refreshes loaded zone/light snapshots,
even when those zones were already exported. The title flyover is sufficient;
loading a character and timed key presses are unnecessary. F5 and O remain
available for manual refreshes while Cube World is the foreground application.
Queued file requests still run on game ticks in the background. When another DLL or legacy `.cwmod`
is present in `Mods`, generator, block and cell dumps go under
`Extract/modded-reference/`; scan failures use that separate directory too.
The header records the build, detected mod filenames and scan error. File presence
is a conservative provenance check, not proof that each mod loaded. Use the isolated session for a vanilla world reference.
Allow the writer a few seconds to finish, close
the game, then use `--setup-restore --game "<Cube World folder>"` to restore Save,
Mods and ReShade. A pending journal is also detected after an interruption.

Deep data checking reports how many zones contain both auxiliary light lists.
A valid empty list counts as a captured list; an omitted list does not. Old
exports remain readable, with a refresh warning rather than guessed lighting.

The optional owner-data light oracle checks a list of fresh `.rec` filenames
(one filename per line) against the engine reader and neighboring-zone light
solve, including the native mesh-written model light samples:

```
EngineTests steamlights "<Extract/seed_N>" "<fresh-zone-filenames.txt>"
```

It fails on mismatches, missing neighborhoods or no nonempty light samples.
Keep the file list and captured records local. The 2026-10-30 isolated run captured
both auxiliary lists and cache metadata in 1,590 zones; all 8 model and 9 static
light records in 15 nonempty zones matched. The same vanilla run resolved both
region-oracle failures: all 3,823 sampled points across 25 terrain fields and 4,847
block samples matched. The earlier region reference came from a BigRegions-modified
session. No generator or expected-value changes were needed.

The 2026-10-31 repeat isolated capture refreshed 1,596 zone records. Its 17 nonempty
zones matched 8 model and 11 static light records, with no mismatches or incomplete
neighborhoods. All 3,823 terrain points across 25 fields, 4,847 block samples and 60,000
cell-object samples matched; the full CPU/owner-data suite passed. Save, Mods and
ReShade's dxgi.dll were restored with all 58 original file hashes unchanged.
Routine extractor startup no longer exports executable function-byte dumps, and
unsupported hook prologues are reported by location without logging their bytes.
Historical diagnostic files remain local; none belongs in an upload package.

## Refreshing terrain edit materials

The updated extractor also saves a `.cwm` material sidecar beside each zone's
`.cwz` blocks, including generation-oracle exports. The live-zone capture holds
its usual zone and mesh locks. These cached inputs let downward terrain edits use
the owner's original materials rather than a guessed replacement color.

DataCheck reports coverage as **Terrain edit materials**. If it reports older
block-only captures, update the extractor with the original game closed, then
refresh the relevant zones with F5 or the F11 sweep. Keep the sidecars beside the
matching blocks and manifest. Old block files remain readable; generated worlds
retain their own inputs. The new codec/reader and synthetic cases are verified;
a fresh original-game sidecar capture/comparison is still an acceptance check.
Exports remain owner-local and are not distributed in either package.

## Primary sprite metadata

The primary `sprites.txt` exporter uses the independently specified native catalog
view in [the bounded registry contract](specs/steam-native-sprite-catalog.md). It
reads catalog-ID order and raw voxel dimensions through guarded byte copies, with
an exporter ceiling of 65,536 entries. Empty/unready records retain their slots.
Capacity entries are not exported, and changing ranges or unreadable fields abort
the table rather than publishing partial metadata. A range recheck does not supply
concurrent lifetime synchronization. No RGB allocation or GPU readiness is required
for this metadata, and the dimensions are not a per-object scale. Procedural and
other resource families remain separate; the whole extractor still depends on SDK
views. Use `EngineTests extractorcatalog` for the portable memory-fixture checks.

## Keep modified captures separate

Ordinary zone exports now use `Extract/modded_seed_<n>` when the Mods inventory
contains another DLL, a legacy `.cwmod`, or cannot be completed. An extractor-only
session retains `Extract/seed_<n>`. Each session writes `capture-source.txt`, whose
classification covers **new writes in that session only**; it does not certify
older retained files. Existing exports are neither moved nor deleted. Historical
`seed_<n>` folders can already contain mixed data and need independently recorded
references for strict comparisons.

Automatic extracted-world selection considers `seed_<n>` folders. To deliberately
replay modified captures, set `World.ExtractedPath` to their full directory and
choose `World.Source=Extracted`. Alpha and Steam generated modes remain separate.

In a modified session, height/block/cell reports and block-writer diagnostics use
`Extract/modded-reference/`. Requested generation uses its `gen/` subdirectory;
the extractor-only path remains `Extract/oracle/gen/`. The formats and native
actions are unchanged. Neither a file inventory nor a directory name proves a
pristine runtime; use the journaled isolation workflow for vanilla candidates.

`CW_TERRAIN_ZONES_DIR` optionally selects a separate, owner-local reference folder
for **only** the terrain comparison in EngineTests. Other owner/spawn datasets
stay unchanged. Its leaf should retain `seed_<n>` for seed parsing. An unavailable
explicit directory fails the check. The override does not certify provenance or
relax any region, geometry, color or lighting assertion. Clear it after use.

Generated-on-request zones have not passed mesh-time lighting. Keep those files
separate from loaded, lit snapshots; comparing the former to a lit terrain oracle
can produce legitimate air/water-light differences. The isolated loaded reference
for four coordinates compared all 106,380 blocks and all four region types exactly.
Captures and this pinned reference remain local and are excluded from packages.

## Active terrain buffer identities

The [independent active-list contract](specs/steam-terrain-buffer-resources.md)
supplies the Zone sentinel/count and linked node/group fields. The reader follows
links, validates their count/tail/backlinks, bounds capture allocations and rejects
changed publication before returning borrowed buffer identities. Pending groups
are not traversed. The ordinary wrapper reader is shared with sprite capture; it
does not invoke the native bind methods or establish GPU readiness.

The caller uses nonblocking loaded-zone then mesh-publication critical-section
acquisitions and holds both through enumeration and buffer visits. Rechecks alone
do not supply synchronization or lifetime extension. The reached eviction path is
covered by that order; no universal guarantee for every external mutation follows.
World-container enumeration in this path now uses the independently specified
sentinel/entry-count sequence, not an SDK container iterator. Other exporter
paths remain SDK-dependent.
`EngineTests extractorchunks` exercises malformed-memory fixtures without assets.
An extractor-only supported-game title smoke identified 3,230 sprite buffers and
1,172 terrain buffers while presenting six distinct decoded frames. Owner
Save/Mods/ReShade were restored and hash-checked after closing. This does not
establish modified-session, character/combat or universal shutdown-lifetime parity.

The [loaded-zone sequence contract](specs/steam-loaded-zone-container.md) now
supplies the containing World iteration too. It reads linked element order and
node+0x18 Zone values, not hash buckets or terrain-node payload offsets. The count
covers entries, including null Zone values that native consumers skip. Capture
limits and count/link rejection are defensive project policies. The shared lease
retains the documented World container/publication lock pair through both the
World and per-Zone traversals. These addresses still require a valid supported
World; neither copied pointers nor a lock guard supply ownership after unlock.

`EngineTests extractorzones` checks the byte view; `extractorlocks` exercises real
Win32 critical-section contention and scope/unwinding release on synthetic storage.
`CaptureMemory.h` centralizes the AMD64 guarded-reader arithmetic contract. These
checks complement, rather than replace, the pending supported-game smoke. Other
extractor paths still use SDK fields, and the known live-installed DLL is retained.
