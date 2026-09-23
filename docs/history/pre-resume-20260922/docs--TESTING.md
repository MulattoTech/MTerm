# Testing

## Quality-gate order

```powershell
npm run typecheck
npm run lint
npm test
npm run build
npm run test:mcp
npm run test:e2e
```

`npm run build` must precede the integration tests because MCP and Electron E2E intentionally execute the built artifacts.

## Unit tests

Vitest currently runs 54 tests across security, canvas and agency modules. Coverage targets path canonicalization, Windows path edge cases, junction/hardlink behavior, policy/grant scoping, redaction, command classification, canvas validation, agency/task/evidence schemas, explicit session transitions, restart reconciliation, duplicate/dangling graph references, size bounds and handoff text.

Do not weaken a security assertion to make a gate pass. Add regression cases when a path/policy bug is found.

## MCP integration

`npm run test:mcp` launches the built stdio server with the official MCP v2 client. It verifies:

- tool discovery;
- Observe filesystem read;
- Observe write denial;
- Observe terminal denial;
- Developer write/readback;
- explicit terminal opt-in and real process execution;
- destructive-command rejection.

The validator isolates its audit database under `artifacts/`.

## Electron E2E

Playwright launches real Electron with an isolated user-data directory. The current scenario verifies the command palette, Observe-by-default and local Developer elevation, persistent note/task state, optimistic stale-write rejection, file-browser/Monaco loading, read-only Git inspection, terminal approval and real PowerShell PTY I/O, an external MCP invocation becoming visible in desktop Audit, screenshot generation, application close/relaunch, and canvas/task recovery.

## Real Codex provider smoke

`node scripts/validate-agent.mjs` is a deliberate one-time/manual validation path, not a routine gate. It launches real Electron, grants `agent.execute`, runs the installed Codex CLI in read-only sandbox mode, waits for the real model response, and verifies a persisted DONE session plus PASS evidence linked to its task. Running it consumes provider/model usage.

## Artifacts

Useful local evidence is written under `artifacts/`, including build logs, Playwright traces on failure, the E2E profile and `e2e-first-slice.png`. These are development evidence, not release artifacts.

## Known test gaps

Add tests before claiming completion for additional provider adapters, terminal supervision across restart, multi-workspace behavior, Git mutations/worktree lifecycle, dedicated Diff/Process nodes, remote MCP auth, browser automation, plugin loading, secret storage, database migrations, update/packaging and large-repository performance.

The renderer build currently warns about large Monaco chunks. It is not a failing gate, but should become a measured performance budget once editor lazy loading lands.
