# Workbench and complete roadmap validation — 2026-09-23

Active source: C:\Tools\Dev\MTerm, branch feat/native-workbench-roadmap-20260923,
based on6344e948baafd6886f54ba9bb815aae7314feb75. No original checkout/user data was removed.

## Implemented native workflows

EditorDeck is a dedicated native component: up to12 independent file documents, each retaining
text, undo, cursor/scroll, version and pending-save state, presented through one QPlainTextEdit.
Open/Reload/New/Save are explicitly scoped. A delayed save updates its own buffer only; a delayed
reload must match the UI document revision; a newer explicit tab selection wins over old I/O.
Dirty background tabs participate in close/workspace-switch checks. Ctrl+W/Ctrl+Tab shortcuts are
scoped to the editor, not stolen from a terminal. Views remain lazy and live instances are retained.

Window geometry/splitter state uses an INI sidecar beside the selected database, avoiding global
preference cross-contamination. Qt's screen-bound restore behavior is respected. Resource filtering
matches title/kind/content in Canvas and Project without deleting or rewriting nodes.

## Native test results

Windows Release and Debug **7/7 CTest suites passed with the default roadmap gate enabled**.
Current per-configuration QtTest results: core40 pass, runtime15, backend22, desktop7, terminal11,
UX14, workbench10 pass/1 deliberate visual-fixture skip. Total119 passed/1 skipped, including
init/cleanup functions and data rows; not119 independent E2E scenarios.
The opt-in visual fixture ran separately with QT_QPA_PLATFORM=windows and passed3 lifecycle/test
outcomes. Its screenshot is in docs/images/native-workbench-20260923/; temporary fixture files and
generic workspace display text are explicit. No model/DevFleet service is running in the image.

Before fixes, tests observed missing file-tab/geometry/search flows, both late read/focus races and
missing keyboard-close behavior. The geometry test was corrected to screen-safe bounds because Qt
correctly clamps restored windows to the available screen; no production bug was hidden by timeout.

Native logs: artifacts/workbench-reviewed-release.log and workbench-reviewed-debug.log.
Reference gate: typecheck, lint,59/59 unit, build, MCP integration and2/2 Electron E2E passed;
log artifacts/workbench-reference-gate.log. No paid model calls were made.

## Roadmap validation

25 pure-Python tests pass: exact ID coverage, no duplicates, weighted math, remaining ranges,
finite inputs, required evidence/test paths, path escape protection, calendar ETA basis, matching
stage rubric, HTML escaping, deterministic rendering and source/catalog review integrity.
Actual local Edge rendering was tested with HTTP requests blocked:496 rows retained, active rows
filtered to472 by default, exact-ID search and blocked filter correct. This is offline HTML, not a
GitHub Pages deployment. GitHub-native Markdown/SVG views require no browser runtime in MTerm.

A controlled probe changed EditorDeck.h: --check rejected stale source. A second probe changed
catalog rationale and ran --write: --check still rejected the unreviewed assessment. Original bytes
were restored and the explicitly reviewed source/catalog passed. Probe artifacts preserved under
artifacts/metric-gate-probe-20260923T074101Z/. Default Release/Debug builds and packaging then passed
without -SkipRoadmapCheck. Developer opt-out is documented as invalid for publication.

## Scope and percent semantics

485 inherited rows +8 proposed DevFleet requirements +3 governance requirements =496 visible rows.
24 legend/dated-reference milestones are visibly excluded with zero weight;472 requirements scored.
Inherited `[x]` does not confer native credit. Fixed stage rubric0/10/40/65/85/100 and baseline effort
weights produce a weighted delivery-maturity score. This is a reviewed planning assessment, not
coverage/release readiness/safety assurance. The complete per-item notes/evidence remain in progress.json.

Each item has an approximate remaining engineer-hour range using explicit low-confidence scope
bands; calendar ETA is Unscheduled without capacity/dependency scheduling. Naive sums can double
count shared work and do not account for all rework. Neither these hours nor AI runtime are a promised
calendar schedule. Baseline weights were not altered to inflate completion during this pass.

Every AI entry point and contribution template requires metric updates/reviews. Source/catalog
fingerprints and generated-file checks catch stale bookkeeping, not dishonest self-reporting.
No global hooks, secrets, new public listeners or unauthorized hosted workflows were installed.

## Standalone startup smoke

Five isolated native package launches with development Qt/compiler paths removed from PATH passed.
Recorded payload32,626,641 bytes; median inside-main first paint50ms, workspace-ready65ms,
process working set98,156,544 bytes (~93.61MiB), private38,375,424 bytes (~36.60MiB).
Only the ordinary empty native workspace was active. These timers exclude OS loader time and are
not a cold-launch, busy-workload, CLI comparison or no-slowdown guarantee. Sources and manifest hashes
are verified by the smoke helper. Staging is unsigned development preview, not distribution approval.
Raw measurements: artifacts/package-smoke-ax9uqu6r/samples.json; artifacts/workbench-package-smoke.log.

## Remaining limits

No unsaved-document crash restore, mature save-all/conflict/LSP UX, independent live per-card session
graph, full native Git mutations/provider/MCP parity, real DevFleet adapter, complete terminal/Unix
transport, hostile-path/process-race certification, actual hosted CI, signing or updates. Those
requirements remain visible and partially/unimplemented, not silently removed by the roadmap.
