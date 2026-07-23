# SentinelForge

SentinelForge is a Detection Regression Testing Platform for security detections.

## Goal

Given attack telemetry and detection rules, SentinelForge validates whether detections still work after changes.

## Technologies

- C++17
- Python 3
- Docker
- Kubernetes
- Git

## Development

### Build

```
.\scripts\build.ps1
```

### Run

```
.\scripts\run.ps1
```

### Test

```
.\scripts\test.ps1
```

### Performance profiling

Each collector run ends with a structured **Performance Summary** (via the
Logger) showing millisecond timings for configuration, rule loading/validation,
event parsing, detection, report generation, and total runtime. Timing is owned
by `PerformanceProfiler`; application code only marks stage boundaries.

### JSON detection export

When `json_export.enabled` is true (default), each successful detection run
also writes `detections.json` (path configurable). The console report is
unchanged; JSON export is an additional output owned by `JsonExporter`.

### Sigma rules

When `sigma.enabled` is true, YAML rules under `sigma-rules/` are parsed,
validated, translated into the internal `Rule` model, and evaluated alongside
native JSON rules. Phase 1 supports a focused subset (`title`, `detection`,
`selection` with `process_name` / `command_line|contains`, `condition: selection`,
`level`, `tags`). See `collector-cpp/README.md` for limitations and roadmap.

### Live event monitoring

When `monitoring.enabled` is true (default), the collector becomes a continuous
service:

1. Load configuration and rules (native + Sigma) once.
2. Poll `events/incoming/` for new `*.json` telemetry files.
3. For each file: parse → normalize → detect → report → JSON export → move to `events/processed/`.
4. Repeat until CTRL+C (graceful shutdown finishes the current event).

`DetectionEngine` evaluates a source-agnostic `NormalizedEvent` produced by
`EventNormalizer`, so matching logic does not depend on JSON-specific types.

### Event correlation

When `correlation.enabled` is true, each normalized event is also evaluated by
`CorrelationEngine` after signature detection. The engine maintains a bounded
rolling history and runs pluggable behavioral rules (example: Office →
PowerShell within 60 seconds). Alerts are logged and included in JSON export
under `correlation_alerts`.

```
events/
    incoming/     # drop new event JSON files here
    processed/    # archived after successful processing (never deleted)
    failed/       # archived after parse/read failures (never deleted)
```

Example:

```
copy sample-logs\process_create.json events\incoming\evt1.json
.\scripts\run.ps1
```

Malformed events are moved to `events/failed/` and monitoring continues.
Set `"monitoring": { "enabled": false }` to restore the previous one-shot
sample-file mode.
