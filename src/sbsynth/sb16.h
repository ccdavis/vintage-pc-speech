/* Minimal Sound Blaster 16 (and SB Pro/2.0-compatible) 8-bit mono PCM output for DJGPP.
 * Single-cycle DMA blocks, completion by polling; no IRQ handler (IRQ masked at the PIC while playing).
 * Good enough for feasibility tests; a production synth wants auto-init DMA + IRQ double-buffering. */
#ifndef SB16_H
#define SB16_H
int  sb_init(int base, int irq, int dma8);      /* returns DSP version (major<<8|minor) or 0 */
void sb_shutdown(void);
void sb_play8(const unsigned char *pcm, unsigned long len, unsigned rate); /* blocking, unsigned 8-bit mono */
extern int sb_verbose;
#endif
