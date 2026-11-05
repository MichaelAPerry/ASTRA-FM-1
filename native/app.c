/* SPDX-License-Identifier: GPL-3.0-only */
/* ASTRA engineering vertical slice. Temporary UI and test signal only. */
#define APP_NAME "ASTRA FM-1"
#include "platform.c"
#include "astro.c"
static volatile uint32_t phase_step, sounding;
static uint32_t phase;
static int32_t amplitude;
static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t i, step = phase_step;
    int32_t target = sounding ? 8000 : 0;
    for (i = 0; i < n; ++i) {
        int32_t x, ax, sample;
        amplitude += (target-amplitude) / 128;
        if (target == 0 && amplitude < 128) amplitude = 0;
        phase += step;
        x = (int32_t)(phase >> 16); if (x >= 32768) x -= 65536;
        ax = x < 0 ? -x : x;
        sample = ((x * (32768-ax)) / 8192 * amplitude) / 32768;
        out[2*i] += sample; out[2*i+1] += sample;
    }
}
static void decimal(char *s, uint32_t n, uint32_t width)
{ while (width) { s[--width] = (char)('0'+n%10); n/=10; } }
static void screen(int32_t day, int32_t lon, uint32_t hz, int play)
{
    char date[] = "DAY +00000 @ 12:00 UTC";
    char angle[] = "SUN 000.000 DEG";
    char freq[] = "TEST TONE 000 HZ";
    date[4] = day < 0 ? '-' : '+'; decimal(date+5,day < 0 ? (uint32_t)-day : (uint32_t)day,5);
    decimal(angle+4,(uint32_t)lon/1000,3); decimal(angle+8,(uint32_t)lon%1000,3);
    decimal(freq+10,hz,3);
    draw_text_box(0,12,240,&FONT_L,"ASTRA FM-1",C_WHITE,1);
    draw_text_box(0,48,240,&FONT_S,"ENGINEERING PROTOTYPE",C_WHITE,1);
    draw_text_box(0,76,240,&FONT_S,"FIXTURE - NOT CURRENT",C_WHITE,1);
    draw_text_box(0,100,240,&FONT_S,date,C_WHITE,1);
    draw_text_box(0,126,240,&FONT_S,angle,C_WHITE,1);
    draw_text_box(0,150,240,&FONT_S,freq,C_WHITE,1);
    draw_text_box(0,176,240,&FONT_S,play ? "PLAYING TEST SIGNAL" : "STOPPED",C_WHITE,1);
    draw_text_box(0,202,240,&FONT_S,"SELECT:DAY  PLAY:TEST",C_WHITE,1);
    draw_text_box(0,222,240,&FONT_S,"DAY 0 = 2000-01-01",C_WHITE,1);
}
static void app_main(void)
{
    int32_t day = 0, last_day = -99999;
    uint32_t previous_buttons = 0, last_play = 2;
    lcd_fill(0,0,240,240,C_BLACK);
    for (;;) {
        int32_t e, lon; uint32_t hz, b, rel;
        platform_poll();
        b = fm1_input_edges(&rel);
        if (b & (1u << BTN_PLAY)) previous_buttons ^= 1u;
        e = fm1_enc_take(ENC_SELECT);
        if (e > 365) e = 365; if (e < -365) e = -365;
        day += e; if (day < -18262) day = -18262; if (day > 18627) day = 18627;
        if (day == last_day && previous_buttons == last_play) continue;
        /* Expensive calculation is in main, never in the audio ISR. */
        lon = astra_sun(day,43200); hz = astra_test_hz(lon);
        phase_step = hz * 97392u; /* rounded 2^32 / 44100; ~3 ppm error */
        sounding = previous_buttons;
        fm1_led_key(BTN_PLAY,sounding);
        screen(day,lon,hz,(int)sounding);
        last_day = day; last_play = previous_buttons;
    }
}
