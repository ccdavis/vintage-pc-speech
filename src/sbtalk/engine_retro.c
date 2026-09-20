/* "1983 voice": SPEECH.COM's 36 one-bit phonemes (via horndrv's PHONEME.C, freeware/GPL) played through
 * the Sound Blaster. Text -> NRL rules (nrl.c) -> SAMPA-ish phones -> McGuire phoneme names -> bits at
 * 33,145 bit/s (the PC refresh-timer clock SPEECH.COM used) resampled to 22050 Hz with a box average. */
#include <string.h>
#include "ring.h"
#include "engine.h"
#include "../klatt/nrl.h"
#ifdef __WATCOMC__
#define RETRO_FAR __far
#else
#define RETRO_FAR
#endif
typedef struct { const char *name; const unsigned char RETRO_FAR *bits; unsigned len; } retro_ph_t;
extern const retro_ph_t retro_ph[]; extern const int retro_nph;
static const struct { const char *sampa; const char *retro; } map[] = {
    {"@U","OH"},{"oU","OH"},{"aI","AH EE"},{"aU","AH OO"},{"OI","AW EE"},{"eI","A"},{"tS","CH"},{"dZ","J"},
    {"i","EE"},{"I","IH"},{"e","EH"},{"E","EH"},{"{","AE"},{"A","AH"},{"Q","AW"},{"O","AW"},{"U","OO"},{"u","OO"},
    {"V","UH"},{"@","UH"},{"3","UH R"},{"R","R"},{"T","TH"},{"D","TZ"},{"S","SH"},{"Z","ZH"},{"N","N G"},{"j","Y"},
    {"p","P"},{"b","B"},{"t","T"},{"d","D"},{"k","K"},{"g","G"},{"m","M"},{"n","N"},{"f","F"},{"v","V"},{"s","S"},
    {"z","Z"},{"h","H"},{"w","W"},{"l","L"},{"r","R"},{" "," "},{"_"," "},{NULL,NULL}};
extern char nrl_phones[512];                 /* engine_sam.c: shared by the NRL engines */
#define phones nrl_phones
static unsigned char obuf[256];
static unsigned on, repeat_max = 2;
int retro_bitmode = 0;                          /* XT: play the raw 1-bit records at the bit rate (no per-sample work) */
#define BIT_RATE 33145u
/* bit expansion for the output interrupt in /BITS mode: a record byte b becomes the 8 samples
 * retro_hi4[b] (bits 7..4) then retro_lo4[b & 15] (bits 3..0), 4 samples packed per unsigned long
 * (0 -> 64, 1 -> 192); 1088 bytes instead of a 2 KB byte-indexed table, one AND more per byte */
unsigned long retro_hi4[256], retro_lo4[16];
static unsigned long step_q16 = 98514UL;      /* bits per output sample, Q16: 33145 bit/s at 22050 Hz */
static void flush_out(void) { if (on) { ring_write_block(obuf, on); on = 0; } }
static void play_name(const char *name)
{
    int i; unsigned r, k; unsigned long acc; const unsigned char RETRO_FAR *b; unsigned len;
    for (i = 0; i < retro_nph; i++) if (!strcmp(retro_ph[i].name, name)) break;
    if (i == retro_nph) return;
    b = retro_ph[i].bits; len = retro_ph[i].len;
    r = strlen(name); if (r > repeat_max) r = repeat_max;
    if (retro_bitmode) {                              /* raw record bytes; the output interrupt expands 1 bit -> 1 sample */
        while (r--) {
            unsigned j;
            for (j = 0; j < len; j++) { obuf[on++] = b[j]; if (on == sizeof obuf) { flush_out(); if (ring_abort) return; } }
        }
        return;
    }
    while (r--) {
        /* bit position in Q16; 33145/22050 = 1.5032 bits per sample */
        unsigned long pos = 0, step = step_q16, nbits = ((unsigned long)len * 8) << 16;
        while (pos + step <= nbits) {
            unsigned i0 = (unsigned)(pos >> 16), i1 = (unsigned)((pos + step) >> 16), n = 0; acc = 0;
            if (i1 >= len * 8) i1 = len * 8 - 1;      /* the box is inclusive: never read past the record */
            for (k = i0; k <= i1; k++) { acc += (b[k >> 3] >> (7 - (k & 7))) & 1; n++; }
            obuf[on++] = (unsigned char)(28 + (acc * 200) / n);
            if (on == sizeof obuf) { flush_out(); if (ring_abort) return; }
            pos += step;
        }
    }
}
static void play_names(const char *s)             /* "AH EE" -> two phonemes */
{
    char name[4]; unsigned n = 0;
    for (;; s++) {
        if (*s && *s != ' ') { if (n < 3) name[n++] = *s; }
        else { if (n) { name[n] = 0; play_name(name); n = 0; } else if (*s == ' ') play_name(" "); if (!*s) break; }
        /* a space that ends no name (the " " map entry for word gaps, or a double space) is the
         * 139 ms silence record, as horndrv plays one after every word */
    }
}
void retro_engine_init(void)
{
    unsigned i, k;
    for (i = 0; i < 256; i++) {
        unsigned long v = 0;
        for (k = 0; k < 4; k++) v |= (unsigned long)((i >> (7 - k)) & 1 ? 192 : 64) << (8 * k);   /* first sample in the low byte */
        retro_hi4[i] = v;
        if (i < 16) { v = 0; for (k = 0; k < 4; k++) v |= (unsigned long)((i >> (3 - k)) & 1 ? 192 : 64) << (8 * k); retro_lo4[i] = v; }
    }
}
unsigned retro_engine_rate(void) { return retro_bitmode ? BIT_RATE : 22050u; }
void retro_engine_set_params(unsigned dt_speed, unsigned dt_pitch)
{   /* pitch 50 = horndrv's 33 kbit/s clock; SPEECH.COM on a 4.77 MHz 8088 ran nearer 40 kbit/s (pitch ~65) */
    unsigned long bitrate = 33145UL * (dt_pitch + 25) / 75;
    step_q16 = (bitrate << 16) / 22050UL;
    repeat_max = dt_speed >= 8 ? 1 : 2;
}
void retro_engine_speak(char *utt, unsigned len)
{
    const char *text = utt, *p; int n, i; size_t l;
    utt[len] = 0; on = 0;
    ring_begin_utterance();
    while ((n = nrl_translate(&text, phones, (int16_t)sizeof phones)) > 0) {
        for (p = phones; *p; ) {
            if (*p == '\'' || *p == ',' || *p == '[' || *p == ']') { p++; continue; }   /* stress marks, brackets */
            for (i = 0; map[i].sampa; i++) { l = strlen(map[i].sampa); if (!strncmp(p, map[i].sampa, l)) break; }
            if (map[i].sampa) { play_names(map[i].retro); p += l; } else p++;
            if (ring_abort) return;
        }
        play_name(" ");
    }
    flush_out();
    ring_end_utterance();
}
