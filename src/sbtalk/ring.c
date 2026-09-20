#include "ring.h"
#ifdef __WATCOMC__
#include <i86.h>
#define RING_LOCK()   _disable()                 /* the synth runs with interrupts enabled; the ISRs share these words */
#define RING_UNLOCK() _enable()
#else
#define RING_LOCK()
#define RING_UNLOCK()
#endif
unsigned char ring[RING_SIZE];
volatile unsigned long ring_committed = 0, ring_consumed = 0;
volatile unsigned char ring_abort = 0;
volatile unsigned char ring_bench = 0;           /* benchmark: count samples, never wait or store */
unsigned long ring_bench_sum = 0;
/* cheap 16-bit rolling sum: a 32-bit multiply per sample would cost more than SAM itself on a 16-bit CPU */
#define BENCH_SUM(b, n) { unsigned q, h = (unsigned)ring_bench_sum; for (q = 0; q < (n); q++) h = (((h << 1) | (h >> 15)) & 0xFFFFu) ^ (b)[q]; ring_bench_sum = h; }
unsigned char ring_silence = 128;                /* padding value (0 when the ring holds packed bits) */
unsigned char text_ring[TEXT_SIZE];
volatile unsigned text_head = 0, text_tail = 0, text_arrivals = 0;
volatile unsigned char vol_lut[256];
unsigned long ring_written = 0;           /* highest sample position written + 1 */
static unsigned long ring_base = 0;              /* SAM restarts its positions at 0 per utterance */
/* ring_consumed is advanced by the output interrupt; a 16-bit CPU reads a long in two halves, so read it
 * until two reads agree (an interrupt between the halves at a 64K carry would show it 64K ahead) */
static unsigned long consumed_now(void) { unsigned long a; do a = ring_consumed; while (a != ring_consumed); return a; }
void ring_begin_utterance(void) { ring_base = ring_written; }
void ring_abort_ack(void)                        /* synth: a flush arrived; rewind under lock so a write that raced the flush cannot leave stale audio committed */
{
    RING_LOCK();
    ring_committed = ring_written = ring_consumed;
    ring_abort = 0;
    RING_UNLOCK();
}

void ring_write5(unsigned long pos, const unsigned char *ary5)
{
    unsigned k;
    pos += ring_base;
    if (ring_bench) { BENCH_SUM(ary5, 5) ring_written = pos + 5; ring_committed = ring_consumed = pos; return; }
    if (ring_abort) { synth_on_abort(); return; }  /* flushed: abandon this utterance */
    while (pos + 5 - consumed_now() > RING_SIZE) { /* would overwrite unplayed samples */
        synth_yield();
        if (ring_abort) return;
    }
    for (k = 0; k < 5; k++) ring[(unsigned)(pos + k) & RING_MASK] = ary5[k];
    if (pos + 5 > ring_written) ring_written = pos + 5;
    RING_LOCK(); ring_committed = pos; RING_UNLOCK();   /* everything before pos is final now (one atomic 32-bit update for the ISR) */
}
void ring_write_block(const unsigned char *b, unsigned n)
{
    unsigned k;
    if (ring_bench) { BENCH_SUM(b, n) ring_written += n; ring_committed = ring_consumed = ring_written; return; }
    while (n) {
        if (ring_abort) { synth_on_abort(); return; }
        while (ring_written + 1 - consumed_now() > RING_SIZE) { synth_yield(); if (ring_abort) { synth_on_abort(); return; } }
        k = (unsigned)(RING_SIZE - (ring_written - consumed_now())); if (k > n) k = n;
        while (k--) { ring[(unsigned)ring_written & RING_MASK] = *b++; ring_written++; n--; }
        RING_LOCK(); ring_committed = ring_written; RING_UNLOCK();
    }
}
void ring_end_utterance(void)
{
    unsigned k;
    if (ring_bench) return;
    if (ring_abort) return;
    while (ring_written + 400 - consumed_now() > RING_SIZE) { synth_yield(); if (ring_abort) return; }
    for (k = 0; k < 400; k++) ring[(unsigned)(ring_written + k) & RING_MASK] = ring_silence;   /* ~18 ms gap */
    ring_written += 400;
    RING_LOCK(); ring_committed = ring_written; RING_UNLOCK();
}
void ring_flush(void)
{
    ring_abort = 1;
    ring_committed = ring_written = ring_consumed;
}
unsigned ring_pull(unsigned char *dst, unsigned n)
{
    unsigned long avail = ring_committed - ring_consumed;
    unsigned got = avail > n ? n : (unsigned)avail, k;
    for (k = 0; k < got; k++) dst[k] = vol_lut[ring[(unsigned)(ring_consumed + k) & RING_MASK]];
    for (; k < n; k++) dst[k] = 128;
    ring_consumed += got;
    return got;
}
int text_put(unsigned char c)
{
    unsigned nh = (text_head + 1) & (TEXT_SIZE - 1);
    if (nh == text_tail) return 0;
    text_ring[text_head] = c; text_head = nh; text_arrivals++;
    return 1;
}
int text_get(void)
{
    int c;
    if (text_tail == text_head) return -1;
    RING_LOCK();                                  /* text_clear() from the INT 14h hook must not land between the read and the advance */
    if (text_tail == text_head) c = -1; else { c = text_ring[text_tail]; text_tail = (text_tail + 1) & (TEXT_SIZE - 1); }
    RING_UNLOCK();
    return c;
}
void text_clear(void) { text_tail = text_head; }
void set_volume(unsigned v)
{
    unsigned i; int s;
    /* DoubleTalk volume 0..9, default 5: keep the 8-bit resolution usable: 5 -> 80 %, 9 -> 100 %, 0 -> off */
    static const unsigned char pct[10] = {0, 20, 35, 50, 65, 80, 85, 90, 95, 100};
    if (v > 9) v = 9;
    for (i = 0; i < 256; i++) { s = ((int)i - 128) * (int)pct[v] / 100 + 128; vol_lut[i] = (unsigned char)s; }
}
