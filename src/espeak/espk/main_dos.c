/* ESPK.EXE - eSpeak NG English frontend + integer Klatt through the Sound
 * Blaster (DJGPP, non-resident).  usage: ESPK [options] text...
 *   -r 8000|11025   output rate      -s N   Klatt frames per parameter step (default 3)
 *   -n 3..5         cascade formants -g dB  level trim       -q  synthesize only, no playback
 *   -w file.wav     also write the 8-bit WAV      -S wpm   speed (default 175)
 * ESPK.DAT is looked for next to ESPK.EXE.  BLASTER=A220 I5 D1 is honoured. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "espk.h"
#include "sb16.h"
#include <pc.h>
#include <time.h>
/* PC speaker output: PWM on PIT channel 2, paced by the PIT channel 0 counter (1.193182 MHz), so
 * it is independent of CPU speed and needs no RTC or interrupt; blocks while playing. */
static void spk_play8(const unsigned char *pcm, unsigned long n, unsigned rate)
{
    unsigned char lut[256], p61; unsigned i, prev, cur;
    unsigned long period = 1193182UL / rate;                 /* PIT ticks per sample, e.g. 149 at 8000 Hz */
    unsigned long acc = 0, pos = 0, ticks_q8 = (1193182UL * 256UL) / rate;
    uclock_t t0 = uclock(), limit = (uclock_t)n * UCLOCKS_PER_SEC / rate + UCLOCKS_PER_SEC;
    for (i = 0; i < 256; i++) lut[i] = (unsigned char)(1 + (unsigned long)i * (period - 2) / 255);
    p61 = inportb(0x61);
    outportb(0x43, 0xB0); outportb(0x42, lut[128]);           /* channel 2, lobyte only, mode 0 */
    outportb(0x61, p61 | 3);
    outportb(0x43, 0x00); prev = inportb(0x40); prev |= inportb(0x40) << 8;
    while (pos < n) {
        outportb(0x43, 0x00); cur = inportb(0x40); cur |= inportb(0x40) << 8;   /* latch + read channel 0 */
        acc += ((prev - cur) & 0xFFFFu) << 8; prev = cur;
        if (acc >= ticks_q8) { acc -= ticks_q8; outportb(0x42, lut[pcm[pos++]]); }
        if (uclock() - t0 > limit) break;
    }
    outportb(0x61, p61 & ~2);
}
#include <unistd.h>

/* integer only: the 386 DX-25 has no FPU (-lemu is linked as a safety net) */
static long ms(long ticks) { return (long)(((long long)ticks * 1000) / UCLOCKS_PER_SEC); }   /* 64-bit: 2 s of ticks * 1000 overflows 32 bits */

static long dos_clock(void) { return (long)uclock(); }

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

static void wav_write(const char *file, const uint8_t *s, long n, int sr)
{
	FILE *f = fopen(file, "wb");
	uint32_t u; uint16_t h;
	if (!f) { perror(file); return; }
	fwrite("RIFF", 1, 4, f); u = 36 + n; fwrite(&u, 4, 1, f);
	fwrite("WAVEfmt ", 1, 8, f); u = 16; fwrite(&u, 4, 1, f);
	h = 1; fwrite(&h, 2, 1, f); h = 1; fwrite(&h, 2, 1, f);
	u = sr; fwrite(&u, 4, 1, f); u = sr; fwrite(&u, 4, 1, f);
	h = 1; fwrite(&h, 2, 1, f); h = 8; fwrite(&h, 2, 1, f);
	fwrite("data", 1, 4, f); u = n; fwrite(&u, 4, 1, f);
	fwrite(s, 1, n, f);
	fclose(f);
}

int main(int argc, char **argv)
{
	int use_spk = 0;
	espk_cfg_t cfg;
	espk_stats_t st;
	char text[2048] = "", dat[260], *wavfile = NULL;
	int i, quiet = 0, a = 0x220, irq = 5, dma = 1;
	uint8_t *s; long n;
	long t0, t1, heap0, heap1, heap2, tot, audio_ms;

	espk_default_cfg(&cfg);
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] && argv[i][2] == 0 && (argv[i][1] == 'q' || i + 1 < argc)) {
			switch (argv[i][1]) {
			case 'r': cfg.sr = atoi(argv[++i]); break;
			case 'n': cfg.nfcascade = atoi(argv[++i]); break;
			case 's': cfg.step = atoi(argv[++i]); break;
			case 'g': cfg.level_db = atoi(argv[++i]); break;
			case 'S': cfg.speed = atoi(argv[++i]); break;
			case 'w': wavfile = argv[++i]; break;
			case 'k': use_spk = 1; break;
			case 'q': quiet = 1; break;
			default: printf("bad option %s\n", argv[i]); return 2;
			}
		} else {
			if (text[0]) strncat(text, " ", sizeof(text) - strlen(text) - 1);
			strncat(text, argv[i], sizeof(text) - strlen(text) - 1);
		}
	}
	if (!text[0]) {
		printf("usage: ESPK [-r rate] [-n formants] [-s step] [-g dB] [-S wpm] [-w out.wav] [-k (PC speaker)] [-q] text\n");
		return 2;
	}
	/* ESPK.DAT next to the EXE */
	strncpy(dat, argv[0], sizeof(dat) - 10); dat[sizeof(dat) - 10] = 0;
	{
		char *p = strrchr(dat, '\\'), *q = strrchr(dat, '/');
		if (q > p) p = q;
		if (p) strcpy(p + 1, "ESPK.DAT"); else strcpy(dat, "ESPK.DAT");
	}
	espk_clock = dos_clock;
	heap0 = (long)sbrk(0);
	t0 = uclock();
	if (espk_init(dat, &cfg)) return 1;
	t1 = uclock();
	heap1 = (long)sbrk(0);
	printf("ESPK: data loaded in %ld ms (%s), heap %ld KB\n", ms(t1 - t0), dat, (heap1 - heap0) / 1024);

	if (espk_speak(text, &s, &n, &st)) return 1;
	heap2 = (long)sbrk(0);
	audio_ms = (n * 1000L) / cfg.sr;
	tot = ms(st.front_ticks + st.synth_ticks);
	printf("%ld.%02ld s of audio in %ld.%03ld s (%ld.%02ldx real time): frontend %ld ms, synthesizer %ld ms (%ld Klatt frames, %ld sampled), heap %ld KB\n",
	       audio_ms / 1000, (audio_ms % 1000) / 10, tot / 1000, tot % 1000,
	       tot > 0 ? audio_ms / tot : 0L, tot > 0 ? (audio_ms * 100L / tot) % 100 : 0L,
	       ms(st.front_ticks), ms(st.synth_ticks), st.n_frames, st.n_wave, (heap2 - heap0) / 1024);
	if (wavfile) wav_write(wavfile, s, n, cfg.sr);
	if (!quiet) {
		parse_blaster(&a, &irq, &dma);
		if (use_spk) { uclock_t p0 = uclock(); spk_play8(s, n, cfg.sr); printf("speaker playback %ld ms\n", (long)((uclock() - p0) * 1000 / UCLOCKS_PER_SEC)); }
		else {
		if (!sb_init(a, irq, dma)) { printf("no Sound Blaster at %x\n", a); return 1; }
		sb_play8(s, n, cfg.sr);
		}
		sb_shutdown();
	}
	return 0;
}
