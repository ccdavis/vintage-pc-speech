#include "wavout.h"
#include <string.h>
static void put32(FILE *f, uint32_t v) { uint8_t b[4] = { v, v >> 8, v >> 16, v >> 24 }; fwrite(b, 1, 4, f); }
static void put16(FILE *f, uint16_t v) { uint8_t b[2] = { v, v >> 8 }; fwrite(b, 1, 2, f); }
FILE *wav_open(const char *path, uint32_t sr, int bits)
{
    FILE *f = fopen(path, "wb");
    if (!f) return NULL;
    fwrite("RIFF", 1, 4, f); put32(f, 0); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put32(f, 16); put16(f, 1); put16(f, 1);
    put32(f, sr); put32(f, sr * bits / 8); put16(f, bits / 8); put16(f, bits);
    fwrite("data", 1, 4, f); put32(f, 0);
    return f;
}
void wav_write16(FILE *f, const int16_t *s, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) put16(f, (uint16_t)s[i]);
}
void wav_write8(FILE *f, const uint8_t *s, size_t n) { fwrite(s, 1, n, f); }
void wav_close(FILE *f)
{
    long len = ftell(f);
    fseek(f, 4, SEEK_SET); put32(f, len - 8);
    fseek(f, 40, SEEK_SET); put32(f, len - 44);
    fclose(f);
}
