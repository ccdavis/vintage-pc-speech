/* Sample ring and text ring shared between the synth thread, the INT 14h hook and the SB IRQ. */
#ifndef RING_H
#define RING_H
#define RING_BITS 12
#define RING_SIZE (1u << RING_BITS)             /* 4096 samples, > 2 DMA halves */
#define RING_MASK (RING_SIZE - 1)
#define TEXT_SIZE 4096
extern unsigned char ring[RING_SIZE];
extern volatile unsigned long ring_committed;    /* samples < this are final */
extern volatile unsigned long ring_consumed;     /* samples < this were sent to the DMA buffer */
extern volatile unsigned char ring_abort;        /* set by flush: synth must drop its utterance */
extern volatile unsigned char ring_bench;
extern unsigned long ring_bench_sum;              /* bench: checksum of every sample the engine produced (host vs DOS parity) */
extern unsigned long ring_written;
extern unsigned char ring_silence;
extern unsigned char text_ring[TEXT_SIZE];
extern volatile unsigned text_head, text_tail;   /* head: producer (INT 14h), tail: consumer (synth) */
extern volatile unsigned text_arrivals;          /* bumped on every byte, lets the synth notice idleness */
extern volatile unsigned char vol_lut[256];      /* output volume mapping applied when copying to DMA */
void ring_write5(unsigned long pos, const unsigned char *ary5);   /* called by SAM's renderer */
void ring_begin_utterance(void);                 /* call before each engine run */
void ring_write_block(const unsigned char *b, unsigned n);  /* append n samples (Klatt path) */
void ring_end_utterance(void);                   /* commit the look-ahead samples, pad silence */
void ring_flush(void);                           /* drop everything queued (ISR-safe) */
void ring_abort_ack(void);                       /* synth side: acknowledge a flush (rewinds under lock, clears ring_abort) */
unsigned ring_pull(unsigned char *dst, unsigned n);  /* ISR: copy up to n committed samples, pad with 128 */
int  text_put(unsigned char c);                  /* producer side, 0 if full */
int  text_get(void);                             /* consumer side, -1 if empty */
void text_clear(void);
void set_volume(unsigned v);                     /* 0..9 */
/* provided by the platform: give the CPU back until the DMA has drained some samples */
void synth_yield(void);
void synth_on_abort(void);       /* platform: longjmp back to the utterance loop (no-op on host) */
#endif
