/* SBPLAY.EXE: play an 8-bit unsigned mono WAV through the Sound Blaster, or a test tone.
 * usage: sbplay file.wav | sbplay -tone [hz]        env BLASTER=A220 I5 D1 is honoured. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sb16.h"

static void parse_blaster(int *a, int *i, int *d) {
    char *e = getenv("BLASTER"), *p;
    if (!e) return;
    for (p = e; *p; p++) {
        if (*p == 'A') *a = (int)strtol(p + 1, NULL, 16);
        else if (*p == 'I') *i = atoi(p + 1);
        else if (*p == 'D') *d = atoi(p + 1);
    }
}

int main(int argc, char **argv) {
    int a = 0x220, i = 5, d = 1;
    sb_verbose = 1;
    parse_blaster(&a, &i, &d);
    if (!sb_init(a, i, d)) return 1;
    if (argc < 2) { printf("usage: sbplay file.wav | sbplay -tone [hz]\n"); return 1; }
    if (!stricmp(argv[1], "-tone")) {
        unsigned rate = 11025, n = rate * 2, k; double hz = argc > 2 ? atof(argv[2]) : 440.0;
        unsigned char *b = malloc(n);
        for (k = 0; k < n; k++) b[k] = (unsigned char)(128 + 100 * sin(6.2831853 * hz * k / rate));
        printf("tone %g Hz, 2 s at %u Hz\n", hz, rate);
        sb_play8(b, n, rate);
    } else {
        FILE *f = fopen(argv[1], "rb"); unsigned char hdr[64]; unsigned rate, bits, ch; long len, off;
        if (!f) { perror(argv[1]); return 1; }
        fread(hdr, 1, 44, f);
        rate = hdr[24] | hdr[25] << 8 | hdr[26] << 16; bits = hdr[34]; ch = hdr[22];
        for (off = 12; off < 60; off++) if (!memcmp(hdr + off, "data", 4)) break;
        len = hdr[off+4] | hdr[off+5] << 8 | hdr[off+6] << 16 | hdr[off+7] << 24; off += 8;
        printf("%s: %u Hz, %u bit, %u ch, %ld bytes\n", argv[1], rate, bits, ch, len);
        if (bits != 8 || ch != 1) { printf("need 8-bit mono\n"); return 1; }
        { unsigned char *b = malloc(len); fseek(f, off, SEEK_SET); len = fread(b, 1, len, f); fclose(f);
          sb_play8(b, len, rate); }
    }
    sb_shutdown();
    printf("done\n");
    return 0;
}
