# MTerm implemented native architecture

The active implementation is native C++20, Qt Core/Sql/Gui/Widgets and pinned MIT libvterm.
The TypeScript/Electron source remains a reference in apps/desktop and packages/core.
It is not a runtime dependency of the native executable.

## Modules and ownership

| Module | Responsibility | Ownership / thread |
|---|---|---|
| mterm_core | Policy, bounded buffers, canvas validation, UTF-8 files, SQLite | No Widgets dependency; Store is thread-confined |
| mterm_runtime | Captured QProcess jobs, native inventory, bounded JSONL | Calling service thread; no shell interpolation |
| mterm_terminal | Windows ConPTY transport and VT screen abstraction | Service thread plus bounded Win32 I/O threads; screen used by UI |
| mterm_services | WorkspaceSession, JobService, TerminalService, Backend | Worker owns privileged state; Backend is GUI facade |
| mterm_desktop | MainWindow, Graphics View cards, native editor and terminal widgets | GUI thread only; asynchronous named service requests |
| mterm_vterm | Unmodified upstream terminal parser | Original MIT attribution and pinned commit retained |

## Requests and scope

Backend queues requests to its Worker with a request ID and explicit workspaceId.
WorkspaceSession owns the canonical root, permission state, Store connection and canvas revision.
Returned state is limited/paginated. Native writes validate JSON types before conversion; a bad
payload is never coerced into an empty document. Canvas saves compare expected revision.
Files use SHA-256 versions and QSaveFile atomic replacement without direct-write fallback.

## Jobs versus terminals

Captured commands/Git/Codex use ProcessRunner/QProcess, not a fake terminal. Captures are bounded
and cancellation/timeouts explicit. Codex JSONL requires a completed turn as well as clean process
exit; evidence states process outcome, not task acceptance. Workspace/run ownership is captured.

The Terminal tab uses real Windows ConPTY. Reader/writer threads prevent pipe deadlocks;
bounded queues apply backpressure. Suspended shell processes join an owned job before execution.
A native Qt widget displays the libvterm screen; no web view or JavaScript terminal is involved.
Transport currently does not survive application shutdown and is Windows-only.

## Persistence

Native SQLite is separate from legacy user data. Schema 3 preserves compatible settings/audit/
records structures while indexing workspace-scoped JSON data. Upserts guard record kind/workspace;
transactions protect linked changes. Settings, notes, tasks and session metadata survive restart.
Full-history reconciliation, backup/import UI, robust crash recovery and normalized model growth
remain explicit work rather than silently changing an existing user's database.

## Heavy adapters

Model CLIs run only by explicit request and permission. Native Git uses the installed executable;
process inventory uses Windows APIs instead of PowerShell. No default network listener or browser.
Future browser, tunnel and plugin workers must be on demand and capability-scoped.

## Extension guidance

Add testable services before views; reuse Backend requests and immutable workspace/run IDs.
Public APIs must document ownership, error results, bounds, units and thread affinity.
Preserve golden reference behaviors, but do not port unsafe type coercion or global mutable state.
Profile before replacing Widgets with Quick, Git with libgit2, or introducing a new dependency.

## Native UX restoration (2026-09-22)

Active root is C:\Tools\Dev\MTerm. Theme/UI resources and original vector icons are reusable
components. WorkspaceShell owns chrome only, MainWindow routes scoped services, CanvasView owns
viewport/scene presentation, ResourceCard is a single graphics item reused by ProjectView's delegate,
and CommandPalette routes named actions. Inspector factories load a panel once on first selection,
then retain it so navigation does not reconstruct a terminal/editor. No browser engine was added.

The GUI uses a 300ms debounced layout save with expected revisions, preserves newer local changes
during an outstanding save, and shows conflict status. Late scoped responses are filtered. Per-node
independent live resources remain separate future work; cards currently route to retained inspectors.
Source-project matrix, full REQ index and proposed DevFleet contracts define future extensions.

## Workbench document ownership and metrics

EditorDeck owns a bounded vector of per-file QTextDocuments with QPlainTextDocumentLayout and
independent undo/cursor/scroll/version. A single QPlainTextEdit presents the selected document.
Backend responses correlate request ID, workspace generation and buffer key; reloads also validate
UI revision, and user selection takes priority over earlier I/O. MainWindow delegates file UI and
checks the deck's aggregate dirty/pending state. Preferences are isolated beside the data location.

Roadmap Python/HTML is offline development tooling only. Native startup neither executes Python
nor loads the catalog. Evidence-backed progress.json renders Markdown/SVG/HTML/JSON; source/catalog
review fingerprints enforce contributor updates through normal build/package scripts.
