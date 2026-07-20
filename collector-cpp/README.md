# Collector (C++)

Telemetry collector service for SentinelForge. Currently implements application
bootstrap and logging only; telemetry ingestion is not yet implemented.

## Layout

```
collector-cpp/
├── CMakeLists.txt
├── include/
│   ├── Application.h   # application lifecycle owner
│   └── Logger.h         # console logger (Info/Warning/Error)
├── src/
│   ├── main.cpp          # entry point: construct Application, run, return exit code
│   ├── Application.cpp
│   └── Logger.cpp
└── README.md
```

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
