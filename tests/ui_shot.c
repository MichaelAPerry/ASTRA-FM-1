/* SPDX-License-Identifier: GPL-3.0-only
 * Host harness for the instrument core: runs core/app with a script of panel events and writes the frames it draws
 * as PPM files. For development and for tests/test_app.c; what is shown to anyone is always the real firmware in
 * fm1-emu. Storage is memory.  ui_shot OUTDIR */
#include <stdio.h>
#include <string.h>
#include "core/app/app.h"
#include "core/ui/gfx.h"

static uint16_t fb[GFX_W * GFX_H];
static uint8_t store[9][4096]; static uint32_t store_n[9];
static int st_load(unsigned w, void *p, uint32_t n) { if (w > 8 || store_n[w] != n) return -1; memcpy(p, store[w], n); return 0; }
static int st_save(unsigned w, const void *p, uint32_t n) { if (w > 8 || n > 4096) return -1; memcpy(store[w], p, n); store_n[w] = n; return 0; }
static const app_store_t ST = {st_load, st_save};
static uint32_t ms;
static const char *dir;
static void run(uint32_t dt) {              /* time passes: audio blocks and main-loop passes */
    static int32_t buf[512];
    uint32_t t;
    for (t = 0; t < dt; t += 6) { memset(buf, 0, sizeof buf); fm_render(buf, 256); ms += 6; app_tick(ms); }
}
static void shot(const char *name) {
    char path[256]; FILE *f; int i;
    app_draw(fb);
    snprintf(path, sizeof path, "%s/%s.ppm", dir, name);
    f = fopen(path, "wb"); if (!f) return;
    fprintf(f, "P6 %d %d 255\n", GFX_W, GFX_H);
    for (i = 0; i < GFX_W * GFX_H; i++) { unsigned c = fb[i]; fputc((int)((c >> 11) * 255 / 31), f); fputc((int)(((c >> 5) & 63) * 255 / 63), f); fputc((int)((c & 31) * 255 / 31), f); }
    fclose(f);
}
static void press(unsigned b) { app_button(b, 1); app_button(b, 0); run(30); }
static void turn(unsigned e, int d) { app_encoder(e, d); run(60); }

int main(int argc, char **argv) {
    int i;
    dir = argc > 1 ? argv[1] : ".";
    fm_init(); app_init(&ST, ms); app_master(700); run(100);
    shot("01_wheel_boot");
    /* birth data: 1987-03-14 06:25 UTC+1, 51.5 N 0.1 W */
    press(APP_BTN_GLO); shot("02_now_notset");
    turn(APP_ENC_K1, 26); turn(APP_ENC_K2, 10); turn(APP_ENC_K3, 11); turn(APP_ENC_K4, 2); press(APP_BTN_PLAY); run(1200); shot("03_now_set");
    press(APP_BTN_GLO); turn(APP_ENC_K1, -3); turn(APP_ENC_K2, 2); turn(APP_ENC_K3, 13); turn(APP_ENC_K4, -6); turn(APP_ENC_SELECT, 25); shot("04_birth");
    press(APP_BTN_GLO); for (i = 0; i < 8; i++) turn(APP_ENC_K1, 3); for (i = 0; i < 35; i++) turn(APP_ENC_K1, 1); turn(APP_ENC_K2, -1); turn(APP_ENC_K3, 4); shot("05_place");
    press(APP_BTN_GLO); shot("06_systems");
    press(APP_BTN_REC); run(200);
    press(APP_BTN_HOME); app_key(7, 1); app_key(14, 1); app_key(19, 1); run(400); shot("07_wheel_natal");
    turn(APP_ENC_ALGORITHM, 24); run(300); shot("08_wheel_transit");
    turn(APP_ENC_ALGORITHM, -12); turn(APP_ENC_SELECT, 4); run(300); shot("09_wheel_sky");
    turn(APP_ENC_ALGORITHM, -12);
    press(APP_BTN_EDIT); shot("10_op");
    press(APP_BTN_EDIT); turn(APP_ENC_SELECT, 1); shot("11_op_tone");
    press(APP_BTN_ENV); shot("12_env");
    press(APP_BTN_LFO); turn(APP_ENC_K1, 5); run(3000); shot("13_motion"); turn(APP_ENC_K1, -5);
    press(APP_BTN_FX); shot("14_fx");
    app_key(7, 0); app_key(14, 0); app_key(19, 0);
    press(APP_BTN_SEQ); turn(APP_ENC_SELECT, 1); shot("15_horo_transits");
    press(APP_BTN_SEQ); run(900); shot("16_horo_timeline");
    press(APP_BTN_SEQ); turn(APP_ENC_SELECT, 2); shot("17_horo_natal");
    press(APP_BTN_SEQ); shot("18_horo_moon");
    press(APP_BTN_SAVE); shot("19_save_ask");
    return 0;
}
