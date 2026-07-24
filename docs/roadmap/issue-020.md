# Issue #020 — Investigation workspace

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part I §5, §8; Part II §5–6.  
Architecture: [`docs/architecture/gui-architecture.md`](../architecture/gui-architecture.md).

**Prerequisite:** Issue #019 P0 closed (panel surfaces + mock seed).

Turn the placeholder inspector into where an analyst works. Dashboard answers *did something fire?*; this issue answers *what happened, why, and what next?*

---

## Scope

### 020.1 — Data model additions *(blocks all other sections)*

Extend `Detection` with inspector fields — **populated by the source, never derived in the GUI**:

- `ruleDescription`, `detectionReason`, `recommendedAction`, `confidence`, `occurrences`
- `host`, `user`, `pid`, `parentPid`, `parentProcessName`, `commandLine`
- `QVector<Ioc> iocs` — pre-extracted by collector/mock
- `QVector<QString> relatedIds` — resolved by collector; GUI looks up by id only
- `rawEventJson` — displayed verbatim in raw-event section only

Add `RuleInfo` aggregate and `ITelemetrySource::ruleInfo(QString ruleId)` — the one permitted synchronous accessor for static rule metadata. Document the exception in `gui-architecture.md`.

**No GUI extraction.** No `extractIocs(rawJson)`, no inferring related events. IOCs and related links arrive pre-computed on the struct.

### 020.2 — Timestamps

- Store `qint64 timestampMs` (epoch UTC); format at paint time only.
- Column: absolute mono `14:22:07.412`.
- Tooltip: full date, absolute time with offset, relative suffix.
- Relative suffix only on Dashboard rows &lt; 60s old — never on Detections page.
- One 1 Hz timer on the view; `dataChanged` for visible viewport rows only (DisplayRole).

### 020.3 — Inspector layout

Widen inspector to **420px**. Six collapsible sections in `QScrollArea`:

1. **Triage** *(expanded)* — severity, rule, action, confidence, occurrences, host, user, timestamp.
2. **Process lineage** *(expanded)* — parent → process, command line (mono, copy button).
3. **Related detections** — up to 8 rows from `relatedIds`; click navigates; back stack.
4. **Attribution** — MITRE block (020.4) + rule detail link (020.5).
5. **Indicators** — IOCs grouped by type, per-row copy.
6. **Raw event** *(collapsed)* — read-only JSON, copy-all.

Micro-label section headers; 1px `BorderSubtle` separators.

### 020.4 — MITRE presentation

Three-line block: technique name, sub-technique name, ID in mono. Description + external-link button.

Bundled `gui/resources/mitre/techniques.json` — presentation reference data, not detection logic.

Validate technique ID against `^T\d{4}(\.\d{3})?$` before building URL; use `QDesktopServices::openUrl`.

### 020.5 — Rule detail view

Modeless dialog or second inspector tab (not modal). All `RuleInfo` fields; **known false positives** and **investigation notes** above logic summary.

## Deferred

AI investigation summary · collector-owned threat scoring · real `CollectorTelemetrySource`.

---

## Acceptance checklist

- [ ] Every inspector field read from `Detection`; only the raw-event viewer touches `rawEventJson` (verbatim display).
- [ ] No function in `gui/` extracts, infers, or derives IOCs or related events.
- [ ] Timestamps stored as epoch ms, formatted at paint time; no formatted strings in models.
- [ ] Relative-time refresh emits `dataChanged` only for visible rows — verified at 10,000 rows.
- [ ] MITRE link built only from regex-validated technique ID.
- [ ] Related-event navigation supports going back.
