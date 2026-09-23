import { defineConfig } from '@playwright/test';

const runId = process.env.ASTRA_TEST_RUN_ID ?? `${Date.now()}-${process.pid}`;

export default defineConfig({
  testDir: 'tests/e2e',
  timeout: 60_000,
  workers: 1,
  reporter: [
    ['list'],
    ['json', { outputFile: `artifacts/e2e-results-${runId}.json` }],
  ],
  outputDir: `artifacts/e2e-${runId}`,
  use: { trace: 'retain-on-failure' },
});
