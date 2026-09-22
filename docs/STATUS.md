# MTerm status — native checkpoint 1

2026-09-22: 34 native core QtTest cases, 15 runtime cases and 8 backend cases pass locally on Windows Release. Counts include QtTest fixture cases. Tests cover core permissions/files/storage, native processes, bounded output, cancellation, malformed JSONL, worker-thread persistence and scope checks.

Native desktop, full VT terminal parity, native MCP conformance and complete provider parity are still in progress. The Electron reference is preserved and rebranded; historical reference baseline was 59 unit and 2 E2E tests, but the renamed reference has not yet been revalidated in this checkout.

No performance comparison claim is made. Earlier Electron branding timing and CLI --version timing were unlike workloads. Native benchmarking must specify measurement endpoints and equivalent work.

Read the active native plan and AI change record before editing. User data, toolchains and raw logs remain local and are excluded from GitHub.
