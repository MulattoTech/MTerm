# MTerm status — native resume checkpoint

Date: 2026-09-22. Native and reference implementations are intentionally separate.

| Area | Native state | Evidence / limitation |
|---|---|---|
| C++20 / Qt Widgets desktop | Implemented, limited | Native QtTest desktop workflows and standalone smoke |
| Workspace catalog/restore | Implemented, limited | Canonical-root identity, last workspace, Observe on open |
| Canvas/notes | Implemented, limited | Native cards, pan/zoom, saved positions and viewport; no edge/group editor |
| Tasks | Implemented, limited | Create/list/status; full checklist/evidence UI remains in reference |
| File editor | Implemented, limited | UTF-8 <=1 MB, hash conflicts, atomic save, dirty protections; one buffer |
| SQLite | Implemented, limited | Schema 3, indexed workspace scope, transactions, newer-version refusal |
| Process inventory | Implemented on Windows | Native OS API; no PowerShell enumeration in native path |
| Captured jobs | Implemented, limited | Bounds, timeout, cancel, explicit scope/grant; not an OS sandbox |
| Git | Read-only native | Status/diff/log/worktree list; mutation is reference-only |
| Windows terminal | Real PTY, limited | ConPTY + MIT libvterm, colors, cursor, key input, resize; no complete scrollback/IME |
| Codex provider | Fixture-validated native | Fresh/resume, final-frame/error handling, process-outcome evidence |
| Native MCP / other providers | Not complete | Keep TypeScript compatibility path; no claim of live native provider validation |
| Standalone Windows staging | Implemented, local preview | SHA-256 manifest; clean-PATH launch, 5/5 samples; not signed/release-certified |
| Linux/macOS native parity | Not validated | CI recipe supplied; Unix PTY adapter absent |
| Hosted CI | Blocked by permission | Workflow installation rejected: OAuth credential lacks workflow scope |

## Current local results

Release and Debug each pass **5/5 CTest suites**. QtTest outcomes including init/cleanup and
parameter rows: core 40, runtime 15, backend 22, desktop 7, terminal 11; **95 total per build**.
Do not call that 95 independent end-to-end scenarios.

Reference: typecheck, lint, 59/59 unit tests, production build, MCP integration, 4/4 E2E
executions (two scenarios repeated twice) passed. A lazy Monaco initialization race was fixed
rather than extending timeouts or removing assertions.

Native malformed text requests formerly coerced to empty strings and could overwrite a file.
Eight red regression outcomes were observed, then fixed; filesystem bytes are now checked preserved.

The native package measured 32,205,531 bytes excluding its manifest. Five empty-workspace
samples reported internal first-paint median 65 ms, workspace-ready median 89 ms, working-set
median 43,876,352 bytes and private-memory median 13,148,160 bytes. These are internal timers,
not external cold-start/CLI comparisons. See validation document for methodology.

## Not release-ready

Handle-anchored path races, QProcess job-assignment race, full subtree/soak coverage, complete
terminal parity, all-history recovery, native resource graph, native MCP, provider conformance,
OS secret storage, packaging compliance, signing/updates and clean-machine certification remain.
No paid providers, public listeners, firewall changes, or original AstraCommander edits occurred.
