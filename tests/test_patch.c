/* SPDX-License-Identifier: GPL-3.0-only
 * Host tests of core/chart and core/synth: a birth chart becomes a patch, the patch and the sky become a program, the
 * program plays. Checks the rules (not the taste): the Sun is the fundamental, wires exist only where aspects do,
 * a return is feedback, every algorithm of every tested sky stays in range and makes sound. `--wav FILE` keeps it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "core/synth/patch.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static FILE *wav;
static double peak, sumsq; static long frames, hot;
static void render(int blocks) {
    static int32_t buf[512]; int b, i;
    for (b = 0; b < blocks; b++) {
        memset(buf, 0, sizeof buf); fm_render(buf, 256);
        for (i = 0; i < 512; i++) { double v = fabs((double)buf[i]); if (v > peak) peak = v; sumsq += v * v; frames++; if (v > 30000) hot++;
            if (wav) { int16_t s = (int16_t)buf[i]; fwrite(&s, 2, 1, wav); } }
    }
}
static void reset(void) { peak = sumsq = 0; frames = hot = 0; }
static void play(const patch_t *p, const chart_t *natal, const chart_t *sky, patch_route_t *r) {
    fm_prog_t *g = fm_prog_begin();
    if (!g) { render(1); g = fm_prog_begin(); }
    patch_derive(p, natal, sky, g, r); fm_prog_commit();
}

int main(int argc, char **argv) {
    static chart_t natal, sky, sky2;
    static patch_route_t route;
    static chart_asp_t asp[CHART_MAX_ASP];
    chart_birth_t b = {1987, 3, 14, 6, 25, 1, 1, 4, 515, -1};      /* 1987-03-14 06:25 UTC+1, 51.5 N 0.1 W */
    patch_t p;
    char s[40];
    static const unsigned WV[4] = {1, 2, 0, 3};
    unsigned i, k, n, algo;
    if (argc == 3 && !strcmp(argv[1], "--wav")) {
        static const unsigned char hdr[44] = {'R','I','F','F',0xff,0xff,0xff,0x7f,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,2,0,
            0x44,0xac,0,0,0x10,0xb1,2,0,4,0,16,0,'d','a','t','a',0xff,0xff,0xff,0x7f};
        wav = fopen(argv[2], "wb"); if (wav) fwrite(hdr, 1, 44, wav);
    }
    fm_init();
    CHECK(chart_natal(&natal, &b, ASTRO_H_PLACIDUS, 0), "natal");
    CHECK(natal.days == astro_days_from_civil(1987, 3, 14) && natal.sec == 5u * 3600u + 25u * 60u, "birth UT: %d %u", (int)natal.days, (unsigned)natal.sec);
    CHECK(astro_sign(natal.lon[ASTRO_SUN]) == 11, "Sun in Pisces on 14 March");
    CHECK(!natal.solar && natal.house[ASTRO_SUN] < 12, "houses");
    for (i = 0; i < ASTRO_NBODY; i++) CHECK(astro_house_of(&natal.h, natal.lon[i]) == natal.house[i], "house of %u", i);
    chart_cast(&sky, astro_days_from_civil(2026, 11, 12), 43200, 0, 0, 0, ASTRO_H_PLACIDUS, 0);
    CHECK(sky.solar && sky.house[ASTRO_SUN] == 0, "solar houses put the Sun in the first");
    { chart_t sid; chart_cast(&sid, sky.days, sky.sec, 0, 0, 0, 0, 1);
      double d = (double)(int32_t)(sky.lon[ASTRO_SUN] - sid.lon[ASTRO_SUN]) * 360 / 4294967296.0; CHECK(d > 24.1 && d < 24.4, "sidereal offset in 2026: %.3f", d); }
    /* aspects: sorted, inside the orb, symmetric facts */
    n = chart_aspects(&natal, &natal, 1, ASTRO_DEG(8), ASTRO_NMAJOR, asp, CHART_MAX_ASP);
    CHECK(n > 3 && n < 30, "natal aspects: %u", n);
    for (i = 0; i < n; i++) {
        double sep = fabs((double)(int32_t)(natal.sky.lon[asp[i].a] - natal.sky.lon[asp[i].b])) * 360 / 4294967296.0, ex = (double)astro_aspect_angle[asp[i].kind] * 360 / 4294967296.0;
        CHECK(fabs(sep - ex) <= 8.0001 && asp[i].a < asp[i].b, "aspect %u: sep %.2f kind %u", i, sep, asp[i].kind);
        if (i) CHECK(asp[i - 1].str >= asp[i].str, "aspects not sorted");
    }
    CHECK(chart_aspects(&natal, &natal, 1, ASTRO_DEG(8), ASTRO_NMAJOR, asp, 3) == 3, "max respected");
    /* a chart against itself: every body conjunct its own place, exact */
    n = chart_aspects(&natal, &natal + 0, 1, ASTRO_DEG(1), ASTRO_NMAJOR, asp, CHART_MAX_ASP); (void)n;
    sky2 = natal; n = chart_aspects(&sky2, &natal, 1, ASTRO_DEG(1), 1, asp, CHART_MAX_ASP);
    for (i = k = 0; i < n; i++) if (asp[i].a == asp[i].b && asp[i].str == 32767) k++;
    CHECK(k == 10, "returns: %u of 10", k);
    /* the cast */
    patch_cast(&p, &natal, 0);
    CHECK(p.op[ASTRO_SUN].coarse == 1 && p.algo == 1, "the Sun is the fundamental (coarse %u, algo %u)", p.op[ASTRO_SUN].coarse, p.algo);
    for (i = 0; i < FM_NOP; i++) {
        unsigned sg = astro_sign(natal.lon[i]);
        CHECK(p.op[i].coarse == (i ? (sg + 12 - 11) % 12 + 1 : 1), "ratio of %u: %u", i, p.op[i].coarse);
        CHECK(p.op[i].wave == WV[sg & 3], "wave of %u", i);
    }
    CHECK(patch_ratio_q16(&p.op[0]) > 0xFE00 && patch_ratio_q16(&p.op[0]) < 0x10200, "Sun ratio %x", (unsigned)patch_ratio_q16(&p.op[0]));
    { patch_op_t o = {3, 64, 0, 0,0,0,0,0,0,0,0,0,0,{0,0},{0,0},0,0,0}; CHECK(patch_ratio_q16(&o) == 0x48000, "3 x 1.5 = %x", (unsigned)patch_ratio_q16(&o));
      o.fine = 0; o.detune = 24; CHECK(fabs(patch_ratio_q16(&o) / 65536.0 - 3 * pow(2, 12 / 1200.0)) < 2e-4, "detune 12 cents: %f", patch_ratio_q16(&o) / 65536.0); }
    /* the wiring follows the aspects: an edge only where there is one, the faster body the source */
    play(&p, &natal, &sky, &route);
    CHECK(route.nedge > 2, "edges: %u", route.nedge);
    for (i = 0; i < route.nedge; i++) {
        unsigned ok = 0;
        for (k = 0; k < route.nasp; k++) if ((route.asp[k].a == route.edge[i].src && route.asp[k].b == route.edge[i].dst) || (route.asp[k].b == route.edge[i].src && route.asp[k].a == route.edge[i].dst)) ok = 1;
        CHECK(ok, "edge %u -> %u without an aspect", route.edge[i].src, route.edge[i].dst);
        if (route.edge[i].kind != ASTRO_OPPOSITION) CHECK(patch_rank[route.edge[i].src] < patch_rank[route.edge[i].dst], "edge %u -> %u against the order", route.edge[i].src, route.edge[i].dst);
    }
    /* a transit chart of the birth moment itself: every planet on its own place is feedback */
    p.algo = 25; play(&p, &natal, &sky2, &route);
    for (i = 0; i < FM_NOP; i++) CHECK(route.fb[i] > 100 && route.fb[i] > p.op[i].feedback, "return feedback of %u: %u", i, route.fb[i]);
    /* every algorithm, three skies, three characters: bounded, sounding, not pinned to the ceiling */
    for (k = 0; k < 3; k++) {
        chart_cast(&sky, astro_days_from_civil(1999 + 13 * (int)k, 1 + 4 * k, 5 + 9 * k), 3600u * (5 + 7 * k), 0, 0, 0, 0, 0);
        for (algo = 1; algo < PATCH_NALGO; algo++) {
            patch_cast(&p, &natal, algo + k); p.algo = (uint8_t)algo;
            play(&p, k == 2 ? 0 : &natal, &sky, &route);
            reset();
            fm_note_on(48, 100); fm_note_on(55, 100); fm_note_on(60, 100); fm_note_on(64, 110); fm_note_on(67, 90);
            render(60);
            CHECK(peak <= 32767 && sqrt(sumsq / frames) > 300, "algo %u sky %u: peak %g rms %.0f", algo, k, peak, sqrt(sumsq / frames));
            CHECK(hot * 50 < frames, "algo %u sky %u: %ld of %ld samples at the ceiling", algo, k, hot, frames);
            if (algo % 12 == 1) printf("patch: algo %2u sky %u: %2u edges, 5 notes: peak %5.0f rms %5.0f\n", algo, k, route.nedge, peak, sqrt(sumsq / frames));
            fm_all_off(); render(40);
        }
    }
    /* the orbit: with the sky a half year on, the Sun's operator is quieter than on its birthday */
    patch_cast(&p, &natal, 0); p.orbit = 127;
    chart_cast(&sky, astro_days_from_civil(2027, 3, 14), 43200, 0, 0, 0, 0, 0); play(&p, &natal, &sky, &route); i = route.lvl[0];
    chart_cast(&sky, astro_days_from_civil(2026, 9, 14), 43200, 0, 0, 0, 0, 0); play(&p, &natal, &sky, &route);
    CHECK(i > 3u * route.lvl[0] && i > 50, "orbit: birthday %u, half a year on %u", i, route.lvl[0]);
    /* the reading: facts, rule, place, time */
    chart_cast(&sky, astro_days_from_civil(2026, 11, 12), 43200, 0, 0, 0, 0, 0);
    n = chart_aspects(&sky, &natal, 1, ASTRO_DEG(6), ASTRO_NMAJOR, asp, CHART_MAX_ASP);
    CHECK(n > 0, "transits");
    for (i = 0; i < n && i < 4; i++) { printf("reading:"); for (k = 0; k < 4; k++) { chart_read_transit(&sky, &natal, &asp[i], k, s, sizeof s); CHECK(strlen(s) < sizeof s && (k == 3 || s[0]), "line"); printf(" [%s]", s); } printf("\n"); }
    for (k = 0; k < 4; k++) { chart_read_place(&natal, ASTRO_MOON, k, s, sizeof s); printf("%s[%s]", k ? " " : "reading: ", s); } printf("\n");
    chart_read_place(&natal, ASTRO_SUN, 0, s, sizeof s); CHECK(!strcmp(s, "SUN IN PISCES"), "%s", s);
    /* 777's trumps: every line of every placement fits the screen; Saturn is XXI, Pisces XVIII; no trump for Pluto */
    for (i = 0; i < ASTRO_NBODY; i++) for (k = 0; k < 7; k++) { chart_read_place(&natal, i, k, s, sizeof s); CHECK(strlen(s) <= 38, "place %u line %u: %zu chars", i, k, strlen(s)); }
    chart_read_place(&natal, ASTRO_SUN, 4, s, sizeof s); CHECK(!strcmp(s, "777: XIX IN XVIII"), "%s", s);
    chart_read_place(&natal, ASTRO_SATURN, 5, s, sizeof s); CHECK(!strcmp(s, "THE GREAT ONE OF THE NIGHT OF TIME"), "%s", s);
    chart_read_place(&natal, ASTRO_PLUTO, 5, s, sizeof s); CHECK(!s[0], "Pluto has no trump in 777: %s", s);
    for (k = 4; k < 7; k++) { chart_read_place(&natal, ASTRO_MOON, k, s, sizeof s); printf("%s[%s]", k == 4 ? "777: " : " ", s); } printf("\n");
    chart_read_place(&natal, ASTRO_SUN, 0, s, 8); CHECK(strlen(s) == 7, "truncation: %s", s);
    CHECK(chart_dignity(ASTRO_SUN, 4) == 2 && chart_dignity(ASTRO_SUN, 10) == -2 && chart_dignity(ASTRO_SUN, 0) == 1 && chart_dignity(ASTRO_SUN, 6) == -1 && chart_dignity(ASTRO_MARS, 7) == 2, "dignities");
    CHECK(chart_moon_phase(&natal) < 256, "phase");
    CHECK(patch_warp_sec(0) == 1 && patch_warp_sec(-4) == -86400 && patch_warp_sec(9) == 157788000, "warp");
    if (wav) fclose(wav);
    printf(fails ? "patch: %d FAILED\n" : "patch: all ok\n", fails);
    return fails != 0;
}
