/* SAMSB.EXE: SAM text-to-speech straight to the Sound Blaster.
 * usage: samsb [-speed N] [-pitch N] text...   |   samsb [-speed N] [-pitch N] -f FILE  (speaks FILE line by line) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sb16.h"
#include "../sam-upstream/src/sam.h"
#include "../sam-upstream/src/reciter.h"
int debug = 0;
int main(int argc, char **argv) {
    char input[256] = ""; int i, a = 0x220, irq = 5, d = 1; char *e = getenv("BLASTER"), *p;
    uclock_t t0, t1; int len; char *file = NULL, *logf = NULL; FILE *f;
    if (e) for (p = e; *p; p++) { if (*p == 'A') a = strtol(p+1, NULL, 16); else if (*p == 'I') irq = atoi(p+1); else if (*p == 'D') d = atoi(p+1); }
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-speed") && i+1 < argc) SetSpeed(atoi(argv[++i]));
        else if (!strcmp(argv[i], "-pitch") && i+1 < argc) SetPitch(atoi(argv[++i]));
        else if (!strcmp(argv[i], "-f") && i+1 < argc) file = argv[++i];
        else if (!strcmp(argv[i], "-log") && i+1 < argc) logf = argv[++i];
        else { strncat(input, argv[i], 250 - strlen(input)); strncat(input, " ", 250 - strlen(input)); }
    }
    if (file) {                                   /* speak a text file, one line per utterance */
        char line[256];
        if (!(f = fopen(file, "r"))) { perror(file); return 1; }
        if (!sb_init(a, irq, d)) { printf("no Sound Blaster\n"); return 1; }
        while (fgets(line, 200, f)) {
            int n = 0;
            for (p = line; *p; p++) { if (*p >= 'a' && *p <= 'z') *p -= 32; if (*p == '\n' || *p == '\r') *p = ' '; if (*p > ' ') n++; }
            if (!n) continue;
            strcat(line, "[");
            if (!TextToPhonemes((unsigned char *)line)) continue;
            SetInput(line);
            if (!SAMMain()) continue;
            sb_play8((unsigned char *)GetBuffer(), GetBufferLength() / 50, 22050);
        }
        fclose(f); sb_shutdown(); return 0;
    }
    if (!input[0]) { printf("usage: samsb [-speed N] [-pitch N] text | -f FILE\n"); return 1; }
    for (p = input; *p; p++) if (*p >= 'a' && *p <= 'z') *p -= 32;
    strcat(input, "[");
    t0 = uclock();
    if (!TextToPhonemes((unsigned char *)input)) { printf("reciter failed\n"); return 1; }
    SetInput(input);
    if (!SAMMain()) { printf("SAM failed\n"); return 1; }
    t1 = uclock();
    len = GetBufferLength() / 50;
    {
        double audio = len / 22050.0, secs = (double)(t1 - t0) / UCLOCKS_PER_SEC;
        printf("synthesized %d samples (%.2f s of audio) in %.3f s, %.1f times real time\n", len, audio, secs, secs > 0 ? audio / secs : 0);
        if (logf && (f = fopen(logf, "a"))) {       /* a line SAM can read aloud */
            fprintf(f, "%d seconds of audio took %d milliseconds, %d times real time.\n", (int)(audio + 0.5), (int)(secs * 1000 + 0.5), (int)(secs > 0 ? audio / secs + 0.5 : 0));
            fclose(f);
        }
    }
    if (!sb_init(a, irq, d)) { printf("no Sound Blaster\n"); return 1; }
    sb_play8((unsigned char *)GetBuffer(), len, 22050);
    sb_shutdown();
    return 0;
}
