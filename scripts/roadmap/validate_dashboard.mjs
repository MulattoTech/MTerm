// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-23-workbench-roadmap
// Development-only local dashboard validation. No model, external web service or native runtime dependency.
import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
import {chromium,expect} from '@playwright/test';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const catalog=JSON.parse(await fs.readFile(path.join(root,'docs/roadmap/progress.json'),'utf8'));
const output=await fs.mkdtemp(path.join(root,'artifacts','roadmap-browser-'));
const browser=await chromium.launch({channel:'msedge',headless:true});
try {
    const context=await browser.newContext({viewport:{width:1440,height:1000}});
    await context.route('**/*',route=>/^https?:/i.test(route.request().url())?route.abort():route.continue());
    const page=await context.newPage();
    await page.goto(pathToFileURL(path.join(root,'docs/roadmap/index.html')).href);
    await expect(page.locator('article')).toHaveCount(catalog.items.length);
    await expect(page.locator('article:visible')).toHaveCount(catalog.items.filter(i=>i.stage!=='excluded').length);
    await page.getByLabel('Search roadmap').fill('REQ-D05E282B6459');
    await expect(page.locator('article:visible')).toHaveCount(1);
    await expect(page.locator('article:visible')).toContainText('Multi-buffer editor tabs');
    await page.getByLabel('Search roadmap').fill('');
    await page.getByLabel('Filter status').selectOption('blocked');
    await expect(page.locator('article:visible')).toHaveCount(catalog.items.filter(i=>Boolean(i.blocker)).length);
    await page.getByLabel('Filter status').selectOption('active');
    await page.screenshot({path:path.join(output,'roadmap-overview.png')});
    await page.getByLabel('Search roadmap').fill('editor');
    await page.screenshot({path:path.join(output,'roadmap-editor-filter.png')});
    await fs.writeFile(path.join(output,'result.json'),JSON.stringify({status:'PASS',rows:catalog.items.length,checks:['all rows rendered','exact ID search','blocked filter','no HTTP requests allowed'],note:'Local HTML rendering; not GitHub Pages deployment'},null,2));
    console.log('ROADMAP_BROWSER_PASS='+output);
} finally {await browser.close();}
