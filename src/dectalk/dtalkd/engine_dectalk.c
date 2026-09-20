/* DTALKD engine glue: synth.c/ring.c drive DECtalk (ARM7 single-threaded configuration).
 * DECtalk delivers 71-sample (11025 Hz) or 51-sample (8000 Hz) 16-bit blocks through its
 * callback; we convert to unsigned 8-bit and append to the sample ring, which yields the
 * coroutine when full.  A flush (ring_abort) makes the callback discard the rest of the
 * utterance (see cb). */
#include <string.h>
#include <stdio.h>
#include "ring.h"
#include "engine.h"
#include "epsonapi.h"

int engine_id = 3;
static int rate = 11025, blksize = 71, fmt = WAVE_FORMAT_1M16;
static short blk[71];
static unsigned char out8[71];
static unsigned cur_speed = 5, cur_pitch = 50;
static int discard_output = 0;
static char prefix[48];
unsigned long engine_utterances = 0, engine_aborted = 0, engine_utt_start = 0, engine_resets = 0;

static short *cb(short *b, long code)
{
    int i;
    if (code == 3) return b;                          /* index mark */
    if (!discard_output) {
        for (i = 0; i < blksize; i++) out8[i] = (unsigned char)((b[i] >> 8) + 128);
        ring_write_block(out8, (unsigned)blksize);
        /* flush: never return NULL.  DECtalk's embedded "halting" path re-initialises the engine
         * and the next TextToSpeechStart then loops forever in the LTS (reproduced on the host with
         * dtsay -a); the utterance is at most 200 characters and synthesis runs far above real
         * time, so simply discard the rest of it. */
        if (ring_abort) discard_output = 1;
    }
    return b;
}
int engine_init_dectalk(int r)
{
    rate = r == 8000 ? 8000 : 11025;
    blksize = rate == 8000 ? 51 : 71;
    fmt = rate == 8000 ? WAVE_FORMAT_08M16 : WAVE_FORMAT_1M16;
    return TextToSpeechInit(cb, NULL) == ERR_NOERROR ? 0 : -1;
}
void engine_init(int which) { (void)which; }
unsigned engine_rate(void) { return (unsigned)rate; }
void engine_set_params(unsigned dt_speed, unsigned dt_pitch)
{   /* DoubleTalk speed 0..9 (5 normal) -> DECtalk words per minute; pitch 0..99 (50 normal) -> average pitch Hz */
    cur_speed = dt_speed > 9 ? 9 : dt_speed; cur_pitch = dt_pitch > 99 ? 99 : dt_pitch;
    sprintf(prefix, "[:ra %u][:dv ap %u]", 100 + cur_speed * 20, 60 + (cur_pitch * 5) / 4);
}
void engine_speak(char *utt, unsigned len)
{
    static char text[256 + 64];
    int r;
    utt[len] = 0;
    engine_utterances++;
    engine_utt_start = ring_written;
    ring_begin_utterance();
    strcpy(text, prefix); strcat(text, utt);
    discard_output = 0;
    r = TextToSpeechStart(text, blk, fmt);
    if (r == ERR_RESET) engine_resets++;
    discard_output = 0;
    if (ring_abort) { engine_aborted++; return; }
    ring_end_utterance();
}
/* run the pipeline once with the output discarded so every lazily initialised table exists
 * before the engine only ever runs from interrupt context */
void engine_warmup(void)
{
    static char text[] = "Warm up: 1,234.5 dollars; the 3rd e-mail from Mr. O'Neil at 10:30 AM (test) & 50% done? Yes!";
    discard_output = 1;
    engine_set_params(5, 50);
    TextToSpeechStart(text, blk, fmt);
    engine_set_params(9, 99);
    TextToSpeechStart("Twenty-two 7 9 2026 x y z. ", blk, fmt);
    engine_set_params(5, 50);
    discard_output = 0;
}
