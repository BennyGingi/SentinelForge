# SentinelForge Desktop

Qt 6 desktop console for SentinelForge (Issue #018).

The GUI is a **separate CMake target** from the collector. It never contains
detection, correlation, Sigma, or parsing logic. Telemetry arrives only through
`ITelemetrySource` batch signals.

## Requirements

- CMake ≥ 3.21
- Qt 6.5+ (Widgets) — MSVC or MinGW kit
- C++20

Set `CMAKE_PREFIX_PATH` to your Qt kit, e.g. `C:\Qt\6.7.3\msvc2019_64`.

## Build / run

```
.\scripts\build-gui.ps1
.\scripts\run-gui.ps1
```

## Architecture

```
MainWindow
  └── NavigationController (lazy pages)
        ├── Dashboard (status, detections, correlation, performance, logs)
        ├── Detections / Correlation / MITRE / Timeline / Infrastructure / Settings
        └── InspectorPane (collapsed placeholder)

ITelemetrySource  ──queued──►  models / dashboard widgets
MockTelemetrySource (worker QThread)
```

Swap mock for a real collector source by changing one line in `src/main.cpp`.

Design tokens live in `cmake/theme.cmake` and generate `Theme.h` + `dark.qss`.

See `docs/BRIEF.md` and `docs/architecture/gui-architecture.md`.
