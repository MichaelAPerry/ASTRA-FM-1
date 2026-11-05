#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build ASTRA in a private ignored platform staging tree, never in MIKU or upstream."""
import ast
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = "f77bf03283bbb2c05294b01cdad68a30b410f93d"

def main():
    cache = ROOT / "build/deps/dinghy"
    source = Path(os.environ.get("DINGHY_SOURCE", cache))
    if not source.exists():
        cache.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["git", "clone", "https://github.com/zvenson/dinghy", str(cache)], check=True)
        subprocess.run(["git", "-C", str(cache), "checkout", "--detach", REV], check=True)
    head = subprocess.check_output(["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    if head != REV:
        raise SystemExit("Dinghy source must be at " + REV)
    if subprocess.check_output(["git", "-C", str(source), "status", "--porcelain"], text=True).strip():
        raise SystemExit("Dinghy dependency has local changes; refusing to build them")
    stage = ROOT / "build/platform"
    stage.mkdir(parents=True, exist_ok=True)
    for name in ("firmware", "tools", "tests"):
        shutil.copytree(source / name, stage / name, dirs_exist_ok=True)
    for name in ("build.sh", "LICENSE"):
        shutil.copy2(source / name, stage / name)
    for name in ("astro.h", "astro.c"):
        shutil.copy2(ROOT / "shared" / name, stage / "firmware/src" / name)
    shutil.copy2(ROOT / "native/app.c", stage / "firmware/src/dinghy.c")
    path = stage / "tools/build.py"
    script = path.read_text()
    replacements = {
        'SCREEN_NAME = "DINGHY"': 'SCREEN_NAME = "ASTRA FM-1"',
        'DATA_FAMILY = "DINGHY"': 'DATA_FAMILY = "ASTRA"',
        'PRODUCT = BOARD_PREFIX[BOARD] + "0"': 'PRODUCT = "FM-1_ASTRADEV"',
    }
    for before, after in replacements.items():
        if script.count(before) != 1:
            raise SystemExit("Upstream patch mismatch: " + before)
        script = script.replace(before, after)
    path.write_text(script)
    # Upstream merely warns on SDK mismatches. ASTRA deliberately fails closed.
    tree = ast.parse(script)
    hashes = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                  and any(isinstance(t, ast.Name) and t.id == "SDK_SHA256" for t in n.targets))
    sdk = Path(os.environ["AC79_SDK"]) / "cpu/wl82/tools"
    for name, expected in hashes.items():
        if hashlib.sha256((sdk / name).read_bytes()).hexdigest() != expected:
            raise SystemExit("SDK checksum mismatch: " + name)
    env = dict(os.environ)
    env["DINGHY_DEFS"] = '-DSTORE_MAGIC=0x41535452u'
    subprocess.run(["bash", "build.sh"], cwd=stage, env=env, check=True)
    out = ROOT / "build/native"
    out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(stage / "build/dinghy.fwsc", out / "astra-dev.fwsc")
    shutil.copy2(stage / "build/dinghy.bin", out / "astra-dev.bin")
    shutil.copy2(stage / "build/dinghy.dis", out / "astra-dev.dis")

if __name__ == "__main__":
    main()
