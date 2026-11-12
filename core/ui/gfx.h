/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA line art: a 240 x 240 frame of 16-bit pixels, drawn as vectors and filled geometry. 
 * Integer only (device code). */
#ifndef ASTRA_GFX_H
#define ASTRA_GFX_H
#include <stdint.h>
#define GFX_W 240
#define GFX_H 240
#ifdef GFX_SWAP
#define GFX_PX(c) ((uint16_t)((((c) >> 8) & 0xFFu) | (((c) & 0xFFu) << 8)))
#else
#define GFX_PX(c) ((uint16_t)(c))
#endif
#define GFX_RGB(r, g, b) GFX_PX((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
typedef uint16_t gfx_col_t;         /* a pixel as stored */

void gfx_target(uint16_t *fb);      /* GFX_W * GFX_H pixels */
void gfx_clear(gfx_col_t c);        /* now takes a background color */
gfx_col_t gfx_hsv(unsigned h, unsigned s, unsigned v);
gfx_col_t gfx_dim(gfx_col_t c, unsigned k);
gfx_col_t gfx_blend(gfx_col_t fg, gfx_col_t bg, unsigned alpha); /* alpha 0..255 */

void gfx_px(int x, int y, gfx_col_t c);
void gfx_add(int x, int y, gfx_col_t c);
void gfx_line(int x0, int y0, int x1, int y1, gfx_col_t c);
void gfx_line_thick(int x0, int y0, int x1, int y1, gfx_col_t c); /* 2px footprint for solid vectors */
void gfx_glow(int x0, int y0, int x1, int y1, gfx_col_t c);
void gfx_hline(int x, int y, int w, gfx_col_t c);
void gfx_vline(int x, int y, int h, gfx_col_t c);

void gfx_fill_rect(int x, int y, int w, int h, gfx_col_t c);
void gfx_fill_circle(int cx, int cy, int r, gfx_col_t c);
void gfx_circle(int cx, int cy, int r, gfx_col_t c);
void gfx_arc(int cx, int cy, int r, uint32_t a0, uint32_t a1, gfx_col_t c);
void gfx_arc_thick(int cx, int cy, int r, int thick, uint32_t a0, uint32_t a1, gfx_col_t c);
void gfx_polar(int cx, int cy, int r, uint32_t a, int *x, int *y);

int gfx_text(int x, int y, const char *s, int scale, gfx_col_t c);
int gfx_text_w(const char *s, int scale);
void gfx_text_c(int cx, int y, const char *s, int scale, gfx_col_t c);

enum { GLYPH_BODY = 0, GLYPH_SIGN = 11, GLYPH_ASPECT = 23, GLYPH_N = 28 };
void gfx_glyph(unsigned id, int cx, int cy, int size, gfx_col_t c);
#endif
