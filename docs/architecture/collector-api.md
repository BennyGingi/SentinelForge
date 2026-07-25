# Collector HTTP API

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) Part I §4 — `CollectorTelemetrySource` is the future `ITelemetrySource` implementation that reads real collector output instead of `MockTelemetrySource`. This is that data source's other end: a read-only HTTP API the collector serves, Issue #035 phase 1. **The GUI does not consume this yet** — building the endpoint is this phase's whole scope; `CollectorTelemetrySource` itself is separate, tracked work (see `docs/BACKLOG.md`).

---

## What it's for

The collector already produces detections, correlation alerts, and log lines while running (`EventMonitor`/`Application`, watching `events/incoming/`). Nothing outside the process could see any of that except by tailing `detections.json` or the log file. This API exposes it over HTTP so a client — the desktop console, eventually, or `curl` today — can poll for what's happened since it last checked, without the collector needing to know or care whether anything is listening.

## Architecture

```
detection pipeline (EventMonitor / Application)
        │  AppendDetection / AppendAlert / AppendLog
        ▼
   TelemetryStore  (bounded ring buffers + cursor, mutex-guarded)
        ▲
        │  DetectionsSince / AlertsSince / LogsSince
   ApiServer (httplib::Server, own thread)
        │
        ▼
   GET /health, /stats, /detections, /correlations, /logs
```

`ApiServer` only ever *reads* from `TelemetryStore`; the detection pipeline only ever *writes* to it. Neither knows the other exists — `TelemetryStore` is the entire coupling, and every write only holds a mutex for a `push_back` + bounded `pop_front`, never I/O. **A slow or absent HTTP client cannot slow down detection.** No mutation endpoints exist at all: this API cannot influence what the collector detects, matching the GUI-side rule (BRIEF.md Part I §5: no business logic outside the collector) mirrored from the server side — nothing about serving this data should let a client steer detection behavior.

`collector_core` gained one new runtime dependency: [`cpp-httplib`](https://github.com/yhirose/cpp-httplib) (header-only, MIT, fetched via the same `FetchContent` pattern as `nlohmann_json`/`yaml-cpp`). No Qt anywhere in `collector-cpp` — that boundary is load-bearing (BRIEF.md Part I §5).

## Cursor semantics — polling, not push

Every list endpoint (`/detections`, `/correlations`, `/logs`) takes `?since=<n>` and returns `{cursor, more, items[]}`. `cursor` is always the sequence number a client should pass as `since` on its *next* request.

**Why cursor polling over WebSockets:**

- **Reconnection is free.** A dropped WebSocket needs a reconnect handshake and, if you want no data loss, some server-side notion of "what has this client already seen" — a session, a subscription, something stateful per connection. A dropped HTTP poller just... polls again with the same `since` it already had. There is no session to lose, because there was never a session to begin with.
- **The server stays trivially stateless per client.** `TelemetryStore` doesn't track which clients exist or what they've seen — it just answers "give me everything after N" for whatever N is asked. Any number of clients (or zero) can poll concurrently with no additional server-side bookkeeping per client.
- **A slow poller can't apply backpressure to the collector.** With a push model (WebSocket send queue), a slow or stuck client can make the server's outbound buffer grow — the classic slow-consumer problem, and exactly the kind of thing that must never touch the detection pipeline (see "must never block on a slow client" in `TelemetryStore`'s own contract). With polling, a slow client just... polls less often. The store evicts on its own schedule regardless of who's asked for what.
- **Debugging is `curl`.** No client library, no handshake, no framing — `curl 'http://127.0.0.1:8787/detections?since=0'` is the whole client.

The tradeoff, honestly: polling means latency is bounded by poll interval, not push-instant. For an operator console glancing at a screen every few seconds, that's the right tradeoff. It would not be the right one for, say, a sub-second alerting pipeline — not this system's job.

### Pagination

Every list response caps at 500 items (`TelemetryStore::kMaxPageSize`) and sets `"more": true` when more are available. Critically, `cursor` on a truncated page is the sequence of the **last item actually returned**, not the buffer's live head — a client working through a large backlog pages through it in order; it is never silently skipped ahead past items it hasn't seen. Keep calling with the returned `cursor` while `more` is `true`.

### The evicted-window case

Buffers are bounded (10,000 detections / 5,000 correlation alerts / 10,000 logs, oldest evicted first). If a client's `since` predates everything still retained — it was offline a while, or this is its first request with `since=0` against a long-running collector — it simply gets everything currently available, starting from the oldest retained item, with no error and no special case. There's no way to ask for data that's been evicted; the API was never designed to be a durable event log, just a bounded recent-activity buffer.

## Endpoints

All responses are `application/json`. No authentication — the API binds `127.0.0.1` only by default (see Config below); this is a security tool, not a service meant to be reachable from the network.

### `GET /health`

```json
{ "status": "ok", "uptime_seconds": 142, "version": "0.1.0" }
```

### `GET /stats`

```json
{
  "rules_loaded": 6,
  "sigma_rules_loaded": 2,
  "correlation_rules_loaded": 1,
  "events_processed": 38,
  "detections": 12,
  "correlation_alerts": 3,
  "events_per_second": 0.4,
  "pipeline_latency_ms": 1.8
}
```

`events_per_second` is a 5-second rolling rate, computed on demand from recent event timestamps — not a background timer, no thread beyond the two the collector already has (pipeline + `ApiServer`). `pipeline_latency_ms` is the most recent event's detection+correlation time, not an average.

### `GET /detections?since=N`

```json
{
  "cursor": 12,
  "more": false,
  "items": [
    {
      "sequence": 12,
      "id": "det-12",
      "timestamp_ms": 1784557927000,
      "severity": "High",
      "rule_name": "Suspicious PowerShell",
      "mitre": "T1059.001",
      "process_name": "powershell.exe",
      "parent_process": "explorer.exe",
      "command_line": "powershell.exe -enc SGVsbG8=",
      "host": "WORKSTATION-07",
      "user": "CORP\\jsmith",
      "reason": "process_name matched 'powershell.exe' and command_line contains '-enc'",
      "pid": 0,
      "parent_pid": 0
    }
  ]
}
```

**`pid`/`parent_pid` are always `0` in this phase.** `Event.h` carries a PID, but `EventNormalizer` never copies it into `NormalizedEvent` — the collector's own normalized pipeline drops it before it reaches anything downstream, including this API. Extending `NormalizedEvent`'s schema is a bigger change than building this endpoint; left as a known gap rather than fabricated. `id` has the same story from the other direction: neither `Rule` nor `DetectionResult` carries a stable id (only `RuleName()`), so `TelemetryStore` generates one from its own sequence (`"det-<sequence>"`) rather than push that problem onto every caller — same reasoning as the regression harness's `RuleName()`-as-identity choice, see `docs/architecture/regression-testing.md`.

### `GET /correlations?since=N`

Same envelope shape; items are `CorrelationAlert`-derived (`title`, `description`, `severity`, `confidence`, `matched_event_count`, `mitre_techniques`).

### `GET /logs?since=N`

Same envelope shape; items are `{sequence, timestamp_ms, level, component, message}` — currently populated with one summary line per processed event (`"Processed event: ... (N match(es), M correlation alert(s))"`), not a full mirror of every `Logger` line. Wiring the full log stream through would mean `Logger` gaining a sink/callback mechanism it doesn't have today; out of scope for phase 1.

## Config

```json
"api": {
  "enabled": true,
  "bind_address": "127.0.0.1",
  "port": 8787
}
```

- `enabled` — the collector runs headless with the API off; `ApiServer::Start()` is always safe to call and just no-ops when disabled, so `Application` doesn't need an `if` at the call site.
- `bind_address` — defaults to loopback-only, deliberately, not `0.0.0.0`. This is a security tool; nothing about it should be reachable from the network by default. Configurable for the future container case (the collector and its API consumer running in the same pod/network namespace, or an explicit operator choice to expose it).
- `port` — default `8787`.

## Threading and shutdown

`ApiServer` owns a single `std::thread` running `httplib::Server::listen()` (a blocking call — that's the thread's entire job). `Application::Run()` starts it after configuration loads and stops it before returning: `Stop()` calls `server_->stop()` (which unblocks `listen()`) then joins the thread. No hang on exit, and the detection pipeline's own timing is unaffected — it was never on this thread to begin with.

## Verifying it works

```
.\scripts\build.ps1
.\scripts\run.ps1
```

Then, from another terminal:

```
curl http://127.0.0.1:8787/health
curl http://127.0.0.1:8787/stats
curl "http://127.0.0.1:8787/detections?since=0"
```
