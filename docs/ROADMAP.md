# Roadmap — post validated first slice

The first vertical slice is now complete enough to hand off: desktop/canvas/files/PTY/MCP/audit/permissions/restart recovery are validated, and the second slice already includes durable tasks, a real Codex CLI provider, validation evidence, Project Mode, read-only Git, a file browser, optimistic saves, and a command palette.

## P0 — harden the working foundation

1. Refactor duplicated desktop/MCP SQLite/audit code into a shared persistence package with explicit migrations.
2. Lazy-load Monaco and establish cold-start/render performance budgets.
3. Add a Windows process/PTY supervisor only if terminal survival across app restart is a product requirement.
4. Expand permission scopes to workspace/path/provider/agent and add grant expiry/revocation UI.
5. Add database integrity checks, backup/export, and recovery tests.

## P1 — finish the second vertical slice

- Build a dedicated Diff node with unified/split views and safe stage/unstage workflows.
- Add policy-gated Git worktree create/remove lifecycle without automatic commit/push.
- Add a general Process node/inspector with bounded output/resource telemetry.
- Move live terminal/editor/agent surfaces into spatial nodes while retaining inspector fallbacks.
- Add task checklist/blocker/acceptance-criteria editing and richer evidence browsing.
- Add canvas edges, groups, search, snapping, multi-select and keyboard navigation.
- Add one additional genuinely testable provider adapter behind the existing capability model.
- Add explicit resume UX for Codex thread IDs and interrupted session metadata.

## P2 — browser and local agency

- Playwright browser runtime/node with explicit network policy and audit.
- Package-manager and Docker adapters with risk-specific approvals.
- System telemetry with bounded event streams.
- Skill registry and provenance-backed project memory.
- OS-backed secret vault before API-key provider adapters.

## P3 — remote MCP

Create a transport abstraction first. Add authenticated localhost HTTP only when tested, then one remote tunnel provider with HTTPS, visible connection state, revocation, graceful shutdown, and no router/firewall changes. Tailscale, Cloudflare Tunnel and ngrok remain candidates rather than promises.

## P4 — optional web harness / GUI control

Keep browser-backed ChatGPT and GUI computer control optional providers. Do not bypass authentication, export cookies, evade bot protections, or make either mechanism foundational.

## Release engineering

Before calling AstraCommander production-ready: packaging/signing/update strategy, installer tests, crash recovery, schema migrations, accessibility checks, performance budgets, support-bundle redaction, and a complete transitive third-party notice pass.

