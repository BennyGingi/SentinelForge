# Issue #018 — Desktop console foundation

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part III.  
Architecture: [`docs/architecture/gui-architecture.md`](../architecture/gui-architecture.md).  
Design: [`docs/design/desktop-design-spec.md`](../design/desktop-design-spec.md).

---

## Scope — ships now

- `gui/` as an independent CMake target; collector builds and tests unchanged.
- Nav rail with all seven entries; Dashboard fully built, others as lazily-constructed placeholders.
- Dashboard: status strip, detection table, correlation table, performance panel, log viewer.
- Inspector pane present, collapsed, placeholder content.
- `ITelemetrySource` + `MockTelemetrySource` on a worker thread, emitting batches.
- `DetectionModel` / `CorrelationModel` with `appendBatch` and bounded ring buffers.
- Generated `Theme.h` and `dark.qss` via `configure_file`.
- `docs/architecture/gui-architecture.md` and `docs/design/desktop-design-spec.md` (this docs tree).

## Deferred

Real collector integration · MITRE page content · timeline · infrastructure views · settings persistence beyond geometry · export.

---

## Acceptance checklist

### Build and runtime

- [ ] Collector builds and its tests pass, untouched.
- [ ] GUI builds as a separate target and launches in under 2 seconds.
- [ ] 1,000 mock events injected in one burst — no frame drop, no freeze.
- [ ] Eight-hour soak at 50 events/sec — memory flat, not growing.
- [ ] Resize 1280×720 → 3840×2160 stays smooth; nothing clips or misaligns.
- [ ] Correct at 100%, 125%, 150% display scaling.

### Architecture

- [ ] No color literal in C++ outside generated `Theme.h`.
- [ ] No parsing, detection, correlation, or rule logic anywhere under `gui/`.
- [ ] Swapping `MockTelemetrySource` for a real source touches one line in `main.cpp`.
- [ ] Adding an eighth nav page touches exactly one file.
- [ ] No widget holds a pointer to the telemetry source.

### Design

- [ ] Squint test: a critical detection is the brightest thing on screen.
- [ ] Grayscale screenshot: every severity still identifiable by label and rail position.
- [ ] Every interactive element has visible hover and visible keyboard focus.
- [ ] Every table has an empty state whose copy says what happens next.
- [ ] Disconnection renders as a status, not a blank screen.
