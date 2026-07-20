# Configures and builds the collector-cpp project.
# Usage (from repository root): .\scripts\build.ps1

. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$collectorDir = Join-Path $repoRoot "collector-cpp"
$buildDir = Join-Path $collectorDir "build"
$cacheFile = Join-Path $buildDir "CMakeCache.txt"

Write-Host "Checking CMake..."
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakeCommand) {
    Write-Error "CMake was not found on PATH. Install CMake (https://cmake.org/download/) and try again."
    exit 1
}

if (-not (Test-Path $buildDir)) {
    Write-Host "Creating build directory..."
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

if (-not (Test-Path $cacheFile)) {
    Write-Host "Configuring project..."
    cmake -S $collectorDir -B $buildDir
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed with exit code $LASTEXITCODE."
        exit $LASTEXITCODE
    }
}

Write-Host "Building collector..."
cmake --build $buildDir --config Debug
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}

Write-Host "Build succeeded."
exit 0
