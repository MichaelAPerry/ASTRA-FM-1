/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA charts (see chart.h). Integer only. */
#include "chart.h"

const char *const chart_body_name[ASTRO_NBODY] = {"SUN", "MOON", "MERCURY", "VENUS", "MARS", "JUPITER", "SATURN", "URANUS", "NEPTUNE", "PLUTO", "NODE"};
const char *const chart_sign_name[12] = {"ARIES", "TAURUS", "GEMINI", "CANCER", "LEO", "VIRGO", "LIBRA", "SCORPIO", "SAGITTARIUS", "CAPRICORN", "AQUARIUS", "PISCES"};
const char *const chart_aspect_name[ASTRO_NASPECT] = {"CONJUNCT", "SEXTILE", "SQUARE", "TRINE", "OPPOSITE", "SEMISEXTILE", "QUINCUNX", "SEMISQUARE", "SESQUISQUARE", "QUINTILE", "BIQUINTILE"};
const char *const chart_hsys_name[ASTRO_NHSYS] = {"PLACIDUS", "KOCH", "REGIOMONTANUS", "CAMPANUS", "PORPHYRY", "EQUAL", "WHOLE SIGN"};

static int32_t deg10_bam(int d10) {          /* tenths of a degree to signed BAM */
    return d10 * 1193046 + (d10 * 30874 >> 16);     /* 2^32 / 3600 = 1193046.471 */
}

void chart_cast(chart_t *c, int32_t ut_days, uint32_t ut_sec, int place, int lat10, int lon10, unsigned hsys, int sidereal) {
    int32_t y;
    uint32_t m, d, i;
    astro_time_t t;
    astro_bam_t ay;
    astro_civil_from_days(ut_days, &y, &m, &d);
    t = astro_time_utc(y, m, d, ut_sec / 3600u, ut_sec / 60u % 60u, ut_sec % 60u);
    c->days = ut_days; c->sec = ut_sec;
    astro_sky(t, &c->sky);
    ay = sidereal ? astro_ayanamsa(t) + (uint32_t)astro_nutation(t) : 0u;
    for (i = 0; i < ASTRO_NBODY; i++) c->lon[i] = c->sky.lon[i] - ay;
    c->solar = (uint8_t)!place;
    if (place) {
        astro_houses(ut_days, ut_sec, deg10_bam(lat10), deg10_bam(lon10), hsys, &c->h);
        c->h.asc -= ay; c->h.mc -= ay;
        for (i = 0; i < 12u; i++) c->h.cusp[i] -= ay;
        if (hsys == ASTRO_H_WHOLE) {             /* whole signs of the chosen zodiac, not of the tropical one */
            uint32_t sg = astro_sign(c->h.asc);
            for (i = 0; i < 12u; i++) c->h.cusp[i] = (sg + i) * 0x15555555u + (sg + i) / 3u;
        }
    } else {                                    /* solar houses: whole signs from the Sun's */
        uint32_t sg = astro_sign(c->lon[ASTRO_SUN]);
        for (i = 0; i < 12u; i++) c->h.cusp[i] = (sg + i) * 0x15555555u + (sg + i) / 3u;
        c->h.asc = c->h.cusp[0]; c->h.mc = c->h.cusp[9]; c->h.system = ASTRO_H_WHOLE;
    }
    for (i = 0; i < ASTRO_NBODY; i++) c->house[i] = (uint8_t)astro_house_of(&c->h, c->lon[i]);
}

void chart_birth_ut(const chart_birth_t *b, int32_t *days, uint32_t *sec) {
    int32_t dz = astro_days_from_civil(b->year, b->month, b->day);
    int32_t s = b->time_known ? (int32_t)b->hour * 3600 + (int32_t)b->minute * 60 : 43200;
    s -= (int32_t)b->tz_q * 900;
    while (s < 0) { s += 86400; dz--; }
    while (s >= 86400) { s -= 86400; dz++; }
    *days = dz; *sec = (uint32_t)s;
}

int chart_natal(chart_t *c, const chart_birth_t *b, unsigned hsys, int sidereal) {
    int32_t dz;
    uint32_t s;
    if (!b->set) return 0;
    chart_birth_ut(b, &dz, &s);
    chart_cast(c, dz, s, b->time_known, b->lat10, b->lon10, hsys, sidereal);
    return 1;
}

unsigned chart_aspects(const chart_t *x, const chart_t *y, unsigned harm, uint32_t orb, unsigned nkinds, chart_asp_t *out, unsigned max) {
    unsigned i, j, n = 0, k;
    if (!harm) harm = 1;
    for (i = 0; i < ASTRO_NPLANET; i++)
        for (j = x == y ? i + 1u : 0u; j < ASTRO_NPLANET; j++) {
            astro_aspect_t a;
            chart_asp_t e;
            int32_t va = x->sky.spd[i] * (int32_t)harm, vb = y->sky.spd[j] * (int32_t)harm, rel, ae;
            if (!astro_aspect(x->sky.lon[i] * harm, y->sky.lon[j] * harm, va, vb, orb, nkinds, &a)) continue;
            e.a = (uint8_t)i; e.b = (uint8_t)j; e.kind = a.kind; e.applying = a.applying; e.err = a.err;
            ae = a.err < 0 ? -a.err : a.err;
            e.str = (uint16_t)(32767u - (uint32_t)astro_divq(ae >> 1, (int32_t)(orb >> 1) + 1, 15));
            rel = va - vb; rel = rel < 0 ? -rel : rel;
            e.days_q4 = 32767;
            if (rel > (ae >> 11)) { int32_t dq = astro_divq(ae >> 4, (rel >> 8) + 1, 0); e.days_q4 = (int16_t)(dq > 32766 ? 32767 : dq); }   /* ae / rel * 16 */
            for (k = n < max ? n : max - 1u; k > 0 && out[k - 1u].str < e.str; k--) if (k < max) out[k] = out[k - 1u];
            if (k < max && (n < max || out[k].str < e.str || k < max - 1u)) out[k] = e;
            if (n < max) n++;
        }
    return n;
}

unsigned chart_moon_phase(const chart_t *c) { return (c->sky.lon[ASTRO_MOON] - c->sky.lon[ASTRO_SUN]) >> 24; }

/* essential dignities (the traditional table; the outer planets by modern rulership): domicile, exaltation */
int chart_dignity(unsigned body, unsigned sign) {
    static const uint16_t DOM[ASTRO_NPLANET] = {1u << 4, 1u << 3, 1u << 2 | 1u << 5, 1u << 1 | 1u << 6, 1u << 0 | 1u << 7,
                                                1u << 8 | 1u << 11, 1u << 9 | 1u << 10, 1u << 10, 1u << 11, 1u << 7};
    static const int8_t EXALT[ASTRO_NPLANET] = {0, 1, 5, 11, 9, 3, 6, -1, -1, -1};
    if (body >= ASTRO_NPLANET || sign > 11u) return 0;
    if (DOM[body] >> sign & 1u) return 2;
    if (DOM[body] >> ((sign + 6u) % 12u) & 1u) return -2;
    if (EXALT[body] == (int)sign) return 1;
    if (EXALT[body] >= 0 && (unsigned)(EXALT[body] + 6) % 12u == sign) return -1;
    return 0;
}

/* ---- the reading: original keyword rules (traditional significations in one word each). A line is always
 * [what the body stands for] [what the aspect does] [what the other stands for]: nothing that is not in the chart. */
static const char *const BODY_WORD[ASTRO_NBODY] = {"WILL", "FEELING", "MIND", "AFFECTION", "DRIVE", "GROWTH", "STRUCTURE", "CHANGE", "DREAM", "DEPTH", "PATH"};
static const char *const ASPECT_VERB[ASTRO_NASPECT] = {"MEETS", "OPENS", "TESTS", "FLOWS WITH", "FACES", "NUDGES", "ADJUSTS", "CHAFES", "STRAINS", "SHAPES", "SHAPES"};
static const char *const HOUSE_WORD[12] = {"SELF", "MEANS", "WORDS", "HOME", "PLAY", "WORK", "OTHERS", "SHARING", "HORIZONS", "CALLING", "FRIENDS", "SOLITUDE"};
static const char *const SIGN_WORD[12] = {"BOLD", "STEADY", "CURIOUS", "PROTECTIVE", "PROUD", "PRECISE", "FAIR", "INTENSE", "FREE", "PATIENT", "ORIGINAL", "FLUID"};

typedef struct { char *p, *end; } wr_t;
static void put(wr_t *w, const char *s) { while (*s && w->p < w->end) *w->p++ = *s++; }
static void put_n(wr_t *w, uint32_t v) {
    char t[10];
    uint32_t i = 0;
    do { t[i++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (i && w->p < w->end) *w->p++ = t[--i];
}

void chart_read_transit(const chart_t *sky, const chart_t *natal, const chart_asp_t *a, unsigned line, char *out, unsigned n) {
    wr_t w;
    uint32_t am;
    w.p = out; w.end = out + (n ? n - 1u : 0u);
    switch (line) {
    case 0: put(&w, chart_body_name[a->a]); put(&w, " "); put(&w, chart_aspect_name[a->kind]); put(&w, " "); put(&w, chart_body_name[a->b]); break;
    case 1: put(&w, BODY_WORD[a->a]); put(&w, " "); put(&w, ASPECT_VERB[a->kind]); put(&w, " "); put(&w, BODY_WORD[a->b]); break;
    case 2: put(&w, "FROM "); put(&w, HOUSE_WORD[astro_house_of(&natal->h, sky->lon[a->a])]); put(&w, " TO "); put(&w, HOUSE_WORD[natal->house[a->b]]); break;
    default:
        am = (uint32_t)(((a->err < 0 ? -a->err : a->err) >> 8) * 675) >> 15;   /* arcminutes: BAM * 21600 / 2^32 */
        put(&w, a->applying ? "APPLYING " : "SEPARATING "); put_n(&w, am / 60u); put(&w, "D"); put_n(&w, am % 60u);
        if (a->days_q4 < 16 * 400) { put(&w, a->applying ? " EXACT IN " : " EXACT "); put_n(&w, (uint32_t)(a->days_q4 + 8) / 16u); put(&w, a->applying ? " DAYS" : " DAYS AGO"); }
    }
    if (n) *w.p = 0;
}

void chart_read_place(const chart_t *c, unsigned body, unsigned line, char *out, unsigned n) {
    wr_t w;
    uint32_t sg = astro_sign(c->lon[body]);
    int dg = chart_dignity(body, sg);
    w.p = out; w.end = out + (n ? n - 1u : 0u);
    switch (line) {
    case 0: put(&w, chart_body_name[body]); put(&w, " IN "); put(&w, chart_sign_name[sg]); break;
    case 1: put(&w, BODY_WORD[body]); put(&w, " IS "); put(&w, SIGN_WORD[sg]); break;
    case 2: put(&w, c->solar ? "SOLAR HOUSE " : "HOUSE "); put_n(&w, c->house[body] + 1u); put(&w, " "); put(&w, HOUSE_WORD[c->house[body]]); break;
    default:
        put(&w, dg == 2 ? "AT HOME: STRONG" : dg == 1 ? "EXALTED: HONOURED" : dg == -1 ? "IN FALL: HUMBLED" : dg == -2 ? "IN DETRIMENT: STRAINED" : "");
        if (c->sky.spd[body] < 0 && body != ASTRO_NODE) put(&w, dg ? " RETROGRADE" : "RETROGRADE: TURNED INWARD");
    }
    if (n) *w.p = 0;
}
