/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA core astronomy: integer-only, no FPU, no libm, no libc, no heap, no 64-bit arithmetic, so the same file builds for
 * the FM-1 (JieLi pi32v2), the host tests and (later) WebAssembly for the site.
 * Angles are BAM: uint32_t, 2^32 = 360 deg, so wraparound at 0/360 is the integer's own overflow.
 * Supported range: years 1800-2200 (T^2 terms are scaled for |T| <= 2.2). See docs/ACCURACY.md for measured errors.
 * Sun, Moon, calendar: astro.c. Planets: planets.c. */
#ifndef ASTRO_H
#define ASTRO_H
#include <stdint.h>

typedef uint32_t astro_bam_t;
/* an instant in TT: whole days from J2000.0 (2000-01-01 12:00 TT) and the day fraction in 1/65536 */
typedef struct { int32_t days; uint32_t frac16; } astro_time_t;
typedef struct { uint32_t a0, ri; int32_t rf, t2; } astro_lin_t;      /* see tools/gen_astro_tables.py */
typedef struct { int8_t d, m, mp, f; int32_t amp; } astro_term_t;
typedef struct { int32_t amp; astro_lin_t arg; } astro_vterm_t;    /* a VSOP87 term: amp * cos(arg) */

/* days from 1970-01-01 of a proleptic Gregorian date (H. Hinnant's algorithm) */
int32_t astro_days_from_civil(int32_t y, uint32_t m, uint32_t d);
/* the inverse: days from 1970-01-01 to a proleptic Gregorian date */
void astro_civil_from_days(int32_t z, int32_t *y, uint32_t *m, uint32_t *d);
/* Delta T = TT - UT in seconds (table 1800-2050, held constant outside: an approximation, documented) */
int32_t astro_delta_t(int32_t year);
/* a UTC civil date and time to TT */
astro_time_t astro_time_utc(int32_t y, uint32_t mo, uint32_t d, uint32_t h, uint32_t mi, uint32_t s);
/* a TT civil date and time (no Delta T) */
astro_time_t astro_time_tt(int32_t y, uint32_t mo, uint32_t d, uint32_t h, uint32_t mi, uint32_t s);

int32_t astro_sin_q30(astro_bam_t a);                 /* sin, Q30 */
int32_t astro_mul_q30(int32_t a, int32_t s_q30);      /* a * s / 2^30, |a| < 2^27 */

/* geocentric ecliptic longitude of date. apparent: with nutation (and aberration for the Sun) */
astro_bam_t astro_sun_lon(astro_time_t t, int apparent);
astro_bam_t astro_moon_lon(astro_time_t t, int apparent);

/* nutation in longitude (BAM, signed), Meeus 22 short form */
int32_t astro_nutation(astro_time_t t);

/* Planets: apparent geocentric ecliptic place of date, as the Sun and Moon (docs/ACCURACY.md has the measured errors).
 * Mercury..Neptune: truncated VSOP87D; Pluto: fitted to JPL DE441 for 1800-01-01..2201-01-02 (held outside). */
enum { ASTRO_MERCURY, ASTRO_VENUS, ASTRO_MARS, ASTRO_JUPITER, ASTRO_SATURN, ASTRO_URANUS, ASTRO_NEPTUNE, ASTRO_PLUTO,
       ASTRO_NPLANET };
typedef struct {
    astro_bam_t lon;       /* apparent longitude of date */
    int32_t lat;           /* apparent latitude, BAM (signed) */
    int32_t dist;          /* distance from the Earth, au Q23 */
} astro_pos_t;
void astro_planet(int body, astro_time_t t, astro_pos_t *p);
/* motion in longitude over the day centred on t, BAM per day (signed): < 0 is retrograde */
int32_t astro_planet_speed(int body, astro_time_t t);

/* display helpers: zodiac sign 0..11 (Aries..Pisces), and position within the sign in arcminutes 0..1799 */
uint32_t astro_sign(astro_bam_t a);
uint32_t astro_sign_arcmin(astro_bam_t a);
/* angle to degrees * 1000 (0..359999), for logs and tests */
uint32_t astro_mdeg(astro_bam_t a);
#endif
