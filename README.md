# SentinelForge

[![CI](https://github.com/BennyGingi/SentinelForge/actions/workflows/ci.yml/badge.svg)](https://github.com/BennyGingi/SentinelForge/actions/workflows/ci.yml)

When a detection rule changes — a tweak to a Sigma translation, a stricter
match condition, a correlation-window adjustment — how do you know you
didn't just silently stop catching a technique you used to catch? Most
detection engineering has no answer to that beyond "wait for the next real
incident and hope." SentinelForge is built around having one: real
attack-shaped telemetry, fed through the real pipeline, checked against what
must still fire, on every change.

## What's here

**Collector** (`collector-cpp/`, C++20) — parses telemetry, normalizes it,
evaluates it against native and Sigma-translated detection rules, and runs a
behavioral correlation engine on top. Builds and tests independently of
everything else. Configuration, live event monitoring, JSON export, Sigma's
supported subset, and performance profiling are documented in
[`collector-cpp/README.md`](collector-cpp/README.md).

**Regression harness** (`collector-cpp/regression/`) — the actual point of
this repo. Links the collector's core library directly (no shelling out),
feeds declarative attack scenarios through the real pipeline, and diffs what
fired against what a scenario declares required. See
[`docs/architecture/regression-testing.md`](docs/architecture/regression-testing.md).

**Desktop console** (`gui/`, Qt 6) — a live operator view: detection table,
correlation feed, MITRE technique presentation, threat scoring, search,
pause/resume, inspector. **It currently runs entirely against
`MockTelemetrySource`** — a `CollectorTelemetrySource` that reads real
collector output does not exist yet; that's the next milestone (see
[`docs/BACKLOG.md`](docs/BACKLOG.md)). Nothing in the GUI reads live
collector data today.

## A scenario, end to end

Scenarios are plain YAML under `sample-attacks/` — one file, no code:

```yaml
name: encoded_powershell
description: >
  A user opens a weaponized Word document; the macro spawns PowerShell with
  a Base64-encoded command line — a standard macro-dropper execution
  pattern.
mitre_techniques:
  - T1059.001

events:
  - timestamp: "2026-07-24T09:12:03Z"
    hostname: WORKSTATION-14
    process_name: WINWORD.EXE
    parent_process: explorer.exe
    command_line: "\"...\\WINWORD.EXE\" /n \"...\\invoice_47281.docm\""
  - timestamp: "2026-07-24T09:12:07Z"
    hostname: WORKSTATION-14
    process_name: powershell.exe
    parent_process: WINWORD.EXE
    command_line: "powershell.exe -NoP -W Hidden -enc SQBFAFgA..."

expected_detections:
  - "Suspicious PowerShell"
```

Running the harness (`collector-cpp/build/regression/Debug/regression_runner.exe`)
against all five checked-in scenarios, real output:

```
Loaded 6 rules from rules/ and sigma-rules/
Running 5 scenario(s) from sample-attacks/

[PASS] credential_dumping
[PASS] encoded_powershell
    UNEXPECTED (fired, not declared): Office application launches PowerShell
    UNEXPECTED (fired, not declared): Suspicious Encoded PowerShell
[PASS] lolbin_download
[PASS] persistence_chain
    UNEXPECTED (fired, not declared): Suspicious Encoded PowerShell
[PASS] service_persistence

5 passed, 0 known gaps, 0 failed
```

Three states, not two: **PASS**, **FAIL** (expected but missing — the
regression case, always fails), and **GAP** (`known_gap: true` — nothing
fired, documented not broken, doesn't fail the build unless `--fail-on-gap`).
`lolbin_download` and `service_persistence` were both `GAP` until
`sigma-rules/certutil_lolbin_download.yml` and
`sigma-rules/sc_service_creation.yml` were written — closing a known gap is
reported loudly as a promotion rather than silently blending into an
ordinary pass, so the scenario file doesn't rot with a stale `known_gap`
once someone writes the rule. Full writeup, including the exact GAP-CLOSED
promotion output:
[`docs/architecture/regression-testing.md`](docs/architecture/regression-testing.md).

## Test counts

- **79** collector unit tests (`collector-cpp/tests`, GoogleTest)
- **31** GUI unit tests (`gui/tests`, GoogleTest — pure logic only: MITRE
  technique-ID validation, timestamp formatting, model batching/eviction)
- **5** regression scenarios (`sample-attacks/`)

All three run in CI on every push and PR to `main` — see the badge above.

## Quick start

Collector:

```
.\scripts\build.ps1
.\scripts\test.ps1
```

Regression harness (after building the collector):

```
.\collector-cpp\build\regression\Debug\regression_runner.exe
```

Desktop console (requires Qt 6 — set `CMAKE_PREFIX_PATH`, e.g.
`C:\Qt\6.7.3\msvc2019_64`):

```
.\scripts\build-gui.ps1
.\scripts\run-gui.ps1
```

## Technologies

- C++20 (collector and desktop console)
- Qt 6 (desktop console)
- Docker (collector container build — see `docker/collector.Dockerfile`)
- Python 3 — **planned, not yet implemented.** `detector-python/` is
  reserved for it; there's no Python code in the repo today.
- Kubernetes — **planned, not yet implemented.** `k8s/` is reserved for it;
  there are no manifests in the repo today.

## Roadmap

What's open, what's next, what's deliberately deferred:
[`docs/BACKLOG.md`](docs/BACKLOG.md). Long-range capability map:
[`docs/roadmap/roadmap.md`](docs/roadmap/roadmap.md). Full architecture and
design contract: [`docs/BRIEF.md`](docs/BRIEF.md).
