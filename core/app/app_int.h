/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA FM-1: the instrument's state, shared by app.c (what it does) and app_draw.c (what it shows). */
#ifndef APP_INT_H
#define APP_INT_H
#include "app.h"
#include "core/ui/gfx.h"

typedef struct {                    /* what survives a power-off, beside the presets */
    uint32_t magic;
    int32_t days; uint32_t sec;     /* the clock as last saved, UT */
    chart_birth_t birth;
    int16_t tz_q;                   /* the zone "now" is shown and entered in, quarter hours */
    uint8_t hsys, sidereal, slot, clock_saved, torb, pad;
} app_settings_t;
#define APP_SET_MAGIC 0x32544553u   /* "SET2" */
typedef struct { uint32_t magic, valid; patch_t p[8]; } app_bank_t;
#define APP_BANK_MAGIC 0x314B4E42u  /* "BNK1" */

typedef struct { const char *name; uint8_t kind, off; int16_t lo, hi; } app_par_t;
enum { PK_G, PK_GS, PK_O, PK_OS, PK_X };
enum { XP_NONE, XP_DAY, XP_MONTH, XP_YEAR, XP_TORB, XP_NOW_Y, XP_NOW_MO, XP_NOW_D, XP_NOW_H, XP_B_Y, XP_B_MO, XP_B_D,
       XP_B_H, XP_LAT, XP_LON, XP_B_TZ, XP_KNOWN, XP_HSYS, XP_ZODIAC, XP_TZ };
enum { DIRTY_DERIVE = 1, DIRTY_NATAL = 2, DIRTY_SET = 4, DIRTY_SKY = 8 };
#define APP_TL_N 61u                /* the timeline: a day a column, 30 either side */

typedef struct {
    const app_store_t *st;
    app_settings_t set;
    app_bank_t bank;
    patch_t patch;
    fm_prog_t prog;
    patch_route_t route;
    chart_t natal, sky;
    uint32_t ms, now_ms, calc_ms, set_ms, save_ms, msg_ms, saves;
    int32_t now_days, sky_days, calc_days, sky_rem, tl_days;
    uint32_t now_sec, sky_sec, calc_sec;
    uint32_t keys_down, keys_latched;
    uint32_t bump_ms[APP_NENC];
    uint32_t tl_lon[APP_TL_N][ASTRO_NPLANET];
    chart_asp_t tr[12];
    uint16_t master;
    int8_t octave;
    uint8_t has_natal, clock, live, run, page, sub, sel, latch, save_arm, save_pending, edited, dirty, bank_no;
    uint8_t ntr, tr_sel, tl_fill, tl_ok;
    uint8_t key_note[27];
    uint8_t bumps[APP_NENC];        /* how often each knob was turned against its end */
    char msg[32];
} app_t;
extern app_t A;

const app_par_t *app_pars(void);    /* the four knobs of the page showing */
int app_par_get(const app_par_t *p);
unsigned app_par_norm(const app_par_t *p);   /* 0..255 for the dial */
void app_local(int32_t days, uint32_t sec, int tz_q, int32_t *y, uint32_t *mo, uint32_t *d, uint32_t *h, uint32_t *mi, uint32_t *s);
#endif
