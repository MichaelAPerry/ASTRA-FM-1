/* SPDX-License-Identifier: GPL-3.0-only
 * Host tests of core/fm: pitch, envelopes, voice handling, the matrix, determinism, and a stress run with random
 * programs (built with the undefined-behaviour sanitizer where the compiler has one: an overflow in the audio path
 * fails the test). `test_fm --wav FILE` also writes what it rendered. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "core/fm/fm.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define BLOCK 256
static int32_t buf[2 * BLOCK];
static FILE *wav;
static double peak, sumsq;
static long frames, maxstep;

static void render(int blocks, int32_t *keep, int nkeep) {     /* keep: the left channel of the last nkeep frames */
    int b, i, got = 0;
    for (b = 0; b < blocks; b++) {
        memset(buf, 0, sizeof buf);
        fm_render(buf, BLOCK);
        for (i = 0; i < BLOCK; i++) {
            static long last;
            long l = buf[2 * i], r = buf[2 * i + 1];
            if (labs(l) > peak) peak = (double)labs(l);
            if (labs(r) > peak) peak = (double)labs(r);
            if (labs(l - last) > maxstep) maxstep = labs(l - last);
            last = l; sumsq += (double)l * l; frames++;
            if (wav) { int16_t s[2] = {(int16_t)(l > 32767 ? 32767 : l < -32767 ? -32767 : l), (int16_t)(r > 32767 ? 32767 : r < -32767 ? -32767 : r)}; fwrite(s, 2, 2, wav); }
            if (keep && blocks * BLOCK - (b * BLOCK + i) <= nkeep) keep[got++] = buf[2 * i];
        }
    }
}
static void stat_reset(void) { peak = sumsq = 0; frames = 0; maxstep = 0; }
static double rms(void) { return sqrt(sumsq / (frames ? frames : 1)); }
static double goertzel(const int32_t *x, int n, double f) {   /* amplitude of f in x (Hann window) */
    const double PI = 3.14159265358979323846;
    double w = 2 * PI * f / FM_FS, c = 2 * cos(w), s0, s1 = 0, s2 = 0; int i;
    for (i = 0; i < n; i++) { s0 = x[i] * (0.5 - 0.5 * cos(2 * PI * i / n)) + c * s1 - s2; s2 = s1; s1 = s0; }
    return sqrt(s1 * s1 + s2 * s2 - c * s1 * s2) * 4 / n;
}

/* a plain program: every operator silent but those the test turns on */
static void base(fm_prog_t *p) {
    unsigned i;
    memset(p, 0, sizeof *p);
    for (i = 0; i < FM_NOP; i++) {
        p->ratio[i] = 1u << 16; p->ek[0][i] = fm_env_k(0); p->ek[1][i] = fm_env_k(60); p->ek[2][i] = fm_env_k(30);
        p->es[i] = 32767; p->pms[0][i] = p->pms[1][i] = p->ams[i] = (uint8_t)i;
    }
    p->gain = 4096; p->bend = 4013; p->dly_time = 4000; p->rev_decay = 26000; p->rev_damp = 16000;
}
static void commit(const fm_prog_t *src) {
    fm_prog_t *p = fm_prog_begin();
    if (!p) { memset(buf, 0, sizeof buf); fm_render(buf, BLOCK); p = fm_prog_begin(); }   /* (the interrupt takes the last one) */
    CHECK(p != 0, "no program buffer"); if (!p) exit(1);
    *p = *src; fm_prog_commit();
}
static void silence(void) { fm_all_off(); render(700, 0, 0); }

int main(int argc, char **argv) {
    static int32_t x[8192], ref[4096];
    fm_prog_t p;
    unsigned i, k;
    double a, f;
    if (argc == 3 && !strcmp(argv[1], "--wav")) {
        static const unsigned char hdr[44] = {'R','I','F','F',0xff,0xff,0xff,0x7f,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,2,0,
            0x44,0xac,0,0,0x10,0xb1,2,0,4,0,16,0,'d','a','t','a',0xff,0xff,0xff,0x7f};
        wav = fopen(argv[2], "wb"); if (wav) fwrite(hdr, 1, 44, wav);
    }
    fm_init();
    CHECK(fm_wave(0, 0x40000000u) > 32700 && fm_wave(0, 0xC0000000u) < -32700 && fm_wave(0, 0) == 0, "sine table");
    for (k = 0; k < FM_NWAVE; k++) { int32_t mx = 0; for (i = 0; i < 4096; i++) { int32_t v = abs(fm_wave(k, i << 20)); if (v > mx) mx = v; }
        CHECK(mx > 32000 && mx <= 32767, "wave %u peak %d", k, (int)mx); }
    CHECK(fm_level(127) == 32767 && fm_level(0) == 0 && fm_level(115) > 16000 && fm_level(115) < 16800, "level table");

    /* 1. silence before a note; one sine carrier at A4 = 440 Hz, level right, both sides */
    base(&p); p.level[0] = 32767; p.gl[0] = p.gr[0] = 512; commit(&p);
    stat_reset(); render(4, 0, 0); CHECK(peak == 0, "sound with no note (peak %g)", peak);
    fm_note_on(69, 127); render(40, x, 8192);
    f = 440.0; a = goertzel(x, 8192, f);
    CHECK(fabs(a - 16383) < 200, "A4 amplitude %.0f, want 16383 (level 1.0, gain 0.5)", a);
    CHECK(goertzel(x, 8192, 466.16) < 60 && goertzel(x, 8192, 880) < 60, "energy off the pitch: %.0f %.0f", goertzel(x, 8192, 466.16), goertzel(x, 8192, 880));
    /* 2. release: it ends and the voice goes idle */
    fm_note_off(69); stat_reset(); render(300, 0, 0); stat_reset(); render(4, 0, 0);
    CHECK(peak == 0 && fm_voices == 0, "no silence after release (peak %g, voices %u)", peak, fm_voices);
    /* 3. ratio: operator 3 at 3x */
    base(&p); p.level[3] = 32767; p.gl[3] = 512; p.gr[3] = 0; p.ratio[3] = 3u << 16; commit(&p);
    fm_note_on(57, 100); stat_reset(); render(40, x, 8192);
    CHECK(goertzel(x, 8192, 660.0) > 15000 && goertzel(x, 8192, 220.0) < 60, "ratio 3: %.0f at 660, %.0f at 220", goertzel(x, 8192, 660.0), goertzel(x, 8192, 220.0));
    silence();
    /* 4. phase modulation: op 1 (ratio 1) into op 0. Index I gives sidebands J_n(I): at I = 2.405 the carrier vanishes */
    base(&p); p.level[0] = 32767; p.gl[0] = p.gr[0] = 512; p.level[1] = 32767; p.pms[0][0] = 1; p.ratio[0] = 4u << 16;
    p.pmd[0][0] = (int32_t)(2.405 / 6.283185307 * 4294967296.0 / 32767.0); commit(&p);
    fm_note_on(45, 127); render(60, x, 8192);                  /* 110 Hz: carrier 440, sidebands 330 550 */
    CHECK(goertzel(x, 8192, 440.0) < 400, "PM: carrier %.0f at the first Bessel zero", goertzel(x, 8192, 440.0));
    a = goertzel(x, 8192, 550.0);
    CHECK(fabs(a - 16383 * 0.5191) < 400, "PM: first sideband %.0f, want %.0f (J1(2.405))", a, 16383 * 0.5191);
    silence();
    /* 5. ring modulation: carrier 440 x source 110 at full ring: 330 and 550, no 440 */
    base(&p); p.level[0] = 32767; p.gl[0] = p.gr[0] = 512; p.level[1] = 32767; p.ratio[0] = 4u << 16; p.ams[0] = 1; p.amd[0] = 32767; commit(&p);
    fm_note_on(45, 127); render(60, x, 8192);
    CHECK(goertzel(x, 8192, 440.0) < 300 && fabs(goertzel(x, 8192, 330.0) - 8191) < 300, "ring: %.0f at 440, %.0f at 330", goertzel(x, 8192, 440.0), goertzel(x, 8192, 330.0));
    silence();
    /* 6. feedback: a sine becomes saw-like: the 2nd harmonic appears, and it stays bounded */
    base(&p); p.level[0] = 32767; p.gl[0] = p.gr[0] = 512; p.fbd[0] = 10000; commit(&p);
    fm_note_on(57, 127); stat_reset(); render(60, x, 8192);
    CHECK(goertzel(x, 8192, 440.0) > 2000 && peak < 17000, "feedback: 2nd harmonic %.0f, peak %g", goertzel(x, 8192, 440.0), peak);
    silence();
    /* 7. a cross-modulating pair (0 <-> 1): any loop is one sample long; bounded, not silent */
    base(&p); p.level[0] = p.level[1] = 32767; p.gl[0] = p.gr[1] = 512; p.pms[0][0] = 1; p.pms[0][1] = 0;
    p.pmd[0][0] = 60000; p.pmd[0][1] = -60000; p.ratio[1] = 0x18000; commit(&p);
    fm_note_on(57, 127); stat_reset(); render(60, 0, 0);
    CHECK(rms() > 3000 && peak < 20000, "cross modulation: rms %.0f peak %g", rms(), peak);
    silence();
    /* 8. envelope: slow attack rises; sustain level holds */
    base(&p); p.level[0] = 32767; p.gl[0] = p.gr[0] = 512; p.ek[0][0] = fm_env_k(70); p.es[0] = 16384; p.ek[1][0] = fm_env_k(40); commit(&p);
    fm_note_on(69, 127); stat_reset(); render(2, 0, 0); a = peak; stat_reset(); render(20, 0, 0);
    CHECK(a < peak && a < 6000, "slow attack: %.0f then %g", a, peak);
    render(400, x, 8192); a = goertzel(x, 8192, 440.0);
    CHECK(fabs(a - 8191) < 250, "sustain at half: %.0f", a);
    silence();
    /* 9. velocity and key scaling */
    base(&p); p.level[0] = 32767; p.gl[0] = p.gr[0] = 512; p.vel[0] = 127; commit(&p);
    fm_note_on(69, 32); render(40, x, 8192); a = goertzel(x, 8192, 440.0);
    CHECK(a > 3000 && a < 6000, "velocity 32 at full sensitivity: %.0f", a);
    silence();
    /* 10. polyphony and stealing: more notes than voices never breaks; the same note retriggers its voice */
    base(&p); p.level[0] = 20000; p.gl[0] = p.gr[0] = 300; commit(&p);
    for (i = 0; i < FM_NVOICE + 3; i++) fm_note_on(48 + 3 * i, 100);
    stat_reset(); render(20, 0, 0);
    CHECK(fm_voices == FM_NVOICE && peak < 32767, "stealing: %u voices, peak %g", fm_voices, peak);
    fm_note_on(48 + 3 * (FM_NVOICE + 2), 100); render(2, 0, 0); CHECK(fm_voices == FM_NVOICE, "retrigger");
    fm_sustain(1); for (i = 0; i < FM_NVOICE + 3; i++) fm_note_off(48 + 3 * i);
    render(100, 0, 0); CHECK(fm_voices == FM_NVOICE, "sustain pedal holds");
    fm_sustain(0); render(400, 0, 0); CHECK(fm_voices == 0, "pedal up releases");
    /* 11. pitch bend: up two semitones (range 0.12246) */
    base(&p); p.level[0] = 32767; p.gl[0] = p.gr[0] = 512; commit(&p);
    fm_bend(8191); fm_note_on(69, 127); render(40, x, 8192);
    CHECK(goertzel(x, 8192, 493.88) > 15000, "bend up: %.0f at B4", goertzel(x, 8192, 493.88));
    fm_bend(-8192); render(40, x, 8192);
    CHECK(goertzel(x, 8192, 392.0) > 15000, "bend down: %.0f at G4", goertzel(x, 8192, 392.0));
    fm_bend(0); silence();
    /* 12. a level change glides: no step bigger than the tone's own slope allows */
    base(&p); p.level[0] = 8000; p.gl[0] = p.gr[0] = 512; commit(&p);
    fm_note_on(45, 127); render(40, 0, 0);
    p.level[0] = 32767; commit(&p); stat_reset(); render(8, 0, 0);
    CHECK(maxstep < 600, "level jump: step %ld", maxstep);
    silence();
    /* 13. determinism: the same notes after the same silence give the same samples */
    for (k = 0; k < 2; k++) {
        base(&p); for (i = 0; i < FM_NOP; i++) { p.level[i] = 26000; p.gl[i] = (uint16_t)(60 * i); p.gr[i] = (uint16_t)(540 - 60 * i); p.ratio[i] = (i + 1u) << 16;
            p.pms[0][i] = (uint8_t)((i + 1) % FM_NOP); p.pmd[0][i] = 30000; p.pms[1][i] = (uint8_t)((i + 7) % FM_NOP); p.pmd[1][i] = -20000; p.ams[i] = (uint8_t)((i + 3) % FM_NOP); p.amd[i] = 9000; p.fbd[i] = 3000; p.wave[i] = (uint8_t)(i & 3); }
        p.gain = 1500; p.cho_mix = 0; p.dly_mix = 0; p.rev_mix = 0; commit(&p);
        fm_note_on(50, 100); fm_note_on(57, 90); render(24, k ? x : ref, 4096);
        fm_all_off(); render(3000, 0, 0);       /* (long: every effect tail to zero) */
    }
    CHECK(!memcmp(x, ref, 4096 * sizeof x[0]), "two identical runs differ");
    /* 14. stress: random programs and notes, full polyphony, effects on; bounded output, no stuck state */
    srand(7);
    stat_reset();
    for (k = 0; k < 300; k++) {
        base(&p);
        for (i = 0; i < FM_NOP; i++) {
            p.ratio[i] = (uint32_t)(rand() % (32 << 16)) + 4096u; p.level[i] = (uint16_t)(rand() % 32768); p.gl[i] = (uint16_t)(rand() % 1025); p.gr[i] = (uint16_t)(rand() % 1025);
            p.pms[0][i] = (uint8_t)(rand() % FM_NOP); p.pms[1][i] = (uint8_t)(rand() % FM_NOP); p.ams[i] = (uint8_t)(rand() % FM_NOP);
            p.pmd[0][i] = rand() % (2 * FM_PM_MAX) - FM_PM_MAX; p.pmd[1][i] = rand() % (2 * FM_PM_MAX) - FM_PM_MAX; p.fbd[i] = rand() % (2 * FM_FB_MAX) - FM_FB_MAX;
            p.amd[i] = (uint16_t)(rand() % 32768); p.wave[i] = (uint8_t)(rand() % 4); p.vel[i] = (uint8_t)(rand() % 128); p.ksc[i] = (uint8_t)(rand() % 128);
            p.ek[0][i] = fm_env_k((unsigned)rand() % 128); p.ek[1][i] = fm_env_k((unsigned)rand() % 128); p.ek[2][i] = fm_env_k((unsigned)rand() % 128); p.es[i] = (uint16_t)(rand() % 32768);
        }
        p.gain = (uint16_t)(rand() % 8192); p.lfo_inc = (uint32_t)rand() * 16u; p.lfo_pitch = (uint16_t)(rand() % 4000); p.lfo_amp = (uint16_t)(rand() % 32768);
        p.cho_inc = (uint32_t)rand(); p.cho_depth = (uint16_t)rand(); p.cho_mix = (uint16_t)(rand() % 32768); p.dly_time = (uint32_t)(rand() % 20000);
        p.dly_fb = (uint16_t)(rand() % 32768); p.dly_mix = (uint16_t)(rand() % 32768); p.rev_decay = (uint16_t)(rand() % 32768); p.rev_damp = (uint16_t)(rand() % 32768); p.rev_mix = (uint16_t)(rand() % 32768);
        commit(&p);
        for (i = 0; i < 4; i++) { if (rand() & 1) fm_note_on((unsigned)(24 + rand() % 84), (unsigned)(1 + rand() % 127)); else fm_note_off((unsigned)(24 + rand() % 84)); }
        fm_bend(rand() % 16384 - 8192); fm_modwheel((unsigned)rand() % 128); fm_sustain(rand() & 1);
        render(6, 0, 0);
    }
    CHECK(peak <= 32767, "stress: peak %g beyond full scale", peak);
    printf("fm: stress over %ld frames: peak %g, rms %.0f\n", frames, peak, rms());
    fm_bend(0); fm_modwheel(0); fm_sustain(0);
    base(&p); commit(&p); fm_all_off(); render(4000, 0, 0); stat_reset(); render(4, 0, 0);
    CHECK(fm_voices == 0, "voices stuck after the stress run: %u", fm_voices);
    if (wav) fclose(wav);
    printf(fails ? "fm: %d FAILED\n" : "fm: all ok\n", fails);
    return fails != 0;
}
