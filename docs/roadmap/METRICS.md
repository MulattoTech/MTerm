# Roadmap metrics: required for every AI and human change

## What the percentage measures

The number is **weighted delivery maturity of the explicitly indexed scope**. It is a planning
assessment, not code coverage, release readiness, safety certification or a measured fraction of
all possible upstream features. The broad inherited list includes overlapping work; do not infer
calendar duration from its sum. Source-stage assessments are reviewed judgments with evidence.

Every original ID/wording is retained. Four legend rows and twenty dated/reference-only observations
are explicitly excluded with zero denominator weight; they are visible, not deleted. Native
requirements remain scored independently. The eight proposed DevFleet requirements and three
roadmap-governance requirements are separately identified. Scope changes must be disclosed.

| Stage | Percent | Required evidence |
|---|---:|---|
| not_started | 0 | No implementation credit |
| planned | 10 | Concrete design/acceptance scope |
| implementation | 40 | Real relevant native code; important integration gaps |
| integrated | 65 | Connected usable workflow; validation/acceptance gaps remain |
| validated | 85 | Relevant tests run; remaining acceptance/platform/performance limits explicit |
| complete | 100 | The exact requirement's acceptance criteria passed, tests linked, no blocker |

A blocked flag turns status red **without erasing previously earned work**. An inherited `[x]`,
a visible button, a stub, an unrun test file, or exit code zero never establishes completion.
A single Windows feature can complete its Windows-scoped requirement without claiming Linux/macOS;
cross-platform requirements are separately scored. Do not enlarge/shrink scope to inflate a bar.

## Formula and estimates

`overall_percent = sum(baseline_weight * stage_percent) / sum(baseline_weight)` for included items.
Weights initially equal the midpoint of low-confidence engineering scope bands: S4–12h, M12–32h,
L32–80h, XL80–200h. These are **uncalibrated assumptions**, not historical velocity. Most ranges
were assigned from broad component complexity; refine them as concrete acceptance work is designed.
Keep baseline weights stable during a task. A changed weight/range needs an explicit review note.

Remaining range = baseline engineer-hour range × (1 − stage_percent/100). It is only a consistent
planning proxy, not a claim that 65% of calendar time is spent. Adding ranges naively can double-count
shared work; uncertainty/rework can also increase cost. There is no team-capacity assumption.

**Calendar ETA stays Unscheduled.** A real date requires an owner/capacity/dependency/schedule basis
in `eta_basis`, and it must be kept current. Never turn AI context windows, tokens, or these rough
hours into a promised delivery date. Blockers must name the actual missing dependency/authorization.

## Required workflow — no exceptions for AI vendor

1. Read `docs/ROADMAP.md`, the item's row in `FEATURES.md`, `progress.json`, source-project matrix,
   current HANDOFF and UI contract. Select exact REQ/DF/GOV IDs.
2. Record provider, actual model or unknown, UTC, scope and base revision under docs/ai/changes/.
3. Write failing acceptance tests, implement, run the relevant native/reference/tooling gates.
4. Review affected item stage, evidence, test references, remaining effort and blocker. Update
   `progress.json` honestly. If unchanged, record why; reviewed no-change work is still an update.
5. Add `roadmap_updates` with reviewed IDs and outcomes to the AI/human change record.
6. Refresh and verify using the actual IDs and review note (example below).
7. Commit catalog, generated Markdown/SVG/HTML/JSON, review snapshot/log, code, tests and provenance
   together. Never edit generated bars or summary files to manufacture a percentage.

```powershell
python -X utf8 scripts/roadmap/test_roadmap.py
python -X utf8 scripts/roadmap/roadmap.py --refresh --change-id 2026-09-23-workbench-roadmap --items REQ-D05E282B6459,REQ-64507D2E8F0A --review-note "Reviewed multi-buffer acceptance; tests passed; remaining limitations recorded."
python -X utf8 scripts/roadmap/roadmap.py --check
```

`--refresh` does not advance any stage automatically. It records the review, code fingerprints and
catalog fingerprint, then deterministically renders views. `--write` only renders; it does not satisfy
the review gate. `--check` rejects altered source/catalog without review, missing/duplicate IDs,
invalid math/estimates, missing evidence/test paths, stale generated views, and unsupported ETAs.
The existence of an evidence file is machine-checked; its correctness still requires human/AI review.
Self-reported provenance is not cryptographic proof, and intentional evasion is outside this gate.

## Enforcement and runtime boundary

The default native build and package scripts run the roadmap check; `npm run roadmap:check` is the
reference/tooling entry point. A native `-SkipRoadmapCheck` is allowed only during a local red/green
iteration, emits a warning, and is **not** a publish/release gate. Packaging never uses that shortcut.
The documented future CI recipe checks metrics once authorized workflow installation is available.
No global hooks, tokens, cloud services, background polling or product-runtime Python were added.
The native application does not parse the whole roadmap on startup.

## Viewing

GitHub renders `docs/ROADMAP.md` and `FEATURES.md` with repository-local SVG bars, no external badge
service. `docs/roadmap/index.html` is an offline searchable/filterable dashboard: download/open it
locally or serve it through an explicitly configured static site. GitHub does not execute raw HTML
as an app by itself. `summary.json` supports other tools and future native status surfaces.
