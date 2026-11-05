# ASTRA FM-1 (working title)

An offline astrological generative music instrument for the M-VAVE FM-1: real astronomy computed on the device,
a personal natal chart, and music driven by both, with a companion GitHub Pages site running the real firmware in fm1-emu.

**Status (2026-11-05): foundation.** An integer-only Sun/Moon engine (`core/astro/`) is host-tested against Astronomy
Engine over 1800–2200 (Sun ≤ 0.0075°, Moon ≤ 0.0105°: [docs/ACCURACY.md](docs/ACCURACY.md)). No firmware, emulator
build or website yet. State and next steps: [docs/HANDOFF.md](docs/HANDOFF.md). Brief: [docs/MASTER_PROMPT.md](docs/MASTER_PROMPT.md).

Astronomy here is computation; astrology is symbolic interpretation, not scientific prediction.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure
```
