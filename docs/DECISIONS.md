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

## Open

- D2 (musical character) after the vertical slice. New questions go below.
- Owner, 2026-11-05: **an FM engine for each planet** (direction; what each engine is and how the sky drives it are
  still open: D10). **Priorities: features and the firmware's visual style before anything else; no flashing and no
  Pages/release work until the build is near done and ready to publish.**

| # | Question | Options | Recommendation |
|---|---|---|---|
| D10 | The per-planet FM engines: what each planet's voice is (operators, timbre, register) and what drives it | To present with demo modes on the built engine | After the engine foundation (HANDOFF step 4) |
| D11 | Firmware visual style and main screen layouts | To present as real emulator captures | After planets + aspects exist to show |
| D9 | Vertical-slice controls, layout and test tone (native/astra.c): K1..K4 + SELECT edit year/month/day/hour/minute, PLAY sets; Moon sign → A3..G#4 sine | All TEMPORARY engineering choices, for testing only | Keep until D2 and the interface proposal (HANDOFF step 8); not a design |

Engineering choices recorded (reversible, not owner decisions): package identity prefix `FM-1_44` (2026-11-05,
survey in HANDOFF); settings store magic "ASTR".
