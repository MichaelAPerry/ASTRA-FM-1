/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA core astronomy, integer only (see astro.h). Sun: Meeus ch. 25 low precision. Moon: Meeus ch. 47, the 34 largest
 * longitude terms. Nutation: Meeus ch. 22 short form. Signed >> is arithmetic on every compiler we target. */
#include "astro.h"
#include "astro_tables.h"

enum { EL_SUN_L0, EL_SUN_M, EL_OMEGA, EL_MOON_LP, EL_MOON_D, EL_MOON_M, EL_MOON_MP, EL_MOON_F, EL_MOON_A1, EL_MOON_A2 };

int32_t astro_days_from_civil(int32_t y, uint32_t m, uint32_t d) {
    int32_t era, yoe, doy, doe;
    y -= m <= 2u;
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (int32_t)((153u * (m > 2u ? m - 3u : m + 9u) + 2u) / 5u + d) - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

/* Delta T (s) every 10 years 1800-2050 (Espenak & Meeus; 2030-2050 their polynomial); after 2050 their long-term
 * parabola. Before 1800: held (outside the supported range). A prediction after ~2025: unknowable to a few seconds. */
static const int16_t dt_tab[] = {14,12,12,8,6,7,8,2,-5,-6,-3,10,21,24,24,29,33,40,51,57,64,66,69,78,85,93};
int32_t astro_delta_t(int32_t year) {
    int32_t i, r;
    if (year <= 1800) return dt_tab[0];
    if (year >= 2050) { int32_t u = year - 1820; return -20 + u * u * 32 / 10000 - (year < 2150 ? 5628 * (2150 - year) / 10000 : 0); }
    i = (year - 1800) / 10; r = (year - 1800) % 10;
    return dt_tab[i] + (dt_tab[i + 1] - dt_tab[i]) * r / 10;
}

static astro_time_t mk_time(int32_t days, int32_t sec) {
    astro_time_t t;
    while (sec < 0) { sec += 86400; days--; }
    while (sec >= 86400) { sec -= 86400; days++; }
    t.days = days; t.frac16 = (uint32_t)sec * 512u / 675u;        /* 65536 / 86400 */
    return t;
}
astro_time_t astro_time_tt(int32_t y, uint32_t mo, uint32_t d, uint32_t h, uint32_t mi, uint32_t s) {
    return mk_time(astro_days_from_civil(y, mo, d) - 10957, (int32_t)(h * 3600u + mi * 60u + s) - 43200);
}
astro_time_t astro_time_utc(int32_t y, uint32_t mo, uint32_t d, uint32_t h, uint32_t mi, uint32_t s) {
    return mk_time(astro_days_from_civil(y, mo, d) - 10957, (int32_t)(h * 3600u + mi * 60u + s) - 43200 + astro_delta_t(y));
}

int32_t astro_sin_q30(astro_bam_t a) {
    uint32_t q = a >> 30, x = a & 0x3FFFFFFFu, i, f, v;
    if (q & 1u) x = 0x40000000u - x;
    i = x >> 21; f = x & 0x1FFFFFu;
    v = (uint32_t)sin_tab[i];
    if (i < ASTRO_SIN_N) v += ((uint32_t)(sin_tab[i + 1] - sin_tab[i]) * (f >> 11)) >> 10;
    return q & 2u ? -(int32_t)v : (int32_t)v;
}

int32_t astro_mul_q30(int32_t a, int32_t s) {
    int32_t ah = a >> 16, sh = s >> 16;
    uint32_t al = (uint32_t)a & 0xFFFFu, sl = (uint32_t)s & 0xFFFFu;
    return ah * sh * 4 + ((ah * (int32_t)sl) >> 14) + (((int32_t)al * sh) >> 14) + (int32_t)((al * sl) >> 30);
}

static int32_t t_q16(astro_time_t t) { return t.days * 1794 / 1000; }   /* Julian centuries, Q16 (|T| <= 2.2) */

static astro_bam_t lin(const astro_lin_t *e, astro_time_t t) {
    int32_t r = (int32_t)e->ri, tq = t_q16(t), t2;
    uint32_t a = e->a0 + e->ri * (uint32_t)t.days + (uint32_t)((e->rf * t.days) >> 12);
    a += (uint32_t)((r >> 16) * (int32_t)t.frac16 + (int32_t)((((uint32_t)r & 0xFFFFu) * t.frac16) >> 16));
    t2 = ((tq >> 2) * (tq >> 2)) >> 16;                                  /* T^2, Q12 */
    return a + (uint32_t)((e->t2 * t2) >> 10);
}

static int32_t nutation(astro_time_t t) {
    astro_bam_t om = lin(&astro_el[EL_OMEGA], t);
    return astro_mul_q30(NUT_OM, astro_sin_q30(om)) + astro_mul_q30(NUT_2L, astro_sin_q30(2u * lin(&astro_el[EL_SUN_L0], t)))
         + astro_mul_q30(NUT_2LP, astro_sin_q30(2u * lin(&astro_el[EL_MOON_LP], t))) + astro_mul_q30(NUT_2OM, astro_sin_q30(2u * om));
}

astro_bam_t astro_sun_lon(astro_time_t t, int apparent) {
    astro_bam_t l0 = lin(&astro_el[EL_SUN_L0], t), m = lin(&astro_el[EL_SUN_M], t);
    int32_t tq = t_q16(t);
    int32_t c = astro_mul_q30(SUN_C1 + ((SUN_C1T * (tq >> 4)) >> 12), astro_sin_q30(m))
              + astro_mul_q30(SUN_C2 + ((SUN_C2T * (tq >> 4)) >> 12), astro_sin_q30(2u * m))
              + astro_mul_q30(SUN_C3, astro_sin_q30(3u * m));
    astro_bam_t l = l0 + (uint32_t)c;
    if (apparent) l += (uint32_t)(SUN_ABERR + nutation(t));
    return l;
}

astro_bam_t astro_moon_lon(astro_time_t t, int apparent) {
    astro_bam_t lp = lin(&astro_el[EL_MOON_LP], t), d = lin(&astro_el[EL_MOON_D], t), m = lin(&astro_el[EL_MOON_M], t);
    astro_bam_t mp = lin(&astro_el[EL_MOON_MP], t), f = lin(&astro_el[EL_MOON_F], t);
    int32_t tq = t_q16(t), e1 = 0x40000000 - ((84420 * (tq >> 4)) >> 7), e2 = astro_mul_q30(e1, e1), sum = 0;
    uint32_t i;
    for (i = 0; i < sizeof moon_terms / sizeof moon_terms[0]; i++) {
        const astro_term_t *k = &moon_terms[i];
        astro_bam_t arg = (uint32_t)(int32_t)k->d * d + (uint32_t)(int32_t)k->m * m + (uint32_t)(int32_t)k->mp * mp + (uint32_t)(int32_t)k->f * f;
        int32_t amp = k->m == 0 ? k->amp : astro_mul_q30(k->amp, k->m == 1 || k->m == -1 ? e1 : e2);
        sum += astro_mul_q30(amp, astro_sin_q30(arg));
    }
    sum += astro_mul_q30(MOON_A1T, astro_sin_q30(lin(&astro_el[EL_MOON_A1], t)))
         + astro_mul_q30(MOON_LFT, astro_sin_q30(lp - f))
         + astro_mul_q30(MOON_A2T, astro_sin_q30(lin(&astro_el[EL_MOON_A2], t)));
    lp += (uint32_t)sum;
    if (apparent) lp += (uint32_t)nutation(t);
    return lp;
}

uint32_t astro_sign(astro_bam_t a) { return (((a >> 16) * 21600u) >> 16) / 1800u; }
uint32_t astro_sign_arcmin(astro_bam_t a) { return (((a >> 16) * 21600u) >> 16) % 1800u; }
uint32_t astro_mdeg(astro_bam_t a) { return ((a >> 16) * 45000u + (((a & 0xFFFFu) * 45000u) >> 16)) >> 13; }
