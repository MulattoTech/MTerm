# MTerm testing

## Native Windows gate

```powershell
.\scripts\native\Build.ps1 -Configuration Release
.\scripts\native\Build.ps1 -Configuration Debug
python scripts/native/verify-provenance.py
```

Each configuration runs five CTest executables. QtTest outcomes include lifecycle/data rows;
current per-build counts are core 40, runtime 15, backend 22, desktop 7, terminal 11.
Reports are `out/native-<configuration>/native/*-tests.txt` and CTest temporary logs.
No real model is called. The compiled mterm-test-child emits controlled protocol fixtures.

Coverage includes canonical paths/hash conflicts/UTF-8, schema upgrades/newer-version refusal,
identity guards, scoped requests, bounded processes/cancel/timeout, malformed final JSONL,
provider fresh/resume outcomes, persistence, native editor, real Windows PTY and GUI keystrokes.
Eight malformed mutation cases were recorded failing before their fixes.

## Standalone preview validation

```powershell
.\scripts\native\Package-Windows.ps1
python scripts/native/smoke_package.py release-local/<created-folder> --runs 5
```

The smoke verifies every manifest hash, removes development Qt/compiler folders from PATH,
uses new temporary workspace/data profiles, captures screenshots and records internal readiness/
paint/memory metrics. It neither launches models nor deletes evidence. It is NOT a pristine-machine,
soak, signing or external cold-launch benchmark. Original dependency-less launch failed as expected;
the deployed package succeeded. See docs/validation/2026-09-22-native-resume.md.

## Preserved Electron gate

```powershell
npm run typecheck
npm run lint
npm test
npm run build
npm run test:mcp
npm run test:e2e -- --repeat-each=2
```

The build precedes integration. Current 59 unit tests and 4 repeated E2E executions pass.
The tests validate an actual loaded editor document, not only an initialization status label.
Reference E2E remains Windows/machine-aware; do not represent it as universal hosted coverage.

## CI and remaining coverage

The pinned Windows/Linux Release/Debug workflow is documented at docs/ci/native.workflow.yml;
activation is blocked by workflow-write permission. No hosted pass is claimed. Unix PTY skips
are explicit, not native Unix parity. macOS, sanitizers/fuzzing, signed installers, real provider
conformance, long-running burst/backpressure, hostile file races and full crash recovery remain.
