import { expect, test, _electron as electron, type ElectronApplication } from '@playwright/test';
import { Client } from '@modelcontextprotocol/client';
import { StdioClientTransport } from '@modelcontextprotocol/client/stdio';
import path from 'node:path';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..', '..');
const main = path.join(root, 'apps', 'desktop', 'dist', 'main.cjs');
const mcpMain = path.join(root, 'apps', 'desktop', 'dist', 'mcp.cjs');
const runId = `${Date.now()}-${process.pid}`;
const userData = path.join(root, 'artifacts', `e2e-profile-${runId}`);
const e2eWrite = `artifacts/e2e-write-${runId}.txt`;
const screenshot = path.join(root, 'artifacts', `e2e-first-slice-${runId}.png`);
const taskTitle = `E2E durable task ${runId}`;

async function launch(): Promise<ElectronApplication> {
  return electron.launch({
    args: [main],
    cwd: root,
    env: { ...process.env, ASTRA_E2E_USER_DATA_DIR: userData },
  });
}

test('first vertical slice is runnable and recoverable', async () => {
  const app = await launch();
  const page = await app.firstWindow();
  await expect(page.locator('.brand').getByText('MTerm', { exact: true })).toBeVisible();
  await expect(page.getByText('A spatial AI command center', { exact: true })).toBeVisible();
  await page.keyboard.press('Control+K');
  await expect(page.getByLabel('Command palette')).toBeVisible();
  await page.getByLabel('Command palette').fill('Show Git');
  await expect(page.getByRole('button', { name: /Show Git/ })).toBeVisible();
  await page.keyboard.press('Escape');
  await expect(page.getByLabel('Command palette')).toBeHidden();

  await expect(page.getByText('Observe profile', { exact: true })).toBeVisible();
  await page.getByRole('button', { name: 'Enable Developer profile' }).click();
  await expect(page.getByText('Developer profile', { exact: true })).toBeVisible();

  await page.getByRole('button', { name: '＋ Note' }).click();
  await expect(page.getByText('New note', { exact: true })).toBeVisible();

  await page.getByRole('button', { name: 'Tasks', exact: true }).click();
  await page.getByLabel('Task title').fill(taskTitle);
  await page.getByLabel('Task objective').fill('Prove task persistence across an Electron restart.');
  await page.getByRole('button', { name: 'Create task' }).click();
  await expect(page.locator('.task-list .task-card').filter({ hasText: taskTitle })).toBeVisible();
  await page.getByLabel(`Status for ${taskTitle}`).selectOption('RUNNING');
  await expect(page.getByLabel(`Status for ${taskTitle}`)).toHaveValue('RUNNING');
  await page.getByLabel('Checklist item').fill('Verify durable evidence');
  await page.getByRole('button', { name: 'Add', exact: true }).click();
  await expect(page.locator('.check-row').filter({ hasText: 'Verify durable evidence' })).toBeVisible();
  await page.getByLabel('Blockers').fill('No blocking issue');
  await page.getByLabel('Acceptance criteria').fill('Task and evidence survive restart');
  await page.getByRole('button', { name: 'Save task details' }).click();
  await page.getByLabel('Evidence label').fill('E2E validation');
  await page.getByLabel('Evidence summary').fill('Task workbench persists structured validation evidence.');
  await page.getByLabel('Evidence reference').fill('artifacts/e2e');
  await page.getByRole('button', { name: 'Record evidence' }).click();
  await expect(page.locator('.evidence-card').filter({ hasText: 'E2E validation' })).toBeVisible();

  await page.getByRole('button', { name: 'Agent', exact: true }).click();
  await expect(page.getByText(/codex-cli/i)).toBeVisible();
  const providerSelect = page.getByLabel('Agent provider');
  await expect(providerSelect).toContainText('Claude Code');
  await providerSelect.selectOption('claude-code');
  await expect(page.getByRole('button', { name: 'Run Claude Code' })).toBeDisabled();
  await providerSelect.selectOption('codex-cli');

  await page.getByRole('button', { name: '＋ Agent node' }).click();
  const spatialAgent = page.locator('.spatial-node').filter({ hasText: 'Provider-selectable local agent execution' }).last();
  await spatialAgent.getByRole('button', { name: 'Open live surface' }).click();
  await expect(spatialAgent.locator('.live-node-surface .agent-panel')).toBeVisible();
  await spatialAgent.getByRole('button', { name: 'Close live surface' }).click();

  const written = await page.evaluate(async (relativePath) => {
    await window.astra.writeFile(relativePath, 'MTerm E2E\n');
    return window.astra.readFile(relativePath);
  }, e2eWrite) as { content: string; version: string };
  expect(written.content).toContain('MTerm E2E');
  const staleResult = await page.evaluate(async ({ relativePath, version }) => {
    await window.astra.writeFile(relativePath, 'MTerm E2E updated\n', version);
    try {
      await window.astra.writeFile(relativePath, 'stale write must fail\n', version);
      return 'unexpected success';
    } catch (error) {
      return error instanceof Error ? error.message : String(error);
    }
  }, { relativePath: e2eWrite, version: written.version });
  expect(staleResult).toContain('STALE_WRITE');

  await page.getByRole('button', { name: 'Editor', exact: true }).click();
  await expect(page.locator('.file-browser').getByText('package.json', { exact: true })).toBeVisible();
  await page.locator('.file-browser').getByText('package.json', { exact: true }).click();
  await expect(page.getByText(/Loaded \d+ bytes\./)).toBeVisible();
  await expect(page.locator(".editor-host .view-lines")).toContainText(/mterm/);
  await expect(page.getByLabel('Workspace relative file')).toHaveValue('package.json');

  await page.getByRole('button', { name: 'Git', exact: true }).click();
  await expect(page.getByRole('heading', { name: 'Git / Diff' })).toBeVisible();
  const gitStatus = await promisify(execFile)('git', ['status', '--short', '--branch'], {
    cwd: root, windowsHide: true, shell: false,
  });
  const expectedHeader = gitStatus.stdout.split(/\r?\n/)[0];
  if (!expectedHeader) throw new Error("Git status returned no branch header");
  await expect(page.locator('.git-lower article').first()).toContainText(expectedHeader);

  await page.getByRole('button', { name: 'Terminal', exact: true }).click();
  await page.getByRole('button', { name: 'Start terminal' }).click();
  await expect(page.getByRole('button', { name: 'Allow this session' })).toBeVisible();
  await page.getByRole('button', { name: 'Allow this session' }).click();
  await expect(page.getByText(/Running pwsh\.exe/)).toBeVisible({ timeout: 15_000 });

  const terminalInput = page.locator('.xterm-helper-textarea');
  await terminalInput.click();
  await terminalInput.pressSequentially('Write-Output ASTRA_E2E', { delay: 5 });
  await terminalInput.press('Enter');
  await expect(page.locator('.xterm-rows')).toContainText('ASTRA_E2E', { timeout: 15_000 });

  await page.getByRole('button', { name: 'Processes', exact: true }).click();
  await expect(page.getByRole('heading', { name: 'Processes' })).toBeVisible();
  const ownedTerminal = page.locator('.process-row').filter({ hasText: 'terminal' }).first();
  await expect(ownedTerminal).toBeVisible();
  await ownedTerminal.getByRole('button', { name: 'Pin' }).click();
  await expect(page.locator('.react-flow__node').filter({ hasText: 'process' }).first()).toBeVisible();

  const mcpEnv = Object.fromEntries(Object.entries({
    ...process.env,
    ASTRA_WORKSPACE_ROOT: root,
    ASTRA_MCP_PROFILE: 'observe',
    ASTRA_MCP_AUDIT_DB: path.join(userData, 'astracommander.sqlite'),
  }).filter((entry): entry is [string, string] => typeof entry[1] === 'string'));
  const mcpTransport = new StdioClientTransport({ command: process.execPath, args: [mcpMain], cwd: root, env: mcpEnv, stderr: 'pipe' });
  const mcpClient = new Client({ name: 'astracommander-e2e', version: '0.1.0' });
  await mcpClient.connect(mcpTransport);
  const mcpRead = await mcpClient.callTool({ name: 'filesystem_read', arguments: { path: 'package.json' } });
  expect(mcpRead.isError).not.toBe(true);
  await mcpClient.close();

  await page.getByRole('button', { name: /Audit/ }).click();
  await expect(page.getByText('terminal.create', { exact: true }).first()).toBeVisible();
  await expect(page.getByText('filesystem.write', { exact: true }).first()).toBeVisible();
  await expect(page.getByText('mcp.filesystem_read', { exact: true })).toBeVisible();
  await page.screenshot({ path: screenshot, fullPage: true });

  await app.close();

  const relaunched = await launch();
  const restoredPage = await relaunched.firstWindow();
  await expect(restoredPage.getByText('New note', { exact: true })).toBeVisible();
  await expect(restoredPage.locator('.brand').getByText('MTerm', { exact: true })).toBeVisible();
  await restoredPage.getByRole('button', { name: 'Tasks', exact: true }).click();
  await expect(restoredPage.locator('.task-list .task-card').filter({ hasText: taskTitle })).toBeVisible();
  await expect(restoredPage.getByLabel(`Status for ${taskTitle}`)).toHaveValue('RUNNING');
  await expect(restoredPage.locator('.check-row').filter({ hasText: 'Verify durable evidence' })).toBeVisible();
  await expect(restoredPage.locator('.evidence-card').filter({ hasText: 'E2E validation' })).toBeVisible();
  await expect(restoredPage.locator('.react-flow__node').filter({ hasText: 'process' }).first()).toBeVisible();
  await relaunched.close();
});
