# SentinelForge — Project Status

Audit date: 2026-07-24. Repo state: branch `main` @ `1fc1562`. This report was produced by reading `docs/BRIEF.md` and `docs/roadmap/issue-01[89].md` / `issue-02[012].md` as the specification, then verifying every claim against the actual repository — building the collector, running its test suite, and grepping/reading source line-by-line. No source files were modified to produce this report. Every claim below cites a file path, and a line number where one is meaningful. Where something could not be verified (e.g. runtime GUI behavior, Docker build), that is stated explicitly rather than assumed.

---

## 1. Inventory

| Component | Language / build | Files | Builds? | Tests? |
|---|---|---|---|---|
| `collector-cpp/` | C++17, CMake ≥3.20 (`collector-cpp/CMakeLists.txt:1,5`) | 22 `.cpp` in `src/` + 22 `.h` in `include/` + 9 test files in `tests/` = 53 | **Yes** — verified live | **Yes** — 65/65 pass |
| `gui/` | C++20, CMake + Qt6 ≥6.5 (Core/Gui/Widgets/Svg) (`gui/CMakeLists.txt:5,13`) | 25 `.cpp` (`src/`) + 29 `.h`/`.h.in` (`include/`) + 12 resource files = 66 | **Yes** — prior build verified via artifacts | **No** — zero test files |
| `detector-python/` | — | **0 files**, not git-tracked | N/A | N/A |
| `docker/` | Dockerfile (multi-stage) | 1 file (`docker/collector.Dockerfile`) | Not attempted (no Docker daemon invoked); recipe inspected and is plausible | N/A |
| `k8s/` | — | **0 files**, not git-tracked | N/A | N/A |
| `rules/` | JSON data | 3 files | N/A (data, not code) | Consumed and exercised by `RuleValidatorTests` |
| `sigma-rules/` | Sigma YAML | 1 file | N/A | Consumed and exercised by `SigmaTests` |
| `tests/` (top-level) | — | **0 files**, not git-tracked | N/A | N/A |

Also empty/stub, not git-tracked: `dashboard/`, `sample-attacks/`, `docs/api/`, `docs/diagrams/`. `events/{incoming,processed,failed}/` contain only `.gitkeep` — real event JSON is gitignored (`.gitignore`: `events/incoming/*.json` etc.).

**collector-cpp detail.** Dependencies via `FetchContent`: nlohmann_json v3.11.3, yaml-cpp 0.8.0, googletest v1.15.2 (`collector-cpp/tests/CMakeLists.txt:4-13`). All logic lives in a static library `collector_core`, linked by both `collector.exe` and `collector_tests.exe` (`collector-cpp/CMakeLists.txt:32-36`). Verified build: `cmake --build collector-cpp/build --config Debug` succeeded, producing `collector-cpp/build/Debug/collector.exe` and `collector-cpp/build/tests/Debug/collector_tests.exe`. Verified tests: `ctest -C Debug` → **65/65 passed, 1.61s total**, covering `ConfigurationTests`, `DetectionEngineTests`, `EventMonitorTests`, `EventNormalizerTests`, `JsonExporterTests`, `LoggerTests`, `PerformanceProfilerTests`, `RuleValidatorTests`, `SigmaTests`.

**gui detail.** Qt resources: 8 SVG icons, `mitre/techniques.json`, `sentinelforge.qrc`/`.qrc.in`, `styles/dark.qss.in`. `gui/build/bin/Debug/sentinelforge_desktop.exe` exists, and `gui/build-out.txt` shows a completed MSBuild + `windeployqt` run (all Qt6 DLLs and plugins reported "up to date"), so the GUI has built and deployed successfully at some point. However, **`gui/build/CMakeCache.txt` is absent** even though the rest of a configured Visual Studio tree is present (`.vcxproj`, `SentinelForgeDesktop.slnx`, `CMakeFiles/`) — see §7. `gui/CMakeLists.txt` contains zero references to "test"/"Test"; there is no GUI test target of any kind.

**detector-python/ and k8s/** are literally empty — no `requirements.txt`, no `.py`, no manifests, not even a `.gitkeep`, not tracked by git at all. The roadmap's Python-detection and Kubernetes-deployment components have no implementation whatsoever.

**docker/**: single file, a real multi-stage build (Ubuntu 24.04 builder compiles `collector-cpp` via the same CMake invocation verified above; Ubuntu 24.04 runtime copies only the binary + `libstdc++6`, runs as non-root). No `docker-compose.yml` exists anywhere in the repo.

**rules/**: `mimikatz_execution.json`, `suspicious_cmd_whoami.json`, `suspicious_powershell.json` — flat JSON objects (`rule_name`, `process_name`, `command_line_contains`, `severity`, `mitre`), matching the fields `RuleParser::ParseFile` reads (`collector-cpp/src/RuleParser.cpp`).

**sigma-rules/**: `suspicious_encoded_powershell.yml` — valid Sigma (title/id/status/description/logsource/detection/condition/level/tags), matching the "supported subset" documented in `collector-cpp/README.md` (top-level fields, `command_line|contains`, `condition: selection` only).

---

## 2. What works

### collector-cpp — implemented **and** tested (65 passing GoogleTest cases)

| Feature | Implementation | Test file |
|---|---|---|
| Configuration loading (file present/missing/invalid) | `collector-cpp/src/Configuration.cpp` | `ConfigurationTests.cpp` |
| Native rule loading + validation | `RuleLoader.cpp`, `RuleValidator.cpp` | `RuleValidatorTests.cpp` |
| Live event monitoring (watches `events/incoming`, moves to `processed`/`failed`) | `EventMonitor.cpp` | `EventMonitorTests.cpp` (9 cases: startup/shutdown, empty dir, single/multi event, malformed JSON, valid JSON, continues-after-failure, file-move preserves contents, graceful stop) |
| Event normalization | `EventNormalizer.cpp` | `EventNormalizerTests.cpp` (7 cases) |
| Detection engine (native rule matching) | `DetectionEngine.cpp` | `DetectionEngineTests.cpp` |
| Sigma → native rule translation (subset) | `SigmaParser.cpp`, `SigmaLoader.cpp`, `SigmaTranslator.cpp`, `SigmaRule.cpp` | `SigmaTests.cpp` |
| JSON detection export | `JsonExporter.cpp` | `JsonExporterTests.cpp` |
| Structured logging | `Logger.cpp` | `LoggerTests.cpp` |
| Performance profiling | `PerformanceProfiler.cpp` | `PerformanceProfilerTests.cpp` |

### gui — implemented, **untested** (gui/ has zero automated tests; every item below is verified by direct code reading only, not by test execution)

| Feature | Evidence |
|---|---|
| `ITelemetrySource` abstraction, `MockTelemetrySource` on worker `QThread` | `gui/include/telemetry/ITelemetrySource.h`, `gui/src/telemetry/MockTelemetrySource.cpp` |
| Mock seeds 40 detections + 8 correlation alerts on `start()` | `MockTelemetrySource.cpp:141-200` (loop `for (int i=0;i<40;...)`, log line "Seeded 40 detections and 8 correlation alerts") |
| `DetectionModel`/`CorrelationModel`, `appendBatch` (one `beginInsertRows`/`endInsertRows` per batch), ring-buffer eviction at 10,000/5,000 | `gui/src/models/DetectionModel.cpp:121-141,192-202`; `CorrelationModel.cpp:73-116` |
| `Panel` primitive (`QFrame` + `WA_StyledBackground`, objectName `"Panel"`) wired to `QFrame#Panel` QSS | `gui/include/widgets/Panel.h`, `gui/src/widgets/Panel.cpp:6-7`, `gui/resources/styles/dark.qss.in:16` |
| Theme generation: `cmake/theme.cmake` → `configure_file` → `Theme.h` + `dark.qss` | `gui/CMakeLists.txt:23-31` |
| Inspector pane, 420px, 6 collapsible sections | `gui/src/widgets/InspectorPane.cpp:42` (width), sections at lines 97 (Triage), 146 (Process lineage), 187 (Related detections), 200 (Attribution), 228 (Indicators), 241 (Raw event) |
| MITRE catalog + technique-ID regex validation before URL build | `gui/src/mitre/MitreCatalog.cpp:47` |
| Field-scoped search (`process:`, `severity:`, `technique:`, `user:`, plus `pid:`/`ioc:`/`cmdline:`), 150ms debounce, cached lowercase blob | `gui/src/models/DetectionFilterProxy.cpp:9-57`; `gui/src/widgets/DetectionTableView.cpp:160` |
| Pause = view-only buffering, drop-count disclosure, Critical/High retained on overflow | `gui/src/ops/LiveOpsController.cpp` (caps 5,000/2,000/5,000 at `LiveOpsController.h:16-19`) |
| Full keyboard shortcut set: j/k, Enter, Esc, `/`, Ctrl+F, Space, c, Shift+C, Ctrl+1…7 | `gui/src/widgets/DetectionTableView.cpp:331-345`; `gui/src/MainWindow.cpp:178-228` |
| Threat score: weight×decay + hysteresis, exact formula match to spec | `gui/src/ops/ThreatScorer.cpp` (weights lines 9-22, decay 52-64, thresholds 24-35, hysteresis 10s/60s at lines 85/95) |
| Timestamps stored as `qint64 timestampMs`, formatted only at paint/tooltip time | `gui/include/telemetry/TelemetryTypes.h:57`; `gui/include/util/TimestampFormat.h` |
| `ruleInfo()` documented synchronous exception, called via `Qt::BlockingQueuedConnection` | `gui/include/telemetry/ITelemetrySource.h:14-30`; `gui/src/MainWindow.cpp:108` |
| `QSettings` persistence for geometry + splitter state | `gui/src/MainWindow.cpp:343-356` |

---

## 3. What's claimed but missing

- **Correlation Engine.** BRIEF.md Part I §3 (line 41) states the existing backend — "already implemented and tested" — includes a correlation engine, and the pipeline diagram (line 38) lists it explicitly. **It does not exist on `main`.** No `Correlation*` file exists under `collector-cpp/src` or `collector-cpp/include`; a stale `CorrelationEngineTests.obj` in the build tree is the only trace. Independently confirmed via git: `CorrelationEngine.h/.cpp`, `CorrelationAlert.h/.cpp`, `CorrelationRule.h`, and `CorrelationEngineTests.cpp` were added in commit `fb80b26` ("Issue #017: Add event correlation framework") on branch `feature/event-correlation` (also present on `origin/feature/event-correlation`), but `git merge-base --is-ancestor fb80b26 HEAD` returns false — **that branch was never merged into `main`**. `collector-cpp/README.md:139` independently confirms this by listing "No correlation / timeframe / pipelines" under its own Limitations section. The GUI's `CorrelationAlert` type and correlation table are presentation-only against mock data; there is no real correlation engine feeding them.
- **`detector-python/`** — completely empty. No Python detection tooling exists anywhere.
- **`k8s/`** — completely empty. No Kubernetes manifests despite BRIEF.md §2 and the "Infrastructure" page concept (Issue #018 §8) explicitly planning for Docker+K8s.
- **`docs/architecture/backend.md` and `docs/architecture/event-pipeline.md`** — BRIEF.md §9 lists these as required documents under `docs/architecture/`. Neither exists; only `gui-architecture.md` and `RFC-0001.md` are present.
- **Issue #022 (identity and final polish) — entirely unimplemented.** No `gui/resources/branding/` directory, no `logo.svg`, no `.ico`, no Windows `.rc`/`IDI_ICON1`, no `QApplication::setWindowIcon` call, no About dialog (`grep -i about gui/src gui/include` → zero hits). The Settings page is still a generic placeholder (`gui/src/MainWindow.cpp:164-171`). This matches git history: the single squashed commit `5d9bfaf` is titled "Issue #018-021" — issue #022 was never started.
- **REST API, Prometheus, Grafana, cloud deployment, AI investigation assistant, threat hunting workspace** — all named in BRIEF.md §2's roadmap; none have any implementation anywhere in the repo.
- **Sysmon / OS-level sensor integration.** `EventMonitor.cpp` watches a filesystem directory (`events/incoming`) for dropped JSON files — it is a directory-polling ingester, not a Sysmon/ETW/ETL consumer. No Linux-specific collection code exists.

### Issue #019 specific checks (asked explicitly)

- **`Panel` class / `QFrame#Panel` styling — EXISTS, not missing.** `gui/include/widgets/Panel.h`, `gui/src/widgets/Panel.cpp:6-7` (`setObjectName("Panel")`, `setAttribute(Qt::WA_StyledBackground, true)`), styled at `gui/resources/styles/dark.qss.in:16`.
- **`MockTelemetrySource` seed data — EXISTS, not missing.** `MockTelemetrySource.cpp:141-200` seeds exactly 40 detections and 8 correlation alerts on first `start()`, spread over the last 15 minutes, before regular streaming begins.

Both Issue #019 P0 items are done. What's missing from the broader roadmap is elsewhere (correlation engine, detector-python, k8s, issue #022), not in #019.

---

## 4. Architecture compliance

**BRIEF.md Part I §5 — no parsing/detection/correlation/rule logic in `gui/`: compliant, no violations found.**
- Only one `QJsonDocument::fromJson` call exists in all of `gui/src` and `gui/include`: `gui/src/mitre/MitreCatalog.cpp:24`, which loads the bundled `gui/resources/mitre/techniques.json` presentation reference file — explicitly permitted by `docs/architecture/gui-architecture.md` §4 ("MITRE technique ID → name/description lookup from bundled ... techniques.json — presentation reference data, not detection logic").
- `rawEventJson` is displayed verbatim only in the Inspector's "Raw event" section (`InspectorPane.cpp:241`); no function extracts IOCs or infers relationships from it — IOCs and `relatedIds` arrive precomputed on `Detection` (per `TelemetryTypes.h`) and are only looked up, never derived.
- No `SigmaParser`/`RuleValidator`/`DetectionEngine`/correlation-evaluation symbols appear anywhere under `gui/`.
- `MainWindow` owns the `ITelemetrySource` via `std::unique_ptr` as designed (`gui-architecture.md` §3); this is the documented owner, not a "widget holding a pointer to the telemetry source" violation — no other widget in `gui/src` holds a reference to the source.

**BRIEF.md Part II §7 — no color literal in C++ outside generated `Theme.h`: compliant.**
- `grep -rnE "#[0-9A-Fa-f]{6}|#[0-9A-Fa-f]{3}"` across `gui/src` and `gui/include` (`.cpp`/`.h`/`.hpp`) returns **zero matches**.
- `gui/resources/styles/dark.qss.in` also has zero hex literals — uses `@SF_*@` placeholders exclusively.
- Every hex value is declared exactly once, in `cmake/theme.cmake:4-37` (e.g. `SF_BG_BASE "#0B0E14"`, `SF_ACCENT "#38D6C4"`), and substituted into `gui/build/generated/Theme.h` and `.../dark.qss` via `configure_file` (`gui/CMakeLists.txt:23-31`).

**One standards deviation (not a `gui/` violation, but worth flagging):** BRIEF.md §6 mandates "C++20 as the floor." `gui/CMakeLists.txt:5` correctly sets C++20. `collector-cpp/CMakeLists.txt:5` sets **C++17**. Since BRIEF.md explicitly says the collector backend must not be modified, this is likely inherited from before the brief was written rather than a new violation — but it means the stated engineering standard doesn't hold repo-wide.

---

## 5. Test coverage

**collector-cpp**: 65 tests, 9 files, 100% passing (verified live via `ctest -C Debug`). See §2 table for feature-to-test mapping.

**gui**: zero test files, zero test target in `gui/CMakeLists.txt`. All of the following are **implemented but completely untested** — no automated test exercises any of them, anywhere in the repository:

- **`TimestampFormat`** (`gui/include/util/TimestampFormat.h`) — `columnTime`, `tooltipTime`, `relativeSuffix`, `logClock` are all pure functions of `qint64`, trivially unit-testable, but no test references them.
- **`MitreCatalog`** (`gui/src/mitre/MitreCatalog.cpp`) — JSON loading, lookup, and validation logic; untested.
- **The technique-ID regex** (`MitreCatalog.cpp:47`, `^T\d{4}(\.\d{3})?$`) — untested; no test asserts it accepts `T1059.001` / rejects malformed IDs.
- **`DetectionModel` batching and ring-buffer eviction** (`DetectionModel.cpp:121-141` `appendBatch`, `:192-202` `evictOldest`) — untested. The logic is internally consistent by inspection (single `beginInsertRows`/`endInsertRows` per batch, `beginRemoveRows` before removal, eviction happens before insertion so the cap is never exceeded), but Issue #020's own acceptance criterion ("verified at 10,000 rows") has no corresponding test to verify it against.

---

## 6. Code health

**Files over 300 lines:**

| File | Lines |
|---|---|
| `gui/src/widgets/InspectorPane.cpp` | 514 |
| `gui/src/widgets/DetectionTableView.cpp` | 414 |
| `gui/src/telemetry/MockTelemetrySource.cpp` | 402 |
| `collector-cpp/src/Configuration.cpp` | 379 |
| `gui/src/MainWindow.cpp` | 367 |

(`collector-cpp/src/Application.cpp` at 294 lines is just under the threshold.) No `.h`/`.hpp` file in either `gui/include` or `collector-cpp/include` exceeds 300 lines (largest: `gui/include/telemetry/TelemetryTypes.h` at 219).

**TODO/FIXME/HACK/XXX:** zero matches anywhere in `gui/src`, `gui/include`, `collector-cpp/src`, `collector-cpp/include`, or `collector-cpp/tests`.

**Duplicated logic:**
- `RuleLoader::DiscoverAndParse` (`collector-cpp/src/RuleLoader.cpp:52-66`) and `SigmaLoader::LoadDirectory` (`collector-cpp/src/SigmaLoader.cpp:10-29`) independently implement the same directory-scan-filter-by-extension-then-`std::sort` pattern, with no shared helper.
- `DetectionModel` and `CorrelationModel` implement near-identical `appendBatch`/`evictOldest` pairs (`DetectionModel.cpp:121-141,192-202` vs `CorrelationModel.cpp:73-90,108-116`) — likely intentional given they operate on different row types, but it is ~20 lines of repeated structure that a shared template/base could absorb if a third model type is ever added.

Neither is severe; both are worth a note, not a fire drill.

---

## 7. Build state

| Build dir | Generator | Evidence | Status |
|---|---|---|---|
| `collector-cpp/build/` | Visual Studio 18 2026 | `CMakeCache.txt`: `CMAKE_GENERATOR:INTERNAL=Visual Studio 18 2026` | Builds and tests pass (verified live: 65/65) |
| `gui/build/` | Not confirmable — **`CMakeCache.txt` is missing** | `.vcxproj`, `SentinelForgeDesktop.slnx`, `CMakeFiles/` all present and consistent with a Visual Studio generator, but the cache file itself is absent | A prior successful build+deploy exists (`sentinelforge_desktop.exe`, `windeployqt` output in `gui/build-out.txt` all "up to date"), but the tree as it sits **cannot be incrementally rebuilt** (`cmake --build` needs a cache) without re-running `cmake -S gui -B gui/build` first |

**Flag:** `gui/build/` is in an inconsistent state — it has the full output of a configured-and-built tree but not the configuration cache that produced it. This isn't a repo problem (`build/` is fully gitignored, `.gitignore` line `**/build/` plus explicit `gui/build/` and `CMakeCache.txt`), just a local-machine anomaly worth knowing before assuming "GUI builds cleanly" — the confidence here is "it built successfully at some point," not "it builds right now from a clean `cmake --build`."

Both build directories are gitignored and represent local, ephemeral state, not committed artifacts.

---

## Methodology notes / what was not verified

- BRIEF.md's runtime/performance acceptance criteria (startup < 2s, 1,000-event burst with no frame drop, 8-hour memory-flat soak, resize smoothness, 100/125/150% DPI correctness, squint test, grayscale severity test) require actually running the GUI and are **not verified** by this static/build-level audit.
- `docker/collector.Dockerfile` was read and judged plausible against the verified CMake build, but `docker build` was not actually invoked.
- `k8s/` and `detector-python/` being empty was confirmed by direct directory listing and `git ls-files`, not by an assumption.
