#ifndef _EXECDRV_H
#define _EXECDRV_H
/***************************************************************************
 *
 * EXECDRV.H
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
#include "types.h"
#include "ring.h"

typedef struct _REQHDR
{
    unsigned char      uchReqHdrLen,
                       uchUnitNum,
                       uchCommandCode;
    unsigned           uiStatus;
    unsigned char      uchReserved[8],
                       uchMediaType;
    unsigned char far *puchTransferBuf;
    unsigned           uiTransferSiz;
} REQHDR;

/* Status Word of ReqHdr */
#define IS_ERROR_D  0x8000
#define IS_BUSY_D   0x0200
#define IS_DONE_D   0x0100

/* Values returned in lower byte of ReqHdr status */
#define WRITE_PROTECT_D    0x0000
#define INVALID_UNIT_D     0x0001
#define NOT_READY_D        0x0002
#define UNKNOWN_COMMAND_D  0x0003
#define CRC_ERROR_D        0x0004
#define BAD_REQ_LENGTH_D   0x0005
#define SEEK_ERROR_D       0x0006
#define UNKNOWN_MEDIA_D    0x0007
#define SECTOR_NOT_FOUND_D 0x0008
#define PAPER_OUT_D        0x0009
#define WRITE_FAULT_D      0x000A
#define READ_FAULT_D       0x000B
#define GENERAL_FAILURE_D  0x000C

#define WinCls()               _fmemset(MK_FP(0xb800,0), ' ', 3840)
#define WinScreenPoke(x, y, i) poke(0xb800, (((y*80) + x) * 2), 0x300+i);

extern void ExecEnglishCommand (REQHDR far *pReqHdrLoc);
extern void ExecPhonemeCommand (REQHDR far *pReqHdrLoc);
extern void PhonemeWrite       (const char *pszPhoneme);

extern RING WriteRing;

#endif
