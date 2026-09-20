#ifndef _ENGLISH_H
#define _ENGLISH_H
#include "ring.h"
/***************************************************************************
 *
 * ENGLISH.H
 *
 * Description - This is the header that .
 *
 * Author      - Jonathan Hornstein
 *
 * Change Log
 *
 * Date      Initials   Description
 * 20-05-92  JSH        Created today
 *
 ***************************************************************************/
extern void *SearchArray (const void   **aUser,
                          const size_t   uiElemSiz,
                          size_t         uiNoOfElem,
                          int          (*pfnCompare)(const void *, const void *),
                          const void    *pszElem);

extern void ConvertTokenPhoneme (const char *pszToken);
#endif
