/* parity.c - run the original float parwave (upstream/klatt-3.04/parwave.c)
 * and klatt_fx on the same parameter track; report max / RMS difference and
 * write both waveforms.
 *
 * usage: parity [-r sr] [-n nfcascade] [-l liteflags] [-e] track.par ref.wav fx.wav
 *   track.par: klatt 3.04 40-column frame file (10 ms frames)
 *   -e         engine only: run klatt_fx alone (for callgrind), no reference
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "parwave.h"
#include "klatt_fx.h"
#include "wavout.h"

/* Give parwave.c the same noise sequence as klatt_fx: its gen_noise() does
 * (rand() % 16383) - 8191, so return kfx noise + 8191 (0..16382). */
static kfx_t noise_twin;
static kfx_t k;
#ifdef KFX_CHECK
static const char *rname[17] = {"rnpp","r1p","r2p","r3p","r4p","r5p","r6p","r1c","r2c","r3c","r4c","r5c","r6c","rnpc","rnz","rlp","rout"};
static long rpeak[17], rovf[17];
static long cur_frame;
static void hook(const kfx_res_t *r, long y, int ovf)
{
    int i = (int)(r - &k.rnpp);
    if (i < 0 || i > 16) return;
    if (y > rpeak[i]) rpeak[i] = y;
    if (ovf) { rovf[i]++; if (rovf[i] == 1) fprintf(stderr, "first overflow in %s at frame %ld\n", rname[i], cur_frame); }
}
#endif
int rand(void) { return (int)(kfx_noise_next(&noise_twin) + 8191); }

int main(int argc, char **argv)
{
    long sr = 8000;
    int nfc = 4, lite = 0, engine_only = 0;
    int i;
    const char *par, *refname, *fxname;
    FILE *in, *wref = NULL, *wfx;
    klatt_global_t g;
    klatt_frame_t fr;
    kfx_frame_t kf;
    int *iwave;
    int16_t *fxbuf;
    long nsp, frames = 0;
    double sumsq = 0, sumsq_ref = 0, maxd = 0;
    long total = 0;
    long dv[40];

    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (!strcmp(argv[i], "-r")) sr = atol(argv[++i]);
        else if (!strcmp(argv[i], "-n")) nfc = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-l")) lite = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-e")) engine_only = 1;
        else { fprintf(stderr, "bad option %s\n", argv[i]); return 2; }
    }
    if (argc - i < (engine_only ? 2 : 3)) {
        fprintf(stderr, "usage: parity [-r sr] [-n nfcascade] [-l lite] [-e] track.par ref.wav fx.wav\n");
        return 2;
    }
    par = argv[i]; refname = engine_only ? NULL : argv[i + 1]; fxname = argv[engine_only ? i + 1 : i + 2];
    nsp = sr / 100;
    in = fopen(par, "r");
    if (!in) { perror(par); return 1; }

    memset(&g, 0, sizeof g);
    g.synthesis_model = CASCADE_PARALLEL;
    g.samrate = sr;
    g.glsource = NATURAL;
    g.nspfr = nsp;
    g.nfcascade = nfc;
    g.f0_flutter = 0;
    g.quiet_flag = TRUE;
    parwave_init(&g);
    kfx_init(&noise_twin, (uint16_t)sr, 1, 0);
    kfx_init(&k, (uint16_t)sr, (uint8_t)nfc, (uint8_t)lite);
#ifdef KFX_CHECK
    { extern void (*kfx_check_hook)(const kfx_res_t *, long, int); kfx_check_hook = hook; }
#endif

    iwave = malloc(sizeof(int) * nsp);
    fxbuf = malloc(sizeof(int16_t) * nsp);
    if (refname) wref = wav_open(refname, sr, 16);
    wfx = wav_open(fxname, sr, 16);
    if ((refname && !wref) || !wfx) { perror("wav"); return 1; }

    for (;;) {
        int n = 0, j;
        for (j = 0; j < 40; j++) {
            if (fscanf(in, "%ld", &dv[j]) != 1) break;
            n++;
        }
        if (n < 40) break;
        {
            long *p = dv;
            int16_t *q = (int16_t *)&kf;
            long *f = (long *)&fr;
            for (j = 0; j < 40; j++) { f[j] = p[j]; q[j] = (int16_t)p[j]; }
        }
        if (!engine_only) {
            parwave(&g, &fr, iwave);
        }
#ifdef KFX_CHECK
        cur_frame = frames;
#endif
        kfx_set_frame(&k, &kf);
        kfx_render16(&k, fxbuf, (int16_t)nsp);
        if (!engine_only) {
            for (j = 0; j < nsp; j++) {
                double d = (double)fxbuf[j] - (double)iwave[j];
                if (getenv("PARITY_DUMP") && frames >= atol(getenv("PARITY_DUMP")) && frames < atol(getenv("PARITY_DUMP")) + 3)
                    printf("f%ld s%d ref=%d fx=%d d=%.0f\n", frames, j, iwave[j], fxbuf[j], d);
                sumsq += d * d;
                sumsq_ref += (double)iwave[j] * (double)iwave[j];
                if (fabs(d) > maxd) maxd = fabs(d);
                fxbuf[j] = fxbuf[j]; /* keep */
            }
            {
                int16_t tmp[4096];
                for (j = 0; j < nsp; j++) tmp[j] = (int16_t)iwave[j];
                wav_write16(wref, tmp, nsp);
            }
        }
        wav_write16(wfx, fxbuf, nsp);
        total += nsp;
        frames++;
    }
    if (wref) wav_close(wref);
    wav_close(wfx);
#ifdef KFX_CHECK
    { extern long kfx_check_overflows, kfx_check_peak;
      int q;
      printf("overflow audit: %ld resonator evaluations overflowed 32 bits; peak |y| = %ld (limit 524287)\n", kfx_check_overflows, kfx_check_peak);
      for (q = 0; q < 17; q++) if (rpeak[q]) printf("  %-5s peak %7ld ovf %ld\n", rname[q], rpeak[q], rovf[q]); }
#endif
    if (!engine_only) {
        double rms = sqrt(sumsq / (total ? total : 1));
        double rmsref = sqrt(sumsq_ref / (total ? total : 1));
        printf("frames=%ld samples=%ld sr=%ld nfcascade=%d lite=%d\n", frames, total, sr, nfc, lite);
        printf("max|diff|=%.1f  rms(diff)=%.3f  rms(ref)=%.1f  SNR=%.1f dB\n",
               maxd, rms, rmsref, 20 * log10(rmsref / (rms > 0 ? rms : 1e-9)));
    } else {
        printf("frames=%ld samples=%ld (engine only)\n", frames, total);
    }
    return 0;
}
