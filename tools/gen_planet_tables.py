#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Generate core/astro/planet_tables.h: everything core/astro/sky.c and houses.c need, in the device's integer units.
#   - Mercury..Neptune and the Earth: the truncated VSOP87B series (heliocentric L, B, R, ecliptic and equinox J2000;
#     Bretagnon & Francou 1988) as Astronomy Engine carries them (Don Cross, MIT; pip install astronomy-engine).
#     The numbers are read from that package at generation time; none of its code is in the firmware.
#   - Pluto: a Chebyshev fit (degree 40, 1799-12 .. 2201-02) of its barycentric position, sampled from Astronomy
#     Engine; the device subtracts the Sun's barycentric offset, which it gets from its own Jupiter..Neptune.
#   - the CORDIC arctangent table, precession (IAU 2006), obliquity, sidereal time, the Lahiri ayanamsa.
# Units: angles BAM (2^32 = 360 deg), distances AU in Q25, rates BAM per day (integer + /4096 fraction).
#   pip install astronomy-engine==2.1.19 && python3 tools/gen_planet_tables.py   (CI checks the header is fresh with this version)
import math, pathlib
import astronomy as A
import astronomy.astronomy as AI

BAM = 2**32 / 360.0
RAD = 2**32 / (2 * math.pi)
Q25 = 1 << 25


def bam(deg):
    return round((deg % 360.0) * BAM) & 0xFFFFFFFF


def lin(name, a0, a1, a2=0.0):
    """angle (deg) = a0 + a1*T + a2*T^2, T in Julian centuries TT from J2000: the astro_lin_t of astro_tables.h"""
    r = a1 / 36525.0 * BAM
    ri = math.floor(r)
    rf = round((r - ri) * 4096)
    if rf == 4096:
        ri, rf = ri + 1, 0
    return f"  /* {name} */ {{0x{bam(a0):08X}u, 0x{ri & 0xFFFFFFFF:08X}u, {rf}, {round(a2 * BAM / 4)}}},"


def rate(c_rad_per_millennium):
    r = c_rad_per_millennium * RAD / 365250.0
    ri = math.floor(r)
    rf = round((r - ri) * 4096)
    if rf == 4096:
        ri, rf = ri + 1, 0
    assert 0 <= ri < 2**31
    return ri, rf


NAMES = ["MERCURY", "VENUS", "EARTH", "MARS", "JUPITER", "SATURN", "URANUS", "NEPTUNE"]
terms, ser, lpoly = [], [0], []
for name, model in zip(NAMES, AI._vsop):
    for ci, formula in enumerate((model.lon, model.lat, model.rad)):
        poly = [0.0, 0.0, 0.0]
        for k in range(3):
            tl = formula.seriesList[k].termList if k < len(formula.seriesList) else []
            for (a, p, f) in tl:
                if ci == 0 and f == 0.0:            # a constant of the longitude: into the polynomial (kept exact)
                    poly[k] += a * math.cos(p)
                    continue
                amp = round(a * (Q25 if ci == 2 else RAD))
                assert abs(amp) < 2**31, (name, ci, k, a)
                ri, rf = rate(f)
                terms.append(f"  {{{amp}, 0x{round((p % (2 * math.pi)) * RAD) & 0xFFFFFFFF:08X}u, {ri}u, {rf}}},")
            ser.append(len(terms))
        if ci == 0:
            d = 180.0 / math.pi
            lpoly.append(lin(name + "_L", poly[0] * d, poly[1] * d / 10.0, poly[2] * d / 100.0))

# ---- Pluto: barycentric, ecliptic J2000, Chebyshev by the discrete cosine sum at the Chebyshev nodes (pure Python:
# the same on every machine, so CI can check this header is fresh)
ROT = A.Rotation_EQJ_ECL()
T0 = A.Time.Make(1799, 12, 1, 0, 0, 0).tt
T1 = A.Time.Make(2201, 2, 1, 0, 0, 0).tt
MID, HALF = (T0 + T1) / 2, (T1 - T0) / 2
NODES, DEG = 256, 40
PQ = 1 << 23


def ecl(v, t):
    r = A.RotateVector(ROT, A.Vector(v.x, v.y, v.z, t))
    return (r.x, r.y, r.z)


samples = []
for j in range(NODES):
    x = math.cos(math.pi * (j + 0.5) / NODES)
    t = A.Time.FromTerrestrialTime(MID + HALF * x)
    samples.append(ecl(A.BaryState(A.Body.Pluto, t), t))
cheb = [[(2.0 / NODES) * math.fsum(samples[j][c] * math.cos(math.pi * k * (j + 0.5) / NODES) for j in range(NODES))
         for k in range(DEG + 1)] for c in range(3)]
for c in range(3):
    cheb[c][0] /= 2
chebq = [[round(v * PQ) for v in cheb[c]] for c in range(3)]

GM_SUN = 0.2959122082855911e-03
GM = {"JUPITER": 0.2825345909524226e-06, "SATURN": 0.8459715185680659e-07, "URANUS": 0.1292024916781969e-07,
      "NEPTUNE": 0.1524358900784276e-07}
MTOT = GM_SUN + sum(GM.values())
mu = {k: round(v / MTOT * (1 << 30)) for k, v in GM.items()}


def check_pluto():
    """the device's recipe in floating point against Astronomy Engine's heliocentric Pluto: the model error"""
    worst, bmax = 0.0, 0.0
    bodies = {"JUPITER": A.Body.Jupiter, "SATURN": A.Body.Saturn, "URANUS": A.Body.Uranus, "NEPTUNE": A.Body.Neptune}
    for i in range(400):
        tt = T0 + (T1 - T0) * (i + 0.5) / 400
        t = A.Time.FromTerrestrialTime(tt)
        x = (tt - MID) / HALF
        p = []
        for c in range(3):
            b1 = b2 = 0.0
            for k in range(DEG, 0, -1):
                b1, b2 = chebq[c][k] / PQ + 2 * x * b1 - b2, b1
                bmax = max(bmax, abs(b1))
            p.append(chebq[c][0] / PQ + x * b1 - b2)
        for nm, body in bodies.items():
            h = ecl(A.HelioVector(body, t), t)
            for c in range(3):
                p[c] += GM[nm] / MTOT * h[c]          # minus the Sun's barycentric position (-sum mu r)
        ref = ecl(A.HelioVector(A.Body.Pluto, t), t)
        worst = max(worst, math.sqrt(sum((p[c] - ref[c]) ** 2 for c in range(3))))
    assert bmax < 200, bmax                           # Clenshaw stays inside Q23's 256 AU
    return worst, bmax


worst, bmax = check_pluto()
print(f"pluto: model against Astronomy Engine, 400 dates: max {worst:.2e} AU (about {worst / 30 * 206265:.1f} arcsec), "
      f"Clenshaw max {bmax:.1f} AU")
assert worst < 1e-3

atan_tab = [round(math.atan(2.0 ** -i) * RAD) for i in range(30)]
gain = 1.0
for i in range(30):
    gain *= math.sqrt(1 + 4.0 ** -i)

out = ["/* SPDX-License-Identifier: GPL-3.0-only */", "/* Generated by tools/gen_planet_tables.py: do not edit. */",
       "#ifndef PLANET_TABLES_H", "#define PLANET_TABLES_H",
       "/* VSOP87B, truncated (Bretagnon & Francou; the truncation is Astronomy Engine's, MIT): amplitude (L, B: BAM;",
       " * R: AU Q25), phase (BAM), rate per day (BAM, integer and /4096 fraction) */",
       "typedef struct { int32_t amp; uint32_t ph, ri; uint32_t rf; } astro_vterm_t;",
       f"static const astro_vterm_t vsop_terms[{len(terms)}] = {{"] + terms + ["};",
       "/* series bounds: body (Mercury Venus Earth Mars Jupiter Saturn Uranus Neptune) x (L B R) x (t^0 t^1 t^2) */",
       f"static const uint16_t vsop_ser[{len(ser)}] = {{" + ",".join(map(str, ser)) + "};",
       "/* mean longitudes (the series' constants), as astro_lin_t */",
       "static const astro_lin_t vsop_l[8] = {"] + lpoly + ["};",
       f"#define PLUTO_DEG {DEG}",
       f"#define PLUTO_MID64 {round(MID * 64)}      /* the fit's middle and half width, in 1/64 day TT from J2000 */",
       f"#define PLUTO_HALF64 {round(HALF * 64)}",
       "/* Pluto, barycentric, ecliptic J2000: Chebyshev coefficients, AU Q23 */",
       f"static const int32_t pluto_cheb[3][PLUTO_DEG + 1] = {{"]
out += ["  {" + ",".join(map(str, chebq[c])) + "}," for c in range(3)]
out += ["};",
        "/* mass / (Sun + Jupiter..Neptune), Q30: the Sun's barycentric position is minus the sum of these times r */",
        "static const int32_t sun_mu[4] = {" + ",".join(str(mu[k]) for k in ("JUPITER", "SATURN", "URANUS", "NEPTUNE")) + "};",
        "static const uint32_t atan_tab[30] = {" + ",".join(f"{v}u" for v in atan_tab) + "};",
        f"#define CORDIC_INV_GAIN {round((1 << 30) / gain)}      /* 1 / the CORDIC gain, Q30 */",
        f"#define LIGHT_DAYS_Q30 {round(0.0057755183 * (1 << 30))}  /* light time for 1 AU in days, Q30 */",
        "/* IAU 2006: general precession in longitude from J2000, the ecliptic's ascending node on the J2000 ecliptic,",
        " * mean obliquity; Greenwich mean sidereal time (rate beyond one turn a day, on UT days from J2000);",
        " * Lahiri ayanamsa (23.85709 deg at J2000, mean, then the precession) */",
        "static const astro_lin_t sky_el[] = {",
        lin("PREC", 0.0, 5028.796195 / 3600, 1.1054348 / 3600),
        lin("ECL_NODE", 174.874109, -867.95758 / 3600, 0.157992 / 3600),
        lin("OBLIQUITY", 84381.406 / 3600, -46.836769 / 3600, -0.0001831 / 3600),
        lin("GMST", 280.46061837, 0.98564736629 * 36525, 0.000387933),
        lin("AYAN_LAHIRI", 23.85709, 5028.796195 / 3600, 1.1054348 / 3600),
        "};",
        f"#define ECL_TILT_CY {round(46.998973 / 3600 * BAM)}       /* the ecliptic's tilt to J2000's, BAM per century */",
        f"#define NUT_OBL_OM {round(9.20 / 3600 * BAM)}        /* nutation in obliquity: cos Om, cos 2L(sun) (BAM) */",
        f"#define NUT_OBL_2L {round(0.57 / 3600 * BAM)}",
        "#endif", ""]
p = pathlib.Path(__file__).resolve().parent.parent / "core/astro/planet_tables.h"
p.write_text("\n".join(out))
print("wrote", p, len(terms), "terms")
