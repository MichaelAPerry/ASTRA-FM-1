/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA line art (see gfx.h): thick strokes, filled discs and panels on a charcoal ground. Integer only.
 * Visual style after a proposal made with Gemini (2026-11-12), merged and fixed here. */
#include "gfx.h"
#include "core/astro/astro.h"

static uint16_t *fb;
void gfx_target(uint16_t *f) { fb = f; }
void gfx_clear(gfx_col_t c) { uint32_t i; for (i = 0; i < (uint32_t)GFX_W * GFX_H; i++) fb[i] = c; }

gfx_col_t gfx_hsv(unsigned h, unsigned s, unsigned v) {
    unsigned r6 = (h & 255u) * 6u, sec = r6 >> 8, f = r6 & 255u, p = v * (255u - s) / 255u;
    unsigned q = v * (255u - s * f / 255u) / 255u, t = v * (255u - s * (255u - f) / 255u) / 255u, r, g, b;
    switch (sec) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q;
    }
    return GFX_RGB(r, g, b);
}
gfx_col_t gfx_dim(gfx_col_t c, unsigned k) {
    unsigned n = GFX_PX(c), r = (n >> 11) * k >> 8, g = ((n >> 5) & 63u) * k >> 8, b = (n & 31u) * k >> 8;
    return GFX_PX(r << 11 | g << 5 | b);
}
gfx_col_t gfx_blend(gfx_col_t fg, gfx_col_t bg, unsigned alpha) {
    unsigned inv = 255u - alpha, f = GFX_PX(fg), b = GFX_PX(bg);
    unsigned fr = f >> 11, fg_g = (f >> 5) & 63u, fb_b = f & 31u;
    unsigned br = b >> 11, bg_g = (b >> 5) & 63u, bb_b = b & 31u;
    unsigned nr = (fr * alpha + br * inv) >> 8;
    unsigned ng = (fg_g * alpha + bg_g * inv) >> 8;
    unsigned nb = (fb_b * alpha + bb_b * inv) >> 8;
    return GFX_PX(nr << 11 | ng << 5 | nb);
}

void gfx_px(int x, int y, gfx_col_t c) { if ((unsigned)x < GFX_W && (unsigned)y < GFX_H) fb[y * GFX_W + x] = c; }
void gfx_add(int x, int y, gfx_col_t c) {
    unsigned a, n, r, g, b;
    if ((unsigned)x >= GFX_W || (unsigned)y >= GFX_H) return;
    a = GFX_PX(fb[y * GFX_W + x]); n = GFX_PX(c);
    r = (a >> 11) + (n >> 11); g = ((a >> 5) & 63u) + ((n >> 5) & 63u); b = (a & 31u) + (n & 31u);
    if (r > 31u) r = 31u;
    if (g > 63u) g = 63u;
    if (b > 31u) b = 31u;
    fb[y * GFX_W + x] = GFX_PX(r << 11 | g << 5 | b);
}

static void line(int x0, int y0, int x1, int y1, gfx_col_t c, int add, int thick) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, guard = 1200;
    if ((x0 < 0 && x1 < 0) || (y0 < 0 && y1 < 0) || (x0 >= GFX_W && x1 >= GFX_W) || (y0 >= GFX_H && y1 >= GFX_H)) return;
    while (guard--) {
        int e2 = 2 * err;
        if (add) { gfx_add(x0, y0, c); if (thick) { gfx_add(x0+1, y0, c); gfx_add(x0, y0+1, c); } }
        else { gfx_px(x0, y0, c); if (thick) { gfx_px(x0+1, y0, c); gfx_px(x0, y0+1, c); gfx_px(x0+1, y0+1, c); } }
        if (x0 == x1 && y0 == y1) break;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void gfx_line(int x0, int y0, int x1, int y1, gfx_col_t c) { line(x0, y0, x1, y1, c, 0, 0); }
void gfx_line_thick(int x0, int y0, int x1, int y1, gfx_col_t c) { line(x0, y0, x1, y1, c, 0, 1); }
void gfx_glow(int x0, int y0, int x1, int y1, gfx_col_t c) { line(x0, y0, x1, y1, c, 1, 1); }

void gfx_hline(int x, int y, int w, gfx_col_t c) { while (w-- > 0) gfx_px(x++, y, c); }
void gfx_vline(int x, int y, int h, gfx_col_t c) { while (h-- > 0) gfx_px(x, y++, c); }
void gfx_fill_rect(int x, int y, int w, int h, gfx_col_t c) { int i; for (i = 0; i < h; i++) gfx_hline(x, y + i, w, c); }

void gfx_circle(int cx, int cy, int r, gfx_col_t c) {
    int x = r, y = 0, e = 1 - r;
    if (r <= 0) { gfx_px(cx, cy, c); return; }
    while (x >= y) {
        gfx_px(cx + x, cy + y, c); gfx_px(cx - x, cy + y, c); gfx_px(cx + x, cy - y, c); gfx_px(cx - x, cy - y, c);
        gfx_px(cx + y, cy + x, c); gfx_px(cx - y, cy + x, c); gfx_px(cx + y, cy - x, c); gfx_px(cx - y, cy - x, c);
        y++; if (e < 0) e += 2 * y + 1; else { x--; e += 2 * (y - x) + 1; }
    }
}
void gfx_fill_circle(int cx, int cy, int r, gfx_col_t c) {
    int x = r, y = 0, e = 1 - r;
    while (x >= y) {
        gfx_hline(cx - x, cy + y, 2 * x + 1, c); gfx_hline(cx - x, cy - y, 2 * x + 1, c);
        gfx_hline(cx - y, cy + x, 2 * y + 1, c); gfx_hline(cx - y, cy - x, 2 * y + 1, c);
        y++; if (e < 0) e += 2 * y + 1; else { x--; e += 2 * (y - x) + 1; }
    }
}
void gfx_polar(int cx, int cy, int r, uint32_t a, int *x, int *y) {
    *x = cx + ((astro_cos_q30(a) >> 14) * r + (r < 0 ? -32768 : 32768)) / 65536;
    *y = cy - ((astro_sin_q30(a) >> 14) * r + (r < 0 ? -32768 : 32768)) / 65536;
}
void gfx_arc(int cx, int cy, int r, uint32_t a0, uint32_t a1, gfx_col_t c) {
    uint32_t span = a1 - a0, n = 2u + (uint32_t)r * (span >> 22) / 300u, i; /* Smoother subdivision */
    int x0, y0, x1, y1;
    if (n > 128u) n = 128u;
    gfx_polar(cx, cy, r, a0, &x0, &y0);
    for (i = 1; i <= n; i++) {
        gfx_polar(cx, cy, r, a0 + (span / n) * i, &x1, &y1);
        gfx_line_thick(x0, y0, x1, y1, c); /* Bold OP-1 style arcs */
        x0 = x1; y0 = y1;
    }
}
void gfx_arc_thick(int cx, int cy, int r, int thick, uint32_t a0, uint32_t a1, gfx_col_t c) {
    int k; for(k = 0; k < thick; k++) gfx_arc(cx, cy, r - k, a0, a1, c);
}

#define P(x, y) ((x) << 4 | (y))
#define UP 0xF0
#define CIR 0xF1
#define ARC 0xF2
#define FCIR 0xF3
#define END 0xFF
/* FONT & SYM arrays remain mostly unchanged, but we add FCIR for OP-1 filled symbols */
static const uint8_t FONT[] = {
    END, P(2,0),P(2,4),UP,P(2,6),P(2,6),END, P(2,0),P(2,1),END, P(2,1),P(2,5),UP,P(0,2),P(4,4),UP,P(0,4),P(4,2),END,
    P(0,3),P(4,3),UP,P(2,1),P(2,5),END, P(0,3),P(4,3),END, P(2,6),P(2,6),END, P(0,6),P(4,0),END,
    P(1,0),P(3,0),P(4,1),P(4,5),P(3,6),P(1,6),P(0,5),P(0,1),P(1,0),UP,P(1,4),P(3,2),END, P(1,1),P(2,0),P(2,6),UP,P(1,6),P(3,6),END,
    P(0,1),P(1,0),P(3,0),P(4,1),P(4,2),P(0,6),P(4,6),END, P(0,1),P(1,0),P(3,0),P(4,1),P(4,2),P(3,3),P(1,3),UP,P(3,3),P(4,4),P(4,5),P(3,6),P(1,6),P(0,5),END,
    P(3,6),P(3,0),P(0,4),P(4,4),END, P(4,0),P(0,0),P(0,3),P(3,3),P(4,4),P(4,5),P(3,6),P(1,6),P(0,5),END,
    P(4,1),P(3,0),P(1,0),P(0,1),P(0,5),P(1,6),P(3,6),P(4,5),P(4,4),P(3,3),P(0,3),END, P(0,0),P(4,0),P(1,6),END,
    P(1,3),P(0,2),P(0,1),P(1,0),P(3,0),P(4,1),P(4,2),P(3,3),P(1,3),P(0,4),P(0,5),P(1,6),P(3,6),P(4,5),P(4,4),P(3,3),END,
    P(4,3),P(1,3),P(0,2),P(0,1),P(1,0),P(3,0),P(4,1),P(4,5),P(3,6),P(1,6),P(0,5),END, P(2,2),P(2,2),UP,P(2,5),P(2,5),END,
    P(3,1),P(1,3),P(3,5),END, P(0,2),P(4,2),UP,P(0,4),P(4,4),END, P(1,1),P(3,3),P(1,5),END,
    P(0,1),P(1,0),P(3,0),P(4,1),P(4,2),P(2,4),UP,P(2,6),P(2,6),END, P(0,6),P(0,2),P(2,0),P(4,2),P(4,6),UP,P(0,4),P(4,4),END,
    P(0,3),P(3,3),P(4,2),P(4,1),P(3,0),P(0,0),P(0,6),P(3,6),P(4,5),P(4,4),P(3,3),END, P(4,1),P(3,0),P(1,0),P(0,1),P(0,5),P(1,6),P(3,6),P(4,5),END,
    P(0,0),P(0,6),P(2,6),P(4,4),P(4,2),P(2,0),P(0,0),END, P(4,0),P(0,0),P(0,6),P(4,6),UP,P(0,3),P(3,3),END,
    P(4,0),P(0,0),P(0,6),UP,P(0,3),P(3,3),END, P(4,1),P(3,0),P(1,0),P(0,1),P(0,5),P(1,6),P(3,6),P(4,5),P(4,3),P(2,3),END,
    P(0,0),P(0,6),UP,P(4,0),P(4,6),UP,P(0,3),P(4,3),END, P(1,0),P(3,0),UP,P(2,0),P(2,6),UP,P(1,6),P(3,6),END,
    P(4,0),P(4,5),P(3,6),P(1,6),P(0,5),END, P(0,0),P(0,6),UP,P(4,0),P(0,3),P(4,6),END, P(0,0),P(0,6),P(4,6),END,
    P(0,6),P(0,0),P(2,3),P(4,0),P(4,6),END, P(0,6),P(0,0),P(4,6),P(4,0),END, P(1,0),P(3,0),P(4,1),P(4,5),P(3,6),P(1,6),P(0,5),P(0,1),P(1,0),END,
    P(0,6),P(0,0),P(3,0),P(4,1),P(4,2),P(3,3),P(0,3),END, P(1,0),P(3,0),P(4,1),P(4,5),P(3,6),P(1,6),P(0,5),P(0,1),P(1,0),UP,P(2,4),P(4,6),END,
    P(0,6),P(0,0),P(3,0),P(4,1),P(4,2),P(3,3),P(0,3),UP,P(2,3),P(4,6),END, P(4,1),P(3,0),P(1,0),P(0,1),P(0,2),P(1,3),P(3,3),P(4,4),P(4,5),P(3,6),P(1,6),P(0,5),END,
    P(0,0),P(4,0),UP,P(2,0),P(2,6),END, P(0,0),P(0,5),P(1,6),P(3,6),P(4,5),P(4,0),END, P(0,0),P(2,6),P(4,0),END,
    P(0,0),P(1,6),P(2,3),P(3,6),P(4,0),END, P(0,0),P(4,6),UP,P(4,0),P(0,6),END, P(0,0),P(2,3),P(4,0),UP,P(2,3),P(2,6),END,
    P(0,0),P(4,0),P(0,6),P(4,6),END, P(1,0),P(2,0),P(2,1),P(1,1),P(1,0),END
};
static const char FONT_CH[] = " !'*+-./0123456789:<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ^";
static const uint8_t SYM[] = {
    CIR,P(7,7),13, CIR,P(7,7),12, FCIR,P(7,7),4, END,   /* Sun: a ring and its centre */
    ARC,P(8,7),12,8,24, ARC,P(11,7),13,10,22, END,   /* Moon: a crescent (two arcs) */
    CIR,P(7,7),6, P(7,10),P(7,14),UP,P(5,12),P(9,12),UP, ARC,P(7,2),5,16,32, END,
    CIR,P(7,5),8, P(7,9),P(7,14),UP,P(4,12),P(10,12),END,
    CIR,P(6,9),8, P(9,6),P(13,2),UP,P(9,2),P(13,2),P(13,6),END,
    P(2,3),P(4,1),P(6,2),P(7,4),P(6,7),P(2,10),P(13,10),UP,P(10,2),P(10,14),END,
    P(4,0),P(4,10),UP,P(2,3),P(7,3),UP,P(4,7),P(6,5),P(9,5),P(10,7),P(10,9),P(8,12),P(9,14),P(11,14),END,
    P(2,1),P(2,9),UP,P(12,1),P(12,9),UP,P(2,5),P(12,5),UP,P(7,1),P(7,10),UP, FCIR,P(7,12),4, END,
    P(2,1),P(2,5),P(4,8),P(7,9),P(10,8),P(12,5),P(12,1),UP,P(7,1),P(7,14),UP,P(4,12),P(10,12),END,
    P(4,1),P(4,13),P(11,13),UP,P(4,1),P(8,1),P(10,3),P(10,5),P(8,7),P(4,7),END,
    ARC,P(7,6),10,29,51, FCIR,P(3,11),3, FCIR,P(11,11),3, END,
    P(7,14),P(7,5),P(5,2),P(3,2),P(2,4),P(2,6),UP,P(7,5),P(9,2),P(11,2),P(12,4),P(12,6),END,
    CIR,P(7,9),8, P(2,1),P(4,4),P(7,5),P(10,4),P(12,1),END,
    P(5,2),P(5,12),UP,P(9,2),P(9,12),UP,P(2,1),P(7,2),P(12,1),UP,P(2,13),P(7,12),P(12,13),END,
    FCIR,P(4,5),4, FCIR,P(10,9),4, P(4,3),P(8,2),P(12,4),UP,P(10,11),P(6,12),P(2,10),END,
    FCIR,P(4,9),4, P(6,9),P(5,5),P(6,2),P(9,1),P(11,3),P(11,6),P(9,10),P(10,13),P(12,13),END,
    P(2,3),P(2,12),UP,P(2,4),P(4,3),P(5,4),P(5,12),UP,P(5,4),P(7,3),P(8,4),P(8,12),UP,P(8,5),P(10,3),P(12,5),P(11,9),P(8,12),UP,P(10,10),P(13,13),END,
    P(1,13),P(13,13),UP,P(1,10),P(5,10),P(4,7),P(5,4),P(7,3),P(9,4),P(10,7),P(9,10),P(13,10),END,
    P(2,3),P(2,12),UP,P(2,4),P(4,3),P(5,4),P(5,12),UP,P(5,4),P(7,3),P(8,4),P(8,11),P(10,13),P(13,10),UP,P(11,10),P(13,10),P(13,12),END,
    P(2,13),P(13,2),UP,P(8,2),P(13,2),P(13,7),UP,P(4,7),P(8,11),END,
    P(1,2),P(3,4),P(4,10),P(6,3),P(8,5),P(8,10),P(7,13),P(5,13),UP,P(8,9),P(10,7),P(12,8),P(12,10),P(10,12),P(8,10),END,
    P(1,5),P(3,3),P(5,5),P(7,3),P(9,5),P(11,3),P(13,5),UP,P(1,10),P(3,8),P(5,10),P(7,8),P(9,10),P(11,8),P(13,10),END,
    P(2,1),P(5,4),P(5,10),P(2,13),UP,P(12,1),P(9,4),P(9,10),P(12,13),UP,P(3,7),P(11,7),END,
    FCIR,P(5,9),6, P(7,7),P(12,2),END,
    P(7,1),P(7,13),UP,P(2,4),P(12,10),UP,P(2,10),P(12,4),END,
    P(2,2),P(12,2),P(12,12),P(2,12),P(2,2),END,
    P(7,1),P(13,12),P(1,12),P(7,1),END,
    FCIR,P(3,11),5, FCIR,P(11,3),5, P(5,9),P(9,5),END,
};

#undef P

static int bold;                    /* strokes 2 px wide: glyphs from 12 px, text from scale 2 (1x text stays crisp) */
static void strokes(const uint8_t *g, int ox, int oy, int num, int den, gfx_col_t c) {
    int px = 0, py = 0, pen = 0;
    for (; *g != END; g++) {
        if (*g == UP) { pen = 0; continue; }
        if (*g == CIR || *g == FCIR) { 
            int r = g[2] * num / (2 * den);
            if (*g == CIR) gfx_circle(ox + (g[1] >> 4) * num / den, oy + (g[1] & 15) * num / den, r, c);
            else gfx_fill_circle(ox + (g[1] >> 4) * num / den, oy + (g[1] & 15) * num / den, r, c);
            g += 2; pen = 0; continue; 
        }
        if (*g == ARC) {
            gfx_arc_thick(ox + (g[1] >> 4) * num / den, oy + (g[1] & 15) * num / den, g[2] * num / (2 * den), bold ? 2 : 1, (uint32_t)g[3] << 27, (uint32_t)g[4] << 27, c);
            g += 4; pen = 0; continue;
        }
        { 
          int x = ox + ((*g >> 4) * num + den/2) / den, y = oy + ((*g & 15) * num + den/2) / den;
          if (pen) { if (bold) gfx_line_thick(px, py, x, y, c); else gfx_line(px, py, x, y, c); } else if (g[1] == END || g[1] == UP) gfx_px(x, y, c);
          px = x; py = y; pen = 1; 
        }
    }
}
static const uint8_t *nth(const uint8_t *g, unsigned n) {
    while (n--) {
        while (*g != END) g += (*g == CIR || *g == FCIR) ? 3 : *g == ARC ? 5 : 1;
        g++;
    }
    return g;
}
int gfx_text(int x, int y, const char *s, int scale, gfx_col_t c) {
    for (; *s; s++, x += 6 * scale) {
        unsigned i, ch = (uint8_t)*s;
        if (ch >= 'a' && ch <= 'z') ch -= 32u;
        for (i = 0; FONT_CH[i] && (unsigned)FONT_CH[i] != ch; i++) ;
        bold = scale >= 2;
        if (FONT_CH[i]) strokes(nth(FONT, i), x, y, scale, 1, c);
    }
    return x;
}
int gfx_text_w(const char *s, int scale) { int n = 0; while (*s++) n++; return n ? n * 6 * scale - scale : 0; }
void gfx_text_c(int cx, int y, const char *s, int scale, gfx_col_t c) { gfx_text(cx - gfx_text_w(s, scale) / 2, y, s, scale, c); }
void gfx_glyph(unsigned id, int cx, int cy, int size, gfx_col_t c) {
    bold = size >= 12;
    if (id < GLYPH_N) strokes(nth(SYM, id), cx - size / 2, cy - size / 2, size, 14, c);
}
