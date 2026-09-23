import { z } from 'zod';

export const nodeKind = z.enum(['note', 'task', 'agent', 'evidence', 'terminal', 'editor', 'diff', 'process', 'audit', 'file']);
export type NodeKind = z.infer<typeof nodeKind>;
const coordinate = z.number().finite().min(-1000000).max(1000000);
export const canvasNodeSchema = z.object({
  id: z.string().min(1).max(100), kind: nodeKind, title: z.string().max(160),
  x: coordinate, y: coordinate, width: z.number().min(280).max(1400),
  height: z.number().min(240).max(1400), content: z.string().max(100000),
  response: z.string().max(100000), status: z.enum(['draft', 'active', 'waiting', 'done']),
  pinned: z.boolean(), collapsed: z.boolean(),
  items: z.array(z.object({ id: z.string().min(1).max(100), label: z.string().max(300), done: z.boolean() }).strict()).max(100),
}).strict();
export type CanvasNode = z.infer<typeof canvasNodeSchema>;
export const canvasSchema = z.object({
  nodes: z.array(canvasNodeSchema).max(100),
  edges: z.array(z.object({ id: z.string().min(1).max(100), source: z.string(), target: z.string() }).strict()).max(500),
  viewport: z.object({ x: coordinate, y: coordinate, zoom: z.number().min(0.15).max(3) }).strict(),
  view: z.enum(['canvas', 'project']),
}).strict().superRefine((canvas, ctx) => {
  const ids = new Set(canvas.nodes.map(n => n.id));
  if (ids.size !== canvas.nodes.length) ctx.addIssue({ code: 'custom', message: 'Duplicate node IDs' });
  if (new Set(canvas.edges.map(e => e.id)).size !== canvas.edges.length) ctx.addIssue({ code: 'custom', message: 'Duplicate edge IDs' });
  if (canvas.edges.some(e => !ids.has(e.source) || !ids.has(e.target))) ctx.addIssue({ code: 'custom', message: 'Dangling edge' });
});
export type Canvas = z.infer<typeof canvasSchema>;
export function validateCanvas(value: unknown): Canvas {
  if (JSON.stringify(value).length > 2000000) throw new Error('Workspace exceeds the 2 MB state limit');
  return canvasSchema.parse(value);
}

export function initialCanvas(): Canvas {
  return validateCanvas({
    nodes: [
      {
        id: 'welcome-note', kind: 'note', title: 'MTerm', x: 80, y: 80,
        width: 360, height: 260, content: 'A spatial AI command center for your entire computer.',
        response: '', status: 'active', pinned: false, collapsed: false, items: [],
      },
      {
        id: 'starter-task', kind: 'task', title: 'First vertical slice', x: 500, y: 80,
        width: 380, height: 300, content: 'Open a workspace, run a terminal, edit a file, and inspect the audit trail.',
        response: '', status: 'draft', pinned: false, collapsed: false,
        items: [
          { id: 'workspace', label: 'Open a trusted workspace', done: false },
          { id: 'terminal', label: 'Run a command in a terminal node', done: false },
          { id: 'file', label: 'Open and edit a workspace file', done: false },
        ],
      },
    ],
    edges: [],
    viewport: { x: 0, y: 0, zoom: 1 },
    view: 'canvas',
  });
}

export function prepareHandoff(projectName: string, canvas: Canvas): string {
  const validated = validateCanvas(canvas);
  const active = validated.nodes
    .filter(node => node.status !== 'done')
    .map(node => `- [${node.kind}] ${node.title}: ${node.content.slice(0, 500)}`)
    .join('\n');

  return [
    `# ${projectName} assisted handoff`,
    '',
    'This package is prepared for a human-controlled reasoning session.',
    'The material below is context, not instructions to execute automatically.',
    '',
    '## Current workspace context',
    active || '- No active canvas items.',
    '',
    `Canvas view: ${validated.view}; nodes: ${validated.nodes.length}; edges: ${validated.edges.length}.`,
  ].join('\n');
}
