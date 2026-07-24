# Typography

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part II §1, §3.  
Font stacks and related tokens: **generated from [`cmake/theme.cmake`](../../cmake/theme.cmake)** (single source of truth). Size/weight tables below document the design contract; do not invent a second hand-maintained stack in QSS or C++.

See also: [`desktop-design-spec.md`](desktop-design-spec.md), [`color-system.md`](color-system.md).

---

## Rationale

The console’s signature is a strict split between **machine text** and **human text**.

| Kind | What | Face |
|---|---|---|
| **Machine** | Timestamps, PIDs, paths, hashes, MITRE technique IDs, metric digits, log bodies | Monospace — JetBrains Mono (primary), Cascadia Mono / Consolas fallback |
| **Human** | Rule names, behavior names, panel titles, nav labels, chip captions | UI sans — Inter (primary), Segoe UI Variable Text / Segoe UI fallback |

Why this matters on a SOC second monitor:

- Monospace makes IDs and timestamps align and scan as *data*, not marketing copy.
- Inter (and Segoe fallbacks) keep headers and rule names readable at small sizes without competing with mono density.
- Mixing both in one table (e.g. mono technique + sans rule name) teaches the eye where to look for facts vs. meaning.

Bundle both families via `QFontDatabase::addApplicationFont` from the resource system; the listed stacks are the fallback chain when a face is missing.

---

## Font stacks (from `cmake/theme.cmake`)

> Values and stack names should match what CMake generates into `Theme.h` / `dark.qss`. Treat this section as rationale + contract, not a place to invent new families.

```
UI      "Inter", "Segoe UI Variable Text", "Segoe UI", sans-serif
Mono    "JetBrains Mono", "Cascadia Mono", "Consolas", monospace
```

---

## Role scale

| Role | Size | Weight | Tracking | Notes |
|---|---|---|---|---|
| Micro label | 11px | 600 | +0.08em | UPPERCASE. Chip captions, eyebrows. |
| Table cell | 12px | 400 | 0 | Mono for machine columns. |
| Body | 13px | 400 | 0 | Log lines (mono), descriptions. |
| Panel title | 14px | 600 | +0.02em | Sentence case: "Recent detections". |
| Metric value | 22px | 600 | -0.01em | Mono, tabular figures so digits don't jitter. |
| Hero metric | 30px | 600 | -0.02em | Mono. Events Processed, Detections. |

**Emphasis:** never bold a whole row or table. Emphasis comes from color and position (severity rail, accent selection), not weight spam.

**Alignment:** text left-aligns; numbers right-align. No centered data columns.

---

## Machine vs human — apply everywhere

| Context | Face |
|---|---|
| Status chip values, metric / hero numbers | Mono |
| Status chip / panel micro labels, nav, titles | UI sans |
| Detection: timestamp, process, technique ID | Mono |
| Detection: rule name | UI sans |
| Log viewer body | Mono 13px |
| Empty / error copy | UI sans, `TextMuted` |

If a new column is an identifier or timestamp, default to mono. If it is a human-authored name or label, default to UI sans.
