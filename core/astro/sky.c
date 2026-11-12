/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA core astronomy: the planets. Integer only (see astro.h): no float, no 64-bit arithmetic.
 *   Mercury..Neptune, the Earth: truncated VSOP87B (heliocentric, ecliptic J2000), spherical series -> rectangular.
 *   Pluto: a Chebyshev fit of its barycentric position, minus the Sun's own offset from the barycentre.
 *   Geocentric: planet - Earth, CORDIC arctangent; then to the ecliptic of date (precession in longitude and the
 *   ecliptic's own tilt), nutation, and light time with aberration to first order (the motion over the light time).
 * Measured errors: docs/ACCURACY.md. Tables: tools/gen_planet_tables.py. */
#include "astro.h"
#include "planet_tables.h"

enum { SKY_PREC, SKY_ECL_NODE, SKY_OBLIQUITY, SKY_GMST, SKY_AYAN };
typedef struct { astro_bam_t lon; int32_t tanb, dist; } geo_t;      /* J2000 longitude, tan(latitude) Q28, AU Q25 */

/* CORDIC, vectoring: the angle of (x, y) and, if r, its length in the units of x and y */
astro_bam_t astro_atan2(int32_t y, int32_t x, int32_t *r) {
    uint32_t ax = x < 0 ? 0u - (uint32_t)x : (uint32_t)x, ay = y < 0 ? 0u - (uint32_t)y : (uint32_t)y, m = ax > ay ? ax : ay;
    astro_bam_t z = 0;
    int32_t sh = 0, i, t;
    if (!m) { if (r) *r = 0; return 0; }
    while (m >= (1u << 29)) { m >>= 1; sh--; }       /* the larger of the two into [2^28, 2^29) */
    while (m < (1u << 28)) { m <<= 1; sh++; }
    if (sh > 0) { x <<= sh; y <<= sh; } else { x >>= -sh; y >>= -sh; }
    if (x < 0) { x = -x; y = -y; z = 0x80000000u; }
    for (i = 0; i < 30; i++) {
        t = x;
        if (y > 0) { x += y >> i; y -= t >> i; z += atan_tab[i]; }
        else { x -= y >> i; y += t >> i; z -= atan_tab[i]; }
    }
    if (r) { x = astro_mulq(x, CORDIC_INV_GAIN, 30); *r = sh > 0 ? x >> sh : x << -sh; }
    return z;
}

astro_bam_t astro_asin(int32_t s) {              /* s Q30, clamped to -1..1 */
    int32_t one = 1 << 30, v, c;
    if (s >= one) return 0x40000000u;
    if (s <= -one) return 0xC0000000u;
    v = astro_mulq(one - s, (one + s) >> 1, 29);   /* 1 - s^2, Q30 */
    c = (int32_t)(astro_isqrt((uint32_t)v) << 15);
    if (c) c = (c >> 1) + (astro_divq(v, c, 30) >> 1);   /* one Newton step: the square root to Q30 */
    return astro_atan2(s, c, 0);
}

static uint32_t term_phase(const astro_vterm_t *k, astro_time_t t) {
    return k->ph + k->ri * (uint32_t)t.days + (uint32_t)(((int32_t)k->rf * t.days) >> 12)
         + (k->ri >> 16) * t.frac16 + (((k->ri & 0xFFFFu) * t.frac16) >> 16);
}
/* one coordinate: sum over k of t^k * sum A cos(phase), t in Julian millennia */
static int32_t series(unsigned s, astro_time_t t) {
    int32_t tm = t.days * 2940 + (int32_t)((t.frac16 * 2940u) >> 16), tp = 1 << 30, sum = 0;   /* 2^30 / 365250 */
    unsigned k, i;
    for (k = 0; k < 3u; k++) {
        int32_t part = 0;
        for (i = vsop_ser[s + k]; i < vsop_ser[s + k + 1u]; i++)
            part += astro_mulq(vsop_terms[i].amp, astro_cos_q30(term_phase(&vsop_terms[i], t)), 30);
        sum += k ? astro_mulq(part, tp, 30) : part;
        tp = astro_mulq(tp, tm, 30);
    }
    return sum;
}
static void helio(unsigned body, astro_time_t t, int32_t v[3]) {   /* AU Q25, ecliptic J2000 */
    unsigned s = body * 9u;
    astro_bam_t l = astro_lin(&vsop_l[body], t) + (uint32_t)series(s, t), b = (uint32_t)series(s + 3u, t);
    int32_t r = series(s + 6u, t), rc = astro_mulq(r, astro_cos_q30(b), 30);
    v[0] = astro_mulq(rc, astro_cos_q30(l), 30);
    v[1] = astro_mulq(rc, astro_sin_q30(l), 30);
    v[2] = astro_mulq(r, astro_sin_q30(b), 30);
}
static void pluto_bary(astro_time_t t, int32_t v[3]) {
    int32_t x = astro_divq(t.days * 64 + (int32_t)(t.frac16 >> 10) - PLUTO_MID64, PLUTO_HALF64, 30), c, k;
    x = x > (1 << 30) ? 1 << 30 : x < -(1 << 30) ? -(1 << 30) : x;
    for (c = 0; c < 3; c++) {                   /* Clenshaw; Q23 holds its 71 AU peak */
        int32_t b1 = 0, b2 = 0, b0;
        for (k = PLUTO_DEG; k > 0; k--) { b0 = pluto_cheb[c][k] + astro_mulq(b1, x, 29) - b2; b2 = b1; b1 = b0; }
        v[c] = (pluto_cheb[c][0] + astro_mulq(b1, x, 30) - b2) * 4;
    }
}
static void geo(const int32_t p[3], const int32_t e[3], geo_t *g) {
    int32_t r;
    g->lon = astro_atan2(p[1] - e[1], p[0] - e[0], &r);
    g->tanb = astro_divq(p[2] - e[2], r, 28);
    astro_atan2(p[2] - e[2], r, &g->dist);
}
static void sky_geo(astro_time_t t, geo_t g[ASTRO_NPLANET]) {
    static const uint8_t ID[8] = {ASTRO_MERCURY, ASTRO_VENUS, 0, ASTRO_MARS, ASTRO_JUPITER, ASTRO_SATURN, ASTRO_URANUS, ASTRO_NEPTUNE};
    int32_t e[3], p[3], sb[3] = {0, 0, 0}, zero[3] = {0, 0, 0};
    unsigned b, c;
    helio(2, t, e);
    geo(zero, e, &g[ASTRO_SUN]);
    for (b = 0; b < 8u; b++) {
        if (b == 2u) continue;
        helio(b, t, p);
        geo(p, e, &g[ID[b]]);
        if (b >= 4u) for (c = 0; c < 3u; c++) sb[c] += astro_mulq(p[c], sun_mu[b - 4u], 30);
    }
    pluto_bary(t, p);
    for (c = 0; c < 3u; c++) p[c] += sb[c];      /* heliocentric = barycentric - Sun, and the Sun is at -sum(mu r) */
    geo(p, e, &g[ASTRO_PLUTO]);
}

void astro_sky(astro_time_t t, astro_sky_t *s) {
    geo_t g0[ASTRO_NPLANET], g1[ASTRO_NPLANET];
    astro_time_t t1 = t;
    astro_bam_t prec = astro_lin(&sky_el[SKY_PREC], t), node = astro_lin(&sky_el[SKY_ECL_NODE], t), m;
    int32_t tilt = astro_mulq(ECL_TILT_CY, astro_t_q16(t), 16), nut = astro_nutation(t);
    unsigned i;
    t1.frac16 += 8192u;                         /* an eighth of a day on: the motion */
    if (t1.frac16 >= 65536u) { t1.frac16 -= 65536u; t1.days++; }
    sky_geo(t, g0);
    sky_geo(t1, g1);
    for (i = 0; i < ASTRO_NPLANET; i++) {
        int32_t v, lt;
        if (i == ASTRO_MOON) continue;
        v = (int32_t)(g1[i].lon - g0[i].lon) * 8;
        lt = astro_mulq(g0[i].dist, LIGHT_DAYS_Q30, 25);            /* light time, days Q30 */
        s->lon[i] = g0[i].lon + prec + (uint32_t)nut
                  + (uint32_t)astro_mulq(astro_mulq(tilt, astro_cos_q30(node - g0[i].lon), 30), g0[i].tanb, 28)
                  - (uint32_t)astro_mulq(v, lt, 30);
        s->spd[i] = v;
    }
    m = astro_moon_lon(t, 1);
    s->lon[ASTRO_MOON] = m;
    s->spd[ASTRO_MOON] = (int32_t)(astro_moon_lon(t1, 1) - m) * 8;
    s->lon[ASTRO_NODE] = astro_mean_node(t);
    s->spd[ASTRO_NODE] = (int32_t)(astro_mean_node(t1) - s->lon[ASTRO_NODE]) * 8;
}

astro_bam_t astro_ayanamsa(astro_time_t t) { return astro_lin(&sky_el[SKY_AYAN], t); }

astro_bam_t astro_obliquity(astro_time_t t) {
    return astro_lin(&sky_el[SKY_OBLIQUITY], t) + (uint32_t)astro_mulq(NUT_OBL_OM, astro_cos_q30(astro_mean_node(t)), 30)
         + (uint32_t)astro_mulq(NUT_OBL_2L, astro_cos_q30(2u * astro_sun_mean_lon(t)), 30);
}

/* Greenwich apparent sidereal time. The turn a day comes from the seconds themselves (frac16 is 1.3 s coarse). */
static astro_bam_t gast(int32_t ut_days, uint32_t ut_sec, astro_time_t *u) {
    uint32_t sn = (ut_sec + 43200u) % 86400u;   /* seconds from the last UT noon: J2000.0 is a noon */
    int32_t s = (int32_t)ut_sec - 43200;
    u->days = ut_days - 10957;
    if (s < 0) { s += 86400; u->days--; }
    u->frac16 = (uint32_t)s * 512u / 675u;
    return astro_lin(&sky_el[SKY_GMST], *u) + sn * 49710u + ((sn * 17670u) >> 16)   /* 2^32 / 86400 = 49710.2696 */
         + (uint32_t)astro_mulq(astro_nutation(*u), astro_cos_q30(astro_lin(&sky_el[SKY_OBLIQUITY], *u)), 30);
}

/* the ecliptic point rising where the sidereal time is th and the latitude's sine and cosine are sf, cf
 * (they may share any positive factor: the house systems scale the tangent) */
static astro_bam_t asc_of(astro_bam_t th, int32_t sf, int32_t cf, int32_t se, int32_t ce) {
    return astro_atan2(astro_mulq(astro_cos_q30(th), cf, 30),
                       -(astro_mulq(astro_mulq(astro_sin_q30(th), ce, 30), cf, 30) + astro_mulq(sf, se, 30)), 0);
}
/* Placidus: the right ascension where a point's distance from the meridian is f/3 of its semi-arc */
static int placidus(astro_bam_t ramc, int32_t f, int below, int32_t tanphi, int32_t se, int32_t ce, astro_bam_t *out) {
    int32_t te = astro_divq(se, ce, 30), ad = 0, i;
    astro_bam_t r = ramc + (below ? 0x80000000u - (uint32_t)f * 0x15555555u : (uint32_t)f * 0x15555555u);
    for (i = 0; i < 40; i++) {
        int32_t x = astro_mulq(tanphi, astro_mulq(te, astro_sin_q30(r), 30), 28), nad;   /* sin AD, Q26 */
        astro_bam_t nr;
        if (x >= (1 << 26) || x <= -(1 << 26)) return 0;            /* never rises or never sets */
        nad = (int32_t)astro_asin(x * 16);
        nr = below ? ramc + 0x80000000u - (uint32_t)(((0x40000000 - nad) / 3) * f) : ramc + (uint32_t)(((0x40000000 + nad) / 3) * f);
        if (i && nad - ad < 64 && ad - nad < 64) { r = nr; break; }
        ad = nad; r = nr;
    }
    *out = astro_atan2(astro_sin_q30(r), astro_mulq(astro_cos_q30(r), ce, 30), 0);
    return 1;
}

void astro_houses(int32_t ut_days, uint32_t ut_sec, int32_t lat, int32_t lon_east, unsigned system, astro_houses_t *h) {
    astro_time_t u;
    astro_bam_t th = gast(ut_days, ut_sec, &u) + (uint32_t)lon_east, eps = astro_obliquity(u), c11, c12, c2, c3, d;
    int32_t se = astro_sin_q30(eps), ce = astro_cos_q30(eps), sf, cf, i;
    const int32_t lim = 0x3FA4FA4F;             /* 89.5 deg: the formulas need a horizon that is not the equator */
    lat = lat > lim ? lim : lat < -lim ? -lim : lat;
    sf = astro_sin_q30((uint32_t)lat); cf = astro_cos_q30((uint32_t)lat);
    h->mc = astro_atan2(astro_sin_q30(th), astro_mulq(astro_cos_q30(th), ce, 30), 0);
    h->asc = asc_of(th, sf, cf, se, ce);
    if (h->asc - h->mc >= 0x80000000u) h->asc += 0x80000000u;      /* inside the polar circles: keep the ascendant east of the MC */
    h->system = system < ASTRO_NHSYS ? system : ASTRO_H_PLACIDUS;
    c11 = c12 = c2 = c3 = 0;
    if (h->system == ASTRO_H_PLACIDUS) {
        int32_t tp = astro_divq(sf, cf, 24);
        if (!(placidus(th, 1, 0, tp, se, ce, &c11) && placidus(th, 2, 0, tp, se, ce, &c12)
              && placidus(th, 2, 1, tp, se, ce, &c2) && placidus(th, 1, 1, tp, se, ce, &c3))) h->system = ASTRO_H_PORPHYRY;
    } else if (h->system == ASTRO_H_KOCH) {
        /* the MC's degree: its declination, its ascensional difference; a third of it shifts each cusp's time */
        int32_t sd = astro_mulq(astro_sin_q30(h->mc), se, 30), cd = (int32_t)astro_isqrt((uint32_t)((1 << 30) - astro_mulq(sd, sd, 30))) << 15;
        int32_t x = astro_mulq(astro_divq(sf, cf, 24), astro_divq(sd, cd ? cd : 1, 30), 28), a3;
        if (x >= (1 << 26) || x <= -(1 << 26)) h->system = ASTRO_H_PORPHYRY;
        else {
            a3 = (int32_t)astro_asin(x * 16) / 3;
            c11 = asc_of(th - 0x2AAAAAABu - (uint32_t)(2 * a3), sf, cf, se, ce);
            c12 = asc_of(th - 0x15555555u - (uint32_t)a3, sf, cf, se, ce);
            c2 = asc_of(th + 0x15555555u + (uint32_t)a3, sf, cf, se, ce);
            c3 = asc_of(th + 0x2AAAAAABu + (uint32_t)(2 * a3), sf, cf, se, ce);
        }
    } else if (h->system == ASTRO_H_REGIO) {    /* poles with tan = tan(lat) * sin 30, sin 60 */
        int32_t s1 = sf >> 1, s2 = astro_mulq(sf, 929887697, 30);
        c11 = asc_of(th - 0x2AAAAAABu, s1, cf, se, ce); c12 = asc_of(th - 0x15555555u, s2, cf, se, ce);
        c2 = asc_of(th + 0x15555555u, s2, cf, se, ce); c3 = asc_of(th + 0x2AAAAAABu, s1, cf, se, ce);
    } else if (h->system == ASTRO_H_CAMPANUS) {  /* the prime vertical in twelfths, seen from the horizon's poles */
        int32_t s1 = sf >> 1, s2 = astro_mulq(sf, 929887697, 30);
        int32_t k1 = (int32_t)astro_isqrt((uint32_t)((1 << 30) - astro_mulq(s1, s1, 30))) << 15;
        int32_t k2 = (int32_t)astro_isqrt((uint32_t)((1 << 30) - astro_mulq(s2, s2, 30))) << 15;
        astro_bam_t x1 = astro_atan2(929887697, cf >> 1, 0), x2 = astro_atan2(309962566, cf >> 1, 0);   /* sqrt 3, 1/sqrt 3 (halved) */
        c11 = asc_of(th - x1, s1, k1, se, ce); c12 = asc_of(th - x2, s2, k2, se, ce);
        c2 = asc_of(th + x2, s2, k2, se, ce); c3 = asc_of(th + x1, s1, k1, se, ce);
    }
    if (h->system == ASTRO_H_PORPHYRY) {        /* each quadrant in three */
        d = (h->asc - h->mc) / 3u;
        c11 = h->mc + d; c12 = h->mc + 2u * d;
        d = (h->mc + 0x80000000u - h->asc) / 3u;
        c2 = h->asc + d; c3 = h->asc + 2u * d;
    }
    if (h->system >= ASTRO_H_EQUAL) {
        astro_bam_t a = h->asc;
        if (h->system == ASTRO_H_WHOLE) { uint32_t sg = astro_sign(a); a = sg * 0x15555555u + sg / 3u; }
        for (i = 0; i < 12; i++) h->cusp[i] = a + (uint32_t)i * 0x15555555u + (uint32_t)i / 3u;
        return;
    }
    h->cusp[0] = h->asc; h->cusp[9] = h->mc; h->cusp[10] = c11; h->cusp[11] = c12; h->cusp[1] = c2; h->cusp[2] = c3;
    for (i = 0; i < 3; i++) { h->cusp[3 + i] = h->cusp[9 + i] + 0x80000000u; h->cusp[6 + i] = h->cusp[i] + 0x80000000u; }
}

unsigned astro_house_of(const astro_houses_t *h, astro_bam_t lon) {
    unsigned i;
    for (i = 0; i < 12u; i++)
        if (lon - h->cusp[i] < h->cusp[(i + 1u) % 12u] - h->cusp[i]) return i;
    return 0;
}

/* ---- aspects */
const astro_bam_t astro_aspect_angle[ASTRO_NASPECT] = {
    0u, 0x2AAAAAABu, 0x40000000u, 0x55555555u, 0x80000000u,      /* 0 60 90 120 180 */
    0x15555555u, 0x6AAAAAABu, 0x20000000u, 0x60000000u, 0x33333333u, 0x66666666u};   /* 30 150 45 135 72 144 */
int astro_aspect(astro_bam_t a, astro_bam_t b, int32_t va, int32_t vb, uint32_t orb, unsigned nkinds, astro_aspect_t *out) {
    int32_t sep = (int32_t)(a - b), rel = sep < 0 ? vb - va : va - vb;   /* rel: how the separation's size changes */
    uint32_t as = sep < 0 ? 0u - (uint32_t)sep : (uint32_t)sep, best = orb + 1u;
    unsigned k;
    int found = 0;
    for (k = 0; k < nkinds && k < ASTRO_NASPECT; k++) {
        int32_t e = (int32_t)(as - astro_aspect_angle[k]);
        uint32_t ae = e < 0 ? 0u - (uint32_t)e : (uint32_t)e;
        if (ae < best) {
            best = ae; found = 1;
            out->kind = (uint8_t)k; out->err = e;
            out->applying = (uint8_t)((e > 0 && rel < 0) || (e < 0 && rel > 0) || e == 0);
        }
    }
    return found;
}
