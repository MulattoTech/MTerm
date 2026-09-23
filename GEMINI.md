# MTerm contributor entry point

AGENTS.md is the shared policy for all tools and model providers. Read it, docs/HANDOFF.md,
docs/ROADMAP.md, docs/UI_DESIGN_CONTRACT.md and docs/roadmap/METRICS.md before editing.
Keep the native UI seamless and reference-compatible without embedding a browser engine.
Preserve permissions, bounded services, original source-project attribution and AI change records.

## Mandatory delivery metrics after every change
Read docs/roadmap/METRICS.md. Every AI/human task must review affected REQ/DF/GOV IDs, update
docs/roadmap/progress.json with evidence-backed stage/estimates/blockers (or explicitly explain
no stage change), record roadmap_updates in its change record, regenerate with --refresh and
run `python -X utf8 scripts/roadmap/roadmap.py --check`. Commit generated views/review log too.
Never fake completion, infer it from inherited checkmarks, invent calendar ETAs, or alter baseline
weights just to raise the overall number. Default builds/packages reject stale source/catalog review.
