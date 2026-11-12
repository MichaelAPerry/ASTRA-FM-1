# Calculation accuracy

Measured, not claimed. Reference: Astronomy Engine 2.x (Don Cross, MIT; VSOP87 and a full lunar theory), via
`tools/gen_reference.py`, 120 dates 1800–2200 (9 fixed incl. J2000.0, Meeus's examples, 1800-01-01, 2199-12-31; 111
seeded-random). Checked by `tests/test_astro.c` on every push. Quantity: geocentric apparent ecliptic longitude of date.

| Body | Method (integer, core/astro) | Max error | Mean error | Test tolerance | Status |
|---|---|---|---|---|---|
| Sun (`astro_sky`) | VSOP87B Earth (truncated), light time, aberration, precession, nutation | 0.0003° (1.0″) | 0.0001° | 0.003° | Host-tested, Emulator-tested (bit-exact) |
| Moon | Meeus ch. 47, 34 largest of 60 longitude terms + 3 additive + nutation | 0.0105° (38″) | 0.0027° | 0.02° | Host-tested, Emulator-tested |
| Mercury | VSOP87B truncated (Astronomy Engine's), geocentric, as the Sun | 0.0002° (0.9″) | 0.0001° | 0.012° | Host-tested, Emulator-tested |
| Venus | 〃 | 0.0005° (1.6″) | 0.0001° | 0.012° | 〃 |
| Mars | 〃 | 0.0005° (1.7″) | 0.0001° | 0.012° | 〃 |
| Jupiter | 〃 | 0.0002° (0.9″) | 0.0001° | 0.012° | 〃 |
| Saturn | 〃 | 0.0002° (0.9″) | 0.0001° | 0.012° | 〃 |
| Uranus | 〃 | 0.0003° (1.2″) | 0.0001° | 0.012° | 〃 |
| Neptune | 〃 | 0.0003° (1.1″) | 0.0002° | 0.012° | 〃 |
| Pluto | Chebyshev fit (degree 40, 1799–2201) of its barycentric position, minus the Sun's offset | 0.0013° (4.8″) | 0.0004° | 0.012° | 〃 |
| Daily motion (retrograde) | difference over 3 h | Moon 0.033°/day, Mercury 0.012°/day, others ≤ 0.006°/day; the retrograde sign always right where \|motion\| > 0.01°/day | | 0.02°/day (Moon 0.25) | Host-tested |
| Mean node | Meeus ch. 22 Ω | (the mean node by definition) | | | Host-tested |
| ASC / MC | sidereal time (IAU, from the UT seconds), true obliquity, closed forms | 0.0007° (2.4″) | | 0.01° | Host-tested |
| Placidus, Koch, Regiomontanus, Campanus | iteration (Placidus), closed forms | 0.0005°, 0.0013°, 0.0004°, 0.0009° | | 0.012° | Host-tested, 60 dates and places to ±62° |
| Porphyry, Equal, Whole Sign | arithmetic on ASC and MC | exact given ASC/MC | | | Host-tested |

The house references are not another library's output: `tools/gen_reference.py` finds each cusp from its
geometric definition by root finding on unit vectors (only the sidereal time and obliquity come from Astronomy
Engine). Placidus and Koch do not exist where some degrees never rise or set: there the device falls back to
Porphyry and says so on the screen. Sidereal zodiac: Lahiri ayanamsa, 23.85709° at J2000 plus IAU 2006 precession
(checked at J2000 and 1950).

Also checked: Meeus example 25.a (Sun, < 0.002°) and 47.a (Moon geometric and apparent, < 0.01°); the sine table
(< 3e-6 over 4096 angles); calendar and J2000.0 conversions; sign boundaries at 0°/360°.

Limits, stated:
- Supported range **1800–2200**. Outside it the T² terms overflow their scaling and Delta T is unmodelled.
- Delta T (TT − UT): Espenak–Meeus table to 2050, their parabola after. After ~2025 it is a prediction; a 10 s error
  moves the Moon by 0.0014°.
- Input is UTC. Local time zones/DST are the caller's job (device: user-entered offset; site: the browser's tz data).
- Cost on the host build: 5.5 KB code+tables (x86-64 -O2), 0 bytes RAM.
- **Device size, measured 2026-11-05** (JieLi clang 4.0.1, `-target pi32v2`, Dinghy's flags `-Os -ffunction-sections
  -fno-builtin`; `core/astro/astro.c` alone, before `astro_civil_from_days` was added): **1,606 B code + 2,536 B tables
  = 4,142 B flash, 0 B RAM**; with `astro_civil_from_days` (+176 B, for the clock) **4,318 B**. No undefined symbols (no 64-bit helpers, no libc). That is 0.7 % of Dinghy's 568 KB
  code+constants budget. The whole app (`native/astra.c` with Dinghy's platform) compiles for the device with no
  warnings and no helper calls. Linked (CI, Dinghy's checks all ok): whole app image 60,180 B of 581,564; RAM
  .data+.bss 15,900 B of 98,304 (most of both is Dinghy's platform).
- **Device == host, bit for bit**: in fm1-emu the firmware's Sun and Moon for 2026-11-05 12:00:01 UTC equal the host
  build's (`tools/emu_smoke.sh`, every push). So the host accuracy figures above hold for the device's output too.
  CPU cost on the device: to be measured (`fm1_ticks()` around one Sun+Moon evaluation); it runs in the main loop,
  never the ISR.
- Precision for astrology: zodiac positions are shown to the arcminute (1′ = 0.0167°), so the Sun and Moon are
  within one displayed arcminute nearly always; the Moon can be off by up to ~1′ at its worst.

- **Device == host, bit for bit, for all eleven bodies** (Sun..Pluto and the node) at the instant the emulator gate
  reaches (`tools/emu_smoke.sh`, CI on every push). Code size of the whole sky (`astro.c` + `sky.c` with the VSOP
  and Pluto tables): 16 KB (astro.c 4.6 KB, sky.c 11.4 KB, JieLi clang -Os) of the 568 KB image; RAM: none beyond the stack. A whole chart (`chart_cast`: eleven
  bodies with motions, houses) runs in the main loop, never the audio interrupt.
