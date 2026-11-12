/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA FM engine: ten operators a voice (one per body, Sun..Pluto), wired by a matrix the chart supplies.
 * Integer only, no libc, no 64-bit arithmetic: the same file runs in the FM-1's audio interrupt and in the host tests.
 *
 * Each operator, every sample:
 *     phase += step
 *     p      = phase + feedback * (own last two outputs) + depth1 * out[src1] + depth2 * out[src2]     phase modulation
 *     s      = wave[p] * amplitude                                 (its envelope, level, velocity: ramped per sample)
 *     out    = s * ((1 - ring) + ring * out[src3])                 amplitude / ring modulation
 *     left  += out * gl;  right += out * gr                        its own place in the stereo field
 * The sources are any operators, itself included: operators run in order, so a source with a lower number is this
 * sample's output and any other is the last sample's. Every loop, of any shape, is one sample long: no topology is
 * special, and none can stall the renderer. The cost is the same whatever the patch does (see docs/ENGINE.md).
 *
 * Threads: fm_render() is the audio interrupt. Everything else is the main loop. The main loop hands over a whole
 * program (fm_prog_t) by publishing one of two buffers, and notes through a one-producer ring. */
#ifndef FM_H
#define FM_H
#include <stdint.h>

#define FM_NOP 10u                  /* operators a voice */
#ifndef FM_NVOICE
#define FM_NVOICE 4u                /* voices: 4 = 54 % of the audio period on the FM-1's measured costs (3: 42 %,
                                       5: 66 %); the owner's call (DECISIONS D12), checked on hardware with the GLO load meter */
#endif
#define FM_SUB 32u                  /* frames per control step: envelopes, smoothing, LFO */
#define FM_FS 44100u
#define FM_NWAVE 4u                 /* AIR (sine), FIRE, EARTH, WATER */
#define FM_SCOPE_N 256u             /* stereo frames in the scope ring (every 2nd output frame) */
#define FM_PM_MAX (1 << 19)         /* the largest phase modulation depth: a full-level source swings 4 cycles */
#define FM_FB_MAX (1 << 15)         /* the largest feedback depth */
#define FM_DLY_MAX 16000u           /* delay line, frames */

typedef struct {
    uint32_t ratio[FM_NOP];         /* frequency = the note's * ratio, Q16 (detune folded in) */
    int32_t pmd[2][FM_NOP];         /* phase modulation depths (signed: polarity), 2^32 a cycle per unit of source output */
    int32_t fbd[FM_NOP];            /* feedback depth, same unit */
    uint32_t ek[3][FM_NOP];         /* envelope attack, decay, release: approach per control step, Q20 (fm_env_k) */
    uint16_t es[FM_NOP];            /* sustain level, Q15 */
    uint16_t level[FM_NOP];         /* amplitude, Q15: for a modulator this is its index */
    uint16_t amd[FM_NOP];           /* ring amount, Q15: 0 none, 16384 full AM, 32767 ring modulation */
    uint16_t gl[FM_NOP], gr[FM_NOP];/* output gains, Q10 */
    uint8_t pms[2][FM_NOP];         /* phase modulation sources (operator numbers) */
    uint8_t ams[FM_NOP];            /* ring source */
    uint8_t wave[FM_NOP];           /* 0..FM_NWAVE-1 */
    uint8_t vel[FM_NOP];            /* velocity sensitivity 0..127 */
    uint8_t ksc[FM_NOP];            /* key scaling 0..127: higher notes quieter (tames a modulator's index up the keyboard) */
    uint32_t lfo_inc;               /* LFO phase step per control step */
    uint16_t lfo_pitch;             /* LFO to pitch, Q15 fraction of the frequency */
    uint16_t lfo_amp;               /* LFO to every operator's amplitude, Q15 */
    uint16_t bend;                  /* pitch bend range, Q15 fraction of the frequency */
    uint16_t gain;                  /* into the effects, Q12 */
    uint32_t cho_inc;               /* chorus LFO step per control step */
    uint16_t cho_depth;             /* chorus sweep, frames Q8 (up to 2048 = 8 frames... see fm.c) */
    uint16_t cho_mix;               /* Q15 */
    uint32_t dly_time;              /* delay, frames (1..FM_DLY_MAX) */
    uint16_t dly_fb, dly_mix;       /* Q15 */
    uint16_t rev_decay, rev_damp, rev_mix;   /* Q15 */
} fm_prog_t;

void fm_init(void);                 /* once, before the first fm_render: builds the wave tables */
fm_prog_t *fm_prog_begin(void);     /* a program to fill, or 0 while the interrupt has not taken the last one */
void fm_prog_commit(void);          /* publish it */
void fm_note_on(unsigned note, unsigned vel);    /* MIDI note 0..127, velocity 1..127 */
void fm_note_off(unsigned note);
void fm_all_off(void);
void fm_bend(int v);                /* -8192..8191 */
void fm_modwheel(unsigned v);       /* 0..127: adds LFO depth */
void fm_sustain(int on);
void fm_render(int32_t *out, uint32_t n);        /* the audio interrupt: adds n stereo frames (n % FM_SUB == 0) to out */

uint32_t fm_level(unsigned p);      /* 0..127 -> amplitude Q15, 0.5 dB a step */
uint32_t fm_env_k(unsigned p);      /* 0..127 (time) -> envelope coefficient */
int32_t fm_wave(unsigned w, uint32_t phase);     /* a wave's value, Q15: for drawing it */

/* what the interrupt shows the main loop (one writer: the interrupt) */
extern volatile uint8_t fm_meter[FM_NOP];        /* each operator's loudest envelope, 0..255 */
extern volatile uint8_t fm_voices;               /* voices sounding */
extern int16_t fm_scope[2u * FM_SCOPE_N];        /* the output, L R, a ring */
extern volatile uint32_t fm_scope_w;             /* next frame to be written */
#endif
