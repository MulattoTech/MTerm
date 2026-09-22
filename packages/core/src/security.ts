import fs from 'node:fs/promises';
import path from 'node:path';

export type PolicyDecision = 'ALLOW' | 'ASK' | 'DENY';
export type CommandRisk = 'READ' | 'EXECUTE' | 'DESTRUCTIVE';

type Request = {
  workspaceId: string;
  actor: string;
  sessionId: string;
  capability: string;
};

type Grant = Request & {
  id?: string;
  scope?: string;
  expiresAt?: number;
};

const reservedWindowsName = /^(?:con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\..*)?$/i;
const secretKey = /(?:authorization|token|password|passphrase|secret|api[-_]?key|private[-_]?key|cookie)/i;

function denied(message: string): Error {
  return new Error(`PATH_DENIED: ${message}`);
}

function inside(root: string, candidate: string): boolean {
  const relative = path.relative(root, candidate);
  return relative === '' || (!relative.startsWith(`..${path.sep}`) && relative !== '..' && !path.isAbsolute(relative));
}

function validateRelativeTarget(target: string): string[] {
  if (!target || target.includes('\0')) throw denied('invalid path');
  if (path.isAbsolute(target) || path.win32.isAbsolute(target) || path.posix.isAbsolute(target)) throw denied('absolute path');
  if (/^[a-zA-Z]:/.test(target) || target.startsWith('\\\\')) throw denied('drive or UNC path');
  const parts = target.split(/[\\/]+/);
  for (const part of parts) {
    if (!part || part === '.') continue;
    if (part === '..') throw denied('parent traversal');
    if (part.includes(':')) throw denied('alternate data stream or drive syntax');
    if (/[. ]$/.test(part)) throw denied('ambiguous trailing character');
    if (reservedWindowsName.test(part)) throw denied('reserved Windows device name');
  }
  return parts.filter(part => part && part !== '.');
}

function writeProtected(parts: string[]): boolean {
  return parts.some((part) => {
    const lower = part.toLowerCase();
    return lower === '.git' || lower === '.ssh' || lower === '.gnupg' || lower === '.env' || lower.startsWith('.env.');
  });
}

async function nearestExisting(candidate: string): Promise<{ real: string; suffix: string[] }> {
  const suffix: string[] = [];
  let current = candidate;
  for (;;) {
    try {
      return { real: await fs.realpath(current), suffix: suffix.reverse() };
    } catch (error) {
      if (!(error instanceof Error) || !('code' in error) || (error as NodeJS.ErrnoException).code !== 'ENOENT') throw error;
      const parent = path.dirname(current);
      if (parent === current) throw denied('no existing ancestor');
      suffix.push(path.basename(current));
      current = parent;
    }
  }
}

export async function safePath(root: string, target: string, mode: 'read' | 'write' = 'read'): Promise<string> {
  const parts = validateRelativeTarget(target);
  if (mode === 'write' && writeProtected(parts)) throw denied('protected metadata or secret path');
  const canonicalRoot = await fs.realpath(root);
  const candidate = path.resolve(canonicalRoot, ...parts);
  if (!inside(canonicalRoot, candidate)) throw denied('outside workspace');

  try {
    const canonicalTarget = await fs.realpath(candidate);
    if (!inside(canonicalRoot, canonicalTarget)) throw denied('symlink or junction escape');
    const stat = await fs.stat(canonicalTarget);
    if (stat.isFile() && stat.nlink > 1) throw denied('hard-linked file alias');
    return canonicalTarget;
  } catch (error) {
    const code = error instanceof Error && 'code' in error ? (error as NodeJS.ErrnoException).code : undefined;
    if (code !== 'ENOENT') throw error;
    if (mode !== 'write') throw denied('path does not exist');
  }

  const existing = await nearestExisting(candidate);
  if (!inside(canonicalRoot, existing.real)) throw denied('symlink or junction escape');
  const rebuilt = path.resolve(existing.real, ...existing.suffix);
  if (!inside(canonicalRoot, rebuilt)) throw denied('outside workspace');
  return rebuilt;
}

const observeAllow = new Set(['filesystem.read', 'git.read', 'process.read', 'system.info']);
const developerAllow = new Set([...observeAllow, 'filesystem.write', 'git.write', 'process.start']);
const approvalRequired = new Set(['terminal.execute', 'agent.execute', 'git.worktree', 'process.kill', 'browser.control', 'network.outbound', 'docker.manage']);

function requestFrom(value: unknown): Request | undefined {
  if (!value || typeof value !== 'object') return undefined;
  const candidate = value as Partial<Request>;
  if (![candidate.workspaceId, candidate.actor, candidate.sessionId, candidate.capability].every(v => typeof v === 'string' && v.length > 0)) return undefined;
  return candidate as Request;
}

export function evaluate(profile: string, request: unknown, grants: unknown[] = [], now = Date.now()): { decision: PolicyDecision; reason: string } {
  const req = requestFrom(request);
  if (!req) return { decision: 'DENY', reason: 'Malformed permission request' };

  if (profile === 'observe') {
    return observeAllow.has(req.capability)
      ? { decision: 'ALLOW', reason: 'Observe profile permits read-only capability' }
      : { decision: 'DENY', reason: 'Observe profile is read-only' };
  }

  if (!['developer', 'autonomous-developer', 'full-control'].includes(profile)) {
    return { decision: 'DENY', reason: 'Unknown permission profile' };
  }

  if (developerAllow.has(req.capability)) return { decision: 'ALLOW', reason: 'Profile permits workspace-scoped capability' };

  if (approvalRequired.has(req.capability)) {
    const matched = grants.some(value => {
      if (!value || typeof value !== 'object') return false;
      const grant = value as Partial<Grant>;
      return grant.workspaceId === req.workspaceId &&
        grant.actor === req.actor &&
        grant.sessionId === req.sessionId &&
        grant.capability === req.capability &&
        typeof grant.expiresAt === 'number' &&
        grant.expiresAt > now;
    });
    return matched
      ? { decision: 'ALLOW', reason: 'Matching unexpired grant' }
      : { decision: 'ASK', reason: 'Explicit approval required' };
  }

  return { decision: 'DENY', reason: 'Capability is dangerous, unknown, or not implemented' };
}

export function redact(value: unknown, secrets: string[] = []): unknown {
  const replacements = secrets.filter(Boolean).sort((a, b) => b.length - a.length);
  const seen = new WeakSet<object>();

  const walk = (input: unknown): unknown => {
    if (typeof input === 'string') {
      return replacements.reduce((text, secret) => text.split(secret).join('[REDACTED]'), input);
    }
    if (Array.isArray(input)) return input.map(walk);
    if (input && typeof input === 'object') {
      if (seen.has(input)) return '[Circular]';
      seen.add(input);
      const output: Record<string, unknown> = {};
      for (const [key, child] of Object.entries(input)) {
        output[key] = secretKey.test(key) ? '[REDACTED]' : walk(child);
      }
      return output;
    }
    return input;
  };

  return walk(value);
}

export function classifyCommand(executable: string, args: string[]): { risk: CommandRisk; summary: string } {
  const command = path.basename(executable).toLowerCase().replace(/\.(?:exe|cmd|bat|ps1)$/i, '');
  const normalizedArgs = args.map(arg => arg.toLowerCase());
  const joined = normalizedArgs.join(' ');
  const summary = [path.basename(executable), ...args].join(' ').trim();

  const destructiveExecutable = new Set(['rm', 'rmdir', 'del', 'erase', 'format', 'diskpart', 'remove-item']);
  if (destructiveExecutable.has(command)) return { risk: 'DESTRUCTIVE', summary };
  if (command === 'git' && (
    (normalizedArgs[0] === 'push' && normalizedArgs.some(arg => arg === '--force' || arg === '-f' || arg.startsWith('--force-with-lease'))) ||
    (normalizedArgs[0] === 'reset' && normalizedArgs.includes('--hard')) ||
    (normalizedArgs[0] === 'clean' && /(^|\s)-[^ ]*[fdx]/.test(joined))
  )) return { risk: 'DESTRUCTIVE', summary };

  const firstArg = normalizedArgs[0] ?? '';
  const versionOnly = normalizedArgs.length === 1 && ['--version', '-v', '-version', 'version'].includes(firstArg);
  if (versionOnly) return { risk: 'READ', summary };
  if (['pwd', 'whoami', 'hostname', 'ls', 'dir'].includes(command)) return { risk: 'READ', summary };
  if (command === 'git' && ['status', 'diff', 'log', 'show', 'rev-parse'].includes(normalizedArgs[0] ?? '')) {
    return { risk: 'READ', summary };
  }

  return { risk: 'EXECUTE', summary };
}
