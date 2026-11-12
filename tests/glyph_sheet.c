/* SPDX-License-Identifier: GPL-3.0-only
 * Draws every glyph and the font to a PPM, to look at them (host only).  glyph_sheet OUT.ppm */
#include <stdio.h>
#include "core/ui/gfx.h"
static uint16_t fb[GFX_W * GFX_H];
void shot(const char *path) {
    FILE *f = fopen(path, "wb"); int i;
    fprintf(f, "P6 %d %d 255\n", GFX_W, GFX_H);
    for (i = 0; i < GFX_W * GFX_H; i++) { unsigned c = fb[i]; fputc((c >> 11) * 255 / 31, f); fputc(((c >> 5) & 63) * 255 / 63, f); fputc((c & 31) * 255 / 31, f); }
    fclose(f);
}
int main(int argc, char **argv) {
    unsigned i;
    gfx_target(fb); gfx_clear();
    for (i = 0; i < GLYPH_N; i++) gfx_glyph(i, 20 + (int)(i % 7) * 33, 18 + (int)(i / 7) * 34, 26, gfx_hsv(i * 9, 200, 255));
    for (i = 0; i < 12; i++) gfx_glyph(i < 11 ? i : 11, 10 + (int)i * 19, 152, 13, GFX_RGB(255, 255, 255));
    for (i = 0; i < 12; i++) gfx_glyph(GLYPH_SIGN + i, 10 + (int)i * 19, 170, 11, GFX_RGB(255, 255, 255));
    gfx_text(4, 184, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 1, GFX_RGB(255, 255, 255));
    gfx_text(4, 194, "0123456789 .:-+/'^<>=?!*", 1, GFX_RGB(120, 255, 160));
    gfx_text(4, 206, "ASTRA FM-1 0123456789", 2, GFX_RGB(150, 170, 255));
    gfx_text(4, 224, "SATURN SQUARE MOON 12^34'", 1, GFX_RGB(255, 200, 80));
    if (argc > 1) shot(argv[1]);
    return 0;
}
