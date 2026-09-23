# AstraCommander — Master ToDo to Reach 100% of the Product Vision

Date: 2026-09-22
Basis: live repository inspection + original autonomous build mission.
Repository: `C:\Users\Dylan\Documents\AstraCommander`

## Meaning of “100%”

This report treats **100%** as implementing and validating the complete product vision defined in the original mission, not merely finishing the first or second vertical slice. Every feature must be real, security-gated where appropriate, documented, and covered by the relevant automated or manual validation.

Legend:
- [x] confirmed in the current live source or current tests/artifacts
- [~] implemented but incomplete, limited, stale in docs, or not fully validated
- [ ] not yet implemented
- [!] blocked by external state or requires explicit external/user action

## 0. Establish a trustworthy new baseline

- [x] TypeScript strict typecheck passes on the current tree.
- [x] ESLint passes on the current tree.
- [x] Current Vitest suite passes: **59/59** on 2026-09-22.
- [x] Schema migration/integrity infrastructure exists in `AstraStore` through schema version 2.
- [x] Codex fresh-session and resume paths have real smoke evidence.
- [x] Git stage/unstage + managed-worktree creation have an Electron E2E test.
- [~] Handoff, implementation-status, roadmap, architecture, and testing documents are stale relative to newer live source.
- [x] Re-ran the **current** routine gate after the latest second-pass work: typecheck, lint, 59/59 unit tests, build, MCP integration, and 2/2 Electron E2E specs all pass. Evidence: `artifacts/report-baseline-gate-20260922.log`.
- [ ] Update every truth-state document from that exact green result.
- [ ] Create a clean release-staging build that does not preserve stale hashed bundles or source maps.
- [ ] Capture a versioned performance baseline in CI/release artifacts.
- [ ] Create the first human-approved Git commit/tag once the owner wants repository history established.

## 1. Desktop shell and application lifecycle

- [x] Secure Electron window with context isolation, sandboxing, CSP, fixed preload bridge, and navigation restrictions.
- [x] Global command palette exists.
- [~] Window lifecycle exists but is minimal.
- [ ] Persist window size/position/maximized state.
- [ ] Multi-window policy and secondary windows where useful.
- [ ] Settings surface with typed persistent settings.
- [ ] Configurable keyboard shortcuts and conflict handling with terminal/editor shortcuts.
- [ ] Native notifications with per-workspace controls.
- [ ] Deep-link/protocol handler design and validation.
- [ ] Recent workspaces/start screen.
- [ ] First-run onboarding and explicit workspace trust flow.
- [ ] Crash-state detection, recovery UI, and safe-mode launch.
- [ ] Local crash/support bundle generation with automatic secret redaction.
- [ ] Accessibility pass: focus order, screen reader labels, high contrast, keyboard-only navigation.
- [ ] Light theme support while preserving dark-first design.
- [ ] Localization-ready string architecture.

## 2. Workspace runtime

- [x] One trusted workspace root can be selected and persisted.
- [x] Observe-by-default workspace security.
- [ ] Workspace catalog supporting multiple saved workspaces.
- [ ] Multiple roots/folders per workspace where explicitly trusted.
- [ ] Workspace aliases, metadata, tags, last-opened state, and favorites.
- [ ] Per-workspace permission profile and settings.
- [ ] Workspace templates.
- [ ] Open/close/archive/export/import workspace flows.
- [ ] File-watcher-backed workspace change notifications.
- [ ] Workspace-specific recent files/sessions/tasks.
- [ ] Clone/open-existing-repository workflow with explicit user action.
- [ ] Multi-workspace isolation/security tests.

## 3. Infinite canvas — domain and interaction completeness

- [x] Renderer-independent canvas schema.
- [x] XYFlow pan/zoom/minimap and persistent node positions.
- [x] Note/task/agent/evidence/terminal/editor/diff/process/audit/file node kinds exist in schema.
- [x] Agent/Terminal/Editor live surfaces can be opened inside spatial nodes.
- [x] Diff and Process resources can be pinned to the canvas.
- [~] Edge schema exists, but renderer currently supplies no actual edges.
- [ ] Create/render/delete/edit edges.
- [ ] Context edges between agents, tasks, files, terminals, evidence, and worktrees.
- [ ] Multi-selection and box/lasso selection.
- [ ] Node resizing with persisted dimensions.
- [ ] Group/container nodes.
- [ ] Group move/collapse/ungroup behavior.
- [ ] Alignment/snapping/guides.
- [ ] Canvas search/filter/jump-to-resource.
- [ ] Full keyboard navigation between nodes.
- [ ] Context menus.
- [ ] Duplicate/copy/paste nodes where sensible.
- [ ] Pin/unpin and collapse/minimize behavior in the actual spatial UI.
- [ ] Undo/redo transaction model.
- [ ] “Send to agent.”
- [ ] “Attach as context.”
- [ ] “Open terminal here.”
- [ ] “Open diff.”
- [ ] “Create worktree.”
- [ ] “Spawn agent.”
- [ ] File/Artifact node specialized viewer.
- [ ] Audit/Tool-call specialized spatial node.
- [ ] Browser node type and live surface.
- [ ] Group node type.
- [ ] Virtualize/suspend expensive off-screen nodes.
- [ ] Spatial index for hundreds/thousands of nodes.
- [ ] Stress test dozens/hundreds of nodes, multiple terminals, agents, and large logs.

## 4. Project Mode completeness

- [x] Canvas and Project Mode share the same persisted workspace state.
- [~] Current Project Mode is primarily a card-oriented alternate view.
- [ ] Project sidebar/workspace switcher.
- [ ] Session list and agent tabs.
- [x] Provider selector exists in Agent inspector.
- [x] Prompt composer exists.
- [ ] Rich Markdown transcript rendering.
- [ ] Image paste.
- [ ] Drag/drop attachments.
- [ ] `@file` attachment picker.
- [ ] Context attachment review/removal.
- [ ] Searchable session history.
- [x] Codex one-click resume path exists.
- [~] Claude resume path exists in code but has not been validated with authenticated Claude.
- [x] Git status/diff workbench exists.
- [ ] Full branch controls.
- [~] Worktree creation exists; full worktree lifecycle does not.
- [x] Task panel with checklist/blockers/acceptance criteria/evidence exists.
- [ ] Provider usage/quota display where available.
- [ ] Traditional project layout optimized for non-spatial workflows.

## 5. Filesystem and editor surface

- [x] Rooted directory listing.
- [x] Rooted UTF-8 file read/write.
- [x] Canonical path/junction/symlink/UNC/ADS protections.
- [x] Optimistic stale-write detection.
- [x] Monaco editor loads lazily.
- [ ] File create.
- [ ] Directory create.
- [ ] Rename/move.
- [ ] Copy.
- [ ] Explicit high-risk delete/recycle flow.
- [ ] Read byte/range for large files.
- [ ] Stat/metadata tool.
- [ ] Filename search.
- [ ] Text search with bounded/paginated results.
- [ ] Patch-based edit operation.
- [ ] Replace-range operation.
- [ ] Multi-buffer editor tabs.
- [ ] Dirty-state indicators and unsaved-change workflow.
- [ ] File-system watching and external-change merge/conflict UX.
- [ ] Encoding and EOL detection/control.
- [ ] Large-file streaming/view mode.
- [ ] Binary/image/artifact previews.
- [ ] Syntax-language detection without loading the entire Monaco language catalog.
- [ ] “Open diff” from editor.
- [ ] “Attach file/range as agent context.”
- [ ] Jump-to-file/line from audit, Git, tasks, and evidence.

## 6. Terminal/runtime sessions

- [x] Real PTY-backed PowerShell terminal.
- [x] Input/output/resize/stop.
- [x] Explicit session approval.
- [ ] Shell selector and saved shell profiles.
- [ ] Per-terminal cwd selection.
- [ ] Per-terminal bounded environment profile.
- [ ] Search in terminal scrollback.
- [ ] Bounded scrollback/ring buffer and memory budget.
- [ ] Multiple named terminals with persistent metadata.
- [ ] Terminal status badges mapped to process/session state.
- [ ] Persist terminal metadata across restart.
- [ ] External supervisor/re-attach design so terminals can survive UI/app restart where feasible.
- [ ] Native Windows ConPTY supervision implementation for final lean architecture.
- [ ] Unix PTY adapter for Linux/macOS.
- [ ] Crash/reconnect tests.
- [ ] Long-running process tests that do not misclassify “still alive” as failure.

## 7. Git and worktree orchestration

- [x] Status.
- [x] Diff.
- [x] Recent log.
- [x] Worktree listing.
- [x] Explicit-path stage.
- [x] Explicit-path unstage.
- [x] Policy-gated managed worktree creation.
- [x] Dedicated/pinnable Diff node exists.
- [ ] Split and unified visual diff modes.
- [ ] Hunk-level stage/unstage.
- [ ] Safe accept/reject/revert-hunk workflow with confirmation.
- [ ] Branch listing.
- [ ] Branch creation.
- [ ] Branch switching with dirty-tree safeguards.
- [ ] Managed worktree remove.
- [ ] Worktree prune/repair.
- [ ] Worktree rename/move if supported safely.
- [ ] Bind task/group/agent/terminal/diff resources to a worktree.
- [ ] Commit preparation UI that never commits without explicit permission.
- [ ] Commit capability with explicit human grant.
- [ ] Push capability with stronger explicit human grant.
- [ ] Merge/rebase assistance and conflict detection.
- [ ] Conflict-resolution Diff node.
- [ ] History/blame browsing.
- [ ] Preserve invariant: no silent commit, push, force-push, branch deletion, or worktree deletion.

## 8. Process and system runtime

- [x] Process inspector exists.
- [x] Current Windows process snapshot reads top processes.
- [x] Only AstraCommander-owned terminal/agent children can currently be stopped.
- [x] Process-stop permission grant exists.
- [ ] Native process enumeration without spawning PowerShell.
- [ ] Process tree/parent-child view.
- [ ] CPU rate sampling over time, not just cumulative CPU.
- [ ] Working-set/private-memory sampling.
- [ ] Uptime/start time.
- [ ] Full executable/command line where accessible.
- [ ] Owned-process stdout/stderr attachment.
- [ ] Start-process service under policy.
- [ ] Restart owned process where appropriate.
- [ ] Per-process resource graph.
- [ ] Event-driven process lifecycle updates.
- [ ] System information: CPU, memory, disks, network, OS, runtimes.
- [ ] Battery and temperatures where platform APIs permit.
- [ ] Cross-platform process/system adapters.

## 9. Tasks, agency, evidence, and local memory

- [x] Durable Task model.
- [x] Task states.
- [x] Owner agent session.
- [x] Checklist.
- [x] Blockers.
- [x] Acceptance criteria.
- [x] Linked evidence.
- [x] Manual evidence creation.
- [x] Provider completion creates evidence.
- [ ] Task priorities.
- [ ] Task dependencies.
- [ ] Task ordering/queues.
- [ ] Task due dates/milestones where useful.
- [ ] Task delete/archive with audit.
- [ ] Evidence viewer by type: screenshot/artifact/test/hash/browser/process.
- [ ] Evidence verification/re-run hooks.
- [ ] Automatic SHA-256 artifact evidence.
- [ ] “Definition of done” evaluator based on explicit acceptance criteria.
- [ ] Project Memory model with provenance/source/timestamp/confidence.
- [ ] Human review/edit/delete of memory.
- [ ] Never promote arbitrary model output to trusted memory without provenance.
- [ ] Skill registry, discovery, provenance, versioning, and workspace enablement.

## 10. Agent provider abstraction and multi-agent runtime

- [x] Provider-neutral manifest/capability model.
- [x] Explicit agent session state machine.
- [x] Cancellation.
- [x] Durable sessions and restart reconciliation.
- [x] Task binding.
- [x] Codex CLI fresh session.
- [x] Codex streamed output.
- [x] Codex persisted thread ID.
- [x] Codex resume proven with real smoke artifact.
- [~] Claude Code provider is implemented with safe-mode/plan/no-tools and resume support.
- [!] Claude Code real model validation is blocked until Claude CLI authentication is completed; the current smoke artifact shows “Not logged in”.
- [ ] Gemini CLI provider.
- [ ] Local OpenAI-compatible HTTP provider.
- [ ] Ollama provider.
- [ ] LM Studio provider.
- [ ] Assisted/Paste provider UI.
- [ ] Provider model selector.
- [ ] Reasoning/effort selector where a provider supports it.
- [ ] Capability-driven UI rather than provider-specific assumptions.
- [ ] Attachment support.
- [ ] Provider tool-call bridge under central policy.
- [ ] Provider usage/token/quota reporting.
- [ ] Queued sessions.
- [ ] Retry/backoff.
- [ ] Multiple concurrent agents with explicit limits.
- [ ] Multi-agent task teams/groups.
- [ ] Agent/worktree visual binding.
- [ ] Agent/terminal/file/diff context graph.
- [ ] Persistent provider process/broker where supported to avoid per-prompt CLI cold-start overhead.
- [ ] Provider conformance tests using recorded fixtures plus opt-in real-provider smokes.

## 11. MCP service — complete tool surface

Current stdio server only exposes the validated core subset. To satisfy the original MCP vision:

### Filesystem
- [x] Read file.
- [x] Write file.
- [ ] Read range.
- [ ] List directory through MCP.
- [ ] Stat.
- [ ] Search filenames.
- [ ] Search text.
- [ ] Create file.
- [ ] Create directory.
- [ ] Patch file.
- [ ] Replace range.
- [ ] Move.
- [ ] Copy.
- [ ] High-risk delete.

### Terminal/process
- [x] Direct bounded command execution.
- [ ] Create persistent terminal.
- [ ] Write terminal.
- [ ] Read terminal output.
- [ ] Resize terminal.
- [ ] Terminate terminal/session.
- [ ] Long-running command handles.
- [ ] Process list.
- [ ] Process inspect.
- [ ] Process monitor/subscribe.
- [ ] Process start.
- [ ] Policy-gated terminate/restart.

### Git
- [ ] MCP status/diff/log.
- [ ] Branches/worktrees.
- [ ] Stage/unstage.
- [ ] Managed worktree create/remove.
- [ ] Commit preparation.
- [ ] Merge/rebase assistance.
- [ ] Explicitly gated commit/push.

### Other categories
- [ ] Package-manager adapters.
- [ ] Docker tools.
- [ ] Browser automation tools.
- [ ] System information tools.
- [ ] Plugin tool registration.
- [ ] MCP client mode for connecting to other MCP servers.
- [ ] Resource/prompt surfaces where they provide real product value.

## 12. Permission/security system to full specification

- [x] Central policy evaluator.
- [x] Observe profile behavior.
- [x] Developer profile behavior.
- [x] Session grants for terminal/agent/worktree/process kill.
- [~] Code recognizes additional profile names, but product-level Autonomous Developer / Full Control behavior and UI are incomplete.
- [ ] Autonomous Developer profile.
- [ ] Full Computer Control profile.
- [ ] User-defined Custom profile.
- [ ] ALLOW ONCE.
- [x] ALLOW FOR SESSION for several capabilities.
- [ ] ALLOW FOR WORKSPACE.
- [ ] ALWAYS ALLOW.
- [x] DENY.
- [ ] ALWAYS DENY.
- [ ] Persisted/revocable grants.
- [ ] Grant expiry UI.
- [ ] Per-agent permissions.
- [ ] Per-provider permissions.
- [ ] Per-tool/operation permissions.
- [ ] Per-path permissions.
- [ ] Target-host/network permissions.
- [ ] Risk-level model.
- [ ] Human-readable approval preview for every dangerous request.
- [ ] Filesystem-delete capability and stronger risk class.
- [ ] Git commit/push capabilities separated from generic Git write.
- [ ] Command parser/classifier for shells, pipes, redirection, elevation, networking, destructive argument patterns.
- [ ] Security fuzz/property tests for malformed IPC, MCP, paths, and commands.

## 13. Secret management

- [x] Structured redaction primitives.
- [ ] OS-backed secret vault abstraction.
- [ ] Windows Credential Manager / DPAPI or Electron/native safe storage migration path.
- [ ] macOS Keychain adapter.
- [ ] Linux Secret Service adapter.
- [ ] Secret references in provider/workspace configuration.
- [ ] Never return secret values to ordinary renderer surfaces.
- [ ] Redaction in errors, audit, support bundles, telemetry, copied diagnostics.
- [ ] Credential rotation/revocation UX.
- [ ] Security tests proving secrets never enter logs/audit exports.

## 14. Audit and observability

- [x] Durable audit store.
- [x] Human-facing Audit inspector.
- [x] MCP and desktop can share audit storage.
- [~] Current AuditEvent is intentionally small and omits much of the final requested metadata.
- [ ] Workspace/session/agent/provider/model fields.
- [ ] Requested operation and normalized parameters without sensitive payloads.
- [ ] Exit status/error class.
- [ ] Evidence reference.
- [ ] Structured log levels.
- [ ] Paginated/virtualized audit querying rather than loading broad histories.
- [ ] Search/filter/export.
- [ ] Running agents/terminals/processes dashboard.
- [ ] Queued tool calls and approval queue.
- [ ] Resource usage metrics.
- [ ] Provider quota/token metrics.
- [ ] Local performance counters.
- [ ] Redacted support-bundle generator.

## 15. Remote MCP

- [x] Local stdio transport.
- [ ] Transport-neutral internal MCP service boundary.
- [ ] Local Streamable HTTP transport.
- [ ] Authentication for HTTP.
- [ ] Session/token lifecycle.
- [ ] TLS/HTTPS strategy.
- [ ] Visible connection-state UI.
- [ ] Revocation.
- [ ] Graceful shutdown/reconnect.
- [ ] Cloudflare Tunnel adapter.
- [ ] Tailscale adapter.
- [ ] ngrok adapter.
- [ ] No-public-endpoint-by-default tests.
- [ ] Remote integration tests using a local/mock authenticated transport.
- [ ] Never modify router/firewall silently.

## 16. Browser node and browser automation

- [ ] Browser session runtime in a separate/on-demand process.
- [ ] Browser Node.
- [ ] Launch/navigate.
- [ ] DOM inspect.
- [ ] Click/type.
- [ ] Screenshot.
- [ ] Controlled evaluate.
- [ ] Console/network log capture.
- [ ] Session/profile persistence policy.
- [ ] Explicit outbound-network/browser-control permissions.
- [ ] Browser assertions as ValidationEvidence.
- [ ] Browser E2E tests.
- [ ] Resource caps and automatic shutdown when unused.

## 17. Optional ChatGPT Web/browser harness

- [ ] Assisted/Paste workflow with human-controlled submission.
- [ ] Provider boundary for browser-backed authenticated sessions.
- [ ] Feasibility/terms review at implementation time.
- [ ] Never export cookies or bypass authentication.
- [ ] Never implement CAPTCHA/bot-evasion behavior.
- [ ] Clearly mark web-harness fragility and disable by default.
- [ ] Tool relay only through the central policy engine.
- [ ] Automated compatibility detection/failure fallback.

## 18. Optional computer-use / GUI automation

- [ ] Disabled-by-default computer-control provider.
- [ ] Screenshot.
- [ ] Window enumeration.
- [ ] Window focus.
- [ ] Mouse move/click.
- [ ] Keyboard input.
- [ ] Explicit visible per-session grant.
- [ ] Revocation/kill switch.
- [ ] Full audit trail.
- [ ] Safe test harness isolated from destructive actions.

## 19. Package managers and Docker

- [ ] Common project-local package-manager abstraction: npm/pnpm/yarn/pip/uv/cargo.
- [ ] OS/system package adapters only with stronger permission.
- [ ] winget/apt/brew adapter where available.
- [ ] Dependency-install preview and working-directory display.
- [ ] Docker container list/inspect/log/start/stop/restart.
- [ ] Image list/inspect.
- [ ] Compose support.
- [ ] Explicit stronger gate for image/volume deletion and prune.
- [ ] Resource/timeout/output caps.
- [ ] Integration tests against disposable fixtures.

## 20. Plugin architecture

- [ ] Formal plugin manifest and version contract.
- [ ] AgentProvider plugin interface.
- [ ] MCPToolProvider plugin interface.
- [ ] NodeType plugin interface.
- [ ] TunnelProvider plugin interface.
- [ ] BrowserProvider plugin interface.
- [ ] ComputerControlProvider plugin interface.
- [ ] SystemIntegration plugin interface.
- [ ] SkillProvider plugin interface.
- [ ] Capability declarations/permissions for plugins.
- [ ] Trusted local plugin loading.
- [ ] Signature/trust policy before third-party distribution.
- [ ] Prefer out-of-process plugins for crash/security isolation.
- [ ] Compatibility/version negotiation.

- [ ] Plugin lifecycle and resource budgets.

## 21. Session continuity and recovery

- [x] Canvas/workspace persistence.
- [x] Task/evidence persistence.
- [x] Agent metadata persistence.
- [x] Codex thread resume.
- [x] Interrupted agent records reconcile to STOPPED.
- [ ] Terminal process survival/re-attach across UI/app restart.
- [ ] Persist owned-process supervisor state safely.
- [ ] Recover worktree/task/agent relationships after crash.
- [ ] Remote session reconnect.
- [ ] Browser session recovery where safe.
- [ ] Database backup/restore.
- [ ] Automated crash-injection recovery tests.
- [ ] Recovery screen explaining what was restored versus terminated.

## 22. Persistence/data model

- [x] Shared `AstraStore`.
- [x] WAL/busy timeout/foreign keys.
- [x] Versioned schema migrations.
- [x] Migration ledger.
- [x] Quick integrity check.
- [ ] Typed relational tables for workspaces, tasks, sessions, evidence, grants, plugins, memory, and resources as scale requires.
- [ ] Paginated/indexed queries instead of loading all JSON records by kind.
- [ ] Atomic transaction boundaries across linked task/evidence/session updates.
- [ ] Database backup/export.
- [ ] Restore/import.
- [ ] Corruption recovery workflow.
- [ ] Migration rollback/backup policy.
- [ ] Multi-workspace isolation/indexing.
- [ ] Soak tests with large histories.

## 23. Performance and lean-runtime definition

- [x] Current Electron first-visible baseline captured locally.
- [x] Current memory/process baseline captured locally.
- [x] Codex CLI warm local startup lower-bound captured locally.
- [ ] Adopt the budgets in `docs/NATIVE_PERFORMANCE_REFACTOR_REPORT.md`.
- [ ] Automated cold/warm startup benchmark.
- [ ] Idle memory benchmark.
- [ ] Process-count budget.
- [ ] UI frame-time/input-latency benchmark.
- [ ] Tool-dispatch overhead benchmark.
- [ ] Terminal input-to-paint benchmark.
- [ ] Large-workspace/canvas benchmark.
- [ ] Large-audit/log benchmark.
- [ ] Provider-launch overhead benchmark.
- [ ] CI/regression thresholds.
- [ ] Complete native Qt/C++ migration if the stated CLI-class memory/startup goals remain non-negotiable.

## 24. Cross-platform runtime

- [x] Windows is the currently validated platform.
- [ ] Windows native core adapters.
- [ ] Linux process/PTY/filesystem/secure-storage adapters.
- [ ] macOS process/PTY/filesystem/secure-storage adapters.
- [ ] Path/security equivalence tests per platform.
- [ ] Git/worktree tests per platform.
- [ ] Provider discovery per platform.
- [ ] Native packaging per platform.
- [ ] Platform-specific limitations documented honestly.

## 25. Release engineering

- [ ] Clean production build directory rather than additive development output.
- [ ] Disable release source maps or ship them separately/private.
- [ ] Remove stale hashed build artifacts from release staging.
- [ ] App icons/version metadata/about page.
- [ ] Windows installer.
- [ ] Code signing.
- [ ] Update strategy with signed manifests and rollback.
- [ ] Linux packages.
- [ ] macOS app bundle/notarization.
- [ ] SBOM.
- [ ] Complete transitive dependency/license notices.
- [ ] Qt licensing compliance if native migration is adopted.
- [ ] Release channels.
- [ ] Backup before incompatible database migration.
- [ ] Uninstall behavior preserving/removing user data by explicit choice.
- [ ] Reproducible or at least auditable release pipeline.

## 26. Test/security/reliability completion gate

- [x] Unit tests for current core domains.
- [x] MCP v2 integration test for current tool subset.
- [x] Electron desktop E2E exists.
- [x] Git-workbench E2E exists.
- [x] Real Codex smoke and resume artifacts exist.
- [!] Real Claude model smoke requires login/authentication.
- [ ] All new native/service APIs unit-tested.
- [ ] All dangerous permissions have deny/allow/expiry/revoke tests.
- [ ] IPC fuzz/malformed-payload tests.
- [ ] MCP JSON-RPC conformance tests.
- [ ] Path fuzzing/canonicalization tests across all supported platforms.
- [ ] Database migration matrix tests.
- [ ] Crash recovery tests.
- [ ] Update/rollback tests.
- [ ] Representative long soak test.
- [ ] 100+ canvas-node performance stress.
- [ ] Multiple live terminals + multiple agents stress.
- [ ] Large-repository Git/file-search benchmark.
- [ ] Security review and dependency audit before 1.0.
- [ ] Accessibility regression checks.

## Final 1.0 definition of done

AstraCommander reaches this report's “100%” only when:

1. the complete original product surface above is implemented, or an intentionally removed requirement is documented as a conscious product decision;
2. every dangerous capability is centrally policy-gated, visible, auditable, and revocable;
3. the application recovers honest state after crash/restart;
4. provider, MCP, Git, terminal, process, browser, plugin, and remote paths have appropriate validation;
5. performance budgets are met on reference hardware;
6. release packaging is signed/auditable and contains no development bloat;
7. all documentation accurately describes the exact shipped implementation; and
8. the full release gate is green from a clean machine/install.

## Recommended execution order from the current tree

1. **Freeze truth:** refresh stale handoff/status docs and establish a clean green baseline for the newer source.
2. **Lock performance budgets:** adopt startup/memory/input/tool-dispatch budgets before more UI surface is built.
3. **Begin native strangler migration:** preserve current behavior/tests but move the final shell/core toward Qt/C++ as detailed in the companion report.
4. **Finish P1 functionality on portable service interfaces:** worktree lifecycle, true diff/hunk workflows, process telemetry, canvas edges/groups/search, provider feature completeness.
5. **Complete core tool surface:** filesystem/search/terminal/process/Git MCP services.
6. **Complete security:** granular/persisted permissions and native secret storage.
7. **Complete provider/local agency:** authenticated Claude proof, Gemini/local providers, context attachments, usage, multi-agent orchestration.
8. **Add advanced on-demand runtimes:** browser, Docker/package tools, remote MCP/tunnels, optional GUI computer use.
9. **Finish plugins/memory/skills.**
10. **Cross-platform + release engineering + performance/security certification.**

Do not add advanced Chromium-heavy features to the always-on core while the lean-runtime migration is in progress.
