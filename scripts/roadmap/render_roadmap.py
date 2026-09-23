# SPDX-License-Identifier: MIT
# AI-Change: 2026-09-23-workbench-roadmap
"""Dependency-free GitHub Markdown/SVG and offline searchable HTML views."""
import html
import json
from pathlib import Path
from roadmap import STAGES,COLORS,SQUARES,pct,remaining,eta_label,summarize,svg_bar

def render(catalog,index):
    meta={i['id']:i for i in index['requirements']+catalog.get('additional_requirements',[])}
    groups={}
    for row in catalog['items']:groups.setdefault(meta[row['id']]['section'],[]).append(row)
    total=summarize(catalog['items']);assessment=catalog['assessed_at'];outputs={}
    for stage,value in STAGES.items():outputs[f'docs/roadmap/bars/{stage}.svg']=svg_bar(value,stage,COLORS[stage])
    for stage,value in STAGES.items():outputs[f'docs/roadmap/bars/blocked-{stage}.svg']=svg_bar(value,'Blocked '+stage,COLORS['blocked'])
    outputs['docs/roadmap/bars/overall.svg']=svg_bar(total['percent'],'Overall scoped delivery')
    overview=['# MTerm roadmap and delivery dashboard','',f'**Assessed {assessment} · {total["percent"]:.2f}% weighted delivery maturity**','',
        '![Overall completion](roadmap/bars/overall.svg)','',f'**{total["included"]} scored entries · {total["complete"]} complete · {total["blocked"]} blocked · {total["excluded"]} explicitly excluded historical/legend entries.**','',
        'This is a reviewed planning model, **not release readiness, code coverage, probability of completion, or the percent of every upstream app implemented**. Native features require native evidence; inherited Electron checkmarks do not confer credit.','',
        'Calendar ETA: **Unscheduled** until capacity, dependencies and a maintained schedule exist. Every feature lists an approximate **remaining engineer-hour range**, not elapsed AI runtime. Estimates are low-confidence scope assumptions, not promises.','',
        f'Naive additive remaining effort: **{total["remaining_hours"][0]:,.0f}–{total["remaining_hours"][1]:,.0f} engineer-hours**. Shared implementation/overlap may reduce it; risk/rework may increase it. Do not convert this into a delivery date using invented staffing.','',
        '[Every feature, percentage, evidence and effort](roadmap/FEATURES.md) · [Offline searchable dashboard](roadmap/index.html) · [Metric rules](roadmap/METRICS.md) · [Source projects](SOURCE_PROJECT_FEATURE_MATRIX.md)','',
        '## Status key','', '⬜ 0% not started · 🟪 10% planned · 🟦 40% implementation · 🟦 65% integrated · 🟨 85% validated with acceptance gaps · 🟩 100% complete · 🟥 blocked (keeps earned percentage).','',
        '## Areas','', '| Area | Progress | Complete / scope | Remaining effort* | ETA |','|---|---|---:|---:|---|']
    full=['# Every MTerm roadmap item','',f'Assessed {assessment}. Generated from `progress.json`; do not hand-edit.','',
        'Percentages use the evidence rubric. **ETA is unscheduled**, not a promised date. Remaining ranges are low-confidence engineer-hour assumptions. Original requirement wording and IDs are retained; notes qualify native scope.','']
    html_groups=[];records=[]
    for n,(section,rows) in enumerate(groups.items()):
        report=summarize(rows);slug=f'area-{n:02d}';outputs[f'docs/roadmap/bars/{slug}.svg']=svg_bar(report['percent'],section)
        lo,hi=report['remaining_hours']
        overview.append(f'| [{section}](roadmap/FEATURES.md#{slug}) | ![{report["percent"]:g}%](roadmap/bars/{slug}.svg) | {report["complete"]} / {report["included"]} | {lo:g}–{hi:g} h | Unscheduled |')
        full += [f'<a id="{slug}"></a>',f'## {section}','',f'![Area progress](bars/{slug}.svg)','', '| ID / feature | Delivery | Remaining h | ETA | Scope / evidence |','|---|---|---:|---|---|']
        cards=[]
        for row in rows:
            m=meta[row['id']];stage=row['stage'];p=pct(row);state='blocked' if row.get('blocker') else stage
            hours='N/A' if stage=='excluded' else '–'.join(f'{v:g}' for v in remaining(row))
            bar_name='blocked-'+stage if row.get('blocker') else stage
            progress='N/A · excluded' if stage=='excluded' else f'![{p}%](bars/{bar_name}.svg) {SQUARES[state]} {state.replace("_"," ")}'
            unique=list(dict.fromkeys(row.get('evidence',[])+row.get('tests',[])))
            links='; '.join(f'[{Path(ref.split("::")[0]).name}](../../{ref.split("::")[0]})' for ref in unique)
            note=row['note']+(' BLOCKED: '+row['blocker'] if row.get('blocker') else '')
            escape=lambda text:text.replace('|','&#124;').replace('\n',' ')
            full.append(f'| **{row["id"]}** — {escape(m["requirement"])} | {progress} | {hours} | {eta_label(row)} | {escape(note)} {links} |')
            refs=''.join(f'<li><a href="../../{html.escape(ref.split("::")[0],quote=True)}">{html.escape(ref)}</a></li>' for ref in unique)
            cards.append(f'<article data-search="{html.escape((row["id"]+" "+m["requirement"]+" "+section).lower(),quote=True)}" data-state="{state}"><div class="row"><small>{row["id"]}</small><span>{html.escape(state)}</span></div><h3>{html.escape(m["requirement"])}</h3><div class="track"><i style="width:{p}%;background:{COLORS[state]}"></i></div><p>{"N/A" if stage=="excluded" else str(p)+"%"} · {hours} engineer-h · ETA: {html.escape(eta_label(row))}</p><details><summary>Evidence and assessment</summary><p>{html.escape(note)}</p><ul>{refs}</ul></details></article>')
            records.append({'id':row['id'],'percent':None if stage=='excluded' else p,'stage':stage,'blocked':bool(row.get('blocker')),'weight':row['weight'],'remaining_hours':remaining(row)})
        full.append('');html_groups.append(f'<section><h2>{html.escape(section)} <small>{report["percent"]:g}%</small></h2><div class="grid">'+''.join(cards)+'</div></section>')
    overview += ['', '## Required maintenance','', 'Every human/AI change must review affected IDs, stages, evidence and estimates, including explicit no-change reviews. Run tooling tests, refresh with a change ID/note/affected IDs, then `--check`. The native build and package gates reject stale source-review fingerprints and outputs. [Instructions](roadmap/METRICS.md).','', '*Effort is not ETA. Stage, scope and weight changes are versioned and must be disclosed.*','']
    outputs['docs/ROADMAP.md']='\n'.join(overview);outputs['docs/roadmap/FEATURES.md']='\n'.join(full)
    outputs['docs/roadmap/summary.json']=json.dumps({'assessed_at':assessment,'overall':total,'items':records},indent=2)+'\n'
    style='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>MTerm roadmap</title><style>
:root{color-scheme:dark;font:15px system-ui;background:#090e18;color:#e6edf8}body{max-width:1280px;margin:auto;padding:32px}h1{font-size:34px}h2{margin:36px 0 16px}h3{font-size:15px;line-height:1.4}small,p,summary{color:#9aacc5}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(310px,1fr));gap:12px}article{background:#111c2e;border:1px solid #283951;border-radius:12px;padding:16px}.row{display:flex;justify-content:space-between;gap:8px;font-size:11px}.track{height:9px;background:#23334d;border-radius:8px;overflow:hidden}.track i{display:block;height:100%}input,select{font:inherit;background:#142339;color:#edf4ff;border:1px solid #3a5274;border-radius:8px;padding:12px}input{min-width:260px;flex:1}.filters{display:flex;gap:12px;position:sticky;top:0;background:#090e18;padding:14px 0;flex-wrap:wrap}a{color:#8db4ff}header{padding:20px;background:#12223a;border-radius:14px}details{font-size:12px}li{overflow-wrap:anywhere}article[hidden],section[hidden]{display:none}</style>'''
    heading=f'<header><h1>MTerm · {total["percent"]:.2f}% scoped delivery</h1><p>Assessed {html.escape(assessment)} · {total["included"]} scored entries · {total["complete"]} complete. Not release readiness.</p><p>ETAs are unscheduled. Effort is low-confidence engineer-hours, not promises. <a href="METRICS.md">Method.</a></p></header>'
    filters='<div class="filters"><input id="search" aria-label="Search roadmap" placeholder="Search feature, ID or area"><select id="status" aria-label="Filter status"><option value="active">Active requirements</option><option value="">All including historical entries</option>'+''.join(f'<option>{key}</option>' for key in [*STAGES,'blocked','excluded'])+'</select></div>'
    js='''<script>const search=document.querySelector('#search'),status=document.querySelector('#status');function filter(){const q=search.value.trim().toLowerCase();document.querySelectorAll('article').forEach(c=>c.hidden=!c.dataset.search.includes(q)||(status.value==='active'?c.dataset.state==='excluded':status.value&&c.dataset.state!==status.value));document.querySelectorAll('section').forEach(s=>s.hidden=!s.querySelector('article:not([hidden])'));}search.addEventListener('input',filter);status.addEventListener('change',filter);filter();</script></html>'''
    outputs['docs/roadmap/index.html']=style+heading+filters+''.join(html_groups)+js
    return outputs
