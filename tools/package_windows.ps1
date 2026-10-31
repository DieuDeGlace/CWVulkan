param(
    [string]$Version = 'development',
    [string]$BuildDirectory = 'build',
    [string]$SourceRevision = ''
)
# Local packaging only. Publishing a GitHub release is a separate, deliberate step.
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$') { throw 'Invalid package version.' }
if ($SourceRevision -and $SourceRevision -notmatch '^[0-9a-f]{7,40}$') { throw 'Invalid source revision.' }
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if (![IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory = Join-Path $sourceRoot $BuildDirectory }
$buildRoot = (Resolve-Path -LiteralPath $BuildDirectory).Path

function Assert-X64Image([string]$Path, [bool]$IsDll) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Build output is missing: $Path" }
    $reader = [IO.BinaryReader]::new([IO.File]::OpenRead($Path))
    try {
        if ($reader.ReadUInt16() -ne 0x5a4d) { throw "Missing PE image header: $Path" }
        $reader.BaseStream.Position = 0x3c
        $offset = $reader.ReadUInt32()
        if ($offset -gt $reader.BaseStream.Length - 26) { throw "Truncated PE image: $Path" }
        $reader.BaseStream.Position = $offset
        if ($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664) {
            throw "Not an x64 Windows image: $Path"
        }
        $reader.BaseStream.Position = $offset + 22
        $dll = ($reader.ReadUInt16() -band 0x2000) -ne 0
        if ($dll -ne $IsDll -or $reader.ReadUInt16() -ne 0x20b) { throw "Unexpected PE image type: $Path" }
    } finally { $reader.Dispose() }
}

$programs = @('CubeVulkan.exe', 'CubeVulkanSetup.exe', 'CubeVulkanDataCheck.exe')
foreach ($name in $programs) { Assert-X64Image (Join-Path $buildRoot $name) $false }
$extractor = Join-Path $buildRoot 'tools/ZoneExtractor.dll'
Assert-X64Image $extractor $true
$sourceArchive = Join-Path $sourceRoot 'build/CubeVulkan-source.zip'
if (!(Test-Path -LiteralPath $sourceArchive)) { throw 'Run tools/package_source.ps1 first.' }

$label = 'CubeVulkan-windows-x64-' + $Version + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$releaseRoot = Join-Path $sourceRoot 'build/releases'
$stage = Join-Path $releaseRoot $label
if (Test-Path -LiteralPath $stage) { throw 'Package directory already exists.' }
$runtime = Join-Path $stage 'CubeVulkan'
[IO.Directory]::CreateDirectory((Join-Path $runtime 'tools')) | Out-Null
foreach ($name in $programs) { Copy-Item -LiteralPath (Join-Path $buildRoot $name) -Destination $runtime }
Copy-Item -LiteralPath $extractor -Destination (Join-Path $runtime 'tools/ZoneExtractor.dll')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'config/CubeVulkan.ini') -Destination $runtime
foreach ($name in @('LICENSE', 'THIRD_PARTY_NOTICES.md', 'PUBLICATION.md')) {
    Copy-Item -LiteralPath (Join-Path $sourceRoot $name) -Destination $runtime
}
Copy-Item -LiteralPath (Join-Path $sourceRoot 'licenses') -Destination $runtime -Recurse
[IO.Directory]::CreateDirectory((Join-Path $runtime 'assets/OverworldEvents')) | Out-Null
foreach ($name in @('quest-icons.png', 'LICENSE')) {
    Copy-Item -LiteralPath (Join-Path $sourceRoot ('assets/OverworldEvents/' + $name)) -Destination (Join-Path $runtime 'assets/OverworldEvents')
}
[IO.Directory]::CreateDirectory((Join-Path $runtime 'third_party/smaa')) | Out-Null
Copy-Item -LiteralPath (Join-Path $sourceRoot 'third_party/smaa/LICENSE.txt') -Destination (Join-Path $runtime 'third_party/smaa')

# Keep the engine in its own subfolder: its Save directory must never be the game's Save.
$launcher = "@echo off`r`ncd /d `"%~dp0CubeVulkan`"`r`nstart `"`" `"%~dp0CubeVulkan\CubeVulkanSetup.exe`"`r`n"
[IO.File]::WriteAllText((Join-Path $stage 'Play-CubeVulkan.cmd'), $launcher, [Text.Encoding]::ASCII)
$instructions = @"
CubeVulkan $Version - development package
Source revision: $SourceRevision

Unpack these files into your Steam Cube World folder, keeping the CubeVulkan subfolder.
Run Play-CubeVulkan.cmd. The launcher checks your installation and opens setup for missing data.
Keep Steam online on an account that owns Cube World. A Vulkan 1.3 GPU/driver is required.

The first run needs local data extraction from your own game. Compatible CubeModLoader
must already be installed; it is a separate dependency and is not included here.
The extractor is included at CubeVulkan/tools/ZoneExtractor.dll; Setup installs it for
the extraction run and restores the previous mods and saves after the game closes.
Back up your saves and follow the setup window's instructions.

CubeVulkan keeps its saves in CubeVulkan/Save, separate from the original game's Save.
Do not move the executables out of the CubeVulkan subfolder.
Enemy settings are in CubeVulkan/CubeVulkan.ini under [Creatures]:
  DeathMode=Timed       (or Instance, or Permanent)
  RespawnSeconds=300
Instance deaths reset when the spawn zone unloads for all players. Restart after edits.

This is a development package, not a finished 1:1 release. See the source repository's
docs/roadmap.md for remaining gameplay, visual and multiplayer validation.
Project source: https://github.com/DieuDeGlace/CWVulkan
The matching source-review ZIP is provided alongside this package. Original code is
GPL-3.0-only; third-party notices are in CubeVulkan/THIRD_PARTY_NOTICES.md.
No game assets, saves, Steam runtime or loader binaries are included.
"@
[IO.File]::WriteAllText((Join-Path $stage 'README.txt'), $instructions, [Text.UTF8Encoding]::new($false))

$hashes = foreach ($file in (Get-ChildItem -LiteralPath $stage -File -Recurse | Sort-Object FullName)) {
    $name = $file.FullName.Substring($stage.Length + 1).Replace('\', '/')
    if ($name -match '(^|/)(Extract|Save|Saves|Mods|sprites|music|sounds)/|\.(cub|cwz|rec|cwa|cwm|cwp|db|dat|plx|pdb|obj|lib|log)$' -or
        ($file.Extension -eq '.dll' -and $name -ne 'CubeVulkan/tools/ZoneExtractor.dll') -or
        ($file.Extension -eq '.png' -and $name -ne 'CubeVulkan/assets/OverworldEvents/quest-icons.png')) {
        throw "Unexpected release payload: $name"
    }
    (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant() + '  ' + $name
}
[IO.File]::WriteAllText((Join-Path $stage 'PACKAGE-MANIFEST.sha256'), ($hashes -join "`n") + "`n", [Text.UTF8Encoding]::new($false))
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = Join-Path $releaseRoot ($label + '.zip')
[IO.Compression.ZipFile]::CreateFromDirectory($stage, $archive)
$sourceCopy = Join-Path $releaseRoot ($label + '-source.zip')
Copy-Item -LiteralPath $sourceArchive -Destination $sourceCopy
Write-Output "Windows package: $archive"
Write-Output "Corresponding source: $sourceCopy"
