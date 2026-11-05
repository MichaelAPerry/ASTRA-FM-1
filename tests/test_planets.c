/* SPDX-License-Identifier: GPL-3.0-only
 * Host test of core/astro/planets.c against JPL Horizons (DE441, tests/planet_ref.h from tools/gen_planet_ref.py):
 * apparent geocentric longitude of date, latitude, and the motion in longitude (retrograde = negative), 120 TT instants
 * 1800-2199 for each planet. Prints the measured errors; fails above the tolerances in docs/ACCURACY.md. */
#include <math.h>
#include <stdio.h>
#include "core/astro/astro.h"
#include "planet_ref.h"

#define BAM_DEG (360.0 / 4294967296.0)
static const char *const NAME[ASTRO_NPLANET] = {"Mercury", "Venus", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"};
static const double TOL_LON = 15.0, TOL_LAT = 15.0, TOL_SPEED = 0.001;   /* arcsec, arcsec, deg/day */

int main(void) {
    unsigned i, n = sizeof planet_ref / sizeof planet_ref[0];
    int b, fails = 0;
    for (b = 0; b < ASTRO_NPLANET; b++) {
        double lmax = 0, lsum = 0, bmax = 0, smax = 0;
        unsigned retro = 0;
        for (i = 0; i < n; i++) {
            astro_time_t t = astro_time_tt(planet_ref[i].y, (uint32_t)planet_ref[i].mo, (uint32_t)planet_ref[i].d,
                                           (uint32_t)planet_ref[i].h, (uint32_t)planet_ref[i].mi, (uint32_t)planet_ref[i].s);
            astro_pos_t p;
            double el, eb, es, sp;
            int wrong;
            astro_planet(b, t, &p);
            sp = astro_planet_speed(b, t) * BAM_DEG;
            el = fmod(p.lon * BAM_DEG - planet_ref[i].v[b][0] + 540.0, 360.0) - 180.0;
            eb = p.lat * BAM_DEG - planet_ref[i].v[b][1];
            es = sp - planet_ref[i].v[b][2];
            el = fabs(el) * 3600; eb = fabs(eb) * 3600; es = fabs(es);
            lsum += el;
            if (el > lmax) lmax = el;
            if (eb > bmax) bmax = eb;
            if (es > smax) smax = es;
            if (planet_ref[i].v[b][2] < 0) retro++;
            wrong = (sp < 0) != (planet_ref[i].v[b][2] < 0) && fabs(planet_ref[i].v[b][2]) > 2 * TOL_SPEED;   /* at a station: noise */
            if (el > TOL_LON || eb > TOL_LAT || es > TOL_SPEED || wrong) {
                fails++;
                printf("FAIL %s %d-%02d-%02d: lon %.1f\" lat %.1f\" speed %.5f deg/d (ref %.5f)\n", NAME[b], planet_ref[i].y,
                       planet_ref[i].mo, planet_ref[i].d, el, eb, sp, planet_ref[i].v[b][2]);
            }
        }
        printf("planets: %-7s lon max %5.1f\" mean %5.1f\"  lat max %5.1f\"  speed max %.5f deg/d  (%u of %u retrograde)\n",
               NAME[b], lmax, lsum / n, bmax, smax, retro, n);
    }
    printf(fails ? "planets: %d FAILED\n" : "planets: all ok\n", fails);
    return fails != 0;
}
