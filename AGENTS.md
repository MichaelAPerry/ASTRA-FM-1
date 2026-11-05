# ASTRA FM-1 — agent instructions (Codex, Claude, any agent)

Read these first, in order: [docs/MASTER_PROMPT.md](docs/MASTER_PROMPT.md) (the owner's full brief: binding),
[docs/HANDOFF.md](docs/HANDOFF.md) (state, verified recipe, next steps), [docs/DECISIONS.md](docs/DECISIONS.md) (what the
owner has decided and what is still open). Update HANDOFF.md and DECISIONS.md at the end of every session.

## Ground rules
- The owner decides product, music, astrology, visual and interaction questions. Present options + one recommendation;
  keep building infrastructure meanwhile. Never finalise a creative mapping silently; temporary mappings say TEMPORARY.
- Status words are never interchangeable: Designed / Implemented / Host-tested / Emulator-tested / Hardware-tested /
  Published. Hardware-tested needs the owner's evidence. Published needs a green Pages run you looked at.
- Everything essential runs offline: on the FM-1 itself, and in the browser (no birth data leaves the page).
- Real fm1-emu only (github.com/joakimeriksson/fm1-emu) running the real .fwsc. No JS LCD mockups, no Web Audio stand-ins.
- **Never flash, or tell the owner to flash, a physical FM-1 without their explicit go-ahead in that session.**
- Do not modify MIKU-FM1 (reference only). Check every licence before reusing code (LICENSING.md).

## Device rules (learned on MIKU-FM1, same hardware: JieLi AC79 / pi32v2, Dinghy platform)
- `core/` is device code: **no float/double/libm, no libc, no heap, no 64-bit multiply/divide.** Angles are BAM
  (uint32, 2^32 = 360°). Floating point is allowed only in tools/ and tests/. CMake target `astra_core_nofloat`
  enforces this (builds core/ with `-ffreestanding -mgeneral-regs-only`).
- Audio ISR: 256 frames at 44.1 kHz = 5.8 ms for everything; keep it under half. No astronomy, screen or flash in the
  ISR: compute in the main loop, hand results over with single-word writes or a one-producer ring.
- Constants are generated (tools/gen_*.py) and checked fresh in CI; don't hand-edit generated headers.

## Build and test
- Host: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure`
  (0 warnings: `-Werror` on core/).
- Reference data: `pip install astronomy-engine && python3 tools/gen_reference.py` (only when adding reference dates).
- Device build, fm1-emu gate, Pages, release: not in this repo yet; recipe and exact pins in docs/HANDOFF.md.

## Git workflow (owner rule)
- Work on `main` directly: no feature branches, no PRs. Push to `main` only after the full host build and
  `ctest` pass (and, once they exist, the device build and the fm1-emu smoke test).

## Style
Terse C11, small functions, comments that say why. Commit by explicit path. Short reports, findings first.
