import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { Client } from '@modelcontextprotocol/client';
import { StdioClientTransport } from '@modelcontextprotocol/client/stdio';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..');
const serverPath = path.join(root, 'apps', 'desktop', 'dist', 'mcp.cjs');
const runId = `${Date.now()}-${process.pid}`;
const auditDb = path.join(root, 'artifacts', `mcp-audit-${runId}.sqlite`);
const validationFile = `artifacts/mcp-validation-${runId}.txt`;

function stringEnv(extra) {
  return Object.fromEntries(
    Object.entries({ ...process.env, ...extra }).filter(([, value]) => typeof value === 'string'),
  );
}

function textOf(result) {
  return result.content
    .filter(item => item.type === 'text')
    .map(item => item.text)
    .join('\n');
}

async function connect(extraEnv) {
  const transport = new StdioClientTransport({
    command: process.execPath,
    args: [serverPath],
    cwd: root,
    env: stringEnv({
      ASTRA_WORKSPACE_ROOT: root,
      ASTRA_MCP_AUDIT_DB: auditDb,
      ...extraEnv,
    }),
    stderr: 'pipe',
  });
  const client = new Client({ name: 'astracommander-validator', version: '0.1.0' });
  const stderr = [];
  transport.stderr?.on('data', chunk => stderr.push(String(chunk)));
  await client.connect(transport);
  return { client, transport, stderr };
}

async function validateObserve() {
  const { client, transport, stderr } = await connect({ ASTRA_MCP_PROFILE: 'observe' });
  try {
    const tools = await client.listTools();
    const names = tools.tools.map(tool => tool.name).sort();
    for (const required of ['filesystem_read', 'filesystem_write', 'terminal_execute']) {
      if (!names.includes(required)) throw new Error(`Missing MCP tool: ${required}`);
    }

    const read = await client.callTool({ name: 'filesystem_read', arguments: { path: 'package.json' } });
    if (read.isError || !textOf(read).includes('astracommander')) throw new Error('Observe read validation failed');

    const deniedWrite = await client.callTool({
      name: 'filesystem_write',
      arguments: { path: 'artifacts/mcp-observe-denied.txt', content: 'must not be written' },
    });
    if (!deniedWrite.isError || !textOf(deniedWrite).includes('DENIED')) throw new Error('Observe write was not denied');

    const deniedTerminal = await client.callTool({
      name: 'terminal_execute',
      arguments: { executable: process.execPath, args: ['--version'] },
    });
    if (!deniedTerminal.isError || !textOf(deniedTerminal).includes('DENIED')) throw new Error('Observe terminal was not denied');
  } finally {
    await client.close();
    await transport.close().catch(() => undefined);
  }
  if (stderr.length) process.stderr.write(stderr.join(''));
}
async function validateDeveloper() {
  const { client, transport, stderr } = await connect({
    ASTRA_MCP_PROFILE: 'developer',
    ASTRA_MCP_ALLOW_TERMINAL: '1',
  });
  try {
    const write = await client.callTool({
      name: 'filesystem_write',
      arguments: { path: validationFile, content: 'MTerm MCP validation\n' },
    });
    if (write.isError) throw new Error(`Developer write failed: ${textOf(write)}`);

    const read = await client.callTool({
      name: 'filesystem_read',
      arguments: { path: validationFile },
    });
    if (read.isError || !textOf(read).includes('MCP validation')) throw new Error('Developer readback failed');

    const command = await client.callTool({
      name: 'terminal_execute',
      arguments: { executable: process.execPath, args: ['--version'], timeoutMs: 15_000 },
    });
    if (command.isError || !/^v\d+/m.test(textOf(command))) throw new Error(`Terminal validation failed: ${textOf(command)}`);

    const destructive = await client.callTool({
      name: 'terminal_execute',
      arguments: { executable: 'git', args: ['reset', '--hard'] },
    });
    if (!destructive.isError || !textOf(destructive).includes('destructive command')) {
      throw new Error('Destructive command was not rejected');
    }
  } finally {
    await client.close();
    await transport.close().catch(() => undefined);
  }
  if (stderr.length) process.stderr.write(stderr.join(''));
}
await validateObserve();
await validateDeveloper();
console.log('MCP validation passed: discovery, Observe denial, Developer read/write, terminal execution, destructive-command rejection.');
