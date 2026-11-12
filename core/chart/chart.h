/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA charts: a moment's sky as an astrologer reads it (zodiac, houses), the aspects inside one chart or between
 * two (transits to a birth chart), and the facts-first reading built from them. Integer only (device code).
 * Astronomy is computed; the reading is symbolic interpretation, not prediction: every line of it names the
 * calculation it comes from. */
#ifndef CHART_H
#define CHART_H
#include "core/astro/astro.h"

typedef struct {                    /* birth data as entered: local civil time and place */
    int16_t year;
    uint8_t month, day, hour, minute;
    uint8_t time_known;             /* 0: only the day is known. Noon is used and houses are counted from the Sun's sign */
    uint8_t set;                    /* 0: no birth data yet */
    int16_t tz_q;                   /* UTC offset, quarter hours, east positive */
    int16_t lat10, lon10;           /* tenths of a degree, north and east positive */
} chart_birth_t;

typedef struct {
    int32_t days;                   /* the instant: UT days from 1970-01-01 and seconds */
    uint32_t sec;
    astro_sky_t sky;                /* tropical longitudes and daily motions */
    astro_bam_t lon[ASTRO_NBODY];   /* longitudes in the chosen zodiac (tropical, or sidereal Lahiri) */
    astro_houses_t h;               /* in the chosen zodiac too */
    uint8_t house[ASTRO_NBODY];     /* 0..11 */
    uint8_t solar;                  /* 1: houses are whole signs from the Sun's (no birth time or no place) */
} chart_t;

typedef struct {
    uint8_t a, b;                   /* bodies: within one chart a < b; between two, a is the first chart's */
    uint8_t kind, applying;
    int32_t err;                    /* separation - exact angle, BAM (in the harmonic chart) */
    uint16_t str;                   /* closeness, Q15: 32767 exact, 0 at the edge of the orb */
    int16_t days_q4;                /* days until exact (applying) or since (separating), in 1/16 day; 32767: unknown or far */
} chart_asp_t;
#define CHART_MAX_ASP 48u

/* a chart for a UT instant. place: 0 = none (solar houses) */
void chart_cast(chart_t *c, int32_t ut_days, uint32_t ut_sec, int place, int lat10, int lon10, unsigned hsys, int sidereal);
/* the birth chart; returns 0 if b is not set */
int chart_natal(chart_t *c, const chart_birth_t *b, unsigned hsys, int sidereal);
/* the birth instant in UT */
void chart_birth_ut(const chart_birth_t *b, int32_t *days, uint32_t *sec);
/* the aspects of the harm-th harmonic within x (y == x) or from x's bodies to y's, among the first nkinds kinds,
 * within orb (BAM, measured in the harmonic chart), the closest first. Returns how many (<= max). */
unsigned chart_aspects(const chart_t *x, const chart_t *y, unsigned harm, uint32_t orb, unsigned nkinds, chart_asp_t *out, unsigned max);
unsigned chart_moon_phase(const chart_t *c);       /* 0..255: 0 new, 64 first quarter, 128 full */

/* ---- names and the reading */
extern const char *const chart_body_name[ASTRO_NBODY];
extern const char *const chart_sign_name[12];
extern const char *const chart_aspect_name[ASTRO_NASPECT];
extern const char *const chart_hsys_name[ASTRO_NHSYS];
enum { CHART_FIRE, CHART_EARTH, CHART_AIR, CHART_WATER };
enum { CHART_CARDINAL, CHART_FIXED, CHART_MUTABLE };
#define chart_element(sign) ((sign) & 3u)
#define chart_modality(sign) ((sign) % 3u)
int chart_dignity(unsigned body, unsigned sign);   /* +2 domicile, +1 exaltation, -1 fall, -2 detriment, 0 */
/* a transit (body a of the sky to body b of the birth chart) in words, four short lines of capitals:
 *   0 the fact ("SATURN SQUARE MOON")   1 the rule ("STRUCTURE TESTS FEELING")
 *   2 where ("FROM WORK TO HOME")       3 when ("APPLYING 1D12 EXACT IN 9 DAYS")
 * Each fits n characters. */
void chart_read_transit(const chart_t *sky, const chart_t *natal, const chart_asp_t *a, unsigned line, char *out, unsigned n);
/* a placement of the birth chart: 0 "MOON IN SCORPIO"  1 "FEELING IS INTENSE"  2 "HOUSE 4 HOME"  3 dignity or motion */
void chart_read_place(const chart_t *c, unsigned body, unsigned line, char *out, unsigned n);
#endif
