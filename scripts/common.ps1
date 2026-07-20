# Shared helpers for build.ps1, run.ps1, and test.ps1.
# Dot-sourced by each script: . "$PSScriptRoot\common.ps1"

function Get-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Get-CollectorExePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RepoRoot
    )
    return Join-Path $RepoRoot "collector-cpp\build\Debug\collector.exe"
}
