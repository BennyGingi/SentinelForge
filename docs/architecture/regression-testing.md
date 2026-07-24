# Detection Regression Testing

Parent brief: [`docs/BRIEF.md`](../BRIEF.md). Root [`README.md`](../../README.md) names this the project's actual purpose: *"Given attack telemetry and detection rules, SentinelForge validates whether detections still work after changes."* This document is the harness that does that.

---

## What it answers

Rules and correlation logic change over time. A rule tweak, a Sigma translation fix, or a correlation-window adjustment can silently stop a real technique from firing. Unit tests exercise the pipeline's *mechanics* (parsing, validation, matching semantics); they don't exercise "does this specific attack still get caught." The regression harness closes that gap: real-shaped attack telemetry goes in, and the harness asserts specific detections come out.

## How it works

`collector-cpp/regression/regression_runner` links `collector_core` directly — the same static library the production `collector` executable links — and drives the pipeline in-process:

```
scenario events → EventNormalizer::NormalizeJson → DetectionEngine::Evaluate → CorrelationEngine::Process
```

No shelling out to the `collector` binary, no temp files. Each scenario gets a fresh `CorrelationEngine`, so scenario results never depend on run order.

Run it:

```
./collector-cpp/build/regression/Debug/regression_runner.exe
```

Optional flags: `--scenarios <dir>`, `--rules <dir>`, `--sigma-rules <dir>` (default to `sample-attacks/`, `rules/`, `sigma-rules/`), `--strict` (see below).

## Scenario format

One YAML file per scenario, under `sample-attacks/`. Adding a scenario means writing one file — no code.

```yaml
name: encoded_powershell
description: >
  A user opens a weaponized Word document; the macro spawns PowerShell with
  a Base64-encoded command line.
mitre_techniques:
  - T1059.001

events:
  - timestamp: "2026-07-24T09:12:03Z"
    hostname: WORKSTATION-14
    username: "CORP\\amiller"
    process_name: WINWORD.EXE
    parent_process: explorer.exe
    command_line: "\"C:\\...\\WINWORD.EXE\" /n \"...\""
  - timestamp: "2026-07-24T09:12:07Z"
    hostname: WORKSTATION-14
    username: "CORP\\amiller"
    process_name: powershell.exe
    parent_process: WINWORD.EXE
    command_line: "powershell.exe -NoP -W Hidden -enc SQBFAFgA..."

expected_detections:
  - "Suspicious PowerShell"
```

| Key | Required | Notes |
|---|---|---|
| `name` | yes | Scenario identifier, shown in the report. |
| `description` | no | Free text. |
| `mitre_techniques` | no | Documentation only — not checked against fired detections. |
| `events` | yes, non-empty | Ordered list, fed through the pipeline in order. |
| `expected_detections` | yes, non-empty | See below. |

### Event shape

Each event under `events:` uses the exact snake_case keys `EventNormalizer::NormalizeJson` consumes (`collector-cpp/src/EventNormalizer.cpp`): `timestamp`, `hostname`, `username`, `event_type`, `process_name`, `parent_process`, `command_line`, `file_path`, `hash`, `source_ip`, `destination_ip`, `destination_port`, `severity`, `provider`. All optional — a field the normalizer doesn't recognize is just carried through and ignored, an omitted one comes through empty, matching normal collector behavior for a telemetry source that doesn't populate every field.

### What "detection ID" means here

Neither `Rule` nor `DetectionResult` carries a separate `id` field today — only `RuleName()` (e.g. `"Suspicious PowerShell"`). Native `rules/*.json` files don't declare an `id` either. So `expected_detections` entries are matched against whichever of these actually fired:

- **Native rule detections** — `Rule::RuleName()`, e.g. `"Suspicious PowerShell"`, `"Mimikatz Execution"`.
- **Correlation alerts** — `CorrelationAlert::Title()`, e.g. `"Office application launches PowerShell"`. `CorrelationRule` does have a stable `Id()` (`"office_launches_powershell"`), but `CorrelationAlert` doesn't carry that ID back through, only the human-readable title — so the title is what scenarios name.

This was a deliberate choice to avoid touching the collector's rule/detection schema for the harness's sake. If a real need for a proper `id` field emerges later, that's a collector-cpp change, not a harness one.

## The missing-vs-unexpected distinction — and why it exists

The diff between a scenario's `expected_detections` and what actually fired has two very different failure modes, and the harness treats them differently on purpose:

- **Expected but MISSING → always a failure**, `--strict` or not. This is *the* regression case: a detection that used to fire no longer does. Exit code is non-zero, CI goes red, no flag disables this.
- **Fired but NOT expected → a warning by default**, promoted to a failure only with `--strict`. An unlisted detection firing isn't necessarily wrong — it's often a *second* legitimate rule catching the same technique (see `encoded_powershell.yaml`, where the Sigma-translated `"Suspicious Encoded PowerShell"` rule and the correlation engine's `office_launches_powershell` both fire alongside the required native rule), or a genuinely new true positive from a rule added since the scenario was written. Failing the build on *more* coverage than expected would punish adding detections and train people to either over-specify `expected_detections` or stop trusting the harness. It's still surfaced — every UNEXPECTED line prints — so a human can judge whether it's a new legitimate signal or a false positive worth investigating, without it blocking the pipeline.

Run with `--strict` when you want zero-tolerance for coverage drift (e.g. right before a release), and without it for normal day-to-day rule work.

## Report format

```
[PASS] credential_dumping
[FAIL] encoded_powershell
    UNEXPECTED (fired, not declared): Office application launches PowerShell
    UNEXPECTED (fired, not declared): Suspicious Encoded PowerShell
```

(That specific example is a `--strict` run — without `--strict` those two UNEXPECTED lines don't flip the scenario to FAIL.) Exit code is non-zero if any scenario fails, for CI.

## Seed scenarios

| File | Technique(s) | Proves |
|---|---|---|
| `encoded_powershell.yaml` | T1059.001 | Native rule detection; also demonstrates the unexpected-vs-missing distinction live (Sigma duplicate + correlation both fire as warnings). |
| `credential_dumping.yaml` | T1003.001 | Native rule detection, clean pass, no incidental extras. |
| `persistence_chain.yaml` | T1059.001, T1547.001, T1204.002 | Multi-event chain that trips the **correlation engine** (`office_launches_powershell`), not just a single per-event rule — the harness covers cross-event behavioral detection, not only `DetectionEngine`. |

## Adding a scenario

1. Write a new `sample-attacks/your_scenario.yaml` with realistic events (real process names, real technique IDs — no placeholder strings).
2. Run `regression_runner` locally, confirm your expected rule/alert names actually appear as fired (copy them from the UNEXPECTED lines on a first run rather than guessing spellings).
3. Commit. CI picks up every `*.yaml`/`*.yml` under `sample-attacks/` automatically — no registration step, no code change.
