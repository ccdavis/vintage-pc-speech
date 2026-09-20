/* ASK.EXE - wait up to N seconds (default 10) for a key; exit code = the digit pressed (1-9), 0 otherwise. */
#include <dos.h>
#include <stdlib.h>
/* INT 16h AH=1 reports "key waiting" in ZF, which union REGS does not expose: use the BIOS keyboard buffer head/tail instead */
static int key_waiting(void) { unsigned far *head = (unsigned far *)MK_FP(0x40, 0x1A), far *tail = (unsigned far *)MK_FP(0x40, 0x1C); return *head != *tail; }
static unsigned long ticks(void) { union REGS r; r.h.ah = 0; int86(0x1A, &r, &r); return ((unsigned long)r.x.cx << 16) | r.x.dx; }
int main(int argc, char **argv)
{
    unsigned long t0 = ticks(), lim = (argc > 1 ? (unsigned long)atoi(argv[1]) : 10UL) * 182UL / 10UL;
    union REGS r;
    for (;;) {
        if (key_waiting()) {
            r.h.ah = 0; int86(0x16, &r, &r);
            return (r.h.al >= '1' && r.h.al <= '9') ? r.h.al - '0' : 0;
        }
        if ((ticks() - t0) >= lim) return 0;
    }
}
