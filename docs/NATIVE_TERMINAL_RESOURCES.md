# Native terminal resource contract

Implemented C++/Qt Windows terminal pool. Main APIs are named Backend requests, not a public network
service. Keep future MCP/plugin adapters behind the existing policy boundary.

## Identity and lifetime

`workspaceId` identifies the canonical workspace. `sessionEpoch` changes on every workspace open,
including reopening the same root; UI grants and retained terminal screens reset with the epoch.
`terminalId` is either `default` or the ID of a current canvas node whose kind is `terminal`.
A new random `runId` is minted for every start. Named resources require the matching run token for
write/resize/stop/ack. The old default-terminal compatibility API can omit it; new consumers must not.
Capturing workspace/resource/run in callbacks prevents a late event from reaching a restarted shell.

There are at most eight live shells and at most eight retained screen views. Selecting a terminal
resource never starts it. Closing a stopped view releases its screen, not its resource/metadata.
Stopping one shell does not stop its neighbor. Downgrading to Observe, opening a different workspace
or exiting the app intentionally stops all currently owned shells. A removed card's running shell
remains reachable in the selector so it can be stopped. No GUI-independent reattachment exists yet.

## Request surface

| Request | Input / result |
|---|---|
| pty-list | Scoped, read-only, pages of100 metadata rows; activeCount/maxActive/mayHaveMore |
| pty-start | terminalId, shell, cwd, columns, rows, optional flowControl; returns new runId |
| pty-write | terminalId/runId and strict bounded base64 bytes; rejects stale/oversized input |
| pty-resize | terminalId/runId and positive integral bounded dimensions |
| pty-stop | Stops only the matching active execution; safety stop does not require new execution grant |
| pty-ack | Consumer acknowledges parsed bytes for one run; invalid counts/stale runs rejected |

Start/write/resize require Developer plus a workspace-session terminal.execute grant. The UI
explicitly approves up to eight shells for that session, then each Start is a separate user action.
Shell presets: cmd, pwsh, powershell. Cwd is a canonical existing workspace-relative directory;
this is a starting-directory restriction, NOT a filesystem sandbox for the approved shell.

## Bounded output and GUI behavior

Native PTY I/O runs outside the GUI thread. The reader's pending buffer is capped at1MiB and the
normal service delivery frame at64KiB. UI consumers enable flow control: after256KiB outstanding,
the service pauses delivery until the GUI acknowledges bytes after parsing them. Normal queued
output can overshoot by one delivery block, up to320KiB. Final exit may flush up to1MiB separately.
Input has a64KiB queue/call bound. This is backpressure, not a promise of lossless shutdown or a
completed long-running stress certification. Hidden retained screens still parse bounded output;
no claim that all offscreen processing is suspended is made.

Terminal selector, Canvas cards and Project cards share current lifecycle status. Runtime badges
never mutate saved canvas task/status fields. RUNNING is a process state, not acceptance evidence.
No pane restart occurs when switching tools or Canvas/Project views.

## Persistence and privacy

Only metadata is stored: resource/workspace/run IDs, title, shell preset, relative cwd, lifecycle
timestamps/status and exit code. Input/output/environment values are not stored. On a fresh app
session, interrupted active agent/terminal records become STOPPED across all pages using a scoped
SQL update. Other workspaces and already terminal/DONE records remain unchanged.
The UI currently reads the latest100 terminal metadata rows; ancient settings outside that page
need an explicit per-resource metadata query before claiming complete history UX.

## Boundaries still open

Windows-only transport; no persisted terminal text, user scrollback/search/selection/IME parity,
Unix PTY, detached supervisor, guaranteed bounded historical-Windows teardown, or remote MCP port.
Native QProcess jobs (not ConPTY) retain their separately documented pre-start ownership race.
Authorized commands retain the OS user's access. No security sandbox or production certification.

## Tests

TerminalResourceTests exercises actual independent shells, one-shell stop, run restart/stale tokens,
invalid resource/cwd/dimensions, privacy/restart metadata, all-page scoped recovery, valid/invalid
workspace switches, consumer backpressure, capacity/revocation, same-root epochs and natural exits.
TerminalWorkspaceTests exercises no-autostart selection, real dual-shell identity across views,
view caps, unique resource labels and transient Canvas/Project badges.
