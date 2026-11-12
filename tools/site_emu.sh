#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Adapted from MIKU-FM1 tools/site_emu.sh (MIT, same owner).
# Add the firmware package, emulated, to the site: fm1-emu's web build (MIT, github.com/joakimeriksson/fm1-emu) in
# SITE/fm1-emu/, its catalog holding only firmware/astra-VERSION.fwsc: the page runs exactly the package it installs.
#   tools/site_emu.sh SITE_DIR VERSION [FM1_EMU_DIR]   (FM1_EMU_DIR: a clone at FM1_EMU_REV; cloned if absent)
set -eu
SITE="${1:?usage: tools/site_emu.sh SITE_DIR VERSION [FM1_EMU_DIR]}"; V="${2:?version}"
REV="${FM1_EMU_REV:-9de1ef053a3c4bf56f16f05b5f0de193f4608a1d}"; EMU="${3:-build/fm1-emu-src}"
[ -f "$SITE/firmware/astra-$V.fwsc" ] || { echo "site_emu.sh: no $SITE/firmware/astra-$V.fwsc"; exit 1; }
[ -d "$EMU/.git" ] || git clone -q https://github.com/joakimeriksson/fm1-emu "$EMU"
git -C "$EMU" -c advice.detachedHead=false checkout -q "$REV"
rustup target add wasm32-unknown-unknown >/dev/null 2>&1 || true
bash "$EMU/web/build.sh" "$(cd "$SITE" && pwd)/fm1-emu"
cp "$EMU/LICENSE" "$SITE/fm1-emu/LICENSE"
rm -rf "$SITE/fm1-emu/smk37"                    # (the SMK-37 page: not this firmware's board)
python3 - "$SITE/fm1-emu/firmwares.json" "$V" <<'PY'
import json, sys
json.dump({"note": "ASTRA FM-1 only: the firmware package this site installs, emulated.", "default": "astra",
           "firmwares": [{"id": "astra", "name": "ASTRA FM-1", "version": sys.argv[2], "author": "Michael Perry",
                          "license": "GPL-3.0-only", "repo": "https://github.com/MichaelAPerry/ASTRA-FM-1",
                          "about": "The sky as an FM synthesizer: ten planets, ten operators, the aspects are the algorithm.",
                          "package": f"../firmware/astra-{sys.argv[2]}.fwsc"}]},
          open(sys.argv[1], "w"), indent=1)
PY
echo "site_emu: $SITE/fm1-emu (fm1-emu $REV, astra-$V.fwsc)"
