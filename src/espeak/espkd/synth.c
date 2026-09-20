/* Copied unchanged from ../../sbtalk/. */
/* DoubleTalk/LiteTalk protocol -> utterances -> SAM. Portable; runs as a coroutine on DOS. */
#include <setjmp.h>
#include <string.h>
#include "ring.h"
#include "synth.h"
#include "engine.h"

#define UTT_MAX 200
static char utt[256];
static unsigned utt_len;
static jmp_buf restart;
static unsigned cmd_val; static unsigned char in_cmd, cmd_has_val;
static unsigned char dt_speed = 5, dt_pitch = 50, dt_volume = 5;
volatile unsigned char synth_stage = 0;
volatile unsigned char synth_waiting = 0;   /* 1 while idle in the text loop (host harness uses it) */

static void apply_params(void)
{   /* DoubleTalk: speed 0..9 (9 fastest), pitch 0..99 (50 normal), volume 0..9 */
    engine_set_params(dt_speed, dt_pitch);
    set_volume(dt_volume);
}
static void do_command(unsigned char letter)
{
    unsigned v = cmd_has_val ? cmd_val : 0;
    switch (letter) {
    case 'S': case 's': dt_speed = v > 9 ? 9 : v; break;
    case 'P': case 'p': dt_pitch = v > 99 ? 99 : v; break;
    case 'V': case 'v': dt_volume = v > 9 ? 9 : v; break;
    case '@': dt_speed = 5; dt_pitch = 50; dt_volume = 5; break;   /* reset */
    default: break;                        /* F, X, M, T, B, E, A, R, I, ... accepted and ignored */
    }
    apply_params();
}
static void speak(void)
{
    if (!utt_len) return;
    synth_stage = 1;
    engine_speak(utt, utt_len);
    synth_stage = 4;
    utt_len = 0;
}
void synth_main(void)
{
    int c;
    synth_stage = 10;
    apply_params();
    synth_stage = 11;
    setjmp(restart);
    ring_abort = 0; utt_len = 0; in_cmd = 0;
    for (;;) {
        if (ring_abort) { ring_abort = 0; utt_len = 0; in_cmd = 0; }
        c = text_get();
        if (c < 0) {
            if (utt_len && synth_idle_ticks() >= 2) speak();
            synth_waiting = (utt_len == 0);
            synth_yield();
            synth_waiting = 0;
            continue;
        }
        if (in_cmd) {
            if (c >= '0' && c <= '9') { cmd_val = cmd_val * 10 + (c - '0'); cmd_has_val = 1; }
            else { in_cmd = 0; do_command((unsigned char)c); }
            continue;
        }
        switch (c) {
        case 1:  in_cmd = 1; cmd_val = 0; cmd_has_val = 0; break;          /* ^A command prefix */
        case 24: utt_len = 0; break;                                        /* ^X flush (ISR already emptied) */
        case '\r': case '\n': case 0: speak(); break;
        default:
            if (c < ' ') break;
            if (utt_len < UTT_MAX) utt[utt_len++] = (char)c;
            if (c == '.' || c == '?' || c == '!' || utt_len >= UTT_MAX) speak();
            break;
        }
    }
}
/* called from platform code when a flush arrives while the synth is inside the engine */
void synth_restart_after_flush(void) { longjmp(restart, 1); }
