# Configures and builds the SentinelForge desktop GUI (Qt).
# Usage (from repository root):
#   .\scripts\build-gui.ps1
# Optional:
#   $env:CMAKE_PREFIX_PATH = "C:\Qt\6.7.3\msvc2019_64"

. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$guiDir = Join-Path $repoRoot "gui"
$buildDir = Join-Path $guiDir "build"

if (-not $env:CMAKE_PREFIX_PATH) {
    $defaultQt = "C:\Qt\6.7.3\msvc2019_64"
    if (Test-Path $defaultQt) {
        $env:CMAKE_PREFIX_PATH = $defaultQt
    }
}

Write-Host "Checking CMake..."
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakeCommand) {
    Write-Error "CMake was not found on PATH."
    exit 1
}

if (-not $env:CMAKE_PREFIX_PATH) {
    Write-Error "CMAKE_PREFIX_PATH is not set. Install Qt 6 and point CMAKE_PREFIX_PATH at the kit (e.g. C:\Qt\6.7.3\msvc2019_64)."
    exit 1
}

Write-Host "Configuring GUI (CMAKE_PREFIX_PATH=$env:CMAKE_PREFIX_PATH)..."
cmake -S $guiDir -B $buildDir -DCMAKE_PREFIX_PATH="$env:CMAKE_PREFIX_PATH"
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}

Write-Host "Building desktop console..."
cmake --build $buildDir --config Debug
if ($LASTEXITCODE -ne 0) {
    Write-Error "GUI build failed with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}

Write-Host "Build succeeded."
Write-Host "Launch: $buildDir\bin\Debug\sentinelforge_desktop.exe  (or .\scripts\run-gui.ps1)"
exit 0
