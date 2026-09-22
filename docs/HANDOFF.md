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
