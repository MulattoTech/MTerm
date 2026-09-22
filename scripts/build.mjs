import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { build as esbuild } from 'esbuild';
import { build as viteBuild } from 'vite';
import react from '@vitejs/plugin-react';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..');
const desktop = path.join(root, 'apps', 'desktop');
const dist = path.join(desktop, 'dist');
const rendererRoot = path.join(desktop, 'src', 'renderer');

// Continuation runs are intentionally additive: preserve prior build artifacts/checkpoints.
await fs.mkdir(dist, { recursive: true });

const shared = {
  bundle: true,
  sourcemap: true,
  target: 'node22',
  logLevel: 'info',
};

await esbuild({
  ...shared,
  entryPoints: [path.join(desktop, 'src', 'main.ts')],
  outfile: path.join(dist, 'main.cjs'),
  platform: 'node',
  format: 'cjs',
  external: ['electron', 'node-pty'],
});

await esbuild({
  ...shared,
  entryPoints: [path.join(desktop, 'src', 'preload.ts')],
  outfile: path.join(dist, 'preload.cjs'),
  platform: 'node',
  format: 'cjs',
  external: ['electron'],
});

await esbuild({
  ...shared,
  entryPoints: [path.join(desktop, 'src', 'mcp.ts')],
  outfile: path.join(dist, 'mcp.cjs'),
  platform: 'node',
  format: 'cjs',
});

await viteBuild({
  root: rendererRoot,
  base: './',
  plugins: [react()],

  build: {
    outDir: path.join(dist, 'renderer'),
    emptyOutDir: false,
    sourcemap: true,
  },
  define: {
    __ASTRA_VERSION__: JSON.stringify(process.env.npm_package_version ?? '0.1.0'),
  },
});

console.log(`MTerm build complete: ${dist}`);
