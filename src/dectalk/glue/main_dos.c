/* DTSAY.EXE - DECtalk through the Sound Blaster (DJGPP, non-resident).
 *   DTSAY [-r 8000|11025] [-w out.wav] [-q] text...     BLASTER=A220 I5 D1 honoured
 * Synthesizes the whole text into memory first (prints the synthesis time), then plays it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "epsonapi.h"
#include "sb16.h"

static short blk[71];
static int blksize = 71;
static unsigned char *pcm;           /* unsigned 8-bit output */
static unsigned long npcm, cappcm;

static short *cb(short *b, long code)
{
    int i;
    if (code == 3) return b;
    if (npcm + blksize > cappcm) {
        cappcm = cappcm ? cappcm * 2 : 65536;
        pcm = realloc(pcm, cappcm);
        if (!pcm) { printf("out of memory\n"); exit(1); }
    }
    for (i = 0; i < blksize; i++) pcm[npcm++] = (unsigned char)((b[i] >> 8) + 128);
    return b;
}
static void parse_blaster(int *a, int *i, int *d)
{
    char *e = getenv("BLASTER"), *p;
    if (!e) return;
    for (p = e; *p; p++) {
        if (*p == 'A') *a = (int)strtol(p + 1, NULL, 16);
        else if (*p == 'I') *i = atoi(p + 1);
        else if (*p == 'D') *d = atoi(p + 1);
    }
}
static void wav_write(const char *file, const unsigned char *s, unsigned long n, int sr)
{
    FILE *f = fopen(file, "wb"); unsigned long u; unsigned short h;
    if (!f) { perror(file); return; }
    fwrite("RIFF", 1, 4, f); u = 36 + n; fwrite(&u, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f); u = 16; fwrite(&u, 4, 1, f);
    h = 1; fwrite(&h, 2, 1, f); h = 1; fwrite(&h, 2, 1, f);
    u = sr; fwrite(&u, 4, 1, f); u = sr; fwrite(&u, 4, 1, f);
    h = 1; fwrite(&h, 2, 1, f); h = 8; fwrite(&h, 2, 1, f);
    fwrite("data", 1, 4, f); u = n; fwrite(&u, 4, 1, f);
    fwrite(s, 1, n, f); fclose(f);
}
static long ms(uclock_t t) { return (long)((t * 1000) / UCLOCKS_PER_SEC); }

int main(int argc, char **argv)
{
    int rate = 11025, quiet = 0, i, fmt, a = 0x220, irq = 5, dma = 1;
    char text[2048] = "", *wavfile = NULL;
    uclock_t t0, t1, t2; long heap0 = (long)sbrk(0), audio_ms, synth_ms;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-r") && i + 1 < argc) rate = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-w") && i + 1 < argc) wavfile = argv[++i];
        else if (!strcmp(argv[i], "-q")) quiet = 1;
        else { if (*text) strncat(text, " ", sizeof text - strlen(text) - 1); strncat(text, argv[i], sizeof text - strlen(text) - 1); }
    }
    if (!*text) { printf("usage: DTSAY [-r 8000|11025] [-w out.wav] [-q] text...\n"); return 2; }
    fmt = rate == 8000 ? WAVE_FORMAT_08M16 : WAVE_FORMAT_1M16;
    blksize = rate == 8000 ? 51 : 71;
    if (rate != 8000) rate = 11025;
    t0 = uclock();
    if (TextToSpeechInit(cb, NULL) != ERR_NOERROR) { printf("DECtalk init failed\n"); return 1; }
    t1 = uclock();
    if (TextToSpeechStart(text, blk, fmt) != ERR_NOERROR) printf("start returned early\n");
    t2 = uclock();
    audio_ms = (long)((npcm * 1000UL) / (unsigned long)rate); synth_ms = ms(t2 - t1);
    printf("DTSAY: init %ld ms; %ld.%02ld s of audio in %ld.%03ld s (%ld.%02ldx real time) at %d Hz, heap %ld KB\n",
           ms(t1 - t0), audio_ms / 1000, (audio_ms % 1000) / 10, synth_ms / 1000, synth_ms % 1000,
           synth_ms > 0 ? audio_ms / synth_ms : 0L, synth_ms > 0 ? (audio_ms * 100L / synth_ms) % 100 : 0L,
           rate, ((long)sbrk(0) - heap0) / 1024);
    if (wavfile) wav_write(wavfile, pcm, npcm, rate);
    if (!quiet) {
        parse_blaster(&a, &irq, &dma);
        if (!sb_init(a, irq, dma)) { printf("no Sound Blaster at %Xh\n", a); return 1; }
        sb_play8(pcm, npcm, (unsigned)rate);
        sb_shutdown();
    }
    return 0;
}
