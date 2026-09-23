# MTerm as a primary DevFleet interface — planned contract

Source of this direction: Dylan's explicit request that MTerm eventually become the main way a
person interacts with DevFleet. **No DevFleet service or repository was modified or connected in
this UI pass.** Protocol details below are proposed acceptance requirements, not claims about
DevFleet's current API. Inspect its authorized current interfaces before implementation.

## Experience contract

DF-01: A workspace can opt into a named DevFleet connection. Discovery reports endpoint/version,
authentication state, capability negotiation and health. Offline/unknown is not success/healthy.
DF-02: Task, run, agent, terminal, artifact, worktree and release-gate identities are shared across
Canvas, Project, inspector and DevFleet adapters; selecting a view never restarts a job.
DF-03: A task card shows current state, owner, blockers and evidence freshness. A release/checklist
view links each green result to exact commit/build/environment evidence. Exit zero is not enough.
DF-04: A user can inspect logs, open the owning diff/worktree, answer an approval, pause/cancel an
owned run, and resume only where the backend supports it. Detached UI is different from stopped work.
DF-05: Events have stable run IDs, monotonic sequence/checkpoint metadata and versioned schemas;
reconnect backfills bounded history without duplicating actions or inventing live state.
DF-06: Mutations require explicit, scoped grants. Credentials use OS-backed storage, not canvas
text, prompts, URLs, support logs or committed config. Never silently grant whole-machine access.
DF-07: Client output is bounded/coalesced; histories are paginated; background connections are
optional and stop when disabled. No always-on browser/Node dependency is introduced in the base UI.
DF-08: Performance is measured under real multi-agent/release workloads. UI frame, input and
cancellation latency are measured separately from provider reasoning/network duration.

## Implementation sequence

1. Authorized read-only discovery of current DevFleet contracts; write a compatibility ADR.
2. Define interface-only adapter DTOs and recorded protocol fixtures with no real credentials.
3. Implement read-only health/task/run/evidence views, visible connection state and offline handling.
4. Add approvals/cancellation with authorization, idempotency and audit tests.
5. Add release-monitor/resource graph integration and reconnect/crash tests.
6. Optional orchestration controls only after capability/security/conformance gates pass.

Keep DevFleet as a service integration behind a modular adapter, not copied into MainWindow.
MTerm must remain usable without DevFleet. UI templates and source-project lineage remain shared.
