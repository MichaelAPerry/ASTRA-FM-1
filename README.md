# ASTRA FM-1 (working title)

The sky as an FM synthesizer, for the M-VAVE FM-1. The FM-1 computes the planets itself, casts your birth chart, and
wires a ten-operator FM synthesizer from the aspects between them: one operator per body, every trine a modulation,
every square a ring, every return to a birth place a feedback loop. Offline, integer-only, on the FM-1's own chip.

**Status (2026-11-12): Emulator-tested, not Hardware-tested.** The real firmware package runs in
[fm1-emu](https://github.com/joakimeriksson/fm1-emu) in CI: controls, state, sound, screens, and the eleven planetary
longitudes equal to the host build's bit for bit. Site and web installer: [site/](site/), deployed by
[pages.yml](.github/workflows/pages.yml) once Pages is enabled. State and next steps: [docs/HANDOFF.md](docs/HANDOFF.md).

- **Sky**: Sun, Moon, Mercury–Pluto, the node, retrograde motion, ASC/MC, seven house systems, tropical or sidereal,
  1800–2200, measured against Astronomy Engine: planets ≤ 2″, Pluto ≤ 5″, Moon ≤ 38″ ([docs/ACCURACY.md](docs/ACCURACY.md)).
- **Engine**: 3 voices × 10 operators, any routing (two phase sources, a ring source and feedback per operator),
  37 chart algorithms (free, birth, sky, transits × harmonics 1–12), chorus, echo, reverb; the audio interrupt at 42 %
  of its period on the FM-1's measured costs ([docs/ENGINE.md](docs/ENGINE.md)).
- **Horoscope**: transits to your birth chart, closest first, with applying/separating and days to exact; the month
  around; placements, dignities, the Moon's phase. Every reading line names the aspect and houses it comes from.
- **Interface**: seven pages of line art; every knob turn changes a colour on screen (tested); 64 preset slots.

Astronomy here is computation; astrology is symbolic interpretation, not scientific prediction.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure
```

Firmware: `bash native/build.sh` (JieLi toolchain and AC79 SDK: see docs/HANDOFF.md), gate:
`tools/emu_smoke.sh build/native/astra.fwsc build/native/astra-probe.txt build/emu build/emu_expect FM1_EMU`.
Brief: [docs/MASTER_PROMPT.md](docs/MASTER_PROMPT.md). Owner decisions: [docs/DECISIONS.md](docs/DECISIONS.md).
