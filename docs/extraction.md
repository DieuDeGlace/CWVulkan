# Checking an owner's extracted data

## First-run wizard

`CubeVulkanSetup` opens **Get ready to play** by itself when something needed is missing (or an earlier run was interrupted); the button of that name opens it at any time. It has three parts and a Play button:

1. **The game:** the Cube World folder (from `CubeVulkan.ini`, else every Steam library listed in `libraryfolders.vdf`), the executable's build (PE timestamp `0x5d93ab0d` is the one the extractor supports), CubeModLoader, and Steam online with a Cube World licence, checked as the engine does.
2. **What is extracted:** each part the engine reads, present or missing, and what is lost without it: game files and extracted land (needed to play; land is not needed with `[World] Source=Game`), the model list, sound list (`Extract\sounds.txt`), creature tables (`Extract\oracle\`: spawn table, races, pickers, looks, gear), item models (`item-models.bin`) and special items (`items.bin`). DataCheck's other findings follow as notes.
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
