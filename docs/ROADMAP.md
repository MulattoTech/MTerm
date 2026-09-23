# MTerm roadmap and delivery dashboard

**Assessed 2026-09-23T19:01:56.954182+00:00 · 17.78% weighted delivery maturity**

![Overall completion](roadmap/bars/overall.svg)

**472 scored entries · 21 complete · 3 blocked · 24 explicitly excluded historical/legend entries.**

This is a reviewed planning model, **not release readiness, code coverage, probability of completion, or the percent of every upstream app implemented**. Native features require native evidence; inherited Electron checkmarks do not confer credit.

Calendar ETA: **Unscheduled** until capacity, dependencies and a maintained schedule exist. Every feature lists an approximate **remaining engineer-hour range**, not elapsed AI runtime. Estimates are low-confidence scope assumptions, not promises.

Naive additive remaining effort: **8,944–22,663 engineer-hours**. Shared implementation/overlap may reduce it; risk/rework may increase it. Do not convert this into a delivery date using invented staffing.

[Every feature, percentage, evidence and effort](roadmap/FEATURES.md) · [Offline searchable dashboard](roadmap/index.html) · [Metric rules](roadmap/METRICS.md) · [Source projects](SOURCE_PROJECT_FEATURE_MATRIX.md)

## Status key

⬜ 0% not started · 🟪 10% planned · 🟦 40% implementation · 🟦 65% integrated · 🟨 85% validated with acceptance gaps · 🟩 100% complete · 🟥 blocked (keeps earned percentage).

## Areas

| Area | Progress | Complete / scope | Remaining effort* | ETA |
|---|---|---:|---:|---|
| [Meaning of “100%”](roadmap/FEATURES.md#area-00) | ![0%](roadmap/bars/area-00.svg) | 0 / 0 | 0–0 h | Unscheduled |
| [0. Establish a trustworthy new baseline](roadmap/FEATURES.md#area-01) | ![63.75%](roadmap/bars/area-01.svg) | 0 / 4 | 17.4–46.4 h | Unscheduled |
| [1. Desktop shell and application lifecycle](roadmap/FEATURES.md#area-02) | ![17.28%](roadmap/bars/area-02.svg) | 1 / 15 | 136.2–363.4 h | Unscheduled |
| [2. Workspace runtime](roadmap/FEATURES.md#area-03) | ![40.42%](roadmap/bars/area-03.svg) | 2 / 12 | 85.8–228.8 h | Unscheduled |
| [3. Infinite canvas — domain and interaction completeness](roadmap/FEATURES.md#area-04) | ![35.21%](roadmap/bars/area-04.svg) | 2 / 31 | 231–616.4 h | Unscheduled |
| [4. Project Mode completeness](roadmap/FEATURES.md#area-05) | ![28.5%](roadmap/bars/area-05.svg) | 1 / 20 | 171.6–457.6 h | Unscheduled |
| [5. Filesystem and editor surface](roadmap/FEATURES.md#area-06) | ![20.75%](roadmap/bars/area-06.svg) | 5 / 25 | 219.6–585.6 h | Unscheduled |
| [6. Terminal/runtime sessions](roadmap/FEATURES.md#area-07) | ![31.8%](roadmap/bars/area-07.svg) | 0 / 16 | 226–576 h | Unscheduled |
| [7. Git and worktree orchestration](roadmap/FEATURES.md#area-08) | ![9.29%](roadmap/bars/area-08.svg) | 0 / 25 | 244.2–652 h | Unscheduled |
| [8. Process and system runtime](roadmap/FEATURES.md#area-09) | ![35.18%](roadmap/bars/area-09.svg) | 0 / 18 | 135–360.2 h | Unscheduled |
| [9. Tasks, agency, evidence, and local memory](roadmap/FEATURES.md#area-10) | ![16.72%](roadmap/bars/area-10.svg) | 2 / 22 | 264–692 h | Unscheduled |
| [10. Agent provider abstraction and multi-agent runtime](roadmap/FEATURES.md#area-11) | ![23.19%](roadmap/bars/area-11.svg) | 0 / 30 | 884.8–2212 h | Unscheduled |
| [11. MCP service — complete tool surface](roadmap/FEATURES.md#area-12) | ![0%](roadmap/bars/area-12.svg) | 0 / 40 | 1280–3200 h | Unscheduled |
| [12. Permission/security system to full specification](roadmap/FEATURES.md#area-13) | ![22.96%](roadmap/bars/area-13.svg) | 0 / 27 | 665.6–1664 h | Unscheduled |
| [13. Secret management](roadmap/FEATURES.md#area-14) | ![1%](roadmap/bars/area-14.svg) | 0 / 10 | 316.8–792 h | Unscheduled |
| [14. Audit and observability](roadmap/FEATURES.md#area-15) | ![29.39%](roadmap/bars/area-15.svg) | 0 / 17 | 138.6–369.8 h | Unscheduled |
| [15. Remote MCP](roadmap/FEATURES.md#area-16) | ![2%](roadmap/bars/area-16.svg) | 0 / 15 | 470.4–1176 h | Unscheduled |
| [16. Browser node and browser automation](roadmap/FEATURES.md#area-17) | ![0%](roadmap/bars/area-17.svg) | 0 / 13 | 416–1040 h | Unscheduled |
| [17. Optional ChatGPT Web/browser harness](roadmap/FEATURES.md#area-18) | ![2.5%](roadmap/bars/area-18.svg) | 0 / 8 | 249.6–624 h | Unscheduled |
| [18. Optional computer-use / GUI automation](roadmap/FEATURES.md#area-19) | ![0%](roadmap/bars/area-19.svg) | 0 / 10 | 320–800 h | Unscheduled |
| [19. Package managers and Docker](roadmap/FEATURES.md#area-20) | ![0%](roadmap/bars/area-20.svg) | 0 / 10 | 320–800 h | Unscheduled |
| [20. Plugin architecture](roadmap/FEATURES.md#area-21) | ![7.58%](roadmap/bars/area-21.svg) | 0 / 15 | 488–1220 h | Unscheduled |
| [21. Session continuity and recovery](roadmap/FEATURES.md#area-22) | ![33.85%](roadmap/bars/area-22.svg) | 1 / 13 | 275.2–688 h | Unscheduled |
| [22. Persistence/data model](roadmap/FEATURES.md#area-23) | ![46.43%](roadmap/bars/area-23.svg) | 4 / 13 | 67.2–179.2 h | Unscheduled |
| [23. Performance and lean-runtime definition](roadmap/FEATURES.md#area-24) | ![28.48%](roadmap/bars/area-24.svg) | 0 / 12 | 151.2–395.2 h | Unscheduled |
| [24. Cross-platform runtime](roadmap/FEATURES.md#area-25) | ![26.11%](roadmap/bars/area-25.svg) | 0 / 9 | 212.8–532 h | Unscheduled |
| [25. Release engineering](roadmap/FEATURES.md#area-26) | ![20.29%](roadmap/bars/area-26.svg) | 0 / 16 | 446.4–1116 h | Unscheduled |
| [26. Test/security/reliability completion gate](roadmap/FEATURES.md#area-27) | ![41.67%](roadmap/bars/area-27.svg) | 0 / 15 | 280–700 h | Unscheduled |
| [27. DevFleet integration (proposed)](roadmap/FEATURES.md#area-28) | ![10%](roadmap/bars/area-28.svg) | 0 / 8 | 230.4–576 h | Unscheduled |
| [28. Roadmap governance](roadmap/FEATURES.md#area-29) | ![100%](roadmap/bars/area-29.svg) | 3 / 3 | 0–0 h | Unscheduled |

## Required maintenance

Every human/AI change must review affected IDs, stages, evidence and estimates, including explicit no-change reviews. Run tooling tests, refresh with a change ID/note/affected IDs, then `--check`. The native build and package gates reject stale source-review fingerprints and outputs. [Instructions](roadmap/METRICS.md).

*Effort is not ETA. Stage, scope and weight changes are versioned and must be disclosed.*
