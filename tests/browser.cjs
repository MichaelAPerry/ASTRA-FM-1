// SPDX-License-Identifier: GPL-3.0-only
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const {createRequire} = require('node:module');
const {spawn} = require('node:child_process');
let playwright;
try { playwright = require('playwright'); }
catch { playwright = createRequire(path.resolve('build/browser/package.json'))('playwright'); }
const {chromium} = playwright;
const root = path.resolve(__dirname,'..');
// Match GitHub project Pages, including its /ASTRA-FM-1/ prefix.
const serverRoot = root+'/build/browser-server';
fs.mkdirSync(serverRoot,{recursive:true});
const mount = serverRoot+'/ASTRA-FM-1';
if(!fs.existsSync(mount)) fs.symlinkSync(root+'/build/site',mount,'dir');
const base = 'http://127.0.0.1:8765/ASTRA-FM-1/';
const server = spawn('python3',['-m','http.server','8765','--bind','127.0.0.1','--directory',serverRoot],{stdio:'ignore'});
(async()=>{
  let browser;
  try {
    for(let i=0;i<50;i++) {
      try { if((await fetch(base+'manifest.json')).ok) break; } catch {}
      await new Promise(r=>setTimeout(r,100));
      if(i===49) throw new Error('HTTP server did not start');
    }
    browser=await chromium.launch({headless:true,...(process.env.CHROMIUM_PATH?{executablePath:process.env.CHROMIUM_PATH}:{}),args:['--no-sandbox','--autoplay-policy=no-user-gesture-required']});
    const context=await browser.newContext();
    const errors=[]; context.on('page',p=>p.on('pageerror',e=>errors.push(e.message)));
    const page=await context.newPage();
    await page.goto(base);
    await page.waitForFunction(()=>document.querySelector('#position').textContent.includes('280.376'));
    await page.locator('#utc').fill('2000-01-31T12:00');
    await page.locator('#sky button').click();
    await page.waitForFunction(()=>document.querySelector('#position').textContent.includes('310.917'));
    await page.evaluate(async()=>{await navigator.serviceWorker.ready; if(!navigator.serviceWorker.controller) await new Promise(r=>navigator.serviceWorker.addEventListener('controllerchange',r,{once:true}));});
    const meta=await page.evaluate(async()=>{
      const m=await(await fetch('manifest.json')).json();
      const pkg=await(await fetch(m.package)).arrayBuffer();
      const digest=[...new Uint8Array(await crypto.subtle.digest('SHA-256',pkg))].map(x=>x.toString(16).padStart(2,'0')).join('');
      if(digest!==m.package_sha256) throw Error('package hash mismatch');
      const src=await(await fetch(m.source_archive)).arrayBuffer();
      const srcHash=[...new Uint8Array(await crypto.subtle.digest('SHA-256',src))].map(x=>x.toString(16).padStart(2,'0')).join('');
      if(srcHash!==m.source_sha256) throw Error('matching source archive hash mismatch');
      const catalog=await(await fetch('fm1-emu/firmwares.json')).json();
      if(catalog.firmwares.length!==1 || catalog.firmwares[0].package!=='../'+m.package) throw Error('emulator/download mismatch');
      return m;
    });
    await page.screenshot({path:root+'/build/browser-online.png',fullPage:true});
    // Genuine offline reload, not just a preloaded calculation while online.
    await context.setOffline(true);
    await page.reload();
    await page.waitForFunction(()=>document.querySelector('#position').textContent.includes('280.376'));
    await page.locator('#utc').fill('2024-04-08T18:00');
    await page.locator('#sky button').click();
    await page.waitForFunction(()=>!document.querySelector('#position').textContent.includes('280.376'));
    const emu=await context.newPage();
    await emu.goto(base+'fm1-emu/');
    await emu.locator('#fw').selectOption('astra');
    const before=await emu.locator('#panel').evaluate(c=>c.toDataURL());
    await emu.locator('#start').click();
    await emu.waitForFunction(()=>document.querySelector('#status').textContent.includes('MHz'),{},{timeout:60000});
    await emu.waitForFunction(old=>document.querySelector('#panel').toDataURL()!==old,before,{timeout:60000});
    await emu.waitForTimeout(4000); // allow a complete firmware LCD update on the worklet
    await emu.screenshot({path:root+'/build/browser-emulator-offline.png',fullPage:true});
    const state=await emu.locator('#status').textContent();
    assert(!/fault|error|failed/i.test(state),state);
    assert.deepEqual(errors,[]);
    await page.setViewportSize({width:390,height:844});
    await page.screenshot({path:root+'/build/browser-mobile-offline.png',fullPage:true});
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'mobile layout overflow');
    fs.writeFileSync(root+'/build/browser-report.json',JSON.stringify({offline_calculation:true,offline_real_emulator:true,package_sha256:meta.package_sha256,status:state,page_errors:errors},null,2)+'\n');
    console.log('PASS: shared calculation, package provenance, offline reload and real offline emulator boot, mobile layout');
  } finally { if(browser) await browser.close(); server.kill(); }
})().catch(e=>{console.error(e);process.exitCode=1;});
