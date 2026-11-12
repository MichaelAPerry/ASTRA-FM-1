/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA FM engine (see fm.h). Integer only. Signed >> is arithmetic on every compiler we target. */
#include "fm.h"
#include "fm_tables.h"

#ifndef FM_HOT
#define FM_HOT                      /* the device puts the per-sample loop in RAM: no flash-cache misses in it */
#endif
#ifndef FM_FAR
#define FM_FAR(fn) (fn)             /* ... and calls it through a pointer: RAM is out of a flash call's reach */
#endif
#ifndef FM_POOL
#define FM_POOL                     /* the device puts the delay lines in the pool */
#endif

enum { EG_IDLE, EG_ATTACK, EG_DECAY, EG_RELEASE };
#define ENV_ONE (1 << 24)
#define ENV_OVER 21810381           /* the attack aims 30 % past full, so it arrives */
#define WAVE_N 4096u

typedef struct {                            /* all one operator needs each sample, side by side: one base register */
    uint32_t ph, inc;
    int32_t amp, damp;                      /* amplitude Q27 and its step per frame */
    int32_t o, fz, fbd;                     /* its output, the one before; feedback depth */
    const volatile int32_t *s0, *s1, *s2;   /* the sources' outputs, in this voice */
    int32_t d0, d1, amd;                    /* depths; ring amount */
    const int16_t *tab;
    int32_t gl, gr;
} fm_op_t;
typedef struct {
    fm_op_t op[FM_NOP];                     /* audio rate (voice_run) */
    uint32_t inc0[FM_NOP], binc, age;       /* control rate */
    int32_t env[FM_NOP];                    /* Q24 */
    uint16_t vk[FM_NOP];                    /* velocity and key scaling, Q15 */
    uint8_t stage[FM_NOP];
    uint8_t note, gate, held, active, quiet;
} fm_voice_t;

typedef struct {                            /* the smoothed program: voice_ctl copies it into each voice */
    const int16_t *tab[FM_NOP];
    int32_t fbd[FM_NOP], pmd[2][FM_NOP], amd[FM_NOP], gl[FM_NOP], gr[FM_NOP];
    uint32_t pms[2][FM_NOP], ams[FM_NOP];
} fm_ctl_t;

static fm_voice_t voice[FM_NVOICE];
static fm_ctl_t ctl;
static int32_t lvl_s[FM_NOP], gl_s[FM_NOP], gr_s[FM_NOP];   /* Q23, Q18, Q18 */
static fm_prog_t prog[2];
static volatile uint32_t prog_pub, prog_seen;
static int16_t wave[FM_NWAVE][WAVE_N];
static volatile uint32_t ev[32];
static volatile uint32_t ev_w;
static uint32_t ev_r, age_ctr, lfo_ph, ready;
static volatile int32_t in_bend, in_modw, in_sus;

volatile uint8_t fm_meter[FM_NOP];
volatile uint8_t fm_voices;
int16_t fm_scope[2u * FM_SCOPE_N];
volatile uint32_t fm_scope_w;

/* ---- effects state */
#define CHO_N 1024u
#define DLY_N 16384u
#define REV_N 4096u
#define AP_N 512u
static int16_t cho_buf[2][CHO_N] FM_POOL, dly_buf[2][DLY_N] FM_POOL, rev_buf[4][REV_N] FM_POOL, ap_buf[2][AP_N] FM_POOL;
static uint32_t fx_w, cho_ph;
static int32_t cho_d[2] = {300 << 8, 300 << 8}, dly_d = 4000 << 8, rev_lp[4];
#define REV_LEN0 2039u              /* the reverb's lines and its two allpasses, frames (primes) */
#define REV_LEN1 2657u
#define REV_LEN2 3251u
#define REV_LEN3 3989u
#define AP_LEN0 225u
#define AP_LEN1 341u

uint32_t fm_level(unsigned p) { return fm_level_tab[p > 127u ? 127u : p]; }
uint32_t fm_env_k(unsigned p) { return fm_env_tab[p > 127u ? 127u : p]; }
int32_t fm_wave(unsigned w, uint32_t phase) { return wave[w % FM_NWAVE][phase >> 20]; }

static int32_t qsin(uint32_t i) {            /* sine of i / 4096 of a turn, Q15 */
    uint32_t q = (i >> 10) & 3u, r = i & 1023u;
    int32_t v = fm_qsin[q & 1u ? 1024u - r : r];
    return q & 2u ? -v : v;
}

void fm_init(void) {
    uint32_t w, i, k;
    for (w = 0; w < FM_NWAVE; w++)
        for (i = 0; i < WAVE_N; i++) {
            int32_t s = 0;
            for (k = fm_harm_at[w]; k < fm_harm_at[w + 1u]; k++) s += qsin(fm_harm[k].h * i + fm_harm[k].q * 1024u) * fm_harm[k].w;
            s >>= 12;
            wave[w][i] = (int16_t)(s > 32767 ? 32767 : s < -32767 ? -32767 : s);
        }
    for (i = 0; i < FM_NOP; i++) ctl.tab[i] = wave[0];
    ready = 1;
}

fm_prog_t *fm_prog_begin(void) { return prog_seen == prog_pub ? &prog[prog_pub ^ 1u] : 0; }
void fm_prog_commit(void) { prog_pub ^= 1u; }

static void ev_put(uint32_t e) {
    uint32_t w = ev_w;
    if (w - ev_r >= 32u) return;             /* full: drop (the interrupt drains it every 5.8 ms) */
    ev[w & 31u] = e;
    ev_w = w + 1u;
}
void fm_note_on(unsigned note, unsigned vel) { ev_put(1u << 16 | (note & 127u) << 8 | (vel & 127u)); }
void fm_note_off(unsigned note) { ev_put(2u << 16 | (note & 127u) << 8); }
void fm_all_off(void) { ev_put(3u << 16); }
void fm_bend(int v) { in_bend = v < -8192 ? -8192 : v > 8191 ? 8191 : v; }
void fm_modwheel(unsigned v) { in_modw = (int32_t)(v > 127u ? 127u : v); }
void fm_sustain(int on) { in_sus = on != 0; }

/* a * ratio (Q16), held below Nyquist */
static uint32_t ratio_mul(uint32_t a, uint32_t r) {
    uint32_t rh = r >> 16, rl = r & 0xFFFFu;
    if (a >= 0x7FFFFFFFu / (rh + 1u)) return 0x7FFFFFFFu;
    return a * rh + (a >> 16) * rl + (((a & 0xFFFFu) * rl) >> 16);
}

static void voice_pitch(fm_voice_t *v, const fm_prog_t *p) {
    uint32_t i;
    for (i = 0; i < FM_NOP; i++) v->inc0[i] = ratio_mul(v->binc, p->ratio[i]);
}

static void voice_start(fm_voice_t *v, const fm_prog_t *p, uint32_t note, uint32_t vel) {
    uint32_t i, fresh = !v->active, k = note > 48u ? note - 48u : 0u;
    v->note = (uint8_t)note; v->gate = 1; v->held = 0; v->active = 1; v->quiet = 0; v->age = ++age_ctr;
    v->binc = fm_inc_top[note % 12u] >> (10u - note / 12u);
    voice_pitch(v, p);
    for (i = 0; i < FM_NOP; i++) {
        int32_t vf = 32768 - (int32_t)((127u - vel) * p->vel[i] * 2u), kf = 32768 - (int32_t)(k * p->ksc[i] * 4u);
        v->vk[i] = (uint16_t)((vf * (kf < 0 ? 0 : kf)) >> 15);
        v->stage[i] = EG_ATTACK;
        if (fresh) { v->op[i].ph = 0; v->op[i].o = v->op[i].fz = 0; v->op[i].amp = v->op[i].damp = 0; v->env[i] = 0; }
    }
}
static void voice_release(fm_voice_t *v) {
    uint32_t i;
    v->gate = 0; v->held = 0;
    for (i = 0; i < FM_NOP; i++) if (v->stage[i] != EG_IDLE) v->stage[i] = EG_RELEASE;
}

static void events(const fm_prog_t *p) {
    uint32_t w = ev_w, i;
    while (ev_r != w) {
        uint32_t e = ev[ev_r++ & 31u], note = (e >> 8) & 127u, type = e >> 16;
        fm_voice_t *pick = 0;
        if (type == 1u) {                    /* the same note again, else a free voice, else the oldest released, else the oldest */
            for (i = 0; i < FM_NVOICE; i++) if (voice[i].active && voice[i].note == note) { pick = &voice[i]; break; }
            if (!pick) for (i = 0; i < FM_NVOICE; i++) if (!voice[i].active) { pick = &voice[i]; break; }
            if (!pick) for (i = 0; i < FM_NVOICE; i++) if (!voice[i].gate && (!pick || voice[i].age < pick->age)) pick = &voice[i];
            if (!pick) for (i = 0; i < FM_NVOICE; i++) if (!pick || voice[i].age < pick->age) pick = &voice[i];
            voice_start(pick, p, note, e & 127u ? e & 127u : 1u);
        } else if (type == 2u) {
            for (i = 0; i < FM_NVOICE; i++) if (voice[i].active && voice[i].gate && voice[i].note == note) {
                if (in_sus) voice[i].held = 1; else voice_release(&voice[i]);
            }
        } else {
            for (i = 0; i < FM_NVOICE; i++) if (voice[i].active) voice_release(&voice[i]);
        }
    }
    if (!in_sus) for (i = 0; i < FM_NVOICE; i++) if (voice[i].held) voice_release(&voice[i]);
}

/* envelopes, amplitude ramps and pitch for one control step, and the smoothed program into the voice's own
 * operator records. pmod: pitch offset, Q15 fraction; trem: Q15 gain */
static void voice_ctl(fm_voice_t *v, const fm_prog_t *p, int32_t pmod, int32_t trem) {
    uint32_t i, live = 0;
    for (i = 0; i < FM_NOP; i++) {
        fm_op_t *q = &v->op[i];
        int32_t e = v->env[i], a, d;
        switch (v->stage[i]) {
        case EG_ATTACK:
            e += (int32_t)((((uint32_t)(ENV_OVER - e) >> 12) * p->ek[0][i]) >> 8);
            if (e >= ENV_ONE) { e = ENV_ONE; v->stage[i] = EG_DECAY; }
            break;
        case EG_DECAY:
            d = ((int32_t)p->es[i] << 9) - e;
            e += d < 0 ? -(int32_t)((((uint32_t)-d >> 12) * p->ek[1][i]) >> 8) : (int32_t)((((uint32_t)d >> 12) * p->ek[1][i]) >> 8);
            break;
        case EG_RELEASE:
            e -= (int32_t)((((uint32_t)e >> 12) * p->ek[2][i]) >> 8) + 16;   /* + a little: the tail ends */
            if (e <= 0) { e = 0; v->stage[i] = EG_IDLE; }
            break;
        default:
            e = 0;
        }
        v->env[i] = e;
        live |= v->stage[i];
        a = (int32_t)(((uint32_t)(e >> 9) * (uint32_t)(lvl_s[i] >> 8)) >> 15);
        a = (((a * v->vk[i]) >> 15) * trem) >> 15;
        q->damp = ((a << 12) - q->amp) >> 5;
        q->inc = v->inc0[i] + (uint32_t)((int32_t)(v->inc0[i] >> 15) * pmod);
        q->fbd = ctl.fbd[i]; q->d0 = ctl.pmd[0][i]; q->d1 = ctl.pmd[1][i]; q->amd = ctl.amd[i];
        q->s0 = &v->op[ctl.pms[0][i]].o; q->s1 = &v->op[ctl.pms[1][i]].o; q->s2 = &v->op[ctl.ams[i]].o;
        q->tab = ctl.tab[i]; q->gl = ctl.gl[i]; q->gr = ctl.gr[i];
    }
    if (live) v->quiet = 0;
    else if (v->quiet) v->active = 0;        /* a step after the last envelope ended: the ramps are at zero */
    else v->quiet = 1;
}

/* ---- the per-sample loop: FM_SUB frames of one voice, ten operators in a straight line (a taken branch costs as
 * much as five loads on this chip, so nothing here loops over operators or tests what a patch uses).
 * The record is read through a volatile pointer on purpose: every load then happens once, where it is written.
 * Left to itself the compiler (JieLi clang 4, -Os) hoists the 40 loop-invariant words into stack slots and reloads
 * them from there: a third more time, measured (docs/ENGINE.md). */
#define FM_OP(i) do { \
    volatile fm_op_t *q = &v->op[i]; \
    int32_t o1 = q->o, s, m; \
    uint32_t p = q->ph + q->inc; \
    q->ph = p; \
    p += (uint32_t)(o1 + q->fz) * (uint32_t)q->fbd + (uint32_t)*q->s0 * (uint32_t)q->d0 + (uint32_t)*q->s1 * (uint32_t)q->d1; \
    q->fz = o1; \
    s = q->amp + q->damp; \
    q->amp = s; \
    s = (q->tab[p >> 20] * (s >> 12)) >> 15; \
    m = q->amd; \
    q->o = (s * (32768 - m + ((*q->s2 * m) >> 15))) >> 15; \
} while (0)
#define FM_MIX(i) do { volatile fm_op_t *q = &v->op[i]; int32_t s = q->o; l += s * q->gl; r += s * q->gr; } while (0)

static FM_HOT void voice_run(fm_voice_t *v, int32_t *mix) {
    uint32_t n = FM_SUB;
    do {
        int32_t l = 0, r = 0;
        FM_OP(0); FM_OP(1); FM_OP(2); FM_OP(3); FM_OP(4); FM_OP(5); FM_OP(6); FM_OP(7); FM_OP(8); FM_OP(9);
        FM_MIX(0); FM_MIX(1); FM_MIX(2); FM_MIX(3); FM_MIX(4); FM_MIX(5); FM_MIX(6); FM_MIX(7); FM_MIX(8); FM_MIX(9);
        mix[0] += l >> 10; mix[1] += r >> 10;
        mix += 2;
    } while (--n);
}

/* ---- effects: chorus, cross-fed delay, a four-line reverb, a soft knee into full scale. One control step of mix into out. */
static inline int32_t sat16(int32_t x) { return x > 32767 ? 32767 : x < -32767 ? -32767 : x; }
static inline int32_t tap(const int16_t *b, uint32_t mask, uint32_t w, int32_t d) {   /* d frames (Q8) behind w */
    uint32_t i = w - (uint32_t)(d >> 8);
    int32_t a = b[i & mask], z = b[(i - 1u) & mask];
    return a + (((z - a) * (d & 255)) >> 8);
}
static inline int32_t soft(int32_t x) {       /* clean to 3/4 of full scale, then a parabola that lands flat on full scale */
    int32_t a = x < 0 ? -x : x, d;
    if (a <= 24576) return x;
    d = a - 24576;
    if (d > 16382) d = 16382;
    a = 24576 + d - ((d * d) >> 15);
    if (a > 32767) a = 32767;
    return x < 0 ? -a : a;
}
static void fx_run(const fm_prog_t *p, const int32_t *mix, int32_t *out) {
    uint32_t i, w = fx_w;
    int32_t ct[2], cs[2], ds, g = p->gain > 8191u ? 8191 : p->gain, cm = p->cho_mix, dm = p->dly_mix, df = p->dly_fb, rm = p->rev_mix;
    int32_t rd = p->rev_decay, rp = p->rev_damp;
    cho_ph += p->cho_inc;
    ct[0] = (300 << 8) + (int32_t)(((uint32_t)(wave[0][cho_ph >> 20] + 32768) * p->cho_depth) >> 16);
    ct[1] = (300 << 8) + (int32_t)(((uint32_t)(wave[0][(cho_ph + 0x40000000u) >> 20] + 32768) * p->cho_depth) >> 16);
    cs[0] = (ct[0] - cho_d[0]) >> 5; cs[1] = (ct[1] - cho_d[1]) >> 5;
    ds = (((int32_t)(p->dly_time > FM_DLY_MAX ? FM_DLY_MAX : p->dly_time ? p->dly_time : 1u) << 8) - dly_d) >> 5;
    ds = ds > 96 ? 96 : ds < -96 ? -96 : ds;  /* the delay time glides (a tape's pitch slide), it never jumps */
    for (i = 0; i < FM_SUB; i++, w++) {
        int32_t l = mix[2u * i], r = mix[2u * i + 1u], a, b, x, y[4], s;
        l = l > 262143 ? 262143 : l < -262143 ? -262143 : l;   /* eight times full scale: the product below fits */
        r = r > 262143 ? 262143 : r < -262143 ? -262143 : r;
        l = (l * g) >> 12; r = (r * g) >> 12;
        l = l > 98300 ? 98300 : l < -98300 ? -98300 : l;     /* three times full scale is all the headroom the effects need */
        r = r > 98300 ? 98300 : r < -98300 ? -98300 : r;
        /* chorus: one swept tap a side, a quarter turn apart */
        cho_buf[0][w & (CHO_N - 1u)] = (int16_t)sat16(l); cho_buf[1][w & (CHO_N - 1u)] = (int16_t)sat16(r);
        cho_d[0] += cs[0]; cho_d[1] += cs[1];
        l += (tap(cho_buf[0], CHO_N - 1u, w, cho_d[0]) * cm) >> 15;
        r += (tap(cho_buf[1], CHO_N - 1u, w, cho_d[1]) * cm) >> 15;
        /* delay: each side feeds the other */
        dly_d += ds;
        a = tap(dly_buf[0], DLY_N - 1u, w, dly_d); b = tap(dly_buf[1], DLY_N - 1u, w, dly_d);
        dly_buf[0][w & (DLY_N - 1u)] = (int16_t)sat16(l + ((b * df) >> 15));
        dly_buf[1][w & (DLY_N - 1u)] = (int16_t)sat16(r + ((a * df) >> 15));
        l += (a * dm) >> 15; r += (b * dm) >> 15;
        /* reverb: two allpasses into four delay lines mixed by a Householder reflection, a lowpass in each loop
         * (written out line by line: a loop's branch costs more than the line itself) */
        x = (l + r) >> 2;
#define FM_AP(k) do { int32_t z = ap_buf[k][(w - AP_LEN##k) & (AP_N - 1u)], v = sat16(x + (z >> 1)); \
                      ap_buf[k][w & (AP_N - 1u)] = (int16_t)v; x = z - (v >> 1); } while (0)
#define FM_RD(k) do { rev_lp[k] += ((rev_buf[k][(w - REV_LEN##k) & (REV_N - 1u)] - rev_lp[k]) * rp) >> 15; y[k] = rev_lp[k]; } while (0)
#define FM_WR(k) rev_buf[k][w & (REV_N - 1u)] = (int16_t)sat16(x + (((y[k] - s) * rd) >> 15))
        FM_AP(0); FM_AP(1);
        FM_RD(0); FM_RD(1); FM_RD(2); FM_RD(3);
        s = (y[0] + y[1] + y[2] + y[3]) >> 1;
        FM_WR(0); FM_WR(1); FM_WR(2); FM_WR(3);
        l += ((y[0] - y[2]) * rm) >> 15; r += ((y[1] - y[3]) * rm) >> 15;
        l = soft(l); r = soft(r);
        out[2u * i] += l; out[2u * i + 1u] += r;
        if (w & 1u) { uint32_t sw = fm_scope_w; fm_scope[2u * sw] = (int16_t)sat16(l); fm_scope[2u * sw + 1u] = (int16_t)sat16(r); fm_scope_w = (sw + 1u) & (FM_SCOPE_N - 1u); }
    }
    fx_w = w;
}

void fm_render(int32_t *out, uint32_t n) {
    static int32_t mix[2u * FM_SUB];
    const fm_prog_t *p;
    uint32_t off, i, k, pi = prog_pub, nv = 0;
    uint8_t met[FM_NOP] = {0};
    if (!ready) return;
    p = &prog[pi];
    if (pi != prog_seen) {                   /* a new program: what cannot glide changes here */
        for (i = 0; i < FM_NOP; i++) {
            ctl.tab[i] = wave[p->wave[i] % FM_NWAVE];
            for (k = 0; k < 2u; k++) if (ctl.pms[k][i] != p->pms[k][i]) { ctl.pms[k][i] = p->pms[k][i] % FM_NOP; ctl.pmd[k][i] = 0; }
            if (ctl.ams[i] != p->ams[i]) { ctl.ams[i] = p->ams[i] % FM_NOP; ctl.amd[i] = 0; }
        }
        for (i = 0; i < FM_NVOICE; i++) if (voice[i].active) voice_pitch(&voice[i], p);
        prog_seen = pi;
    }
    events(p);
    for (off = 0; off < n; off += FM_SUB) {
        int32_t lfo, pmod, trem, bend = in_bend, br = p->bend;
        for (i = 0; i < FM_NOP; i++) {       /* everything continuous glides: a knob, or the sky moving, never steps */
            lvl_s[i] += (((int32_t)p->level[i] << 8) - lvl_s[i]) >> 2;
            gl_s[i] += (((int32_t)p->gl[i] << 8) - gl_s[i]) >> 2; ctl.gl[i] = gl_s[i] >> 8;
            gr_s[i] += (((int32_t)p->gr[i] << 8) - gr_s[i]) >> 2; ctl.gr[i] = gr_s[i] >> 8;
            ctl.fbd[i] += (p->fbd[i] - ctl.fbd[i]) >> 2;
            ctl.pmd[0][i] += (p->pmd[0][i] - ctl.pmd[0][i]) >> 2;
            ctl.pmd[1][i] += (p->pmd[1][i] - ctl.pmd[1][i]) >> 2;
            ctl.amd[i] += ((int32_t)p->amd[i] - ctl.amd[i]) >> 2;
        }
        lfo_ph += p->lfo_inc;
        lfo = wave[0][lfo_ph >> 20];
        if (bend < 0) br = (br << 15) / (32768 + br);        /* down by the same interval as up */
        pmod = ((bend * br) >> 13) + ((lfo * ((int32_t)p->lfo_pitch + in_modw * 16)) >> 15);
        trem = 32768 - ((((lfo + 32768) >> 1) * ((int32_t)p->lfo_amp + in_modw * 64 > 32767 ? 32767 : (int32_t)p->lfo_amp + in_modw * 64)) >> 15);
        for (i = 0; i < 2u * FM_SUB; i++) mix[i] = 0;
        for (i = 0; i < FM_NVOICE; i++) {
            if (!voice[i].active) continue;
            voice_ctl(&voice[i], p, pmod, trem);
            FM_FAR(voice_run)(&voice[i], mix);
        }
        fx_run(p, mix, out + 2u * off);
    }
    for (i = 0; i < FM_NVOICE; i++) {
        if (!voice[i].active) continue;
        nv++;
        for (k = 0; k < FM_NOP; k++) { uint32_t m = (uint32_t)voice[i].env[k] >> 16; if (m > met[k]) met[k] = (uint8_t)(m > 255u ? 255u : m); }
    }
    for (k = 0; k < FM_NOP; k++) fm_meter[k] = met[k];
    fm_voices = (uint8_t)nv;
}
