# Native resume validation — 2026-09-22

This report covers the Windows native preview and preserved reference separately.
Qt 6.8.3, GCC/MinGW 13.1.0, Windows 11 x64. No real model was invoked.

## Native regression results

| CTest executable | Release QtTest outcomes | Debug QtTest outcomes |
|---|---:|---:|
| Core | 40 pass | 40 pass |
| Runtime | 15 pass | 15 pass |
| Backend | 22 pass | 22 pass |
| Desktop | 7 pass | 7 pass |
| Terminal | 11 pass | 11 pass |

Five suites / 95 QtTest outcomes per configuration, including lifecycle functions and data rows.
Windows tests exercised actual ConPTY and native GUI keyboard input, not captured-command substitutes.
The provider tests use mterm-test-child protocol fixtures; they do not prove a real native model call.

Eight new mutation/request regressions first failed (six malformed text values, invalid task/node
fields, and malformed workspace-root input). Fixes reject incorrect JSON types before conversion.
The file-write tests verify original bytes remain unchanged. Raw red/green logs are local artifacts.

## Reference regression results

Typecheck, lint, 59 unit tests, production build and MCP integration passed.
Both Electron E2E scenarios passed twice (4 executions). The ported branch-header assertion was
made type-safe. A genuine lazy Monaco race was repaired: file load/save cannot precede editor
initialization, avoiding an empty editor/default-empty write. The test also inspects loaded content.

## Standalone native deployment

A bare executable with development Qt/compiler removed from PATH failed with 0xC0000135,
confirming that packaging dependencies was necessary. Windeployqt staging then passed the same
isolated-launch check. Unused shader compiler and network/SQL-server plugins were excluded from
subsequent staging; no existing package folder was deleted.

Final measured staging payload: 32,205,531 bytes (about 32.2 MB decimal), 17 files excluding manifest.
Every listed file was checked by SHA-256. The preview has no Node, Electron or Qt WebEngine binary.
This is unsigned local development staging, not a public redistributable/certified installer.

Five consecutive empty-workspace smokes produced:

| Metric | Median |
|---|---:|
| First paint, timer started inside main() | 65 ms |
| Workspace-ready signal, same internal timer | 89 ms |
| Current process working set | 43,876,352 bytes (41.84 MiB) |
| Current process private memory | 13,148,160 bytes (12.54 MiB) |

These figures exclude OS process-loader time. Workspaces were new; no model, browser or terminal
was active. Memory was sampled after a short startup delay. Samples are exploratory, not p95/p99,
steady-state soak or comparative CLI evidence. No claim of faster remote model inference is made.
A second 2-run invocation of the committed smoke helper also passed; variance is expected.

Reproduce: Package-Windows.ps1, then `python scripts/native/smoke_package.py <package> --runs 5`.
Raw local evidence: artifacts/lean-package-smoke-20260922T223020Z/samples.json.
Native logs: artifacts/resume-baseline-release.log, resume-boundary-red-tests.txt,
resume-boundary-green.log, resume-native-debug.log. Reference green log:
artifacts/resume-reference-gate-green.log. Screenshots/profiles remain local to avoid exposing paths.

## Not validated / blocked

Hosted CI workflow installation is denied by the saved Git credential's missing workflow scope.
The workflow exists as non-executable source under docs/ci/. Linux/macOS, external cold-launch
latency, multi-agent/terminal soak, full scrollback/IME, hostile path races, full QProcess child-tree
ownership, real native provider conformance, signing, updates and public distribution remain open.
