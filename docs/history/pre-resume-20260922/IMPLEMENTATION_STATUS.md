# Implementation Status — 2026-09-22

Status vocabulary: `IMPLEMENTED`, `IMPLEMENTED — LIMITED`, `PARTIAL`, `SCAFFOLDED`, `DEFERRED`, `BLOCKED`.

| Area | State | Evidence / limits |
|---|---|---|
| Electron desktop shell | IMPLEMENTED | Real Electron launch and Playwright E2E pass. |
| Secure renderer boundary | IMPLEMENTED | Context isolation, sandbox, no renderer Node access, narrow typed preload IPC, CSP. |
| Workspace selection | IMPLEMENTED — LIMITED | One persisted current workspace; no catalog/multi-workspace manager yet. |
| Canvas domain + persistence | IMPLEMENTED — LIMITED | XYFlow pan/zoom/minimap, node move/add, SQLite persistence; grouping/edges/search/snapping/virtualization unfinished. |
| Project Mode | IMPLEMENTED — LIMITED | Alternate project/card view over the same persisted canvas state. |
| Command palette | IMPLEMENTED — LIMITED | Ctrl+K / Ctrl+Shift+P for core safe UI actions; customization/search breadth can expand. |
| Note nodes | IMPLEMENTED — LIMITED | Persistent cards; rich Markdown editing remains unfinished. |
| File/editor flow | IMPLEMENTED — LIMITED | File browser + Monaco read/save, <=1 MB, optimistic stale-write rejection, rooted canonical paths. |
| PTY terminal | IMPLEMENTED | Real `node-pty` PowerShell, resize/input/output/stop, explicit session approval. |
| Terminal restart continuity | DEFERRED | PTYs end with the app process; no external supervisor/re-attach layer yet. |
| Policy engine | IMPLEMENTED — LIMITED | Observe + Developer, scoped session grants for terminal/agent, fail-closed unknown capabilities. |
| Path security | IMPLEMENTED | Traversal, junction/symlink escape, ADS/UNC/reserved names and conservative hardlink defense tested. |
| Secret redaction | IMPLEMENTED — LIMITED | Structured recursive redaction tested; OS-backed secret vault not implemented. |
| Durable audit | IMPLEMENTED | SQLite/WAL events visible in Audit inspector; MCP can share desktop audit DB. |
| Local MCP stdio | IMPLEMENTED — LIMITED | MCP SDK v2 stdio; read/write/direct-process tools; no network listener/remote transport. |
| MCP filesystem tools | IMPLEMENTED | Observe read; Developer write; rooted security; official v2 client integration validated. |
| MCP terminal tool | IMPLEMENTED — LIMITED | Requires Developer + explicit env opt-in; execFile/no shell; destructive classifier gate. |
| MCP → desktop audit | IMPLEMENTED | Cross-process Electron E2E verifies MCP activity in the desktop Audit UI. |
| Git visibility | IMPLEMENTED — LIMITED | Read-only status, diff stat, worktree list, recent log; no mutation. |
| Durable tasks | IMPLEMENTED — LIMITED | Create/list/status transitions persist in SQLite and survive restart. |
| Agent provider abstraction | IMPLEMENTED — LIMITED | Core provider/session schema plus a real read-only Codex CLI adapter; one provider only. |
| Codex CLI provider | IMPLEMENTED — LIMITED | Real app-level smoke passed; read-only sandbox, streamed JSONL, thread IDs, stop, durable sessions. |
| Agent restart reconciliation | IMPLEMENTED | Interrupted active agent sessions become STOPPED at next app startup. |
| Validation evidence | IMPLEMENTED — LIMITED | Codex completion creates durable PASS/FAIL evidence linked back to an optional task. |
| Diff node | DEFERRED | Git diff stat exists, but no dedicated visual Diff node/accept-reject flow. |
| Process node | DEFERRED | No general system process inspector yet. |
| Browser node/automation | DEFERRED | No product Playwright browser runtime yet. |
| Remote MCP / tunnels | DEFERRED | No listener, tunnel, public endpoint, firewall/router change. |
| Docker/package tools | DEFERRED | No product adapters yet. |
| Plugin SDK | DEFERRED | Domain direction documented only. |
| Source/license audit | IMPLEMENTED | `docs/SOURCE_AND_LICENSE_AUDIT.md` records inspected sources/licenses. |

## Current quality gates

Latest baseline before final documentation refresh:
- `npm run typecheck` — PASS.
- `npm run lint` — PASS.
- `npm test` — PASS, 54/54 tests.
- `npm run build` — PASS.
- `npm run test:mcp` — PASS.
- `npm run test:e2e` — PASS.
- Real Codex CLI direct smoke — PASS.
- Real Electron → Codex → session → evidence smoke — PASS.

Known non-blocking warning: Monaco remains eagerly bundled and makes the initial renderer bundle large.

## Repository safety

Repository: `C:\Users\Dylan\Documents\AstraCommander` on unborn branch `build/first-vertical-slice`.
No commit or push was performed. No destructive Git cleanup/reset, unrelated repository changes, firewall changes, tunnels, or global-package changes were performed.

