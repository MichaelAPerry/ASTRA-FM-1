#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The site's screen captures, from the real package in fm1-emu (never a host render): the clock and a birth chart
# entered with the knobs, three keys held, then each page in turn. Writes site/media/screens/*.png (240 x 240 x 2).
#   tools/site_shots.sh PACKAGE.fwsc OUT_DIR FM1_EMU_BIN
set -eu
U="usage: tools/site_shots.sh PACKAGE.fwsc OUT_DIR FM1_EMU_BIN"
abs() { case "$1" in */*) echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")";; *) echo "$1";; esac; }
PKG="$(abs "${1:?$U}")"; OUT="${2:?$U}"; EMU="$(abs "${3:?$U}")"; ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$OUT/work" && cd "$OUT"
# GLO: 2026-11-12 14:00 UTC, PLAY sets it; GLO: born 1987-03-14 06:25; GLO: 52.0 N 0.1 W, UTC+1; HOME, keys; pages
"$EMU" --fwsc "$PKG" --ms 6300 --snap 2700,3050,3350,3750,4150,4650,5650,6200 --keys \
'300:GLO,400:K1:+26,500:K2:+10,600:K3:+11,700:K4:+2,900:PLAY,1100:GLO,1200:K1:-3,1300:K2:+2,1400:K3:+13,1500:K4:-6,1600:SELECT:+25,'\
'1700:GLO,1800:K1:+26,1900:K2:-1,2000:K3:+4,2100:HOME,2200-3500:n7,2250-3500:n14,2300-3500:n19,2400:ALGO:+24,'\
'2800:EDIT,3100:ENV,3400:LFO,3800:FX,4200:SEQ,4700:SEQ,5700:GLO' > shots.log 2>&1
python3 - "$ROOT/site/media/screens" <<'PY'
import sys
from PIL import Image
names = {2700: "wheel", 3050: "operator", 3350: "envelope", 3750: "motion", 4150: "effects", 4650: "transits", 5650: "month", 6200: "clock"}
for t, n in names.items():
    Image.open(f"work/lcd_{t}.bmp").convert("RGB").resize((480, 480), Image.NEAREST).save(f"{sys.argv[1]}/{n}.png", optimize=True)
    print("site_shots:", n)
PY
