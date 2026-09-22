# Native UX parity and relocation implementation plan

Goal: restore the user-approved Electron interaction model in the native app, not invent a replacement product.
Spec: user request of 2026-09-22 plus apps/desktop/src/renderer/main.tsx and styles.css.
Active root: C:\Tools\Dev\MTerm. Prior checkouts remain recovery snapshots.
Architecture: preserve native Backend/security/terminal services; replace presentation and navigation only,
using reusable native Theme, WorkspaceShell, ResourceCard, ProjectView and CommandPalette components.
No embedded browser, framework switch, new model calls, fake DevFleet connection or third-party artwork.

## Tasks and acceptance
1. Relocate committed source/history/toolchain, recreate path-sensitive dev environments, verify tracked hashes and native baseline.
2. Add failing Qt UI tests: sidebar+canvas+inspector geometry, searchable keyboard command palette,
   Canvas/Project switching, editor preservation across tools, real file selection, bounded scene item count.
3. Theme and shell: top workspace bar, left Create/Inspect navigation, shared Canvas/Project surface,
   resizable right inspector, visible safety state, reusable design tokens and original vector icons.
4. Canvas: one painted item per resource, kind/status/header/footer, click-to-inspect, pin/collapse,
   pan/zoom/fit/minimap, Project cards from the same nodes; no per-node browser/widget trees.
5. Preserve and route native file/task/terminal/Git/agent/process/audit workflows. Wire buttons to actual
   services; unimplemented native parity remains explicitly listed. Do not hide regressions in tests.
6. Compare screenshots at desktop and compact widths; native Release/Debug, reference gates, deployment
   smoke and scale/memory samples. Performance numbers are measurements, never a promise of no slowdowns.
7. Publish source-project feature/attribution matrix, full backlog pointer, future DevFleet contract,
   current handoff and a reusable feature/UI review template. No missing requirements silently dropped.

## Review focus
Scope/permissions unchanged; terminal survives inspector switching; no editor text lost on view change;
no body text spills from cards; keyboard/focus behavior; responsive sizing; hidden panels do not spawn
providers; data is persisted honestly; inherited licensing and provenance preserved.

## Rulings
The user explicitly selected the previous Electron UI/UX as the design reference. This is a parity
restoration of already defined flows, not a new product architecture proposal. Qt Widgets remains;
Qt Quick/custom GPU work requires measured justification. Template Creator was searched but no exact
connected plugin was returned; repository-local reusable templates are the available implementation.
