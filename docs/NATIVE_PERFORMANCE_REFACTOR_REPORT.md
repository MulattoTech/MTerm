# AstraCommander — Native Performance / C++ + Qt Refactor Report

Date: 2026-09-22
Repository: `C:\Users\Dylan\Documents\AstraCommander`

## Executive conclusion

If the product requirement is literally **“fast, lean, no bloat, and locally faster to become usable than the Codex CLI”**, Electron should now be treated as the **validated behavioral prototype and compatibility implementation**, not the final always-on shell.

The current Electron implementation is useful because it has already proven product behavior, policy boundaries, persistence, MCP, terminals, Git, processes, agents, evidence, and recovery. That work should not be discarded. Its tests, schemas, SQLite migration semantics, security behavior, and E2E flows should become the oracle for a native implementation.

For the final performance-oriented product, the recommended target is:

- **Qt Quick / QML for presentation and GPU scene graph**
- **C++ for state models, services, policy, persistence, filesystem, Git orchestration, process/system adapters, terminal supervision, event routing, and hot data paths**
- minimal QML JavaScript
- no embedded Chromium in the default process
- heavyweight browser/provider functionality launched **on demand** as separate processes
- no permanent Node.js runtime in the final default install if the strict footprint target remains non-negotiable

This is not a recommendation for a blind big-bang rewrite. Use a **strangler migration** with cross-implementation golden tests.

## What “faster than the CLI” must mean

The phrase needs measurable definitions.

A UI rewrite cannot make remote model inference itself faster. If a Codex or Claude request spends seconds in network/model reasoning, changing Electron to C++ does not alter that remote compute time.

AstraCommander can, however, beat a CLI on:
- application startup / first usable frame;
- local command-palette/input latency;
- workspace/file/Git/process navigation;
- local tool dispatch overhead;
- terminal input-to-paint;
- session switching and already-loaded context;
- avoiding repeated provider-process startup when a persistent provider transport is technically supported;
- retaining state so the user does not repeatedly reconstruct context.

Therefore compare **local control-plane overhead** separately from provider inference latency.

## Current measured baseline on MulattoTechBox

Artifacts:
- `artifacts/performance-baseline-20260922.json`
- `artifacts/codex-cli-startup-baseline-20260922.json`

### Current Electron AstraCommander

Five real Electron launches through Playwright, measured until the AstraCommander brand was visible:

| Metric | Current result |
| --- | ---: |
| median first visible | **325.21 ms** |
| sample range | **301.78–342.41 ms** |
| median process count | **5** |
| median total working set | **374.43 MiB** |
| median total private memory | **223.57 MiB** |

The current app is already visually quick, but the idle memory/process footprint is incompatible with a strict “CLI-class lean” objective.

### Current Codex CLI lower-bound

The installed Codex CLI is `codex-cli 0.157.0-alpha.2`.

Ten measurements of `node codex.js --version`:
- first sample: **259.27 ms**
- warm median: **69.265 ms**
- warm p90: **74.54 ms**

This is **not** a full interactive-ready benchmark; it is a small CLI startup lower-bound. It is deliberately used as a difficult local target, not as an apples-to-apples product benchmark.

### Current build footprint caveat

The development `apps/desktop/dist` directory measured about **203.5 MB**, but that is **not a valid release-package size**:
- builds intentionally preserve older hashed artifacts;
- source maps are enabled;
- multiple prior bundles coexist.

The installed Electron package itself measured about **386.6 MB decimal** in `node_modules/electron`, illustrating the underlying runtime weight.

The current active initial renderer entry is about **768 KB JS** plus **37 KB CSS**. Monaco is now dynamically imported, which is good, but the lazy editor chunk is still roughly **3.77 MB minified** and the TypeScript worker is roughly **7.0 MB**, with many additional language/worker chunks and large source maps.

## Proposed hard performance budgets

These should be validated on the same reference hardware and adjusted only through an explicit ADR.

### Startup
- **Stretch:** warm first visible frame <= **60 ms**
- **Hard target:** warm first visible frame <= **90 ms**
- cold first visible frame <= **180 ms**
- ready-to-command <= **100 ms warm**
- no provider/browser subprocess started during ordinary shell startup

The stretch target is intentionally aggressive because the measured warm Codex `--version` median is ~69 ms.

### Idle footprint
- one primary AstraCommander process in the ordinary no-browser/no-agent idle state
- **hard target <= 80 MiB working set**
- **stretch target <= 60 MiB working set**
- no always-on Chromium/Node child processes

### UI latency
- 60 Hz frame budget: p95 frame <= **16.7 ms**
- no GUI-thread blocking task > **4 ms** under normal interaction
- command palette/open-resource action p95 <= **8 ms**
- ordinary selection/pan/zoom input response within one frame

### Local runtime overhead
- in-process policy decision p95 <= **0.25 ms**
- cached metadata/state lookup p95 <= **1 ms**
- local service dispatch excluding OS/external-command work p95 <= **2 ms**
- terminal keystroke-to-render p95 <= **8 ms**
- audit append should not synchronously stall the GUI thread

### Scale targets
- 500 lightweight canvas nodes without frame collapse
- 100 visible ordinary nodes at >=60 FPS on reference hardware
- multiple live terminals and agents with inactive/off-screen live surfaces suspended
- audit/session histories in the tens of thousands without loading all records into UI memory

## Recommended final architecture

```text
AstraCommander.exe
├─ Qt Quick/QML presentation
│  ├─ native scene-graph canvas
│  ├─ command palette / project mode
│  ├─ lightweight task/audit/process models
│  └─ native terminal/editor surfaces
├─ C++ application core
│  ├─ workspace/domain model
│  ├─ event bus / state store
│  ├─ policy + risk engine
│  ├─ persistence + migrations
│  ├─ filesystem/index/search
│  ├─ Git/worktree service
│  ├─ process/system service
│  ├─ terminal supervisor
│  ├─ provider supervisor
│  └─ observability/performance counters
└─ on-demand workers only
   ├─ model provider CLIs where required
   ├─ browser/Playwright worker
   ├─ tunnel adapters
   ├─ Docker/package-manager commands
   └─ optional MCP compatibility worker during migration
```

The steady-state core should not need a browser engine.

## C++/Qt refactor matrix

### 1. Electron shell — MUST MIGRATE for the stated footprint goal

**Current:** Electron/Chromium main + renderer + preload.

**Native target:** `QGuiApplication` + `QQmlApplicationEngine` with C++ application services and minimal QML.

**Why:**
- removes Chromium/Node from the default shell;
- collapses the ordinary process tree;
- removes V8/DOM/React baseline memory;
- enables QML ahead-of-time compilation;
- permits direct native service calls instead of Electron IPC.

**Priority:** P0 native migration.

Do not spend large effort micro-optimizing Electron startup if Electron itself is not the intended final shell.

### 2. React + XYFlow canvas — MUST MIGRATE

**Current:** React/XYFlow with all flow nodes remapped through React state; domain model is fortunately renderer-independent.

**Native target:**
- Qt Quick scene graph;
- C++ canvas model;
- C++ spatial index;
- custom `QQuickItem` / scene-graph geometry for background, edges, selection, minimap, and large-scale primitives;
- instantiate/suspend complex node delegates based on viewport visibility.

**Why:**
- canvas is the primary always-on surface;
- direct retained/GPU scene graph is a better fit for hundreds of spatial resources;
- avoids React reconciliation and DOM component trees for large workspaces.

**Important:** preserve `packages/core/src/canvas.ts` JSON behavior through golden fixtures while porting.

### 3. Terminal: xterm.js + node-pty — MUST MIGRATE

**Current:** xterm.js renderer + `node-pty`.

**Windows native target:**
- direct ConPTY supervisor;
- native terminal screen/cell buffer;
- permissively licensed terminal escape parser selected after license audit;
- custom `QQuickItem` scene-graph renderer;
- bounded scrollback ring buffer.

**Linux/macOS:** native PTY adapters behind the same terminal service contract.

**Why:**
- terminal is latency-sensitive and frequently live;
- removes JS terminal rendering and native-addon bridge;
- makes restart supervision/process ownership easier;
- allows tight memory control.

Do not hand-roll a complete VT parser unless profiling/license review proves an existing small library unsuitable.

### 4. Monaco editor — MUST REPLACE for the strict lean target

**Current:** Monaco dynamically imports only when editor UI is opened, but brings a multi-megabyte editor engine, large TypeScript worker, language workers, language definitions, and fonts.

**Native target:**
- lightweight native editor/text component;
- incremental document model;
- syntax highlighter loaded only for the current language;
- optional external language server only when requested;
- diff/text models shared with Git service.

Run a dedicated editor spike before choosing a third-party native editor engine. Perform a license audit before adopting any editor component.

**Why:** Monaco is excellent functionality but fundamentally designed for a browser/worker environment and is one of the largest payloads in the current renderer.

### 5. `apps/desktop/src/main.ts` service layer — MUST DECOMPOSE AND PORT

The current ~1000-line Electron main process contains filesystem, task/evidence, providers, terminal, Git, process, workspace, IPC, and lifecycle responsibilities.

Split native services:
- `WorkspaceService`
- `FileService`
- `PolicyService`
- `PersistenceService`
- `AuditService`
- `TerminalService`
- `GitService`
- `ProcessService`
- `AgentSupervisor`
- `McpService`
- `PerformanceService`

Use explicit C++ interfaces and event signals. GUI objects must not own business logic.

### 6. Security/policy/path handling — PORT TO C++ CORE, preserve behavior exactly

**Current:** TypeScript safe-path logic and policy evaluator have good regression coverage.

**Native target:**
- immutable/validated domain requests;
- platform-native canonical path verification;
- Windows handle/reparse-point checks where necessary;
- policy engine independent from QML;
- cached canonical workspace roots;
- structured command risk model.

**Why:** not primarily because TypeScript is slow; because the security boundary must live in the final privileged native core after Node/Electron is removed.

Port using golden tests before changing policy semantics.

### 7. SQLite persistence — PORT AND MOVE OFF GUI THREAD

**Current:** Node `DatabaseSync`, shared `AstraStore`, migrations, WAL, integrity checks.

**Native target:**
- SQLite C API or Qt SQL QSQLITE;
- one database actor/thread or bounded worker;
- prepared statements;
- batched audit writes;
- typed/indexed queries;
- explicit transactions for linked state;
- paginated results.

**Why:**
- current sync DB calls occur in the Electron main process;
- JSON record scans do not scale;
- native implementation eliminates Node dependency and provides predictable threading.

Preserve the existing database schema and migration compatibility first; evolve it with migrations only.

### 8. Process telemetry — HIGH-ROI NATIVE PORT

**Current Windows path:** starts PowerShell and runs `Get-Process` for each snapshot.

**Windows native target:**
- Toolhelp/process enumeration APIs;
- `GetProcessMemoryInfo`;
- `GetProcessTimes` sampled for CPU rate;
- `QueryFullProcessImageName`;
- native parent/child ownership tracking.

**Why:** spawning PowerShell for telemetry is high overhead and unnecessary. This is one of the clearest areas where C++ directly improves latency and allocation.

Cross-platform implementations should use native OS adapters behind one service interface.

### 9. Git service — PORT SUPERVISION FIRST; use libgit2 only if profiling justifies it

**Current:** safe `git` subprocesses for status/diff/log/stage/unstage/worktree.

**Phase 1 native target:** `QProcess` spawning the system Git binary with bounded output, explicit pathspecs, and the same policy rules.

**Phase 2 option:** use libgit2 for high-frequency status/diff/history operations if process-spawn benchmarks prove worthwhile.

**Why not immediately replace all Git with libgit2:** command-line Git has excellent behavior compatibility; correctness and repository semantics matter more than shaving milliseconds from infrequent mutations.

### 10. Agent providers — KEEP PROVIDER CLIs EXTERNAL, PORT THE SUPERVISOR

**Current:** Node spawns Codex/Claude per run and parses stream JSON.

**Native target:**
- `QProcess` provider supervisor;
- background JSON/JSONL parsing;
- bounded event ring buffers;
- C++ session state machine;
- provider manifest/capabilities unchanged semantically.

**Do not rewrite Codex/Claude/Gemini themselves.**

For lower latency:
- cache provider availability/version/auth status instead of spawning version/auth commands on UI refresh;
- investigate provider-supported persistent transport/server modes;
- use a persistent provider broker only where the provider officially supports it;
- direct API/native network adapters can avoid CLI startup for providers where the user explicitly configures them.

### 11. MCP — DO NOT RUSH INTO AN UNOFFICIAL C++ SDK

The current official MCP support matrix does not list C++ as an official SDK.

Migration strategy:
1. keep the tested TypeScript MCP server during the first native-shell migration;
2. define protocol-independent C++ tool/service interfaces;
3. add protocol conformance fixtures;
4. for the final no-Node build, choose between:
   - a small clean C++ JSON-RPC/MCP implementation maintained against conformance tests; or
   - an on-demand native sidecar using an officially supported SDK language such as Rust/Go;
5. remove the TS sidecar from the default install only after protocol parity is proven.

Do not make an unaudited third-party C++ SDK a single point of protocol correctness.

### 12. Browser runtime — KEEP OUT OF THE BASE PROCESS

Do **not** replace Electron with Qt and then link Qt WebEngine into every launch. Qt WebEngine is Chromium-based and multi-process, recreating much of the footprint the migration is intended to remove.

Recommended:
- browser automation = separate Playwright/Chromium worker launched only when requested;
- simple embedded web content = investigate OS-native Qt WebView where its platform limitations are acceptable;
- browser process terminates/suspends when no browser node needs it.

### 13. Package managers / Docker / tunnel providers — KEEP EXTERNAL

These are naturally command/service adapters:
- use `QProcess` / native child supervision;
- parse structured output where available;
- do not reimplement npm, Docker, Tailscale, Cloudflare, etc.;
- launch only when used.

### 14. Audit/log/event buffers — PORT AND BOUND

**Current:** JSON records plus renderer state refreshes.

**Native target:**
- typed event structs;
- producer/consumer queues;
- batch SQLite commits;
- bounded in-memory ring buffers;
- paginated UI models;
- no full-history reload per state refresh.

### 15. File index/search — STRONGLY RECOMMENDED NATIVE

Use:
- native directory enumeration;
- OS file watchers;
- background index worker;
- cancellation;
- bounded result streams;
- path-policy check before every privileged operation.

This will matter more than micro-optimizing single file reads.

## What should NOT be rewritten merely for speed

- provider model logic: Codex/Claude/Gemini remain external/provider-owned;
- Git mutation semantics do not need to be reimplemented if `git` subprocess performance is adequate;
- Docker/package-manager behavior should remain external;
- browser automation should remain an on-demand isolated worker;
- validated schemas/security behavior should be **ported**, not redesigned during the performance migration;
- source/license research and tests remain valuable regardless of implementation language.

## Immediate optimizations before the native shell exists

These reduce current pain and also clarify the migration contract:

1. **Create a real clean release-staging build.**
   - clean a separate staging directory;
   - no source maps in shipped package;
   - never mix additive continuation artifacts with release payload.

2. **Trim Monaco.**
   - import only editor APIs/features actually needed;
   - avoid shipping unused languages/workers;
   - load TypeScript/language-service workers only for matching files.

3. **Cache provider discovery.**
   - Codex/Claude version/auth checks should have a TTL or invalidate on user action;
   - do not spawn provider CLIs on every panel refresh.

4. **Paginate state.**
   - stop returning audit/history collections inside broad application state refreshes;
   - query only the visible page.

5. **Delta events instead of global refresh.**
   - emit typed task/evidence/Git/process updates;
   - avoid remapping every ReactFlow node after unrelated operations.

6. **Suspend live spatial surfaces.**
   - only one/few active terminal/editor/agent live components at a time;
   - off-screen or collapsed nodes become lightweight representations.

7. **Replace PowerShell process inventory early.**
   - a tiny native helper can prove the process-service architecture and remove one obvious subprocess bottleneck before the full Qt shell lands.

8. **Benchmark Git command aggregation.**
   - reduce redundant Git process launches per refresh;
   - retain path-safe explicit commands.

## Strangler migration plan

### Phase N0 — freeze behavior and performance contracts

- Refresh stale architecture/handoff/status docs from the live source.
- Re-run current full E2E/build/MCP gate.
- Keep the Electron build as the reference implementation.
- Save golden JSON fixtures for Canvas, Task, Evidence, AgentSession, permissions, audit, migrations, and Git results.
- Adopt performance budgets and make regression scripts repeatable.

### Phase N1 — create `libastra-core`

C++20 + CMake:
- domain types;
- event bus;
- policy/security;
- SQLite persistence;
- filesystem;
- audit;
- performance counters.

Add a tiny CLI/test host so TypeScript tests can compare native and existing behavior.

Do not change the GUI yet.

### Phase N2 — native platform services

Port:
- process/system telemetry;
- terminal supervisor;
- Git supervisor;
- provider supervisor.

Current Electron can temporarily communicate with an `astra-core-host` process for parity testing. This transitional process is removed from the final steady-state architecture.

### Phase N3 — Qt Quick shell

Build:
- main window;
- command palette;
- workspace switcher;
- task/audit/process project views;
- C++ models exposed to QML;
- QML compiled ahead of time.

Target the <=90 ms warm shell budget before adding expensive surfaces.

### Phase N4 — native spatial canvas

Implement:
- custom C++ canvas model/spatial index;
- Qt Quick/QSG edges/background/minimap/selection;
- visible-node virtualization;
- lightweight node delegates.

Prove 500 lightweight nodes / 100 visible nodes at target frame budget.

### Phase N5 — native terminal and editor

Terminal first because it is more latency-sensitive:
- ConPTY/PTY;
- parser;
- GPU cell renderer;
- bounded scrollback.

Then native editor:
- incremental document model;
- syntax-only baseline;
- optional language service on demand.

### Phase N6 — provider/MCP parity

- QProcess Codex/Claude adapters;
- session resume;
- evidence and audit parity;
- native or lean official-SDK MCP strategy;
- remove Node from default install when parity gates pass.

### Phase N7 — remove Electron from release

Only after:
- every mandatory Electron E2E behavior has a native equivalent test;
- data migrations are compatible;
- startup/memory targets pass;
- recovery/security tests pass.

Keep the old Electron implementation in history/reference, not shipped.

### Phase N8 — advanced features as isolated workers

Add browser, remote tunnel, Docker/package tools, and optional computer use without contaminating base-process startup or idle memory.

## Native data/UI design rules

- QML is a **view language**, not the core business-logic language.
- Keep models and transformations in C++.
- Keep QML JavaScript short and non-blocking.
- Batch model changes.
- Worker-thread I/O/database/indexing.
- GUI/render thread handles only presentation-critical work.
- Never spin nested event loops.
- No polling when event-driven notification exists.
- Use bounded buffers everywhere long-running output can accumulate.
- Do not instantiate expensive live nodes when off-screen.
- Precompile QML/AOT for release.

## Qt module policy for a lean build

Default allowlist should stay narrow:
- Qt Core
- Qt GUI
- Qt Quick
- Qt QML
- selected Qt Quick Controls
- Qt Network only when needed
- Qt SQL or direct SQLite

Avoid linking optional modules simply because they are convenient.

**Qt WebEngine is prohibited from the default always-on core** under the no-bloat target.

## Qt licensing checkpoint

Qt is dual-licensed. The open-source distribution path is primarily LGPLv3/GPL, and some modules have different availability/obligations.

Before native implementation begins:
- decide community LGPL/GPL compliance versus commercial Qt;
- keep a module-by-module license inventory;
- prefer dynamic linking if using LGPL and ensure all redistribution/relinking/source obligations are met;
- do not assume AstraCommander's current MIT license cancels Qt's separate obligations;
- obtain legal review if distribution/commercial plans make compliance uncertain.

This report is engineering guidance, not legal advice.

## MCP licensing/protocol checkpoint

The current official MCP SDK matrix includes TypeScript, Python, Go, Rust, Java, Kotlin, C#, PHP, Ruby, and Swift; C++ is not currently listed as an official SDK.

Therefore:
- do not delete the tested TypeScript MCP path early;
- make tool implementations protocol-independent;
- create cross-language conformance fixtures;
- require full wire/conformance tests before replacing the SDK.

## Performance verification plan

Every native milestone records:
- cold/warm first visible;
- ready-to-command;
- idle working/private memory;
- process count;
- p50/p95/p99 GUI frame time;
- command-palette latency;
- canvas pan/zoom frame time;
- terminal input-to-paint;
- DB/audit append latency;
- file listing/search throughput;
- process enumeration latency;
- Git status/diff latency;
- provider spawn/first event;
- tool dispatch overhead.

Store machine fingerprint with results so numbers are comparable.

## Decision recommendation

**Adopt a native-performance migration now, before the browser/plugin/remote feature expansion.**

Continue implementing portable backend behavior only when it has an interface that can be reused by the C++ core. Avoid adding more Electron-specific UI architecture that will immediately be discarded.

The Electron implementation has served its purpose extremely well: it proved the product. The next architectural goal is to preserve its behavior while removing Chromium/Node from the default control plane.

## Success criterion for the native migration

The native shell is not “done” because it looks similar.

It is done when:
- it opens and becomes command-ready within the agreed startup budget;
- it meets the idle-memory/process-count budget;
- current workspace/task/evidence/session data opens without conversion loss;
- policy/security behavior matches golden tests;
- real PTY, file, Git, process, provider, and MCP operations pass;
- crash/restart state is honest;
- the native E2E suite covers the old Electron vertical-slice behaviors;
- advanced heavy workers remain absent until explicitly invoked;
- the old Electron runtime is no longer required in the default release.

## Current routine validation at report completion

After reconciling the newer live source, the current routine gates were rerun:

- strict TypeScript typecheck — PASS
- ESLint — PASS
- Vitest — **59/59 PASS**
- production build — PASS
- MCP integration validator — PASS
- Electron E2E — **2/2 PASS**, including the Git mutation/worktree scenario

Evidence: `artifacts/report-baseline-gate-20260922.log`.

This does not turn the Electron build into the final recommended runtime; it establishes a known-good behavioral reference for the native migration.
