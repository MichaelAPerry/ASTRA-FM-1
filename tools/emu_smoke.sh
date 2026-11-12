#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Adapted from MIKU-FM1 tools/emu_smoke.sh (MIT, same owner).
# Boot an ASTRA package in fm1-emu (github.com/joakimeriksson/fm1-emu) and check, from the real .fwsc:
#   - it boots and draws the wheel (many colours), silent until a key is held;
#   - the controls: GLO, the knobs turn the clock from the chart time (2000-01-01 12:00, NOT SET) to 2026-11-05,
#     PLAY sets it; HOME; two keys held; ALGORITHM and KNOB 1 turned; SEQ (the horoscope);
#   - the state after it (astra_probe, read with --dump): page, clock SET and running, the sky on it, algorithm,
#     wires and transits found;
#   - the astronomy: the eleven longitudes the firmware computed equal, to the bit, what the host build of the same
#     core gives for the same instant (EXPECT = build/emu_expect);
#   - the sound: silence before the keys, sound while they are held, the lower key's own pitch in it;
#   - the screen: each step changed it, and the knob turn put a colour on its dial that was not there before.
# Evidence (WAV, BMPs, log) goes to OUT_DIR.
#   tools/emu_smoke.sh PACKAGE.fwsc PROBE.txt OUT_DIR EXPECT [FM1_EMU_BIN]
set -eu
U="usage: tools/emu_smoke.sh PACKAGE.fwsc PROBE.txt OUT_DIR EXPECT [FM1_EMU_BIN]"
abs() { case "$1" in */*) echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")";; *) echo "$1";; esac; }
PKG="$(abs "${1:?$U}")"; PROBE="$(cat "${2:?$U}")"; OUT="${3:?$U}"; EXPECT="$(abs "${4:?$U}")"; EMU="$(abs "${5:-fm1-emu}")"
mkdir -p "$OUT/work"                            # fm1-emu writes snapshots to work/ (and fails silently without it)
cd "$OUT"
"$EMU" --fwsc "$PKG" --ms 3800 \
       --keys '500:GLO,700:K1:+26,800:K2:+10,900:K3:+4,1300:PLAY,1600:HOME,1800-3200:n7,1900-3200:n14,2500:ALGO:+12,2700:K1:+3,3300:SEQ' \
       --snap 400,1500,2400,3000,3600 --wav audio.wav --dump "$PROBE" 37 > boot.log 2>&1
python3 - "$EXPECT" <<'PY'
import array, math, re, subprocess, sys, wave
from PIL import Image
log = open("boot.log").read()
m = re.search(r"^dump 0x[0-9a-f]+ \(37 words\):\n((?:\s+0x[0-9a-f]+:(?: [0-9a-f]{8})+\n?)+)", log, re.M)
assert m, "no probe dump in boot.log"
w = [int(x, 16) for line in m[1].splitlines() for x in line.split(":")[1].split()]
names = "magic page sub sel slot algo clock live run edited nedge ntransit latch saves sky_days sky_sec now_days now_sec".split()
p = dict(zip(names, w)); lon = w[18:29]; voices, isr_max, isr_mean, frames, rows = w[30:35]
assert p["magic"] == 0x50525432, f"probe never filled (magic {p['magic']:#x})"
print("emu_smoke: probe", {k: v for k, v in p.items() if k != "magic"})
assert p["page"] == 5 and p["sub"] == 0, f"page {p['page']}.{p['sub']}, want 5.0 (the horoscope): SEQ did not reach the firmware"
assert p["clock"] == 2, f"clock state {p['clock']}, want 2 (SET): GLO, the knobs or PLAY did not reach the firmware"
assert p["now_days"] == 20762, f"date: day {p['now_days']}, want 20762 (2026-11-05)"
assert 43200 < p["now_sec"] <= 43200 + 4, f"now {p['now_sec']} s: want 12:00:00 plus the ~2.5 s since PLAY"
assert p["live"] == 1 and p["sky_days"] == p["now_days"] and 0 <= p["now_sec"] - p["sky_sec"] <= 1, "the sky is not on the clock"
assert p["algo"] == 13 and p["edited"] == 1, f"algorithm {p['algo']}, want 13 (SKY H1): ALGORITHM did not turn"
assert p["nedge"] > 0 and p["ntransit"] > 0, "no wires or no transits from the sky"
want = subprocess.run([sys.argv[1], str(p["sky_days"]), str(p["sky_sec"])], check=True, capture_output=True, text=True).stdout.split()
got = [f"{v:08x}" for v in lon]
assert got == want, f"device {got}\nhost   {want}"
print("emu_smoke: Sun..Pluto, Node:", " ".join(f"{v * 360 / 2**32:.3f}" for v in lon), "deg: device == host, bit for bit")
print(f"emu_smoke: audio interrupt mean {isr_mean} us, max {isr_max} us of 5805 (fm1-emu counts an instruction a cycle: "
      f"the FM-1's own costs are measured with tools/emu_cpu.sh); {frames} frames drawn, {rows} rows sent")
assert isr_max < 5805, "the audio interrupt overran its period"

with wave.open("audio.wav", "rb") as wv:
    rate, ch, n = wv.getframerate(), wv.getnchannels(), wv.getnframes()
    a = array.array("h", wv.readframes(n))
left = a[::ch]
rms = lambda x: (sum(v * v for v in x) / max(len(x), 1)) ** 0.5
def tone(x, f):                                  # amplitude of f in x (Hann window)
    N = len(x); c = 2 * math.cos(2 * math.pi * f / rate); s1 = s2 = 0.0
    for i, v in enumerate(x):
        s0 = v * (0.5 - 0.5 * math.cos(2 * math.pi * i / N)) + c * s1 - s2; s2, s1 = s1, s0
    return math.sqrt(max(s1 * s1 + s2 * s2 - c * s1 * s2, 0)) * 4 / N
# the WAV starts when the firmware starts its audio, a little after boot: time it from the first sound, the keys
onset = next((i for i, v in enumerate(left) if abs(v) > 64), len(left))
quiet, held = left[:onset], left[onset + rate // 10:onset + rate // 2]
c4, off = tone(held, 261.63), tone(held, 233.0)
print(f"emu_smoke: {n} frames at {rate} Hz; silent for {onset / rate:.2f} s (no key held), then held RMS {rms(held):.0f}, "
      f"peak {max(abs(v) for v in a)}; C4 {c4:.0f}, between the notes {off:.0f}")
assert n > 100000, "short audio"
assert onset > rate, "sound before any key was held (or the keys came too early)"
assert rms(quiet) < 3, "noise with no key held"
assert rms(held) > 150, "silent while two keys are held"
assert c4 > 8 * off and c4 > 40, "the held key's pitch is not in the sound"

im = {t: Image.open(f"work/lcd_{t}.bmp").convert("RGB") for t in (400, 1500, 2400, 3000, 3600)}
def cols(i, box=None):                           # the distinct colours in an image (or a box of it)
    b = (i.crop(box) if box else i).tobytes()
    return {b[k:k + 3] for k in range(0, len(b), 3)}
print(f"emu_smoke: screen colours at boot {len(cols(im[400]))}, playing {len(cols(im[2400]))}, horoscope {len(cols(im[3600]))}")
assert len(cols(im[400])) > 12, "the wheel is not drawn"
body = (0, 12, 240, 200)
assert im[400].crop(body).tobytes() != im[1500].crop(body).tobytes(), "GLO changed nothing on the screen"
assert im[1500].crop(body).tobytes() != im[2400].crop(body).tobytes(), "HOME changed nothing on the screen"
assert im[3000].crop(body).tobytes() != im[3600].crop(body).tobytes(), "SEQ changed nothing on the screen"
dial = (0, 196, 60, 240)                         # KNOB 1's dial
new = cols(im[3000], dial) - cols(im[2400], dial)
print(f"emu_smoke: KNOB 1 turned: {len(new)} new colours on its dial")
assert new, "the knob turn did not change a colour on its dial"
PY
