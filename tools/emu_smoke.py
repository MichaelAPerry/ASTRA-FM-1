#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Boot the real package, verify date->calculation->sound plus PLAY stop and LCD transitions."""
import array
import hashlib
import json
import os
from pathlib import Path
import subprocess
import wave
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]
out = ROOT / "build/emulator"
out.mkdir(parents=True, exist_ok=True)
(out / "work").mkdir(exist_ok=True)
# Remove previous results so a failed run cannot pass on stale evidence.
for name in ("audio.wav", "report.json", "work/lcd_700.bmp", "work/lcd_1700.bmp", "work/lcd_2300.bmp"):
    (out / name).unlink(missing_ok=True)
with (out / "boot.log").open("w") as log:
    subprocess.run([os.environ.get("FM1_EMU_BIN", "fm1-emu"), "--fwsc", str(ROOT / "build/native/astra-dev.fwsc"),
                    "--ms", "2600", "--keys", "300:PLAY,1000:enc0:+30,1900:PLAY",
                    "--snap", "700,1700,2300", "--wav", "audio.wav"], cwd=out, stdout=log,
                   stderr=subprocess.STDOUT, check=True, timeout=240)
with wave.open(str(out / "audio.wav")) as w:
    rate, channels, frames = w.getframerate(), w.getnchannels(), w.getnframes()
    assert w.getsampwidth() == 2 and channels == 2
    samples = array.array("h", w.readframes(frames))[::channels]
def window(a, b):
    return samples[int(a*rate):int(b*rate)]
def rms(a):
    assert len(a) > 1000, "missing audio window"
    return (sum(x*x for x in a)/len(a))**.5
def frequency(a):
    crossings = [i for i in range(1,len(a)) if a[i-1] <= 0 < a[i]]
    assert len(crossings) > 10
    return rate*(len(crossings)-1)/(crossings[-1]-crossings[0])
assert frames > 90000 and rate == 44100
first, changed, stopped = window(.5,.8), window(1.5,1.8), window(2.25,2.4)
f1, f2 = frequency(first), frequency(changed)
# Independent expected fixture values: J2000 sun about 280.38 deg; day 30 about 310.92 deg.
assert abs(f1-500) < 2, ("initial fixture pitch", f1)
assert abs(f2-530) < 2, ("date encoder did not change the solar pitch", f2)
assert rms(first) > 100 and rms(changed) > 100
assert rms(stopped) < 1, "PLAY stop left audio running"
screens = [Image.open(out / f"work/lcd_{t}.bmp").convert("RGB") for t in (700,1700,2300)]
for im in screens:
    assert im.size == (240,240) and len(im.getcolors(65536)) > 1
# Only text areas for the calculated longitude/frequency and transport respectively.
assert screens[0].crop((0,100,240,166)).tobytes() != screens[1].crop((0,100,240,166)).tobytes()
assert screens[1].crop((0,176,240,194)).tobytes() != screens[2].crop((0,176,240,194)).tobytes()
report = dict(frames=frames, first_hz=f1, changed_hz=f2, stopped_rms=rms(stopped),
              package_sha256=hashlib.sha256((ROOT / "build/native/astra-dev.fwsc").read_bytes()).hexdigest())
(out / "report.json").write_text(json.dumps(report,indent=2)+"\n")
print("PASS", json.dumps(report))
