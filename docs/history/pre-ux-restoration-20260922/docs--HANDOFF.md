# MTerm native handoff — 2026-09-22

## Read first

This is the C++20/Qt MTerm checkout, not the original AstraCommander folder.
Start with AGENTS.md, CONTRIBUTING.md, STATUS.md, ARCHITECTURE.md and ROADMAP.md.
The original local AstraCommander tree is unchanged. Imported reference documentation is preserved
under docs/history/pre-resume-20260922/; old checkmarks are not native feature parity.

## Local and GitHub state

Development checkout: `C:\Users\Dylan\Documents\MTerm`.
Recovery commit: `e82fc49f64958700489c6ffb6c9daf56d78da623`.
Local continuation commit `38012ae` contains the workflow GitHub refused to install.
The source-only publishing branch is `feat/native-resume-20260922`, with a separate local
worktree under `artifacts/source-publish`. Do not overwrite or rebase the recovered development
branch to resolve this difference. The full workflow remains local and mirrored at
`docs/ci/native.workflow.yml` as non-executable source until workflow-write access is authorized.

Pre-resume source archive: `artifacts/checkpoints/resume-20260922T221152Z`.
It contains a source ZIP, SHA-256 manifest and Git patches. One unreadable, untracked
`native/runtime/ProcessInventory.cpp.tmp0` was excluded and left untouched. Do not delete it
or broadly change permissions to make a clean-status claim.

## Build, run and verify

```powershell
.\scripts\native\Bootstrap-Windows.ps1
.\scripts\native\Build.ps1 -Configuration Release
.\scripts\native\Build.ps1 -Configuration Debug
python scripts/native/verify-provenance.py
.\Start-MTerm.ps1
.\scripts\native\Package-Windows.ps1
python scripts/native/smoke_package.py release-local/<new-package-folder>
```

Qt 6.8.3 / MinGW 13.1.0 are project-local initial pins. They need security/license review before
public release; do not equate reproducibility with current security support.
Launch `mterm.exe --workspace PATH --data-dir PATH` for explicit isolation.
`--smoke-dir PATH` writes a screenshot/internal timing sample and exits without invoking models.

The normal source tree also retains the Electron reference. Its standard npm test/build scripts
remain separate; do not launch it when assessing native memory.

## Verified this continuation

- Native Windows Release and Debug: 5/5 suites each, 95 QtTest outcomes per configuration
  including fixture lifecycle/data rows (40 core, 15 runtime, 22 backend, 7 desktop, 11 terminal).
- Eight intentionally failing malformed-request cases were observed before fixing type coercion.
  Non-string write payloads now fail without changing file bytes; bad workspace roots do not
  silently reset the current profile through type coercion.
- Reference: typecheck, lint, 59 unit tests, build, MCP and two E2E scenarios repeated twice pass.
  The old branch-specific expectation and a lazy-editor race were repaired without removing tests.
- Native standalone package launches with development Qt/compiler directories absent from PATH.
  Five isolated empty-workspace smokes pass; repeatable smoke script also verified.
- No native live-model, hosted CI, Linux/macOS, signed installer, crash-soak or CLI superiority claim.

## Architecture to preserve

Qt Widgets/Graphics View is the current renderer; Qt Quick is only a benchmark-driven option.
Core/domain/storage have no Widgets dependency. GUI calls Backend; one service worker owns
SQLite, file I/O, job lifecycle and terminal transport. ConPTY has separate bounded reader/writer
threads; the VT screen is drawn natively. No idle Chromium/Node runtime belongs in the native app.

Native data is separate schema-3 SQLite. Existing active legacy databases are never opened/imported
automatically. Workspace IDs scope records, responses, grants and job outcomes. Revocation/opening
a workspace stops owned jobs; separate terminal/agent approval remains mandatory.

## Next bounded tasks

1. Issue #2: handle-anchored file safety and true pre-execution ownership for captured QProcess
   children; the ConPTY path already assigns its suspended child before resuming.
2. Issues #2/#5: test late callbacks during workspace switches, all-history reconciliation beyond
   the latest page, cancellation vs provider errors, and UTF-8 across batched captured output.
3. Issue #3: terminal scrollback/selection/IME, multiple resource identities, shutdown bounds,
   Unix PTY transport, and GUI-independent supervised sessions.
4. Issues #4/#6: shared resource graph, native file tree, task/evidence detail UI, Git mutation
   and worktree parity. Do not duplicate mutable state per canvas/project view.
5. Issue #5: native protocol-neutral MCP services and provider conformance. Keep the TypeScript
   MCP server until wire compatibility/security gates pass; real model tests are opt-in.
6. Issue #7: obtain authorized workflow installation, execute hosted Windows/Linux matrix,
   fix genuine portability failures, then certify packaging/dependencies/signing/updates.
7. Issue #8: external launch-to-command-ready and equivalent-workload benchmarks; the current
   inside-main paint timings cannot prove that MTerm is faster than a CLI.

All contribution provenance belongs in `docs/ai/changes/`, headers and commit trailers.
Use actual model identity or unknown, preserve original attribution, and leave exact tests,
limitations and first unvalidated next task. Never include credentials or private chat exports.
