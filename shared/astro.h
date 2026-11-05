/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ASTRA_ASTRO_H
#define ASTRA_ASTRO_H
#include <stdint.h>
/* Gregorian UTC, 1950-01-01 through 2050-12-31; no leap seconds, no timezone guessing. */
int astra_day(int year, int month, int day, int32_t *days_since_2000);
/* Approximate geocentric solar ecliptic longitude, millidegrees [0,360000).
 * seconds_utc: 0..86399. Returns -1 for invalid/out-of-range input. */
int32_t astra_sun(int32_t days_since_2000, int32_t seconds_utc);
int32_t astra_separation(int32_t a, int32_t b);
/* Engineering signal only: 220..579 Hz, explicitly NOT an artistic mapping. */
uint32_t astra_test_hz(int32_t longitude);
#endif
