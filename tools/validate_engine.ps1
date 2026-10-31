param(
    [switch]$SkipGpu,
    [string]$LogDirectory
)

$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$testProgram = Join-Path $sourceRoot 'build/EngineTests.exe'
if (!(Test-Path -LiteralPath $testProgram -PathType Leaf)) {
    throw 'Build EngineTests first with build.cmd --target EngineTests.'
}
if (!$LogDirectory) {
    $LogDirectory = Join-Path $sourceRoot ('build/validation-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
}
$null = New-Item -ItemType Directory -Path $LogDirectory -Force
$LogDirectory = (Resolve-Path -LiteralPath $LogDirectory).Path

Push-Location $sourceRoot
try {
    $cpuLog = Join-Path $LogDirectory 'engine-tests.log'
    & $testProgram *> $cpuLog
    if ($LASTEXITCODE -ne 0) { throw "Engine tests failed; read $cpuLog" }
    Write-Output "CPU/owner-data engine suite passed: $cpuLog"

    if ($SkipGpu) {
        Write-Output 'SKIP Vulkan image checks: requested with -SkipGpu.'
    } else {
        # steamvoxel is a CPU data test. steamshader creates the Vulkan device,
        # renders captures and compares the resulting image pixels.
        $gpuLog = Join-Path $LogDirectory 'vulkan-images.log'
        & $testProgram steamshader *> $gpuLog
        if ($LASTEXITCODE -ne 0) { throw "Vulkan image tests failed; read $gpuLog" }
        $passed = @(Select-String -LiteralPath $gpuLog -Pattern '^Steam shader .* PASS$')
        if (!$passed.Count) { throw "No Vulkan image results were produced; read $gpuLog" }
        Write-Output "Vulkan image checks passed: $($passed.Count); $gpuLog"
        $guiLog = Join-Path $LogDirectory 'gui-item-images.log'
        & $testProgram guirender *> $guiLog
        if ($LASTEXITCODE -ne 0) { throw "GUI item image tests failed; read $guiLog" }
        $guiPassed = @(Select-String -LiteralPath $guiLog -Pattern '^GUI WORLD .* PASS$')
        if (!$guiPassed.Count) { throw "No GUI item image results were produced; read $guiLog" }
        Write-Output "GUI item image checks passed: $($guiPassed.Count); $guiLog"
        $waterLog = Join-Path $LogDirectory 'shared-water-images.log'
        & $testProgram sharedwater *> $waterLog
        if ($LASTEXITCODE -ne 0) { throw "Shared water image tests failed; read $waterLog" }
        Write-Output "Shared water image checks passed: $waterLog"
        $alphaOrderLog = Join-Path $LogDirectory 'alpha-scenery-images.log'
        & $testProgram alphasceneryshader *> $alphaOrderLog
        if ($LASTEXITCODE -ne 0) { throw "Alpha scenery image tests failed; read $alphaOrderLog" }
        Write-Output "Alpha scenery image checks passed: $alphaOrderLog"
    }
} finally {
    Pop-Location
}
