# Collector (C++)

Telemetry collector service for SentinelForge. Loads runtime settings from a
central configuration file, ingests a telemetry event, evaluates it against a
directory of detection rules, and prints a report.

## Layout

```
collector-cpp/
├── CMakeLists.txt
├── include/
│   ├── Application.h      # application lifecycle owner; owns Configuration
│   ├── Configuration.h    # immutable, centrally-owned runtime settings
│   ├── Logger.h           # console logger with severity levels
│   └── ...                # Event/Rule parsing, validation, detection, reporting
├── src/
│   ├── main.cpp           # entry point: construct Application, run, return exit code
│   ├── Application.cpp
│   ├── Configuration.cpp
│   ├── Logger.cpp
│   └── ...
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
| `logging_level`     | string  | `DEBUG`, `INFO`, `WARNING`, or `ERROR`    |
| `output_directory`  | string  | Directory for future output artifacts     |
| `api_port`          | integer | Future API port (1–65535)                 |
| `dashboard_enabled` | boolean | Future dashboard toggle                   |

Relative paths are resolved against the repository root. Behavior:

- **File present and valid** — file values override the built-in defaults.
- **File missing** — a warning is logged and built-in defaults are used; the
  collector continues running.
- **File present but invalid** (bad type, empty path, unknown logging level,
  out-of-range port) — a clear error is printed and the collector exits
  cleanly with a non-zero status.

## Build

```
cmake -S . -B build
cmake --build build
```

Produces the `collector` executable.

## Run

```
./build/Debug/collector      # Windows (MSVC multi-config)
./build/collector            # single-config generators
```
