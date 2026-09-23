# Feature / native UX parity record

Feature ID / REQ-ID:
GitHub issue:
AI change record (provider, actual model or unknown, UTC, scope):
Source-project lineage and snapshot path:
License/reuse decision (idea vs code/assets, original attribution retained):

## User task and reference behavior

What is the user trying to do? Which exact existing Electron/source-supported interaction defines
success? Include before screenshot/path, keyboard route, initial state and expected result.
Separate existing reference behavior from a new proposal. Do not call a static mock “implemented.”

## Native contract

Entry points (sidebar, card, palette, shortcut):
State owner / stable resource IDs:
Background service / thread affinity:
Permissions / confirmation / cancellation:
Persistence / restart / view-switch behavior:
Loading / empty / error / offline / denied states:
Accessibility, keyboard, focus, IME, high-DPI, compact-window requirements:

## Design tokens and component reuse

Reuse native/desktop/ui/Theme and shared ResourceCard/WorkspaceShell components.
No hard-coded per-feature design system, duplicated session state or embedded browser by default.
Select expensive surfaces lazily, bound logs, and avoid one heavyweight widget tree per canvas card.

## Acceptance tests

- [ ] Test exact reference task, not just presence of a button/data type.
- [ ] Observe intended failing test before implementation.
- [ ] Permission denial does not execute or lose data.
- [ ] Switching view/inspector preserves the same live resource and editor content.
- [ ] Concurrent/late responses cannot update the wrong workspace or overwrite newer input.
- [ ] Real rendered screenshots checked at desktop and compact sizes.
- [ ] Empty/error/offline states are honest and actionable.
- [ ] New output/state remains bounded; performance is measured on a described workload.
- [ ] Existing native and reference regressions remain green.
- [ ] Source/license attribution, STATUS, HANDOFF, backlog and AI records updated.

## Evidence

Commands/platform/build/commit:
Actual results, failures and intentional skips:
Screenshots (test fixture explicitly labeled; no private paths/credentials):
Timing/memory methodology and samples (no unlike GUI/CLI comparisons):
Known gaps and first unvalidated next step:

## DevFleet compatibility

Applicable DF IDs from DEVFLEET_INTERFACE_VISION.md:
Actual integration status (none/fixture/read-only/live):
Protocol/schema version and authorization source:
Do not infer a live integration from a proposed adapter or UI label.

## Mandatory delivery metrics after every change
Read docs/roadmap/METRICS.md. Every AI/human task must review affected REQ/DF/GOV IDs, update
docs/roadmap/progress.json with evidence-backed stage/estimates/blockers (or explicitly explain
no stage change), record roadmap_updates in its change record, regenerate with --refresh and
run `python -X utf8 scripts/roadmap/roadmap.py --check`. Commit generated views/review log too.
Never fake completion, infer it from inherited checkmarks, invent calendar ETAs, or alter baseline
weights just to raise the overall number. Default builds/packages reject stale source/catalog review.
