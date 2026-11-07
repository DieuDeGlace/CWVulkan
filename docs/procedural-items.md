# Procedural item data

Category 23 artifacts use per-key voxel resources rather than the ordinary sprite
catalog. The independent native contract is in
[steam-procedural-item-resources.md](specs/steam-procedural-item-resources.md).
Extracted volumes belong to the owner's installation and are excluded from source
and runtime packages.

## Owner extraction request

With the supported original game and extractor running, write
`Extract/procedural-items.request` as UTF-8 text:

```
CWP1
0 ff112233
3 00000001
```

Each row is a signed decimal ID and the full modifier as up to eight hexadecimal
digits, without a prefix. Blank rows and rows starting with `#` are allowed.
There may be 1..512 unique keys. Keys retain all bits and compare in the native
signed-pair order. A request interrupted by closing the game resumes from
`procedural-items.processing` on a subsequent run.

The extractor resolves one key per game tick, only on the game's window thread,
through the initialized native model store. It copies the returned resource's
full allocation and metadata before handing the owned copy to its file worker.
There is no inventory/character/world mutation or model-pointer export. A native
cache miss can allocate its normal voxel/mesh resource; it requires the original
initialized game/device, not an off-thread memory-reader approximation.

Read `Extract/procedural-items.status` for remaining, written, lookup-failure and
write-failure counts. A complete status waits for the queued atomic file writes;
closing sooner can leave the request for recovery. Each final data file is in
`Extract/procedural-items/`, named `p_<ID bits>_<modifier bits>.cwp`. Each word is
eight lowercase hexadecimal digits. A temporary file is renamed only after its
write closes successfully.

## Isolated capture through Setup

To preserve existing terrain, character recordings and table exports, stage only
procedural-resource capture while the original game is closed:

```powershell
.\build\CubeVulkanDataCheck.exe --setup-capture --procedural-items keys.txt --game "<Steam Cube World folder>"
```

The request file uses the CWP1 format above. `--dry-run` validates it without
changing the installation. Setup backs up Save, sets aside Mods/ReShade and
installs the extractor with a persistent `procedural-only.request` marker. Launch
the game through Steam after this command exits and allow mods. The initialized
title world can supply the model store; no inventory or character interaction is
required. Wait for `procedural-items.status` to report complete, with zero lookup
and write failures, then close the game and run:

```powershell
.\build\CubeVulkanDataCheck.exe --setup-restore --game "<Steam Cube World folder>"
```

This mode skips general export hooks, recorders and hotkeys, while retaining the
procedural capture's native-window-thread requirement and asynchronous writer.
The mode marker remains until restoration. Setup rejects mixed capture modes,
malformed/oversized input and existing competing requests before moving owner
files. Recovery removes its marker and unanswered request, preserves completed
resources, and retains an interrupted `.processing` file for the existing resume
protocol. A new isolated batch is refused while that file is present.

## CWP1 resource record

All multibyte fields are little-endian. This is CubeVulkan's data format, not a
native object dump. The fixed header is 44 bytes, followed by the unchanged RGB
allocation. Native pointers, tree nodes and device handles are never stored.

| Offset | Field |
|---|---|
| 0 | Four bytes `CWP1` |
| 4 | Signed 32-bit ID |
| 8 | Full 32-bit modifier word |
| 12, 16, 20 | Signed 32-bit allocation dimensions X/Y/Z |
| 24 | Native voxel-classification policy byte |
| 25 | Native mesh input/color-mode byte |
| 26..28 | Native additive shade R/G/B |
| 29 | Origin: 0 independent generation, 1 completed native lookup, 2 completed native generation |
| 30..31 | Reserved zero bytes |
| 32 | Signed 32-bit corner-sampling radius |
| 36 | Supported source executable timestamp, or 0 for independent generation |
| 40 | Unsigned 32-bit RGB byte count |
| 44 | Raw R/G/B triples, X fastest, then Y, then Z |

The transport accepts dimensions 1..256 per axis, validates the exact byte count
and rejects trailing data. Native category23 capture requires its full 24³
allocation. Metadata is preserved independently of renderer support. In
particular, mode 1 must not silently take stock-CUB color halving, and cropping
the occupied volume changes native icon placement.

The codec's synthetic tests check key bits/order, endianness, full RGB ordering,
metadata transport and malformed/truncated input without distributing game data.
Native owner captures remain necessary to validate procedural shape generation.

## Engine consumers

The engine first looks for a completed owner resource with the full ID/modifier
key. It accepts the supported 24-cube allocation, classification policy 1, color
mode 1, radius 3 and zero additive shade. Missing or unsupported records use the
independent shape generator described by the specification. All seven named
families and the default sphere family are implemented; native RGB comparisons
remain a separate validation gate.

GUI icons and dropped items retain the full allocation. Steam meshes use native
mode 1 inputs without stock-CUB halving. The optional Alpha appearance converts
the resource's stored half colors to logical colors before applying Alpha's
shading. Ordinary CUB resources retain their existing input path.

Run `EngineTests proceduralitems` for codec, request, cache, shape and consumer
checks. Native comparisons read only completed owner captures and compare the
independent generator directly with every stored RGB byte and semantic metadata.
Set `CW_REQUIRE_PROCEDURAL_ORACLES=1` to fail when no native captures exist.
Synthetic cache fixtures are kept under the build directory, separate from the
owner-capture corpus. Neither corpus belongs in a release.

## Local native comparison coverage

The owner extractor completed a 332-key reference capture on the supported Steam
build (PE timestamp `5d93ab0d`) with the owner's mods active. Every resource was
reported as a completed native lookup. Independent generation matched every RGB byte and
semantic metadata field in all 332 records. The sample covers IDs 0 through 6 at
modifiers 0..31, additional vessel/crown modifiers 32..63, both default-family
IDs -1 and 7, and four high-bit modifier words across all nine tested IDs.
This is bounded sample coverage, not an exhaustive claim over every possible key
or a reconstruction of internal CRT trigonometry. The captured files and their
private manifest are not distributed.

### Fresh-PC isolated comparison — 2026-11-07

An extractor-only isolated run completed 324 native lookups with no lookup or
write failures: IDs -1 through 7, modifiers 0..31 and the four full words
7fffffff, 80000000, ff112233 and ffffffff for every ID. Independent generation
matched all RGB bytes and metadata in the 324 resources. The prior 3,882 hashed
terrain/table/reference files remained byte-identical; saves and display settings
matched their backups after restoration. Local evidence is under
`build/procedural-capture`. This sample does not exhaust the full key space.
