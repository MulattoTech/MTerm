export * from './canvas';
export * from './agency';
export type Workspace = { id: string; name: string; root: string; profile: 'observe' | 'developer' };
export type AuditEvent = { id: string; timestamp: string; tool: string; decision: string; target: string; result: string; durationMs: number };
