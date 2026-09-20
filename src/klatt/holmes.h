/* holmes.h - Holmes-style phoneme-to-parameter frontend (from rsynth).
 *
 * Integer-only port of rsynth's phtoelm.c / holmes.c (Copyright (c)
 * 1994,2001-2004 Nick Ing-Simmons, GNU Library General Public License v2 or
 * later).  Turns a SAMPA phone string into a sequence of "elements"
 * (Elements.def), interpolates the 18 element parameters frame by frame with
 * the Holmes transition rules, and maps each frame onto a kfx_frame_t.
 *
 * Same portability rules as klatt_fx.h: no float, no malloc, no stdio, no
 * recursion; int32_t where 32 bits are needed.
 */
#ifndef HOLMES_H
#define HOLMES_H

#include <stdint.h>
#include "klatt_fx.h"

/* On 16-bit Watcom (both the -0 and the -3 build; __386__ is only the 32-bit
 * compiler) the tables live in a far data segment so they leave DGROUP. */
#if defined(__WATCOMC__) && !defined(__386__)
#define KLT_FAR __far
#else
#define KLT_FAR
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define KLT_NPARM 18
enum {
    KP_FN, KP_F1, KP_F2, KP_F3, KP_B1, KP_B2, KP_B3, KP_PN,
    KP_A2, KP_A3, KP_A4, KP_A5, KP_A6, KP_AB, KP_AV, KP_AVC, KP_ASP, KP_AF
};

typedef struct {
    int16_t stdy;   /* steady-state value (Hz or dB) */
    int8_t  prop;   /* % of stdy contributed to the adjacent element */
    int8_t  ed;     /* external transition duration (frames) */
    int8_t  id;     /* internal transition duration (frames) */
    int8_t  rk;     /* rank for dominance */
} klt_interp_t;

#define KLT_FEAT_VWL 1
#define KLT_FEAT_STP 2
#define KLT_FEAT_NAS 4

typedef struct {
    const char *name;
    uint8_t rk;     /* obsolete rank */
    uint8_t du;     /* normal duration, frames */
    uint8_t ud;     /* unstressed duration, frames */
    uint8_t feat;   /* KLT_FEAT_* */
    klt_interp_t p[KLT_NPARM];
} klt_elm_t;

extern const klt_elm_t KLT_FAR klt_elements[];   /* elements_tab.c: 84 x 98 bytes, far data on 16-bit DOS */
extern const int klt_num_elements;

typedef struct {
    const char *sampa;
    uint8_t n;
    uint8_t e[5];
} klt_ph_t;

extern const klt_ph_t klt_phtab[];       /* phtoelm_tab.c, longest first */
extern const int klt_num_phtab;

/* ---- voice / configuration ---- */
typedef struct {
    uint16_t sr;
    int32_t  f0hz;       /* base F0 in Hz (rsynth default 133) */
    int16_t  speed_q8;   /* element duration multiplier, 256 = 1.0 */
    int16_t  smooth_q8;  /* parameter smoothing, 128 = rsynth's 0.5 */
    int16_t  gain0;      /* Klatt Gain0 in dB */
    int16_t  f4, b4, f5, b5, f6, b6;      /* fixed higher formants (cascade) */
    int16_t  b4p, b5p, b6p;               /* parallel bandwidths */
    int16_t  fnp, bn;                     /* fixed nasal pole, nasal bandwidth */
    int16_t  tilt;                        /* TLTdb */
    int16_t  kopen_pct;                   /* open quotient in %, 40 */
    /* dB offsets applied when mapping rsynth element amplitudes onto Klatt
     * parallel amplitudes.  parwave scales A2..A6/AB/AF/ASP by fixed factors
     * (0.15, 0.06, 0.04, 0.022, 0.03, 0.05, 0.25, 0.05) and its noise source
     * is 6 dB weaker than rsynth's (uniform +-8191 vs the sum of 16 of them
     * halved); rsynth's own synth (opsynth.c) has neither, and its element
     * table was tuned without them, so the defaults undo both: -20 log10 of
     * the factor + 6 dB, assuming KFX_LITE_NOISE_WHITE (with parwave's
     * low-passed noise subtract 6 dB again, then the noise is 12 dB too
     * strong below 300 Hz and 5 dB too weak at 4 kHz). */
    int16_t  off_a2, off_a3, off_a4, off_a5, off_a6, off_ab, off_af, off_asp;
    int16_t  af_max;                      /* AF clamp (70 with parwave's low-passed noise) */
    uint8_t  use_avc;                     /* 1: map avc -> parallel voicing (A1/AVpdb);
                                             2: AVdb = max(av, avc - avc_off) (voice bar as ordinary voicing) */
    int16_t  avc_off;                     /* dB taken off avc in mode 2 */
} klt_voice_t;

void klt_voice_default(klt_voice_t *v, uint16_t sr);

/* ---- element sequence for one utterance chunk ----
 * A chunk holds up to KLT_MAX_ELM elements (a phone is 1..5 elements, so
 * ~100-250 phones, 2.5-6 s of speech at normal rate) and KLT_MAX_F0/4
 * stress marks.  klt_phones_to_seq() stops at a phone boundary when either
 * fills up and reports how many phone characters it consumed in seq->used;
 * the caller renders the chunk and calls again from phones + used.  A 200
 * character clause (the DOS utterance limit) with dense consonant clusters
 * needs ~2 chunks; each chunk restarts the F0 declination.
 * Bounds: dur <= 255 (element du <= 16 frames x speed factor <= 2.3), so
 * frames <= 256 x 37 < 32767; f0 entries are centi-Hz <= 1.16 x 280 Hz
 * (f0hz is clamped at 280 here) or frame counts, both fit int16. */
#define KLT_MAX_ELM 256
#define KLT_MAX_F0  256
#define KLT_F0_MAX_HZ 280

typedef struct {
    uint8_t elm[KLT_MAX_ELM];
    uint8_t dur[KLT_MAX_ELM];
    int16_t nelm;
    int16_t f0[KLT_MAX_F0];   /* centi-Hz: f0[0] = start, then (frames, target) pairs */
    int16_t nf0;
    int16_t frames;           /* total frames */
    int16_t used;             /* phone characters consumed (< n: call again from phones + used) */
} klt_seq_t;

/* rsynth phone_to_elm(): SAMPA phones (with ' , + stress marks) -> sequence.
 * Returns the number of frames; seq->used tells how far it got. */
int16_t klt_phones_to_seq(const klt_voice_t *v, const char *phones, int16_t n, klt_seq_t *seq);

/* ---- frame-by-frame interpolation (rsynth_interpolate, pull model) ---- */
typedef struct {
    int32_t v;   /* boundary value, Q8 */
    int16_t t;   /* transition time, frames */
} klt_slope_t;

typedef struct {
    const klt_voice_t *voice;
    const klt_seq_t *seq;
    int16_t i;            /* next element index in seq */
    int16_t t, dur;       /* frame within current element */
    uint8_t le, ce;       /* last / current element */
    klt_slope_t start[KLT_NPARM], end[KLT_NPARM];
    int32_t flt[KLT_NPARM];   /* smoothing filter state, Q8 */
    int32_t ep[KLT_NPARM];    /* current frame parameters, Q8 */
    int16_t f0i;              /* index into seq->f0 */
    int32_t f0s, f0e;         /* current F0 segment, centi-Hz */
    int16_t tf0, ntf0;
    int32_t f0;               /* current F0, centi-Hz */
} klt_holmes_t;

void klt_holmes_start(klt_holmes_t *h, const klt_voice_t *v, const klt_seq_t *seq);

/* Produce the next 10 ms frame.  Returns 1 and fills *fr, or 0 at the end
 * of the sequence. */
int klt_holmes_next(klt_holmes_t *h, kfx_frame_t *fr);

/* Map the current parameter set onto a Klatt frame (exposed for tests). */
void klt_map_frame(const klt_voice_t *v, const int32_t *ep_q8, int32_t f0_centi, kfx_frame_t *fr);

#ifdef __cplusplus
}
#endif
#endif /* HOLMES_H */
