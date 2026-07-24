# SentinelForge — Backlog

Not a duplicate of the roadmap docs — this is the working list of what's actually open right now. See [`docs/roadmap/roadmap.md`](roadmap/roadmap.md) for the long-range capability map and [`docs/PROJECT_STATUS.md`](PROJECT_STATUS.md) for the last full audit (2026-07-24) this list is grounded in.

## Now

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
- AI investigation assistant, threat hunting workspace.

## Known, accepted limitations

- `DetectionModel`/`CorrelationModel` batch-eviction: fixed to respect the cap under a single oversized batch (was a real bug; see `SingleOversizedBatchRespectsCapacityCap` tests).
- Sigma support is Phase 1 only: no `and`/`or`/`not`/aggregations, no field modifiers beyond `|contains`. Confirmed by directly attempting to load a `1 of selection*` rule — `SigmaLoader` rejects it outright with "Missing required field: selection" and "Unsupported condition." This is the concrete reason the two gap-closing rules above are each narrower than the full technique.
