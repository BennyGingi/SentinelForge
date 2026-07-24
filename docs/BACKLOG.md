# SentinelForge — Backlog

Not a duplicate of the roadmap docs — this is the working list of what's actually open right now. See [`docs/roadmap/roadmap.md`](roadmap/roadmap.md) for the long-range capability map and [`docs/PROJECT_STATUS.md`](PROJECT_STATUS.md) for the last full audit (2026-07-24) this list is grounded in.

## Now

- **Issue #038 — Sigma condition support.** Empirically confirmed (not just from reading source — by attempting to load a real multi-selection rule and getting hard-rejected): `SigmaParser`/`SigmaTranslator` support exactly one *unnamed* `selection` map, with exactly one `process_name` and one `command_line|contains`. Named selections, multiple selections, `and`/`or`/`not`, and `1 of selection*` are all hard-rejected — not degraded, not ignored, the whole rule fails to load.
  - **Scope:** named and multiple selections; `and`/`or`/`not`; `1 of` / `all of` with wildcards; keywords; field lists as implicit OR; the common modifiers (`startswith`, `endswith`, `re`, `all`).
  - **Priority: above #025 (threat intel) and #026 (AI assistant).** Neither is its own numbered entry in this document yet — they're the unnumbered "threat intelligence" (roadmap capability map) and "AI investigation assistant" (Later, below) items — but wherever they land, this sits above them. `README.md` names Sigma support as a core capability of the platform; supporting only flat single-selection rules means most of the public Sigma ruleset can't be loaded at all. That's a correctness gap in an advertised feature, not an enhancement request.
  - **Unblocks:** the two sub-gaps noted below — `sc.exe config` (needs OR with `create` to cover T1543.003 fully) and the bitsadmin download vector (needs its own selection alongside certutil's, or true multi-process OR, to cover T1105/T1218 fully).
- **`CollectorTelemetrySource`** — the desktop console currently runs only against `MockTelemetrySource`. Wiring real collector output (detections, `CorrelationAlert`s, stats) into the GUI through the same `ITelemetrySource` interface is the next milestone; nothing else in the GUI roadmap is blocked on it, but it's the one that makes the console show real data.
- **Remaining detection coverage gaps** — T1543.003 and T1105/T1218 got real Sigma rules (`sigma-rules/sc_service_creation.yml`, `sigma-rules/certutil_lolbin_download.yml`), but each rule is intentionally narrower than the full technique, a direct consequence of the Sigma translator not supporting OR conditions (see below):
  - `sc_service_creation.yml` matches `sc.exe create` only — reconfiguring an existing service's binary path (`sc.exe config`, an equally valid T1543.003 persistence technique) isn't covered.
  - `certutil_lolbin_download.yml` matches certutil.exe fetching a URL — bitsadmin.exe's equivalent `/transfer` download vector isn't covered (different process_name; Phase 1's `process_name` is single-value-exact-match, so it needs its own rule file).

## Next

- Issue #022 remainder: interaction polish (critical-row flash, inspector-open animation, reduced-motion setting) and the alignment audit — explicitly deferred out of the branding pass that shipped the icon/logo/About dialog.
- Full Detections / Correlation investigative pages (currently placeholders) — search, multi-column filtering, column chooser, export.
- MITRE ATT&CK page, Threat Timeline page, Infrastructure page (Docker + Kubernetes tabs) — all placeholder stubs today.

## Later

- REST API, Prometheus/Grafana, cloud deployment.
- `detector-python/`, `k8s/` — reserved directories, no implementation yet.
- Sysmon / Linux event collection (today's `EventMonitor` is a directory-polling JSON ingester, not an OS-level sensor).
- AI investigation assistant (#026 candidate), threat hunting workspace.

## Known, accepted limitations

- `DetectionModel`/`CorrelationModel` batch-eviction: fixed to respect the cap under a single oversized batch (was a real bug; see `SingleOversizedBatchRespectsCapacityCap` tests).
- Sigma support is Phase 1 only: no `and`/`or`/`not`/aggregations, no field modifiers beyond `|contains`. Confirmed by directly attempting to load a `1 of selection*` rule — `SigmaLoader` rejects it outright with "Missing required field: selection" and "Unsupported condition." This is the concrete reason the two gap-closing rules above are each narrower than the full technique.
