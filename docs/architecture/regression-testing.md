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

Optional flags: `--scenarios <dir>`, `--rules <dir>`, `--sigma-rules <dir>` (default to `sample-attacks/`, `rules/`, `sigma-rules/`), `--strict`, `--fail-on-gap` (see below).

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
| `expected_detections` | yes (key must exist; `[]` is valid) | See below. |
| `known_gap` | no, default `false` | Declares that nothing is expected to fire — a documented coverage gap, not a forgotten expectation. |
| `gap_reason` | required whenever `known_gap: true` | Why the gap exists. The file fails to load without one — a gap that isn't explained isn't documented, it's just missing. |

### Event shape

Each event under `events:` uses the exact snake_case keys `EventNormalizer::NormalizeJson` consumes (`collector-cpp/src/EventNormalizer.cpp`): `timestamp`, `hostname`, `username`, `event_type`, `process_name`, `parent_process`, `command_line`, `file_path`, `hash`, `source_ip`, `destination_ip`, `destination_port`, `severity`, `provider`. All optional — a field the normalizer doesn't recognize is just carried through and ignored, an omitted one comes through empty, matching normal collector behavior for a telemetry source that doesn't populate every field.

### What "detection ID" means here

Neither `Rule` nor `DetectionResult` carries a separate `id` field today — only `RuleName()` (e.g. `"Suspicious PowerShell"`). Native `rules/*.json` files don't declare an `id` either. So `expected_detections` entries are matched against whichever of these actually fired:

- **Native rule detections** — `Rule::RuleName()`, e.g. `"Suspicious PowerShell"`, `"Mimikatz Execution"`.
- **Correlation alerts** — `CorrelationAlert::Title()`, e.g. `"Office application launches PowerShell"`. `CorrelationRule` does have a stable `Id()` (`"office_launches_powershell"`), but `CorrelationAlert` doesn't carry that ID back through, only the human-readable title — so the title is what scenarios name.

This was a deliberate choice to avoid touching the collector's rule/detection schema for the harness's sake. If a real need for a proper `id` field emerges later, that's a collector-cpp change, not a harness one.

### Sigma translator coverage — a real limitation, not a style choice

Writing `sigma-rules/certutil_lolbin_download.yml` and `sigma-rules/sc_service_creation.yml` surfaced a concrete gap in `collector-cpp`'s Phase-1 Sigma translator: `detection` supports exactly one map named `selection`, with exactly one `process_name` and one `command_line|contains`/`command_line` — no named/multiple selections, no `and`/`or`/`not`, no `1 of selection*`. `condition` must be the literal string `"selection"`; anything else is rejected outright (confirmed empirically, not just from reading the source — see the rejection log in either rule file's trailing comment). Practically: a Sigma rule needing OR logic across command-line variants or process names has to either collapse onto one genuinely behavioral indicator that covers all variants (what both new rules do) or split into multiple rule files, one selection each. Both rule files document this inline for whoever writes the next one.

## Three states, not two — and why

Every scenario resolves to exactly one of three verdicts. The distinction matters because "missing" and "unexpected" are not the same kind of problem, and a documented gap is not the same kind of thing as either:

- **FAIL — expected but MISSING.** Always a failure, `--strict` or not, `known_gap` or not. This is *the* regression case: a detection that used to fire no longer does. `known_gap` never suppresses this — declaring "nothing is expected" and then explicitly listing something in `expected_detections` that doesn't fire is still a broken promise. No flag disables this.
- **GAP — `known_gap: true` and, as expected, nothing fired.** Documented, not broken. Doesn't fail the build by default — a known, explained absence of coverage is information, not a bug. Promotable to a failure with `--fail-on-gap`, for a stricter mode (e.g. "no new known gaps allowed past this point," or a periodic audit that forces someone to look at the list).
- **PASS — everything expected fired**, the ordinary case. Also covers the promotion case below, since a scenario doing *better* than promised is never a failure.

Layered on top of FAIL/PASS, independent of `known_gap`:

- **Fired but NOT expected → a warning by default**, promoted to a failure only with `--strict`. An unlisted detection firing isn't necessarily wrong — often a *second* legitimate rule catching the same technique (see `encoded_powershell.yaml`, where the Sigma-translated `"Suspicious Encoded PowerShell"` rule and the correlation engine's `office_launches_powershell` both fire alongside the required native rule). Failing the build on *more* coverage than expected would punish adding detections and train people to either over-specify `expected_detections` or stop trusting the harness.

### GAP CLOSED — the promotion case

If a scenario marked `known_gap: true` actually produces a detection — someone wrote the rule and didn't come back to update the scenario — that's **good news reported loudly**, not silently absorbed into an ordinary PASS. This actually happened: `service_persistence.yaml` and `lolbin_download.yaml` were both `known_gap: true` until `sigma-rules/sc_service_creation.yml` and `sigma-rules/certutil_lolbin_download.yml` were written. Real captured output from that exact run, before either scenario file was touched:

```
[PASS] service_persistence
    UNEXPECTED (fired, not declared): Suspicious Service Creation via sc.exe
    GAP CLOSED — now detected: Suspicious Service Creation via sc.exe
    Remove known_gap/gap_reason from this scenario file — the gap it documented ("No rule in
    rules/ or sigma-rules/ matches sc.exe...") is closed.
```

It never fails the build — even under `--fail-on-gap`, which only affects gaps still genuinely open — because penalizing improved coverage would be perverse. The loud message exists specifically so these files don't rot: without it, a closed gap looks identical to an ordinary pass and nobody goes back to delete `known_gap`/`gap_reason`, and the scenario quietly stops documenting anything true. After seeing this, `known_gap`/`gap_reason` were removed from both files and `expected_detections` populated with the new rule titles — the normal end state, not a special case.

Run with `--strict` for zero-tolerance on coverage drift, `--fail-on-gap` when you want every gap resolved or explicitly re-justified (e.g. before a release), and with neither for normal day-to-day rule work.

## Report format

Real output, current state of `sample-attacks/`:

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

Exit code is non-zero only if `failed > 0` — GAP never contributes to that count unless `--fail-on-gap` is set, in which case open gaps become FAIL and count there instead.

## Seed scenarios

| File | Technique(s) | Proves |
|---|---|---|
| `encoded_powershell.yaml` | T1059.001 | Native rule detection; also demonstrates the unexpected-vs-missing distinction live (Sigma duplicate + correlation both fire as warnings). |
| `credential_dumping.yaml` | T1003.001 | Native rule detection, clean pass, no incidental extras. |
| `persistence_chain.yaml` | T1059.001, T1547.001, T1204.002 | Multi-event chain that trips the **correlation engine** (`office_launches_powershell`), not just a single per-event rule — the harness covers cross-event behavioral detection, not only `DetectionEngine`. |
| `service_persistence.yaml` | T1543.003 | Detects `sc.exe create` (`sigma-rules/sc_service_creation.yml`). Was `known_gap: true` until that rule existed — see GAP CLOSED below for the real promotion output. |
| `lolbin_download.yaml` | T1105, T1218 | Detects a URL in a `certutil.exe` command line (`sigma-rules/certutil_lolbin_download.yml`) — behavioral, not a single flag string. Same promotion history as above. |

## Adding a scenario

1. Write a new `sample-attacks/your_scenario.yaml` with realistic events (real process names, real technique IDs — no placeholder strings).
2. Run `regression_runner` locally, confirm your expected rule/alert names actually appear as fired (copy them from the UNEXPECTED lines on a first run rather than guessing spellings).
3. If nothing fires and it should — no rule exists yet — don't write a rule just to make the scenario pass. Set `known_gap: true` and a real `gap_reason` instead. A scenario that documents an actual coverage gap is more useful than one fitted to whatever already exists.
4. Commit. CI picks up every `*.yaml`/`*.yml` under `sample-attacks/` automatically — no registration step, no code change.
