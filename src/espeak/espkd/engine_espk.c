/* ESPKD engine glue: sbtalk's synth.c/ring.c drive eSpeak NG + klatt_fx (espk) as the engine.
 * engine_speak streams samples into the ring through wgout_sink; the ring yields (coroutine)
 * when full and raises wgout_abort on a flush so the pipeline unwinds without a longjmp. */
#include <string.h>
#include "ring.h"
#include "engine.h"
#include "espk.h"

int engine_id = 1;
static espk_cfg_t cfg;
static unsigned cur_speed = 7, cur_pitch = 50;
unsigned long engine_utterances = 0, engine_aborted = 0, engine_utt_start = 0;

static void sink(const uint8_t *p, int n)
{
    ring_write_block(p, (unsigned)n);
    if (ring_abort) wgout_abort = 1;
}
static void discard(const uint8_t *p, int n) { (void)p; (void)n; }

int engine_init_espk(const char *datafile, int rate)
{
    espk_default_cfg(&cfg);
    cfg.sr = rate;
    if (rate > 8000) cfg.nfcascade = 4;
    cfg.level_db = 3;
    return espk_init(datafile, &cfg);
}
void engine_init(int which) { (void)which; }
unsigned engine_rate(void) { return (unsigned)cfg.sr; }
void engine_set_params(unsigned dt_speed, unsigned dt_pitch)
{   /* DoubleTalk speed 0..9, Provox's default 7S = normal (175 wpm); pitch 0..99 = espeak's scale */
    unsigned pct = 100 + ((int)dt_speed - 7) * 8;
    cur_speed = dt_speed; cur_pitch = dt_pitch;
    espk_set_rate((int)(175 * pct / 100));
    espk_set_pitch((int)dt_pitch);
}
void engine_speak(char *utt, unsigned len)
{
    utt[len] = 0;
    engine_utterances++;
    engine_utt_start = ring_written;
    ring_begin_utterance();
    wgout_sink = sink;
    espk_speak_stream(utt);
    wgout_sink = NULL;
    if (ring_abort) { engine_aborted++; return; }
    ring_end_utterance();
}
/* run the whole pipeline once with the output discarded, so that every lazily allocated
 * buffer exists before the engine is only ever run from interrupt context */
void engine_warmup(void)
{
    static char text[] = "Warm up: 1,234.5 dollars; the 3rd e-mail from Mr. O'Neil at 10:30 AM (test) & 50% done? Yes!";
    wgout_sink = discard;
    engine_set_params(7, 50);
    espk_speak_stream(text);
    engine_set_params(9, 99);
    espk_speak_stream("Twenty-two 7 9 2026 x y z. ");
    engine_set_params(cur_speed = 5, cur_pitch = 50);
    wgout_sink = NULL;
}
