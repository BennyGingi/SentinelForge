# Issue #021 — Operator controls

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part I §7–8.  
Architecture: [`docs/architecture/gui-architecture.md`](../architecture/gui-architecture.md).

**Prerequisite:** Issue #020 (investigation workspace).

Investigation depth without speed is still a demo. This issue is how fast an analyst can move.

---

## Scope

### 021.1 — Search and filter

Single search field + severity combobox above the detection table.

- Field-scoped syntax: `process:powershell severity:high technique:T1059 user:svc_backup`; bare terms match all searchable fields.
- Lowercase search blob cached per row at insert time.
- `QSortFilterProxyModel` + custom `filterAcceptsRow`; **150 ms debounce** on text change.
- Result count beside field (`412 → 27 shown`); inline clear.
- Filtered-empty copy distinct from pipeline-empty.

### 021.2 — Pause and resume

**Pause is view-only; the source never stops.**

- Batches buffer in model staging while paused (caps: 5,000 detections, 2,000 alerts, 5,000 log lines).
- Overflow: retain all Critical/High; drop oldest Medium/Low/Info first; count every drop.
- Banner: `Paused · 1,284 buffered · 312 dropped` — dropped count always disclosed.
- Resume flushes in 200-row batches via zero-delay timer.

The collector never loses events; the **view buffer** is bounded and may drop display rows.

### 021.3 — Keyboard workflow

| Key | Action |
|---|---|
| `↓` / `↑`, `j` / `k` | Move row selection |
| `Enter` | Open inspector |
| `Esc` | Close inspector or clear search |
| `/`, `Ctrl+F` | Focus search |
| `Space` | Toggle pause |
| `c` | Copy row as TSV |
| `Shift+C` | Copy row as JSON |
| `Ctrl+1`…`Ctrl+7` | Switch nav page |

Viewport scrolls to keep selection visible. Every shortcut also exists as menu/button.

### 021.4 — Copy actions

Row context menu + shortcuts: copy as tab-separated text; copy as JSON. Per-field copy on command line, IOCs, raw JSON.

### 021.5 — Threat summary card

Status-strip card with explicit formula over last 15 minutes:

```
score = Σ weight(severity) × decay(age)
weight: Critical 100 · High 25 · Medium 5 · Low 1 · Info 0
decay:  linear 1.0 → 0.0 over 15 min
level:  ≥100 Critical · ≥40 High · ≥10 Medium · else Low
```

Hysteresis: promote after 10s above threshold; demote after 60s below. Document as presentation heuristic in `gui-architecture.md`.

### 021.6 — Status and performance polish

- Six status chips: Collector, Rules, Events/sec, Total events, Detections, Correlation alerts.
- Smooth counter animation (300 ms ease-out; suppress when delta &gt; 50%).
- Performance panel: value + unit, 60s sparkline, avg/peak in `TextMuted`; fixed `std::array<float, 60>` ring buffers.

## Deferred

Collector-owned threat scoring · export.

---

## Acceptance checklist

- [ ] 12-character query at 10,000 rows — no perceptible lag; re-filter at most once per 150 ms.
- [ ] Filtered-empty and pipeline-empty states show different copy.
- [ ] Pausing does not stop the source — event counter climbs while paused.
- [ ] Overflow during pause retains all Critical/High and reports non-zero dropped count.
- [ ] Resuming a full 5,000-row buffer does not block UI &gt; one frame.
- [ ] Full triage loop (find, select, read, copy) completable without mouse.
- [ ] Threat level does not change more than once in any 10-second window under steady load.
