# Launches the SentinelForge desktop console.
. "$PSScriptRoot\common.ps1"

$repoRoot = Get-RepoRoot
$exeCandidates = @(
    (Join-Path $repoRoot "gui\build\bin\Debug\sentinelforge_desktop.exe"),
    (Join-Path $repoRoot "gui\build\bin\sentinelforge_desktop.exe"),
    (Join-Path $repoRoot "gui\build\Debug\sentinelforge_desktop.exe")
)

$exe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exe) {
    Write-Error "Desktop executable not found. Run .\scripts\build-gui.ps1 first."
    exit 1
}

Write-Host "Starting $exe"
Start-Process $exe
exit 0
