# Validation — first engineering slice

These states are distinct: designed, implemented, host-tested, emulator-tested, hardware-tested, published.
Only the first four apply to this milestone. No real FM-1 has been flashed and no deployment is claimed.

## Host and shared calculation

The Release CTest suite checks independent Astronomy Engine fixtures, Gregorian leap-year/range validation,
angle wraparound, every day's solar motion and midnight continuity throughout 1950–2050. Checks remain enabled
with NDEBUG. An independent Python sweep compares 1,515 UTC samples with Astronomy Engine 2.1.19 SunPosition
(apparent ecliptic of date); maximum sampled discrepancy: **0.012140°**, at 2038-03-01 00:17:31 UTC.
The prototype acceptance tolerance is **0.05°**; this is not a rigorous error bound between samples and not
a precision claim for the unimplemented planets/Moon/houses. See tools/compare_sun.py and committed fixtures.

## Native build

JieLi jieli-linux-toolchains-20260730.1 (clang 4.0.1), AC79 SDK d179b44 with three enforced file hashes,
Dinghy f77bf03. Firmware application: **54,840 / 581,564 bytes**. Static RAM data+bss:
**11,760 / 98,304 bytes**. Pool: **30,720 / 344,064 bytes**. `.ram_text`: 916 instructions, no calls.
Upstream hardware-register checks pass. Package: **612,224 bytes**, provisional identity `FM-1_ASTRADEV`.
Shared browser calculation WASM: **990 bytes**. No float/libgcc or 64-bit division is linked into the native app.

## Real emulator acceptance

tools/emu_smoke.py boots this package using fm1-emu 9de1ef0, presses PLAY, turns SELECT forward by 30 days,
then stops. It checks the actual WAV against expected fixture frequencies (~500 Hz then ~530 Hz), silence
after stop, valid 240×240 LCD frames, changed date/calculation and transport regions, and stores the package
SHA-256 with evidence. Encoder detents take emulated time: a 180-detent sweep had not settled inside the
first test's audio window, so the test now uses 30 detents with time to settle. This was a test-stimulus issue.

Measured run: 107,264 stereo frames at 44.1 kHz; initial pitch 499.992 Hz, changed pitch 530.020 Hz,
stopped RMS 0.0. Both initial and changed LCD values were inspected in the actual emulator output.

The display identifies its initial J2000 fixture and says NOT CURRENT. Boot, calculated solar value,
calculation-to-signal path, date control and stop behavior are exercised. Peripheral bring-up warnings about
unimplemented MMIO are emulator limitations and are retained in boot.log; faults or incorrect behavior fail
the functional checks. Emulator ISR timing is not device CPU performance.

## Browser and publication

The site build embeds the same smoke-tested package in the real WebAssembly emulator. Browser tests exercise
date calculation, emulator startup, version/checksum consistency, service-worker readiness and reload with the
network disabled. Test evidence lives in build/ and is uploaded by CI. GitHub workflow execution and actual
Pages publication must be verified separately after push/deployment; local success does not establish either.

Local Chromium checks passed: J2000 and +30-day WebAssembly results, package SHA-256/catalog consistency,
fresh real-emulator startup with networking disabled, no JavaScript page errors and a 390-pixel mobile
layout. Testing found and fixed cache lookup for directory URLs and wrapping of the package checksum on
small screens. Host tests also passed AddressSanitizer and UndefinedBehaviorSanitizer.

## Not implemented or tested

Hardware installation/update identity, physical audio/CPU/latency/power loss, ambient synthesis, battery-backed
timekeeping, UTC profile transfer, planets/Moon/retrogrades, natal charts, location-dependent houses,
aspect exactness/orbs, personalized interpretation, long sessions and release attestations on GitHub.
These remain explicit next milestones, not silent reductions of the project requirements.
