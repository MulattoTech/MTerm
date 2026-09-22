# MTerm native UI/UX contract

The user explicitly rejected the original full-window Qt tab strip as an unacceptable regression.
The existing Electron app is the interaction/layout reference, not a disposable prototype whose UX
may be dropped. The native build must be fast **and** preserve the familiar mental model.

## Stable shell

A top workspace bar, left Create/Inspect navigation, persistent spatial canvas, resizable right
inspector and Canvas/Project switch. Tools materialize once on first use and remain mounted while navigation changes; a shell or
editor cannot be recreated just to switch views. Permission state stays visible. Keyboard-first
command search routes named actions and never evaluates arbitrary text.

## Native components

Theme owns palette/typography/semantic roles; original vector glyphs use QPainter and system fonts.
WorkspaceShell owns navigation/chrome only. ResourceCard paints one scene item per resource and is
reused by ProjectView's delegate. CanvasView owns spatial presentation/viewport, not jobs. Backend
owns privileged workspace operations on its worker. Terminal transport/parser remain native.

## Behavioral requirements

Cards are readable at normal zoom; Fit and Arrange serve different purposes. Viewport overlays stay
fixed when the scene pans. Notes/layout changes persist with visible saved/error states. File views
preserve content across navigation and protect dirty/stale writes. Terminal input survives switching
inspectors and Canvas/Project. Observe shows disabled execution, not a clickable trap that fails later.

## Deliberate remaining gaps

This pass restores the shell and implemented workflow routes, not full feature parity. Independent
per-node live sessions, full canvas edge/group/resize/undo tooling, rich task/evidence editing, native
Git mutations/worktrees, full provider/context/history controls, native MCP, advanced terminal IME/
scrollback and actual DevFleet integration remain tracked work. Do not present them as done.

Use templates/FEATURE_UI_PARITY_TEMPLATE.md for every further feature, independent of AI vendor.
Template Creator discovery did not expose an exact connected integration; this reusable repository
artifact and the native theme/components are the implemented template package, not an installed app.
