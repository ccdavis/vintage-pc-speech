#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pc.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/nearptr.h>
#include "sb16.h"

int sb_verbose = 0;
static int base, irq, dma8;
static int dosseg = 0, dossel = 0;      /* DMA buffer in conventional memory */
static unsigned long dosphys = 0;
#define BUFSZ 16384UL

static void iodelay(void) { int i; for (i = 0; i < 4; i++) inportb(base + 0xE); }

static int dsp_write(int v) {
    long t;
    for (t = 0; t < 100000; t++) if (!(inportb(base + 0xC) & 0x80)) { outportb(base + 0xC, v); return 0; }
    return -1;
}
static int dsp_read(void) {
    long t;
    for (t = 0; t < 100000; t++) if (inportb(base + 0xE) & 0x80) return inportb(base + 0xA);
    return -1;
}
static int dsp_reset(void) {
    outportb(base + 0x6, 1); iodelay(); iodelay(); outportb(base + 0x6, 0);
    return dsp_read() == 0xAA ? 0 : -1;
}

int sb_init(int b, int i, int d) {
    int ver;
    base = b; irq = i; dma8 = d;
    if (dsp_reset()) { if (sb_verbose) printf("sb16: no DSP at %x\n", base); return 0; }
    dsp_write(0xE1); ver = dsp_read() << 8; ver |= dsp_read();
    /* 2 buffers of DOS memory so we can pick one block that does not cross a 64K physical page */
    dosseg = __dpmi_allocate_dos_memory(2 * BUFSZ / 16, &dossel);
    if (dosseg < 0) { printf("sb16: no DOS memory (dpmi error %x)\n", __dpmi_error); return 0; }
    dosphys = (unsigned long)dosseg << 4;
    if ((dosphys & 0xFFFF) + BUFSZ > 0x10000) dosphys = (dosphys + 0x10000) & ~0xFFFFUL; /* skip to the next page */
    dsp_write(0xD1);                      /* speaker on (needed on pre-SB16) */
    if (sb_verbose) printf("sb16: DSP v%d.%02d at %x irq %d dma %d, buffer phys %lx\n", ver >> 8, ver & 0xFF, base, irq, dma8, dosphys);
    return ver;
}

void sb_shutdown(void) {
    if (dossel) { dsp_write(0xD3); __dpmi_free_dos_memory(dossel); dossel = 0; }
}

/* program the 8-bit DMA controller for a single-cycle memory->device transfer */
static void dma_setup(unsigned long phys, unsigned len) {
    static const int addrport[4] = {0x00, 0x02, 0x04, 0x06}, cntport[4] = {0x01, 0x03, 0x05, 0x07};
    static const int pageport[4] = {0x87, 0x83, 0x81, 0x82};
    outportb(0x0A, 0x04 | dma8);               /* mask */
    outportb(0x0C, 0);                         /* clear flip-flop */
    outportb(0x0B, 0x48 | dma8);               /* single cycle, address increment, read (mem->dev) */
    outportb(addrport[dma8], phys & 0xFF); outportb(addrport[dma8], (phys >> 8) & 0xFF);
    outportb(pageport[dma8], (phys >> 16) & 0xFF);
    outportb(0x0C, 0);
    outportb(cntport[dma8], (len - 1) & 0xFF); outportb(cntport[dma8], ((len - 1) >> 8) & 0xFF);
    outportb(0x0A, dma8);                      /* unmask */
}
static unsigned dma_count(void) {
    static const int cntport[4] = {0x01, 0x03, 0x05, 0x07};
    unsigned lo, hi;
    outportb(0x0C, 0); lo = inportb(cntport[dma8]); hi = inportb(cntport[dma8]);
    return lo | (hi << 8);
}

void sb_play8(const unsigned char *pcm, unsigned long len, unsigned rate) {
    unsigned char picmask = inportb(0x21);
    if (irq < 8) outportb(0x21, picmask | (1 << irq));    /* we poll; keep the BIOS from seeing the IRQ */
    dsp_write(0x41); dsp_write(rate >> 8); dsp_write(rate & 0xFF);   /* SB16 output rate */
    while (len) {
        unsigned n = len > BUFSZ ? BUFSZ : (unsigned)len;
        uclock_t t0, limit;
        dosmemput(pcm, n, dosphys);
        dma_setup(dosphys, n);
        dsp_write(0xC0); dsp_write(0x00);            /* 8-bit output, single cycle; mono unsigned */
        dsp_write((n - 1) & 0xFF); dsp_write((n - 1) >> 8);
        t0 = uclock(); limit = (uclock_t)n * UCLOCKS_PER_SEC / rate + UCLOCKS_PER_SEC / 4;
        for (;;) {                                    /* done when the DMA count wraps or time is up */
            unsigned c = dma_count();
            if (c == 0xFFFF) break;
            if (uclock() - t0 > limit) break;
        }
        inportb(base + 0xE);                          /* ack 8-bit DMA interrupt */
        pcm += n; len -= n;
    }
    if (irq < 8) outportb(0x21, picmask);
}
