# Security Model — current implementation

## Trust boundaries

- The React renderer is untrusted relative to OS capabilities.
- Electron main is privileged and owns files, PTYs, Git, SQLite and policy.
- Preload exposes a narrow named API through `contextBridge`.
- MCP clients are external actors and must pass the same core path/policy primitives.
- Terminal execution is powerful OS-user execution, not a sandbox.

Electron uses `contextIsolation: true`, `nodeIntegration: false`, renderer sandboxing, navigation denial, HTTPS-only external-window handling, and a restrictive renderer CSP.

## Workspace paths

`safePath` canonicalizes the workspace root and targets. It rejects absolute/drive/UNC paths, parent traversal, Windows ADS/colon syntax, trailing-dot/space ambiguity, reserved device names, symlink/junction escape, and conservative existing hard-linked file aliases.

Writes additionally reject `.git`, `.ssh`, `.gnupg`, `.env`, and `.env.*` path components. New write targets are checked through their nearest existing canonical ancestor.

## Permission profiles

**Observe** permits read capabilities only. **Developer** permits workspace-scoped file/Git modifications defined by core policy, but terminal/network/browser/docker-style capabilities still require explicit grants or remain denied.

The desktop begins in Observe. The local UI may switch to Developer. Terminal execution and agent execution each require their own separate in-memory session approval. Observe cannot be bypassed by an existing grant.

## Agent execution

The current built-in Codex CLI provider is local and explicit. It requires Developer profile plus an `agent.execute` session grant, is capped at four concurrent child processes, runs Codex with `--sandbox read-only`, and does not persist prompts or model output in audit records. Provider sessions persist metadata/thread IDs; interrupted active sessions are reconciled to `STOPPED` on application restart. This is a permission boundary, not a guarantee that third-party model behavior is harmless.

## MCP

MCP stdio defaults to Observe. `ASTRA_MCP_PROFILE=developer` enables permitted workspace writes. `ASTRA_MCP_ALLOW_TERMINAL=1` additionally creates a time-bounded terminal grant for that MCP process.

The MCP terminal tool uses `execFile` with `shell:false`, bounded arguments/output/timeout, workspace cwd, and destructive-command classification. It is still not a security sandbox: an explicitly authorized non-classified executable can act with the user's OS permissions.

No HTTP server, tunnel, firewall change, router change, or public endpoint is created.

## Audit and redaction

Desktop and MCP emit structured audit metadata: timestamp, tool, decision, target, result and duration. MCP terminal audit records the executable but deliberately does not persist command arguments or command output. File audit records path and byte count, not contents.

Core structured redaction handles secret-like field names and supplied secret values, including nested/cyclic data. The product does not yet have an OS-backed secret store, so no provider credentials should be stored in workspace configuration.

## Known limitations / next security work

- Add an OS-backed secret vault (`safeStorage`/Credential Manager abstraction) before provider credentials.
- Add signed/trusted plugin policy before third-party plugin execution.
- Extend command classification beyond the current conservative rules before broad autonomous shell grants.
- Add per-agent/provider/path permission scopes and expiry UI.
- Add database migrations and integrity/version checks.
- Add multi-workspace policy isolation and tests.
- Expand the existing optimistic stale-write check into a full conflict/diff UX.
