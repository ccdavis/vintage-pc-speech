/* LOOPS.EXE: real-mode CPU benchmark for the child shell.  LOOPS [n]: runs n iterations of an
 * integer loop and prints the elapsed BIOS ticks; run it idle and while ESPKD is speaking,
 * the ratio is the CPU share taken by the resident synthesizer (Open Watcom 16-bit). */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
int main(int argc, char **argv)
{
    unsigned long n = argc > 1 ? atol(argv[1]) : 2000000UL, i, t0, t1;
    volatile unsigned long x = 0;
    volatile unsigned long __far *bios = MK_FP(0x40, 0x6C);
    t0 = *bios;
    for (i = 0; i < n; i++) x += i ^ (x >> 3);
    t1 = *bios;
    printf("LOOPS %lu: %lu ticks = %lu ms\n", n, t1 - t0, (t1 - t0) * 55);
    return 0;
}
