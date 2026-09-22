import { describe, expect, it } from 'vitest';
import {
  assistedProviderManifest,
  canTransitionSession,
  createAssistedSession,
  createEvidence,
  createTask,
  reconcileInterruptedSession,
  transitionSession,
  validateAgentSession,
  validateEvidence,
  validateTask,
} from '../packages/core/src/agency';

const now = new Date('2026-09-22T12:00:00.000Z');

describe('agency domain', () => {
  it('creates a durable task with explicit initial state', () => {
    const task = createTask({ id: 'task-1', title: 'Ship slice', objective: 'Validate the next slice' }, now);
    expect(task.status).toBe('READY');
    expect(task.createdAt).toBe(now.toISOString());
    expect(validateTask(task)).toEqual(task);
  });

  it('creates an assisted session without pretending a model ran', () => {
    const session = createAssistedSession({ id: 'session-1', workspaceId: 'workspace-1', taskId: 'task-1' }, now);
    expect(session.providerId).toBe(assistedProviderManifest.id);
    expect(session.status).toBe('IDLE');
    expect(assistedProviderManifest.mode).toBe('assisted');
    expect(validateAgentSession(session)).toEqual(session);
  });

  it('enforces explicit agent session state transitions', () => {
    const session = createAssistedSession({ id: 'session-1', workspaceId: 'workspace-1' }, now);
    expect(canTransitionSession('IDLE', 'STARTING')).toBe(true);
    expect(canTransitionSession('DONE', 'RUNNING')).toBe(false);
    expect(canTransitionSession('DONE', 'STARTING')).toBe(true);
    const starting = transitionSession(session, 'STARTING', new Date('2026-09-22T12:01:00.000Z'));
    expect(starting.status).toBe('STARTING');
    expect(() => transitionSession(starting, 'DONE', now)).toThrow(/Invalid agent session transition/);
  });

  it('validates evidence with a concrete result and optional provenance', () => {
    const evidence = createEvidence({
      id: 'evidence-1',
      taskId: 'task-1',
      kind: 'test',
      label: 'Vitest',
      status: 'PASS',
      summary: '47 tests passed',
      reference: 'artifacts/quality-gates.log',
      exitCode: 0,
    }, now);
    expect(validateEvidence(evidence)).toEqual(evidence);
  });

  it('rejects malformed task/session/evidence input', () => {
    expect(() => validateTask({ id: 'x' })).toThrow();
    expect(() => validateAgentSession({ id: 'x', status: 'MAGIC' })).toThrow();
    expect(() => validateEvidence({ id: 'x', status: 'PASS', kind: 'unknown' })).toThrow();
  });
});

describe('restart reconciliation', () => {
  it('marks interrupted active sessions stopped without rewriting terminal states', () => {
    const session = createAssistedSession({ id: 'resume-1', workspaceId: 'workspace-1' }, now);
    const running = transitionSession(transitionSession(session, 'STARTING', now), 'RUNNING', now);
    const recoveredAt = new Date('2026-09-22T13:00:00.000Z');
    const recovered = reconcileInterruptedSession(running, recoveredAt);
    expect(recovered.status).toBe('STOPPED');
    expect(recovered.updatedAt).toBe(recoveredAt.toISOString());
    expect(reconcileInterruptedSession({ ...running, status: 'DONE' }, recoveredAt).status).toBe('DONE');
  });
});
