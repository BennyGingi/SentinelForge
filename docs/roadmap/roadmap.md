# Platform Roadmap

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part I §1–2, §8.

SentinelForge is becoming a modern security platform — not “another GUI.” The desktop console is one component. Design today so none of the items below require redesigning the application (navigation factory, `ITelemetrySource`, reserved inspector pane).

---

## High-level capability map

| Area | Direction |
|---|---|
| **Collection** | Windows and Linux event collection · Sysmon support |
| **Detection** | Sigma rules · native detection rules · behavioral detection · threat intelligence |
| **Analytics** | Correlation engine · threat hunting workspace |
| **Export & API** | JSON export · REST API |
| **Console** | Desktop console (Qt) — live dashboard + investigative pages |
| **Ops / cloud** | Docker · Kubernetes · Prometheus · Grafana · cloud deployment |
| **Assist** | AI investigation assistant |

---

## Desktop console trajectory

**Done (Issue #018):** independent `gui/` target, mock telemetry, Dashboard fully built, rail placeholders, inspector reserved. See [`issue-018.md`](issue-018.md).

**Now (Issue #019):** visual QA pass — panel surface hierarchy, mock seed, dashboard polish. Hard prerequisite for #020. See [`issue-019.md`](issue-019.md).

**Next — SOC investigation experience (#020–#022):**

| Issue | Focus |
|---|---|
| [#020](issue-020.md) | Investigation workspace — data model, timestamps, inspector, MITRE, rule detail |
| [#021](issue-021.md) | Operator controls — search, pause (view-only), keyboard, copy, threat summary |
| [#022](issue-022.md) | Identity and polish — logo, branding, interaction motion, `QSettings` |

**Later integration:** `CollectorTelemetrySource` behind the same `ITelemetrySource` (one-line swap in `main.cpp`).

**Roadmap pages:** full Detections / Correlation investigative views · MITRE ATT&CK · Threat Timeline · Infrastructure (Docker + Kubernetes as tabs) · Settings · export.

---

## Design invariants that protect the roadmap

1. **GUI never embeds collector business logic** — swap and grow the backend without rewriting widgets.
2. **Batched, threaded `ITelemetrySource`** — REST/IPC later without interface rewrite.
3. **Lazy page factory** — new rail entries without startup regression.
4. **Inspector reserved in the layout** — AI / hunting detail without a three-panel rewrite.
5. **Tokens only in `cmake/theme.cmake`** — visual evolution without palette drift across QSS and C++.

---

## Related

- [`docs/BRIEF.md`](../BRIEF.md)
- [`docs/architecture/gui-architecture.md`](../architecture/gui-architecture.md)
- [`docs/roadmap/issue-018.md`](issue-018.md)
- [`docs/roadmap/issue-019.md`](issue-019.md)
- [`docs/roadmap/issue-020.md`](issue-020.md)
- [`docs/roadmap/issue-021.md`](issue-021.md)
- [`docs/roadmap/issue-022.md`](issue-022.md)
