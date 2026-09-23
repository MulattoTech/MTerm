import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..');
const npm = process.platform === 'win32' ? 'npm.cmd' : 'npm';
const electron = process.platform === 'win32'
  ? path.join(root, 'node_modules', '.bin', 'electron.cmd')
  : path.join(root, 'node_modules', '.bin', 'electron');

const build = spawn(npm, ['run', 'build'], { cwd: root, stdio: 'inherit' });
build.on('exit', code => {
  if (code !== 0) process.exit(code ?? 1);
  const child = spawn(electron, ['apps/desktop/dist/main.cjs'], { cwd: root, stdio: 'inherit' });
  child.on('exit', childCode => process.exit(childCode ?? 0));
});
