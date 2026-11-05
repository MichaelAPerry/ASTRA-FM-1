# Handoff (2026-11-05, session 2: device size, vertical slice, emulator gate — Emulator-tested)

## Where things stand

| Item | Status |
|---|---|
| Repo layout, CMake, host CI (`.github/workflows/host.yml`) | Host-tested; CI green (run 42079005067, looked at) |
| Integer Sun + Moon longitude engine `core/astro/` (no float, freestanding) | Host-tested: 120 reference dates, see [ACCURACY.md](ACCURACY.md) |
| Calendar both ways (`astro_civil_from_days` new: round-trips every day 1700–2300), Delta T, sign helpers | Host-tested |
| `core/astro` device size: **4,318 B flash, 0 B RAM**, no helper calls | Measured with JieLi clang 4.0.1 (compile only) |
| Vertical slice `native/astra.c` (Dinghy app): UTC entry, Sun + Moon on screen, TEMPORARY Moon-sign tone, D5 clock states | **Emulator-tested** (real .fwsc in fm1-emu 9de1ef0, CI run 42079005186, evidence below). Not Hardware-tested |
| `native/build.sh` (Dinghy at f77bf03, identity `FM-1_44XXYYZZ`, store magic "ASTR") | Works in CI: 612,218 B package, identity `FM-1_44000000`, all Dinghy checks ok |
| `tools/emu_smoke.sh` + `tests/emu_expect.c`: the fm1-emu gate with a bit-exact astronomy check | Passing on the real emulator (and its fail path checked against a fake one) |
| `.github/workflows/device.yml`: toolchain, SDK, `native/build.sh`, fm1-emu 9de1ef0, gate, artifacts | Green on its first run (3 min; fm1-emu build now cached) |
| Planets, retrograde, aspects, Ascendant/houses | Designed (below) |
| GitHub Pages site, installer, release/attestation | Not started — recipe below |
| Owner decisions | D1, D3–D8 decided 2026-11-05; D2 waits for the vertical slice: [DECISIONS.md](DECISIONS.md) |

Nothing is Hardware-tested or Published yet.

### Emulator evidence (CI run 42079005186, commit ed7b350, looked at 2026-11-05)
- Dinghy's checks: `ok .ram_text: 916 insns, no calls`; `ok image 60180 B of 581564; RAM .data+.bss 15900 B of 98304;
  pool 30720 B of 344064`; loader 10,476 B. Package `astra.fwsc` 612,218 B, identity `FM-1_44000000`,
  SHA-256 e8f29500…7add0 (dev build, not attested).
- Gate: `probe days 20762 sec 43201 state 2` (2026-11-05 12:00:01 UTC, SET: knobs and PLAY reached the firmware);
  `Sun 223.0844 deg, Moon 178.5637 deg: device == host, bit for bit`; `124928 frames, RMS 1611, tone 292.0 Hz (note 62:
  293.7 Hz)`; 5 screen colours, state line redrawn after PLAY.
- Screens (artifact `lcd_1300.bmp`, `lcd_2800.bmp`, looked at): "UTC 2026-11-05 12:00:00", amber "NOT SET: CHART TIME"
  → green "SET THIS POWER-ON" at 12:00:01; SUN "13 SCO 04'", MOON "28 VIR 33'"; everything fits the 240 px width.
  They showed one bug: "TEST TONE D4 (TEMP))" (an unterminated string); fixed after ed7b350, its CI run is the check.
- The 26 encoder clicks in one event did arrive as 26 detents (the year reached 2026).

### Session 2 notes
- **Sandbox limit:** this session's safety check refused to run Dinghy's `build.sh`/`tools/build.py` (third-party code)
  in the sandbox, so the full firmware build and fm1-emu were not run here. What was run: our own code, and JieLi's
  clang as a compiler on our sources (and Dinghy's headers, compile only). The full build and the gate run in CI
  (`device.yml`), on GitHub's runners, exactly as MIKU-FM1 does. To run them in a session instead, the owner can allow
  it in that session's permissions.
- Toolchain: `pkgman.jieliapp.com/s/linux-toolchain` today serves `jieli-linux-toolchains-20260730.1` (clang 4.0.1).
  Gitee resets most connections from the sandbox; the three SDK files came through raw links after retries, with the
  SHA-256s Dinghy's build.py expects.
- **Identity `FM-1_44`:** decoded the identity of the 151 downloadable packages (of 155) in fm1-emu's hub catalog
  (`web/firmwares.json`; the identity is in the package bytes, MIKU's `site_front.sh` formula) and searched public
  GitHub code. Taken 2-digit prefixes: 00 10 15 19 21 22 23 25 30 32 33 35 37 38 42 43 46 47 48 50 57 60 61 64 66 67 70 71 72 73 76 77 79 80 81 83 87 88 90 91 92 93 94 95 96 97 98 99; plus 39 (MIKU, not in the catalog) and 12 (fm1-base's examples). `FM-1_44`: no package, no code hit. Note it is not
  `FM-1_9NN`, so the web installers will not resume a cut-off install of ASTRA's loader (Dinghy MAKE-IT-YOURS §3);
  MIKU has the same trade-off with `FM-1_39`. Not checked: the hub's live table (MIKU did that with Playwright).
- The gate (`tools/emu_smoke.sh`): boots, K1 +26 / K2 +10 / K3 +4 turn the editor from 2000-01-01 12:00 to
  2026-11-05 12:00 UTC, PLAY sets it; then reads `astra_probe` (address from the ELF via `nm`, written by build.sh to
  `build/native/astra-probe.txt`) with `--dump`, and requires: date 2026-11-05, state SET, clock running, Sun and Moon
  BAM equal to `build/emu_expect` for the same instant (bit for bit), tone pitch within 3 % of the Moon-sign note, > 2
  screen colours, and the state line redrawn after PLAY. Risk not yet seen: whether fm1-emu delivers 26 encoder clicks
  given in one event as 26 detents (if not, split them into several timed events).
- The settings save erases a flash sector, which stalls the test tone ~50 ms (Dinghy saves only when silent; this
  tone never is). Fine for the slice; the music engine will need a quiet moment or a fade before saving.

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

Owner, 2026-11-05: features and the firmware's visual style come first. **No hardware flashing and no Pages/release
work until the build is near done and ready to publish** (Pages is already enabled in the repo settings; leave it).

1. Check the `device` run for the label fix (8675557); main follows the branch once host + device are green.
2. **Planets** (JPL Keplerian elements, fixed-point Kepler solver, CORDIC atan2): Mercury–Pluto geocentric longitude,
   host-tested against Astronomy Engine, then bit-exact in the emulator gate like the Sun and Moon. Retrograde
   (sign of the longitude change over ±12 h). Measure flash and CPU per evaluation on the device build.
3. **Aspects, orbs, applying/separating** (pure BAM arithmetic), host-tested.
4. **FM engine foundation** (owner: an FM engine for each planet). Build the engine, not the mapping: N FM voices
   (operators, ratios, index, envelope) sized to the ISR budget (5.8 ms per 256 frames, keep under half), one voice
   slot per body, parameters handed from the main loop with single-word writes. Measure ISR time in the emulator
   (cycle count) and record it. How each planet's position/aspects drive its voice stays the owner's (D2, D3, D10).
5. **Present options to the owner**: D2 musical character (2–3 demo modes on the per-planet engine), D10 what each
   planet's engine is, and D11 the firmware's visual style and screen layouts (mockups made from real emulator
   captures, never JS stand-ins).
6. Ascendant/MC/houses (D4: every system we implement, tropical and sidereal), time-zone offset (D5), natal profile.
7. WASM build of `core/` and the companion chart (D6), offline.
8. Later, when the owner says the build is near done: Pages (`pages.yml`, `site_emu.sh`, `site_front.sh` asserting
   `FM-1_44`), `release.yml` with `zvenson/fm1-build@v1`, and only with the owner's go-ahead, hardware tests.
