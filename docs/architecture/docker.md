# Collector in Docker

Issue #029. Runs `collector-cpp` (the API server from
[`collector-api.md`](collector-api.md)) inside a container so the HTTP API is
reachable from the host without a native build.

**The Qt GUI is not containerized, on purpose.** It needs X11/VNC to render
under Docker, and doing that would demonstrate nothing beyond "Qt can draw
into a VNC session" — the actual thing worth showing is the collector running
detached from the host toolchain while a normal desktop client talks to it.
The GUI stays a native desktop binary; it connects to the containerized
collector over the same HTTP API it would use against a locally-built one.

## Image layout

`docker/collector.Dockerfile` is a two-stage build:

- **builder** (`ubuntu:24.04` + cmake/g++/make) compiles only the `collector`
  target — not `collector_tests`/`regression_runner`, which would also pull
  in a googletest `FetchContent` at configure time for no runtime benefit.
- **runtime** (fresh `ubuntu:24.04` + `libstdc++6` only) ships just the
  compiled binary, running as a non-root `collector` user.

The runtime stage recreates `collector-cpp`'s repo-relative layout under
`/src` (`/src/config`, `/src/rules`, `/src/sigma-rules`, `/src/events/...`,
`/src/output`, `/src/logs`) because `CONFIG_FILE_PATH` and the `DEFAULT_*`
fallback paths are baked in at CMake configure time as paths relative to
`collector-cpp/`'s parent directory — mirroring that layout in the image is
what makes those compiled-in paths resolve correctly at runtime.

`docker/collector.config.json` is copied in as `/src/config/sentinelforge.json`.
It's identical to the repo default except `api.bind_address` is `0.0.0.0`
instead of `127.0.0.1` — the collector's own loopback default is correct for
a local build (nothing outside the host should reach it) but useless in a
container, where the host can only reach a bind that isn't localhost-only.
This bind address was already configurable from phase 1; nothing in the
collector itself changed for Docker.

## Building and running

```
docker compose up --build
```

This builds the image from `docker/collector.Dockerfile` and starts the
`collector` service defined in `docker-compose.yml`:

- `8787:8787` — the API port, published to the host.
- `./rules:/src/rules:ro`, `./sigma-rules:/src/sigma-rules:ro` — the same
  rule directories the collector reads outside Docker, bind-mounted
  read-only so rule edits on the host take effect on the next container
  restart without a rebuild.
- `./events/incoming:/src/events/incoming` — read-write, so the host can
  drop event files in for the container's `EventMonitor` to pick up.
  `events/processed` and `events/failed` are **not** mounted; they're
  purely internal to the container's own filesystem.
- `restart: unless-stopped`.

Verify the API is reachable from the host:

```
curl http://localhost:8787/health
# {"status":"ok","uptime_seconds":...,"version":"0.1.0"}
```

## Feeding events in

Drop a JSON event file into `events/incoming/` on the host (any filename,
e.g. `sample-logs/process_create.json`):

```
cp sample-logs/process_create.json events/incoming/test.json
```

The container's `EventMonitor` polls that mounted directory every second,
picks the file up, runs it through detection, and moves it to the
container-internal `events/processed/` (or `events/failed/` on a parse
error) — it disappears from the host-visible `events/incoming/` once
picked up. Confirm it was detected:

```
curl http://localhost:8787/detections
```

**Don't also run a native `collector` on the host against the same
`events/incoming/` directory at the same time as the container** — both
poll the same host path and will race to consume the same files, since only
`events/incoming` (not the processed/failed dirs) is shared between them.

## Connecting the GUI

Build and run the GUI natively on the host, same as always, without
`--mock`:

```
gui/build/bin/Debug/sentinelforge_desktop.exe
```

`CollectorTelemetrySource`'s default base URL is `http://127.0.0.1:8787`
(`gui/include/telemetry/CollectorTelemetrySource.h`) — the same port the
compose file publishes — so no flags or config are needed for the GUI to
find the containerized collector. It polls `/detections`, `/correlations`,
`/logs`, and `/stats` exactly as it would against a locally-built collector;
it has no way to tell the difference.
