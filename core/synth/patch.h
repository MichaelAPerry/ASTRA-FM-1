/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA patches: what a preset slot holds, how a chart becomes one (patch_cast), and how a patch and the charts
 * become what the FM engine plays (patch_derive). Integer only (device code).
 *
 * The chart is the algorithm. The ten operators are the ten bodies; an aspect between two of them is a wire:
 *   conjunction   strong phase modulation (the two act as one); a planet's return to its own place is feedback
 *   trine         phase modulation, flowing          sextile   phase modulation, light
 *   square        ring modulation (friction), with a little inverted phase modulation
 *   opposition    phase modulation both ways, the return inverted: a loop (the two face each other)
 * The faster body modulates the slower (the Moon colours everything, Pluto only listens); in a transit chart the
 * moving sky modulates the birth chart. A closer aspect is a deeper one, an applying one deeper than a separating one,
 * a retrograde source turns its modulation upside down. Each operator hears its two deepest phase sources and its
 * deepest ring source. Harmonic charts (longitudes times 1..12) give twelve wirings from one sky. */
#ifndef PATCH_H
#define PATCH_H
#include "core/chart/chart.h"
#include "core/fm/fm.h"

#define PATCH_VERSION 1u
enum { ALGO_FREE, ALGO_NATAL, ALGO_SKY, ALGO_TRANSIT };   /* algorithm number: 0 FREE, then 12 harmonics of each */
#define PATCH_NALGO 37u
#define patch_algo_src(a) ((a) ? 1u + ((a) - 1u) / 12u : 0u)
#define patch_algo_harm(a) ((a) ? 1u + ((a) - 1u) % 12u : 1u)

typedef struct {                    /* an operator, as the knobs see it (all 0..127 unless said) */
    uint8_t coarse;                 /* frequency ratio: 0 = 1/2, 1..24 */
    uint8_t fine;                   /* ratio * (1 + fine / 128) */
    int8_t detune;                  /* half cents, -63..63 */
    uint8_t level, feedback, wave, out, vel, ksc;
    uint8_t a, d, s, r;             /* envelope: attack, decay times; sustain level; release time */
    uint8_t src[2];                 /* FREE algorithm: phase sources and depths (sign: polarity) */
    int8_t depth[2];
    uint8_t rsrc, ring;             /* FREE algorithm: ring source and amount */
    uint8_t mute;
} patch_op_t;

typedef struct {
    uint8_t version;
    uint8_t algo;                   /* 0..PATCH_NALGO-1 */
    uint8_t orb;                    /* degrees, 1..15 */
    uint8_t index, ring;            /* how deep the aspects' phase and ring modulation go */
    uint8_t orbit;                  /* how much each operator swells as its planet nears its own birth place */
    uint8_t width;                  /* the wheel as a stereo field */
    int8_t warp;                    /* the sky clock: 0 real time, +-1..9 faster, forward or back (patch_warp_sec) */
    uint8_t lfo_rate, lfo_pitch, lfo_amp, bend, volume;
    int8_t octave;
    uint8_t cho_depth, cho_mix, dly_time, dly_fb, dly_mix, rev_decay, rev_damp, rev_mix;
    uint8_t pad[2];
    patch_op_t op[FM_NOP];
} patch_t;

typedef struct {                    /* the wiring as derived, for the screen */
    struct { uint8_t src, dst, kind, ring; uint8_t depth; int8_t sign; } edge[3u * FM_NOP];
    uint8_t nedge;
    uint8_t fb[FM_NOP];             /* feedback as played, 0..255 */
    uint8_t pan[FM_NOP];            /* 0 left .. 255 right */
    uint8_t lvl[FM_NOP];            /* level after the orbit, 0..255 */
    chart_asp_t asp[CHART_MAX_ASP]; /* the aspects the algorithm read */
    uint8_t nasp;
} patch_route_t;

void patch_cast(patch_t *p, const chart_t *c, unsigned variant);   /* a whole patch from a chart; variant: its character */
/* the program for the engine. natal may be 0 (the sky then stands in). route may be 0. */
void patch_derive(const patch_t *p, const chart_t *natal, const chart_t *sky, fm_prog_t *out, patch_route_t *route);
int32_t patch_warp_sec(int warp);   /* sky seconds per real second */
uint32_t patch_ratio_q16(const patch_op_t *o);
void patch_sanitize(patch_t *p);    /* clamp every field into range (a slot read from flash) */
extern const uint8_t patch_rank[ASTRO_NPLANET];   /* 0 fastest (Moon) .. 9 slowest (Pluto) */
#endif
