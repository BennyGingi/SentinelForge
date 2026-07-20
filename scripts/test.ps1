# Smoke test: builds, runs, and verifies expected collector output.
# Usage (from repository root): .\scripts\test.ps1

. "$PSScriptRoot\common.ps1"

$buildScript = Join-Path $PSScriptRoot "build.ps1"
$runScript = Join-Path $PSScriptRoot "run.ps1"

& $buildScript
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL"
    Write-Error "build.ps1 failed with exit code $LASTEXITCODE."
    exit 1
}

$output = & $runScript | Out-String
$runExitCode = $LASTEXITCODE
if ($runExitCode -ne 0) {
    Write-Host "FAIL"
    Write-Error "run.ps1 failed with exit code $runExitCode."
    exit 1
}

$requiredStrings = @(
    "SentinelForge Collector started",
    "Rules loaded:",
    "Rules evaluated:",
    "Matches:"
)

$missingStrings = @()
foreach ($text in $requiredStrings) {
    if ($output -notmatch [regex]::Escape($text)) {
        $missingStrings += $text
    }
}

if ($missingStrings.Count -eq 0) {
    Write-Host "PASS"
    exit 0
}

Write-Host "FAIL"
foreach ($text in $missingStrings) {
    Write-Error "Missing expected output: '$text'"
}
exit 1
