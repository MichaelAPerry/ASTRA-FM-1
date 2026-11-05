/* SPDX-License-Identifier: GPL-3.0-only */
/* ASTRA FM-1, vertical slice (docs/HANDOFF.md step 2). Built on Dinghy (zvenson/dinghy, GPL-3.0-only): native/build.sh
 * copies this file to Dinghy's firmware/src/dinghy.c, so platform.c (updates, USB, panel, screen, storage) is Dinghy's.
 *
 * What it does: you enter a UTC date and time with the knobs; the FM-1 computes the Sun's and the Moon's geocentric
 * ecliptic longitude itself (core/astro, integer only) and shows sign, degree and arcminute. A test tone follows the
 * Moon's sign.
 *
 * TEMPORARY, not the design (owner decisions D2, D3, and the screen layout and controls are still open):
 *   - the tone: one sine, Moon in Aries..Pisces -> A3..G#4 (12 signs, 12 semitones);
 *   - the controls: K1 year, K2 month, K3 day, K4 hour, SELECT minute; PLAY sets the clock;
 *   - the layout and colours below.
 *
 * Time (owner decision D5): the FM-1 has no confirmed battery clock, so this never calls a time "now" unless it was set
 * or confirmed this power cycle. States:
 *   NOT SET     nothing saved: the knobs pick a chart time (it does not run); PLAY = this is now
 *   UNCONFIRMED the saved time came back from flash and runs from boot, but the power-off gap is unknown; PLAY confirms
 *   SET         set or confirmed this power cycle: it runs, and is saved
 * The editor starts at J2000.0 (2000-01-01 12:00 UTC) when nothing is saved, marked NOT SET: a starting point to turn
 * from, never shown as the present. Time zone offset (D5): not in the slice yet; UTC only. */
#define APP_NAME "ASTRA"
#include "platform.c"
#include "core/astro/astro.c"

/* ---- the sound (audio ISR: one sine; the main loop writes inc, one 32-bit word) */
static volatile uint32_t tone_inc;
static uint32_t tone_ph;
static int32_t tone_amp;

static int32_t sine(uint32_t ph)                /* Dinghy's refined parabola: a cycle over 2^32 -> -32767..32767 */
{
    int32_t x = (int32_t)ph >> 16, ax = x < 0 ? -x : x, y, ay;
    y = (x * (32768 - ax)) >> 13;
    ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    return y > 32767 ? 32767 : y < -32767 ? -32767 : y;
}

static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t i, ph = tone_ph, inc = tone_inc;
    int32_t amp = tone_amp, g = inc ? 6000 : 0;    /* about -15 dBFS */
    for (i = 0; i < n; i++) {
        int32_t s;
        if (amp < g) amp += 2;                     /* fade in over ~70 ms: no click at boot */
        else if (amp > g) amp -= 2;
        ph += inc;
        s = (sine(ph) * amp) >> 15;
        out[2u * i] += s;
        out[2u * i + 1u] += s;
    }
    tone_ph = ph;
    tone_amp = amp;
}

/* Dinghy's phase steps for MIDI 120..131; note n: INC_C9[n % 12] >> (10 - n / 12) */
static const uint32_t INC_C9[12] = {
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
    1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};
#define TONE_BASE 57u                           /* A3: Moon in Aries. TEMPORARY */

/* ---- the clock: days from 1970-01-01 and seconds of the day, advanced from fm1_ms */
enum { T_NOT_SET, T_UNCONFIRMED, T_SET };
static struct { uint32_t magic; int32_t days; uint32_t sec; } saved;   /* what survives a power-off */
#define SAVE_MAGIC 0x31525453u                  /* "STR1": this struct's layout, version 1 */
static int32_t clk_days;
static uint32_t clk_sec, clk_ms, clk_state;
/* what the emulator gate reads back (tools/emu_smoke.sh, fm1-emu --dump): the last computed instant and its result.
 * Not static, so build.sh finds its address by name. Written only by the main loop. */
volatile struct { uint32_t magic; int32_t days; uint32_t sec, state; astro_bam_t sun, moon; uint32_t note; } astra_probe;
#define DAYS_MIN (-62091)                       /* 1800-01-01: core/astro's supported range */
#define DAYS_MAX 84371                          /* 2200-12-31 */

static void clk_norm(int32_t ds)               /* add ds seconds, carry into days, clamp to the range */
{
    int32_t s = (int32_t)clk_sec + ds;
    while (s < 0) { s += 86400; clk_days--; }
    while (s >= 86400) { s -= 86400; clk_days++; }
    clk_sec = (uint32_t)s;
    if (clk_days < DAYS_MIN) clk_days = DAYS_MIN, clk_sec = 0;
    if (clk_days > DAYS_MAX) clk_days = DAYS_MAX, clk_sec = 86399;
}
static void clk_run(void)                       /* fold whole elapsed seconds in: fm1_ms may wrap, the clock does not */
{
    uint32_t el = (fm1_ms - clk_ms) / 1000u;
    if (clk_state == T_NOT_SET) { clk_ms = fm1_ms; return; }   /* a chart time stands still */
    if (el) { clk_ms += el * 1000u; clk_norm((int32_t)el); }
}
static uint32_t month_len(int32_t y, uint32_t m)
{
    return (uint32_t)(astro_days_from_civil(m == 12u ? y + 1 : y, m == 12u ? 1u : m + 1u, 1u) - astro_days_from_civil(y, m, 1u));
}
static void clk_edit(int32_t dy, int32_t dmo, int32_t dd, int32_t dh, int32_t dmi)
{
    int32_t y;
    uint32_t m, d, ml;
    astro_civil_from_days(clk_days, &y, &m, &d);
    if (dy || dmo) {                            /* year and month: keep the day where it fits */
        int32_t mm = (int32_t)m - 1 + dmo;
        y += dy;
        while (mm < 0) mm += 12;                /* the month wraps without changing the year */
        m = (uint32_t)(mm % 12) + 1u;
        y = y < 1800 ? 1800 : y > 2200 ? 2200 : y;
        ml = month_len(y, m);
        if (d > ml) d = ml;
        clk_days = astro_days_from_civil(y, m, d);
    }
    clk_days += dd;
    clk_norm(dh * 3600 + dmi * 60);
}

/* ---- the screen. TEMPORARY layout */
#define C_ACC RGB(150, 170, 255)
#define C_GREY RGB(110, 110, 110)
#define C_WARN RGB(255, 180, 60)
#define C_OK RGB(110, 220, 130)
static const char SIGN3[12][4] = {"ARI", "TAU", "GEM", "CAN", "LEO", "VIR", "LIB", "SCO", "SAG", "CAP", "AQU", "PIS"};
static const char *const NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

static char *put(char *p, const char *s) { while (*s) *p++ = *s++; return p; }
static char *put_n(char *p, uint32_t v, uint32_t digits)
{
    char t[10];
    uint32_t i = 0;
    do { t[i++] = (char)('0' + v % 10u); v /= 10u; } while (v || i < digits);
    while (i) *p++ = t[--i];
    return p;
}
static void lon_text(char *p, astro_bam_t a)    /* "13 SCO 42'" */
{
    uint32_t am = astro_sign_arcmin(a);
    p = put_n(p, am / 60u, 2);
    *p++ = ' ';
    p = put(p, SIGN3[astro_sign(a)]);
    *p++ = ' ';
    p = put_n(p, am % 60u, 2);
    *p++ = '\'';
    *p = 0;
}
static void draw_time(void)
{
    char s[32], *p = put(s, "UTC ");
    int32_t y;
    uint32_t m, d;
    astro_civil_from_days(clk_days, &y, &m, &d);
    p = put_n(p, (uint32_t)y, 4); *p++ = '-';
    p = put_n(p, m, 2); *p++ = '-';
    p = put_n(p, d, 2); *p++ = ' ';
    p = put_n(p, clk_sec / 3600u, 2); *p++ = ':';
    p = put_n(p, clk_sec / 60u % 60u, 2); *p++ = ':';
    p = put_n(p, clk_sec % 60u, 2);
    *p = 0;
    draw_text_box(0, 48, 240, &FONT_S, s, C_WHITE, 1);
}
static void draw_state(void)
{
    static const char *const TXT[3] = {"NOT SET: CHART TIME", "UNCONFIRMED: PRESS PLAY", "SET THIS POWER-ON"};
    static const uint16_t COL[3] = {C_WARN, C_WARN, C_OK};
    draw_text_box(0, 68, 240, &FONT_S, TXT[clk_state], COL[clk_state], 1);
}

static void app_main(void)
{
    uint32_t drawn_sec = ~0u, drawn_state = ~0u, drawn_note = ~0u, dirty = 0, t_change = 0;
    int32_t drawn_days = 0;
    astro_bam_t sun = 0, moon = 0;
    uint32_t sun_drawn = ~0u, moon_drawn = ~0u;   /* sign * 1800 + arcminute as drawn */

    if (!settings_load(&saved, sizeof saved) && saved.magic == SAVE_MAGIC && saved.days >= DAYS_MIN &&
        saved.days <= DAYS_MAX && saved.sec < 86400u) {
        clk_days = saved.days, clk_sec = saved.sec, clk_state = T_UNCONFIRMED;
    } else {
        clk_days = 10957, clk_sec = 43200, clk_state = T_NOT_SET;   /* J2000.0: where the editor starts */
    }
    clk_ms = fm1_ms;

    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(0, 8, 240, &FONT_L, APP_NAME, C_ACC, 1);
    draw_text_box(0, 98, 240, &FONT_S, "SUN", C_GREY, 1);
    draw_text_box(0, 150, 240, &FONT_S, "MOON", C_GREY, 1);

    for (;;) {
        uint32_t b, rel, note;
        int32_t e1, e2, e3, e4, e5;
        platform_poll();

        b = fm1_input_edges(&rel);
        e1 = fm1_enc_take(ENC_KNOB1); e2 = fm1_enc_take(ENC_KNOB2); e3 = fm1_enc_take(ENC_KNOB3);
        e4 = fm1_enc_take(ENC_KNOB4); e5 = fm1_enc_take(ENC_SELECT);
        clk_run();
        if (e1 | e2 | e3 | e4 | e5) {
            clk_edit(e1, e2, e3, e4, e5);
            if (clk_state == T_SET) dirty = 1, t_change = fm1_ms;   /* a set clock, adjusted: save it again */
        }
        if (b & (1u << BTN_PLAY) && clk_state != T_SET) {
            clk_state = T_SET;
            clk_ms = fm1_ms;
            dirty = 1, t_change = fm1_ms - 2000u;   /* save now */
        }

        if (clk_sec != drawn_sec || clk_days != drawn_days) {   /* once a second at most: Sun and Moon from core/astro */
            astro_time_t t;
            int32_t y;
            uint32_t m, d;
            astro_civil_from_days(clk_days, &y, &m, &d);
            t = astro_time_utc(y, m, d, clk_sec / 3600u, clk_sec / 60u % 60u, clk_sec % 60u);
            sun = astro_sun_lon(t, 1);
            moon = astro_moon_lon(t, 1);
            note = TONE_BASE + astro_sign(moon);
            astra_probe.days = clk_days, astra_probe.sec = clk_sec, astra_probe.sun = sun, astra_probe.moon = moon;
            astra_probe.note = note, astra_probe.magic = 0x50525453u;   /* "STRP": the fields above are filled */
            tone_inc = INC_C9[note % 12u] >> (10u - note / 12u);
            if (note != drawn_note) {
                char s[24], *p = put(s, "TEST TONE ");
                p = put(p, NOTE[note % 12u]);
                p = put_n(p, note / 12u - 1u, 1);
                put(p, " (TEMP)");
                draw_text_box(0, 214, 240, &FONT_S, s, C_GREY, 1);
                drawn_note = note;
            }
            draw_time();
            drawn_sec = clk_sec, drawn_days = clk_days;
        }
        if (clk_state != drawn_state) { draw_state(); drawn_state = clk_state; astra_probe.state = clk_state; }
        if (astro_sign(sun) * 1800u + astro_sign_arcmin(sun) != sun_drawn) {
            char s[16];
            lon_text(s, sun);
            draw_text_box(0, 116, 240, &FONT_L, s, C_WHITE, 1);
            sun_drawn = astro_sign(sun) * 1800u + astro_sign_arcmin(sun);
        }
        if (astro_sign(moon) * 1800u + astro_sign_arcmin(moon) != moon_drawn) {
            char s[16];
            lon_text(s, moon);
            draw_text_box(0, 168, 240, &FONT_L, s, C_WHITE, 1);
            moon_drawn = astro_sign(moon) * 1800u + astro_sign_arcmin(moon);
        }

        if (dirty && fm1_ms - t_change >= 2000u) {   /* 2 s after the last change; the erase stalls the tone ~50 ms */
            saved.magic = SAVE_MAGIC, saved.days = clk_days, saved.sec = clk_sec;
            settings_save(&saved, sizeof saved);
            dirty = 0;
        }
    }
}
