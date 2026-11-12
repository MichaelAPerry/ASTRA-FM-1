# Handoff (2026-11-12, session 3: the instrument — Emulator-tested; site Published, nothing Hardware-tested)

## Where things stand

| Item | Status |
|---|---|
| Sky: Sun, Moon, Mercury–Pluto, node, retrograde motion; ASC/MC, 7 house systems; tropical + sidereal (Lahiri); aspects (11 kinds, applying/separating, orbs, harmonics) | Host-tested (docs/ACCURACY.md: planets ≤ 2″, Pluto ≤ 5″, Moon ≤ 38″, houses ≤ 5″); Emulator-tested: eleven longitudes device == host, bit for bit |
| FM engine `core/fm`: 4 voices × 10 operators (one per body), any routing (2 phase sources, ring source, averaged feedback per operator), 4 element waves, envelopes, LFO, bend, mod wheel, sustain, chorus, cross-fed echo, reverb, soft knee | Host-tested (incl. 300 random programs under UBSan); Emulator-tested; interrupt measured on the FM-1's costs: mean 2.94 ms, max 3.15 ms of 5.8 (54 %; owner's choice, D12); the GLO clock page shows the real load on hardware |
| Chart → synthesis `core/synth/patch.c`: aspects are the algorithm (37: FREE, BIRTH/SKY/TRANSIT × harmonics 1–12), placements are the timbre, orbit, wheel as stereo field | Host-tested; mapping in docs/ENGINE.md; owner's to change (D13) |
| Horoscope `core/chart`: natal chart from birth data (date, time or unknown, zone, place), transits closest first with applying/separating and days to exact, the month around (±30 days), placements with dignity and retrograde, Moon phase, keyword readings naming their source | Host-tested |
| Instrument `core/app` + `core/ui`: seven pages of line art (Gemini's visual revision adopted and fixed), clocks (D5 states), sky clock (warp ±5 years/s), 64 presets (cast from the chart until saved), latch, octave, mute, USB and TRS MIDI | Host-tested (252 knob turns, each changes a colour); Emulator-tested |
| Firmware `native/astra.c` on Dinghy f77bf03 | Builds (image 83,824 B of 581,564; RAM 62,780 of 98,304; pool 250,368 of 344,064); Emulator-tested. **Not Hardware-tested** |
| `tools/emu_smoke.sh` (CI): controls, state, astronomy bit for bit, silence then the key's pitch, screens, a dial's colour | Passing locally on the real package (fm1-emu 9de1ef0) |
| `tools/emu_cpu.sh` (CI): interrupt on fm1-live FM1_STRICT, under 60 % | Passing locally (54 %) |
| Site `site/` + `tools/site_emu.sh`, `site_front.sh`, `site_shots.sh`, `.github/workflows/pages.yml` (MIKU's pipeline: the page installs exactly what its emulator runs; identity asserted `FM-1_44`) | **Published**: Pages runs 43322931207 (ec95b2b) and 43325598056 (1dc46b6, 4 voices) green and looked at: https://michaelaperry.github.io/ASTRA-FM-1/ serves the package (SHA-256 matches), the installer reports `FM-1_44000000` ready, the emulator boots ASTRA in Chromium. Each later Pages run to be looked at |
| Release / attestation (`release.yml`, `zvenson/fm1-build`) | Not started (D15: after a first hardware test) |
| Companion chart in the browser (WASM core, D6) | Not started |

### Session 3 notes
- This session could run Dinghy's build and fm1-emu itself (the toolchain from pkgman.jieliapp.com, the three SDK
  files from gitee raw links with retries). fm1-live (for FM1_STRICT) needs `libasound2-dev libx11-dev
  libxkbcommon-dev libxcursor-dev libxrandr-dev libxi-dev libwayland-dev` (after `apt-get update`).
- fm1-live's `FM1_BENCH_KEYS` takes one time per event (`600:n0`); `300-1400:n0` is silently ignored, and
  `FM1_BENCH_NOKEYS` clears the script. fm1-emu's `--keys` takes ranges. Under FM1_STRICT, MIDI-in events at 0.4 s did
  not arrive (keys did); not investigated.
- The emulator's WAV starts when the firmware starts its audio, not at boot: the gate times audio from the first sound.
- Gemini's graphics drop (`astra_fm1_op1_gfx.zip`, 2026-11-12) did not compile as given (a `'''` literal) and had: glyph
  circles drawn at the wrong x, the Moon glyph a plain disc, a dark full Moon, doubled strokes smudging 1× text.
  All fixed; dropped facts restored (Porphyry fallback, solar houses, no birth data, feedback rings, ±30 D, globe meridians).
- Interrupt budget (docs/ENGINE.md): 5 voices measured 66 %, 4 voices 54 % (the owner chose 4, D12), 3 voices 42 %.
  Tried and measured slower: folding the mix into the operator; a loop over operators.
- The browser emulator runs at `?mhz=240` (fm1-emu's page defaults to 100 MHz, too slow for ASTRA's interrupt). On this
  session's machine Chromium emulated 0.8x real time at 240 MHz: a fast desktop is needed for clean sound in the browser.
- Readings (D14): original wording of the traditional significations; 777's trump titles quoted (Table VI, from the
  archive.org transcription "Aleister Crowley - Liber 777", whose transcriber states its tables are facsimiles of
  1909; not checked against a 1909 scan). sacred-texts.com and hermetic.com refuse automated fetches.
- Screen: whole frame in the pool, changed rows sent by DMA; ~12 frames/s with four voices sounding (FM1_STRICT).

## The pipeline (copied from MIKU-FM1 at 781760c, its identity replaced)

1. `native/build.sh`: Dinghy f77bf03, identity `FM-1_44XXYYZZ`, store magic "ASTR", copies `core/*` beside the app.
2. `tools/emu_smoke.sh` + `tests/emu_expect.c`: the gate (above); `tools/emu_cpu.sh`: the interrupt on FM-1 costs.
3. `.github/workflows/device.yml`: both, on every push, with evidence artifacts.
4. `.github/workflows/pages.yml`: on push to main: host tests, package (the latest release's, or a dev build while
   none exists), gate, `tools/site_emu.sh` (fm1-emu's web build, a catalog of only this package),
   `tools/site_front.sh` (fills version, SHA-256, build, identity), deploy. **Repository setting needed: Pages →
   Source: GitHub Actions** (the owner said they would set it).
5. Installer: `site/install.html` + `site/js/fm1ota.js`, `fm1pkg.js` (Felucca's, via MIKU, GPL-3.0-only): Web MIDI,
   Chrome/Edge, resumes a cut-off install, returns to official V15 from the user's own file.
6. Release (`release.yml` with `zvenson/fm1-build@v1`, attestations, Firmware Hub listing): not yet (D15).

## Next steps (in order)

1. Look at each push's CI runs (host, device incl. emu_cpu, Pages) before calling a change Emulator-tested or Published.
2. Owner: listen in the browser emulator; D13 (mappings) and D15 (release) remain open.
3. **No hardware flashing without the owner's go-ahead in that session.** After a go-ahead: install with the site's
   installer, check boot, sound, keys, saves; record evidence as MIKU's docs/VALIDATION.md does.
4. Release v0.1 through `zvenson/fm1-build` (MIKU's release.yml), attested; Pages then carries the release package.
5. The browser companion chart (D6): build `core/astro` + `core/chart` to WASM, own SVG chart, profile transfer (D8).
6. Engine ideas measured but not built: a 5th voice (66 %) only after the hardware load meter shows room; effects at half rate (−60 µs);
   the second core of the AC79 (not supported by Dinghy; would need hardware testing).
