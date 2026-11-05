# Owner decisions

The owner decides; agents propose with one recommendation. Record the answer and date here and stop asking.

## Decided
- (none yet) — "ASTRA FM-1" is a working title. Licence: the repo was created with GPL-3.0 (fits Dinghy/Felucca, GPL-3.0-only).

## Open

| # | Question | Options | Recommendation |
|---|---|---|---|
| D1 | Firmware platform | Dinghy · fm1-base (MPL template) · COSMOS fork · Felucca | **Dinghy**: proven by MIKU-FM1 end to end; confirm after a first-hand look at COSMOS's astronomy engine |
| D2 | Musical character | slow ambient drones · generative melodic/arpeggiated · rhythmic/sequenced · selectable modes | Ask after the vertical slice; a demo of 2–3 modes makes this concrete |
| D3 | How the sky drives music | planets as voices · aspects as harmony/consonance · Moon as tempo/phase · natal chart sets key/scale, transits modulate | Natal chart = the piece's "identity" (key, scale, timbre), transits = slow change over it: personal and audibly evolving |
| D4 | Zodiac and house systems | tropical/sidereal; Whole-sign, Equal, Placidus, Koch | Tropical + Whole-sign and Placidus on the site; Whole-sign/Equal on the device first |
| D5 | Time on the device | enter date/time at each power-on · sync over USB from the site (no internet) · keep a free-running clock and ask to confirm | Entry at power-on (always works) + optional USB sync from the site |
| D6 | Site charting | own SVG chart (full control, offline, GPL) · an open-source chart library (licence check needed) | Own SVG drawn from the shared WASM core: one source of truth with the device |
| D7 | Horoscope text | none (show the facts: aspects, transits) · rule-based short readings traced to the chart · a tradition chosen by the owner | Facts first; add rule-based lines only in a tradition the owner picks |
| D8 | Birth profile transfer site → FM-1 | USB (WebSerial/WebMIDI SysEx) · typed code · entry on the device | Device entry first; USB transfer once the installer's USB path is understood |
