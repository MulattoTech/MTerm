# Native terminal resource parity and integrated candidate

User-authorized continuation of existing multi-session/resource roadmap, not a new UI redesign.
Active tree C:\Tools\Dev\MTerm; candidate/native-20260923.

T1: archive refs and consolidate all actual GitHub branch heads; do not import the unpublishable
local workflow by accident. Preserve original feature branches, history, user data and checkpoints.
T2: extend existing native TerminalService to bounded independent resources (default + canvas terminal
IDs). Own each PTY separately. Capture immutable workspace/resource/run IDs in every callback.
Separate run IDs on restart prevent late input/resize/stop from reaching a new shell.
T3: persist metadata only, never terminal text/input/environment secrets. Restart reports STOPPED,
not reattachment. Per-terminal cwd is existing canonical workspace-relative directory. Stop/revoke/
workspace change/shutdown stop only the appropriate owned shells (or all when explicitly intended).
T4: retained native terminal selector/screens tied to canvas resource IDs; switching inspector,
Canvas/Project or terminal does not reset another shell. No auto-start on selection. Keep approved
native shell profiles, permission prompts and no-browser/lazy UI invariant.
T5: red/green service + real ConPTY multi-session and UI tests, bounds/invalid/stale/denied requests,
restart/workspace isolation, original suites, reference gate, packaging and visual fixtures.
T6: update evidence-based roadmap stages with explicit remaining gaps. Commit and publish candidate,
check branch ancestry, then merge via expected-SHA GitHub PR if unblocked. No forced merge/protection
bypass. Source comments and AI record retain all prior authorship and exact tests.

Ruling: user explicitly requests continuing the approved UI/native roadmap and merging all branches;
execute existing multi-terminal requirement inline. Full detachable supervisor/Unix support and
complete per-resource agent graph remain separate tasks, not hidden claims.

## Resume review 2026-09-23

Write/rename/read-back access passed under artifacts/access-check-20260923T184417Z.
The inherited local merge6ff5418 includes all5 published branch heads; none removed.
Recovered native tests had observed red cases for resource separation, output backpressure and unique
resource names, then green local iterations. Resume added same-root epoch/revocation, natural exit/
restart/stale ACK and cross-workspace lifetime checks; all pass. A new live Canvas/Project badge
regression was observed red, then implemented as a transient projection that never overwrites
persisted task/canvas status. No additional provider calls or operating-system scope were introduced.

Ruling: consolidate into candidate/native-20260923 and merge to main only through an ordinary
expected-SHA GitHub merge after the actual local gates pass. Preserve source branches; do not import
the local unpublishable CI archive or imply hosted CI/signing certification.
