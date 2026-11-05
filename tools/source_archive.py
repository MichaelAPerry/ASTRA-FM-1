#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Provide exact authored and staged platform sources alongside development binaries."""
from pathlib import Path
import subprocess
import tarfile
ROOT = Path(__file__).resolve().parents[1]
files = subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT).split(b'\0')
with tarfile.open(ROOT/'build/astra-source.tar.gz','w:gz') as archive:
    for item in files:
        if item:
            path = ROOT/item.decode()
            archive.add(path,arcname='astra/'+path.relative_to(ROOT).as_posix(),recursive=False)
    stage=ROOT/'build/platform'
    def source_only(info):
        return None if '__pycache__' in info.name or info.name.endswith('.pyc') else info
    for name in ('firmware','tools','tests','assets','docs','build.sh','LICENSE','LICENSING.md','README.md','SDK-LICENSE'):
        archive.add(stage/name,arcname='astra-platform/'+name,filter=source_only)
print('Matching sources: build/astra-source.tar.gz')
