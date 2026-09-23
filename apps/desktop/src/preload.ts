import { contextBridge, ipcRenderer } from 'electron';

type TerminalData = { id: string; data: string };
type TerminalExit = { id: string; exitCode: number };
type AgentEvent = { id: string; type: string; data?: string; text?: string; message?: string; threadId?: string; exitCode?: number | null; signal?: string | null };

const api = {
  state: () => ipcRenderer.invoke('astra:state'),
  chooseWorkspace: () => ipcRenderer.invoke('astra:workspace:choose'),
  setProfile: (profile: 'observe' | 'developer') => ipcRenderer.invoke('astra:profile:set', profile),
  saveCanvas: (canvas: unknown) => ipcRenderer.invoke('astra:canvas:save', canvas),
  listDirectory: (relativePath: string) => ipcRenderer.invoke('astra:file:list', relativePath),
  readFile: (relativePath: string) => ipcRenderer.invoke('astra:file:read', relativePath),
  writeFile: (relativePath: string, content: string, expectedVersion?: string) => ipcRenderer.invoke('astra:file:write', relativePath, content, expectedVersion),
  gitSnapshot: () => ipcRenderer.invoke('astra:git:snapshot'),
  gitDiff: (filePath: string, staged = false) => ipcRenderer.invoke('astra:git:diff', filePath, staged),
  gitStage: (filePath: string) => ipcRenderer.invoke('astra:git:stage', filePath),
  gitUnstage: (filePath: string) => ipcRenderer.invoke('astra:git:unstage', filePath),
  grantGitWorktree: () => ipcRenderer.invoke('astra:git:worktree:grant'),
  createGitWorktree: (name: string) => ipcRenderer.invoke('astra:git:worktree:create', name),
  listProcesses: () => ipcRenderer.invoke('astra:process:list'),
  grantProcessKill: () => ipcRenderer.invoke('astra:process:grant-kill'),
  killOwnedProcess: (pid: number) => ipcRenderer.invoke('astra:process:kill-owned', pid),
  listTasks: () => ipcRenderer.invoke('astra:task:list'),
  createTask: (title: string, objective: string) => ipcRenderer.invoke('astra:task:create', title, objective),
  updateTaskStatus: (id: string, status: string) => ipcRenderer.invoke('astra:task:status', id, status),
  updateTaskDetails: (id: string, patch: unknown) => ipcRenderer.invoke('astra:task:update-details', id, patch),
  listEvidence: () => ipcRenderer.invoke('astra:evidence:list'),
  createEvidence: (input: unknown) => ipcRenderer.invoke('astra:evidence:create', input),
  listAgentSessions: () => ipcRenderer.invoke('astra:agent:sessions'),
  agentStatus: () => ipcRenderer.invoke('astra:agent:status'),
  agentProviders: () => ipcRenderer.invoke('astra:agent:providers'),
  grantAgent: () => ipcRenderer.invoke('astra:agent:grant'),
  startAgent: (prompt: string, taskId?: string) => ipcRenderer.invoke('astra:agent:start', prompt, taskId),
  resumeAgent: (sessionId: string, prompt: string, taskId?: string) => ipcRenderer.invoke('astra:agent:resume', sessionId, prompt, taskId),
  runProviderAgent: (providerId: string, prompt: string, taskId?: string, resumeSessionId?: string) => ipcRenderer.invoke('astra:agent:provider-run', providerId, prompt, taskId, resumeSessionId),
  stopAgent: (id: string) => ipcRenderer.invoke('astra:agent:stop', id),
  grantTerminal: () => ipcRenderer.invoke('astra:terminal:grant'),
  createTerminal: (cols: number, rows: number) => ipcRenderer.invoke('astra:terminal:create', cols, rows),
  writeTerminal: (id: string, data: string) => ipcRenderer.invoke('astra:terminal:write', id, data),
  resizeTerminal: (id: string, cols: number, rows: number) => ipcRenderer.invoke('astra:terminal:resize', id, cols, rows),
  killTerminal: (id: string) => ipcRenderer.invoke('astra:terminal:kill', id),

  onTerminalData: (listener: (event: TerminalData) => void) => {
    const wrapped = (_event: Electron.IpcRendererEvent, payload: TerminalData) => listener(payload);
    ipcRenderer.on('terminal:data', wrapped);
    return () => { ipcRenderer.removeListener('terminal:data', wrapped); };
  },
  onTerminalExit: (listener: (event: TerminalExit) => void) => {
    const wrapped = (_event: Electron.IpcRendererEvent, payload: TerminalExit) => listener(payload);
    ipcRenderer.on('terminal:exit', wrapped);
    return () => { ipcRenderer.removeListener('terminal:exit', wrapped); };
  },
  onAgentEvent: (listener: (event: AgentEvent) => void) => {
    const wrapped = (_event: Electron.IpcRendererEvent, payload: AgentEvent) => listener(payload);
    ipcRenderer.on('agent:event', wrapped);
    return () => { ipcRenderer.removeListener('agent:event', wrapped); };
  },
};

contextBridge.exposeInMainWorld('astra', api);

export type AstraBridge = typeof api;
