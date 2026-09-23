# MTerm integrated native development candidate

Date:2026-09-23. Active root:`C:\Tools\Dev\MTerm`. Candidate branch:`candidate/native-20260923`.
This is a consolidated development candidate, not a signed production release.

## Published-source integration

The user explicitly requested branch consolidation. The previous interrupted attempt created local
merge `6ff541844abe1bf0d6fa11cc727e5fdbff68a993`, preserving both main and the latest feature history.
All published branch heads listed below are ancestors. No feature branch is deleted or force-pushed.
The older local-only workflow-containing archive is preserved separately, not silently published.

| Published branch at consolidation | Head | Candidate ancestry |
|---|---|---|
| feat/native-core-20260922 | `e82fc49f64958700489c6ffb6c9daf56d78da623` | Verified ancestor |
| feat/native-resume-20260922 | `f132bb6f24893b3557a8f727c4b238a6d0589caf` | Verified ancestor |
| feat/native-ux-20260922 | `6344e948baafd6886f54ba9bb815aae7314feb75` | Verified ancestor |
| feat/native-workbench-roadmap-20260923 | `6203c7a21ad5cbe2cdddde1056ae8ab05f775524` | Verified ancestor |
| main | `f653e6c70950b65d07c24b0c7f149d0a33d0f37a` | Verified ancestor |

## Candidate publication policy

Candidate validation includes current native Release/Debug, roadmap, reference and package checks.
Publish the candidate normally, then use an ordinary expected-head-SHA GitHub merge into main.
If branch protection or permission prevents merging, leave the reviewable candidate and report the
barrier; never override protection or use a force push. The PR discussion records the actual final
merge SHA/state; this source file alone is not proof that a remote merge has already happened.

## What this candidate adds

Independent native resource-owned terminal sessions, per-terminal cwd/shell metadata, stale-run
protection, scoped full-history recovery, invalid-workspace preservation, consumer backpressure,
retained per-resource screens and transient shared lifecycle badges. See NATIVE_TERMINAL_RESOURCES.
The previously working native workbench and full colored roadmap remain included.

## Known integration limits

The inherited hosted-CI recipe remains documentation until authorized workflow-write access exists.
No hosted pass, signed build, Unix support or completed native MCP/provider/DevFleet parity is implied.
All old feature and local recovery histories remain available for audits and continuation.
