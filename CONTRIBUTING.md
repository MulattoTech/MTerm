# Contributing to MTerm

Any AI provider or human may contribute using ordinary Git; no vendor account or chat transcript is required to understand this repo. Write access still requires repository authorization.

## Change records
Create docs/ai/changes/<UTC-date>-<task>.json before substantial edits. Include change_id, started_at_utc, provider, model, identity_basis, human_request, files_created, files_modified, decisions, tests, limitations, and next_steps. Use unknown rather than guessing an older model's identity. Update the record when finishing. Never record hidden reasoning or credentials.

A new source file begins with:
```cpp
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// See docs/ai/changes/2026-09-22-native-foundation.json for UTC dates and scope.
```
For later edits, preserve the origin header and add the new change ID only when it materially improves traceability. Exact per-line attribution belongs in Git history, which travels with this repository. Do not put a changing date on every function or falsely claim legacy code as newly authored.

## Commenting standard
Document public contracts with Doxygen-style comments: purpose, parameters/units, returned values, error behavior, ownership/lifetime, thread affinity and security preconditions. Explain decisions and surprising invariants inside implementations. Prefer clear identifiers over obvious commentary. Every TODO names a roadmap ID or GitHub issue and an acceptance test. Do not write speculative performance claims as facts.

## Commit example
```text
feat(core): add bounded process capture

AI-Provider: OpenAI
AI-Model: GPT-6 Astra Pro
AI-Change: 2026-09-22-native-foundation
Tests: ctest --preset native-release
```
Human authors may use AI-Provider: none. Keep the configured human Git identity; trailers disclose assistance. Never impersonate another developer or fabricate co-authorship.

## Engineering
C++20, RAII, explicit ownership, small services and dependency injection at OS/process boundaries. No raw owning pointers unless an OS/Qt interface requires them. No shell interpolation for structured commands. Prefer Qt's existing components to hand-built frameworks. Prove a performance bottleneck before introducing an alternative library. Build both Debug and Release for validation; never rely on assert() alone in release tests.

Review checklist: correctness, cancellation, bounds, concurrency, scope isolation, secret redaction, recovery, accessibility, licensing, upgrade compatibility and measured performance. Public release requires native parity and signing/compliance review; an experimental build is not a release certification.

## Mandatory delivery metrics after every change
Read docs/roadmap/METRICS.md. Every AI/human task must review affected REQ/DF/GOV IDs, update
docs/roadmap/progress.json with evidence-backed stage/estimates/blockers (or explicitly explain
no stage change), record roadmap_updates in its change record, regenerate with --refresh and
run `python -X utf8 scripts/roadmap/roadmap.py --check`. Commit generated views/review log too.
Never fake completion, infer it from inherited checkmarks, invent calendar ETAs, or alter baseline
weights just to raise the overall number. Default builds/packages reject stale source/catalog review.
