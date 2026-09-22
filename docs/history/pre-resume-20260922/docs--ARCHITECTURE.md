# Architecture — implemented state

## Runtime shape

AstraCommander is currently a TypeScript/Node/Electron application with three execution boundaries:

1. **Renderer** — React, XYFlow, Monaco, xterm. No direct Node/OS access.
2. **Electron main** — workspace, SQLite, files, Git, PTY ownership, policy and audit.
3. **MCP stdio process** — standalone v2 server using the same core path/policy code and compatible audit schema.

The renderer calls a fixed preload bridge. The bridge maps only named operations to IPC; it does not expose arbitrary IPC channels or Node primitives.

## Core domain

`packages/core/src/canvas.ts` owns the renderer-independent canvas schema. Zod validates node/edge/layout data and rejects duplicate/dangling identifiers and oversized state. XYFlow is an adapter over this model rather than the persistence format.

`packages/core/src/security.ts` owns canonical workspace paths, policy evaluation, redaction, and command classification. Trust boundaries use runtime validation rather than TypeScript types alone.

## Persistence

Electron main uses Node's built-in SQLite `DatabaseSync` with WAL mode. The current schema has:

- `settings(key,value)` for workspace and canvas JSON;
- `audit(id,timestamp,payload)` for durable structured audit events.

This intentionally favors a small proven vertical slice over a speculative full schema. Future migrations should split workspace/session/task/evidence tables once those runtimes are implemented.

## Workspace and policy flow

A workspace is `{ id, name, root, profile }`. New/default workspaces begin in `observe`. The local UI may switch to `developer`; MCP uses environment configuration and defaults to Observe.

Filesystem operations pass through `safePath`. Terminal creation passes through `evaluate` and requires both Developer profile and an in-memory session grant. MCP terminal execution requires Developer plus `ASTRA_MCP_ALLOW_TERMINAL=1` and uses `execFile` with `shell:false`.

## Terminal

Electron main owns a `Map<string, IPty>` of `node-pty` sessions. Windows sessions use `pwsh.exe` and the workspace root as cwd. Data and exit events cross fixed IPC event names. PTYs are killed on app quit and are not persisted as live sessions.

## Files

Desktop read/write is limited to UTF-8 files <=1 MB. Writes use a workspace-rooted temporary file and rename. Protected metadata/secret-style paths are denied by the core path policy.

The current editor is path-driven Monaco, not yet a filesystem tree or multi-buffer editor.

## MCP

`apps/desktop/src/mcp.ts` uses `@modelcontextprotocol/server` v2 and `serveStdio`. It exposes:

- `filesystem_read`
- `filesystem_write`
- `terminal_execute`

No HTTP listener exists. MCP audit writes use the desktop audit schema; on normal Windows defaults both processes target the AstraCommander roaming user-data database. Tests can override the database path.

## Agency runtime

`packages/core/src/agency.ts` defines runtime-validated Task, AgentSession, AgentProviderManifest and ValidationEvidence models plus explicit agent-session transitions. Electron persists tasks, sessions and evidence in the generic `records` table. The first provider is Codex CLI: Electron discovers the installed CLI, requires an `agent.execute` session grant, starts `codex exec --json --sandbox read-only`, streams JSONL events to the renderer, persists thread IDs for resumability metadata, and records PASS/FAIL evidence on exit. Active persisted sessions are reconciled to `STOPPED` at startup because child processes do not survive the app process.

## Git

The current Git runtime is intentionally read-only. Electron main executes Git directly without a shell and returns status, diff stat, worktree porcelain output and up to five recent commits. Unborn repositories are supported.

## UI

The main UI has:

- top workspace/view controls;
- create actions for Note, Terminal and Editor domain nodes;
- Canvas and Project views over the same model;
- Terminal, Editor, Git and Audit inspectors;
- a security card showing current profile and terminal authorization.

The current canvas renders domain nodes as cards. Live terminal/editor objects are still inspector-hosted rather than embedded directly inside individual spatial nodes.

## Build

`scripts/build.mjs` uses esbuild for Electron main/preload/MCP and Vite for the renderer. `scripts/dev.mjs` performs a build then launches local Electron.

## Deliberately absent

No second model-provider adapter, remote MCP, browser runtime, GUI computer control, write-capable Git orchestration, dedicated Diff/Process node, custom plugin loading, OS secret vault, Docker adapter, package-manager adapter or public endpoint exists yet. These remain continuation work rather than hidden stubs.
