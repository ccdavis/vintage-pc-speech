/* Minimal WAV writer for the host tools. */
#ifndef WAVOUT_H
#define WAVOUT_H
#include <stdio.h>
#include <stdint.h>
FILE *wav_open(const char *path, uint32_t sr, int bits);
void wav_write16(FILE *f, const int16_t *s, size_t n);
void wav_write8(FILE *f, const uint8_t *s, size_t n);
void wav_close(FILE *f);
#endif
