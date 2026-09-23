# SPDX-License-Identifier: MIT
# AI-Change: 2026-09-23-workbench-roadmap
import copy
import importlib.util
import tempfile
import unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location("roadmap",Path(__file__).with_name("roadmap.py"))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class RoadmapTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.root=Path(self.tmp.name)
        (self.root/'proof.txt').write_text('test evidence',encoding='utf-8')
        self.index={'requirements':[{'id':'REQ-A','requirement':'First','section':'Core'}, {'id':'REQ-B','requirement':'Second','section':'Core'}]}
        def item(id,stage,weight):return {'id':id,'stage':stage,'weight':weight,'estimate_hours':[4,12],'estimate_basis':'Planning assumption; uncalibrated','reviewed_at':'2026-09-23T00:00:00Z','evidence':['proof.txt'],'tests':['proof.txt'],'note':'Acceptance scope reviewed','blocker':None,'eta_date':None,'eta_basis':None}
        self.catalog={'items':[item('REQ-A','complete',1),item('REQ-B','implementation',3)],'additional_requirements':[]}
    def test_weighted_completion_not_checkbox_ratio(self):
        report=module.summarize(self.catalog['items']);self.assertEqual(report['percent'],55.0)
    def test_remaining_effort_is_explicit_range(self):
        self.assertEqual(module.remaining(self.catalog['items'][1]),[2.4,7.2])
    def test_unknown_eta_not_invented(self):
        self.assertEqual(module.eta_label(self.catalog['items'][1]),'Unscheduled')
    def test_missing_requirement_rejected(self):
        self.catalog['items'].pop()
        with self.assertRaises(ValueError):module.validate(self.catalog,self.index,self.root)
    def test_duplicate_requirement_rejected(self):
        self.catalog['items'].append(copy.deepcopy(self.catalog['items'][0]))
        with self.assertRaises(ValueError):module.validate(self.catalog,self.index,self.root)
    def test_complete_without_test_rejected(self):
        self.catalog['items'][0]['tests']=[]
        with self.assertRaises(ValueError):module.validate(self.catalog,self.index,self.root)
    def test_missing_evidence_rejected(self):
        self.catalog['items'][0]['evidence']=['absent.txt']
        with self.assertRaises(ValueError):module.validate(self.catalog,self.index,self.root)
    def test_evidence_escape_rejected(self):
        self.catalog['items'][0]['evidence']=['../proof.txt']
        with self.assertRaises(ValueError):module.validate(self.catalog,self.index,self.root)
    def test_eta_requires_basis(self):
        self.catalog['items'][1]['eta_date']='2026-10-01'
        with self.assertRaises(ValueError):module.validate(self.catalog,self.index,self.root)
    def test_excluded_legend_does_not_inflate_percent(self):
        row=copy.deepcopy(self.catalog['items'][0]);row.update(stage='excluded',weight=0)
        result=module.summarize([row,self.catalog['items'][1]])
        self.assertEqual(result['percent'],40);self.assertEqual(result['excluded'],1)
    def test_not_started_has_no_false_credit(self):
        row=copy.deepcopy(self.catalog['items'][1]);row['stage']='not_started'
        self.assertEqual(module.summarize([row])['percent'],0)
    def test_valid_catalog(self):
        module.validate(self.catalog,self.index,self.root)
    def test_bar_contains_text_and_safe_markup(self):
        svg=module.svg_bar(40,'<unsafe>')
        self.assertIn('40%',svg);self.assertNotIn('<unsafe>',svg)
    def test_stale_output_detected(self):
        (self.root/'a.md').write_text('old',encoding='utf-8')
        self.assertEqual(module.stale_outputs(self.root,{'a.md':'new'}),['a.md'])
    def test_nonfinite_weight_rejected(self):
        self.catalog['items'][0]['weight']=float('nan')
        with self.assertRaises(ValueError):module.validate(self.catalog,self.index,self.root)
    def test_nonfinite_effort_rejected(self):
        self.catalog['items'][0]['estimate_hours']=[1,float('inf')]
        with self.assertRaises(ValueError):module.validate(self.catalog,self.index,self.root)
    def test_catalog_change_requires_explicit_review(self):
        original=module.catalog_digest(self.catalog)
        self.catalog['items'][1]['stage']='integrated'
        with self.assertRaises(ValueError):module.check_review({'catalog_sha256':original,'source_fingerprints':{}},self.catalog,{})
    def test_source_change_requires_explicit_review(self):
        review={'catalog_sha256':module.catalog_digest(self.catalog),'source_fingerprints':{'a.cpp':'old'}}
        with self.assertRaises(ValueError):module.check_review(review,self.catalog,{'a.cpp':'new'})
    def test_review_matches_catalog_and_sources(self):
        review={'catalog_sha256':module.catalog_digest(self.catalog),'source_fingerprints':{'a.cpp':'same'}}
        module.check_review(review,self.catalog,{'a.cpp':'same'})
    def test_render_keeps_every_requirement_and_original_wording(self):
        from render_roadmap import render
        self.catalog['assessed_at']='2026-09-23T00:00:00Z'
        result=render(self.catalog,self.index)
        for item in self.index['requirements']:
            self.assertIn(item['id'],result['docs/roadmap/FEATURES.md'])
            self.assertIn(item['requirement'],result['docs/roadmap/FEATURES.md'])
        self.assertEqual(result['docs/roadmap/index.html'].count('<article '),2)
    def test_rendered_html_escapes_untrusted_feature_text(self):
        from render_roadmap import render
        self.catalog['assessed_at']='2026-09-23T00:00:00Z'
        self.index['requirements'][0]['requirement']='<script>unsafe()</script>'
        result=render(self.catalog,self.index)['docs/roadmap/index.html']
        self.assertNotIn('<script>unsafe()',result)
        self.assertIn('&lt;script&gt;',result)
    def test_stage_mapping_cannot_be_changed_silently_in_catalog(self):
        self.catalog['stage_percentages']={'not_started':100}
        with self.assertRaises(ValueError):module.validate(self.catalog,self.index,self.root)
    def test_complete_item_has_no_future_eta(self):
        self.assertEqual(module.eta_label(self.catalog['items'][0]),'Complete')
    def test_excluded_item_has_no_eta(self):
        self.catalog['items'][0]['stage']='excluded'
        self.assertEqual(module.eta_label(self.catalog['items'][0]),'Not applicable')
    def test_outputs_deterministic(self):
        self.assertEqual(module.svg_bar(65,'Feature'),module.svg_bar(65,'Feature'))

if __name__=='__main__':unittest.main()
