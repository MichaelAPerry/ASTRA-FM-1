#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Adapted from MIKU-FM1 tools/site_front.sh (MIT, same owner).
# Install the front page (site/index.html), the installer page (site/install.html) and their assets (site.css, js/,
# media/) into SITE, filling in the version, the build kind and the commit, the SHA-256 and the identity of the package.
# The identity is decoded from the package bytes and must be ASTRA's (FM-1_44...): the installer refuses anything else.
# release: the attested package of a published release. dev: built by the Pages workflow from a commit of main (not attested).
#   tools/site_front.sh SITE_DIR VERSION [release|dev] [COMMIT]
set -eu
SITE="${1:?usage: tools/site_front.sh SITE_DIR VERSION [release|dev] [COMMIT]}"; V="${2:?version}"
KIND="${3:-release}"; COMMIT="${4:-}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PKG="astra-$V.fwsc"
[ -f "$SITE/firmware/$PKG" ] || { echo "site_front.sh: no $SITE/firmware/$PKG"; exit 1; }
python3 - "$ROOT" "$SITE" "$V" "$PKG" "$KIND" "$COMMIT" <<'PY'
import hashlib, html, json, shutil, sys
from pathlib import Path
root, site, v, pkg, kind, commit = Path(sys.argv[1]), Path(sys.argv[2]), *sys.argv[3:7]
repo = "https://github.com/MichaelAPerry/ASTRA-FM-1"
raw = (site / "firmware" / pkg).read_bytes()
sha = hashlib.sha256(raw).hexdigest()
if kind == "release":
    build = (f'Release {html.escape(v)}, built and attested on GitHub: '
             f'<a href="{repo}/releases/tag/v{html.escape(v)}">release page</a>, '
             f'<a href="{repo}/attestations">attestations</a>.')
else:
    build = (f'Development build of <a href="{repo}/commit/{html.escape(commit)}">{html.escape(commit[:7])}</a> '
             f'on main: <strong>not attested</strong>. No release has been made yet: the first one follows the first hardware test. '
             f'Until then this development build is what the page installs, and what the emulator above runs.')
product = "".join(chr((raw[i * 0x30 + 0x2F] - i - 1) & 0xFF) for i in range(20) if raw[i * 0x30 + 0x2F] != 0x7D)
assert product.startswith("FM-1_44"), product
for d in ("js", "media"):
    if (site / d).exists(): shutil.rmtree(site / d)
    shutil.copytree(root / "site" / d, site / d)
shutil.copy(root / "site" / "site.css", site / "site.css")
fill = {"{{VERSION}}": html.escape(v), "{{PACKAGE}}": html.escape(pkg), "{{SHA256}}": sha, "{{BUILD}}": build,
        "/*META*/": json.dumps({"version": v, "product": product, "pkg": "firmware/" + pkg})}
for name, keys in (("index.html", ("{{VERSION}}", "{{PACKAGE}}", "{{SHA256}}", "{{BUILD}}")),
                   ("install.html", ("{{VERSION}}", "{{SHA256}}", "{{BUILD}}", "/*META*/"))):
    page = (root / "site" / name).read_text()
    for k in keys:
        assert k in page, (name, k)
        page = page.replace(k, fill[k])
    assert "{{" not in page and "/*META*/" not in page, name
    (site / name).write_text(page)
(site / ".nojekyll").touch()
print(f"site_front: identity {product}, SHA-256 {sha}")
PY
echo "site_front: $SITE/index.html ($KIND $V)"
