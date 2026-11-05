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
    for name in ('firmware','tools','tests','build.sh','LICENSE'):
        archive.add(stage/name,arcname='astra-platform/'+name)
print('Matching sources: build/astra-source.tar.gz')
