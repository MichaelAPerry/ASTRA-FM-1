/* SPDX-License-Identifier: GPL-3.0-only */
/* ASTRA FM-1: the firmware. Built on Dinghy (zvenson/dinghy, GPL-3.0-only): native/build.sh copies this file to
 * Dinghy's firmware/src/dinghy.c, so platform.c (updates, USB, panel, screen, storage) is Dinghy's, unchanged.
 *
 * Everything the instrument is lives in core/ (integer only, the same sources the host tests build):
 *   core/astro   the sky: Sun, Moon, planets, houses, aspects      core/chart   charts, transits, the reading
 *   core/fm      the engine: 5 voices of 10 operators              core/synth   chart -> patch -> program
 *   core/ui      line art                                          core/app     the instrument: panel, clocks, pages
 * This file is the wiring: the audio interrupt, the panel and MIDI into core/app, the frame to the screen, flash.
 *
 * The screen: core/app draws a whole 240 x 240 frame; only the rows that changed since the last one are sent
 * (a full frame takes 77 ms on the wire at 12 MHz; a moving figure is a fraction of it). Nothing waits for it:
 * a transfer runs by DMA while the main loop goes on, and the next frame is drawn when the last has left. */
#define APP_NAME "ASTRA"
#include "platform.c"

#define GFX_SWAP                                /* the frame in the panel's byte order */
#define FM_HOT __attribute__((section(".ram_text")))   /* the per-sample loop runs from RAM: no flash-cache misses */
#define FM_FAR(fn) FL_FAR(fn)
#define FM_POOL __attribute__((section(".pool")))
#include "core/astro/astro.c"
#include "core/astro/sky.c"
#include "core/fm/fm.c"
#include "core/chart/chart.c"
#include "core/synth/patch.c"
#include "core/ui/gfx.c"
#include "core/app/app.c"
#include "core/app/app_draw.c"

/* ---- the audio interrupt: the engine, and how long it took */
static volatile uint32_t isr_max, isr_sum, isr_n;
static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t t0 = fm1_ticks(), t;
    fm_render(out, n);
    t = fm1_ticks() - t0;
    if (t > isr_max) isr_max = t;
    isr_sum += t; isr_n++;
}

/* what the emulator gate reads back (tools/emu_smoke.sh, --dump): where the instrument is, what it computed, what
 * the interrupt costs. Not static, so build.sh finds its address. Written only by the main loop. */
volatile struct {
    uint32_t magic, page, sub, sel, slot, algo, clock, live, run, edited, nedge, ntransit, latch, saves;
    int32_t sky_days; uint32_t sky_sec; int32_t now_days; uint32_t now_sec;
    uint32_t lon[ASTRO_NBODY], asc;
    uint32_t voices, isr_max_us, isr_mean_us, frames, rows;
} astra_probe;

/* ---- storage: settings, and eight blobs for the eight preset banks */
static int st_load(unsigned what, void *p, uint32_t n) { return what ? blob_load(what - 1u, p, n) : settings_load(p, n); }
static int st_save(unsigned what, const void *p, uint32_t n) { return what ? blob_save(what - 1u, p, n) : settings_save(p, n); }
static const app_store_t STORE = {st_load, st_save};

/* ---- the screen */
#define SEND_ROWS 120u                          /* rows per transfer: 57,600 bytes, as much as Dinghy's own canvas sends */
static uint16_t frame[GFX_W * GFX_H] __attribute__((section(".pool")));
static uint32_t row_sum[GFX_H];
static uint8_t row_dirty[GFX_H];
static uint32_t send_y;                         /* next row to look at; GFX_H: the frame has left */

static void frame_draw(void)                    /* a new frame, and which rows differ from the screen's */
{
    uint32_t y, x, rows = 0;
    app_draw(frame);
    for (y = 0; y < GFX_H; y++) {
        const uint32_t *p = (const uint32_t *)(const void *)&frame[y * GFX_W];
        uint32_t h = 0;
        for (x = 0; x < GFX_W / 2; x++) h = (h << 5 | h >> 27) + (h ^ p[x]);
        if (h != row_sum[y]) { row_sum[y] = h; row_dirty[y] = 1; rows++; }
    }
    astra_probe.frames++; astra_probe.rows += rows;
    send_y = 0;
}
static void frame_send(void)                    /* the next run of changed rows, if the last transfer is done */
{
    uint32_t y0, n;
    if (lcd_busy && !fm1_lcd_done()) return;
    while (send_y < GFX_H && !row_dirty[send_y]) send_y++;
    if (send_y >= GFX_H) return;
    y0 = send_y;
    for (n = 0; n < SEND_ROWS && send_y < GFX_H && row_dirty[send_y]; n++) row_dirty[send_y++] = 0;
    lcd_blit(0, y0, GFX_W, n, &frame[y0 * GFX_W]);
}

static void app_main(void)
{
    uint32_t keys_was = 0, leds_was = ~0u, kleds_was = ~0u, i, load_ms = 0, load_sum = 0, load_n = 0, load_mean = 0;
    fm_init();
    app_init(&STORE, fm1_ms);
    send_y = GFX_H;
    for (;;) {
        uint32_t rel, b, keys, tap, l;
        uint8_t st, d1, d2;
        app_info_t info;
        platform_poll();
        /* the panel and MIDI in */
        b = fm1_input_edges(&rel);
        for (i = 0; i < APP_NBTN; i++) {
            if (b >> i & 1u) app_button(i, 1);
            if (rel >> i & 1u) app_button(i, 0);
        }
        for (i = 0; i < APP_NENC; i++) { int32_t d = fm1_enc_take(i); if (d) app_encoder(i, d); }
        keys = fm1_in.notes; tap = fm1_input_note_edges() & ~keys_was & ~keys;   /* tap: down and up within one pass */
        for (i = 0; i < 27u; i++) {
            if (((keys & ~keys_was) | tap) >> i & 1u) app_key(i, 1);
            if (((keys_was & ~keys) | tap) >> i & 1u) app_key(i, 0);
        }
        keys_was = keys;
        while (midi_get(&st, &d1, &d2)) app_midi(st, d1, d2);
        app_master(astro_isqrt((uint32_t)master_q12 << 8));
        if (fm1_ms - load_ms >= 1000u) {       /* the load meter: the peak since boot, the mean of the last second */
            uint32_t s = isr_sum, n = isr_n;
            load_mean = n != load_n ? (s - load_sum) / (n - load_n) : 0u;
            load_sum = s; load_n = n; load_ms = fm1_ms;
            app_load(isr_max * 10u / (5805u * FM1_TICKS_PER_US / 100u), load_mean * 10u / (5805u * FM1_TICKS_PER_US / 100u));
        }
        app_tick(fm1_ms);
        /* LEDs, only what changed */
        l = app_leds();
        for (i = 0; i < APP_NBTN; i++) if ((l ^ leds_was) >> i & 1u) fm1_led_key(i, (int)(l >> i & 1u));
        leds_was = l;
        l = app_key_leds();
        for (i = 0; i < 27u; i++) if ((l ^ kleds_was) >> i & 1u) fm1_led_key(KEY_LED(i), (int)(l >> i & 1u));
        kleds_was = l;
        /* the screen */
        if (send_y >= GFX_H && (!lcd_busy || fm1_lcd_done())) frame_draw();
        frame_send();
        /* the probe */
        app_info(&info);
        astra_probe.page = info.page; astra_probe.sub = info.sub; astra_probe.sel = info.sel; astra_probe.slot = info.slot;
        astra_probe.algo = info.algo; astra_probe.clock = info.clock; astra_probe.live = info.live; astra_probe.run = info.run;
        astra_probe.edited = info.edited; astra_probe.nedge = info.nedge; astra_probe.ntransit = info.ntransit;
        astra_probe.latch = info.latch; astra_probe.saves = info.saves;
        astra_probe.sky_days = info.sky_days; astra_probe.sky_sec = info.sky_sec; astra_probe.now_days = info.now_days; astra_probe.now_sec = info.now_sec;
        for (i = 0; i < ASTRO_NBODY; i++) astra_probe.lon[i] = info.lon[i];
        astra_probe.asc = info.asc; astra_probe.voices = fm_voices;
        astra_probe.isr_max_us = isr_max / FM1_TICKS_PER_US;
        if (isr_n) astra_probe.isr_mean_us = isr_sum / isr_n / FM1_TICKS_PER_US;
        astra_probe.magic = 0x50525432u;        /* "2TRP": the fields above are filled */
    }
}
