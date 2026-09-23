# MTerm status — native UX restoration

Active root: `C:\Tools\Dev\MTerm`. Branch: `feat/native-ux-20260922`.
This is an implemented native redesign plus partial feature migration, not 100% parity.

| Area | Current native behavior | Limits |
|---|---|---|
| Shell | Sidebar, header, center workspace, right inspector, splitter | Desktop/compact layouts checked, accessibility audit pending |
| Canvas | One painted item/resource, card actions, pan/zoom/fit/minimap, Arrange, pin/collapse, saved viewport | Full groups/edge editing/resize/undo not implemented |
| Project view | Same resource IDs/data and shared card rendering | Full traditional project/session workflow remains to port |
| Command palette | Search, keyboard navigation, Enter/Escape, named action routing | No arbitrary command execution in palette |
| Notes/layout | Editable note details and debounced revision-checked persistence | Rich Markdown and full conflict-resolution UI remain |
| Inspector lifetime | Lazy construction, then retained instances | Single existing terminal resource; independent multi-session graph not yet |
| Files/editor | Native browser/load/new/save, dirty/version safety, state survives tool switches | One buffer, UTF-8 <=1 MB, no full native LSP |
| Terminal | Real ConPTY/libvterm; approval; survives inspector and Canvas/Project switches | Full scrollback/IME/selection, Unix PTY, GUI-restart supervision remain |
| Core/services | Existing scoped policy/SQLite/files/jobs/native inventory preserved | Earlier race/recovery/security issues still open |
| Git/agents/MCP | Read-only native Git and fixture-tested Codex supervisor; reference MCP retained | Not full native Git/provider/MCP parity; no paid model test this pass |
| Full scope | 485 inherited checkbox entries indexed with stable IDs; six source-project matrix | Counts are not completion percentage |
| DevFleet | Proposed integration/experience contract DF-01..08 | Not connected or modified |
| Template | Native design components and vendor-neutral feature/UI review template | No exact connected Template Creator plugin was returned by discovery |

## Validation

Native Windows Release/Debug: six CTest suites including original service, backend, terminal and
GUI tests plus UI restoration tests. Reference typecheck/lint/59 unit/build/MCP/2 E2E passed after
relocation. Actual Windows visual fixtures—not fontless headless images—were inspected at
1500x940 and 1100x780 plus Project and command-palette views.

Five packaged startup samples after lazy loading: median internal first paint 87 ms, workspace
ready 102 ms, working set 98,336,768 bytes (~93.8 MiB), private memory 40,218,624 bytes (~38.4 MiB).
Payload 32,533,573 bytes excluding manifest. The new presentation uses more memory than the bare
preview; further real-workload profiling remains necessary. No promise that arbitrary workloads
never slow down, and no unlike GUI/CLI comparison, is made.

Initial redesigned startup was 269 ms median; phase timing showed eager hidden inspectors were
most of UI construction. They now initialize only when selected; a regression test protects that.
Details/commands/screenshots: docs/validation/2026-09-22-native-ux.md.
