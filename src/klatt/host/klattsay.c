/* klattsay.c - host frontend: English text -> phones -> Holmes parameter
 * frames -> klatt_fx -> WAV.
 *
 * usage: klattsay [options] "text" out.wav
 *   -r 8000|11025   sample rate (default 8000)
 *   -8              8-bit unsigned output (default 16-bit)
 *   -F hz           base F0 (133)
 *   -S pct          speed percent (100)
 *   -G db           Gain0 (60)
 *   -T db           spectral tilt TLTdb (0)
 *   -n N            cascade formants (8k: 3, 11k: 4)
 *   -l flags        KFX_LITE_* flags (1 = 1x glottal source, 19 = KFX_FLAGS_DOS:
 *                   1x source + white noise + pre-emphasised output)
 *   -a              map avc onto parallel voicing (A1/AVpdb)
 *   -P              print the phone string for each chunk
 *   -E              print the element sequence
 *   -d file.par     dump the parameter track (klatt 3.04 40-column format)
 *   -p              treat the text as SAMPA phones instead of English
 *   -U              no heuristic stress marks (plain rsynth NRL behaviour)
 *   -u mode         stress heuristic: 1 first vowel primary, rest tertiary;
 *                   2 all primary (default); 3 first primary, rest secondary
 *   -A db           aspiration offset (26)   -Z db  frication offset (12)
 *   -K pct          open quotient (40)       -B hz  nasal bandwidth (500)
 *   -M q8           parameter smoothing, 256 = none (128)
 *   -O a2,a3,a4,a5,a6,ab   parallel amplitude offsets in dB (see klt_voice_default)
 *   -4 f4,b4        fixed F4 / B4 in Hz
 *   -N q4           noise low-pass coefficient, 12 = parwave's 0.75, 0 = white
 *   -c db           voice bar: AVdb = max(av, avc - db)
 *   -D sh           SRC_DIFF (-l 8) gain shift (2)
 *   -X db           AF clamp (70)
 *   -o k,g          OUT_FIR (-l 16) coefficient and gain in Q10 (922,4096)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "klatt_fx.h"
#include "holmes.h"
#include "nrl.h"
#include "wavout.h"

static void dump_par(FILE *f, const kfx_frame_t *fr)
{
    const int16_t *p = (const int16_t *)fr;
    int i;
    for (i = 0; i < 40; i++)
        fprintf(f, "%d%c", p[i], i == 39 ? '\n' : ' ');
}

int main(int argc, char **argv)
{
    int sr = 8000, bits = 16, nfc = -1, lite = 0;
    int f0 = 133, speed = 100, gain = 60, tilt = 0, use_avc = 0;
    int print_ph = 0, print_elm = 0, raw_phones = 0;
    int asp_off = -999, af_off = -999, kopen = -1, bn = -1, smooth = -1;
    const char *dump = NULL;
    const char *offs = NULL, *f4s = NULL, *out_fir = NULL;
    int noise_k = -1, avc_off = -1, diff_sh = -1, af_max = -1;
    const char *text, *outname;
    int i;
    kfx_t k;
    klt_voice_t voice;
    static klt_seq_t seq;
    static klt_holmes_t hol;
    static char phones[1024];
    FILE *wav, *par = NULL;
    long total = 0, clipped = 0;
    int16_t buf16[256];
    uint8_t buf8[256];
    int nsp;

    for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-r")) sr = atoi(argv[++i]);
        else if (!strcmp(a, "-8")) bits = 8;
        else if (!strcmp(a, "-F")) f0 = atoi(argv[++i]);
        else if (!strcmp(a, "-S")) speed = atoi(argv[++i]);
        else if (!strcmp(a, "-G")) gain = atoi(argv[++i]);
        else if (!strcmp(a, "-T")) tilt = atoi(argv[++i]);
        else if (!strcmp(a, "-n")) nfc = atoi(argv[++i]);
        else if (!strcmp(a, "-l")) lite = atoi(argv[++i]);
        else if (!strcmp(a, "-a")) use_avc = 1;
        else if (!strcmp(a, "-c")) { use_avc = 2; avc_off = atoi(argv[++i]); }
        else if (!strcmp(a, "-D")) diff_sh = atoi(argv[++i]);
        else if (!strcmp(a, "-X")) af_max = atoi(argv[++i]);
        else if (!strcmp(a, "-o")) out_fir = argv[++i];
        else if (!strcmp(a, "-P")) print_ph = 1;
        else if (!strcmp(a, "-E")) print_elm = 1;
        else if (!strcmp(a, "-p")) raw_phones = 1;
        else if (!strcmp(a, "-d")) dump = argv[++i];
        else if (!strcmp(a, "-U")) nrl_stress_mode = 0;
        else if (!strcmp(a, "-u")) nrl_stress_mode = atoi(argv[++i]);
        else if (!strcmp(a, "-A")) asp_off = atoi(argv[++i]);
        else if (!strcmp(a, "-Z")) af_off = atoi(argv[++i]);
        else if (!strcmp(a, "-K")) kopen = atoi(argv[++i]);
        else if (!strcmp(a, "-B")) bn = atoi(argv[++i]);
        else if (!strcmp(a, "-M")) smooth = atoi(argv[++i]);
        else if (!strcmp(a, "-O")) offs = argv[++i];
        else if (!strcmp(a, "-4")) f4s = argv[++i];
        else if (!strcmp(a, "-N")) noise_k = atoi(argv[++i]);
        else { fprintf(stderr, "unknown option %s\n", a); return 2; }
    }
    if (argc - i < 2) {
        fprintf(stderr, "usage: klattsay [-r sr] [-8] [-F f0] [-S pct] [-G db] [-T tilt] [-n nfc] [-l lite] [-a] [-P] [-E] [-p] [-d track.par] \"text\" out.wav\n");
        return 2;
    }
    text = argv[i];
    outname = argv[i + 1];
    if (nfc < 0)
        nfc = (sr <= 8000) ? 3 : 4;

    klt_voice_default(&voice, (uint16_t)sr);
    voice.f0hz = f0;
    voice.speed_q8 = (int16_t)((100L * 256L) / (speed > 0 ? speed : 100));
    voice.gain0 = (int16_t)gain;
    voice.tilt = (int16_t)tilt;
    voice.use_avc = (uint8_t)use_avc;
    if (avc_off >= 0) voice.avc_off = (int16_t)avc_off;
    if (af_max > 0) voice.af_max = (int16_t)af_max;
    if (asp_off != -999) voice.off_asp = (int16_t)asp_off;
    if (af_off != -999) voice.off_af = (int16_t)af_off;
    if (kopen > 0) voice.kopen_pct = (int16_t)kopen;
    if (bn > 0) voice.bn = (int16_t)bn;
    if (smooth >= 0) voice.smooth_q8 = (int16_t)smooth;
    if (!(lite & KFX_LITE_NOISE_WHITE)) {
        /* the voice defaults assume white noise; with parwave's low-passed
         * noise (DC gain 4) take 6 dB off the noise-driven amplitudes and
         * keep parwave's AF limit */
        voice.off_a2 -= 6; voice.off_a3 -= 6; voice.off_a4 -= 6;
        voice.off_a5 -= 6; voice.off_a6 -= 6; voice.off_ab -= 6;
        voice.off_asp -= 6; voice.af_max = 70;
    }
    if (offs) {
        int o[6] = {0, 0, 0, 0, 0, 0};
        sscanf(offs, "%d,%d,%d,%d,%d,%d", &o[0], &o[1], &o[2], &o[3], &o[4], &o[5]);
        voice.off_a2 = (int16_t)o[0]; voice.off_a3 = (int16_t)o[1]; voice.off_a4 = (int16_t)o[2];
        voice.off_a5 = (int16_t)o[3]; voice.off_a6 = (int16_t)o[4]; voice.off_ab = (int16_t)o[5];
    }
    if (f4s) {
        int f4 = 0, b4 = 0;
        sscanf(f4s, "%d,%d", &f4, &b4);
        if (f4 > 0) voice.f4 = (int16_t)f4;
        if (b4 > 0) voice.b4 = (int16_t)b4;
    }

    kfx_init(&k, (uint16_t)sr, (uint8_t)nfc, (uint8_t)lite);
    if (noise_k >= 0) k.noise_k = noise_k;
    if (diff_sh >= 0) k.src_diff_sh = (int8_t)diff_sh;
    if (out_fir) {
        int kq = 0, gq = 0;
        sscanf(out_fir, "%d,%d", &kq, &gq);       /* Q10: e.g. 922,4096 = 0.9, x4 */
        if (kq > 0) k.out_k = (int16_t)kq;
        if (gq > 0) k.out_gain = (int16_t)gq;
    }
    nsp = sr / 100;

    wav = wav_open(outname, sr, bits);
    if (!wav) { perror(outname); return 1; }
    if (dump) {
        par = fopen(dump, "w");
        if (!par) { perror(dump); return 1; }
    }

    for (;;) {
        int16_t n;
        int16_t frames, pos;
        kfx_frame_t fr;
        if (raw_phones) {
            if (!*text) break;
            strncpy(phones, text, sizeof phones - 1);
            phones[sizeof phones - 1] = 0;
            n = (int16_t)strlen(phones);
            text += strlen(text);
        } else {
            n = nrl_translate(&text, phones, (int16_t)sizeof phones);
            if (n <= 0) break;
        }
        if (print_ph)
            fprintf(stderr, "[%s]\n", phones);
      for (pos = 0; pos < n; pos += seq.used) {   /* a long clause takes several sequence chunks */
        frames = klt_phones_to_seq(&voice, phones + pos, (int16_t)(n - pos), &seq);
        if (seq.used <= 0) break;
        if (frames <= 0) continue;
        if (print_elm) {
            int j;
            for (j = 0; j < seq.nelm; j++)
                fprintf(stderr, "%s.%d ", klt_elements[seq.elm[j]].name, seq.dur[j]);
            fprintf(stderr, " (%d frames)\n", frames);
        }
        klt_holmes_start(&hol, &voice, &seq);
        while (klt_holmes_next(&hol, &fr)) {
            int j;
            if (par) dump_par(par, &fr);
            kfx_set_frame(&k, &fr);
            if (bits == 16) {
                kfx_render16(&k, buf16, (int16_t)nsp);
                for (j = 0; j < nsp; j++)
                    if (buf16[j] >= 32767 || buf16[j] <= -32767) clipped++;
                wav_write16(wav, buf16, nsp);
            } else {
                kfx_render8(&k, buf8, (int16_t)nsp);
                for (j = 0; j < nsp; j++)
                    if (buf8[j] == 0 || buf8[j] == 255) clipped++;
                wav_write8(wav, buf8, nsp);
            }
            total += nsp;
        }
      }
    }
    /* a little silence at the end so the last resonator tails play out */
    {
        kfx_frame_t fr;
        int32_t zero[KLT_NPARM];
        int j;
        for (j = 0; j < KLT_NPARM; j++)
            zero[j] = (int32_t)klt_elements[0].p[j].stdy << 8;
        klt_map_frame(&voice, zero, 0, &fr);
        for (j = 0; j < 10; j++) {
            kfx_set_frame(&k, &fr);
            if (par) dump_par(par, &fr);
            if (bits == 16) { kfx_render16(&k, buf16, (int16_t)nsp); wav_write16(wav, buf16, nsp); }
            else { kfx_render8(&k, buf8, (int16_t)nsp); wav_write8(wav, buf8, nsp); }
            total += nsp;
        }
    }
    wav_close(wav);
    if (par) fclose(par);
    fprintf(stderr, "%s: %ld samples (%.2f s) at %d Hz, %d-bit, %ld clipped\n",
            outname, total, (double)total / sr, sr, bits, clipped);
    return 0;
}
