# Decisions

- Use the user-supplied autonomous build mission as the product specification and advance implementation without recurring design approval.
- Continue in the existing dedicated AstraCommander repository; do not touch DevFleet or unrelated repositories.
- Native Windows is the first validated platform. Do not alter WSL/Docker/OS security configuration to simulate portability.
- Electron remains the shell because the validated slice needs Node/PTYS/filesystem/process integration without a fragmented sidecar.
- TypeScript is the default implementation language.
- XYFlow is a renderer adapter only; persisted canvas data is owned by `packages/core`.
- SQLite/WAL is the current local persistence substrate. Keep schema small until real task/session/evidence models exist.
- New workspaces start Observe. Developer elevation is a local-human action; terminal execution requires an additional session grant.
- Terminal execution is explicitly not treated as a sandbox.
- MCP uses the installed stable v2 split SDK and stdio only in this slice. No network listener is created.
- MCP direct-process execution uses `execFile` with `shell:false` and an explicit environment opt-in.
- Desktop and MCP share durable audit schema; do not log file contents, command output, or terminal arguments merely for completeness.
- Git starts read-only. Mutation/worktree lifecycle comes only after policy and tests.
- Provider integrations remain deferred until one can be implemented and validated end-to-end; do not create a provider zoo of stubs.
- Application code is original MIT code. Upstream projects are architectural references only unless the license audit explicitly permits a future reuse.
- No automatic commit or push was performed; repository history remains under human control.

## Continuation decisions — 2026-09-22

- Supersedes the earlier blanket provider-deferral note: the installed Codex CLI is now the first real provider adapter because it can be locally validated behind an explicit `agent.execute` grant and read-only sandbox.
- Keep real-model provider validation separate from routine gates so repeated CI-style checks do not silently consume model quota.
- Reconcile interrupted agent sessions to `STOPPED` at startup rather than persisting a false RUNNING state after process loss.
- Persist provider completion as ValidationEvidence and link it to an optional Task; do not persist prompt or model-output content in audit metadata.
- The command palette only routes to existing named safe operations and does not expose a privileged arbitrary-command path.
- Continue deferring remote MCP/public tunnel work until persistence migrations and permission scopes are hardened.
