# Issue #019 — Desktop console visual QA pass

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part II.  
Architecture: [`docs/architecture/gui-architecture.md`](../architecture/gui-architecture.md).

**Hard prerequisite for Issue #020.** Do not begin investigation work until the two P0 items below are closed.

---

## Scope — corrective only

No new features, no new pages. Fix surface hierarchy, mock seed data, and dashboard polish against the brief.

### P0 — done

- **Panel surfaces** — shared `Panel` primitive (`QFrame` + `WA_StyledBackground`); three distinct luminance levels: `#0B0E14` base, `#121620` panel, `#171C28` header/chip.
- **Mock seed** — `MockTelemetrySource::start()` emits 40 detections and 8 correlation alerts on launch (realistic Windows telemetry, plausible severity mix), then streams at default rate.

### P1 — rendering and density

- Escape `&` in nav labels (e.g. `MITRE ATT&&CK`); remove bare filled rects in status chips.
- Table headers always visible; empty state in rows area only.
- Performance panel: Events/sec, Pipeline latency, CPU, Memory — units, sparklines, progress tracks.
- Accent only on active/hover/focus; resting filters use `BgRaised` + `BorderSubtle`.
- Status chips ~52px; five chips (Rules tooltip holds Sigma/native/correlation split).
- Nav rail 56px collapsed default, SVG icons, 4px item spacing, active left border on item.
- Splitter defaults ~42% / 30% / 28%; persist user choice.

### P2 — polish

- Correlation severity rail; confidence bar per spec; log viewer severity spine.
- Empty-state copy retained; headers stay visible.

## Deferred

Frameless custom title bar · real collector integration · investigation features (#020+).

---

## Acceptance checklist

### P0

- [x] Three distinct surface luminance levels visible in a screenshot.
- [x] Dashboard populated on launch with 40 seeded detections.

### P1

- [ ] No text renders a stray underscore or missing character.
- [ ] No bare filled rectangles anywhere.
- [ ] Table headers visible with an empty model.
- [ ] Performance panel shows Events/sec, Pipeline latency, CPU, Memory — with units.
- [ ] Accent appears only on the active nav item and on hover/focus states.
- [ ] Nav rail 56px collapsed, SVG icons, active item carries the left border.

### Design

- [ ] Grayscale screenshot: severity still identifiable by rail position and pill label.
- [ ] Squint test: the critical row is the brightest thing on screen.
