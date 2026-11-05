param([string]$BuildDirectory = 'build')
$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if (![IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory = Join-Path $sourceRoot $BuildDirectory }
$buildRoot = (Resolve-Path -LiteralPath $BuildDirectory).Path
$cache = Get-Content -LiteralPath (Join-Path $buildRoot 'CMakeCache.txt')
$entry = @($cache | Where-Object { $_ -like 'CMAKE_MAKE_PROGRAM:FILEPATH=*' })
if ($entry.Count -ne 1) { throw 'Expected a configured Ninja build.' }
$ninja = $entry[0].Substring($entry[0].IndexOf('=') + 1)
if (!(Test-Path -LiteralPath $ninja -PathType Leaf) -or [IO.Path]::GetFileName($ninja) -ne 'ninja.exe') {
    throw 'Configured build tool is not Ninja.'
}
$commands = @(& $ninja -C $buildRoot -t commands ZoneExtractor)
if ($LASTEXITCODE -ne 0) { throw 'Unable to inspect extractor build commands.' }
$dependencies = [Collections.Generic.List[string]]::new()
foreach ($object in @('CMakeFiles/ZoneExtractor.dir/tools/ZoneExtractor/ZoneExtractor.cpp.obj',
                      'CMakeFiles/ZoneExtractor.dir/src/ProceduralItemData.cpp.obj')) {
    $records = @(& $ninja -C $buildRoot -t deps $object)
    if ($LASTEXITCODE -ne 0 -or !($records -match '#deps\s+\d+.*\(VALID\)')) {
        throw "Missing or stale dependency record: $object"
    }
    foreach ($line in $records) { $dependencies.Add($line) }
}
$forbidden = '(?i)(cwsdk\.h|cwmods|SteamCubeWorldMods|/permissive(?:\s|$))'
if (@($commands + $dependencies.ToArray() | Where-Object { $_ -match $forbidden }).Count) {
    throw 'Extractor build still references SDK headers, include paths or its former permissive option.'
}
$commands | Set-Content -LiteralPath (Join-Path $buildRoot 'extractor-no-sdk-commands.log') -Encoding utf8
$dependencies | Set-Content -LiteralPath (Join-Path $buildRoot 'extractor-no-sdk-dependencies.log') -Encoding utf8
Write-Output 'Extractor command/dependency audit PASS: no CWSDK path, header or former permissive option.'
