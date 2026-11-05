#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/web
python3 -m ziglang cc -target wasm32-freestanding -Oz -nostdlib shared/astro.c \
  -Wl,--no-entry -Wl,--export=astra_day -Wl,--export=astra_sun \
  -Wl,--export=astra_separation -Wl,--export=astra_test_hz -o build/web/astro.wasm
