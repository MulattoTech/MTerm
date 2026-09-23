# SPDX-License-Identifier: MIT
# AI-Change: 2026-09-23-workbench-roadmap
"""Offline roadmap math, strict validation, source-review checking and CLI.
Generation never infers completion. Only explicit reviewed progress.json fields confer credit.
"""
from __future__ import annotations
import argparse
from datetime import date, datetime, timezone
import hashlib
import html
import json
import math
from pathlib import Path
import re
import subprocess
import sys
STAGES={'not_started':0,'planned':10,'implementation':40,'integrated':65,'validated':85,'complete':100}
COLORS={'not_started':'#64748b','planned':'#a78bfa','implementation':'#60a5fa','integrated':'#22d3ee','validated':'#fbbf24','complete':'#34d399','blocked':'#fb7185','excluded':'#94a3b8'}
SQUARES={'not_started':'⬜','planned':'🟪','implementation':'🟦','integrated':'🟦','validated':'🟨','complete':'🟩','blocked':'🟥','excluded':'⬛'}
ROOT=Path(__file__).resolve().parents[2]
CATALOG='docs/roadmap/progress.json'
INDEX='docs/backlog/requirements-index.json'

def pct(item): return STAGES.get(item['stage'],0)

def remaining(item):
    if item['stage']=='excluded':return [0.0,0.0]
    return [round(v*(1-pct(item)/100),2) for v in item['estimate_hours']]

def eta_label(item):
    if item['stage']=='complete':return 'Complete'
    if item['stage']=='excluded':return 'Not applicable'
    return item.get('eta_date') or ('Blocked / unscheduled' if item.get('blocker') else 'Unscheduled')

def summarize(items):
    included=[i for i in items if i['stage']!='excluded'];weight=sum(i['weight'] for i in included)
    return {'percent':round(sum(i['weight']*pct(i) for i in included)/weight,2) if weight else 0,
        'weight':weight,'included':len(included),'excluded':len(items)-len(included),
        'complete':sum(i['stage']=='complete' for i in included),
        'remaining_hours':[round(sum(remaining(i)[n] for i in included),1) for n in (0,1)],
        'blocked':sum(bool(i.get('blocker')) for i in included)}

def resolve_evidence(root,reference):
    if not isinstance(reference,str):raise ValueError('Evidence must be a path string')
    name=reference.split('::',1)[0]
    if not name or '\\' in name or Path(name).is_absolute():raise ValueError('Unsafe evidence path')
    path=(root/name).resolve()
    if not path.is_relative_to(root.resolve()) or not path.is_file():raise ValueError('Missing/unsafe evidence: '+name)
    if '::' in reference:
        symbol=reference.split('::',1)[1]
        if not symbol or symbol not in path.read_text(encoding='utf-8-sig'):raise ValueError('Missing evidence symbol: '+reference)

def validate(catalog,index,root):
    if 'stage_percentages' in catalog and catalog['stage_percentages']!=STAGES:raise ValueError('Stage rubric changed; reconcile generator, tests and methodology explicitly')
    originals={i['id'] for i in index['requirements']};additions=catalog.get('additional_requirements',[])
    if len({i['id'] for i in additions})!=len(additions):raise ValueError('Duplicate additional requirement')
    extra={i['id'] for i in additions}
    if originals & extra:raise ValueError('Additional ID collides with original')
    expected=originals|extra;ids=[i['id'] for i in catalog['items']]
    if len(set(ids))!=len(ids) or set(ids)!=expected:raise ValueError('Coverage missing, duplicated, or unexpected IDs')
    for item in catalog['items']:
        stage=item['stage']
        if stage not in {*STAGES,'excluded'}:raise ValueError('Unknown stage')
        if not item.get('note') or not item.get('reviewed_at'):raise ValueError('Rationale/date required')
        datetime.fromisoformat(item['reviewed_at'].replace('Z','+00:00'))
        weight=item['weight']
        if isinstance(weight,bool) or not isinstance(weight,(int,float)) or not math.isfinite(weight) or weight<0:raise ValueError('Invalid weight')
        if (stage=='excluded') != (weight==0):raise ValueError('Excluded rows alone have zero weight')
        effort=item['estimate_hours']
        if len(effort)!=2 or any(isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) or v<0 for v in effort) or effort[0]>effort[1]:raise ValueError('Invalid effort range')
        if not item.get('estimate_basis'):raise ValueError('Effort requires explicit basis')
        if stage!='excluded' and not effort[1]:raise ValueError('Included scope requires baseline estimate')
        if stage not in ('not_started','excluded') and not item.get('evidence'):raise ValueError('Credit requires evidence')
        if stage in ('validated','complete') and not item.get('tests'):raise ValueError('Validated/done requires tests')
        if stage=='complete' and item.get('blocker'):raise ValueError('Blocked cannot be complete')
        for ref in item.get('evidence',[])+item.get('tests',[]):resolve_evidence(root,ref)
        if item.get('eta_date'):
            date.fromisoformat(item['eta_date'])
            if not item.get('eta_basis'):raise ValueError('Calendar ETA requires capacity/dependency basis')

def svg_bar(percent,label,color=None):
    percent=max(0,min(100,float(percent)));text=f'{percent:g}%';safe=html.escape(label)
    color=color or ('#34d399' if percent==100 else '#fbbf24' if percent>=85 else '#60a5fa' if percent else '#64748b')
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="220" height="24" role="img" aria-label="{safe}: {text}"><title>{safe}: {text}</title><rect width="220" height="24" rx="6" fill="#162235"/><rect width="{percent*1.6:g}" height="24" rx="6" fill="{color}"/><text x="192" y="16" text-anchor="middle" font-family="sans-serif" font-size="11" fill="#edf2ff">{text}</text></svg>\n'

def stale_outputs(root,outputs):
    return [n for n,t in outputs.items() if not (root/n).is_file() or (root/n).read_text(encoding='utf-8')!=t]

def source_fingerprints(root):
    raw=subprocess.check_output(['git','-C',str(root),'ls-files','-co','--exclude-standard','-z']).decode('utf-8')
    result={}
    for name in sorted(set(filter(None,raw.split('\0')))):
        if re.search(r'\.tmp\d*$',name) or name.startswith(('docs/','artifacts/')) or '__pycache__' in Path(name).parts or name.endswith('.pyc'):continue
        if name.startswith(('native/','apps/','packages/','tests/','scripts/')) or name in {'CMakeLists.txt','CMakePresets.json','package.json','AGENTS.md','CONTRIBUTING.md','CLAUDE.md','GEMINI.md','Start-MTerm.ps1','.github/copilot-instructions.md','.github/pull_request_template.md'}:
            p=root/name
            if p.is_file():result[name]=hashlib.sha256(p.read_bytes().replace(b'\r\n',b'\n')).hexdigest()
    return result

def catalog_digest(catalog):
    return hashlib.sha256(json.dumps(catalog,sort_keys=True,separators=(',',':')).encode('utf-8')).hexdigest()

def check_review(review,catalog,current):
    if review.get('catalog_sha256')!=catalog_digest(catalog):raise ValueError('Assessment changed without explicit metric review')
    if review.get('source_fingerprints')!=current:
        changed=[n for n in sorted(set(current)|set(review.get('source_fingerprints',{}))) if current.get(n)!=review.get('source_fingerprints',{}).get(n)]
        raise ValueError('Source changes require reviewed metric update: '+', '.join(changed[:15]))

def main():
    from render_roadmap import render
    parser=argparse.ArgumentParser(description=__doc__)
    for flag in ('check','write','refresh'):parser.add_argument('--'+flag,action='store_true')
    parser.add_argument('--change-id');parser.add_argument('--review-note');parser.add_argument('--items',default='')
    args=parser.parse_args()
    catalog=json.loads((ROOT/CATALOG).read_text(encoding='utf-8'));index=json.loads((ROOT/INDEX).read_text(encoding='utf-8'))
    validate(catalog,index,ROOT)
    if hashlib.sha256((ROOT/'docs/MASTER_TODO_100_PERCENT.md').read_bytes()).hexdigest()!=index['source_sha256']:raise ValueError('Master backlog changed; reconcile the index explicitly')
    current=source_fingerprints(ROOT);review_file=ROOT/'docs/roadmap/review.json'
    if args.refresh:
        ids=[i.strip() for i in args.items.split(',') if i.strip()]
        if not args.change_id or not args.review_note or not ids:raise ValueError('Refresh needs --change-id, --review-note and --items')
        if set(ids)-{i['id'] for i in catalog['items']}:raise ValueError('Unknown reviewed ID')
        if not (ROOT/'docs/ai/changes'/f'{args.change_id}.json').is_file():raise ValueError('Missing change record')
        old=json.loads(review_file.read_text(encoding='utf-8')) if review_file.exists() else {}
        now=datetime.now(timezone.utc).isoformat();catalog['assessed_at']=now
        (ROOT/CATALOG).write_text(json.dumps(catalog,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
        event={'reviewed_at':now,'change_id':args.change_id,'reviewed_ids':ids,'note':args.review_note,'changed_sources':[n for n in sorted(set(current)|set(old.get('source_fingerprints',{}))) if current.get(n)!=old.get('source_fingerprints',{}).get(n)],'source_fingerprints':current,'catalog_sha256':catalog_digest(catalog)}
        review_file.write_text(json.dumps(event,indent=2)+'\n',encoding='utf-8')
        with (ROOT/'docs/roadmap/reviews.jsonl').open('a',encoding='utf-8') as stream:stream.write(json.dumps({k:v for k,v in event.items() if k!='source_fingerprints'})+'\n')
    outputs=render(catalog,index)
    if args.refresh or args.write:
        for name,text in outputs.items():
            p=ROOT/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text,encoding='utf-8')
    if args.check:
        if not review_file.is_file():raise ValueError('Roadmap has not been reviewed')
        review=json.loads(review_file.read_text(encoding='utf-8'))
        check_review(review,catalog,current)
        stale=stale_outputs(ROOT,outputs)
        if stale:raise ValueError('Generated roadmap stale: '+', '.join(stale[:10]))
    print(json.dumps(summarize(catalog['items']),indent=2));print('Roadmap verified' if args.check else 'Roadmap rendered/validated')

if __name__=='__main__':
    try:main()
    except (ValueError,KeyError,OSError,json.JSONDecodeError) as error:
        print('ROADMAP ERROR: '+str(error),file=sys.stderr);sys.exit(1)
