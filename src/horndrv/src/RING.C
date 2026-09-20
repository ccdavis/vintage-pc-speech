/***************************************************************************
 *
 * RING.C
 *
 * Description - This module manipulates any kind of ring circular buffer.
 *               All a ring buffer can do is to add or remove an element from
 *               the current head or tail of the buffer. This concept is to be
 *               used mainly for buffering I/O.
 *
 * Author      - Jonathan Hornstein
 *
 * Change Log
 *
 * Date      Initials   Description
 * 05-06-92  JSH        Created today
 *
 ***************************************************************************/
#include <string.h>

#include "ring.h"


/****************************************************************************
 *
 * IsRingEmpty - Checks if there is any characater in the ring buffer
 *
 * Arguements:   PRING  pointer to the ring buffer structure
 *
 * Returns       BOOL   TRUE means that the ring buffer is empty
 *
 *
 *****************************************************************************/
unsigned short RingElemCnt (PRING pRhdr)
{
    return(pRhdr->usRingChCnt);
}
/****************************************************************************
 *
 * IsRingEmpty - Checks if there is any characater in the ring buffer
 *
 * Arguements:   PRING  pointer to the ring buffer structure
 *
 * Returns       BOOL   TRUE means that the ring buffer is empty
 *
 *
 *****************************************************************************/
BOOL IsRingEmpty (PRING pRhdr)
{
    return(RingElemCnt(pRhdr) == 0);
}
/****************************************************************************
 *
 * IsRingFull - Checks if there is any characater in the ring buffer
 *
 * Arguements:  PRING  pointer to the ring buffer structure
 *
 * Returns      BOOL   TRUE means that the ring buffer is full
 *
 *
 *****************************************************************************/
BOOL IsRingFull (PRING pRhdr)
{
    return(RingElemCnt(pRhdr) == pRhdr->usRingBufSiz);
}
/****************************************************************************
 *
 * ReadChRing -  Read the oldest character in the ring. This does not remove the
 *               character from the ring
 *
 * Arguements: PRING   pointer to the ring buffer structure
 *             PUCHAR   the character to remove
 *
 * Returns     BOOL    Status TRUE read a character
 *
 *
 *****************************************************************************/
BOOL ReadChRing (PRING pRhdr, unsigned char *puchData)
{
    if(IsRingEmpty(pRhdr))
    {
        *puchData = '\0';
        return(FALSE);
    }
    *puchData = pRhdr->auchRingBuf[pRhdr->usTailPos];
    return(TRUE);
}
/****************************************************************************
 *
 * DelChRing -  Delete the oldest character in the ring.
 *
 * Arguements:  PRING   pointer to the ring buffer structure
 *
 * Returns      BOOL    Status TRUE a character was deleted
 *
 *
 *****************************************************************************/
BOOL DelChRing (PRING pRhdr)
{
BOOL bRet;

    if(!(bRet = IsRingEmpty(pRhdr)))
    {
        if(++pRhdr->usTailPos == pRhdr->usRingBufSiz)
        {
            pRhdr->usTailPos = 0;
        }
        pRhdr->usRingChCnt--;
    }
    return(!bRet);
}
/****************************************************************************
 *
 * RemoveChRing -  Remove a character from the ring. This removes the oldest
 *                 character in the ring
 *
 * Arguements: PRING   pointer to the ring buffer structure
 *             PUCHAR   the character to remove
 *
 * Returns     BOOL    Status TRUE retrieved a character
 *
 *
 *****************************************************************************/
BOOL RemoveChRing (PRING pRhdr, unsigned char *puchData)
{
    return(ReadChRing(pRhdr, puchData) ? DelChRing(pRhdr) :
                                         FALSE);
}
/****************************************************************************
 *
 * AddChRing -  Add a character to the ring. This character becomes the newest
 *              charactr in the ring. If the ring is full don't add the character
 *              but report an error to calling function
 *
 * Arguements: PRING   pointer to the ring buffer structure
 *             UCHAR   the character to add
 *
 * Returns     BOOL   Whether able to add or not
 *
 *
 *****************************************************************************/
BOOL AddChRing (PRING pRhdr, unsigned char uchData)
{
BOOL bDataOverRun;

    if(!(bDataOverRun = IsRingFull(pRhdr)))
    {
       pRhdr->auchRingBuf[pRhdr->usHeadPos++] = uchData;
       /* Update the head counter */
       pRhdr->usHeadPos %= pRhdr->usRingBufSiz;
       pRhdr->usRingChCnt++;
    }
    return(!bDataOverRun);
}
/****************************************************************************
 *
 * InitialiseRing - Initialise the ring buffer
 *
 * Arguements: PRING   pointer to the ring buffer structure
 *             PVOID   pointer to the data buffer
 *             size_t  the sizof the ring buffer
 *
 * Returns     BOOL   whether or not the initialise was successful
 *
 *
 *****************************************************************************/
BOOL InitialiseRing (PRING pRing, void near *pBuf, unsigned short usRingBufSiz)
{
    /* Clear out the ring buffer */
    memset(pBuf, '\0', usRingBufSiz);
    pRing->usRingBufSiz = usRingBufSiz;
    pRing->usHeadPos    = pRing->usTailPos = pRing->usRingChCnt = 0;
    pRing->auchRingBuf  = pBuf;
    return(TRUE);
}
