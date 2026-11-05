# Initial architecture assessment

The starting ASTRA repository contained only LICENSE. No application was replaced. The first milestone is
an engineering prototype, not a finalized feature or aesthetic specification.

## Inspected foundations

| Source inspected | Relevant capability | Tradeoff for ASTRA |
| --- | --- | --- |
| MIKU-FM1 native/build.sh, tools/site_emu.sh, site_front.sh, emu_smoke.sh, workflows/pages.yml and release.yml | PI32 .fwsc; real emulator gate; one package for download and emulator; checksums and attested releases | Proven infrastructure reference. Its application, identity and artwork belong to MIKU; its documentation contains historical validation statements, not evidence for ASTRA |
| Dinghy f77bf03 | Minimal audio ISR, keys, display, storage, MIDI, USB audio/update/rescue, resource checks; native packaging | Small demonstrated baseline; no software floating point or 64-bit division. Used for the first slice, not assumed sufficient for full astrology |
| fm1-base 3c4a57e | MPL-2.0 reusable platform, application build configuration, fastmath, UI/MIDI/storage, optional radios | More capable foundation for float-heavy astronomy; investigate software floating-point CPU/size costs before porting the engine. Disable radio dependencies for essential offline calculations |
| COSMOS 1fbabe1 | GPLv3-or-later app on fm1-base, local sky calculations and JPL approximate planetary elements | Real precedent for offline Sun/Moon/planet calculations. Its weather/network behavior and default-clock strategy are unsuitable as ASTRA requirements; do not copy its musical mappings |
| Felucca a4f007c | GPL-3.0-only full synth, MIT engines, MPL-2.0 BSP, installer/editor ecosystem | More complete DSP but larger existing application. Reuse vetted engines if they fit approved sounds instead of adopting its whole UI |
| fm1-emu 9de1ef0 | CPU/peripheral emulation of actual firmware; scripted controls, WAV/LCD captures, WebAssembly | Necessary gate; cannot establish physical audio quality, flash endurance or hardware timing |

## Astronomy options

The first model follows USNO's approximate solar coordinates: mean longitude and mean anomaly with two
sinusoidal corrections. Angles are unsigned Q32 turns, output millidegrees. A fixed-point sine avoids both
floating point and 64-bit division. The browser compiles **the same C file**, not a separate JS approximation.
UTC civil input is explicit; valid Gregorian dates are checked. Supported prototype interval: 1950–2050.
Geometric approximation versus apparent reference coordinates is reflected in measured error, not hidden.

Astronomy Engine (MIT) offers a much broader validated model, including planets/Moon and observer transforms.
Its double-heavy C implementation needs a compatible arithmetic/runtime layer and actual resource measurement
before committing to device use. Compact Keplerian elements (COSMOS/JPL approach) reduce resources but need
separate lunar corrections, explicit date limits and quantified longitude/retrograde errors. Truncated VSOP
tables offer tunable size/precision but add coefficients and license/provenance work. Swiss Ephemeris has
licensing and data-size implications and is not selected.

Recommendation: retain a platform-neutral calculation API and independent reference fixtures. Evaluate
fm1-base plus Astronomy Engine on the actual compiler against the fixed-point/JPL-element alternative for
the next planetary milestone. Do not promise the solar prototype's precision for planets, Moon, houses or
historical dates beyond the measured range. No essential device calculation will be offloaded to a browser.

## Time, audio and resource contract

Dinghy provides elapsed milliseconds, not evidence of a battery-backed calendar clock. The prototype always
labels its fixed date, including after power loss. It intentionally does not display “now.” Next foundation:
manual UTC calendar entry, explicit time-valid state, bounded elapsed-time advancement, then optional local
USB synchronization. Saved wall time alone cannot determine time elapsed while powered off. No birth-time
timezone/DST conversion is implemented; ambiguous civil times must eventually be resolved explicitly.

Astronomy runs in the main loop on input changes, never in the audio interrupt. Only aligned 32-bit control
values cross into the ISR. The present oscillator is a quiet, smoothed diagnostic signal, not a final ambient
voice, and makes no artistic planet mapping claim. The owner selected slowly evolving ambient textures.
Next propose sustained voices, sparse events and modulation options before assigning chart features to them.

Provisional budget: stay below 60% of flash and static RAM limits, reserve at least 100 KiB of the pool for
future synthesis, and target <50% of each 5.8 ms audio deadline on actual hardware. These are engineering
targets, not measured device performance. Current compiled sizes are in VALIDATION.md. Natal data, all-planet
ephemerides, aspect search, interpretations and persistence have **not** been budgeted by actual builds yet.

## Browser and release contract

The static site compiles shared C to a 990-byte WebAssembly solar engine. Its separate iframe runs fm1-emu's
real firmware binary, with a single-entry local package catalog. The calculator does not synchronize its date
into the emulator yet. No external service receives date/profile data. A versioned service worker caches the
complete artifact; no CDNs/fonts are required. Responsive engineering styling is temporary.

The build refuses to package the site if the firmware checksum differs from the passing smoke report.
Manifest records the source commit/dirty state, platform/SDK/emulator pins and package hash. Cache version
depends on all asset contents, avoiding an old package under a new catalog. Matching staged platform source
and license notices accompany binary artifacts. CI must pass before a Pages artifact is available; pushes
to main deploy that validated artifact automatically. A manual deployment option also exists on main.
The astra branch validates without deploying. No automatic hardware flashing.

Advanced natal wheels, profiles and chart libraries are future work. Prefer a small original SVG renderer for
auditability, or evaluate an MIT chart library against house/aspect requirements before adoption. The final
chart design, house system, interpretation tradition and orb rules need owner decisions.
