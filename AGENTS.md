# MTerm: instructions for every human and AI contributor

Read README.md, docs/STATUS.md, docs/HANDOFF.md, docs/ROADMAP.md and the relevant module before editing.
The active migration plan is docs/plans/2026-09-22-native-foundation.md.

## Boundaries
- Preserve the Electron reference until native feature parity is proven. Native code lives in native/.
- Never delete user data, credentials, worktrees, or previous checkpoints. Never force-push.
- No public listeners or paid model calls without explicit user authorization.
- Native UI contains no browser engine. Slow I/O and process output must not block the UI.
- All operations are workspace-scoped; execution requires an explicit session grant.
- Native storage is separate from legacy storage. Import must be explicit and non-destructive.
- Feature presence is not proof of correctness. Record tests and limitations honestly.

## Required workflow
1. Inspect git status and current handoff; select a bounded task from the roadmap/issues.
2. Record task ID, actual model/provider (unknown if unavailable), UTC time and scope in docs/ai/changes/.
3. Write a failing test, implement, rerun relevant tests. Preserve regression/security assertions.
4. Keep modules focused; public APIs document invariants, ownership, errors, threading, and units.
5. Update docs/STATUS.md and docs/HANDOFF.md; record exact commands and outcomes.
6. Review staged files for credentials, personal transcripts, generated binaries, and unrelated changes.
7. Commit with AI-Provider, AI-Model, AI-Change, and Tests trailers. Never invent model provenance.

## Comments and provenance
See CONTRIBUTING.md. New AI-authored source files have SPDX-License-Identifier and AI-Change headers.
Explain WHY, security assumptions and edge cases; do not narrate obvious statements or stamp every line.
Existing code retains original attribution. Modifications append a change record; they do not claim whole-file authorship.
AI provenance is a self-reported audit trail, not cryptographic proof. Git commits retain exact line diffs.
No private chain-of-thought, API keys, auth cookies, or raw chat exports belong in the repository.

## Resuming after interruption
Read the latest docs/ai/changes/ record and handoff, inspect git diff, then resume the first unvalidated task.
Do not repeat completed work or count untested features as done. Local artifacts/ and .tools/ are not published.

## Active UX continuation and full scope
Active development is C:\Tools\Dev\MTerm. Old Documents checkouts are recovery-only.
Read docs/UI_DESIGN_CONTRACT.md and docs/SOURCE_PROJECT_FEATURE_MATRIX.md before UI changes.
The Electron interaction model is the user-approved reference; do not regress to full-window tool tabs.
Use docs/templates/FEATURE_UI_PARITY_TEMPLATE.md and the full docs/backlog/requirements-index.json.
DevFleet integration is future opt-in work; read docs/DEVFLEET_INTERFACE_VISION.md and never imply it exists.
