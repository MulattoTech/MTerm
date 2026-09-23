# MTerm native security model and limits

Experimental preview, not an OS sandbox or a completed security audit.

- New/opened native workspaces are Observe; Developer enables scoped editing. Execution requires
  an additional terminal/agent grant. Profile downgrade cancels owned running work.
- Backend requests carry workspace IDs. Files/records/jobs validate scope before operations;
  native JSON mutation fields now require correct types before converting them.
- File paths reject traversal, absolute/drive/ADS paths, aliases/reparse points, protected metadata,
  hardlinks and invalid UTF-8. Hash-based versions detect stale writes; atomic replacement is used.
- SQLite operations run on the service thread, scope records by workspace and preserve IDs/kinds.
- No privileged OS APIs are exposed through arbitrary renderer scripting. Native UI is local code;
  future plugins and remote clients must receive narrower capabilities, not arbitrary Backend calls.
- Captured process arguments are structured, not interpolated into a shell. An approved executable
  still runs as the OS user. A working directory is not filesystem isolation.
- ConPTY creates its shell suspended and attaches the job before resuming it. QProcess captured
  jobs currently attach after start: race-free descendant ownership is NOT yet guaranteed there.
- JSONL/provider output is bounded. Exit zero alone does not indicate task acceptance.
- Prompt/model content is not stored in audit merely for completeness; local smoke/evidence data
  and test profiles are ignored by Git. No credential extraction or public listener is implemented.

## Required further hardening (issue #2)

Handle-anchored path operations against hostile races; deterministic pre-start QProcess ownership;
late-callback and cross-workspace cancellation tests; terminal teardown bounds; full-history startup
reconciliation; stricter request schemas; environment/argument secret redaction; all failure paths.
Native process approval must never be described as sandboxing.

## Release and integration barriers

Native secret vault, authenticated remote MCP, plugin isolation, signing/updates and complete
runtime redistribution/license review remain incomplete. The local staging script produces an
unsigned developer preview, not a public release. Hosted workflow installation currently requires
additional authorized GitHub workflow-write access; this barrier was not bypassed.

The original AstraCommander folder and active legacy databases remain untouched.
