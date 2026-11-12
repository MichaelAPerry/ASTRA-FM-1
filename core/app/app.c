/* SPDX-License-Identifier: GPL-3.0-only
 * ASTRA FM-1, the instrument (see app.h): clocks, the sky, presets, the panel. Integer only. */
#include <stddef.h>
#include "app_int.h"

app_t A;

/* ---- the knobs, page by page */
#define G(n, f, lo, hi) {n, PK_G, (uint8_t)offsetof(patch_t, f), lo, hi}
#define GS(n, f, lo, hi) {n, PK_GS, (uint8_t)offsetof(patch_t, f), lo, hi}
#define O(n, f, lo, hi) {n, PK_O, (uint8_t)offsetof(patch_op_t, f), lo, hi}
#define OS(n, f, lo, hi) {n, PK_OS, (uint8_t)offsetof(patch_op_t, f), lo, hi}
#define X(n, id) {n, PK_X, id, 0, 0}
static const app_par_t PARS[][4] = {
    /* 0 wheel */ {G("INDEX", index, 0, 127), G("RING", ring, 0, 127), G("ORB", orb, 1, 15), X("DAY", XP_DAY)},
    /* 1 operator */ {O("RATIO", coarse, 0, 24), O("FINE", fine, 0, 127), O("LEVEL", level, 0, 127), O("FEEDBACK", feedback, 0, 127)},
    {O("WAVE", wave, 0, 3), O("OUTPUT", out, 0, 127), OS("DETUNE", detune, -63, 63), O("VELOCITY", vel, 0, 127)},
    {O("SOURCE A", src[0], 0, 9), OS("DEPTH A", depth[0], -127, 127), O("SOURCE B", src[1], 0, 9), OS("DEPTH B", depth[1], -127, 127)},
    {O("RING SRC", rsrc, 0, 9), O("RING", ring, 0, 127), O("KEYSCALE", ksc, 0, 127), O("MUTE", mute, 0, 1)},
    /* 5 envelope */ {O("ATTACK", a, 0, 127), O("DECAY", d, 0, 127), O("SUSTAIN", s, 0, 127), O("RELEASE", r, 0, 127)},
    /* 6 motion */ {GS("WARP", warp, -9, 9), G("ORBIT", orbit, 0, 127), G("WIDTH", width, 0, 127), G("VOLUME", volume, 0, 127)},
    {G("LFO RATE", lfo_rate, 0, 127), G("VIBRATO", lfo_pitch, 0, 127), G("TREMOLO", lfo_amp, 0, 127), G("BEND", bend, 0, 12)},
    /* 8 effects */ {G("CHORUS", cho_depth, 0, 127), G("SPREAD", cho_mix, 0, 127), G("ECHO", dly_time, 0, 127), G("REPEAT", dly_fb, 0, 127)},
    {G("ECHO MIX", dly_mix, 0, 127), G("SPACE", rev_decay, 0, 127), G("DAMP", rev_damp, 0, 127), G("REVERB", rev_mix, 0, 127)},
    /* 10 horoscope */ {X("DAY", XP_DAY), X("MONTH", XP_MONTH), X("YEAR", XP_YEAR), X("ORB", XP_TORB)},
    /* 11 global */ {X("YEAR", XP_NOW_Y), X("MONTH", XP_NOW_MO), X("DAY", XP_NOW_D), X("HOUR", XP_NOW_H)},
    {X("YEAR", XP_B_Y), X("MONTH", XP_B_MO), X("DAY", XP_B_D), X("HOUR", XP_B_H)},
    {X("LATITUDE", XP_LAT), X("LONGITUDE", XP_LON), X("ZONE", XP_B_TZ), X("TIME", XP_KNOWN)},
    {X("HOUSES", XP_HSYS), X("ZODIAC", XP_ZODIAC), X("ZONE", XP_TZ), X("-", XP_NONE)},
};
static const uint8_t ROW0[APP_NPAGE] = {0, 1, 5, 6, 8, 10, 11}, NSUB[APP_NPAGE] = {1, 4, 1, 2, 2, 4, 4};
const app_par_t *app_pars(void) { return PARS[ROW0[A.page] + (A.page == APP_PG_HORO ? 0u : A.sub)]; }

static void say(const char *s) { unsigned i; for (i = 0; s[i] && i < sizeof A.msg - 1u; i++) A.msg[i] = s[i]; A.msg[i] = 0; A.msg_ms = A.ms; }
static void say_n(const char *s, unsigned n, const char *t) {
    unsigned i = 0;
    for (; *s && i < sizeof A.msg - 4u; s++) A.msg[i++] = *s;
    A.msg[i++] = (char)('0' + n / 10u % 10u); A.msg[i++] = (char)('0' + n % 10u);
    for (; *t && i < sizeof A.msg - 1u; t++) A.msg[i++] = *t;
    A.msg[i] = 0; A.msg_ms = A.ms;
}

/* ---- time */
static void norm(int32_t *days, uint32_t *sec, int32_t ds) {   /* add ds seconds, inside the supported range; */
    int32_t s = (int32_t)*sec + ds % 86400;
    *days += ds / 86400;
    if (s < 0) { s += 86400; (*days)--; }
    if (s >= 86400) { s -= 86400; (*days)++; }
    *sec = (uint32_t)s;
    if (*days < APP_DAYS_MIN) { *days = APP_DAYS_MIN; *sec = 0; }
    if (*days > APP_DAYS_MAX) { *days = APP_DAYS_MAX; *sec = 86399; }
}
void app_local(int32_t days, uint32_t sec, int tz_q, int32_t *y, uint32_t *mo, uint32_t *d, uint32_t *h, uint32_t *mi, uint32_t *s) {
    int32_t t = (int32_t)sec + tz_q * 900;
    while (t < 0) { t += 86400; days--; }
    while (t >= 86400) { t -= 86400; days++; }
    astro_civil_from_days(days, y, mo, d);
    *h = (uint32_t)t / 3600u; *mi = (uint32_t)t / 60u % 60u; *s = (uint32_t)t % 60u;
}
static uint32_t month_len(int32_t y, uint32_t m) {
    return (uint32_t)(astro_days_from_civil(m == 12u ? y + 1 : y, m == 12u ? 1u : m + 1u, 1u) - astro_days_from_civil(y, m, 1u));
}
/* move a UT instant by civil years, months, days, hours, minutes of the zone tz_q; 1 if it moved */
static int civil_move(int32_t *days, uint32_t *sec, int tz_q, int dy, int dmo, int dd, int dh, int dmi) {
    int32_t y, d0 = *days, nd;
    uint32_t mo, d, h, mi, s, s0 = *sec, ml;
    app_local(*days, *sec, tz_q, &y, &mo, &d, &h, &mi, &s);
    if (dy || dmo) {
        int32_t mm = (int32_t)mo - 1 + dmo;
        y += dy + (mm >= 0 ? mm / 12 : -((11 - mm) / 12));
        mo = (uint32_t)((mm % 12 + 12) % 12) + 1u;
        y = y < 1800 ? 1800 : y > 2200 ? 2200 : y;
        ml = month_len(y, mo);
        if (d > ml) d = ml;
    }
    nd = astro_days_from_civil(y, mo, d) + dd;
    *days = nd; *sec = 0;
    norm(days, sec, (int32_t)(h * 3600u + mi * 60u + s) + dh * 3600 + dmi * 60 - tz_q * 900);
    return *days != d0 || *sec != s0;
}

/* ---- parameters */
static uint8_t *par_ptr(const app_par_t *p) {
    return p->kind == PK_G || p->kind == PK_GS ? (uint8_t *)&A.patch + p->off : (uint8_t *)&A.patch.op[A.sel] + p->off;
}
int app_par_get(const app_par_t *p) {
    int32_t y; uint32_t mo, d, h, mi, s;
    if (p->kind == PK_G || p->kind == PK_O) return *par_ptr(p);
    if (p->kind == PK_GS || p->kind == PK_OS) return (int8_t)*par_ptr(p);
    switch (p->off) {
    case XP_DAY: case XP_MONTH: case XP_YEAR:
        app_local(A.sky_days, A.sky_sec, A.set.tz_q, &y, &mo, &d, &h, &mi, &s); return p->off == XP_DAY ? (int)d : p->off == XP_MONTH ? (int)mo : y;
    case XP_NOW_Y: case XP_NOW_MO: case XP_NOW_D: case XP_NOW_H:
        app_local(A.now_days, A.now_sec, A.set.tz_q, &y, &mo, &d, &h, &mi, &s);
        return p->off == XP_NOW_Y ? y : p->off == XP_NOW_MO ? (int)mo : p->off == XP_NOW_D ? (int)d : (int)h;
    case XP_TORB: return A.set.torb;
    case XP_B_Y: return A.set.birth.year;
    case XP_B_MO: return A.set.birth.month;
    case XP_B_D: return A.set.birth.day;
    case XP_B_H: return A.set.birth.hour;
    case XP_LAT: return A.set.birth.lat10;
    case XP_LON: return A.set.birth.lon10;
    case XP_B_TZ: return A.set.birth.tz_q;
    case XP_KNOWN: return A.set.birth.time_known;
    case XP_HSYS: return A.set.hsys;
    case XP_ZODIAC: return A.set.sidereal;
    case XP_TZ: return A.set.tz_q;
    default: return 0;
    }
}
unsigned app_par_norm(const app_par_t *p) {
    int v = app_par_get(p), lo = p->lo, hi = p->hi;
    if (p->kind == PK_X) switch (p->off) {
    case XP_DAY: case XP_NOW_D: case XP_B_D: lo = 1; hi = 31; break;
    case XP_MONTH: case XP_NOW_MO: case XP_B_MO: lo = 1; hi = 12; break;
    case XP_YEAR: case XP_NOW_Y: case XP_B_Y: lo = 1800; hi = 2200; break;
    case XP_NOW_H: case XP_B_H: lo = 0; hi = 23; break;
    case XP_TORB: lo = 1; hi = 10; break;
    case XP_LAT: lo = -900; hi = 900; break;
    case XP_LON: lo = -1800; hi = 1800; break;
    case XP_B_TZ: case XP_TZ: lo = -48; hi = 56; break;
    case XP_KNOWN: case XP_ZODIAC: lo = 0; hi = 1; break;
    case XP_HSYS: lo = 0; hi = ASTRO_NHSYS - 1; break;
    default: return 0;
    }
    return (unsigned)((v - lo) * 255 / (hi > lo ? hi - lo : 1));
}
static int clampi(int v, int lo, int hi, int *moved, int old) { v = v < lo ? lo : v > hi ? hi : v; *moved = v != old; return v; }
static int birth_touch(void) {
    chart_birth_t *b = &A.set.birth;
    uint32_t ml;
    if (!b->set) { b->set = 1; b->time_known = 1; }
    ml = month_len(b->year, b->month);
    if (b->day > ml) b->day = (uint8_t)ml;
    A.dirty |= DIRTY_NATAL | DIRTY_SET; A.set_ms = A.ms;
    return 1;
}
/* turn a knob by d detents; 0 if it was at its end (the dial then flashes) */
static int par_turn(const app_par_t *p, int d) {
    int moved = 0, v, acc = d * (d > 2 || d < -2 ? 20 : d > 1 || d < -1 ? 4 : 1);
    chart_birth_t *b = &A.set.birth;
    if (p->kind != PK_X) {
        uint8_t *q = par_ptr(p);
        v = p->kind == PK_GS || p->kind == PK_OS ? (int8_t)*q : *q;
        v = clampi(v + (p->hi - p->lo > 30 && (d > 2 || d < -2) ? d * 3 : d), p->lo, p->hi, &moved, v);
        *q = (uint8_t)v;
        if (moved) { A.dirty |= DIRTY_DERIVE; A.edited = 1; }
        return moved;
    }
    switch (p->off) {
    case XP_DAY: case XP_MONTH: case XP_YEAR:
        moved = civil_move(&A.sky_days, &A.sky_sec, A.set.tz_q, p->off == XP_YEAR ? d : 0, p->off == XP_MONTH ? d : 0, p->off == XP_DAY ? d : 0, 0, 0);
        if (moved) A.live = 0;
        return moved;
    case XP_NOW_Y: case XP_NOW_MO: case XP_NOW_D: case XP_NOW_H:
        moved = civil_move(&A.now_days, &A.now_sec, A.set.tz_q, p->off == XP_NOW_Y ? d : 0, p->off == XP_NOW_MO ? d : 0, p->off == XP_NOW_D ? d : 0, p->off == XP_NOW_H ? d : 0, 0);
        if (moved && A.clock == APP_T_SET) { A.dirty |= DIRTY_SET; A.set_ms = A.ms; }
        return moved;
    case XP_TORB: A.set.torb = (uint8_t)clampi(A.set.torb + d, 1, 10, &moved, A.set.torb); if (moved) { A.dirty |= DIRTY_SET | DIRTY_SKY; A.set_ms = A.ms; } return moved;
    case XP_B_Y: b->year = (int16_t)clampi((b->set ? b->year : 1990) + d, 1800, 2200, &moved, b->set ? b->year : 0); return moved && birth_touch();
    case XP_B_MO: b->month = (uint8_t)clampi((b->month ? b->month : 1) + d, 1, 12, &moved, b->set ? b->month : 0); return moved && birth_touch();
    case XP_B_D: b->day = (uint8_t)clampi((b->day ? b->day : 1) + d, 1, (int)month_len(b->year, b->month ? b->month : 1), &moved, b->set ? b->day : 0); return moved && birth_touch();
    case XP_B_H: b->hour = (uint8_t)clampi(b->hour + d, 0, 23, &moved, b->hour); if (moved) b->time_known = 1; return moved && birth_touch();
    case XP_LAT: b->lat10 = (int16_t)clampi(b->lat10 + acc, -900, 900, &moved, b->lat10); return moved && birth_touch();
    case XP_LON: b->lon10 = (int16_t)clampi(b->lon10 + acc, -1800, 1800, &moved, b->lon10); return moved && birth_touch();
    case XP_B_TZ: b->tz_q = (int16_t)clampi(b->tz_q + d, -48, 56, &moved, b->tz_q); return moved && birth_touch();
    case XP_KNOWN: v = b->time_known; b->time_known = (uint8_t)clampi(v + d, 0, 1, &moved, v); if (moved) { v = b->time_known; birth_touch(); b->time_known = (uint8_t)v; } return moved;
    case XP_HSYS: A.set.hsys = (uint8_t)clampi(A.set.hsys + d, 0, ASTRO_NHSYS - 1, &moved, A.set.hsys); break;
    case XP_ZODIAC: A.set.sidereal = (uint8_t)clampi(A.set.sidereal + d, 0, 1, &moved, A.set.sidereal); break;
    case XP_TZ: A.set.tz_q = (int16_t)clampi(A.set.tz_q + d, -48, 56, &moved, A.set.tz_q); break;
    default: return 0;
    }
    if (moved) { A.dirty |= DIRTY_NATAL | DIRTY_SKY | DIRTY_SET; A.set_ms = A.ms; }
    return moved;
}

/* ---- presets: 8 banks of 8; a slot never saved is cast from the chart, each with its own character */
static void bank_get(unsigned bank) {
    unsigned i;
    if (A.bank_no == bank) return;
    if (!A.st || !A.st->load || A.st->load(1u + bank, &A.bank, sizeof A.bank) || A.bank.magic != APP_BANK_MAGIC) { A.bank.magic = APP_BANK_MAGIC; A.bank.valid = 0; }
    for (i = 0; i < 8u; i++) if (A.bank.valid >> i & 1u) patch_sanitize(&A.bank.p[i]);
    A.bank_no = (uint8_t)bank;
}
static void slot_load(unsigned slot) {
    A.set.slot = (uint8_t)(slot % APP_NSLOT);
    bank_get(A.set.slot / 8u);
    if (A.bank.valid >> (A.set.slot & 7u) & 1u) A.patch = A.bank.p[A.set.slot & 7u];
    else patch_cast(&A.patch, A.has_natal ? &A.natal : &A.sky, A.set.slot);
    A.edited = 0; A.dirty |= DIRTY_DERIVE; A.save_arm = 0;
    if (A.patch.warp) A.live = 0;
}
static void slot_save(void) {
    bank_get(A.set.slot / 8u);
    A.bank.p[A.set.slot & 7u] = A.patch; A.bank.valid |= 1u << (A.set.slot & 7u);
    if (A.st && A.st->save) A.st->save(1u + A.bank_no, &A.bank, sizeof A.bank);
    A.saves++; A.edited = 0; A.save_pending = 0;
    say_n("SAVED TO ", A.set.slot + 1u, "");
}
static void settings_save(void) {
    A.set.magic = APP_SET_MAGIC; A.set.days = A.now_days; A.set.sec = A.now_sec; A.set.clock_saved = A.clock != APP_T_NOT_SET;
    if (A.st && A.st->save) A.st->save(0, &A.set, sizeof A.set);
    A.dirty &= (uint8_t)~DIRTY_SET; A.save_ms = A.ms; A.saves++;
}

/* ---- notes */
static unsigned key_to_note(unsigned k) { int n = 53 + (int)k + 12 * (A.octave + A.patch.octave); return (unsigned)(n < 0 ? 0 : n > 127 ? 127 : n); }
void app_key(unsigned k, int down) {
    if (k >= 27u) return;
    if (down) {
        A.keys_down |= 1u << k;
        if (A.latch && (A.keys_latched >> k & 1u)) { A.keys_latched &= ~(1u << k); fm_note_off(A.key_note[k]); return; }
        if (A.keys_latched >> k & 1u) fm_note_off(A.key_note[k]);
        A.key_note[k] = (uint8_t)key_to_note(k);
        fm_note_on(A.key_note[k], 100);
        if (A.latch) A.keys_latched |= 1u << k;
    } else {
        A.keys_down &= ~(1u << k);
        if (!(A.keys_latched >> k & 1u)) fm_note_off(A.key_note[k]);
    }
}
void app_midi(unsigned st, unsigned d1, unsigned d2) {
    switch (st & 0xF0u) {
    case 0x90: if (d2) { fm_note_on(d1, d2); break; }   /* (velocity 0: a note off) */
    /* fall through */
    case 0x80: fm_note_off(d1); break;
    case 0xE0: fm_bend((int)(d1 | d2 << 7) - 8192); break;
    case 0xB0:
        if (d1 == 1u) fm_modwheel(d2);
        else if (d1 == 64u) fm_sustain(d2 >= 64u);
        else if (d1 == 120u || d1 == 123u) fm_all_off();
        break;
    case 0xC0: slot_load(d1 % APP_NSLOT); break;
    default: break;
    }
}
void app_master(unsigned v) { A.master = (uint16_t)(v > 1023u ? 1023u : v); }

/* ---- the panel */
void app_encoder(unsigned e, int d) {
    int moved = 1;
    if (!d || e >= APP_NENC) return;
    if (e >= APP_ENC_K1 && e <= APP_ENC_K4) moved = par_turn(&app_pars()[e - APP_ENC_K1], d);
    else if (e == APP_ENC_ALGORITHM) {
        A.patch.algo = (uint8_t)(((int)A.patch.algo + d % (int)PATCH_NALGO + (int)PATCH_NALGO) % (int)PATCH_NALGO);
        A.dirty |= DIRTY_DERIVE; A.edited = 1;
    } else if (e == APP_ENC_PRESETS) slot_load((unsigned)(((int)A.set.slot + d % (int)APP_NSLOT + (int)APP_NSLOT) % (int)APP_NSLOT)), A.dirty |= DIRTY_SET, A.set_ms = A.ms;
    else if (A.page == APP_PG_GLOBAL && A.sub == 0u) {
        moved = civil_move(&A.now_days, &A.now_sec, A.set.tz_q, 0, 0, 0, 0, d);
        if (moved && A.clock == APP_T_SET) { A.dirty |= DIRTY_SET; A.set_ms = A.ms; }
    } else if (A.page == APP_PG_GLOBAL && A.sub == 1u) {
        int v = A.set.birth.minute;
        A.set.birth.minute = (uint8_t)((v + d % 60 + 60) % 60); A.set.birth.time_known = 1; birth_touch(); A.set.birth.time_known = 1;
    } else if (A.page == APP_PG_HORO && A.sub < 2u) {
        if (A.ntr) A.tr_sel = (uint8_t)(((int)A.tr_sel + d % (int)A.ntr + (int)A.ntr) % (int)A.ntr); else moved = 0;
    } else A.sel = (uint8_t)(((int)A.sel + d % 10 + 10) % 10);
    if (!moved) { A.bump_ms[e] = A.ms | 1u; A.bumps[e]++; }
}

static void cast_now(void) {
    patch_cast(&A.patch, A.has_natal ? &A.natal : &A.sky, A.set.slot);
    A.edited = 1; A.dirty |= DIRTY_DERIVE;
    say(A.has_natal ? "CAST FROM THE BIRTH CHART" : "CAST FROM THE SKY");
}
void app_button(unsigned b, int down) {
    static const uint8_t PAGE_OF[APP_NBTN] = {255, 255, APP_PG_FX, 255, APP_PG_ENV, APP_PG_MOTION, APP_PG_OP, APP_PG_GLOBAL, APP_PG_WHEEL, 255, 255, APP_PG_HORO, 255, 255};
    unsigned k;
    if (!down || b >= APP_NBTN) return;
    if (b != APP_BTN_SAVE) A.save_arm = 0;
    if (PAGE_OF[b] != 255u) {
        if (A.page == PAGE_OF[b]) A.sub = (uint8_t)((A.sub + 1u) % NSUB[A.page]); else { A.page = PAGE_OF[b]; A.sub = 0; }
        return;
    }
    switch (b) {
    case APP_BTN_OCT_DOWN: if (A.octave > -2) A.octave--; say_n("OCTAVE ", (unsigned)(A.octave + 2), " OF 04"); break;
    case APP_BTN_OCT_UP: if (A.octave < 2) A.octave++; say_n("OCTAVE ", (unsigned)(A.octave + 2), " OF 04"); break;
    case APP_BTN_SEL: A.patch.op[A.sel].mute ^= 1u; A.edited = 1; A.dirty |= DIRTY_DERIVE; say(A.patch.op[A.sel].mute ? "OPERATOR MUTED" : "OPERATOR SOUNDS"); break;
    case APP_BTN_ARP:
        A.latch ^= 1u;
        if (!A.latch) { for (k = 0; k < 27u; k++) if ((A.keys_latched >> k & 1u) && !(A.keys_down >> k & 1u)) fm_note_off(A.key_note[k]); A.keys_latched = 0; }
        say(A.latch ? "LATCH: KEYS HOLD" : "LATCH OFF");
        break;
    case APP_BTN_PLAY:
        if (A.page == APP_PG_GLOBAL && A.sub == 0u) {      /* this is now */
            A.clock = APP_T_SET; A.now_ms = A.ms; A.live = 1; A.run = 1; A.dirty |= DIRTY_SET; A.set_ms = A.ms - 2000u; say("CLOCK SET");
        } else { A.run ^= 1u; say(A.run ? "THE SKY MOVES" : "THE SKY HOLDS"); }
        break;
    case APP_BTN_REC:
        if (A.page == APP_PG_GLOBAL && A.sub) cast_now();
        else { A.live = 1; A.run = 1; A.sky_days = A.now_days; A.sky_sec = A.now_sec; say(A.clock == APP_T_NOT_SET ? "SKY AT THE CHART TIME" : "SKY AT NOW"); }
        break;
    case APP_BTN_SAVE:
        if (!A.save_arm) { A.save_arm = 1; say_n("SAVE TO ", A.set.slot + 1u, "? PRESS SAVE"); }
        else { A.save_arm = 0; A.save_pending = 1; A.save_ms = A.ms; }
        break;
    default: break;
    }
}
uint32_t app_leds(void) {
    static const uint8_t BTN_OF[APP_NPAGE] = {APP_BTN_HOME, APP_BTN_EDIT, APP_BTN_ENV, APP_BTN_LFO, APP_BTN_FX, APP_BTN_SEQ, APP_BTN_GLO};
    uint32_t l = 1u << BTN_OF[A.page];
    if (A.latch) l |= 1u << APP_BTN_ARP;
    if (A.run) l |= 1u << APP_BTN_PLAY;
    if (A.live) l |= 1u << APP_BTN_REC;
    if (A.save_arm || (A.edited && (A.ms >> 9 & 1u))) l |= 1u << APP_BTN_SAVE;
    if (A.patch.op[A.sel].mute) l |= 1u << APP_BTN_SEL;
    if (A.octave < 0) l |= 1u << APP_BTN_OCT_DOWN;
    if (A.octave > 0) l |= 1u << APP_BTN_OCT_UP;
    return l;
}
uint32_t app_key_leds(void) { return A.keys_down | A.keys_latched; }

/* ---- start, and every pass of the main loop */
void app_init(const app_store_t *st, uint32_t ms) {
    uint8_t *b = (uint8_t *)&A;
    uint32_t i;
    for (i = 0; i < sizeof A; i++) b[i] = 0;
    A.st = st; A.ms = A.now_ms = ms; A.bank_no = 255; A.run = 1; A.live = 1;
    if (!st || !st->load || st->load(0, &A.set, sizeof A.set) || A.set.magic != APP_SET_MAGIC || A.set.days < APP_DAYS_MIN
        || A.set.days > APP_DAYS_MAX || A.set.sec >= 86400u || A.set.hsys >= ASTRO_NHSYS || A.set.slot >= APP_NSLOT) {
        for (b = (uint8_t *)&A.set, i = 0; i < sizeof A.set; i++) b[i] = 0;
        A.set.torb = 6; A.set.birth.year = 1990; A.set.birth.month = 1; A.set.birth.day = 1; A.set.birth.hour = 12;
    }
    if (A.set.torb < 1u || A.set.torb > 10u) A.set.torb = 6;
    /* the clock: a saved time comes back unconfirmed (the FM-1 has no battery clock: the gap is unknown);
     * with nothing saved the time is J2000.0, a chart time that stands still and is never called now */
    if (A.set.clock_saved) { A.now_days = A.set.days; A.now_sec = A.set.sec; A.clock = APP_T_UNCONFIRMED; }
    else { A.now_days = 10957; A.now_sec = 43200; A.clock = APP_T_NOT_SET; }
    A.sky_days = A.now_days; A.sky_sec = A.now_sec; A.calc_days = A.sky_days + 1;
    A.has_natal = (uint8_t)chart_natal(&A.natal, &A.set.birth, A.set.hsys, A.set.sidereal);
    chart_cast(&A.sky, A.sky_days, A.sky_sec, 0, 0, 0, A.set.hsys, A.set.sidereal);
    A.calc_days = A.sky_days; A.calc_sec = A.sky_sec; A.calc_ms = ms;
    slot_load(A.set.slot);
    A.dirty = DIRTY_DERIVE | DIRTY_SKY;
}

static void transits(void) {
    A.ntr = (uint8_t)chart_aspects(&A.sky, A.has_natal ? &A.natal : &A.sky, 1, (uint32_t)A.set.torb * 11930465u, ASTRO_NMAJOR, A.tr, 12);
    if (A.tr_sel >= A.ntr) A.tr_sel = 0;
}

void app_tick(uint32_t ms) {
    uint32_t dt = ms - A.ms, el;
    A.ms = ms;
    /* the clock */
    if (A.clock == APP_T_NOT_SET) A.now_ms = ms;
    else if ((el = (ms - A.now_ms) / 1000u) != 0u) { A.now_ms += el * 1000u; norm(&A.now_days, &A.now_sec, (int32_t)el); }
    /* the sky's own clock: on now, or running at the patch's warp from wherever it was put */
    if (A.patch.warp) A.live = 0;
    if (A.live) { A.sky_days = A.now_days; A.sky_sec = A.now_sec; A.sky_rem = 0; }
    else if (A.run && dt && dt < 2000u) {
        int32_t rate = patch_warp_sec(A.patch.warp), ar = rate < 0 ? -rate : rate;
        int32_t acc = (ar % 1000) * (int32_t)dt + A.sky_rem, ds = (ar / 1000) * (int32_t)dt + acc / 1000, d0 = A.sky_days;
        uint32_t s0 = A.sky_sec;
        A.sky_rem = acc % 1000;
        norm(&A.sky_days, &A.sky_sec, rate < 0 ? -ds : ds);
        if (ds && A.sky_days == d0 && A.sky_sec == s0) { A.run = 0; say("THE END OF THE EPHEMERIS"); }
    }
    /* the charts */
    if (A.dirty & DIRTY_NATAL) {
        A.has_natal = (uint8_t)chart_natal(&A.natal, &A.set.birth, A.set.hsys, A.set.sidereal);
        A.dirty = (uint8_t)((A.dirty & ~DIRTY_NATAL) | DIRTY_SKY); A.tl_ok = 0; A.tl_fill = 0;
    }
    if ((A.dirty & DIRTY_SKY) || ((A.sky_days != A.calc_days || A.sky_sec != A.calc_sec) && ms - A.calc_ms >= 40u)) {
        chart_cast(&A.sky, A.sky_days, A.sky_sec, 0, 0, 0, A.set.hsys, A.set.sidereal);
        A.calc_days = A.sky_days; A.calc_sec = A.sky_sec; A.calc_ms = ms;
        transits();
        A.dirty = (uint8_t)((A.dirty & ~DIRTY_SKY) | DIRTY_DERIVE);
    }
    if (A.dirty & DIRTY_DERIVE) {
        fm_prog_t *g = fm_prog_begin();
        if (g) { patch_derive(&A.patch, A.has_natal ? &A.natal : 0, &A.sky, &A.prog, &A.route); *g = A.prog; fm_prog_commit(); A.dirty &= (uint8_t)~DIRTY_DERIVE; }
    }
    /* the timeline: a day's sky a pass, 30 days either side of the sky's day */
    if (A.page == APP_PG_HORO && A.sub == 1u) {
        if (!A.tl_ok || (A.tl_days != A.sky_days && A.tl_fill >= APP_TL_N)) { A.tl_fill = 0; A.tl_days = A.sky_days; A.tl_ok = 1; }   /* (a running sky: finish one month before starting the next) */
        if (A.tl_fill < APP_TL_N) {
            static chart_t c;
            int32_t dz = A.tl_days + (int32_t)A.tl_fill - 30;
            uint32_t i;
            chart_cast(&c, dz < APP_DAYS_MIN ? APP_DAYS_MIN : dz > APP_DAYS_MAX ? APP_DAYS_MAX : dz, A.sky_sec, 0, 0, 0, ASTRO_H_WHOLE, 0);
            for (i = 0; i < ASTRO_NPLANET; i++) A.tl_lon[A.tl_fill][i] = c.sky.lon[i];
            A.tl_fill++;
        }
    }
    /* saves wait for silence (an erase stops the sound for 50 ms), but not for ever */
    if (A.save_pending && (!fm_voices || ms - A.save_ms > 3000u)) slot_save();
    if (A.clock == APP_T_SET && ms - A.save_ms > 3600000u) A.dirty |= DIRTY_SET;       /* the running clock, hourly */
    if ((A.dirty & DIRTY_SET) && ms - A.set_ms >= 2000u && (!fm_voices || ms - A.set_ms > 8000u)) settings_save();
}

void app_info(app_info_t *o) {
    uint32_t i;
    o->page = A.page; o->sub = A.sub; o->sel = A.sel; o->slot = A.set.slot; o->algo = A.patch.algo; o->clock = A.clock;
    o->live = A.live; o->run = A.run; o->edited = A.edited; o->nedge = A.route.nedge; o->ntransit = A.ntr; o->latch = A.latch; o->saves = A.saves;
    o->sky_days = A.calc_days; o->sky_sec = A.calc_sec; o->now_days = A.now_days; o->now_sec = A.now_sec;
    for (i = 0; i < ASTRO_NBODY; i++) o->lon[i] = A.sky.sky.lon[i];
    o->asc = A.has_natal ? A.natal.h.asc : A.sky.h.asc;
}
const patch_t *app_patch(void) { return &A.patch; }
