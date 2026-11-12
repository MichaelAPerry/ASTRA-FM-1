/* SPDX-License-Identifier: GPL-3.0-only
 * The host side of the emulator gate (tools/emu_smoke.sh): what core/astro gives for a UT instant, to compare with
 * what the firmware computed in fm1-emu, bit for bit.
 *   emu_expect DAYS_FROM_1970 SECONDS  ->  the eleven longitudes (Sun..Pluto, Node), hex BAM */
#include <stdio.h>
#include <stdlib.h>
#include "core/chart/chart.h"

int main(int argc, char **argv) {
    static chart_t c;
    unsigned i;
    if (argc != 3) { fprintf(stderr, "usage: emu_expect DAYS_FROM_1970 SECONDS\n"); return 2; }
    chart_cast(&c, (int32_t)strtol(argv[1], 0, 10), (uint32_t)strtoul(argv[2], 0, 10), 0, 0, 0, 0, 0);
    for (i = 0; i < ASTRO_NBODY; i++) printf("%08x%c", (unsigned)c.sky.lon[i], i + 1 < ASTRO_NBODY ? ' ' : '\n');
    return 0;
}
