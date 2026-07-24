# SentinelForge — Backlog

Not a duplicate of the roadmap docs — this is the working list of what's actually open right now. See [`docs/roadmap/roadmap.md`](roadmap/roadmap.md) for the long-range capability map and [`docs/PROJECT_STATUS.md`](PROJECT_STATUS.md) for the last full audit (2026-07-24) this list is grounded in.

## Now

- **`CollectorTelemetrySource`** — the desktop console currently runs only against `MockTelemetrySource`. Wiring real collector output (detections, `CorrelationAlert`s, stats) into the GUI through the same `ITelemetrySource` interface is the next milestone; nothing else in the GUI roadmap is blocked on it, but it's the one that makes the console show real data.
- **Detection coverage gaps** — the regression harness (`collector-cpp/regression/`) documents these rather than papering over them:
  - T1543.003 (service persistence via `sc.exe`) — no rule.
  - T1105 / T1218 (LOLBin ingress via `certutil`/`bitsadmin`) — no rule.

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
- Sigma support is Phase 1 only: no `and`/`or`/`not`/aggregations, no field modifiers beyond `|contains`.
