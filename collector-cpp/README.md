# Collector (C++)

Telemetry collector service for SentinelForge. Loads runtime settings from a
central configuration file, ingests a telemetry event, evaluates it against a
directory of detection rules, and prints a report.

## Layout

```
collector-cpp/
├── CMakeLists.txt
├── include/
│   ├── Application.h           # application lifecycle owner; owns Configuration
│   ├── Configuration.h         # immutable, centrally-owned runtime settings
│   ├── Logger.h                # structured logger (levels, destinations, components)
│   ├── PerformanceProfiler.h   # named stage timing and performance summary
│   ├── JsonExporter.h          # JSON detection export
│   ├── SigmaParser.h           # Sigma YAML parse + Phase-1 schema validation
│   ├── SigmaTranslator.h       # SigmaRule -> internal Rule
│   ├── SigmaLoader.h           # Sigma directory loading
│   └── ...                     # Event/Rule parsing, validation, detection, reporting
├── src/
│   ├── main.cpp                # entry point: construct Application, run, return exit code
│   ├── Application.cpp
│   ├── Configuration.cpp
│   ├── Logger.cpp
│   ├── PerformanceProfiler.cpp
│   ├── JsonExporter.cpp
│   ├── SigmaParser.cpp
│   ├── SigmaTranslator.cpp
│   ├── SigmaLoader.cpp
│   └── ...
├── tests/                      # GoogleTest unit suite (collector_tests)
└── README.md
```

## Configuration

Runtime settings live in `config/sentinelforge.json` at the repository root.
No component hardcodes runtime paths; `Application` loads the configuration
once at startup and passes each collaborator only the settings it needs.

| Key                 | Type    | Purpose                                   |
| ------------------- | ------- | ----------------------------------------- |
| `rules_directory`   | string  | Directory scanned for `*.json` rules      |
| `sample_event_file` | string  | Telemetry event to evaluate               |
| `logging`           | object  | Nested logging settings (see below)       |
| `json_export`       | object  | Nested JSON export settings (see below)   |
| `sigma`             | object  | Nested Sigma loading settings (see below) |
| `output_directory`  | string  | Directory for future output artifacts     |
| `api_port`          | integer | Future API port (1–65535)                 |
| `dashboard_enabled` | boolean | Future dashboard toggle                   |

### Logging object

| Key       | Type    | Purpose                                      |
| --------- | ------- | -------------------------------------------- |
| `level`   | string  | `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL` |
| `console` | boolean | Emit to stdout/stderr                        |
| `file`    | boolean | Append to the log file                       |
| `path`    | string  | Log file path (default `logs/sentinelforge.log`) |

### JSON export object

| Key           | Type    | Purpose                                         |
| ------------- | ------- | ----------------------------------------------- |
| `enabled`     | boolean | Write a detections JSON file after each run     |
| `output_file` | string  | Destination path (default `detections.json`)    |

When `enabled` is `false`, `JsonExporter` does nothing. When enabled, it writes
a document with `generated_at`, `rules_loaded`, `rules_evaluated`, `matches`,
and a `detections` array (empty when there are no matches). The console report
from `ReportPrinter` is unchanged.

### Sigma object

| Key               | Type    | Purpose                                      |
| ----------------- | ------- | -------------------------------------------- |
| `enabled`         | boolean | Load Sigma YAML rules in addition to JSON    |
| `rules_directory` | string  | Directory scanned for `*.yml` / `*.yaml`     |

### Monitoring object

| Key                   | Type    | Purpose                                         |
| --------------------- | ------- | ----------------------------------------------- |
| `enabled`             | boolean | Continuous file-based event monitoring          |
| `input_directory`     | string  | Polled for new `*.json` events (`events/incoming`) |
| `processed_directory` | string  | Archive location (`events/processed`)           |
| `poll_interval_ms`    | integer | Poll sleep when the input directory is empty    |

When monitoring is enabled, `EventMonitor` loads rules once, then loops until
CTRL+C. Each event is parsed, evaluated by `DetectionEngine`, printed,
exported, and moved into the processed directory (files are never deleted).
When disabled, the collector falls back to one-shot processing of
`sample_event_file`.

## Live event layout

```
events/
    incoming/      # place new telemetry JSON files here
    processed/     # EventMonitor archives files here after processing
```

## Sigma support (Phase 1)

Pipeline: `Sigma YAML → SigmaParser → SigmaRule → SigmaTranslator → Rule → DetectionEngine`.

`DetectionEngine` never sees Sigma types. Native JSON rules continue to load
unchanged and are merged with translated Sigma rules before evaluation.

### Supported subset

- Top-level: `title`, `id`, `status`, `description`, `logsource`, `detection`, `level`, `tags`
- `detection.selection`: `process_name`, `command_line|contains` (and plain `command_line`)
- `detection.condition`: `selection` only

Unsupported fields and modifiers are ignored safely. Rules missing `title`,
`detection`, `selection`, or `condition` are rejected with descriptive errors;
remaining rules continue to load.

### Limitations / future roadmap

- No full Sigma condition language (`and` / `or` / `not` / aggregations)
- No field modifiers beyond `|contains`
- No correlation / timeframe / pipelines
- Planned expansions: broader modifiers, richer conditions, more log sources
Relative paths are resolved against the repository root. Behavior:

- **File present and valid** — file values override the built-in defaults.
- **File missing** — a warning is logged and built-in defaults are used; the
  collector continues running.
- **File present but invalid** (bad type, empty path, unknown logging level,
  out-of-range port) — a clear error is printed and the collector exits
  cleanly with a non-zero status.

## Performance profiling

`PerformanceProfiler` measures named application stages with
`std::chrono::steady_clock`. `Application` marks stages (`Start` / `Stop`);
it never reads clocks itself. At the end of every run the profiler emits a
structured performance summary through the Logger, covering at least:

- Configuration
- Rule Loading
- Rule Validation
- Event Loading
- Detection Engine
- Report Generation
- Total Runtime

New stages can be added later by name without changing the profiler.

## Build

```
cmake -S . -B build
cmake --build build
```

Produces the `collector` executable and `collector_tests`.

## Run

```
./build/Debug/collector      # Windows (MSVC multi-config)
./build/collector            # single-config generators
```
