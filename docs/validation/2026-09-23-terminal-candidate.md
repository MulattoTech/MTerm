# Native terminal candidate validation — 2026-09-23

## Recovered work and access

Actual RDC create/write/read-back/rename/read-back passed before source changes. Proof retained at
artifacts/access-check-20260923T184417Z. The previous failed attempt left local merge6ff5418 and
uncommitted terminal work. The complete source and all refs were backed up under
artifacts/checkpoints/terminal-resume-20260923T184521Z before continuation. Original source/history/
user data remain intact; one unreadable formatter tmp is explicitly excluded and untouched.

All5 actual published branch heads are ancestors of the candidate; see CANDIDATE.md for SHA table.
No local-only unpublishable workflow archive is imported. Remote merge is separately recorded in
GitHub; successful local ancestry alone does not claim main has been updated.

## Native implementation scope

Eight independent resource-owned Windows ConPTY shells, separate run IDs, canonical relative cwd,
bounded retained screen views, explicit workspace-session approval, no-autostart selection,
metadata-only persistence, full-history recovery and acknowledgement output backpressure.
Canvas/Project/selector display live status without modifying persisted canvas status.
No paid provider or DevFleet operation was performed.

## Observed iteration results

Recovered red/green logs were inspected for resource identity, GUI switching, output bounds and
unique naming. Fresh continuation runs added and passed same-root epoch/revocation, natural exit7/
restart/stale ACK, and cross-workspace late-action regressions. Live badge acceptance was observed
failing first (empty status instead of RUNNING) then implemented and passed.

Formatted Windows Release and Debug both passed9/9 CTest suites in iteration mode. Per build:
core40, runtime15, backend22, desktop7, UX14, workbench10 pass/1 deliberate visual-fixture skip,
terminal11, terminal-resource13, terminal-workspace6. **138 pass/1 skip** includes lifecycle/data
rows; it is not138 independent E2E scenarios. Windows-only tests are explicitly skipped elsewhere.
The old guarded iteration log is not substituted for the final default roadmap-aware gate.

Real Windows `twoLiveTerminalCardsKeepDifferentShellState` visual fixture passed separately
(3 QtTest lifecycle/test outcomes). It used real isolated cmd shells and a generic display label.
The screenshot under docs/images/native-terminal-20260923/ is actual native output, not a mockup.

## Safety and semantics exercised

- Different per-shell environment values and output remain isolated; stopping one preserves another.
- Old run IDs cannot stop/write/resize/ack a replacement execution.
- Same-root reopen changes epoch, revokes approvals, resets Observe and stops old shells.
- A wrong workspace cannot operate another run; invalid proposed roots do not kill current shells.
- Malformed resource/kind/cwd/dimensions/base64/ack counts are rejected.
- Latest-page limits do not leave older active agent/terminal database rows RUNNING after recovery;
  seeded250-row histories per kind become STOPPED while DONE/other-workspace records remain unchanged.
- UI transport pauses at a256KiB high-water mark (one64KiB frame overshoot), resumes after consumer
  acknowledgement, and rejects over-ack. This is not scrollback, full soak or lossless teardown proof.
- Eight-view/eight-shell caps preserve resources and existing live shells.
- Live badges are ephemeral; they do not award task completion or rewrite canvas status.

## Final gates

Final default-gated Release/Debug, roadmap, reference and packaged results are appended after actual
execution. No iteration bypass or mere presence of a test is counted as release evidence.

## Remaining limits

No terminal process survival across app exit, Unix transport, persisted terminal input/output,
complete scrollback/selection/IME, certified bounded older-Windows teardown, full historic metadata
UI beyond100 rows, native MCP/provider/Git mutation parity, live DevFleet adapter, hosted CI, signed
installer or production security certification. Approved shells retain OS-user access.
Timing samples, when recorded, begin inside main() on isolated empty workspaces, not before OS
loading and not during real agents/terminal workloads; they are never an equivalent CLI comparison.

## Final observed default-gated results

Windows Release and Debug each passed 9/9 CTest suites with the default roadmap check enabled.
Per configuration: 138 QtTest outcomes passed and one deliberate screenshot fixture was skipped;
counts include lifecycle functions and data rows, not 138 independent end-to-end scenarios.
Roadmap tests passed 25/25; source/catalog/generated checks and provenance links passed.
The preserved reference passed typecheck, lint, 59 units, build, MCP integration and two E2E tests.
The offline browser dashboard validator passed. No paid model calls were performed.

Native staging contained 17 payload files totaling 32,766,695 bytes, excluding its manifest.
Five isolated clean-PATH launches and SHA-256 checks completed. This is an unsigned developer preview.
Logs: artifacts/candidate-default-release.log, candidate-default-debug.log,
candidate-reference-gate.log, candidate-package.log and candidate-package-smoke.log.

The printed smoke reported median internal workspace readiness 55ms and first paint 655ms.
Do not interpret the latter as validated startup performance: the inherited recorder takes a
screenshot before reading the first-paint counter, so capture-triggered painting can affect it.
55+600ms matches the 600ms capture delay; a separated pre-capture/external timer needs validation.
The raw samples.json later returned EPERM on re-read and was left untouched; the printed log is retained.
No broad permissions were changed and no performance-completion credit was added.

Weighted maturity changed 17.36% to 17.78%, with the same baseline weight of 19,220 and 472 scored
requirements (496 total rows, 24 historical exclusions). There are now 21 complete and three blocked.
Calendar ETAs remain unscheduled. Scrollback, crash reattachment and cross-platform parity remain open.
