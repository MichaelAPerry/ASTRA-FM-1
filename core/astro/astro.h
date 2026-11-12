/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA core astronomy: integer-only, no FPU, no libm, no libc, no heap, no 64-bit arithmetic, so the same file builds for
 * the FM-1 (JieLi pi32v2), the host tests and (later) WebAssembly for the site.
 * Angles are BAM: uint32_t, 2^32 = 360 deg, so wraparound at 0/360 is the integer's own overflow.
 * Supported range: years 1800-2200 (T^2 terms are scaled for |T| <= 2.2). See docs/ACCURACY.md for measured errors. */
#ifndef ASTRO_H
#define ASTRO_H
#include <stdint.h>

typedef uint32_t astro_bam_t;
/* an instant in TT: whole days from J2000.0 (2000-01-01 12:00 TT) and the day fraction in 1/65536 */
typedef struct { int32_t days; uint32_t frac16; } astro_time_t;
typedef struct { uint32_t a0, ri; int32_t rf, t2; } astro_lin_t;      /* see tools/gen_astro_tables.py */
typedef struct { int8_t d, m, mp, f; int32_t amp; } astro_term_t;

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

#define astro_cos_q30(a) astro_sin_q30((a) + 0x40000000u)
int32_t astro_mulq(int32_t a, int32_t b, unsigned k); /* (a * b) >> k, any magnitudes (result must fit) */
int32_t astro_divq(int32_t a, int32_t b, unsigned k); /* (a << k) / b */
uint32_t astro_isqrt(uint32_t v);
astro_bam_t astro_atan2(int32_t y, int32_t x, int32_t *r);   /* the angle of (x, y); *r (if not 0) = its length */
astro_bam_t astro_asin(int32_t s_q30);
astro_bam_t astro_lin(const astro_lin_t *e, astro_time_t t);  /* a0 + rate * t + t2 * T^2 */
int32_t astro_t_q16(astro_time_t t);                  /* Julian centuries from J2000, Q16 */
int32_t astro_nutation(astro_time_t t);               /* nutation in longitude, BAM (signed) */
astro_bam_t astro_mean_node(astro_time_t t);          /* the Moon's mean ascending node, of date */
astro_bam_t astro_sun_mean_lon(astro_time_t t);

/* geocentric ecliptic longitude of date. apparent: with nutation (and aberration for the Sun).
 * astro_sun_lon is the low-precision Sun of the first slice (27 arcsec); astro_sky's Sun is the better one. */
astro_bam_t astro_sun_lon(astro_time_t t, int apparent);
astro_bam_t astro_moon_lon(astro_time_t t, int apparent);

/* display helpers: zodiac sign 0..11 (Aries..Pisces), and position within the sign in arcminutes 0..1799 */
uint32_t astro_sign(astro_bam_t a);
uint32_t astro_sign_arcmin(astro_bam_t a);
/* angle to degrees * 1000 (0..359999), for logs and tests */
uint32_t astro_mdeg(astro_bam_t a);

/* ---- the whole sky (sky.c): apparent geocentric ecliptic longitude of date (tropical), and its daily motion */
enum { ASTRO_SUN, ASTRO_MOON, ASTRO_MERCURY, ASTRO_VENUS, ASTRO_MARS, ASTRO_JUPITER, ASTRO_SATURN, ASTRO_URANUS,
       ASTRO_NEPTUNE, ASTRO_PLUTO, ASTRO_NPLANET, ASTRO_NODE = ASTRO_NPLANET, ASTRO_NBODY };
typedef struct {
    astro_bam_t lon[ASTRO_NBODY];
    int32_t spd[ASTRO_NBODY];                         /* BAM per day; negative: retrograde */
} astro_sky_t;
void astro_sky(astro_time_t t, astro_sky_t *s);
astro_bam_t astro_ayanamsa(astro_time_t t);           /* Lahiri: sidereal = tropical - this */

/* ---- angles and houses (houses.c). ut_days: days from 1970-01-01, ut_sec: seconds of that UT day.
 * lat, lon_east: signed BAM (north and east positive). */
enum { ASTRO_H_PLACIDUS, ASTRO_H_KOCH, ASTRO_H_REGIO, ASTRO_H_CAMPANUS, ASTRO_H_PORPHYRY, ASTRO_H_EQUAL, ASTRO_H_WHOLE,
       ASTRO_NHSYS };
typedef struct { astro_bam_t asc, mc, cusp[12]; uint32_t system; } astro_houses_t;
/* system: what was asked for; h->system: what was used (Placidus and Koch do not exist near the poles: Porphyry) */
void astro_houses(int32_t ut_days, uint32_t ut_sec, int32_t lat, int32_t lon_east, unsigned system, astro_houses_t *h);
unsigned astro_house_of(const astro_houses_t *h, astro_bam_t lon);   /* 0..11 */
astro_bam_t astro_obliquity(astro_time_t t);          /* true obliquity of the ecliptic */

/* ---- aspects (aspect.c) */
enum { ASTRO_CONJ, ASTRO_SEXTILE, ASTRO_SQUARE, ASTRO_TRINE, ASTRO_OPPOSITION, ASTRO_NMAJOR,
       ASTRO_SEMISEXTILE = ASTRO_NMAJOR, ASTRO_QUINCUNX, ASTRO_SEMISQUARE, ASTRO_SESQUI, ASTRO_QUINTILE,
       ASTRO_BIQUINTILE, ASTRO_NASPECT };
extern const astro_bam_t astro_aspect_angle[ASTRO_NASPECT];
typedef struct { uint8_t kind, applying; int32_t err; } astro_aspect_t;   /* err: separation - exact angle, BAM */
/* the aspect between longitudes a and b moving at va and vb (BAM per day), among the first nkinds kinds, within
 * orb (BAM): 1 and *out, or 0. The tightest wins. */
int astro_aspect(astro_bam_t a, astro_bam_t b, int32_t va, int32_t vb, uint32_t orb, unsigned nkinds, astro_aspect_t *out);
#define ASTRO_DEG(d) ((astro_bam_t)((d) * 11930464.7111))    /* degrees to BAM (a constant expression) */
#endif
