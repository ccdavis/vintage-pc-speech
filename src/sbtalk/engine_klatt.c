/* Klatt engine glue: rsynth NRL rules -> Holmes elements -> fixed-point parwave, 10 ms frames. */
#include <string.h>
#include "ring.h"
#include "engine.h"
#include "../klatt/klatt_fx.h"
#include "../klatt/holmes.h"
#include "../klatt/nrl.h"
#define KRATE 8000u
static kfx_t k;
static klt_voice_t voice;
static klt_seq_t seq;
static klt_holmes_t hol;
extern char nrl_phones[512];                 /* engine_sam.c: shared by the NRL engines */
#define phones nrl_phones
static unsigned char buf[KRATE / 100];
static int16_t base_f0;
void klatt_engine_init(void)
{
    klt_voice_default(&voice, (uint16_t)KRATE);
    base_f0 = voice.f0hz;
    kfx_init(&k, (uint16_t)KRATE, 3, KFX_FLAGS_DOS);   /* lite source, white noise, FIR output, DOS gain */
}
void klatt_engine_set_params(unsigned dt_speed, unsigned dt_pitch)
{
    unsigned pct = 100 + ((int)dt_speed - 7) * 8;           /* Provox's default 7S = normal rate; 0..9 -> 44..116 % */
    voice.speed_q8 = (int16_t)((100L * 256L) / pct);
    voice.f0hz = (int16_t)((long)base_f0 * (dt_pitch + 25) / 75);   /* 50 -> base, 99 -> 1.65x */
    if (voice.f0hz < 50) voice.f0hz = 50;
}
void klatt_engine_speak(char *utt, unsigned len)
{
    const char *text = utt; int16_t n, i, pos;
    kfx_frame_t fr;
    utt[len] = 0;
    ring_begin_utterance();
    for (;;) {
        n = nrl_translate(&text, phones, (int16_t)sizeof phones);
        if (n <= 0) break;
        for (pos = 0; pos < n; pos += seq.used) {           /* a long clause takes several sequence chunks */
            if (klt_phones_to_seq(&voice, phones + pos, (int16_t)(n - pos), &seq) <= 0) { if (seq.used <= 0) break; continue; }
            klt_holmes_start(&hol, &voice, &seq);
            while (klt_holmes_next(&hol, &fr)) {
                kfx_set_frame(&k, &fr);
                kfx_render8(&k, buf, (int16_t)sizeof buf);
                ring_write_block(buf, sizeof buf);
                if (ring_abort) return;
            }
        }
    }
    {   /* let the resonators ring out */
        int32_t zero[KLT_NPARM];
        for (i = 0; i < KLT_NPARM; i++) zero[i] = (int32_t)klt_elements[0].p[i].stdy << 8;
        klt_map_frame(&voice, zero, 0, &fr);
        for (i = 0; i < 5; i++) { kfx_set_frame(&k, &fr); kfx_render8(&k, buf, (int16_t)sizeof buf); ring_write_block(buf, sizeof buf); if (ring_abort) return; }
    }
    ring_end_utterance();
}
