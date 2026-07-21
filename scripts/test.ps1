# Builds the project and runs the GoogleTest unit suite via CTest.
# Reports PASS only if every unit test succeeds.
# Usage (from repository root): .\scripts\test.ps1

. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$buildScript = Join-Path $PSScriptRoot "build.ps1"
$buildDir = Join-Path $repoRoot "collector-cpp\build"

& $buildScript
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL"
    Write-Error "build.ps1 failed with exit code $LASTEXITCODE."
    exit 1
}

Write-Host "Running unit tests (ctest)..."
ctest --test-dir $buildDir -C Debug --output-on-failure
$testExitCode = $LASTEXITCODE
if ($testExitCode -ne 0) {
    Write-Host "FAIL"
    Write-Error "Unit tests failed with exit code $testExitCode."
    exit 1
}

Write-Host "PASS"
exit 0
