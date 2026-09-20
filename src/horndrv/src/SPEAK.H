#ifndef _SPEAK_H
#define _SPEAK_H
/***************************************************************************
 *
 * SPEAK.H
 *
 * Description - This is the code brings together the english code and
 *               the phonetic code to allow speech.
 *
 * Author      - Jonathan Hornstein
 *
 * Change Log
 *
 * Date      Initials   Description
 * 22-02-93  JSH        Created today
 *
 ***************************************************************************/
#include <dos.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "execdrv.h"
extern void TalkPhoneme (void);
extern void EnglishTokenToPhoneme (const char chTransferBuf);
#endif
