# MTerm

A lean, native-first local workstation for AI-assisted work. Formerly AstraCommander.

**Experimental C++20 / Qt 6 desktop, not a complete migration or certified release.**
The native application uses Qt Widgets/Graphics View, SQLite, native Windows process inventory,
and a real ConPTY/libvterm terminal. It does not require Electron, Chromium or Node to run.
The Electron application remains a separately runnable behavioral reference.

## Build and run on Windows

```powershell
.\scripts\native\Bootstrap-Windows.ps1
.\scripts\native\Build.ps1 -Configuration Release
.\Start-MTerm.ps1
```

The bootstrap installs development tools inside `.tools/`, not globally. The pinned initial
baseline is Qt 6.8.3 / MinGW 13.1.0, not an assertion of current release/security approval.
For an isolated workspace: `Start-MTerm.ps1 -Workspace C:\Projects\Example`.
The native app defaults to its last saved workspace, in Observe mode on each open.
Developer editing and separately approved execution are intentional user actions.

`Build.ps1 -Configuration Debug` runs the Debug gate.
`Package-Windows.ps1` in `scripts/native/` creates a new standalone local preview folder under
`release-local/`, with runtime dependencies and SHA-256 inventory. No old package is deleted.
The staging output is unsigned and not approved for public redistribution.

## What runs natively

- Native spatial notes/canvas, saved layout, tasks, and last-workspace restoration.
- Bounded UTF-8 file read/write with SHA-256 stale-write checks and dirty-editor protection.
- Worker-thread SQLite migrations, scoped/paginated records, and audit.
- Asynchronous captured commands, read-only Git jobs, and OS process inventory.
- Real Windows PTY with shell selection, VT colors/cursor/keyboard, and bounded transport.
- Native Codex fresh/resume/cancel supervisor tested with deterministic protocol fixtures.
  Real-model validation is separate and was not invoked in this continuation.

Full terminal scrollback/IME, native MCP, native Git mutation/worktrees, complete provider
parity, resource graph, plugins, signing and update safety remain work. See [status](docs/STATUS.md).

## Start here: any AI vendor or human

Read [AGENTS.md](AGENTS.md), [CONTRIBUTING.md](CONTRIBUTING.md), [handoff](docs/HANDOFF.md),
[architecture](docs/ARCHITECTURE.md), and [roadmap](docs/ROADMAP.md).
Issue #1 links the coordinated workstreams. Source headers, JSON change records and Git trailers
carry self-reported AI provenance; no external conversation is needed.

## Verification

Current Windows Release and Debug: five CTest suites pass, with 95 QtTest outcomes per
configuration including lifecycle/data rows. The preserved reference passes typecheck, lint,
59 unit tests, build, MCP integration, and both E2E scenarios repeated twice.
[Evidence and limits](docs/validation/2026-09-22-native-resume.md).

Hosted CI is **not validated**: the saved publishing credential lacks workflow-write permission.
The complete pinned Windows/Linux recipe is preserved as [non-executable CI source](docs/ci/native.workflow.yml).
See [CI activation](docs/ci/README.md); ordinary source publication is unaffected.

## Reference and privacy

`apps/desktop/` and `packages/core/` contain the rebranded Electron reference.
Install with the pinned pnpm version, then use `npm run build` / `npm start`.
Native data lives separately; there is no automatic import of the old app's active database.
User data, raw chats, credentials, toolchains, binaries and private logs are excluded from GitHub.
MTerm code is MIT; Qt/libvterm/compiler-runtime obligations remain separate.
