# MTerm native roadmap

The inherited MASTER_TODO_100_PERCENT.md preserves the broad vision. Its reference checkmarks
are not native parity. This roadmap and STATUS.md govern current native work.

| Priority / issue | Workstream | Immediate acceptance gate |
|---|---|---|
| P0 #2 | File/process/workspace safety | Handle-anchored path races; pre-execution QProcess ownership; late callbacks; transactional switching |
| P0 #7 | CI and release foundations | Authorized workflow installation, actual Windows/Linux hosted results, dependency/license review |
| P1 #3 | Terminal parity and continuity | Scrollback/selection/IME, bounded shutdown, Unicode bursts, multiple IDs, Unix PTY, GUI-independent supervisor |
| P1 #4 | Resource graph and native UI | Stable shared IDs across Canvas/Project; files, tasks, evidence; groups/edges/undo/search |
| P1 #5 | Providers and MCP | Native conformance fixtures, scoped tools, actual opt-in provider proofs, cancellation/resume/usage |
| P1 #6 | File/Git/evidence parity | File tree/ranges/search/patching; native diff/stage/worktree lifecycle with explicit approval |
| P1 #8 | Performance | Equivalent external readiness/input/frame/dispatch benchmarks; long histories and multi-surface budgets |
| P2 #9 | Optional integrations | On-demand browser/tunnels/plugins/skills/memory/Docker; no default background bloat |

## Completed baseline, not complete parity

Native core, database migrations/scoping, captured job supervision, Windows inventory, Qt desktop,
real Windows PTY, fixture-tested Codex fresh/resume, and local standalone packaging exist and pass
the documented Windows checks. Continue from those implementations instead of generating stubs.

## Next implementer contract

Choose one bounded issue acceptance item, record a change ID/provider/model/date, write and observe
failing tests, implement, run the full relevant gate, update STATUS/HANDOFF/evidence, then publish a
reviewable commit. Preserve old author headers and state unknown identity honestly.
Never infer a feature is complete because a data structure, button or process-exit-zero exists.

## Release remains separate

No current 0.2.0 preview is a signed production release. Complete dependency notices, update integrity,
rollback, backup/import safety, installer/uninstaller behavior, accessibility and clean-machine tests.
Retain the reference until native security and functional parity tests justify dropping its runtime.
