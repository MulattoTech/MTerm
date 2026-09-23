import { defineConfig } from 'vitest/config';

const runId = process.env.ASTRA_TEST_RUN_ID ?? `${Date.now()}-${process.pid}`;

export default defineConfig({
  test: {
    include: ['tests/**/*.test.ts'],
    exclude: ['tests/e2e/**'],
    testTimeout: 20_000,
    hookTimeout: 20_000,
    pool: 'forks',
    maxWorkers: 2,
    reporters: ['default'],
    outputFile: `artifacts/vitest-${runId}.json`,
  },
});
