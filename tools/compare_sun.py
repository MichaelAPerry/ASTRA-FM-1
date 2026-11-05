#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline independent reference comparison; astronomy-engine==2.1.19 (MIT)."""
import ctypes
import datetime as dt
import json
from pathlib import Path
import astronomy

ROOT = Path(__file__).resolve().parents[1]
lib = ctypes.CDLL(str(ROOT / "build/libastra.so"))
lib.astra_sun.argtypes = [ctypes.c_int32, ctypes.c_int32]
lib.astra_sun.restype = ctypes.c_int32
epoch = dt.datetime(2000, 1, 1)
samples = []
for year in range(1950, 2051):
    for month in (1, 3, 6, 9, 12):
        for hour in (0, 12, 23):
            date = dt.datetime(year, month, 1, hour, 17, 31)
            delta = date - epoch
            actual = lib.astra_sun(delta.days, delta.seconds) / 1000
            t = astronomy.Time.Make(year, month, 1, hour, 17, 31)
            reference = astronomy.SunPosition(t).elon
            error = abs((actual - reference + 180) % 360 - 180)
            samples.append(dict(utc=date.isoformat()+"Z", millidegrees=round(reference*1000), error_deg=error))
worst = max(samples, key=lambda x: x["error_deg"])
print(f"{len(samples)} comparisons; max error {worst['error_deg']:.6f} deg at {worst['utc']}")
assert worst["error_deg"] < .05, "solar model exceeds prototype's 0.05-degree tolerance"
out = ROOT / "build/sun-comparison.json"
out.write_text(json.dumps(dict(reference="astronomy-engine 2.1.19 SunPosition (apparent ecliptic of date)",
                               count=len(samples), worst=worst), indent=2)+"\n")
