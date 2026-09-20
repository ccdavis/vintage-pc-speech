/* Host harness: feed a DoubleTalk byte stream through the same parser + engine, write a WAV.
 * usage: sbtalk_host out.wav "text with \001 commands"  (\001 and \030 escapes accepted, \r newline) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ring.h"
#include "synth.h"
#include "engine.h"
extern volatile unsigned char synth_waiting;
static FILE *out; static unsigned long total; static int no_more_text = 0; static unsigned long flush_at = 0; static int flushed = 0;
static void drain(void) {
    unsigned char buf[512]; unsigned n;
    while (ring_committed - ring_consumed >= 512) { n = ring_pull(buf, 512); fwrite(buf, 1, n, out); total += n; }
}
void synth_yield(void) {
    unsigned char buf[512]; unsigned n;
    drain();
    if (flush_at && !flushed && total >= flush_at) {     /* simulate ^X arriving from the screen reader */
        flushed = 1; text_clear(); ring_flush(); text_put(24); { const char *m = "next.\r"; while (*m) text_put((unsigned char)*m++); }
        fprintf(stderr, "flush injected at %lu samples\n", total);
    }
    if (text_head == text_tail && synth_waiting) {       /* text exhausted and synth idle: flush the rest and stop */
        while (ring_committed > ring_consumed) { n = ring_pull(buf, ring_committed - ring_consumed > 512 ? 512 : (unsigned)(ring_committed - ring_consumed)); fwrite(buf, 1, n, out); total += n; }
        if (no_more_text) {
            unsigned char h[44]; unsigned long r = engine_rate(), d = total;
            memcpy(h, "RIFF", 4); *(unsigned *)(h+4) = 36 + d; memcpy(h+8, "WAVEfmt ", 8); *(unsigned *)(h+16) = 16;
            *(unsigned short *)(h+20) = 1; *(unsigned short *)(h+22) = 1; *(unsigned *)(h+24) = r; *(unsigned *)(h+28) = r;
            *(unsigned short *)(h+32) = 1; *(unsigned short *)(h+34) = 8; memcpy(h+36, "data", 4); *(unsigned *)(h+40) = d;
            fseek(out, 0, SEEK_SET); fwrite(h, 1, 44, out); fclose(out);
            fprintf(stderr, "%lu samples (%.2f s)\n", total, (double)total / engine_rate());
            exit(0);
        }
        no_more_text = 1;
    }
}
int synth_idle_ticks(void) { return 2; }
void synth_on_abort(void) {}
int main(int argc, char **argv) {
    const char *p; unsigned char h[44] = {0};
    if (argc > 1 && !strcmp(argv[1], "-k")) { engine_init(ENGINE_KLATT); argv++; argc--; }
    else if (argc > 1 && !strcmp(argv[1], "-r")) { engine_init(ENGINE_RETRO); argv++; argc--; } else engine_init(ENGINE_SAM);
    if (argc > 1 && !strcmp(argv[1], "-b")) {          /* bench path check: count samples only */
        static char text[256] = "Hello. This is the speech synthesizer speaking on Free DOS. One two three four five six seven.";   /* engine_speak() needs room for 256 bytes (SAM reads 255 and appends) */
        ring_bench = 1; engine_set_params(5, 50); engine_speak(text, (unsigned)strlen(text));
        fprintf(stderr, "bench count: %lu samples = %.2f s, sum %08lX\n", ring_written, (double)ring_written / engine_rate(), ring_bench_sum); return 0;
    }
    if (argc < 3) { fprintf(stderr, "usage: %s [-k] out.wav text [flush_at_samples]\n", argv[0]); return 1; }
    if (argc > 3) flush_at = strtoul(argv[3], NULL, 10);
    out = fopen(argv[1], "wb"); fwrite(h, 1, 44, out);
    for (p = argv[2]; *p; p++) {
        if (*p == '\\' && p[1]) { p++; if (*p == 'a') text_put(1); else if (*p == 'x') text_put(24); else if (*p == 'r') text_put('\r'); else text_put(*p); }
        else text_put((unsigned char)*p);
    }
    text_put('\r');
    synth_main();
    return 0;
}
