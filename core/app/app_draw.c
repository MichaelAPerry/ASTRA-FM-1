/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA FM-1, the screen: every page is drawn fresh each frame into a 240 x 240 frame (the glue sends only what
 * changed): thick line art and filled geometry on a charcoal ground, after a proposal made with Gemini (2026-11-12).
 * Colour carries the values: each body has its hue, each aspect kind its colour, each dial turns through the
 * spectrum as its knob does. Numbers appear only where a date or a place must be read. Integer only. */
#include "app_int.h"

static const uint8_t HUE[ASTRO_NBODY] = {34, 136, 72, 100, 0, 236, 48, 124, 160, 200, 20};
static const uint8_t EL_HUE[4] = {8, 88, 44, 160};

#define WHITE GFX_RGB(255, 255, 255)
#define GREY(v) GFX_RGB(v, v, v)
#define BG_COL GFX_RGB(11, 13, 17)       /* #0B0D11 Deep charcoal canvas */
#define PANEL_COL GFX_RGB(20, 24, 32)    /* #141820 Instrument cards */
#define ACCENT_COL GFX_RGB(38, 45, 58)   /* Hairlines */

static gfx_col_t body_col(unsigned i, unsigned v) { return gfx_hsv(HUE[i], i == ASTRO_MOON ? 70u : 215u, v); }
static gfx_col_t kind_col(unsigned k, unsigned v) {
    static const uint8_t H[5] = {40, 128, 0, 90, 220}, S[5] = {110, 220, 235, 220, 220};
    return gfx_hsv(H[k % 5u], S[k % 5u], v);
}
static gfx_col_t clock_col(void) { return A.clock == APP_T_SET ? GFX_RGB(110, 230, 140) : A.clock == APP_T_UNCONFIRMED ? GFX_RGB(240, 220, 80) : GFX_RGB(255, 160, 50); }

static char *put(char *p, const char *s) { while (*s) *p++ = *s++; return p; }
static char *putn(char *p, uint32_t v, uint32_t digits) {
    char t[10]; uint32_t i = 0; do { t[i++] = (char)('0' + v % 10u); v /= 10u; } while (v || i < digits);
    while (i) *p++ = t[--i];
    return p;
}
static char *put_date(char *p, int32_t days, uint32_t sec, int tz_q, int with_time) {
    int32_t y; uint32_t mo, d, h, mi, s;
    app_local(days, sec, tz_q, &y, &mo, &d, &h, &mi, &s);
    p = putn(p, (uint32_t)y, 4); *p++ = '-'; p = putn(p, mo, 2); *p++ = '-'; p = putn(p, d, 2);
    if (with_time) { *p++ = ' '; p = putn(p, h, 2); *p++ = ':'; p = putn(p, mi, 2); }
    return p;
}
static char *put_zone(char *p, int tz_q) {
    uint32_t a = (uint32_t)(tz_q < 0 ? -tz_q : tz_q);
    p = put(p, "UTC"); *p++ = tz_q < 0 ? '-' : '+'; p = putn(p, a / 4u, 2); *p++ = ':'; return putn(p, a % 4u * 15u, 2);
}
static char *put_deg(char *p, astro_bam_t lon) {
    uint32_t am = astro_sign_arcmin(lon);
    p = putn(p, am / 60u, 2); *p++ = '^'; p = putn(p, am % 60u, 2); *p++ = '\''; return p;
}

static void bar(const char *title) {
    char s[8], *p = s; unsigned i;
    int on = A.msg[0] && A.ms - A.msg_ms < 1800u;
    gfx_fill_rect(0, 0, GFX_W, 14, PANEL_COL);
    gfx_hline(0, 14, GFX_W, ACCENT_COL);
    gfx_text(4, 4, on ? A.msg : title, 1, on ? GFX_RGB(255, 230, 150) : GREY(200));
    *p++ = 'P'; p = putn(p, A.set.slot + 1u, 2); if (A.edited) *p++ = '*'; *p = 0;
    gfx_text(240 - 6 * 4 - 4, 4, s, 1, gfx_hsv(A.set.slot * 4u + 150u, 180, A.edited ? 255 : 200));
    for (i = 0; i < FM_NVOICE; i++) gfx_line_thick(186 + (int)i * 4, 3, 186 + (int)i * 4, i < fm_voices ? 11 : 6, i < fm_voices ? WHITE : ACCENT_COL);
    gfx_fill_circle(178, 7, 3, A.live ? clock_col() : ACCENT_COL);
    gfx_glyph(A.sel, 161, 7, 9, gfx_hsv(HUE[A.sel], 120, 255));
    gfx_line_thick(170, 4, 170, 10, gfx_hsv(A.patch.algo * 7u + 90u, 150, 255));
    gfx_hline(156, 13, 11, gfx_hsv(A.bumps[APP_ENC_SELECT] * 23u + A.tr_sel * 31u + 5u, 200, 200));   /* SELECT's own line */
    gfx_hline(0, 15, A.master * GFX_W / 1023, gfx_hsv(170u - A.master / 8u, 255, 215));
}

static void dials(void) {
    const app_par_t *p = app_pars(); unsigned i;
    gfx_fill_rect(0, 196, GFX_W, 44, PANEL_COL);
    gfx_hline(0, 195, GFX_W, ACCENT_COL);
    for (i = 0; i < 4u; i++) {
        int cx = 30 + 60 * (int)i, cy = 214, x, y, raw = app_par_get(&p[i]);
        unsigned v = app_par_norm(&p[i]), hue = (unsigned)(p[i].kind == PK_X || p[i].hi - p[i].lo > 50 ? raw * 5 : (int)v) + 40u * i + 20u;
        uint32_t av = 0xA0000000u - v * 12632256u;
        gfx_col_t c = gfx_hsv(hue, 230, 255);
        hue += A.bumps[APP_ENC_K1 + i] * 11u; c = gfx_hsv(hue, 230, 255);
        if (A.bump_ms[APP_ENC_K1 + i] && A.ms - A.bump_ms[APP_ENC_K1 + i] < 180u) c = gfx_hsv(hue, 60, 255);
        
        gfx_fill_circle(cx, cy, 14, BG_COL);
        gfx_circle(cx, cy, 14, ACCENT_COL);
        gfx_arc_thick(cx, cy, 12, 3, 0xE0000000u, 0xA0000000u, ACCENT_COL);
        gfx_arc_thick(cx, cy, 12, 3, av, 0xA0000000u, c);
        gfx_polar(cx, cy, 10, av, &x, &y);
        gfx_fill_circle(x, y, 2, WHITE);
        gfx_text_c(cx, 233, p[i].name, 1, gfx_dim(c, 190));
    }
}

typedef struct { int cx, cy, R; astro_bam_t asc; } wheel_t;
static uint32_t wang(const wheel_t *w, astro_bam_t lon) { return 0x80000000u + (lon - w->asc); }
static void wheel_ring(const wheel_t *w, int shake, const chart_t *houses) {
    unsigned j; int x0 = 0, y0 = 0, x, y; uint32_t sw = fm_scope_w;
    gfx_fill_circle(w->cx, w->cy, w->R, PANEL_COL);
    gfx_circle(w->cx, w->cy, w->R, ACCENT_COL);
    for (j = 0; j <= 120u; j++) {
        astro_bam_t lon = (j % 120u) * 35791394u;
        int s = shake ? (fm_scope[2u * ((sw + j * 2u) & (FM_SCOPE_N - 1u))] + fm_scope[2u * ((sw + j * 2u) & (FM_SCOPE_N - 1u)) + 1u]) / 2 : 0;
        int a = s < 0 ? -s : s;
        gfx_polar(w->cx, w->cy, w->R + s * 6 / 32768, wang(w, lon), &x, &y);
        if (j) gfx_line_thick(x0, y0, x, y, gfx_hsv(EL_HUE[chart_element(astro_sign(lon - 1u))], 220, 120u + (unsigned)(a > 8000 ? 135 : a * 135 / 8000)));
        x0 = x; y0 = y;
    }
    for (j = 0; j < 12u; j++) {
        gfx_col_t c = gfx_hsv(EL_HUE[chart_element(j)], 200, 200);
        gfx_polar(w->cx, w->cy, w->R * 9 / 10, wang(w, j * 0x15555555u + 0x0AAAAAABu), &x, &y);
        gfx_glyph(GLYPH_SIGN + j, x, y, w->R / 8 + 2, c);
        gfx_polar(w->cx, w->cy, w->R * 4 / 5, wang(w, j * 0x15555555u), &x0, &y0);
        gfx_polar(w->cx, w->cy, w->R - 2, wang(w, j * 0x15555555u), &x, &y);
        gfx_line(x0, y0, x, y, ACCENT_COL);
    }
    gfx_arc_thick(w->cx, w->cy, w->R * 4 / 5, 2, 0, 0xFFFFFFFFu, ACCENT_COL);
    if (houses) for (j = 0; j < 12u; j++) {
        int ang = j % 3u == 0u && !houses->solar;
        gfx_polar(w->cx, w->cy, w->R * 4 / 5, wang(w, houses->h.cusp[j]), &x0, &y0);
        gfx_polar(w->cx, w->cy, ang ? w->R * 3 / 10 : w->R * 7 / 10, wang(w, houses->h.cusp[j]), &x, &y);
        gfx_line_thick(x0, y0, x, y, ang ? GREY(120) : ACCENT_COL);
    }
}

static void wheel_bodies(const wheel_t *w, const chart_t *c, int r, int size, unsigned flags, unsigned bright) {
    uint8_t ord[ASTRO_NPLANET]; unsigned i, j, in = 0; int x, y;
    for (i = 0; i < ASTRO_NPLANET; i++) { for (j = i; j > 0 && c->lon[ord[j - 1u]] > c->lon[i]; j--) ord[j] = ord[j - 1u]; ord[j] = (uint8_t)i; }
    for (i = 0; i < ASTRO_NPLANET; i++) {
        unsigned b = ord[i], v = bright;
        in = i && c->lon[b] - c->lon[ord[i - 1u]] < 0x07800000u ? in + 1u : 0u;
        gfx_polar(w->cx, w->cy, r - (int)(in % 3u) * (size + 1), wang(w, c->lon[b]), &x, &y);
        if (flags & 1u) v = A.patch.op[b].mute ? 60u : 120u + fm_meter[b] * 135u / 255u;
        gfx_fill_circle(x, y, size / 2 + 2, PANEL_COL); /* OP-1 Knockout mask */
        gfx_glyph(b, x, y, size, body_col(b, v));
        if (c->sky.spd[b] < 0) { gfx_fill_circle(x + size/2, y - size/2, 2, GFX_RGB(255, 80, 80)); gfx_circle(x + size/2, y - size/2, 2, PANEL_COL); }
        if ((flags & 2u) && b == A.sel) gfx_circle(x, y, size / 2 + 3, WHITE);
    }
}

static void wire(int x0, int y0, int x1, int y1, gfx_col_t c, int dashed) {
    int k;
    if (!dashed) gfx_glow(x0, y0, x1, y1, c);
    else for (k = 0; k < 9; k += 2) gfx_line_thick(x0 + (x1 - x0) * k / 9, y0 + (y1 - y0) * k / 9, x0 + (x1 - x0) * (k + 1) / 9, y0 + (y1 - y0) * (k + 1) / 9, c);
    gfx_fill_circle(x1, y1, 2, c);
}

static void wheel_wires(const wheel_t *w, const chart_t *from, const chart_t *to, int ra, int rb) {
    unsigned i;
    for (i = 0; i < A.route.nedge; i++) {
        unsigned s = A.route.edge[i].src, d = A.route.edge[i].dst, v = 70u + A.route.edge[i].depth * 185u / 255u;
        int x0, y0, x1, y1;
        gfx_col_t c = patch_algo_src(A.patch.algo) == ALGO_FREE ? body_col(s, v) : kind_col(A.route.edge[i].kind, v);
        if (s == d) { gfx_polar(w->cx, w->cy, rb, wang(w, to->lon[d]), &x1, &y1); gfx_fill_circle(x1, y1, 4, c); continue; }
        gfx_polar(w->cx, w->cy, ra, wang(w, from->lon[s]), &x0, &y0);
        gfx_polar(w->cx, w->cy, rb, wang(w, to->lon[d]), &x1, &y1);
        wire(x0, y0, x1, y1, c, A.route.edge[i].ring);
    }
    for (i = 0; i < FM_NOP; i++) if (A.route.fb[i] > 8u) {  /* feedback: a loop at the body's own place */
        int x, y;
        gfx_polar(w->cx, w->cy, rb - 6, wang(w, to->lon[i]), &x, &y);
        gfx_circle(x, y, 3, body_col(i, 60u + A.route.fb[i] * 3u / 4u));
    }
}

static const chart_t *ref_chart(void) { return A.has_natal ? &A.natal : &A.sky; }

static void page_wheel(void) {
    static const char *const SRC[4] = {"FREE WIRES", "BIRTH H", "SKY H", "TRANSIT H"};
    char s[40], *p; unsigned src = patch_algo_src(A.patch.algo);
    wheel_t w = {120, 110, 80, 0}; const chart_t *ref = ref_chart(); int x;
    w.asc = ref->h.asc;
    p = put(s, SRC[src]); if (src) p = putn(p, patch_algo_harm(A.patch.algo), 1); *p = 0;
    bar(s);
    *put_date(s, A.sky_days, A.sky_sec, A.set.tz_q, 1) = 0;
    gfx_text(2, 19, s, 1, A.live ? clock_col() : gfx_hsv(190, 160, 255));
    *put_deg(s, ref->lon[A.sel]) = 0;
    x = 240 - 2 - 13 - gfx_text_w(s, 1) - 4;
    gfx_glyph(A.sel, x - 9, 22, 11, body_col(A.sel, 255));
    gfx_text(x, 19, s, 1, body_col(A.sel, 230));
    gfx_glyph(GLYPH_SIGN + astro_sign(ref->lon[A.sel]), 232, 22, 11, gfx_hsv(EL_HUE[chart_element(astro_sign(ref->lon[A.sel]))], 200, 255));
    wheel_ring(&w, 1, ref);
    if (src == ALGO_TRANSIT && A.has_natal) {
        wheel_wires(&w, &A.sky, &A.natal, 46, 30);
        wheel_bodies(&w, &A.sky, 56, 9, 0, 170);
        wheel_bodies(&w, &A.natal, 38, 11, 3, 255);
    } else {
        const chart_t *c = src == ALGO_SKY ? &A.sky : ref;
        wheel_wires(&w, c, c, 40, 40);
        wheel_bodies(&w, c, 53, 12, 3, 255);
    }
    if (!A.has_natal) gfx_text_c(120, 186, "NO BIRTH DATA: GLO", 1, GREY(110));
}

static void strip(int y) {
    unsigned i;
    for (i = 0; i < FM_NOP; i++) {
        int x = 12 + 24 * (int)i;
        gfx_glyph(i, x, y, 13, body_col(i, A.patch.op[i].mute ? 55u : 110u + fm_meter[i] * 145u / 255u));
        if (i == A.sel) { gfx_hline(x - 8, y + 10, 17, body_col(i, 255)); gfx_hline(x - 8, y - 10, 17, body_col(i, 255)); }
    }
}

static int32_t src_val(unsigned s, uint32_t ph) { return (fm_wave(A.prog.wave[s], (ph >> 8) * (A.prog.ratio[s] >> 8)) * (int32_t)A.prog.level[s]) >> 15; }

static void op_wave(int x0, int y0, int w, int h, unsigned op, gfx_col_t c) {
    int32_t prev = 0, fz = 0, s; int x, py = 0;
    gfx_fill_rect(x0, y0, w, h, PANEL_COL);
    gfx_hline(x0, y0 + h/2, w, ACCENT_COL);
    for (x = 0; x < 2 * w; x++) {
        uint32_t ph = (uint32_t)(x % w) * (0xFFFFFFFFu / (uint32_t)w), p = (ph >> 8) * (A.prog.ratio[op] >> 8);
        unsigned k;
        for (k = 0; k < 2u; k++) {
            unsigned sc = A.prog.pms[k][op];
            p += (uint32_t)(sc == op ? prev : src_val(sc, ph)) * (uint32_t)A.prog.pmd[k][op];
        }
        p += (uint32_t)(prev + fz) * (uint32_t)A.prog.fbd[op];
        fz = prev;
        s = fm_wave(A.prog.wave[op], p);
        s = (s * (32768 - (int32_t)A.prog.amd[op] + (((A.prog.ams[op] == op ? prev : src_val(A.prog.ams[op], ph)) * (int32_t)A.prog.amd[op]) >> 15))) >> 15;
        prev = (s * (int32_t)A.prog.level[op]) >> 15;
        if (x >= w) {
            int y = y0 + h / 2 - s * (h / 2 - 1) / 32768;
            if (x > w) { gfx_line_thick(x0 + x - w - 1, py, x0 + x - w, y, c); gfx_vline(x0 + x - w, y > y0 + h/2 ? y0+h/2 : y, y > y0 + h/2 ? y - (y0+h/2) : (y0+h/2) - y, gfx_dim(c, 40)); }
            py = y;
        }
    }
}

static void page_op(void) {
    static const char *const SUB[4] = {"OPERATOR: PITCH AND LEVEL", "OPERATOR: TONE", "OPERATOR: FREE WIRES", "OPERATOR: RING AND KEYS"};
    const chart_t *ref = ref_chart(); const patch_op_t *o = &A.patch.op[A.sel];
    unsigned i, sg = astro_sign(ref->lon[A.sel]); int y, lv; char s[12];
    bar(SUB[A.sub]);
    gfx_fill_circle(34, 54, 26, PANEL_COL);
    gfx_glyph(A.sel, 34, 54, 44, body_col(A.sel, o->mute ? 70u : 150u + fm_meter[A.sel] * 105u / 255u));
    gfx_glyph(GLYPH_SIGN + sg, 16, 88, 13, gfx_hsv(EL_HUE[chart_element(sg)], 200, 255));
    *put_deg(s, ref->lon[A.sel]) = 0; gfx_text(28, 85, s, 1, GREY(150));
    
    op_wave(76, 22, 140, 76, A.sel, body_col(A.sel, 255));
    
    lv = (int)A.route.lvl[A.sel] * 76 / 255;
    gfx_fill_rect(224, 22, 10, 76, PANEL_COL);
    gfx_fill_rect(224, 98 - lv, 10, lv, gfx_hsv(o->level * 2u, 220, 255));
    gfx_hline(220, 98 - (int)o->out * 76 / 127, 18, WHITE);
    
    for (i = 0; i < 3u; i++) {
        unsigned sc = i < 2u ? A.prog.pms[i][A.sel] : A.prog.ams[A.sel], kind = 0, j, v;
        int32_t d = i < 2u ? A.prog.pmd[i][A.sel] / (FM_PM_MAX / 1536) : (int32_t)A.prog.amd[A.sel] / 128;
        gfx_col_t c; y = 112 + 20 * (int)i;
        v = (unsigned)(d < 0 ? -d : d); v = v > 255u ? 255u : v;
        for (j = 0; j < A.route.nedge; j++) if (A.route.edge[j].dst == A.sel && A.route.edge[j].src == sc && A.route.edge[j].ring == (i == 2u)) kind = A.route.edge[j].kind;
        c = patch_algo_src(A.patch.algo) == ALGO_FREE ? body_col(sc, 80u + v * 175u / 255u) : kind_col(kind, 80u + v * 175u / 255u);
        gfx_glyph(sc, 86, y, 13, body_col(sc, v ? 255u : 70u));
        if (v) {
            wire(98, y, 98 + (int)v * 100 / 255, y, c, i == 2u);
            if (d < 0) gfx_text(100, y - 10, "-", 1, c);
            if (patch_algo_src(A.patch.algo) != ALGO_FREE) { gfx_fill_circle(214, y, 7, PANEL_COL); gfx_glyph(GLYPH_ASPECT + kind, 214, y, 11, c); }
        } else gfx_hline(98, y, 100, ACCENT_COL);
        gfx_text(228, y - 3, i < 2u ? "P" : "R", 1, GREY(90));
    }
    if (A.route.fb[A.sel]) gfx_arc_thick(34, 54, 30, 3, 0x20000000u, 0x20000000u + A.route.fb[A.sel] * 0x00C00000u, body_col(A.sel, 255));
    strip(176);
}

static void env_curve(unsigned op, gfx_col_t c, int thick) {
    const patch_op_t *o = &A.patch.op[op];
    int wa = 3 + o->a * 44 / 127, wd = 3 + o->d * 52 / 127, wr = 3 + o->r * 62 / 127, top = 30, base = 158, hs = (base - top) * o->s / 127, k;
    int x = 12, y = base, nx, ny;
    for (k = 1; k <= 8; k++) {
        nx = 12 + wa * k / 8; ny = base - (base - top) * (64 - (8 - k) * (8 - k)) / 64;
        if(thick) { gfx_line_thick(x, y, nx, ny, c); for(int s = x; s < nx; s++) gfx_vline(s, y + (ny-y)*(s-x)/(nx-x), base - (y + (ny-y)*(s-x)/(nx-x)), gfx_dim(c, 50)); } else gfx_line(x, y, nx, ny, c);
        x = nx; y = ny;
    }
    if(thick) gfx_fill_circle(x, y, 3, WHITE);
    for (k = 1; k <= 8; k++) {
        nx = 12 + wa + wd * k / 8; ny = base - hs - (base - top - hs) * (8 - k) * (8 - k) / 64;
        if(thick) { gfx_line_thick(x, y, nx, ny, c); for(int s = x; s < nx; s++) gfx_vline(s, y + (ny-y)*(s-x)/(nx-x), base - (y + (ny-y)*(s-x)/(nx-x)), gfx_dim(c, 50)); } else gfx_line(x, y, nx, ny, c);
        x = nx; y = ny;
    }
    if(thick) gfx_fill_circle(x, y, 3, WHITE);
    nx = x + 40; if(thick) { gfx_hline(x, y, 40, c); gfx_hline(x, y+1, 40, c); gfx_fill_rect(x, y, 40, base - y, gfx_dim(c, 50)); } else gfx_line(x, y, nx, y, c); x = nx;
    for (k = 1; k <= 8; k++) {
        nx = 12 + wa + wd + 40 + wr * k / 8; ny = base - hs * (8 - k) * (8 - k) / 64;
        if(thick) { gfx_line_thick(x, y, nx, ny, c); for(int s = x; s < nx; s++) gfx_vline(s, y + (ny-y)*(s-x)/(nx-x), base - (y + (ny-y)*(s-x)/(nx-x)), gfx_dim(c, 50)); } else gfx_line(x, y, nx, ny, c);
        x = nx; y = ny;
    }
    if(thick) gfx_fill_circle(x, y, 3, WHITE);
}
static void page_env(void) {
    unsigned i; bar("ENVELOPE");
    gfx_hline(8, 159, 224, ACCENT_COL);
    for (i = 0; i < FM_NOP; i++) if (i != A.sel) env_curve(i, body_col(i, 70), 0);
    env_curve(A.sel, body_col(A.sel, 255), 1);
    gfx_glyph(A.sel, 222, 34, 21, body_col(A.sel, 150u + fm_meter[A.sel] * 105u / 255u));
    strip(176);
}

static void page_motion(void) {
    char s[24]; unsigned i, x; int px, py, qx, qy;
    bar(A.sub ? "MOTION: LFO AND BEND" : "MOTION: THE SKY CLOCK");
    *put_date(s, A.sky_days, A.sky_sec, A.set.tz_q, 1) = 0;
    gfx_text(2, 19, s, 1, A.live ? clock_col() : gfx_hsv(190, 160, 255));
    gfx_text(240 - 2 - 6 * 6, 19, A.run ? (A.patch.warp < 0 ? "<< RUN" : ">> RUN") : "  HOLD", 1, A.run ? gfx_hsv((unsigned)(A.patch.warp * 12 + 110), 200, 255) : GREY(120));
    
    for (i = 0; i < FM_NOP; i++) {
        int r = 12 + 6 * (int)patch_rank[i];
        uint32_t a = A.sky.lon[i] + 0x80000000u - ref_chart()->h.asc, tail = (uint32_t)(A.sky.sky.spd[i] < 0 ? -A.sky.sky.spd[i] : A.sky.sky.spd[i]);
        gfx_arc_thick(120, 94, r, 2, 0, 0xFFFFFFFFu, body_col(i, 30u + A.route.lvl[i] / 3u));
        tail = tail * 6u + 0x04000000u; if (tail > 0x30000000u) tail = 0x30000000u;
        if (A.sky.sky.spd[i] < 0) gfx_arc_thick(120, 94, r, 2, a, a + tail, GFX_RGB(255, 70, 70)); else gfx_arc_thick(120, 94, r, 2, a - tail, a, body_col(i, 230));
        gfx_polar(120, 94, r, a, &px, &py);
        gfx_fill_circle(px, py, 5, PANEL_COL); gfx_glyph(i, px, py, 9, body_col(i, 255));
    }
    gfx_polar(120, 94, 72, 0x80000000u, &px, &py); gfx_polar(120, 94, 66, 0x80000000u, &qx, &qy);
    gfx_line_thick(px, py, qx, qy, WHITE);
    
    gfx_fill_rect(0, 166, GFX_W, 28, PANEL_COL); gfx_hline(0, 166, GFX_W, ACCENT_COL);
    for (x = 0; x < 240u; x++) {
        int amp = 1 + (A.patch.lfo_pitch + A.patch.lfo_amp) * 5 / 254;
        int y = 180 - fm_wave(0, x * (A.prog.lfo_inc / 64u + 0x00400000u) + A.ms * (A.prog.lfo_inc / 23u)) * amp / 32768;
        gfx_px(x, y, gfx_hsv(A.patch.lfo_rate * 2u + 60u, 200, 255)); gfx_vline(x, y, 194-y, gfx_dim(gfx_hsv(A.patch.lfo_rate * 2u + 60u, 200, 255), 40));
    }
}

static void page_fx(void) {
    int x, i, h, gap; bar(A.sub ? "EFFECTS: ECHO AND ROOM" : "EFFECTS: CHORUS AND ECHO");
    gfx_fill_rect(6, 26, 228, 28, PANEL_COL);
    for (x = 0; x < 228; x++) {
        uint32_t ph = (uint32_t)x * 0x04000000u + A.ms * 0x00080000u;
        gfx_px(6 + x, 40 - fm_wave(0, ph) * 12 / 32768, gfx_hsv(A.patch.cho_depth * 2u + 100u, 200, 230));
        gfx_px(6 + x, 40 - fm_wave(0, ph + A.patch.cho_depth * 0x00800000u + (uint32_t)(fm_wave(0, A.ms * 0x00030000u) + 32768) * (A.patch.cho_depth * 40u)) * 12 / 32768, gfx_hsv(A.patch.cho_mix * 2u + 10u, 220, 60u + A.patch.cho_mix * 195u / 127u));
    }
    gap = 6 + A.patch.dly_time * 30 / 127; h = 44;
    gfx_hline(6, 116, 228, ACCENT_COL);
    for (i = 0, x = 12; x < 232 && h > 0; i++, x += gap) {
        gfx_col_t c = gfx_hsv(A.patch.dly_time * 2u + (unsigned)i * 9u, 220, i ? 70u + A.patch.dly_mix * 185u / 127u : 255u);
        gfx_fill_rect(x, 116 - h, gap > 6 ? 4 : 2, h, c);
        h = h * (20 + A.patch.dly_fb * 100 / 127) / 128;
    }
    for (i = 0; i < 2 + A.patch.rev_decay / 12; i++) {
        unsigned v = (40u + A.patch.rev_mix * 215u / 127u) * (unsigned)(14 - i) / 14u;
        gfx_arc_thick(120, 186, 10 + i * 9, 2, 0x06000000u, 0x7A000000u, gfx_hsv(20u + A.patch.rev_damp + (unsigned)i * 3u, 200, v));
    }
}

static void horo_moon(void) {
    static const char *const PH[8] = {"NEW MOON", "WAXING CRESCENT", "FIRST QUARTER", "WAXING GIBBOUS", "FULL MOON", "WANING GIBBOUS", "LAST QUARTER", "WANING CRESCENT"};
    const chart_t *c = ref_chart(); unsigned ph = chart_moon_phase(&A.sky), i;
    int k, w = astro_cos_q30((uint32_t)ph << 24) / 23860929; char s[24];
    
    gfx_fill_circle(64, 100, 45, GREY(30)); /* Shaded base */
    for (k = -45; k <= 45; k++) {
        int hw = (int)astro_isqrt((uint32_t)(45 * 45 - k * k));
        int term = hw * w / 45;
        if(ph < 128u) gfx_hline(64 + term, 100 + k, hw - term, GFX_RGB(255, 250, 220));
        else gfx_hline(64 - hw, 100 + k, hw - term, GFX_RGB(255, 250, 220));
    }
    
    gfx_text_c(64, 156, PH[((ph + 16u) >> 5) & 7u], 1, GFX_RGB(255, 240, 200));
    *put_date(s, A.sky_days, A.sky_sec, A.set.tz_q, 1) = 0; gfx_text_c(64, 170, s, 1, GREY(130));
    for (i = 0; i < 3u; i++) {
        unsigned sg = astro_sign(i == 2u ? c->h.asc : c->lon[i]); int y = 46 + 44 * (int)i;
        if (i < 2u) gfx_glyph(i, 150, y, 25, body_col(i, 255)); else gfx_text(140, y - 6, c->solar ? "--" : "AC", 2, GREY(200));
        if (i < 2u || !c->solar) gfx_glyph(GLYPH_SIGN + sg, 196, y, 25, gfx_hsv(EL_HUE[chart_element(sg)], 210, 255));
        if (i < 2u || !c->solar) gfx_text_c(196, y + 16, chart_sign_name[sg], 1, GREY(140));
    }
}
static void horo_transits(void) {
    char s[44]; unsigned i, first = A.tr_sel > 2u ? A.tr_sel - 2u : 0u; const chart_t *nat = ref_chart();
    if (A.ntr > 6u && first > A.ntr - 6u) first = A.ntr - 6u;
    for (i = first; i < A.ntr && i < first + 6u; i++) {
        const chart_asp_t *a = &A.tr[i]; int y = 34 + 16 * (int)(i - first), len = a->str * 118 / 32767;
        gfx_col_t c = kind_col(a->kind, i == A.tr_sel ? 255u : 150u);
        gfx_glyph(a->a, 14, y, 13, body_col(a->a, 255)); gfx_glyph(GLYPH_ASPECT + a->kind, 34, y, 11, c); gfx_glyph(a->b, 54, y, 13, body_col(a->b, 200));
        gfx_hline(70, y, 118, ACCENT_COL); gfx_fill_rect(70, y-1, len, 3, c);
        if (a->applying) { gfx_line_thick(196, y - 4, 202, y, c); gfx_line_thick(196, y + 4, 202, y, c); }
        else { gfx_line_thick(202, y - 4, 196, y, GREY(120)); gfx_line_thick(202, y + 4, 196, y, GREY(120)); }
        if (a->days_q4 < 16 * 100) { *put(putn(s, (uint32_t)(a->days_q4 + 8) / 16u, 1), "D") = 0; gfx_text(208, y - 3, s, 1, GREY(160)); }
        if (i == A.tr_sel) { gfx_fill_rect(2, y - 7, 2, 14, WHITE); gfx_fill_rect(237, y - 7, 2, 14, WHITE); }
    }
    if (A.ntr) for (i = 0; i < 4u; i++) { chart_read_transit(&A.sky, nat, &A.tr[A.tr_sel], i, s, 40); gfx_text_c(120, 134 + 13 * (int)i, s, 1, i == 0u ? WHITE : i == 1u ? kind_col(A.tr[A.tr_sel].kind, 255) : GREY(150)); }
    else gfx_text_c(120, 90, "NOTHING WITHIN THE ORB", 1, GREY(140));
    if (!A.has_natal) gfx_text_c(120, 186, "NO BIRTH DATA: THE SKY TO ITSELF", 1, GREY(120));
}
static void horo_timeline(void) {
    const chart_t *nat = ref_chart(); unsigned i, c; uint32_t orb = (uint32_t)A.set.torb * 11930465u;
    gfx_hline(12, 178, 218, ACCENT_COL);
    for (i = 0; i <= 6u; i++) gfx_vline(12 + (int)i * 36, i == 3u ? 30 : 178, i == 3u ? 152 : 4, i == 3u ? ACCENT_COL : GREY(110));
    for (i = 0; i < A.ntr && i < 7u; i++) {
        const chart_asp_t *a = &A.tr[i]; int px = 0, py = 0, best = 178, bx = 12;
        gfx_col_t col = body_col(a->a, i == A.tr_sel ? 255u : 130u);
        for (c = 0; c < A.tl_fill; c++) {
            int32_t sep = (int32_t)(A.tl_lon[c][a->a] - nat->sky.lon[a->b]), e; int x = 12 + (int)c * 18 / 5, y;
            if (!A.has_natal) sep = (int32_t)(A.tl_lon[c][a->a] - A.tl_lon[c][a->b]);
            e = (int32_t)((uint32_t)(sep < 0 ? -sep : sep) - astro_aspect_angle[a->kind]); e = e < 0 ? -e : e;
            y = 178 - ((uint32_t)e < orb ? (int)(140u - (uint32_t)e / (orb / 140u + 1u)) : 0);
            if (c) { if (i == A.tr_sel) gfx_line_thick(px, py, x, y, col); else gfx_line(px, py, x, y, col); }
            if (y < best) { best = y; bx = x; } px = x; py = y;
        }
        if (best < 170) { gfx_fill_circle(bx - 8, best - 9, 6, BG_COL); gfx_glyph(a->a, bx - 8, best - 9, 9, col); gfx_glyph(GLYPH_ASPECT + a->kind, bx, best - 9, 7, kind_col(a->kind, 220)); gfx_glyph(a->b, bx + 8, best - 9, 9, body_col(a->b, 200)); }
    }
    gfx_text(12, 182, "-30 D", 1, GREY(110)); gfx_text(240 - 12 - 30, 182, "+30 D", 1, GREY(110));
    if (!A.has_natal) gfx_text_c(120, 24, "NO BIRTH DATA: THE SKY TO ITSELF", 1, GREY(120));
}
static void horo_natal(void) {
    const chart_t *c = ref_chart(); char s[40]; unsigned i, n[4] = {0}, x;
    for (i = 0; i < FM_NOP; i++) {
        unsigned sg = astro_sign(c->lon[i]); x = 16 + 23 * (int)i; n[chart_element(sg)]++;
        gfx_glyph(i, x, 36, 15, body_col(i, i == A.sel ? 255u : 150u)); gfx_glyph(GLYPH_SIGN + sg, x, 55, 11, gfx_hsv(EL_HUE[chart_element(sg)], 200, i == A.sel ? 255u : 150u));
        if (i == A.sel) { gfx_fill_rect(x - 10, 26, 20, 2, WHITE); gfx_fill_rect(x - 10, 64, 20, 2, WHITE); }
    }
    for (i = 0; i < 4u; i++) { chart_read_place(c, A.sel, i, s, 40); if (!i && gfx_text_w(s, 2) <= 236) gfx_text_c(120, 72, s, 2, body_col(A.sel, 255)); else gfx_text_c(120, i ? 78 + 13 * (int)i : 76, s, 1, i == 0u ? body_col(A.sel, 255) : i == 1u ? WHITE : GREY(150)); }
    /* the Tarot trumps of Crowley's 777 (1909): the body's and its sign's, their titles quoted */
    gfx_hline(40, 128, 160, ACCENT_COL);
    chart_read_place(c, A.sel, 4, s, 40); gfx_text_c(120, 136, s, 1, GFX_RGB(249, 199, 79));
    chart_read_place(c, A.sel, 5, s, 40); if (s[0]) gfx_text_c(120, 152, s, 1, body_col(A.sel, 230));
    chart_read_place(c, A.sel, 6, s, 40); gfx_text_c(120, 166, s, 1, gfx_hsv(EL_HUE[chart_element(astro_sign(c->lon[A.sel]))], 200, 230));
    if (!A.has_natal) gfx_text_c(120, 182, "NO BIRTH DATA: THE SKY AS IT IS", 1, GREY(120));
}
static void page_horo(void) {
    static const char *const T[4] = {"HOROSCOPE: TRANSITS", "HOROSCOPE: THE MONTH", "HOROSCOPE: BIRTH CHART", "HOROSCOPE: MOON AND SIGNS"}; char s[24]; bar(T[A.sub]);
    if (A.sub < 2u) { *put_date(s, A.sky_days, A.sky_sec, A.set.tz_q, 1) = 0; gfx_text(2, 19, s, 1, A.live ? clock_col() : gfx_hsv(190, 160, 255)); }
    if (A.sub == 0u) horo_transits(); else if (A.sub == 1u) horo_timeline(); else if (A.sub == 2u) horo_natal(); else horo_moon();
}

static void page_global(void) {
    static const char *const T[4] = {"NOW: THE CLOCK", "BIRTH: DATE AND TIME", "BIRTH: PLACE", "CHART SYSTEMS"};
    static const char *const ST[3] = {"NOT SET: A CHART TIME. PLAY SETS IT", "UNCONFIRMED: PLAY IF IT IS RIGHT", "SET AND RUNNING"};
    const chart_birth_t *b = &A.set.birth; char s[40], *p; int32_t y; uint32_t mo, d, h, mi, sc; int x, yy, k; bar(T[A.sub]);
    if (A.sub == 0u) {
        app_local(A.now_days, A.now_sec, A.set.tz_q, &y, &mo, &d, &h, &mi, &sc);
        *put_date(s, A.now_days, A.now_sec, A.set.tz_q, 0) = 0; gfx_text_c(120, 24, s, 3, clock_col());
        p = putn(s, h, 2); *p++ = ':'; p = putn(p, mi, 2); *p++ = ':'; p = putn(p, sc, 2); *p = 0; gfx_text(34, 54, s, 2, gfx_hsv(mi * 4u, 50, 255));
        *put_zone(s, A.set.tz_q) = 0; gfx_text(146, 60, s, 1, GREY(150)); gfx_text_c(120, 78, ST[A.clock], 1, clock_col());
        gfx_fill_circle(120, 140, 44, PANEL_COL); gfx_circle(120, 140, 44, ACCENT_COL);
        for (k = 0; k < 12; k++) { int x2, y2; gfx_polar(120, 140, 44, (uint32_t)k * 0x15555555u, &x, &yy); gfx_polar(120, 140, 39, (uint32_t)k * 0x15555555u, &x2, &y2); gfx_line_thick(x, yy, x2, y2, WHITE); }
        gfx_polar(120, 140, 26, 0x40000000u - (h % 12u * 60u + mi) * 5965232u, &x, &yy); gfx_line_thick(120, 140, x, yy, gfx_hsv(h * 10u, 200, 255));
        gfx_polar(120, 140, 38, 0x40000000u - (mi * 60u + sc) * 1193046u, &x, &yy); gfx_line(120, 140, x, yy, gfx_hsv(mi * 4u + 128u, 200, 255));
        if (A.load_peak) {                                  /* the audio interrupt's load: for the hardware test */
            /* (per mille to angle with small constants and a shift: a folded constant like -3221225 would read as a ROM
             * address to Dinghy's build checks) */
            uint32_t a0 = 0xE0000000u, span = 768u << 22, pk = (A.load_peak * 768u / 1000u) << 22, mn = (A.load_mean * 768u / 1000u) << 22;
            gfx_col_t c = A.load_peak < 500u ? GFX_RGB(110, 230, 140) : A.load_peak < 700u ? GFX_RGB(250, 210, 80) : GFX_RGB(255, 80, 70);
            gfx_arc_thick(204, 150, 22, 3, a0 - span, a0, ACCENT_COL);          /* 0 .. 100 % clockwise from lower left */
            gfx_arc_thick(204, 150, 22, 3, a0 - mn, a0, gfx_dim(c, 150));
            gfx_arc(204, 150, 25, a0 - pk, a0 - pk + 0x01000000u, c);
            /* the 50 % mark */
            gfx_polar(204, 150, 26, a0 - span / 2u, &x, &yy); { int x2, y2; gfx_polar(204, 150, 17, a0 - span / 2u, &x2, &y2); gfx_line(x, yy, x2, y2, WHITE); }
            p = put(s, "AUDIO "); p = putn(p, A.load_peak / 10u, 1); *p++ = '%'; *p = 0;
            gfx_text_c(204, 178, s, 1, c);
            p = put(s, "MEAN "); p = putn(p, A.load_mean / 10u, 1); *p++ = '%'; *p = 0;
            gfx_text_c(204, 188, s, 1, GREY(130));
        }
    } else if (A.sub == 1u) {
        p = putn(s, (uint32_t)b->year, 4); *p++ = '-'; p = putn(p, b->month, 2); *p++ = '-'; p = putn(p, b->day, 2); *p = 0;
        gfx_text_c(120, 24, s, 3, b->set ? GFX_RGB(255, 220, 150) : GREY(110));
        if (b->time_known) { p = putn(s, b->hour, 2); *p++ = ':'; p = putn(p, b->minute, 2); *p++ = ' '; *put_zone(p, b->tz_q) = 0; } else *put(s, "TIME UNKNOWN: NOON") = 0;
        gfx_text_c(120, 54, s, 2, b->set ? gfx_hsv(b->minute * 4u, 50, 255) : GREY(110));
        if (A.has_natal) { wheel_t w = {120, 132, 52, 0}; w.asc = A.natal.h.asc; wheel_ring(&w, 0, &A.natal); wheel_bodies(&w, &A.natal, 33, 9, 0, 255); }
        else gfx_text_c(120, 120, "TURN A KNOB TO ENTER IT", 1, GREY(140));
    } else if (A.sub == 2u) {
        uint32_t la = (uint32_t)(b->lat10 < 0 ? -b->lat10 : b->lat10), lo = (uint32_t)(b->lon10 < 0 ? -b->lon10 : b->lon10);
        p = putn(s, la / 10u, 1); *p++ = '.'; p = putn(p, la % 10u, 1); *p++ = b->lat10 < 0 ? 'S' : 'N'; *p++ = ' '; p = putn(p, lo / 10u, 1); *p++ = '.'; p = putn(p, lo % 10u, 1); *p++ = b->lon10 < 0 ? 'W' : 'E'; *p = 0;
        gfx_text_c(120, 18, s, 2, GFX_RGB(160, 220, 255));
        gfx_fill_circle(120, 104, 56, GFX_RGB(20, 30, 45)); gfx_circle(120, 104, 56, GFX_RGB(60, 110, 200));
        for (k = -60; k <= 60; k += 30) {
            int r = (astro_cos_q30((uint32_t)(k * 11930465)) >> 15) * 56 / 32768, py = 104 - (astro_sin_q30((uint32_t)(k * 11930465)) >> 15) * 56 / 32768;
            gfx_hline(120 - r, py, 2 * r, k ? GFX_RGB(30, 60, 120) : GFX_RGB(50, 100, 180));
        }
        yy = 104 - (astro_sin_q30((uint32_t)(b->lat10 * 1193046)) >> 15) * 56 / 32768;
        for (k = 0; k < 12; k++) {                          /* meridians on the near side, sliding as the longitude turns */
            uint32_t ma = (uint32_t)k * 357913941u - (uint32_t)(b->lon10 * 1193046);
            int32_t off = astro_sin_q30(ma) >> 15;
            int j, px = 0, py = 0;
            if (astro_cos_q30(ma) < 0) continue;
            for (j = -6; j <= 6; j++) {
                int r = (astro_cos_q30((uint32_t)(j * 15 * 11930465)) >> 15) * 56 / 32768;
                x = 120 + r * off / 32768; yy = 104 - (astro_sin_q30((uint32_t)(j * 15 * 11930465)) >> 15) * 56 / 32768;
                if (j > -6) gfx_line(px, py, x, yy, GFX_RGB(30, 60, 120));
                px = x; py = yy;
            }
        }
        yy = 104 - (astro_sin_q30((uint32_t)(b->lat10 * 1193046)) >> 15) * 56 / 32768;
        gfx_fill_circle(120, yy, 3, GFX_RGB(255, 200, 80));
        gfx_circle(120, yy, 6, GFX_RGB(255, 200, 80));
        p = put_zone(s, b->tz_q); p = put(p, b->time_known ? "  TIME KNOWN" : "  TIME UNKNOWN"); *p = 0; gfx_text_c(120, 170, s, 1, WHITE);
        if (!b->time_known) gfx_text_c(120, 182, "NOON, AND HOUSES FROM THE SUN'S SIGN", 1, GREY(120));
    } else {
        wheel_t w = {120, 126, 50, 0}; const chart_t *c = ref_chart(); w.asc = c->h.asc;
        gfx_text_c(120, 18, chart_hsys_name[c->solar ? ASTRO_H_WHOLE : A.set.hsys], 2, gfx_hsv(A.set.hsys * 30u + 20u, 180, 255));
        if (c->solar) gfx_text_c(120, 38, "SOLAR HOUSES UNTIL A BIRTH TIME IS KNOWN", 1, GREY(120));
        else if (c->h.system != A.set.hsys) gfx_text_c(120, 38, "TOO NEAR THE POLE: PORPHYRY", 1, GREY(140));
        wheel_ring(&w, 0, c); wheel_bodies(&w, c, 32, 9, 0, 255);
        p = put(s, A.set.sidereal ? "SIDEREAL LAHIRI  " : "TROPICAL  "); *put_zone(p, A.set.tz_q) = 0; gfx_text_c(120, 184, s, 1, A.set.sidereal ? GFX_RGB(200, 160, 255) : GFX_RGB(160, 255, 200));
    }
}

void app_draw(uint16_t *fb) {
    static void (*const PAGE[APP_NPAGE])(void) = {page_wheel, page_op, page_env, page_motion, page_fx, page_horo, page_global};
    gfx_target(fb);
    gfx_clear(BG_COL);
    PAGE[A.page]();
    dials();
}
