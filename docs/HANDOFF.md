# Current candidate continuation — 2026-09-23

Active source:`C:\Tools\Dev\MTerm`, branch`candidate/native-20260923`.
The user authorized consolidation of all published branches and a current GitHub candidate.
Read CANDIDATE.md for verified ancestry and the actual PR discussion for final remote merge state.
Do not resume from an older feature branch and overwrite newer candidate work.

The previous failed turn left tested native terminal changes uncommitted. This continuation verified
real RDC write/rename/read-back, saved a complete working-source checkpoint and refs bundle, recovered
the code and expanded lifecycle/UI tests. The new terminal contract is NATIVE_TERMINAL_RESOURCES.md.
Each terminal card owns a bounded independent ConPTY run, cwd/shell metadata and retained screen.
Immutable run IDs reject old input/resize/stop/ACK. New selection never auto-starts. Live status badges
are transient across Canvas/Project; saved canvas acceptance/status is untouched.

Native metadata is separate from terminal text; restart reports STOPPED, never falsely reattached.
Store recovery now updates all pages of the selected workspace in one scoped SQL operation, and an
invalid new root is rejected before current jobs are stopped. Final release-style validation must
use default roadmap gates, not the local red/green bypass. Update all affected REQ IDs/evidence.

Remaining high-value tasks: terminal scrollback/IME/selection/Unix/detached supervision, per-resource
metadata lookup beyond latest100, safe file races and QProcess ownership, richer task/evidence/Git/
provider/native-MCP parity. DevFleet remains a proposed integration, not connected. Six source-project
attributions and the496-row/472-scored roadmap scope remain intact; no denominator manipulation.

Write proof:`artifacts/access-check-20260923T184417Z/write-test-verified.txt`.
Resume checkpoint:`artifacts/checkpoints/terminal-resume-20260923T184521Z` (sourceZIP+Gitbundle).
Preserved temp exception:`native/desktop/MainWindow.cpp.tmp0`; do not delete or grant broad permissions.
See validation/2026-09-23-terminal-candidate.md for tested scope and actual limits.

---

# Current continuation — workbench and mandatory roadmap (2026-09-23)

Active root `C:\Tools\Dev\MTerm`, branch `feat/native-workbench-roadmap-20260923`, based on6344e94.
The prior handoff below is retained as history; this section and current STATUS/ROADMAP take precedence.

**Read docs/roadmap/METRICS.md before modifying anything.** All496 rows are retained:485 inherited,
8 DevFleet,3 governance.24 legend/dated-reference rows are visibly excluded;472 requirements are
scored. Do not treat inherited checkmarks or phase percentages as native release readiness.
Every change must review affected IDs, evidence, stage, estimate and blocker, update its AI record,
run `--refresh --change-id ... --items ... --review-note ...` and pass `--check`. Normal builds and
packaging reject stale source/catalog fingerprints; local -SkipRoadmapCheck is not a release gate.

Native file ownership is now EditorDeck, not MainWindow's single-file fields. Up to12 retained
QTextDocuments, each with undo/cursor/scroll/version and pending operations keyed by buffer identity,
workspace generation and explicit user selection. Late reloads must match the document revision;
late saves update only their target. Closing and switching workspace inspect background dirty tabs.
Window geometry/splitter use a sidecar INI at `<database>.window.ini`. Unsaved text is not persisted
across application restart; do not claim crash recovery of editor contents.

New workbench suite tests tab identity/undo, late-save and late-reload races, user-selected tab priority,
background close cancel, capacity/keyboard close, geometry and no-delete filtering. Real Windows
screenshots are opt-in; no models are called. Run all native, reference and roadmap checks before
publishing. The roadmap itself is offline documentation/tooling, not a new product browser runtime.

Next: full per-resource native sessions, richer task/evidence and Git/provider/MCP parity; editor
save-all/find/syntax/LSP/recovery; realistic perf/security/cross-platform gates. Source-project matrix
and proposed DevFleet contract remain unchanged. Hosted CI workflow permission is still blocked.

---

# MTerm active handoff — native UX restoration, 2026-09-22

## Active location and branch

**Use `C:\Tools\Dev\MTerm`, branch `feat/native-ux-20260922`.**
This branch builds on `f132bb6f24893b3557a8f727c4b238a6d0589caf`, the source-only native resume
branch from PR #10. Older Documents/MTerm and Documents/AstraCommander are preserved recovery copies.
Do not resume development in them, overwrite them, delete the unreadable old tmp file, or copy old
CMake/Python/Node caches into the active tree. Source/SDK were relocated and path-sensitive tools
were recreated. 164 baseline source files were compared against the prior publishing worktree.

All old refs, including the local unpublishable CI commit, are archived in
`artifacts/relocation/all-pre-relocation-refs.bundle`. The local archive branch preserves
`2a3cfbfe9d7010449384d4fd7eb73ef4abecafbf`. GitHub source publication is normal; executable CI
workflow installation still needs authorized workflow-write access. Do not bypass that restriction.

## User's explicit design decision

The basic tabbed native preview was rejected. The Electron interface/interaction model is the
reference: top workspace bar, left navigation, persistent canvas, right inspector, Canvas/Project
switch. Modern native presentation and performance are both required; neither replaces the other.
See UI_DESIGN_CONTRACT.md and actual Windows screenshots under docs/images/native-ux-20260922/.

## Implemented this pass

- Reusable Theme/resources, original vector glyphs, WorkspaceShell and named CommandPalette.
- Single-item native ResourceCards shared with ProjectView; pan/zoom/fit/minimap, readable Arrange,
  existing-edge rendering, card actions, position pinning/collapse and preserved resource IDs.
- Right inspectors retain state across navigation. Tools materialize only on first use: no eager
  hidden editor/agent/task widgets at startup. Their live instances are reused afterward.
- File browser/real file load, native editor preservation and old safe-write protections.
- Note selection/edit/save and debounced revision-checked layout persistence with visible status.
- Observe terminal controls visibly disabled; actual native PTY survives tool and view changes.
- Full inherited source-project map, unchanged master backlog indexed as 485 stable REQ IDs,
  repository-local feature/UI template and proposed DevFleet integration contract.

## Verify

```powershell
.\scripts\native\Build.ps1 -Configuration Release
.\scripts\native\Build.ps1 -Configuration Debug
python scripts/native/verify-provenance.py
.\scripts\native\Package-Windows.ps1
python scripts/native/smoke_package.py release-local/<new-folder> --runs 5
```

Unset `MTERM_UX_CAPTURE_DIR` during offscreen CTest. Explicitly use `QT_QPA_PLATFORM=windows`
when capturing visual fixtures; headless fontless screenshots are not appearance evidence.
Use the standard npm reference gate separately. Read docs/validation/2026-09-22-native-ux.md.
The native launcher opens the active checkout unless -Workspace or -UseLastWorkspace is specified.

## Next highest-value work

1. Expand native per-resource sessions and the shared graph, not independent duplicated view state.
   Multiple terminal cards currently route to the existing single native terminal inspector.
2. Restore richer reference task/evidence, context attachments, provider history and native Git
   mutation/diff/worktree workflows using the saved source-project matrix and acceptance template.
3. Continue file/process race hardening, full-history recovery and bounded terminal shutdown from #2/#3.
4. Certify external readiness, interaction/frame times and retained-history memory under realistic
   workloads. Startup samples are internal timers, not proof of being faster than Codex CLI.
5. Implement DevFleet only after an authorized read of its actual current API/contracts. DF-01..08
   are proposed requirements, not implemented services or assertions about DevFleet's current API.
6. Obtain authorized workflow activation and complete cross-platform/release gates; no hosted pass
   or signed release is implied by local tests.

Known UX rough edge to review: pending-layout-save workspace switching currently asks the user
to retry after saving. The repeated close confirmation while saving was reproduced and repaired;
approval is bound to the exact editor revision and the dirty flag is retained on save failure.

## Preserve the complete goal

Read SOURCE_PROJECT_FEATURE_MATRIX.md, MASTER_TODO_100_PERCENT.md and
backlog/requirements-index.json. All six original sources are retained; NodeTerm is conceptual,
not copied code/artwork. Native parity status comes from tests/STATUS, not inherited checkmarks.
AI records must give actual identity or unknown, UTC, scope, exact tests and first unvalidated step.
