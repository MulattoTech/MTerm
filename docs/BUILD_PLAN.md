# AstraCommander first vertical slice implementation plan

Goal: a working local Windows desktop workspace with spatial tools, durable state, and inspectable policy decisions.
Spec: the autonomous build mission supplied by the user on 2026-09-22.
Execution: inline on MulattoTechBox, dedicated build/first-vertical-slice branch; no commit/push.

Architecture: Electron sandboxed React renderer + minimal typed preload -> main-process workspace runtime -> validated services shared with current MCP v2 stdio. SQLite WAL owns workspace/layout/session/audit state. Node-pty provides real ConPTY. XYFlow is only the view adapter; domain nodes do not depend on it.

Constraints: no unrelated repositories, no global installs, no public endpoints, no cloud credentials or billable provider calls. Shell execution is explicitly approved OS-user code execution, not a filesystem sandbox. Browser harness is not essential; assisted provider is the first honest provider.

Review focus: symlink/junction/hardlink escapes; renderer privilege escalation; stale file saves; interrupted sessions; workspace-switch layout races.

- [ ] Foundation: root pnpm workspace, apps/desktop, packages/core; failing security tests then canonical paths/policy/redaction/schema implementation.
- [ ] Runtime: failing integration tests then SQLite, atomic checked file saves, bounded real PTY/process lifecycle and durable audit.
- [ ] MCP: real SDK client + stdio subprocess tests, then tool service adapter and CLI. No network listener.
- [ ] Desktop: Playwright flow first; Electron shell/preload, workspace dialog, canvas, file browser/editor, terminal, note, audit and permission approval.
- [ ] Continuity: renderer refresh + full restart E2E; preserve layout/drafts, mark ended processes honestly.
- [ ] Extensions: Git read-only status/diff, tasks, assisted provider, shared Project Mode, only where validated.
- [ ] Quality: typecheck, lint, unit/integration, production build, E2E, visible app launch, self-review and evidence-backed handoff.

Interfaces: WorkspaceRuntime.invoke(tool,input,actor) -> Promise<JSON result>; Store owns Workspace/Canvas/AuditEvent; TerminalManager owns PID/terminal state; DesktopBridge exposes fixed invoke/listen methods. Policy configuration is local-human-only, never an MCP tool.
