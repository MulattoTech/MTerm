import { z } from 'zod';

const id = z.string().min(1).max(160);
const timestamp = z.string().datetime({ offset: true });

export const taskStatusSchema = z.enum(['READY', 'RUNNING', 'WAITING', 'BLOCKED', 'FAILED', 'DONE']);
export type TaskStatus = z.infer<typeof taskStatusSchema>;

export const taskChecklistItemSchema = z.object({
  id,
  label: z.string().min(1).max(500),
  done: z.boolean(),
}).strict();

export const taskSchema = z.object({
  id,
  workspaceId: id.optional(),
  title: z.string().min(1).max(200),
  objective: z.string().min(1).max(20_000),
  status: taskStatusSchema,
  ownerSessionId: id.optional(),
  checklist: z.array(taskChecklistItemSchema).max(200),
  blockers: z.array(z.string().max(2_000)).max(100),
  acceptanceCriteria: z.array(z.string().max(2_000)).max(100),
  evidenceIds: z.array(id).max(500),
  createdAt: timestamp,
  updatedAt: timestamp,
}).strict();
export type Task = z.infer<typeof taskSchema>;

export const evidenceKindSchema = z.enum([
  'command', 'test', 'screenshot', 'artifact', 'file-hash', 'process-health', 'browser-assertion',
]);
export type EvidenceKind = z.infer<typeof evidenceKindSchema>;

export const validationEvidenceSchema = z.object({
  id,
  taskId: id.optional(),
  kind: evidenceKindSchema,
  label: z.string().min(1).max(300),
  status: z.enum(['PASS', 'FAIL', 'INFO']),
  summary: z.string().max(10_000),
  reference: z.string().max(4_096).optional(),
  exitCode: z.number().int().optional(),
  sha256: z.string().regex(/^[a-f0-9]{64}$/i).optional(),
  createdAt: timestamp,
}).strict();
export type ValidationEvidence = z.infer<typeof validationEvidenceSchema>;

export const agentSessionStatusSchema = z.enum([
  'IDLE', 'STARTING', 'RUNNING', 'WAITING', 'STOPPING', 'STOPPED', 'FAILED', 'DONE',
]);
export type AgentSessionStatus = z.infer<typeof agentSessionStatusSchema>;

export const providerCapabilitySchema = z.enum([
  'create-session', 'resume-session', 'submit-prompt', 'stream-events', 'cancel',
  'attachments', 'tool-calls', 'usage', 'model-metadata',
]);
export type ProviderCapability = z.infer<typeof providerCapabilitySchema>;

export const agentProviderManifestSchema = z.object({
  id,
  name: z.string().min(1).max(160),
  mode: z.enum(['assisted', 'cli', 'api', 'local', 'browser']),
  capabilities: z.array(providerCapabilitySchema).max(50),
  executable: z.string().max(4_096).optional(),
  models: z.array(z.string().max(300)).max(500).optional(),
}).strict();
export type AgentProviderManifest = z.infer<typeof agentProviderManifestSchema>;

export const agentSessionSchema = z.object({
  id,
  workspaceId: id,
  providerId: id,
  model: z.string().max(300).optional(),
  taskId: id.optional(),
  status: agentSessionStatusSchema,
  contextRefs: z.array(z.string().max(4_096)).max(500),
  resumabilityData: z.record(z.string(), z.string()).optional(),
  createdAt: timestamp,
  updatedAt: timestamp,
}).strict();
export type AgentSession = z.infer<typeof agentSessionSchema>;

export type AgentProvider = {
  manifest: AgentProviderManifest;
  createSession(input: { workspaceId: string; taskId?: string; model?: string }): Promise<AgentSession>;
  resumeSession?(session: AgentSession): Promise<AgentSession>;
  submitPrompt?(session: AgentSession, prompt: string): AsyncIterable<unknown> | Promise<unknown>;
  cancel?(session: AgentSession): Promise<void>;
};

const sessionTransitions: Record<AgentSessionStatus, ReadonlySet<AgentSessionStatus>> = {
  IDLE: new Set(['STARTING', 'STOPPED']),
  STARTING: new Set(['RUNNING', 'WAITING', 'FAILED', 'STOPPING']),
  RUNNING: new Set(['WAITING', 'STOPPING', 'FAILED', 'DONE']),
  WAITING: new Set(['RUNNING', 'STOPPING', 'FAILED', 'DONE']),
  STOPPING: new Set(['STOPPED', 'FAILED']),
  STOPPED: new Set(['STARTING']),
  FAILED: new Set(['STARTING', 'STOPPED']),
  DONE: new Set(['STARTING']),
};

export function canTransitionSession(from: AgentSessionStatus, to: AgentSessionStatus): boolean {
  return from === to || sessionTransitions[from].has(to);
}

export function transitionSession(session: AgentSession, status: AgentSessionStatus, now = new Date()): AgentSession {
  if (!canTransitionSession(session.status, status)) {
    throw new Error(`Invalid agent session transition: ${session.status} -> ${status}`);
  }
  return agentSessionSchema.parse({ ...session, status, updatedAt: now.toISOString() });
}

export const codexCliProviderManifest: AgentProviderManifest = agentProviderManifestSchema.parse({
  id: 'codex-cli',
  name: 'Codex CLI',
  mode: 'cli',
  capabilities: ['create-session', 'resume-session', 'submit-prompt', 'stream-events', 'cancel', 'tool-calls', 'usage', 'model-metadata'],
});

export const claudeCodeProviderManifest: AgentProviderManifest = agentProviderManifestSchema.parse({
  id: 'claude-code',
  name: 'Claude Code',
  mode: 'cli',
  capabilities: ['create-session', 'resume-session', 'submit-prompt', 'stream-events', 'cancel', 'usage', 'model-metadata'],
});

export const assistedProviderManifest: AgentProviderManifest = agentProviderManifestSchema.parse({
  id: 'assisted-handoff',
  name: 'Assisted Handoff',
  mode: 'assisted',
  capabilities: ['create-session', 'attachments', 'model-metadata'],
});

export function createTask(input: Pick<Task, 'id' | 'title' | 'objective' | 'workspaceId'>, now = new Date()): Task {
  const iso = now.toISOString();
  return taskSchema.parse({
    ...input,
    status: 'READY',
    checklist: [],
    blockers: [],
    acceptanceCriteria: [],
    evidenceIds: [],
    createdAt: iso,
    updatedAt: iso,
  });
}

export function createAssistedSession(
  input: { id: string; workspaceId: string; taskId?: string; model?: string },
  now = new Date(),
): AgentSession {
  const iso = now.toISOString();
  return agentSessionSchema.parse({
    ...input,
    providerId: assistedProviderManifest.id,
    status: 'IDLE',
    contextRefs: [],
    createdAt: iso,
    updatedAt: iso,
  });
}

export function createEvidence(
  input: Omit<ValidationEvidence, 'createdAt'>,
  now = new Date(),
): ValidationEvidence {
  return validationEvidenceSchema.parse({ ...input, createdAt: now.toISOString() });
}

export function validateTask(value: unknown): Task {
  return taskSchema.parse(value);
}

export function validateAgentSession(value: unknown): AgentSession {
  return agentSessionSchema.parse(value);
}

export function validateEvidence(value: unknown): ValidationEvidence {
  return validationEvidenceSchema.parse(value);
}

const interruptedSessionStatuses = new Set<AgentSessionStatus>(['STARTING', 'RUNNING', 'WAITING', 'STOPPING']);

export function reconcileInterruptedSession(session: AgentSession, now = new Date()): AgentSession {
  const validated = validateAgentSession(session);
  if (!interruptedSessionStatuses.has(validated.status)) return validated;
  return agentSessionSchema.parse({
    ...validated,
    status: 'STOPPED',
    updatedAt: now.toISOString(),
  });
}
