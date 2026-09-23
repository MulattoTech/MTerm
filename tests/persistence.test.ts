import { afterEach, describe, expect, it } from 'vitest';
import fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import { DatabaseSync } from 'node:sqlite';
import {
  AstraStore,
  CURRENT_SCHEMA_VERSION,
  createAuditEvent,
} from '../packages/core/src/persistence';

const cleanup: string[] = [];

async function tempDb(): Promise<string> {
  const dir = await fs.mkdtemp(path.join(os.tmpdir(), 'astra-store-'));
  cleanup.push(dir);
  return path.join(dir, 'test.sqlite');
}

afterEach(async () => {
  await Promise.all(cleanup.splice(0).map(dir => fs.rm(dir, { recursive: true, force: true })));
});
describe('AstraStore migrations', () => {
  it('creates the latest schema and passes integrity checking', async () => {
    const file = await tempDb();
    const store = new AstraStore(file);
    expect(store.schemaVersion()).toBe(CURRENT_SCHEMA_VERSION);
    expect(store.integrity()).toEqual({ ok: true, message: 'ok' });
    store.close();
  });

  it('migrates schema v1 in place without losing settings, records, or audit', async () => {
    const file = await tempDb();
    const legacy = new DatabaseSync(file);
    legacy.exec('CREATE TABLE settings (key TEXT PRIMARY KEY, value TEXT NOT NULL);');
    legacy.exec('CREATE TABLE audit (id TEXT PRIMARY KEY, timestamp TEXT NOT NULL, payload TEXT NOT NULL);');
    legacy.exec('CREATE TABLE records (id TEXT PRIMARY KEY, kind TEXT NOT NULL, updatedAt TEXT NOT NULL, payload TEXT NOT NULL);');
    legacy.exec('PRAGMA user_version=1;');
    legacy.prepare('INSERT INTO settings(key,value) VALUES(?,?)').run('workspace', JSON.stringify({ id: 'w1' }));
    legacy.prepare('INSERT INTO records(id,kind,updatedAt,payload) VALUES(?,?,?,?)')
      .run('t1', 'task', '2026-09-22T00:00:00.000Z', JSON.stringify({ id: 't1' }));
    const audit = createAuditEvent({ tool: 'legacy', decision: 'ALLOW', target: 'x', result: 'ok' });
    legacy.prepare('INSERT INTO audit(id,timestamp,payload) VALUES(?,?,?)')
      .run(audit.id, audit.timestamp, JSON.stringify(audit));
    legacy.close();

    const store = new AstraStore(file);
    expect(store.schemaVersion()).toBe(CURRENT_SCHEMA_VERSION);
    expect(store.get('workspace', null)).toEqual({ id: 'w1' });
    expect(store.records<{ id: string }>('task')).toEqual([{ id: 't1' }]);
    expect(store.audits(10).some(event => event.tool === 'legacy')).toBe(true);
    store.close();
  });

  it('refuses a database created by a newer incompatible schema', async () => {
    const file = await tempDb();
    const db = new DatabaseSync(file);
    db.exec(`PRAGMA user_version=${CURRENT_SCHEMA_VERSION + 1};`);
    db.close();
    expect(() => new AstraStore(file)).toThrow(/newer than supported/);
  });
});
