/* SPDX-License-Identifier: GPL-3.0-only
 * Host tests of core/astro/sky.c: planets against Astronomy Engine, angles and houses against their definitions
 * (tools/gen_reference.py), aspects, the integer helpers. Tolerances are set from measured errors: docs/ACCURACY.md. */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "core/astro/astro.h"
#include "sky_ref.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static double deg(astro_bam_t a) { return a * (360.0 / 4294967296.0); }
static double sdeg(int32_t a) { return a * (360.0 / 4294967296.0); }
static double diff(double a, double b) { return fabs(fmod(a - b + 540.0, 360.0) - 180.0); }
static int32_t bam(double d) { return (int32_t)(long long)llround(fmod(d + 540.0, 360.0) * (4294967296.0 / 360.0) - 2147483648.0); }
static const char *const NAME[10] = {"Sun", "Moon", "Mercury", "Venus", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"};
static const double TOL[10] = {0.003, 0.02, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012, 0.012};

int main(void) {
    unsigned i, b, n = sizeof sky_ref / sizeof sky_ref[0], nh = sizeof house_ref / sizeof house_ref[0];
    double mx[10] = {0}, sm[10] = {0}, smx[10] = {0}, hmx[5] = {0};
    static const char *const HN[5] = {"ASC/MC", "Placidus", "Koch", "Regiomontanus", "Campanus"};
    /* helpers */
    for (i = 0; i < 2000; i++) {
        int32_t a = (int32_t)(rand() * 2654435761u), c = (int32_t)(rand() * 40503u), r;
        unsigned k = 1u + (unsigned)rand() % 31u;
        long long w = ((long long)a * c) / (1ll << k);            /* astro_mulq rounds toward zero */
        if (w > 2147483647ll || w < -2147483647ll) continue;
        CHECK(astro_mulq(a, c, k) == (int32_t)w, "mulq %d %d %u", (int)a, (int)c, k);
        if (c) { w = ((long long)(a >> 12) << 10) / c; if (w < 2147483647ll && w > -2147483647ll) CHECK(astro_divq(a >> 12, c, 10) == (int32_t)w, "divq"); }
        r = 0;
        { double ang = atan2((double)c, (double)a) * 180 / 3.14159265358979323846, e = diff(deg(astro_atan2(c, a, &r)), ang);
          if (abs(a) > 1000 || abs(c) > 1000) CHECK(e < 2e-4, "atan2 %d %d err %g", (int)c, (int)a, e);
          if (hypot(a, c) < 2e9) CHECK(fabs(r - hypot(a, c)) < 8 + hypot(a, c) * 1e-7, "hypot %d %d: %d", (int)a, (int)c, (int)r); }
        CHECK(astro_isqrt((uint32_t)a) == (uint32_t)sqrt((double)(uint32_t)a), "isqrt");
        { double s = ((int)i - 1000) / 1000.0; CHECK(diff(deg(astro_asin((int32_t)(s * 1073741824.0))), asin(s) * 180 / 3.14159265358979323846) < 2e-4, "asin %g", s); }
    }
    /* planets, 1800-2200 */
    for (i = 0; i < n; i++) {
        astro_sky_t s;
        astro_sky(astro_time_utc(sky_ref[i].y, sky_ref[i].mo, sky_ref[i].d, sky_ref[i].h, sky_ref[i].mi, sky_ref[i].s), &s);
        for (b = 0; b < 10; b++) {
            double e = diff(deg(s.lon[b]), sky_ref[i].lon[b]), es = fabs(sdeg(s.spd[b]) - sky_ref[i].spd[b]);
            sm[b] += e; if (e > mx[b]) mx[b] = e; if (es > smx[b]) smx[b] = es;
            CHECK(e < TOL[b], "%s %d-%d-%d err %.5f deg", NAME[b], sky_ref[i].y, sky_ref[i].mo, sky_ref[i].d, e);
            CHECK(es < (b == 1 ? 0.25 : 0.02), "%s %d-%d-%d motion err %.5f deg/day", NAME[b], sky_ref[i].y, sky_ref[i].mo, sky_ref[i].d, es);
            if (fabs(sky_ref[i].spd[b]) > 0.01) CHECK((s.spd[b] < 0) == (sky_ref[i].spd[b] < 0), "%s retrograde flag", NAME[b]);
        }
    }
    for (b = 0; b < 10; b++)
        printf("sky: %-8s max %.4f deg (%4.1f\"), mean %.4f deg; motion max err %.4f deg/day\n", NAME[b], mx[b], mx[b] * 3600, sm[b] / n, smx[b]);
    /* angles and houses */
    for (i = 0; i < nh; i++) {
        astro_houses_t h;
        const double *a = house_ref[i].a;
        int32_t dz = astro_days_from_civil(house_ref[i].y, (unsigned)house_ref[i].mo, (unsigned)house_ref[i].d), sys;
        uint32_t sec = (uint32_t)(house_ref[i].h * 3600 + house_ref[i].mi * 60 + house_ref[i].s);
        for (sys = 0; sys < 4; sys++) {
            static const unsigned C[4] = {10, 11, 1, 2};
            double e;
            astro_houses(dz, sec, bam(house_ref[i].lat), bam(house_ref[i].lon), (unsigned)sys, &h);
            CHECK(h.system == (unsigned)sys, "house system %d fell back at lat %.1f", (int)sys, house_ref[i].lat);
            if (!sys) {
                e = diff(deg(h.asc), a[0]); if (e > hmx[0]) hmx[0] = e; CHECK(e < 0.01, "ASC row %u err %.5f", i, e);
                e = diff(deg(h.mc), a[1]); if (e > hmx[0]) hmx[0] = e; CHECK(e < 0.01, "MC row %u err %.5f", i, e);
            }
            for (b = 0; b < 4; b++) {
                e = diff(deg(h.cusp[C[b]]), a[2 + 4 * sys + b]); if (e > hmx[1 + sys]) hmx[1 + sys] = e;
                CHECK(e < 0.012, "%s cusp %u row %u (lat %.1f) err %.5f: %.4f want %.4f", HN[1 + sys], C[b] + 1, i, house_ref[i].lat, e, deg(h.cusp[C[b]]), a[2 + 4 * sys + b]);
            }
            for (b = 0; b < 6; b++) CHECK(h.cusp[b] + 0x80000000u == h.cusp[b + 6], "opposite cusps");
            for (b = 0; b < 12; b++) CHECK(astro_house_of(&h, h.cusp[b] + 1000u) == b, "house_of %u", b);
        }
        astro_houses(dz, sec, bam(house_ref[i].lat), bam(house_ref[i].lon), ASTRO_H_PORPHYRY, &h);
        CHECK(diff(deg(h.cusp[10]), deg(h.mc) + diff(deg(h.asc), deg(h.mc)) / 3) < 1e-4, "Porphyry");
        astro_houses(dz, sec, bam(house_ref[i].lat), bam(house_ref[i].lon), ASTRO_H_EQUAL, &h);
        CHECK(h.cusp[0] == h.asc && diff(deg(h.cusp[3]), deg(h.asc) + 90) < 1e-5, "Equal");
        astro_houses(dz, sec, bam(house_ref[i].lat), bam(house_ref[i].lon), ASTRO_H_WHOLE, &h);
        CHECK(diff(deg(h.cusp[0]), floor(deg(h.asc) / 30) * 30) < 1e-5 && astro_house_of(&h, h.asc) == 0, "Whole sign");
    }
    for (b = 0; b < 5; b++) printf("houses: %-14s max err %.4f deg (%4.1f\")\n", HN[b], hmx[b], hmx[b] * 3600);
    { astro_houses_t h; astro_houses(10957, 43200, bam(78.0), bam(15.0), ASTRO_H_PLACIDUS, &h);
      CHECK(h.system == ASTRO_H_PORPHYRY, "Placidus at 78 N must fall back to Porphyry"); }
    CHECK(diff(deg(astro_ayanamsa(astro_time_tt(2000, 1, 1, 12, 0, 0))), 23.85709) < 1e-4, "ayanamsa J2000");
    CHECK(diff(deg(astro_ayanamsa(astro_time_tt(1950, 1, 1, 12, 0, 0))), 23.85709 - 50.0 * 50.2880 / 3600) < 2e-3, "ayanamsa 1950");
    /* aspects */
    { astro_aspect_t a;
      CHECK(astro_aspect(ASTRO_DEG(100), ASTRO_DEG(10.5), 0, 0, ASTRO_DEG(2), ASTRO_NMAJOR, &a) && a.kind == ASTRO_SQUARE, "square");
      CHECK(fabs(sdeg(a.err) + 0.5) < 1e-4, "square err %.4f", sdeg(a.err));
      CHECK(astro_aspect(ASTRO_DEG(359), ASTRO_DEG(1), ASTRO_DEG(1), 0, ASTRO_DEG(3), ASTRO_NMAJOR, &a) && a.kind == ASTRO_CONJ && a.applying, "conjunction across 0, applying");
      CHECK(astro_aspect(ASTRO_DEG(1), ASTRO_DEG(359), ASTRO_DEG(1), 0, ASTRO_DEG(3), ASTRO_NMAJOR, &a) && !a.applying, "separating");
      CHECK(astro_aspect(ASTRO_DEG(10), ASTRO_DEG(188), 0, ASTRO_DEG(1), ASTRO_DEG(3), ASTRO_NMAJOR, &a) && a.kind == ASTRO_OPPOSITION && a.applying, "opposition applying");
      CHECK(!astro_aspect(ASTRO_DEG(10), ASTRO_DEG(50), 0, 0, ASTRO_DEG(3), ASTRO_NMAJOR, &a), "no aspect");
      CHECK(astro_aspect(ASTRO_DEG(10), ASTRO_DEG(160.5), 0, 0, ASTRO_DEG(2), ASTRO_NASPECT, &a) && a.kind == ASTRO_QUINCUNX, "quincunx");
      CHECK(astro_aspect(ASTRO_DEG(10), ASTRO_DEG(82), 0, 0, ASTRO_DEG(1), ASTRO_NASPECT, &a) && a.kind == ASTRO_QUINTILE, "quintile"); }
    printf(fails ? "sky: %d FAILED\n" : "sky: all ok\n", fails);
    return fails != 0;
}
