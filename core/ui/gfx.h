/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA line art: a 240 x 240 frame of 16-bit pixels, lines, circles, arcs, a stroke font and the astrological
 * glyphs, all drawn as vectors. Integer only (device code). The frame is the caller's; on the FM-1 its pixels are
 * kept in the panel's byte order (GFX_SWAP), so a strip of it goes to the screen as it is. */
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
void gfx_clear(void);
gfx_col_t gfx_hsv(unsigned h, unsigned s, unsigned v);   /* hue 0..255 (wraps), saturation and value 0..255 */
gfx_col_t gfx_dim(gfx_col_t c, unsigned k);              /* c * k / 256 */
void gfx_px(int x, int y, gfx_col_t c);
void gfx_add(int x, int y, gfx_col_t c);                 /* light on light: channels add and saturate */
void gfx_line(int x0, int y0, int x1, int y1, gfx_col_t c);
void gfx_glow(int x0, int y0, int x1, int y1, gfx_col_t c);   /* a line that adds */
void gfx_hline(int x, int y, int w, gfx_col_t c);
void gfx_circle(int cx, int cy, int r, gfx_col_t c);
void gfx_arc(int cx, int cy, int r, uint32_t a0, uint32_t a1, gfx_col_t c);   /* BAM, counter-clockwise from 3 o'clock */
void gfx_polar(int cx, int cy, int r, uint32_t a, int *x, int *y);
int gfx_text(int x, int y, const char *s, int scale, gfx_col_t c);   /* capitals 5 x 7 px times scale; returns the end x */
int gfx_text_w(const char *s, int scale);
void gfx_text_c(int cx, int y, const char *s, int scale, gfx_col_t c);   /* centred on cx */
/* symbols: 0..10 the bodies (Sun..Pluto, Node), 11..22 the signs, 23..27 the aspects (conj sext square trine opp) */
enum { GLYPH_BODY = 0, GLYPH_SIGN = 11, GLYPH_ASPECT = 23, GLYPH_N = 28 };
void gfx_glyph(unsigned id, int cx, int cy, int size, gfx_col_t c);   /* centred, size px across */
#endif
