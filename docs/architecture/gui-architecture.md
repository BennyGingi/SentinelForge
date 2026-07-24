# GUI Architecture

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part I §4–6 (GUI architecture, separation of responsibilities, engineering standards), plus §7–8 (performance contract, navigation).

This document expands those sections into the working contract for `gui/`.

---

## 1. Dependency inversion via `ITelemetrySource`

The desktop never knows whether it is fed by mock data or the real collector. All telemetry enters through one abstract source:

```
Desktop (pages, models, views)
        ▲
        │  queued signals only
ITelemetrySource
        │
   ┌────┴────┐
MockTelemetrySource    CollectorTelemetrySource (future)
```

### Contract

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

### Four load-bearing rules

1. **No blocking getters for telemetry.** Live detections/alerts/logs/stats arrive only via signals. The mock is in-process and instant; the real source will be REST polling or IPC.
2. **Signals carry batches, never single items.** A per-event signal at 1,000 events/sec is 1,000 queued cross-thread invocations. Models consume `appendBatch` (see §4).
3. **The source lives on its own `QThread`.** `moveToThread` in `main.cpp`; all connections are `Qt::QueuedConnection`. The UI thread never blocks on telemetry streams.
4. **`connectionStateChanged` exists from day one.** The mock emits `Connected` immediately. The real source will emit `Reconnecting` and `Failed`; the UI must already render those states (status chip / error state — not a blank table).

**Documented exception — `ruleInfo(ruleId)`.** Rule metadata is static reference data, not a telemetry stream. `ITelemetrySource::ruleInfo` is the one permitted synchronous accessor. When the source lives on a worker thread, the UI calls it with `Qt::BlockingQueuedConnection`. Widgets must never parse `rawEventJson` to invent IOCs, related events, or rule fields — those arrive precomputed on `Detection`.

`MainWindow` takes `std::unique_ptr<ITelemetrySource>` by constructor injection and never learns the concrete type. Swapping mock for collector is one line in `main.cpp`.

---

## 2. Thread model

```
main thread                          worker thread
─────────────                        ─────────────
QApplication                         ITelemetrySource
MainWindow                             (Mock or Collector)
  pages / models / views
       ▲
       │  Qt::QueuedConnection
       └──────────────────────────── signals with batches
```

- Construct the concrete source, `moveToThread` onto a dedicated `QThread`, then `start()` the thread and call `source->start()` via a queued invoke or direct call after the object lives on the worker.
- Connect every telemetry signal to UI slots with **queued** connections only.
- Never call into the source from the UI thread except through the start/stop API (also queued when the object is on another thread).
- Widgets and models never hold a raw pointer used for synchronous reads; they only react to signals.

---

## 3. MainWindow injection and ownership

```cpp
// Conceptual — MainWindow never names MockTelemetrySource
auto source = std::make_unique<MockTelemetrySource>();
auto* window = new MainWindow(std::move(source));
```

| Object | Ownership |
|---|---|
| `ITelemetrySource` | `std::unique_ptr` held by `MainWindow` (or a thin shell that owns both window and thread). Not parented as a widget. |
| Worker `QThread` | Owned by the application / MainWindow; stopped cleanly on shutdown. |
| Pages / widgets | Parented `QObject`s in layouts — **raw pointers with parents**, not `unique_ptr` (avoids double-ownership). |
| Table models | Prefer parented to the view or MainWindow; use smart pointers only if not parented. |

Qt ownership note (from the brief): parented `QObject`s are already RAII. Reserve smart pointers for non-parented objects: the telemetry source, unparented models, and worker objects.

---

## 4. No business logic in the GUI

The GUI must **never** contain:

- Detection logic, rule evaluation, Sigma parsing
- Event normalization, correlation logic
- Configuration validation, file monitoring
- Any other security / pipeline business logic

The GUI is responsible **only** for:

- Presenting telemetry
- User interaction, navigation, visualization
- Window management, rendering, accessibility

All security logic belongs in the collector. A widget that parses event payloads is a bug.

**Allowed presentation parsing (Issue #020/#021):**

- Field-scoped search query tokenization over already-normalized model fields (UI control text).
- MITRE technique ID → name/description lookup from bundled `gui/resources/mitre/techniques.json` — presentation reference data, not detection logic.
- Timestamp formatting from epoch milliseconds at paint time.

**Forbidden:** extracting IOCs from `rawEventJson`, inferring related events, Sigma/rule evaluation, or walking raw JSON to build inspector fields.

Acceptance implication: no detection, correlation, or rule-evaluation logic under `gui/`. No widget holds a pointer to the telemetry source for streaming queries — only signal-driven updates into models/views, plus the documented `ruleInfo` exception.

---

## 5. Page factory navigation

Rail order:

Dashboard · Detections · Correlation · MITRE ATT&CK · Threat Timeline · Infrastructure · (spacer) · Settings

**Lazy construction.** Only the Dashboard is built at startup. Other pages are constructed on first navigation and cached. This protects the &lt; 2s startup target as the roadmap adds pages.

**Factory registration.** Adding a page must be one call:

```cpp
navigation.addPage(icon, title, factory);
```

`factory` is a callable that constructs the page on first visit. If adding a page requires touching more than one file, the navigation architecture is wrong.

### Dashboard vs investigative pages

| Context | Job | Table configuration |
|---|---|---|
| **Dashboard** | Live view — *is anything happening right now?* | Recent detections (last 50), recent correlation alerts, severity filter only |
| **Detections / Correlation pages** | Investigative — *what happened, and why?* | Full retained history, search, multi-column filtering, column chooser, export, inspector |

Build **one** `DetectionTableView` (and one correlation table component) and configure it differently per context. Do not duplicate the table.

**Infrastructure:** Docker and Kubernetes merge into a single page with tabs (operator question: *where is this running and is it healthy?*). Split later only if the views diverge enough to justify two rail entries.

**Inspector:** Collapsible right pane (`QSplitter`), default collapsed, **420px** when open. Investigation workspace (Issue #020): triage, process lineage, related detections (id lookup only), MITRE presentation block, precomputed IOCs, raw JSON verbatim. Related-event navigation uses a back stack.

### Live operations (Issue #021)

- **Pause is view-only.** `LiveOpsController` buffers batches while paused; `ITelemetrySource` keeps emitting. Caps: 5,000 detections / 2,000 alerts / 5,000 logs. Overflow retains Critical/High first and **always discloses drop counts**.
- Resume flushes in 200-row chunks on a zero-delay timer so a full buffer does not stall a frame.
- Search uses a lowercase blob cached at insert time and a 150 ms debounced `DetectionFilterProxy`. Field-scoped query tokens (`process:`, `severity:`, …) parse UI control text over already-normalized fields — not event-payload parsing.
- Threat level on the status strip is a **presentation heuristic** (15-minute weighted decay + hysteresis). The collector should own scoring in a later issue.

---

## 6. Model batching and bounded memory

Naive per-row updates cannot meet the performance contract (startup &lt; 2s · dashboard refresh &lt; 100ms · 60 FPS resize · no freeze at 1,000+ events/sec).

| Mechanism | Rule |
|---|---|
| **Batching** | Models expose `appendBatch(std::span<const T>)` with exactly one `beginInsertRows` / `endInsertRows` per batch. A ~100ms coalescing `QTimer` in the source flushes accumulated events (≈10 UI updates/sec under burst). |
| **Bounded memory** | Detection model cap 10,000 rows; correlation 5,000; evict oldest via `beginRemoveRows`. Log viewer: `QPlainTextEdit::setMaximumBlockCount(5000)`. |
| **Paint path** | No allocation in `paint()`. Delegates cache `QFont`, `QPen`, `QColor`, pill metrics as members. |
| **Sparklines** | Fixed `std::array<float, N>` ring buffer — no `QVector` growth per sample. |
| **Filtering** | `QSortFilterProxyModel` only; invalidate on filter change, never rebuild per insert. |
| **Scroll safety** | Auto-scroll suspends when the user leaves the bottom; show a resume affordance. |

Tables are `QAbstractTableModel` subclasses — **never** `QTableWidget`. `QTableWidget` welds data to the view and makes the mock-to-collector swap a rewrite.

---

## 7. Engineering standards (GUI scope)

- **C++20 floor**, C++23 where the toolchain supports it; guard beyond `std::span` / designated initializers with feature-test macros.
- SOLID · RAII · smart pointers (non-parented) · DI · SRP · composition over inheritance · interface-driven design.
- Avoid: globals, singleton abuse, tight coupling, circular deps, hardcoded constants, widget-owned business logic.

---

## Related docs

- [`docs/BRIEF.md`](../BRIEF.md) — parent brief (Parts I–III)
- [`docs/design/desktop-design-spec.md`](../design/desktop-design-spec.md) — visual / Qt presentation contract
- [`docs/roadmap/issue-018.md`](../roadmap/issue-018.md) — scope and acceptance for the first desktop slice
