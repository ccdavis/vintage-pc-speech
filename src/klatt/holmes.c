/* holmes.c - integer port of rsynth's phtoelm.c + holmes.c
 *
 * Copyright (c) 1994,2001-2004 Nick Ing-Simmons. All rights reserved.
 * Integer port (c) 2026 accessible_os FreeDOS speech project.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */
#include "holmes.h"

#define E_END 0
#define E_Q   1

/* ------------------------------------------------------------------ */
/* voice defaults                                                      */

void klt_voice_default(klt_voice_t *v, uint16_t sr)
{
    v->sr = sr;
    v->f0hz = 133;
    v->speed_q8 = 256;
    v->smooth_q8 = 128;
    v->gain0 = 60;
    if (sr <= 8000) {
        /* Nyquist is 4 kHz: F4 sits just below it, F5/F6 fold back and are
         * only used (softly) by the parallel branch. */
        v->f4 = 3300; v->b4 = 300;
        v->f5 = 3800; v->b5 = 300;
        v->f6 = 3950; v->b6 = 400;
    } else {
        v->f4 = 3500; v->b4 = 300;
        v->f5 = 4500; v->b5 = 250;
        v->f6 = 5200; v->b6 = 400;
    }
    v->b4p = 500; v->b5p = 600; v->b6p = 800;
    v->fnp = 270; v->bn = 100;   /* rsynth uses 500 for both; 100 (Klatt's value) scored better with Whisper */
    v->tilt = 0;
    v->kopen_pct = 40;
    v->off_a2 = 23; v->off_a3 = 30; v->off_a4 = 34;
    v->off_a5 = 39; v->off_a6 = 37; v->off_ab = 32;
    v->off_af = 12; v->off_asp = 32;
    v->af_max = 80;    /* audited with white noise: peak 126k of 524k */
    v->use_avc = 0;
    v->avc_off = 8;
}

/* ------------------------------------------------------------------ */
/* phones -> element sequence                                           */

static int seq_push(klt_seq_t *s, uint8_t e, int16_t dur)
{
    if (s->nelm >= KLT_MAX_ELM)
        return 0;
    if (dur > 255) dur = 255;
    if (dur < 0) dur = 0;
    s->elm[s->nelm] = e;
    s->dur[s->nelm] = (uint8_t)dur;
    s->nelm++;
    return 1;
}

static void f0_push(klt_seq_t *s, int32_t v)
{
    if (s->nf0 < KLT_MAX_F0)
        s->f0[s->nf0++] = (int16_t)v;
}

/* the F0 base used by the contour, clamped so centi-Hz values fit int16 */
static int32_t f0_base(const klt_voice_t *v)
{
    return v->f0hz > KLT_F0_MAX_HZ ? KLT_F0_MAX_HZ : v->f0hz;
}

/* rsynth decline_f0(): F0 drops 0.12 Hz per frame, floor at 0.7 * F0 */
static int32_t decline_f0(const klt_voice_t *v, klt_seq_t *s, int32_t f, int32_t t)
{
    f0_push(s, t);
    f -= 12L * t;
    if (f < 70L * f0_base(v))
        f = 70L * f0_base(v);
    f0_push(s, f);
    return f;
}

static int ph_match(const char *s, int16_t n, const klt_ph_t **out)
{
    int i;
    for (i = 0; i < klt_num_phtab; i++) {
        const char *p = klt_phtab[i].sampa;
        int16_t j = 0;
        while (p[j] && j < n && p[j] == s[j])
            j++;
        if (p[j] == 0) {
            *out = &klt_phtab[i];
            return j;
        }
    }
    return 0;
}

int16_t klt_phones_to_seq(const klt_voice_t *v, const char *phones, int16_t n, klt_seq_t *seq)
{
    int16_t stress = 0, seen_vowel = 0;
    int32_t t = 0, f0t = 0;
    int32_t f;
    int16_t pos = 0;

    seq->nelm = 0;
    seq->nf0 = 0;
    seq->used = 0;
    f = f0_base(v) * 110L;            /* 1.1 * F0, centi-Hz */
    f0_push(seq, f);

    while (pos < n && phones[pos]) {
        const klt_ph_t *ph;
        int m;
        /* stop at a phone boundary when the next phone (<= 5 elements) or a
         * stress mark (4 F0 entries, plus 2 for the closing decline) might
         * not fit; the caller continues from seq->used */
        if (seq->nelm + 5 > KLT_MAX_ELM || seq->nf0 + 6 > KLT_MAX_F0)
            break;
        m = ph_match(phones + pos, (int16_t)(n - pos), &ph);
        if (m > 0) {
            int j;
            pos = (int16_t)(pos + m);
            for (j = 0; j < ph->n; j++) {
                const klt_elm_t KLT_FAR *e = &klt_elements[ph->e[j]];
                /* StressDur: only vowels have ud != du */
                int32_t d = e->ud + ((int32_t)(e->du - e->ud) * stress) / 3;
                d = (d * v->speed_q8) >> 8;
                if (!seq_push(seq, ph->e[j], (int16_t)d))
                    goto done;
                t += seq->dur[seq->nelm - 1];
                if (e->feat & KLT_FEAT_VWL)
                    seen_vowel = 1;
                else if (seen_vowel)
                    stress = 0;
            }
        } else {
            char ch = phones[pos++];
            switch (ch) {
            case '\'':  stress++;            /* primary */
            case ',':   stress++;            /* secondary */
            case '+':   stress++;            /* tertiary */
                if (stress > 3) stress = 3;
                seen_vowel = 0;
                f = decline_f0(v, seq, f, t - f0t);
                f0t = t;
                f0_push(seq, 0);
                f0_push(seq, f + f0_base(v) * 2L * stress);   /* F0 * stress * 0.02 */
                break;
            default:    /* '-', ':' and anything unknown are ignored */
                break;
            }
        }
    }
done:
    decline_f0(v, seq, f, t - f0t);
    seq->used = pos;
    seq->frames = (int16_t)t;
    return (int16_t)t;
}

/* ------------------------------------------------------------------ */
/* interpolation                                                        */

/* value in Q8 */
static int32_t q8(int16_t v) { return (int32_t)v << 8; }

/* a + (b - a) * t / d, Q8 */
static int32_t linear(int32_t a, int32_t b, int32_t t, int32_t d)
{
    if (t <= 0)
        return a;
    if (t >= d)
        return b;
    return a + ((b - a) * t) / d;
}

/* rsynth set_trans(): 'a' dominates, 'b' is dominated; ext != 0 when 'a'
 * is not the current element (use its external time). */
static void set_trans(const klt_voice_t *v, klt_slope_t *t, int i,
                      const klt_elm_t KLT_FAR *a, const klt_elm_t KLT_FAR *b, int ext)
{
    int32_t tt = (int32_t)(ext ? a->p[i].ed : a->p[i].id);
    tt = (tt * v->speed_q8) >> 8;
    t->t = (int16_t)tt;
    if (tt) {
        int32_t prop = a->p[i].prop;
        t->v = (((100L - prop) * a->p[i].stdy + prop * b->p[i].stdy) << 8) / 100L;
    } else {
        t->v = q8(b->p[i].stdy);
    }
}

static int32_t interpolate(const klt_slope_t *s, const klt_slope_t *e, int32_t mid,
                           int32_t t, int32_t d)
{
    int32_t steady = d - (s->t + e->t);
    if (steady >= 0) {
        if (t < s->t)
            return linear(s->v, mid, t, s->t);
        t -= s->t;
        if (t <= steady)
            return mid;
        return linear(mid, e->v, t - steady, e->t);
    } else {
        int32_t sp = linear(s->v, mid, t, s->t);
        int32_t ep = linear(e->v, mid, d - t, e->t);
        /* f * sp + (1 - f) * ep with f = 1 - t/d */
        return ((d - t) * sp + t * ep) / d;
    }
}

static void next_element(klt_holmes_t *h)
{
    const klt_seq_t *seq = h->seq;
    /* skip zero-length elements (they only shape neighbours' boundaries) */
    while (h->i < seq->nelm) {
        uint8_t ce = seq->elm[h->i];
        int16_t dur = seq->dur[h->i];
        uint8_t ne;
        int j;
        h->i++;
        if (dur == 0) {
            h->le = ce;
            continue;
        }
        ne = (h->i < seq->nelm) ? seq->elm[h->i] : E_END;
        for (j = 0; j < KLT_NPARM; j++) {
            const klt_elm_t KLT_FAR *L = &klt_elements[h->le];
            const klt_elm_t KLT_FAR *C = &klt_elements[ce];
            const klt_elm_t KLT_FAR *N = &klt_elements[ne];
            if (C->p[j].rk > L->p[j].rk)
                set_trans(h->voice, &h->start[j], j, C, L, 0);   /* we dominate last */
            else
                set_trans(h->voice, &h->start[j], j, L, C, 1);   /* last dominates us */
            if (N->p[j].rk > C->p[j].rk)
                set_trans(h->voice, &h->end[j], j, N, C, 1);     /* next dominates us */
            else
                set_trans(h->voice, &h->end[j], j, C, N, 0);     /* we dominate next */
        }
        h->ce = ce;
        h->dur = dur;
        h->t = 0;
        return;
    }
    h->dur = 0;   /* end of sequence */
}

void klt_holmes_start(klt_holmes_t *h, const klt_voice_t *v, const klt_seq_t *seq)
{
    int j;
    h->voice = v;
    h->seq = seq;
    h->i = 0;
    h->le = E_END;
    h->ce = E_END;
    for (j = 0; j < KLT_NPARM; j++) {
        h->flt[j] = q8(klt_elements[E_END].p[j].stdy);
        h->ep[j] = h->flt[j];
    }
    h->f0i = 1;
    h->f0s = h->f0e = seq->nf0 ? seq->f0[0] : v->f0hz * 100L;
    h->tf0 = 0;
    h->ntf0 = 0;
    h->f0 = h->f0s;
    next_element(h);
}

int klt_holmes_next(klt_holmes_t *h, kfx_frame_t *fr)
{
    const klt_elm_t KLT_FAR *C;
    int32_t smooth = h->voice->smooth_q8;
    int j;

    if (h->dur == 0)
        return 0;
    C = &klt_elements[h->ce];
    for (j = 0; j < KLT_NPARM; j++) {
        int32_t val = interpolate(&h->start[j], &h->end[j], q8(C->p[j].stdy), h->t, h->dur);
        /* rsynth's experimental low-pass on the parameter tracks */
        h->flt[j] = (smooth * val + (256L - smooth) * h->flt[j]) >> 8;
        h->ep[j] = h->flt[j];
    }

    /* F0 contour: (frames, target) pairs, zero-length pairs are stress pulses */
    while (h->tf0 == h->ntf0) {
        h->tf0 = 0;
        h->f0s = h->f0e;
        if (h->f0i + 1 < h->seq->nf0) {
            h->ntf0 = (int16_t)h->seq->f0[h->f0i];
            h->f0e = h->seq->f0[h->f0i + 1];
            h->f0i = (int16_t)(h->f0i + 2);
        } else {
            h->ntf0 = 32000;   /* contour exhausted: hold */
        }
    }
    h->f0 = linear(h->f0s, h->f0e, h->tf0, h->ntf0);
    h->tf0++;

    klt_map_frame(h->voice, h->ep, h->f0, fr);

    h->t++;
    if (h->t >= h->dur) {
        h->le = h->ce;
        next_element(h);
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Holmes parameters -> Klatt frame                                     */

static int16_t r8(int32_t q) { return (int16_t)((q + 128) >> 8); }

static int16_t clampi(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return (int16_t)v;
}

void klt_map_frame(const klt_voice_t *v, const int32_t *ep, int32_t f0_centi, kfx_frame_t *fr)
{
    int16_t av = r8(ep[KP_AV]);
    int16_t avc = r8(ep[KP_AVC]);
    int16_t f0hz10 = (int16_t)((f0_centi + 5) / 10);
    int32_t kopen;

    if (av <= 0 && avc <= 0)
        f0hz10 = 0;                       /* unvoiced: no source */
    fr->F0hz10 = f0hz10;
    if (v->use_avc == 2 && avc - v->avc_off > av)
        av = (int16_t)(avc - v->avc_off);
    fr->AVdb = clampi(av, 0, 70);
    fr->F1hz = clampi(r8(ep[KP_F1]), 150, 1300);
    fr->B1hz = clampi(r8(ep[KP_B1]), 40, 1000);
    fr->F2hz = clampi(r8(ep[KP_F2]), 500, 3000);
    fr->B2hz = clampi(r8(ep[KP_B2]), 40, 1000);
    fr->F3hz = clampi(r8(ep[KP_F3]), 1200, v->f4 - 200);
    fr->B3hz = clampi(r8(ep[KP_B3]), 40, 1000);
    fr->F4hz = v->f4; fr->B4hz = v->b4;
    fr->F5hz = v->f5; fr->B5hz = v->b5;
    fr->F6hz = v->f6; fr->B6hz = v->b6;
    fr->FNZhz = clampi(r8(ep[KP_FN]), 200, 700);
    fr->BNZhz = v->bn;
    fr->FNPhz = v->fnp;
    fr->BNPhz = v->bn;
    fr->ASP = clampi(r8(ep[KP_ASP]) > 0 ? r8(ep[KP_ASP]) + v->off_asp : 0, 0, 70);
    /* open phase = kopen_pct % of the period, in samples at sr */
    if (f0hz10 > 0)
        kopen = ((int32_t)v->kopen_pct * 10L * v->sr) / (100L * f0hz10);
    else
        kopen = 30;
    fr->Kopen = clampi(kopen, 10, 65);
    fr->Aturb = 0;
    fr->TLTdb = v->tilt;
    fr->AF = clampi(r8(ep[KP_AF]) > 0 ? r8(ep[KP_AF]) + v->off_af : 0, 0, v->af_max);
    fr->Kskew = 0;
    if (v->use_avc == 1 && avc > 0) {
        fr->A1 = 60;
        fr->AVpdb = clampi(avc, 0, 70);
    } else {
        fr->A1 = 0;
        fr->AVpdb = 0;
    }
    fr->B1phz = fr->B1hz;
#define AOFF(i, off) (r8(ep[i]) > 0 ? clampi(r8(ep[i]) + (off), 0, 80) : 0)
    fr->A2 = AOFF(KP_A2, v->off_a2); fr->B2phz = fr->B2hz;
    fr->A3 = AOFF(KP_A3, v->off_a3); fr->B3phz = fr->B3hz;
    fr->A4 = AOFF(KP_A4, v->off_a4); fr->B4phz = v->b4p;
    fr->A5 = AOFF(KP_A5, v->off_a5); fr->B5phz = v->b5p;
    fr->A6 = AOFF(KP_A6, v->off_a6); fr->B6phz = v->b6p;
    fr->ANP = 0;
    fr->AB = AOFF(KP_AB, v->off_ab);
#undef AOFF
    fr->Gain0 = v->gain0;
}
