# Handoff (2026-11-05, session 1: foundation)

## Where things stand

| Item | Status |
|---|---|
| Repo layout, CMake, host CI (`.github/workflows/host.yml`) | Implemented, Host-tested (CI run not yet observed) |
| Integer Sun + Moon longitude engine `core/astro/` (no float, freestanding) | Host-tested: 120 reference dates, see [ACCURACY.md](ACCURACY.md) |
| Calendar (UTC civil → TT, Delta T), zodiac sign/arcminute helpers | Host-tested |
| Planets, retrograde, aspects, Ascendant/houses | Designed (below) |
| Native firmware (Dinghy app), .fwsc, fm1-emu gate | Not started — recipe below |
| GitHub Pages site, installer, release/attestation | Not started — recipe below |
| Owner decisions | D1, D3–D8 decided 2026-11-05; D2 waits for the vertical slice: [DECISIONS.md](DECISIONS.md) |

Session 1 was short (owner near a usage limit). It spent its effort on the riskiest unknown: can real astronomy run
on the FM-1's integer-only CPU? Answer for Sun and Moon: yes, ~5.5 KB and sub-arcminute accuracy.

## MIKU-FM1: the proven pipeline to copy (inspected 2026-11-05 at 88b0ef3)

Same hardware, same owner, released and hardware-tested (v0.6). Reuse its shape; replace its identity.

1. **Firmware build** — `native/build.sh`: clones **Dinghy** (github.com/zvenson/dinghy, GPL-3.0-only) at a pinned
   commit (`f77bf03283bbb2c05294b01cdad68a30b410f93d`), copies the app's single C file to `firmware/src/dinghy.c` (it
   `#include`s platform.c and the app's modules as .c files), copies modules under `firmware/src/firmware/...`, then
   patches Dinghy's `tools/build.py` with exact-match substitutions: `SCREEN_NAME`, `DATA_FAMILY`, `SCREEN_COLOUR`,
   `BOARD_PREFIX` (identity `FM-1_39…` for MIKU — **ASTRA needs its own unused prefix**; MIKU checked the Firmware Hub's
   live table with Playwright and public GitHub code before choosing), and the version→identity rule `FM-1_PPXXYYZZ`.
   Sets `DINGHY_DEFS=-DSTORE_MAGIC=…` so it never parses another firmware's saved data. Output `build/native/*.fwsc`.
   Toolchain: `curl -fL https://pkgman.jieliapp.com/s/linux-toolchain | tar -xJ` (clang 4.0.1) → `JIELI_TOOLCHAIN`;
   SDK: gitee `Jieli-Tech/fw-AC79_AIoT_SDK` tag `AC79NN_SDK_V1.2.1_2023-12-13`, commit `d179b44…` → `AC79_SDK`
   (only `cpu/wl82/tools/{uboot.boot,cfg_tool.bin,cfg/eq_cfg_hw.bin}` are used; in a sandbox fetch those three via raw
   links with retries — a full clone fails there). Dinghy's size/register checks must print ok.
2. **App API seen in MIKU's `native/dinghy.c`**: `#define APP_NAME`, `app_audio(int32_t *out, uint32_t n)` (the ISR),
   `platform_poll()`, `fm1_ms`, `lcd_fill/lcd_blit/lcd_sync`, RGB565 `RGB(r,g,b)`, text via `txt(x,y,&FONT_S,…)`.
   Read Dinghy's own example app before writing `native/astra.c`.
3. **Emulator gate** — `tools/emu_smoke.sh PKG OUT EMU`: fm1-emu at `9de1ef0` (`cargo build --release -p fm1-emu`),
   `--fwsc PKG --ms 3000 --keys '200:PLAY,1500:enc0:+2' --snap 1000,2500 --wav audio.wav`; asserts >10000 frames,
   RMS > 10, > 2 screen colours. Must run in a dir with `work/` (snapshots fail silently otherwise). ~30–60 s per
   emulated second. Evidence uploaded as a CI artifact.
4. **Pages** — `.github/workflows/pages.yml`: on release / push to main / manual. Push to main carries the latest
   *release's attested* package (dev build only while no release exists, labelled "not attested"). Boots it in fm1-emu
   first, then `tools/site_emu.sh` (fm1-emu `web/build.sh` → `site/fm1-emu/`, a `firmwares.json` catalog with only this
   package) and `tools/site_front.sh` (fills `{{VERSION}} {{PACKAGE}} {{SHA256}} {{BUILD}}` and `/*META*/` in
   index.html/install.html; decodes the package identity from the bytes and asserts its prefix). This is what keeps
   site, emulator and installer on the same package.
5. **Installer** — `site/install.html` + `site/js/fm1ota.js`, `fm1pkg.js`: Felucca's web installer (GPL-3.0-only,
   Hügelton Instruments). Reuse with attribution.
6. **Release** — `.github/workflows/release.yml`: tag `vX.Y[.Z]` → draft release → `zvenson/fm1-build/.github/workflows/build.yml@v1`
   (shared builder; Firmware Hub shows "Built from source") → verify SHA256SUMS + `gh attestation verify` → publish.
   The `github-pages` environment must allow tags `v*`.
7. **Listing** — `fmhub.json` + `docs/card.png` (a real 240×240 screen capture) for thefirmwarehub.com.

## Architecture (platform: Dinghy, owner decision D1)

- **Platform: Dinghy**, as MIKU. Proven by this owner end to end (build, emu, Pages, attested release, hardware
  install/update, saves). Not yet compared first-hand this session: fm1-base (MPL platform template, MPFM1), COSMOS
  (has an FM-1 astronomy engine — inspect its method, accuracy and licence before anything else in Step 4), Felucca.
- **One integer astronomy core** (`core/`) compiled three ways: device (JieLi clang), host tests (gcc), browser
  (clang/emscripten → WASM, same source, so the site and the device agree to the bit). The site may *add* a
  float/high-precision path for charting, but the device never depends on it.
- **Planets (next)**: JPL "Approximate Positions of the Major Planets" Keplerian elements (Standish; public, 1800–2050
  table plus the 3000 BC–3000 AD table with Jupiter–Pluto corrections). Needs a fixed-point Kepler solver and
  atan2 (CORDIC) for heliocentric→geocentric. Expect ~1′ for inner planets, a few ′ for outer; measure against
  Astronomy Engine like the Sun/Moon. Truncated VSOP87 is the fallback if that is not good enough.
- **Retrograde**: sign of the geocentric longitude change over ±12 h. **Aspects/orbs/applying-separating**: pure BAM
  arithmetic (difference, wrap, compare to aspect angle ± orb; applying if |error| shrinks over the next hour).
- **Ascendant/MC/houses**: needs sidereal time (integer from days+frac), obliquity, atan2 and an arcsine. Whole-sign
  and Equal are trivial; Placidus/Koch need iteration and fail at high latitudes. Which systems: DECISIONS D4.
- **Timekeeping**: unknown whether the FM-1 keeps time across power loss (likely no battery RTC — verify in Dinghy/SDK
  and on hardware). Never show "now" unless time is set this power cycle; options for the owner in DECISIONS D5.

## Next steps (in order)

1. **Device size check**: drop `core/astro/astro.c` into a minimal Dinghy app and build with the JieLi toolchain;
   record flash/RAM from Dinghy's checks. Time one Sun+Moon evaluation on device later (emulator CPU% is not a device figure).
2. **Vertical slice (Step 7)**: `native/astra.c` on Dinghy: boot screen asks for UTC date/time (encoders), shows Sun and
   Moon sign + degree, and a TEMPORARY test tone whose pitch follows the Moon's longitude (e.g. 12 signs → 12 semitones).
   `native/build.sh` adapted from MIKU (new identity prefix, store magic "ASTR", screen name "ASTRA").
3. `tools/emu_smoke.sh` adapted; add an astronomy assertion: boot with a fixed date via `--keys`, snapshot, compare the
   rendered longitude text/pixels with the host value for the same date (repeatable).
4. `pages.yml`, `site_emu.sh`, `site_front.sh`, `site/index.html`, `site/install.html` adapted from MIKU; enable Pages
   (Settings → Pages → GitHub Actions). Then `release.yml` with `zvenson/fm1-build@v1`.
5. Planets (JPL elements), retrograde, aspects — each host-tested against Astronomy Engine.
6. WASM build of `core/` and the companion chart page (offline: service worker). Charting library choice: DECISIONS D6.
7. Present the creative options (DECISIONS D2, D3, D7) to the owner.
