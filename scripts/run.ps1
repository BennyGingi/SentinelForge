# Runs the built collector executable.
# Usage (from repository root): .\scripts\run.ps1

. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$exePath = Get-CollectorExePath -RepoRoot $repoRoot

if (-not (Test-Path $exePath)) {
    Write-Error "collector.exe not found at '$exePath'. Run .\scripts\build.ps1 first."
    exit 1
}

Write-Host "Running SentinelForge..."
& $exePath
$exitCode = $LASTEXITCODE

Write-Host "Execution completed."
exit $exitCode
