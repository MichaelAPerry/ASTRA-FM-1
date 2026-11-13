# ASTRA FM-1

Free firmware for the M-VAVE FM-1 that turns the planets, and your birth chart, into sound.

ASTRA runs on the FM-1 itself. It calculates the positions of the Sun, Moon, and planets for any date between 1800 and 2200 without needing internet or a phone. You enter your birth details, and the device builds a chart. That chart becomes the instrument's wiring. When the sky moves, the sound moves with it. The screen also reads your horoscope: today's transits to your chart, closest first.

FM (frequency modulation) means one tone wobbles the pitch of another so fast that you hear a new, richer tone. Each tone is called an operator. ASTRA has ten, one per body from the Sun to Pluto, and the aspects between the planets decide which operator bends which. A trine is a smooth bend, a square a metallic ring, an opposition a loop.

## Try it

Play it in your browser on the [ASTRA website](https://michaelaperry.github.io/ASTRA-FM-1/). The site runs the real firmware in an emulator, so you can try it before installing (it needs a fast computer). The installer is on the same site. New to it? [Start here](docs/QUICKSTART.md): five minutes. [The full guide](docs/GUIDE.md) explains every button and knob, the astrology behind the readings, and how to shape the sound.

## Status

*   **Emulator-tested**: the real firmware runs in [fm1-emu](https://github.com/joakimeriksson/fm1-emu) in automated tests.
*   **Not yet tested on a physical FM-1.** This is an early development build. Install it only if you are comfortable with the USB rescue (hold OCT- while switching on).
*   ASTRA FM-1 is a working title.

## What's inside

*   **Sky**: The FM-1 calculates positions for Sun, Moon, Mercury through Pluto and the Moon's node from 1800 to 2200. Seven house systems, tropical or sidereal. Accurate to within 2 arcseconds for the planets and 38 for the Moon ([accuracy](docs/ACCURACY.md)).
*   **Sound**: Four notes play at once. Each note uses ten operators. There are 37 ways to wire them from the chart, plus chorus, echo and reverb ([how it works](docs/ENGINE.md)).
*   **Horoscope**: It shows transits closest to exact first, then a graph of the 30 days either side, your birth chart, and the Moon's phase. Every reading lists the specific aspect and houses involved. It also shows the Tarot trump titles Aleister Crowley gave each planet and sign in *777* (1909).
*   **Screens**: Seven pages of colourful line art, with the chart drawn as a wheel. Every knob turn changes a colour on screen.
*   **Presets**: 64 preset slots. The ones you haven't saved yet build a sound from your chart.
*   **Offline**: Everything is computed on the device; your birth details never leave it.

Astronomy here is calculation; astrology is symbolic interpretation, not prediction.

## For developers

Host build and tests:
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure
```

Firmware (needs the JieLi toolchain and AC79 SDK, see docs/HANDOFF.md):
```
bash native/build.sh
tools/emu_smoke.sh build/native/astra.fwsc build/native/astra-probe.txt build/emu build/emu_expect FM1_EMU
```

The second command is the emulator test the firmware must pass. Links: [docs/HANDOFF.md](docs/HANDOFF.md) (current state, next steps), [docs/ENGINE.md](docs/ENGINE.md) (how the chart becomes sound), [docs/ACCURACY.md](docs/ACCURACY.md), [docs/DECISIONS.md](docs/DECISIONS.md) (the owner's decisions), [docs/MASTER_PROMPT.md](docs/MASTER_PROMPT.md) (the original brief), [AGENTS.md](AGENTS.md) (rules for contributors and coding agents).

## Credits and licence

Built on [Dinghy](https://github.com/zvenson/dinghy) (zvenson), [Felucca](https://github.com/hugelton/Felucca) (Leo Kuroshita, Hügelton Instruments) and [SLOOP](https://github.com/isod89/sloop-fm1) (isod89). Emulator: [fm1-emu](https://github.com/joakimeriksson/fm1-emu) (Joakim Eriksson). Planet positions: VSOP87 as truncated by [Astronomy Engine](https://github.com/cosinekitty/astronomy) (Don Cross), and Jean Meeus's *Astronomical Algorithms*. Licence: GPL-3.0-only ([LICENSING.md](LICENSING.md)). ASTRA is unofficial and not associated with or endorsed by M-VAVE.
