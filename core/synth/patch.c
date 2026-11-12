/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA patches (see patch.h). Integer only. */
#include "patch.h"

const uint8_t patch_rank[ASTRO_NPLANET] = {3, 0, 1, 2, 4, 5, 6, 7, 8, 9};

int32_t patch_warp_sec(int w) {
    static const int32_t R[10] = {1, 60, 3600, 21600, 86400, 604800, 2629800, 7889400, 31557600, 157788000};
    int a = w < 0 ? -w : w;
    if (a > 9) a = 9;
    return w < 0 ? -R[a] : R[a];
}

uint32_t patch_ratio_q16(const patch_op_t *o) {
    uint32_t r = o->coarse ? (uint32_t)o->coarse << 16 : 0x8000u;
    int32_t x = (int32_t)o->detune * 4845, x1;            /* half cents * ln 2 / 2400, Q24 */
    r += (r >> 7) * o->fine;
    x1 = ((int32_t)(r >> 8) * (x >> 8)) >> 8;               /* r * x, then r * x^2 / 2: 2^x to a hundredth of a cent */
    return (uint32_t)((int32_t)r + x1 + ((x1 * (x >> 8)) >> 17));
}

void patch_sanitize(patch_t *p) {
    unsigned i;
#define CL(f, hi) if ((f) > (hi)) (f) = (hi)
#define CS(f, lo, hi) if ((f) < (lo)) (f) = (lo); else if ((f) > (hi)) (f) = (hi)
    p->version = PATCH_VERSION;
    CL(p->algo, PATCH_NALGO - 1u); CS(p->orb, 1, 15); CL(p->index, 127); CL(p->ring, 127); CL(p->orbit, 127); CL(p->width, 127);
    CS(p->warp, -9, 9); CL(p->lfo_rate, 127); CL(p->lfo_pitch, 127); CL(p->lfo_amp, 127); CL(p->bend, 12); CL(p->volume, 127);
    CS(p->octave, -3, 3); CL(p->cho_depth, 127); CL(p->cho_mix, 127); CL(p->dly_time, 127); CL(p->dly_fb, 127); CL(p->dly_mix, 127);
    CL(p->rev_decay, 127); CL(p->rev_damp, 127); CL(p->rev_mix, 127);
    for (i = 0; i < FM_NOP; i++) {
        patch_op_t *o = &p->op[i];
        CL(o->coarse, 24); CL(o->fine, 127); CS(o->detune, -63, 63); CL(o->level, 127); CL(o->feedback, 127); CL(o->wave, FM_NWAVE - 1u);
        CL(o->out, 127); CL(o->vel, 127); CL(o->ksc, 127); CL(o->a, 127); CL(o->d, 127); CL(o->s, 127); CL(o->r, 127);
        CL(o->src[0], FM_NOP - 1u); CL(o->src[1], FM_NOP - 1u); CL(o->rsrc, FM_NOP - 1u); CL(o->ring, 127); CL(o->mute, 1);
        if (o->depth[0] == -128) o->depth[0] = -127;
        if (o->depth[1] == -128) o->depth[1] = -127;
    }
}

/* ---- a chart becomes a patch.
 *   ratio     the planet's solar house: signs counted from the Sun's (the Sun is always 1, the fundamental)
 *   detune    its degree within the sign, around the middle (up to +-15 cents)
 *   wave      its sign's element                  envelope  its sign's modality, slower for slower planets
 *   level     its dignity                         feedback  domicile or exaltation; more when retrograde
 *   loudness  its house: angular, succedent, cadent; a higher harmonic sounds softer; the Sun leads */
void patch_cast(patch_t *p, const chart_t *c, unsigned variant) {
    static const uint8_t WAVE[4] = {1, 2, 0, 3};                    /* fire, earth, air, water */
    static const uint8_t ENV[3][4] = {{2, 62, 52, 58}, {44, 74, 112, 72}, {18, 84, 78, 92}};   /* cardinal, fixed, mutable */
    static const uint8_t ROLL[13] = {0, 0, 4, 7, 8, 10, 11, 12, 12, 13, 14, 14, 15};
    static const uint8_t ANG[3] = {124, 116, 108};
    uint32_t i, ss = astro_sign(c->lon[ASTRO_SUN]), k;
    uint8_t *b = (uint8_t *)p;
    for (i = 0; i < sizeof *p; i++) b[i] = 0;
    p->version = PATCH_VERSION;
    p->algo = (uint8_t)(1u + (variant * 5u) % (PATCH_NALGO - 1u));
    p->orb = 8; p->index = 92; p->ring = 84; p->orbit = 36; p->width = 104; p->warp = 0;
    p->lfo_rate = 62; p->bend = 2; p->volume = 112;
    p->cho_depth = 56; p->cho_mix = 44; p->dly_time = 78; p->dly_fb = 52; p->dly_mix = 26; p->rev_decay = 100; p->rev_damp = 64; p->rev_mix = 54;
    for (i = 0; i < FM_NOP; i++) {
        patch_op_t *o = &p->op[i];
        uint32_t sg = astro_sign(c->lon[i]), n = (sg + 12u - ss) % 12u + 1u, mo = chart_modality(sg), r = patch_rank[i];
        int dg = chart_dignity(i, sg), retro = c->sky.spd[i] < 0, v;
        o->coarse = (uint8_t)(i == ASTRO_SUN ? 1u : n);
        o->detune = (int8_t)(((int32_t)astro_sign_arcmin(c->lon[i]) - 900) / 30);
        o->wave = WAVE[chart_element(sg)];
        o->a = (uint8_t)(ENV[mo][0] + r * 3u); o->d = (uint8_t)(ENV[mo][1] + r * 2u); o->s = ENV[mo][2]; o->r = (uint8_t)(ENV[mo][3] + r * 2u);
        o->level = (uint8_t)(104 + 5 * dg);
        o->feedback = (uint8_t)((dg == 2 ? 70 : dg == 1 ? 50 : 0) + (retro ? 24 : 0));
        v = (i == ASTRO_SUN ? 127 : ANG[c->house[i] % 3u]) - ROLL[o->coarse] - (r < 3u ? 8 : 0);
        o->out = (uint8_t)(v < 0 ? 0 : v);
        o->vel = 48; o->ksc = (uint8_t)(24u + o->coarse * 3u);
        o->src[0] = o->src[1] = o->rsrc = (uint8_t)i;
    }
    k = variant & 3u;                           /* the slot's character on top of the chart */
    if (k == 1u) {                              /* slow: a pad */
        for (i = 0; i < FM_NOP; i++) { p->op[i].a = (uint8_t)(p->op[i].a + 34u); p->op[i].r = (uint8_t)(p->op[i].r + 22u); if (p->op[i].s < 90u) p->op[i].s = 90; }
        p->rev_mix = 78; p->cho_mix = 64;
    } else if (k == 2u) {                       /* struck: bells and plucks */
        for (i = 0; i < FM_NOP; i++) { p->op[i].a = 0; p->op[i].d = (uint8_t)(46u + patch_rank[i] * 4u); p->op[i].s = (uint8_t)(patch_rank[i] * 3u); p->op[i].r = (uint8_t)(p->op[i].d + 6u); }
        p->index = 104; p->dly_mix = 48;
    } else if (k == 3u) {                       /* moving: the sky runs a day a second and the operators follow */
        p->warp = 4; p->orbit = 100; p->rev_mix = 70;
        if (patch_algo_src(p->algo) == ALGO_NATAL) p->algo = (uint8_t)(p->algo + 12u);
    }
    patch_sanitize(p);
}

/* ---- a patch and the charts become the engine's program */
typedef struct { int32_t w[2]; uint8_t s[2], k[2]; int32_t rw; uint8_t rs, rk; int32_t fb; } cand_t;

static void add_pm(cand_t *c, unsigned src, int32_t w, unsigned kind) {
    int32_t aw = w < 0 ? -w : w, a0 = c->w[0] < 0 ? -c->w[0] : c->w[0], a1 = c->w[1] < 0 ? -c->w[1] : c->w[1];
    if (aw > a0) { c->w[1] = c->w[0]; c->s[1] = c->s[0]; c->k[1] = c->k[0]; c->w[0] = w; c->s[0] = (uint8_t)src; c->k[0] = (uint8_t)kind; }
    else if (aw > a1) { c->w[1] = w; c->s[1] = (uint8_t)src; c->k[1] = (uint8_t)kind; }
}

void patch_derive(const patch_t *p, const chart_t *natal, const chart_t *sky, fm_prog_t *out, patch_route_t *route) {
    static const uint16_t BEND[13] = {0, 1948, 4013, 6200, 8517, 10972, 13573, 16329, 19248, 22341, 25618, 29090, 32767};
    static chart_asp_t asp_buf[CHART_MAX_ASP];
    cand_t cand[FM_NOP];
    chart_asp_t *asp = route ? route->asp : asp_buf;
    uint32_t i, k, n = 0, src = patch_algo_src(p->algo), harm = patch_algo_harm(p->algo);
    const chart_t *ref = natal ? natal : sky, *pos = src == ALGO_SKY || !natal ? sky : natal, *from = sky, *to = sky;
    int32_t idx = (int32_t)fm_level(p->index), ring = (int32_t)p->ring * 258, vol = (int32_t)fm_level(p->volume), centre = 0, wsum = 0;
    uint8_t *ob = (uint8_t *)out;
    for (i = 0; i < sizeof *out; i++) ob[i] = 0;
    for (i = 0; i < FM_NOP; i++) { cand[i].w[0] = cand[i].w[1] = cand[i].rw = cand[i].fb = 0; cand[i].s[0] = cand[i].s[1] = cand[i].rs = (uint8_t)i; cand[i].k[0] = cand[i].k[1] = cand[i].rk = 0; }

    if (src == ALGO_FREE) {
        for (i = 0; i < FM_NOP; i++) {
            const patch_op_t *o = &p->op[i];
            for (k = 0; k < 2u; k++) { int32_t d = o->depth[k]; cand[i].s[k] = o->src[k]; cand[i].w[k] = d < 0 ? -(int32_t)fm_level((unsigned)-d) : (int32_t)fm_level((unsigned)d); }
            cand[i].rs = o->rsrc; cand[i].rw = (int32_t)o->ring * 258;
        }
    } else {
        if (src == ALGO_NATAL) from = to = ref;
        else if (src == ALGO_TRANSIT && natal) to = natal;
        n = chart_aspects(from, to, harm, (uint32_t)p->orb * 11930465u, ASTRO_NMAJOR, asp, CHART_MAX_ASP);
        for (k = 0; k < n; k++) {
            const chart_asp_t *a = &asp[k];
            uint32_t m = a->a, c = a->b;                          /* modulator, carrier */
            int32_t w = a->applying ? a->str : (a->str * 3) >> 2, wm, wc;
            if (from == to && patch_rank[m] > patch_rank[c]) { m = a->b; c = a->a; }
            /* a high-ratio modulator needs less depth for the same brightness; a retrograde one is inverted */
            wm = (w * (int32_t)(512u / (1u + p->op[m].coarse) > 256u ? 256u : 512u / (1u + p->op[m].coarse))) >> 8;
            wc = (w * (int32_t)(512u / (1u + p->op[c].coarse) > 256u ? 256u : 512u / (1u + p->op[c].coarse))) >> 8;
            if (from->sky.spd[m] < 0) wm = -wm;
            if (to->sky.spd[c] < 0) wc = -wc;
            switch (a->kind) {
            case ASTRO_CONJ: if (m == c) cand[c].fb += w; else add_pm(&cand[c], m, wm, a->kind); break;
            case ASTRO_SEXTILE: add_pm(&cand[c], m, (wm * 115) >> 8, a->kind); break;
            case ASTRO_TRINE: add_pm(&cand[c], m, (wm * 180) >> 8, a->kind); break;
            case ASTRO_SQUARE:
                if (w > cand[c].rw) { cand[c].rw = w; cand[c].rs = (uint8_t)m; cand[c].rk = a->kind; }
                add_pm(&cand[c], m, -(wm * 77) >> 8, a->kind); break;
            default:
                add_pm(&cand[c], m, (wm * 154) >> 8, a->kind);
                if (m != c) add_pm(&cand[m], c, -(wc * 154) >> 8, a->kind);
            }
        }
    }
    if (route) { route->nasp = (uint8_t)n; route->nedge = 0; }
    for (i = 0; i < FM_NOP; i++) {              /* where the sound's weight is on the wheel, left to right */
        int32_t g = p->op[i].mute ? 0 : (int32_t)fm_level(p->op[i].out) >> 4;
        centre += -(astro_cos_q30(pos->lon[i] - pos->h.asc) >> 16) * g; wsum += g;
    }
    centre = wsum ? centre / wsum : 0;
    for (i = 0; i < FM_NOP; i++) {
        const patch_op_t *o = &p->op[i];
        int32_t lvl = (int32_t)fm_level(o->level), u, g, pn, fb;
        astro_bam_t ang;
        out->ratio[i] = patch_ratio_q16(o);
        for (k = 0; k < 2u; k++) {
            int32_t w = src == ALGO_FREE ? cand[i].w[k] : (cand[i].w[k] * idx) >> 15;          /* Q15, signed */
            out->pms[k][i] = cand[i].s[k];
            out->pmd[k][i] = w * (FM_PM_MAX >> 15);
            if (route && w && route->nedge < 3u * FM_NOP) {
                int32_t aw = w < 0 ? -w : w;
                route->edge[route->nedge].src = cand[i].s[k]; route->edge[route->nedge].dst = (uint8_t)i; route->edge[route->nedge].kind = cand[i].k[k];
                route->edge[route->nedge].ring = 0; route->edge[route->nedge].depth = (uint8_t)(aw >> 7); route->edge[route->nedge].sign = (int8_t)(w < 0 ? -1 : 1);
                route->nedge++;
            }
        }
        u = src == ALGO_FREE ? cand[i].rw : (cand[i].rw * ring) >> 15;
        out->ams[i] = cand[i].rs; out->amd[i] = (uint16_t)(u > 32767 ? 32767 : u);
        if (route && u && route->nedge < 3u * FM_NOP) {
            route->edge[route->nedge].src = cand[i].rs; route->edge[route->nedge].dst = (uint8_t)i; route->edge[route->nedge].kind = cand[i].rk;
            route->edge[route->nedge].ring = 1; route->edge[route->nedge].depth = (uint8_t)(u >> 7); route->edge[route->nedge].sign = 1;
            route->nedge++;
        }
        fb = (int32_t)fm_level(o->feedback) + ((cand[i].fb * idx) >> 13);       /* a return: its own feedback */
        if (fb > 32767) fb = 32767;
        out->fbd[i] = fb * (FM_FB_MAX >> 15);
        /* the orbit: an operator is fullest when its planet stands on its birth place, quietest opposite it */
        u = astro_cos_q30(sky->sky.lon[i] - (natal ? natal->sky.lon[i] : 0u)) >> 15;          /* -32768..32768 */
        lvl -= (((lvl * (int32_t)p->orbit) >> 7) * ((32768 - u) >> 1)) >> 15;
        if (o->mute || lvl < 0) lvl = 0;
        out->level[i] = (uint16_t)lvl;
        /* the wheel is the stereo field: the ascendant to the left, the descendant to the right, and the whole
         * picture shifted so that its loudness sits in the middle (a chart with everything rising is not a mix) */
        pn = 16384 - (astro_cos_q30(pos->lon[i] - pos->h.asc) >> 16) - centre;                 /* 0 left .. 32768 right */
        pn = 16384 + (((pn - 16384) * (int32_t)p->width) >> 7);
        pn = pn < 0 ? 0 : pn > 32768 ? 32768 : pn;
        ang = (uint32_t)pn << 15;                                                                /* 0 .. a quarter turn */
        g = ((int32_t)fm_level(o->out) * vol) >> 15;                                             /* Q15 */
        out->gl[i] = (uint16_t)((g * (astro_cos_q30(ang) >> 15)) >> 20);                        /* Q10 */
        out->gr[i] = (uint16_t)((g * (astro_sin_q30(ang) >> 15)) >> 20);
        out->wave[i] = o->wave; out->vel[i] = o->vel; out->ksc[i] = o->ksc;
        out->ek[0][i] = fm_env_k(o->a); out->ek[1][i] = fm_env_k(o->d); out->ek[2][i] = fm_env_k(o->r);
        out->es[i] = (uint16_t)fm_level(o->s);
        if (route) { route->fb[i] = (uint8_t)(fb >> 7); route->pan[i] = (uint8_t)(pn > 32767 ? 255 : pn >> 7); route->lvl[i] = (uint8_t)(lvl >> 7); }
    }
    out->lfo_inc = ((155829u << (p->lfo_rate >> 4)) * (16u + (p->lfo_rate & 15u))) >> 4;
    out->lfo_pitch = (uint16_t)(((uint32_t)p->lfo_pitch * p->lfo_pitch * 1949u) / 16129u);
    out->lfo_amp = (uint16_t)(p->lfo_amp * 258u);
    out->bend = BEND[p->bend > 12u ? 12u : p->bend];
    out->gain = 8191;                           /* the voices sum quietly; the effects' input brings them up */
    out->cho_inc = 1870000u; out->cho_depth = (uint16_t)(p->cho_depth * 500u); out->cho_mix = (uint16_t)(p->cho_mix * 129u);
    out->dly_time = 441u + p->dly_time * 122u; out->dly_fb = (uint16_t)(p->dly_fb * 240u); out->dly_mix = (uint16_t)(p->dly_mix * 200u);
    out->rev_decay = (uint16_t)(16000u + p->rev_decay * 125u); out->rev_damp = (uint16_t)(4000u + p->rev_damp * 200u); out->rev_mix = (uint16_t)(p->rev_mix * 220u);
}
