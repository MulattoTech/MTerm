import fs from 'node:fs/promises';
import path from 'node:path';
import os from 'node:os';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { randomUUID } from 'node:crypto';
import { McpServer } from '@modelcontextprotocol/server';
import { serveStdio } from '@modelcontextprotocol/server/stdio';
import { z } from 'zod';
import { classifyCommand, evaluate, safePath } from '../../../packages/core/src/security';
import { AstraStore, createAuditEvent } from '../../../packages/core/src/persistence';

const execFileAsync = promisify(execFile);
const root = path.resolve(process.env.ASTRA_WORKSPACE_ROOT ?? process.cwd());
const profile = process.env.ASTRA_MCP_PROFILE === 'developer' ? 'developer' : 'observe';
const actor = 'mcp-client';
const sessionId = process.env.ASTRA_MCP_SESSION_ID ?? randomUUID();
const terminalOptIn = process.env.ASTRA_MCP_ALLOW_TERMINAL === '1';
const platformDataRoot = process.platform === 'win32'
  ? (process.env.APPDATA ?? path.join(os.homedir(), 'AppData', 'Roaming'))
  : process.platform === 'darwin'
    ? path.join(os.homedir(), 'Library', 'Application Support')
    : (process.env.XDG_CONFIG_HOME ?? path.join(os.homedir(), '.config'));
const auditDbPath = path.resolve(process.env.ASTRA_MCP_AUDIT_DB ?? path.join(platformDataRoot, 'astracommander', 'astracommander.sqlite'));
const store = new AstraStore(auditDbPath);
const integrity = store.integrity();
if (!integrity.ok) throw new Error(`SQLite integrity check failed: ${integrity.message}`);

const grants = terminalOptIn ? [{
  id: randomUUID(),
  workspaceId: root,
  actor,
  sessionId,
  capability: 'terminal.execute',
  scope: 'session',
  expiresAt: Date.now() + 8 * 60 * 60 * 1000,
}] : [];

function decision(capability: string) {
  return evaluate(profile, {
    workspaceId: root,
    actor,
    sessionId,
    capability,
  }, grants);
}

function audit(tool: string, outcome: string, target: string, result: string, started = Date.now()): void {
  store.addAudit(createAuditEvent({ tool, decision: outcome, target, result, started }));
}

function toolError(error: unknown) {
  const message = error instanceof Error ? error.message : String(error);
  return { content: [{ type: 'text' as const, text: message }], isError: true };
}
process.once('exit', () => {
  try { store.close(); } catch { /* process is already exiting */ }
});

function createServer(): McpServer {
  const server = new McpServer(
    { name: 'astracommander-local', version: '0.1.0' },
    { capabilities: { tools: {} } },
  );

  server.registerTool('filesystem_read', {
    title: 'Read workspace file',
    description: 'Read a UTF-8 text file rooted inside the configured MTerm workspace.',
    inputSchema: z.object({
      path: z.string().min(1).max(4096),
    }),
    annotations: { readOnlyHint: true },
  }, async ({ path: relativePath }) => {
    const started = Date.now();
    try {
      const policy = decision('filesystem.read');
      if (policy.decision !== 'ALLOW') throw new Error(`DENIED: ${policy.reason}`);
      const resolved = await safePath(root, relativePath, 'read');
      const stat = await fs.stat(resolved);
      if (!stat.isFile()) throw new Error('DENIED: target is not a file');
      if (stat.size > 1_000_000) throw new Error('DENIED: file exceeds 1 MB MCP read limit');
      const text = await fs.readFile(resolved, 'utf8');
      audit('mcp.filesystem_read', 'ALLOW', relativePath, `Read ${stat.size} bytes`, started);
      return { content: [{ type: 'text' as const, text }] };
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      audit('mcp.filesystem_read', message.includes('DENIED') ? 'DENY' : 'FAILED', relativePath, message, started);
      return toolError(error);
    }
  });
  server.registerTool('filesystem_write', {
    title: 'Write workspace file',
    description: 'Write a UTF-8 text file inside the configured workspace. Requires ASTRA_MCP_PROFILE=developer.',
    inputSchema: z.object({
      path: z.string().min(1).max(4096),
      content: z.string().max(1_000_000),
    }),
    annotations: { readOnlyHint: false, destructiveHint: false },
  }, async ({ path: relativePath, content }) => {
    const started = Date.now();
    try {
      const policy = decision('filesystem.write');
      if (policy.decision !== 'ALLOW') throw new Error(`DENIED: ${policy.reason}`);
      if (Buffer.byteLength(content, 'utf8') > 1_000_000) throw new Error('DENIED: content exceeds 1 MB MCP write limit');
      const resolved = await safePath(root, relativePath, 'write');
      await fs.mkdir(path.dirname(resolved), { recursive: true });
      const temporary = `${resolved}.astra-${randomUUID()}.tmp`;
      await fs.writeFile(temporary, content, { encoding: 'utf8', flag: 'wx' });
      await fs.rename(temporary, resolved);
      const byteLength = Buffer.byteLength(content, 'utf8');
      audit('mcp.filesystem_write', 'ALLOW', relativePath, `Wrote ${byteLength} bytes`, started);
      return { content: [{ type: 'text' as const, text: `Wrote ${byteLength} bytes to ${relativePath}` }] };
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      audit('mcp.filesystem_write', message.includes('DENIED') ? 'DENY' : 'FAILED', relativePath, message, started);
      return toolError(error);
    }
  });
  server.registerTool('terminal_execute', {
    title: 'Execute workspace command',
    description: 'Execute one program without a shell in the workspace. Requires Developer profile and ASTRA_MCP_ALLOW_TERMINAL=1.',
    inputSchema: z.object({
      executable: z.string().min(1).max(1024),
      args: z.array(z.string().max(4096)).max(64).default([]),
      timeoutMs: z.number().int().min(100).max(120_000).default(30_000),
    }),
    annotations: { readOnlyHint: false, destructiveHint: true },
  }, async ({ executable, args, timeoutMs }) => {
    const started = Date.now();
    try {
      const policy = decision('terminal.execute');
      if (policy.decision !== 'ALLOW') throw new Error(`DENIED: ${policy.reason}`);
      const classified = classifyCommand(executable, args);
      if (classified.risk === 'DESTRUCTIVE') throw new Error(`DENIED: destructive command classification: ${classified.summary}`);
      const result = await execFileAsync(executable, args, {
        cwd: root,
        timeout: timeoutMs,
        windowsHide: true,
        maxBuffer: 1_000_000,
        shell: false,
      });
      const stdout = result.stdout ?? '';
      const stderr = result.stderr ?? '';
      const text = [stdout, stderr && `[stderr]\n${stderr}`].filter(Boolean).join('\n').slice(0, 1_000_000);
      audit('mcp.terminal_execute', 'ALLOW', executable, `Completed ${classified.risk.toLowerCase()} command`, started);
      return { content: [{ type: 'text' as const, text: text || '(command completed with no output)' }] };
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      audit('mcp.terminal_execute', message.includes('DENIED') ? 'DENY' : 'FAILED', executable, message, started);
      return toolError(error);
    }
  });

  return server;
}
serveStdio(() => createServer(), {
  onerror: error => {
    process.stderr.write(`[MTerm MCP] ${error.message}\n`);
  },
});
