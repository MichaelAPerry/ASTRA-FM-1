/* SPDX-License-Identifier: GPL-3.0-only */
#include "astro.h"
static int leap(int y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }
int astra_day(int y, int m, int d, int32_t *out)
{
    static const int md[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int i, n = -18262;
    if (!out || y < 1950 || y > 2050 || m < 1 || m > 12 || d < 1 ||
        d > md[m-1] + (m == 2 && leap(y))) return 0;
    for (i = 1950; i < y; ++i) n += 365 + leap(i);
    for (i = 1; i < m; ++i) n += md[i-1] + (i == 2 && leap(y));
    *out = n + d - 1;
    return 1;
}
/* Refined parabolic sine, Q15, adapted from Dinghy's sine() by
 * Leo Kuroshita / Hugelton Instruments (GPL-3.0-only).
 * Phase wraps at 2^32. Error is sampled in the independent reference
 * comparison; no floating point or 64-bit division. */
static int32_t sin_turn(uint32_t phase)
{
    int32_t x = (int32_t)(phase >> 16), ax, y, ay;
    if (x >= 32768) x -= 65536;
    ax = x < 0 ? -x : x;
    y = (x * (32768 - ax)) / 8192;
    ay = y < 0 ? -y : y;
    y += (((y * ay) / 32768 - y) * 7373) / 32768;
    return y;
}
int32_t astra_sun(int32_t day, int32_t sec)
{
    int32_t fraction;
    uint32_t mean, anomaly, longitude;
    if (day < -18262 || day > 18627 || sec < 0 || sec >= 86400) return -1;
    fraction = ((sec - 43200) * 16384) / 21600; /* J2000 is noon, fraction Q16 */
    mean = 3346018133u + (uint32_t)day * 11759232u +
           (uint32_t)(((int64_t)11759232 * fraction) >> 16);
    anomaly = 4265475187u + (uint32_t)day * 11758670u +
              (uint32_t)(((int64_t)11758670 * fraction) >> 16);
    longitude = mean + (uint32_t)(((int64_t)22846840 * sin_turn(anomaly)) >> 15) +
                (uint32_t)(((int64_t)238609 * sin_turn(anomaly * 2u)) >> 15);
    return (int32_t)(((uint64_t)longitude * 360000u) >> 32);
}
int32_t astra_separation(int32_t a, int32_t b)
{
    int32_t d;
    if (a < 0 || a >= 360000 || b < 0 || b >= 360000) return -1;
    d = a > b ? a-b : b-a;
    return d > 180000 ? 360000-d : d;
}
uint32_t astra_test_hz(int32_t longitude)
{ return longitude < 0 || longitude >= 360000 ? 0u : 220u + (uint32_t)longitude / 1000u; }
