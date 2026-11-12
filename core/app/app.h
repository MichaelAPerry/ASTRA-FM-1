/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA FM-1, the instrument: everything between the panel and the engine that is not hardware. The FM-1 glue
 * (native/astra.c) and the host tests feed it buttons, encoders, keys, MIDI and time, and give it a frame to draw in.
 * Integer only (device code). One instance. */
#ifndef APP_H
#define APP_H
#include <stdint.h>
#include "core/synth/patch.h"

/* the FM-1's panel, numbered as Dinghy numbers it */
enum { APP_BTN_OCT_DOWN, APP_BTN_OCT_UP, APP_BTN_FX, APP_BTN_SEL, APP_BTN_ENV, APP_BTN_LFO, APP_BTN_EDIT, APP_BTN_GLO,
       APP_BTN_HOME, APP_BTN_SAVE, APP_BTN_ARP, APP_BTN_SEQ, APP_BTN_PLAY, APP_BTN_REC, APP_NBTN };
enum { APP_ENC_SELECT, APP_ENC_ALGORITHM, APP_ENC_K1, APP_ENC_K2, APP_ENC_K3, APP_ENC_K4, APP_ENC_PRESETS, APP_NENC };
enum { APP_PG_WHEEL, APP_PG_OP, APP_PG_ENV, APP_PG_MOTION, APP_PG_FX, APP_PG_HORO, APP_PG_GLOBAL, APP_NPAGE };
enum { APP_T_NOT_SET, APP_T_UNCONFIRMED, APP_T_SET };   /* the clock (owner decision D5) */
#define APP_NSLOT 64u
#define APP_DAYS_MIN (-62091)       /* 1800-01-01 .. 2200-12-31: core/astro's range */
#define APP_DAYS_MAX 84371

typedef struct {                    /* storage: what 0 = settings, 1..8 = the preset banks. 0 = done */
    int (*load)(unsigned what, void *p, uint32_t n);
    int (*save)(unsigned what, const void *p, uint32_t n);
} app_store_t;

typedef struct {                    /* for the tests and the emulator gate: where the instrument is */
    uint32_t page, sub, sel, slot, algo, clock, live, run, edited, nedge, ntransit, latch, saves;
    int32_t sky_days; uint32_t sky_sec;
    int32_t now_days; uint32_t now_sec;
    uint32_t lon[ASTRO_NBODY];      /* the sky chart's tropical longitudes */
    uint32_t asc;
} app_info_t;

void app_init(const app_store_t *st, uint32_t ms);
void app_button(unsigned b, int down);
void app_encoder(unsigned e, int delta);
void app_key(unsigned k, int down);         /* the 27 keys, F3..G5 */
void app_midi(unsigned st, unsigned d1, unsigned d2);
void app_master(unsigned v);                /* the MASTER knob, 0..1023 (the platform applies it: this is for the screen) */
void app_tick(uint32_t ms);                 /* clocks, sky, program, saves: call it every pass of the main loop */
void app_draw(uint16_t *fb);                /* the whole screen, GFX_W x GFX_H */
uint32_t app_leds(void);                    /* button LEDs, bit = button */
uint32_t app_key_leds(void);                /* key LEDs, bit = key */
void app_info(app_info_t *i);
const patch_t *app_patch(void);
#endif
