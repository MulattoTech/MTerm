import { app, BrowserWindow, dialog, ipcMain, shell } from 'electron';
import fs from 'node:fs/promises';
import path from 'node:path';
import { randomUUID } from 'node:crypto';
import { execFile, spawn } from 'node:child_process';
import { promisify } from 'node:util';
import pty from 'node-pty';
import { evaluate, safePath } from '../../../packages/core/src/security';
import { initialCanvas, validateCanvas, type Canvas } from '../../../packages/core/src/canvas';
import { agentSessionSchema, claudeCodeProviderManifest, codexCliProviderManifest, createEvidence, createTask, evidenceKindSchema, reconcileInterruptedSession, taskStatusSchema, transitionSession, validateEvidence, validateTask, type AgentSession, type Task, type ValidationEvidence } from '../../../packages/core/src/agency';
import { AstraStore, createAuditEvent } from '../../../packages/core/src/persistence';

type Workspace = {
  id: string;
  name: string;
  root: string;
  profile: 'observe' | 'developer';
};

const execFileAsync = promisify(execFile);
const sessionId = randomUUID();
if (process.env.ASTRA_E2E_USER_DATA_DIR) {
  app.setPath('userData', path.resolve(process.env.ASTRA_E2E_USER_DATA_DIR));
}
const grants: Array<Record<string, unknown>> = [];
const terminals = new Map<string, pty.IPty>();
const agentProcesses = new Map<string, ReturnType<typeof spawn>>();
let mainWindow: BrowserWindow | null = null;
let store: AstraStore;
let workspace: Workspace;
let canvas: Canvas;

function assertRelativePath(value: unknown): string {
  if (typeof value !== 'string' || value.length < 1 || value.length > 4096) throw new Error('Invalid relative path');
  return value;
}
function audit(tool: string, decision: string, target: string, result: string, started = Date.now()): void {
  store.addAudit(createAuditEvent({ tool, decision, target, result, started }));
}

function permission(capability: string): ReturnType<typeof evaluate> {
  return evaluate(workspace.profile, {
    workspaceId: workspace.id,
    actor: 'human',
    sessionId,
    capability,
  }, grants);
}

function stateSnapshot() {
  return {
    workspace,
    canvas,
    audits: store.audits(),
    terminalGranted: permission('terminal.execute').decision === 'ALLOW',
  };
}
async function listWorkspaceDirectory(relativePath: unknown = '.') {
  const started = Date.now();
  const target = typeof relativePath === 'string' && relativePath.length > 0 ? relativePath : '.';
  const decision = permission('filesystem.read');
  if (decision.decision !== 'ALLOW') {
    audit('filesystem.list', decision.decision, target, decision.reason, started);
    throw new Error(decision.reason);
  }
  const resolved = await safePath(workspace.root, target, 'read');
  const stat = await fs.stat(resolved);
  if (!stat.isDirectory()) throw new Error('Target is not a directory');
  const entries = (await fs.readdir(resolved, { withFileTypes: true })).slice(0, 500)
    .map(entry => ({
      name: entry.name,
      path: target === '.' ? entry.name : `${target.replace(/\\/g, '/')}/${entry.name}`,
      kind: entry.isDirectory() ? 'directory' as const : entry.isFile() ? 'file' as const : 'other' as const,
    }))
    .sort((a, b) => (a.kind === b.kind ? a.name.localeCompare(b.name) : a.kind === 'directory' ? -1 : 1));
  audit('filesystem.list', 'ALLOW', target, `Listed ${entries.length} entries`, started);
  return { path: target, entries };
}

async function readWorkspaceFile(relativePath: unknown) {
  const started = Date.now();
  const target = assertRelativePath(relativePath);
  const decision = permission('filesystem.read');
  if (decision.decision !== 'ALLOW') {
    audit('filesystem.read', decision.decision, target, decision.reason, started);
    throw new Error(decision.reason);
  }
  const resolved = await safePath(workspace.root, target, 'read');
  const stat = await fs.stat(resolved);
  if (!stat.isFile() || stat.size > 1_000_000) throw new Error('File is not a readable text-sized file');
  const content = await fs.readFile(resolved, 'utf8');
  const version = `${stat.size}:${stat.mtimeMs}`;
  audit('filesystem.read', 'ALLOW', target, `Read ${stat.size} bytes`, started);
  return { relativePath: target, content, size: stat.size, version };
}

async function writeWorkspaceFile(relativePath: unknown, content: unknown, expectedVersion?: unknown) {
  const started = Date.now();
  const target = assertRelativePath(relativePath);
  if (typeof content !== 'string' || Buffer.byteLength(content, 'utf8') > 1_000_000) throw new Error('Invalid or oversized content');
  const decision = permission('filesystem.write');
  if (decision.decision !== 'ALLOW') {
    audit('filesystem.write', decision.decision, target, decision.reason, started);
    throw new Error(decision.reason);
  }
  const resolved = await safePath(workspace.root, target, 'write');
  if (expectedVersion !== undefined && typeof expectedVersion !== 'string') throw new Error('Invalid file version');
  if (typeof expectedVersion === 'string') {
    try {
      const current = await fs.stat(resolved);
      const currentVersion = `${current.size}:${current.mtimeMs}`;
      if (currentVersion !== expectedVersion) throw new Error('STALE_WRITE: file changed since it was loaded');
    } catch (error) {
      if (error instanceof Error && error.message.startsWith('STALE_WRITE:')) throw error;
      const code = error instanceof Error && 'code' in error ? (error as NodeJS.ErrnoException).code : undefined;
      if (code === 'ENOENT') throw new Error('STALE_WRITE: file no longer exists');
      throw error;
    }
  }
  await fs.mkdir(path.dirname(resolved), { recursive: true });
  const temporary = `${resolved}.astra-${randomUUID()}.tmp`;
  await fs.writeFile(temporary, content, { encoding: 'utf8', flag: 'wx' });
  await fs.rename(temporary, resolved);
  const savedStat = await fs.stat(resolved);
  const version = `${savedStat.size}:${savedStat.mtimeMs}`;
  audit('filesystem.write', 'ALLOW', target, `Wrote ${Buffer.byteLength(content, 'utf8')} bytes`, started);
  return { relativePath: target, saved: true, version };
}

function workspaceTasks(): Task[] {
  return store.records<unknown>('task').flatMap(value => {
    try {
      const task = validateTask(value);
      return task.workspaceId === workspace.id ? [task] : [];
    } catch {
      return [];
    }
  });
}

function createWorkspaceTask(titleValue: unknown, objectiveValue: unknown): Task {
  if (typeof titleValue !== 'string' || typeof objectiveValue !== 'string') throw new Error('Invalid task input');
  const task = createTask({
    id: randomUUID(), workspaceId: workspace.id, title: titleValue.trim(), objective: objectiveValue.trim(),
  });
  store.putRecord('task', task.id, task.updatedAt, task);
  audit('task.create', 'ALLOW', task.id, 'Task created');
  return task;
}

function updateWorkspaceTask(idValue: unknown, statusValue: unknown): Task {
  if (typeof idValue !== 'string') throw new Error('Invalid task id');
  const status = taskStatusSchema.parse(statusValue);
  const task = workspaceTasks().find(candidate => candidate.id === idValue);
  if (!task) throw new Error('Task not found');
  const updated = validateTask({ ...task, status, updatedAt: new Date().toISOString() });
  store.putRecord('task', updated.id, updated.updatedAt, updated);
  audit('task.status', 'ALLOW', updated.id, status);
  return updated;
}

function updateWorkspaceTaskDetails(idValue: unknown, patchValue: unknown): Task {
  if (typeof idValue !== 'string' || !patchValue || typeof patchValue !== 'object') throw new Error('Invalid task detail update');
  const task = workspaceTasks().find(candidate => candidate.id === idValue);
  if (!task) throw new Error('Task not found');
  const patch = patchValue as Record<string, unknown>;
  const updated = validateTask({
    ...task,
    checklist: patch.checklist ?? task.checklist,
    blockers: patch.blockers ?? task.blockers,
    acceptanceCriteria: patch.acceptanceCriteria ?? task.acceptanceCriteria,
    ownerSessionId: patch.ownerSessionId === null ? undefined : (patch.ownerSessionId ?? task.ownerSessionId),
    updatedAt: new Date().toISOString(),
  });
  if (updated.ownerSessionId && !workspaceAgentSessions().some(session => session.id === updated.ownerSessionId)) {
    throw new Error('Task owner session does not belong to this workspace');
  }
  store.putRecord('task', updated.id, updated.updatedAt, updated);
  audit('task.details', 'ALLOW', updated.id, 'Updated checklist/blockers/acceptance criteria/owner');
  return updated;
}

function createWorkspaceEvidence(inputValue: unknown): ValidationEvidence {
  if (!inputValue || typeof inputValue !== 'object') throw new Error('Invalid evidence input');
  const input = inputValue as Record<string, unknown>;
  const taskId = typeof input.taskId === 'string' && input.taskId ? input.taskId : undefined;
  if (taskId && !workspaceTasks().some(task => task.id === taskId)) throw new Error('Evidence task does not belong to this workspace');
  const kind = evidenceKindSchema.parse(input.kind);
  const status = input.status;
  if (status !== 'PASS' && status !== 'FAIL' && status !== 'INFO') throw new Error('Invalid evidence status');
  if (typeof input.label !== 'string' || typeof input.summary !== 'string') throw new Error('Evidence label/summary must be text');
  const evidence = createEvidence({
    id: randomUUID(),
    taskId,
    kind,
    label: input.label,
    status,
    summary: input.summary,
    reference: typeof input.reference === 'string' && input.reference ? input.reference : undefined,
  });
  recordEvidence(evidence);
  audit('evidence.create', 'ALLOW', evidence.id, `${evidence.status} ${evidence.kind}`);
  return evidence;
}

function workspaceEvidence(): ValidationEvidence[] {
  const taskIds = new Set(workspaceTasks().map(task => task.id));
  return store.records<unknown>('evidence').flatMap(value => {
    try {
      const evidence = validateEvidence(value);
      return !evidence.taskId || taskIds.has(evidence.taskId) ? [evidence] : [];
    } catch { return []; }
  });
}

function recordEvidence(evidence: ValidationEvidence): ValidationEvidence {
  store.putRecord('evidence', evidence.id, evidence.createdAt, evidence);
  if (evidence.taskId) {
    const task = workspaceTasks().find(candidate => candidate.id === evidence.taskId);
    if (task && !task.evidenceIds.includes(evidence.id)) {
      const updated = validateTask({ ...task, evidenceIds: [...task.evidenceIds, evidence.id], updatedAt: new Date().toISOString() });
      store.putRecord('task', updated.id, updated.updatedAt, updated);
    }
  }
  return evidence;
}

function workspaceAgentSessions(): AgentSession[] {
  return store.records<unknown>('agent-session').flatMap(value => {
    try {
      const session = agentSessionSchema.parse(value);
      return session.workspaceId === workspace.id ? [session] : [];
    } catch {
      return [];
    }
  });
}

function saveAgentSession(session: AgentSession): void {
  store.putRecord('agent-session', session.id, session.updatedAt, session);
}

function reconcilePersistedAgentSessions(): number {
  let reconciled = 0;
  for (const value of store.records<unknown>('agent-session')) {
    try {
      const existing = agentSessionSchema.parse(value);
      const recovered = reconcileInterruptedSession(existing);
      if (recovered.status !== existing.status) {
        saveAgentSession(recovered);
        reconciled += 1;
      }
    } catch {
      // Invalid legacy records remain untouched for forensic visibility.
    }
  }
  return reconciled;
}

function shellEnvironment(): Record<string, string> {
  const env: Record<string, string> = {};
  for (const [key, value] of Object.entries(process.env)) {
    if (typeof value === 'string') env[key] = value;
  }
  return env;
}

function codexScriptPath(): string | null {
  const appData = process.env.APPDATA;
  return appData ? path.join(appData, 'npm', 'node_modules', '@openai', 'codex', 'bin', 'codex.js') : null;
}

async function codexProviderStatus() {
  const script = codexScriptPath();
  if (!script) return { ...codexCliProviderManifest, available: false, authenticated: false, reason: 'APPDATA is unavailable' };
  try {
    await fs.stat(script);
    const result = await execFileAsync('node', [script, '--version'], { windowsHide: true, timeout: 10_000, shell: false });
    return { ...codexCliProviderManifest, available: true, authenticated: true, version: String(result.stdout ?? '').trim(), sandbox: 'read-only' };
  } catch (error) {
    return { ...codexCliProviderManifest, available: false, authenticated: false, reason: error instanceof Error ? error.message : String(error) };
  }
}

function claudeExecutablePath(): string {
  if (process.platform === 'win32' && process.env.USERPROFILE) {
    return path.join(process.env.USERPROFILE, '.local', 'bin', 'claude.exe');
  }
  return 'claude';
}

async function claudeProviderStatus() {
  const executable = claudeExecutablePath();
  try {
    if (path.isAbsolute(executable)) await fs.stat(executable);
    const [versionResult, authResult] = await Promise.all([
      execFileAsync(executable, ['--version'], { windowsHide: true, timeout: 10_000, shell: false }),
      execFileAsync(executable, ['auth', 'status', '--json'], { windowsHide: true, timeout: 10_000, shell: false }),
    ]);
    const auth = JSON.parse(String(authResult.stdout || '{}')) as { loggedIn?: boolean; authMethod?: string; apiProvider?: string };
    return {
      ...claudeCodeProviderManifest,
      available: true,
      authenticated: auth.loggedIn === true,
      authMethod: auth.authMethod ?? 'unknown',
      apiProvider: auth.apiProvider ?? 'unknown',
      version: String(versionResult.stdout ?? '').trim(),
      sandbox: 'safe-mode + plan + no-tools',
      reason: auth.loggedIn ? undefined : 'Claude Code is installed but not authenticated',
    };
  } catch (error) {
    return {
      ...claudeCodeProviderManifest,
      available: false,
      authenticated: false,
      reason: error instanceof Error ? error.message : String(error),
    };
  }
}

function createTerminal(colsValue: unknown, rowsValue: unknown) {
  const started = Date.now();
  const decision = permission('terminal.execute');
  if (decision.decision !== 'ALLOW') {
    audit('terminal.create', decision.decision, workspace.root, decision.reason, started);
    return { needsApproval: true, reason: decision.reason };
  }
  const cols = typeof colsValue === 'number' && colsValue >= 20 && colsValue <= 400 ? Math.floor(colsValue) : 100;
  const rows = typeof rowsValue === 'number' && rowsValue >= 5 && rowsValue <= 200 ? Math.floor(rowsValue) : 30;
  const id = randomUUID();
  const shellExe = process.platform === 'win32' ? 'pwsh.exe' : (process.env.SHELL ?? '/bin/bash');
  const terminal = pty.spawn(shellExe, [], {
    name: 'xterm-256color',
    cols,
    rows,
    cwd: workspace.root,
    env: shellEnvironment(),
  });
  terminals.set(id, terminal);
  terminal.onData(data => mainWindow?.webContents.send('terminal:data', { id, data }));
  terminal.onExit(event => {
    terminals.delete(id);
    audit('terminal.exit', 'ALLOW', id, `exitCode=${event.exitCode}; signal=${event.signal}`);
    mainWindow?.webContents.send('terminal:exit', { id, exitCode: event.exitCode });
  });
  audit('terminal.create', 'ALLOW', workspace.root, `Started ${shellExe}`, started);
  return { id, needsApproval: false, shell: shellExe };
}

async function startCodexAgent(promptValue: unknown, taskIdValue?: unknown, resumeSessionIdValue?: unknown) {
  const started = Date.now();
  if (typeof promptValue !== 'string' || promptValue.trim().length < 1 || promptValue.length > 50_000) {
    throw new Error('Agent prompt must be 1-50,000 characters');
  }
  const resumeSessionId = typeof resumeSessionIdValue === 'string' && resumeSessionIdValue ? resumeSessionIdValue : undefined;
  const existingSession = resumeSessionId
    ? workspaceAgentSessions().find(session => session.id === resumeSessionId && session.providerId === 'codex-cli')
    : undefined;
  if (resumeSessionId && !existingSession) throw new Error('Codex session is unavailable for resume');
  const threadId = existingSession?.resumabilityData?.threadId;
  if (existingSession && !threadId) throw new Error('Codex session has no persisted thread ID');
  if (existingSession && agentProcesses.has(existingSession.id)) throw new Error('Codex session is already running');
  const requestedTaskId = typeof taskIdValue === 'string' && taskIdValue ? taskIdValue : undefined;
  const taskId = requestedTaskId ?? existingSession?.taskId;
  if (taskId && !workspaceTasks().some(task => task.id === taskId)) throw new Error('Selected task does not belong to this workspace');
  const auditTool = existingSession ? 'agent.codex.resume' : 'agent.codex.start';
  const policy = permission('agent.execute');
  if (policy.decision !== 'ALLOW') {
    audit(auditTool, policy.decision, workspace.root, policy.reason, started);
    return { needsApproval: policy.decision === 'ASK', denied: policy.decision === 'DENY', reason: policy.reason };
  }
  if (agentProcesses.size >= 4) throw new Error('Agent concurrency limit reached');

  const script = codexScriptPath();
  if (!script) throw new Error('Codex CLI path is unavailable');
  await fs.stat(script);

  const id = existingSession?.id ?? randomUUID();
  const createdAt = new Date().toISOString();
  let sessionRecord = existingSession
    ? agentSessionSchema.parse({
        ...transitionSession(existingSession, 'STARTING'),
        taskId,
      })
    : agentSessionSchema.parse({
        id,
        workspaceId: workspace.id,
        providerId: 'codex-cli',
        taskId,
        status: 'STARTING',
        contextRefs: [],
        createdAt,
        updatedAt: createdAt,
      });
  saveAgentSession(sessionRecord);
  if (taskId) updateWorkspaceTaskDetails(taskId, { ownerSessionId: id });

  const codexArgs = [
    script, 'exec', '--json', '--sandbox', 'read-only', '--skip-git-repo-check',
    '-C', workspace.root, '--color', 'never',
  ];
  if (existingSession && threadId) codexArgs.push('resume', threadId, '-');
  else codexArgs.push('-');

  const child = spawn('node', codexArgs, {
    cwd: workspace.root,
    env: shellEnvironment(),
    windowsHide: true,
    shell: false,
    stdio: ['pipe', 'pipe', 'pipe'],
  });

  agentProcesses.set(id, child);
  sessionRecord = agentSessionSchema.parse({ ...sessionRecord, status: 'RUNNING', updatedAt: new Date().toISOString() });
  saveAgentSession(sessionRecord);
  let stdoutBuffer = '';
  const emit = (payload: Record<string, unknown>) => mainWindow?.webContents.send('agent:event', { id, ...payload });

  child.stdout?.on('data', chunk => {
    const data = String(chunk);
    emit({ type: 'stdout', data });
    stdoutBuffer += data;
    const lines = stdoutBuffer.split(/\r?\n/);
    stdoutBuffer = lines.pop() ?? '';
    for (const line of lines) {
      if (!line.trim()) continue;
      try {
        const event = JSON.parse(line) as { type?: string; thread_id?: string; item?: { type?: string; text?: string } };
        if (event.type === 'thread.started' && event.thread_id) {
          sessionRecord = agentSessionSchema.parse({ ...sessionRecord, resumabilityData: { threadId: event.thread_id }, updatedAt: new Date().toISOString() });
          saveAgentSession(sessionRecord);
          emit({ type: 'thread', threadId: event.thread_id });
        }
        if (event.type === 'item.completed' && event.item?.type === 'agent_message' && typeof event.item.text === 'string') {
          emit({ type: 'message', text: event.item.text });
        }
      } catch {
        emit({ type: 'parse-warning', data: line.slice(0, 2_000) });
      }
    }
  });
  child.stderr?.on('data', chunk => emit({ type: 'stderr', data: String(chunk).slice(0, 20_000) }));
  child.on('error', error => {
    sessionRecord = agentSessionSchema.parse({ ...sessionRecord, status: 'FAILED', updatedAt: new Date().toISOString() });
    saveAgentSession(sessionRecord);
    audit('agent.codex.run', 'FAILED', workspace.root, error.message);
    emit({ type: 'error', message: error.message });
  });
  child.on('exit', (code, signal) => {
    agentProcesses.delete(id);
    sessionRecord = agentSessionSchema.parse({ ...sessionRecord, status: code === 0 ? 'DONE' : 'FAILED', updatedAt: new Date().toISOString() });
    saveAgentSession(sessionRecord);
    const outcome = code === 0 ? 'ALLOW' : 'FAILED';
    const summary = `Codex CLI exited with code ${code ?? 'null'}; signal=${signal ?? 'none'}`;
    const evidence = recordEvidence(createEvidence({
      id: randomUUID(),
      taskId: sessionRecord.taskId,
      kind: 'command',
      label: 'Codex CLI agent run',
      status: code === 0 ? 'PASS' : 'FAIL',
      summary,
      reference: sessionRecord.resumabilityData?.threadId,
      exitCode: code ?? undefined,
    }));
    audit('agent.codex.run', outcome, workspace.root, summary);
    emit({ type: 'evidence', evidenceId: evidence.id });
    emit({ type: 'exit', exitCode: code, signal });
  });

  child.stdin?.end(promptValue);
  audit(auditTool, 'ALLOW', workspace.root, existingSession ? 'Resumed read-only Codex CLI agent' : 'Started read-only Codex CLI agent', started);
  return { id, needsApproval: false, provider: 'codex-cli', sandbox: 'read-only', resumed: Boolean(existingSession) };
}

async function startClaudeAgent(promptValue: unknown, taskIdValue?: unknown, resumeSessionIdValue?: unknown) {
  const started = Date.now();
  if (typeof promptValue !== 'string' || promptValue.trim().length < 1 || promptValue.length > 50_000) {
    throw new Error('Agent prompt must be 1-50,000 characters');
  }
  const status = await claudeProviderStatus();
  if (!status.available) throw new Error(status.reason ?? 'Claude Code is unavailable');
  if (!status.authenticated) throw new Error(status.reason ?? 'Claude Code is not authenticated');
  const resumeSessionId = typeof resumeSessionIdValue === 'string' && resumeSessionIdValue ? resumeSessionIdValue : undefined;
  const existingSession = resumeSessionId
    ? workspaceAgentSessions().find(session => session.id === resumeSessionId && session.providerId === claudeCodeProviderManifest.id)
    : undefined;
  if (resumeSessionId && !existingSession) throw new Error('Claude session is unavailable for resume');
  const claudeSessionId = existingSession?.resumabilityData?.sessionId;
  if (existingSession && !claudeSessionId) throw new Error('Claude session has no persisted session ID');
  if (existingSession && agentProcesses.has(existingSession.id)) throw new Error('Claude session is already running');
  const requestedTaskId = typeof taskIdValue === 'string' && taskIdValue ? taskIdValue : undefined;
  const taskId = requestedTaskId ?? existingSession?.taskId;
  if (taskId && !workspaceTasks().some(task => task.id === taskId)) throw new Error('Selected task does not belong to this workspace');

  const policy = permission('agent.execute');
  const auditTool = existingSession ? 'agent.claude.resume' : 'agent.claude.start';
  if (policy.decision !== 'ALLOW') {
    audit(auditTool, policy.decision, workspace.root, policy.reason, started);
    return { needsApproval: policy.decision === 'ASK', denied: policy.decision === 'DENY', reason: policy.reason };
  }
  if (agentProcesses.size >= 4) throw new Error('Agent concurrency limit reached');

  const id = existingSession?.id ?? randomUUID();
  const createdAt = new Date().toISOString();
  let sessionRecord = existingSession
    ? agentSessionSchema.parse({ ...transitionSession(existingSession, 'STARTING'), taskId })
    : agentSessionSchema.parse({
        id, workspaceId: workspace.id, providerId: claudeCodeProviderManifest.id, taskId,
        status: 'STARTING', contextRefs: [], createdAt, updatedAt: createdAt,
      });
  saveAgentSession(sessionRecord);
  if (taskId) updateWorkspaceTaskDetails(taskId, { ownerSessionId: id });

  const args = ['-p', '--verbose', '--output-format', 'stream-json', '--permission-mode', 'plan', '--tools', '', '--safe-mode'];
  if (existingSession && claudeSessionId) args.push('--resume', claudeSessionId);
  args.push(promptValue);

  const child = spawn(claudeExecutablePath(), args, {
    cwd: workspace.root,
    env: shellEnvironment(),
    windowsHide: true,
    shell: false,
    stdio: ['ignore', 'pipe', 'pipe'],
  });
  agentProcesses.set(id, child);
  sessionRecord = agentSessionSchema.parse({ ...sessionRecord, status: 'RUNNING', updatedAt: new Date().toISOString() });
  saveAgentSession(sessionRecord);
  const emit = (payload: Record<string, unknown>) => mainWindow?.webContents.send('agent:event', { id, providerId: claudeCodeProviderManifest.id, ...payload });
  let stdoutBuffer = '';
  let providerError = false;

  child.stdout?.on('data', chunk => {
    stdoutBuffer += String(chunk);
    const lines = stdoutBuffer.split(/\r?\n/);
    stdoutBuffer = lines.pop() ?? '';
    for (const line of lines) {
      if (!line.trim()) continue;
      try {
        const event = JSON.parse(line) as {
          type?: string; subtype?: string; session_id?: string; model?: string; tools?: unknown[];
          is_error?: boolean; result?: string; message?: { content?: Array<{ type?: string; text?: string }> };
        };
        if (event.type === 'system' && event.subtype === 'init') {
          if (Array.isArray(event.tools) && event.tools.length > 0) {
            providerError = true;
            emit({ type: 'error', message: 'Claude provider unexpectedly exposed tools; terminating run.' });
            child.kill();
            continue;
          }
          if (event.session_id) {
            sessionRecord = agentSessionSchema.parse({
              ...sessionRecord,
              model: typeof event.model === 'string' ? event.model : sessionRecord.model,
              resumabilityData: { sessionId: event.session_id },
              updatedAt: new Date().toISOString(),
            });
            saveAgentSession(sessionRecord);
            emit({ type: 'thread', threadId: event.session_id });
          }
        }
        if (event.type === 'assistant') {
          for (const block of event.message?.content ?? []) {
            if (block.type === 'text' && typeof block.text === 'string') emit({ type: 'message', text: block.text });
          }
        }
        if (event.type === 'result' && event.is_error) {
          providerError = true;
          emit({ type: 'error', message: event.result ?? 'Claude provider returned an error result' });
        }
      } catch {
        emit({ type: 'parse-warning', data: line.slice(0, 2_000) });
      }
    }
  });
  child.stderr?.on('data', chunk => emit({ type: 'stderr', data: String(chunk).slice(0, 20_000) }));
  child.on('error', error => {
    providerError = true;
    sessionRecord = agentSessionSchema.parse({ ...sessionRecord, status: 'FAILED', updatedAt: new Date().toISOString() });
    saveAgentSession(sessionRecord);
    audit('agent.claude.run', 'FAILED', workspace.root, error.message);
    emit({ type: 'error', message: error.message });
  });
  child.on('exit', (code, signal) => {
    agentProcesses.delete(id);
    const passed = code === 0 && !providerError;
    sessionRecord = agentSessionSchema.parse({ ...sessionRecord, status: passed ? 'DONE' : 'FAILED', updatedAt: new Date().toISOString() });
    saveAgentSession(sessionRecord);
    const summary = `Claude Code exited with code ${code ?? 'null'}; signal=${signal ?? 'none'}`;
    const evidence = recordEvidence(createEvidence({
      id: randomUUID(), taskId: sessionRecord.taskId, kind: 'command',
      label: 'Claude Code agent run', status: passed ? 'PASS' : 'FAIL',
      summary, reference: sessionRecord.resumabilityData?.sessionId, exitCode: code ?? undefined,
    }));
    audit('agent.claude.run', passed ? 'ALLOW' : 'FAILED', workspace.root, summary);
    emit({ type: 'evidence', evidenceId: evidence.id });
    emit({ type: 'exit', exitCode: code, signal });
  });

  audit(auditTool, 'ALLOW', workspace.root, existingSession ? 'Resumed no-tools Claude Code session' : 'Started no-tools Claude Code session', started);
  return { id, needsApproval: false, provider: claudeCodeProviderManifest.id, sandbox: 'safe-mode + plan + no-tools', resumed: Boolean(existingSession) };
}

async function startProviderAgent(providerIdValue: unknown, prompt: unknown, taskId?: unknown, resumeSessionId?: unknown) {
  if (providerIdValue === codexCliProviderManifest.id) return startCodexAgent(prompt, taskId, resumeSessionId);
  if (providerIdValue === claudeCodeProviderManifest.id) return startClaudeAgent(prompt, taskId, resumeSessionId);
  throw new Error('Unknown or unsupported agent provider');
}

function stopAgentProcess(idValue: unknown) {
  if (typeof idValue !== 'string') return false;
  const child = agentProcesses.get(idValue);
  if (!child) return false;
  const stopped = child.kill();
  const providerId = workspaceAgentSessions().find(session => session.id === idValue)?.providerId ?? 'unknown';
  audit('agent.stop', stopped ? 'ALLOW' : 'FAILED', `${providerId}:${idValue}`, stopped ? 'Stop signal sent' : 'Stop signal was not sent');
  return stopped;
}

async function runGit(args: string[], maxBuffer = 1_000_000): Promise<string> {
  const result = await execFileAsync('git', args, {
    cwd: workspace.root,
    windowsHide: true,
    timeout: 20_000,
    maxBuffer,
    shell: false,
  });
  return String(result.stdout ?? '').slice(0, Math.min(maxBuffer, 500_000));
}

async function gitPathspec(value: unknown): Promise<string> {
  const target = assertRelativePath(value);
  const resolved = await safePath(workspace.root, target, 'write');
  const relative = path.relative(workspace.root, resolved).replace(/\\/g, '/');
  if (!relative || relative === '.' || relative.startsWith('../')) throw new Error('A specific workspace file is required');
  return relative;
}

async function gitChangedFiles() {
  const policy = permission('git.read');
  if (policy.decision !== 'ALLOW') throw new Error(policy.reason);
  const output = await runGit(['status', '--porcelain=v1', '-z']);
  const records = output.split('\0').filter(Boolean);
  const files: Array<{ path: string; index: string; workingTree: string }> = [];
  for (let index = 0; index < records.length && files.length < 250; index += 1) {
    const record = records[index]!;
    if (record.length < 4) continue;
    const xy = record.slice(0, 2);
    const filePath = record.slice(3);
    files.push({ path: filePath, index: xy[0] ?? ' ', workingTree: xy[1] ?? ' ' });
    if (xy.includes('R') || xy.includes('C')) index += 1;
  }
  return files;
}

async function gitSnapshot() {
  const started = Date.now();
  const policy = permission('git.read');
  if (policy.decision !== 'ALLOW') {
    audit('git.inspect', policy.decision, workspace.root, policy.reason, started);
    throw new Error(policy.reason);
  }

  try {
    const status = await runGit(['status', '--short', '--branch']);
    const [diffStat, worktrees, log, changedFiles] = await Promise.all([
      runGit(['diff', '--stat']).catch(() => ''),
      runGit(['worktree', 'list', '--porcelain']).catch(() => ''),
      runGit(['log', '-5', '--oneline', '--decorate']).catch(() => ''),
      gitChangedFiles().catch(() => []),
    ]);
    audit('git.inspect', 'ALLOW', workspace.root, 'Read status, diff stat, worktrees, and recent log', started);
    return { status, diffStat, worktrees, log, changedFiles, isRepository: true };
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    audit('git.inspect', 'FAILED', workspace.root, message, started);
    return { status: '', diffStat: '', worktrees: '', log: '', changedFiles: [], isRepository: false, error: message };
  }
}

async function gitDiff(pathValue: unknown, stagedValue: unknown) {
  const started = Date.now();
  const policy = permission('git.read');
  if (policy.decision !== 'ALLOW') throw new Error(policy.reason);
  const pathspec = await gitPathspec(pathValue);
  const staged = stagedValue === true;
  const args = ['diff', '--no-ext-diff', '--no-color'];
  if (staged) args.push('--cached');
  args.push('--', pathspec);
  const patch = await runGit(args, 500_000);
  audit('git.diff', 'ALLOW', pathspec, staged ? 'Read staged diff' : 'Read working-tree diff', started);
  return { path: pathspec, staged, patch };
}

async function gitStage(pathValue: unknown) {
  const started = Date.now();
  const policy = permission('git.write');
  if (policy.decision !== 'ALLOW') throw new Error(policy.reason);
  const pathspec = await gitPathspec(pathValue);
  await runGit(['add', '--', pathspec]);
  audit('git.stage', 'ALLOW', pathspec, 'Staged one explicit path', started);
  return { path: pathspec, staged: true };
}

async function gitUnstage(pathValue: unknown) {
  const started = Date.now();
  const policy = permission('git.write');
  if (policy.decision !== 'ALLOW') throw new Error(policy.reason);
  const pathspec = await gitPathspec(pathValue);
  const hasHead = await runGit(['rev-parse', '--verify', 'HEAD']).then(() => true).catch(() => false);
  if (hasHead) {
    await runGit(['restore', '--staged', '--', pathspec]);
  } else {
    await runGit(['rm', '--cached', '--ignore-unmatch', '--', pathspec]);
  }
  audit('git.unstage', 'ALLOW', pathspec, 'Unstaged one explicit path', started);
  return { path: pathspec, staged: false };
}

async function createGitWorktree(nameValue: unknown) {
  const started = Date.now();
  const policy = permission('git.worktree');
  if (policy.decision !== 'ALLOW') {
    audit('git.worktree.create', policy.decision, workspace.root, policy.reason, started);
    return { needsApproval: policy.decision === 'ASK', denied: policy.decision === 'DENY', reason: policy.reason };
  }
  if (typeof nameValue !== 'string') throw new Error('Worktree name must be text');
  const slug = nameValue.trim();
  if (!/^[a-zA-Z0-9][a-zA-Z0-9._-]{0,48}$/.test(slug) || slug.includes('..')) {
    throw new Error('Worktree name must be 1-49 safe filename characters');
  }
  await runGit(['rev-parse', '--verify', 'HEAD']);
  const suffix = Date.now().toString(36);
  const branch = `astra/${slug}-${suffix}`;
  const root = path.join(app.getPath('userData'), 'worktrees', workspace.id);
  await fs.mkdir(root, { recursive: true });
  const target = path.join(root, `${slug}-${suffix}`);
  await runGit(['worktree', 'add', '-b', branch, target, 'HEAD'], 2_000_000);
  audit('git.worktree.create', 'ALLOW', branch, `Created managed worktree at ${target}`, started);
  return { needsApproval: false, branch, path: target };
}

async function processSnapshot() {
  const started = Date.now();
  const policy = permission('process.read');
  if (policy.decision !== 'ALLOW') throw new Error(policy.reason);
  const terminalPids = new Set([...terminals.values()].map(terminal => terminal.pid));
  const agentPids = new Set([...agentProcesses.values()].flatMap(child => child.pid ? [child.pid] : []));
  let processes: Array<{ pid: number; name: string; cpu: number | null; memoryBytes: number; owned: boolean; owner?: string }> = [];

  if (process.platform === 'win32') {
    const command = [
      '$ErrorActionPreference="SilentlyContinue"',
      'Get-Process | Sort-Object WorkingSet64 -Descending | Select-Object -First 150 Id,ProcessName,CPU,WorkingSet64 | ConvertTo-Json -Compress',
    ].join('; ');
    const result = await execFileAsync('pwsh.exe', ['-NoLogo', '-NoProfile', '-Command', command], {
      windowsHide: true, timeout: 15_000, maxBuffer: 2_000_000, shell: false,
    });
    const parsed = JSON.parse(String(result.stdout || '[]')) as unknown;
    const rows = Array.isArray(parsed) ? parsed : [parsed];
    processes = rows.flatMap(row => {
      if (!row || typeof row !== 'object') return [];
      const item = row as { Id?: unknown; ProcessName?: unknown; CPU?: unknown; WorkingSet64?: unknown };
      const pid = Number(item.Id);
      if (!Number.isInteger(pid) || typeof item.ProcessName !== 'string') return [];
      return [{
        pid,
        name: item.ProcessName,
        cpu: typeof item.CPU === 'number' ? item.CPU : null,
        memoryBytes: Number(item.WorkingSet64) || 0,
        owned: terminalPids.has(pid) || agentPids.has(pid),
        owner: terminalPids.has(pid) ? 'terminal' : agentPids.has(pid) ? 'agent' : undefined,
      }];
    });
  } else {
    const result = await execFileAsync('ps', ['-eo', 'pid=,comm=,rss=,pcpu='], {
      timeout: 15_000, maxBuffer: 2_000_000, shell: false,
    });
    processes = String(result.stdout ?? '').split(/\r?\n/).flatMap(line => {
      const match = line.trim().match(/^(\d+)\s+(\S+)\s+(\d+)\s+([\d.]+)/);
      if (!match) return [];
      const pid = Number(match[1]);
      return [{
        pid, name: match[2]!, cpu: Number(match[4]), memoryBytes: Number(match[3]) * 1024,
        owned: terminalPids.has(pid) || agentPids.has(pid),
        owner: terminalPids.has(pid) ? 'terminal' : agentPids.has(pid) ? 'agent' : undefined,
      }];
    }).sort((a, b) => b.memoryBytes - a.memoryBytes).slice(0, 150);
  }

  audit('process.list', 'ALLOW', workspace.id, `Returned ${processes.length} process summaries`, started);
  return processes;
}

function killOwnedProcess(pidValue: unknown) {
  const started = Date.now();
  const policy = permission('process.kill');
  if (policy.decision !== 'ALLOW') {
    audit('process.kill', policy.decision, String(pidValue), policy.reason, started);
    return { needsApproval: policy.decision === 'ASK', denied: policy.decision === 'DENY', reason: policy.reason };
  }
  const pid = typeof pidValue === 'number' ? Math.trunc(pidValue) : Number.NaN;
  if (!Number.isInteger(pid) || pid <= 0) throw new Error('Invalid PID');
  const terminal = [...terminals.values()].find(candidate => candidate.pid === pid);
  if (terminal) {
    terminal.kill();
    audit('process.kill', 'ALLOW', String(pid), 'Stopped MTerm-owned terminal', started);
    return { stopped: true, owner: 'terminal' };
  }
  const agent = [...agentProcesses.values()].find(candidate => candidate.pid === pid);
  if (agent) {
    const stopped = agent.kill();
    audit('process.kill', stopped ? 'ALLOW' : 'FAILED', String(pid), 'Stop requested for MTerm-owned agent', started);
    return { stopped, owner: 'agent' };
  }
  audit('process.kill', 'DENY', String(pid), 'Only MTerm-owned processes can be terminated', started);
  return { stopped: false, denied: true, reason: 'Only MTerm-owned processes can be terminated' };
}

async function chooseWorkspace(): Promise<Workspace | null> {
  if (!mainWindow) return null;
  const result = await dialog.showOpenDialog(mainWindow, { properties: ['openDirectory'] });
  const selected = result.filePaths[0];
  if (result.canceled || !selected) return null;
  workspace = {
    id: randomUUID(),
    name: path.basename(selected),
    root: await fs.realpath(selected),
    profile: 'observe',
  };
  store.set('workspace', workspace);
  canvas = initialCanvas();
  store.set('canvas', canvas);
  audit('workspace.open', 'ALLOW', workspace.root, 'Workspace selected');
  return workspace;
}
function registerIpc(): void {
  ipcMain.handle('astra:state', () => stateSnapshot());
  ipcMain.handle('astra:workspace:choose', chooseWorkspace);
  ipcMain.handle('astra:profile:set', (_event, value: unknown) => {
    if (value !== 'observe' && value !== 'developer') throw new Error('Invalid permission profile');
    workspace = { ...workspace, profile: value };
    store.set('workspace', workspace);
    audit('permission.profile', 'ALLOW', value, 'Profile changed by the local UI');
    return workspace;
  });
  ipcMain.handle('astra:canvas:save', (_event, value: unknown) => {
    canvas = validateCanvas(value);
    store.set('canvas', canvas);
    audit('canvas.save', 'ALLOW', workspace.id, `${canvas.nodes.length} nodes`);
    return canvas;
  });
  ipcMain.handle('astra:file:list', (_event, relativePath: unknown) => listWorkspaceDirectory(relativePath));
  ipcMain.handle('astra:file:read', (_event, relativePath: unknown) => readWorkspaceFile(relativePath));
  ipcMain.handle('astra:file:write', (_event, relativePath: unknown, content: unknown, expectedVersion: unknown) => writeWorkspaceFile(relativePath, content, expectedVersion));
  ipcMain.handle('astra:git:snapshot', () => gitSnapshot());
  ipcMain.handle('astra:git:diff', (_event, filePath: unknown, staged: unknown) => gitDiff(filePath, staged));
  ipcMain.handle('astra:git:stage', (_event, filePath: unknown) => gitStage(filePath));
  ipcMain.handle('astra:git:unstage', (_event, filePath: unknown) => gitUnstage(filePath));
  ipcMain.handle('astra:git:worktree:grant', () => {
    const current = permission('git.worktree');
    if (current.decision === 'DENY') return { granted: false, reason: current.reason };
    if (current.decision === 'ALLOW') return { granted: true };
    grants.push({
      id: randomUUID(), workspaceId: workspace.id, actor: 'human', sessionId,
      capability: 'git.worktree', scope: 'session', expiresAt: Date.now() + 8 * 60 * 60 * 1000,
    });
    audit('permission.grant', 'ALLOW', 'git.worktree', 'Granted for current application session');
    return { granted: true };
  });
  ipcMain.handle('astra:git:worktree:create', (_event, name: unknown) => createGitWorktree(name));
  ipcMain.handle('astra:process:list', () => processSnapshot());
  ipcMain.handle('astra:process:grant-kill', () => {
    const current = permission('process.kill');
    if (current.decision === 'DENY') return { granted: false, reason: current.reason };
    if (current.decision === 'ALLOW') return { granted: true };
    grants.push({
      id: randomUUID(), workspaceId: workspace.id, actor: 'human', sessionId,
      capability: 'process.kill', scope: 'session', expiresAt: Date.now() + 8 * 60 * 60 * 1000,
    });
    audit('permission.grant', 'ALLOW', 'process.kill', 'Granted for current application session');
    return { granted: true };
  });
  ipcMain.handle('astra:process:kill-owned', (_event, pid: unknown) => killOwnedProcess(pid));
  ipcMain.handle('astra:task:list', () => workspaceTasks());
  ipcMain.handle('astra:task:create', (_event, title: unknown, objective: unknown) => createWorkspaceTask(title, objective));
  ipcMain.handle('astra:task:status', (_event, id: unknown, status: unknown) => updateWorkspaceTask(id, status));
  ipcMain.handle('astra:task:update-details', (_event, id: unknown, patch: unknown) => updateWorkspaceTaskDetails(id, patch));
  ipcMain.handle('astra:evidence:list', () => workspaceEvidence());
  ipcMain.handle('astra:evidence:create', (_event, input: unknown) => createWorkspaceEvidence(input));
  ipcMain.handle('astra:agent:sessions', () => workspaceAgentSessions());
  ipcMain.handle('astra:agent:status', async () => ({
    ...(await codexProviderStatus()),
    granted: permission('agent.execute').decision === 'ALLOW',
    profile: workspace.profile,
    active: agentProcesses.size,
  }));
  ipcMain.handle('astra:agent:providers', async () => {
    const statuses = await Promise.all([codexProviderStatus(), claudeProviderStatus()]);
    return statuses.map(status => ({
      ...status,
      granted: permission('agent.execute').decision === 'ALLOW',
      profile: workspace.profile,
      active: workspaceAgentSessions().filter(session =>
        session.providerId === status.id && agentProcesses.has(session.id)).length,
    }));
  });
  ipcMain.handle('astra:agent:grant', () => {
    const current = permission('agent.execute');
    if (current.decision === 'DENY') {
      audit('permission.grant', 'DENY', 'agent.execute', current.reason);
      return { granted: false, reason: current.reason };
    }
    if (current.decision === 'ALLOW') return { granted: true };
    grants.push({
      id: randomUUID(), workspaceId: workspace.id, actor: 'human', sessionId,
      capability: 'agent.execute', scope: 'session', expiresAt: Date.now() + 8 * 60 * 60 * 1000,
    });
    audit('permission.grant', 'ALLOW', 'agent.execute', 'Granted for current application session');
    return { granted: true };
  });
  ipcMain.handle('astra:agent:start', (_event, prompt: unknown, taskId: unknown) => startCodexAgent(prompt, taskId));
  ipcMain.handle('astra:agent:resume', (_event, sessionIdValue: unknown, prompt: unknown, taskId: unknown) => startCodexAgent(prompt, taskId, sessionIdValue));
  ipcMain.handle('astra:agent:provider-run', (_event, providerId: unknown, prompt: unknown, taskId: unknown, resumeSessionId: unknown) => startProviderAgent(providerId, prompt, taskId, resumeSessionId));
  ipcMain.handle('astra:agent:stop', (_event, id: unknown) => stopAgentProcess(id));
  ipcMain.handle('astra:terminal:grant', () => {
    grants.push({
      id: randomUUID(), workspaceId: workspace.id, actor: 'human', sessionId,
      capability: 'terminal.execute', scope: 'session', expiresAt: Date.now() + 8 * 60 * 60 * 1000,
    });
    audit('permission.grant', 'ALLOW', 'terminal.execute', 'Granted for current application session');
    return { granted: true };
  });
  ipcMain.handle('astra:terminal:create', (_event, cols: unknown, rows: unknown) => createTerminal(cols, rows));
  ipcMain.handle('astra:terminal:write', (_event, id: unknown, data: unknown) => {
    if (typeof id !== 'string' || typeof data !== 'string' || data.length > 100_000) return false;
    terminals.get(id)?.write(data);
    return terminals.has(id);
  });
  ipcMain.handle('astra:terminal:resize', (_event, id: unknown, cols: unknown, rows: unknown) => {
    if (typeof id !== 'string' || typeof cols !== 'number' || typeof rows !== 'number') return false;
    if (cols < 20 || rows < 5 || cols > 400 || rows > 200) return false;
    terminals.get(id)?.resize(Math.floor(cols), Math.floor(rows));
    return terminals.has(id);
  });
  ipcMain.handle('astra:terminal:kill', (_event, id: unknown) => {
    if (typeof id !== 'string') return false;
    const terminal = terminals.get(id);
    if (!terminal) return false;
    terminal.kill();
    return true;
  });
}
function createWindow(): void {
  mainWindow = new BrowserWindow({
    width: 1500,
    height: 940,
    minWidth: 1000,
    minHeight: 700,
    backgroundColor: '#080b12',
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
    },
  });

  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    if (/^https:\/\//i.test(url)) void shell.openExternal(url);
    return { action: 'deny' };
  });
  mainWindow.webContents.on('will-navigate', event => event.preventDefault());
  void mainWindow.loadFile(path.join(__dirname, 'renderer', 'index.html'));
  mainWindow.on('closed', () => { mainWindow = null; });
}
app.whenReady().then(() => {
  store = new AstraStore(path.join(app.getPath('userData'), 'astracommander.sqlite'));
  const integrity = store.integrity();
  if (!integrity.ok) throw new Error(`SQLite integrity check failed: ${integrity.message}`);
  const cwd = process.cwd();
  workspace = store.get<Workspace>('workspace', {
    id: 'local-default',
    name: path.basename(cwd),
    root: cwd,
    profile: 'observe',
  });
  const reconciledAgents = reconcilePersistedAgentSessions();
  if (reconciledAgents > 0) {
    audit('agent.reconcile', 'ALLOW', workspace.id, `Marked ${reconciledAgents} interrupted session(s) STOPPED at startup`);
  }
  const storedCanvas = store.get<unknown>('canvas', initialCanvas());
  try {
    canvas = validateCanvas(storedCanvas);
  } catch {
    canvas = initialCanvas();
    store.set('canvas', canvas);
  }
  registerIpc();
  createWindow();
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});
app.on('before-quit', () => {
  for (const terminal of terminals.values()) {
    try {
      terminal.kill();
    } catch {
      // Best effort during shutdown.
    }
  }
  terminals.clear();
  for (const child of agentProcesses.values()) {
    try { child.kill(); } catch { /* Best effort during shutdown. */ }
  }
  agentProcesses.clear();
  if (store) store.close();
});
