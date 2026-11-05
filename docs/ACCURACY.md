# Calculation accuracy

Measured, not claimed. Reference: Astronomy Engine 2.x (Don Cross, MIT; VSOP87 and a full lunar theory), via
`tools/gen_reference.py`, 120 dates 1800–2200 (9 fixed incl. J2000.0, Meeus's examples, 1800-01-01, 2199-12-31; 111
seeded-random). Checked by `tests/test_astro.c` on every push. Quantity: geocentric apparent ecliptic longitude of date.

| Body | Method (integer, core/astro) | Max error | Mean error | Test tolerance | Status |
|---|---|---|---|---|---|
| Sun | Meeus ch. 25 low precision + aberration + nutation (4 terms) | 0.0075° (27″) | 0.0025° | 0.01° | Host-tested |
| Moon | Meeus ch. 47, 34 largest of 60 longitude terms + 3 additive + nutation | 0.0105° (38″) | 0.0027° | 0.02° | Host-tested |
| Planets | — | — | — | — | Designed (HANDOFF) |
| Ascendant / houses | — | — | — | — | Designed |

Also checked: Meeus example 25.a (Sun, < 0.002°) and 47.a (Moon geometric and apparent, < 0.01°); the sine table
(< 3e-6 over 4096 angles); calendar and J2000.0 conversions; sign boundaries at 0°/360°.

Limits, stated:
- Supported range **1800–2200**. Outside it the T² terms overflow their scaling and Delta T is unmodelled.
- Delta T (TT − UT): Espenak–Meeus table to 2050, their parabola after. After ~2025 it is a prediction; a 10 s error
  moves the Moon by 0.0014°.
- Input is UTC. Local time zones/DST are the caller's job (device: user-entered offset; site: the browser's tz data).
- Cost on the host build: 5.5 KB code+tables (x86-64 -O2), 0 bytes RAM. Device size: to be measured with the JieLi
  toolchain (HANDOFF next step 1). CPU cost on the device: to be measured; it runs in the main loop, never the ISR.
- Precision for astrology: zodiac positions are shown to the arcminute (1′ = 0.0167°), so the Sun and Moon are
  within one displayed arcminute nearly always; the Moon can be off by up to ~1′ at its worst.
