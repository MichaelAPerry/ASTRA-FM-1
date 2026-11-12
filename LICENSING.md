# Licensing

- This repository: **GPL-3.0** ([LICENSE](LICENSE)). The planned firmware builds on Dinghy (GPL-3.0-only) and the site's
  installer on Felucca's web installer (GPL-3.0-only), so the combined firmware and installer are GPL-3.0-only.
- `core/astro/`: original code. Algorithms and coefficients from J. Meeus, *Astronomical Algorithms* (2nd ed., 1998),
  ch. 22, 25, 47 — published mathematical methods, reimplemented here; Delta T from Espenak & Meeus (NASA).
- `core/astro/sky.c` and `planet_tables.h`: original code; the planetary series are VSOP87B (P. Bretagnon and
  G. Francou, 1988, published coefficients) as truncated in Astronomy Engine (Don Cross, MIT): `tools/gen_planet_tables.py`
  reads the coefficients from the installed `astronomy-engine` package at generation time and writes them in the
  device's integer units. MIT is compatible with GPL-3.0; the notice is kept here. Pluto's Chebyshev coefficients
  are fitted to Astronomy Engine's Pluto. Precession, obliquity and sidereal time: IAU 2006 (published).
- Astronomy Engine (Don Cross, MIT) is otherwise used only by `tools/gen_reference.py` to produce test reference
  values (`tests/astro_ref.h`, `tests/sky_ref.h`); none of its code is in the firmware.
- `native/astra.c` is compiled as Dinghy's app (zvenson/dinghy, GPL-3.0-only, Leo Kuroshita / Hügelton Instruments
  via Felucca, isod89 via SLOOP): it includes Dinghy's `platform.c`. The built `.fwsc` is GPL-3.0-only.
- `native/build.sh`, `tools/emu_smoke.sh`, `.github/workflows/device.yml`: adapted from MIKU-FM1 (MIT, same owner).
- fm1-emu (Joakim Eriksson, MIT): built and run by CI; its web build is published on the site (`fm1-emu/`, with its
  LICENSE), running ASTRA's own package. fm1-live (same repository) measures the interrupt in CI.
- `site/js/fm1ota.js`, `fm1pkg.js`: Felucca's web installer (Leo Kuroshita, Hügelton Instruments, GPL-3.0-only),
  unchanged, via MIKU-FM1 (`site/js/README.md`). `site/install.html`'s flow follows Felucca's `web/index_pkg.html`.
- `tools/site_emu.sh`, `tools/site_front.sh`, `.github/workflows/pages.yml`: adapted from MIKU-FM1 (MIT, same owner).
- `core/ui/gfx.c`, `core/app/app_draw.c`: the visual style revision was proposed by the owner with Google Gemini
  (2026-11-12) and merged by hand; released under this repository's GPL-3.0 like the rest.
- `core/chart/chart.c`: the reading text is original wording. The Tarot trump titles it quotes are from Aleister
  Crowley's *777* (1909), Table VI (the Golden Dawn's attributions): Crowley died in 1947, so his work is public domain in
  life+70 countries since 2018, and works published before 1931 are public domain in the US. Read from the archive.org
  transcription "Aleister Crowley - Liber 777" (its tables stated to be facsimiles of 1909). Manly P. Hall: nothing
  quoted (his works are in copyright in life+70 countries until 2061). Not legal advice.
- JieLi toolchain and AC79 SDK files (`uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`): vendor tools used by Dinghy's build, not part of this repo.
- Check every new dependency's licence and record it here before use.
