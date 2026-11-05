#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build a self-contained Pages artifact using exactly one firmware package."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = "9de1ef053a3c4bf56f16f05b5f0de193f4608a1d"
def main():
    source = Path(os.environ.get("FM1_EMU_SOURCE", ROOT / "build/deps/fm1-emu"))
    if not source.exists():
        source.parent.mkdir(parents=True,exist_ok=True)
        subprocess.run(["git","clone","https://github.com/joakimeriksson/fm1-emu",str(source)],check=True)
        subprocess.run(["git","-C",str(source),"checkout","--detach",REV],check=True)
    if subprocess.check_output(["git","-C",str(source),"rev-parse","HEAD"],text=True).strip() != REV:
        raise SystemExit("Unexpected fm1-emu revision")
    if subprocess.check_output(["git","-C",str(source),"status","--porcelain"],text=True).strip():
        raise SystemExit("fm1-emu source has local changes")
    site = ROOT / "build/site"
    if site.exists(): shutil.rmtree(site)
    shutil.copytree(ROOT / "site",site)
    subprocess.run(["bash",str(ROOT / "tools/build_wasm.sh")],check=True)
    shutil.copy2(ROOT / "build/web/astro.wasm",site / "astro.wasm")
    subprocess.run(["bash",str(source / "web/build.sh"),str(site / "fm1-emu")],check=True)
    shutil.copy2(source / "LICENSE",site / "fm1-emu/LICENSE")
    # Keep the upstream emulator intact; replace its general catalog instructions
    # with a clear ASTRA wrapper without inventing a different emulator.
    html_path = site / "fm1-emu/index.html"
    page = html_path.read_text().replace('<title>FM-1 Emulator</title>', '<title>ASTRA FM-1 · Real firmware emulator</title>')
    page = page.replace('</head>', '''<style>
    .library,.guide,footer.site,.devices,.sitenav,.more{display:none!important}
    .astra-guide{max-width:1280px;margin:auto;padding:16px 24px;color:#c9dfab}
    .astra-guide a{color:inherit}
    </style></head>''')
    page = page.replace('<main>', '''<div class="astra-guide"><a href="../">ASTRA companion</a> ·
    Actual ASTRA development firmware in fm1-emu by Joakim Eriksson (MIT).
    Press Start, then Space / PLAY to hear the temporary test tone. Arrow Left/Right or SELECT changes days.
    The initial date is a fixture, not current time. Only PLAY and SELECT are implemented by this prototype.</div><main>''',1)
    html_path.write_text(page)
    (site / "firmware").mkdir()
    pkg = ROOT / "build/native/astra-dev.fwsc"
    sha = hashlib.sha256(pkg.read_bytes()).hexdigest()
    # Require smoke evidence for precisely this binary, not an earlier passing build.
    report = json.loads((ROOT / "build/emulator/report.json").read_text())
    if report["package_sha256"] != sha: raise SystemExit("Firmware changed since smoke validation")
    shutil.copy2(pkg,site / "firmware/astra-dev.fwsc")
    subprocess.run(["python3",str(ROOT / "tools/source_archive.py")],check=True)
    archive = ROOT / "build/astra-source.tar.gz"
    shutil.copy2(archive,site / "astra-source.tar.gz")
    shutil.copy2(ROOT / "build/platform/SDK-LICENSE",site / "SDK-LICENSE")
    catalog = dict(default="astra",firmwares=[dict(id="astra",name="ASTRA FM-1 (engineering prototype)",
        version="dev",author="ASTRA contributors",license="GPL-3.0-only",repo="https://github.com/MichaelAPerry/ASTRA-FM-1",
        about="Offline solar longitude and temporary test signal. Not hardware-tested.",package="../firmware/astra-dev.fwsc")])
    (site / "fm1-emu/firmwares.json").write_text(json.dumps(catalog,indent=2)+"\n")
    (site / "fm1-emu/smk37/firmwares.json").write_text('{"firmwares":[]}\n')
    manifest = dict(commit=subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT,text=True).strip(),
        dirty=bool(subprocess.check_output(["git","status","--porcelain"],cwd=ROOT,text=True).strip()),
        package="firmware/astra-dev.fwsc",package_sha256=sha,emulator_commit=REV,
        platform_commit="f77bf03283bbb2c05294b01cdad68a30b410f93d",sdk_commit="d179b4484759423312073f5fbb232501aa491047",
        compiler="jieli-linux-toolchains-20260730.1",status="emulator-tested; not hardware-tested",
        source_archive="astra-source.tar.gz",source_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),
        smoke=report)
    (site / "manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    for name in ("LICENSE","THIRD_PARTY.md"):
        shutil.copy2(ROOT / name,site / name)
    shutil.copy2(ROOT / "docs/VALIDATION.md",site / "VALIDATION.md")
    (site / ".nojekyll").touch()
    files = sorted("./"+p.relative_to(site).as_posix() for p in site.rglob("*") if p.is_file())
    cache_key = hashlib.sha256(b"".join(p.read_bytes() for p in sorted(site.rglob("*")) if p.is_file())).hexdigest()[:16]
    sw = 'const CACHE='+json.dumps("astra-"+cache_key)+';\nconst ASSETS='+json.dumps(["./"]+files)+';\n'
    sw += '''self.addEventListener('install', e => {e.waitUntil(caches.open(CACHE).then(c => c.addAll(ASSETS)).then(() => self.skipWaiting()));});
self.addEventListener('activate', e => e.waitUntil((async()=>{
  for(const k of await caches.keys()) if(k.startsWith('astra-') && k!==CACHE) await caches.delete(k);
  await self.clients.claim();
  for(const c of await self.clients.matchAll()) c.postMessage('offline-ready');
})()));
self.addEventListener('fetch', e => {
  if(e.request.method!=='GET' || new URL(e.request.url).origin!==self.location.origin) return;
  const url=new URL(e.request.url);
  if(url.pathname.endsWith('/')) url.pathname+='index.html';
  e.respondWith(caches.open(CACHE).then(c=>c.match(url.href,{ignoreSearch:true})).then(r=>r||fetch(e.request)));
});
'''
    (site / "sw.js").write_text(sw)
    print("Site:",site,"package",sha,"cache",cache_key)
if __name__ == "__main__": main()
