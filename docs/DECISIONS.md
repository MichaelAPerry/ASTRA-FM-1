# Owner decisions

The owner decides; agents propose with one recommendation. Record the answer and date here and stop asking.

## Decided
- Licence: GPL-3.0 (repo created with it; fits Dinghy/Felucca, GPL-3.0-only). "ASTRA FM-1" is still a working title.

Owner answers, 2026-11-05:
- **D1 Platform: Dinghy** (as MIKU-FM1).
- **D2 Musical character: wait.** Decide after the vertical slice, with 2–3 demo modes to listen to.
- **D3 Sky → music: approved.** Natal chart = the piece's identity (key, scale, timbre); transits = slow change over it.
  Exact mappings still go to the owner as options.
- **D4 Zodiacs and houses: all modes on the device** (every house system we implement, tropical and sidereal), and
  tropical + sidereal on the website. (Read as: the device is not a reduced subset. Ayanamsa choice: ask when building.)
- **D5 Time: save it**, so the user does not re-enter it every time. Design: store the set time (UTC + time-zone
  offset) in flash and keep it running; if the FM-1 has no battery-backed clock (to verify), at power-on restore the
  saved time and mark it UNCONFIRMED until one press confirms or adjusts it. Never call a stale time "now" silently.
- **D6 Site chart: own SVG** drawn from the shared WASM core.
- **D7 Horoscope text: facts first.** Interpretive sources the owner prefers: **Aleister Crowley and Manly P. Hall.**
  Copyright caveat (to check per text before quoting, not legal advice): neither author's whole body of work is
  public domain. Hall died 1990, so his work is not PD in life+70 countries; in the US only works published before
  1931 (e.g. *The Secret Teachings of All Ages*, 1928) are PD by age, later ones (e.g. *Astrological Keywords*, 1931)
  only if not renewed. Crowley died 1947 (PD in life+70 countries since 2018), but his *Astrology* (with Evangeline
  Adams) was first published in 1974 and may still be in copyright in the US. Plan: quote only texts verified PD in a
  recorded edition (record each in LICENSING.md); otherwise write original rule text citing them as sources.
- **D8 Birth profile transfer: through the install site** (the same USB path as the installer).

Owner, 2026-11-12 (session 3, the build brief): **complete ASTRA around two priorities, a comprehensive personal
horoscope mode and the deepest FM the FM-1 can run, with astrology shaping the synthesis architecture itself
(algorithms, routing, feedback, modulation, spectral motion).** Everything GUI-controlled with colourful, dynamic line
art; **every knob turn visibly changes a colour**; fewer numbers, more waves and geometric figures. **No song mode or
sequencer**: deep FM control, horoscope exploration, preset slots. The agent makes the engineering decisions,
implements, optimises, tests; the instrument before publishing. Then (same day): **push to main, and build the web
page and installer like MIKU-FM1** (the owner enables Pages through Actions). The owner supplied a **visual style
revision made with Gemini** (charcoal ground, instrument panels, thick strokes, filled discs): adopted, with its
bugs fixed (core/ui, core/app/app_draw.c).

Owner, 2026-11-12, later: **4 voices** (D12: 54 % of the audio period accepted; CI ceiling 60 %; the GLO clock page shows
the interrupt's real load on hardware), and **the readings as recommended** (D14: original wording of the traditional
significations, plus Crowley quotations only from texts public domain everywhere: the Tarot trump titles of *777*, 1909).

Read as answers to D2, D10 and D11 for now: the musical character is the chart-driven FM engine (docs/ENGINE.md);
each planet's engine is an operator of a ten-operator voice; the visual style is the line-art pages above. Every
mapping in docs/ENGINE.md stays the owner's to change.

## Open

- D2 (musical character): answered for now by the session-3 brief (above). New questions go below.
- Owner, 2026-11-05: **an FM engine for each planet** (direction; what each engine is and how the sky drives it are
  still open: D10). **Priorities: features and the firmware's visual style before anything else; no flashing and no
  Pages/release work until the build is near done and ready to publish.**

| # | Question | Options | Recommendation |
|---|---|---|---|
| D12 | Polyphony vs. the interrupt rule. Measured on the FM-1's costs (fm1-live FM1_STRICT): 3 voices × 10 operators = 42 % of the audio period, 4 = 54 %, 5 = 66 % | 3 (the rule: under half) · 4 (more chords, the interrupt over half: less room for USB audio and the screen) | **Decided 2026-11-12: 4** (54 %; AGENTS.md's half relaxed by the owner for this; ceiling 60 % in CI; checked on hardware by the GLO load meter) |
| D13 | The chart-to-sound mapping in docs/ENGINE.md (aspect → wire, placement → timbre, orbit, wheel as stereo field) | as built, or any rule changed | Keep as built until heard on hardware |
| D14 | Reading text | **Decided 2026-11-12**: original wording (core/chart/chart.c) + 777's trump titles quoted (Crowley 1909, PD everywhere). Hall: nothing quoted (PD in the US only, not in life+70 countries until 2061) | Done; more Crowley text only from pre-1931 works, each checked |
| D15 | First release (v0.1) and its attestation through `zvenson/fm1-build` (MIKU's release.yml) | after a first hardware test · now | After a first hardware test |
| D10 | The per-planet FM engines | Built as one operator per planet (session 3, the owner's brief); details are D13 | Answered 2026-11-12 |
| D11 | Firmware visual style | Line art, colour on every knob (owner's brief), Gemini revision adopted | Answered 2026-11-12 |
| D9 | Vertical-slice controls and test tone | Replaced by the instrument (session 3) | Closed |

Engineering choices recorded (reversible, not owner decisions): package identity prefix `FM-1_44` (2026-11-05,
survey in HANDOFF); settings store magic "ASTR"; 64 preset slots in Dinghy's 8 blobs; the sky-only
chart when no birth data is set; Lahiri ayanamsa for the sidereal zodiac (D4 said: ask when building; Lahiri
is the common default, one table row to change).
