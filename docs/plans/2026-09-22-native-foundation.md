# MTerm native foundation implementation plan

Goal: begin the C++/Qt migration with runnable, testable native services and a native desktop, preserve the working Electron reference, and make GitHub a self-contained multi-AI workspace.
Architecture: C++20/Qt6 library modules own policy, bounded file I/O, SQLite, process supervision and domain validation. Native desktop consumes services. Heavy optional adapters remain separate. Existing Electron code is the reference, not discarded.
Tech stack: CMake, Ninja, Qt Core/Gui/Widgets/Sql/Test; Windows MinGW; Python only for development scripts.
Spec: docs/NATIVE_PERFORMANCE_REFACTOR_REPORT.md plus the user's explicit MTerm rename/migration request.

## Rulings
- This is incremental, not a claim of complete parity. Do not erase the original AstraCommander directory.
- First native presentation uses Qt Widgets/Graphics View rather than QML. Existing model/view and text widgets minimize implementation and dependency weight. A Qt Quick canvas remains a benchmark-driven option, not a prerequisite.
- Old timing report compares visible Electron branding with CLI --version; it is NOT a fair proof of CLI superiority. All new budgets are provisional targets until equivalent-workload measurement.
- Legacy ASTRA_* IPC/environment/data names remain compatibility identifiers; visible branding becomes MTerm.
- Original AI identities are not reconstructable with certainty. Record imported baseline as prior AI-assisted work, model unknown.

## Tasks and acceptance
- [ ] T0: preserve/import baseline and current reports into GitHub; no secrets/build/dependency trees.
- [ ] T1: native build with failing behavioral tests, then policy, path safety, bounded buffers and state validation.
- [ ] T2: SQLite migrations/records/audit with transactions, pagination and recovery tests.
- [ ] T3: asynchronous bounded process capture, native Windows inventory, Git and read-only provider adapters.
- [ ] T4: native desktop with workspace, canvas/notes/tasks, safe editor, process and audit views.
- [ ] T5: native terminal/PTY capability as feasible; label incomplete VT/continuity behavior explicitly.
- [ ] T6: measure launch/process memory without paid provider calls; capture smoke evidence.
- [ ] T7: publish code, CI, provenance, roadmap issues, current status and exact resume instructions.

## Review focus
Traversal/reparse/hardlink escape; stale writes; broken/forward-version databases; output flood/cancel/child cleanup; mixed-workspace or mixed-agent state. Each requires tests or an explicit limitation before shipping.

## Execution
Use inline test-first implementation. Validate each task before marking complete. Record deviations and results in docs/ai/changes/2026-09-22-native-foundation.json. No background agent jobs are required to resume.
