# CubeVulkan

An unofficial, work-in-progress Vulkan 1.3 replacement engine for Steam Cube World.
The goal is the original game's appearance and gameplay using content from each
owner's installation. This repository contains engine and extraction-tool source;
it does not contain Cube World, its assets, extracted worlds or Steam runtime.

CubeVulkan requires Steam to be running online with an account that owns Cube World
(App ID 1128000). It loads the Steam runtime from the installed game and checks the
license at startup and during play. It does not bypass ownership checks.

## Build on Windows

Install Git and Visual Studio 2022 Build Tools (or Community), including Desktop
Development with C++, the Windows SDK, and C++ CMake tools. A Vulkan 1.3-capable GPU
and driver are required to play. A separate Vulkan SDK is unnecessary: CMake fetches
pinned dependency sources, builds the shader compiler and uses volk to load Vulkan.
The first build needs network access.

```bat
build.cmd
build\EngineTests.exe
build\CubeVulkanSetup.exe
```

The extractor additionally needs the separately obtained CWSDK headers and a
compatible CubeModLoader installation. Neither is bundled. CMake looks for headers
at `../SteamCubeWorldMods/vendor/cwmods` by default; from a developer command prompt,
set `-DCWSDK_DIR="<path-to-headers>"` when configuring CMake to use another location.
Without those headers the engine builds, but the `ZoneExtractor` target is omitted.
See [dependency notices](THIRD_PARTY_NOTICES.md).

## Set up the owner's game data

1. Install Steam Cube World and launch it normally once. Back up your saves.
2. Open **CubeVulkanSetup.exe** and choose **Get ready to play**. The wizard checks
   the supported game build, Steam connection, ownership and missing data. It needs
   a locally built `ZoneExtractor.dll` and compatible CubeModLoader.
3. Follow the wizard's in-game extraction steps. It journals the temporary changes,
   sets aside mods and ReShade, preserves Save, and restores them after the game
   closes. An interrupted run offers recovery on the next launch.
4. Use **Check data** to inspect the result, then **Play**. The game content stays
   in your installation; do not put it in this repository.

The wizard has automated tests against a simulated installation. A real first run
and a fresh-machine installation still need validation. Review the
[extraction guide](docs/extraction.md) before using it with your game.

`CubeVulkanDataCheck.exe --deep --report owner-data-report.txt` checks local exports
without opening the renderer. `CubeVulkan.exe --check-steam` checks the connection.
Both retain the Steam/license requirement.

Cube World generates its landscape in code. The local extractor exports generated
blocks, placements and the data tables the replacement engine needs. The engine
reads the owner's `.cub` sprites, fonts, GUI shapes, dictionary, music and sounds.
The extractor source is included; its output is private game data.

## World and rendering options

Edit `CubeVulkan.ini` beside the executable (the template is in `config/`).

- `[World] Source=Extracted` is the default: load the owner's exported zones. Areas
  outside those exports are unavailable.
- `[World] Source=Game` uses the independently implemented generator in
  `src/worldgen/`: terrain, structures, objects, plants and creatures. Owner-exported
  tables are still required for complete content. See [world generation](docs/worldgen.md).
- `[World] Source=Generated` is an older experimental terrain mode for development.
- `[Render] Style=Alpha` is the default lighting/sky style; `Steam` selects Steam's
  style. Rendering still has parity gaps. Neither setting establishes a 1:1 match.
- Shadows, TAA/SMAA, moonlight and default-on water lilies are optional CubeVulkan
  additions. `[Shadows] Enabled=0`, `[Render] Moon=0`, and `[WaterLilies] Enabled=0`
  disable the corresponding additions. Lilies decorate rendering without changing
  generated blocks or consuming the generator's random stream.

Normal startup opens the eagle title flyover, character selection/creation and
options. `--play` skips the frontend. Characters and world changes use the native
save formats in the engine's own `Save/` directory. `[Saves] ImportFrom=` can import
an owner's native Save directory when no engine save exists; the originals are read
only. See [menus](docs/menu.md) and [saves](docs/saves.md).

Enemy deaths are configurable in `CubeVulkan.ini`:

```ini
[Creatures]
DeathMode=Timed
RespawnSeconds=300
```

`Timed` is the default: defeated spawn-point enemies return after five minutes of
active host time, even if their zone unloads meanwhile. `Instance` keeps them dead
until their original spawn zone unloads for every player. `Permanent` retains native
saved-death behavior. These are host settings; restart to apply changes. In respawn
modes, temporary deaths are not saved and old enemy death entries are recovered as
their zones load. Player deaths, friendly NPCs, pets and world objects keep their
own rules. See [enemy lifecycle](docs/creatures.md#configurable-enemy-deaths).

Controls come from `controls.cfg` and are editable in Options. Defaults include
WASD movement, left/right mouse attacks, middle mouse dodge, Shift class skill,
B inventory, Enter chat, J multiplayer and Esc/O system menu. Development shortcuts:
F11 flight, Page Up/Down time, F7 shadows, F8 leaf sway, F9 TAA, F10 SMAA, F12 capture.

## Implementation and validation

The engine implements native-format saves, character control and poses, creature
simulation, combat, conversations, regional missions, items, shops, crafting, loot,
map rendering and Steam/UDP networking. These follow the written behavioural
specifications in `docs/specs/`; implementation agents read those specifications
rather than game code. See [the clean-room process](docs/CLEANROOM.md).

`EngineTests` runs synthetic regressions and comparisons with owner-exported data
when available. Missing local oracles are skipped, so a passing run on a new machine
does not verify all game comparisons. GPU smoke captures and recorded data checks
are evidence for the tested cases, not complete gameplay or pixel parity.
On Windows with a Vulkan driver, `build\EngineTests.exe steamshader` additionally
checks lit/Glow/White color references and full-fog interpolation with synthetic
geometry. It needs no game assets and is separate from the default CPU suite.

The [current checklist](docs/roadmap.md) tracks remaining work. In particular:

- real two-account Steam sessions and an original-game peer remain untested;
- fresh-machine setup and extended gameplay/save recovery need testing;
- some ability/AI, pet, controller, conversation and network payload details remain open;
- water lighting, distant-model ordering, logo shading and visual comparisons need work.

The optional mod API is in `include/cubevulkan/ModApi.h`; `HelloMod` builds into
`build/examples` and only runs if copied into the engine's `mods/` directory.
CubeModLoader game patches are separate from engine mods.

## Prepare source for GitHub review

```powershell
powershell -ExecutionPolicy Bypass -File tools\package_source.ps1
```

This creates a fresh `build/CubeVulkan-upload-<timestamp>/` tree and
`build/CubeVulkan-source.zip`, with a SHA-256 file manifest. Upload the contents of
that reviewed tree, rather than this development directory. The packager includes
source `.inc` files, specifications, configuration templates, dependency notices
and separately licensed project art. It excludes game data, captures, saves,
binaries, SDK checkouts, private handoffs and local build/research folders.

CubeVulkan's original engine and extractor source is licensed under the
[GNU General Public License version 3](LICENSE) (`GPL-3.0-only`), without warranty.
Third-party components and project art retain their separate licenses and notices;
see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Read [PUBLICATION.md](PUBLICATION.md) and review source provenance and rights before
public release. The clean-room
process, ownership check and asset separation do not themselves establish legal
clearance. CubeVulkan is not affiliated with or endorsed by Picroma or Valve.
