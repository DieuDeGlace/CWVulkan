# Windows development package

`tools/package_windows.ps1` prepares a local package. It does not publish a release
or claim that the readiness checklist is complete.

Build `CubeVulkan`, `CubeVulkanSetup`, `CubeVulkanDataCheck` and `ZoneExtractor`.
The Windows extractor target uses owned adapters without CWSDK; CubeModLoader
remains a separate installation for loading it into the original game.
For an independent DLL build without renderer dependency downloads, configure
`-DCUBEVULKAN_EXTRACTOR_ONLY=ON` in a separate build directory. Its output is not
a complete engine package. The normal multi-target build below remains the
packaging source.
Run EngineTests, then create the matching source archive:

```powershell
.\build.cmd --target CubeVulkan CubeVulkanSetup CubeVulkanDataCheck ZoneExtractor EngineTests
.\build\EngineTests.exe
.\tools\package_source.ps1
.\tools\package_windows.ps1 -Version development -SourceRevision <git-commit>
```

The tool validates the executables and extractor as x64 PE images. It stages a fresh
folder under `build/releases/`, produces a Windows ZIP with a SHA-256 manifest and
copies the source ZIP beside it. It includes:

- `Play-CubeVulkan.cmd`, opening the setup/launcher;
- `CubeVulkan/` with the engine, setup and data checker;
- `CubeVulkan/tools/ZoneExtractor.dll` for Setup to install locally;
- the default config, GPL text, dependency notices and separately licensed project art.

The user unpacks the Windows ZIP into the installed Steam Cube World folder and
runs `Play-CubeVulkan.cmd`. Keeping the engine in its subfolder isolates its
`CubeVulkan/Save` from the original game's `Save`. Do not place the executables
directly beside the original game's executable.

Steam must be online with game ownership. The first run still needs owner-data
extraction; compatible CubeModLoader is a separate prerequisite, not bundled here.
Setup reports missing dependencies and guides the game/extraction steps. Packaging
an extractor DLL cannot remove the requirement to obtain content from the owner's
installation. Back up saves before the first real extraction run.

Setup launches the selected game folder with Steam running. Leave the game
open until setup reports that its requested data has been collected. If setup
closes while extraction is still active, quit Cube World and open the wizard
again. The command-line recovery is `--setup-restore --game PATH` from
CubeVulkanDataCheck. Setup preserves collected files and will offer to extract
an incomplete item-model table again.

No game assets, saves, exports, Steam DLL, loader binary, PDB or build library is
bundled. The package includes only the extractor DLL compiled from this project's
source. Retain the matching source asset when distributing GPL executables.

Run `tools/validate_extractor_no_sdk.ps1` after building to inspect the actual
compiler commands and header dependencies. The [historical SDK audit](sdk-license-provenance.md)
still grants no additional rights to legacy SDK material. This target no longer
includes those headers; live supported-game qualification is a separate gate.

An isolated `--setup-capture --title-only` session writes six frames to
`Extract/ui` when the game starts. Create an empty `Extract/ui.request` during
the same session to capture six more frames. The extractor consumes the
request after the current batch finishes. Use `--setup-restore` after quitting
to recover the copied saves and mods.

A final public release waits for the [remaining readiness checks](roadmap.md).
In particular, a local package/PE check does not validate the real first-run wizard,
an original-game network peer or a fresh machine.
