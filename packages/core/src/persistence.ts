import { mkdirSync } from 'node:fs';
import path from 'node:path';
import { randomUUID } from 'node:crypto';
import { DatabaseSync } from 'node:sqlite';

export const CURRENT_SCHEMA_VERSION = 2;

export type AuditEvent = {
  id: string;
  timestamp: string;
  tool: string;
  decision: string;
  target: string;
  result: string;
  durationMs: number;
};

type IntegrityRow = { quick_check?: string };

export function createAuditEvent(
  input: Omit<AuditEvent, 'id' | 'timestamp' | 'durationMs'> & { started?: number },
): AuditEvent {
  return {
    id: randomUUID(),
    timestamp: new Date().toISOString(),
    tool: input.tool,
    decision: input.decision,
    target: input.target.slice(0, 1000),
    result: input.result.slice(0, 1000),
    durationMs: Math.max(0, Date.now() - (input.started ?? Date.now())),
  };
}
export class AstraStore {
  private readonly db: DatabaseSync;

  constructor(file: string) {
    mkdirSync(path.dirname(file), { recursive: true });
    this.db = new DatabaseSync(file);
    try {
      this.db.exec('PRAGMA journal_mode=WAL;');
      this.db.exec('PRAGMA busy_timeout=3000;');
      this.db.exec('PRAGMA foreign_keys=ON;');
      this.migrate();
    } catch (error) {
      this.db.close();
      throw error;
    }
  }

  private version(): number {
    const row = this.db.prepare('PRAGMA user_version').get() as { user_version?: number };
    return Number(row?.user_version ?? 0);
  }

  private migrate(): void {
    let version = this.version();
    if (version > CURRENT_SCHEMA_VERSION) {
      throw new Error(`Database schema ${version} is newer than supported ${CURRENT_SCHEMA_VERSION}`);
    }
    if (version === 0) {
      this.db.exec('BEGIN IMMEDIATE;');
      try {
        this.db.exec('CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT NOT NULL);');
        this.db.exec('CREATE TABLE IF NOT EXISTS audit (id TEXT PRIMARY KEY, timestamp TEXT NOT NULL, payload TEXT NOT NULL);');
        this.db.exec('CREATE TABLE IF NOT EXISTS records (id TEXT PRIMARY KEY, kind TEXT NOT NULL, updatedAt TEXT NOT NULL, payload TEXT NOT NULL);');
        this.db.exec('CREATE INDEX IF NOT EXISTS records_kind_updated ON records(kind, updatedAt DESC);');
        this.db.exec('PRAGMA user_version=1;');
        this.db.exec('COMMIT;');
        version = 1;
      } catch (error) {
        this.db.exec('ROLLBACK;');
        throw error;
      }
    }
    if (version === 1) {
      this.db.exec('BEGIN IMMEDIATE;');
      try {
        this.db.exec('CREATE TABLE IF NOT EXISTS schema_migrations (version INTEGER PRIMARY KEY, name TEXT NOT NULL, appliedAt TEXT NOT NULL);');
        this.db.prepare('INSERT OR IGNORE INTO schema_migrations(version,name,appliedAt) VALUES(?,?,?)')
          .run(1, 'initial-settings-audit-records', new Date().toISOString());
        this.db.exec('CREATE INDEX IF NOT EXISTS audit_timestamp ON audit(timestamp DESC);');
        this.db.prepare('INSERT OR REPLACE INTO schema_migrations(version,name,appliedAt) VALUES(?,?,?)')
          .run(2, 'migration-ledger-and-audit-index', new Date().toISOString());
        this.db.exec('PRAGMA user_version=2;');
        this.db.exec('COMMIT;');
      } catch (error) {
        this.db.exec('ROLLBACK;');
        throw error;
      }
    }
  }
  schemaVersion(): number {
    return this.version();
  }

  integrity(): { ok: boolean; message: string } {
    const row = this.db.prepare('PRAGMA quick_check').get() as IntegrityRow | undefined;
    const message = String(row?.quick_check ?? 'unknown');
    return { ok: message.toLowerCase() === 'ok', message };
  }

  get<T>(key: string, fallback: T): T {
    const row = this.db.prepare('SELECT value FROM settings WHERE key = ?').get(key) as { value?: string } | undefined;
    if (!row?.value) return fallback;
    try { return JSON.parse(row.value) as T; } catch { return fallback; }
  }

  set(key: string, value: unknown): void {
    this.db.prepare('INSERT INTO settings(key,value) VALUES(?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value')
      .run(key, JSON.stringify(value));
  }

  addAudit(event: AuditEvent): void {
    this.db.prepare('INSERT INTO audit(id,timestamp,payload) VALUES(?,?,?)')
      .run(event.id, event.timestamp, JSON.stringify(event));
  }

  audits(limit = 100): AuditEvent[] {
    const safeLimit = Math.max(1, Math.min(5000, Math.trunc(limit)));
    const rows = this.db.prepare('SELECT payload FROM audit ORDER BY timestamp DESC LIMIT ?').all(safeLimit) as Array<{ payload: string }>;
    return rows.flatMap(row => {
      try { return [JSON.parse(row.payload) as AuditEvent]; } catch { return []; }
    });
  }

  records<T>(kind: string): T[] {
    const rows = this.db.prepare('SELECT payload FROM records WHERE kind = ? ORDER BY updatedAt DESC').all(kind) as Array<{ payload: string }>;
    return rows.flatMap(row => {
      try { return [JSON.parse(row.payload) as T]; } catch { return []; }
    });
  }

  putRecord(kind: string, id: string, updatedAt: string, value: unknown): void {
    this.db.prepare(
      'INSERT INTO records(id,kind,updatedAt,payload) VALUES(?,?,?,?) ON CONFLICT(id) DO UPDATE SET kind=excluded.kind, updatedAt=excluded.updatedAt, payload=excluded.payload',
    ).run(id, kind, updatedAt, JSON.stringify(value));
  }

  deleteRecord(kind: string, id: string): boolean {
    const result = this.db.prepare('DELETE FROM records WHERE kind = ? AND id = ?').run(kind, id);
    return Number(result.changes) > 0;
  }

  close(): void {
    this.db.close();
  }
}
