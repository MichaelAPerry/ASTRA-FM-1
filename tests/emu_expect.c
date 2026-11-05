/* SPDX-License-Identifier: GPL-3.0-only
 * The host side of the emulator gate (tools/emu_smoke.sh): for a UTC instant as the firmware holds it (days from
 * 1970-01-01, seconds of the day), print what core/astro gives here: the Sun's and the Moon's longitude (BAM, hex) and
 * the TEMPORARY test tone's MIDI note. The firmware runs the same source, so the two must agree to the bit.
 *   emu_expect DAYS SEC  ->  "SUN MOON NOTE"   e.g. "8c2f... 1a7b... 62"
 *   emu_expect planet BODY DAYS SEC  ->  "LON SPEED"   (BODY 0 Mercury .. 7 Pluto; speed: BAM/day, signed, hex) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/astro/astro.h"

int main(int argc, char **argv) {
    int32_t y, days;
    uint32_t m, d, sec;
    astro_time_t t;
    astro_bam_t moon;
    if (argc == 5 && !strcmp(argv[1], "planet")) {
        astro_pos_t p;
        int body = atoi(argv[2]);
        days = (int32_t)strtol(argv[3], 0, 0);
        sec = (uint32_t)strtoul(argv[4], 0, 0);
        if (body < 0 || body >= ASTRO_NPLANET || sec >= 86400u) { fprintf(stderr, "emu_expect: bad planet args\n"); return 2; }
        astro_civil_from_days(days, &y, &m, &d);
        t = astro_time_utc(y, m, d, sec / 3600u, sec / 60u % 60u, sec % 60u);
        astro_planet(body, t, &p);
        printf("%08x %08x\n", (unsigned)p.lon, (unsigned)astro_planet_speed(body, t));
        return 0;
    }
    if (argc != 3) { fprintf(stderr, "usage: emu_expect DAYS SEC | emu_expect planet BODY DAYS SEC\n"); return 2; }
    days = (int32_t)strtol(argv[1], 0, 0);
    sec = (uint32_t)strtoul(argv[2], 0, 0);
    if (sec >= 86400u) { fprintf(stderr, "emu_expect: SEC >= 86400\n"); return 2; }
    astro_civil_from_days(days, &y, &m, &d);
    t = astro_time_utc(y, m, d, sec / 3600u, sec / 60u % 60u, sec % 60u);
    moon = astro_moon_lon(t, 1);
    printf("%08x %08x %u\n", (unsigned)astro_sun_lon(t, 1), (unsigned)moon, 57u + astro_sign(moon));   /* 57: astra.c TONE_BASE */
    return 0;
}
