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
