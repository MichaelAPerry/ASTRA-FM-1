/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA planets, integer only (see astro.h). Mercury..Neptune and the Earth: VSOP87D (Bretagnon & Francou), truncated
 * (tools/fetch_ephem.py), heliocentric L, B, R of the mean ecliptic and equinox of date. Pluto: Chebyshev series
 * fitted to JPL DE441 in the same frame. Apparent geocentric place: one light-time pass, the Earth's aberration
 * (Meeus 23.2), the FK5 offset of VSOP87 (Meeus 32.3) and nutation in longitude. No 64-bit arithmetic: products go
 * through astro_mul_q30, angles and lengths through a 32-bit CORDIC. Units: BAM for angles, au in Q23 for lengths. */
#include "astro.h"
#include "planet_tables.h"

static const uint8_t VSOP_OF[ASTRO_NPLANET] = {0, 1, 3, 4, 5, 6, 7, 0};   /* body -> VSOP table (Pluto: unused) */
#define COS(a) astro_sin_q30((uint32_t)(a) + 0x40000000u)

/* a VSOP argument at days + frac/65536 from J2000.0 (the rate's 12-bit fraction times frac is under 1 BAM: left out) */
static uint32_t arg(const astro_lin_t *e, int32_t days, uint32_t frac)
{
    uint32_t r = e->ri;
    return e->a0 + r * (uint32_t)days + (uint32_t)((e->rf * days) >> 12) + (r >> 16) * frac + (((r & 0xFFFFu) * frac) >> 16);
}
/* time in Julian millennia, Q30: 2^30 / 365250 = 2940 - 623/2048 (to 1e-7) */
static int32_t tau_q30(astro_time_t t)
{
    return t.days * 2940 - ((t.days * 623) >> 11) + (int32_t)((t.frac16 * 2940u) >> 16);
}
/* sum over powers 0..5 of tau^k * sum(A cos(arg)), by Horner */
static int32_t series(uint32_t b, uint32_t v, astro_time_t t, int32_t tau)
{
    const uint16_t *ix = vsop_idx[b][v];
    int32_t acc = 0, k;
    for (k = 5; k >= 0; k--) {
        int32_t s = 0;
        uint32_t i;
        for (i = ix[k]; i < ix[k + 1]; i++)
            s += astro_mul_q30(vsop_terms[i].amp, COS(arg(&vsop_terms[i].arg, t.days, t.frac16)));
        acc = astro_mul_q30(acc, tau) + s;
    }
    return acc;
}
static void vsop_lbr(uint32_t b, astro_time_t t, astro_bam_t *l, int32_t *lat, int32_t *r)
{
    int32_t tau = tau_q30(t);
    *l = arg(&vsop_main[b], t.days, t.frac16) + (uint32_t)series(b, 0, t, tau);
    *lat = series(b, 1, t, tau);
    *r = series(b, 2, t, tau);
}
static void to_xyz(astro_bam_t l, int32_t lat, int32_t r, int32_t v[3])
{
    int32_t rc = astro_mul_q30(r, COS(lat));
    v[0] = astro_mul_q30(rc, COS(l));
    v[1] = astro_mul_q30(rc, astro_sin_q30(l));
    v[2] = astro_mul_q30(r, astro_sin_q30((uint32_t)lat));
}

/* Pluto: segment k covers PLUTO_SEG_DAYS whole days from J0 (0h TT); x in -1..1 (Q30); Clenshaw's recurrence */
static void pluto_xyz(astro_time_t t, int32_t v[3])
{
    int32_t d = t.days - PLUTO_D0, f = (int32_t)t.frac16 - 32768, k, c, n, q, rm, x;
    if (f < 0) f += 65536, d--;
    if (d < 0) d = 0, f = 0;                                  /* outside 1800-2201: held at the ends */
    if (d >= PLUTO_NSEG * PLUTO_SEG_DAYS) d = PLUTO_NSEG * PLUTO_SEG_DAYS, f = 0;
    k = d / PLUTO_SEG_DAYS;
    if (k >= PLUTO_NSEG) k = PLUTO_NSEG - 1;
    n = 2 * ((d - k * PLUTO_SEG_DAYS) * 65536 + f) - PLUTO_SEG_DAYS * 65536;   /* (2u - L) in 1/65536 day */
    q = n / PLUTO_SEG_DAYS; rm = n % PLUTO_SEG_DAYS;
    x = q * 16384 + rm * 16384 / PLUTO_SEG_DAYS;
    for (c = 0; c < 3; c++) {
        const int32_t *cf = pluto_cheb[k][c];
        int32_t b1 = 0, b2 = 0, b0, j;
        for (j = PLUTO_DEG; j >= 1; j--) {
            b0 = 2 * astro_mul_q30(b1, x) - b2 + cf[j];
            b2 = b1; b1 = b0;
        }
        v[c] = astro_mul_q30(b1, x) - b2 + cf[0];
    }
}

/* CORDIC vectoring: the angle of (x, y) in BAM, and its length in the units of x and y */
static astro_bam_t cordic(int32_t x, int32_t y, int32_t *len)
{
    astro_bam_t a = 0;
    int32_t m, sh = 0, i;
    if (!x && !y) { *len = 0; return 0; }
    if (x < 0) { x = -x; y = -y; a = 0x80000000u; }          /* to the right half: within CORDIC's +-99 deg */
    m = x > (y < 0 ? -y : y) ? x : (y < 0 ? -y : y);
    while (m >= 1 << 29) { x >>= 1; y >>= 1; m >>= 1; sh--; }   /* scale to 2^28..2^29: full precision, no overflow */
    while (m < 1 << 28) { x <<= 1; y <<= 1; m <<= 1; sh++; }
    for (i = 0; i < 30; i++) {
        int32_t xn;
        if (y > 0) { xn = x + (y >> i); y -= x >> i; a += cordic_atan[i]; }
        else       { xn = x - (y >> i); y += x >> i; a -= cordic_atan[i]; }
        x = xn;
    }
    m = astro_mul_q30(x, CORDIC_INVK);
    *len = sh > 0 ? m >> sh : m << -sh;
    return a;
}

static void helio_xyz(int body, astro_time_t t, int32_t v[3])
{
    astro_bam_t l;
    int32_t b, r;
    if (body == ASTRO_PLUTO) { pluto_xyz(t, v); return; }
    vsop_lbr(VSOP_OF[body], t, &l, &b, &r);
    to_xyz(l, b, r, v);
}
static astro_time_t minus_q16(astro_time_t t, uint32_t q16)   /* t - q16/65536 days, q16 < 65536 */
{
    if (t.frac16 >= q16) t.frac16 -= q16;
    else t.frac16 += 65536u - q16, t.days--;
    return t;
}

void astro_planet(int body, astro_time_t t, astro_pos_t *p)
{
    astro_bam_t le, lon;
    int32_t be, re, e[3], v[3], rho, dist, lat, dl, cb;
    if (body < 0 || body >= ASTRO_NPLANET) body = 0;
    vsop_lbr(VSOP_EARTH, t, &le, &be, &re);
    to_xyz(le, be, re, e);
    helio_xyz(body, t, v);                                    /* geometric: only for the light time */
    lon = cordic(v[0] - e[0], v[1] - e[1], &rho);
    (void)cordic(rho, v[2] - e[2], &dist);
    helio_xyz(body, minus_q16(t, (uint32_t)astro_mul_q30(dist, LIGHT_Q30)), v);   /* where the light left it */
    lon = cordic(v[0] - e[0], v[1] - e[1], &rho);
    lat = (int32_t)cordic(rho, v[2] - e[2], &dist);
    /* aberration (Meeus 23.2): (-k cos(sun - lon) + e k cos(peri - lon)) / cos(lat); the Sun at the Earth's L + 180 */
    dl = astro_mul_q30(-ABERR_K, COS(le + 0x80000000u - lon)) + astro_mul_q30(ABERR_EK, COS(ABERR_PERI - lon));
    cb = COS(lat) >> 16;
    p->lon = lon + (uint32_t)((dl * 16384) / (cb ? cb : 1) + FK5_DL + astro_nutation(t));
    /* aberration in latitude: -k sin(lat) (sin(sun - lon) - e sin(peri - lon)) */
    p->lat = lat + astro_mul_q30(astro_mul_q30(-ABERR_K, astro_sin_q30((uint32_t)lat)),
                                 astro_sin_q30(le + 0x80000000u - lon) - astro_mul_q30(ABERR_E, astro_sin_q30(ABERR_PERI - lon)));
    p->dist = dist;
}

int32_t astro_planet_speed(int body, astro_time_t t)
{
    astro_pos_t a, b;
    astro_time_t t1 = t, t0 = minus_q16(t, 32768u);
    if (t1.frac16 < 32768u) t1.frac16 += 32768u; else t1.frac16 -= 32768u, t1.days++;
    astro_planet(body, t1, &b);
    astro_planet(body, t0, &a);
    return (int32_t)(b.lon - a.lon);
}
