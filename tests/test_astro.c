/* SPDX-License-Identifier: GPL-3.0-only
 * Host tests of core/astro (integer engine) against Meeus's worked examples and Astronomy Engine reference positions.
 * Floating point is allowed here (host only) to measure errors. Tolerances are set from measured errors, see docs/ACCURACY.md. */
#include <stdio.h>
#include <math.h>
#include "core/astro/astro.h"
#include "astro_ref.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static double deg(astro_bam_t a) { return a * (360.0 / 4294967296.0); }
static double diff(double a, double b) { double d = fmod(a - b + 540.0, 360.0) - 180.0; return fabs(d); }

int main(void) {
    unsigned i, n = sizeof astro_ref / sizeof astro_ref[0];
    double e, sun_max = 0, moon_max = 0, sun_sum = 0, moon_sum = 0;
    /* calendar: J2000.0 and known day counts */
    CHECK(astro_days_from_civil(1970, 1, 1) == 0, "epoch");
    CHECK(astro_days_from_civil(2000, 1, 1) == 10957, "2000");
    CHECK(astro_days_from_civil(1800, 3, 1) - astro_days_from_civil(1800, 2, 28) == 1, "1800 not leap");
    CHECK(astro_days_from_civil(2000, 3, 1) - astro_days_from_civil(2000, 2, 28) == 2, "2000 leap");
    { int32_t z, y; uint32_t m, d;              /* the inverse round-trips every day 1700-2300 */
      for (z = astro_days_from_civil(1700, 1, 1); z <= astro_days_from_civil(2300, 12, 31); z++) {
          astro_civil_from_days(z, &y, &m, &d);
          if (astro_days_from_civil(y, m, d) != z || m < 1 || m > 12 || d < 1 || d > 31) { CHECK(0, "civil %d", (int)z); break; }
      }
      astro_civil_from_days(0, &y, &m, &d); CHECK(y == 1970 && m == 1 && d == 1, "civil epoch");
      astro_civil_from_days(astro_days_from_civil(2000, 2, 29), &y, &m, &d); CHECK(y == 2000 && m == 2 && d == 29, "civil leap"); }
    { astro_time_t t = astro_time_tt(2000, 1, 1, 12, 0, 0); CHECK(t.days == 0 && t.frac16 == 0, "J2000.0"); }
    { astro_time_t t = astro_time_tt(2000, 1, 1, 0, 0, 0); CHECK(t.days == -1 && t.frac16 == 32768, "J2000.0 - 12h"); }
    /* sine and wraparound */
    CHECK(astro_sin_q30(0) == 0 && astro_sin_q30(0x40000000u) == 1 << 30 && astro_sin_q30(0xC0000000u) == -(1 << 30), "sin");
    for (i = 0; i < 4096; i++) {
        uint32_t a = i * 1048573u; double s = astro_sin_q30(a) / 1073741824.0;
        e = fabs(s - sin(deg(a) * 3.14159265358979323846 / 180)); CHECK(e < 3e-6, "sin err %g at %u", e, a);
    }
    CHECK(astro_sign(0xFFFFFFFFu) == 11 && astro_sign(0) == 0 && astro_sign(0x80000000u) == 6, "sign");
    CHECK(astro_mdeg(0x80000000u) == 180000, "mdeg %u", astro_mdeg(0x80000000u));
    /* Meeus example 25.a: Sun 1992-10-13 0h TD, apparent 199.90895 (low-precision method) */
    e = diff(deg(astro_sun_lon(astro_time_tt(1992, 10, 13, 0, 0, 0), 1)), 199.90895);
    CHECK(e < 0.002, "Meeus 25.a sun err %.5f", e);
    /* Meeus example 47.a: Moon 1992-04-12 0h TD, geometric 133.162655, apparent 133.167265 (full 60-term series) */
    e = diff(deg(astro_moon_lon(astro_time_tt(1992, 4, 12, 0, 0, 0), 0)), 133.162655);
    CHECK(e < 0.01, "Meeus 47.a moon err %.5f", e);
    e = diff(deg(astro_moon_lon(astro_time_tt(1992, 4, 12, 0, 0, 0), 1)), 133.167265);
    CHECK(e < 0.01, "Meeus 47.a moon apparent err %.5f", e);
    /* Astronomy Engine, 1800-2200 */
    for (i = 0; i < n; i++) {
        astro_time_t t = astro_time_utc(astro_ref[i].y, astro_ref[i].mo, astro_ref[i].d, astro_ref[i].h, astro_ref[i].mi, astro_ref[i].s);
        double es = diff(deg(astro_sun_lon(t, 1)), astro_ref[i].sun), em = diff(deg(astro_moon_lon(t, 1)), astro_ref[i].moon);
        sun_sum += es; moon_sum += em; if (es > sun_max) sun_max = es; if (em > moon_max) moon_max = em;
        CHECK(es < 0.01, "sun %d-%d-%d err %.4f", astro_ref[i].y, astro_ref[i].mo, astro_ref[i].d, es);
        CHECK(em < 0.02, "moon %d-%d-%d err %.4f", astro_ref[i].y, astro_ref[i].mo, astro_ref[i].d, em);
    }
    printf("astro: %u reference dates 1800-2200: Sun max %.4f mean %.4f deg; Moon max %.4f mean %.4f deg\n",
           n, sun_max, sun_sum / n, moon_max, moon_sum / n);
    printf(fails ? "astro: %d FAILED\n" : "astro: all ok\n", fails);
    return fails != 0;
}
