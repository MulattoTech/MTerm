# MTerm

A native-first, local AI workstation. Formerly AstraCommander.

**Status: incremental C++20 / Qt 6 migration, not feature-complete or release-certified.**

- `native/core/`: permissions, bounded file I/O, SHA-256 stale-write checks, canvas validation, SQLite migrations and paginated records.
- `native/runtime/`: asynchronous bounded process capture, cancellation/timeouts, Windows native inventory and JSONL decoding.
- `native/services/`: worker-thread workspace/task/file/audit services and policy-gated job/provider supervision.
- `apps/desktop/`, `packages/core/`: preserved Electron/TypeScript reference implementation; visible branding is MTerm. Legacy IPC/environment identifiers intentionally remain compatible.

## Start here — any AI provider or human

Read [AGENTS.md](AGENTS.md), [CONTRIBUTING.md](CONTRIBUTING.md), [current status](docs/STATUS.md), [handoff](docs/HANDOFF.md) and [migration plan](docs/plans/2026-09-22-native-foundation.md). Full inherited feature scope is in [the master backlog](docs/MASTER_TODO_100_PERCENT.md). Track coordinated work in GitHub issue #1 and its child issues.

## Native build

Install CMake >=3.24, a C++20 compiler and Qt >=6.5 (Core, Gui, Widgets, Sql, Test). The initial Windows development toolchain is Qt 6.8.3 + MinGW 13.1; this is a pinned reproducibility baseline, not a claim to be the newest Qt release.

```sh
cmake --preset native-release -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build --preset native-release
ctest --preset native-release
```

Native desktop and runnable packaging are in active development; do not assume Electron parity. Qt is dynamically linked and carries its own license obligations; see the third-party notice work before redistributing binaries.

## Electron reference

Use the pinned pnpm version in package.json to install dependencies, then `npm run build` and `npm start`. Routine gates: `npm run typecheck`, `npm run lint`, `npm test`, `npm run build`, `npm run test:mcp`, `npm run test:e2e`. Real provider smoke scripts consume provider usage and are opt-in only.

## Safety and privacy

Native workspace data is separate from the old app. The original local AstraCommander tree is unchanged. Credentials, raw conversations, caches, dependency trees and local user databases are not published. Observe is the default; execution requires explicit approval and remains OS-user execution, not a security sandbox.

## License and authorship

MTerm-authored code is MIT. Dependency licenses remain separate. AI assistance is recorded in `docs/ai/changes/`, source headers and Git trailers; unknown historic model identity is not invented. No external chat is required to resume development.
