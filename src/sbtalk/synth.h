#ifndef SYNTH_H
#define SYNTH_H
/* The synth thread: pulls bytes from the text ring, interprets the DoubleTalk command set, and
 * speaks utterances through the engine into the sample ring. Never returns on DOS. */
void synth_main(void);
/* platform hooks */
void synth_yield(void);            /* wait for the DMA to drain / more text to arrive */
int  synth_idle_ticks(void);       /* ticks (SB IRQs) since the last text byte arrived */
#endif
