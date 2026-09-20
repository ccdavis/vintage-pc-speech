#ifndef RING_H
#define RING_H

#include "types.h"

/* ----------  RING ELEMENT  ---------- */
typedef struct _RING
{
    unsigned short      usRingBufSiz,
                        usHeadPos,
                        usTailPos,
                        usRingChCnt;
    unsigned char near *auchRingBuf;
} RING, near *PRING;

extern unsigned short RingElemCnt (PRING pRhdr);
extern BOOL IsRingEmpty           (PRING pRhdr);
extern BOOL IsRingFull            (PRING pRhdr);
extern BOOL ReadChRing            (PRING pRhdr, unsigned char near *puchData);
extern BOOL DelChRing             (PRING pRhdr);
extern BOOL RemoveChRing          (PRING pRhdr, unsigned char near *puchData);
extern BOOL AddChRing             (PRING pRhdr, unsigned char uchData);
extern BOOL InitialiseRing        (PRING pRing, void near *pBuf, unsigned short usRingBufSiz);
#endif