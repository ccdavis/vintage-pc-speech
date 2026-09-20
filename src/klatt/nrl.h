/* nrl.h - English text to SAMPA phones (rsynth's NRL letter-to-sound rules,
 * number and character spelling).  Integer only, no malloc/stdio.
 *
 * Copyright (c) 1994,2001-2002 Nick Ing-Simmons (rsynth english.c, text.c,
 * say.c); rules from NRL Report 7948 (1976).  LGPL 2 or later.
 */
#ifndef NRL_H
#define NRL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *left;
    const char *match;
    const char *right;
    const char *output;
} nrl_rule_t;

/* 1 (default): add heuristic stress marks (first vowel of each word gets
 * primary stress); 0: none, as plain rsynth without a dictionary. */
extern int nrl_stress_mode;

/* Translate one word (letters only, any case) with the NRL rules.
 * Appends phones to out (NUL terminated), returns number of chars added. */
int16_t nrl_word(const char *word, int16_t n, char *out, int16_t outmax);

/* Translate text up to the next sentence/clause boundary (. ! ? ; , ( )
 * followed by white space or end of text) or the end of the text.  Numbers
 * become cardinal words, odd tokens are spelled letter by letter.  Advances
 * *textp.  Returns the number of phone characters written (0 = nothing
 * left to say). */
int16_t nrl_translate(const char **textp, char *phones, int16_t max);

#ifdef __cplusplus
}
#endif
#endif /* NRL_H */
