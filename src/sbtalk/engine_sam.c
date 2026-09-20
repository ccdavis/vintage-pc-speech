#include <string.h>
#include "ring.h"
#include "engine.h"
#ifndef NO_SAM
#include "sam/sam.h"
#include "sam/reciter.h"
#endif
#ifdef NO_SAM
int engine_id = ENGINE_RETRO;
#else
int engine_id = ENGINE_SAM;
#endif
char nrl_phones[512];                            /* NRL phone buffer shared by the Klatt and 1983 engines */
#ifdef HAVE_KLATT
void klatt_engine_init(void); void klatt_engine_set_params(unsigned, unsigned); void klatt_engine_speak(char *, unsigned);
#endif
#ifdef HAVE_RETRO
void retro_engine_init(void); void retro_engine_set_params(unsigned, unsigned); void retro_engine_speak(char *, unsigned); unsigned retro_engine_rate(void);
#endif
void engine_init(int which)
{
    engine_id = which;
#ifdef HAVE_KLATT
    if (which == ENGINE_KLATT) klatt_engine_init();
#endif
#ifdef HAVE_RETRO
    if (which == ENGINE_RETRO) retro_engine_init();
#endif
}
unsigned engine_rate(void)
{
#ifdef HAVE_RETRO
    if (engine_id == ENGINE_RETRO) return retro_engine_rate();
#endif
    return engine_id == ENGINE_KLATT ? 8000u : 22050u;
}
void engine_set_params(unsigned dt_speed, unsigned dt_pitch)
{
#ifdef HAVE_KLATT
    if (engine_id == ENGINE_KLATT) { klatt_engine_set_params(dt_speed, dt_pitch); return; }
#endif
#ifdef HAVE_RETRO
    if (engine_id == ENGINE_RETRO) { retro_engine_set_params(dt_speed, dt_pitch); return; }
#endif
#ifndef NO_SAM
    SetSpeed((unsigned char)(72 + (7 - (int)dt_speed) * 6));   /* Provox's default 7S = SAM's normal 72 */
    SetPitch((unsigned char)(64 + (50 - (int)dt_pitch)));
#endif
}
void engine_speak(char *utt, unsigned len)
{
    unsigned i;
#ifdef HAVE_KLATT
    if (engine_id == ENGINE_KLATT) { klatt_engine_speak(utt, len); return; }
#endif
#ifdef HAVE_RETRO
    if (engine_id == ENGINE_RETRO) { retro_engine_speak(utt, len); return; }
#endif
#ifndef NO_SAM
    for (i = 0; i < len; i++) { char c = utt[i]; if (c >= 'a' && c <= 'z') c -= 32; utt[i] = c; }
    utt[len] = '['; utt[len + 1] = 0;
    if (TextToPhonemes((unsigned char *)utt)) {
        SetInput(utt);
        ring_begin_utterance();
        SAMMain();
        ring_end_utterance();
    }
#else
    (void)utt; (void)len; (void)i;
#endif
}
