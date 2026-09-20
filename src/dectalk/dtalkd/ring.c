/* Copied unchanged from ../../sbtalk/. */
#include "ring.h"
unsigned char ring[RING_SIZE];
volatile unsigned long ring_committed = 0, ring_consumed = 0;
volatile unsigned char ring_abort = 0;
volatile unsigned char ring_bench = 0;           /* benchmark: count samples, never wait or store */
unsigned char text_ring[TEXT_SIZE];
volatile unsigned text_head = 0, text_tail = 0, text_arrivals = 0;
volatile unsigned char vol_lut[256];
unsigned long ring_written = 0;           /* highest sample position written + 1 */
static unsigned long ring_base = 0;              /* SAM restarts its positions at 0 per utterance */
void ring_begin_utterance(void) { ring_base = ring_written; }

void ring_write5(unsigned long pos, const unsigned char *ary5)
{
    unsigned k;
    pos += ring_base;
    if (ring_bench) { ring_written = pos + 5; ring_committed = ring_consumed = pos; return; }
    if (ring_abort) { synth_on_abort(); return; }  /* flushed: abandon this utterance */
    while (pos + 5 - ring_consumed > RING_SIZE) { /* would overwrite unplayed samples */
        synth_yield();
        if (ring_abort) return;
    }
    for (k = 0; k < 5; k++) ring[(unsigned)(pos + k) & RING_MASK] = ary5[k];
    if (pos + 5 > ring_written) ring_written = pos + 5;
    ring_committed = pos;                         /* everything before pos is final now */
}
void ring_write_block(const unsigned char *b, unsigned n)
{
    unsigned k;
    if (ring_bench) { ring_written += n; ring_committed = ring_consumed = ring_written; return; }
    while (n) {
        if (ring_abort) { synth_on_abort(); return; }
        while (ring_written + 1 - ring_consumed > RING_SIZE) { synth_yield(); if (ring_abort) { synth_on_abort(); return; } }
        k = (unsigned)(RING_SIZE - (ring_written - ring_consumed)); if (k > n) k = n;
        while (k--) { ring[(unsigned)ring_written & RING_MASK] = *b++; ring_written++; n--; }
        ring_committed = ring_written;
    }
}
void ring_end_utterance(void)
{
    unsigned k;
    if (ring_bench) return;
    if (ring_abort) return;
    while (ring_written + 400 - ring_consumed > RING_SIZE) { synth_yield(); if (ring_abort) return; }
    for (k = 0; k < 400; k++) ring[(unsigned)(ring_written + k) & RING_MASK] = 128;   /* ~18 ms gap */
    ring_written += 400;
    ring_committed = ring_written;
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
    c = text_ring[text_tail]; text_tail = (text_tail + 1) & (TEXT_SIZE - 1);
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
