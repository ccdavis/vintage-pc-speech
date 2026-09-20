/* espk_host: text -> 8-bit WAV on the host, for WER tests and calibration.
 * usage: espk_host [-d ESPK.DAT] [-r 8000|11025] [-n 3..5] [-l flags] [-s step]
 *                  [-k kopen] [-g level_db] [-G gain_ref] [-S wpm] [-P pitch]
 *                  [-t] [-T] "text" out.wav */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "espk.h"

#include <sys/time.h>
static long host_clock(void) { struct timeval tv; gettimeofday(&tv, NULL); return (long)(tv.tv_sec * 1000000L + tv.tv_usec); }
#define TICKS_PER_SEC 1000000.0

static void wav_write(const char *file, const uint8_t *s, long n, int sr)
{
	FILE *f = fopen(file, "wb");
	uint32_t u; uint16_t h;
	if (!f) { perror(file); exit(1); }
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
	espk_cfg_t cfg;
	espk_stats_t st;
	const char *dat = "data/ESPK.DAT", *text = NULL, *out = NULL;
	uint8_t *s; long n; int i;
	espk_default_cfg(&cfg);
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] && i + 1 <= argc) {
			switch (argv[i][1]) {
			case 'd': dat = argv[++i]; break;
			case 'r': cfg.sr = atoi(argv[++i]); break;
			case 'n': cfg.nfcascade = atoi(argv[++i]); break;
			case 'l': cfg.flags = atoi(argv[++i]); break;
			case 's': cfg.step = atoi(argv[++i]); break;
			case 'k': cfg.kopen = atoi(argv[++i]); break;
			case 'g': cfg.level_db = atoi(argv[++i]); break;
			case 'G': cfg.gain_ref = atoi(argv[++i]); break;
			case 'S': cfg.speed = atoi(argv[++i]); break;
			case 'P': cfg.pitch = atoi(argv[++i]); break;
			case 't': cfg.trace = 1; break;
			case 'T': cfg.trace = 2; break;
			default: fprintf(stderr, "bad option %s\n", argv[i]); return 2;
			}
		} else if (!text) text = argv[i];
		else out = argv[i];
	}
	if (!text || !out) { fprintf(stderr, "usage: espk_host [options] text out.wav\n"); return 2; }
	espk_clock = host_clock;
	if (espk_init(dat, &cfg)) return 1;
	if (espk_speak(text, &s, &n, &st)) return 1;
	wav_write(out, s, n, cfg.sr);
	fprintf(stderr, "%ld samples (%.2f s) frames %ld wave %ld; front %.1f ms synth %.1f ms\n", n, (double)n / cfg.sr,
	        st.n_frames, st.n_wave, st.front_ticks * 1000.0 / TICKS_PER_SEC, st.synth_ticks * 1000.0 / TICKS_PER_SEC);
	return 0;
}
