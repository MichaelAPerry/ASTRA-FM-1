# The engine: the chart as an FM synthesizer

How ASTRA turns a sky into sound, what it costs on the FM-1, and where each rule lives in the code. Status words as in
AGENTS.md. The firmware is Hardware-tested (owner's report, 2026-11-13: it installs and works); the interrupt costs below
are still the emulator's measurements until the GLO page's load meter is read on hardware.

## Layers (all in `core/`, integer only, the same sources the host tests build)

| Layer | Files | What it does | Status |
|---|---|---|---|
| Sky | `core/astro/astro.c`, `sky.c` | Sun, Moon, Mercury–Pluto, mean node; obliquity, sidereal time, ASC/MC, 7 house systems, aspects | Host-tested (docs/ACCURACY.md), Emulator-tested (bit-exact) |
| Charts | `core/chart/chart.c` | casting (tropical / sidereal Lahiri), birth data, harmonic aspects, dignities, the reading rules | Host-tested |
| Patch | `core/synth/patch.c` | chart → patch (`patch_cast`), patch + charts → engine program (`patch_derive`) | Host-tested |
| Engine | `core/fm/fm.c` | 4 voices × 10 operators, any routing, envelopes, LFO, chorus, echo, reverb | Host-tested, Emulator-tested |
| Instrument | `core/app/`, `core/ui/` | panel, clocks, sky clock, 64 presets, seven pages of line art | Host-tested, Emulator-tested |
| Glue | `native/astra.c` | Dinghy app: interrupt, panel, MIDI, screen diff, flash | Emulator-tested |

## The operator

Ten per voice, one per body (Sun, Moon, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune, Pluto). Every sample:

```
phase += step
p      = phase + feedback·(last two outputs) + depthA·out[srcA] + depthB·out[srcB]      phase modulation
s      = wave[p] · amplitude                     (envelope × level × velocity × key scaling × tremolo, ramped)
out    = s · ((1 − ring) + ring · out[srcR])                                              ring / amplitude modulation
left  += out · gainL;  right += out · gainR                                               its own stereo place
```

Sources are any operator, itself included. Operators run in order, so a lower-numbered source is this sample's
output and any other is the last sample's: every loop of any shape is one sample long, so no routing is special and
none can stall the renderer. Waves: AIR (sine), FIRE (saw-like, 4 harmonics), EARTH (square-like, odd harmonics),
WATER (sine with 2nd and 4th in quadrature). Phase-modulation depth reaches 4 cycles per full-scale source.

## The chart is the algorithm (`patch_derive`)

`ALGORITHM` picks one of 37: FREE (the operator pages' own wires), or the aspects of the BIRTH chart, of the SKY now,
or of the TRANSITS (sky to birth), each in harmonic 1–12 (longitudes × n before the aspects are found: the 5th
harmonic finds quintiles as conjunctions, and so on). Major aspects within the patch's orb become wires:

| Aspect | Wire |
|---|---|
| conjunction | strong phase modulation; a body conjunct its own birth place (transits) is feedback |
| trine | phase modulation, 0.7 |
| sextile | phase modulation, 0.45 |
| square | ring modulation, and a little inverted phase modulation |
| opposition | phase modulation both ways, the return inverted: a loop |

Depth = closeness (exact = full, edge of the orb = 0) × 1 for applying, 0.75 for separating; divided by the
modulator's ratio (a high ratio is bright at a lower index); inverted when the source is retrograde. Within one chart
the faster body modulates the slower (Moon, Mercury, Venus, Sun, Mars … Pluto); in a transit chart the sky
modulates the birth chart. Each operator keeps its two deepest phase sources and its deepest ring source. INDEX and
RING scale all of them.

Also derived every time the sky moves (the program glides, it never steps):
- **orbit**: each operator's level swells as its planet nears its own birth longitude and dips opposite it (ORBIT);
- **stereo**: the wheel is the stereo field, ascendant left, descendant right, centred on the patch's loudness (WIDTH).

## The chart is the timbre (`patch_cast`, a starting point every knob can change)

| From | To |
|---|---|
| solar house (signs counted from the Sun's) | ratio 1–12 (the Sun is always 1) |
| degree within the sign | detune ±15 cents |
| element of the sign | wave (fire FIRE, earth EARTH, air AIR, water WATER) |
| modality of the sign (+ the body's speed) | envelope: cardinal fast, fixed slow, mutable in between |
| dignity | level (±) and feedback (domicile, exaltation; more when retrograde) |
| house (angular, succedent, cadent) | output level |

Preset slots never saved are cast from the birth chart (else the sky) in four characters by slot: plain, pad,
struck, moving (warp a day a second, the sky algorithm, deep orbit). TEMPORARY in the sense of DECISIONS D3/D10:
these are the engineering mapping the owner can change; they are documented, tested and reversible.

## Cost on the FM-1 (measured, `tools/emu_cpu.sh`)

fm1-live with `FM1_STRICT=1`: each instruction costs what harbor's probes measured on a real FM-1 (loads 1.25,
multiplies 1.9, taken branches ~5 cycles at 240 MHz), flash-cache misses included. The firmware times its own
interrupt with the 24 MHz counter. The per-sample loop runs from RAM (`.ram_text`), unrolled over the ten operators,
reading each operator's record through a volatile pointer (JieLi clang otherwise hoists 40 words to the stack and
reloads them: a third slower, measured).

| Voices sounding (10 operators each) | Audio interrupt, of 5,805 µs |
|---|---|
| 0 (effects and platform only) | 361 µs (6 %); the reverb is 120 µs of it |
| 1 | 1,061 µs (18 %) |
| 3 | mean 2,293 µs, max 2,451 µs (42 %), whole firmware |
| 4 (the build) | mean 2,938 µs, max 3,150 µs (54 %), whole firmware |
| 5 | 3,857 µs (66 %) |

AGENTS.md keeps the interrupt under half its period; the owner chose 4 voices at 54 % (DECISIONS D12, 2026-11-12):
40 operators every sample. CI fails above 60 %. Why there is a margin at all: the cost model leaves out load-use
stalls, and this loop loads a value and uses it at once dozens of times a sample, so the FM-1 may be slower than
measured; the 10 kHz timer interrupt (keys, USB audio) runs inside the audio interrupt's time; and the screen and
the sky are computed in what is left. The GLO clock page shows the interrupt's peak and mean load, measured by the
firmware itself: the first hardware session reads the real figure there. Tried and measured: folding the mix into
the operator (slower: the compiler spills), and the plain loop over operators (slower: a taken branch costs five loads).

The screen: a full frame is drawn into a 240 × 240 buffer in the pool and only the rows that changed go out by DMA
while the main loop runs on (`native/astra.c`). With four voices sounding the main loop still draws ~12 frames a
second on the FM-1's costs.

## Tests

- `tests/test_fm.c`: pitch, ratios, the Bessel zero of phase modulation, ring sidebands, feedback, loops, envelopes,
  velocity, stealing, sustain, bend, gliding levels, determinism, and 300 random programs under the
  undefined-behaviour sanitizer (an overflow anywhere in the audio path fails).
- `tests/test_patch.c`: casting rules, wires only where aspects are, the faster body first, returns as feedback,
  every algorithm on three skies bounded and sounding, the orbit.
- `tests/test_app.c`: the panel, clocks, presets, latch, MIDI, the sky clock, and every knob on every page changes a
  colour (252 turns).
- `tools/emu_smoke.sh` (CI, real package in fm1-emu): controls, state, eleven longitudes bit for bit, silence then
  the held key's pitch, screen changes, a knob's dial changing colour.
- `tools/emu_cpu.sh` (CI): the interrupt on the FM-1's costs, under half.
