# SentinelForge — Engineering Brief & Desktop Design Specification

> **How to use.** Paste this entire document into Cursor, then append the current issue (e.g. Issue #018) below it. Part I is the strategic context. Part II is the implementation contract. Part III scopes what ships now. Commit as `docs/BRIEF.md`; it is the parent of everything in `docs/`.

---

# Part I — Product Vision & Architecture

## 0. Role

You are joining SentinelForge as **Lead Software Architect, Senior C++/Qt Engineer, and Product Designer**.

Think beyond writing code. Design a product that could realistically evolve into a commercial enterprise security platform. Every decision should support scalability, maintainability, operator usability, and future expansion.

You are encouraged to improve the architecture, UI, and developer experience — provided you preserve the existing backend architecture. Do not merely satisfy the specification. If you identify a significantly better layout, navigation structure, interaction model, or component architecture, explain your reasoning and implement it.

## 1. What SentinelForge is becoming

Not another GUI. A modern security platform that demonstrates modern C++, software architecture, desktop engineering, detection engineering, security engineering, DevOps, cloud-native deployment, and enterprise UI design.

The bar: someone opens it and thinks *"this looks like an actual commercial security product."*

## 2. Roadmap

The desktop console is one component. The platform will eventually include:

Windows and Linux event collection · Sysmon support · Sigma rules · native detection rules · threat intelligence · behavioral detection · correlation engine · JSON export · REST API · desktop console · Docker · Kubernetes · Prometheus · Grafana · cloud deployment · AI investigation assistant · threat hunting workspace.

**Design today so none of these require redesigning the application.** Section 8 and the inspector pane in Part II §5 exist specifically to honor this.

## 3. Existing backend — do not modify

Already implemented and tested:

```
Configuration → Rule Loader → Live Event Monitor → JSON Parser
  → Event Normalizer → Detection Engine → Correlation Engine
  → Reporting → JSON Export
```

Includes configuration system, rule validation, native detection rules, Sigma translation, structured logging, performance profiler, JSON export, live event monitoring, event normalization, detection engine, correlation engine, and automated tests.

**Scope of "implemented and tested" — collector only, not yet wired to the GUI.** Every item above is implemented and covered by `collector-cpp`'s GoogleTest suite. None of it is reachable from the desktop console today: `ITelemetrySource` (§4) has exactly one implementation, `MockTelemetrySource`, and the GUI's Detections/Correlation tables render mock data, not collector output. `CollectorTelemetrySource` — the adapter that would carry real detections and `CorrelationAlert`s from this backend into the GUI — does not exist yet; it remains future work per §4's diagram ("future integration"). Do not read "implemented and tested" as "visible in the running desktop console."

The collector must continue to build and test independently of the GUI. The GUI is a separate CMake target.

## 4. GUI architecture

```
+----------------------------------------------------+
|               SentinelForge Desktop                |
|  Dashboard · Detections · Correlation · MITRE      |
|  Timeline · Infrastructure · Settings              |
+-------------------------▲--------------------------+
                          │  (queued signals only)
                  ITelemetrySource
                          │
        +-----------------+-----------------+
        |                                   |
MockTelemetrySource            CollectorTelemetrySource
(initial development)          (future integration)
                          │
                          ▼
+----------------------------------------------------+
|             SentinelForge Collector                |
+----------------------------------------------------+
```

The GUI must never know whether it is connected to mock data or the real collector. Dependency inversion is non-negotiable.

### ITelemetrySource contract

```cpp
class ITelemetrySource : public QObject {
    Q_OBJECT
public:
    ~ITelemetrySource() override = default;
    virtual void start() = 0;
    virtual void stop()  = 0;
signals:
    void detectionsReceived(QVector<Detection>);          // batched
    void correlationAlertsReceived(QVector<CorrelationAlert>);
    void logLinesReceived(QVector<LogLine>);
    void statsUpdated(CollectorStats);
    void connectionStateChanged(ConnectionState, QString detail);
};
```

Four rules, all load-bearing:

1. **No blocking getters.** Nothing returns data synchronously. The mock is in-process and instant; the real source will be REST polling or IPC and will not be. An interface that permits `getDetections()` today forces a rewrite tomorrow.
2. **Signals carry batches, never single items.** A per-event signal at 1,000 events/sec is 1,000 queued cross-thread invocations. See §7.
3. **The source lives on its own `QThread`.** `moveToThread` in `main.cpp`; all connections are `Qt::QueuedConnection`. The UI thread never blocks on telemetry.
4. **`connectionStateChanged` exists from day one.** The mock emits `Connected` immediately. The real source will emit `Reconnecting` and `Failed`, and the UI must already have somewhere to render that.

`MainWindow` takes `std::unique_ptr<ITelemetrySource>` by constructor injection and never learns the concrete type. Swapping mock for collector is one line in `main.cpp`.

## 5. Separation of responsibilities

The GUI must **never** contain: detection logic, rule evaluation, Sigma parsing, event normalization, correlation logic, configuration validation, file monitoring, or any business logic.

The GUI is responsible **only** for: presenting telemetry, user interaction, navigation, visualization, window management, rendering, and accessibility.

All security logic belongs in the collector. A widget that parses anything is a bug.

## 6. Engineering standards

Target **C++20 as the floor, C++23 where the toolchain supports it** — MSVC and MinGW diverge on C++23 library support, so guard anything beyond `std::span` and designated initializers behind feature-test macros. Do not let the standard version become a build blocker.

Use: SOLID · RAII · smart pointers · dependency injection · single responsibility · composition over inheritance · interface-driven design.

Avoid: global variables · singleton abuse · tight coupling · circular dependencies · hardcoded constants · widget-owned business logic.

Qt ownership note: parented `QObject`s are already RAII. Use raw pointers with parents for widgets in a layout — do **not** wrap parented widgets in `unique_ptr`, that is double-ownership. Reserve smart pointers for non-parented objects: the telemetry source, models not parented to a view, and worker objects.

## 7. Performance contract

Targets: startup < 2s · dashboard refresh < 100ms · 60 FPS resize · no UI freeze at 1,000+ events · continuous log streaming without stutter · no unnecessary allocation during rendering.

These are not achievable with naive per-row model updates. Implement the mechanisms:

**Batching.** Models expose `appendBatch(std::span<const T>)` issuing exactly one `beginInsertRows`/`endInsertRows` per batch. A 100ms coalescing `QTimer` in the source flushes accumulated events, so burst traffic becomes ten updates per second regardless of event rate.

**Bounded memory.** Detection model caps at 10,000 rows, correlation at 5,000, evicting oldest via `beginRemoveRows`. `QPlainTextEdit::setMaximumBlockCount(5000)` on the log viewer. Memory stays flat across an eight-hour session; unbounded growth is the single most common failure in tools like this.

**No allocation in `paint()`.** Delegates cache `QFont`, `QPen`, `QColor`, and pill metrics as members initialized once. Constructing a `QFont` per cell per repaint is what kills scroll performance.

**Fixed-size sparklines.** `std::array<float, N>` ring buffer, no `QVector` growth, no reallocation per sample.

**Lazy page construction.** Only the Dashboard is built at startup. Other pages are constructed on first navigation and cached. This protects the < 2s target permanently, no matter how many pages the roadmap adds.

**Scroll safety.** Auto-scroll suspends the moment the user scrolls away from the bottom, and a "resume" affordance appears. An analyst reading a log line must never have it yanked away.

**Filtering.** Use `QSortFilterProxyModel` for filtering only; never rebuild the proxy per event. Invalidate on filter change, not on insert.

## 8. Navigation model

**Reconciliation — read this before building.** The roadmap lists Detections and Correlation as top-level pages; Issue #018 places both as panels on the main window. Both are correct, for different jobs:

- **Dashboard** — the *live* view. Recent detections (last 50), recent correlation alerts, performance, logs. Severity filter only. Answers: *is anything happening right now?*
- **Detections / Correlation pages** — the *investigative* view. Full retained history, search, multi-column filtering, column chooser, export, inspector pane. Answers: *what happened, and why?*

Build **one** `DetectionTableView` component and configure it differently in each context. Do not write the table twice.

Rail order — Dashboard · Detections · Correlation · MITRE ATT&CK · Threat Timeline · Infrastructure · (spacer) · Settings.

**Proposed change:** Docker and Kubernetes merge into a single **Infrastructure** page with tabs. They answer the same operator question — *where is this running and is it healthy?* — and seven rail items scan faster than eight. Split them back out if deployment views diverge enough to justify it.

Adding a page must be one call: `navigation.addPage(icon, title, factory)`, taking a factory callable for lazy construction. If adding a page requires touching more than one file, the navigation architecture is wrong.

## 9. Documentation

```
docs/
  BRIEF.md                        ← this document, the parent
  architecture/
    backend.md
    event-pipeline.md
    gui-architecture.md           ← expands Part I §4–6
  design/
    desktop-design-spec.md        ← Part II verbatim, single source
    color-system.md               ← rationale; values generated
    typography.md                 ← rationale; values generated
  roadmap/
    roadmap.md
    issue-018.md
```

**Single source of truth for design tokens.** Hex values and font stacks live in exactly one place: `cmake/theme.cmake`. `Theme.h` and `dark.qss` are both generated from it via `configure_file`. The design docs explain *why* each token exists and mark any value table as generated. Three hand-maintained copies of a palette drift within a month.

```cmake
# cmake/theme.cmake — the only place a hex value is written
set(SF_BG_BASE      "#0B0E14")
set(SF_BG_SURFACE   "#121620")
set(SF_ACCENT       "#38D6C4")
# ... full token set

configure_file(${CMAKE_SOURCE_DIR}/gui/include/Theme.h.in
               ${CMAKE_BINARY_DIR}/generated/Theme.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/gui/resources/styles/dark.qss.in
               ${CMAKE_BINARY_DIR}/generated/dark.qss @ONLY)
```

Every architectural decision gets documented. A new contributor should understand the project without reading the implementation first.

---

# Part II — Desktop Design Specification

## 1. Design intent

This is a **12-hour-shift instrument panel**, not a marketing dashboard. It sits on a second monitor and gets glanced at every few minutes. The main view answers, in under two seconds of peripheral vision: *is the collector healthy, and did anything just fire that I need to look at?*

Three consequences that drive everything below:

1. **Low luminance, low saturation at rest.** Nothing is bright unless it means something. A calm screen makes a red row impossible to miss.
2. **Machine facts look like machine facts.** Timestamps, PIDs, paths, hashes, and MITRE technique IDs render in monospace. Human-authored labels — rule names, behavior names, headers, nav — render in the UI sans. This split is the console's signature; apply it everywhere.
3. **Severity is readable without color.** Color is never the only signal. Every severity carries a text label and a positional cue.

## 2. Color tokens

Defined in `cmake/theme.cmake`, generated into `Theme.h` and `dark.qss`.

### Surfaces — cool slate, never pure black
| Token | Hex | Use |
|---|---|---|
| `BgBase` | `#0B0E14` | Application background, nav rail |
| `BgSurface` | `#121620` | Panels, cards, table body |
| `BgRaised` | `#171C28` | Table headers, hovered rows, chips |
| `BgInset` | `#0E1219` | Log viewer, mono areas, inspector |
| `BorderSubtle` | `#1F2633` | Panel edges, separators |
| `BorderStrong` | `#2A3342` | Focused inputs, splitter handles |

### Text
| Token | Hex | Use |
|---|---|---|
| `TextPrimary` | `#E4E9F2` | Values, cells, log body |
| `TextSecondary` | `#9AA6B8` | Labels, headers, inactive nav |
| `TextMuted` | `#6B7688` | Units, resting timestamps, empty states |

### Accent — one only
| Token | Hex | Use |
|---|---|---|
| `Accent` | `#38D6C4` | Active nav, focus ring, selection edge, primary button |
| `AccentDim` | `#1E6E68` | Progress fill, subdued accent backgrounds |

**Hard rule:** the accent means *interactive or active*. It never indicates a threat, a status, or a metric magnitude. If a color could be mistaken for an alert, it is the wrong color there.

### Severity — owns all warm hues
| Level | Foreground | Badge background | Rail |
|---|---|---|---|
| Critical | `#FF4D6A` | `#2A0F17` | `#FF4D6A` |
| High | `#FF8A3D` | `#2A1710` | `#FF8A3D` |
| Medium | `#FFC53D` | `#2A2210` | `#FFC53D` |
| Low | `#4DA3FF` | `#10202E` | `#4DA3FF` |
| Info | `#7C8899` | `#161B24` | `#7C8899` |

### Operational status
`StatusOk #3ECF8E` · `StatusWarn #FFC53D` · `StatusError #FF4D6A` · `StatusIdle #6B7688`

Status renders as an 8px dot. Never emoji, never "OK ✅".

## 3. Typography

```
UI      "Inter", "Segoe UI Variable Text", "Segoe UI", sans-serif
Mono    "JetBrains Mono", "Cascadia Mono", "Consolas", monospace
```

Bundle both via `QFontDatabase::addApplicationFont` from the resource system; the stack is the fallback chain.

| Role | Size | Weight | Tracking | Notes |
|---|---|---|---|---|
| Micro label | 11px | 600 | +0.08em | UPPERCASE. Chip captions, eyebrows. |
| Table cell | 12px | 400 | 0 | Mono for machine columns. |
| Body | 13px | 400 | 0 | Log lines (mono), descriptions. |
| Panel title | 14px | 600 | +0.02em | Sentence case: "Recent detections". |
| Metric value | 22px | 600 | -0.01em | Mono, tabular figures so digits don't jitter. |
| Hero metric | 30px | 600 | -0.02em | Mono. Events Processed, Detections. |

Never bold a whole row or table. Emphasis comes from color and position.

## 4. Spacing, sizing, radius

- Base unit **4px**. Only 4 / 8 / 12 / 16 / 24 / 32.
- Panel padding **16px** · panel gap **12px** · window margin **12px**.
- Row height **32px** · header height **36px** · cell padding `8px 12px`.
- Nav rail **56px** collapsed, **200px** expanded. Status strip **64px**. Inspector **380px**.
- Radius: **6px** panels, **4px** chips/inputs/buttons, **10px** severity pills. Nothing else is rounded.
- Borders are always **1px**.

## 5. Layout skeleton

```
┌──────┬────────────────────────────────────────┬───────────────┐
│      │  STATUS STRIP  [collector · rules ·    │               │
│ nav  │   sigma · correlation · events ·       │   INSPECTOR   │
│ rail │   detections · alerts]                 │   380px       │
│ 56px ├────────────────────────────────────────┤   collapsed   │
│      │ ┌────────────────────────────────────┐ │   by default  │
│  ▣   │ │ Recent detections     [severity ▾] │ │               │
│  ◫   │ │ ▌ time  severity  rule  process  T#│ │   selected    │
│  ◈   │ └────────────────────────────────────┘ │   detection   │
│  ⌗   │ ═══════ QSplitter (vertical) ════════  │   detail      │
│  ⬡   │ ┌──────────────────┬─────────────────┐ │               │
│      │ │ Correlation      │ Performance     │ │   (roadmap:   │
│      │ └──────────────────┴─────────────────┘ │   raw event,  │
│      │ ═══════ QSplitter (vertical) ════════  │   rule, AI    │
│  ⚙   │ ┌────────────────────────────────────┐ │   summary)    │
│      │ │ Live logs (collapsible)            │ │               │
└──────┴────────────────────────────────────────┴───────────────┘
```

**The inspector is reserved now and populated later.** The roadmap's AI Investigation Assistant and Threat Hunting Workspace both need a detail view for a selected detection — raw event JSON, matched rule, technique context, generated summary. Retrofitting a right-hand pane into a finished three-panel layout is precisely the redesign this brief forbids. Build it now as a collapsible `QWidget` in a `QSplitter`, hidden until a row is activated, containing a placeholder. Cost today: ~30 lines. Cost later: a layout rewrite.

Splitters get sensible `setStretchFactor` values and non-zero minimums so no panel collapses to invisible. Persist splitter sizes, window geometry, and inspector state via `QSettings`.

## 6. Component specs

### Status chip
`BgRaised`, 1px `BorderSubtle`, 4px radius, padding `8px 12px`. Micro label in `TextSecondary` above a mono value in `TextPrimary`. The collector chip prefixes an 8px status dot driven by `connectionStateChanged`. Chips live in a `QHBoxLayout` that wraps gracefully — never fix their widths.

### Detection table — signature element
A **3px severity rail** runs the full height of each row's left edge, flush to the panel border. It is the fastest scan target on screen and the console's identity mark. Paint it in a `QStyledItemDelegate`; never set item backgrounds directly.

- Severity renders as a pill: severity background and foreground, 10px radius, `2px 8px` padding, 11px/600 uppercase.
- Timestamp, process, and technique columns are **mono**; rule name is UI sans.
- Technique shows `T1059.001` in `TextSecondary` mono, with the technique name as tooltip.
- `setShowGrid(false)` · `setAlternatingRowColors(false)` · vertical header hidden · `SelectRows` · `NoEditTriggers` · `setUniformRowHeights(true)`.
- Hover → `BgRaised`. Selected → `BgRaised` plus a 2px `Accent` inset inside the severity rail.
- Header: `BgRaised`, `TextSecondary`, micro-label type, 1px bottom border.
- `QHeaderView`: `ResizeToContents` on timestamp/severity/technique, `Stretch` on rule and process.
- Row activation opens the inspector.

### Correlation table
Same chrome. Confidence renders as a horizontal bar — track `BgRaised`, fill `Accent`, 4px tall, 6px radius — with the percentage in mono beside it. **Never color the confidence bar by value.** Confidence is not severity, and coloring it warm makes a high-confidence low-severity alert look like an emergency.

### Performance panel
Four stat blocks in a 2×2 `QGridLayout`: micro label, hero mono value, unit in `TextMuted`, and a 28px `SparklineWidget` drawn with `QPainter` — 1px `Accent` stroke, no fill, no gradient. CPU and memory add a 4px progress track. Values update on a `QTimer` so the panel visibly lives.

### Log viewer
`QPlainTextEdit`, read-only, `BgInset`, mono 13px, line height 1.5, `setMaximumBlockCount(5000)`. Each line carries a 2px severity spine in the left margin echoing the detection rail. Level tokens take the severity foreground; the body is `TextPrimary`; timestamps `TextMuted`. Header row carries a level filter and a clear action. Auto-scroll pauses on manual scroll.

### Empty and error states
Every table ships an empty state: centered, `TextMuted`, 13px, stating what will appear and what to do — *"No detections yet. Events will appear here as the collector processes them."* Errors state what failed and the next step, in the interface's voice. No apologies, no vagueness. Disconnection is an error state, not a blank table.

## 7. Qt implementation rules

- `QApplication::setStyle(QStyleFactory::create("Fusion"))` **first**, then a matching `QPalette`, then the QSS. Fusion stops unstyled widgets falling back to native Windows chrome.
- One stylesheet, generated to `dark.qss`, compiled into `sentinelforge.qrc`, applied once at `QApplication` level. No scattered `setStyleSheet` calls.
- No color literal appears in C++ outside generated `Theme.h`.
- Icons: bundled monochrome **SVG**, tinted via `QIcon` and palette. No emoji, no `QStyle::standardIcon`, no PNG.
- High-DPI: `setHighDpiScaleFactorRoundingPolicy(PassThrough)` before constructing `QApplication`. Verify at 100%, 125%, 150%.
- QSS has no transitions, shadows, or opacity animation. Do not fake them with `border-image`. Use flat contrast shifts on hover.
- Focus is always visible: 1px `Accent` outline. Full keyboard tab order through nav, tables, filters, inspector.
- Tables are `QAbstractTableModel` subclasses — **never** `QTableWidget`. `QTableWidget` welds data to view and makes the mock-to-collector swap a rewrite.

## 8. Do not

- Pure black `#000` or pure white `#FFF`.
- Gradients, bevels, drop shadows, glows.
- Emoji as icons or status.
- Gridlines or zebra striping.
- Centered text in data columns — text left-aligns, numbers right-align.
- A second accent, or accent on anything non-interactive.
- Severity by color alone.
- Hardcoded pixel positions, fixed widget sizes, `move()` calls. Layouts only.
- `!important` in QSS, or selectors specific enough to cancel each other out.
- Default Qt blue selection highlight.

## 9. Starter QSS

```css
/* dark.qss.in — generated by CMake from cmake/theme.cmake. Do not edit output. */

QWidget {
    background-color: @SF_BG_BASE@;
    color: @SF_TEXT_PRIMARY@;
    font-family: "Inter", "Segoe UI Variable Text", "Segoe UI", sans-serif;
    font-size: 13px;
}

QFrame#Panel {
    background-color: @SF_BG_SURFACE@;
    border: 1px solid @SF_BORDER_SUBTLE@;
    border-radius: 6px;
}

QLabel#PanelTitle {
    font-size: 14px; font-weight: 600; letter-spacing: 0.02em;
    color: @SF_TEXT_PRIMARY@; padding: 0 0 8px 0;
}

QLabel#MicroLabel {
    font-size: 11px; font-weight: 600; letter-spacing: 0.08em;
    color: @SF_TEXT_SECONDARY@;
}

QLabel#MetricValue {
    font-family: "JetBrains Mono", "Cascadia Mono", "Consolas", monospace;
    font-size: 22px; font-weight: 600; color: @SF_TEXT_PRIMARY@;
}

QFrame#StatusChip {
    background-color: @SF_BG_RAISED@;
    border: 1px solid @SF_BORDER_SUBTLE@;
    border-radius: 4px; padding: 8px 12px;
}

QTableView {
    background-color: @SF_BG_SURFACE@;
    alternate-background-color: @SF_BG_SURFACE@;
    gridline-color: transparent;
    border: none; outline: none;
    selection-background-color: @SF_BG_RAISED@;
    selection-color: @SF_TEXT_PRIMARY@;
    font-size: 12px;
}
QTableView::item { padding: 8px 12px; border: none; }
QTableView::item:hover { background-color: @SF_BG_RAISED@; }

QHeaderView::section {
    background-color: @SF_BG_RAISED@;
    color: @SF_TEXT_SECONDARY@;
    font-size: 11px; font-weight: 600; letter-spacing: 0.08em;
    text-transform: uppercase;
    padding: 8px 12px;
    border: none; border-bottom: 1px solid @SF_BORDER_SUBTLE@;
}

QPlainTextEdit#LogViewer {
    background-color: @SF_BG_INSET@;
    border: 1px solid @SF_BORDER_SUBTLE@;
    border-radius: 6px;
    font-family: "JetBrains Mono", "Cascadia Mono", "Consolas", monospace;
    font-size: 13px; color: @SF_TEXT_PRIMARY@; padding: 8px;
}

QToolButton#NavItem {
    background: transparent; border: none;
    border-left: 2px solid transparent;
    padding: 12px 0; color: @SF_TEXT_MUTED@;
}
QToolButton#NavItem:hover   { color: @SF_TEXT_SECONDARY@; background-color: @SF_BG_SURFACE@; }
QToolButton#NavItem:checked { color: @SF_ACCENT@; border-left: 2px solid @SF_ACCENT@; }

QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
QScrollBar::handle:vertical {
    background: @SF_BORDER_STRONG@; border-radius: 5px; min-height: 24px;
}
QScrollBar::handle:vertical:hover { background: #3A455A; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; }

QSplitter::handle { background-color: @SF_BORDER_SUBTLE@; }
QSplitter::handle:horizontal { width: 1px; }
QSplitter::handle:vertical   { height: 1px; }
```

---

# Part III — Scope for Issue #018

Ships now:

- `gui/` as an independent CMake target; collector builds and tests unchanged.
- Nav rail with all seven entries; Dashboard fully built, others as lazily-constructed placeholders.
- Dashboard: status strip, detection table, correlation table, performance panel, log viewer.
- Inspector pane present, collapsed, placeholder content.
- `ITelemetrySource` + `MockTelemetrySource` on a worker thread, emitting batches.
- `DetectionModel` / `CorrelationModel` with `appendBatch` and bounded ring buffers.
- Generated `Theme.h` and `dark.qss` via `configure_file`.
- `docs/architecture/gui-architecture.md` and `docs/design/desktop-design-spec.md`.

Deferred: real collector integration, MITRE page content, timeline, infrastructure views, settings persistence beyond geometry, export.

## Acceptance criteria

**Build and runtime**
- [ ] Collector builds and its tests pass, untouched.
- [ ] GUI builds as a separate target and launches in under 2 seconds.
- [ ] 1,000 mock events injected in one burst — no frame drop, no freeze.
- [ ] Eight-hour soak at 50 events/sec — memory flat, not growing.
- [ ] Resize 1280×720 → 3840×2160 stays smooth; nothing clips or misaligns.
- [ ] Correct at 100%, 125%, 150% display scaling.

**Architecture**
- [ ] No color literal in C++ outside generated `Theme.h`.
- [ ] No parsing, detection, correlation, or rule logic anywhere under `gui/`.
- [ ] Swapping `MockTelemetrySource` for a real source touches one line in `main.cpp`.
- [ ] Adding an eighth nav page touches exactly one file.
- [ ] No widget holds a pointer to the telemetry source.

**Design**
- [ ] Squint test: a critical detection is the brightest thing on screen.
- [ ] Grayscale screenshot: every severity still identifiable by label and rail position.
- [ ] Every interactive element has visible hover and visible keyboard focus.
- [ ] Every table has an empty state whose copy says what happens next.
- [ ] Disconnection renders as a status, not a blank screen.
