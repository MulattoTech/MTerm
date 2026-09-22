import { expect, test, _electron as electron } from '@playwright/test';
import fs from 'node:fs/promises';
import path from 'node:path';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { fileURLToPath } from 'node:url';

const execFileAsync = promisify(execFile);
const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..', '..');
const main = path.join(root, 'apps', 'desktop', 'dist', 'main.cjs');

async function git(cwd: string, args: string[]) {
  await execFileAsync('git', args, { cwd, windowsHide: true, shell: false });
}

test('Git workbench mutates only explicit paths and creates managed worktrees', async () => {
  const id = `${Date.now()}-${process.pid}`;
  const repo = path.join(root, 'artifacts', `e2e-git-repo-${id}`);
  const userData = path.join(root, 'artifacts', `e2e-git-profile-${id}`);
  await fs.mkdir(repo, { recursive: true });
  await git(repo, ['init', '-b', 'main']);
  await git(repo, ['config', 'user.email', 'astra-e2e@example.invalid']);
  await git(repo, ['config', 'user.name', 'MTerm E2E']);
  await fs.writeFile(path.join(repo, 'hello.txt'), 'before\n', 'utf8');
  await git(repo, ['add', '--', 'hello.txt']);
  await git(repo, ['commit', '-m', 'baseline']);
  await fs.writeFile(path.join(repo, 'hello.txt'), 'after\n', 'utf8');

  const app = await electron.launch({
    args: [main],
    cwd: repo,
    env: { ...process.env, ASTRA_E2E_USER_DATA_DIR: userData },
  });
  const page = await app.firstWindow();

  await page.getByRole('button', { name: 'Enable Developer profile' }).click();
  await page.getByRole('button', { name: 'Git', exact: true }).click();
  await expect(page.getByRole('heading', { name: 'Git / Diff' })).toBeVisible();

  const changed = page.locator('.git-changes button').filter({ hasText: 'hello.txt' });
  await expect(changed).toBeVisible();
  await changed.click();
  await expect(page.locator('.diff-view > pre')).toContainText('after');
  await page.getByRole('button', { name: 'Pin Diff node' }).click();
  await expect(page.locator('.react-flow__node').filter({ hasText: 'diff' }).first()).toBeVisible();

  await page.getByRole('button', { name: 'Stage file', exact: true }).click();
  await expect(page.locator('.diff-view > pre')).toContainText('after');
  await expect.poll(async () => {
    const staged = await execFileAsync('git', ['diff', '--cached', '--name-only'], {
      cwd: repo, windowsHide: true, shell: false,
    });
    return staged.stdout.trim();
  }).toContain('hello.txt');

  await page.getByRole('button', { name: 'Unstage file', exact: true }).click();
  await expect.poll(async () => {
    const unstaged = await execFileAsync('git', ['diff', '--cached', '--name-only'], {
      cwd: repo, windowsHide: true, shell: false,
    });
    return unstaged.stdout.trim();
  }).toBe('');

  await page.getByLabel('Worktree name').fill('e2e');
  await page.getByRole('button', { name: 'Create worktree' }).click();
  await expect(page.getByRole('button', { name: 'Allow worktrees this session' })).toBeVisible();
  await page.getByRole('button', { name: 'Allow worktrees this session' }).click();
  await expect(page.locator('.git-lower').getByText(/astra\/e2e-/)).toBeVisible();
  const worktrees = await execFileAsync('git', ['worktree', 'list', '--porcelain'], {
    cwd: repo, windowsHide: true, shell: false,
  });
  expect(worktrees.stdout).toContain('refs/heads/astra/e2e-');
  expect(worktrees.stdout).toContain(userData.replace(/\\/g, '/'));

  await app.close();
});
