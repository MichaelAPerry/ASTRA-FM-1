#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# The three files of JieLi's AC79 SDK that Dinghy's build uses (cpu/wl82/tools/uboot.boot, cfg_tool.bin,
# cfg/eq_cfg_hw.bin), from gitee at the pinned commit, each checked against the SHA-256 Dinghy's tools/build.py
# expects. A whole clone of the SDK is hundreds of MB from a host that often stalls or resets; three raw files are
# 15 KB. Every transfer has a time limit; the clone is only the fallback, and it has one too.
#   tools/get_sdk.sh DIR      (then AC79_SDK=DIR)
set -euo pipefail
DIR="${1:?usage: tools/get_sdk.sh DIR}"
COMMIT="${AC79_SDK_COMMIT:-d179b4484759423312073f5fbb232501aa491047}"
TAG="${AC79_SDK_TAG:-AC79NN_SDK_V1.2.1_2023-12-13}"
declare -A SHA=(
  [uboot.boot]=4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4
  [cfg_tool.bin]=276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b
  [cfg/eq_cfg_hw.bin]=41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d
)
T="$DIR/cpu/wl82/tools"
mkdir -p "$T/cfg"
ok=1
for f in "${!SHA[@]}"; do
  for i in 1 2 3 4 5 6; do
    if curl -fsSL --connect-timeout 20 --max-time 60 -o "$T/$f" \
         "https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/raw/$COMMIT/cpu/wl82/tools/$f" \
       && [ "$(sha256sum "$T/$f" | cut -c1-64)" = "${SHA[$f]}" ]; then
      echo "get_sdk: $f ok"; continue 2
    fi
    sleep $((i * 3))
  done
  echo "get_sdk: $f: no verified copy from the raw link"; ok=0
done
[ "$ok" = 1 ] && exit 0
echo "get_sdk: falling back to a shallow clone of $TAG (10 minutes at most a try)"
for i in 1 2 3; do
  rm -rf "$DIR.clone"
  if timeout 600 git clone -q --depth 1 --branch "$TAG" https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git "$DIR.clone" \
     && [ "$(git -C "$DIR.clone" rev-parse HEAD)" = "$COMMIT" ]; then
    for f in "${!SHA[@]}"; do
      [ "$(sha256sum "$DIR.clone/cpu/wl82/tools/$f" | cut -c1-64)" = "${SHA[$f]}" ] || { echo "get_sdk: $f differs in the clone"; exit 1; }
      cp "$DIR.clone/cpu/wl82/tools/$f" "$T/$f"
    done
    rm -rf "$DIR.clone"; echo "get_sdk: from the clone"; exit 0
  fi
  sleep $((i * 15))
done
echo "get_sdk: the AC79 SDK files could not be fetched"; exit 1
