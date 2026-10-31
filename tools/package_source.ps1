# Prepare a separate source-review tree. Never clean or publish the developer's checkout.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$files = [System.Collections.Generic.List[System.IO.FileInfo]]::new()
foreach ($name in @('.gitignore', '.gitattributes', 'CMakeLists.txt', 'build.cmd', 'README.md',
                     'PUBLICATION.md', 'THIRD_PARTY_NOTICES.md', 'LICENSE')) {
    $path = Join-Path $sourceRoot $name
    if (Test-Path -LiteralPath $path -PathType Leaf) { $files.Add((Get-Item -LiteralPath $path)) }
}
# .inc contains source tables needed by the engine, not local game exports.
$extensions = @('.cpp', '.c', '.h', '.hpp', '.inc', '.vert', '.frag', '.glsl', '.cmd', '.ps1', '.py', '.md')
foreach ($directory in @('src', 'include', 'shaders', 'tests', 'setup', 'examples', 'tools')) {
    Get-ChildItem -LiteralPath (Join-Path $sourceRoot $directory) -File -Recurse |
        Where-Object { $_.Extension -in $extensions } | ForEach-Object { $files.Add($_) }
}
Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'docs') -File -Recurse -Filter '*.md' |
    ForEach-Object { $files.Add($_) }
Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'third_party/smaa') -File |
    Where-Object { $_.Name -in @('SMAA.hlsl', 'AreaTex.h', 'SearchTex.h', 'LICENSE.txt') } |
    ForEach-Object { $files.Add($_) }
Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'config') -File -Filter '*.ini' |
    ForEach-Object { $files.Add($_) }
Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'licenses') -File |
    Where-Object { $_.Extension -in @('.txt', '.md') } | ForEach-Object { $files.Add($_) }
# The project's MIT-licensed quest art is the only permitted bitmap. No game art.
foreach ($name in @('assets/OverworldEvents/quest-icons.png', 'assets/OverworldEvents/LICENSE')) {
    $files.Add((Get-Item -LiteralPath (Join-Path $sourceRoot $name)))
}

$buildDirectory = Join-Path $sourceRoot 'build'
[System.IO.Directory]::CreateDirectory($buildDirectory) | Out-Null
$label = 'CubeVulkan-upload-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$stagingDirectory = Join-Path $buildDirectory $label
if (Test-Path -LiteralPath $stagingDirectory) { throw 'Review directory already exists; rerun to create a fresh one.' }
[System.IO.Directory]::CreateDirectory($stagingDirectory) | Out-Null
$manifest = [System.Collections.Generic.List[string]]::new()
foreach ($file in ($files | Sort-Object FullName -Unique)) {
    if (($file.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Source allowlist contains a link: $($file.FullName)"
    }
    $relative = $file.FullName.Substring($sourceRoot.Length + 1).Replace('\', '/')
    if ($relative -match '(^|/)(build[^/]*|Extract|Save|Saves|reference|references|scratchpad|vendor|\.git)/' -or
        $relative -match '\.(exe|dll|obj|lib|pdb|cub|cwz|rec|cwa|cws|cwm|cwp|db|bin|dat|plx|log)$') {
        throw "Private or generated file entered the source allowlist: $relative"
    }
    $destination = Join-Path $stagingDirectory $relative
    [System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($destination)) | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $destination
    $hash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
    $manifest.Add("$hash  $relative")
}
[System.IO.File]::WriteAllText((Join-Path $stagingDirectory 'SOURCE-MANIFEST.sha256'),
                              ($manifest -join "`n") + "`n", [System.Text.UTF8Encoding]::new($false))
$archivePath = Join-Path $buildDirectory 'CubeVulkan-source.zip'
$temporaryArchive = Join-Path $buildDirectory ($label + '.zip')
[System.IO.Compression.ZipFile]::CreateFromDirectory($stagingDirectory, $temporaryArchive,
    [System.IO.Compression.CompressionLevel]::Optimal, $false)
Move-Item -LiteralPath $temporaryArchive -Destination $archivePath -Force
Write-Output "Review tree: $stagingDirectory"
Write-Output "Source archive: $archivePath ($($manifest.Count) files plus manifest)"
if (!(Test-Path -LiteralPath (Join-Path $stagingDirectory 'LICENSE'))) {
    Write-Warning 'Project-wide source license is not selected. Read PUBLICATION.md before uploading.'
}
