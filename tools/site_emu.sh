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
# the SMK-37 page: not this firmware's board. An empty catalog (the FM-1 page reads it for its SMK-37 links) and a
# note where the emulator's SMK-37 tab points, so neither is a 404
rm -rf "$SITE/fm1-emu/smk37" && mkdir -p "$SITE/fm1-emu/smk37"
echo '{"note": "ASTRA FM-1 is built for the FM-1 only.", "firmwares": []}' > "$SITE/fm1-emu/smk37/firmwares.json"
cat > "$SITE/fm1-emu/smk37/index.html" <<'HTML'
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ASTRA FM-1: FM-1 only</title><link rel="stylesheet" href="../../site.css"></head>
<body><main><section class="card"><h2>FM-1 only.</h2><p>ASTRA FM-1 is built and tested for the M-VAVE FM-1, not the SMK-37.</p>
<p><a class="btn" href="../?mhz=240">Back to the FM-1 emulator</a> <a class="btn alt" href="../../">ASTRA FM-1</a></p></section></main></body></html>
HTML
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
