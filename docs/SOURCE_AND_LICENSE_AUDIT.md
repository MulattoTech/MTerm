# Source and license audit

Audited 2026-09-22 using live GitHub repository/license/README APIs. Application license: MIT. All application implementation is original; no source code or artwork from these reference apps has been copied or adapted. Inspection was README architecture/capability descriptions plus LICENSE, not a complete source-code review. Local research snapshots are reference material, not application modules.

| Project | Repository | License | Components inspected | Ideas reused | Code copied/adapted | Attribution requirement | Compatibility notes |
|---|---|---|---|---|---|---|---|
| wonderwhy-er/DesktopCommanderMCP | https://github.com/wonderwhy-er/DesktopCommanderMCP | MIT | README and LICENSE | Stable filesystem/terminal services exposed as tools | None | Preserve copyright/license if code is reused | No source imported; upstream notices still govern research snapshots |
| rebel0789/codexpro | https://github.com/rebel0789/codexpro | MIT | README and LICENSE | Explicit workspace roots and bounded access | None | Preserve copyright/license if code is reused | No source imported; upstream notices still govern research snapshots |
| eneskirca/nodeterm | https://github.com/eneskirca/nodeterm | BUSL-1.1 (license text verified; API NOASSERTION) | README and LICENSE | Independent spatial terminal/editor organization | None | Do not copy without separate license strategy | NOT permissive open source; no implementation or assets reused |
| miuuyy/codex-chatgpt-web | https://github.com/miuuyy/codex-chatgpt-web | MIT | README and LICENSE | Provider isolation and streaming capability boundary; no web automation copied | None | Preserve copyright/license if code is reused | No source imported; upstream notices still govern research snapshots |
| Kyne0328/rel-ai-chatgpt-web-harness | https://github.com/Kyne0328/rel-ai-chatgpt-web-harness | Apache-2.0 | README and LICENSE | Durable task/session/evidence separation | None | Preserve license/NOTICE and mark modifications if reused | No source imported; upstream notices still govern research snapshots |
| lulu-sk/CodexFlow | https://github.com/lulu-sk/CodexFlow | Apache-2.0 | README and LICENSE | Two views over shared project/session state | None | Preserve license/NOTICE and mark modifications if reused | No source imported; upstream notices still govern research snapshots |

## Repository revisions observed

- `wonderwhy-er/DesktopCommanderMCP`: `da36b38329fa41e993a59c979e1935648f542f1e`; license https://github.com/wonderwhy-er/DesktopCommanderMCP/blob/main/LICENSE.
- `rebel0789/codexpro`: `482d0035e0c08cceb5916958df652b325b4c52d8`; license https://github.com/rebel0789/codexpro/blob/main/LICENSE.
- `eneskirca/nodeterm`: `3f695ab28a56ae8b3fefc9775fc69b8b71587b6b`; license https://github.com/eneskirca/nodeterm/blob/main/LICENSE.
- `miuuyy/codex-chatgpt-web`: `eaf4f09ae92d4dc4429fa597b0861663138f08f8`; license https://github.com/miuuyy/codex-chatgpt-web/blob/main/LICENSE.
- `Kyne0328/rel-ai-chatgpt-web-harness`: `ff29b2645236ce7a979bdcbe49d001ed4331db65`; license https://github.com/Kyne0328/rel-ai-chatgpt-web-harness/blob/main/LICENSE.
- `lulu-sk/CodexFlow`: `25879764d2ab97eecdbeb875ea93791defa9d60a`; license https://github.com/lulu-sk/CodexFlow/blob/master/LICENSE.

## Protocol and platform references

- https://modelcontextprotocol.io/specification/2026-07-28 â€” latest spec endpoint resolved here. Standard bindings: stdio and Streamable HTTP; older connection-handshake examples are not assumed current.
- https://github.com/modelcontextprotocol/typescript-sdk â€” stable v2 split packages; use installed package declarations as executable interface truth.
- https://modelcontextprotocol.io/specification/2026-07-28/basic/authorization â€” HTTP authorization; no HTTP endpoint is shipped in this slice.
- https://help.openai.com/en/articles/12584461-developer-mode-and-mcp-apps-in-chatgpt â€” remote MCP apps/developer mode, availability and permissions depend on plan/workspace. No promise that a local stdio process connects directly to ChatGPT.
- https://www.electronjs.org/docs/latest/tutorial/security â€” isolated sandboxed renderer, narrow validated IPC, restricted navigation/CSP.
- https://playwright.dev/docs/api/class-electron â€” Electron launch automation.
- https://github.com/microsoft/node-pty â€” real native terminal adapter.

## Browser-backed provider decision

No automatic ChatGPT Web extraction, cookie access, stealth, authentication bypass, CAPTCHA handling, or private-endpoint relay is implemented. Assisted copy/paste preserves the human-controlled workflow. Upstream harness descriptions do not grant contractual permission to automate a third-party service.

## Dependency reuse

Dependencies retain their own licenses. Exact resolved versions are in pnpm-lock.yaml; registry metadata is recorded in docs/research. Distribution of an installer requires a full transitive third-party-notice pass; this iteration is a source/development build.
