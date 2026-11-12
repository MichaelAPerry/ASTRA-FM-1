#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The audio interrupt's cost on the FM-1, measured: the real package in fm1-live with FM1_STRICT=1 (each instruction
# costs the cycles harbor's probes measured on a real FM-1, flash-cache misses included), five notes held (every voice
# sounding) on a cast patch with every effect on. The firmware times its own interrupt with the 24 MHz counter (native/astra.c
# app_audio) and the result is read from astra_probe in a RAM dump. Fails above half the 5.8 ms period (AGENTS.md).
#   tools/emu_cpu.sh PACKAGE.fwsc PROBE.txt OUT_DIR FM1_LIVE_BIN
set -eu
U="usage: tools/emu_cpu.sh PACKAGE.fwsc PROBE.txt OUT_DIR FM1_LIVE_BIN"
abs() { case "$1" in */*) echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")";; *) echo "$1";; esac; }
PKG="$(abs "${1:?$U}")"; PROBE="$(cat "${2:?$U}")"; OUT="${3:?$U}"; LIVE="$(abs "${4:?$U}")"
mkdir -p "$OUT/work" && cd "$OUT"
# five keys pressed at 0.6 s and held to the end (fm1-live's script: one time per event, no ranges)
FM1_STRICT=1 FM1_DUMPRAM=ram.bin FM1_WARMUP_MS=200 FM1_BENCH_KEYS='600:n0,610:n4,620:n7,630:n11,640:n14' \
  "$LIVE" --fwsc "$PKG" --bench 2400 > cpu.log 2>&1
python3 - "$PROBE" <<'PY'
import struct, sys
d = open("ram.bin", "rb").read()
w = struct.unpack_from("<35I", d, int(sys.argv[1], 16) - 0x01C00000)
assert w[0] == 0x50525432, f"probe never filled ({w[0]:#x})"
voices, isr_max, isr_mean, frames, rows = w[30:35]
print(f"emu_cpu: FM-1 costs (FM1_STRICT): {voices} voices, audio interrupt mean {isr_mean} us, max {isr_max} us of 5805 "
      f"({isr_max / 58.05:.0f} %); main loop drew {frames} frames, sent {rows} rows")
assert voices >= 3, f"five notes held, {voices} voices"
assert isr_max < 2903, "the audio interrupt takes more than half its period"
PY
