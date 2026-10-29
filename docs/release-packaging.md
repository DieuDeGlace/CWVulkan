# Windows development package

`tools/package_windows.ps1` prepares a local package. It does not publish a release
or claim that the readiness checklist is complete.

Build `CubeVulkan`, `CubeVulkanSetup`, `CubeVulkanDataCheck` and `ZoneExtractor`.
The extractor target needs the separate compatible CWSDK headers (see README).
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

No game assets, saves, exports, Steam DLL, loader binary, PDB or build library is
bundled. The package includes only the extractor DLL compiled from this project's
source. Retain the matching source asset when distributing GPL executables.

A final public release waits for the [remaining readiness checks](roadmap.md).
In particular, a local package/PE check does not validate the real first-run wizard,
an original-game network peer or a fresh machine.
