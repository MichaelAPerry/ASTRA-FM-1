/* SPDX-License-Identifier: GPL-3.0-only */
#include "astro.h"
#include <stdio.h>
#include <stdlib.h>
#include "sun_reference.h"
/* Checks remain active in Release builds (do not use NDEBUG-sensitive assert). */
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
int main(void)
{
    int32_t d, n, previous;
    unsigned i;
    for (i = 0; i < sizeof SUN_REFERENCE / sizeof SUN_REFERENCE[0]; ++i)
        CHECK(astra_separation(astra_sun(SUN_REFERENCE[i][0],SUN_REFERENCE[i][1]),SUN_REFERENCE[i][2]) <= 50);
    CHECK(astra_day(2000,1,1,&d) && d == 0);
    CHECK(astra_day(1950,1,1,&d) && d == -18262);
    CHECK(astra_day(2050,12,31,&d) && d == 18627);
    CHECK(astra_day(2000,2,29,&d)); CHECK(!astra_day(2001,2,29,&d));
    CHECK(!astra_day(1900,2,29,&d)); CHECK(!astra_day(2051,1,1,&d));
    CHECK(!astra_day(2000,0,1,&d)); CHECK(!astra_day(2000,12,32,&d));
    CHECK(!astra_day(2000,1,1,0));
    CHECK(astra_sun(-18263,0) == -1 && astra_sun(18628,0) == -1);
    CHECK(astra_sun(0,-1) == -1 && astra_sun(0,86400) == -1);
    CHECK(astra_separation(359999,1) == 2);
    CHECK(astra_separation(0,180000) == 180000);
    CHECK(astra_separation(-1,0) == -1);
    CHECK(astra_test_hz(-1) == 0 && astra_test_hz(360000) == 0);
    previous = astra_sun(-18262,43200);
    for (d = -18261; d <= 18627; ++d) {
        n = astra_sun(d,43200);
        CHECK(n >= 0 && n < 360000);
        CHECK(astra_separation(n,previous) > 900 && astra_separation(n,previous) < 1100);
        CHECK(astra_separation(astra_sun(d-1,86399),astra_sun(d,0)) <= 1);
        previous = n;
    }
    puts("PASS: calendar, range, wraparound, daily motion and midnight continuity across 1950-2050");
    return 0;
}
