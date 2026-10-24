/* Run with a local HTTP server at SSQ_URL (default :8765). Playwright is test-only. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const {chromium} = require(process.env.PLAYWRIGHT_MODULE || 'playwright');
(async () => {
  const out=path.resolve(process.env.SSQ_OUT || 'private/ssq-validation/browser');fs.mkdirSync(out,{recursive:true});
  const browser=await chromium.launch({headless:true,...(process.env.CHROMIUM_EXECUTABLE?{executablePath:process.env.CHROMIUM_EXECUTABLE}:{})});
  try {
    const context=await browser.newContext({timezoneId:'America/Los_Angeles',viewport:{width:1100,height:900}});
    await context.route('https://**/*',route=>route.abort()); // Must work with all remote resources unavailable.
    const page=await context.newPage(),errors=[],missing=[];
    page.on('pageerror',e=>errors.push(String(e)));
    page.on('response',r=>{if(r.url().startsWith('http://127.0.0.1')&&r.status()>=400&&!r.url().endsWith('favicon.ico'))missing.push(r.url())});
    await page.clock.install({time:new Date('2026-10-24T00:01:00+08:00')});
    await page.goto(process.env.SSQ_URL || 'http://127.0.0.1:8765/prototype/index.html#daily');
    await page.evaluate(()=>document.fonts.ready);
    await page.clock.runFor(500);
    const state=()=>page.evaluate(()=>{const s=__nf.S.daily;return {ph:s.ph,sel:s.sel,page:s.page,pages:s.pages,stamp:s.stamp,ssq:s.ssq,basis:s.basis}});
    const tap=async key=>{await page.keyboard.down(key);await page.clock.runFor(80);await page.keyboard.up(key);await page.clock.runFor(400)};
    const shot=async tag=>page.locator('#scr').screenshot({path:path.join(out,tag+'.png')});
    assert.equal((await state()).sel,0);await shot('choices');
    await tap('ArrowDown');assert.equal((await state()).sel,1);await shot('selected');
    await tap('Enter');assert.equal((await state()).ph,'calc');await shot('calculating');
    await page.clock.runFor(8000);assert.equal((await state()).ph,'done');
    const first=await state();assert.equal(first.stamp,'2026.10.24'); // East-8, despite the host timezone.
    assert.deepEqual(first.ssq.red,[10,13,17,19,25,32]);assert.equal(first.ssq.blue,4);await shot('numbers');
    assert.match(first.basis,/灵签 第2签/);assert.match(first.basis,/奇门 12:00 阴遁8局中元/);
    for(let i=1;i<first.pages;i++){await tap('ArrowDown');assert.equal((await state()).page,i);await shot('basis_'+i)}
    await tap('ArrowDown');assert.equal((await state()).page,0);
    await tap('Enter');await tap('Enter');await page.clock.runFor(8000);assert.deepEqual((await state()).ssq,first.ssq);
    await page.reload();await page.evaluate(()=>document.fonts.ready);await page.clock.runFor(500);
    await tap('ArrowDown');await tap('Enter');await page.clock.runFor(8000);assert.deepEqual((await state()).ssq,first.ssq);
    await page.clock.setFixedTime(new Date('2026-10-24T23:59:59+08:00'));
    await tap('Enter');await tap('Enter');await page.clock.runFor(8000);assert.deepEqual((await state()).ssq,first.ssq);
    await page.clock.setFixedTime(new Date('2026-10-25T00:00:00+08:00'));
    await tap('Enter');await tap('Enter');await page.clock.runFor(8000);assert.equal((await state()).stamp,'2026.10.25');assert.notDeepEqual((await state()).ssq.red,first.ssq.red);
    // Old ticket must still complete print -> tear -> readable result.
    await tap('Enter');await tap('ArrowDown');await tap('Enter');await page.clock.runFor(8000);
    assert.equal((await state()).ph,'done');assert.equal((await state()).sel,0);await shot('original_ticket');
    assert.deepEqual(errors,[]);assert.deepEqual(missing,[]);
    fs.writeFileSync(path.join(out,'result.json'),JSON.stringify({status:'PASS',offline:true,timezone:'America/Los_Angeles',east8Date:true,reprint:true,reload:true,midnight:true,errors,missing,seed:first.ssq.seed,red:first.ssq.red,blue:first.ssq.blue},null,2)+'\n');
    console.log('Browser SSQ PASS: offline, actual keys, calculation/printing, pages, reprint, reload, East-8 midnight, original ticket');
  } finally {await browser.close()}
})().catch(e=>{console.error(e);process.exitCode=1});
