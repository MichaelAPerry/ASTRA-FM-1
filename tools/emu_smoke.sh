#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Adapted from MIKU-FM1 tools/emu_smoke.sh (MIT, same owner).
# Boot an ASTRA package in fm1-emu (github.com/joakimeriksson/fm1-emu) and check, from the real .fwsc:
#   - it boots, draws (screen not one colour) and sounds (audio RMS above a floor);
#   - the controls: from the editor's start (2000-01-01 12:00 UTC, NOT SET) the knobs turn it to 2026-11-05, PLAY sets it;
#   - the UI state: NOT SET before PLAY, SET after, the clock running from 12:00:00 since;
#   - the astronomy: the Sun and Moon longitudes the firmware computed (astra_probe, read with --dump) equal, to the bit,
#     what the host build of the same core/astro gives for the same instant (EXPECT = build/emu_expect); the same for
#     the eight planets' longitude and day motion (each computed in its own main-loop pass, at its own second);
#   - the TEMPORARY test tone: its measured pitch is the note the Moon's sign gives.
# Evidence (WAV, BMPs, log) goes to OUT_DIR.
#   tools/emu_smoke.sh PACKAGE.fwsc PROBE.txt OUT_DIR EXPECT [FM1_EMU_BIN]
set -eu
U="usage: tools/emu_smoke.sh PACKAGE.fwsc PROBE.txt OUT_DIR EXPECT [FM1_EMU_BIN]"
abs() { case "$1" in */*) echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")";; *) echo "$1";; esac; }
PKG="$(abs "${1:?$U}")"; PROBE="$(cat "${2:?$U}")"; OUT="${3:?$U}"; EXPECT="$(abs "${4:?$U}")"; EMU="$(abs "${5:-fm1-emu}")"
mkdir -p "$OUT/work"                            # fm1-emu writes snapshots to work/ (and fails silently without it)
cd "$OUT"
# K1 (enc2) +26 years, K2 (enc3) +10 months, K3 (enc4) +4 days: 2026-11-05 12:00 UTC. PLAY at 1500 ms sets it.
"$EMU" --fwsc "$PKG" --ms 3000 --keys '300:enc2:+26,600:enc3:+10,900:enc4:+4,1500:PLAY' --snap 1300,2800 \
       --wav audio.wav --dump "$PROBE" 32 > boot.log 2>&1
python3 - "$EXPECT" <<'PY'
import array, re, subprocess, sys, wave
from PIL import Image
log = open("boot.log").read()
m = re.search(r"^dump 0x[0-9a-f]+ \(32 words\):\n((?:\s+0x[0-9a-f]+:(?: [0-9a-f]{8})+\n?)+)", log, re.M)
assert m, "no probe dump in boot.log"
w = [int(x, 16) for row in m[1].strip().splitlines() for x in row.split(":")[1].split()]
assert len(w) == 32, f"probe: {len(w)} words"
magic, days, sec, state, sun, moon, note = w[:7]
days -= (days >> 31) << 32                       # int32
assert magic == 0x50525453, f"probe never filled (magic {magic:#x})"
print(f"emu_smoke: probe days {days} sec {sec} state {state} sun {sun:08x} moon {moon:08x} note {note}")
assert days == 20762, f"date: day {days}, want 20762 (2026-11-05): the knobs did not reach the firmware"
assert state == 2, f"state {state}, want 2 (SET): PLAY did not set the clock"
assert 43200 < sec <= 43200 + 2, f"sec {sec}: want 12:00:00 plus the ~1.5 s since PLAY"
want = subprocess.run([sys.argv[1], str(days), str(sec)], check=True, capture_output=True, text=True).stdout.split()
assert [f"{sun:08x}", f"{moon:08x}", str(note)] == want, f"device {sun:08x} {moon:08x} {note}, host {want}"
print(f"emu_smoke: Sun {sun * 360 / 2**32:.4f} deg, Moon {moon * 360 / 2**32:.4f} deg: device == host, bit for bit")
NAMES = ["Mercury", "Venus", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"]
for b in range(8):
    lon, speed, psec = w[7 + 3 * b:10 + 3 * b]
    assert 43200 <= psec <= 43202, f"{NAMES[b]}: computed at sec {psec}"
    want = subprocess.run([sys.argv[1], "planet", str(b), str(days), str(psec)], check=True, capture_output=True, text=True).stdout.split()
    assert [f"{lon:08x}", f"{speed:08x}"] == want, f"{NAMES[b]}: device {lon:08x} {speed:08x}, host {want}"
    sp = (speed - (speed >> 31 << 32)) * 360 / 2**32
    print(f"emu_smoke: {NAMES[b]:7s} {lon * 360 / 2**32:8.4f} deg, {sp:+.4f} deg/day{' (retrograde)' if sp < 0 else ''}: device == host")
print(f"emu_smoke: longest planet pass {w[31]} us (emulator timer: not a device figure)")

with wave.open("audio.wav", "rb") as w:
    rate, ch, n = w.getframerate(), w.getnchannels(), w.getnframes()
    a = array.array("h", w.readframes(n))
rms = (sum(x * x for x in a) / max(len(a), 1)) ** 0.5
tail = a[::ch][-rate // 2:]                      # the last 0.5 s, left channel: well after PLAY, the tone settled
ups = sum(1 for i in range(1, len(tail)) if tail[i - 1] < 0 <= tail[i])
f, f_want = ups * rate / len(tail), 440 * 2 ** ((note - 69) / 12)
print(f"emu_smoke: {n} frames at {rate} Hz, RMS {rms:.0f}, tone {f:.1f} Hz (note {note}: {f_want:.1f} Hz)")
assert n > 10000, "short audio"
assert rms > 10, "silent"
assert abs(f / f_want - 1) < 0.03, "the tone is not the Moon's note"

a, b = (Image.open(f"work/lcd_{t}.bmp").convert("RGB") for t in (1300, 2800))
colours = len(b.getcolors(1 << 16))
state_line = (0, 68, 240, 84)                     # astra.c draw_state(): NOT SET (amber) -> SET (green)
changed = a.crop(state_line).tobytes() != b.crop(state_line).tobytes()
print(f"emu_smoke: {colours} screen colours; state line changed after PLAY: {changed}")
assert colours > 2, "blank screen"
assert changed, "the screen did not show the new state"
PY
