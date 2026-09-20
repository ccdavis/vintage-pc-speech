/* nrl.c - English text -> SAMPA phones.  Port of rsynth's text.c (rule
 * matcher), english.c (rules, numbers, character names) and the word/number
 * scanner from say.c, without the dictionary and without malloc/stdio.
 *
 * Copyright (c) 1994,2001-2002 Nick Ing-Simmons. All rights reserved.
 * Integer/no-libc port (c) 2026 accessible_os FreeDOS speech project.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */
#include "nrl.h"

#define NRL_WORDMAX 64

int nrl_stress_mode = 2;   /* 2: primary stress on every vowel (best Whisper WER, see NOTES.md) */

/* ---- tiny ctype (ASCII only, no locale) ---- */
static int is_alpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static int is_digit(int c) { return c >= '0' && c <= '9'; }
static int is_upper(int c) { return c >= 'A' && c <= 'Z'; }
static int is_lower(int c) { return c >= 'a' && c <= 'z'; }
static int to_lower(int c) { return is_upper(c) ? c + 32 : c; }
static int is_space(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static int is_punct(int c) { return c > 32 && c < 127 && !is_alpha(c) && !is_digit(c); }

static int vowel(int chr, int maybe)
{
    switch (chr) {
    case 'a': case 'e': case 'i': case 'o': case 'u':
    case 'A': case 'E': case 'I': case 'O': case 'U':
        return 1;
    case 'y': case 'Y':
        return maybe;
    }
    return 0;
}
static int consonant(int chr) { return is_alpha(chr) && !vowel(chr, 0); }

/* ---- rules ---- */
static const char Anything[] = "";
static const char Nothing[] = " ";
static const char Silent[] = "";

#include "nrl_rules.inc"

static const nrl_rule_t *const Rules[27] = {
    punct_rules,
    A_rules, B_rules, C_rules, D_rules, E_rules, F_rules, G_rules,
    H_rules, I_rules, J_rules, K_rules, L_rules, M_rules, N_rules,
    O_rules, P_rules, Q_rules, R_rules, S_rules, T_rules, U_rules,
    V_rules, W_rules, X_rules, Y_rules, Z_rules
};

static const nrl_rule_t *rules_for(int ch)
{
    if (is_alpha(ch))
        return Rules[to_lower(ch) - 'a' + 1];
    return Rules[0];
}

/* ---- output buffer ---- */
typedef struct {
    char *p;
    int16_t n, max;
} obuf_t;

static void out_ch(obuf_t *o, char c)
{
    if (o->n < o->max - 1) {
        o->p[o->n++] = c;
        o->p[o->n] = 0;
    }
}
static void out_str(obuf_t *o, const char *s)
{
    while (*s)
        out_ch(o, *s++);
}

/* ---- context matching (text.c) ---- */
static int leftmatch(const char *pattern, const char *context)
{
    const char *pat;
    const char *text;
    int count = 0;
    if (*pattern == 0)
        return 1;
    while (pattern[count]) count++;
    pat = pattern + (count - 1);
    text = context;
    for (; count > 0; pat--, count--) {
        if (is_alpha(*pat) || *pat == '\'' || *pat == ' ') {
            if (*pat != *text)
                return 0;
            text--;
            continue;
        }
        switch (*pat) {
        case '#':
            if (!vowel(*text, 0)) return 0;
            text--;
            while (vowel(*text, 0)) text--;
            break;
        case ':':
            while (consonant(*text)) text--;
            break;
        case '^':
            if (!consonant(*text)) return 0;
            text--;
            break;
        case '.':
            if (*text != 'b' && *text != 'd' && *text != 'v' && *text != 'g' && *text != 'j' &&
                *text != 'l' && *text != 'm' && *text != 'n' && *text != 'r' && *text != 'w' &&
                *text != 'z')
                return 0;
            text--;
            break;
        case '+':
            if (*text != 'e' && *text != 'i' && *text != 'y') return 0;
            text--;
            break;
        default:
            return 0;
        }
    }
    return 1;
}

static int rightmatch(const char *pattern, const char *context)
{
    const char *pat;
    const char *text = context;
    if (*pattern == 0)
        return 1;
    for (pat = pattern; *pat; pat++) {
        if (is_alpha(*pat) || *pat == '\'' || *pat == ' ') {
            if (*pat != *text)
                return 0;
            text++;
            continue;
        }
        switch (*pat) {
        case '#':
            if (!vowel(*text, 0)) return 0;
            text++;
            while (vowel(*text, 0)) text++;
            break;
        case '=':
            if (*text == 's' && text[1] == ' ') text++;
            if (*text != ' ') return 0;
            text++;
            break;
        case ':':
            while (consonant(*text)) text++;
            break;
        case '^':
            if (!consonant(*text)) return 0;
            text++;
            break;
        case '.':
            if (*text != 'b' && *text != 'd' && *text != 'v' && *text != 'g' && *text != 'j' &&
                *text != 'l' && *text != 'm' && *text != 'n' && *text != 'r' && *text != 'w' &&
                *text != 'z')
                return 0;
            text++;
            break;
        case '+':
            if (*text != 'e' && *text != 'i' && *text != 'y') return 0;
            text++;
            break;
        case '%':
            if (*text == 'e') {
                text++;
                if (*text == 'l') {
                    text++;
                    if (*text == 'y') { text++; break; }
                    text--;
                    break;
                } else if (*text == 'r' || *text == 's' || *text == 'd')
                    text++;
                break;
            } else if (*text == 'i') {
                text++;
                if (*text == 'n') {
                    text++;
                    if (*text == 'g') { text++; break; }
                }
                return 0;
            }
            return 0;
        default:
            return 0;
        }
    }
    return 1;
}

static int find_rule(obuf_t *o, const char *word, int index, const nrl_rule_t *rules)
{
    for (;;) {
        const nrl_rule_t *rule = rules++;
        const char *match = rule->match;
        int remainder;
        if (match == 0)
            return index + 1;          /* no rule: skip the character */
        for (remainder = index; *match; match++, remainder++)
            if (*match != word[remainder])
                break;
        if (*match)
            continue;
        if (!leftmatch(rule->left, &word[index - 1]))
            continue;
        if (!rightmatch(rule->right, &word[remainder]))
            continue;
        out_str(o, rule->output);
        return remainder;
    }
}

static void guess_word(obuf_t *o, const char *word)
{
    int index = 1;
    do {
        index = find_rule(o, word, index, rules_for(word[index]));
    } while (word[index]);
}

/* SAMPA vowel letters as used by the rules' output */
static int is_sampa_vowel(int c)
{
    switch (c) {
    case 'a': case 'e': case 'i': case 'o': case 'u':
    case 'A': case 'E': case 'I': case 'O': case 'U':
    case 'Q': case 'V': case '@': case '{': case '3': case '&': case '}':
        return 1;
    }
    return 0;
}

/* The NRL rules carry no lexical stress (rsynth gets it from a dictionary
 * we do not have).  Without it every vowel takes its short "unstressed"
 * duration and the F0 contour is a flat decline, which hurts intelligibility
 * a lot.  Heuristic (nrl_stress_mode 1): primary stress (') on the first
 * vowel of a word, tertiary (+) on the following ones; mode 2: primary on
 * every vowel; mode 3: primary then secondary (,). */
static void add_stress(obuf_t *o, int16_t from)
{
    int16_t i = from;
    int first = 1;
    while (i < o->n) {
        if (is_sampa_vowel((unsigned char)o->p[i]) && (i == from || !is_sampa_vowel((unsigned char)o->p[i - 1]))) {
            char mark = first ? '\'' : (nrl_stress_mode == 2 ? '\'' : nrl_stress_mode == 3 ? ',' : '+');
            int16_t j;
            if (o->n >= o->max - 1)
                return;
            for (j = o->n; j >= i; j--)
                o->p[j + 1] = o->p[j];
            o->p[i] = mark;
            o->n++;
            first = 0;
            i += 2;
            continue;
        }
        i++;
    }
}

int16_t nrl_word(const char *s, int16_t n, char *out, int16_t outmax)
{
    char word[NRL_WORDMAX + 3];
    obuf_t o;
    int16_t i, d = 0;
    o.p = out; o.max = outmax; o.n = 0;
    while (out[o.n]) o.n++;            /* append */
    if (n > NRL_WORDMAX) n = NRL_WORDMAX;
    word[d++] = ' ';
    for (i = 0; i < n; i++)
        word[d++] = (char)to_lower((unsigned char)s[i]);
    word[d++] = ' ';
    word[d] = 0;
    {
        int16_t before = o.n;
        guess_word(&o, word);
        if (nrl_stress_mode)
            add_stress(&o, before);
        return (int16_t)(o.n - before);
    }
}

/* ---- numbers (english.c cardinal(), made iterative) ---- */
static const char *const Cardinals[] = {
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
    "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen",
    "seventeen", "eighteen", "nineteen"
};
static const char *const Twenties[] = {
    "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"
};

static void say_word(obuf_t *o, const char *w)
{
    int16_t n = 0;
    while (w[n]) n++;
    nrl_word(w, n, o->p, o->max);
    while (o->p[o->n]) o->n++;
}

/* 1..999 */
static void say_small(obuf_t *o, int32_t value)
{
    if (value >= 100) {
        say_word(o, Cardinals[value / 100]);
        say_word(o, "hundred");
        value %= 100;
        if (value == 0) return;
    }
    if (value >= 20) {
        say_word(o, Twenties[(value - 20) / 10]);
        value %= 10;
        if (value == 0) return;
    }
    say_word(o, Cardinals[value]);
}

static void say_cardinal(obuf_t *o, int32_t value)
{
    if (value < 0) {
        say_word(o, "minus");
        value = -value;
    }
    if (value >= 1000000000L) {
        say_small(o, value / 1000000000L);
        say_word(o, "billion");
        value %= 1000000000L;
        if (value == 0) return;
        if (value < 100) say_word(o, "and");
    }
    if (value >= 1000000L) {
        say_small(o, value / 1000000L);
        say_word(o, "million");
        value %= 1000000L;
        if (value == 0) return;
        if (value < 100) say_word(o, "and");
    }
    /* 1100..1999 are said as "eleven hundred" etc. */
    if ((value >= 1000L && value <= 1099L) || value >= 2000L) {
        say_small(o, value / 1000L);
        say_word(o, "thousand");
        value %= 1000L;
        if (value == 0) return;
        if (value < 100) say_word(o, "and");
    }
    if (value >= 100) {
        say_word(o, Cardinals[value / 100]);
        say_word(o, "hundred");
        value %= 100;
        if (value == 0) return;
    }
    if (value >= 20) {
        say_word(o, Twenties[(value - 20) / 10]);
        value %= 10;
        if (value == 0) return;
    }
    say_word(o, Cardinals[value]);
}

/* ---- character names (english.c ASCII[]) ---- */
static const char *const char_name[96] = {
    "space", "exclamation mark", "double quote", "hash",
    "dollar", "percent", "ampersand", "quote",
    "open parenthesis", "close parenthesis", "asterisk", "plus",
    "comma", "minus", "full stop", "slash",
    "zero", "one", "two", "three", "four", "five", "six", "seven",
    "eight", "nine", "colon", "semi colon",
    "less than", "equals", "greater than", "question mark",
    "at", "ay", "bee", "see", "dee", "e", "eff", "gee",
    "aych", "i", "jay", "kay", "ell", "em", "en", "oh",
    "pee", "kju", "are", "es", "tee", "you", "vee", "double you",
    "eks", "why", "zed", "open bracket",
    "back slash", "close bracket", "circumflex", "underscore",
    "back quote", "ay", "bee", "see", "dee", "e", "eff", "gee",
    "aych", "i", "jay", "kay", "ell", "em", "en", "oh",
    "pee", "kju", "are", "es", "tee", "you", "vee", "double you",
    "eks", "why", "zed", "open brace",
    "vertical bar", "close brace", "tilde", "delete"
};

static void spell_out(obuf_t *o, const char *w, int16_t n)
{
    while (n-- > 0) {
        int c = (unsigned char)*w++;
        const char *name;
        if (c < 32 || c > 127)
            continue;
        name = char_name[c - 32];
        /* the names are words; "kju" etc. are already phonetic-ish spellings
         * that the rules handle */
        say_word(o, name);
        out_ch(o, ' ');
    }
}

/* say.c suspect_word(): no vowel, or mixed case -> spell it */
static int suspect_word(const char *w, int16_t n)
{
    int16_t i;
    int seen_lower = 0, seen_upper = 0, seen_vowel = 0, last = 0;
    for (i = 0; i < n; i++) {
        int ch = (unsigned char)w[i];
        if (i && last != '-' && is_upper(ch))
            seen_upper = 1;
        if (is_lower(ch)) {
            seen_lower = 1;
            ch -= 32;
        }
        if (vowel(ch, 1))
            seen_vowel = 1;
        last = ch;
    }
    if (!seen_lower && seen_vowel && n >= 3)
        return 0;                      /* short all-caps acronym with a vowel: say it as a word (DOS, NASA) */
    return !seen_vowel || (seen_upper && seen_lower) || !seen_lower;
}

static void xlate_word(obuf_t *o, const char *word, int16_t n)
{
    if (*word == '[') {                /* [phones] pass-through */
        int16_t i;
        for (i = 1; i < n && word[i] != ']'; i++)
            out_ch(o, word[i]);
        out_ch(o, ' ');
        return;
    }
    if (suspect_word(word, n)) {
        spell_out(o, word, n);
        return;
    }
    /* split at '.' or '-' inside a word */
    {
        int16_t i;
        for (i = 0; i < n; i++) {
            if (word[i] == '.' || word[i] == '-') {
                xlate_word(o, word, i);
                xlate_word(o, word + i + 1, (int16_t)(n - i - 1));
                return;
            }
        }
    }
    nrl_word(word, n, o->p, o->max);
    while (o->p[o->n]) o->n++;
}

int16_t nrl_translate(const char **textp, char *phones, int16_t max)
{
    const char *s = *textp;
    obuf_t o;
    o.p = phones; o.max = max; o.n = 0;
    phones[0] = 0;
    while (is_space((unsigned char)*s))
        s++;
    while (*s && o.n < max - 16) {
        int ch = (unsigned char)*s;
        const char *word = s;
        int16_t before = o.n;
        if (is_alpha(ch)) {
            while (is_alpha(ch = (unsigned char)*s) ||
                   ((ch == '\'' || ch == '-' || ch == '.') && is_alpha((unsigned char)s[1])))
                s++;
            if (!ch || is_space(ch) || is_punct(ch) ||
                (is_digit(ch) && !suspect_word(word, (int16_t)(s - word)))) {
                xlate_word(&o, word, (int16_t)(s - word));
            } else {
                while ((ch = (unsigned char)*s) && !is_space(ch) && !is_punct(ch))
                    s++;
                spell_out(&o, word, (int16_t)(s - word));
            }
        } else if (is_digit(ch) || (ch == '-' && is_digit((unsigned char)s[1]))) {
            int32_t sign = (ch == '-') ? -1 : 1;
            int32_t value = 0;
            int ndig = 0;
            if (sign < 0)
                s++;
            while (is_digit(ch = (unsigned char)*s)) {
                if (ndig < 9)
                    value = value * 10 + (ch - '0');
                ndig++;
                s++;
            }
            if (ndig > 9) {
                spell_out(&o, word, (int16_t)(s - word));   /* too big: spell digits */
            } else {
                say_cardinal(&o, value * sign);
                if (ch == '.' && is_digit((unsigned char)s[1])) {
                    word = ++s;
                    say_word(&o, "point");
                    while (is_digit((unsigned char)*s))
                        s++;
                    spell_out(&o, word, (int16_t)(s - word));
                }
            }
        } else if (ch == '[') {
            while (*s && *s++ != ']')
                ;
            xlate_word(&o, word, (int16_t)(s - word));
        } else if (is_punct(ch)) {
            switch (ch) {
            case '!': case '?': case '.': case ';': case ',': case '(': case ')':
                if ((!s[1] || is_space((unsigned char)s[1])) && o.n) {
                    s++;
                    out_ch(&o, ' ');
                    *textp = s;
                    return o.n;        /* end of clause: caller synthesizes */
                }
                s++;
                out_ch(&o, ' ');
                break;
            case '"': case ':': case '-':
                s++;
                out_ch(&o, ' ');
                break;
            default:
                spell_out(&o, word, 1);
                s++;
                break;
            }
        } else {
            while ((ch = (unsigned char)*s) && !is_space(ch))
                s++;
            spell_out(&o, word, (int16_t)(s - word));
        }
        if (o.n >= max - 1 && before > 0) {
            /* the buffer filled inside this token (out_ch drops characters
             * silently): give it back whole on the next call instead of
             * saying a cut word.  A single token longer than the buffer
             * (before == 0) stays truncated. */
            o.n = before;
            phones[before] = 0;
            s = word;
            break;
        }
        while (is_space((unsigned char)*s))
            s++;
    }
    *textp = s;
    return o.n;
}
