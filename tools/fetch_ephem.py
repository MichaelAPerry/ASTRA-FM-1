#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Build the planet model's source data (run once; needs the network; the outputs are committed):
#   tools/data/vsop87d_trunc.txt  VSOP87D (Bretagnon & Francou 1988, IMCCE; the CDS distribution VI/81), Mercury..Neptune
#                                 and the Earth, truncated: a term is kept if |A| * 0.22^k >= THRESH (0.22 millennia =
#                                 the farthest point of 1800-2200 from J2000). The originals come from a GitHub mirror of
#                                 VI/81 (CDS refuses this sandbox), pinned by commit and checked by SHA-256; a second,
#                                 independent mirror gave the same bytes (2026-11-05).
#   tools/data/pluto_cheb.txt     Pluto: Chebyshev series fitted to JPL Horizons (DE441) heliocentric vectors of the Pluto
#                                 system barycentre, rotated to the mean ecliptic and equinox of date (Meeus 21.5), so it
#                                 is in VSOP87D's frame. 16 segments of 9154 days from 1800-01-01, degree 10.
# tools/gen_planet_tables.py turns both into core/astro/planet_tables.h.   python3 tools/fetch_ephem.py
import hashlib, math, pathlib, sys, time, urllib.parse, urllib.request
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
MIRROR = "https://raw.githubusercontent.com/ctdk/vsop87/faa11898fc4c683e068fc056a9a68c71534e0d42/VSOP87D."
SHA256 = {"mer": "f468481b5a05080a943ad4746ff7ea7e0ff6652b71a46d83c9c636cb69485e34",
          "ven": "cb2f3a738289ed45f69fec1845e480baf4b32d481eccc21b8629a2d0d10e8261",
          "ear": "8b160c859136d467f2be7fc29efa8a9652e95516dfbde00e4c739d7ddc90ca91",
          "mar": "b1184df9553d85ffcf904c16bd437ab668804fa98859f27fe2e7bf6cfa6bc07e",
          "jup": "3f3dfbc7d117ecad2b2dadf2fc626b260a3cd5efa98e7d4c6b26cd682fc48090",
          "sat": "2e49e19396f24c17298f0b667e7763ee5c28b60d549c89d72a17dfd5f8d46b05",
          "ura": "80eb3a778d53f450066d9b13f17e15b979e253437d22f33ec5a4604cdb2872a7",
          "nep": "3ff65a5cabc04c411f975888f77268b89e336ece0b75d198a3935090fdde3ae6"}
THRESH = {"mer": 1e-5, "ven": 5e-6, "ear": 2e-6, "mar": 5e-6, "jup": 1e-5, "sat": 1e-5, "ura": 1e-5, "nep": 1e-5}
TMAX = 0.22
PLUTO_J0, PLUTO_J1, PLUTO_NSEG, PLUTO_DEG = 2378496.5, 2378496.5 + 16 * 9154, 16, 10   # TT: 1800-01-01 .. 2201-01-02, whole days a segment


def get(url, what):
    for i in range(5):
        try:
            return urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "astra-fm1-gen"}), timeout=120).read()
        except Exception as e:
            print("retry", what, e, file=sys.stderr); time.sleep(5 * (i + 1))
    raise SystemExit(f"fetch failed: {what}")


def vsop():
    out = ["# VSOP87D truncated (tools/fetch_ephem.py): body var(1 L, 2 B, 3 R) power A B C (rad or au; rad; rad/millennium)"]
    for p, th in THRESH.items():
        raw = get(MIRROR + p, p)
        if hashlib.sha256(raw).hexdigest() != SHA256[p]: raise SystemExit(f"VSOP87D.{p}: SHA-256 mismatch")
        var = k = None; kept = total = 0
        for line in raw.decode().splitlines():
            if "VSOP87" in line:
                var = int(line.split("VARIABLE")[1].split()[0]); k = int(line.split("*T**")[1].split()[0]); continue
            a, b, c = line.split()[-3:]; total += 1
            mean_motion = var == 1 and k == 1 and float(c) == 0.0
            if abs(float(a)) * TMAX ** k >= th or mean_motion:
                out.append(f"{p} {var} {k} {a} {b} {c}"); kept += 1
        print(f"VSOP87D.{p}: kept {kept} of {total} terms (threshold {th:g})")
    (ROOT / "tools/data/vsop87d_trunc.txt").write_text("\n".join(out) + "\n")


def precess(jd, v):   # Meeus 21.5 (IAU 1976): J2000 ecliptic -> mean ecliptic and equinox of date
    T = (jd - 2451545.0) / 36525; s = math.radians(1 / 3600)
    eta = (47.0029 * T - 0.03302 * T * T + 0.000060 * T ** 3) * s
    Pi = math.radians(174.876384) - (869.8089 * T - 0.03536 * T * T) * s
    p = (5029.0966 * T + 1.11113 * T * T - 0.000006 * T ** 3) * s
    l = math.atan2(v[1], v[0]); b = math.atan2(v[2], math.hypot(v[0], v[1])); r = math.sqrt(sum(x * x for x in v))
    A = math.cos(eta) * math.cos(b) * math.sin(Pi - l) - math.sin(eta) * math.sin(b)
    B = math.cos(b) * math.cos(Pi - l)
    C = math.cos(eta) * math.sin(b) + math.sin(eta) * math.cos(b) * math.sin(Pi - l)
    l2 = p + Pi - math.atan2(A, B); b2 = math.asin(C)
    return [r * math.cos(b2) * math.cos(l2), r * math.cos(b2) * math.sin(l2), r * math.sin(b2)]


def pluto():
    q = {"format": "text", "COMMAND": "'9'", "EPHEM_TYPE": "VECTORS", "CENTER": "'500@10'", "START_TIME": "'1799-11-01'",
         "STOP_TIME": "'2201-03-01'", "STEP_SIZE": "'10 d'", "TIME_TYPE": "TT", "REF_PLANE": "ECLIPTIC", "REF_SYSTEM": "ICRF",
         "VEC_TABLE": "1", "VEC_CORR": "NONE", "OUT_UNITS": "AU-D", "CSV_FORMAT": "YES"}
    t = get("https://ssd.jpl.nasa.gov/api/horizons.api?" + urllib.parse.urlencode(q), "Horizons Pluto").decode()
    rows = t[t.index("$$SOE") + 5:t.index("$$EOE")].strip().splitlines()
    jd = np.array([float(r.split(",")[0]) for r in rows])
    od = np.array([precess(j, [float(v) for v in r.split(",")[2:5]]) for j, r in zip(jd, rows)])
    seg = (PLUTO_J1 - PLUTO_J0) / PLUTO_NSEG
    out = [f"# Pluto (tools/fetch_ephem.py): JPL Horizons DE441, {len(rows)} vectors; segments J0 {PLUTO_J0} J1 {PLUTO_J1}"
           f" n {PLUTO_NSEG} degree {PLUTO_DEG}; per segment: coordinate (0 x, 1 y, 2 z, au, mean ecliptic of date) c0..cn"]
    worst = 0.0
    for k in range(PLUTO_NSEG):
        a, b = PLUTO_J0 + k * seg, PLUTO_J0 + (k + 1) * seg
        m = (jd >= a - 30) & (jd <= b + 30); x = (2 * jd[m] - a - b) / (b - a)
        for c in range(3):
            cf = np.polynomial.chebyshev.chebfit(x, od[m, c], PLUTO_DEG)
            worst = max(worst, float(np.max(np.abs(np.polynomial.chebyshev.chebval(x, cf) - od[m, c]))))
            out.append(f"{k} {c} " + " ".join(f"{v:.12e}" for v in cf))
    print(f"Pluto: {PLUTO_NSEG} segments, degree {PLUTO_DEG}: worst fit error {worst:.2e} au ({worst / 29 * 206265:.2f}\" at 29 au)")
    (ROOT / "tools/data/pluto_cheb.txt").write_text("\n".join(out) + "\n")


(ROOT / "tools/data").mkdir(exist_ok=True)
vsop()
pluto()
