import { describe, expect, it } from 'vitest';
import { initialCanvas, validateCanvas, prepareHandoff } from '../packages/core/src/canvas';

describe('canvas domain', () => {
  it('creates a usable initial workspace with unique node IDs', () => {
    const canvas = initialCanvas();
    expect(canvas.nodes.length).toBeGreaterThan(0);
    expect(new Set(canvas.nodes.map(n => n.id)).size).toBe(canvas.nodes.length);
    expect(validateCanvas(canvas)).toEqual(canvas);
  });
  it('rejects unknown fields and invalid node types', () => {
    expect(() => validateCanvas({ ...initialCanvas(), command: 'unexpected' })).toThrow();
    const canvas = initialCanvas();
    expect(() => validateCanvas({ ...canvas, nodes: [{ ...canvas.nodes[0], kind: 'shell' }] })).toThrow();
  });
  it('rejects duplicate IDs, dangling edges, and invalid zoom', () => {
    const canvas = initialCanvas();
    expect(() => validateCanvas({ ...canvas, nodes: [canvas.nodes[0], canvas.nodes[0]] })).toThrow();
    expect(() => validateCanvas({ ...canvas, edges: [{ id: 'edge', source: 'missing', target: 'missing' }] })).toThrow();
    expect(() => validateCanvas({ ...canvas, viewport: { x: 0, y: 0, zoom: 0 } })).toThrow();
  });
  it('builds an assisted handoff without claiming any model ran', () => {
    const text = prepareHandoff('Example project', initialCanvas());
    expect(text).toContain('Example project');
    expect(text).toContain('human-controlled');
    expect(text).toContain('not instructions to execute automatically');
  });
});
