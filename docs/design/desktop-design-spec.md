# Desktop Design Specification

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part II (implementation contract).

**Single source of truth for token values:** [`cmake/theme.cmake`](../../cmake/theme.cmake). Hex values and font stacks are written only there. `Theme.h` and `dark.qss` are generated via `configure_file`. Tables below mirror the brief; treat them as documentation of the generated set, not a second palette to edit by hand.

Rationale companions: [`color-system.md`](color-system.md), [`typography.md`](typography.md).

---

## 1. Design intent

This is a **12-hour-shift instrument panel**, not a marketing dashboard. It sits on a second monitor and gets glanced at every few minutes. The main view answers, in under two seconds of peripheral vision: *is the collector healthy, and did anything just fire that I need to look at?*

Three consequences:

1. **Low luminance, low saturation at rest.** Nothing is bright unless it means something. A calm screen makes a red row impossible to miss.
2. **Machine facts look like machine facts.** Timestamps, PIDs, paths, hashes, and MITRE technique IDs render in monospace. Human-authored labels — rule names, behavior names, headers, nav — render in the UI sans. This split is the console's signature.
3. **Severity is readable without color.** Color is never the only signal. Every severity carries a text label and a positional cue.

---

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

**Hard rule:** the accent means *interactive or active*. It never indicates a threat, a status, or a metric magnitude.

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

---

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

---

## 4. Spacing, sizing, radius

- Base unit **4px**. Only 4 / 8 / 12 / 16 / 24 / 32.
- Panel padding **16px** · panel gap **12px** · window margin **12px**.
- Row height **32px** · header height **36px** · cell padding `8px 12px`.
- Nav rail **56px** collapsed, **200px** expanded. Status strip **64px**. Inspector **380px**.
- Radius: **6px** panels, **4px** chips/inputs/buttons, **10px** severity pills. Nothing else is rounded.
- Borders are always **1px**.

---

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

**The inspector is reserved now and populated later.** Build it as a collapsible `QWidget` in a `QSplitter`, hidden until a row is activated, containing a placeholder. Persist splitter sizes, window geometry, and inspector state via `QSettings`. Splitters get sensible `setStretchFactor` values and non-zero minimums.

Rail order: Dashboard · Detections · Correlation · MITRE ATT&CK · Threat Timeline · Infrastructure · (spacer) · Settings.

---

## 6. Component specs

### Status chip

`BgRaised`, 1px `BorderSubtle`, 4px radius, padding `8px 12px`. Micro label in `TextSecondary` above a mono value in `TextPrimary`. The collector chip prefixes an 8px status dot driven by `connectionStateChanged`. Chips live in a `QHBoxLayout` that wraps — never fix their widths.

### Detection table — signature element

A **3px severity rail** runs the full height of each row's left edge, flush to the panel border. Paint it in a `QStyledItemDelegate`; never set item backgrounds directly.

- Severity pill: severity background and foreground, 10px radius, `2px 8px` padding, 11px/600 uppercase.
- Timestamp, process, technique columns: **mono**; rule name: UI sans.
- Technique: `T1059.001` in `TextSecondary` mono; technique name as tooltip.
- `setShowGrid(false)` · `setAlternatingRowColors(false)` · vertical header hidden · `SelectRows` · `NoEditTriggers` · `setUniformRowHeights(true)`.
- Hover → `BgRaised`. Selected → `BgRaised` plus a 2px `Accent` inset inside the severity rail.
- Header: `BgRaised`, `TextSecondary`, micro-label type, 1px bottom border.
- `QHeaderView`: `ResizeToContents` on timestamp/severity/technique, `Stretch` on rule and process.
- Row activation opens the inspector.

### Correlation table

Same chrome. Confidence: horizontal bar — track `BgRaised`, fill `Accent`, 4px tall, 6px radius — percentage in mono beside it. **Never color the confidence bar by value.** Confidence is not severity.

### Performance panel

Four stat blocks in a 2×2 `QGridLayout`: micro label, hero mono value, unit in `TextMuted`, 28px `SparklineWidget` (`QPainter`, 1px `Accent` stroke, no fill). CPU and memory add a 4px progress track. Values update on a `QTimer`.

### Log viewer

`QPlainTextEdit`, read-only, `BgInset`, mono 13px, line height 1.5, `setMaximumBlockCount(5000)`. 2px severity spine in the left margin. Level tokens use severity foreground; body `TextPrimary`; timestamps `TextMuted`. Header: level filter + clear. Auto-scroll pauses on manual scroll.

### Empty and error states

Every table ships an empty state: centered, `TextMuted`, 13px — what will appear and what to do. Errors state what failed and the next step. Disconnection is an error state, not a blank table.

---

## 7. Qt implementation rules

- `QApplication::setStyle(QStyleFactory::create("Fusion"))` **first**, then a matching `QPalette`, then the QSS.
- One stylesheet, generated to `dark.qss`, compiled into `sentinelforge.qrc`, applied once at `QApplication` level. No scattered `setStyleSheet` calls.
- No color literal in C++ outside generated `Theme.h`.
- Icons: bundled monochrome **SVG**, tinted via `QIcon` and palette. No emoji, no `QStyle::standardIcon`, no PNG.
- High-DPI: `setHighDpiScaleFactorRoundingPolicy(PassThrough)` before `QApplication`. Verify at 100%, 125%, 150%.
- QSS: no transitions, shadows, or opacity animation. Flat contrast shifts on hover.
- Focus always visible: 1px `Accent` outline. Full keyboard tab order.
- Tables are `QAbstractTableModel` subclasses — **never** `QTableWidget`.

---

## 8. Do not

- Pure black `#000` or pure white `#FFF`.
- Gradients, bevels, drop shadows, glows.
- Emoji as icons or status.
- Gridlines or zebra striping.
- Centered text in data columns — text left-aligns, numbers right-align.
- A second accent, or accent on anything non-interactive.
- Severity by color alone.
- Hardcoded pixel positions, fixed widget sizes, `move()` calls. Layouts only.
- `!important` in QSS, or selectors that cancel each other out.
- Default Qt blue selection highlight.

---

## 9. Starter QSS

Source template: `gui/resources/styles/dark.qss.in` (generated by CMake from `cmake/theme.cmake`). Do not edit the generated output.

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
