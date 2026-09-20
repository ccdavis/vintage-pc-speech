/* SBTALK: resident software speech synthesizer that looks like a serial DoubleTalk/LiteTalk on a COM
 * port (INT 14h) and speaks through a Sound Blaster.  Open Watcom 16-bit, small model.
 * usage: SBTALK [COMn]  (default COM3)   BLASTER=A220 I5 D1 is honoured.
 * Screen readers: Provox -> PV7 LITETALK COM3; JAWS/ASAP/Tinytalk -> DoubleTalk LT/LiteTalk on COM3.
 * No stdio/heap: everything the start-up half prints goes through tprintf() (INT 21h/40h), which keeps
 * the C runtime out of the resident image (about 8 KB of code and 1 KB of data). */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dos.h>
#include <i86.h>
#include <conio.h>
#include "ring.h"
#include "synth.h"
#include "engine.h"

struct ctx { unsigned sp, ss; };
extern void co_switch(struct ctx *save, unsigned new_ss, unsigned new_sp);
#pragma aux co_switch "_*" parm [ax] [dx] [bx] modify [ax bx cx dx es];
extern unsigned _STACKLOW;                              /* Watcom: bottom of the start-up stack = end of _BSS */

static unsigned HALF = 2048u, RATE = 22050u;   /* per engine: 22050/2048 (SAM), 8000/1024 (Klatt) */

static int sb_base = 0x220, sb_irq = 5, sb_dma = 1, dsp_ver = 0;
static unsigned port_index = 2;                        /* COM3 */
static unsigned dma_seg;                                /* DMA buffer: its own DOS block, conventional memory, no 64K crossing */
static unsigned long dma_phys;
static unsigned char __far *dma_buf;
static struct ctx isr_ctx, synth_ctx;
static volatile unsigned char synth_busy = 0;
static volatile unsigned tick = 0, last_arrival_tick = 0;
static unsigned idle_div = 1;                           /* output interrupts per synth slice: 1 (SB), 512 (RTC), 256 (PIT) */
#define SYNTH_STACK 4096
static unsigned char synth_stack[SYNTH_STACK];
static void (__interrupt __far *old_irq)(void);
static void (__interrupt __far *old_int14)(void);
static unsigned irq_vector;
static int test_mode = 0, isr_level = 2, fg_mode = 0, trivial = 0, nosti = 0, bench = 0;
#ifdef NO_SAM
static int engine = ENGINE_RETRO;
#else
static int engine = ENGINE_SAM;
#endif
#ifdef HAVE_RETRO
extern int retro_bitmode; extern unsigned long retro_hi4[256], retro_lo4[16];   /* engine_retro.c: 4 packed samples per nibble */
#endif
static int use_spk = 0, say_mode = 0, ask_mode = 0, use_pit = 0, blocking = 0;   /* blocking: XT speaker bit-bang, no output interrupt */   /* /SPK: PC speaker (RTC); /PIT: speaker via timer 0; /SAY; /ASK */
static unsigned char bda_set = 0;                       /* we made the COM port appear in the BIOS data area */
extern int cpu_class(void);
#pragma aux cpu_class "_*" value [ax] modify [ax];
/* timer-0 speaker path (XT class, no RTC): PIT channel 0 reprogrammed to the speaker rate, the BIOS 18.2 Hz
 * tick chained every 65536 counts so the clock keeps time. Load SBTALK *after* other timer hooks (Provox),
 * so this handler runs first and they still see 18.2 Hz. */
#define PIT_RATE 5512u
static unsigned pit_div, pit_acc;
static void (__interrupt __far *old_int8)(void);
static char say_text[256];
/* PC speaker path: RTC periodic interrupt (IRQ 8, INT 70h) at 8192 Hz, PWM on PIT channel 2 */
#define SPK_RATE 8192u
static unsigned char pwm_lut[256];                           /* sample -> PIT count (1..145) */
static unsigned long spk_pos_q16, spk_step_q16;              /* engine samples per speaker sample */
static unsigned spk_count;
static void (__interrupt __far *old_int70)(void);
static unsigned char rtc_a, rtc_b, port61;
static char bench_text[256]; static volatile unsigned probe = 0; extern volatile unsigned char synth_stage;   /* 0: ack+EOI only, 1: + refill, 2: + synth */
extern volatile unsigned char synth_waiting;
#define SIG_MAGIC 0x5342u                                    /* INT 14h AH=F7h: "are you SBTALK?" -> AX */

/* ---- tiny printf (start-up only): %d %u %x %X %s %c %%, 'l' modifier, '0' flag, width ---- */
static void dos_puts(const char *s, unsigned n);
#pragma aux dos_puts = "mov bx, 1" "mov ah, 40h" "int 21h" parm [dx] [cx] modify [ax bx];
static void tvprintf(const char *fmt, va_list ap)
{
    char buf[200], num[12]; unsigned n = 0;
    for (; *fmt && n < sizeof buf - 1; fmt++) {
        unsigned long v; unsigned width = 0, base, k, neg = 0, lng = 0; char pad = ' ';
        const char *s = num;
        if (*fmt != '%') { buf[n++] = *fmt; continue; }
        fmt++;
        if (*fmt == '0') { pad = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        if (*fmt == 'l') { lng = 1; fmt++; }
        switch (*fmt) {
        case 'c': num[0] = (char)va_arg(ap, int); num[1] = 0; break;
        case 's': s = va_arg(ap, const char *); if (!s) s = "(null)"; break;
        case 'd': case 'u': case 'x': case 'X':
            base = (*fmt == 'x' || *fmt == 'X') ? 16 : 10;
            if (lng) v = va_arg(ap, unsigned long); else if (*fmt == 'd') v = (long)va_arg(ap, int); else v = va_arg(ap, unsigned);
            if (*fmt == 'd' && (long)v < 0) { neg = 1; v = 0UL - v; }
            k = sizeof num - 1; num[k] = 0;
            do { unsigned d = (unsigned)(v % base); num[--k] = (char)(d < 10 ? '0' + d : 'A' + d - 10); v /= base; } while (v && k > 1);
            if (neg) num[--k] = '-';
            s = num + k; break;
        case 0: fmt--; /* fall through */
        default: num[0] = *fmt; num[1] = 0; break;
        }
        for (k = strlen(s); k < width && n < sizeof buf - 1; k++) buf[n++] = pad;
        while (*s && n < sizeof buf - 1) buf[n++] = *s++;
    }
    dos_puts(buf, n);
}
static void tprintf(const char *fmt, ...) { va_list ap; va_start(ap, fmt); tvprintf(fmt, ap); va_end(ap); }
static unsigned char resident = 0;
int printf(const char *fmt, ...)                        /* engines compiled with a stray printf get this one: no stdio in the image, silent once resident */
{
    va_list ap;
    if (resident) return 0;
    va_start(ap, fmt); tvprintf(fmt, ap); va_end(ap);
    return 0;
}
static int ieq(const char *a, const char *b)            /* case-insensitive equality (ASCII) */
{
    for (;; a++, b++) {
        unsigned char x = *a, y = *b;
        if (x >= 'a' && x <= 'z') x -= 32;
        if (y >= 'a' && y <= 'z') y -= 32;
        if (x != y) return 0;
        if (!x) return 1;
    }
}

/* ---- DSP ---- */
static int dsp_write(int v) { int t; for (t = 0; t < 30000; t++) if (!(inp(sb_base + 0xC) & 0x80)) { outp(sb_base + 0xC, v); return 0; } return -1; }
static int dsp_read(void)   { int t; for (t = 0; t < 30000; t++) if (inp(sb_base + 0xE) & 0x80) return inp(sb_base + 0xA); return -1; }
static int dsp_reset(void)  { int i; outp(sb_base + 6, 1); for (i = 0; i < 20; i++) inp(sb_base + 0xE); outp(sb_base + 6, 0); return dsp_read() == 0xAA ? 0 : -1; }

/* ---- DMA (8237, 8-bit channel) ---- */
static const int dma_addr[4] = {0, 2, 4, 6}, dma_cnt[4] = {1, 3, 5, 7}, dma_page[4] = {0x87, 0x83, 0x81, 0x82};
static void dma_start(void)
{
    outp(0x0A, 4 | sb_dma);
    outp(0x0C, 0);
    outp(0x0B, 0x58 | sb_dma);                          /* auto-init, increment, read (mem -> device) */
    outp(dma_addr[sb_dma], (int)(dma_phys & 0xFF)); outp(dma_addr[sb_dma], (int)((dma_phys >> 8) & 0xFF));
    outp(dma_page[sb_dma], (int)((dma_phys >> 16) & 0xFF));
    outp(0x0C, 0);
    outp(dma_cnt[sb_dma], (2 * HALF - 1) & 0xFF); outp(dma_cnt[sb_dma], (2 * HALF - 1) >> 8);
    outp(0x0A, sb_dma);
}
static unsigned dma_pos(void)                          /* current offset within the buffer */
{
    unsigned lo, hi;
    outp(0x0C, 0); lo = inp(dma_addr[sb_dma]); hi = inp(dma_addr[sb_dma]);
    return (unsigned)((((unsigned long)hi << 8 | lo) - (dma_phys & 0xFFFF)) & 0xFFFF);
}
static void dsp_start(void)
{
    dsp_write(0xD1);                                    /* speaker on (pre-SB16 needs it) */
    if (dsp_ver >= 0x400) {
        dsp_write(0x41); dsp_write(RATE >> 8); dsp_write(RATE & 0xFF);
        dsp_write(0xC6); dsp_write(0x00);               /* 8-bit auto-init output, mono unsigned */
        dsp_write((HALF - 1) & 0xFF); dsp_write((HALF - 1) >> 8);
    } else {
        dsp_write(0x40); dsp_write(256 - (int)(1000000UL / RATE));   /* time constant */
        dsp_write(0x48); dsp_write((HALF - 1) & 0xFF); dsp_write((HALF - 1) >> 8);
        dsp_write(RATE > 23000 ? 0x90 : 0x1C);          /* 8-bit auto-init DMA output; high-speed above 23 kHz (DSP 2.01+) */
    }
}
static void dsp_stop(void)
{
    dsp_write(0xD0); dsp_write(0xDA); dsp_reset();      /* pause, exit auto-init, reset (high-speed mode only obeys the reset) */
    outp(0x0A, 4 | sb_dma);                             /* mask the DMA channel */
}
/* DOS memory allocation strategy / UMB link (INT 21h 58xxh); errors (DOS < 5) are ignored */
static unsigned dos58(unsigned ax, unsigned bx);
#pragma aux dos58 = "int 21h" parm [ax] [bx] value [ax] modify [bx];
static int dma_alloc(void)
{   /* the buffer must be in conventional memory even when SBTALK itself is loaded high (LH): the 8237 sees
     * physical addresses, and UMBs under a 386 memory manager are remapped pages */
    unsigned paras = 2 * HALF / 16, seg, seg2, strat, link; int ok;
    strat = dos58(0x5800, 0); link = dos58(0x5802, 0) & 0xFF;
    dos58(0x5801, 0); dos58(0x5803, 0);                 /* first fit low, UMBs unlinked */
    ok = _dos_allocmem(paras, &seg) == 0;
    if (ok && ((((unsigned long)seg << 4) & 0xFFFF) + 2 * HALF > 0x10000UL)) {   /* crosses a 64K page: take the next block */
        ok = _dos_allocmem(paras, &seg2) == 0;          /* first fit lands right after seg, which ends past the boundary */
        _dos_freemem(seg); seg = seg2;
    }
    dos58(0x5801, strat); dos58(0x5803, link);
    if (!ok) return -1;
    dma_seg = seg; dma_phys = (unsigned long)seg << 4; dma_buf = MK_FP(seg, 0);
    return 0;
}

/* ---- the Sound Blaster interrupt: refill the half that just finished, then let the synth run ---- */
static void fill_half(unsigned half)
{
    unsigned char __far *d = dma_buf + half * HALF;
    unsigned long avail = ring_committed - ring_consumed;
    unsigned got, k;
    if (avail > RING_SIZE) avail = 0;                   /* torn read of ring_committed (16-bit CPU): play silence this half */
#ifdef HAVE_RETRO
    if (retro_bitmode) {                                /* ring holds packed bits: 8 samples per byte, cheap enough for an 8088 */
        unsigned want = HALF / 8; unsigned idx = (unsigned)ring_consumed;
        got = avail > want ? want : (unsigned)avail;
        for (k = 0; k < got; k++, idx++) {
            unsigned char b = ring[idx & RING_MASK];
            *(unsigned long __far *)d = retro_hi4[b]; *(unsigned long __far *)(d + 4) = retro_lo4[b & 15]; d += 8;
        }
        for (k = got * 8; k < HALF; k++) *d++ = 64;
        ring_consumed += got;
        return;
    }
#endif
    got = avail > HALF ? HALF : (unsigned)avail;
    for (k = 0; k < got; k++) d[k] = vol_lut[ring[(unsigned)(ring_consumed + k) & RING_MASK]];
    for (; k < HALF; k++) d[k] = 128;
    ring_consumed += got;
}
static void __interrupt __far sb_isr(void)
{
    unsigned pos;
    if (dsp_ver >= 0x400) { outp(sb_base + 4, 0x82); if (!(inp(sb_base + 5) & 1)) { /* not ours */ } }
    inp(sb_base + 0xE);                                 /* acknowledge 8-bit DMA transfer */
    tick++;
    if (isr_level >= 1) { pos = dma_pos(); fill_half(pos >= HALF ? 0 : 1); }   /* refill the half not being played */
    if (sb_irq >= 8) outp(0xA0, 0x20);
    outp(0x20, 0x20);
    if (isr_level >= 2 && !synth_busy) {
        synth_busy = 1;
        if (!nosti) _enable();
        co_switch(&isr_ctx, synth_ctx.ss, synth_ctx.sp);
        _disable();
        synth_busy = 0;
    }
}
/* the synth thread gives the CPU back until the next SB interrupt */
static void spk_bits_drain(void);
void synth_yield(void) { if (blocking) spk_bits_drain(); co_switch(&synth_ctx, isr_ctx.ss, isr_ctx.sp); }
int  synth_idle_ticks(void) { return blocking ? 2 : (int)((tick - last_arrival_tick) / idle_div); }
extern void synth_restart_after_flush(void);
void synth_on_abort(void) { synth_restart_after_flush(); }
static void synth_entry(void) { if (trivial) for (;;) { probe++; synth_yield(); } synth_main(); for (;;) synth_yield(); }

/* ---- PC speaker: one sample per RTC tick ---- */
static void __interrupt __far spk_isr(void)
{
    unsigned char smp = 128;
    if (ring_committed > ring_consumed) {                      /* ring position tracks the engine rate */
        smp = vol_lut[ring[(unsigned)ring_consumed & RING_MASK]];
        spk_pos_q16 += spk_step_q16;
        while (spk_pos_q16 >= 0x10000UL && ring_committed > ring_consumed) { spk_pos_q16 -= 0x10000UL; ring_consumed++; }
    }
    outp(0x42, pwm_lut[smp]);                                  /* reload channel 2: pulse width = sample */
    outp(0x70, 0x0C); inp(0x71);                               /* RTC register C: acknowledge periodic interrupt */
    tick++;
    outp(0xA0, 0x20); outp(0x20, 0x20);
    if (++spk_count >= 512) {
        spk_count = 0;
        if (isr_level >= 2 && !synth_busy) {
            synth_busy = 1;
            if (!nosti) _enable();
            co_switch(&isr_ctx, synth_ctx.ss, synth_ctx.sp);
            _disable();
            synth_busy = 0;
        }
    }
}
static void __interrupt __far pit_isr(void)
{
    unsigned char smp = 128;
    if (ring_committed > ring_consumed) {
        smp = vol_lut[ring[(unsigned)ring_consumed & RING_MASK]];
        spk_pos_q16 += spk_step_q16;
        while (spk_pos_q16 >= 0x10000UL && ring_committed > ring_consumed) { spk_pos_q16 -= 0x10000UL; ring_consumed++; }
    }
    outp(0x42, pwm_lut[smp]);
    tick++;
    pit_acc += pit_div;
    if (pit_acc < pit_div) old_int8();                          /* every 65536 counts (18.2 Hz): the BIOS tick, which sends the EOI */
    else outp(0x20, 0x20);                                      /* exactly one EOI per interrupt either way */
    if (++spk_count >= 256) {
        spk_count = 0;
        if (isr_level >= 2 && !synth_busy) {
            synth_busy = 1;
            _enable();
            co_switch(&isr_ctx, synth_ctx.ss, synth_ctx.sp);
            _disable();
            synth_busy = 0;
        }
    }
}
static void pit_start(void)
{
    unsigned i; unsigned long period = 1193182UL / PIT_RATE;
    for (i = 0; i < 256; i++) pwm_lut[i] = (unsigned char)(1 + (unsigned long)i * (period - 2) / 255);
    spk_step_q16 = ((unsigned long)RATE << 16) / PIT_RATE; spk_pos_q16 = 0; spk_count = 0;
    pit_div = (unsigned)period; pit_acc = 0; idle_div = 256;
    _disable();
    port61 = inp(0x61);
    outp(0x43, 0xB0); outp(0x42, pwm_lut[128]);
    outp(0x61, port61 | 3);
    old_int8 = _dos_getvect(0x08); _dos_setvect(0x08, pit_isr);
    outp(0x43, 0x34); outp(0x40, pit_div & 0xFF); outp(0x40, pit_div >> 8);   /* channel 0, mode 2, rate generator */
    _enable();
}
static void pit_stop(void)
{
    _disable();
    outp(0x43, 0x36); outp(0x40, 0); outp(0x40, 0);                          /* back to the BIOS's mode 3, 18.2 Hz */
    _dos_setvect(0x08, old_int8);
    outp(0x61, port61 & ~2);
    _enable();
}
static void spk_start(void)
{
    unsigned i; unsigned long period = 1193182UL / SPK_RATE;   /* 145 PIT ticks per sample */
    for (i = 0; i < 256; i++) pwm_lut[i] = (unsigned char)(1 + (unsigned long)i * (period - 2) / 255);
    spk_step_q16 = ((unsigned long)RATE << 16) / SPK_RATE; spk_pos_q16 = 0; spk_count = 0; idle_div = 512;
    _disable();
    port61 = inp(0x61);
    outp(0x43, 0xB0);                                          /* channel 2, lobyte only, mode 0 */
    outp(0x42, pwm_lut[128]);
    outp(0x61, port61 | 3);                                    /* gate + speaker data on */
    old_int70 = _dos_getvect(0x70); _dos_setvect(0x70, spk_isr);
    outp(0x70, 0x8A); rtc_a = inp(0x71); outp(0x70, 0x8A); outp(0x71, (rtc_a & 0xF0) | 0x03);   /* rate 3 = 8192 Hz */
    outp(0x70, 0x8B); rtc_b = inp(0x71); outp(0x70, 0x8B); outp(0x71, rtc_b | 0x40);            /* periodic interrupt enable */
    outp(0x70, 0x0C); inp(0x71);
    outp(0xA1, inp(0xA1) & ~1); outp(0x21, inp(0x21) & ~4);    /* unmask IRQ 8 and the cascade */
    _enable();
}
static void spk_stop(void)
{
    _disable();
    outp(0x70, 0x8B); outp(0x71, rtc_b); outp(0x70, 0x8A); outp(0x71, rtc_a);
    outp(0x70, 0x0C); inp(0x71);                               /* last access with bit 7 clear: leave NMI enabled */
    _dos_setvect(0x70, old_int70);
    outp(0x61, port61 & ~2);
    _enable();
}

/* ---- XT speaker, blocking: the 1983 voice's bits straight out of port 61h at 33 kbit/s, paced by a
 * calibrated LOOP delay (SPEECH.COM's method, but measured against the PIT at start-up so it is
 * right on any CPU). No interrupt could keep up on an 8088, so this speaks inside the caller's INT 14h
 * call, as SPEECH.COM did. ---- */
extern void spk_bit8(unsigned char b, unsigned delay, unsigned onoff);
#pragma aux spk_bit8 "_*" parm [al] [dx] [bx] modify [ax];
extern void cpu_loops(unsigned n);
#pragma aux cpu_loops "_*" parm [cx] modify [cx];
static unsigned bit_delay = 1, force_delay = 0;
/* bit time = fixed part (~3.5 LOOP-equivalents of shift/out/branch) + delay * t_loop; target 30.17 us.
 * Pure CPU measurement, no port I/O, so emulators cannot skew it. */
static unsigned pit_read(void) { unsigned v; outp(0x43, 0x00); v = inp(0x40); v |= inp(0x40) << 8; return v; }
static void spk_calibrate(void)
{   /* time 5000 LOOP iterations with the PIT channel 0 counter (1.193182 MHz). The BIOS runs channel 0 in
     * mode 3, where the count moves by 2 per clock, so switch it to mode 2 for the measurement (the tick
     * that was in progress restarts: one 18.2 Hz tick arrives up to 55 ms late, once). */
    unsigned t0, t1, el; unsigned long t_loop_ns, d;
    _disable();
    outp(0x43, 0x34); outp(0x40, 0); outp(0x40, 0);
    t0 = pit_read(); cpu_loops(5000); t1 = pit_read();
    outp(0x43, 0x36); outp(0x40, 0); outp(0x40, 0);
    _enable();
    el = (t0 - t1) & 0xFFFFu; if (el < 6) el = 6;                    /* 8088: ~21000 ticks; 386: ~2000 */
    t_loop_ns = ((unsigned long)el * 16762UL + 50000UL) / 100000UL;   /* 838.095 ns per tick / 5000 loops (no overflow) */
    if (t_loop_ns < 1) t_loop_ns = 1;
    d = 30170UL > 3 * t_loop_ns ? (30170UL - 3 * t_loop_ns) / t_loop_ns : 1;   /* round(30170/t - 3.5) */
    bit_delay = d < 1 ? 1 : d > 65535UL ? 65535u : (unsigned)d;
    if (force_delay) bit_delay = force_delay;
    tprintf("speaker timing: 5000 LOOPs = %u PIT ticks, %lu ns each, delay %u per bit\n", el, t_loop_ns, bit_delay);
}
static void spk_bits_drain(void)
{
    unsigned onoff = ((unsigned)(port61 & 0xFC) << 8) | ((port61 & 0xFC) | 2);   /* bh = off, bl = on */
    while (ring_committed > ring_consumed) {
        spk_bit8(ring[(unsigned)ring_consumed & RING_MASK], bit_delay, onoff);
        ring_consumed++;
    }
    outp(0x61, port61 & 0xFC);
}
static void spk_bits_start(void)
{
    port61 = inp(0x61) & 0xFC;                                  /* gate low: timer 2 output stays high, bit 1 drives the cone */
    outp(0x43, 0xB6); outp(0x42, 0); outp(0x42, 0);
    outp(0x61, port61);
    spk_calibrate();
}
static void spk_bits_stop(void) { outp(0x61, port61); }
static void resume_synth_now(void)
{
    if (!synth_busy) { synth_busy = 1; co_switch(&isr_ctx, synth_ctx.ss, synth_ctx.sp); synth_busy = 0; }
}
/* blocking mode: run the synth until it has spoken everything queued (each resume plays at most one
 * ring-full, about a second, before the synth yields), bounded so a stuck engine cannot hang the caller */
static void resume_synth_blocking(void)
{
    int r;
    if (synth_busy) return;
    for (r = 0; r < 64; r++) { resume_synth_now(); if (synth_waiting && text_head == text_tail) break; }
}

/* ---- INT 14h: the virtual serial port ---- */
static void __interrupt __far int14_isr(union INTPACK r)
{
    if (r.x.dx != port_index) { _chain_intr(old_int14); return; }
    switch (r.h.ah) {
    case 0: case 3: r.h.ah = 0x60; r.h.al = 0xB0; break;      /* THRE+TEMT; CTS+DSR+DCD */
    case 1:
        if (r.h.al == 24) { text_clear(); ring_flush(); }     /* ^X: stop speaking now */
        text_put(r.h.al);
        last_arrival_tick = tick;
        if (blocking && (r.h.al == '\r' || r.h.al == '.' || r.h.al == '!' || r.h.al == '?')) { _enable(); resume_synth_blocking(); }
        r.h.ah = 0x60; break;
    case 2: r.h.ah = 0x80; break;                              /* nothing to receive */
    case 0xF7: r.x.ax = SIG_MAGIC; break;                      /* identification for a second SBTALK */
    default: r.h.ah = 0x60; break;
    }
}
static unsigned int14_probe(unsigned dx);
#pragma aux int14_probe = "mov ax, 0F700h" "int 14h" parm [dx] value [ax] modify [ax bx cx dx];
static void int14_send(unsigned dx, unsigned char c);
#pragma aux int14_send = "mov ah, 1" "int 14h" parm [dx] [al] modify [ax bx cx dx];
static int resident_port(void)                                 /* port index of a resident SBTALK, or -1 */
{
    unsigned p;
    for (p = 0; p < 4; p++) if (int14_probe(p) == SIG_MAGIC) return (int)p;
    return -1;
}

/* ---- setup ---- */
static unsigned parse_num(const char __far *p, unsigned base)
{
    unsigned v = 0;
    for (;; p++) {
        unsigned d;
        if (*p >= '0' && *p <= '9') d = *p - '0';
        else if (base == 16 && ((*p | 32) >= 'a' && (*p | 32) <= 'f')) d = (*p | 32) - 'a' + 10;
        else return v;
        v = v * base + d;
    }
}
static void parse_blaster(void)
{   /* the environment block: "NAME=value\0"... "\0" at PSP:2Ch */
    const char __far *e = MK_FP(*(unsigned __far *)MK_FP(_psp, 0x2C), 0); const char __far *p;
    while (*e) {
        if (e[0] == 'B' && e[1] == 'L' && e[2] == 'A' && e[3] == 'S' && e[4] == 'T' && e[5] == 'E' && e[6] == 'R' && e[7] == '=') {
            for (p = e + 8; *p; p++) {
                if (*p == 'A') sb_base = (int)parse_num(p + 1, 16);
                else if (*p == 'I') sb_irq = (int)parse_num(p + 1, 10);
                else if (*p == 'D') sb_dma = (int)parse_num(p + 1, 10);
            }
            return;
        }
        while (*e) e++;
        e++;
    }
}
static void teardown(void)                                     /* /SAY and /TEST: put everything back */
{
    _disable();
    if (blocking) spk_bits_stop(); else if (use_pit) pit_stop(); else if (use_spk) spk_stop(); else { dsp_stop(); _dos_setvect(irq_vector, old_irq); }
    _dos_setvect(0x14, old_int14);
    _enable();
    if (bda_set) *(unsigned __far *)MK_FP(0x40, port_index * 2) = 0;
    if (dma_seg) { _dos_freemem(dma_seg); dma_seg = 0; }
}
static void stack_paint(void) { unsigned k; for (k = 0; k < SYNTH_STACK - 8; k++) synth_stack[k] = 0xA5; }
static unsigned stack_high_water(void) { unsigned k; for (k = 0; k < SYNTH_STACK && synth_stack[k] == 0xA5; k++) ; return SYNTH_STACK - k; }
static const char *engine_name(void) { return engine == ENGINE_KLATT ? "Klatt" : engine == ENGINE_RETRO ? "1983" : "SAM"; }
static int split_args(char **argv, int max)             /* PSP command tail -> argv (main(void) keeps Watcom's argv builder and malloc out) */
{
    static char tail[128]; unsigned char __far *p = MK_FP(_psp, 0x80); unsigned n = *p, i; int argc = 1; char *q = tail;
    if (n > 126) n = 126;
    for (i = 0; i < n; i++) tail[i] = p[1 + i];
    tail[n] = 0;
    argv[0] = "SBTALK";
    while (*q && argc < max - 1) {
        while (*q == ' ' || *q == '\t' || *q == '\r') q++;
        if (!*q) break;
        argv[argc++] = q;
        while (*q && *q != ' ' && *q != '\t' && *q != '\r') q++;
        if (*q) *q++ = 0;
    }
    argv[argc] = 0;
    return argc;
}
int main(void)
{
    unsigned paras, k, *stk; int i, res, argc; char *argv[32];
    const char *hello = "\x01" "50P\x01" "5S\x01" "5V S B talk ready.\r";
    argc = split_args(argv, 32);
    for (i = 1; i < argc; i++) {
        if ((argv[i][0] | 32) == 'c' && (argv[i][1] | 32) == 'o' && (argv[i][2] | 32) == 'm' && argv[i][3] >= '1' && argv[i][3] <= '4' && !argv[i][4]) port_index = argv[i][3] - '1';
        else if (ieq(argv[i], "/TEST")) test_mode = 1;
        else if (ieq(argv[i], "/SPK")) use_spk = 1;
        else if (ieq(argv[i], "/ASK")) ask_mode = 1;
        else if (ieq(argv[i], "/SAY")) say_mode = 1;
        else if (say_mode) { if (strlen(say_text) + strlen(argv[i]) < 250) { strcat(say_text, argv[i]); strcat(say_text, " "); } }
        else if (ieq(argv[i], "/ISR0")) isr_level = 0;
        else if (ieq(argv[i], "/TRIVIAL")) trivial = 1;
        else if (ieq(argv[i], "/NOSTI")) nosti = 1;
        else if (ieq(argv[i], "/BENCH")) bench = 1;
        else if (bench) { if (strlen(bench_text) + strlen(argv[i]) < 250) { strcat(bench_text, argv[i]); strcat(bench_text, " "); } }
#ifdef HAVE_KLATT
        else if (ieq(argv[i], "/KLATT")) engine = ENGINE_KLATT;
#endif
#ifdef HAVE_RETRO
        else if (ieq(argv[i], "/RETRO")) engine = ENGINE_RETRO;
        else if (ieq(argv[i], "/BITS")) { retro_bitmode = 1; ring_silence = 0; }
#endif
#ifndef NO_SAM
        else if (ieq(argv[i], "/SAM")) engine = ENGINE_SAM;
#endif
        else if (ieq(argv[i], "/PIT")) { use_spk = 1; use_pit = 1; }
        else if (ieq(argv[i], "/CPU")) { int c = cpu_class(); tprintf("CPU class %d (0=8086/88, 2=286, 3=386+)\n", c); return c; }
        else if (ieq(argv[i], "/FG")) { fg_mode = 1; isr_level = 1; test_mode = 1; }
        else if (ieq(argv[i], "/ISR1")) isr_level = 1;
        else if (ieq(argv[i], "/DELAY") && i + 1 < argc) force_delay = (unsigned)parse_num(argv[++i], 10);   /* diag: override the calibrated bit delay */
        else { tprintf("usage: SBTALK [COM1..COM4] [/KLATT|/RETRO [/BITS]] [/SPK|/PIT] [/TEST] [/SAY text] [/ASK] [/CPU]\n"); return 1; }
    }
    if (ask_mode) {                                    /* wait for a key, return it as the errorlevel (digits -> 1..9) */
        int c = getch(); if (!c) c = getch();
        if (c >= '0' && c <= '9') return c - '0';
        return c & 0x7F;
    }
#ifdef HAVE_RETRO
    if (retro_bitmode && engine != ENGINE_RETRO) { tprintf("SBTALK: /BITS needs /RETRO\n"); return 1; }
#endif
    parse_blaster();
    if (sb_dma < 0 || sb_dma > 3 || sb_irq < 0 || sb_irq > 15) { tprintf("SBTALK: BLASTER must name an 8-bit DMA channel (D0..D3) and IRQ 0..15\n"); return 1; }
    engine_init(engine); RATE = engine_rate(); HALF = RATE > 16000 ? 2048u : 1024u;
    if (use_spk && RATE > 23000) { blocking = 1; use_pit = 0; }   /* XT speaker: blocking bit-bang */
    if (bench) {                                       /* foreground synthesis speed, no sound card needed */
        static char text[256] = "Hello. This is the speech synthesizer speaking on Free DOS. One two three four five six seven.";
        volatile unsigned long __far *bios = MK_FP(0x40, 0x6C); unsigned long t0, t1, ms, audio_ms;
        if (bench_text[0]) strcpy(text, bench_text);
        ring_bench = 1; engine_set_params(5, 50);
        t0 = *bios; engine_speak(text, (unsigned)strlen(text)); t1 = *bios;
        ms = (t1 - t0) * 55; audio_ms = ring_written * 1000UL / RATE;
        tprintf("bench %s: %lu ms of audio in %lu ms = %lu.%02lu x real time\n", engine_name(),
               audio_ms, ms, ms ? audio_ms / ms : 0, ms ? (audio_ms * 100 / ms) % 100 : 0);
        return 0;
    }
    res = resident_port();
    if (res >= 0 && say_mode) {                        /* a resident SBTALK owns the card: speak through it */
        int14_send(res, 24);
        for (i = 0; say_text[i]; i++) int14_send(res, (unsigned char)say_text[i]);
        int14_send(res, '\r');
        return 0;
    }
    if (res >= 0) { tprintf("SBTALK: already resident on COM%d\n", res + 1); return 1; }
    if (!use_spk) {
        if (dsp_reset()) { tprintf("SBTALK: no Sound Blaster DSP at %Xh\n", sb_base); return 1; }
        dsp_write(0xE1); dsp_ver = dsp_read() << 8; dsp_ver |= dsp_read();
        if (dma_alloc()) { tprintf("SBTALK: no memory for the DMA buffer\n"); return 1; }
        for (k = 0; k < 2 * HALF; k++) dma_buf[k] = 128;
    }
    set_volume(5);
    /* synth thread stack: pop di/si/bp then "ret" into synth_entry */
    stack_paint();
    stk = (unsigned *)(synth_stack + sizeof(synth_stack) - 8);
    stk[0] = stk[1] = stk[2] = 0; stk[3] = (unsigned)synth_entry;
    synth_ctx.ss = FP_SEG((void __far *)synth_stack); synth_ctx.sp = FP_OFF((void __far *)stk);
    /* BIOS data area: make the COM port look present */
    { unsigned __far *bda = MK_FP(0x40, port_index * 2); if (*bda == 0) { *bda = 0x3E8; bda_set = 1; } }
    irq_vector = sb_irq < 8 ? 8 + sb_irq : 0x70 + sb_irq - 8;
    _disable();
    old_int14 = _dos_getvect(0x14);    _dos_setvect(0x14, int14_isr);
    if (!use_spk) {
        old_irq = _dos_getvect(irq_vector); _dos_setvect(irq_vector, sb_isr);
        if (sb_irq < 8) outp(0x21, inp(0x21) & ~(1 << sb_irq)); else { outp(0xA1, inp(0xA1) & ~(1 << (sb_irq - 8))); outp(0x21, inp(0x21) & ~4); }
    }
    _enable();
    if (blocking) spk_bits_start(); else if (use_pit) pit_start(); else if (use_spk) spk_start(); else { dma_start(); dsp_start(); }
    if (say_mode) { text_put(1); text_put('7'); text_put('S'); for (i = 0; say_text[i]; i++) text_put((unsigned char)say_text[i]); text_put('\r'); }
    else for (i = 0; hello[i]; i++) text_put((unsigned char)hello[i]);
    last_arrival_tick = tick;
    if (say_mode) {                                    /* speak once in the foreground, then uninstall */
        volatile unsigned long __far *bios = MK_FP(0x40, 0x6C); unsigned long t0 = *bios;
        if (blocking) resume_synth_blocking();
        else while (*bios - t0 < 18UL * 60) { if (synth_waiting && text_head == text_tail && ring_committed <= ring_consumed && tick > 40) break; }
        teardown();
        tprintf("said %lu samples, %u ticks\n", ring_consumed, tick);
        return 0;
    }
    if (blocking) tprintf("SBTALK 0.3: PC speaker, 1983 voice bit-banged at 33 kbit/s (blocking, delay %u), emulating a serial synthesizer on COM%u\n", bit_delay, port_index + 1);
    else if (use_pit) tprintf("SBTALK 0.3: PC speaker (timer 0, %u Hz PWM), %s engine at %u Hz, emulating a serial synthesizer on COM%u\n", PIT_RATE, engine_name(), RATE, port_index + 1);
    else if (use_spk) tprintf("SBTALK 0.3: PC speaker (RTC %u Hz PWM), %s engine at %u Hz, emulating a serial synthesizer on COM%u\n", SPK_RATE, engine_name(), RATE, port_index + 1);
    else tprintf("SBTALK 0.3: DSP %d.%02d at %Xh IRQ %d DMA %d, %s engine at %u Hz, emulating a serial synthesizer on COM%u\n",
           dsp_ver >> 8, dsp_ver & 0xFF, sb_base, sb_irq, sb_dma, engine_name(), RATE, port_index + 1);
    if (test_mode) {                                   /* foreground diagnostics for ~6 s, then uninstall */
        volatile unsigned long __far *bios = MK_FP(0x40, 0x6C); unsigned long t0 = *bios, last = 0; int sent = 0;
        while (*bios - t0 < 110) {
            if (fg_mode) {                                 /* drive the synth thread from here instead of the ISR */
                static unsigned n; unsigned long c0 = ring_committed;
                synth_busy = 1; co_switch(&isr_ctx, synth_ctx.ss, synth_ctx.sp); synth_busy = 0;
                if (++n % 4 == 0 || ring_committed != c0) tprintf("fg resume %u: stage %u probe %u committed %lu consumed %lu text %u/%u\n", n, synth_stage, probe, ring_committed, ring_consumed, text_head, text_tail);
                if (n > 200) break;
            }
            if (*bios - last >= 18) {
                unsigned irr, isr;
                last = *bios;
                outp(0x20, 0x0A); irr = inp(0x20); outp(0x20, 0x0B); isr = inp(0x20);
                outp(sb_base + 4, 0x82);
                tprintf("t=%lu stage=%u probe=%u ticks=%u dmapos=%u imr=%02X irr=%02X isr=%02X sbint=%02X committed=%lu consumed=%lu text=%u/%u busy=%u\n",
                       *bios - t0, synth_stage, probe, tick, use_spk ? 0 : dma_pos(), inp(0x21), irr, isr, inp(sb_base + 5), ring_committed, ring_consumed, text_head, text_tail, synth_busy);
                if (!sent && *bios - t0 > 30) { sent = 1; text_put('T'); text_put('e'); text_put('s'); text_put('t'); text_put('\r'); last_arrival_tick = tick; }
            }
        }
        teardown();
        tprintf("synth stack high-water %u of %u bytes; DGROUP %04X, BSS end %04X\n", stack_high_water(), SYNTH_STACK, FP_SEG((void __far *)&paras), _STACKLOW);
        if (!use_spk) tprintf("dma buffer at %04X:0000 phys %lX\n", dma_seg, dma_phys);
        return 0;
    }
    /* resident: PSP .. end of _BSS. The start-up stack (last in DGROUP) and the environment block go;
     * the handlers run on the interrupted stack and the synth on its own, and nothing reads getenv again. */
    if (blocking) resume_synth_blocking();             /* no output interrupt to speak the greeting: do it now */
    { unsigned env = *(unsigned __far *)MK_FP(_psp, 0x2C); if (env) { _dos_freemem(env); *(unsigned __far *)MK_FP(_psp, 0x2C) = 0; } }
    paras = (FP_SEG((void __far *)&paras) + (_STACKLOW + 15u) / 16u) - _psp;
    resident = 1;
    _dos_keep(0, paras);
    return 0;
}
