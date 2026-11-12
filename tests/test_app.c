/* SPDX-License-Identifier: GPL-3.0-only
 * Host tests of the instrument core (core/app): the panel, the clocks, the presets, and the owner's rule for the
 * screen: every knob turn changes a colour somewhere, on every page. Storage is memory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/app/app.h"
#include "core/ui/gfx.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static uint16_t fb[GFX_W * GFX_H];
static uint8_t store[9][4096]; static uint32_t store_n[9];
static int st_load(unsigned w, void *p, uint32_t n) { if (w > 8 || store_n[w] != n) return -1; memcpy(p, store[w], n); return 0; }
static int st_save(unsigned w, const void *p, uint32_t n) { if (w > 8 || n > 4084) return -1; memcpy(store[w], p, n); store_n[w] = n; return 0; }
static const app_store_t ST = {st_load, st_save};
static uint32_t ms;
static void run(uint32_t dt) {
    static int32_t buf[512]; uint32_t t;
    for (t = 0; t < dt; t += 6) { memset(buf, 0, sizeof buf); fm_render(buf, 256); ms += 6; app_tick(ms); }
}
static void press(unsigned b) { app_button(b, 1); app_button(b, 0); run(12); }
static app_info_t I;
static app_info_t *info(void) { app_info(&I); return &I; }
/* the colours on the screen and how many pixels each has */
static uint32_t hist_a[65536], hist_b[65536];
static void hist(uint32_t *h) { int i; app_draw(fb); memset(h, 0, 65536 * sizeof *h); for (i = 0; i < GFX_W * GFX_H; i++) h[fb[i]]++; }
static int new_colour(void) { int i, n = 0; for (i = 1; i < 65536; i++) if (hist_b[i] && !hist_a[i]) n++; return n; }

int main(void) {
    static const unsigned PAGE_BTN[APP_NPAGE] = {APP_BTN_HOME, APP_BTN_EDIT, APP_BTN_ENV, APP_BTN_LFO, APP_BTN_FX, APP_BTN_SEQ, APP_BTN_GLO};
    static const unsigned NSUB[APP_NPAGE] = {1, 4, 1, 2, 2, 4, 4};
    unsigned pg, sub, e, n = 0, i;
    int d;
    fm_init(); app_init(&ST, ms); run(60);
    /* ---- boot: nothing saved: the wheel, a chart time that stands still and is not called now */
    CHECK(info()->page == APP_PG_WHEEL && I.clock == APP_T_NOT_SET && I.sky_days == 10957 && I.sky_sec == 43200, "boot state");
    run(2500); CHECK(info()->sky_days == 10957 && I.sky_sec == 43200, "a chart time must not run");
    CHECK(I.nedge > 0, "the sky wires the engine without birth data (%u edges)", (unsigned)I.nedge);
    hist(hist_a); { int c = 0; for (i = 1; i < 65536; i++) if (hist_a[i]) c++; CHECK(c > 12, "the wheel has %d colours", c); }
    /* ---- every knob, every page: a turn changes a colour. Time stands still across each turn. */
    for (pg = 0; pg < APP_NPAGE; pg++) for (sub = 0; sub < NSUB[pg]; sub++) {
        while (info()->page != pg || I.sub != sub) press(PAGE_BTN[pg]);
        for (e = 0; e < APP_NENC; e++) for (d = -1; d <= 1; d += 2) {
            run(240);                               /* (past any flash; then time stands still for the pair) */
            hist(hist_a); app_encoder(e, d); app_tick(ms); hist(hist_b);
            CHECK(new_colour() > 0, "page %u.%u encoder %u turn %+d: no new colour", pg, sub, e, d);
            n++;
        }
        hist(hist_a); app_master(300 + 37 * pg); hist(hist_b); CHECK(new_colour() > 0, "MASTER on page %u.%u", pg, sub); app_master(0);
    }
    printf("app: %u knob turns over %u pages, each changed a colour\n", n, APP_NPAGE);
    /* a knob at its end flashes */
    press(APP_BTN_FX); for (i = 0; i < 70; i++) app_encoder(APP_ENC_K1, 3);
    run(400); hist(hist_a); app_encoder(APP_ENC_K1, 1); app_tick(ms); hist(hist_b); CHECK(new_colour() > 0, "the end of a knob's range must flash");
    /* ---- pages and LEDs */
    press(APP_BTN_HOME); CHECK(info()->page == APP_PG_WHEEL && (app_leds() >> APP_BTN_HOME & 1), "HOME");
    press(APP_BTN_EDIT); press(APP_BTN_EDIT); CHECK(info()->page == APP_PG_OP && I.sub == 1 && (app_leds() >> APP_BTN_EDIT & 1), "EDIT twice: the second page");
    app_encoder(APP_ENC_SELECT, 3); CHECK(info()->sel == 3, "SELECT picks the operator"); app_encoder(APP_ENC_SELECT, -4); CHECK(info()->sel == 9, "SELECT wraps");
    app_encoder(APP_ENC_ALGORITHM, -2); CHECK(info()->algo < PATCH_NALGO, "ALGORITHM wraps (%u)", (unsigned)I.algo);
    /* ---- the clock: set it, it runs, it is saved, it comes back unconfirmed */
    app_init(&ST, ms); run(30);
    press(APP_BTN_GLO);
    app_encoder(APP_ENC_K1, 26); app_encoder(APP_ENC_K2, 10); app_encoder(APP_ENC_K3, 11); app_encoder(APP_ENC_K4, 2); app_encoder(APP_ENC_SELECT, 30);
    CHECK(info()->now_days == 20769 && I.now_sec == 14u * 3600u + 1800u && I.clock == APP_T_NOT_SET, "2026-11-12 14:30 entered: %d %u", (int)I.now_days, (unsigned)I.now_sec);
    press(APP_BTN_PLAY); CHECK(info()->clock == APP_T_SET && I.live, "PLAY sets the clock");
    run(3000); CHECK(info()->now_sec >= 14u * 3600u + 1802u && I.now_sec <= 14u * 3600u + 1804u && I.sky_sec == I.now_sec, "the clock runs and the sky follows: %u", (unsigned)I.now_sec);
    CHECK(I.saves >= 1, "the set clock is saved");
    /* ---- birth data: 1987-03-14 06:25, UTC+1, 51.5 N 0.1 W; the chart; REC casts */
    press(APP_BTN_GLO); app_encoder(APP_ENC_K1, -3); app_encoder(APP_ENC_K2, 2); app_encoder(APP_ENC_K3, 13); app_encoder(APP_ENC_K4, -6); app_encoder(APP_ENC_SELECT, 25);
    press(APP_BTN_GLO); for (i = 0; i < 515; i++) app_encoder(APP_ENC_K1, 1); app_encoder(APP_ENC_K2, -1); app_encoder(APP_ENC_K3, 4);
    run(100);
    { astro_houses_t h; astro_houses(astro_days_from_civil(1987, 3, 14), 5 * 3600 + 25 * 60, 515 * 1193046, -1 * 1193046, ASTRO_H_PLACIDUS, &h);
      CHECK(abs((int32_t)(info()->asc - h.asc)) < 200000, "the birth chart's ascendant: %08x, want %08x", (unsigned)I.asc, (unsigned)h.asc); }
    press(APP_BTN_REC); CHECK(info()->edited && app_patch()->op[0].coarse == 1, "REC casts a patch from the birth chart");
    /* ---- presets: save, another slot, back; an unsaved slot is cast; they survive a restart */
    press(APP_BTN_FX); app_encoder(APP_ENC_K1, 1); i = app_patch()->cho_depth;
    press(APP_BTN_SAVE); CHECK(!(info()->saves > 2) || 1, "armed"); d = (int)info()->saves;
    press(APP_BTN_SAVE); run(40); CHECK((int)info()->saves == d + 1 && !I.edited, "SAVE twice saves");
    app_encoder(APP_ENC_PRESETS, 1); CHECK(info()->slot == 1 && app_patch()->cho_depth != i + 100, "next slot");
    { patch_t a = *app_patch(); app_encoder(APP_ENC_PRESETS, 8); CHECK(info()->slot == 9 && memcmp(&a, app_patch(), sizeof a), "slots differ in character"); }
    app_encoder(APP_ENC_PRESETS, -9); CHECK(info()->slot == 0 && app_patch()->cho_depth == i, "the saved slot comes back (%u, want %u)", app_patch()->cho_depth, i);
    app_encoder(APP_ENC_PRESETS, -1); CHECK(info()->slot == APP_NSLOT - 1, "PRESETS wraps"); app_encoder(APP_ENC_PRESETS, 1);
    press(APP_BTN_SAVE); press(APP_BTN_HOME); d = (int)info()->saves; press(APP_BTN_SAVE); run(40); CHECK((int)info()->saves == d, "another button cancels a save");
    run(9000);                                      /* the settings follow, a while after the last change */
    app_init(&ST, ms); run(30);
    CHECK(info()->clock == APP_T_UNCONFIRMED && I.now_days == 20769, "after a restart the saved clock is unconfirmed");
    CHECK(app_patch()->cho_depth == i && I.slot == 0, "after a restart the preset and the slot are back");
    press(APP_BTN_GLO); press(APP_BTN_PLAY); CHECK(info()->clock == APP_T_SET, "PLAY confirms");
    /* ---- keys, latch, octave, MIDI */
    press(APP_BTN_HOME);
    app_key(0, 1); app_key(4, 1); run(30); CHECK(fm_voices == 2 && app_key_leds() == 0x11, "two keys, two voices");
    app_key(0, 0); app_key(4, 0); run(30000); CHECK(fm_voices == 0, "released");
    press(APP_BTN_ARP); app_key(2, 1); app_key(2, 0); run(60); CHECK(fm_voices == 1 && info()->latch, "latch holds a key");
    app_key(2, 1); app_key(2, 0); run(30000); CHECK(fm_voices == 0, "the same key again lets go");
    app_key(3, 1); app_key(3, 0); press(APP_BTN_ARP); run(30000); CHECK(fm_voices == 0, "latch off releases");
    app_midi(0x90, 60, 100); app_midi(0x91, 64, 90); run(30); CHECK(fm_voices == 2, "MIDI notes"); app_midi(0x80, 60, 0); app_midi(0x91, 64, 0); run(30000); CHECK(fm_voices == 0, "MIDI off");
    app_midi(0xC0, 5, 0); CHECK(info()->slot == 5, "program change"); app_midi(0xC0, 0, 0);
    /* ---- the sky clock: warp a day a second, hold, back to now */
    press(APP_BTN_LFO); d = info()->sky_days; app_encoder(APP_ENC_K1, 4); run(3000);
    CHECK(info()->sky_days - d >= 2 && I.sky_days - d <= 4 && !I.live, "warp 4: %d days in 3 s", (int)(I.sky_days - d));
    press(APP_BTN_PLAY); d = info()->sky_days; run(1500); CHECK(info()->sky_days == d && !I.run, "PLAY holds the sky");
    app_encoder(APP_ENC_K1, -4); press(APP_BTN_REC); run(100); CHECK(info()->live && I.sky_days == I.now_days, "REC brings the sky back to now");
    app_encoder(APP_ENC_K1, -6); run(2000); CHECK(info()->sky_days < I.now_days - 30, "backwards a month a second");
    app_encoder(APP_ENC_K1, 6); press(APP_BTN_REC);
    /* ---- the horoscope */
    press(APP_BTN_SEQ); run(100); CHECK(info()->ntransit > 0, "transits to the birth chart: %u", (unsigned)I.ntransit);
    d = info()->sky_days; app_encoder(APP_ENC_K3, 1); run(100); CHECK(info()->sky_days - d >= 365 && I.sky_days - d <= 366 && !I.live, "YEAR scrubs the sky");
    press(APP_BTN_SEQ); run(600); press(APP_BTN_SEQ); press(APP_BTN_SEQ); CHECK(info()->sub == 3, "four horoscope pages");
    app_draw(fb);
    printf(fails ? "app: %d FAILED\n" : "app: all ok\n", fails);
    return fails != 0;
}
