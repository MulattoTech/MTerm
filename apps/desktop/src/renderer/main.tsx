import React, { useCallback, useEffect, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import {
  Background, Controls, MiniMap, ReactFlow, applyNodeChanges,
  type Node, type NodeChange,
} from '@xyflow/react';
import { Terminal as XTerm } from '@xterm/xterm';
import { FitAddon } from '@xterm/addon-fit';
import type { editor as MonacoEditor } from 'monaco-editor';
import EditorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker';
import type { Canvas, CanvasNode } from '../../../../packages/core/src/canvas';
import type { AgentSession, EvidenceKind, Task, TaskStatus, ValidationEvidence } from '../../../../packages/core/src/agency';
import '@xyflow/react/dist/style.css';
import '@xterm/xterm/css/xterm.css';
import './styles.css';

(self as typeof self & { MonacoEnvironment?: { getWorker: () => Worker } }).MonacoEnvironment = {
  getWorker: () => new EditorWorker(),
};

type Workspace = { id: string; name: string; root: string; profile: 'observe' | 'developer' };
type AuditEvent = {
  id: string; timestamp: string; tool: string; decision: string;
  target: string; result: string; durationMs: number;
};
type AppState = { workspace: Workspace; canvas: Canvas; audits: AuditEvent[]; terminalGranted: boolean };
type GitChangedFile = { path: string; index: string; workingTree: string };
type GitSnapshot = { status: string; diffStat: string; worktrees: string; log: string; changedFiles: GitChangedFile[]; isRepository: boolean; error?: string };
type GitDiff = { path: string; staged: boolean; patch: string };
type ProcessInfo = { pid: number; name: string; cpu: number | null; memoryBytes: number; owned: boolean; owner?: string };
type DirectoryEntry = { name: string; path: string; kind: 'directory' | 'file' | 'other' };
type AgentStatus = { id: string; name?: string; available: boolean; authenticated?: boolean; version?: string; reason?: string; sandbox?: string; capabilities?: string[]; granted: boolean; profile: Workspace['profile']; active: number };
type AgentEvent = { id: string; type: string; data?: string; text?: string; message?: string; threadId?: string; exitCode?: number | null; signal?: string | null };
type OnNodeDragStop = NonNullable<React.ComponentProps<typeof ReactFlow>['onNodeDragStop']>;
function NodeCard({ node }: { node: CanvasNode }) {
  return (
    <div className="node-card">
      <div className="node-card__top">
        <span className={`kind kind--${node.kind}`}>{node.kind}</span>
        <span className={`status status--${node.status}`}>{node.status}</span>
      </div>
      <strong>{node.title || 'Untitled'}</strong>
      <p>{node.content || 'No content yet.'}</p>
      {node.kind === 'diff' && node.response && <pre className="node-diff-preview">{node.response.slice(0, 3500)}</pre>}
      {node.kind === 'process' && node.response && <pre className="node-process-preview">{node.response.slice(0, 1800)}</pre>}
      {node.items.length > 0 && (
        <div className="node-checklist">
          {node.items.slice(0, 4).map(item => (
            <span key={item.id}>{item.done ? '☑' : '☐'} {item.label}</span>
          ))}
        </div>
      )}
    </div>
  );
}

const noopChanged = async () => {};

function SpatialLiveSurface({ node }: { node: CanvasNode }) {
  if (node.kind === 'terminal') return <TerminalPanel onChanged={noopChanged} />;
  if (node.kind === 'editor') return <EditorPanel onChanged={noopChanged} />;
  if (node.kind === 'agent') return <AgentPanel onChanged={noopChanged} />;
  return null;
}

function flowNodes(
  canvas: Canvas,
  liveNodeId: string | null,
  onToggleLive: (id: string) => void,
): Node[] {
  return canvas.nodes.map(node => {
    const supportsLive = node.kind === 'terminal' || node.kind === 'editor' || node.kind === 'agent';
    const live = supportsLive && liveNodeId === node.id;
    return {
      id: node.id,
      position: { x: node.x, y: node.y },
      data: {
        label: (
          <div className={`spatial-node ${live ? 'spatial-node--live' : ''}`}>
            <NodeCard node={node} />
            {supportsLive && (
              <button className="live-node-toggle nodrag nopan" onClick={() => onToggleLive(node.id)}>
                {live ? 'Close live surface' : 'Open live surface'}
              </button>
            )}
            {live && <div className="live-node-surface nodrag nopan nowheel"><SpatialLiveSurface node={node} /></div>}
          </div>
        ),
      },
      style: {
        width: live ? Math.max(node.width, 760) : node.width,
        minHeight: live ? 640 : Math.min(node.height, 360),
      },
      draggable: !node.pinned,
      selectable: true,
    };
  });
}
function TerminalPanel({ onChanged }: { onChanged: () => Promise<void> }) {
  const host = useRef<HTMLDivElement>(null);
  const terminal = useRef<XTerm | null>(null);
  const fit = useRef<FitAddon | null>(null);
  const terminalId = useRef<string | null>(null);
  const pending = useRef(new Map<string, string[]>());
  const [needsApproval, setNeedsApproval] = useState(false);
  const [running, setRunning] = useState(false);
  const [message, setMessage] = useState('No terminal running.');

  useEffect(() => {
    if (!host.current) return;
    const instance = new XTerm({
      fontFamily: 'Cascadia Mono, Consolas, monospace',
      fontSize: 13,
      cursorBlink: true,
      convertEol: true,
      theme: { background: '#090d16' },
    });
    const fitAddon = new FitAddon();
    instance.loadAddon(fitAddon);
    instance.open(host.current);
    fitAddon.fit();
    terminal.current = instance;
    fit.current = fitAddon;
    const dataDisposable = instance.onData(data => {
      if (terminalId.current) void window.astra.writeTerminal(terminalId.current, data);
    });
    const offData = window.astra.onTerminalData(event => {
      if (event.id === terminalId.current) terminal.current?.write(event.data);
      else pending.current.set(event.id, [...(pending.current.get(event.id) ?? []), event.data]);
    });
    const offExit = window.astra.onTerminalExit(event => {
      if (event.id === terminalId.current) {
        terminal.current?.writeln(`\r\n[process exited ${event.exitCode}]`);
        terminalId.current = null;
        setRunning(false);
        setMessage('Terminal ended.');
        void onChanged();
      }
    });
    const observer = new ResizeObserver(() => {
      fit.current?.fit();
      if (terminalId.current && terminal.current) {
        void window.astra.resizeTerminal(terminalId.current, terminal.current.cols, terminal.current.rows);
      }
    });
    observer.observe(host.current);
    return () => {
      observer.disconnect();
      offData();
      offExit();
      dataDisposable.dispose();
      instance.dispose();
    };
  }, [onChanged]);

  const start = async () => {
    if (!terminal.current) return;
    const result = await window.astra.createTerminal(terminal.current.cols, terminal.current.rows) as {
      id?: string; needsApproval: boolean; reason?: string; shell?: string;
    };
    if (result.needsApproval || !result.id) {
      setNeedsApproval(true);
      setMessage(result.reason ?? 'Approval required.');
      return;
    }
    terminalId.current = result.id;
    for (const chunk of pending.current.get(result.id) ?? []) terminal.current.write(chunk);
    pending.current.delete(result.id);
    setNeedsApproval(false);
    setRunning(true);
    setMessage(`Running ${result.shell ?? 'shell'}`);
    await onChanged();
  };

  const grant = async () => {
    await window.astra.grantTerminal();
    setNeedsApproval(false);
    await onChanged();
    await start();
  };
  const stop = async () => {
    if (!terminalId.current) return;
    await window.astra.killTerminal(terminalId.current);
  };

  return (
    <section className="tool-panel">
      <div className="tool-panel__header">
        <div><h2>Terminal</h2><small>{message}</small></div>
        <div className="button-row">
          {!running && <button onClick={() => void start()}>Start terminal</button>}
          {needsApproval && <button className="accent" onClick={() => void grant()}>Allow this session</button>}
          {running && <button className="danger" onClick={() => void stop()}>Stop</button>}
        </div>
      </div>
      <div ref={host} className="terminal-host" />
    </section>
  );
}

// Modified: 2026-09-22-resume-native; guard lazy editor readiness before file I/O.
function EditorPanel({ onChanged }: { onChanged: () => Promise<void> }) {
  const host = useRef<HTMLDivElement>(null);
  const editor = useRef<MonacoEditor.IStandaloneCodeEditor | null>(null);
  const [editorReady, setEditorReady] = useState(false);
  const [file, setFile] = useState('README.md');
  const [directory, setDirectory] = useState('.');
  const [entries, setEntries] = useState<DirectoryEntry[]>([]);
  const [version, setVersion] = useState<string | undefined>();
  const [message, setMessage] = useState('Browse or enter a workspace-relative file path.');
  useEffect(() => {
    if (!host.current) return;
    let disposed = false;
    let instance: MonacoEditor.IStandaloneCodeEditor | null = null;
    setMessage('Loading Monaco editor engine…');
    void import('monaco-editor').then(monaco => {
      if (disposed || !host.current) return;
      instance = monaco.editor.create(host.current, {
        value: '',
        language: 'plaintext',
        theme: 'vs-dark',
        automaticLayout: true,
        minimap: { enabled: false },
        fontSize: 13,
        tabSize: 2,
      });
      editor.current = instance;
      setEditorReady(true);
      setMessage('Browse or enter a workspace-relative file path.');
    }).catch(error => {
      setMessage(`Editor failed to load: ${error instanceof Error ? error.message : String(error)}`);
    });
    return () => {
      disposed = true;
      instance?.dispose();
      editor.current = null;
    };
  }, []);

  const loadDirectory = useCallback(async (target = directory) => {
    try {
      const result = await window.astra.listDirectory(target) as { path: string; entries: DirectoryEntry[] };
      setDirectory(result.path);
      setEntries(result.entries);
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  }, [directory]);

  useEffect(() => {
    void loadDirectory('.');
  }, []);

  const load = async (target = file) => {
    try {
      if (!editor.current) throw new Error("Editor is not ready yet");
      const result = await window.astra.readFile(target) as { content: string; size: number; version: string };
      setFile(target);
      setVersion(result.version);
      editor.current?.setValue(result.content);
      setMessage(`Loaded ${result.size} bytes.`);
      await onChanged();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  };

  const save = async () => {
    try {
      if (!editor.current || !version) throw new Error("Load the file before saving");
      const result = await window.astra.writeFile(file, editor.current.getValue(), version) as { version: string };
      setVersion(result.version);
      setMessage('Saved atomically inside the trusted workspace.');
      await onChanged();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  };

  const parentDirectory = directory === '.' ? '.' : (directory.replace(/\\/g, '/').split('/').slice(0, -1).join('/') || '.');

  return (
    <section className="tool-panel">
      <div className="tool-panel__header">
        <div><h2>Editor</h2><small>{message}</small></div>
        <div className="file-controls">
          <input value={file} onChange={event => setFile(event.target.value)} aria-label="Workspace relative file" />
          <button disabled={!editorReady} onClick={() => void load()}>Load</button>
          <button className="accent" disabled={!editorReady || !version} onClick={() => void save()}>Save</button>
        </div>
      </div>
      <div className="editor-body">
        <aside className="file-browser">
          <div className="file-browser__header">
            <button disabled={directory === '.'} onClick={() => void loadDirectory(parentDirectory)}>↑</button>
            <code title={directory}>{directory}</code>
            <button onClick={() => void loadDirectory(directory)}>↻</button>
          </div>
          <div className="file-browser__list">
            {entries.map(entry => (
              <button key={entry.path} className={`file-entry file-entry--${entry.kind}`}
                disabled={entry.kind === 'other' || (entry.kind === 'file' && !editorReady)}
                onClick={() => entry.kind === 'directory' ? void loadDirectory(entry.path) : void load(entry.path)}>
                <span>{entry.kind === 'directory' ? '▸' : '·'}</span><span>{entry.name}</span>
              </button>
            ))}
          </div>
        </aside>
        <div ref={host} className="editor-host" />
      </div>
    </section>
  );
}

function AuditPanel({ events }: { events: AuditEvent[] }) {
  return (
    <section className="tool-panel audit-panel">
      <div className="tool-panel__header"><div><h2>Audit trail</h2><small>Durable local SQLite events.</small></div></div>
      <div className="audit-list">
        {events.length === 0 && <p className="muted">No tool events yet.</p>}
        {events.map(event => (
          <article key={event.id} className="audit-event">
            <div><span className={`decision decision--${event.decision.toLowerCase()}`}>{event.decision}</span> <strong>{event.tool}</strong></div>
            <code>{event.target}</code>
            <span>{event.result}</span>
            <small>{new Date(event.timestamp).toLocaleTimeString()} · {event.durationMs} ms</small>
          </article>
        ))}
      </div>
    </section>
  );
}

const taskStatuses: TaskStatus[] = ['READY', 'RUNNING', 'WAITING', 'BLOCKED', 'FAILED', 'DONE'];

function TaskPanel({ onChanged }: { onChanged: () => Promise<void> }) {
  const [tasks, setTasks] = useState<Task[]>([]);
  const [evidence, setEvidence] = useState<ValidationEvidence[]>([]);
  const [selectedId, setSelectedId] = useState('');
  const [title, setTitle] = useState('');
  const [objective, setObjective] = useState('');
  const [checklistText, setChecklistText] = useState('');
  const [blockersText, setBlockersText] = useState('');
  const [criteriaText, setCriteriaText] = useState('');
  const [evidenceKind, setEvidenceKind] = useState<EvidenceKind>('test');
  const [evidenceStatus, setEvidenceStatus] = useState<ValidationEvidence['status']>('INFO');
  const [evidenceLabel, setEvidenceLabel] = useState('');
  const [evidenceSummary, setEvidenceSummary] = useState('');
  const [evidenceReference, setEvidenceReference] = useState('');
  const [message, setMessage] = useState('Durable tasks, criteria and validation evidence.');

  const load = useCallback(async () => {
    const [nextTasks, nextEvidence] = await Promise.all([
      window.astra.listTasks() as Promise<Task[]>,
      window.astra.listEvidence() as Promise<ValidationEvidence[]>,
    ]);
    setTasks(nextTasks);
    setEvidence(nextEvidence);
    setSelectedId(current => nextTasks.some(task => task.id === current) ? current : (nextTasks[0]?.id ?? ''));
  }, []);

  useEffect(() => {
    void load().catch(error => setMessage(error instanceof Error ? error.message : String(error)));
  }, [load]);

  const selected = tasks.find(task => task.id === selectedId);
  useEffect(() => {
    if (!selected) return;
    setBlockersText(selected.blockers.join('\n'));
    setCriteriaText(selected.acceptanceCriteria.join('\n'));
  }, [selected?.id, selected?.updatedAt]);

  const create = async () => {
    try {
      if (!title.trim() || !objective.trim()) return;
      const task = await window.astra.createTask(title, objective) as Task;
      setTitle('');
      setObjective('');
      setSelectedId(task.id);
      setMessage('Task created.');
      await load();
      await onChanged();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  };

  const updateStatus = async (id: string, status: TaskStatus) => {
    await window.astra.updateTaskStatus(id, status);
    await load();
    await onChanged();
  };

  const updateDetails = async (patch: unknown) => {
    if (!selected) return;
    try {
      await window.astra.updateTaskDetails(selected.id, patch);
      setMessage('Task details saved.');
      await load();
      await onChanged();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  };

  const addChecklist = async () => {
    if (!selected || !checklistText.trim()) return;
    await updateDetails({
      checklist: [...selected.checklist, { id: crypto.randomUUID(), label: checklistText.trim(), done: false }],
    });
    setChecklistText('');
  };

  const toggleChecklist = async (itemId: string, done: boolean) => {
    if (!selected) return;
    await updateDetails({
      checklist: selected.checklist.map(item => item.id === itemId ? { ...item, done } : item),
    });
  };

  const saveLists = async () => {
    const lines = (value: string) => value.split(/\r?\n/).map(line => line.trim()).filter(Boolean);
    await updateDetails({ blockers: lines(blockersText), acceptanceCriteria: lines(criteriaText) });
  };

  const addEvidence = async () => {
    if (!selected || !evidenceLabel.trim()) return;
    try {
      await window.astra.createEvidence({
        taskId: selected.id,
        kind: evidenceKind,
        status: evidenceStatus,
        label: evidenceLabel.trim(),
        summary: evidenceSummary.trim(),
        reference: evidenceReference.trim(),
      });
      setEvidenceLabel('');
      setEvidenceSummary('');
      setEvidenceReference('');
      setMessage('Validation evidence recorded.');
      await load();
      await onChanged();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  };

  const selectedEvidence = evidence.filter(item => item.taskId === selected?.id);

  return (
    <section className="tool-panel task-panel">
      <div className="tool-panel__header">
        <div><h2>Tasks & Evidence</h2><small>{message}</small></div>
        <button onClick={() => void load()}>Refresh</button>
      </div>
      <div className="task-workbench">
        <div className="task-create">
          <input aria-label="Task title" placeholder="Task title" value={title} onChange={event => setTitle(event.target.value)} />
          <textarea aria-label="Task objective" placeholder="Objective / definition of done" value={objective} onChange={event => setObjective(event.target.value)} />
          <button className="accent" disabled={!title.trim() || !objective.trim()} onClick={() => void create()}>Create task</button>
        </div>
        <div className="task-list">
          {tasks.length === 0 && <p className="muted">No durable tasks in this workspace yet.</p>}
          {tasks.map(task => (
            <article className={`task-card ${selectedId === task.id ? 'selected' : ''}`} key={task.id}
              onClick={() => setSelectedId(task.id)}>
              <div><strong>{task.title}</strong><span className={`status status--${task.status.toLowerCase()}`}>{task.status}</span></div>
              <p>{task.objective}</p>
              <select aria-label={`Status for ${task.title}`} value={task.status}
                onClick={event => event.stopPropagation()}
                onChange={event => void updateStatus(task.id, event.target.value as TaskStatus)}>
                {taskStatuses.map(status => <option value={status} key={status}>{status}</option>)}
              </select>
            </article>
          ))}
        </div>
        <div className="task-detail">
          {!selected && <p className="muted">Select a task to edit its agency state.</p>}
          {selected && <>
            <h3>{selected.title}</h3>
            <small>Owner: {selected.ownerSessionId ?? 'unassigned'} · Evidence: {selected.evidenceIds.length}</small>
            <section><strong>Checklist</strong>
              {selected.checklist.map(item => (
                <label className="check-row" key={item.id}>
                  <input type="checkbox" checked={item.done} onChange={event => void toggleChecklist(item.id, event.target.checked)} />
                  <span>{item.label}</span>
                </label>
              ))}
              <div className="inline-create"><input aria-label="Checklist item" value={checklistText}
                onChange={event => setChecklistText(event.target.value)} placeholder="Add checklist item…" />
                <button onClick={() => void addChecklist()}>Add</button></div>
            </section>
            <section className="task-text-lists">
              <label>Blockers<textarea value={blockersText} onChange={event => setBlockersText(event.target.value)} placeholder="One blocker per line" /></label>
              <label>Acceptance criteria<textarea value={criteriaText} onChange={event => setCriteriaText(event.target.value)} placeholder="One criterion per line" /></label>
              <button onClick={() => void saveLists()}>Save task details</button>
            </section>
            <section className="evidence-list"><strong>Validation evidence</strong>
              {selectedEvidence.map(item => (
                <article key={item.id} className={`evidence-card evidence-card--${item.status.toLowerCase()}`}>
                  <div><span>{item.status}</span><strong>{item.label}</strong><code>{item.kind}</code></div>
                  <p>{item.summary || '(no summary)'}</p>
                  {item.reference && <small>{item.reference}</small>}
                </article>
              ))}
              <div className="evidence-create">
                <select value={evidenceKind} onChange={event => setEvidenceKind(event.target.value as EvidenceKind)}>
                  {['command','test','screenshot','artifact','file-hash','process-health','browser-assertion'].map(kind => <option key={kind}>{kind}</option>)}
                </select>
                <select value={evidenceStatus} onChange={event => setEvidenceStatus(event.target.value as ValidationEvidence['status'])}>
                  <option>INFO</option><option>PASS</option><option>FAIL</option>
                </select>
                <input aria-label="Evidence label" value={evidenceLabel} onChange={event => setEvidenceLabel(event.target.value)} placeholder="Evidence label" />
                <textarea aria-label="Evidence summary" value={evidenceSummary} onChange={event => setEvidenceSummary(event.target.value)} placeholder="Summary" />
                <input aria-label="Evidence reference" value={evidenceReference} onChange={event => setEvidenceReference(event.target.value)} placeholder="Artifact/path/thread reference" />
                <button disabled={!evidenceLabel.trim()} onClick={() => void addEvidence()}>Record evidence</button>
              </div>
            </section>
          </>}
        </div>
      </div>
    </section>
  );
}

function AgentPanel({ onChanged }: { onChanged: () => Promise<void> }) {
  const [providers, setProviders] = useState<AgentStatus[]>([]);
  const [providerId, setProviderId] = useState('codex-cli');
  const [sessions, setSessions] = useState<AgentSession[]>([]);
  const [tasks, setTasks] = useState<Task[]>([]);
  const [taskId, setTaskId] = useState('');
  const [prompt, setPrompt] = useState('Summarize the current workspace and identify the highest-priority next engineering task. Do not modify files.');
  const [output, setOutput] = useState('');
  const [message, setMessage] = useState('Checking Codex CLI…');
  const [needsApproval, setNeedsApproval] = useState(false);
  const [pendingResume, setPendingResume] = useState<{ id: string; taskId?: string; providerId: string } | null>(null);
  const [runningId, setRunningId] = useState<string | null>(null);

  const refreshProvider = useCallback(async () => {
    const [nextProviders, history, taskList] = await Promise.all([
      window.astra.agentProviders() as Promise<AgentStatus[]>,
      window.astra.listAgentSessions() as Promise<AgentSession[]>,
      window.astra.listTasks() as Promise<Task[]>,
    ]);
    setProviders(nextProviders);
    setSessions(history);
    setTasks(taskList);
    setProviderId(current => nextProviders.some(provider => provider.id === current)
      ? current
      : (nextProviders[0]?.id ?? 'codex-cli'));
  }, []);

  const provider = providers.find(candidate => candidate.id === providerId) ?? null;
  useEffect(() => {
    if (!provider) return;
    const state = !provider.available
      ? (provider.reason ?? `${provider.name ?? provider.id} unavailable`)
      : provider.authenticated === false
        ? (provider.reason ?? 'Authentication required')
        : `${provider.version ?? provider.name ?? provider.id} · ${provider.sandbox ?? 'local provider'}`;
    setMessage(state);
  }, [provider?.id, provider?.available, provider?.authenticated, provider?.version, provider?.reason, provider?.sandbox]);

  useEffect(() => {
    void refreshProvider();
    return window.astra.onAgentEvent((event: AgentEvent) => {
      if (event.type === 'message' && event.text) setOutput(current => (current + `\n${event.text}\n`).slice(-100_000));
      else if (event.type === 'stderr' && event.data) setOutput(current => (current + `\n[stderr] ${event.data}`).slice(-100_000));
      else if (event.type === 'thread' && event.threadId) setMessage(`Provider session ${event.threadId}`);
      else if (event.type === 'error' && event.message) setMessage(event.message);
      else if (event.type === 'exit') {
        setRunningId(null);
        setMessage(event.exitCode === 0 ? 'Agent run completed.' : `Agent exited with code ${event.exitCode ?? 'unknown'}.`);
        void refreshProvider();
        void onChanged();
      }
    });
  }, [onChanged, refreshProvider]);

  const run = async (resume?: { id: string; taskId?: string; providerId: string }) => {
    setOutput('');
    const linkedTask = resume?.taskId ?? (taskId || undefined);
    const runProviderId = resume?.providerId ?? providerId;
    const result = await window.astra.runProviderAgent(
      runProviderId,
      prompt,
      linkedTask,
      resume?.id,
    ) as {
      id?: string; needsApproval?: boolean; denied?: boolean; reason?: string; resumed?: boolean;
    };
    if (!result.id) {
      setNeedsApproval(Boolean(result.needsApproval));
      setPendingResume(resume ?? null);
      setMessage(result.reason ?? 'Agent start denied.');
      return;
    }
    setNeedsApproval(false);
    setPendingResume(null);
    setRunningId(result.id);
    const runningProvider = providers.find(candidate => candidate.id === runProviderId);
    setMessage(result.resumed ? `${runningProvider?.name ?? runProviderId} session resumed…` : `${runningProvider?.name ?? runProviderId} is running…`);
    await onChanged();
  };

  const grant = async () => {
    const result = await window.astra.grantAgent() as { granted: boolean; reason?: string };
    if (!result.granted) {
      setMessage(result.reason ?? 'Agent approval was denied.');
      return;
    }
    await refreshProvider();
    setNeedsApproval(false);
    await run(pendingResume ?? undefined);
  };

  return (
    <section className="tool-panel agent-panel">
      <div className="tool-panel__header">
        <div><h2>Agent</h2><small>{message}</small></div>
        <div className="button-row">
          {!runningId && <button className="accent" disabled={!provider?.available || provider.authenticated === false || !prompt.trim()} onClick={() => void run()}>Run {provider?.name ?? providerId}</button>}
          {needsApproval && <button onClick={() => void grant()}>Allow agent this session</button>}
          {runningId && <button className="danger" onClick={() => void window.astra.stopAgent(runningId)}>Stop</button>}
        </div>
      </div>
      <div className="agent-body">
        <label className="agent-task-link">Provider
          <select aria-label="Agent provider" value={providerId} onChange={event => setProviderId(event.target.value)}>
            {providers.map(candidate => (
              <option key={candidate.id} value={candidate.id}>
                {candidate.name ?? candidate.id}{candidate.authenticated === false ? ' · login required' : ''}
              </option>
            ))}
          </select>
        </label>
        <label className="agent-task-link">Task
          <select aria-label="Agent task" value={taskId} onChange={event => setTaskId(event.target.value)}>
            <option value="">Unbound session</option>
            {tasks.map(task => <option key={task.id} value={task.id}>{task.status} · {task.title}</option>)}
          </select>
        </label>
        <textarea value={prompt} onChange={event => setPrompt(event.target.value)} aria-label="Agent prompt" />
        <pre>{output || 'Agent output will stream here. Prompts and model output are not written to the audit database.'}</pre>
        <div className="agent-sessions">
          <strong>Recent sessions</strong>
          {sessions.slice(0, 6).map(session => (
            <div key={session.id}>
              <span>{session.status}</span>
              <code title={session.resumabilityData?.threadId ?? session.id}>{session.resumabilityData?.threadId ?? session.id}</code>
              <small>{session.taskId ? (tasks.find(task => task.id === session.taskId)?.title ?? 'Linked task') : 'Unbound'}</small>
              {session.resumabilityData?.threadId && ['DONE', 'STOPPED', 'FAILED'].includes(session.status)
                ? <button disabled={Boolean(runningId)} onClick={() => {
                    setTaskId(session.taskId ?? '');
                    setProviderId(session.providerId);
                    void run({ id: session.id, taskId: session.taskId, providerId: session.providerId });
                  }}>Resume</button>
                : <span />}
            </div>
          ))}
          {sessions.length === 0 && <small>No persisted Codex sessions yet.</small>}
        </div>
      </div>
    </section>
  );
}

function GitPanel({ onChanged, onPinDiff }: {
  onChanged: () => Promise<void>;
  onPinDiff: (diff: GitDiff) => Promise<void>;
}) {
  const [snapshot, setSnapshot] = useState<GitSnapshot | null>(null);
  const [selected, setSelected] = useState('');
  const [diff, setDiff] = useState<GitDiff | null>(null);
  const [stagedDiff, setStagedDiff] = useState(false);
  const [worktreeName, setWorktreeName] = useState('feature');
  const [worktreeApproval, setWorktreeApproval] = useState(false);
  const [message, setMessage] = useState('Reading repository state…');

  const load = useCallback(async () => {
    const next = await window.astra.gitSnapshot() as GitSnapshot;
    setSnapshot(next);
    setSelected(current => current && next.changedFiles.some(file => file.path === current)
      ? current
      : (next.changedFiles[0]?.path ?? ''));
    setMessage(next.isRepository ? 'Git inspection with explicit-path mutations.' : 'Workspace is not a readable Git repository.');
    await onChanged();
  }, [onChanged]);

  const loadDiff = useCallback(async (filePath = selected, staged = stagedDiff) => {
    if (!filePath) return;
    try {
      const next = await window.astra.gitDiff(filePath, staged) as GitDiff;
      setDiff(next);
      setStagedDiff(staged);
      setMessage(next.patch ? `Loaded ${staged ? 'staged' : 'working-tree'} diff for ${filePath}.` : 'No patch in this diff view.');
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  }, [selected, stagedDiff]);

  useEffect(() => {
    void load().catch(error => setMessage(error instanceof Error ? error.message : String(error)));
  }, [load]);

  useEffect(() => {
    if (selected) void loadDiff(selected, stagedDiff);
  }, [selected, stagedDiff]);

  const mutate = async (action: 'stage' | 'unstage') => {
    if (!selected) return;
    try {
      if (action === 'stage') await window.astra.gitStage(selected);
      else await window.astra.gitUnstage(selected);
      setMessage(`${action === 'stage' ? 'Staged' : 'Unstaged'} ${selected}.`);
      await load();
      await loadDiff(selected, action === 'stage');
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  };

  const createWorktree = async () => {
    try {
      const result = await window.astra.createGitWorktree(worktreeName) as {
        needsApproval?: boolean; denied?: boolean; reason?: string; branch?: string; path?: string;
      };
      if (result.needsApproval) {
        setWorktreeApproval(true);
        setMessage(result.reason ?? 'Worktree approval required.');
        return;
      }
      if (result.denied) throw new Error(result.reason ?? 'Worktree creation denied.');
      setWorktreeApproval(false);
      setMessage(`Created ${result.branch} at ${result.path}.`);
      await load();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  };

  const grantWorktree = async () => {
    const result = await window.astra.grantGitWorktree() as { granted: boolean; reason?: string };
    if (!result.granted) {
      setMessage(result.reason ?? 'Worktree grant denied.');
      return;
    }
    setWorktreeApproval(false);
    await createWorktree();
  };

  return (
    <section className="tool-panel git-panel">
      <div className="tool-panel__header">
        <div><h2>Git / Diff</h2><small>{message}</small></div>
        <button onClick={() => void load()}>Refresh</button>
      </div>
      <div className="git-workbench">
        <aside className="git-changes">
          <strong>Changes</strong>
          {snapshot?.changedFiles.map(file => (
            <button key={file.path} className={selected === file.path ? 'selected' : ''}
              onClick={() => setSelected(file.path)}>
              <code>{file.index}{file.workingTree}</code><span>{file.path}</span>
            </button>
          ))}
          {snapshot?.changedFiles.length === 0 && <small>Working tree clean.</small>}
        </aside>
        <div className="diff-view">
          <div className="diff-toolbar">
            <button className={!stagedDiff ? 'active' : ''} onClick={() => setStagedDiff(false)}>Working tree</button>
            <button className={stagedDiff ? 'active' : ''} onClick={() => setStagedDiff(true)}>Staged</button>
            <button disabled={!selected} onClick={() => void mutate('stage')}>Stage file</button>
            <button disabled={!selected} onClick={() => void mutate('unstage')}>Unstage file</button>
            <button disabled={!diff} onClick={() => diff && void onPinDiff(diff)}>Pin Diff node</button>
          </div>
          <pre>{diff?.patch || (selected ? '(no patch in this view)' : '(select a changed file)')}</pre>
        </div>
        <div className="git-lower">
          <article><h3>Status</h3><pre>{snapshot?.status || '(clean or unavailable)'}</pre></article>
          <article><h3>Worktrees</h3><pre>{snapshot?.worktrees || '(none)'}</pre></article>
          <article className="worktree-create">
            <h3>Create managed worktree</h3>
            <input aria-label="Worktree name" value={worktreeName} onChange={event => setWorktreeName(event.target.value)} />
            <button onClick={() => void createWorktree()}>Create worktree</button>
            {worktreeApproval && <button className="accent" onClick={() => void grantWorktree()}>Allow worktrees this session</button>}
          </article>
        </div>
      </div>
    </section>
  );
}

function ProcessPanel({ onChanged, onPinProcess }: {
  onChanged: () => Promise<void>;
  onPinProcess: (process: ProcessInfo) => Promise<void>;
}) {
  const [processes, setProcesses] = useState<ProcessInfo[]>([]);
  const [query, setQuery] = useState('');
  const [pendingKill, setPendingKill] = useState<number | null>(null);
  const [message, setMessage] = useState('Read-only process inventory.');

  const load = useCallback(async () => {
    const next = await window.astra.listProcesses() as ProcessInfo[];
    setProcesses(next);
    setMessage(`Showing ${next.length} processes; only MTerm-owned children can be stopped.`);
    await onChanged();
  }, [onChanged]);

  useEffect(() => {
    void load().catch(error => setMessage(error instanceof Error ? error.message : String(error)));
  }, [load]);

  const stop = async (pid: number) => {
    const result = await window.astra.killOwnedProcess(pid) as {
      stopped?: boolean; needsApproval?: boolean; denied?: boolean; reason?: string;
    };
    if (result.needsApproval) {
      setPendingKill(pid);
      setMessage(result.reason ?? 'Process-stop approval required.');
      return;
    }
    setPendingKill(null);
    setMessage(result.stopped ? `Stopped owned process ${pid}.` : (result.reason ?? 'Process was not stopped.'));
    await load();
  };

  const grant = async () => {
    const result = await window.astra.grantProcessKill() as { granted: boolean; reason?: string };
    if (!result.granted) {
      setMessage(result.reason ?? 'Process-stop permission denied.');
      return;
    }
    const pid = pendingKill;
    setPendingKill(null);
    if (pid) await stop(pid);
  };

  const visible = processes.filter(process =>
    !query.trim() || `${process.pid} ${process.name} ${process.owner ?? ''}`.toLowerCase().includes(query.toLowerCase()),
  );

  return (
    <section className="tool-panel process-panel">
      <div className="tool-panel__header">
        <div><h2>Processes</h2><small>{message}</small></div>
        <div className="button-row">
          <input aria-label="Process filter" placeholder="Filter PID/name…" value={query} onChange={event => setQuery(event.target.value)} />
          <button onClick={() => void load()}>Refresh</button>
          {pendingKill && <button className="accent" onClick={() => void grant()}>Allow owned stop</button>}
        </div>
      </div>
      <div className="process-table">
        <div className="process-row process-row--header"><span>PID</span><span>Process</span><span>CPU s</span><span>Memory</span><span>Owner</span><span /></div>
        {visible.map(process => (
          <div className="process-row" key={process.pid}>
            <code>{process.pid}</code>
            <strong>{process.name}</strong>
            <span>{process.cpu == null ? '—' : process.cpu.toFixed(1)}</span>
            <span>{(process.memoryBytes / 1048576).toFixed(1)} MB</span>
            <span>{process.owner ?? 'system/user'}</span>
            <div className="button-row">
              <button onClick={() => void onPinProcess(process)}>Pin</button>
              {process.owned && <button className="danger" onClick={() => void stop(process.pid)}>Stop</button>}
            </div>
          </div>
        ))}
      </div>
    </section>
  );
}

function ProjectMode({ canvas }: { canvas: Canvas }) {
  return (
    <div className="project-mode">
      <div className="project-summary">
        <h2>Project view</h2>
        <p>{canvas.nodes.length} resources · {canvas.edges.length} relationships</p>
      </div>
      <div className="project-grid">
        {canvas.nodes.map(node => <NodeCard key={node.id} node={node} />)}
      </div>
    </div>
  );
}
function App() {
  const [state, setState] = useState<AppState | null>(null);
  const [nodes, setNodes] = useState<Node[]>([]);
  const [liveNodeId, setLiveNodeId] = useState<string | null>(null);
  const [tool, setTool] = useState<'tasks' | 'agent' | 'terminal' | 'editor' | 'git' | 'processes' | 'audit'>('terminal');
  const [paletteOpen, setPaletteOpen] = useState(false);
  const [paletteQuery, setPaletteQuery] = useState('');
  const [error, setError] = useState<string | null>(null);

  const toggleLiveNode = useCallback((id: string) => {
    setLiveNodeId(current => current === id ? null : id);
  }, []);

  const refresh = useCallback(async () => {
    const next = await window.astra.state() as AppState;
    setState(next);
  }, []);

  useEffect(() => {
    void refresh().catch(reason => setError(String(reason)));
  }, [refresh]);

  useEffect(() => {
    if (!state) return;
    setNodes(flowNodes(state.canvas, liveNodeId, toggleLiveNode));
  }, [state?.canvas, liveNodeId, toggleLiveNode]);

  const persistCanvas = useCallback(async (next: Canvas) => {
    setState(previous => previous ? { ...previous, canvas: next } : previous);
    await window.astra.saveCanvas(next);
  }, []);

  const onNodesChange = useCallback((changes: NodeChange[]) => {
    setNodes(current => applyNodeChanges(changes, current));
  }, []);

  const onNodeDragStop: OnNodeDragStop = useCallback((_event, flowNode) => {
    if (!state) return;
    const next: Canvas = {
      ...state.canvas,
      nodes: state.canvas.nodes.map(node => node.id === flowNode.id
        ? { ...node, x: flowNode.position.x, y: flowNode.position.y }
        : node),
    };
    void persistCanvas(next).then(refresh).catch(reason => setError(String(reason)));
  }, [persistCanvas, refresh, state]);

  const addNode = async (kind: CanvasNode['kind']) => {
    if (!state) return;
    const id = `${kind}-${Date.now().toString(36)}`;
    const offset = state.canvas.nodes.length * 36;
    const node: CanvasNode = {
      id, kind, title: kind === 'note' ? 'New note' : kind === 'terminal' ? 'Terminal' : kind === 'agent' ? 'Agent' : 'Editor',
      x: 160 + offset, y: 160 + offset, width: 360, height: 280,
      content: kind === 'terminal' ? 'Live shell is available in the Terminal inspector.' :
        kind === 'editor' ? 'Workspace file editing is available in the Editor inspector.' :
        kind === 'agent' ? 'Provider-selectable local agent execution is available here or in the Agent inspector.' : 'Write a durable workspace note here.',
      response: '', status: 'draft', pinned: false, collapsed: false, items: [],
    };
    await persistCanvas({ ...state.canvas, nodes: [...state.canvas.nodes, node] });
  };

  const pinDiff = async (diff: GitDiff) => {
    if (!state) return;
    const offset = state.canvas.nodes.length * 32;
    const node: CanvasNode = {
      id: `diff-${Date.now().toString(36)}`,
      kind: 'diff',
      title: `${diff.staged ? 'Staged' : 'Working'} diff · ${diff.path}`,
      x: 180 + offset,
      y: 140 + offset,
      width: 560,
      height: 440,
      content: diff.path,
      response: diff.patch,
      status: 'active',
      pinned: false,
      collapsed: false,
      items: [],
    };
    await persistCanvas({ ...state.canvas, nodes: [...state.canvas.nodes, node] });
  };

  const pinProcess = async (process: ProcessInfo) => {
    if (!state) return;
    const offset = state.canvas.nodes.length * 30;
    const node: CanvasNode = {
      id: `process-${process.pid}-${Date.now().toString(36)}`,
      kind: 'process',
      title: `${process.name} · PID ${process.pid}`,
      x: 200 + offset,
      y: 160 + offset,
      width: 420,
      height: 300,
      content: process.owner ? `MTerm-owned ${process.owner}` : 'Observed OS process',
      response: `PID: ${process.pid}\nCPU seconds: ${process.cpu ?? 'unknown'}\nMemory: ${(process.memoryBytes / 1048576).toFixed(1)} MB\nOwner: ${process.owner ?? 'system/user'}`,
      status: process.owned ? 'active' : 'draft',
      pinned: false,
      collapsed: false,
      items: [],
    };
    await persistCanvas({ ...state.canvas, nodes: [...state.canvas.nodes, node] });
  };

  const chooseWorkspace = async () => {
    await window.astra.chooseWorkspace();
    await refresh();
  };
  const toggleProfile = async () => {
    if (!state) return;
    await window.astra.setProfile(state.workspace.profile === 'observe' ? 'developer' : 'observe');
    await refresh();
  };
  const toggleView = async (view: Canvas['view']) => {
    if (!state || state.canvas.view === view) return;
    await persistCanvas({ ...state.canvas, view });
  };

  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      const open = (event.ctrlKey || event.metaKey) && (event.key.toLowerCase() === 'k' || (event.shiftKey && event.key.toLowerCase() === 'p'));
      if (open) {
        event.preventDefault();
        setPaletteOpen(value => !value);
        setPaletteQuery('');
      } else if (event.key === 'Escape') setPaletteOpen(false);
    };
    window.addEventListener('keydown', onKeyDown);
    return () => window.removeEventListener('keydown', onKeyDown);
  }, []);

  if (!state) return <div className="loading">Launching MTerm… {error && <pre>{error}</pre>}</div>;

  const paletteCommands: Array<{ label: string; detail: string; run: () => void | Promise<void> }> = [
    { label: 'New Note', detail: 'Create a persistent note node', run: () => addNode('note') },
    { label: 'New Agent', detail: 'Create an agent node and open Agent', run: async () => { await addNode('agent'); setTool('agent'); } },
    { label: 'New Terminal', detail: 'Create a terminal node and open Terminal', run: async () => { await addNode('terminal'); setTool('terminal'); } },
    { label: 'New Editor', detail: 'Create an editor node and open Editor', run: async () => { await addNode('editor'); setTool('editor'); } },
    { label: 'Show Tasks', detail: 'Open durable task inspector', run: () => setTool('tasks') },
    { label: 'Show Agent', detail: 'Open provider/session inspector', run: () => setTool('agent') },
    { label: 'Show Git', detail: 'Open Git / Diff workbench', run: () => setTool('git') },
    { label: 'Show Processes', detail: 'Open OS process inspector', run: () => setTool('processes') },
    { label: 'Show Audit', detail: 'Open durable tool-call audit', run: async () => { setTool('audit'); await refresh(); } },
    { label: 'Canvas Mode', detail: 'Switch to spatial workspace', run: () => toggleView('canvas') },
    { label: 'Project Mode', detail: 'Switch to traditional project view', run: () => toggleView('project') },
    { label: 'Open Workspace', detail: 'Choose a trusted workspace root', run: chooseWorkspace },
  ];
  const paletteMatches = paletteCommands.filter(command => `${command.label} ${command.detail}`.toLowerCase().includes(paletteQuery.trim().toLowerCase()));
  const runPaletteCommand = async (command: (typeof paletteCommands)[number]) => {
    setPaletteOpen(false);
    setPaletteQuery('');
    await command.run();
  };

  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand"><span className="brand-mark">A</span><div><strong>MTerm</strong><small>A spatial AI command center</small></div></div>
        <div className="workspace-pill"><span className="pulse" /> <strong>{state.workspace.name}</strong><code>{state.workspace.root}</code></div>
        <div className="view-toggle">
          <button className="command-button" onClick={() => { setPaletteOpen(true); setPaletteQuery(''); }}>Command <kbd>Ctrl K</kbd></button>
          <button className={state.canvas.view === 'canvas' ? 'active' : ''} onClick={() => void toggleView('canvas')}>Canvas</button>
          <button className={state.canvas.view === 'project' ? 'active' : ''} onClick={() => void toggleView('project')}>Project</button>
        </div>
      </header>
      <div className="body">
        <aside className="sidebar">
          <button className="workspace-button" onClick={() => void chooseWorkspace()}>Open workspace</button>
          <h3>Create</h3>
          <button onClick={() => void addNode('note')}>＋ Note</button>
          <button onClick={() => { void addNode('agent'); setTool('agent'); }}>＋ Agent node</button>
          <button onClick={() => { void addNode('terminal'); setTool('terminal'); }}>＋ Terminal node</button>
          <button onClick={() => { void addNode('editor'); setTool('editor'); }}>＋ Editor node</button>
          <h3>Inspect</h3>
          <button className={tool === 'tasks' ? 'selected' : ''} onClick={() => setTool('tasks')}>Tasks</button>
          <button className={tool === 'agent' ? 'selected' : ''} onClick={() => setTool('agent')}>Agent</button>
          <button className={tool === 'terminal' ? 'selected' : ''} onClick={() => setTool('terminal')}>Terminal</button>
          <button className={tool === 'editor' ? 'selected' : ''} onClick={() => setTool('editor')}>Editor</button>
          <button className={tool === 'git' ? 'selected' : ''} onClick={() => setTool('git')}>Git</button>
          <button className={tool === 'processes' ? 'selected' : ''} onClick={() => setTool('processes')}>Processes</button>
          <button className={tool === 'audit' ? 'selected' : ''} onClick={() => { setTool('audit'); void refresh(); }}>Audit <span>{state.audits.length}</span></button>
          <div className="security-card">
            <strong>{state.workspace.profile === 'observe' ? 'Observe profile' : 'Developer profile'}</strong>
            <span>Files: {state.workspace.profile === 'observe' ? 'read-only' : 'workspace-scoped writes'}</span>
            <span>Terminal: {state.workspace.profile === 'observe' ? 'disabled' : state.terminalGranted ? 'session approved' : 'approval required'}</span>
            <span>Remote listener: disabled</span>
            <button onClick={() => void toggleProfile()}>{state.workspace.profile === 'observe' ? 'Enable Developer profile' : 'Switch to Observe'}</button>
          </div>
        </aside>
        <main className="workspace-area">
          <div className="primary-view">
            {state.canvas.view === 'canvas' ? (
              <ReactFlow nodes={nodes} edges={[]} onNodesChange={onNodesChange} onNodeDragStop={onNodeDragStop} fitView>
                <Background gap={28} size={1} />
                <MiniMap pannable zoomable />
                <Controls />
              </ReactFlow>
            ) : <ProjectMode canvas={state.canvas} />}
          </div>
          <div className="inspector">
            {tool === 'tasks' && <TaskPanel onChanged={refresh} />}
            {tool === 'agent' && <AgentPanel onChanged={refresh} />}
            {tool === 'terminal' && <TerminalPanel onChanged={refresh} />}
            {tool === 'editor' && <EditorPanel onChanged={refresh} />}
            {tool === 'git' && <GitPanel onChanged={refresh} onPinDiff={pinDiff} />}
            {tool === 'processes' && <ProcessPanel onChanged={refresh} onPinProcess={pinProcess} />}
            {tool === 'audit' && <AuditPanel events={state.audits} />}
          </div>
        </main>
      </div>
      {paletteOpen && (
        <div className="palette-backdrop" onMouseDown={() => setPaletteOpen(false)}>
          <div className="command-palette" onMouseDown={event => event.stopPropagation()}>
            <input autoFocus aria-label="Command palette" placeholder="Type a command…" value={paletteQuery}
              onChange={event => setPaletteQuery(event.target.value)} />
            <div className="command-palette__list">
              {paletteMatches.map(command => (
                <button key={command.label} onClick={() => void runPaletteCommand(command)}>
                  <strong>{command.label}</strong><span>{command.detail}</span>
                </button>
              ))}
              {paletteMatches.length === 0 && <p className="muted">No commands match.</p>}
            </div>
          </div>
        </div>
      )}
      {error && <div className="error-toast" onClick={() => setError(null)}>{error}</div>}
    </div>
  );
}

const root = document.getElementById('root');
if (!root) throw new Error('Renderer root was not found');
createRoot(root).render(<App />);
