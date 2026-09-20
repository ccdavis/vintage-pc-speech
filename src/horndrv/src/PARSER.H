#ifndef _parser_H
#define _parser_H

#include "types.h"
/***************************************************************************
 *
 * parser.H
 *
 * Description - This is the header that describes DOS structuresetation.
 *
 * Author      - Jonathan Hornstein
 *
 * Change Log
 *
 * Date      Initials   Description
 * 20-05-92  JSH        Created today
 *
 ***************************************************************************/
extern BOOL AddCharParseBuf    (unsigned char  chInput);
extern BOOL RemoveCharParseBuf (unsigned char *pchInput);
extern BOOL IsParseBufFull     (void);
extern void InitParseRingBuf   (unsigned char *achRingBuf, size_t uiRingBufSiz);
extern char *NextToken         (void);
#endif
