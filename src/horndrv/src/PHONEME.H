#ifndef _PHONEME_H
#define _PHONEME_H

typedef struct _PHONEME
{
    char near *pszPhoneme,
              *achVoice;
    size_t     uiPhonemeSiz;
} PHONEME ;
extern void PlayPhoneme (const PHONEME *pPhoneme);
extern PHONEME near *FindPhoneme (const char near *pszPhoneme);
#endif