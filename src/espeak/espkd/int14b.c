/* INT14B.EXE: cost of the virtual serial port.  Times N INT 14h status calls (AH=3, answered by
 * the real-mode stub unless ESPKD /PMALL) and N "send byte 0" calls (AH=1, each one a DPMI
 * real-mode callback) on COM3, in BIOS ticks.  Open Watcom 16-bit, real mode. */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
static unsigned long ticks(void) { return *(volatile unsigned long __far *)MK_FP(0x40, 0x6C); }
int main(int argc, char **argv)
{
    unsigned long n = argc > 1 ? atol(argv[1]) : 5000UL, i, t0, t1, t2;
    union REGS r;
    t0 = ticks();
    for (i = 0; i < n; i++) { r.h.ah = 3; r.x.dx = 2; int86(0x14, &r, &r); }
    t1 = ticks();
    for (i = 0; i < n; i++) { r.h.ah = 1; r.h.al = 0; r.x.dx = 2; int86(0x14, &r, &r); }
    t2 = ticks();
    printf("INT14B %lu calls: status %lu ticks = %lu us/call, send %lu ticks = %lu us/call\n", n,
           t1 - t0, (t1 - t0) * 55000UL / n, t2 - t1, (t2 - t1) * 55000UL / n);
    return 0;
}
