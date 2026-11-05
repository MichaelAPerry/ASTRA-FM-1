# Licensing

- This repository: **GPL-3.0** ([LICENSE](LICENSE)). The planned firmware builds on Dinghy (GPL-3.0-only) and the site's
  installer on Felucca's web installer (GPL-3.0-only), so the combined firmware and installer are GPL-3.0-only.
- `core/astro/`: original code. Algorithms and coefficients from J. Meeus, *Astronomical Algorithms* (2nd ed., 1998),
  ch. 22, 25, 47 — published mathematical methods, reimplemented here; Delta T from Espenak & Meeus (NASA).
- Astronomy Engine (Don Cross, MIT) is used only by `tools/gen_reference.py` to produce test reference values
  (`tests/astro_ref.h`); none of its code is in the firmware.
- Check every new dependency's licence and record it here before use.
