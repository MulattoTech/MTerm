/* global window */
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { _electron as electron } from '@playwright/test';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..');
const runId = `${Date.now()}-${process.pid}`;
const userData = path.join(root, 'artifacts', `agent-resume-profile-${runId}`);
const outputPath = path.join(root, 'artifacts', `agent-resume-smoke-${runId}.json`);
const main = path.join(root, 'apps', 'desktop', 'dist', 'main.cjs');

const app = await electron.launch({
  args: [main],
  cwd: root,
  env: { ...process.env, ASTRA_E2E_USER_DATA_DIR: userData },
});
const page = await app.firstWindow();

const result = await page.evaluate(async () => {
  await window.astra.setProfile('developer');
  const task = await window.astra.createTask(
    'Codex resume smoke',
    'Prove a persisted Codex thread can resume through the MTerm provider boundary.',
  );
  const grant = await window.astra.grantAgent();
  if (!grant.granted) throw new Error(grant.reason ?? 'Agent permission was not granted');

  const collect = async (start) => {
    const messages = [];
    let runningId = '';
    const completed = new Promise((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error('Agent smoke timed out')), 90_000);
      const off = window.astra.onAgentEvent(event => {
        if (runningId && event.id !== runningId) return;
        if (event.type === 'message' && event.text) messages.push(event.text);
        if (event.type === 'error') {
          clearTimeout(timer);
          off();
          reject(new Error(event.message ?? 'Agent event error'));
        }
        if (event.type === 'exit') {
          clearTimeout(timer);
          off();
          resolve({ exitCode: event.exitCode, messages });
        }
      });
    });
    const started = await start();
    if (!started.id) throw new Error(started.reason ?? 'Agent did not start');
    runningId = started.id;
    return { started, exit: await completed };
  };
  const first = await collect(() => window.astra.startAgent(
    'Return exactly ASTRA_FIRST_OK. Do not inspect or modify files and do not call tools.',
    task.id,
  ));
  const firstSessions = await window.astra.listAgentSessions();
  const firstSession = firstSessions.find(session => session.id === first.started.id);
  if (!firstSession?.resumabilityData?.threadId) throw new Error('Initial Codex thread ID was not persisted');

  const resumed = await collect(() => window.astra.resumeAgent(
    first.started.id,
    'Return exactly ASTRA_RESUME_OK. Do not inspect or modify files and do not call tools.',
    task.id,
  ));

  const sessions = await window.astra.listAgentSessions();
  const evidence = await window.astra.listEvidence();
  const tasks = await window.astra.listTasks();
  return {
    first,
    resumed,
    firstThreadId: firstSession.resumabilityData.threadId,
    sessions,
    evidence,
    tasks,
    taskId: task.id,
  };
});

await app.close();
const session = result.sessions.find(item => item.id === result.first.started.id);
if (!session || session.status !== 'DONE') {
  throw new Error(`Expected DONE resumed session, got ${session?.status ?? 'missing'}`);
}
if (result.resumed.started.id !== result.first.started.id || !result.resumed.started.resumed) {
  throw new Error('Resume did not reuse the persisted MTerm session');
}
if (session.resumabilityData?.threadId !== result.firstThreadId) {
  throw new Error('Codex resume did not preserve the original thread ID');
}
if (!result.first.exit.messages.some(text => text.includes('ASTRA_FIRST_OK'))) {
  throw new Error('Initial Codex provider response was not observed');
}
if (!result.resumed.exit.messages.some(text => text.includes('ASTRA_RESUME_OK'))) {
  throw new Error('Resumed Codex provider response was not observed');
}

const task = result.tasks.find(item => item.id === result.taskId);
const agentEvidence = result.evidence.filter(item =>
  item.taskId === result.taskId && item.label === 'Codex CLI agent run' && item.status === 'PASS');
if (agentEvidence.length < 2 || !task || agentEvidence.some(item => !task.evidenceIds.includes(item.id))) {
  throw new Error('Initial/resumed Codex PASS evidence was not linked to the task');
}
await fs.writeFile(outputPath, JSON.stringify(result, null, 2), 'utf8');
console.log(`Codex start/resume app smoke passed: ${outputPath}`);
