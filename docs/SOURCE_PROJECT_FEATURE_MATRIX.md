# Source-project lineage and feature-completeness matrix

Basis: the six README/LICENSE snapshots already in `docs/research/`, their pinned revisions in
`SOURCE_AND_LICENSE_AUDIT.md`, the inherited master backlog, and current native/reference source.
The source-description column reports those snapshots; the implementation columns report MTerm.
“Best” is a product goal, not an externally proven ranking. No source feature is declared complete
merely because its source app has it. MTerm is not affiliated with or endorsed by these projects.

| Source project | Source-supported design lineage | Current native MTerm | Remaining / acceptance workstream |
|---|---|---|---|
| wonderwhy-er/DesktopCommanderMCP | Local filesystem/search/edit tools, long-running terminal sessions, previews, bounded tool history and MCP | Scoped files, native commands/PTY, process inventory and audit; current MCP remains TypeScript reference | #5/#6: native MCP wire parity, range/search/patch tools, richer previews and owned long-running sessions |
| rebel0789/codexpro | Explicit allowed repositories, narrow read/write/command/session/handoff controls, compact bounded results and public HTTPS connection options | Canonical workspace boundaries, separate execution approval, bounded process output | #2/#5/#9: fine-grained scopes, safe handoffs, authenticated remote transport and reconnect |
| eneskirca/nodeterm | Infinite canvas, terminals/agents/notes/editors/diffs, shared live sessions across views, worktree groups, continuity, notifications, mobile/voice | Restored native canvas beside inspector, common Project resource projection, notes, real PTY and card actions | #3/#4/#9: per-resource live surfaces, groups/edges/kanban, durable supervisors, worktree binding, mobile/voice optional |
| miuuyy/codex-chatgpt-web | Browser-backed harness/provider boundary described in saved README; harness is not foundational to the local runtime | Native provider supervisor remains CLI-based; no browser-backed provider claim | #5/#9: optional assisted/browser provider only after auth/terms/permission review; never export cookies or evade controls |
| Kyne0328/rel-ai-chatgpt-web-harness (Rel.AI) | Local agency around ChatGPT, task ownership, validation evidence, skills/memory, observable processes, separate Git publishing and recovery | Scoped tasks/session/evidence storage, audit, bounded jobs and honest process-outcome labels | #2/#5/#6/#9: richer task-owned validation, verified artifact evidence, memory/skill provenance and recoverable work-session lifecycle |
| lulu-sk/CodexFlow | Unified CLI workbench across engines, project-organized history, Markdown/search, images/files/@attachments, worktree parallelism, Git panel and usage | Project view shares resource data; native inspector workflow, fixture-tested Codex fresh/resume, read-only Git | #4/#5/#6: full multi-engine controls/history/context, precise diff review, worktree lifecycle, quota/usage capabilities where supported |

## Attribution and reuse boundaries

The existing audit records MIT for DesktopCommanderMCP/CodexPro/codex-chatgpt-web, Apache-2.0 for
Rel.AI/CodexFlow, and BUSL-1.1 for NodeTerm. These are snapshot findings, not a blanket permission
to copy every dependency or asset. **No code, UI artwork or screenshots from those six projects
were copied into this redesign.** The native glyphs and presentation are original MTerm work.
NodeTerm remains a conceptual reference; adopting its source requires a separate license decision.

Actual bundled parser code is MIT libvterm and is separately attributed under native/third_party.
MTerm-owned code remains MIT; Qt/compiler/runtime obligations remain separate. See THIRD_PARTY.md.
Do not replace original attribution with AI attribution. New AI changes retain origin headers and
record dates/provider/model/scope under docs/ai/changes/ with exact Git diffs.

## Full scope and DevFleet

The complete inherited backlog and machine-readable requirements-index.json remain the scope ledger.
This matrix does not collapse hundreds of requirements into six completed rows. Future DevFleet
connectivity is specified in DEVFLEET_INTERFACE_VISION.md as planned work, not a current connection.
Every future PR must identify the source-inspired capability, existing reference behavior, native
acceptance criteria, safety boundary and measured runtime cost using the repository template.
