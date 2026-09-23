# MTerm testing

## Native Windows gate

```powershell
.\scripts\native\Build.ps1 -Configuration Release
.\scripts\native\Build.ps1 -Configuration Debug
python scripts/native/verify-provenance.py
```

Each configuration runs five CTest executables. QtTest outcomes include lifecycle/data rows;
current per-build counts are core 40, runtime 15, backend 22, desktop 7, terminal 11.
Reports are `out/native-<configuration>/native/*-tests.txt` and CTest temporary logs.
No real model is called. The compiled mterm-test-child emits controlled protocol fixtures.

Coverage includes canonical paths/hash conflicts/UTF-8, schema upgrades/newer-version refusal,
identity guards, scoped requests, bounded processes/cancel/timeout, malformed final JSONL,
provider fresh/resume outcomes, persistence, native editor, real Windows PTY and GUI keystrokes.
Eight malformed mutation cases were recorded failing before their fixes.

## Standalone preview validation

```powershell
.\scripts\native\Package-Windows.ps1
python scripts/native/smoke_package.py release-local/<created-folder> --runs 5
```

The smoke verifies every manifest hash, removes development Qt/compiler folders from PATH,
uses new temporary workspace/data profiles, captures screenshots and records internal readiness/
paint/memory metrics. It neither launches models nor deletes evidence. It is NOT a pristine-machine,
soak, signing or external cold-launch benchmark. Original dependency-less launch failed as expected;
the deployed package succeeded. See docs/validation/2026-09-22-native-resume.md.

## Preserved Electron gate

```powershell
npm run typecheck
npm run lint
npm test
npm run build
npm run test:mcp
npm run test:e2e -- --repeat-each=2
```

The build precedes integration. Current 59 unit tests and 4 repeated E2E executions pass.
The tests validate an actual loaded editor document, not only an initialization status label.
Reference E2E remains Windows/machine-aware; do not represent it as universal hosted coverage.

## CI and remaining coverage

The pinned Windows/Linux Release/Debug workflow is documented at docs/ci/native.workflow.yml;
activation is blocked by workflow-write permission. No hosted pass is claimed. Unix PTY skips
are explicit, not native Unix parity. macOS, sanitizers/fuzzing, signed installers, real provider
conformance, long-running burst/backpressure, hostile file races and full crash recovery remain.

## Native UX restoration and relocation

Run from C:\Tools\Dev\MTerm. The current native suite adds `native-ux` to the five original
suites. Tests cover shell geometry, palette, shared Project/Canvas IDs, real file selection, view
state preservation, fixed overlays, disabled Observe controls, note autosave/restart, readable
Arrange, 500-card item bounds, and lazy-but-retained inspectors. The real PTY desktop test now
switches tools and Canvas/Project, then confirms the same shell variable/output persists.

Optional visual evidence: set MTERM_UX_CAPTURE_DIR to a new artifact directory and explicitly set
QT_QPA_PLATFORM=windows, then run mterm-ux-tests responsiveWorkspaceAndOptionalVisualEvidence.
Unset capture env before offscreen CTest to avoid overwriting reviewed images with fontless output.
Theme diagnostics in --smoke-dir JSON expose construction phases; they are internal, not external
process launch timings. See validation/2026-09-22-native-ux.md and the feature/UI parity template.

## Native workbench and roadmap gate

New native-workbench executable tests multiple documents, undo/dirty state, late read/write races,
explicit tab selection, all-buffer close cancellation,12-buffer cap, shortcuts, geometry within
screen bounds and resource filtering. Its screenshot fixture is opt-in via
MTERM_WORKBENCH_CAPTURE_DIR with QT_QPA_PLATFORM=windows; ordinary offscreen run records a skip.

Run `python -X utf8 scripts/roadmap/test_roadmap.py` then perform an explicit metric review/refresh
and `--check`. Normal Build.ps1 and Package-Windows.ps1 enforce that check. Development-only
-SkipRoadmapCheck must never be cited as a final publish gate. Dashboard browser validation is
`scripts/roadmap/validate_dashboard.mjs`, using installed Edge and blocking HTTP requests.

## Native terminal candidate

Two additional native suites exercise resource lifecycle and actual multi-terminal GUI behavior.
TerminalResourceTests covers real dual shells, stale restarts, cwd/invalid data, all-page recovery,
invalid root preservation, capacity/revocation, consumer backpressure, natural exit and cross-scope
protection. TerminalWorkspaceTests covers card-owned screens, no-autostart, unique names, view caps,
view-switch survival and transient Canvas/Project statuses. All tests use isolated local shells,
not paid model calls. Set MTERM_TERMINAL_CAPTURE_DIR only for explicit Windows visual fixtures;
unset it for normal CTest. Expected totals and commands are in the current validation document.
