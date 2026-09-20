/***************************************************************************
 *
 * EXECDRV.C
 *
 * Description - This is the code for the phonem device driver.
 *
 * Author      - Jonathan Hornstein
 *
 * Change Log
 *
 * Date      Initials   Description
 * 04-06-92  JSH        Created today
 *
 ***************************************************************************/
#include <dos.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "execdrv.h"
#include "speak.h"
#include "ring.h"

/*#define DRIVER_DISPLAY */
#define INCL_BIOS_DISPLAY
#define AT_TYPE_MACHINE_D ((unsigned char)0xFC)
#define ESC_D             '\033'

static unsigned char achWriteRingBuf[128],
                     achReadRingBuf[128];
static char bQuietInit        = FALSE,
            bInterruptDisable = FALSE,
            bValidDriver      = TRUE,
            bTraceOn          = FALSE;
extern unsigned char _TheEnd;
RING        WriteRing,
            ReadRing;

#if defined(INCL_BIOS_DISPLAY)
#undef putchar
int errno = 0,
    _doserrno;    // Variable indicating actual DOS error code.
/***************************************************************************
 * putchar
 *
 * Description - This function displays a string from the driver
 *
 * Arguements  - char * string to display
 *
 * Returns     - None
 *
 ***************************************************************************/
static int putchar (int iDisplay)
{
union REGS regs;

    if(!bQuietInit)
    {
        regs.h.dl = (unsigned char)iDisplay;
        regs.h.ah = 0x06;
        int86(0x21, &regs, &regs);
    }
    return iDisplay;
}
/***************************************************************************
 * puts
 *
 * Description - This function displays a string from the driver
 *
 * Arguements  - char * string to display
 *
 * Returns     - None
 *
 ***************************************************************************/
static int puts (const char *pszDisplay)
{
    while(*pszDisplay)
    {
        putchar(*pszDisplay++);
    }
    putchar('\r');
    putchar('\n');
    return 0;
}
/***************************************************************************
 * _IOerror
 *
 * Description - Gets rid of bullsit code
 *
 * Arguements  -
 *
 * Returns     - arguement
 *
 ***************************************************************************/
int _IOERROR (int iAx)
{
    return(iAx);
}
#else
#define puts(a)
#endif
/***************************************************************************
 * strtoul
 *
 * Description - [ws] [sn] [0] [x] [ddd]
 *
 * Arguements  -
 *
 * Returns     -
 *
 ***************************************************************************/
int atoi (const char far *s)
{
int iValue;

    for(iValue=0; isdigit(*s); s++)
    {
       iValue = iValue * 10 + (*s - '0');
    }
    return iValue;
}
/***************************************************************************
 * InitPhonemeDriver
 *
 * Description - This function initialises the device driver.
 *               Note that only a few MS-DOS system functions are available
 *               during initialization (Interrupt 21h Functions 01h through
 *               0Ch, 25h, 30h, and 35h). In general, the interrupt routine
 *               can display messages at the standard output device, but it
 *               cannot open files or allocate additional memory.
 *
 * Arguements  - pReqHdr
 *
 * Returns     - unsigned
 *
 ***************************************************************************/
static unsigned InitPhonemeDriver (REQHDR far *pReqHdr)
{
    if(bValidDriver)
    {
        pReqHdr->puchTransferBuf = (void far *)&_TheEnd;
        InitialiseRing(&WriteRing, achWriteRingBuf, sizeof(achWriteRingBuf));
        InitialiseRing(&ReadRing, achReadRingBuf, sizeof(achReadRingBuf));

    }
    else
    {
        pReqHdr->puchTransferBuf = MK_FP(_CS, 0);
        pReqHdr->uchMediaType = 0;
        return IS_ERROR_D|IS_DONE_D;
    }
    return IS_DONE_D;
}
/***************************************************************************
 * InitEnglishDriver
 *
 * Description - This function initialises the device driver.
 *               Note that only a few MS-DOS system functions are available
 *               during initialization (Interrupt 21h Functions 01h through
 *               0Ch, 25h, 30h, and 35h). In general, the interrupt routine
 *               can display messages at the standard output device, but it
 *               cannot open files or allocate additional memory.
 *
 * Arguements  - pReqHdr
 *
 * Returns     - unsigned
 *
 ***************************************************************************/
static unsigned InitEnglishDriver (REQHDR far *pReqHdr)
{
char far *pszToken;
int       iDmaRate;

    if(!(bValidDriver = (AT_TYPE_MACHINE_D == (unsigned char)peekb(0xf000, 0xfffe))))
    {
        puts("Warning, Driver was designed for an AT-style computer");
    }
    pszToken     = *(char far * far *)&pReqHdr->uiTransferSiz;
    for(;*pszToken && bValidDriver;)
    {
        if(*pszToken++ == '-')
        {
            switch(toupper(*pszToken))
            {
                case 'D':
                     if((iDmaRate = atoi(pszToken+1))!=0)
                     {
	                    outportb(0x43, 0x46);     // Latch the DRAM DMA refresh counter
	                    outportb(0x41, iDmaRate); // Set the counter
                     }
                     break;

                case 'I':
                     bInterruptDisable = TRUE;
                     break;

                case 'Q':
                     bQuietInit = TRUE;
                     break;

                case 'T':
                     bTraceOn = TRUE;
                     break;

                default:
                     puts("Invalid talk driver option");
                     puts("Valid options are:");
                     puts("-d{num} DMA rate");
                     puts("-i      Disable Interrupts during speech");
                     puts("-q      Quiet no initialisation messages please");
                     bValidDriver = FALSE;
                     break;
            }
        }
    }
    if(bValidDriver)
    {
        puts("Text To Speech Device Driver");
        puts("Written by Jon Hornstein");
        puts("Melbourne, Australia");
        puts("Ver 0.5, " __DATE__);
        puts("");
    }
    else
    {
        puts("Talking Device Driver is not installed");
    }
    return InitPhonemeDriver(pReqHdr);
}
/***************************************************************************
 * OutputStatus
 *
 * Description - This function checks to see if the output ring is full yet
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 * Globals     - WriteRing
 *
 ***************************************************************************/
#pragma argsused
static unsigned OutputStatus (REQHDR far *pReqHdr)
{
    return(IS_DONE_D);
}
/***************************************************************************
 * DeviceEnglishWrite
 *
 * Description - This function
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 * Globals     - pReqHdr
 *
 ***************************************************************************/
static unsigned DeviceEnglishWrite (REQHDR far *pReqHdr)
{
    if(pReqHdr->uiTransferSiz > 0)
    {
        /* If you have something to speak say it */
        EnglishTokenToPhoneme(toupper(*pReqHdr->puchTransferBuf));
    }
    return(OutputStatus(pReqHdr));
}
/***************************************************************************
 * DevicePhonemeWrite

 *
 * Description - This function
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 * Globals     - pReqHdr
 *
 ***************************************************************************/
static unsigned DevicePhonemeWrite (REQHDR far *pReqHdr)
{
char               chTransferBuf;
static char        chScreenSize;

    if(pReqHdr->uiTransferSiz > 0)
    {
        chTransferBuf = toupper(*pReqHdr->puchTransferBuf);
        AddChRing(&ReadRing, chTransferBuf);
        if(bTraceOn)
        {
            WinScreenPoke(chScreenSize++, 23, chTransferBuf);
            chScreenSize %= 80;
        }
        if(!isalpha(chTransferBuf))
        {
            /* You now have something to talk about */
            TalkPhoneme();
        }
        if(isalpha(chTransferBuf) || isspace(chTransferBuf))
        {
            AddChRing(&WriteRing, chTransferBuf);
            if(isspace(chTransferBuf))
            {
                /* You now have something to talk about */
                TalkPhoneme();
            }
        }
    }
    return(OutputStatus(pReqHdr));
}
/***************************************************************************
 * InputFlush
 *
 * Description - This function
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 ***************************************************************************/
#pragma argsused
static unsigned InputFlush (REQHDR far *pReqHdr)
{
    for(;DelChRing(&WriteRing);)
    {
    }
    return(IS_DONE_D);
}
/***************************************************************************
 * EnglishInputStatus
 *
 * Description - This function
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 ***************************************************************************/
#pragma argsused
static unsigned EnglishInputStatus (REQHDR far *pReqHdr)
{
  return(!IsRingEmpty(&WriteRing) ? IS_BUSY_D : IS_DONE_D);
}
/***************************************************************************
 * EnglishNdRead
 *
 * Description - This function
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 * Globals     - pReqHdr
 *
 ***************************************************************************/
static unsigned EnglishNdRead (REQHDR far *pReqHdr)
{
unsigned char uchNDRead;

    if(!(EnglishInputStatus(pReqHdr) & IS_BUSY_D))
    {
        pReqHdr->uchMediaType = ReadChRing(&ReadRing,
                                           (void *)&uchNDRead) ? uchNDRead : 0;
    }
    return(EnglishInputStatus(pReqHdr));
}
/***************************************************************************
 * DeviceEnglishRead
 *
 * Description - This function reads from the device
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 * Globals     - pReqHdr
 *
 ***************************************************************************/
static unsigned DeviceEnglishRead (REQHDR far *pReqHdr)
{
unsigned char uchTransferBuf;

    pReqHdr->uiTransferSiz += !RemoveChRing(&ReadRing, &uchTransferBuf);
    *pReqHdr->puchTransferBuf = uchTransferBuf;
    return(EnglishInputStatus(pReqHdr));
}
/***************************************************************************
 * PhonemeNdRead
 *
 * Description - This function
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 * Globals     - pReqHdr
 *
 ***************************************************************************/
#pragma argsused
static unsigned PhonemeNdRead (REQHDR far *pReqHdr)
{
    return(IS_DONE_D|IS_ERROR_D|READ_FAULT_D);
}
/***************************************************************************
 * IgnoreCommand
 *
 * Description - This function
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 * Globals     - pReqHdr
 *
 ***************************************************************************/
#pragma argsused
static unsigned IgnoreCommand (REQHDR far *pReqHdr)
{
    return(IS_DONE_D);
}
/***************************************************************************
 * Driver Dispatch Table for DOS interface
 *
 * Description - This table is a jump table corresponding to command ID's
 *               for DOS commands
 *
 * Arguements  - Pointer to the request header
 *
 * Returns     - unsigned status
 *
 ***************************************************************************/
static unsigned (*pfnEnglishDispatch[])(REQHDR far *) =
{
    InitEnglishDriver   , /*  0 = Initialise driver              A */
    IgnoreCommand       , /*  1 = Media Check                    B */
    IgnoreCommand       , /*  2 = Build Bpb                      C */
    DeviceEnglishRead   , /*  3 = Ioctl Read                     D */
    DeviceEnglishRead   , /*  4 = Device Read                    E */
    EnglishNdRead       , /*  5 = Non destructive Read           F */
    EnglishInputStatus  , /*  6 = Input Status                   G */
    InputFlush          , /*  7 = Flush Input Buffers            H */
    DeviceEnglishWrite  , /*  8 = Device Write                   I */
    DeviceEnglishWrite  , /*  9 = Verify Device Write            J */
    OutputStatus        , /* 10 = Output Status                  K */
    InputFlush          , /* 11 = Flush Output Buffers           L */
    DeviceEnglishWrite  , /* 12 = Ioctl Write                    M */
    IgnoreCommand       , /* 13 = Device Open DOS 3+             N */
    IgnoreCommand       , /* 14 = Device Close DOS 3+            O */
    IgnoreCommand       , /* 15 = Removable Media                P */
    DeviceEnglishWrite  , /* 16 = Output until Busy              Q */
};
/***************************************************************************
 * ExecEnglishCommand
 *
 * Description - This function is the central driving function for driver
 *
 * Arguements  - pReqHdr   the DOS request header
 *
 * Returns     - void
 *
 * Globals
 *
 ***************************************************************************/
void ExecEnglishCommand (REQHDR far *pReqHdr)
{
    if(bInterruptDisable)
    {
        disable();
    }
    if(bTraceOn)
    {
        WinScreenPoke(79, pReqHdr->uchCommandCode, pReqHdr->uchCommandCode+'A');
    }
    pReqHdr->uiStatus = ((pReqHdr->uchCommandCode >= ARRAYSIZE_M(pfnEnglishDispatch)) ?
                                   UNKNOWN_COMMAND_D :
                                   (*pfnEnglishDispatch[pReqHdr->uchCommandCode])(pReqHdr));
}
static unsigned (*pfnPhonemeDispatch[])(REQHDR far *) =
{
    InitPhonemeDriver   , /*  0 = Initialise driver              A */
    IgnoreCommand       , /*  1 = Media Check                    B */
    IgnoreCommand       , /*  2 = Build Bpb                      C */
    IgnoreCommand       , /*  3 = Ioctl Read                     D */
    PhonemeNdRead       , /*  4 = Device Read                    E */
    PhonemeNdRead       , /*  5 = Non destructive Read           F */
    InputFlush          , /*  6 = Input Status                   G */
    IgnoreCommand       , /*  7 = Flush Input Buffers            H */
    DevicePhonemeWrite  , /*  8 = Device Write                   I */
    DevicePhonemeWrite  , /*  9 = Verify Device Write            J */
    OutputStatus        , /* 10 = Output Status                  K */
    IgnoreCommand       , /* 11 = Flush Output Buffers           L */
    DevicePhonemeWrite  , /* 12 = Ioctl Write                    M */
    IgnoreCommand       , /* 13 = Device Open DOS 3+             N */
    IgnoreCommand       , /* 14 = Device Close DOS 3+            O */
    IgnoreCommand       , /* 15 = Removable Media                P */
    DevicePhonemeWrite  , /* 16 = Output until Busy              Q */
};
/***************************************************************************
 * ExecPhonemeCommand
 *
 * Description - This function is the central driving function for driver
 *
 * Arguements  - pReqHdr   the DOS request header
 *
 * Returns     - void
 *
 * Globals
 *
 ***************************************************************************/
void ExecPhonemeCommand (REQHDR far *pReqHdr)
{
    if(bInterruptDisable)
    {
        disable();
    }
    if(bTraceOn)
    {
        WinScreenPoke(79, pReqHdr->uchCommandCode, pReqHdr->uchCommandCode+'A');
    }
    pReqHdr->uiStatus = ((pReqHdr->uchCommandCode >= ARRAYSIZE_M(pfnPhonemeDispatch)) ?
                                   UNKNOWN_COMMAND_D :
                                   (*pfnPhonemeDispatch[pReqHdr->uchCommandCode])(pReqHdr));
}
/***************************************************************************
 * PhonemeWrite
 *
 * Description - This function sends a character to phoneme device
 *
 * Arguements  - None
 *
 * Returns     - unsigned
 *
 * Globals     - pReqHdr
 *
 ***************************************************************************/
void PhonemeWrite (const char *pszPhoneme)
{
REQHDR PhonemeReqHdr;

    PhonemeReqHdr.uchCommandCode  = 9;
    for(;*pszPhoneme;pszPhoneme++)
    {
       PhonemeReqHdr.puchTransferBuf = (unsigned char far *)MK_FP(_CS, pszPhoneme);
       PhonemeReqHdr.uiTransferSiz   = 1;
       ExecPhonemeCommand(&PhonemeReqHdr);
    }
}